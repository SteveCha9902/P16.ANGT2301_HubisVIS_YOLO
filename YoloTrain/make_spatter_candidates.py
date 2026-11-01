"""spatter 후보 자동 검출 — 라벨이 아니라 검수 도구에서 사람이 확정할 후보를 만든다.

대상: 밝은 금속판 위의 작은 어두운 원형 점(비드 밖). 비드 타원에 붙은 덩어리는 제외한다.
방법: black-hat(어두운 작은 구조 강조) → 임계값 → 크기·원형도·판 내부·비드와의 거리로 거른다.

실행: python make_spatter_candidates.py            (After 영상 전부, 기존 후보 덮어씀)
      python make_spatter_candidates.py --preview  (몇 장의 확인용 그림만 만든다)
출력: data/spatter_candidates/<이름>.txt  "1 cx cy d d 0" (ellipse_labels 형식, 원)
"""
from __future__ import annotations

import argparse
import math
from pathlib import Path

import cv2
import numpy as np

import ellipse_labels as el

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"
OUT = DATA / "spatter_candidates"

BLACKHAT_KERNEL = 31      # 이보다 작은 어두운 구조를 강조 (px)
# 사람이 확정하는 후보이므로 놓치지 않는 쪽(느슨한 기준)으로 둔다. 오탐은 검수에서 버린다.
BLACKHAT_THRESH = 22      # black-hat 응답 임계값
AREA_MIN, AREA_MAX = 8, 700    # 후보 면적 (px²) ≈ 지름 3~30 px
CIRCULARITY_MIN = 0.45    # 4πA/P²
PLATE_BRIGHT = 90         # 금속판으로 볼 밝기 (블러 후)
PLATE_MARGIN = 12         # 판 가장자리(버·그림자) 제외 폭 (px)
BEAD_GAP = 4              # 비드 타원에서 이만큼 떨어져야 spatter (붙은 덩어리 제외)


def bead_mask(shape, ellipses, grow):
    m = np.zeros(shape, np.uint8)
    for c, cx, cy, w, h, a in ellipses:
        if c == 0:
            cv2.ellipse(m, ((cx, cy), (w + 2 * grow, h + 2 * grow), a), 255, -1)
    return m


def detect(gray: np.ndarray, ellipses) -> list[tuple[float, float, float]]:
    blur = cv2.GaussianBlur(gray, (0, 0), 15)
    plate = (blur > PLATE_BRIGHT).astype(np.uint8) * 255
    plate = cv2.erode(plate, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * PLATE_MARGIN + 1,) * 2))
    plate[bead_mask(gray.shape, ellipses, BEAD_GAP) > 0] = 0

    k = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (BLACKHAT_KERNEL, BLACKHAT_KERNEL))
    bh = cv2.morphologyEx(gray, cv2.MORPH_BLACKHAT, k)
    binary = ((bh > BLACKHAT_THRESH) & (plate > 0)).astype(np.uint8)

    out = []
    contours, _ = cv2.findContours(binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
    for c in contours:
        area = cv2.contourArea(c)
        if not AREA_MIN <= area <= AREA_MAX:
            continue
        perim = cv2.arcLength(c, True)
        if perim <= 0 or 4 * math.pi * area / perim ** 2 < CIRCULARITY_MIN:
            continue
        (cx, cy), r = cv2.minEnclosingCircle(c)
        out.append((cx, cy, 2 * r + 4))  # 경계 여유 2 px
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview", action="store_true")
    args = parser.parse_args()

    names = sorted(p.stem for p in (DATA / "images").glob("*_after_*.png"))
    OUT.mkdir(exist_ok=True)
    total, tiles = 0, []
    for i, n in enumerate(names):
        gray = cv2.imdecode(np.fromfile(str(DATA / "images" / f"{n}.png"), np.uint8), cv2.IMREAD_GRAYSCALE)
        ellipses = el.load(DATA / "ellipses" / f"{n}.txt")
        cands = detect(gray, ellipses)
        total += len(cands)
        if args.preview:
            if i % 8 == 0 and len(tiles) < 6:
                vis = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
                for c, cx, cy, w, h, a in ellipses:
                    cv2.ellipse(vis, ((cx, cy), (w, h), a), (0, 0, 255), 3)
                for cx, cy, d in cands:
                    cv2.circle(vis, (int(cx), int(cy)), int(d / 2) + 6, (0, 255, 255), 3)
                ys = [e[2] for e in ellipses if e[0] == 0] or [1150]
                y0 = int(max(0, min(ys) - 350))
                tiles.append(cv2.resize(vis[y0:y0 + 700, :], (1224, 350)))
            continue
        el.save(OUT / f"{n}.txt", [[1, cx, cy, d, d, 0.0] for cx, cy, d in cands])
    if args.preview:
        cv2.imwrite(str(HERE / "spatter_preview.png"), np.vstack(tiles))
    print(f"{len(names)} After images, {total} candidates" + (" (preview only)" if args.preview else f" → {OUT}"))


if __name__ == "__main__":
    main()
