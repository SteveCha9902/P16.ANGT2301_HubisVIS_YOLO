"""YOLO 라벨 검수 도구 - 타원 라벨 (tkinter, 추가 설치 불필요).

실행: python review_labels.py            (data/review_list.txt 순서, 검수 안 한 첫 영상부터)
      python review_labels.py --all      (처음부터)
      python review_labels.py --name <영상이름>  (그 영상부터, 검수 완료 영상 다시 고칠 때)
먼저 한 번: python convert_box_labels.py (기존 박스 초안 → 타원 초안)

조작
  왼쪽 드래그(빈 곳)   : 새 타원 (드래그한 사각형에 내접, 현재 클래스)
  왼쪽 클릭(빈 곳)     : spatter 클래스일 때 작은 원 spatter (지름 SPATTER_DIAMETER px). 비드 밖 방울을 빠르게 표시할 때
  [ / ]                : 선택 타원 크기 10% 축소 / 확대 (작은 spatter 맞출 때)
  왼쪽 클릭(타원 안)   : 선택 → 드래그로 이동
  축 끝 점(■) 드래그   : 그 축 길이 조절 (반대쪽 끝은 고정)
  회전 점(●) 드래그    : 회전
  Q / E                : 선택 타원 1° 회전 (Shift+Q / Shift+E : 0.1°)
  1 / 2                : 현재 클래스 welding / spatter (타원 선택 중이면 그 타원 클래스 변경)
  Delete / BackSpace   : 선택 타원 삭제
  휠                   : 확대/축소 (커서 기준),  오른쪽 드래그 : 화면 이동,  F : 전체 보기
  D / → / Space        : 저장 후 다음,   A / ← : 저장 후 이전   (아래 버튼으로도 가능, 한글 입력 상태에서도 동작)
  Ctrl+Z               : 이 영상 변경 되돌리기 (마지막 저장 상태로)
  Esc                  : 선택 해제
점선 사각형은 타원을 감싸는 회전 박스다. W(가로)·H(세로)는 이 박스의 변 길이(픽셀)다.
저장 형식: data/ellipses/<이름>.txt  "class cx cy w h angle" (ellipse_labels.py 참고). 검수 완료 목록: data/reviewed.txt
"""
from __future__ import annotations

import argparse
import math
import tkinter as tk
from pathlib import Path

from PIL import Image, ImageTk

import ellipse_labels as el

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"
ELLIPSES = DATA / "ellipses"
CLASS_NAMES = ["welding", "spatter"]
CLASS_COLORS = ["#ff3030", "#30c0ff"]
HANDLE = 8          # 점 잡기 범위 (화면 픽셀)
ROTATE_OFFSET = 30  # 회전 점을 축 끝에서 띄우는 거리 (화면 픽셀)
MIN_AXIS = 2.0      # 축 최소 길이 (영상 픽셀)
SPATTER_DIAMETER = 24.0  # 클릭으로 만드는 spatter 원 지름 (영상 픽셀)


class Reviewer:
    def __init__(self, root: tk.Tk, names: list[str], start: int):
        self.root = root
        self.names = names
        self.index = start
        self.reviewed = set()
        reviewed_file = DATA / "reviewed.txt"
        if reviewed_file.exists():
            self.reviewed = set(reviewed_file.read_text(encoding="utf-8").split())

        self.current_class = 0
        self.selected: int | None = None
        self.drag_mode = None  # "new" | "move" | ("axis", 0|1, ±1) | "rotate"
        self.drag_start = (0.0, 0.0)
        self.drag_item0 = None

        bar = tk.Frame(root)
        bar.pack(side=tk.BOTTOM, fill=tk.X)
        tk.Button(bar, text="◀ 저장 후 이전 (A / ←)", command=lambda: self.go(-1)).pack(side=tk.LEFT)
        tk.Button(bar, text="저장 후 다음 (D / → / Space) ▶", command=lambda: self.go(1)).pack(side=tk.LEFT)
        self.status = tk.Label(bar, anchor="w", font=("Consolas", 10))
        self.status.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.canvas = tk.Canvas(root, bg="#202020", highlightthickness=0, cursor="crosshair")
        self.canvas.pack(fill=tk.BOTH, expand=True)

        c = self.canvas
        c.bind("<ButtonPress-1>", self.on_left_down)
        c.bind("<B1-Motion>", self.on_left_drag)
        c.bind("<ButtonRelease-1>", self.on_left_up)
        c.bind("<ButtonPress-3>", self.on_right_down)
        c.bind("<B3-Motion>", self.on_right_drag)
        c.bind("<MouseWheel>", self.on_wheel)
        c.bind("<Configure>", lambda e: self.fit())
        for key, fn in [("1", lambda e: self.set_class(0)), ("2", lambda e: self.set_class(1)),
                        ("<Delete>", self.delete_selected), ("<BackSpace>", self.delete_selected),
                        ("d", lambda e: self.go(1)), ("<Right>", lambda e: self.go(1)), ("<space>", lambda e: self.go(1)),
                        ("a", lambda e: self.go(-1)), ("<Left>", lambda e: self.go(-1)),
                        ("q", lambda e: self.rotate(-1.0)), ("e", lambda e: self.rotate(1.0)),
                        ("Q", lambda e: self.rotate(-0.1)), ("E", lambda e: self.rotate(0.1)),
                        ("<bracketleft>", lambda e: self.resize(0.9)), ("<bracketright>", lambda e: self.resize(1.1)),
                        ("f", lambda e: self.fit()), ("<Escape>", lambda e: self.select(None)),
                        ("<Control-z>", lambda e: self.load())]:
            root.bind(key, fn)
        # 한글 입력 상태에서는 D가 'ㅇ'처럼 들어와 위 단축키가 동작하지 않으므로 같은 자판 위치의 자모도 받는다
        hangul = {"ㅇ": lambda: self.go(1), "ㅁ": lambda: self.go(-1), "ㄹ": self.fit,
                  "ㅂ": lambda: self.rotate(-1.0), "ㄷ": lambda: self.rotate(1.0),
                  "ㅃ": lambda: self.rotate(-0.1), "ㄸ": lambda: self.rotate(0.1)}
        root.bind("<Key>", lambda e: hangul[e.char]() if e.char in hangul else None)
        root.protocol("WM_DELETE_WINDOW", self.quit)
        self.load()

    # ---- 좌표 변환 ----
    def to_screen(self, x, y):
        return x * self.scale + self.ox, y * self.scale + self.oy

    def to_image(self, sx, sy):
        return (sx - self.ox) / self.scale, (sy - self.oy) / self.scale

    # ---- 영상 ----
    def load(self):
        name = self.names[self.index]
        self.image = Image.open(DATA / "images" / f"{name}.png").convert("L")
        self.w, self.h = self.image.size
        self.label_path = ELLIPSES / f"{name}.txt"
        self.items = el.load(self.label_path)
        self.selected = None
        self.fit()

    def fit(self):
        cw, ch = max(self.canvas.winfo_width(), 100), max(self.canvas.winfo_height(), 100)
        self.scale = min(cw / self.w, ch / self.h)
        self.ox = (cw - self.w * self.scale) / 2
        self.oy = (ch - self.h * self.scale) / 2
        self.redraw()

    def handles(self, i):
        """선택 타원의 점: [(종류, 화면x, 화면y)], 종류 = ("axis", 축, 부호) | "rotate" """
        _, cx, cy, w, h, a = self.items[i]
        (ux, uy), (vx, vy) = el.axes(a)
        out = []
        for axis, (ax, ay), half in ((0, (ux, uy), w / 2), (1, (vx, vy), h / 2)):
            for sign in (1, -1):
                out.append((("axis", axis, sign), *self.to_screen(cx + sign * half * ax, cy + sign * half * ay)))
        ex, ey = self.to_screen(cx + w / 2 * ux, cy + w / 2 * uy)
        out.append(("rotate", ex + ROTATE_OFFSET * ux, ey + ROTATE_OFFSET * uy))
        return out

    def redraw(self):
        c = self.canvas
        c.delete("all")
        cw, ch = max(c.winfo_width(), 100), max(c.winfo_height(), 100)
        # 화면에 보이는 영역만 잘라서 확대 (큰 영상도 빠르게)
        x0, y0 = self.to_image(0, 0)
        x1, y1 = self.to_image(cw, ch)
        ix0, iy0 = max(0, int(x0)), max(0, int(y0))
        ix1, iy1 = min(self.w, int(x1) + 1), min(self.h, int(y1) + 1)
        if ix1 > ix0 and iy1 > iy0:
            crop = self.image.crop((ix0, iy0, ix1, iy1))
            sw, sh = max(1, int((ix1 - ix0) * self.scale)), max(1, int((iy1 - iy0) * self.scale))
            resample = Image.NEAREST if self.scale > 2 else Image.BILINEAR
            self.photo = ImageTk.PhotoImage(crop.resize((sw, sh), resample))
            sx, sy = self.to_screen(ix0, iy0)
            c.create_image(sx, sy, anchor="nw", image=self.photo)

        for i, (cls, cx, cy, w, h, a) in enumerate(self.items):
            color = CLASS_COLORS[cls % len(CLASS_COLORS)]
            sel = i == self.selected
            poly = [v for p in el.polygon(cx, cy, w, h, a, 72) for v in self.to_screen(*p)]
            c.create_polygon(*poly, outline=color, fill="", width=3 if sel else 2)
            box = [v for p in el.corners(cx, cy, w, h, a) for v in self.to_screen(*p)]
            c.create_polygon(*box, outline=color, fill="", width=1, dash=(4, 3))
            tx, ty = self.to_screen(*min(el.corners(cx, cy, w, h, a), key=lambda p: p[1] * 10000 + p[0]))
            c.create_text(tx + 3, ty - 2, text=f"{CLASS_NAMES[cls]} W{w:.0f} H{h:.0f} {a:+.1f}°", fill=color, anchor="sw",
                          font=("Consolas", 10, "bold"))
            if sel:
                for kind, hx, hy in self.handles(i):
                    if kind == "rotate":
                        ex, ey = self.to_screen(cx + w / 2 * math.cos(math.radians(a)), cy + w / 2 * math.sin(math.radians(a)))
                        c.create_line(ex, ey, hx, hy, fill=color)
                        c.create_oval(hx - 5, hy - 5, hx + 5, hy + 5, fill=color, outline="white")
                    else:
                        c.create_rectangle(hx - 4, hy - 4, hx + 4, hy + 4, fill=color, outline="white")
        self.update_status()

    def update_status(self):
        name = self.names[self.index]
        counts = [sum(1 for b in self.items if b[0] == k) for k in range(len(CLASS_NAMES))]
        done = len([n for n in self.names if n in self.reviewed])
        mark = "검수완료" if name in self.reviewed else "미검수"
        sel = ""
        if self.selected is not None:
            _, cx, cy, w, h, a = self.items[self.selected]
            sel = f"   선택: W {w:.1f} H {h:.1f} px  각도 {a:+.2f}°  중심 ({cx:.0f}, {cy:.0f})"
        inside = self.spatter_inside_bead()
        if inside:
            sel += f"   ※ 비드 안 spatter {inside}개 (비드에 붙은 덩어리는 welding에 포함)"
        self.status.config(text=f"[{self.index + 1}/{len(self.names)}] {name}  ({mark}, 전체 검수 {done}/{len(self.names)})   "
                                f"welding {counts[0]}  spatter {counts[1]}   현재 클래스: {CLASS_NAMES[self.current_class]}   "
                                f"배율 {self.scale:.2f}{sel}")
        self.root.title(f"라벨 검수 - {name}")

    # ---- 편집 ----
    def hit_handle(self, sx, sy):
        if self.selected is None:
            return None
        for kind, hx, hy in self.handles(self.selected):
            if abs(hx - sx) <= HANDLE and abs(hy - sy) <= HANDLE:
                return kind
        return None

    def hit_item(self, x, y):
        # 작은 타원을 먼저 고른다 (겹칠 때 spatter 선택이 쉽도록)
        margin = HANDLE / self.scale / 2
        hits = [i for i, b in enumerate(self.items) if el.contains(b, x, y, margin)]
        if not hits:
            return None
        return min(hits, key=lambda i: self.items[i][3] * self.items[i][4])

    def select(self, i):
        self.selected = i
        self.redraw()

    def on_left_down(self, e):
        self.canvas.focus_set()
        x, y = self.to_image(e.x, e.y)
        handle = self.hit_handle(e.x, e.y)
        if handle is not None:
            self.drag_mode = handle
        else:
            hit = self.hit_item(x, y)
            if hit is not None:
                self.selected = hit
                self.drag_mode = "move"
            else:
                self.items.append([self.current_class, x, y, 0.0, 0.0, 0.0])
                self.selected = len(self.items) - 1
                self.drag_mode = "new"
        self.drag_start = (x, y)
        self.drag_item0 = list(self.items[self.selected])
        self.redraw()

    def on_left_drag(self, e):
        if self.selected is None or self.drag_mode is None:
            return
        x, y = self.to_image(e.x, e.y)
        x = max(0.0, min(self.w, x)); y = max(0.0, min(self.h, y))
        b, b0 = self.items[self.selected], self.drag_item0
        _, cx0, cy0, w0, h0, a0 = b0
        if self.drag_mode == "new":
            sx, sy = self.drag_start
            b[1], b[2], b[3], b[4], b[5] = (sx + x) / 2, (sy + y) / 2, abs(x - sx), abs(y - sy), 0.0
        elif self.drag_mode == "move":
            b[1], b[2] = cx0 + x - self.drag_start[0], cy0 + y - self.drag_start[1]
        elif self.drag_mode == "rotate":
            b[5] = math.degrees(math.atan2(y - cy0, x - cx0))
        else:
            _, axis, sign = self.drag_mode
            (ax, ay) = el.axes(a0)[axis]
            length0 = w0 if axis == 0 else h0
            # 반대쪽 끝(-sign 쪽)을 고정하고 잡은 끝만 마우스 투영 위치로 옮긴다
            p = sign * ((x - cx0) * ax + (y - cy0) * ay)
            length = max(MIN_AXIS, p + length0 / 2)
            shift = sign * (length - length0) / 2
            b[1], b[2] = cx0 + shift * ax, cy0 + shift * ay
            if axis == 0:
                b[3] = length
            else:
                b[4] = length
        self.redraw()

    def on_left_up(self, e):
        if self.selected is not None and self.drag_mode is not None:
            b = self.items[self.selected]
            # 클릭만 한 새 타원: spatter면 작은 원으로 만들고, welding이면 실수로 보고 버린다
            if self.drag_mode == "new" and (b[3] * self.scale < 4 or b[4] * self.scale < 4):
                if b[0] == 1:
                    b[1], b[2] = self.drag_start
                    b[3] = b[4] = SPATTER_DIAMETER
                    b[5] = 0.0
                else:
                    self.items.pop(self.selected)
                    self.selected = None
            else:
                b[1:] = el.normalize(*b[1:])
        self.drag_mode = None
        self.redraw()

    def on_right_down(self, e):
        self.pan_start = (e.x, e.y, self.ox, self.oy)

    def on_right_drag(self, e):
        sx, sy, ox, oy = self.pan_start
        self.ox, self.oy = ox + e.x - sx, oy + e.y - sy
        self.redraw()

    def on_wheel(self, e):
        factor = 1.25 if e.delta > 0 else 0.8
        x, y = self.to_image(e.x, e.y)
        self.scale = max(0.05, min(20.0, self.scale * factor))
        self.ox, self.oy = e.x - x * self.scale, e.y - y * self.scale
        self.redraw()

    def rotate(self, degrees):
        if self.selected is not None:
            b = self.items[self.selected]
            b[1:] = el.normalize(b[1], b[2], b[3], b[4], b[5] + degrees)
            self.redraw()

    def resize(self, factor):
        if self.selected is not None:
            b = self.items[self.selected]
            b[3], b[4] = max(MIN_AXIS, b[3] * factor), max(MIN_AXIS, b[4] * factor)
            self.redraw()

    def spatter_inside_bead(self) -> int:
        """중심이 welding 타원 안에 있는 spatter 수 (라벨 기준: 비드 밖 방울만 spatter)"""
        beads = [b for b in self.items if b[0] == 0]
        return sum(1 for s in self.items if s[0] == 1 and any(el.contains(b, s[1], s[2]) for b in beads))

    def set_class(self, k):
        self.current_class = k
        if self.selected is not None:
            self.items[self.selected][0] = k
        self.redraw()

    def delete_selected(self, e=None):
        if self.selected is not None:
            self.items.pop(self.selected)
            self.selected = None
            self.redraw()

    # ---- 저장·이동 ----
    def save(self):
        el.save(self.label_path, self.items, MIN_AXIS)
        self.reviewed.add(self.names[self.index])
        (DATA / "reviewed.txt").write_text("\n".join(n for n in self.names if n in self.reviewed) + "\n", encoding="utf-8")

    def go(self, step):
        self.save()
        self.index = max(0, min(len(self.names) - 1, self.index + step))
        self.load()

    def quit(self):
        self.save()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--all", action="store_true", help="검수 여부와 관계없이 처음부터")
    parser.add_argument("--name", help="이 영상(확장자 없는 이름)부터 연다. 검수 완료 영상을 다시 고칠 때")
    args = parser.parse_args()

    if not ELLIPSES.exists():
        raise SystemExit("data/ellipses가 없습니다. python convert_box_labels.py를 먼저 실행하세요.")
    names = [n for n in (DATA / "review_list.txt").read_text(encoding="utf-8").split() if (DATA / "images" / f"{n}.png").exists()]
    if not names:
        raise SystemExit("data/review_list.txt가 비어 있습니다. make_draft_labels.py를 먼저 실행하세요.")
    reviewed = set((DATA / "reviewed.txt").read_text(encoding="utf-8").split()) if (DATA / "reviewed.txt").exists() else set()
    start = 0 if args.all else next((i for i, n in enumerate(names) if n not in reviewed), 0)
    if args.name:
        if args.name not in names:
            raise SystemExit(f"review_list.txt에 없는 영상입니다: {args.name}")
        start = names.index(args.name)

    root = tk.Tk()
    root.geometry("1400x1000")
    Reviewer(root, names, start)
    root.mainloop()


if __name__ == "__main__":
    main()
