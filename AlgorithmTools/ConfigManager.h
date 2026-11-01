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
	// 후검사 YOLO 서버 연동 설정 [DeepLearning] (P16)
	CString m_strYOLOServerIP;		// YOLO 서버 IP (영상 경로를 보내므로 같은 PC 권장)
	long m_nYOLOServerPort;			// YOLO 서버 포트
	long m_nYOLOTimeoutMs;			// 송수신 timeout (ms)
	float m_fYOLOConfidence;		// 이 값 이상 confidence 박스만 사용
	float m_fYOLOMatchRatio;		// Blob2 영역과 YOLO 박스 매칭 최소 겹침 비율 (교집합 / 작은 쪽 면적)
	BOOL m_bYOLOFailFallback;		// TRUE: YOLO 실패 시 룰베이스(Blob2) 판정, FALSE: NG
	BOOL m_bYOLOSpatterNG;			// TRUE: 검사 영역에 spatter 박스가 있으면 NG
	CString m_strYOLOWorkPath;		// YOLO 입력 영상 임시 폴더 (빈 값이면 %TEMP%\HubisVIS_YOLO)

	void SetCurPath(CString strCurPath);
	void InitParameters();
	BOOL LoadConfig(const CString strLoadini = _T(""));
	BOOL SaveConfig(const BOOL bBackup = FALSE);
	BOOL LoadParameters(const CString strPath);
	BOOL SaveParameters(const CString strPath);
};

extern ALGORITHM_TOOLS_API CConfigManager g_ConfigManager;