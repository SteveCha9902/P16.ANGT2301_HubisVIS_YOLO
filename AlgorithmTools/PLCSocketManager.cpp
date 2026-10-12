#include "pch.h"
#include "PLCSocketManager.h"
#include "../VisionMain/DlgVisionMain.h"

#include "CtrlPLC.h"

#define BUFFER_SIZE 10000
#define COUNT_MAX 5
#define PLC_START_DIAG_DELAY_WARN_MS 500
#define PLC_UI_SEND_TIMEOUT_MS 1000
#define PLC_SCAN_READY_WAIT_TIMEOUT_MS 1000
#define PLC_START_RESP_MIN_HOLD_MS 100		// PLC 회신 S3: Vision 신호 최소 유지 시간
#define PLC_START_REQ_OFF_WAIT_MS 1000		// RESP_Insp_Start ON 후 REQ_Insp_Start OFF 확인 대기 상한
#define PLC_MOVE_RESP_WARN_MS 500			// REQ_Insp_Move ON 후 RESP 미수신 경고 (정상 최대 약 177 ms)
#define PLC_MOVE_RESP_ALARM_MS 5000			// REQ_Insp_Move ON 후 RESP 미수신 시 ALARM ON
#define PLC_WAIT_SLICE_MS 50				// 스캔 대기 중 LinkTest 처리 간격

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPLCSocketManager g_PLCSocketManager;

// PLC 핸드쉐이크 상태. PLC 스레드(g_PLCSocketManager 단일 인스턴스) 전용이며,
// DLL export 클래스의 레이아웃을 바꾸지 않기 위해 헤더 대신 파일 내부에 둔다.
static BOOL s_bStartRespOn = FALSE;		// RESP_Insp_Start ON 유지 중 (REQ_Insp_Start OFF 확인 후 OFF)
static DWORD s_dwStartRespOnTick = 0;
static BOOL s_bMoveRespWarned = FALSE;	// 현재 Move 요청에 대해 미응답 경고를 남겼는지
static BOOL s_bMoveAlarmOn = FALSE;		// Move 미응답으로 ALARM을 켰는지
static BOOL s_bInitRespOn = FALSE;		// RESP_Initialize ON 유지 중 (REQ_Initialize OFF 확인 후 OFF)

// UI 타이머가 요청한 LinkTest 값을 PLC에 쓴다.
// 스캔 대기 중에도 호출해 PLC의 LinkTest 감시(5 s)에 걸리지 않게 한다.
static void ServicePendingLinkTest(CPLCSocketManager *pManager)
{
	LONG nPendingLinkTest = InterlockedExchange(&pManager->m_nPendingLinkTestValue, -1);
	if(nPendingLinkTest < 0)
	{
		return;
	}

	auto &linkData = ::g_PLCManager.m_sPLC.vecsVision2PLCData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_LinkTest];
	DWORD dwLinkWriteTick = GetTickCount();
	BOOL bLinkWriteRet = pManager->m_pCtrlPLC->WriteData(linkData, (short)nPendingLinkTest);
	DWORD dwLinkWriteElapsed = GetTickCount() - dwLinkWriteTick;
	if(bLinkWriteRet == FALSE || dwLinkWriteElapsed >= PLC_START_DIAG_DELAY_WARN_MS)
	{
		::g_Log.Log(_T("[PLC_LINK_DIAG][WARN] LinkTest write Value:%d Ret:%d Elapsed:%lu ms"), nPendingLinkTest, bLinkWriteRet, (unsigned long)dwLinkWriteElapsed);
	}
}

// 핸들을 기다리는 동안(핸들이 없으면 단순 대기) LinkTest 쓰기를 계속 처리한다.
// 반환값은 WaitForMultipleObjects와 같고, 시간 초과 시 WAIT_TIMEOUT을 반환한다.
static DWORD WaitWithLinkTest(CPLCSocketManager *pManager, DWORD nCount, const HANDLE *pHandles, BOOL bWaitAll, DWORD dwTimeoutMs)
{
	const DWORD dwBeginTick = GetTickCount();
	while(1)
	{
		DWORD dwElapsed = GetTickCount() - dwBeginTick;
		DWORD dwRemain = (dwElapsed >= dwTimeoutMs) ? 0 : (dwTimeoutMs - dwElapsed);
		DWORD dwSlice = (dwRemain < PLC_WAIT_SLICE_MS) ? dwRemain : PLC_WAIT_SLICE_MS;

		DWORD dwRet = WAIT_TIMEOUT;
		if(nCount > 0)
		{
			dwRet = ::WaitForMultipleObjects(nCount, pHandles, bWaitAll, dwSlice);
		}
		else
		{
			Sleep(dwSlice);
		}

		if(dwRet != WAIT_TIMEOUT || GetTickCount() - dwBeginTick >= dwTimeoutMs)
		{
			return dwRet;
		}

		ServicePendingLinkTest(pManager);
	}
}

CPLCSocketManager::CPLCSocketManager()
	: m_pParent(nullptr)
	, m_strBarcode(_T(""))
	, m_strModel(_T(""))
	, m_nCameraIndex(0)
	, m_nDivIndex(0)
	, m_bStart(FALSE)
	, m_nJobNumber(0)
	, m_bScanDone(FALSE)
	, m_bInitialize(FALSE)
	, m_bPrevStartSignal(FALSE)
	, m_nPendingLinkTestValue(-1)
	, m_dwStartSignalOnTick(0)
	, m_nStartSignalSeq(0)
	, m_bPrevMoveRespSignal(FALSE)
	, m_bMoveReqOn(FALSE)
	, m_dwMoveReqOnTick(0)
	, m_dwMoveRespOnTick(0)

	, m_strScanTime(_T(""))
	, m_ScanCount(0)
{
	m_bTryingServerConnect = FALSE;
	m_bConnect = FALSE;
}

CPLCSocketManager::~CPLCSocketManager()
{
	delete m_pCtrlPLC;
}

void CPLCSocketManager::InitThread()
{
	// Client Thread 초기화
	m_Control_Client.m_hSignal_Start = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_Client.m_hSignal_Kill = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_Client.m_Handle_Thread = INVALID_HANDLE_VALUE;

	HANDLE hClientThread = ::AfxBeginThread(ThreadClientFunc, (void *)this)->m_hThread;
	::DuplicateHandle(::GetCurrentProcess(), hClientThread, ::GetCurrentProcess(), &m_Control_Client.m_Handle_Thread, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);
}

void CPLCSocketManager::FreeThread()
{

	// 모든 쓰레드에게 일을 그만하라고 알린다.
	::SetEvent(m_Control_Client.m_hSignal_Kill);
	if(m_pCtrlPLC != nullptr)
	{
		m_pCtrlPLC->Disconnect();
	}

	// 쓰레드가 종료되기를 기다린다.
	DWORD dwWaitRet = ::WaitForSingleObject(m_Control_Client.m_Handle_Thread, 5000);
	if(dwWaitRet != WAIT_OBJECT_0)
	{
		::g_Log.Log(_T("[THREAD_DIAG][ERROR] PLC thread stop timeout - Ret:%lu"), dwWaitRet);
	}

	// 관련 핸들을 모두 닫는다.
	::CloseHandle(m_Control_Client.m_hSignal_Kill);
	::CloseHandle(m_Control_Client.m_Handle_Thread);
	::CloseHandle(m_Control_Client.m_hSignal_Start);
}

void CPLCSocketManager::SetParent(CDlgVisionMain* pParent)
{
	m_pParent = pParent;

	m_pCtrlPLC = new CCtrlPLC;
	m_pCtrlPLC->Initialize();
}

// 클라이언트 스레드 함수
UINT CPLCSocketManager::ThreadClientFunc(LPVOID pParam)
{
	CPLCSocketManager *pProcessClientThread = (CPLCSocketManager *)pParam;

	pProcessClientThread->ThreadClient();

	return 0;
}

// 클라이언트 스레드
void CPLCSocketManager::ThreadClient()
{
	// 클라이언트 Thread Handle
	HANDLE pHandles[2] = {m_Control_Client.m_hSignal_Start, m_Control_Client.m_hSignal_Kill};

	::g_Log.Log(_T("PLC Client Thread : Thread Ready"));

	while(1)
	{
		DWORD dwRet = ::WaitForMultipleObjects(2, pHandles, FALSE, INFINITE);

		// 클라이언트 시작
		if(dwRet == WAIT_OBJECT_0)
		{
			m_bTryingServerConnect = TRUE;

			if(m_pCtrlPLC->Connect() == FALSE)
			{
				m_bConnect = FALSE;
				m_bTryingServerConnect = FALSE;

				::g_Log.Log(_T("PLC Client Thread : Attempt to connect to %s:%d failed, server not found."), ::g_ConfigManager.m_strPLCIP, ::g_ConfigManager.m_nPLCPort);

				// 아이콘 상태 변경
				::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_CTRL_STATUS, 0, FALSE);
			}
			else
			{
				m_bConnect = TRUE;
				m_pParent->KillTimer(CTRL_CONNECT);

				::g_Log.Log(_T("PLC Client Thread : Connected to %s:%d."), ::g_ConfigManager.m_strPLCIP, ::g_ConfigManager.m_nPLCPort);

				// 아이콘 상태 변경
				::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_CTRL_STATUS, 0, TRUE);

				// PLC와 데이터 통신
				while (1)
				{
					BOOL bRet = m_pCtrlPLC->ReadPLC2VisionData();
					if(bRet == FALSE)
					{
						::g_Log.Log(_T("[PLC_START_DIAG] ReadPLC2VisionData Fail - Last REQ[30301.1]:%d, Seq:%d, Job:%d, Cam:%d, Div:%d"), m_bStart, m_nStartSignalSeq, m_nJobNumber, m_nCameraIndex, m_nDivIndex);
					}
					MessageProcess();
					if(bRet != FALSE)
					{
						ServicePendingLinkTest(this);
					}

					if (bRet == FALSE)
					{
						//읽기 실패
						m_bConnect = FALSE;
						m_bTryingServerConnect = FALSE;

						// 접속이 끊겼을 경우 다시 타이머를 시작해서 서버를 찾는다.
						m_pParent->SetTimer(CTRL_CONNECT, ::g_ConfigManager.m_nPLCAutoConnectTimes, NULL);

						// 아이콘 상태 변경
						::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_CTRL_STATUS, 0, FALSE);

						break;
					}

					Sleep(10);
				}
			}
		}
		else
		{
			m_bConnect = FALSE;
			break;
		}
	}
}

void CPLCSocketManager::MessageProcess()
{
	auto &plcData = ::g_PLCManager.m_sPLC.vecsVision2PLCData;
	const BOOL bCurStartSignal = m_bStart;
	const BOOL bStartRising = (bCurStartSignal == TRUE && m_bPrevStartSignal == FALSE);
	SStartSignalContext startContext;

	if(bStartRising == TRUE)
	{
		++m_nStartSignalSeq;
		m_dwStartSignalOnTick = GetTickCount();

		startContext.seq = m_nStartSignalSeq;
		startContext.jobNumber = m_nJobNumber;
		startContext.cameraIndex = m_nCameraIndex;
		startContext.divIndex = m_nDivIndex;
		startContext.barcode = m_strBarcode;
		startContext.model = m_strModel;
		startContext.reqOnTick = m_dwStartSignalOnTick;

		::g_Log.Log(_T("[PLC_START_DIAG] PLC -> Vision::Read [address 30301.1] PLC_DI_REQ_Insp_Start : ON, Seq:%d, Job:%d, Cam:%d, Div:%d, Barcode:%s, Model:%s"), startContext.seq, startContext.jobNumber, startContext.cameraIndex, startContext.divIndex, startContext.barcode, startContext.model);

	}
	else if(bCurStartSignal == FALSE && m_bPrevStartSignal == TRUE)
	{
		DWORD dwOnElapsed = (m_dwStartSignalOnTick == 0) ? 0 : (GetTickCount() - m_dwStartSignalOnTick);
		::g_Log.Log(_T("[PLC_START_DIAG] PLC -> Vision::Read [address 30301.1] PLC_DI_REQ_Insp_Start : OFF, Seq:%d, ON elapsed:%lu ms"), m_nStartSignalSeq, (unsigned long)dwOnElapsed);
	}


	m_bPrevStartSignal = bCurStartSignal;

	const BOOL bCurMoveRespSignal = m_bScanDone;
	if(bCurMoveRespSignal == TRUE && m_bPrevMoveRespSignal == FALSE)
	{
		m_dwMoveRespOnTick = GetTickCount();
		DWORD dwReqOnElapsed = (m_dwMoveReqOnTick == 0) ? 0 : (m_dwMoveRespOnTick - m_dwMoveReqOnTick);
		::g_Log.Log(_T("[PLC_MOVE_DIAG] Seq:%d Read 30301.4 PLC_DI_RESP_Insp_Move ON reqOn:%lu ms Job:%d Cam:%d Div:%d"), m_nStartSignalSeq, (unsigned long)dwReqOnElapsed, m_nJobNumber, m_nCameraIndex, m_nDivIndex);
	}
	else if(bCurMoveRespSignal == FALSE && m_bPrevMoveRespSignal == TRUE)
	{
		DWORD dwRespOnElapsed = (m_dwMoveRespOnTick == 0) ? 0 : (GetTickCount() - m_dwMoveRespOnTick);
		::g_Log.Log(_T("[PLC_MOVE_DIAG] Seq:%d Read 30301.4 PLC_DI_RESP_Insp_Move OFF on:%lu ms"), m_nStartSignalSeq, (unsigned long)dwRespOnElapsed);
	}
	m_bPrevMoveRespSignal = bCurMoveRespSignal;

	switch(::g_ConfigManager.m_nCustomer)
	{
	case (long)CUSTOMER::BORGWARNER:
	{
		// Get Data : 필요한 부분 추가 필요
		// ::g_PLCManager.GetReadData();  // 251029 sjpark 중복 불러오기 이슈가 있어보여서 일단 주석처리

		// Start 4-phase: 대기 상한 안에 REQ OFF를 못 본 경우, 이후 폴링에서 REQ OFF를 확인하면 RESP를 끈다
		if(s_bStartRespOn == TRUE && bCurStartSignal == FALSE)
		{
			BOOL bRespOffRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::OFF);
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d REQ[30301.1] OFF confirmed late - Write [address 30201.1] PLC_DO_RESP_Insp_Start OFF ret:%d hold:%lu ms"), m_nStartSignalSeq, bRespOffRet, (unsigned long)(GetTickCount() - s_dwStartRespOnTick));
			if(bRespOffRet != FALSE)
			{
				s_bStartRespOn = FALSE;
			}
		}

		// ScanStart
		if(bStartRising == TRUE)
		{
			// 이전 Start의 RESP가 남아 있으면 먼저 끄고 새로 켠다 (REQ OFF를 관측하지 못한 경우)
			if(s_bStartRespOn == TRUE)
			{
				BOOL bRespOffRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::OFF);
				::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d Previous RESP_Insp_Start still ON at new Start - Write OFF ret:%d"), startContext.seq, bRespOffRet);
				s_bStartRespOn = FALSE;
			}

			// 응답받지 못한 Move 요청이 남아 있으면 경고를 남기고 명시적으로 끈다
			if(m_bMoveReqOn == TRUE)
			{
				DWORD dwMovePending = (m_dwMoveReqOnTick == 0) ? 0 : (GetTickCount() - m_dwMoveReqOnTick);
				BOOL bMoveOffRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_REQ_Insp_Move], (long)BORGWARNER::DATA::OFF);
				::g_Log.Log(_T("[PLC_MOVE_DIAG][WARN] Seq:%d New Start while REQ_Insp_Move pending without RESP (%lu ms) - Write 30201.4 PLC_DO_REQ_Insp_Move OFF ret:%d"), startContext.seq, (unsigned long)dwMovePending, bMoveOffRet);
				if(bMoveOffRet != FALSE)
				{
					m_bMoveReqOn = FALSE;
					m_dwMoveReqOnTick = 0;
				}
			}
			if(s_bMoveAlarmOn == TRUE)
			{
				BOOL bAlarmOffRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_ALARM], (long)BORGWARNER::DATA::OFF);
				::g_Log.Log(_T("[PLC_MOVE_DIAG] Seq:%d New Start - Write 30200.4 PLC_DO_ALARM OFF ret:%d"), startContext.seq, bAlarmOffRet);
				if(bAlarmOffRet != FALSE)
				{
					s_bMoveAlarmOn = FALSE;
				}
			}

			// Scan 신호 받았으니 잘 받았다고 알려주자
			// JobNumber 확인, 검사결과 초기화, InspStart ON 확인 하면 PLC InspStart를 OFF 한다.
			// 그 후에 Scan Start 시작
			DWORD dwResponseBeginTick = GetTickCount();
			DWORD dwReqElapsed = (startContext.reqOnTick == 0) ? 0 : (dwResponseBeginTick - startContext.reqOnTick);
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Start Response Begin - REQ[30301.1] elapsed:%lu ms, Job:%d, Cam:%d, Div:%d, Barcode:%s"), startContext.seq, (unsigned long)dwReqElapsed, startContext.jobNumber, startContext.cameraIndex, startContext.divIndex, startContext.barcode);

			DWORD dwWriteTick = GetTickCount();
			BOOL bWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_JobNumber], (short)startContext.jobNumber);
			DWORD dwWriteElapsed = GetTickCount() - dwWriteTick;
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Write [address 30211] PLC_OUT_JobNumber ret:%d elapsed:%lu ms total:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed, (unsigned long)(GetTickCount() - dwResponseBeginTick));
			if(bWriteRet == FALSE || dwWriteElapsed > PLC_START_DIAG_DELAY_WARN_MS)
			{
				::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d PLC_OUT_JobNumber write abnormal - ret:%d elapsed:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed);
			}
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30211] PLC_OUT_JobNumber : %d"), startContext.jobNumber);

			// 검사결과 초기화는 RESP보다 먼저 기록한다 (PLC가 RESP를 본 시점에 이전 결과가 남지 않도록)
			dwWriteTick = GetTickCount();
			bWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_InspRet], (long)BORGWARNER::DATA::OFF);
			dwWriteElapsed = GetTickCount() - dwWriteTick;
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Write [address 30210] PLC_OUT_InspRet OFF ret:%d elapsed:%lu ms total:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed, (unsigned long)(GetTickCount() - dwResponseBeginTick));
			if(bWriteRet == FALSE || dwWriteElapsed > PLC_START_DIAG_DELAY_WARN_MS)
			{
				::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d PLC_OUT_InspRet OFF write abnormal - ret:%d elapsed:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed);
			}
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30210] PLC_OUT_InspRet : OFF"));

			dwWriteTick = GetTickCount();
			bWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::ON);
			dwWriteElapsed = GetTickCount() - dwWriteTick;
			DWORD dwReqToRespOnElapsed = (startContext.reqOnTick == 0) ? 0 : (GetTickCount() - startContext.reqOnTick);
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Write [address 30201.1] PLC_DO_RESP_Insp_Start ON ret:%d elapsed:%lu ms total:%lu ms reqToRespOn:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed, (unsigned long)(GetTickCount() - dwResponseBeginTick), (unsigned long)dwReqToRespOnElapsed);
			if(bWriteRet == FALSE || dwWriteElapsed > PLC_START_DIAG_DELAY_WARN_MS || dwReqToRespOnElapsed > PLC_START_DIAG_DELAY_WARN_MS)
			{
				::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d PLC_DO_RESP_Insp_Start ON write abnormal - ret:%d elapsed:%lu ms reqToRespOn:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed, (unsigned long)dwReqToRespOnElapsed);
			}
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.1] PLC_DO_RESP_Insp_Start : ON"));

			if(bWriteRet != FALSE)
			{
				s_bStartRespOn = TRUE;
				s_dwStartRespOnTick = GetTickCount();

				// Start 4-phase (PLC 회신 S1~S4): RESP를 최소 100 ms 유지하고 REQ OFF를 확인한 뒤 끈다.
				// 상한 안에 REQ OFF를 못 보면 RESP는 켠 채로 스캔을 진행하고, 이후 폴링에서 REQ OFF 확인 시 끈다.
				BOOL bReqOffSeen = FALSE;
				BOOL bReadFail = FALSE;
				DWORD dwReqOffTick = 0;
				while(1)
				{
					DWORD dwHold = GetTickCount() - s_dwStartRespOnTick;
					if((bReqOffSeen == TRUE && dwHold >= PLC_START_RESP_MIN_HOLD_MS) || dwHold >= PLC_START_REQ_OFF_WAIT_MS)
					{
						break;
					}

					Sleep(10);
					ServicePendingLinkTest(this);

					if(bReqOffSeen == FALSE)
					{
						if(m_pCtrlPLC->ReadPLC2VisionData() == FALSE)
						{
							bReadFail = TRUE;
							break;
						}
						if(m_bStart == FALSE)
						{
							bReqOffSeen = TRUE;
							dwReqOffTick = GetTickCount();
						}
					}
				}

				if(bReqOffSeen == TRUE)
				{
					::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d REQ[30301.1] OFF seen after RESP ON:%lu ms"), startContext.seq, (unsigned long)(dwReqOffTick - s_dwStartRespOnTick));

					dwWriteTick = GetTickCount();
					bWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::OFF);
					dwWriteElapsed = GetTickCount() - dwWriteTick;
					::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Write [address 30201.1] PLC_DO_RESP_Insp_Start OFF ret:%d elapsed:%lu ms total:%lu ms hold:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed, (unsigned long)(GetTickCount() - dwResponseBeginTick), (unsigned long)(GetTickCount() - s_dwStartRespOnTick));
					if(bWriteRet == FALSE || dwWriteElapsed > PLC_START_DIAG_DELAY_WARN_MS)
					{
						::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d PLC_DO_RESP_Insp_Start OFF write abnormal - ret:%d elapsed:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed);
					}
					if(bWriteRet != FALSE)
					{
						s_bStartRespOn = FALSE;
					}
					::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.1] PLC_DO_RESP_Insp_Start : OFF"));
				}
				else
				{
					::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d REQ[30301.1] not OFF within %lu ms (readFail:%d) - RESP_Insp_Start kept ON, continue scan"), startContext.seq, (unsigned long)(GetTickCount() - s_dwStartRespOnTick), bReadFail);
				}
			}

			DWORD dwResponseTotal = GetTickCount() - dwResponseBeginTick;
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Start Response End total:%lu ms"), startContext.seq, (unsigned long)dwResponseTotal);
			if(dwResponseTotal > PLC_START_DIAG_DELAY_WARN_MS)
			{
				::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d Start Response delayed - total:%lu ms"), startContext.seq, (unsigned long)dwResponseTotal);
			}

			// m_nJobNumber 예외처리
			if(startContext.jobNumber <= 0 || (long)::g_PLCManager.m_vecstrUseRecipe.size() < startContext.jobNumber)
			{
				::g_Log.Log(_T("PLC -> Vision::JobNumber Error(PLC:%d, Vision Max:%d)"), startContext.jobNumber, (long)::g_PLCManager.m_vecstrUseRecipe.size());
				return;
			}

			DWORD dwRecipeUpdateTick = GetTickCount();
			DWORD_PTR dwRecipeUpdateResult = 0;
			LRESULT lRecipeSendResult = ::SendMessageTimeout(m_pParent->GetSafeHwnd(), UM_UPDATE_RECIPE_UPDATE, m_ScanCount, startContext.jobNumber - 1, SMTO_ABORTIFHUNG | SMTO_BLOCK, PLC_UI_SEND_TIMEOUT_MS, &dwRecipeUpdateResult);
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Recipe Update SendMessageTimeout elapsed:%lu ms, SendRet:%ld, ReceiveRecipeOpen:%d"), startContext.seq, (unsigned long)(GetTickCount() - dwRecipeUpdateTick), lRecipeSendResult, m_pParent->m_bReceiveRecipeOpen);

			if(lRecipeSendResult == 0)
			{
				m_pParent->m_bReceiveRecipeOpen = FALSE;
				::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d Recipe Update UI timeout/fail - PLC thread released"), startContext.seq);
			}

			if(m_pParent->m_bReceiveRecipeOpen == TRUE)
			{
				::g_Log.Log(_T("Captured PLC Start Context - Cam:%d Div:%d Barcode:%s"), startContext.cameraIndex, startContext.divIndex, startContext.barcode);
				DWORD dwScanStartTick = GetTickCount();
				::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d ScanStart Begin - PLC polling paused, Cam:%d, Div:%d"), startContext.seq, startContext.cameraIndex, startContext.divIndex);
				ScanStart(startContext.cameraIndex, startContext.divIndex, startContext.barcode);
				::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d ScanStart End - elapsed:%lu ms"), startContext.seq, (unsigned long)(GetTickCount() - dwScanStartTick));

				m_pParent->m_bReceiveRecipeOpen = FALSE;
			}
			else
			{
				// 250711 TODO: 현장에서 해당 부분 가장 먼저 체크 필요 
				::g_Log.Log(_T("PLC Recipe Load Fail")); // 해당 로그 있는지 확인해보고  없으면 PLC 알람 띄워달라 요청.. 필요한 경우 아래 주석 사용
				// m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_REQ_Insp_Move], (long)BORGWARNER::DATA::OFF); 
			}
		}
		// ScanStart를 실행한 호출에서는 m_bScanDone이 스캔 전에 읽은 값이므로 판단하지 않는다 (다음 폴링에서 처리)
		if(bStartRising == FALSE && m_bScanDone == TRUE)
		{
			const BOOL bLogMoveOff = m_bMoveReqOn;
			DWORD dwMoveOffTick = GetTickCount();
			DWORD dwMoveReqOnElapsed = (m_dwMoveReqOnTick == 0) ? 0 : (dwMoveOffTick - m_dwMoveReqOnTick);
			BOOL bMoveWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_REQ_Insp_Move], (long)BORGWARNER::DATA::OFF); 
			DWORD dwMoveWriteElapsed = GetTickCount() - dwMoveOffTick;
			if(bLogMoveOff || bMoveWriteRet == FALSE || dwMoveWriteElapsed > PLC_START_DIAG_DELAY_WARN_MS)
			{
				::g_Log.Log(_T("[PLC_MOVE_DIAG] Seq:%d Write 30201.4 PLC_DO_REQ_Insp_Move OFF ret:%d wr:%lu ms reqOn:%lu ms"), m_nStartSignalSeq, bMoveWriteRet, (unsigned long)dwMoveWriteElapsed, (unsigned long)dwMoveReqOnElapsed);
			}
			if(bMoveWriteRet != FALSE)
			{
				m_bMoveReqOn = FALSE;
				m_dwMoveReqOnTick = 0;

				if(bLogMoveOff == TRUE && s_bMoveAlarmOn == TRUE)
				{
					BOOL bAlarmOffRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_ALARM], (long)BORGWARNER::DATA::OFF);
					::g_Log.Log(_T("[PLC_MOVE_DIAG] Seq:%d RESP_Insp_Move received - Write 30200.4 PLC_DO_ALARM OFF ret:%d"), m_nStartSignalSeq, bAlarmOffRet);
					if(bAlarmOffRet != FALSE)
					{
						s_bMoveAlarmOn = FALSE;
					}
				}
			}
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.4] PLC_DO_REQ_Insp_Move : OFF"));
		}
		else if(bStartRising == FALSE && m_bMoveReqOn == TRUE && m_dwMoveReqOnTick != 0)
		{
			// Move 응답 대기 감시: 경고 로그 후 ALARM. 자동 재시도는 PLC 재확인(M5) 전까지 하지 않는다
			DWORD dwMoveWait = GetTickCount() - m_dwMoveReqOnTick;
			if(dwMoveWait >= PLC_MOVE_RESP_WARN_MS && s_bMoveRespWarned == FALSE)
			{
				s_bMoveRespWarned = TRUE;
				::g_Log.Log(_T("[PLC_MOVE_DIAG][WARN] Seq:%d No RESP_Insp_Move for %lu ms - REQ[30301.1]:%d Job:%d Cam:%d Div:%d"), m_nStartSignalSeq, (unsigned long)dwMoveWait, m_bStart, m_nJobNumber, m_nCameraIndex, m_nDivIndex);
			}
			if(dwMoveWait >= PLC_MOVE_RESP_ALARM_MS && s_bMoveAlarmOn == FALSE)
			{
				BOOL bAlarmOnRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_ALARM], (long)BORGWARNER::DATA::ON);
				::g_Log.Log(_T("[PLC_MOVE_DIAG][ERROR] Seq:%d No RESP_Insp_Move for %lu ms - Write 30200.4 PLC_DO_ALARM ON ret:%d"), m_nStartSignalSeq, (unsigned long)dwMoveWait, bAlarmOnRet);
				if(bAlarmOnRet != FALSE)
				{
					s_bMoveAlarmOn = TRUE;
				}
			}
		}

		// Initialize (PLC 회신 L3·L4): REQ ON이면 1회 초기화 후 RESP ON, REQ OFF 확인 시 RESP OFF.
		// RESP를 켜지 못한 경우에만 다음 폴링에서 다시 시도한다.
		if(m_bInitialize == FALSE && s_bInitRespOn == TRUE)
		{
			BOOL bInitOffRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Initialize], (long)BORGWARNER::DATA::OFF);
			if(bInitOffRet != FALSE)
			{
				s_bInitRespOn = FALSE;
			}
			::g_Log.Log(_T("[PLC_INIT_DIAG] REQ[30300.1] OFF - Write [address 30200.1] PLC_DO_RESP_Initialize OFF ret:%d"), bInitOffRet);
		}

		if(m_bInitialize == TRUE && s_bInitRespOn == FALSE)
		{
			DWORD dwInitBeginTick = GetTickCount();
			::g_Log.Log(_T("[PLC_INIT_DIAG] REQ[30300.1] PLC_DI_REQ_Initialize ON - Initialize Begin"));

			// Vision -> PLC Data 초기화
			long startIndex = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_000;
			long endIndex = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_181;
			long startAddr = plcData[startIndex].nAddr;

			std::vector<SPLCData> vecsVision2PLCData;
			for(long idx = startIndex; idx <= endIndex; idx++)
			{
				plcData[idx].nValue = 0;

				vecsVision2PLCData.push_back(plcData[idx]);
			}

			//m_pCtrlPLC->WriteDatasDouble(vecsVision2PLCData, startAddr);

			for(long idx = startIndex; idx <= endIndex; idx++)
			{
				m_pCtrlPLC->WriteDataDouble(plcData[idx], 0);
			}

			m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_InspRet], (long)BORGWARNER::DATA::OFF);
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30210] PLC_OUT_InspRet : OFF"));
			m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_JobNumber], 0);
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30211] PLC_OUT_JobNumber : 0"));

			// division index 초기화

			// 초기화 응답 비트
			BOOL bInitOnRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Initialize], (long)BORGWARNER::DATA::ON);
			if(bInitOnRet != FALSE)
			{
				s_bInitRespOn = TRUE;
			}
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30200.1] PLC_DO_RESP_Initialize : ON"));
			::g_Log.Log(_T("[PLC_INIT_DIAG] Initialize End - RESP ON ret:%d elapsed:%lu ms"), bInitOnRet, (unsigned long)(GetTickCount() - dwInitBeginTick));
		}
	}
	break;
	}
}

void CPLCSocketManager::MessageProcessSimul(int nCameraIndex, int nDivIndex, int nJobNumber, bool bScanDone)
{
	//auto &plcData = ::g_PLCManager.m_sPLC.vecsVision2PLCData;

	switch (::g_ConfigManager.m_nCustomer)
	{
	case (long)CUSTOMER::BORGWARNER:
	{
		// Get Data : 필요한 부분 추가 필요
		::g_PLCManager.GetReadDataSimul(nCameraIndex, nDivIndex, nJobNumber, bScanDone);

		// ScanStart
		if (m_bStart == TRUE)
		{
			// Scan 신호 받았으니 잘 받았다고 알려주자
			// JobNumber 확인, 검사결과 초기화, InspStart ON 확인 하면 PLC InspStart를 OFF 한다.
			// 그 후에 Scan Start 시작
			//m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_JobNumber], (short)m_nJobNumber);
			//m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::ON);
			//m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_InspRet], (long)BORGWARNER::DATA::OFF);
			//Sleep(10); // PLC InspStart OFF 될 시간을 좀 기다려주자.
			//m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::OFF);

			// m_nJobNumber 예외처리
			//if ((long)::g_PLCManager.m_vecstrUseRecipe.size() <= m_nJobNumber || m_nJobNumber < 0)
			//{	::g_Log.Log(_T("PLC -> Vision::JobNumber Error(PLC:%d, Vision Max:%d)"), m_nJobNumber, (long)::g_PLCManager.m_vecstrUseRecipe.size());	return;		}

			// Recipe Load
			//::SendMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_RECIPE_UPDATE, 0, m_nJobNumber - 1);
			::SendMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_RECIPE_UPDATE, 0, 1);

			if (m_pParent->m_bReceiveRecipeOpen == TRUE)
			{
				ScanStart(m_nCameraIndex, m_nDivIndex);

				m_pParent->m_bReceiveRecipeOpen = FALSE;
			}
			else
			{
				::g_Log.Log(_T("PLC Recipe Load Fail"));
			}
		}

		//if (m_bScanDone == TRUE)
		//{	m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_REQ_Insp_Move], (long)BORGWARNER::DATA::OFF);		}

		if (m_bInitialize == TRUE)
		{
			// Vision -> PLC Data 초기화
			//long startIndex = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_000;
			//long endIndex = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_181;
			//long startAddr = plcData[startIndex].nAddr;

			//std::vector<SPLCData> vecsVision2PLCData;
			//for (long idx = startIndex; idx <= endIndex; idx++)
			//{
			//	plcData[idx].nValue = 0;

			//	vecsVision2PLCData.push_back(plcData[idx]);
			//}

			////m_pCtrlPLC->WriteDatasDouble(vecsVision2PLCData, startAddr);

			//for (long idx = startIndex; idx <= endIndex; idx++)
			//{
			//	m_pCtrlPLC->WriteDataDouble(plcData[idx], 0);
			//}

			//m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_InspRet], (long)BORGWARNER::DATA::OFF);
			//m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_JobNumber], 0);

			//// division index 초기화

			//// 초기화 응답 비트
			//m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Initialize], (long)BORGWARNER::DATA::ON);
		}
	}
	break;
	}
}

void CPLCSocketManager::ScanStart(const long cameraIndex, const long divIndex, const CString barcode)
{
	DWORD dwScanStartBeginTick = GetTickCount();
	CString cycleBarcode = barcode;
	if(cycleBarcode == _T(""))
	{
		cycleBarcode = m_strBarcode;
	}

	// 시간 가져오기...
	// 스캔 시작한 시간 먼저 받아놓자.
	// 이걸로 RawImage, ResultImage, ResultData까지 모두 통일
	m_strScanTime = _T("");
	COleDateTime currentTime;
	currentTime = COleDateTime::GetCurrentTime();
	m_strScanTime = currentTime.Format(_T("%Y%m%d_%H%M%S")); // 날짜, 시간, 분, 초

	// Multi 조명컨트롤러를 사용하지 않을 때는 한번에 켜고 한번에 끄자
	if(::g_ConfigManager.m_bMultiIllumination == FALSE)
	{
		// Illum. On
		auto illum = ::g_RecipeManager.m_sIllum.m_usIllum;

		for(long idx = 0; idx < ILLUM_CHANNEL_MAX; idx++)
		{
			::g_CtrlSerial.IllumOn(idx, illum[idx]);
		}
	}

	const long cameraCount = ::g_ConfigManager.m_nMainUI + 1;
	BOOL bAllScanStarted = TRUE;
	std::vector<HANDLE> vechScanThread;

	for(long idx = 0; idx < cameraCount; idx++)
	{
		if(idx >= (long)::g_ScanManager.m_Control_Scan.size())
		{
			::g_Log.Log(_T("[SCAN_DIAG][WARN] ScanStart invalid thread index - Cam:%d Div:%d ThreadCount:%d"), idx, divIndex, (long)::g_ScanManager.m_Control_Scan.size());
			bAllScanStarted = FALSE;
			break;
		}

		DWORD dwReadyWaitBegin = GetTickCount();
		DWORD dwReadyRet = WaitWithLinkTest(this, 1, &::g_ScanManager.m_Control_Scan[idx].m_hSignal_Ready, FALSE, PLC_SCAN_READY_WAIT_TIMEOUT_MS);
		DWORD dwReadyElapsed = GetTickCount() - dwReadyWaitBegin;
		if(dwReadyRet != WAIT_OBJECT_0)
		{
			::g_Log.Log(_T("[SCAN_DIAG][WARN] ScanStart ready wait fail - Seq:%d Cam:%d Div:%d Ret:%lu Err:%lu Elapsed:%lu ms"), m_nStartSignalSeq, idx, divIndex, dwReadyRet, GetLastError(), (unsigned long)dwReadyElapsed);
			bAllScanStarted = FALSE;
			break;
		}

		::g_ScanManager.m_vecnCamIndex[idx] = idx;
		::g_ScanManager.m_vecnDivIndex[idx] = divIndex;
		::g_ScanManager.m_vecstrBarcode[idx] = cycleBarcode;
		::g_ScanManager.m_vecstrScanTime[idx] = m_strScanTime;
		::g_ScanManager.StartScan(idx);
		vechScanThread.push_back(::g_ScanManager.m_Control_Scan[idx].m_hSignal_Finish);

		if(idx + 1 < cameraCount)
		{
			WaitWithLinkTest(this, 0, nullptr, FALSE, 700);// Camera Scan간 시간차를 준다. (대기 중 LinkTest 유지) // 250714 쓰레드 대기시간 500으로 증가 // 250715 쓰레드 대기시간 700
		}
	}

	HANDLE* pScanThreadHandles = vechScanThread.empty() ? nullptr : &vechScanThread[0];

	auto &plcData = ::g_PLCManager.m_sPLC.vecsVision2PLCData;

	DWORD dwScanWaitBeginTick = GetTickCount();
	DWORD ret = WAIT_FAILED;
	if(pScanThreadHandles != nullptr)
	{
		ret = WaitWithLinkTest(this, (DWORD)vechScanThread.size(), pScanThreadHandles, TRUE, 5000);
	}
	DWORD dwScanWaitElapsed = GetTickCount() - dwScanWaitBeginTick;
	const BOOL bScanSuccess = (bAllScanStarted == TRUE && ret == WAIT_OBJECT_0);
	if(bScanSuccess == FALSE)
	{
		::g_ScanManager.m_bScanDone = FALSE;

		::g_Log.Log(_T("[SCAN_DIAG][ERROR] Scan completion timeout/failure - Started:%d WaitRet:%lu Elapsed:%lu ms Threads:%d"), bAllScanStarted, (unsigned long)ret, (unsigned long)dwScanWaitElapsed, (long)vechScanThread.size());
		for(long idx = 0; idx < cameraCount && idx < (long)::g_ScanManager.m_vecbScanRunning.size(); idx++)
		{
			::g_ScanManager.m_vecbScanRunning[idx] = FALSE;
			if(idx < (long)::g_ScanManager.m_Control_Scan.size())
			{
				DWORD dwRecoverRet = WaitWithLinkTest(this, 1, &::g_ScanManager.m_Control_Scan[idx].m_hSignal_Finish, FALSE, 1000);
				::g_Log.Log(_T("[SCAN_DIAG] Scan cancel recovery - Thread:%d Ret:%lu"), idx, dwRecoverRet);
			}
		}

		// 스캔 실패 통보 방식(ALARM/NG)은 PLC 재확인(Q4) 후 확정. 지금은 기존 신호를 유지하고,
		// 비트 보존 쓰기로 RESP가 남지 않도록 REQ OFF 확인 시 폴링에서 끄게 한다.
		if(m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::ON) != FALSE)
		{
			s_bStartRespOn = TRUE;
			s_dwStartRespOnTick = GetTickCount();
		}
		::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.1] PLC_DO_RESP_Insp_Start : ON"));
		DWORD dwMoveWriteTick = GetTickCount();
		BOOL bMoveWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_REQ_Insp_Move], (long)BORGWARNER::DATA::OFF);
		DWORD dwMoveWriteElapsed = GetTickCount() - dwMoveWriteTick;
		if(bMoveWriteRet != FALSE)
		{
			m_bMoveReqOn = FALSE;
			m_dwMoveReqOnTick = 0;
		}
		::g_Log.Log(_T("[PLC_MOVE_DIAG] Seq:%d Write 30201.4 PLC_DO_REQ_Insp_Move OFF ret:%d wr:%lu ms scan:%lu ms wait:%lu ms ret:%lu"), m_nStartSignalSeq, bMoveWriteRet, (unsigned long)dwMoveWriteElapsed, (unsigned long)(GetTickCount() - dwScanStartBeginTick), (unsigned long)dwScanWaitElapsed, (unsigned long)ret);
		::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.4] PLC_DO_REQ_Insp_Move : OFF"));

		::g_Log.Log(_T("MSG_V2H::V2H_SCAN_FAIL"));
	}
	else
	{
		::g_ScanManager.m_bScanDone = TRUE;

		// RESP_Insp_Start는 Start 4-phase에서 REQ OFF 확인 후 끈다 (여기서 강제로 끄지 않음)
		DWORD dwMoveWriteTick = GetTickCount();
		BOOL bMoveWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_REQ_Insp_Move], (long)BORGWARNER::DATA::ON);
		DWORD dwMoveWriteElapsed = GetTickCount() - dwMoveWriteTick;
		if(bMoveWriteRet != FALSE)
		{
			m_bMoveReqOn = TRUE;
			m_dwMoveReqOnTick = GetTickCount();
			s_bMoveRespWarned = FALSE;
		}
		else
		{
			m_bMoveReqOn = FALSE;
			m_dwMoveReqOnTick = 0;
		}
		::g_Log.Log(_T("[PLC_MOVE_DIAG] Seq:%d Write 30201.4 PLC_DO_REQ_Insp_Move ON ret:%d wr:%lu ms scan:%lu ms wait:%lu ms ret:%lu"), m_nStartSignalSeq, bMoveWriteRet, (unsigned long)dwMoveWriteElapsed, (unsigned long)(GetTickCount() - dwScanStartBeginTick), (unsigned long)dwScanWaitElapsed, (unsigned long)ret);
		::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.4] PLC_DO_REQ_Insp_Move : ON"));

		::g_Log.Log(_T("MSG_V2H::V2H_SCAN_DONE"));
	}

	// Multi 조명컨트롤러를 사용하지 않을 때는 한번에 켜고 한번에 끄자
	if(::g_ConfigManager.m_bMultiIllumination == FALSE)
	{
		for(long idx = 0; idx < ILLUM_CHANNEL_MAX; idx++)
		{
			::g_CtrlSerial.IllumOff(idx);
		}
	}

	// 다시 Ready 상태로 돌려 놓는다.
	// Scan thread owns Ready state. Do not force-ready from PLC thread.

}

BOOL CPLCSocketManager::IsConnect()
{
	return m_bConnect;
}

void CPLCSocketManager::RequestLinkTestWrite(const BOOL bOn)
{
	InterlockedExchange(&m_nPendingLinkTestValue, bOn ? (long)BORGWARNER::DATA::ON : (long)BORGWARNER::DATA::OFF);
}
