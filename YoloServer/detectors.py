"""HubisVIS 후검사 YOLO 서버의 추론 backend.

모든 backend는 detect(image_bgr) -> list[Detection]을 제공한다.
박스 좌표는 원본 영상 기준 픽셀 [x1, y1, x2, y2]이다.
segmentation 모델이면 마스크에 맞춘 타원 [cx, cy, w, h, angle]도 함께 낸다.
  - w: (cos angle, sin angle) 방향 축의 전체 길이, h: 그 수직 축 길이 (원본 픽셀, 영상 좌표는 y 아래 방향)
  - angle은 (-45, 45]도로 정규화한다. w는 항상 가로축에 가까운 축이다 (YoloTrain/ellipse_labels.py와 같은 규칙).
"""
from __future__ import annotations

import ast
import math
from dataclasses import dataclass
from typing import Sequence

import cv2
import numpy as np


@dataclass
class Detection:
    class_name: str
    confidence: float
    box: tuple[float, float, float, float]
    ellipse: tuple[float, float, float, float, float] | None = None

    def to_dict(self) -> dict:
        # HubisVIS 파서는 항목 안에서 "class" 다음에 "confidence"와 첫 "[...]"(box)를 찾는다. 키 순서를 유지하고
        # ellipse는 box 뒤에 둔다 (ellipse를 모르는 이전 파서도 box를 그대로 읽는다).
        out = {
            "class": self.class_name,
            "confidence": round(float(self.confidence), 4),
            "box": [round(float(v), 1) for v in self.box],
        }
        if self.ellipse is not None:
            out["ellipse"] = [round(float(v), 2) for v in self.ellipse]
        return out


def normalize_ellipse(cx: float, cy: float, w: float, h: float, angle: float):
    """angle을 (-45, 45]로 옮기고 필요하면 w/h를 바꾼다."""
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


class MockDetector:
    """설정에 적은 고정 박스를 돌려준다. 모델 없이 HubisVIS 연동(프로토콜·판정 흐름)을 시험할 때 쓴다."""

    def __init__(self, boxes: Sequence[dict]):
        self._detections = [
            Detection(str(b["class"]), float(b["confidence"]), tuple(float(v) for v in b["box"]),
                      normalize_ellipse(*map(float, b["ellipse"])) if b.get("ellipse") else None)
            for b in boxes
        ]

    def detect(self, image: np.ndarray) -> list[Detection]:
        return list(self._detections)


def letterbox(image: np.ndarray, size: int) -> tuple[np.ndarray, float, float, float]:
    """비율을 유지해 size x size에 맞추고 남는 곳은 114로 채운다. (영상, 배율, x 여백, y 여백)"""
    h, w = image.shape[:2]
    scale = min(size / w, size / h)
    new_w, new_h = int(round(w * scale)), int(round(h * scale))
    resized = cv2.resize(image, (new_w, new_h), interpolation=cv2.INTER_LINEAR)
    pad_x = (size - new_w) / 2
    pad_y = (size - new_h) / 2
    left, top = int(round(pad_x - 0.1)), int(round(pad_y - 0.1))
    out = cv2.copyMakeBorder(resized, top, size - new_h - top, left, size - new_w - left,
                             cv2.BORDER_CONSTANT, value=(114, 114, 114))
    return out, scale, left, top


def decode_yolo_output(output: np.ndarray, num_classes: int, conf_threshold: float, num_masks: int = 0):
    """YOLO 출력 텐서를 (cx, cy, w, h), score, class_id, mask 계수 배열로 바꾼다.

    - YOLOv8/11:     (1, 4 + nc, N) - objectness 없음
    - YOLOv8/11 seg: (1, 4 + nc + nm, N) - 뒤 nm개는 mask 계수
    - YOLOv5:        (1, N, 5 + nc) - objectness 포함
    """
    pred = np.squeeze(output, axis=0)
    if pred.ndim != 2:
        raise ValueError(f"unexpected output shape {output.shape}")

    rows = 4 + num_classes + num_masks
    if pred.shape[0] == rows and pred.shape[1] != rows:
        pred = pred.T  # (N, 4 + nc + nm)
    if pred.shape[1] == rows:
        boxes = pred[:, :4]
        class_scores = pred[:, 4:4 + num_classes]
        class_ids = np.argmax(class_scores, axis=1)
        scores = class_scores[np.arange(len(pred)), class_ids]
        coefs = pred[:, 4 + num_classes:]
        keep = scores >= conf_threshold
        return boxes[keep], scores[keep], class_ids[keep], coefs[keep]
    elif num_masks == 0 and pred.shape[1] == 5 + num_classes:
        boxes = pred[:, :4]
        class_scores = pred[:, 5:] * pred[:, 4:5]
        class_ids = np.argmax(class_scores, axis=1)
        scores = class_scores[np.arange(len(pred)), class_ids]
    else:
        raise ValueError(f"output shape {output.shape} does not match {num_classes} classes")

    keep = scores >= conf_threshold
    return boxes[keep], scores[keep], class_ids[keep], np.zeros((int(keep.sum()), 0), np.float32)


def mask_ellipse(coef: np.ndarray, protos: np.ndarray, box, scale: float, pad_x: float, pad_y: float,
                 input_size: int, image_w: int, image_h: int):
    """검출 하나의 마스크를 원본 해상도로 복원해 가장 큰 영역에 타원을 맞춘다. 실패하면 None.

    protos: (nm, mh, mw), 입력(letterbox) 대비 stride = input_size / mw.
    마스크 logit을 원본 픽셀 격자로 직접 보간하므로 letterbox 축소(약 0.42배)에 따른 계단이 생기지 않는다.
    마스크는 검출 박스 안만 쓴다(Ultralytics crop_mask와 같음). seg 학습은 박스 안에서만 마스크를 학습하므로
    박스 밖으로 번진 마스크까지 쓰면 타원이 커진다(실측: 여유 5%를 두었을 때 H +6 px 편향).
    """
    nm, mh, mw = protos.shape
    stride = input_size / mw
    x1, y1, x2, y2 = box
    X0, Y0 = max(0, int(math.floor(x1))), max(0, int(math.floor(y1)))
    X1, Y1 = min(image_w, int(math.ceil(x2))), min(image_h, int(math.ceil(y2)))
    if X1 - X0 < 3 or Y1 - Y0 < 3:
        return None

    logits = (coef.astype(np.float32) @ protos.reshape(nm, -1)).reshape(mh, mw)
    # 원본 픽셀 (X0+u)의 중심 → letterbox 연속 좌표 → proto 픽셀 인덱스 좌표
    a = scale / stride
    m = np.array([[a, 0, ((X0 + 0.5) * scale + pad_x) / stride - 0.5],
                  [0, a, ((Y0 + 0.5) * scale + pad_y) / stride - 0.5]], np.float32)
    crop = cv2.warpAffine(logits, m, (X1 - X0, Y1 - Y0), flags=cv2.INTER_LINEAR | cv2.WARP_INVERSE_MAP,
                          borderMode=cv2.BORDER_CONSTANT, borderValue=-20.0)
    binary = (crop > 0).astype(np.uint8)
    contours, _ = cv2.findContours(binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
    if not contours:
        return None
    contour = max(contours, key=cv2.contourArea)
    if len(contour) < 5:
        return None
    (cx, cy), (w, h), angle = cv2.fitEllipse(contour)
    # 윤곽선은 경계 픽셀의 중심을 지나므로 실제 경계보다 양쪽 0.5 px씩 안쪽이다
    return normalize_ellipse(cx + X0, cy + Y0, w + 1.0, h + 1.0, angle)


def postprocess(output: np.ndarray, class_names: Sequence[str], conf_threshold: float, iou_threshold: float,
                scale: float, pad_x: float, pad_y: float, image_w: int, image_h: int,
                protos: np.ndarray | None = None, input_size: int = 0) -> list[Detection]:
    num_masks = 0 if protos is None else protos.shape[0]
    boxes, scores, class_ids, coefs = decode_yolo_output(output, len(class_names), conf_threshold, num_masks)
    if len(boxes) == 0:
        return []

    # letterbox 좌표 → 원본 좌표
    x1 = (boxes[:, 0] - boxes[:, 2] / 2 - pad_x) / scale
    y1 = (boxes[:, 1] - boxes[:, 3] / 2 - pad_y) / scale
    x2 = (boxes[:, 0] + boxes[:, 2] / 2 - pad_x) / scale
    y2 = (boxes[:, 1] + boxes[:, 3] / 2 - pad_y) / scale
    x1 = np.clip(x1, 0, image_w - 1); x2 = np.clip(x2, 0, image_w - 1)
    y1 = np.clip(y1, 0, image_h - 1); y2 = np.clip(y2, 0, image_h - 1)

    detections: list[Detection] = []
    # 클래스별 NMS (welding과 spatter가 서로 지우지 않도록)
    for cls in np.unique(class_ids):
        idx = np.where(class_ids == cls)[0]
        rects = [[float(x1[i]), float(y1[i]), float(x2[i] - x1[i]), float(y2[i] - y1[i])] for i in idx]
        keep = cv2.dnn.NMSBoxes(rects, [float(scores[i]) for i in idx], conf_threshold, iou_threshold)
        for k in np.array(keep).reshape(-1):
            i = idx[int(k)]
            box = (float(x1[i]), float(y1[i]), float(x2[i]), float(y2[i]))
            ellipse = None
            if protos is not None:
                ellipse = mask_ellipse(coefs[i], protos, box, scale, pad_x, pad_y, input_size, image_w, image_h)
            detections.append(Detection(class_names[int(cls)], float(scores[i]), box, ellipse))

    detections.sort(key=lambda d: (d.class_name, d.box[0], d.box[1]))
    return detections


class OnnxYoloDetector:
    """YOLO(v5/v8/11) ONNX 모델 추론. onnxruntime이 있으면 사용하고, 없으면 OpenCV DNN을 쓴다."""

    def __init__(self, model_path: str, class_names: Sequence[str] | None, image_size: int,
                 conf_threshold: float, iou_threshold: float, device: str = "cpu"):
        self.image_size = int(image_size)
        self.conf_threshold = float(conf_threshold)
        self.iou_threshold = float(iou_threshold)
        self._session = None
        self._net = None

        names_from_model = None
        try:
            import onnxruntime as ort  # type: ignore
            providers = ["CUDAExecutionProvider", "CPUExecutionProvider"] if device == "cuda" else ["CPUExecutionProvider"]
            available = ort.get_available_providers()
            self._session = ort.InferenceSession(model_path, providers=[p for p in providers if p in available])
            self._input_name = self._session.get_inputs()[0].name
            meta = self._session.get_modelmeta().custom_metadata_map
            if "names" in meta:  # Ultralytics export: "{0: 'welding', 1: 'spatter'}"
                parsed = ast.literal_eval(meta["names"])
                names_from_model = [parsed[k] for k in sorted(parsed)]
            self.engine = "onnxruntime:" + ",".join(self._session.get_providers())
        except ImportError:
            self._net = cv2.dnn.readNetFromONNX(model_path)
            if device == "cuda":
                self._net.setPreferableBackend(cv2.dnn.DNN_BACKEND_CUDA)
                self._net.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA)
            self.engine = "opencv-dnn"

        self.class_names = list(class_names) if class_names else names_from_model
        if not self.class_names:
            raise ValueError("class_names must be set in config (model has no names metadata)")

    def detect(self, image: np.ndarray) -> list[Detection]:
        if image.ndim == 2:
            image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
        h, w = image.shape[:2]
        lb, scale, pad_x, pad_y = letterbox(image, self.image_size)
        blob = cv2.dnn.blobFromImage(lb, 1 / 255.0, swapRB=True)  # NCHW, RGB, 0~1

        if self._session is not None:
            outputs = self._session.run(None, {self._input_name: blob})
        else:
            self._net.setInput(blob)
            outputs = self._net.forward(self._net.getUnconnectedOutLayersNames())

        # segmentation 모델은 출력이 둘이다: 검출 (1, 4+nc+nm, N), proto (1, nm, mh, mw). 순서는 엔진마다 다를 수 있다.
        output = next(o for o in outputs if o.ndim == 3)
        proto = next((o for o in outputs if o.ndim == 4), None)
        protos = proto[0] if proto is not None else None
        return postprocess(output, self.class_names, self.conf_threshold, self.iou_threshold,
                           scale, pad_x, pad_y, w, h, protos, self.image_size)


def create_detector(config: dict):
    backend = config.get("backend", "onnx")
    if backend == "mock":
        return MockDetector(config.get("mock_boxes", []))
    if backend == "onnx":
        return OnnxYoloDetector(
            model_path=config["model_path"],
            class_names=config.get("class_names"),
            image_size=config.get("image_size", 640),
            conf_threshold=config.get("conf_threshold", 0.25),
            iou_threshold=config.get("iou_threshold", 0.45),
            device=config.get("device", "cpu"),
        )
    raise ValueError(f"unknown backend: {backend}")
