#pragma once

#include <deque>

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class CDlgLog;
class CDlgVisionMain;

class ALGORITHM_TOOLS_API CLog
{
public:
	CLog();
	~CLog();

private:
	CString m_strCurPath;
	CDlgLog* m_pDlgLog;
	CDlgVisionMain* m_pParent;

	struct SLogWriteJob
	{
		CString text;
		BOOL resultLog;

		SLogWriteJob()
			: resultLog(FALSE)
		{
		}
	};

	struct SLogWriterControl
	{
		HANDLE m_hSignal_Start;
		HANDLE m_hSignal_Kill;
		HANDLE m_Handle_Thread;
	};

	SLogWriterControl m_Control_LogWriter;
	CRITICAL_SECTION m_csLogWriteQueue;
	std::deque<SLogWriteJob> m_queueLogWrite;
	BOOL m_bLogWriterThreadInit;

	void InitLogWriterThread();
	void FreeLogWriterThread();
	static UINT ThreadLogWriterFunc(LPVOID pParam);
	void ThreadLogWriter();
	void EnqueueLogWrite(const CString strText, const BOOL bResultLog);
	BOOL PopLogWriteJob(SLogWriteJob& job);
	CString GetLogWritePath(const BOOL bResultLog);
	BOOL OpenLogWriteFile(FILE** ppFile, CString& strCurrentPath, const BOOL bResultLog);
	void WriteLogTextDirect(const CString strText, const BOOL bResultLog);

public:
	void SetCurPath(CString strCurPath);
	void SetParent(CDlgVisionMain* pParent);
	void Log(LPCTSTR szFmt, ...);
	void LogText(const CString strText);
	void LogResult(LPCTSTR szFmt, ...);
	void LogResultText(const CString strText);
	void SaveLog();
	void ClearLog();
	CString GetCurPath();
	BOOL OpenDialog(HWND i_hParentWnd);
};

extern ALGORITHM_TOOLS_API CLog g_Log;