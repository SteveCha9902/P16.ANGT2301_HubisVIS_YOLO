#include "pch.h"
#include "DataSaveManager.h"

#define BUF_MAX_SIZE 2048
#define DATA_SAVE_RAW_QUEUE_HIGH_WATERMARK_COUNT 120
#define DATA_SAVE_RAW_QUEUE_HIGH_WATERMARK_BYTES (600ULL * 1024ULL * 1024ULL)
#define DATA_SAVE_QUEUE_WAIT_LOG_INTERVAL_MS 5000

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDataSaveManager g_DataSaveManager;

typedef BOOL(WINAPI *PGETDISKFREESPACEEX)(LPCSTR,
	PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

CDataSaveManager::CDataSaveManager()
	: m_SaveMethod(DATA_SAVE::RAW_IMAGE)
	, m_strBarcode(_T(""))
	, m_strTime(_T(""))
	, m_strBeforpath(_T(""))
	, m_nCamIdx(0)
	, m_nDivIdx(0)
	, m_nResult(-1)
	, m_nAlarm(ALARM_OK)
	, m_nImageWidth(0)
	, m_nImageHeight(0)
	, m_nImageSize(0)
	, m_strYear(_T(""))
	, m_strMonth(_T(""))
	, m_strDay(_T(""))
	, m_strInspectionType(_T(""))
	, m_nQueuedRawImageJobCount(0)
	, m_ullQueuedRawImageBytes(0)
	, m_bProcessingDataSaveJob(FALSE)
	, m_hSignal_QueueSpace(NULL)
	, m_strCurrentCycleKey(_T(""))
{
	m_Control_DataSave.m_hSignal_Start = NULL;
	m_Control_DataSave.m_hSignal_Kill = NULL;
	m_Control_DataSave.m_Handle_Thread = INVALID_HANDLE_VALUE;
	m_hSignal_QueueSpace = CreateEvent(NULL, TRUE, TRUE, NULL);
	::InitializeCriticalSection(&m_csDataSaveQueue);
}
CDataSaveManager::~CDataSaveManager()
{
	if(m_hSignal_QueueSpace != NULL)
	{
		::CloseHandle(m_hSignal_QueueSpace);
		m_hSignal_QueueSpace = NULL;
	}
	::DeleteCriticalSection(&m_csDataSaveQueue);
}

void CDataSaveManager::SetImageSize(const long nImageWidth, const long nImageHeight)
{
	m_nImageWidth = nImageWidth;
	m_nImageHeight = nImageHeight;
	m_nImageSize = nImageWidth * nImageHeight;
}

void CDataSaveManager::InitThread()
{
	// Data Save...
	m_Control_DataSave.m_hSignal_Start = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_DataSave.m_hSignal_Kill = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_DataSave.m_Handle_Thread = INVALID_HANDLE_VALUE;
	if(m_hSignal_QueueSpace == NULL)
	{
		m_hSignal_QueueSpace = CreateEvent(NULL, TRUE, TRUE, NULL);
	}

	HANDLE hDataSaveThread = ::AfxBeginThread(ThreadDataSaveFunc, (void *)this)->m_hThread;
	::DuplicateHandle(::GetCurrentProcess(), hDataSaveThread, ::GetCurrentProcess(), &m_Control_DataSave.m_Handle_Thread, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);
}

void CDataSaveManager::FreeThread()
{
	if(m_Control_DataSave.m_Handle_Thread == INVALID_HANDLE_VALUE)
	{
		return;
	}

	::SetEvent(m_Control_DataSave.m_hSignal_Kill);
	::WaitForSingleObject(m_Control_DataSave.m_Handle_Thread, 1000);

	::CloseHandle(m_Control_DataSave.m_hSignal_Start);
	::CloseHandle(m_Control_DataSave.m_hSignal_Kill);
	::CloseHandle(m_Control_DataSave.m_Handle_Thread);
	m_Control_DataSave.m_hSignal_Start = NULL;
	m_Control_DataSave.m_hSignal_Kill = NULL;
	m_Control_DataSave.m_Handle_Thread = INVALID_HANDLE_VALUE;

	if(m_hSignal_QueueSpace != NULL)
	{
		::CloseHandle(m_hSignal_QueueSpace);
		m_hSignal_QueueSpace = NULL;
	}
}
// DataSave Thread
UINT CDataSaveManager::ThreadDataSaveFunc(LPVOID pParam)
{
	CDataSaveManager *pDataSave = (CDataSaveManager *)pParam;

	pDataSave->ThreadDataSave();

	return 0;
}


void CDataSaveManager::ThreadDataSave()
{
	HANDLE pHandles[] = {m_Control_DataSave.m_hSignal_Start, m_Control_DataSave.m_hSignal_Kill};
	const DWORD nSignalCount = sizeof(pHandles) / sizeof(HANDLE);

	::g_Log.Log(_T("Data Save Thread Ready"));

	while(1)
	{
		DWORD dwRet = ::WaitForMultipleObjects(nSignalCount, pHandles, FALSE, INFINITE);

		if(dwRet == WAIT_OBJECT_0)
		{
			SDataSaveJob saveJob;
			while(PopDataSaveJob(saveJob) == TRUE)
			{
				SetDataSaveProcessing(TRUE);
				ApplyDataSaveJob(saveJob);

				CString strPath(_T(""));
				if(::g_ConfigManager.m_strRawImageSavePath != _T(""))
				{
					strPath = ::g_ConfigManager.m_strRawImageSavePath;
				}
				else if(::g_ConfigManager.m_strResultDataSavePath != _T(""))
				{
					strPath = ::g_ConfigManager.m_strResultDataSavePath;
				}
				else if(::g_ConfigManager.m_strResultImageSavePath != _T(""))
				{
					strPath = ::g_ConfigManager.m_strResultImageSavePath;
				}

				DWORDLONG nSize(0);
				if(strPath != _T(""))
				{
					nSize = MyGetDiskFreeSpaceEx(CStringA(strPath));
				}

				if(strPath == _T("") || nSize > 5368709120)
				{
					switch(saveJob.method)
					{
					case DATA_SAVE::RAW_IMAGE:
						RawImageSaveJob(saveJob);
						break;
					case DATA_SAVE::RESULT_IMAGE:
						ResultImageSave(saveJob.manualInspection);
						break;
					case DATA_SAVE::RESULT_DATA_CSV:
						ResultDataSave(saveJob.manualInspection);
						break;
					};
				}
				else
				{
					::g_Log.Log(_T("[STORAGE_BACKPRESSURE] Data Save Thread : Drive No Capacity - job retained"));
					::EnterCriticalSection(&m_csDataSaveQueue);
					m_queueDataSaveJob.push_front(saveJob);
					if(saveJob.method == DATA_SAVE::RAW_IMAGE)
					{
						m_nQueuedRawImageJobCount++;
						m_ullQueuedRawImageBytes += (ULONGLONG)saveJob.rawImage.size();
					}
					UpdateDataSaveQueueSpaceSignal();
					::LeaveCriticalSection(&m_csDataSaveQueue);
					SetDataSaveProcessing(FALSE);

					if(::WaitForSingleObject(m_Control_DataSave.m_hSignal_Kill, 5000) == WAIT_OBJECT_0)
					{
						::SetEvent(m_Control_DataSave.m_hSignal_Kill);
					}
					else
					{
						::SetEvent(m_Control_DataSave.m_hSignal_Start);
					}
					break;
				}

				SetDataSaveProcessing(FALSE);
			}
		}
		else
		{
			break;
		}
	}

	SetDataSaveProcessing(FALSE);
}
void CDataSaveManager::StartDataSave(DATA_SAVE method, CString barcode, const long camIdx, const long divIdx, CString time, const int result)
{
	SDataSaveJob job;
	job.method = method;
	job.barcode = barcode;
	job.camIdx = camIdx;
	job.divIdx = divIdx;
	job.result = result;
	job.time = time;
	job.manualInspection = FALSE;

	if(method == DATA_SAVE::RAW_IMAGE)
	{
		job.imageWidth = m_nImageWidth;
		job.imageHeight = m_nImageHeight;
		job.imageSize = m_nImageSize;

		if(job.imageSize <= 0)
		{
			job.imageWidth = ::g_MemoryManager.m_nImageWidth;
			job.imageHeight = ::g_MemoryManager.m_nImageHeight;
			job.imageSize = job.imageWidth * job.imageHeight;
		}

		if(job.imageSize > 0)
		{
			if(WaitDataSaveQueueSpace((DWORD)job.imageSize) == FALSE)
			{
				::g_Log.Log(_T("[STORAGE_BACKPRESSURE][ERROR] RAW save dropped - queue remained full Cam:%d Div:%d Bytes:%d"), camIdx, divIdx, job.imageSize);
				return;
			}

			const long imageIndex = camIdx + divIdx * 2;
			BYTE* pImage = ::g_MemoryManager.GetByteImage(imageIndex);
			if(pImage != nullptr)
			{
				job.rawImage.resize(job.imageSize);
				memcpy(&job.rawImage[0], pImage, job.imageSize);
			}
			else
			{
				::g_Log.Log(_T("[STORAGE_BACKPRESSURE][WARN] RAW image copy skipped - image buffer null Cam:%d Div:%d Index:%d"), camIdx, divIdx, imageIndex);
			}
		}
	}

	::EnterCriticalSection(&m_csDataSaveQueue);
	m_queueDataSaveJob.push_back(job);
	if(job.method == DATA_SAVE::RAW_IMAGE)
	{
		m_nQueuedRawImageJobCount++;
		m_ullQueuedRawImageBytes += (ULONGLONG)job.rawImage.size();
	}
	UpdateDataSaveQueueSpaceSignal();
	::LeaveCriticalSection(&m_csDataSaveQueue);

	::SetEvent(m_Control_DataSave.m_hSignal_Start);
}

BOOL CDataSaveManager::PopDataSaveJob(SDataSaveJob& job)
{
	BOOL bRet = FALSE;

	::EnterCriticalSection(&m_csDataSaveQueue);
	if(m_queueDataSaveJob.empty() == false)
	{
		job = m_queueDataSaveJob.front();
		m_queueDataSaveJob.pop_front();
		if(job.method == DATA_SAVE::RAW_IMAGE)
		{
			m_nQueuedRawImageJobCount--;
			if(m_nQueuedRawImageJobCount < 0)
			{
				m_nQueuedRawImageJobCount = 0;
			}

			ULONGLONG jobBytes = (ULONGLONG)job.rawImage.size();
			if(m_ullQueuedRawImageBytes >= jobBytes)
			{
				m_ullQueuedRawImageBytes -= jobBytes;
			}
			else
			{
				m_ullQueuedRawImageBytes = 0;
			}
		}
		UpdateDataSaveQueueSpaceSignal();
		bRet = TRUE;
	}
	::LeaveCriticalSection(&m_csDataSaveQueue);

	return bRet;
}

void CDataSaveManager::ApplyDataSaveJob(const SDataSaveJob& job)
{
	m_SaveMethod = job.method;
	m_strBarcode = job.barcode;
	if(m_strBarcode == _T(""))
	{
		m_strBarcode = _T("BarcodeEmpty");
	}
	m_nCamIdx = job.camIdx;
	m_nDivIdx = job.divIdx;
	m_nResult = job.result;

	if(job.time != _T(""))
	{
		m_strTime = job.time;
	}
}

BOOL CDataSaveManager::WaitDataSaveQueueSpace(const DWORD dwImageBytes)
{
	DWORD dwLastLogTick = 0;
	const DWORD dwWaitStartTick = GetTickCount();

	while(TRUE)
	{
		BOOL bWait = FALSE;
		long nQueuedRawCount = 0;
		ULONGLONG ullQueuedRawBytes = 0;

		::EnterCriticalSection(&m_csDataSaveQueue);
		nQueuedRawCount = m_nQueuedRawImageJobCount;
		ullQueuedRawBytes = m_ullQueuedRawImageBytes;
		if(nQueuedRawCount >= DATA_SAVE_RAW_QUEUE_HIGH_WATERMARK_COUNT ||
			ullQueuedRawBytes + (ULONGLONG)dwImageBytes > DATA_SAVE_RAW_QUEUE_HIGH_WATERMARK_BYTES)
		{
			bWait = TRUE;
			::ResetEvent(m_hSignal_QueueSpace);
		}
		::LeaveCriticalSection(&m_csDataSaveQueue);

		if(bWait == FALSE)
		{
			return TRUE;
		}

		DWORD dwNow = GetTickCount();
		if(dwLastLogTick == 0 || dwNow - dwLastLogTick >= DATA_SAVE_QUEUE_WAIT_LOG_INTERVAL_MS)
		{
			::g_Log.Log(_T("[STORAGE_BACKPRESSURE] DataSave queue high watermark - rawJobs:%d rawBytes:%I64u waitImageBytes:%lu"), nQueuedRawCount, ullQueuedRawBytes, (unsigned long)dwImageBytes);
			dwLastLogTick = dwNow;
		}

		if(dwNow - dwWaitStartTick >= 3000)
		{
			::g_Log.Log(_T("[STORAGE_BACKPRESSURE][ERROR] Queue wait timeout after %lums - rawJobs:%d rawBytes:%I64u waitImageBytes:%lu"), (unsigned long)(dwNow - dwWaitStartTick), nQueuedRawCount, ullQueuedRawBytes, (unsigned long)dwImageBytes);
			return FALSE;
		}
		::WaitForSingleObject(m_hSignal_QueueSpace, 1000);

	}
}

void CDataSaveManager::UpdateDataSaveQueueSpaceSignal()
{
	if(m_hSignal_QueueSpace == NULL)
	{
		return;
	}

	if(m_nQueuedRawImageJobCount < DATA_SAVE_RAW_QUEUE_HIGH_WATERMARK_COUNT &&
		m_ullQueuedRawImageBytes < DATA_SAVE_RAW_QUEUE_HIGH_WATERMARK_BYTES)
	{
		::SetEvent(m_hSignal_QueueSpace);
	}
	else
	{
		::ResetEvent(m_hSignal_QueueSpace);
	}
}

void CDataSaveManager::SetDataSaveProcessing(const BOOL bProcessing)
{
	::EnterCriticalSection(&m_csDataSaveQueue);
	m_bProcessingDataSaveJob = bProcessing;
	UpdateDataSaveQueueSpaceSignal();
	::LeaveCriticalSection(&m_csDataSaveQueue);
}

BOOL CDataSaveManager::IsQueueIdle()
{
	BOOL bIdle = FALSE;

	::EnterCriticalSection(&m_csDataSaveQueue);
	bIdle = (m_queueDataSaveJob.empty() == true && m_bProcessingDataSaveJob == FALSE);
	::LeaveCriticalSection(&m_csDataSaveQueue);

	return bIdle;
}
void CDataSaveManager::CreateDirectoryRecursive(const CString strPath)
{
	if(strPath.IsEmpty())
	{
		return;
	}

	CString strFullPath = strPath;
	strFullPath.TrimRight(_T("\\"));

	int pos = 0;
	while((pos = strFullPath.Find(_T("\\"), pos + 1)) != -1)
	{
		CString strPartialPath = strFullPath.Left(pos);
		if(strPartialPath.GetLength() > 2)
		{
			CreateDirectory(strPartialPath, nullptr);
		}
	}

	CreateDirectory(strFullPath, nullptr);
}

CString CDataSaveManager::GetRawImageSaveFolder(const SDataSaveJob& job)
{
	CString savePath = ::g_ConfigManager.m_strRawImageSavePath;
	if(savePath == _T(""))
	{
		return _T("");
	}

	CString barcode = job.barcode;
	if(barcode == _T(""))
	{
		barcode = _T("BarcodeEmpty");
	}

	CString scanTime = job.time;
	if(scanTime == _T(""))
	{
		scanTime = m_strTime;
	}
	if(scanTime == _T(""))
	{
		COleDateTime currentTime = COleDateTime::GetCurrentTime();
		scanTime = currentTime.Format(_T("%Y%m%d_%H%M%S"));
	}

	CString fileName = ::g_RecipeManager.m_currentRecipeName;
	int lastSlash = fileName.ReverseFind('\\');
	if(lastSlash >= 0)
	{
		fileName = fileName.Mid(lastSlash + 1);
	}

	int dotPos = fileName.ReverseFind('.');
	CString fileNameWithoutExt = (dotPos >= 0) ? fileName.Left(dotPos) : fileName;
	CString prefix = fileNameWithoutExt.Left(3);

	CString inspectionType(_T("Undefined"));
	if(prefix == _T("Pre"))
	{
		inspectionType = _T("Before");
	}
	else if(prefix == _T("Pos"))
	{
		inspectionType = _T("After");
	}

	CString year, month, day;
	if(scanTime.GetLength() >= 8)
	{
		year = scanTime.Left(4);
		month = scanTime.Mid(4, 2);
		day = scanTime.Mid(6, 2);
	}
	else
	{
		CTime curTime(CTime::GetCurrentTime());
		year.Format(_T("%04d"), curTime.GetYear());
		month.Format(_T("%02d"), curTime.GetMonth());
		day.Format(_T("%02d"), curTime.GetDay());
	}

	CString cycleKey;
	cycleKey.Format(_T("%s|%s|%s"), inspectionType, barcode, scanTime);
	if(cycleKey == m_strCurrentCycleKey && m_strCurrentSaveFolder != _T(""))
	{
		return m_strCurrentSaveFolder;
	}

	CString strFullPath;
	strFullPath.Format(_T("%s\\%s\\%s\\%s\\%s"), savePath, inspectionType, year, month, day);
	CreateDirectoryRecursive(strFullPath);

	CString strBcrPath;
	strBcrPath.Format(_T("%s\\%s"), strFullPath, barcode);
	if(PathFileExists(strBcrPath))
	{
		int folderSuffix = 1;
		CString strNewBcrPath;
		do
		{
			strNewBcrPath.Format(_T("%s\\%s_%d"), strFullPath, barcode, folderSuffix++);
		} while(PathFileExists(strNewBcrPath));

		strBcrPath = strNewBcrPath;
	}

	CreateDirectory(strBcrPath, nullptr);

	m_strCurrentCycleKey = cycleKey;
	m_strCurrentSaveFolder = strBcrPath;
	m_strCurrentBarcode = barcode;
	m_strYear = year;
	m_strMonth = month;
	m_strDay = day;
	m_strInspectionType = inspectionType;

	return m_strCurrentSaveFolder;
}

void CDataSaveManager::RawImageSaveJob(const SDataSaveJob& job)
{
	m_strCurrentImageName = _T("");

	if(::g_ConfigManager.m_strRawImageSavePath == _T(""))
	{
		::g_Log.Log(_T("RawImageSave::Save Fail - Path Error!!"));
		return;
	}

	if(job.rawImage.empty() == true || job.imageWidth <= 0 || job.imageHeight <= 0)
	{
		::g_Log.Log(_T("[STORAGE_IO][WARN] RawImageSave skipped - empty payload Cam:%d Div:%d Barcode:%s Time:%s"), job.camIdx, job.divIdx, job.barcode, job.time);
		return;
	}

	CString strSaveFolder = GetRawImageSaveFolder(job);
	if(strSaveFolder == _T(""))
	{
		::g_Log.Log(_T("RawImageSave::Save Fail - Folder Error!!"));
		return;
	}

	CString strPath;
	strPath.Format(_T("%s\\%d_%d_raw.bmp"), strSaveFolder, job.camIdx, job.divIdx);
	if(PathFileExists(strPath))
	{
		DeleteFile(strPath);
	}

	DWORD dwWriteBeginTick = GetTickCount();
	BYTE* pImage = const_cast<BYTE*>(&job.rawImage[0]);
	HUBSDK::ImageInterface::ImageSaveBMP(strPath, pImage, job.imageWidth, job.imageHeight, 8, CRect(0, 0, job.imageWidth, job.imageHeight), TRUE);
	DWORD dwWriteElapsed = GetTickCount() - dwWriteBeginTick;

	m_strCurrentImageName = strPath;
	if(::g_ConfigManager.m_bUseAfterInspectionDL == TRUE)
	{
		m_strYOLOResultImageName = MakeYOLOResultPath(strPath);
	}

	if(dwWriteElapsed > 500)
	{
		::g_Log.Log(_T("[STORAGE_IO][WARN] RawImageSave slow - Cam:%d Div:%d elapsed:%lu ms Path:%s"), job.camIdx, job.divIdx, (unsigned long)dwWriteElapsed, strPath);
	}
}
void CDataSaveManager::ResultDataSave(const BOOL bManualInspection)
{
	auto savePath = ::g_ConfigManager.m_strResultImageSavePath;

	if(savePath == _T(""))
	{
		::g_Log.Log(_T("ResultDataSave::Save Fail - Path Error!!"));
		return;
	}

	CString strSavePath = GetSavePath(savePath, bManualInspection);

	CString strPath, strResult(_T(""));
	switch(m_nResult)
	{
	case 0: // NG
		strResult.Format(_T("NG"));
		break;
	case 1: // OK
		strResult.Format(_T("OK"));
		break;
	}

	strPath.Format(_T("%s\\%s_%s_%d_%d_result_%s.csv"), strSavePath, m_strBarcode, m_strTime, m_nCamIdx, m_nDivIdx, strResult);

	switch(::g_ConfigManager.m_nCustomer)
	{
	case (long)CUSTOMER::BORGWARNER:
		ResultDataSave_forBorgWarner(strPath);
		break;
	}
}

void CDataSaveManager::ResultImageSave(const BOOL bManualInspection)
{
	CString strTemp;

	auto savePath = ::g_ConfigManager.m_strResultImageSavePath;

	if(savePath == _T(""))
	{
		::g_Log.Log(_T("ResultImageSave::Save Fail - Path Error!!"));
		return;
	}

	BYTE* pbyImage = ::g_MemoryManager.GetByteImage(m_nCamIdx);

	CImage* image = new CImage;
	image->Destroy();
	image->Create(m_nImageWidth, m_nImageHeight, 24);

	::g_Algorithm.GetGrayToColorImage(pbyImage, m_nImageWidth, m_nImageHeight, 8, CRect(0, 0, m_nImageWidth, m_nImageHeight), image);

	if(image != nullptr)
	{
		CDC* pDC = CDC::FromHandle(image->GetDC());

		pDC->SetBkMode(TRANSPARENT);
		CBrush redBrush(RGB(255, 0, 0));
		CPen redPen(PS_SOLID, 3, RGB(255, 0, 0));
		CPen redPen2(PS_SOLID, 10, RGB(255, 0, 0));

		CBrush greenBrush(RGB(0, 255, 0));
		CPen greenPen(PS_SOLID, 3, RGB(0, 255, 0));

		CBrush yellowBrush(RGB(255, 255, 0));
		CPen yellowPen(PS_SOLID, 3, RGB(255, 255, 0));

		CFont *pOldFont;
		pOldFont = pDC->GetCurrentFont();

		CFont font;
		font.CreatePointFont(750, _T("Tahoma"));

		pDC->SelectObject(&font);

		switch(m_nResult)
		{
		case 0: // NG
			pDC->SelectObject(&redBrush);
			pDC->SelectObject(&redPen);
			break;
		case 1: // OK
			pDC->SelectObject(&greenBrush);
			pDC->SelectObject(&greenPen);
			break;
		default:
			pDC->SelectObject(&yellowBrush);
			pDC->SelectObject(&yellowPen);
			break;
		}

		// PLCÍ∞Ä ?êÎèô Î™®Îìú???åÎäî autoÎ°? ?òÎèô Î™®Îìú???åÎäî ManualÎ°??ôÏûë
		auto overlayManager = ::g_InspectionManager.m_pInspection->m_pOverlayManager;

		for(long nStep = DEBUG_RESULT; nStep <= DEBUG_RESULT_NG; nStep++)
		{
			long nType;
			long nOverlay = 0;

			COLORREF color = overlayManager->GetStepColor(nStep);
			void *data = NULL;

			while(data = overlayManager->GetData(nStep, nOverlay, nType, TRUE))
			{
				switch(nType)
				{
				case INTERMEDIATE_RECT:
				{
					CRect &sdata = *(CRect *)data;

					pDC->MoveTo(sdata.left, sdata.top);
					pDC->LineTo(sdata.right, sdata.top);
					pDC->MoveTo(sdata.left, sdata.top);
					pDC->LineTo(sdata.left, sdata.bottom);
					pDC->MoveTo(sdata.left, sdata.bottom);
					pDC->LineTo(sdata.right, sdata.bottom);
					pDC->MoveTo(sdata.right, sdata.top);
					pDC->LineTo(sdata.right, sdata.bottom);
				}
				break;
				case INTERMEDIATE_CRPOINT:		pDC->SetPixelV(*(CPoint *)data, color);
					break;
				case INTERMEDIATE_POINT:		pDC->SetPixelV(*(CPoint *)data, color);
					break;
				case INTERMEDIATE_LINE:
				{
					SOverlay_Line &sdata = *(SOverlay_Line *)data;
					pDC->MoveTo(sdata.ptStart.x, sdata.ptStart.y);
					pDC->LineTo(sdata.ptEnd.x, sdata.ptEnd.y);
				}
				break;
				case INTERMEDIATE_STRING:
				{
					SOverlay_String &sdata = *(SOverlay_String *)data;
					pDC->DrawText(sdata.strText, CRect(sdata.ptXY.x - 5, sdata.ptXY.y - 5, sdata.ptXY.x + 5, sdata.ptXY.y + 5), DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
				}
				break;
				case INTERMEDIATE_ELLIPSE:
					pDC->Ellipse(*(CRect *)data);
					break;
				}

				nOverlay++;
			}
		}

		image->ReleaseDC();

		CString strSavePath = GetSavePath(savePath, bManualInspection);

		CString strPath, strResult(_T(""));
		switch(m_nResult)
		{
		case 0: // NG
			strResult.Format(_T("NG"));
			break;
		case 1: // OK
			strResult.Format(_T("OK"));
			break;
		}

		strPath.Format(_T("%s\\%s_%s_%d_%d_result_%s.jpg"), strSavePath, m_strBarcode, m_strTime, m_nCamIdx, m_nDivIdx, strResult);

		// image->Save(strPath); // ?¥Î?ÏßÄ ?Ä?•Ïïà?òÎèÑÎ°??òÏ†ï
	}

	delete image;
}

void CDataSaveManager::RawImageSave(const BOOL bManualInspection)
{
	SDataSaveJob job;
	job.method = DATA_SAVE::RAW_IMAGE;
	job.barcode = m_strBarcode;
	job.time = m_strTime;
	job.camIdx = m_nCamIdx;
	job.divIdx = m_nDivIdx;
	job.result = m_nResult;
	job.imageWidth = m_nImageWidth;
	job.imageHeight = m_nImageHeight;
	job.imageSize = m_nImageSize;
	job.manualInspection = bManualInspection;

	if(job.imageSize <= 0)
	{
		job.imageWidth = ::g_MemoryManager.m_nImageWidth;
		job.imageHeight = ::g_MemoryManager.m_nImageHeight;
		job.imageSize = job.imageWidth * job.imageHeight;
	}

	if(job.imageSize <= 0)
	{
		::g_Log.Log(_T("RawImageSave::Save Fail - Image Size Error!!"));
		return;
	}

	const long imageIndex = m_nCamIdx + m_nDivIdx * 2;
	BYTE* image = ::g_MemoryManager.GetByteImage(imageIndex);
	if(image == nullptr)
	{
		::g_Log.Log(_T("[STORAGE_IO][WARN] RawImageSave skipped - image buffer null Cam:%d Div:%d Index:%d"), m_nCamIdx, m_nDivIdx, imageIndex);
		return;
	}

	job.rawImage.resize(job.imageSize);
	memcpy(&job.rawImage[0], image, job.imageSize);

	RawImageSaveJob(job);
}
CString CDataSaveManager::MakeYOLOResultPath(const CString& originalPath)
{
	CString resultPath = originalPath;

	// ?åÏùºÎ™ÖÏóê???ïÏû•???ÑÏπò Ï∞æÍ∏∞
	int dotPos = resultPath.ReverseFind('.');
	if (dotPos != -1)
	{
		resultPath.Insert(dotPos, _T("_result"));
	}

	return resultPath;
}

void CDataSaveManager::RawImageSave2(const BOOL bManualInspection) // 241223 sjpark ?àÎ°ú??save ?®Ïàò Ï∂îÍ? Î≤ÑÏ†Ñ
{
	auto savePath = ::g_ConfigManager.m_strResultImageSavePath;

	if (savePath == _T(""))
	{
		::g_Log.Log(_T("RawImageSave::Save Fail - Path Error!!"));
		return;
	}

	CString strSavePath = GetSavePath(savePath, bManualInspection);

	CString strPath, strBcrPath, strTime;//CString strPath, strBcrPath;

	// 6Í∞úÏùò ?¥Î?ÏßÄÎ•?Ï≤òÎ¶¨?òÎäî Í≤ΩÏö∞, ???¥Îçî ?ùÏÑ±
	if (m_ImageCount == 0)
	{
		// ?ÑÏû¨ ?úÍ∞Ñ ?ïÎ≥¥ ?ùÏÑ±
		CTime curTime(CTime::GetCurrentTime());
		CString strFolderTime;
		strFolderTime.Format(_T("%04d%02d%02d_%02d%02d"),
			curTime.GetYear(), curTime.GetMonth(), curTime.GetDay(),
			curTime.GetHour(), curTime.GetMinute());

		strBcrPath.Format(_T("%s\\%s"), strSavePath, m_strBarcode);

		while (PathFileExists(strBcrPath)) {

			if (m_strBeforpath == strBcrPath)
			{
				break;
			}
			strBcrPath.Format(_T("%s\\%s_%s"), strSavePath, m_strBarcode, strTime);//LYS 241018 Î∞îÏΩî???¥ÎçîÎ™???Ï§ëÎ≥µ???ºÌïòÍ∏??ÑÌï¥ ?úÍ∞Ñ??Î∂ôÏó¨???¥ÎçîÎ™ÖÏùÑ ?ùÏÑ±.
			m_strBeforpath = strBcrPath;
			break;
		}
		CreateDirectory(strBcrPath, nullptr);
	}

	// ?¥Î?ÏßÄ ?åÏùºÎ™??ùÏÑ±
	CString strFileName;
	strFileName.Format(_T("%s\\%d_%d_raw.bmp"), strBcrPath, m_nCamIdx, m_nDivIdx);

	// ?¥Î?ÏßÄ ?Ä??
	BYTE* image = ::g_MemoryManager.GetByteImage(m_nCamIdx + m_nDivIdx * 2);
	HUBSDK::ImageInterface::ImageSaveBMP(strFileName, image, m_nImageWidth, m_nImageHeight, 8,
		CRect(0, 0, m_nImageWidth, m_nImageHeight), TRUE);

	// ?¥Î?ÏßÄ ?Ä??Ïπ¥Ïö¥??Ï¶ùÍ?
	m_ImageCount++;

	// 6Î≤àÏß∏ ?¥Î?ÏßÄÍπåÏ? ?Ä?•ÎêòÎ©?Ïπ¥Ïö¥??Ï¥àÍ∏∞??
	if (m_ImageCount == 5)
	{
		m_ImageCount = 0;  // Ïπ¥Ïö¥??Ï¥àÍ∏∞??
		strBcrPath.Empty();  // ???¥ÎçîÎ•??ùÏÑ±?????àÎèÑÎ°?Ï¥àÍ∏∞??
	}
}

CString CDataSaveManager::CreateSaveFolder(CString strPath)
{
	CreateDirectory(strPath, NULL);

	return strPath;
}

DWORDLONG CDataSaveManager::MyGetDiskFreeSpaceEx(LPCSTR pszDrive)
{
	PGETDISKFREESPACEEX pGetDiskFreeSpaceEx;
	__int64 i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;

	DWORD dwSectPerClust,
		dwBytesPerSect,
		dwFreeClusters,
		dwTotalClusters;

	BOOL fResult;

	pGetDiskFreeSpaceEx = (PGETDISKFREESPACEEX)GetProcAddress(
		GetModuleHandle(_T("kernel32.dll")),
		"GetDiskFreeSpaceExA");

	if(pGetDiskFreeSpaceEx)
	{
		fResult = pGetDiskFreeSpaceEx(pszDrive,
			(PULARGE_INTEGER)&i64FreeBytesToCaller,
			(PULARGE_INTEGER)&i64TotalBytes,
			(PULARGE_INTEGER)&i64FreeBytes);

		// Process GetDiskFreeSpaceEx results.

		return i64FreeBytes;
	}

	else
	{
		fResult = GetDiskFreeSpaceA(pszDrive,
			&dwSectPerClust,
			&dwBytesPerSect,
			&dwFreeClusters,
			&dwTotalClusters);

		// Process GetDiskFreeSpace results.
		return dwFreeClusters * dwSectPerClust*dwBytesPerSect;
	}

	return 0;
}

CString CDataSaveManager::GetSavePath(const CString strPath, const BOOL bManualInspection)
{
	CString strSavePath = _T("");
	strSavePath = CreateSaveFolder(strPath);

	if(bManualInspection == TRUE)
	{
		strSavePath += _T("\\Manual");
		CreateDirectory(strSavePath, nullptr);
	}

	CString strInspDivision(_T(""));
	switch(::g_RecipeManager.m_sBusbarInfoParam.nInspectionDivision)
	{
	case (long)INSPECTION_DIVISION::BEFORE: strInspDivision.Format(_T("\\Before")); break;
	case (long)INSPECTION_DIVISION::AFTER: strInspDivision.Format(_T("\\After")); break;
	}

	if(strInspDivision != _T(""))
	{
		strSavePath += strInspDivision;
		CreateDirectory(strSavePath, nullptr);
	}

	return strSavePath;
}

void CDataSaveManager::ResultDataSave_forBorgWarner(const CString strPath)
{
	CFile pFile;
	if(pFile.Open(strPath, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite, NULL) == FALSE)
	{
		::g_Log.Log(_T("ResultDataSave::Save Fail - File Open Error!!"));
		return;
	}

	int nLen = 0;
	char pTmpData[BUF_MAX_SIZE] = {0,};

	if(m_nDivIdx == 0)
	{
		CString strTemp;
		strTemp.Format(_T("Barcode,%s\n"), m_strBarcode);

		nLen = sprintf_s(pTmpData, BUF_MAX_SIZE, (CStringA)strTemp);
		pFile.Write(pTmpData, nLen);
		memset(pTmpData, 0, BUF_MAX_SIZE);

		strTemp.Format(_T("Time,%s\n"), m_strTime);
		nLen = sprintf_s(pTmpData, BUF_MAX_SIZE, (CStringA)strTemp);
		pFile.Write(pTmpData, nLen);
		memset(pTmpData, 0, BUF_MAX_SIZE);

		nLen = sprintf_s(pTmpData, BUF_MAX_SIZE, "Result,DivIdx,Start_X,Start_Y,End_X,END_Y,TopBusbarHeight,Length,Angle\n");
		pFile.Write(pTmpData, nLen);
		memset(pTmpData, 0, BUF_MAX_SIZE);
	}
	else
	{
		pFile.SeekToEnd();
	}

	CRect rtROI;
	CPoint ptStart;
	CPoint ptEnd;

	CString *pstrResult = nullptr;
	auto &inspection = ::g_InspectionManager.m_vecpInspection;

	if((long)inspection.size() != 0)
	{
		pstrResult = new CString[(long)inspection.size()];
	}

	CString strData = _T("");
	for(int insp = 0; insp < (long)inspection.size(); insp++)
	{
		auto &result = inspection[insp]->m_vecResult;
		auto &critera = inspection[insp]->m_vecCriteriaResult;

		for(long idx = 0; idx < result.size(); idx++)
		{
			if(critera[idx].nErr == (long)INSP_NONE)
			{
				continue;
			}

			CString str = _T("");
			str.Format(_T("%s,%d,%d,%d,%d,%d,%f,%f,%f\n")
				, (critera[idx].nErr == INSP_ERR_OK) ? _T("OK") : _T("NG")
				, m_nDivIdx
				, result[idx].ptStartResult.x
				, result[idx].ptStartResult.y
				, result[idx].ptEndResult.x
				, result[idx].ptEndResult.y
				, result[idx].fEdgeTopBusbarHeight
				, result[idx].fEdgeLength
				, result[idx].fEdgeAngle
			);
			strData += str;
		}
		pstrResult[m_nDivIdx] = strData;
	}

	if(pstrResult != nullptr)
	{
		for(int nCount = 0; nCount < inspection.size(); nCount++)
		{
			nLen = sprintf_s(pTmpData, BUF_MAX_SIZE, (CStringA)pstrResult[nCount]);
			pFile.Write(pTmpData, nLen);
			memset(pTmpData, 0, BUF_MAX_SIZE);
		}
		delete[] pstrResult;
	}

	pFile.Close();
}
