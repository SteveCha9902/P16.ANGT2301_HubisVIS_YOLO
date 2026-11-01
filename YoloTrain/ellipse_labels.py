"""타원 라벨 형식과 변환.

라벨 원본: data/ellipses/<이름>.txt, 한 줄에 타원 하나
  class cx cy w h angle      (픽셀, angle은 도)
  - w: 축 방향 (cos angle, sin angle)의 전체 길이, h: 그 수직 축의 전체 길이 (영상 좌표, y 아래 방향)
  - angle은 (-45, 45]로 정규화한다. 그래서 w는 항상 영상 가로축에 가까운 축(=가로), h는 세로다.
    HubisVIS 레시피의 fWeldWidth / fWeldHeight와 같은 의미가 되도록 정한 규칙이다.

학습용 data/labels/<이름>.txt는 train.py가 타원 둘레 다각형(YOLO segmentation 형식)으로 만든다.
"""
from __future__ import annotations

import math
from pathlib import Path

POLYGON_POINTS = 36  # 학습용 다각형 꼭짓점 수


def normalize(cx: float, cy: float, w: float, h: float, angle: float):
    """angle을 (-45, 45]로 옮기고 필요하면 w/h를 바꾼다. 같은 타원을 한 가지로만 표현하기 위함."""
    w, h = abs(w), abs(h)
    angle = math.fmod(angle, 180.0)
    if angle <= -90.0:
        angle += 180.0
    elif angle > 90.0:
        angle -= 180.0
    if angle > 45.0:
        angle -= 90.0
        w, h = h, w
    elif angle <= -45.0:
        angle += 90.0
        w, h = h, w
    return cx, cy, w, h, angle


def load(path: Path) -> list[list[float]]:
    """[[class, cx, cy, w, h, angle], ...]"""
    out = []
    if path.exists():
        for line in path.read_text(encoding="utf-8").split("\n"):
            p = line.split()
            if len(p) == 6:
                out.append([int(p[0]), *normalize(*map(float, p[1:]))])
    return out


def save(path: Path, ellipses, min_size: float = 2.0):
    lines = []
    for c, cx, cy, w, h, a in ellipses:
        cx, cy, w, h, a = normalize(cx, cy, w, h, a)
        if w < min_size or h < min_size:
            continue
        lines.append(f"{int(c)} {cx:.2f} {cy:.2f} {w:.2f} {h:.2f} {a:.3f}")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")


def axes(angle: float):
    """(u, v): w 방향 단위벡터, h 방향 단위벡터 (영상 좌표)"""
    t = math.radians(angle)
    return (math.cos(t), math.sin(t)), (-math.sin(t), math.cos(t))


def polygon(cx, cy, w, h, angle, n: int = POLYGON_POINTS):
    (ux, uy), (vx, vy) = axes(angle)
    pts = []
    for k in range(n):
        t = 2 * math.pi * k / n
        a, b = w / 2 * math.cos(t), h / 2 * math.sin(t)
        pts.append((cx + a * ux + b * vx, cy + a * uy + b * vy))
    return pts


def corners(cx, cy, w, h, angle):
    """타원을 감싸는 회전 박스의 네 꼭짓점"""
    (ux, uy), (vx, vy) = axes(angle)
    out = []
    for su, sv in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        out.append((cx + su * w / 2 * ux + sv * h / 2 * vx, cy + su * w / 2 * uy + sv * h / 2 * vy))
    return out


def bounding_box(cx, cy, w, h, angle):
    """타원의 축 정렬 외접 박스 (x1, y1, x2, y2)"""
    t = math.radians(angle)
    ex = math.sqrt((w / 2 * math.cos(t)) ** 2 + (h / 2 * math.sin(t)) ** 2)
    ey = math.sqrt((w / 2 * math.sin(t)) ** 2 + (h / 2 * math.cos(t)) ** 2)
    return cx - ex, cy - ey, cx + ex, cy + ey


def contains(ellipse, x, y, margin: float = 0.0) -> bool:
    _, cx, cy, w, h, a = ellipse
    (ux, uy), (vx, vy) = axes(a)
    dx, dy = x - cx, y - cy
    pu, pv = dx * ux + dy * uy, dx * vx + dy * vy
    ra, rb = w / 2 + margin, h / 2 + margin
    if ra <= 0 or rb <= 0:
        return False
    return (pu / ra) ** 2 + (pv / rb) ** 2 <= 1.0


def to_yolo_seg_line(ellipse, img_w: int, img_h: int) -> str:
    c, cx, cy, w, h, a = ellipse
    coords = []
    for x, y in polygon(cx, cy, w, h, a):
        coords.append(f"{min(max(x / img_w, 0.0), 1.0):.6f}")
        coords.append(f"{min(max(y / img_h, 0.0), 1.0):.6f}")
    return f"{int(c)} " + " ".join(coords)
