#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CConfigManager
{
public:
	CConfigManager();
	~CConfigManager();

public:
	CString m_strCurPath;
	CString m_strPath;
	CString m_strPrevRecipeFilePath;
	BOOL m_bHardwareExist;
	long m_nMainUI;
	long m_nImageWidth[CAMERA_MAX_COUNT];
	long m_nImageHeight[CAMERA_MAX_COUNT];
	long m_nCameraType[CAMERA_MAX_COUNT];
	CString m_strCameraIP[CAMERA_MAX_COUNT]; // Gige Camera IP 추가 - SHK 2023.12.27
	long m_nCameraTriggerMode[CAMERA_MAX_COUNT];
	CString m_strCameraName[CAMERA_MAX_COUNT];
	float m_fDistToCamera;
	long m_nDivisionIndex;
	BOOL m_bMultiIllumination;
	long m_nLEDComPort;
	long m_nLEDComBaudRate;
	BOOL m_bUseLaserTCPIP;
	long m_nLaserTCPIPNum;
	std::vector<CString> m_strLaserIP;
	std::vector<long> m_nLaserPort;
	long m_nLaserAutoConnectTimes;
	BOOL m_bUsePLCTCPIP;
	CString m_strPLCIP;
	long m_nPLCPort;
	long m_nPLCAutoConnectTimes;
	CString m_strRawImageSavePath;
	CString m_strResultImageSavePath;
	CString m_strResultDataSavePath;
	BOOL m_bUseDataAutoDelete;
	long m_nAutoDeleteDays;
	long m_nAutoDeleteTimes;

	long m_nCustomer;
	BOOL m_bPLCAutoPass;
	BOOL m_bManualAutoPass;
	BOOL m_bUseAfterInspectionDL; // 250619 sjpark 후검사 딥러닝 기능 사용여부 설정

	void SetCurPath(CString strCurPath);
	void InitParameters();
	BOOL LoadConfig(const CString strLoadini = _T(""));
	BOOL SaveConfig(const BOOL bBackup = FALSE);
	BOOL LoadParameters(const CString strPath);
	BOOL SaveParameters(const CString strPath);
};

extern ALGORITHM_TOOLS_API CConfigManager g_ConfigManager;