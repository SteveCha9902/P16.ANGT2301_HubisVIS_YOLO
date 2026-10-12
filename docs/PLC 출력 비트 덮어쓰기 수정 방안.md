---
title: PLC 출력 비트 덮어쓰기 수정 방안
tags:
  - P15
  - HubisVIS
  - PLC
  - 핸드쉐이크
status: 승인 대기 (미적용)
updated: 2026-10-01
---

# PLC 출력 비트 덮어쓰기 수정 방안

> [!warning] 상태
> 제안 단계입니다. 소스는 변경하지 않았습니다. PLC 래더 확인([[PLC 핸드쉐이크 확인 질문지]])과 사용자 승인 후 적용합니다.

## 문제

`CCtrlPLC::WriteData()`(`AlgorithmTools/CtrlPLC.cpp:233`)는 BIT 신호를 쓸 때 `buffer = 0`에서 대상 비트만 세운 뒤 D 레지스터 워드 1개를 통째로 씁니다. 그래서 같은 워드의 다른 출력 비트가 모두 0이 됩니다.

- D30201: `RESP_Insp_Start ON`을 쓰면 `REQ_Insp_Move`가 꺼집니다. 응답받지 못한 Move 요청이 다음 사이클 Start 응답 때 기록 없이 사라집니다(정지 사건 2건에서 발생).
- D30200: 500 ms마다 LinkTest를 쓰면 `RESP_ReadyInspection`(시작 시 1회 ON), `RESP_Initialize`, ALARM, Auto, Manual이 꺼집니다.
- 스캔 실패 경로(`PLCSocketManager.cpp:575-578`)는 RESP_Start ON 직후 REQ_Move OFF를 써서 RESP_Start가 바로 0이 됩니다.

## 확인한 사실

- D30200, D30201의 출력 신호 12개는 모두 BIT 타입입니다(`PLCMap.ini` Write_0~11).
- 이 비트들을 개별로 쓰는 곳은 PLC 폴링 스레드뿐입니다(LinkTest 포함). 검사 스레드는 InspRet/JobNumber(INT 워드)만 씁니다. 그 밖에는 시작 시 UI의 일괄 쓰기(`WriteVision2PLCData`, `DlgMain2.cpp:279`)가 있습니다.
- `WriteData()`는 이미 `data.sValue = value`로 각 신호의 현재 값을 `vecsVision2PLCData`에 저장하고 있습니다. 문제는 전송 워드를 만들 때 이 저장값을 쓰지 않는다는 점입니다.

## 권장안: 저장된 비트 값으로 워드 전체를 다시 만들어 쓰기

BIT 신호를 쓸 때 같은 주소의 다른 출력 비트 저장값까지 모아 워드를 만든 뒤 씁니다. 변경 위치는 `CCtrlPLC::WriteData()` 하나입니다.

```cpp
// 같은 워드를 공유하는 출력 비트를 보존하기 위한 쓰기 직렬화 (헤더 변경 없이 파일 내부에 둠)
static CCriticalSection s_csBitWordWrite;

BOOL CCtrlPLC::WriteData(SPLCData &data, const short value)
{
	short buffer(0);

	data.sValue = value;

	if(data.nType == (long)PLC_MEMORY_TYPE::BIT)
	{
		CSingleLock lock(&s_csBitWordWrite, TRUE);

		// 같은 주소의 다른 출력 비트 현재값을 반영한다
		for(const auto &bit : ::g_PLCManager.m_sPLC.vecsVision2PLCData)
		{
			if(bit.nType == (long)PLC_MEMORY_TYPE::BIT && bit.nAddr == data.nAddr && bit.sValue == 1)
				buffer |= (short)(1 << bit.nBitNum);
		}
		SetPLCData(data, buffer);	// 대상 비트는 인자 값으로 확정 (복사본이 넘어와도 안전)

		return m_pMelsec->WritePLC(data.nAddr, 1, &buffer);
	}

	SetPLCData(data, buffer);
	// ...기존 코드 그대로
}
```

- 수정 범위가 한 함수입니다. 호출부, PLCMap 주소, 전송 데이터 형식은 그대로입니다.
- 락을 파일 내부 static으로 두어 DLL export 클래스 `CCtrlPLC`의 헤더 구조를 바꾸지 않습니다.
- 통신 횟수는 늘지 않습니다.
- `WriteVision2PLCData()`는 이미 저장값으로 워드를 만들므로 규칙이 일치합니다. 같은 락을 걸어 둡니다.

## 수정 후 PLC가 보는 신호 변화 (결정 필요)

| 신호 | 현재 실제 동작 | 수정 후 | 필요한 결정 |
| --- | --- | --- | --- |
| RESP_ReadyInspection | 시작 후 약 500 ms 안에 꺼지고 계속 OFF | 계속 ON | 래더가 Ready를 인터록에 쓰는지 (질문지 L1, L2) |
| RESP_Initialize | ON 후 LinkTest가 지움 | 영원히 ON (OFF 코드 주석 처리됨) | REQ_Initialize OFF 시 RESP OFF 추가 여부 (L3) |
| REQ_Insp_Move (미응답) | 다음 Start 응답 쓰기 때 몰래 OFF | 다음 사이클까지 ON, 새 요청 때 상승 엣지 없음 | 새 Start 시 남은 Move를 명시적으로 OFF + 로그 (M3, M5) |
| RESP_Insp_Start (스캔 실패) | 바로 지워져 PLC가 못 봄 | 다음 Start까지 ON | 실패 통보 방식 (L7) |

REQ_Insp_Move 줄이 특히 중요합니다. 비트만 고치면 지금 사건에서 우연히 생기던 "OFF 후 다시 ON"이 없어집니다. PLC가 상승 엣지로 Move를 받으면 오히려 매번 정지할 수 있습니다. 그래서 새 Start를 받았을 때 미응답 Move가 남아 있으면 OFF를 명시적으로 쓰고 `[PLC_MOVE_DIAG][WARN]` 로그를 남기는 처리를 함께 넣는 것을 권장합니다. 현재의 실제 동작은 유지하면서 기록이 남습니다.

## 검토했지만 권하지 않는 방법

- PLC에서 워드를 읽어 고쳐 쓰기: 비트 쓰기마다 통신이 한 번 더(약 16 ms) 늘고, Vision 전용 영역이라 읽을 이유가 없습니다.
- MC 프로토콜 비트 단위 쓰기: D 레지스터 비트 직접 쓰기 지원 여부가 PLC 기종에 따라 달라 확인이 필요하고, 통신 코드를 새로 작성해야 합니다.
- PLC에서 신호별로 워드 분리: 래더와 PLCMap을 함께 바꿔야 해서 범위가 큽니다.

## 적용 순서

1. PLC 담당자 회신 확보(질문지 M3, M5, L1~L3, L7).
2. 승인 후 `WriteData` 수정, 미응답 Move 명시적 OFF + 로그, `RESP_Initialize` OFF 처리 반영. `CtrlPLC.cpp`, `PLCSocketManager.cpp`는 현재 CP949 → 편집 시 UTF-8 with BOM 변환, 기존 깨진 주석은 유지.
3. `HubisVIS.sln` Debug|x64 빌드, PLC 없이 PLC 모니터링 화면과 로그로 워드 값 확인(LinkTest 토글 중 Ready bit 3 유지 여부).
4. PLC 연동 시험은 별도 승인 후 진행.
