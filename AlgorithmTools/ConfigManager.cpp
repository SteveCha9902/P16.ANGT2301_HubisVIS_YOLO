#include "pch.h"
#include "ConfigManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CConfigManager g_ConfigManager;

CConfigManager::CConfigManager()
	: m_strPath(_T(""))
{
}

CConfigManager::~CConfigManager()
{
}

void CConfigManager::SetCurPath(CString strCurPath)
{
	m_strCurPath = strCurPath;

	InitParameters();
}

void CConfigManager::InitParameters()
{
	CString strTemp;

	m_strPath.Format(_T("%s\\Config\\Config.ini"), m_strCurPath);
	m_strPrevRecipeFilePath.Format(_T("%s\\Recipe\\Default.recipe"), m_strCurPath);
	m_bHardwareExist = FALSE;
	m_nMainUI = (long)MAINUI::UI_FOR_SINGLE_CAMERA;
	m_nDivisionIndex = 10;
	m_nLEDComPort = 1;
	m_nLEDComBaudRate = 9600;
	m_bUseLaserTCPIP = FALSE;
	m_nLaserTCPIPNum = 1;
	
	m_strLaserIP.clear();
	m_nLaserPort.clear();
	m_strLaserIP.push_back(_T("127.0.0.1"));
	m_nLaserPort.push_back(9000);

	m_nLaserAutoConnectTimes = 5000;
	m_bUsePLCTCPIP = FALSE;
	m_strPLCIP = _T("192.168.3.1");
	m_nPLCPort = 8100;
	m_nPLCAutoConnectTimes = 5000;
	m_strRawImageSavePath = _T("D:\\Hubis Data");
	m_strResultImageSavePath = _T("D:\\Hubis Data");
	m_strResultDataSavePath = _T("D:\\Hubis Data");
	m_bUseDataAutoDelete = TRUE;
	m_nAutoDeleteDays = 180;
	m_nAutoDeleteTimes = 60000;

	m_nCustomer = (long)CUSTOMER::BORGWARNER;

	for(long idx = 0; idx < CAMERA_MAX_COUNT; idx++)
	{
		m_nImageWidth[idx] = 2448;
		m_nImageHeight[idx] = 2048;
		m_nCameraType[idx] = (long)CAMERA_TYPE::ACA244020GM;
		m_strCameraIP[idx] = _T("192.168.4.2"); // Gige Camera IP 추가 - SHK 2023.12.27
		m_nCameraTriggerMode[idx] = (long)TRIGGER_MODE::TRIGGER;

		strTemp.Format(_T("Camara%d"), idx + 1);
		m_strCameraName[idx] = strTemp;
	}
	m_fDistToCamera = 0.f;
	m_bMultiIllumination = FALSE;
}

BOOL CConfigManager::LoadConfig(const CString strLoadini)
{
	CString strPath;

	if(strLoadini == _T(""))
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

BOOL CConfigManager::SaveConfig(const BOOL bBackup)
{
	CString strPath;

	if(bBackup == TRUE)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);

		strPath.Format(_T("%s\\Config\\Config_%04d%02d%02d%02d%02d%02d.ini"), m_strCurPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	}
	else
	{
		strPath = m_strPath;
	}

	SaveParameters(strPath);

	return TRUE;
}

BOOL CConfigManager::LoadParameters(const CString strPath)
{
	CString strTemp;

	HUBSDK::IniReadWriter::Read(strPath, _T("DeepLearning"), _T("AfterInspectionDL"), &m_bUseAfterInspectionDL, m_bUseAfterInspectionDL); // 250627 PSJ 후검사 YOLO DeepLearning 사용 여부
	HUBSDK::IniReadWriter::Read(strPath, _T("AutoPass"), _T("ManualAutoPass"), &m_bManualAutoPass, m_bManualAutoPass); // Auto pass 기능 config에 등록 - PSJ 250228 
	HUBSDK::IniReadWriter::Read(strPath, _T("AutoPass"), _T("PLCAutoPass"), &m_bPLCAutoPass, m_bPLCAutoPass); // Auto pass 기능 config에 등록 - PSJ 250228 
	HUBSDK::IniReadWriter::Read(strPath, _T("Customer"), _T("Customer"), &m_nCustomer, m_nCustomer);
	HUBSDK::IniReadWriter::Read(strPath, _T("Software"), _T("Prev. Job File Path"), &m_strPrevRecipeFilePath, m_strPrevRecipeFilePath);
	HUBSDK::IniReadWriter::Read(strPath, _T("Hardware_Exist"), _T("Hardware Exist"), &m_bHardwareExist, m_bHardwareExist);
	HUBSDK::IniReadWriter::Read(strPath, _T("Main_UI"), _T("Main UI"), &m_nMainUI, m_nMainUI);
	HUBSDK::IniReadWriter::Read(strPath, _T("Index"), _T("Division Index"), &m_nDivisionIndex, m_nDivisionIndex);
	HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), _T("LED Com Port"), &m_nLEDComPort, m_nLEDComPort);
	HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), _T("LED Com BaudRate"), &m_nLEDComBaudRate, m_nLEDComBaudRate);
	HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), _T("Use Laser TCPIP"), &m_bUseLaserTCPIP, m_bUseLaserTCPIP);
	HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), _T("Use Laser TCPIP Num"), &m_nLaserTCPIPNum, m_nLaserTCPIPNum);

	if (m_strLaserIP.size() != m_nLaserTCPIPNum || m_nLaserPort.size() != m_nLaserTCPIPNum)
	{
		m_strLaserIP.resize(m_nLaserTCPIPNum);
		m_nLaserPort.resize(m_nLaserTCPIPNum);
	}
	for (long idx = 0; idx < m_nLaserTCPIPNum; idx++)
	{
		strTemp.Format(_T("Laser IP %d"), idx + 1);
		HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), strTemp, &m_strLaserIP[idx], m_strLaserIP[idx]);
		strTemp.Format(_T("Laser Port %d"), idx + 1);
		HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), strTemp, &m_nLaserPort[idx], m_nLaserPort[idx]);
	}

	HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), _T("Laser Auto Connect Times"), &m_nLaserAutoConnectTimes, m_nLaserAutoConnectTimes);
	HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), _T("Use PLC TCPIP"), &m_bUsePLCTCPIP, m_bUsePLCTCPIP);
	HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), _T("PLC IP"), &m_strPLCIP, m_strPLCIP);
	HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), _T("PLC Port"), &m_nPLCPort, m_nPLCPort);
	HUBSDK::IniReadWriter::Read(strPath, _T("Communication"), _T("PLC Auto Connect Times"), &m_nPLCAutoConnectTimes, m_nPLCAutoConnectTimes);
	HUBSDK::IniReadWriter::Read(strPath, _T("Save"), _T("Raw Image Save Folder"), &m_strRawImageSavePath, m_strRawImageSavePath);
	HUBSDK::IniReadWriter::Read(strPath, _T("Save"), _T("Result Image Save Folder"), &m_strResultImageSavePath, m_strResultImageSavePath);
	HUBSDK::IniReadWriter::Read(strPath, _T("Save"), _T("Result Data Save Folder"), &m_strResultDataSavePath, m_strResultDataSavePath);
	HUBSDK::IniReadWriter::Read(strPath, _T("Save"), _T("Use Data Auto Delete"), &m_bUseDataAutoDelete, m_bUseDataAutoDelete);
	HUBSDK::IniReadWriter::Read(strPath, _T("Save"), _T("Data Auto Delete Days"), &m_nAutoDeleteDays, m_nAutoDeleteDays);
	HUBSDK::IniReadWriter::Read(strPath, _T("Save"), _T("Data Auto Delete Times"), &m_nAutoDeleteTimes, m_nAutoDeleteTimes);

	for(long idx = 0; idx < CAMERA_MAX_COUNT; idx++)
	{
		strTemp.Format(_T("Image Width %d"), idx + 1); // 버그 수정- SHK 2023.12.27
		HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), strTemp, &m_nImageWidth[idx], m_nImageWidth[idx]);
		strTemp.Format(_T("Image Height %d"), idx + 1);// 버그 수정- SHK 2023.12.27
		HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), strTemp, &m_nImageHeight[idx], m_nImageHeight[idx]);
		strTemp.Format(_T("Camera%d Type"), idx + 1);
		HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), strTemp, &m_nCameraType[idx], m_nCameraType[idx]);
		strTemp.Format(_T("Camera%d IP"), idx + 1); // Gige Camera IP 추가 - SHK 2023.12.27
		HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), strTemp, &m_strCameraIP[idx], m_strCameraIP[idx]); // Gige Camera IP 추가 - SHK 2023.12.27
		strTemp.Format(_T("Camera%d Trigger Mode"), idx + 1);
		HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), strTemp, &m_nCameraTriggerMode[idx], m_nCameraTriggerMode[idx]);
		strTemp.Format(_T("Camera%d Name"), idx + 1);
		HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), strTemp, &m_strCameraName[idx], m_strCameraName[idx]);
	}
	HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), _T("Dist. Camera to Camera"), &m_fDistToCamera, m_fDistToCamera);
	HUBSDK::IniReadWriter::Read(strPath, _T("Hardware"), _T("Multi Illumination"), &m_bMultiIllumination, m_bMultiIllumination);
	
	return TRUE;
}

BOOL CConfigManager::SaveParameters(const CString strPath)
{
	CString strTemp;
	
	HUBSDK::IniReadWriter::Write(strPath, _T("DeepLearning"), _T("AfterInspectionDL"), &m_bUseAfterInspectionDL); // 250627 PSJ 후검사 YOLO DeepLearning 사용 여부
	HUBSDK::IniReadWriter::Write(strPath, _T("AutoPass"), _T("ManualAutoPass"), &m_bManualAutoPass); // Auto pass 기능 config에 등록 - PSJ 250228 
	HUBSDK::IniReadWriter::Write(strPath, _T("AutoPass"), _T("PLCAutoPass"), &m_bPLCAutoPass); // Auto pass 기능 config에 등록 - PSJ 250228 
	HUBSDK::IniReadWriter::Write(strPath, _T("Customer"), _T("Customer"), &m_nCustomer);
	HUBSDK::IniReadWriter::Write(strPath, _T("Software"), _T("Prev. Job File Path"), &m_strPrevRecipeFilePath);
	HUBSDK::IniReadWriter::Write(strPath, _T("Hardware_Exist"), _T("Hardware Exist"), &m_bHardwareExist);
	HUBSDK::IniReadWriter::Write(strPath, _T("Main_UI"), _T("Main UI"), &m_nMainUI);
	HUBSDK::IniReadWriter::Write(strPath, _T("Index"), _T("Division Index"), &m_nDivisionIndex);
	HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), _T("LED Com Port"), &m_nLEDComPort);
	HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), _T("LED Com BaudRate"), &m_nLEDComBaudRate);
	HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), _T("Use Laser TCPIP"), &m_bUseLaserTCPIP); // 버그 수정- SHK 2023.12.27
	HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), _T("Use Laser TCPIP Num"), &m_nLaserTCPIPNum); // 버그 수정- SHK 2023.12.27

	for (long idx = 0; idx < m_nLaserTCPIPNum; idx++)
	{
		strTemp.Format(_T("Laser IP %d"), idx + 1);
		HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), strTemp, &m_strLaserIP[idx]);
		strTemp.Format(_T("Laser Port %d"), idx + 1);
		HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), strTemp, &m_nLaserPort[idx]);
	}
	
	HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), _T("Laser Auto Connect Times"), &m_nLaserAutoConnectTimes);
	HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), _T("Use PLC TCPIP"), &m_bUsePLCTCPIP);
	HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), _T("PLC IP"), &m_strPLCIP);
	HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), _T("PLC Port"), &m_nPLCPort);
	HUBSDK::IniReadWriter::Write(strPath, _T("Communication"), _T("PLC Auto Connect Times"), &m_nPLCAutoConnectTimes);
	HUBSDK::IniReadWriter::Write(strPath, _T("Save"), _T("Raw Image Save Folder"), &m_strRawImageSavePath);
	HUBSDK::IniReadWriter::Write(strPath, _T("Save"), _T("Result Image Save Folder"), &m_strResultImageSavePath);
	HUBSDK::IniReadWriter::Write(strPath, _T("Save"), _T("Result Data Save Folder"), &m_strResultDataSavePath);
	HUBSDK::IniReadWriter::Write(strPath, _T("Save"), _T("Use Data Auto Delete"), &m_bUseDataAutoDelete);
	HUBSDK::IniReadWriter::Write(strPath, _T("Save"), _T("Data Auto Delete Days"), &m_nAutoDeleteDays);
	HUBSDK::IniReadWriter::Write(strPath, _T("Save"), _T("Data Auto Delete Times"), &m_nAutoDeleteTimes);

	for(long idx = 0; idx < CAMERA_MAX_COUNT; idx++)
	{
		strTemp.Format(_T("Image Width %d"), idx + 1); // 버그 수정- SHK 2023.12.27
		HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), strTemp, &m_nImageWidth[idx]);
		strTemp.Format(_T("Image Height %d"), idx + 1); // 버그 수정- SHK 2023.12.27
		HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), strTemp, &m_nImageHeight[idx]);
		strTemp.Format(_T("Camera%d Type"), idx + 1);
		HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), strTemp, &m_nCameraType[idx]);
		strTemp.Format(_T("Camera%d IP"), idx + 1); // Gige Camera IP 추가 - SHK 2023.12.27
		HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), strTemp, &m_strCameraIP[idx]); // Gige Camera IP 추가 - SHK 2023.12.27
		strTemp.Format(_T("Camera%d Trigger Mode"), idx + 1);
		HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), strTemp, &m_nCameraTriggerMode[idx]);
		strTemp.Format(_T("Camera%d Name"), idx + 1);
		HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), strTemp, &m_strCameraName[idx]);
	}
	HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), _T("Dist. Camera to Camera"), &m_fDistToCamera);
	HUBSDK::IniReadWriter::Write(strPath, _T("Hardware"), _T("Multi Illumination"), &m_bMultiIllumination);

	return TRUE;
}