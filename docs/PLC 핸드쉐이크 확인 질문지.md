---
title: HubisVIS PLC 핸드쉐이크 확인 질문지
tags:
  - P15
  - HubisVIS
  - PLC
  - 핸드쉐이크
status: 회신 대기
updated: 2026-10-01
---

# HubisVIS PLC 핸드쉐이크 확인 질문지

> [!info] 원본
> 공유용 원본은 Claude Docs 문서입니다: https://claude.ai/code/artifact/aad8dcee-d5bd-49b6-bb1a-88e34cc2a0b4
> 이 파일은 2026-10-01 시점 사본이며, PLC 회신은 원본 문서에 받습니다.

## 요청 배경

Vision 쪽 수정 전에 PLC 래더의 핸드쉐이크 조건 확인을 요청드립니다. 각 표의 **PLC 회신** 칸에 직접 적어 주시면 됩니다.

- 08-25 09:40, 08-26 10:49에 시퀀스가 멈춰 운전자가 Vision 프로그램을 재시작했습니다.
- 두 번 모두 진행 순서가 같았습니다. Vision이 `REQ_Insp_Move`를 켰는데 `RESP_Insp_Move`가 오지 않았고, PLC는 다음 Start를 바로 올렸습니다. 그다음 사이클의 Move 요청에는 PLC가 142초, 24.5초 동안 응답하지 않았습니다.
- 같은 기간 나머지 4,758 사이클은 정상이었고, 통신 timeout이나 재접속은 한 건도 없었습니다.
- Vision에는 비트 하나를 쓰면 같은 워드의 다른 비트가 0으로 덮어써지는 결함이 있습니다. 이 결함을 고치면 PLC가 받는 신호가 지금과 달라지므로 PLC 동작부터 확인해야 합니다.

시각은 Vision PC 로그 기준입니다. Vision 기록 시점과 PLC 시각은 수십 ms 차이가 날 수 있습니다.

## 1. 신호 맵 확인

Vision의 `PLCMap.ini` 기준 주소입니다. PLC 측 정의와 다르거나, 래더에서 쓰지 않는 신호는 표시해 주세요.

| 주소 | 방향 | 신호 | PLC에서 사용 여부·용도 |
| --- | --- | --- | --- |
| D30300.0 | PLC→Vision | LinkTest | |
| D30300.1 | PLC→Vision | REQ_Initialize | |
| D30301.1 | PLC→Vision | REQ_Insp_Start | |
| D30301.4 | PLC→Vision | RESP_Insp_Move | |
| D30200.0 | Vision→PLC | LinkTest | |
| D30200.1 | Vision→PLC | RESP_Initialize | |
| D30200.2 | Vision→PLC | RESP_TimeSync | |
| D30200.3 | Vision→PLC | RESP_ReadyInspection | |
| D30200.4~.6 | Vision→PLC | ALARM, Auto, Manual | |
| D30201.0 | Vision→PLC | RESP_Insp_InitScanPos | |
| D30201.1 | Vision→PLC | RESP_Insp_Start | |
| D30201.2 / .3 | Vision→PLC | RESP_Insp_End / RESP_Insp_Stop | |
| D30201.4 | Vision→PLC | REQ_Insp_Move | |
| D30210 | Vision→PLC | InspRet (0 초기화, 1 OK, 2 NG) | |
| D30211 | Vision→PLC | JobNumber | |
| D31000~ | Vision→PLC | ResultData (double word) | |

참고로 Vision은 현재 RESP_TimeSync, ALARM, Auto, Manual, RESP_Insp_InitScanPos, RESP_Insp_End, RESP_Insp_Stop을 켜지 않습니다.

## 2. Move 핸드쉐이크 (가장 중요)

정상 4,758 사이클에서 PLC의 `REQ_Insp_Start OFF`와 `RESP_Insp_Move ON`은 항상 같은 읽기에서 함께 보였습니다. Move ON에서 응답까지는 중앙값 119 ms, 최대 177 ms였습니다. 정지 사건에서는 Start OFF는 왔지만 Move 응답이 없었습니다.

| No. | 질문 | Vision 측 현재 동작 | PLC 회신 |
| --- | --- | --- | --- |
| M1 | `RESP_Insp_Move`를 켜는 조건은 무엇입니까? (이동 완료, 결과 수신, 기타 인터록) | REQ_Move ON 후 RESP_Move를 기다림 | |
| M2 | `RESP_Insp_Move`는 Vision이 `REQ_Insp_Move`를 끌 때까지 유지합니까, 아니면 타이머·단일 스캔 펄스입니까? 펄스라면 길이는? | RESP를 본 뒤 REQ_Move OFF. 스캔 직후 88~96 ms는 PLC를 읽지 못함 | |
| M3 | `REQ_Insp_Move`를 상승 엣지로 받습니까, ON 상태(레벨)로 받습니까? | 사이클마다 ON | |
| M4 | Move 응답을 보내지 않고 다음 `REQ_Insp_Start`를 올리는 경로가 있습니까? 있다면 어떤 조건입니까? | 사건에서 Move ON 441 ms, 452 ms 뒤 다음 Start 수신 (정상 범위 441~692 ms) | |
| M5 | 응답 전에 `REQ_Insp_Move`가 OFF됐다가 약 1.2초 뒤 다시 ON되면 PLC 시퀀스는 어떻게 됩니까? | 사건에서 다음 Start 응답을 쓸 때 REQ_Move 비트가 같이 지워짐 (의도하지 않은 동작) | |
| M6 | Move 대기 중 timeout이나 알람이 있습니까? 있다면 시간과 복구 절차는? | Vision에는 timeout이 없음 | |
| M7 | Vision이 Move 미응답 시 REQ_Move를 끄고 다시 켜는 재시도를 해도 됩니까? 허용되는 대기 시간은? | 검토 중인 개선안 | |

## 3. Start 핸드쉐이크

Vision은 `REQ_Insp_Start`의 상승 엣지에서 JobNumber와 `RESP_Insp_Start ON`을 쓰고, 50 ms 뒤 무조건 OFF 합니다. 실측상 PLC는 RESP_Start가 아니라 Move 요청을 본 뒤(중앙값 118 ms) REQ_Start를 내렸습니다.

| No. | 질문 | Vision 측 현재 동작 | PLC 회신 |
| --- | --- | --- | --- |
| S1 | 래더가 `RESP_Insp_Start`를 사용합니까? 사용한다면 어느 단계의 조건입니까? | ON 후 50 ms 뒤 OFF, REQ_Start OFF는 확인하지 않음 | |
| S2 | `REQ_Insp_Start`를 끄는 조건은 무엇입니까? (REQ_Move ON, RESP_Start, 타이머 등) | 실측 ON 유지 중앙값 1,375 ms | |
| S3 | Vision 신호가 인식되려면 최소 몇 ms를 유지해야 합니까? (PLC 스캔 주기, MC 프로토콜 처리 시점 포함) | 50 ms 고정 | |
| S4 | 4-phase(REQ ON → RESP ON 유지 → REQ OFF 확인 → RESP OFF)로 바꿔도 래더가 동작합니까? | 검토 중인 개선안 | |
| S5 | Vision이 스캔 중일 때 Start가 오면 어떻게 알려주길 원하십니까? (Busy 비트, 무시, 대기) | 스캔 중에는 PLC를 읽지 않음 (약 1.1초) | |

## 4. 검사 결과 읽기 시점

현재 Vision은 검사 결과보다 Move 요청을 먼저 보냅니다. 4,760 사이클 중 4,756 사이클에서 마지막 InspRet 기록이 REQ_Move ON보다 평균 46 ms(최대 168 ms) 늦었고, 28 사이클은 PLC가 Move에 응답한 뒤에 기록됐습니다.

| No. | 질문 | Vision 측 현재 동작 | PLC 회신 |
| --- | --- | --- | --- |
| R1 | PLC는 InspRet(D30210)과 ResultData(D31000~)를 언제 읽습니까? (REQ_Move ON 순간, 이동 후, 제품 배출 시 등) | 카메라별로 검사가 끝날 때마다 기록 | |
| R2 | InspRet이 0일 때 PLC는 어떻게 처리합니까? | Start를 받으면 먼저 0으로 초기화 | |
| R3 | InspRet은 두 카메라를 합친 값이어야 합니까, 카메라별 값이어도 됩니까? | 카메라별로 덮어씀 (마지막 카메라 값이 남음) | |
| R4 | 검사 결과가 비었을 때 어떤 값을 받기를 원하십니까? | 아무것도 쓰지 않음 (189 사이클). 이전 제품의 ResultData가 남음 | |
| R5 | "결과 기록 완료 후 Move 요청" 순서로 바꾸면 사이클이 최대 약 170 ms 느려질 수 있습니다. 허용됩니까? | 검토 중인 개선안 | |

## 5. Ready · Initialize · LinkTest · 실패 통보

Vision은 LinkTest를 500 ms마다 토글합니다. 이때 같은 워드 D30200의 다른 비트가 모두 0으로 쓰여서, 운전 중 `RESP_ReadyInspection`은 사실상 꺼져 있습니다. 이 결함을 고치면 Ready가 계속 ON으로 유지되므로 아래 답변이 필요합니다.

| No. | 질문 | Vision 측 현재 동작 | PLC 회신 |
| --- | --- | --- | --- |
| L1 | `RESP_ReadyInspection`을 인터록에 씁니까? 쓴다면 Ready가 꺼진 지금 어떻게 동작하고 있습니까? | 시작 시 1회 ON, 약 500 ms 뒤 LinkTest가 지움 | |
| L2 | Ready가 항상 ON으로 유지되면 래더 동작이 바뀝니까? | 수정 후 예상 동작 | |
| L3 | `RESP_Initialize`는 언제 OFF되기를 원하십니까? (REQ_Initialize OFF 후, 타이머 등) | ON만 하고 OFF 코드는 주석 처리. LinkTest가 우연히 지움 | |
| L4 | REQ_Initialize는 얼마나 오래 ON을 유지합니까? | ON인 동안 루프마다 ResultData 182개를 다시 씀 (루프 1회 약 3초) | |
| L5 | Vision LinkTest를 감시하는 watchdog 시간은 몇 초입니까? 끊김으로 판단하면 어떤 동작을 합니까? | 스캔 중에는 토글이 멈춤 (정상 약 1.1초, 실패 시 최대 약 8초) | |
| L6 | RESP_TimeSync, ALARM, Auto, Manual, RESP_Insp_InitScanPos, RESP_Insp_End, RESP_Insp_Stop 중 PLC가 기다리는 신호가 있습니까? | 모두 켜지 않음 | |
| L7 | 스캔 실패를 PLC에 어떻게 알리기를 원하십니까? (ALARM, NG 결과, 전용 비트 등) | 현재 실패 신호가 PLC에 도달하지 않음 (기간 중 실패 0건) | |

## 6. 사건 이력과 PLC 정보

아래 구간의 PLC 알람, 시퀀스 스텝, 운전자 조작 이력이 남아 있으면 공유해 주세요.

| 구간 (Vision PC 시각) | Vision 로그에서 본 내용 | PLC 알람·스텝·조작 이력 |
| --- | --- | --- |
| 2026-08-25 09:40:42 ~ 09:43:24 | 09:40:42.693 Move ON 후 응답 없음 → 09:40:43.134 다음 Start(Job 2, Div 2) → 09:40:44.358 Move ON → 09:43:06 Move 응답(Job 1, Div 0) → 09:43:24 Vision 종료 | |
| 2026-08-26 10:49:21 ~ 10:50:06 | 10:49:21.734 Move ON 후 응답 없음 → 10:49:22.186 다음 Start(Job 1, Div 1) → 10:49:23.419 Move ON, REQ_Start를 5.9초 더 유지 → 10:49:47 Move 응답(Div 2) → 10:50:06 Vision 종료 | |

| 항목 | PLC 회신 |
| --- | --- |
| PLC 기종 (Q / L / iQ-R, CPU 형명) | |
| 평균·최대 스캔 타임 (ms) | |
| MC 프로토콜 포트 설정 (8106, 3E 바이너리, TCP) 및 동시 접속 수 | |
| D 레지스터 비트 단위 쓰기 지원 여부 | |
| 회신 담당자·회신일 | |
