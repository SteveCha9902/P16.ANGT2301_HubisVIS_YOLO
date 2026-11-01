# HubisVIS 후검사 YOLO 서버

HubisVIS(`AlgorithmTools` `CInspection::SocketYOLO_SingleCall`)가 후검사(`DETECT_BLOB2`) 판정에 쓰는 YOLO 추론 서버다.

## 실행

```
run_server.cmd
```

필요한 것: Python 3.10 이상, `numpy`, `opencv-python`. `onnxruntime`은 선택이다. 설치되어 있으면 쓰고, 없으면 OpenCV DNN으로 추론한다.

HubisVIS 쪽 설정은 `Config\Config.ini` `[DeepLearning]`에서 한다.

- `AfterInspectionDL=1`
- `YOLOServerIP` / `YOLOServerPort`: 이 서버의 `host` / `port`와 같아야 한다.
- 서버는 HubisVIS가 저장한 영상 경로를 직접 읽으므로 같은 PC에서 실행한다.

## 프로토콜

연결 하나에 요청 하나를 처리한다.

- 요청: `영상경로\n`. HubisVIS는 ANSI(CP949) 경로를 보낸다. 서버는 UTF-8을 먼저 시도하고, 실패하면 ANSI로 읽는다.
- 정상 응답: `[{"class": "welding", "confidence": 0.93, "box": [x1, y1, x2, y2], "ellipse": [cx, cy, w, h, angle]}, ...]\n`
  - 박스와 타원은 원본 영상 픽셀 좌표다.
  - `ellipse`는 segmentation 모델일 때만 있다. 마스크에 맞춘 타원이며, `w`는 `angle`(도, -45~45) 방향 축 길이(=가로), `h`는 그 수직 축 길이(=세로)다. HubisVIS는 이 타원을 감싸는 회전 박스의 W/H로 판정하고, `ellipse`가 없으면 `box` 크기로 판정한다.
  - 키 순서(`class` → `confidence` → `box` → `ellipse`)는 HubisVIS 파서가 가정하므로 바꾸지 않는다. `box`가 첫 `[...]`여야 이전 HubisVIS도 그대로 읽는다.
- 실패 응답: `{"error": "<사유>"}\n`. 사유는 `image not found`, `image decode fail`, `empty request`, `request too long`, `inference error: ...` 가운데 하나다. HubisVIS는 이 응답을 YOLO 실패로 보고 `YOLOFailFallback` 정책을 따른다.

## 설정 (`server_config.json`)

| 키 | 의미 |
| --- | --- |
| `host`, `port` | 수신 주소. 기본 `127.0.0.1:9999` |
| `client_timeout_s` | 클라이언트 송수신 timeout |
| `log_dir` | 로그 폴더. 10 MB × 5개로 교체 |
| `backend` | `onnx`(실사용) / `mock`(고정 박스, 연동 시험용) |
| `model_path` | ONNX 모델 경로 (상대 경로는 설정 파일 기준) |
| `class_names` | 클래스 이름 (모델 출력 순서). Ultralytics export 모델은 비워 두면 모델 metadata에서 읽는다 |
| `image_size` | 모델 입력 크기 (letterbox) |
| `conf_threshold`, `iou_threshold` | 서버 측 최소 점수, NMS IoU. 최종 confidence 필터는 HubisVIS `YOLOConfidence`가 한 번 더 적용한다 |
| `device` | `cpu` / `cuda` |
| `mock_boxes` | `mock` backend가 돌려줄 박스 |

ONNX backend는 YOLOv8/11 검출 출력 `(1, 4+nc, N)`, YOLOv8/11 segmentation 출력(`(1, 4+nc+32, N)` + proto `(1, 32, mh, mw)`), YOLOv5 출력 `(1, N, 5+nc)`를 지원하고, NMS는 클래스별로 한다. segmentation이면 마스크 logit을 원본 해상도로 보간해 가장 큰 영역에 `cv2.fitEllipse`로 타원을 맞춘다.

## 현재 상태

- 기본 설정은 `backend: onnx`, `image_size: 1024`, `models/weld.onnx`(검수 라벨 YOLO11n-seg, 2026-11-01)다. 모델 파일은 Git에 넣지 않으므로 `../YoloTrain/train.py --deploy`로 만들거나 따로 복사한다.
- 모델 없이 연동만 시험하려면 `backend`를 `mock`으로 바꾼다.

## 테스트

```
python -m unittest test_yolo_server -v
```

후처리(좌표 복원, NMS, v5/v8 형식, clip, seg 마스크 → 타원·각도 정규화)와 프로토콜(정상, CRLF, CP949/UTF-8 한글 경로, 파일 없음, 영상 아님, 빈 요청, 긴 요청, newline 없이 종료, 32건 동시 요청)을 검사한다.
