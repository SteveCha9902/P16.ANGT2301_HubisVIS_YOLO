#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CCalibrationManager
{
public:
	CCalibrationManager();
	~CCalibrationManager();

public:
	CString m_strCurPath;
	CString m_strPath;

	float m_fPixelResolutionX[CAMERA_MAX_COUNT];
	float m_fPixelResolutionY[CAMERA_MAX_COUNT];

	void SetCurPath(CString strCurPath);
	void InitParameters();
	BOOL LoadCalibration(const CString strLoadini = _T(""));
	BOOL SaveCalibration();
	BOOL LoadParameters(const CString strPath);
	BOOL SaveParameters(const CString strPath);
};

extern ALGORITHM_TOOLS_API CCalibrationManager g_CalibrationManager;
