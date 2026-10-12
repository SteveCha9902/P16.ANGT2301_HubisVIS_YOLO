
// ConfigSetupDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ConfigSetup.h"
#include "ConfigSetupDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CConfigSetupDlg 대화 상자

CConfigSetupDlg::CConfigSetupDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CONFIGSETUP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CConfigSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_CONFIG, m_propertyGridConfig);
}

BEGIN_MESSAGE_MAP(CConfigSetupDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CConfigSetupDlg::OnBnClickedOk)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CConfigSetupDlg::OnAfxWmPropertyChanged)
	ON_BN_CLICKED(IDC_BTN_LOAD, &CConfigSetupDlg::OnBnClickedBtnLoad)
END_MESSAGE_MAP()


// CConfigSetupDlg 메시지 처리기

BOOL CConfigSetupDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	CString strCurPath, strConfig;
	GetModuleFileName(nullptr, strCurPath.GetBuffer(_MAX_PATH + 1), _MAX_PATH);
	strCurPath.ReleaseBuffer();
	m_strPath = strCurPath.Left(strCurPath.ReverseFind(_T('\\')));
	strConfig.Format(_T("%s\\Config"), m_strPath);
	CreateDirectory(strConfig, nullptr);

	::g_ConfigManager.SetCurPath(m_strPath);

	// config.ini 파일을 읽어온다.
	::g_ConfigManager.InitParameters();
	::g_ConfigManager.LoadConfig();
	::g_ConfigManager.SaveConfig();

	m_ConfigManager = ::g_ConfigManager;

	m_bChange = FALSE;

	InitPropertyGrid();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CConfigSetupDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CConfigSetupDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CConfigSetupDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CConfigSetupDlg::InitPropertyGrid()
{
	CString strTemp;

	m_propertyGridConfig.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 250;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridConfig.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Hardware Exist Group
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupHardwareExist = new CMFCPropertyGridProperty(_T("Hardware Exist"));
	m_propertyGridConfig.AddProperty(pGroupHardwareExist);

	CString strHardwareExist = ::g_ConfigManager.m_bHardwareExist == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupHardwareExist_HardwareExist = new CMFCPropertyGridProperty(_T("Hardware Exist"), strHardwareExist, _T(""), (long)CONFIG::HARDWARE_EXIST); // 생성시 ID 추가
	pGroupHardwareExist_HardwareExist->AddOption(_T("TRUE"));
	pGroupHardwareExist_HardwareExist->AddOption(_T("FALSE"));
	pGroupHardwareExist->AddSubItem(pGroupHardwareExist_HardwareExist);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Main UI Group
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupMainUI = new CMFCPropertyGridProperty(_T("Main UI"));
	m_propertyGridConfig.AddProperty(pGroupMainUI);

	CString strMainUI;
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		strMainUI = _T("UI for Single Camera");
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		strMainUI = _T("UI for Dual Camera");
		break;
	}
	CMFCPropertyGridProperty* pGroupMainUI_MainUI = new CMFCPropertyGridProperty(_T("Main UI"), strMainUI, _T(""), (long)CONFIG::MAIN_UI);
	pGroupMainUI_MainUI->AddOption(_T("UI for Single Camera"));
	pGroupMainUI_MainUI->AddOption(_T("UI for Dual Camera"));
	pGroupMainUI->AddSubItem(pGroupMainUI_MainUI);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Index Group
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupIndex = new CMFCPropertyGridProperty(_T("Index"));
	m_propertyGridConfig.AddProperty(pGroupIndex);

	CMFCPropertyGridProperty* pGroupIndex_DivisionIndex= new CMFCPropertyGridProperty(_T("Division Index"), ::g_ConfigManager.m_nDivisionIndex, _T(""), (long)CONFIG::DIVISION_INDEX);
	pGroupIndex_DivisionIndex->EnableSpinControl(TRUE, 1, 20);
	pGroupIndex->AddSubItem(pGroupIndex_DivisionIndex);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Hardware Group
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupHardware = new CMFCPropertyGridProperty(_T("Hardware"));
	m_propertyGridConfig.AddProperty(pGroupHardware);

	long camCount(0);
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		camCount = 1;
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		camCount = 2;
		break;
	}

	strTemp = camCount > 0 ? _T("1") : _T("No Camera");
	CMFCPropertyGridProperty* pGroupHardware_Camera = new CMFCPropertyGridProperty(_T("Camera No."), strTemp, _T(""), (long)CONFIG::CAMERA_NO);
	for(long idx = 0; idx < camCount; idx++)
	{
		strTemp.Format(_T("%d"), idx + 1);
		pGroupHardware_Camera->AddOption(strTemp);
	}
	pGroupHardware->AddSubItem(pGroupHardware_Camera);

	strTemp.Format(_T("Camera Name"));
	CMFCPropertyGridProperty* pGroupHardware_CameraName = new CMFCPropertyGridProperty(strTemp, ::g_ConfigManager.m_strCameraName[0], _T(""), (long)CONFIG::CAMERA_NAME);
	pGroupHardware->AddSubItem(pGroupHardware_CameraName);

	CString strCameraType;
	switch(::g_ConfigManager.m_nCameraType[0])
	{
	case (long)CAMERA_TYPE::ACA244020GM: strCameraType = _T("Basler acA2440-20gm(5M)"); break;
	}

	strTemp.Format(_T("Camera Type"));
	CMFCPropertyGridProperty* pGroupHardware_CameraType = new CMFCPropertyGridProperty(strTemp, strCameraType, _T(""), (long)CONFIG::CAMERA_TYPE);
	pGroupHardware_CameraType->AddOption(_T("Basler acA2440-20gm(5M)"));
	pGroupHardware->AddSubItem(pGroupHardware_CameraType);

	strTemp.Format(_T("Camera IP")); // Gige Camera IP 추가 - SHK 2023.12.27
	CMFCPropertyGridProperty* pGroupHardware_CameraIP = new CMFCPropertyGridProperty(strTemp, ::g_ConfigManager.m_strCameraIP[0], _T(""), (long)CONFIG::CAMERA_IP); // Gige Camera IP 추가 - SHK 2023.12.27
	pGroupHardware->AddSubItem(pGroupHardware_CameraIP); // Gige Camera IP 추가 - SHK 2023.12.27

	strTemp.Format(_T("Camera Trigger Mode"));
	CString strCameraTriggerMode = ::g_ConfigManager.m_nCameraTriggerMode[0] == 0 ? _T("Trigger") : _T("Free Run");
	CMFCPropertyGridProperty* pGroupHardware_CameraTriggerMode = new CMFCPropertyGridProperty(strTemp, strCameraTriggerMode, _T(""), (long)CONFIG::CAMERA_MODE);
	pGroupHardware_CameraTriggerMode->AddOption(_T("Trigger"));
	pGroupHardware_CameraTriggerMode->AddOption(_T("Free Run"));
	pGroupHardware->AddSubItem(pGroupHardware_CameraTriggerMode);

	strTemp.Format(_T("Camera Image Width"));
	CMFCPropertyGridProperty* pGroupHardware_CameraImageWidth = new CMFCPropertyGridProperty(strTemp, ::g_ConfigManager.m_nImageWidth[0], _T(""), (long)CONFIG::IMAGE_WIDTH);
	pGroupHardware_CameraImageWidth->EnableSpinControl(TRUE, 50, 10000);
	pGroupHardware->AddSubItem(pGroupHardware_CameraImageWidth);

	strTemp.Format(_T("Camera Image Height"));
	CMFCPropertyGridProperty* pGroupHardware_CameraImageHeight = new CMFCPropertyGridProperty(strTemp, ::g_ConfigManager.m_nImageHeight[0], _T(""), (long)CONFIG::IMAGE_HEIGHT);
	pGroupHardware_CameraImageHeight->EnableSpinControl(TRUE, 50, 10000);
	pGroupHardware->AddSubItem(pGroupHardware_CameraImageHeight);

	strTemp.Format(_T("Dist. Camera to Camera"));
	CMFCPropertyGridProperty* pGroupHardware_DistCameraToCamera = new CMFCPropertyGridProperty(strTemp, ::g_ConfigManager.m_fDistToCamera, _T(""), (long)CONFIG::DIST_CAMERA_TO_CAMERA);
	pGroupHardware->AddSubItem(pGroupHardware_DistCameraToCamera);

	CString strMultiIllumination = ::g_ConfigManager.m_bMultiIllumination == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupHardware_MultiIllumination = new CMFCPropertyGridProperty(_T("Multi Illumination"), strMultiIllumination, _T(""), (long)CONFIG::MULTI_ILLUMINATION);
	pGroupHardware_MultiIllumination->AddOption(_T("TRUE"));
	pGroupHardware_MultiIllumination->AddOption(_T("FALSE"));
	pGroupHardware->AddSubItem(pGroupHardware_MultiIllumination);

	CMFCPropertyGridProperty* pGroupHardware_LEDComPort = new CMFCPropertyGridProperty(_T("LED Com Port"), ::g_ConfigManager.m_nLEDComPort, _T(""), (long)CONFIG::LED_PORT);
	pGroupHardware_LEDComPort->EnableSpinControl(TRUE, 1, 20);
	pGroupHardware->AddSubItem(pGroupHardware_LEDComPort);

	CMFCPropertyGridProperty* pGroupHardware_LEDComBaudRate = new CMFCPropertyGridProperty(_T("LED Com BaudRate"), (_variant_t)::g_ConfigManager.m_nLEDComBaudRate, _T(""), (long)CONFIG::LED_BAUDRATE);
	pGroupHardware_LEDComBaudRate->AddOption(_T("9600"));
	pGroupHardware_LEDComBaudRate->AddOption(_T("14400"));
	pGroupHardware_LEDComBaudRate->AddOption(_T("19200"));
	pGroupHardware_LEDComBaudRate->AddOption(_T("38400"));
	pGroupHardware_LEDComBaudRate->AddOption(_T("56000"));
	pGroupHardware_LEDComBaudRate->AddOption(_T("57600"));
	pGroupHardware_LEDComBaudRate->AddOption(_T("115200"));
	pGroupHardware_LEDComBaudRate->AddOption(_T("128000"));
	pGroupHardware_LEDComBaudRate->AddOption(_T("256000"));
	pGroupHardware->AddSubItem(pGroupHardware_LEDComBaudRate);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Communication Group
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupCommunication = new CMFCPropertyGridProperty(_T("Communication"));
	m_propertyGridConfig.AddProperty(pGroupCommunication);

	CString strUseLaserTCPIP = ::g_ConfigManager.m_bUseLaserTCPIP == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupCommunication_UseLaserTCPIP = new CMFCPropertyGridProperty(_T("Use Laser TCP/IP"), strUseLaserTCPIP, _T(""), (long)CONFIG::USE_LASER_TCPIP);
	pGroupCommunication_UseLaserTCPIP->AddOption(_T("TRUE"));
	pGroupCommunication_UseLaserTCPIP->AddOption(_T("FALSE"));
	pGroupCommunication->AddSubItem(pGroupCommunication_UseLaserTCPIP);

	CMFCPropertyGridProperty* pGroupCommunication_LaserTCPIPNum = new CMFCPropertyGridProperty(_T("Laser TCP/IP Num"), ::g_ConfigManager.m_nLaserTCPIPNum, _T(""), (long)CONFIG::LASER_TCPIP_NUM);
	pGroupCommunication_LaserTCPIPNum->EnableSpinControl(TRUE, 1, 1000);
	pGroupCommunication->AddSubItem(pGroupCommunication_LaserTCPIPNum);

	strTemp = ::g_ConfigManager.m_nLaserTCPIPNum == TRUE ? _T("Laser TCP/IP 1") : _T("No Laser TCP/IP");
	CMFCPropertyGridProperty* pGroupCommunication_LaserTCPIPNo = new CMFCPropertyGridProperty(_T("Laser TCP/IP No."), strTemp, _T(""), (long)CONFIG::LASER_TCPIP_NO);
	for (long idx = 0; idx < ::g_ConfigManager.m_nLaserTCPIPNum; idx++)
	{
		strTemp.Format(_T("Laser TCP/IP %d"), idx + 1);
		pGroupCommunication_LaserTCPIPNo->AddOption(strTemp);
	}
	pGroupCommunication->AddSubItem(pGroupCommunication_LaserTCPIPNo);


	CMFCPropertyGridProperty* pGroupCommunication_LaserIP = new CMFCPropertyGridProperty(_T("Laser IP"), ::g_ConfigManager.m_strLaserIP[0], _T(""), (long)CONFIG::LASER_IP);
	pGroupCommunication->AddSubItem(pGroupCommunication_LaserIP);

	CMFCPropertyGridProperty* pGroupCommunication_LaserPort = new CMFCPropertyGridProperty(_T("Laser Port"), ::g_ConfigManager.m_nLaserPort[0], _T(""), (long)CONFIG::LASER_PORT);
	pGroupCommunication_LaserPort->EnableSpinControl(TRUE, 0, 100000);
	pGroupCommunication->AddSubItem(pGroupCommunication_LaserPort);

	CMFCPropertyGridProperty* pGroupCommunication_LaserAutoConnectTimes = new CMFCPropertyGridProperty(_T("Laser Auto Connect Times(ms)"), ::g_ConfigManager.m_nLaserAutoConnectTimes, _T(""), (long)CONFIG::LASER_CONNECT_TIME);
	pGroupCommunication_LaserAutoConnectTimes->EnableSpinControl(TRUE, 1000, 100000);
	pGroupCommunication->AddSubItem(pGroupCommunication_LaserAutoConnectTimes);

	CString strUsePLCTCPIP = ::g_ConfigManager.m_bUsePLCTCPIP == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupCommunication_UsePLCTCPIP = new CMFCPropertyGridProperty(_T("Use PLC TCP/IP"), strUsePLCTCPIP, _T(""), (long)CONFIG::USE_PLC_TCPIP);
	pGroupCommunication_UsePLCTCPIP->AddOption(_T("TRUE"));
	pGroupCommunication_UsePLCTCPIP->AddOption(_T("FALSE"));
	pGroupCommunication->AddSubItem(pGroupCommunication_UsePLCTCPIP);

	CMFCPropertyGridProperty* pGroupCommunication_PLCIP = new CMFCPropertyGridProperty(_T("PLC IP"), ::g_ConfigManager.m_strPLCIP, _T(""), (long)CONFIG::PLC_IP);
	pGroupCommunication->AddSubItem(pGroupCommunication_PLCIP);

	CMFCPropertyGridProperty* pGroupCommunication_PLCPort = new CMFCPropertyGridProperty(_T("PLC Port"), ::g_ConfigManager.m_nPLCPort, _T(""), (long)CONFIG::PLC_PORT);
	pGroupCommunication_PLCPort->EnableSpinControl(TRUE, 0, 100000);
	pGroupCommunication->AddSubItem(pGroupCommunication_PLCPort);

	CMFCPropertyGridProperty* pGroupCommunication_PLCAutoConnectTimes = new CMFCPropertyGridProperty(_T("PLC Auto Connect Times(ms)"), ::g_ConfigManager.m_nPLCAutoConnectTimes, _T(""), (long)CONFIG::PLC_CONNECT_TIME);
	pGroupCommunication_PLCAutoConnectTimes->EnableSpinControl(TRUE, 1000, 100000);
	pGroupCommunication->AddSubItem(pGroupCommunication_PLCAutoConnectTimes);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Image & Data Save Group
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupImageDataSave = new CMFCPropertyGridProperty(_T("Image & Data Save"));
	m_propertyGridConfig.AddProperty(pGroupImageDataSave);

	CMFCPropertyGridFileProperty* pGroupImageDataSave_RawImageSavePath = new CMFCPropertyGridFileProperty(_T("Raw Image Save Path"), ::g_ConfigManager.m_strRawImageSavePath, (long)CONFIG::RAW_IMAGE_SAVE_PATH);
	pGroupImageDataSave->AddSubItem(pGroupImageDataSave_RawImageSavePath);

	CMFCPropertyGridFileProperty* pGroupImageDataSave_ResultImageSavePath = new CMFCPropertyGridFileProperty(_T("Result Image Save Path"), ::g_ConfigManager.m_strResultImageSavePath, (long)CONFIG::RESULT_IMAGE_SAVE_PATH);
	pGroupImageDataSave->AddSubItem(pGroupImageDataSave_ResultImageSavePath);

	CMFCPropertyGridFileProperty* pGroupImageDataSave_ResultDataSavePath = new CMFCPropertyGridFileProperty(_T("Result Data Save Path"), ::g_ConfigManager.m_strResultDataSavePath, (long)CONFIG::RESULT_DATA_SAVE_PATH);
	pGroupImageDataSave->AddSubItem(pGroupImageDataSave_ResultDataSavePath);

	CString strUseDataAutoDelete = ::g_ConfigManager.m_bUseDataAutoDelete == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupImageDataSave_UseAutoDelete = new CMFCPropertyGridProperty(_T("Use Data Auto Delete"), strUseDataAutoDelete, _T(""), (long)CONFIG::USE_AUTO_DELETE);
	pGroupImageDataSave_UseAutoDelete->AddOption(_T("TRUE"));
	pGroupImageDataSave_UseAutoDelete->AddOption(_T("FALSE"));
	pGroupImageDataSave->AddSubItem(pGroupImageDataSave_UseAutoDelete);

	CMFCPropertyGridProperty* pGroupImageDataSave_AutoDeleteDays = new CMFCPropertyGridProperty(_T("Auto Delete Days"), ::g_ConfigManager.m_nAutoDeleteDays, _T(""), (long)CONFIG::AUTO_DELETE_DAYS);
	pGroupImageDataSave_AutoDeleteDays->EnableSpinControl(TRUE, 1, 180);
	pGroupImageDataSave->AddSubItem(pGroupImageDataSave_AutoDeleteDays);

	CMFCPropertyGridProperty* pGroupImageDataSave_AutoDeleteTimes = new CMFCPropertyGridProperty(_T("Auto Delete Times(ms)"), ::g_ConfigManager.m_nAutoDeleteTimes, _T(""), (long)CONFIG::AUTO_DELETE_TIMES);
	pGroupImageDataSave_AutoDeleteTimes->EnableSpinControl(TRUE, 10000, 100000);
	pGroupImageDataSave->AddSubItem(pGroupImageDataSave_AutoDeleteTimes);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridConfig.AddProperty(pGroupEtc);
}

void CConfigSetupDlg::OnBnClickedOk()
{
	if(m_bChange == TRUE)
	{
		m_ConfigManager.SaveConfig(TRUE); // 기존 Config backup 저장

		::g_ConfigManager.SaveConfig(); // 덮어쓰기
	}

	CDialogEx::OnOK();
}


LRESULT CConfigSetupDlg::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	CMFCPropertyGridProperty *pProp = (CMFCPropertyGridProperty*)lParam;

	if(pProp == nullptr)
		return 0;

	CString strTemp;

	CString str = pProp->GetName();
	_variant_t value = pProp->GetValue();

	//ID를 설정하여 string이 아니라 ID로 제어 가능
	auto *GroupHardware = m_propertyGridConfig.GetProperty(3);//LYS 241016 90버전에서 4->3
	auto *GroupHardware_CameraNo = GroupHardware->GetSubItem(0);
	auto *GroupHardware_CameraName = GroupHardware->GetSubItem(1);
	auto *GroupHardware_CameraType = GroupHardware->GetSubItem(2);
	auto *GroupHardware_CameraIP = GroupHardware->GetSubItem(3);
	auto *GroupHardware_CameraMode = GroupHardware->GetSubItem(4);
	auto *GroupHardware_CameraWidth = GroupHardware->GetSubItem(5);
	auto *GroupHardware_CameraHeight = GroupHardware->GetSubItem(6);

	auto *GroupCommunication = m_propertyGridConfig.GetProperty(4);//LYS 241016 90버전에서 5->4
	auto *GroupCommunication_LaserTCPIPUse = GroupCommunication->GetSubItem(0);
	auto *GroupCommunication_LaserTCPIPNo  = GroupCommunication->GetSubItem(2);
	auto *GroupCommunication_LaserIP = GroupCommunication->GetSubItem(3);
	auto *GroupCommunication_LaserPort = GroupCommunication->GetSubItem(4);

	CString strValue = GroupCommunication_LaserTCPIPNo->GetValue();
	strValue.Replace(_T("Laser TCP/IP "), _T(""));
	long LaserNo = _ttol((CString)(strValue)) - 1;

	auto &param = ::g_ConfigManager;

	long data = (long)pProp->GetData();
	switch(data)
	{
	case (long)CONFIG::HARDWARE_EXIST:
		param.m_bHardwareExist = (CString)value == _T("TRUE") ? TRUE : FALSE;
		break;
	case (long)CONFIG::MAIN_UI:
		GroupHardware_CameraNo->RemoveAllOptions();

		if((CString)value == _T("UI for Single Camera"))
		{
			param.m_nMainUI = (long)MAINUI::UI_FOR_SINGLE_CAMERA;
			GroupHardware_CameraNo->AddOption(_T("1"));
		}
		else if((CString)value == _T("UI for Dual Camera"))
		{
			param.m_nMainUI = (long)MAINUI::UI_FOR_DUAL_CAMERA;
			GroupHardware_CameraNo->AddOption(_T("1"));
			GroupHardware_CameraNo->AddOption(_T("2"));
		}
		GroupHardware_CameraNo->SetValue(_T("1"));
		break;
	case (long)CONFIG::CAMERA_NO:
	{
		long camNo = _ttol((CString)(GroupHardware_CameraNo->GetValue())) - 1;

		strTemp.Format(_T("%s"), param.m_strCameraName[camNo]);
		GroupHardware_CameraName->SetValue(strTemp);

		CString strCameraType;
		switch(param.m_nCameraType[camNo])
		{
		case (long)CAMERA_TYPE::ACA244020GM: strCameraType = _T("Basler acA2440-20gm(5M)"); break;
		}
		GroupHardware_CameraType->SetValue(strCameraType);

		strTemp.Format(_T("%s"), param.m_strCameraIP[camNo]);
		GroupHardware_CameraIP->SetValue(strTemp);

		CString strCameraMode;
		switch(param.m_nCameraTriggerMode[camNo])
		{
		case (long)TRIGGER_MODE::TRIGGER: strCameraMode = _T("Trigger"); break;
		case (long)TRIGGER_MODE::FREERUN: strCameraMode = _T("Free Run"); break;
		}
		GroupHardware_CameraMode->SetValue(strCameraMode);

		GroupHardware_CameraWidth->SetValue(param.m_nImageWidth[camNo]);
		GroupHardware_CameraHeight->SetValue(param.m_nImageHeight[camNo]);
	}
	break;
	case (long)CONFIG::CAMERA_NAME:
	{
		long camNo = _ttol((CString)(GroupHardware_CameraNo->GetValue())) - 1;
		param.m_strCameraName[camNo] = (CString)value;
	}
	break;
	case (long)CONFIG::CAMERA_TYPE:
	{
		long camNo = _ttol((CString)(GroupHardware_CameraNo->GetValue())) - 1;

		if((CString)value == _T("Basler acA2440-20gm(5M)"))
		{
			param.m_nCameraType[camNo] = (long)CAMERA_TYPE::ACA244020GM;
		}
	}
	break;
	case (long)CONFIG::CAMERA_IP:
	{
		long camNo = _ttol((CString)(GroupHardware_CameraNo->GetValue())) - 1;
		param.m_strCameraIP[camNo] = (CString)value;
	}
	break;
	case (long)CONFIG::CAMERA_MODE:
	{
		long camNo = _ttol((CString)(GroupHardware_CameraNo->GetValue())) - 1;

		if((CString)value == _T("Trigger"))
		{
			param.m_nCameraTriggerMode[camNo] = (long)TRIGGER_MODE::TRIGGER;
		}
		else if((CString)value == _T("Free Run"))
		{
			param.m_nCameraTriggerMode[camNo] = (long)TRIGGER_MODE::FREERUN;
		}
	}
	break;
	case (long)CONFIG::IMAGE_WIDTH:
	{
		long camNo = _ttol((CString)(GroupHardware_CameraNo->GetValue())) - 1;
		param.m_nImageWidth[camNo] = (long)value;
	}
	break;
	case (long)CONFIG::IMAGE_HEIGHT:
	{
		long camNo = _ttol((CString)(GroupHardware_CameraNo->GetValue())) - 1;
		param.m_nImageHeight[camNo] = (long)value;
	}
	break;
	case (long)CONFIG::DIST_CAMERA_TO_CAMERA:
		param.m_fDistToCamera = (float)value;
	break;
	case (long)CONFIG::MULTI_ILLUMINATION:
		param.m_bMultiIllumination = (CString)value == _T("TRUE") ? TRUE : FALSE;
		break;
	case (long)CONFIG::DIVISION_INDEX:
		param.m_nDivisionIndex = (long)value;
		break;
	case (long)CONFIG::LED_PORT:
		param.m_nLEDComPort = (long)value;
		break;
	case (long)CONFIG::LED_BAUDRATE:
		param.m_nLEDComBaudRate = (long)value;
		break;
	case (long)CONFIG::USE_LASER_TCPIP:
	{
		param.m_bUseLaserTCPIP = (CString)value == _T("TRUE") ? TRUE : FALSE;
		GroupCommunication_LaserTCPIPNo->SetValue(_T("Laser TCP/IP 1"));
	}	
	break;
	case (long)CONFIG::LASER_TCPIP_NUM:
	{	
		param.m_nLaserTCPIPNum = (long)value;
		
		// NUM을 늘렸다가 다시 줄인 경우
		if (param.m_nLaserTCPIPNum < param.m_strLaserIP.size())
		{
			long count = (long)param.m_strLaserIP.size() - param.m_nLaserTCPIPNum;
			for (int i = 0; i < count; i++)
			{
				param.m_strLaserIP.pop_back();
				param.m_nLaserPort.pop_back();
			}
		}
		else if (param.m_nLaserTCPIPNum > param.m_strLaserIP.size()) // NUM이 늘어난 경우
		{
			long count = param.m_nLaserTCPIPNum - (long)param.m_strLaserIP.size();
			for (int i = 0; i < count; i++)
			{// Default [0]번째 내용 삽입
				param.m_strLaserIP.push_back(param.m_strLaserIP[0]);
				param.m_nLaserPort.push_back(param.m_nLaserPort[0]);
			}
		}

		GroupCommunication_LaserTCPIPNo->RemoveAllOptions();
		for (long idx = 0; idx < param.m_nLaserTCPIPNum; idx++)
		{
			strTemp.Format(_T("Laser TCP/IP %d"), idx + 1);
			GroupCommunication_LaserTCPIPNo->AddOption(strTemp);
		}

		if (param.m_nLaserTCPIPNum > 0)
		{
			GroupCommunication_LaserTCPIPNo->SetValue(_T("Laser TCP/IP 1"));
			GroupCommunication_LaserIP->SetValue(param.m_strLaserIP[0]);
			GroupCommunication_LaserPort->SetValue(param.m_nLaserPort[0]);
		}
	}
	break;
	case (long)CONFIG::LASER_TCPIP_NO:
	{
		GroupCommunication_LaserIP->SetValue(param.m_strLaserIP[LaserNo]);
		GroupCommunication_LaserPort->SetValue(param.m_nLaserPort[LaserNo]);
	}
		break;
	case (long)CONFIG::LASER_IP:
	{	
		param.m_strLaserIP[LaserNo] = (CString)value;
	}
		break;
	case (long)CONFIG::LASER_PORT:
	{	
		param.m_nLaserPort[LaserNo] = (long)value;
	}
		break;
	case (long)CONFIG::LASER_CONNECT_TIME:
		param.m_nLaserAutoConnectTimes = (long)value;
		break;
	case (long)CONFIG::USE_PLC_TCPIP:
		param.m_bUsePLCTCPIP = (CString)value == _T("TRUE") ? TRUE : FALSE;
		break;
	case (long)CONFIG::PLC_IP:
		param.m_strPLCIP = (CString)value;
		break;
	case (long)CONFIG::PLC_PORT:
		param.m_nPLCPort = (long)value;
		break;
	case (long)CONFIG::PLC_CONNECT_TIME:
		param.m_nPLCAutoConnectTimes = (long)value;
		break;
	case (long)CONFIG::RAW_IMAGE_SAVE_PATH:
		param.m_strRawImageSavePath = (CString)value;
		break;
	case (long)CONFIG::RESULT_IMAGE_SAVE_PATH:
		param.m_strResultImageSavePath = (CString)value;
		break;
	case (long)CONFIG::RESULT_DATA_SAVE_PATH:
		param.m_strResultDataSavePath = (CString)value;
		break;
	case (long)CONFIG::USE_AUTO_DELETE:
		param.m_bUseDataAutoDelete = (CString)value == _T("TRUE") ? TRUE : FALSE;
		break;
	case (long)CONFIG::AUTO_DELETE_DAYS:
		param.m_nAutoDeleteDays = (long)value;
		break;
	case (long)CONFIG::AUTO_DELETE_TIMES:
		param.m_nAutoDeleteTimes = (long)value;
		break;
	default:
		break;
	}

	//Property가 수정되었을 경우
	BOOL bModified = pProp->IsModified();
	if(bModified == TRUE)
	{
		m_bChange = TRUE;
	}
	else
	{
		m_bChange = FALSE;
	}

	return 0;
}


void CConfigSetupDlg::OnBnClickedBtnLoad()
{
	CFileDialog Dlg(TRUE, _T("ini"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Config File (*.ini)|*.ini||"), NULL, NULL, FALSE);

	if(Dlg.DoModal() == IDOK)
	{
		CString strPath = Dlg.GetPathName();

		::g_ConfigManager.LoadConfig(strPath);

		InitPropertyGrid();
	}
}
