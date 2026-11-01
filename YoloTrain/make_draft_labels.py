"""현장 영상으로 데이터셋을 만들고 welding 라벨 초안을 템플릿 매칭으로 생성한다.

- After(용접 후) 영상: 카메라별 비드 템플릿으로 매칭해 YOLO 라벨 초안을 쓴다 (cam0 최대 3개, cam1 최대 1개).
- Before(용접 전) 영상: 비드가 없으므로 빈 라벨(배경 예시)로 넣는다.
- 초안은 반드시 review_labels.py로 사람이 검수한다. 매칭 점수가 낮은 영상은 review_list.txt 앞쪽에 둔다.

실행: python make_draft_labels.py [--before-step 3]
출력: data/images/*.png, data/labels/*.txt (class cx cy w h, 0~1 정규화), data/draft_scores.csv, data/review_list.txt
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import os
import re
from pathlib import Path

import cv2
import numpy as np

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"

# 영상 출처 (읽기 전용으로 복사만 한다)
SOURCES = [
    ("borg1014", Path(r"D:\02.Works\04.Codex\P15.ANGT2301_HubisVIS_Line\26.1014_borg")),
    ("hubisdata", Path(r"D:\Hubis Data")),
]

CLASS_WELDING = 0
CLASS_NAMES = ["welding", "spatter"]

# 기준 영상(borg1014 After 2026/10/01/DLM0ESA0A26072614132)에서 직접 확인한 비드 박스 (x1, y1, x2, y2)
TEMPLATE_IMAGE = "After/2026/10/01/DLM0ESA0A26072614132"
TEMPLATES = {
    0: ("0_0_raw.bmp", (1100, 1080, 1420, 1222), 3),   # cam0: 한 화면에 비드 3개
    1: ("1_0_raw.bmp", (1172, 975, 1595, 1265), 1),    # cam1: 비드 1개
}
SCALE = 0.25                     # 매칭은 1/4 영상에서 한다
TEMPLATE_SCALES = (0.85, 0.92, 1.0, 1.08, 1.15)
MIN_SCORE = 0.30                 # 이 점수 미만 후보는 버린다
REVIEW_SCORE = 0.55              # 이 점수 미만 박스가 있으면 우선 검수 대상


def prep(gray: np.ndarray) -> np.ndarray:
    small = cv2.resize(gray, None, fx=SCALE, fy=SCALE, interpolation=cv2.INTER_AREA)
    small = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8, 8)).apply(small)
    return cv2.GaussianBlur(small, (3, 3), 0)


def load_templates(root: Path) -> dict:
    out = {}
    for cam, (fname, box, count) in TEMPLATES.items():
        gray = cv2.imread(str(root / TEMPLATE_IMAGE / fname), cv2.IMREAD_GRAYSCALE)
        small = prep(gray)
        x1, y1, x2, y2 = [int(v * SCALE) for v in box]
        out[cam] = (small[y1:y2, x1:x2].copy(), count)
    return out


def iou(a, b) -> float:
    ix1, iy1, ix2, iy2 = max(a[0], b[0]), max(a[1], b[1]), min(a[2], b[2]), min(a[3], b[3])
    inter = max(0, ix2 - ix1) * max(0, iy2 - iy1)
    union = (a[2] - a[0]) * (a[3] - a[1]) + (b[2] - b[0]) * (b[3] - b[1]) - inter
    return inter / union if union > 0 else 0.0


def match(gray: np.ndarray, template: np.ndarray, max_count: int):
    small = prep(gray)
    candidates = []
    for s in TEMPLATE_SCALES:
        t = cv2.resize(template, None, fx=s, fy=s, interpolation=cv2.INTER_LINEAR)
        if t.shape[0] >= small.shape[0] or t.shape[1] >= small.shape[1]:
            continue
        res = cv2.matchTemplate(small, t, cv2.TM_CCOEFF_NORMED)
        th, tw = t.shape
        # 점수 상위 위치를 뽑아 후보로 (최대값 주변을 지우며 반복)
        res = res.copy()
        for _ in range(max_count * 4):
            _, score, _, (x, y) = cv2.minMaxLoc(res)
            if score < MIN_SCORE:
                break
            candidates.append((float(score), (x / SCALE, y / SCALE, (x + tw) / SCALE, (y + th) / SCALE)))
            cv2.rectangle(res, (max(0, x - tw // 2), max(0, y - th // 2)), (x + tw // 2, y + th // 2), -1.0, -1)

    candidates.sort(key=lambda c: -c[0])
    picked = []
    for score, box in candidates:
        if any(iou(box, p[1]) > 0.0 for p in picked):
            continue  # 비드끼리는 겹치지 않는다
        if picked:
            # 여러 비드는 한 줄로 놓인다: 최고점 후보와 세로 중심이 박스 높이의 40% 이내
            best = picked[0][1]
            if abs((box[1] + box[3]) / 2 - (best[1] + best[3]) / 2) > 0.4 * (best[3] - best[1]):
                continue
        picked.append((score, box))
        if len(picked) == max_count:
            break
    return picked


def collect_images(before_step: int):
    """(출처, 상대경로, 'After'|'Before') 목록. 내용이 같은 파일은 한 번만 넣는다."""
    seen = set()
    items = []
    for tag, root in SOURCES:
        if not root.exists():
            print(f"skip missing source {root}")
            continue
        for kind in ("After", "Before"):
            files = sorted((root / kind).rglob("*_raw.bmp"))
            if kind == "Before":
                files = files[::before_step]
            for f in files:
                digest = hashlib.md5(f.read_bytes()).hexdigest()
                if digest in seen:
                    continue
                seen.add(digest)
                items.append((tag, root, f, kind))
    return items


def output_name(tag: str, root: Path, path: Path, kind: str) -> str:
    rel = path.relative_to(root / kind)
    parts = [p for p in rel.parts[:-1]]  # 날짜/바코드
    stem = path.stem.replace("_raw", "")
    name = "_".join([tag, kind.lower()] + parts + [stem])
    return re.sub(r"[^0-9A-Za-z_.-]", "_", name)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--before-step", type=int, default=3, help="Before 영상은 n장마다 1장 사용")
    parser.add_argument("--overwrite", action="store_true", help="이미 검수한 라벨도 덮어쓴다")
    args = parser.parse_args()

    (DATA / "images").mkdir(parents=True, exist_ok=True)
    (DATA / "labels").mkdir(parents=True, exist_ok=True)
    templates = load_templates(SOURCES[0][1])

    rows = []
    for tag, root, path, kind in collect_images(args.before_step):
        name = output_name(tag, root, path, kind)
        gray = cv2.imread(str(path), cv2.IMREAD_GRAYSCALE)
        if gray is None:
            print(f"read fail {path}")
            continue
        h, w = gray.shape
        img_out = DATA / "images" / f"{name}.png"
        if not img_out.exists():
            cv2.imwrite(str(img_out), gray)

        label_out = DATA / "labels" / f"{name}.txt"
        if label_out.exists() and not args.overwrite:
            rows.append((name, kind, "kept", ""))
            continue

        lines, scores = [], []
        if kind == "After":
            cam = int(path.name.split("_")[0])
            template, count = templates[cam]
            for score, (x1, y1, x2, y2) in match(gray, template, count):
                cx, cy = (x1 + x2) / 2 / w, (y1 + y2) / 2 / h
                bw, bh = (x2 - x1) / w, (y2 - y1) / h
                lines.append(f"{CLASS_WELDING} {cx:.6f} {cy:.6f} {bw:.6f} {bh:.6f}")
                scores.append(score)
        label_out.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
        rows.append((name, kind, len(lines), " ".join(f"{s:.2f}" for s in scores)))

    with open(DATA / "draft_scores.csv", "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["image", "kind", "boxes", "scores"])
        writer.writerows(rows)

    # 검수 순서: 점수 낮은 After → 나머지 After → Before
    def priority(r):
        if r[1] != "After":
            return (2, 0.0)
        s = [float(v) for v in str(r[3]).split()] or [0.0]
        return (0 if min(s) < REVIEW_SCORE else 1, min(s))
    with open(DATA / "review_list.txt", "w", encoding="utf-8") as f:
        for r in sorted(rows, key=priority):
            f.write(r[0] + "\n")

    with open(DATA / "classes.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(CLASS_NAMES) + "\n")

    after = [r for r in rows if r[1] == "After"]
    print(f"images {len(rows)} (After {len(after)}, Before {len(rows) - len(after)})")


if __name__ == "__main__":
    main()
