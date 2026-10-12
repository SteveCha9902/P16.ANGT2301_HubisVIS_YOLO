#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class CDlgVisionMain;

enum class SCAN_SEQUENCE : long
{
	NONE = -1,
	SCAN_READY, // H/W Check, Illum On
	START_SCAN, // Grab
	SCAN_DONE,
	INSPECTION_START, // Inspection Start
	IMAGE_DISPLAY, // 영상 표시
	SAVE_IMAGE, // Image 저장
	SEQUENCE_DONE, // Sequence 완료
	ERROR_STEP,
};

class ALGORITHM_TOOLS_API CScanManager
{
public:
	CScanManager();
	~CScanManager();

private:
	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;

public:
	CDlgVisionMain* m_pParent;
	CAlgorithm* m_pAlgorithm;

	void SetParent(CDlgVisionMain* pParent);

	///////////////////////////////////////////////////////////////
	// Scan Manage Thread 관련
	struct SScanManage
	{
		HANDLE m_hSignal_Start;
		HANDLE m_hSignal_Kill;
		HANDLE m_Handle_Thread;
	};
	SScanManage m_Manage_Scan;

	long m_nCamIndex;
	long m_nDivIndex;
	BOOL m_bScanDone;

	void InitScanManageThread();
	void FreeScanManageThread();
	static UINT ThreadScanManageFunc(LPVOID pParam);
	void ScanManage();
	void StartScanManage(const long camIndex, const long divIndex);
	///////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////
	// Scan Thread 관련
	struct SScanControl
	{
		HANDLE m_hSignal_Ready;
		HANDLE m_hSignal_Start;
		HANDLE m_hSignal_Kill;
		HANDLE m_Handle_Thread;
		HANDLE m_hSignal_Finish;
	};
	std::vector<SScanControl> m_Control_Scan;

	void InitScanThread();
	void FreeScanThread();
	static UINT ThreadScanFunc(LPVOID pParam);
	void Scan();

	std::vector<long> m_vecnCamIndex;
	std::vector<long> m_vecnDivIndex;
	std::vector<SCAN_SEQUENCE> m_vecScanStep;
	std::vector<BOOL> m_vecbScanRunning;
	std::vector<BOOL> m_vecbScanDone;
	std::vector<CString> m_vecstrBarcode;
	std::vector<CString> m_vecstrScanTime;

	void StartScan(const long threadIndex);
	void RunScan(const long threadIndex);
	///////////////////////////////////////////////////////////////
};

extern ALGORITHM_TOOLS_API CScanManager g_ScanManager;