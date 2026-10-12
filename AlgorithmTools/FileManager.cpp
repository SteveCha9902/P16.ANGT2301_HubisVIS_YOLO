#include "pch.h"
#include "FileManager.h"

#define FILE_DELETE_MIN_SCAN_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFileManager g_FileManager;

CFileManager::CFileManager()
	: m_dwLastDeleteScanTick(0)
{
	m_Control_File_Delete.m_hSignal_Start = NULL;
	m_Control_File_Delete.m_hSignal_Kill = NULL;
	m_Control_File_Delete.m_Handle_Thread = INVALID_HANDLE_VALUE;
}
CFileManager::~CFileManager()
{
}

void CFileManager::InitThread()
{
	// Image Delete Thread 초기화
	m_Control_File_Delete.m_hSignal_Start = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_File_Delete.m_hSignal_Kill = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_File_Delete.m_Handle_Thread = INVALID_HANDLE_VALUE;

	HANDLE hFileDeleteThread = ::AfxBeginThread(ThreadFileDeleteFunc, (void *)this)->m_hThread;
	::DuplicateHandle(::GetCurrentProcess(), hFileDeleteThread, ::GetCurrentProcess(), &m_Control_File_Delete.m_Handle_Thread, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);
}

void CFileManager::FreeThread()
{
	if(m_Control_File_Delete.m_Handle_Thread == INVALID_HANDLE_VALUE)
	{
		return;
	}

	// 모든 쓰레드에게 일을 그만하라고 알린다.
	::SetEvent(m_Control_File_Delete.m_hSignal_Kill);

	// 쓰레드가 종료되기를 기다린다.
	::WaitForSingleObject(m_Control_File_Delete.m_Handle_Thread, 1000);

	// 관련 핸들을 모두 닫는다.
	::CloseHandle(m_Control_File_Delete.m_hSignal_Start);
	::CloseHandle(m_Control_File_Delete.m_hSignal_Kill);
	::CloseHandle(m_Control_File_Delete.m_Handle_Thread);
	m_Control_File_Delete.m_hSignal_Start = NULL;
	m_Control_File_Delete.m_hSignal_Kill = NULL;
	m_Control_File_Delete.m_Handle_Thread = INVALID_HANDLE_VALUE;
}
UINT CFileManager::ThreadFileDeleteFunc(LPVOID pParam)
{
	CFileManager *pFileManager = (CFileManager *)pParam;

	pFileManager->ThreadFileDelete();

	return 0;
}

void CFileManager::ThreadFileDelete()
{
	HANDLE pHandles[2] = {m_Control_File_Delete.m_hSignal_Start, m_Control_File_Delete.m_hSignal_Kill};

	::g_Log.Log(_T("File Delete Thread Ready"));

	while(1)
	{
		DWORD dwRet = ::WaitForMultipleObjects(2, pHandles, FALSE, INFINITE);

		if(dwRet == WAIT_OBJECT_0)
		{
			if(IsDeleteScanAllowed() == FALSE)
			{
				continue;
			}

			if(::g_DataSaveManager.IsQueueIdle() == FALSE)
			{
				::g_Log.Log(_T("[FILE_DELETE] Skip - DataSave queue is busy"));
				continue;
			}

			DWORD dwScanBeginTick = GetTickCount();
			m_dwLastDeleteScanTick = dwScanBeginTick;
			::g_Log.Log(_T("[FILE_DELETE] Scan Start"));

			std::vector<CString> vecstrDataSavePath;

			PathCompare(vecstrDataSavePath, ::g_ConfigManager.m_strRawImageSavePath);
			PathCompare(vecstrDataSavePath, ::g_ConfigManager.m_strResultImageSavePath);
			PathCompare(vecstrDataSavePath, ::g_ConfigManager.m_strResultDataSavePath);

			std::vector<CString> vecstrFilePathList;
			for(long idx = 0; idx < (long)vecstrDataSavePath.size(); idx++)
			{
				GetFileList(vecstrDataSavePath[idx], vecstrFilePathList);
			}

			COleDateTime curDate = COleDateTime::GetCurrentTime();
			COleDateTime refDate = curDate;
			refDate -= ::g_ConfigManager.m_nAutoDeleteDays;

			long nDeleteCount = 0;
			for(long path = 0; path < (long)vecstrFilePathList.size(); path++)
			{
				COleDateTime fileCreateTime;
				GetFileTimes(vecstrFilePathList[path], fileCreateTime);

				if(refDate > fileCreateTime)
				{
					if(DeleteFile(vecstrFilePathList[path]) != FALSE)
					{
						nDeleteCount++;
					}
				}
			}

			::g_Log.Log(_T("[FILE_DELETE] Scan End - files:%d deleted:%d elapsed:%lu ms"), (long)vecstrFilePathList.size(), nDeleteCount, (unsigned long)(GetTickCount() - dwScanBeginTick));
		}
		else
		{
			break;
		}
	}
}

BOOL CFileManager::IsDeleteScanAllowed()
{
	if(m_dwLastDeleteScanTick == 0)
	{
		return TRUE;
	}

	DWORD dwElapsed = GetTickCount() - m_dwLastDeleteScanTick;
	return dwElapsed >= FILE_DELETE_MIN_SCAN_INTERVAL_MS;
}
BOOL CFileManager::DeleteFolder(CString strPath)
{
	if(strPath.IsEmpty())
		return FALSE;

	BOOL bVal = FALSE;
	int nVal = 0;

	CString strNextDirPath;
	CString strRootPath;
	CFileFind ff;

	bVal = ff.FindFile(strPath);
	if(!bVal)
		return bVal;

	while(bVal)
	{
		bVal = ff.FindNextFile();
		if(ff.IsDots())
			continue;

		if(ff.IsDirectory())
		{
			strNextDirPath.Format(_T("%s\\*.*"), ff.GetFilePath());
			DeleteFolder(strNextDirPath);
		}
		else
			DeleteFile(ff.GetFilePath());
	}

	strRootPath = ff.GetRoot();
	ff.Close();

	bVal = RemoveDirectory(strRootPath);

	return bVal;
}

void CFileManager::PathCompare(std::vector<CString> &vecstrDataSavePath, const CString strPath)
{
	if(strPath.IsEmpty())
	{
		return;
	}

	BOOL bExist(FALSE);
	for(long idx = 0; idx < (long)vecstrDataSavePath.size(); idx++)
	{
		if(vecstrDataSavePath[idx] == strPath)
		{
			bExist = TRUE;
			break;
		}
	}

	if(bExist == FALSE)
	{
		vecstrDataSavePath.push_back(strPath);
	}
}
void CFileManager::GetFileList(CString strFolder, std::vector<CString> &vecstrFilePathList)
{
	CFileFind file;
	BOOL b = file.FindFile(strFolder + _T("\\*.*"));		// 모든 확장자를 다 사용.
	CString strFolderItem, strFileExt, strTempString;

	while(b)
	{
		b = file.FindNextFile();
		if(file.IsDirectory() && !file.IsDots())			// 디렉토리 발견시
		{
			strFolderItem = file.GetFilePath();
			GetFileList(strFolderItem, vecstrFilePathList);						// 하위폴더를 검색하기 위해 재귀호출 발생
		}

		strFolderItem = file.GetFilePath();
		strFileExt = strFolderItem.Mid(strFolderItem.ReverseFind('.'));		// 확장자만 추출한다.

		if(!file.IsDots())									// 파일 탐색 필터 정의에따라 해당 StringList에 추가
		{
			strFileExt.MakeUpper();							// strFileExt 에는 확장자 (.EXE 형태) 가 들어옴. 비교위해 대문자화 함
			if(file.IsDirectory())
				continue;				// 폴더만 남는 경우는 넣으면 안됨

			vecstrFilePathList.push_back(strFolderItem);
		}
	}
}

void CFileManager::GetFileTimes(CString strPath, COleDateTime &createTime)
{
	if(strPath.IsEmpty())
		return;

	HANDLE hFile = NULL;
	try
	{
		// 파일을 엽니다. (파일 핸들을 연다고 표현함)
		hFile = CreateFile(strPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			FILETIME timeCreate;
			FILETIME timeAccess;
			FILETIME timeWrite;

			// 여기서 생성 시간, 접근 시간, 쓰기 시간을 받아옵니다.
			BOOL bRet = GetFileTime(hFile, &timeCreate, &timeAccess, &timeWrite);
			if(bRet == FALSE)
				return;

			SYSTEMTIME stimeCreate;
			SYSTEMTIME sTimeTemp{};
			SYSTEMTIME* outCreate = &sTimeTemp;

			if(outCreate != nullptr)
			{
				// 받아온 생성 시간을 시스템 시간으로 변환 (이때 시간은 UTC 시간으로 반환)
				bRet = FileTimeToSystemTime(&timeCreate, &stimeCreate);

				if(bRet) // UTC 시간을 Local 시간으로 변환
				{
					SystemTimeToTzSpecificLocalTime(NULL, &stimeCreate, outCreate);

					createTime.SetDate(outCreate->wYear, outCreate->wMonth, outCreate->wDay);
				}
			}
		}

		// 파일을 닫습니다. (핸들을 닫는다고 표현함)
		CloseHandle(hFile);
	}
	catch(CException& e)
	{
		TCHAR szException[1024] = {0,};
		e.GetErrorMessage(szException, 1024);
		TRACE(szException);

		CloseHandle(hFile);
		hFile = NULL;
	}
}
