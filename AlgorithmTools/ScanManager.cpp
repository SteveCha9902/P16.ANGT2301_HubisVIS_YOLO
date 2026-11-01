#include "pch.h"
#include "ScanManager.h"
#include "../VisionMain/DlgVisionMain.h"
#include "../VisionMain/DlgMain1.h"
#include "../VisionMain/DlgMain2.h"
#include <map>

#define SCAN_THREAD_READY_WAIT_TIMEOUT_MS 1000
#define INSPECTION_THREAD_READY_WAIT_TIMEOUT_MS 1000

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CScanManager g_ScanManager;

// PLCAutoPass=1 전용 Master 이미지 대체.
// 카메라·조명 연결 실패나 Grab 타임아웃 시 실행 폴더 Simulation\{Before|After}\{cam}_{div}_raw.bmp를 촬영 영상 대신 검사한다.
// (8bit gray BMP, 크기는 이미지 메모리와 같아야 한다. 처음 쓸 때 한 번 읽어 메모리에 보관한다)
static CCriticalSection s_csMasterImage;
static std::map<CString, std::vector<BYTE>> s_mapMasterImage;

static BOOL LoadMasterImageFile(const CString &strPath, const long imageWidth, const long imageHeight, std::vector<BYTE> &image)
{
	HBITMAP hBitmap = (HBITMAP)::LoadImage(NULL, strPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if(hBitmap == NULL)
	{
		return FALSE;
	}

	BOOL bRet = FALSE;
	DIBSECTION dib;
	memset(&dib, 0, sizeof(dib));
	if(::GetObject(hBitmap, sizeof(dib), &dib) == sizeof(dib))
	{
		const BITMAP &bm = dib.dsBm;
		const long height = abs(bm.bmHeight);
		if(bm.bmBits != nullptr && bm.bmBitsPixel == 8 && bm.bmPlanes == 1 && bm.bmWidth == imageWidth && height == imageHeight && bm.bmWidthBytes >= imageWidth)
		{
			image.resize((size_t)imageWidth * (size_t)imageHeight);
			for(long r = 0; r < height; r++)
			{
				const long srcRow = (dib.dsBmih.biHeight > 0) ? (height - 1 - r) : r;	// bottom-up DIB
				memcpy(&image[(size_t)r * (size_t)imageWidth], (const BYTE*)bm.bmBits + (size_t)srcRow * (size_t)bm.bmWidthBytes, (size_t)imageWidth);
			}
			bRet = TRUE;
		}
	}

	::DeleteObject(hBitmap);
	return bRet;
}

static BOOL CopyMasterImage(const long threadIndex, const long camIndex, const long divIndex)
{
	const long imageWidth = ::g_MemoryManager.m_nImageWidth;
	const long imageHeight = ::g_MemoryManager.m_nImageHeight;
	BYTE* org = ::g_MemoryManager.GetByteImage(camIndex + divIndex * 2);   // 0,1,2,3,4,5

	// 검사 구분: 레시피 파일명이 Post로 시작하면 후검사(After), 아니면 전검사(Before)
	CString fileName = ::g_RecipeManager.m_currentRecipeName;
	int lastSlash = fileName.ReverseFind('\\');
	if(lastSlash >= 0)
	{
		fileName = fileName.Mid(lastSlash + 1);
	}
	const CString inspType = (fileName.Left(4).CompareNoCase(_T("Post")) == 0) ? _T("After") : _T("Before");

	CString strPath;
	strPath.Format(_T("%s\\Simulation\\%s\\%d_%d_raw.bmp"), ::g_ConfigManager.m_strCurPath, inspType, camIndex, divIndex);

	if(org == nullptr || imageWidth <= 0 || imageHeight <= 0)
	{
		::g_Log.Log(_T("Scan Thread [%d] : [MASTER_IMAGE][ERROR] Image memory not ready - Cam:%d Div:%d"), threadIndex, camIndex, divIndex);
		return FALSE;
	}

	CSingleLock lock(&s_csMasterImage, TRUE);
	auto it = s_mapMasterImage.find(strPath);
	if(it == s_mapMasterImage.end())
	{
		std::vector<BYTE> image;
		if(LoadMasterImageFile(strPath, imageWidth, imageHeight, image) == FALSE)
		{
			::g_Log.Log(_T("Scan Thread [%d] : [MASTER_IMAGE][ERROR] Load failed (8bit gray %dx%d BMP required) - %s"), threadIndex, imageWidth, imageHeight, strPath);
			return FALSE;
		}
		it = s_mapMasterImage.insert(std::make_pair(strPath, std::move(image))).first;
	}

	memcpy(org, &it->second[0], it->second.size());
	::g_Log.Log(_T("Scan Thread [%d] : [MASTER_IMAGE] Camera image replaced - Cam:%d Div:%d %s [Applying AutoPass]"), threadIndex, camIndex, divIndex, strPath);
	return TRUE;
}

CScanManager::CScanManager()
	: m_pAlgorithm(nullptr)
{
	// 변수 초기화
	m_pAlgorithm = new CAlgorithm;
}

CScanManager::~CScanManager()
{
	delete m_pAlgorithm;
}

void CScanManager::SetParent(CDlgVisionMain* pParent)
{
	m_pParent = pParent;
}

void CScanManager::InitScanManageThread()
{
	// Scan Manage Thread 초기화
	m_Manage_Scan.m_hSignal_Start = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Manage_Scan.m_hSignal_Kill = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Manage_Scan.m_Handle_Thread = INVALID_HANDLE_VALUE;

	HANDLE hScanManageThread = ::AfxBeginThread(ThreadScanManageFunc, (void *)this)->m_hThread;
	::DuplicateHandle(::GetCurrentProcess(), hScanManageThread, ::GetCurrentProcess(), &m_Manage_Scan.m_Handle_Thread, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);
}

void CScanManager::FreeScanManageThread()
{
	// 모든 Thread에게 일을 그만하라고 알린다.
	::SetEvent(m_Manage_Scan.m_hSignal_Kill);

	// Thread가 종료되기를 기다린다.
	DWORD dwWaitRet = ::WaitForSingleObject(m_Manage_Scan.m_Handle_Thread, 5000);
	if(dwWaitRet != WAIT_OBJECT_0)
	{
		::g_Log.Log(_T("[THREAD_DIAG][ERROR] Scan manage thread stop timeout - Ret:%lu"), dwWaitRet);
	}

	// 관련 핸들을 모두 닫는다.
	::CloseHandle(m_Manage_Scan.m_hSignal_Start);
	::CloseHandle(m_Manage_Scan.m_hSignal_Kill);
	::CloseHandle(m_Manage_Scan.m_Handle_Thread);
}

void CScanManager::InitScanThread()
{
	// Scan Thread 초기화
	m_Control_Scan.resize(SCAN_THREAD_COUNT);
	m_vecnCamIndex.resize(SCAN_THREAD_COUNT);
	m_vecnDivIndex.resize(SCAN_THREAD_COUNT);
	m_vecScanStep.resize(SCAN_THREAD_COUNT);
	m_vecbScanRunning.resize(SCAN_THREAD_COUNT);
	m_vecbScanDone.resize(SCAN_THREAD_COUNT);
	m_vecstrBarcode.resize(SCAN_THREAD_COUNT);
	m_vecstrScanTime.resize(SCAN_THREAD_COUNT);

	for(long nThread = 0; nThread < SCAN_THREAD_COUNT; nThread++)
	{
		auto &control = m_Control_Scan[nThread];

		control.m_hSignal_Kill = CreateEvent(NULL, FALSE, FALSE, NULL);
		control.m_hSignal_Ready = CreateEvent(NULL, FALSE, FALSE, NULL);
		control.m_hSignal_Start = CreateEvent(NULL, FALSE, FALSE, NULL);
		control.m_hSignal_Finish = CreateEvent(NULL, FALSE, FALSE, NULL);
		control.m_Handle_Thread = INVALID_HANDLE_VALUE;

		HANDLE hScanThread = ::AfxBeginThread(ThreadScanFunc, LPVOID(this))->m_hThread;
		::DuplicateHandle(::GetCurrentProcess(), hScanThread, ::GetCurrentProcess(), &control.m_Handle_Thread, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);

		m_vecnCamIndex[nThread] = 0;
		m_vecnDivIndex[nThread] = 0;
		m_vecScanStep[nThread] = SCAN_SEQUENCE::NONE;
		m_vecbScanRunning[nThread] = FALSE;
		m_vecbScanDone[nThread] = FALSE;
		m_vecstrBarcode[nThread] = _T("");
		m_vecstrScanTime[nThread] = _T("");
	}
}

void CScanManager::FreeScanThread()
{
	// 모든 Thread에게 일을 그만하라고 알린다.
	for(long nThread = long(m_Control_Scan.size() - 1); nThread >= 0; nThread--)
	{
		::SetEvent(m_Control_Scan[nThread].m_hSignal_Kill);
		m_vecbScanRunning[nThread] = FALSE;
	}

	// Thread가 종료되기를 기다린다.
	for(long nThread = long(m_Control_Scan.size() - 1); nThread >= 0; nThread--)
	{
		DWORD dwWaitRet = ::WaitForSingleObject(m_Control_Scan[nThread].m_Handle_Thread, 5000);
		if(dwWaitRet != WAIT_OBJECT_0)
		{
			::g_Log.Log(_T("[THREAD_DIAG][ERROR] Scan thread stop timeout - Thread:%d Ret:%lu"), nThread, dwWaitRet);
		}
	}

	// 관련 핸들을 모두 닫는다.
	for(long nThread = long(m_Control_Scan.size() - 1); nThread >= 0; nThread--)
	{
		::CloseHandle(m_Control_Scan[nThread].m_hSignal_Kill);
		::CloseHandle(m_Control_Scan[nThread].m_hSignal_Ready);
		::CloseHandle(m_Control_Scan[nThread].m_hSignal_Start);
		::CloseHandle(m_Control_Scan[nThread].m_hSignal_Finish);
		::CloseHandle(m_Control_Scan[nThread].m_Handle_Thread);
	}
}

// Scan Manage Thread 함수
UINT CScanManager::ThreadScanManageFunc(LPVOID pParam)
{
	CScanManager *pSeqThreadManager = (CScanManager *)pParam;

	pSeqThreadManager->ScanManage();

	return 0;
}

// Scan Manage Thread
void CScanManager::ScanManage()
{
	CString strTemp;

	// Scan Manage Thread Handle
	HANDLE pHandles[2] = {m_Manage_Scan.m_hSignal_Start, m_Manage_Scan.m_hSignal_Kill};

	::g_Log.Log(_T("Scan Manage Thread Ready"));

	while(1)
	{
		DWORD dwRet = ::WaitForMultipleObjects(2, pHandles, FALSE, INFINITE);

		if(dwRet == WAIT_OBJECT_0)
		{
			// Scan Manage Thread Handle
			std::vector<HANDLE> vechScanReady(SCAN_THREAD_COUNT);
			HANDLE* pScanReadyHandles = &vechScanReady[0];

			for(long thread = 0; thread < SCAN_THREAD_COUNT; thread++)
			{
				vechScanReady[thread] = m_Control_Scan[thread].m_hSignal_Ready;
			}

			// 사용할 수 있는 Scan 쓰레드를 찾는다...
			DWORD dwRet = ::WaitForMultipleObjects(SCAN_THREAD_COUNT, pScanReadyHandles, FALSE, SCAN_THREAD_READY_WAIT_TIMEOUT_MS);

			if(dwRet == WAIT_TIMEOUT)
			{
				::g_Log.Log(_T("[SCAN_DIAG][WARN] Scan Manage ready wait timeout - Cam:%d Div:%d"), m_nCamIndex, m_nDivIndex);
				continue;
			}
			else if(dwRet == WAIT_FAILED)
			{
				::g_Log.Log(_T("[SCAN_DIAG][WARN] Scan Manage ready wait failed - Err:%lu Cam:%d Div:%d"), GetLastError(), m_nCamIndex, m_nDivIndex);
				continue;
			}

			const long threadIndex = dwRet - WAIT_OBJECT_0;

			if(threadIndex < 0 || threadIndex >= SCAN_THREAD_COUNT)
			{
				::g_Log.Log(_T("[SCAN_DIAG][WARN] Scan Manage invalid ready index - Ret:%lu Cam:%d Div:%d"), dwRet, m_nCamIndex, m_nDivIndex);
				continue;
			}

			m_vecnCamIndex[threadIndex] = m_nCamIndex;
			m_vecnDivIndex[threadIndex] = m_nDivIndex;

			// 사용할 수 있는 Scan 쓰레드 한테 Scan 시작 지시한다.
			StartScan(threadIndex);
		}
		else // Scan Manage Thread 종료
		{
			break;
		}
	}
}

void CScanManager::StartScanManage(const long camIndex, const long divIndex)
{
	m_nCamIndex = camIndex;
	m_nDivIndex = divIndex;

	// PLC 접속 되지 않았을 때는 MainUI 가 가지고 있는 m_nVisionIndex, m_nDivIndex 를 가져온다.
	if(::g_PLCSocketManager.IsConnect() == FALSE)
	{
		switch(::g_ConfigManager.m_nMainUI)
		{
		case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
			m_nCamIndex = m_pParent->m_pDlgMain1->m_nVisionIndex;
			m_nDivIndex = m_pParent->m_pDlgMain1->m_nDivIndex;
			break;
		case (long)MAINUI::UI_FOR_DUAL_CAMERA:
			m_nCamIndex = m_pParent->m_pDlgMain2->m_nVisionIndex;
			m_nDivIndex = m_pParent->m_pDlgMain2->m_nDivIndex;
			break;
		}
	}

	::SetEvent(m_Manage_Scan.m_hSignal_Start);

	Sleep(100);
}

// Scan Thread 함수
UINT CScanManager::ThreadScanFunc(LPVOID pParam)
{
	CScanManager *pSeqThreadManager = (CScanManager *)pParam;

	pSeqThreadManager->Scan();

	return 0;
}

// Scan Thread
void CScanManager::Scan()
{
	CString strTemp;

	static CCriticalSection cs;

	cs.Lock();
	static long nLatestThreadID = 0;
	const long nThreadID = nLatestThreadID++;
	cs.Unlock();

	auto &control = m_Control_Scan[nThreadID];

	::SetEvent(control.m_hSignal_Ready);

	HANDLE pHandles[] = {control.m_hSignal_Start, control.m_hSignal_Kill};
	const DWORD nSignalCount = sizeof(pHandles) / sizeof(HANDLE);

	while(1)
	{
		BOOL bQuit = FALSE;

		::SetEvent(control.m_hSignal_Ready);
		::g_Log.Log(_T("Scan Thread [%d] : Thread Ready"), nThreadID);

		DWORD dwRet = ::WaitForMultipleObjects(nSignalCount, pHandles, FALSE, INFINITE);

		switch(dwRet)
		{
		case WAIT_OBJECT_0: // Do Scan
		{
			::g_Log.Log(_T("Scan Thread [%d] : Scan Start"), nThreadID);

			while(1)
			{
				if(m_vecbScanRunning[nThreadID] == TRUE)
				{
					RunScan(nThreadID);
				}
				else
				{
					break;
				}

				Sleep(1);
			}

			::g_Log.Log(_T("Scan Thread [%d] : Scan End"), nThreadID);

			// Scan 끝
			::SetEvent(control.m_hSignal_Finish);

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

void CScanManager::StartScan(const long threadIndex)
{
	CString strTemp;

	m_vecScanStep[threadIndex] = SCAN_SEQUENCE::SCAN_READY;
	m_vecbScanRunning[threadIndex] = TRUE;
	m_vecbScanDone[threadIndex] = FALSE;

	::ResetEvent(m_Control_Scan[threadIndex].m_hSignal_Finish);
	::SetEvent(m_Control_Scan[threadIndex].m_hSignal_Start);
}

void CScanManager::RunScan(const long threadIndex)
{
	static long eTime[3] = {0,};

	auto camIndex = m_vecnCamIndex[threadIndex];
	auto divIndex = m_vecnDivIndex[threadIndex];
	auto barcode = m_vecstrBarcode[threadIndex];
	auto scanTime = m_vecstrScanTime[threadIndex];
	auto &scanStep = m_vecScanStep[threadIndex];
	auto &scanRunning = m_vecbScanRunning[threadIndex];
	auto &scanDone = m_vecbScanDone[threadIndex];

	CCtrlCamera* pCtrlCamera = nullptr;
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		pCtrlCamera = m_pParent->m_pDlgMain1->m_pCtrlCamera;
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		pCtrlCamera = m_pParent->m_pDlgMain2->m_pCtrlCamera[camIndex];
		break;
	}

	switch(scanStep)
	{
	case SCAN_SEQUENCE::SCAN_READY: // H/W Check, Illum On
	{
		::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SCAN_READY"), threadIndex);

		// PLCAutoPass=1: 카메라·조명이 연결되지 않았으면 Master 이미지로 대체해 검사를 진행한다
		if(::g_ConfigManager.m_bPLCAutoPass != FALSE && (pCtrlCamera == nullptr || pCtrlCamera->IsOpen() == FALSE || ::g_CtrlSerial.IsOpen() == FALSE))
		{
			::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SCAN_READY - Camera(%d) open:%d, Illumination open:%d"), threadIndex, camIndex,
				(pCtrlCamera != nullptr) ? pCtrlCamera->IsOpen() : FALSE, ::g_CtrlSerial.IsOpen());

			if(CopyMasterImage(threadIndex, camIndex, divIndex) == TRUE)
			{
				scanDone = TRUE;
				scanStep = SCAN_SEQUENCE::INSPECTION_START;
			}
			else
			{
				scanDone = FALSE;
				scanStep = SCAN_SEQUENCE::ERROR_STEP;
			}
			break;
		}

		// 카메라 연결 확인
		if(pCtrlCamera == nullptr || pCtrlCamera->IsOpen() == FALSE)
		{
			::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SCAN_READY - No Found Camera"), threadIndex);

			scanStep = SCAN_SEQUENCE::ERROR_STEP;
			break;
		}

		// 조명 연결 확인
		if(::g_CtrlSerial.IsOpen() == FALSE)
		{
			::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SCAN_READY - No Found Illumination"), threadIndex);

			scanStep = SCAN_SEQUENCE::ERROR_STEP;
			break;
		}

		// 조명조건 가져오기...
		// 반드시 InspItem 에서 조명설정을 해야 정상적인 Scan 이 가능하다.
		SIllumParams sillum;
		sillum.Init();
		BOOL bExistIllum(FALSE);
		for(long idx = 0; idx < (long)::g_RecipeManager.m_vecsInspItem.size(); idx++)
		{
			if(::g_RecipeManager.m_vecsInspItem[idx].visionIndex == camIndex)
			{
				sillum = ::g_RecipeManager.m_vecsInspItem[idx].sIllum;
				bExistIllum = TRUE;
				break;
			}
		}

		if(bExistIllum == FALSE) // 조명조건 못찾았으면 Scan 하지 말자.
		{
			::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SCAN_READY - No Found Illumination Setting"), threadIndex);

			scanStep = SCAN_SEQUENCE::ERROR_STEP;
			break;
		}

		// Illum. On
		unsigned short *illum = nullptr;
		if(::g_ConfigManager.m_bMultiIllumination == TRUE) // Multi 조명컨트롤러를 사용할 때
		{
			illum = sillum.m_usIllum;
		}
		else
		{
			illum = ::g_RecipeManager.m_sIllum.m_usIllum;
		}

		for(long idx = 0; idx < ILLUM_CHANNEL_MAX; idx++)
		{
			::g_CtrlSerial.IllumOn(idx, illum[idx]);
		}

		scanStep = SCAN_SEQUENCE::START_SCAN;
	}
	break;
	case SCAN_SEQUENCE::START_SCAN: // Grab
	{
		::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::START_SCAN"), threadIndex);

		pCtrlCamera->Grab1Shot();

		// Grab Status...
		::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_GRAB_STATUS, TRUE, camIndex);

		scanStep = SCAN_SEQUENCE::SCAN_DONE;
	}
	break;
	case SCAN_SEQUENCE::SCAN_DONE:
	{
		::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SCAN_DONE"), threadIndex);

		// Grab Done
		DWORD dwGrabDone = WaitForSingleObject(pCtrlCamera->GetGrabDoneHandle(), 1000);

		if(dwGrabDone == WAIT_OBJECT_0)
		{
			// Grab Image copy
			BYTE* org = ::g_MemoryManager.GetByteImage(camIndex + divIndex* 2);   // 0,1,2,3,4,5
			memcpy(org, pCtrlCamera->GetImage(), pCtrlCamera->GetImageSize());

			// Illum. Off
			if(::g_ConfigManager.m_bMultiIllumination == TRUE) // Multi 조명컨트롤러를 사용할 때
			{
				for(long idx = 0; idx < ILLUM_CHANNEL_MAX; idx++)
				{
					::g_CtrlSerial.IllumOff(idx);
				}
			}

			// Grab Status...
			::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_GRAB_STATUS, FALSE, camIndex);

			scanDone = TRUE;

			scanStep = SCAN_SEQUENCE::INSPECTION_START;
		}
		else
		{
			::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SCAN_READY - Camera(%d) Grab Time out"), threadIndex, camIndex);

			// PLCAutoPass=1: Grab 실패 시 Master 이미지로 대체해 검사를 진행한다
			if(::g_ConfigManager.m_bPLCAutoPass != FALSE && CopyMasterImage(threadIndex, camIndex, divIndex) == TRUE)
			{
				if(::g_ConfigManager.m_bMultiIllumination == TRUE)
				{
					for(long idx = 0; idx < ILLUM_CHANNEL_MAX; idx++)
					{
						::g_CtrlSerial.IllumOff(idx);
					}
				}
				::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_GRAB_STATUS, FALSE, camIndex);

				scanDone = TRUE;
				scanStep = SCAN_SEQUENCE::INSPECTION_START;
				break;
			}

			scanDone = FALSE;
			scanStep = SCAN_SEQUENCE::ERROR_STEP;
		}
	}
	break;
	case SCAN_SEQUENCE::INSPECTION_START: // Inspection
	{
		::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::INSPECTION_START"), threadIndex);

		// Inspection Thread Handle
		std::vector<HANDLE> vechInspReady(INSPECTION_THREAD_COUNT);
		HANDLE* pInspReadyHandles = &vechInspReady[0];

		for(long thread = 0; thread < INSPECTION_THREAD_COUNT; thread++)
		{
			vechInspReady[thread] = ::g_InspectionManager.m_Control_Inspection[thread].m_hSignal_Ready;
		}

		// 사용할 수 있는 검사 쓰레드를 찾는다...
		DWORD dwRet = ::WaitForMultipleObjects(INSPECTION_THREAD_COUNT, pInspReadyHandles, FALSE, INSPECTION_THREAD_READY_WAIT_TIMEOUT_MS);

		if(dwRet == WAIT_TIMEOUT)
		{
			::g_Log.Log(_T("[SCAN_DIAG][WARN] Inspection ready wait timeout - ScanThread:%d Cam:%d Div:%d Step:%d"), threadIndex, camIndex, divIndex, (long)scanStep);
			scanDone = FALSE;
			scanStep = SCAN_SEQUENCE::ERROR_STEP;
			break;
		}
		else if(dwRet == WAIT_FAILED)
		{
			::g_Log.Log(_T("[SCAN_DIAG][WARN] Inspection ready wait failed - Err:%lu ScanThread:%d Cam:%d Div:%d Step:%d"), GetLastError(), threadIndex, camIndex, divIndex, (long)scanStep);
			scanDone = FALSE;
			scanStep = SCAN_SEQUENCE::ERROR_STEP;
			break;
		}

		const long inspThreadIndex = dwRet - WAIT_OBJECT_0;

		if(inspThreadIndex < 0 || inspThreadIndex >= INSPECTION_THREAD_COUNT)
		{
			::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::INSPECTION : Insp. Thread [%d] index is Wrong!!"), threadIndex, inspThreadIndex);
			scanDone = FALSE;
			scanStep = SCAN_SEQUENCE::ERROR_STEP;
			break;
		}

		// 사용할 수 있는 검사 쓰레드 한테 검사 시작 지시한다.
		::g_InspectionManager.StartInsp(inspThreadIndex, camIndex, divIndex, barcode, scanTime);
		// 바로 Image 를 표시한다.
		// 검사 Time out은 검사 쓰레드에서 체크한다.
		scanStep = SCAN_SEQUENCE::IMAGE_DISPLAY;
	}
	break;
	case SCAN_SEQUENCE::IMAGE_DISPLAY: // 영상 표시
	{
		::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::IMAGE_DISPLAY"), threadIndex);

		// 영상 표시
		::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_IMAGE, camIndex, divIndex);

		scanStep = SCAN_SEQUENCE::SAVE_IMAGE;
	}
	break;
	case SCAN_SEQUENCE::SAVE_IMAGE: // Image 저장
	{
		::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SAVE_IMAGE"), threadIndex);

		::g_DataSaveManager.StartDataSave(DATA_SAVE::RAW_IMAGE, barcode, camIndex, divIndex, scanTime);

		scanStep = SCAN_SEQUENCE::SEQUENCE_DONE;
	}
	break;
	case SCAN_SEQUENCE::SEQUENCE_DONE: // Sequence 완료
	{
		::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::SEQUENCE_DONE"), threadIndex);

		scanDone = TRUE;
		scanRunning = FALSE;

		scanStep = SCAN_SEQUENCE::NONE;
	}
	break;
	case SCAN_SEQUENCE::ERROR_STEP:
	{
		::g_Log.Log(_T("Scan Thread [%d] : SCAN_SEQUENCE::ERROR_STEP"), threadIndex);

		scanDone = FALSE;
		scanRunning = FALSE;

		scanStep = SCAN_SEQUENCE::NONE;
	}
	break;
	}
}
