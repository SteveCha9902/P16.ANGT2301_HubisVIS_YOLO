#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class CDlgVisionMain;
class CInspection;

enum class INSP_SEQUENCE : long
{
	NONE = -1,
	INSPECTION_READY, // Inspection Start
	INSPECTION_START, // Inspection Start
	INSPECTION_DONE, // Inspection Done
	RESULT_DISPLAY, // 결과 표시
	SEND_DATA, // 결과값 전송
	SAVE_RESULT_IMAGE, // Image 저장
	SEQUENCE_DONE, // Sequence 완료
	ERROR_STEP,
};


class ALGORITHM_TOOLS_API CInspectionManager
{
public:
	CInspectionManager();
	~CInspectionManager();

private:
	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;

public:
	CDlgVisionMain* m_pParent;
	CAlgorithm* m_pAlgorithm;
	CInspection* m_pInspection; // Setup 때만 사용할 변수
	std::vector<CInspection *> m_vecpInspection; // Auto Run시 사용

	// 임시
	std::vector<std::vector<SInspEdgeResult>> m_vecsInspEdgeResult;
	std::vector<std::vector<SInspBlobResult>> m_vecsInspBlobResult;

	void SetParent(CDlgVisionMain* pParent);

	/////////////////////////////////////////////////////////////////////////////
	// Inspection Threads
	struct SInspectionControl
	{
		HANDLE m_hSignal_Ready;
		HANDLE m_hSignal_Start;
		HANDLE m_hSignal_Kill;
		HANDLE m_Handle_Thread;
	};
	std::vector<SInspectionControl> m_Control_Inspection;

	void InitInspectionThread();
	void FreeInspectionThread();
	static UINT ThreadInspectionFunc(LPVOID pParam);
	void ThreadInspection();

	std::vector<long> m_vecnCamIndex;
	std::vector<long> m_vecnDivIndex;
	std::vector<INSP_SEQUENCE> m_vecInspStep;
	std::vector<BOOL> m_vecbInspRunning;
	std::vector<BOOL> m_vecbInspDone;
	std::vector<CString> m_vecstrBarcode;
	std::vector<CString> m_vecstrScanTime;

	void StartInsp(const long threadIndex, const long camIndex, const long divIndex, const CString barcode, const CString scanTime);
	void RunInsp(const long threadIndex);

	BOOL m_bResult[6];
	/////////////////////////////////////////////////////////////////////////////
};

extern ALGORITHM_TOOLS_API CInspectionManager g_InspectionManager;