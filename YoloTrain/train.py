"""YOLO11 segmentation 학습 → ONNX export → 서버 backend로 검증.

라벨 원본은 data/ellipses(타원)이다. 학습 전에 data/labels를 타원 둘레 다각형(YOLO seg 형식)으로 다시 만든다.

실행 (YoloTrain/.venv):
  .venv\\Scripts\\python train.py                       # 검수 완료 영상만 사용 (data/reviewed.txt)
  .venv\\Scripts\\python train.py --include-unreviewed  # 초안 라벨까지 사용 (파이프라인 시험용)

출력:
  runs/<name>/weights/best.pt, best.onnx
  ../YoloServer/models/weld.onnx (--deploy 시)
  runs/<name>/server_eval.txt (서버 OnnxYoloDetector로 검증셋 평가: 검출 + 타원 W/H/각도 오차)

주의: Ultralytics는 AGPL-3.0이다. 현장 배포 전 라이선스(Enterprise 등) 확인이 필요하다.
"""
from __future__ import annotations

import argparse
import random
import re
import shutil
import sys
from pathlib import Path

import ellipse_labels

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"
SERVER_DIR = HERE.parent / "YoloServer"
CLASS_NAMES = ["welding", "spatter"]


def sample_key(name: str) -> str:
    """같은 시료(바코드)의 영상은 같은 split에 둔다. 예: borg1014_after_2026_10_01_DLM0ESA0A26072614132_0_0"""
    m = re.match(r"(.+)_\d_\d$", name)
    key = m.group(1) if m else name
    return re.sub(r"_(after|before)_", "_", key)


def build_split(include_unreviewed: bool, val_ratio: float, seed: int) -> Path:
    names = sorted(p.stem for p in (DATA / "images").glob("*.png"))
    if not include_unreviewed:
        reviewed = set((DATA / "reviewed.txt").read_text(encoding="utf-8").split()) if (DATA / "reviewed.txt").exists() else set()
        names = [n for n in names if n in reviewed]
    if not names:
        raise SystemExit("학습할 영상이 없습니다. review_labels.py로 검수하거나 --include-unreviewed를 쓰세요.")

    keys = sorted({sample_key(n) for n in names})
    random.Random(seed).shuffle(keys)
    val_keys = set(keys[: max(1, int(len(keys) * val_ratio))])

    split_dir = DATA / "split"
    split_dir.mkdir(exist_ok=True)
    train = [str((DATA / "images" / f"{n}.png").resolve()) for n in names if sample_key(n) not in val_keys]
    val = [str((DATA / "images" / f"{n}.png").resolve()) for n in names if sample_key(n) in val_keys]
    (split_dir / "train.txt").write_text("\n".join(train) + "\n", encoding="utf-8")
    (split_dir / "val.txt").write_text("\n".join(val) + "\n", encoding="utf-8")

    yaml_path = split_dir / "weld.yaml"
    yaml_path.write_text(
        f"path: {DATA.resolve().as_posix()}\n"
        f"train: {(split_dir / 'train.txt').resolve().as_posix()}\n"
        f"val: {(split_dir / 'val.txt').resolve().as_posix()}\n"
        "names:\n" + "".join(f"  {i}: {n}\n" for i, n in enumerate(CLASS_NAMES)),
        encoding="utf-8")
    print(f"train {len(train)} / val {len(val)} images ({len(keys) - len(val_keys)} / {len(val_keys)} samples)")
    return yaml_path


def write_seg_labels():
    """data/ellipses → data/labels (YOLO seg 다각형). 타원 라벨이 없는 영상은 빈 라벨(배경)이 된다."""
    from PIL import Image

    labels = DATA / "labels"
    labels.mkdir(exist_ok=True)
    count = 0
    for img_path in sorted((DATA / "images").glob("*.png")):
        w, h = Image.open(img_path).size
        ellipses = ellipse_labels.load(DATA / "ellipses" / f"{img_path.stem}.txt")
        lines = [ellipse_labels.to_yolo_seg_line(e, w, h) for e in ellipses]
        (labels / f"{img_path.stem}.txt").write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
        count += len(lines)
    for cache in DATA.glob("*.cache"):
        cache.unlink()
    print(f"seg labels: {count} polygons from data/ellipses")


def iou(a, b):
    ix = max(0.0, min(a[2], b[2]) - max(a[0], b[0]))
    iy = max(0.0, min(a[3], b[3]) - max(a[1], b[1]))
    inter = ix * iy
    union = (a[2] - a[0]) * (a[3] - a[1]) + (b[2] - b[0]) * (b[3] - b[1]) - inter
    return inter / union if union > 0 else 0.0


def evaluate_with_server_backend(onnx_path: Path, image_size: int, val_list: Path, conf: float, out_path: Path):
    """서버와 같은 OnnxYoloDetector로 검증셋을 돌려 클래스별 정밀도·재현율(외접 박스 IoU 0.5)과
    맞은 검출의 타원 W/H(픽셀)·각도 오차를 낸다."""
    sys.path.insert(0, str(SERVER_DIR))
    import cv2
    import numpy as np
    from detectors import OnnxYoloDetector

    det = OnnxYoloDetector(str(onnx_path), CLASS_NAMES, image_size, conf, 0.45)
    stats = {c: {"tp": 0, "fp": 0, "fn": 0, "dw": [], "dh": [], "da": [], "no_ellipse": 0} for c in CLASS_NAMES}
    lines = []
    for img_path in [Path(p) for p in val_list.read_text(encoding="utf-8").split("\n") if p]:
        img = cv2.imdecode(np.fromfile(str(img_path), np.uint8), cv2.IMREAD_GRAYSCALE)
        gts = ellipse_labels.load(DATA / "ellipses" / f"{img_path.stem}.txt")
        preds = det.detect(img)
        for ci, cname in enumerate(CLASS_NAMES):
            g = [e for e in gts if e[0] == ci]
            g_box = [ellipse_labels.bounding_box(*e[1:]) for e in g]
            p = sorted([d for d in preds if d.class_name == cname], key=lambda d: -d.confidence)
            s = stats[cname]
            used = set()
            for d in p:
                best, bi = 0.0, -1
                for k, gb in enumerate(g_box):
                    if k not in used and iou(d.box, gb) > best:
                        best, bi = iou(d.box, gb), k
                if best >= 0.5:
                    used.add(bi); s["tp"] += 1
                    if d.ellipse is None:
                        s["no_ellipse"] += 1
                        lines.append(f"NO-ELLIPSE {cname} {img_path.stem}")
                        continue
                    _, gcx, gcy, gw, gh, ga = g[bi]
                    dw, dh, da = d.ellipse[2] - gw, d.ellipse[3] - gh, d.ellipse[4] - ga
                    s["dw"].append(dw); s["dh"].append(dh); s["da"].append(da)
                    if abs(dw) > 10 or abs(dh) > 10 or abs(da) > 3:
                        lines.append(f"SIZE {cname} {img_path.stem} gt W{gw:.0f} H{gh:.0f} {ga:+.1f} / pred W{d.ellipse[2]:.0f} "
                                     f"H{d.ellipse[3]:.0f} {d.ellipse[4]:+.1f}")
                else:
                    s["fp"] += 1
                    lines.append(f"FP {cname} {img_path.stem} conf={d.confidence:.2f} box={[round(v) for v in d.box]}")
            s["fn"] += len(g) - len(used)
            for k, gb in enumerate(g_box):
                if k not in used:
                    lines.append(f"FN {cname} {img_path.stem} box={[round(v) for v in gb]}")

    def err(v):
        a = np.abs(np.array(v)) if v else np.array([np.nan])
        return f"mean {np.mean(v) if v else float('nan'):+.2f} |mean| {a.mean():.2f} max {a.max():.2f}"

    report = [f"engine={det.engine} conf>={conf} IoU>=0.5 (ellipse error: pred - gt, pixels / degrees)"]
    for c, s in stats.items():
        prec = s["tp"] / (s["tp"] + s["fp"]) if s["tp"] + s["fp"] else float("nan")
        rec = s["tp"] / (s["tp"] + s["fn"]) if s["tp"] + s["fn"] else float("nan")
        report.append(f"{c}: TP {s['tp']} FP {s['fp']} FN {s['fn']} precision {prec:.3f} recall {rec:.3f} no-ellipse {s['no_ellipse']}")
        if s["dw"]:
            report.append(f"  W {err(s['dw'])} | H {err(s['dh'])} | angle {err(s['da'])}")
    text = "\n".join(report + [""] + lines) + "\n"
    out_path.write_text(text, encoding="utf-8")
    print("\n".join(report))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", default="yolo11n-seg.pt", help="사전학습 가중치 (yolo11n-seg.pt / yolo11s-seg.pt)")
    parser.add_argument("--imgsz", type=int, default=1024)
    parser.add_argument("--epochs", type=int, default=100)
    parser.add_argument("--batch", type=int, default=4)
    parser.add_argument("--workers", type=int, default=0, help="DataLoader 프로세스 수 (Windows는 프로세스마다 torch를 다시 올려 메모리 부족이 날 수 있어 기본 0)")
    parser.add_argument("--name", default="weld")
    parser.add_argument("--val-ratio", type=float, default=0.2)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--include-unreviewed", action="store_true")
    parser.add_argument("--eval-conf", type=float, default=0.5)
    parser.add_argument("--deploy", action="store_true", help="ONNX를 ../YoloServer/models/weld.onnx로 복사")
    args = parser.parse_args()

    from ultralytics import YOLO

    write_seg_labels()
    yaml_path = build_split(args.include_unreviewed, args.val_ratio, args.seed)
    model = YOLO(args.model)
    model.train(data=str(yaml_path), imgsz=args.imgsz, epochs=args.epochs, batch=args.batch,
                project=str(HERE / "runs"), name=args.name, exist_ok=True, seed=args.seed,
                workers=args.workers, patience=30,
                # 흑백 산업 영상: 색 증강은 끄고, 기하·밝기 증강 위주
                hsv_h=0.0, hsv_s=0.0, hsv_v=0.3, fliplr=0.5, flipud=0.0, degrees=3.0,
                translate=0.1, scale=0.3, mosaic=1.0)

    run_dir = HERE / "runs" / args.name
    best = YOLO(str(run_dir / "weights" / "best.pt"))
    onnx_path = Path(best.export(format="onnx", imgsz=args.imgsz, opset=12, simplify=True, dynamic=False))
    print(f"exported {onnx_path}")

    evaluate_with_server_backend(onnx_path, args.imgsz, DATA / "split" / "val.txt", args.eval_conf,
                                 run_dir / "server_eval.txt")

    if args.deploy:
        dst = SERVER_DIR / "models" / "weld.onnx"
        dst.parent.mkdir(exist_ok=True)
        shutil.copy2(onnx_path, dst)
        print(f"deployed {dst}")


if __name__ == "__main__":
    main()
