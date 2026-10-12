#pragma once

#include <deque>
#include <vector>

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CDataSaveManager
{
public:
	CDataSaveManager();
	~CDataSaveManager();

	DATA_SAVE m_SaveMethod;
	CString m_strBarcode;
	CString m_strTime;
	CString m_strBeforpath;

	long m_nCamIdx;
	long m_nDivIdx;
	int m_nResult;
	int m_nAlarm;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;
	
	CString m_strYear, m_strMonth, m_strDay, m_strInspectionType;

	void SetImageSize(const long nImageWidth, const long nImageHeight);
	void InitThread();
	void FreeThread();
	struct SDataSaveJob
	{
		DATA_SAVE method;
		CString barcode;
		CString time;
		long camIdx;
		long divIdx;
		int result;
		long imageWidth;
		long imageHeight;
		long imageSize;
		BOOL manualInspection;
		std::vector<BYTE> rawImage;

		SDataSaveJob()
			: method(DATA_SAVE::RAW_IMAGE)
			, camIdx(0)
			, divIdx(0)
			, result(-1)
			, imageWidth(0)
			, imageHeight(0)
			, imageSize(0)
			, manualInspection(FALSE)
		{
		}
	};
	CRITICAL_SECTION m_csDataSaveQueue;
	std::deque<SDataSaveJob> m_queueDataSaveJob;
	long m_nQueuedRawImageJobCount;
	ULONGLONG m_ullQueuedRawImageBytes;
	BOOL m_bProcessingDataSaveJob;
	HANDLE m_hSignal_QueueSpace;
	CString m_strCurrentCycleKey;

	void StartDataSave(DATA_SAVE method, CString barcode, const long camIdx, const long divIdx, const CString time, const int result = -1);
	BOOL PopDataSaveJob(SDataSaveJob& job);
	void ApplyDataSaveJob(const SDataSaveJob& job);
	BOOL WaitDataSaveQueueSpace(const DWORD dwImageBytes);
	void UpdateDataSaveQueueSpaceSignal();
	void SetDataSaveProcessing(const BOOL bProcessing);
	BOOL IsQueueIdle();
	CString GetRawImageSaveFolder(const SDataSaveJob& job);
	void CreateDirectoryRecursive(const CString strPath);
	void RawImageSaveJob(const SDataSaveJob& job);
	void ResultDataSave(const BOOL bManualInspection = FALSE); // csv형식
	void ResultImageSave(const BOOL bManualInspection = FALSE);
	void RawImageSave(const BOOL bManualInspection = FALSE);
	void RawImageSave2(const BOOL bManualInspection = FALSE);
	CString CreateSaveFolder(CString strPath);
	DWORDLONG MyGetDiskFreeSpaceEx(LPCSTR pszDrive);
	CString GetSavePath(const CString strPath, const BOOL bManualInspection);

	struct SDataSaveHandle
	{
		HANDLE m_hSignal_Start;
		HANDLE m_hSignal_Kill;
		HANDLE m_Handle_Thread;
	};
	SDataSaveHandle m_Control_DataSave;

	static UINT ThreadDataSaveFunc(LPVOID pParam);
	void ThreadDataSave();

	////////////////////////////////////////////////////////////////////////////////
	// 장비, 업체마다 저장할 Data가 다르므로
	// Borgwarner 향 버스바 용점기 전용 ResultDataSave
	void ResultDataSave_forBorgWarner(const CString strPath);
	////////////////////////////////////////////////////////////////////////////////

	int m_ImageCount = 0;
	CString m_strCurrentSaveFolder;
	CString m_strCurrentBarcode;
	CString m_strCurrentImageName; // 250619 sjpark 저장되는 이미지 이름 변수 추가
	CString m_strYOLOResultImageName; // 250620 sjpark 결과이미지 변수 추가
	CString MakeYOLOResultPath(const CString& originalPath);
};

extern ALGORITHM_TOOLS_API CDataSaveManager g_DataSaveManager;