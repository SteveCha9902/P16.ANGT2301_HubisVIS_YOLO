"""박스 라벨(data/labels, YOLO detect 형식)을 타원 라벨(data/ellipses)로 옮긴다.

- 박스에 내접하는 기울기 0 타원을 만든다. 검수 도구에서 모양·기울기를 고친다.
- 처음 실행할 때 data/labels를 data/labels_box로 백업한다. 이후 data/labels는 train.py가 타원에서 다시 만든다.
- 새 영상(make_draft_labels.py로 추가, 백업에 없음)은 data/labels의 박스 초안을 읽는다.
- 이미 타원 라벨이 있는 영상은 건드리지 않는다(--overwrite로 강제).

실행: python convert_box_labels.py
"""
from __future__ import annotations

import argparse
import shutil
from pathlib import Path

from PIL import Image

import ellipse_labels

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args()

    labels, backup, out_dir = DATA / "labels", DATA / "labels_box", DATA / "ellipses"
    if not backup.exists():
        shutil.copytree(labels, backup)
        print(f"backup {labels} -> {backup}")

    made = skipped = boxes = 0
    for img_path in sorted((DATA / "images").glob("*.png")):
        dst = out_dir / f"{img_path.stem}.txt"
        if dst.exists() and not args.overwrite:
            skipped += 1
            continue
        w, h = Image.open(img_path).size
        ellipses = []
        # 처음 변환한 영상은 백업에서, 나중에 make_draft_labels.py로 추가한 영상은 data/labels의 박스 초안에서 읽는다
        # (data/labels의 seg 다각형 줄은 필드 수가 달라 아래에서 건너뛴다)
        src = backup / f"{img_path.stem}.txt"
        if not src.exists():
            src = labels / f"{img_path.stem}.txt"
        if src.exists():
            for line in src.read_text(encoding="utf-8").split("\n"):
                p = line.split()
                if len(p) != 5:
                    continue
                c, bx, by, bw, bh = int(p[0]), *map(float, p[1:])
                ellipses.append([c, bx * w, by * h, bw * w, bh * h, 0.0])
        ellipse_labels.save(dst, ellipses)
        made += 1
        boxes += len(ellipses)
    print(f"ellipse labels: {made} written ({boxes} ellipses), {skipped} kept")


if __name__ == "__main__":
    main()
