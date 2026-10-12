#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CPLCManager
{
public:
	CPLCManager();
	~CPLCManager();

public:
	CString m_strCurPath;
	CString m_strPath;

	SPLC m_sPLC;
	long m_nUseRecipeNum;

	int m_nJudge;
	std::vector<CString> m_vecstrUseRecipe;

	void SetCurPath(CString strCurPath);
	void InitParameters();
	BOOL LoadPLCMap(const CString strLoadini = _T(""));
	BOOL SavePLCMap();
	BOOL LoadParameters(const CString strPath);
	BOOL SaveParameters(const CString strPath);
	BOOL SaveUseRecipe(const CString strPath);

	void GetReadData();
	void GetReadDataSimul(int nCameraIndex, int nDivIndex, int nJobNumber, bool bScanDone);
	BOOL GetScanDone();
	BOOL GetInitialize();
	CString GetBarcode();
	CString GetModel();
	long GetCamIndex();
	long GetDivIndex();
	BOOL GetScanStart();
	long GetJobNumber();

	////////////////////////////////////////////////////////////////////////////////
	// 장비마다 결과 전송 주소 및 Data 가 다르므로 함수 분리
	// Borgwarner 향 버스바 용점기 전용 SetResultData
	void SetResultData_forBorgwarner(std::vector<SResultParams> vecResult, std::vector<SCriteriaParams> vecCriteriaResult, int idivIndex);
	////////////////////////////////////////////////////////////////////////////////

	// 임시 
	float m_fPLCPosX[3];
	float m_fPLCPosY[3];
};

extern ALGORITHM_TOOLS_API CPLCManager g_PLCManager;
