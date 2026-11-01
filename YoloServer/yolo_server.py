"""HubisVIS 후검사 YOLO 서버.

프로토콜 (HubisVIS AlgorithmTools CInspection::SocketYOLO_SingleCall):
  요청: 영상 파일 경로 + "\\n"  (HubisVIS는 ANSI(CP949) 경로를 보낸다)
  응답: JSON 한 줄 + "\\n"
        정상: [{"class": "welding", "confidence": 0.93, "box": [x1, y1, x2, y2]}, ...]
        실패: {"error": "<사유>"}   (HubisVIS는 "error" 키가 있으면 YOLO 실패로 처리)
박스는 원본 영상 픽셀 좌표. 연결 하나에 요청 하나.

실행: python yolo_server.py [--config server_config.json]
"""
from __future__ import annotations

import argparse
import json
import logging
import logging.handlers
import os
import socket
import socketserver
import sys
import threading
import time

import cv2
import numpy as np

from detectors import create_detector

MAX_REQUEST_BYTES = 4096

log = logging.getLogger("yolo_server")


def decode_path(raw: bytes) -> str:
    """요청 바이트를 경로 문자열로 바꾼다. UTF-8을 먼저 시도하고, 아니면 Windows ANSI 코드페이지로 읽는다."""
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError:
        return raw.decode("mbcs" if os.name == "nt" else "cp949", errors="replace")


def read_image(path: str) -> np.ndarray | None:
    # cv2.imread는 Windows에서 비-ASCII 경로를 못 읽으므로 바이트로 읽어 디코딩한다
    try:
        data = np.fromfile(path, dtype=np.uint8)
    except OSError:
        return None
    if data.size == 0:
        return None
    return cv2.imdecode(data, cv2.IMREAD_UNCHANGED)


class InferenceService:
    """detector를 한 번에 하나의 요청만 쓰도록 직렬화한다 (GPU 메모리·세션 스레드 안전성)."""

    def __init__(self, detector):
        self._detector = detector
        self._lock = threading.Lock()

    def handle(self, raw_request: bytes) -> dict | list:
        path = decode_path(raw_request).strip().strip('"')
        if not path:
            return {"error": "empty request"}
        if not os.path.isfile(path):
            log.warning("file not found: %s", path)
            return {"error": "image not found"}

        t0 = time.perf_counter()
        image = read_image(path)
        if image is None:
            log.warning("image decode fail: %s", path)
            return {"error": "image decode fail"}
        t1 = time.perf_counter()

        with self._lock:
            detections = self._detector.detect(image)
        t2 = time.perf_counter()

        counts: dict[str, int] = {}
        for d in detections:
            counts[d.class_name] = counts.get(d.class_name, 0) + 1
        log.info("%s %dx%d %s read:%.0fms infer:%.0fms", os.path.basename(path), image.shape[1], image.shape[0],
                 counts or "{}", (t1 - t0) * 1000, (t2 - t1) * 1000)
        return [d.to_dict() for d in detections]


class RequestHandler(socketserver.BaseRequestHandler):
    def handle(self):
        sock: socket.socket = self.request
        sock.settimeout(self.server.client_timeout_s)
        try:
            raw = b""
            while b"\n" not in raw:
                chunk = sock.recv(1024)
                if not chunk:
                    break
                raw += chunk
                if len(raw) > MAX_REQUEST_BYTES:
                    self._reply({"error": "request too long"})
                    return
            if not raw:
                return
            raw = raw.split(b"\n", 1)[0].rstrip(b"\r")

            try:
                result = self.server.service.handle(raw)
            except Exception as exc:  # 추론 예외가 서버를 죽이지 않게 한다
                log.exception("inference error")
                result = {"error": f"inference error: {type(exc).__name__}"}
            self._reply(result)
        except socket.timeout:
            log.warning("client timeout %s", self.client_address)
        except OSError as exc:
            log.warning("socket error %s: %s", self.client_address, exc)

    def _reply(self, result):
        self.request.sendall((json.dumps(result, ensure_ascii=False) + "\n").encode("utf-8"))


class YoloTCPServer(socketserver.ThreadingTCPServer):
    daemon_threads = True
    allow_reuse_address = False  # Windows에서 같은 포트를 다른 프로세스가 같이 여는 것을 막는다

    def __init__(self, address, service: InferenceService, client_timeout_s: float):
        self.service = service
        self.client_timeout_s = client_timeout_s
        super().__init__(address, RequestHandler)

    def server_bind(self):
        if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        super().server_bind()


def load_config(path: str) -> dict:
    with open(path, encoding="utf-8") as f:
        config = json.load(f)
    base = os.path.dirname(os.path.abspath(path))
    if config.get("model_path") and not os.path.isabs(config["model_path"]):
        config["model_path"] = os.path.join(base, config["model_path"])
    if config.get("log_dir") and not os.path.isabs(config["log_dir"]):
        config["log_dir"] = os.path.join(base, config["log_dir"])
    return config


def setup_logging(log_dir: str | None):
    log.setLevel(logging.INFO)
    fmt = logging.Formatter("%(asctime)s %(levelname)s %(message)s")
    console = logging.StreamHandler(sys.stdout)
    console.setFormatter(fmt)
    log.addHandler(console)
    if log_dir:
        os.makedirs(log_dir, exist_ok=True)
        fh = logging.handlers.RotatingFileHandler(os.path.join(log_dir, "yolo_server.log"),
                                                  maxBytes=10 * 1024 * 1024, backupCount=5, encoding="utf-8")
        fh.setFormatter(fmt)
        log.addHandler(fh)


def build_server(config: dict) -> YoloTCPServer:
    detector = create_detector(config)
    # 첫 요청이 느려지지 않도록 미리 한 번 추론한다
    warmup = np.zeros((int(config.get("warmup_height", 2048)), int(config.get("warmup_width", 2448))), np.uint8)
    t0 = time.perf_counter()
    detector.detect(warmup)
    log.info("backend=%s engine=%s warmup:%.0fms", config.get("backend"), getattr(detector, "engine", "-"),
             (time.perf_counter() - t0) * 1000)

    address = (config.get("host", "127.0.0.1"), int(config.get("port", 9999)))
    return YoloTCPServer(address, InferenceService(detector), float(config.get("client_timeout_s", 10)))


def main():
    parser = argparse.ArgumentParser(description="HubisVIS 후검사 YOLO 서버")
    parser.add_argument("--config", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "server_config.json"))
    args = parser.parse_args()

    config = load_config(args.config)
    setup_logging(config.get("log_dir"))
    server = build_server(config)
    host, port = server.server_address[:2]
    log.info("listening %s:%d", host, port)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        log.info("stopped")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
