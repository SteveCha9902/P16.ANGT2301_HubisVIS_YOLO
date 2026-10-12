#include "pch.h"
#include "CalibrationManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCalibrationManager g_CalibrationManager;

CCalibrationManager::CCalibrationManager()
	: m_strPath(_T(""))
{
}

CCalibrationManager::~CCalibrationManager()
{
}

void CCalibrationManager::SetCurPath(CString strCurPath)
{
	m_strCurPath = strCurPath;

	InitParameters();
}

void CCalibrationManager::InitParameters()
{
	m_strPath.Format(_T("%s\\Config\\Calibration.ini"), m_strCurPath);

	for (long idx = 0; idx < CAMERA_MAX_COUNT; idx++)
	{
		m_fPixelResolutionX[idx] = 0.f;
		m_fPixelResolutionY[idx] = 0.f;
	}
}

BOOL CCalibrationManager::LoadCalibration(const CString strLoadini)
{
	CString strPath;

	if (strLoadini == _T(""))
	{
		strPath = m_strPath;
	}
	else
	{
		strPath = strLoadini;
	}

	LoadParameters(strPath);

	return TRUE;
}

BOOL CCalibrationManager::SaveCalibration()
{
	SaveParameters(m_strPath);

	return TRUE;
}

BOOL CCalibrationManager::LoadParameters(const CString strPath)
{
	CString strTemp;

	for (long idx = 0; idx < CAMERA_MAX_COUNT; idx++)
	{
		strTemp.Format(_T("Pixel Resolution X %d"), idx + 1); 
		HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), strTemp, &m_fPixelResolutionX[idx], m_fPixelResolutionX[idx]);
		strTemp.Format(_T("Pixel Resolution Y %d"), idx + 1);
		HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), strTemp, &m_fPixelResolutionY[idx], m_fPixelResolutionY[idx]);
	}

	return TRUE;
}

BOOL CCalibrationManager::SaveParameters(const CString strPath)
{
	CString strTemp;

	for (long idx = 0; idx < CAMERA_MAX_COUNT; idx++)
	{
		strTemp.Format(_T("Pixel Resolution X %d"), idx + 1);
		HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), strTemp, &m_fPixelResolutionX[idx]);
		strTemp.Format(_T("Pixel Resolution Y %d"), idx + 1);
		HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), strTemp, &m_fPixelResolutionY[idx]);
	}

	return TRUE;
}
