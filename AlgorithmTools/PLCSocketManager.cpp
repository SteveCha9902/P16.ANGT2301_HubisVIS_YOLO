#include "pch.h"
#include "PLCSocketManager.h"
#include "../VisionMain/DlgVisionMain.h"

#include "CtrlPLC.h"

#define BUFFER_SIZE 10000
#define COUNT_MAX 5
#define PLC_START_DIAG_DELAY_WARN_MS 500
#define PLC_UI_SEND_TIMEOUT_MS 1000
#define PLC_SCAN_READY_WAIT_TIMEOUT_MS 1000

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPLCSocketManager g_PLCSocketManager;

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
					LONG nPendingLinkTest = InterlockedExchange(&m_nPendingLinkTestValue, -1);
					if(nPendingLinkTest >= 0 && bRet != FALSE)
					{
						auto &linkData = ::g_PLCManager.m_sPLC.vecsVision2PLCData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_LinkTest];
						DWORD dwLinkWriteTick = GetTickCount();
						BOOL bLinkWriteRet = m_pCtrlPLC->WriteData(linkData, (short)nPendingLinkTest);
						DWORD dwLinkWriteElapsed = GetTickCount() - dwLinkWriteTick;
						if(bLinkWriteRet == FALSE || dwLinkWriteElapsed >= PLC_START_DIAG_DELAY_WARN_MS)
						{
							::g_Log.Log(_T("[PLC_LINK_DIAG][WARN] LinkTest write Value:%d Ret:%d Elapsed:%lu ms"), nPendingLinkTest, bLinkWriteRet, (unsigned long)dwLinkWriteElapsed);
						}
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

		// ScanStart
		if(bStartRising == TRUE)
		{
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
			Sleep(50); // wait before PLC InspStart OFF.
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Wait Before RESP_Insp_Start OFF elapsed:%lu ms"), startContext.seq, (unsigned long)(GetTickCount() - dwWriteTick));
			dwWriteTick = GetTickCount();
			bWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::OFF);
			dwWriteElapsed = GetTickCount() - dwWriteTick;
			::g_Log.Log(_T("[PLC_START_DIAG] Seq:%d Write [address 30201.1] PLC_DO_RESP_Insp_Start OFF ret:%d elapsed:%lu ms total:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed, (unsigned long)(GetTickCount() - dwResponseBeginTick));
			if(bWriteRet == FALSE || dwWriteElapsed > PLC_START_DIAG_DELAY_WARN_MS)
			{
				::g_Log.Log(_T("[PLC_START_DIAG][WARN] Seq:%d PLC_DO_RESP_Insp_Start OFF write abnormal - ret:%d elapsed:%lu ms"), startContext.seq, bWriteRet, (unsigned long)dwWriteElapsed);
			}
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.1] PLC_DO_RESP_Insp_Start : OFF"));

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
		if(m_bScanDone == TRUE)
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
			}
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.4] PLC_DO_REQ_Insp_Move : OFF"));
		}

		if(m_bInitialize == TRUE)
		{
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
			m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Initialize], (long)BORGWARNER::DATA::ON);
			::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30200.1] PLC_DO_RESP_Initialize : ON"));
		}

		//if (m_bInitialize == FALSE)
		//{
		//	// 초기화 응답 비트
		//	m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Initialize], (long)BORGWARNER::DATA::OFF);
		//}
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
		DWORD dwReadyRet = ::WaitForSingleObject(::g_ScanManager.m_Control_Scan[idx].m_hSignal_Ready, PLC_SCAN_READY_WAIT_TIMEOUT_MS);
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
			Sleep(700);// Camera Scan간 시간차를 준다. // 250714 쓰레드 대기시간 500으로 증가 // 250715 쓰레드 대기시간 700
		}
	}

	HANDLE* pScanThreadHandles = vechScanThread.empty() ? nullptr : &vechScanThread[0];

	auto &plcData = ::g_PLCManager.m_sPLC.vecsVision2PLCData;

	DWORD dwScanWaitBeginTick = GetTickCount();
	DWORD ret = WAIT_FAILED;
	if(pScanThreadHandles != nullptr)
	{
		ret = ::WaitForMultipleObjects((DWORD)vechScanThread.size(), pScanThreadHandles, TRUE, 5000);
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
				DWORD dwRecoverRet = ::WaitForSingleObject(::g_ScanManager.m_Control_Scan[idx].m_hSignal_Finish, 1000);
				::g_Log.Log(_T("[SCAN_DIAG] Scan cancel recovery - Thread:%d Ret:%lu"), idx, dwRecoverRet);
			}
		}

		m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::ON);
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

		m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_Insp_Start], (long)BORGWARNER::DATA::OFF);
		::g_Log.Log(_T("Vision -> PLC::Write Complete [address 30201.1] PLC_DO_RESP_Insp_Start : OFF"));
		DWORD dwMoveWriteTick = GetTickCount();
		BOOL bMoveWriteRet = m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_REQ_Insp_Move], (long)BORGWARNER::DATA::ON);
		DWORD dwMoveWriteElapsed = GetTickCount() - dwMoveWriteTick;
		if(bMoveWriteRet != FALSE)
		{
			m_bMoveReqOn = TRUE;
			m_dwMoveReqOnTick = GetTickCount();
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
