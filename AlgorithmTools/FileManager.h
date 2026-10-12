#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CFileManager
{
public:
	CFileManager();
	~CFileManager();

	DWORD m_dwLastDeleteScanTick;

	void InitThread();
	void FreeThread();
	BOOL DeleteFolder(CString strPath);
	void PathCompare(std::vector<CString> &vecstrDataSavePath, const CString strPath);
	void GetFileList(CString strFolder, std::vector<CString> &vecstrCopyList);
	void GetFileTimes(CString strPath, COleDateTime &createTime);

	/////////////////////////////////////////////////////////////////////////////
	// File Threads
	struct SFileDeleteControl
	{
		HANDLE m_hSignal_Start;
		HANDLE m_hSignal_Kill;
		HANDLE m_Handle_Thread;
	};
	SFileDeleteControl m_Control_File_Delete;

	static UINT ThreadFileDeleteFunc(LPVOID pParam);
	void ThreadFileDelete();
	BOOL IsDeleteScanAllowed();
	/////////////////////////////////////////////////////////////////////////////
};

extern ALGORITHM_TOOLS_API CFileManager g_FileManager;