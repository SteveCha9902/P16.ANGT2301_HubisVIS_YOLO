"""YOLO 서버 단위·프로토콜 테스트. 실행: python -m unittest test_yolo_server -v"""
from __future__ import annotations

import json
import os
import socket
import tempfile
import threading
import unittest
from concurrent.futures import ThreadPoolExecutor

import cv2
import numpy as np

from detectors import Detection, MockDetector, decode_yolo_output, letterbox, normalize_ellipse, postprocess
from yolo_server import InferenceService, YoloTCPServer, decode_path

NAMES = ["welding", "spatter"]


def make_v8_output(rows):
    """rows: (cx, cy, w, h, score_welding, score_spatter) → (1, 6, N)"""
    return np.array(rows, dtype=np.float32).T[np.newaxis, ...]


class PostprocessTest(unittest.TestCase):
    def test_letterbox_geometry(self):
        img = np.zeros((2048, 2448, 3), np.uint8)
        out, scale, px, py = letterbox(img, 640)
        self.assertEqual(out.shape, (640, 640, 3))
        self.assertAlmostEqual(scale, 640 / 2448, places=6)
        self.assertEqual(px, 0)
        self.assertEqual(py, round((640 - 2048 * scale) / 2 - 0.1))

    def test_v8_decode_maps_to_original_and_nms(self):
        scale, px, py = 0.5, 0, 20
        # 원본 (100, 200)-(300, 300) welding → letterbox (50, 120)-(150, 170): cx 100, cy 145, w 100, h 50
        out = make_v8_output([
            (100, 145, 100, 50, 0.9, 0.0),
            (101, 146, 100, 50, 0.8, 0.0),   # 중복 → NMS로 제거
            (300, 300, 10, 10, 0.0, 0.7),    # spatter
            (400, 400, 10, 10, 0.1, 0.1),    # 낮은 점수 → 제거
        ])
        dets = postprocess(out, NAMES, 0.25, 0.45, scale, px, py, 2448, 2048)
        self.assertEqual([d.class_name for d in dets], ["spatter", "welding"])
        weld = dets[1]
        self.assertAlmostEqual(weld.confidence, 0.9, places=5)
        np.testing.assert_allclose(weld.box, (100, 200, 300, 300), atol=1e-3)

    def test_nms_keeps_overlapping_different_classes(self):
        out = make_v8_output([(100, 100, 50, 50, 0.9, 0.0), (100, 100, 50, 50, 0.0, 0.8)])
        dets = postprocess(out, NAMES, 0.25, 0.45, 1.0, 0, 0, 640, 640)
        self.assertEqual(sorted(d.class_name for d in dets), ["spatter", "welding"])

    def test_v5_decode_uses_objectness(self):
        out = np.array([[[100, 100, 20, 20, 0.5, 0.9, 0.1]]], dtype=np.float32)  # (1, N, 5 + nc)
        boxes, scores, ids, _ = decode_yolo_output(out, 2, 0.25)
        self.assertEqual(len(boxes), 1)
        self.assertAlmostEqual(float(scores[0]), 0.45, places=5)
        self.assertEqual(int(ids[0]), 0)

    def test_boxes_clipped_to_image(self):
        out = make_v8_output([(5, 5, 40, 40, 0.9, 0.0)])
        dets = postprocess(out, NAMES, 0.25, 0.45, 1.0, 0, 0, 100, 100)
        self.assertEqual(dets[0].box[:2], (0.0, 0.0))

    def test_shape_mismatch_raises(self):
        with self.assertRaises(ValueError):
            decode_yolo_output(np.zeros((1, 9, 100), np.float32), 2, 0.25)

    def test_empty_output(self):
        self.assertEqual(postprocess(make_v8_output([(1, 1, 1, 1, 0.01, 0.01)]), NAMES, 0.25, 0.45, 1, 0, 0, 10, 10), [])

    def test_to_dict_key_order_for_hubisvis_parser(self):
        d = Detection("welding", 0.912345, (1.04, 2, 3, 4)).to_dict()
        self.assertEqual(list(d.keys()), ["class", "confidence", "box"])
        self.assertEqual(d, {"class": "welding", "confidence": 0.9123, "box": [1.0, 2.0, 3.0, 4.0]})

    def test_to_dict_ellipse_after_box(self):
        d = Detection("welding", 0.9, (1, 2, 3, 4), (10, 20, 30.456, 15, 5.5)).to_dict()
        self.assertEqual(list(d.keys()), ["class", "confidence", "box", "ellipse"])
        self.assertEqual(d["ellipse"], [10.0, 20.0, 30.46, 15.0, 5.5])

    def test_normalize_ellipse(self):
        np.testing.assert_allclose(normalize_ellipse(0, 0, 300, 100, 80), (0, 0, 100, 300, -10))
        np.testing.assert_allclose(normalize_ellipse(0, 0, 300, 100, -45), (0, 0, 100, 300, 45))
        np.testing.assert_allclose(normalize_ellipse(0, 0, 300, 100, 175), (0, 0, 300, 100, -5))


def make_seg_case(cx, cy, w, h, angle, image_w=2448, image_h=2048, size=1024, nm=32):
    """정답 타원 하나를 표현하는 합성 seg 출력. proto 0번 채널 = 1 - (타원 정규화 반지름)^2 (경계에서 0)."""
    _, scale, px, py = letterbox(np.zeros((image_h, image_w), np.uint8), size)
    mh = mw = size // 4
    j, i = np.mgrid[0:mh, 0:mw].astype(np.float64)
    X = ((i + 0.5) * 4 - px) / scale
    Y = ((j + 0.5) * 4 - py) / scale
    t = np.radians(angle)
    du, dv = (X - cx) * np.cos(t) + (Y - cy) * np.sin(t), -(X - cx) * np.sin(t) + (Y - cy) * np.cos(t)
    protos = np.zeros((nm, mh, mw), np.float32)
    protos[0] = 1 - (du / (w / 2)) ** 2 - (dv / (h / 2)) ** 2
    coef = np.zeros(nm, np.float32); coef[0] = 1
    # 검출 박스 (letterbox 좌표 cx, cy, w, h): 타원 외접 박스
    ex = np.hypot(w / 2 * np.cos(t), h / 2 * np.sin(t)); ey = np.hypot(w / 2 * np.sin(t), h / 2 * np.cos(t))
    row = np.concatenate([[cx * scale + px, cy * scale + py, 2 * ex * scale, 2 * ey * scale, 0.95, 0.0], coef])
    return row.astype(np.float32)[:, None][None], protos[None], scale, px, py


class SegmentationTest(unittest.TestCase):
    def check_case(self, cx, cy, w, h, angle, expect):
        out, proto, scale, px, py = make_seg_case(cx, cy, w, h, angle)
        dets = postprocess(out, NAMES, 0.25, 0.45, scale, px, py, 2448, 2048, proto[0], 1024)
        self.assertEqual(len(dets), 1)
        e = dets[0].ellipse
        self.assertIsNotNone(e)
        ecx, ecy, ew, eh, ea = expect
        self.assertAlmostEqual(e[0], ecx, delta=1.5)
        self.assertAlmostEqual(e[1], ecy, delta=1.5)
        self.assertAlmostEqual(e[2], ew, delta=3.0)
        self.assertAlmostEqual(e[3], eh, delta=3.0)
        self.assertAlmostEqual(e[4], ea, delta=0.7)

    def test_tilted_bead(self):
        self.check_case(1000, 1100, 360, 160, 12, (1000, 1100, 360, 160, 12))

    def test_negative_tilt(self):
        self.check_case(1900, 1150, 370, 150, -8, (1900, 1150, 370, 150, -8))

    def test_vertical_bead_keeps_width_horizontal(self):
        # 세로로 긴 비드(장축 80°) → w는 가로축에 가까운 축(=단축), angle -10
        self.check_case(1200, 900, 300, 120, 80, (1200, 900, 120, 300, -10))

    def test_box_clipped_at_image_edge(self):
        out, proto, scale, px, py = make_seg_case(2400, 1000, 200, 100, 0)
        dets = postprocess(out, NAMES, 0.25, 0.45, scale, px, py, 2448, 2048, proto[0], 1024)
        self.assertEqual(len(dets), 1)  # 잘린 타원도 실패 없이 처리

    def test_empty_mask_gives_no_ellipse(self):
        out, proto, scale, px, py = make_seg_case(1000, 1000, 300, 150, 0)
        proto[:] = -1.0
        dets = postprocess(out, NAMES, 0.25, 0.45, scale, px, py, 2448, 2048, proto[0], 1024)
        self.assertEqual(len(dets), 1)
        self.assertIsNone(dets[0].ellipse)
        self.assertNotIn("ellipse", dets[0].to_dict())

    def test_mock_ellipse(self):
        det = MockDetector([{"class": "welding", "confidence": 0.9, "box": [0, 0, 10, 10], "ellipse": [5, 5, 4, 10, 90]}])
        np.testing.assert_allclose(det.detect(None)[0].ellipse, (5, 5, 10, 4, 0))


class ProtocolTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.image_path = os.path.join(cls.tmp.name, "yolo_t0_item0.bmp")
        cv2.imwrite(cls.image_path, np.zeros((64, 80), np.uint8))
        cls.korean_path = os.path.join(cls.tmp.name, "한글_영상.bmp")
        cv2.imencode(".bmp", np.zeros((64, 80), np.uint8))[1].tofile(cls.korean_path)
        cls.not_image = os.path.join(cls.tmp.name, "broken.bmp")
        with open(cls.not_image, "wb") as f:
            f.write(b"not an image")

        mock = MockDetector([{"class": "welding", "confidence": 0.95, "box": [1, 2, 3, 4]}])
        cls.server = YoloTCPServer(("127.0.0.1", 0), InferenceService(mock), client_timeout_s=2)
        cls.port = cls.server.server_address[1]
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.tmp.cleanup()

    def request(self, payload: bytes, close_write=False) -> str:
        with socket.create_connection(("127.0.0.1", self.port), timeout=5) as s:
            s.sendall(payload)
            if close_write:
                s.shutdown(socket.SHUT_WR)
            data = b""
            while not data.endswith(b"\n"):
                chunk = s.recv(1024)
                if not chunk:
                    break
                data += chunk
        return data.decode("utf-8")

    def test_normal_request(self):
        resp = self.request(self.image_path.encode("mbcs") + b"\n")
        self.assertTrue(resp.endswith("\n"))
        self.assertEqual(json.loads(resp), [{"class": "welding", "confidence": 0.95, "box": [1.0, 2.0, 3.0, 4.0]}])

    def test_crlf_request(self):
        self.assertIsInstance(json.loads(self.request(self.image_path.encode("mbcs") + b"\r\n")), list)

    def test_ansi_korean_path(self):
        # HubisVIS는 CT2A(ANSI, CP949)로 경로를 보낸다
        resp = json.loads(self.request(self.korean_path.encode("cp949") + b"\n"))
        self.assertIsInstance(resp, list)

    def test_utf8_korean_path(self):
        self.assertIsInstance(json.loads(self.request(self.korean_path.encode("utf-8") + b"\n")), list)

    def test_missing_file(self):
        self.assertEqual(json.loads(self.request(b"C:\\no\\such\\file.bmp\n")), {"error": "image not found"})

    def test_not_an_image(self):
        self.assertEqual(json.loads(self.request(self.not_image.encode("mbcs") + b"\n")), {"error": "image decode fail"})

    def test_empty_request(self):
        self.assertEqual(json.loads(self.request(b"\n")), {"error": "empty request"})

    def test_too_long_request(self):
        self.assertEqual(json.loads(self.request(b"A" * 5000)), {"error": "request too long"})

    def test_request_without_newline_then_close(self):
        self.assertIsInstance(json.loads(self.request(self.image_path.encode("mbcs"), close_write=True)), list)

    def test_concurrent_requests(self):
        payload = self.image_path.encode("mbcs") + b"\n"
        with ThreadPoolExecutor(max_workers=8) as pool:
            results = list(pool.map(lambda _: json.loads(self.request(payload)), range(32)))
        self.assertTrue(all(isinstance(r, list) and len(r) == 1 for r in results))

    def test_decode_path(self):
        self.assertEqual(decode_path("한글".encode("cp949")), "한글")
        self.assertEqual(decode_path("한글".encode("utf-8")), "한글")


if __name__ == "__main__":
    unittest.main()
