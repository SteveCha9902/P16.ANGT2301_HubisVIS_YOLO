#include "pch.h"
#include "Log.h"
#include "DlgLog.h"
#include "../VisionMain/DlgVisionMain.h"
#include "../VisionMain/DlgMain1.h"
#include "../VisionMain/DlgMain2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLog g_Log;

CLog::CLog()
	: m_pDlgLog(nullptr)
	, m_pParent(nullptr)
	, m_bLogWriterThreadInit(FALSE)
{
	m_Control_LogWriter.m_hSignal_Start = NULL;
	m_Control_LogWriter.m_hSignal_Kill = NULL;
	m_Control_LogWriter.m_Handle_Thread = INVALID_HANDLE_VALUE;
	::InitializeCriticalSection(&m_csLogWriteQueue);
}

CLog::~CLog()
{
	FreeLogWriterThread();
	::DeleteCriticalSection(&m_csLogWriteQueue);
	delete m_pDlgLog;
}

void CLog::SetCurPath(CString strCurPath)
{
	m_strCurPath = strCurPath;
	InitLogWriterThread();
}

void CLog::SetParent(CDlgVisionMain* pParent)
{
	m_pParent = pParent;
}

void CLog::InitLogWriterThread()
{
	if(m_bLogWriterThreadInit == TRUE)
	{
		return;
	}

	m_Control_LogWriter.m_hSignal_Start = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_LogWriter.m_hSignal_Kill = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_LogWriter.m_Handle_Thread = INVALID_HANDLE_VALUE;

	HANDLE hLogWriterThread = ::AfxBeginThread(ThreadLogWriterFunc, (void*)this)->m_hThread;
	::DuplicateHandle(::GetCurrentProcess(), hLogWriterThread, ::GetCurrentProcess(), &m_Control_LogWriter.m_Handle_Thread, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);

	m_bLogWriterThreadInit = TRUE;
}

void CLog::FreeLogWriterThread()
{
	if(m_bLogWriterThreadInit == FALSE)
	{
		return;
	}

	::SetEvent(m_Control_LogWriter.m_hSignal_Kill);
	::SetEvent(m_Control_LogWriter.m_hSignal_Start);
	::WaitForSingleObject(m_Control_LogWriter.m_Handle_Thread, 3000);

	::CloseHandle(m_Control_LogWriter.m_hSignal_Start);
	::CloseHandle(m_Control_LogWriter.m_hSignal_Kill);
	::CloseHandle(m_Control_LogWriter.m_Handle_Thread);

	m_Control_LogWriter.m_hSignal_Start = NULL;
	m_Control_LogWriter.m_hSignal_Kill = NULL;
	m_Control_LogWriter.m_Handle_Thread = INVALID_HANDLE_VALUE;
	m_bLogWriterThreadInit = FALSE;
}

UINT CLog::ThreadLogWriterFunc(LPVOID pParam)
{
	CLog* pLog = (CLog*)pParam;
	pLog->ThreadLogWriter();
	return 0;
}

void CLog::ThreadLogWriter()
{
	HANDLE pHandles[] = {m_Control_LogWriter.m_hSignal_Start, m_Control_LogWriter.m_hSignal_Kill};
	const DWORD nSignalCount = sizeof(pHandles) / sizeof(HANDLE);

	FILE* pLogFile = nullptr;
	FILE* pResultLogFile = nullptr;
	CString strCurrentLogPath(_T(""));
	CString strCurrentResultLogPath(_T(""));

	while(TRUE)
	{
		DWORD dwRet = ::WaitForMultipleObjects(nSignalCount, pHandles, FALSE, 200);
		if(dwRet == WAIT_OBJECT_0 + 1)
		{
			break;
		}

		SLogWriteJob job;
		int nWriteCount = 0;
		while(PopLogWriteJob(job) == TRUE)
		{
			FILE** ppFile = &pLogFile;
			CString* pCurrentPath = &strCurrentLogPath;
			if(job.resultLog == TRUE)
			{
				ppFile = &pResultLogFile;
				pCurrentPath = &strCurrentResultLogPath;
			}

			if(OpenLogWriteFile(ppFile, *pCurrentPath, job.resultLog) == TRUE && *ppFile != nullptr)
			{
				CString strTextFinal;
				strTextFinal.Format(_T("%s\n"), job.text);
				CStringA strTextA(strTextFinal);
				fputs(LPCSTR(strTextA), *ppFile);
				nWriteCount++;

				if(nWriteCount % 100 == 0)
				{
					fflush(*ppFile);
				}
			}
		}

		if(nWriteCount > 0)
		{
			if(pLogFile != nullptr)
			{
				fflush(pLogFile);
			}
			if(pResultLogFile != nullptr)
			{
				fflush(pResultLogFile);
			}
		}
	}

	SLogWriteJob job;
	while(PopLogWriteJob(job) == TRUE)
	{
		FILE** ppFile = &pLogFile;
		CString* pCurrentPath = &strCurrentLogPath;
		if(job.resultLog == TRUE)
		{
			ppFile = &pResultLogFile;
			pCurrentPath = &strCurrentResultLogPath;
		}

		if(OpenLogWriteFile(ppFile, *pCurrentPath, job.resultLog) == TRUE && *ppFile != nullptr)
		{
			CString strTextFinal;
			strTextFinal.Format(_T("%s\n"), job.text);
			CStringA strTextA(strTextFinal);
			fputs(LPCSTR(strTextA), *ppFile);
		}
	}

	if(pLogFile != nullptr)
	{
		fflush(pLogFile);
		fclose(pLogFile);
		pLogFile = nullptr;
	}
	if(pResultLogFile != nullptr)
	{
		fflush(pResultLogFile);
		fclose(pResultLogFile);
		pResultLogFile = nullptr;
	}
}
void CLog::EnqueueLogWrite(const CString strText, const BOOL bResultLog)
{
	if(m_bLogWriterThreadInit == FALSE)
	{
		WriteLogTextDirect(strText, bResultLog);
		return;
	}

	SLogWriteJob job;
	job.text = strText;
	job.resultLog = bResultLog;

	::EnterCriticalSection(&m_csLogWriteQueue);
	m_queueLogWrite.push_back(job);
	::LeaveCriticalSection(&m_csLogWriteQueue);

	::SetEvent(m_Control_LogWriter.m_hSignal_Start);
}

BOOL CLog::PopLogWriteJob(SLogWriteJob& job)
{
	BOOL bRet = FALSE;

	::EnterCriticalSection(&m_csLogWriteQueue);
	if(m_queueLogWrite.empty() == false)
	{
		job = m_queueLogWrite.front();
		m_queueLogWrite.pop_front();
		bRet = TRUE;
	}
	::LeaveCriticalSection(&m_csLogWriteQueue);

	return bRet;
}

CString CLog::GetLogWritePath(const BOOL bResultLog)
{
	CString strFolder;
	strFolder.Format(_T("%s\\%s"), m_strCurPath, bResultLog == TRUE ? _T("Result") : _T("Log"));
	CreateDirectory(strFolder, nullptr);

	COleDateTime currentTime = COleDateTime::GetCurrentTime();
	CString strDate = currentTime.Format(_T("%Y%m%d_%H"));

	CString strLog;
	strLog.Format(_T("%s\\Log_%s.txt"), strFolder, strDate);
	return strLog;
}

BOOL CLog::OpenLogWriteFile(FILE** ppFile, CString& strCurrentPath, const BOOL bResultLog)
{
	CString strPath = GetLogWritePath(bResultLog);
	if(*ppFile != nullptr && strCurrentPath == strPath)
	{
		return TRUE;
	}

	if(*ppFile != nullptr)
	{
		fflush(*ppFile);
		fclose(*ppFile);
		*ppFile = nullptr;
	}

	CStringA strPathA(strPath);
	fopen_s(ppFile, LPCSTR(strPathA), "a+");
	if(*ppFile == nullptr)
	{
		return FALSE;
	}

	strCurrentPath = strPath;
	return TRUE;
}

void CLog::WriteLogTextDirect(const CString strText, const BOOL bResultLog)
{
	CString strPath = GetLogWritePath(bResultLog);
	CStringA strPathA(strPath);

	FILE* fp = nullptr;
	fopen_s(&fp, LPCSTR(strPathA), "a+");
	if(fp == nullptr)
	{
		return;
	}

	CString strTextFinal;
	strTextFinal.Format(_T("%s\n"), strText);
	CStringA strTextA(strTextFinal);
	fputs(LPCSTR(strTextA), fp);
	fclose(fp);
}
void CLog::Log(LPCTSTR szFmt, ...)
{
	CString strLog;

	va_list argptr;
	va_start(argptr, szFmt);
	strLog.FormatV(szFmt, argptr);
	va_end(argptr);

	SYSTEMTIME st;
	GetLocalTime(&st);

	CString strWrite;
	//strWrite.Format(_T("[%04d-%02d-%02d %02d:%02d:%02d.%03d] "), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	strWrite.Format(_T("[%02d:%02d:%02d.%03d] "), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	strWrite += strLog;

	::EnterCriticalSection(&m_pDlgLog->m_csLog);
	m_pDlgLog->m_pendingLog.push_back(strWrite);

	m_pParent->m_strMessage = strLog;
	::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_MESSAGE, NULL, NULL);

	::PostMessage(m_pDlgLog->GetSafeHwnd(), UM_UPDATE_LOG, NULL, NULL);
	::LeaveCriticalSection(&m_pDlgLog->m_csLog);

	// DlgMain1 Log...
	auto dlgMain1 = m_pParent->m_pDlgMain1;
	if(dlgMain1 != nullptr)
	{
		::EnterCriticalSection(&dlgMain1->m_csLog);
		dlgMain1->m_pendingLog.push_back(strWrite);
		::PostMessage(dlgMain1->GetSafeHwnd(), UM_UPDATE_LOG, NULL, NULL);
		::LeaveCriticalSection(&dlgMain1->m_csLog);
	}

	// DlgMain2 Log...
	auto dlgMain2 = m_pParent->m_pDlgMain2;
	if(dlgMain2 != nullptr)
	{
		::EnterCriticalSection(&dlgMain2->m_csLog);
		dlgMain2->m_pendingLog.push_back(strWrite);
		::PostMessage(dlgMain2->GetSafeHwnd(), UM_UPDATE_LOG, NULL, NULL);
		::LeaveCriticalSection(&dlgMain2->m_csLog);
	}

	LogText(strWrite);
}

// Log 파일을 남긴다...
void CLog::LogText(const CString strText)
{
	EnqueueLogWrite(strText, FALSE);
}

// 250613 sjpark Result 결과파일의 Log 파일을 남긴다.(기존 CLog::Log 함수 대비 결과값을 arg로 받아서 별도의 경로에서 처리)
void CLog::LogResult(LPCTSTR szFmt, ...)
{
	CString strLog;

	va_list argptr;
	va_start(argptr, szFmt);
	strLog.FormatV(szFmt, argptr);
	va_end(argptr);

	SYSTEMTIME st;
	GetLocalTime(&st);

	CString strWrite;
	//strWrite.Format(_T("[%04d-%02d-%02d %02d:%02d:%02d.%03d] "), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	strWrite.Format(_T("[%02d:%02d:%02d.%03d] "), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	strWrite += strLog;

	::EnterCriticalSection(&m_pDlgLog->m_csLog);
	m_pDlgLog->m_pendingLog.push_back(strWrite);

	m_pParent->m_strMessage = strLog;
	::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_MESSAGE, NULL, NULL);

	::PostMessage(m_pDlgLog->GetSafeHwnd(), UM_UPDATE_LOG, NULL, NULL);
	::LeaveCriticalSection(&m_pDlgLog->m_csLog);

	// DlgMain1 Log...
	auto dlgMain1 = m_pParent->m_pDlgMain1;
	if (dlgMain1 != nullptr)
	{
		::EnterCriticalSection(&dlgMain1->m_csLog);
		dlgMain1->m_pendingLog.push_back(strWrite);
		::PostMessage(dlgMain1->GetSafeHwnd(), UM_UPDATE_LOG, NULL, NULL);
		::LeaveCriticalSection(&dlgMain1->m_csLog);
	}

	// DlgMain2 Log...
	auto dlgMain2 = m_pParent->m_pDlgMain2;
	if (dlgMain2 != nullptr)
	{
		::EnterCriticalSection(&dlgMain2->m_csLog);
		dlgMain2->m_pendingLog.push_back(strWrite);
		::PostMessage(dlgMain2->GetSafeHwnd(), UM_UPDATE_LOG, NULL, NULL);
		::LeaveCriticalSection(&dlgMain2->m_csLog);
	}

	LogResultText(strWrite);
}

// 250613 sjpark Result 결과파일의 Log 파일을 남긴다. - 실제내용 작성부
void CLog::LogResultText(const CString strText)
{
	EnqueueLogWrite(strText, TRUE);
}

void CLog::SaveLog()
{
	if(m_pDlgLog == nullptr)
		return;

	m_pDlgLog->SaveLog();
}

void CLog::ClearLog()
{
	if(m_pDlgLog == nullptr)
		return;

	m_pDlgLog->ClearLog();
}

CString CLog::GetCurPath()
{
	return m_strCurPath;
}

BOOL CLog::OpenDialog(HWND i_hParentWnd)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CWnd pWnd;
	if(m_pDlgLog == nullptr)
	{
		m_pDlgLog = new CDlgLog(this);
		pWnd.Attach(i_hParentWnd);
		m_pDlgLog->Create(CDlgLog::IDD, &pWnd);
		m_pDlgLog->ShowWindow(SW_SHOW);
		pWnd.Detach();
	}
	else
	{
		m_pDlgLog->ShowWindow(SW_SHOW);
	}

	//m_pDlgLog->SetData();

	return TRUE;
}