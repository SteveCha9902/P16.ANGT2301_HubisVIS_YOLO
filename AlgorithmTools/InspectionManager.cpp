#include "pch.h"
#include "InspectionManager.h"
#include "Inspection.h"
#include "../VisionMain/DlgVisionMain.h"

#define INSPECTION_UI_SEND_TIMEOUT_MS 500

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CInspectionManager g_InspectionManager;

CInspectionManager::CInspectionManager()
	: m_pInspection(nullptr)
{
	m_vecsInspEdgeResult.clear();
	m_vecsInspBlobResult.clear();
}

CInspectionManager::~CInspectionManager()
{

}

void CInspectionManager::SetParent(CDlgVisionMain* pParent)
{
	m_pParent = pParent;

	// Image 정보 설정 - SHK 2023.12.27
	m_nImageWidth = m_pParent->m_nImageWidth;
	m_nImageHeight = m_pParent->m_nImageHeight;
	m_nImageSize = m_pParent->m_nImageSize;
}

void CInspectionManager::InitInspectionThread()
{
	m_pInspection = new CInspection;
	m_pInspection->SetImageInfo(m_nImageWidth, m_nImageHeight, -1);

	long camCount(0);
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		camCount = 1;
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		camCount = 2;
		break;
	}
	m_vecsInspEdgeResult.resize(camCount);
	m_vecsInspBlobResult.resize(camCount);

	for(auto &result : m_vecsInspEdgeResult)
	{ 
		result.resize(::g_ConfigManager.m_nDivisionIndex);
	}
	for(auto &result : m_vecsInspBlobResult)
	{
		result.resize(::g_ConfigManager.m_nDivisionIndex);
	}

	//LYS 241010
	int divindex = ::g_ConfigManager.m_nDivisionIndex;
	for (int i = 0; i < divindex*camCount; i++)
	{
		m_bResult[i] = 0;
	}
	

	m_vecpInspection.resize(INSPECTION_THREAD_COUNT);
	for(long thread = 0; thread < (long)m_vecpInspection.size(); thread++)
	{
		m_vecpInspection[thread] = new CInspection;
		m_vecpInspection[thread]->SetImageInfo(m_nImageWidth, m_nImageHeight, thread); // Image 정보 설정 - SHK 2023.12.27
	}

	// Inspection Thread 초기화
	m_Control_Inspection.resize(INSPECTION_THREAD_COUNT);
	m_vecnCamIndex.resize(INSPECTION_THREAD_COUNT);
	m_vecnDivIndex.resize(INSPECTION_THREAD_COUNT);
	m_vecInspStep.resize(INSPECTION_THREAD_COUNT);
	m_vecbInspRunning.resize(INSPECTION_THREAD_COUNT);
	m_vecbInspDone.resize(INSPECTION_THREAD_COUNT);
	m_vecstrBarcode.resize(INSPECTION_THREAD_COUNT);
	m_vecstrScanTime.resize(INSPECTION_THREAD_COUNT);
	for(long nThread = 0; nThread < INSPECTION_THREAD_COUNT; nThread++)
	{
		auto &control = m_Control_Inspection[nThread];

		control.m_hSignal_Kill = CreateEvent(NULL, FALSE, FALSE, NULL);
		control.m_hSignal_Ready = CreateEvent(NULL, FALSE, FALSE, NULL);
		control.m_hSignal_Start = CreateEvent(NULL, FALSE, FALSE, NULL);
		control.m_Handle_Thread = INVALID_HANDLE_VALUE;

		HANDLE hInspectionThread = ::AfxBeginThread(ThreadInspectionFunc, LPVOID(this))->m_hThread;
		::DuplicateHandle(::GetCurrentProcess(), hInspectionThread, ::GetCurrentProcess(), &control.m_Handle_Thread, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);

		m_vecnCamIndex[nThread] = 0;
		m_vecnDivIndex[nThread] = 0;
		m_vecInspStep[nThread] = INSP_SEQUENCE::NONE;
		m_vecbInspRunning[nThread] = FALSE;
		m_vecbInspDone[nThread] = FALSE;
		m_vecstrBarcode[nThread] = _T("");
		m_vecstrScanTime[nThread] = _T("");
	}
}

void CInspectionManager::FreeInspectionThread()
{

	// Inspection Class 소멸

	// 모든 Thread에게 일을 그만하라고 알린다.
	for(long nThread = long(m_Control_Inspection.size() - 1); nThread >= 0; nThread--)
	{
		m_vecbInspRunning[nThread] = FALSE;
		::SetEvent(m_Control_Inspection[nThread].m_hSignal_Kill);
	}

	// Thread가 종료되기를 기다린다.
	for(long nThread = long(m_Control_Inspection.size() - 1); nThread >= 0; nThread--)
	{
		DWORD dwWaitRet = ::WaitForSingleObject(m_Control_Inspection[nThread].m_Handle_Thread, 5000);
		if(dwWaitRet != WAIT_OBJECT_0)
		{
			::g_Log.Log(_T("[THREAD_DIAG][ERROR] Inspection thread stop timeout - Thread:%d Ret:%lu"), nThread, dwWaitRet);
		}
	}

	// 관련 핸들을 모두 닫는다.
	if(m_pInspection != nullptr)
	{
		delete m_pInspection;
		m_pInspection = nullptr;
	}
	for(long insp = 0; insp < (long)m_vecpInspection.size(); insp++)
	{
		delete m_vecpInspection[insp];
	}
	m_vecpInspection.clear();

	for(long nThread = long(m_Control_Inspection.size() - 1); nThread >= 0; nThread--)
	{
		::CloseHandle(m_Control_Inspection[nThread].m_hSignal_Kill);
		::CloseHandle(m_Control_Inspection[nThread].m_hSignal_Ready);
		::CloseHandle(m_Control_Inspection[nThread].m_hSignal_Start);
		::CloseHandle(m_Control_Inspection[nThread].m_Handle_Thread);
	}
}

UINT CInspectionManager::ThreadInspectionFunc(LPVOID pParam)
{
	CInspectionManager *pThreadControl = (CInspectionManager *)pParam;

	pThreadControl->ThreadInspection();

	return 0;
}

void CInspectionManager::ThreadInspection()
{
	static CCriticalSection cs;

	cs.Lock();
	static long nLatestThreadID = 0;
	const long nThreadID = nLatestThreadID++;
	cs.Unlock();

	auto &control = m_Control_Inspection[nThreadID];

	::SetEvent(control.m_hSignal_Ready);

	HANDLE pHandles[] = {control.m_hSignal_Start, control.m_hSignal_Kill};
	const DWORD nSignalCount = sizeof(pHandles) / sizeof(HANDLE);

	while(1)
	{
		BOOL bQuit = FALSE;

		::g_Log.Log(_T("Inspection Thread [%d] : Thread Ready"), nThreadID);

		DWORD dwRet = ::WaitForMultipleObjects(nSignalCount, pHandles, FALSE, INFINITE);

		switch(dwRet)
		{
		case WAIT_OBJECT_0: // Do Inspection
		{
			// 검사
			::g_Log.Log(_T("Inspection Thread [%d] : Inspection Start"), nThreadID);

			// Inspection Status...
			::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_INSP_STATUS, TRUE, nThreadID);

			while(1)
			{
				if(m_vecbInspRunning[nThreadID] == TRUE)
				{
					RunInsp(nThreadID);
				}
				else
				{
					break;
				}

				Sleep(1);
			}

			::g_Log.Log(_T("Inspection Thread [%d] : Inspection Result : %s"), nThreadID, _T(""));

			// 검사 끝나면 다시 Ready 상태로 돌려 놓는다.
			::SetEvent(control.m_hSignal_Ready);

			::g_Log.Log(_T("Inspection Thread [%d] : Inspection Done."), nThreadID);

			// Inspection Status...
			::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_INSP_STATUS, FALSE, nThreadID);

			break;
		}
		case WAIT_OBJECT_0 + 1:	// Quit Thread
			bQuit = TRUE;
			break;
		}

		if(bQuit == TRUE)
		{
			break;
		}
	}
}

// 5단계 결과 완료 장벽용 카운터 (PLCSocketManager.cpp). SEND_DATA를 마칠 때마다 1 증가
extern volatile LONG g_lPlcResultCommitCount;

void CInspectionManager::StartInsp(const long threadIndex, const long camIndex, const long divIndex, const CString barcode, const CString scanTime)
{
	CString strTemp;

	m_vecnCamIndex[threadIndex] = camIndex;
	m_vecnDivIndex[threadIndex] = divIndex;
	m_vecstrBarcode[threadIndex] = barcode;
	m_vecstrScanTime[threadIndex] = scanTime;
	m_vecInspStep[threadIndex] = INSP_SEQUENCE::INSPECTION_START;
	m_vecbInspRunning[threadIndex] = TRUE;
	m_vecbInspDone[threadIndex] = FALSE;

	::SetEvent(m_Control_Inspection[threadIndex].m_hSignal_Start);
}

void CInspectionManager::RunInsp(const long threadIndex)
{
	static long eTime[3] = {0,};

	auto camIndex = m_vecnCamIndex[threadIndex];
	auto divIndex = m_vecnDivIndex[threadIndex];
	auto barcode = m_vecstrBarcode[threadIndex];
	auto scanTime = m_vecstrScanTime[threadIndex];
	auto &inspStep = m_vecInspStep[threadIndex];
	auto &inspRunning = m_vecbInspRunning[threadIndex];
	auto &inspDone = m_vecbInspDone[threadIndex];
	auto inspection = m_vecpInspection[threadIndex];

	switch(inspStep)
	{
	case INSP_SEQUENCE::INSPECTION_READY: // Inspection Ready. 점검
	{
		::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::INSPECTION_READY"), threadIndex);

		if((long)::g_RecipeManager.m_vecsInspItem.size() >= camIndex)
		{
			::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::INSPECTION_START - Insp. Item Num Error(cam: %d, div: %d, InspItem Num: %d)"), threadIndex, camIndex, divIndex, (long)::g_RecipeManager.m_vecsInspItem.size());

			inspStep = INSP_SEQUENCE::ERROR_STEP;
		}
		else
		{
			inspStep = INSP_SEQUENCE::INSPECTION_START;
		}
	}
	break;
	case INSP_SEQUENCE::INSPECTION_START: // Inspection
	{
		::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::INSPECTION_START"), threadIndex);
		::g_Log.Log(_T("CInspection::DoInspection( camIndex : [%d]  divIndex : [%d]"), camIndex, divIndex);

		// 자동
		if(inspection->DoInspection(camIndex, divIndex) == TRUE)
		{
			inspStep = INSP_SEQUENCE::INSPECTION_DONE;
		}
		else // 검사에 이상이 있어 중간에 빠져나갔을 경우 무조건 INVALID 처리해서 결과 전송한다.
		{
			::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::INSPECTION_START - Inspection(cam: %d, div: %d) Error!!"), threadIndex, camIndex, divIndex);

			inspStep = INSP_SEQUENCE::ERROR_STEP;
		}

		// 250712 임시 테스트
		/*::g_DataSaveManager.RawImageSave(TRUE);*/
	}
	break;
	case INSP_SEQUENCE::INSPECTION_DONE: // Inspection Done Check
	{
		::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::INSPECTION_DONE"), threadIndex);
		
		inspStep = INSP_SEQUENCE::RESULT_DISPLAY;
	}
	break;
	case INSP_SEQUENCE::RESULT_DISPLAY: // 결과 표시, Overlay만 추가해준다.
	{
		::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::RESULT_DISPLAY"), threadIndex);

		// 250714 sjpark  Insp. Thread 불량현상으로 PostMessage -> SendMessage 변경
		// 결과 표시
		//::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_RESULT, camIndex, threadIndex);
		// Result UI update: avoid indefinite worker-thread blocking.
		DWORD_PTR dwUiResult = 0;
		LPARAM lResultParam = MAKELPARAM((WORD)threadIndex, (WORD)divIndex);
		LRESULT lUiSendResult = ::SendMessageTimeout(m_pParent->GetSafeHwnd(), UM_UPDATE_RESULT, camIndex, lResultParam, SMTO_ABORTIFHUNG | SMTO_BLOCK, INSPECTION_UI_SEND_TIMEOUT_MS, &dwUiResult);
		if(lUiSendResult == 0)
		{
			::g_Log.Log(_T("[INSP_DIAG][WARN] Result UI update timeout/fail - Thread:%d Cam:%d Div:%d"), threadIndex, camIndex, divIndex);
		}

		inspStep = INSP_SEQUENCE::SEND_DATA;
	}
	break;
	case INSP_SEQUENCE::SEND_DATA: // 결과값 전송 or 결과값 저장
	{
		::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::SEND_DATA"), threadIndex);

		int nResultCount = (long)m_vecpInspection[threadIndex]->m_vecResult.size();
		for(int i = 0; i < nResultCount; i++)
		{
			m_vecpInspection[threadIndex]->m_vecResult[i].visionIndex = camIndex;
			m_vecpInspection[threadIndex]->m_vecResult[i].divIndex = divIndex;
		}

		// PLC로 결과 전송
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
			::g_PLCManager.SetResultData_forBorgwarner(
				m_vecpInspection[threadIndex]->m_vecResult,
				m_vecpInspection[threadIndex]->m_vecCriteriaResult, divIndex);
			break;
		}

		// 결과 기록 완료 알림: PLC 스레드는 카메라 수만큼 완료된 뒤 REQ_Insp_Move를 켠다 (빈 결과도 완료로 계산)
		InterlockedIncrement(&g_lPlcResultCommitCount);

		inspStep = INSP_SEQUENCE::SAVE_RESULT_IMAGE;
	}
	break;
	case INSP_SEQUENCE::SAVE_RESULT_IMAGE: // 결과 Image 저장
	{
		::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::SAVE_RESULT_IMAGE"), threadIndex);

		int nResult = -1;
		auto algoSequence = ::g_RecipeManager.m_vecsInspItem[camIndex].algoSequence;
		switch(algoSequence)
		{
		case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
		{
			m_vecsInspEdgeResult[camIndex][divIndex] = m_vecpInspection[threadIndex]->m_sInspEdgeResult;
			nResult = m_vecsInspEdgeResult[camIndex][divIndex].bResult == TRUE ? 1 : 0;
		}
		break;
		case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
		{
			m_vecsInspBlobResult[camIndex][divIndex] = m_vecpInspection[threadIndex]->m_sInspBlobResult;
			nResult = m_vecsInspBlobResult[camIndex][divIndex].bResult == TRUE ? 1 : 0;
		}
		break;
		case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
		{
			m_vecsInspBlobResult[camIndex][divIndex] = m_vecpInspection[threadIndex]->m_sInspBlobResult;
			nResult = m_vecsInspBlobResult[camIndex][divIndex].bResult == TRUE ? 1 : 0;
		}
		break;
		}

		
		// 결과 이미지: 이 검사 쓰레드의 결과 오버레이를 넘긴다 (Setup 화면 객체 m_pInspection 아님)
		::g_DataSaveManager.StartDataSave(DATA_SAVE::RESULT_IMAGE, barcode, camIndex, divIndex, scanTime, nResult, m_vecpInspection[threadIndex]->m_pOverlayManager);

		inspStep = INSP_SEQUENCE::SEQUENCE_DONE;
	}
	break; 
	case INSP_SEQUENCE::SEQUENCE_DONE: // Sequence 완료
	{
		::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::SEQUENCE_DONE"), threadIndex);

		inspDone = TRUE;
		inspRunning = FALSE;

		inspStep = INSP_SEQUENCE::NONE;
	}
	break;
	case INSP_SEQUENCE::ERROR_STEP:
	{
		::g_Log.Log(_T("Insp. Thread [%d] : INSP_SEQUENCE::ERROR_STEP"), threadIndex);

		// PLCAutoPass=1: 검사 실패도 OK(1)로 결과 완료 처리한다.
		// 완료로 세지 않으면 PLC 스레드가 결과 대기 초과로 ALARM을 켜고, PLC는 ALARM 후 프로그램 재시작이 필요하다.
		if(::g_ConfigManager.m_bPLCAutoPass != FALSE)
		{
			if(::g_ConfigManager.m_nCustomer == (long)CUSTOMER::BORGWARNER)
			{
				auto &plcData = ::g_PLCManager.m_sPLC.vecsVision2PLCData;
				BOOL bRet = ::g_PLCSocketManager.m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_InspRet], 1);
				::g_Log.Log(_T("Insp. Thread [%d] : ERROR_STEP - Inspection(cam: %d, div: %d) Fail -> InspRet 1 ret:%d [Applying AutoPass]"), threadIndex, camIndex, divIndex, bRet);
			}

			InterlockedIncrement(&g_lPlcResultCommitCount);
		}

		inspDone = FALSE;
		inspRunning = FALSE;

		inspStep = INSP_SEQUENCE::NONE;
	}
	break;
	}
}


