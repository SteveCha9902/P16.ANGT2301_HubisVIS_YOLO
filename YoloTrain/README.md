# 후검사 YOLO 학습

HubisVIS 후검사용 welding / spatter 모델을 Ultralytics YOLO11 **segmentation**으로 학습하고, ONNX로 export해 `../YoloServer`에서 쓴다.

라벨은 **타원**이다(2026-11-01 결정). 모델이 비드 영역(마스크)을 찾으면 서버가 마스크에 타원을 맞추고, 타원을 감싸는 **회전 박스의 가로(W)·세로(H)**로 HubisVIS가 크기를 판정한다.

> **라이선스:** Ultralytics는 AGPL-3.0이다. 학습한 모델을 고객 라인에 비공개로 배포하려면 Ultralytics Enterprise 라이선스가 필요할 수 있다. 현장 배포 전에 회사 판단을 받는다.

## 환경

```
python -m venv .venv
.venv\Scripts\python -m pip install torch torchvision --index-url https://download.pytorch.org/whl/cu124
.venv\Scripts\python -m pip install ultralytics onnx onnxslim onnxruntime
```

## 라벨 형식 (`ellipse_labels.py`)

`data/ellipses/<이름>.txt`, 한 줄에 타원 하나: `class cx cy w h angle` (픽셀, 각도는 도)

- `w`: 기울기 `angle` 방향 축의 전체 길이, `h`: 그 수직 축의 전체 길이
- `angle`은 -45°~45°로 정규화한다. 그래서 `w`는 항상 영상 가로축에 가까운 축(=가로), `h`는 세로다. HubisVIS 레시피의 Width / Height와 같은 의미다.
- `data/labels/`는 학습용으로 `train.py`가 타원에서 다시 만든다(타원 둘레 36점 다각형). 직접 고치지 않는다.

## 순서

1. **데이터셋과 라벨 초안:** `python make_draft_labels.py` → `python convert_box_labels.py`
   - 현장 영상(P15 Line `26.1014_borg`, `D:\Hubis Data`)을 `data/images/*.png`로 복사한다.
   - After 영상은 카메라별 템플릿 매칭으로 welding 박스 초안을 만든다.
   - Before 영상(용접 전)은 3장마다 1장을 빈 라벨(배경)로 넣는다.
   - `convert_box_labels.py`는 박스 초안을 박스에 내접하는 타원(기울기 0)으로 바꾼다. 원래 박스 라벨은 `data/labels_box/`에 백업한다.
2. **검수:** `.venv\Scripts\python review_labels.py`
   - 타원을 비드 외곽에 맞추고(이동·축 길이·회전), spatter를 추가하고, 빠진 비드를 보완한다. 조작법은 파일 머리말에 있다.
   - 점선 사각형이 판정에 쓰는 회전 박스다. 선택한 타원의 W/H/각도는 아래 상태줄에 나온다.
   - 라벨 기준 (2026-11-01 결정)
     - welding: 비드 하나당 타원 하나. 비드 끝에 붙은 덩어리는 비드 타원에 포함한다.
     - spatter: 비드 경계 밖에 떨어져 있는 구형 금속 방울만 표시한다. 비드에 붙은 덩어리는 spatter로 표시하지 않는다.
   - 검수를 마친 영상은 `data/reviewed.txt`에 기록된다.
   - **spatter 추가 라벨링(후속):** 비드 검수가 끝난 뒤 `python review_labels.py --all`로 다시 돌면서 비드 밖 방울만 표시한다.
     `2`로 spatter 클래스를 고르고 방울을 **클릭**하면 지름 24 px 원이 생긴다. `[` / `]`로 크기, 드래그로 위치를 맞춘다.
     비드 안을 클릭하면 비드가 선택될 뿐 spatter가 생기지 않는다. 중심이 비드 안에 있는 spatter는 상태줄에 경고가 나온다.
     spatter 라벨이 생긴 뒤 다시 학습하면 서버가 spatter도 내고, HubisVIS `YOLOSpatterNG=1`로 판정에 쓸 수 있다.
3. **학습:** `.venv\Scripts\python train.py --deploy`
   - `data/ellipses`로 `data/labels`(seg 다각형)를 다시 만들고, 시료(바코드) 단위로 train/val을 8:2로 나눈다.
   - 학습 후 ONNX로 export하고, 서버와 같은 `OnnxYoloDetector`로 검증셋을 평가해 `runs/<name>/server_eval.txt`에 남긴다.
     검출 정밀도·재현율과 함께 타원 W/H(픽셀)·각도 오차를 낸다.
   - `--deploy`를 주면 `../YoloServer/models/weld.onnx`로 복사한다. 이후 `server_config.json`의 `backend`를 `onnx`, `image_size`를 학습 크기(기본 1024)로 바꾼다.
   - 기본은 검수한 영상만 쓴다. `--include-unreviewed`를 주면 초안까지 쓴다(파이프라인 시험용).

`data/`, `runs/`, `.venv/`는 Git에 넣지 않는다(현장 영상·대용량).
