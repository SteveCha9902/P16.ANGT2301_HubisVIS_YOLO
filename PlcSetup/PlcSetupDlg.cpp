
// PlcSetupDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "PlcSetup.h"
#include "PlcSetupDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CPlcSetupDlg 대화 상자

CPlcSetupDlg::CPlcSetupDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PLCSETUP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CPlcSetupDlg::~CPlcSetupDlg()
{
	if(m_bConnect == TRUE)
	{
		KillTimer(0);

		m_pCtrlPLC->Disconnect();
	}

	delete m_pCtrlPLC;
}

void CPlcSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_PLC, m_propertyGridPLC);
	DDX_Control(pDX, IDC_STATIC_CTRL, m_IconCtrl);
	DDX_Control(pDX, IDC_GRID_PLC_TO_VISION, m_gridPLC2Vision);
	DDX_Control(pDX, IDC_GRID_VISION_TO_PLC, m_gridVision2PLC);
}

BEGIN_MESSAGE_MAP(CPlcSetupDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CPlcSetupDlg::OnAfxWmPropertyChanged)
	ON_MESSAGE(UM_UPDATE_CTRL_STATUS, &CPlcSetupDlg::OnUpdateCtrlStatus)
	ON_NOTIFY(GVN_ENDLABELEDIT, IDC_GRID_PLC_TO_VISION, OnGridPLC2VisionEditEnd)
	ON_NOTIFY(GVN_STATECHANGED, IDC_GRID_PLC_TO_VISION, OnGridPLC2VisionEditEnd)
	ON_NOTIFY(GVN_ENDLABELEDIT, IDC_GRID_VISION_TO_PLC, OnGridVision2PLCEditEnd)
	ON_NOTIFY(GVN_STATECHANGED, IDC_GRID_VISION_TO_PLC, OnGridVision2PLCEditEnd)
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CPlcSetupDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CPlcSetupDlg::OnBnClickedBtnDisconnect)
	ON_BN_CLICKED(IDC_BTN_PLC_SAVE, &CPlcSetupDlg::OnBnClickedBtnPlcSave)
	ON_BN_CLICKED(IDC_BTN_PLC_LOAD, &CPlcSetupDlg::OnBnClickedBtnPlcLoad)
	ON_BN_CLICKED(IDC_BTN_START, &CPlcSetupDlg::OnBnClickedBtnStart)
	ON_BN_CLICKED(IDC_BTN_STOP, &CPlcSetupDlg::OnBnClickedBtnStop)
END_MESSAGE_MAP()


// CPlcSetupDlg 메시지 처리기

BOOL CPlcSetupDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if(pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if(!strAboutMenu.IsEmpty())
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

	m_hiconConnect = AfxGetApp()->LoadIcon(IDI_ICON_CONNECT);
	m_hiconDisConnect = AfxGetApp()->LoadIcon(IDI_ICON_DISCONNECT);

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

	::g_PLCManager.SetCurPath(m_strPath);
	::g_PLCManager.InitParameters();
	if (::g_PLCManager.LoadPLCMap(::g_PLCManager.m_strPath) == FALSE)
	{
		::g_PLCManager.SavePLCMap();
	}

	m_bChange = FALSE;
	m_bConnect = FALSE;

	InitPropertyGrid();
	InitPLC2VisionGrid();
	InitVision2PLCGrid();

	SetPLC2VisionGrid();
	SetVision2PLCGrid();

	m_pCtrlPLC = new CCtrlPLC;
	m_pCtrlPLC->Initialize();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CPlcSetupDlg::OnSysCommand(UINT nID, LPARAM lParam)
{

	CDialogEx::OnSysCommand(nID, lParam);

}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CPlcSetupDlg::OnPaint()
{
	if(IsIconic())
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
HCURSOR CPlcSetupDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CPlcSetupDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
		case VK_RETURN:
		case VK_ESCAPE:
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CPlcSetupDlg::InitPropertyGrid()
{
	CString strTemp;

	m_propertyGridPLC.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 150;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridPLC.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PLC
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupPLC = new CMFCPropertyGridProperty(_T("PLC"));
	m_propertyGridPLC.AddProperty(pGroupPLC);

	CMFCPropertyGridProperty* pGroupPLC_PLCIP = new CMFCPropertyGridProperty(_T("IP"), ::g_ConfigManager.m_strPLCIP, _T(""), (long)PLC::IP);
	pGroupPLC->AddSubItem(pGroupPLC_PLCIP);

	CMFCPropertyGridProperty* pGroupPLC_PLCPort = new CMFCPropertyGridProperty(_T("Port"), ::g_ConfigManager.m_nPLCPort, _T(""), (long)PLC::PORT);
	pGroupPLC->AddSubItem(pGroupPLC_PLCPort);

	CMFCPropertyGridProperty* pGroupPLC_PLCReadStartAddr = new CMFCPropertyGridProperty(_T("Read Start Address"), ::g_PLCManager.m_sPLC.nPLC2VisionStartAddr, _T(""), (long)PLC::START_READ_ADDRESS);
	pGroupPLC->AddSubItem(pGroupPLC_PLCReadStartAddr);

	CMFCPropertyGridProperty* pGroupPLC_PLCReadAddressCnt = new CMFCPropertyGridProperty(_T("Read Address Count"), ::g_PLCManager.m_sPLC.nPLC2VisionCnt, _T(""), (long)PLC::READ_COUNT);
	pGroupPLC->AddSubItem(pGroupPLC_PLCReadAddressCnt);

	CMFCPropertyGridProperty* pGroupPLC_PLCWriteStartAddr = new CMFCPropertyGridProperty(_T("Write Start Address"), ::g_PLCManager.m_sPLC.nVision2PLCStartAddr, _T(""), (long)PLC::START_WRITE_ADDRESS);
	pGroupPLC->AddSubItem(pGroupPLC_PLCWriteStartAddr);

	CMFCPropertyGridProperty* pGroupPLC_PLCWriteAddressCnt = new CMFCPropertyGridProperty(_T("Write Address Count"), ::g_PLCManager.m_sPLC.nVision2PLCCnt, _T(""), (long)PLC::WRITE_COUNT);
	pGroupPLC->AddSubItem(pGroupPLC_PLCWriteAddressCnt);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridPLC.AddProperty(pGroupEtc);
}

void CPlcSetupDlg::InitPLC2VisionGrid()
{
	auto param = ::g_PLCManager.m_sPLC;

	m_gridPLC2Vision.GetDefaultCell(FALSE, TRUE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridPLC2Vision.GetDefaultCell(TRUE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridPLC2Vision.GetDefaultCell(FALSE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridPLC2Vision.SetGridLineColor(RGB(0, 0, 0));
	m_gridPLC2Vision.SetDefCellMargin(0);
	m_gridPLC2Vision.SetFixedColumnSelection(FALSE);
	m_gridPLC2Vision.SetFixedRowSelection(FALSE);
	m_gridPLC2Vision.SetSingleColSelection(FALSE);
	m_gridPLC2Vision.SetSingleRowSelection(FALSE);
	m_gridPLC2Vision.SetTrackFocusCell(FALSE);
	m_gridPLC2Vision.SetRowResize(FALSE);
	m_gridPLC2Vision.SetColumnResize(FALSE);

	m_gridPLC2Vision.SetColumnCount(6);
	m_gridPLC2Vision.SetFixedRowCount(1);
	m_gridPLC2Vision.SetFixedColumnCount(1);
	m_gridPLC2Vision.SetColumnWidth(0, 30);
	m_gridPLC2Vision.SetColumnWidth(1, 170);
	m_gridPLC2Vision.SetColumnWidth(2, 100);
	m_gridPLC2Vision.SetColumnWidth(3, 70);
	m_gridPLC2Vision.SetColumnWidth(4, 50);
	m_gridPLC2Vision.SetColumnWidth(5, 50);

	m_gridPLC2Vision.SetItemText(0, 0, _T("No."));
	m_gridPLC2Vision.SetItemText(0, 1, _T("Name"));
	m_gridPLC2Vision.SetItemText(0, 2, _T("Address"));
	m_gridPLC2Vision.SetItemText(0, 3, _T("Type"));
	m_gridPLC2Vision.SetItemText(0, 4, _T("Bit"));
	m_gridPLC2Vision.SetItemText(0, 5, _T("Value"));

	m_gridPLC2Vision.DeleteNonFixedRows();
	m_gridPLC2Vision.SetRowCount(param.nPLC2VisionCnt + 1);
}

void CPlcSetupDlg::SetPLC2VisionGrid()
{
	CString strTemp;

	auto param = ::g_PLCManager.m_sPLC;

	for(long idx = 0; idx < param.nPLC2VisionCnt; idx++)
	{
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 0, _T("%d"), idx + 1);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 1, _T("%s"), param.vecsPLC2VisionData[idx].strName);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 2, _T("%d"), param.vecsPLC2VisionData[idx].nAddr);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 4, _T("%d"), param.vecsPLC2VisionData[idx].nBitNum);

		switch(param.vecsPLC2VisionData[idx].nType)
		{
		case (long)PLC_MEMORY_TYPE::BIT:
		{
			m_gridPLC2Vision.SetItemTextFmt(idx + 1, 3, _T("BIT"));
			if(param.vecsPLC2VisionData[idx].sValue == 0)
				strTemp.Format(_T("OFF"));
			else
				strTemp.Format(_T("ON"));
		}
		break;
		case (long)PLC_MEMORY_TYPE::INT:
		{
			m_gridPLC2Vision.SetItemTextFmt(idx + 1, 3, _T("INT"));
			strTemp.Format(_T("%d"), param.vecsPLC2VisionData[idx].sValue);
		}
		break;
		case (long)PLC_MEMORY_TYPE::WORD:
		{
			m_gridPLC2Vision.SetItemTextFmt(idx + 1, 3, _T("WORD"));
			char pString[3] = {0,};
			memcpy(pString, &param.vecsPLC2VisionData[idx].sValue, 2);
			strTemp.Format(_T("%s"), (CString)pString);
		}
		break;
		case (long)PLC_MEMORY_TYPE::DOUBLE:
		{
			m_gridPLC2Vision.SetItemTextFmt(idx + 1, 3, _T("DOUBLE"));
			strTemp.Format(_T("%d"), param.vecsPLC2VisionData[idx].nValue);
		}
		break;
		}

		COLORREF color(RGB(0, 0, 0));
		if(strTemp == _T("ON") || strTemp != _T("0"))
		{
			if(strTemp == _T("OFF") || strTemp == _T(""))
			{
				color = RGB(255, 0, 0);
			}
			else
			{
				color = RGB(0, 255, 0);
			}
		}
		else if(strTemp == _T("0"))
		{
			color = RGB(255, 0, 0);
		}

		m_gridPLC2Vision.SetItemBkColour(idx + 1, 5, color);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 5, strTemp);
	}
}

void CPlcSetupDlg::InitVision2PLCGrid()
{
	auto param = ::g_PLCManager.m_sPLC;

	m_gridVision2PLC.GetDefaultCell(FALSE, TRUE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridVision2PLC.GetDefaultCell(TRUE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridVision2PLC.GetDefaultCell(FALSE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridVision2PLC.SetGridLineColor(RGB(0, 0, 0));
	m_gridVision2PLC.SetDefCellMargin(0);
	m_gridVision2PLC.SetFixedColumnSelection(FALSE);
	m_gridVision2PLC.SetFixedRowSelection(FALSE);
	m_gridVision2PLC.SetSingleColSelection(FALSE);
	m_gridVision2PLC.SetSingleRowSelection(FALSE);
	m_gridVision2PLC.SetTrackFocusCell(FALSE);
	m_gridVision2PLC.SetRowResize(FALSE);
	m_gridVision2PLC.SetColumnResize(FALSE);

	m_gridVision2PLC.SetColumnCount(6);
	m_gridVision2PLC.SetFixedRowCount(1);
	m_gridVision2PLC.SetFixedColumnCount(1);
	m_gridVision2PLC.SetColumnWidth(0, 30);
	m_gridVision2PLC.SetColumnWidth(1, 170);
	m_gridVision2PLC.SetColumnWidth(2, 100);
	m_gridVision2PLC.SetColumnWidth(3, 70);
	m_gridVision2PLC.SetColumnWidth(4, 50);
	m_gridVision2PLC.SetColumnWidth(5, 50);

	m_gridVision2PLC.SetItemText(0, 0, _T("No."));
	m_gridVision2PLC.SetItemText(0, 1, _T("Name"));
	m_gridVision2PLC.SetItemText(0, 2, _T("Address"));
	m_gridVision2PLC.SetItemText(0, 3, _T("Type"));
	m_gridVision2PLC.SetItemText(0, 4, _T("Bit"));
	m_gridVision2PLC.SetItemText(0, 5, _T("Value"));

	m_gridVision2PLC.DeleteNonFixedRows();
	m_gridVision2PLC.SetRowCount(param.nVision2PLCCnt + 1);
}

void CPlcSetupDlg::SetVision2PLCGrid()
{
	CString strTemp;

	auto param = ::g_PLCManager.m_sPLC;

	for(long idx = 0; idx < param.nVision2PLCCnt; idx++)
	{
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 0, _T("%d"), idx + 1);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 1, _T("%s"), param.vecsVision2PLCData[idx].strName);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 2, _T("%d"), param.vecsVision2PLCData[idx].nAddr);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 4, _T("%d"), param.vecsVision2PLCData[idx].nBitNum);

		switch(param.vecsVision2PLCData[idx].nType)
		{
		case (long)PLC_MEMORY_TYPE::BIT:
		{
			m_gridVision2PLC.SetItemTextFmt(idx + 1, 3, _T("BIT"));
			if(param.vecsVision2PLCData[idx].sValue == 0)
				strTemp.Format(_T("OFF"));
			else
				strTemp.Format(_T("ON"));
		}
		break;
		case (long)PLC_MEMORY_TYPE::INT:
		{
			m_gridVision2PLC.SetItemTextFmt(idx + 1, 3, _T("INT"));
			strTemp.Format(_T("%d"), param.vecsVision2PLCData[idx].sValue);
		}
		break;
		case (long)PLC_MEMORY_TYPE::WORD:
		{
			m_gridVision2PLC.SetItemTextFmt(idx + 1, 3, _T("WORD"));
			char pString[3] = {0,};
			memcpy(pString, &param.vecsVision2PLCData[idx].sValue, 2);
			strTemp.Format(_T("%s"), (CString)pString);
		}
		break;
		case (long)PLC_MEMORY_TYPE::DOUBLE:
		{
			m_gridVision2PLC.SetItemTextFmt(idx + 1, 3, _T("DOUBLE"));
			strTemp.Format(_T("%d"), param.vecsVision2PLCData[idx].sValue);
		}
		break;
		}

		COLORREF color(RGB(0, 0, 0));
		if(strTemp == _T("ON") || strTemp != _T("0"))
		{
			if(strTemp == _T("OFF") || strTemp == _T(""))
			{
				color = RGB(255, 0, 0);
			}
			else
			{
				color = RGB(0, 255, 0);
			}
		}
		else if(strTemp == _T("0"))
		{
			color = RGB(255, 0, 0);
		}

		m_gridVision2PLC.SetItemBkColour(idx + 1, 5, color);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 5, strTemp);
	}
}

void CPlcSetupDlg::SetCtrlConnectIcon(const BOOL bConnect)
{
	if(bConnect)
	{
		m_IconCtrl.SetIcon(m_hiconConnect);
	}
	else
	{
		m_IconCtrl.SetIcon(m_hiconDisConnect);
	}
}

LRESULT CPlcSetupDlg::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	CMFCPropertyGridProperty *pProp = (CMFCPropertyGridProperty*)lParam;

	if(pProp == nullptr)
		return 0;

	CString strTemp;

	CString str = pProp->GetName();
	_variant_t value = pProp->GetValue();

	auto *GroupPLC = m_propertyGridPLC.GetProperty(0);
	auto *GroupPLC_PLCNum = GroupPLC->GetSubItem(2);
	auto *GroupPLC_PLC = GroupPLC->GetSubItem(3);
	auto *GroupPLC_StartReadAddr = GroupPLC->GetSubItem(4);
	auto *GroupPLC_ReadCount = GroupPLC->GetSubItem(5);
	auto *GroupPLC_StartWriteAddr = GroupPLC->GetSubItem(6);
	auto *GroupPLC_WriteCount = GroupPLC->GetSubItem(7);

	CString strValue = GroupPLC_PLC->GetValue();
	strValue.Replace(_T("PLC "), _T(""));

	auto &ConfigParam = ::g_ConfigManager;
	auto &PLCParam = ::g_PLCManager;

	long data = (long)pProp->GetData();
	switch(data)
	{
	case (long)PLC::IP:
		ConfigParam.m_strPLCIP = (CString)value;
		break;
	case (long)PLC::PORT:
		ConfigParam.m_nPLCPort = (long)value;
		break;
	case (long)PLC::START_READ_ADDRESS:
	{
		PLCParam.m_sPLC.nPLC2VisionStartAddr = (long)value;
	}
	break;
	case (long)PLC::READ_COUNT:
	{
		PLCParam.m_sPLC.nPLC2VisionCnt = (long)value;

		long newSize = PLCParam.m_sPLC.nPLC2VisionCnt;
		long oldSize = (long)PLCParam.m_sPLC.vecsPLC2VisionData.size();

		if(newSize > oldSize)
		{
			for(long idx = 0; idx < newSize - oldSize; idx++)
			{
				SPLCData sPLCData;
				sPLCData.Init();

				sPLCData.nAddr = PLCParam.m_sPLC.nPLC2VisionStartAddr;
				PLCParam.m_sPLC.vecsPLC2VisionData.push_back(sPLCData);
			}
		}
		else if(newSize < oldSize)
		{
			PLCParam.m_sPLC.vecsPLC2VisionData.resize(newSize);
		}
	}
	break;

	case (long)PLC::START_WRITE_ADDRESS:
	{
		PLCParam.m_sPLC.nVision2PLCStartAddr = (long)value;
	}
	break;
	case (long)PLC::WRITE_COUNT:
	{
		PLCParam.m_sPLC.nVision2PLCCnt = (long)value;

		long newSize = PLCParam.m_sPLC.nVision2PLCCnt;
		long oldSize = (long)PLCParam.m_sPLC.vecsVision2PLCData.size();

		if(newSize > oldSize)
		{
			for(long idx = 0; idx < newSize - oldSize; idx++)
			{
				SPLCData sPLCData;
				sPLCData.Init();

				sPLCData.nAddr = PLCParam.m_sPLC.nVision2PLCStartAddr;
				PLCParam.m_sPLC.vecsVision2PLCData.push_back(sPLCData);
			}
		}
		else if(newSize < oldSize)
		{
			PLCParam.m_sPLC.vecsVision2PLCData.resize(newSize);
		}
	}
	break;
	default:
		break;
	}

	SetPLC2VisionGrid();
	SetVision2PLCGrid();

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

LRESULT CPlcSetupDlg::OnUpdateCtrlStatus(WPARAM wParam, LPARAM lParam)
{
	BOOL bConnect = (long)lParam;

	SetCtrlConnectIcon(bConnect);

	return 0L;
}

void CPlcSetupDlg::OnBnClickedBtnConnect()
{
	m_bConnect = m_pCtrlPLC->Connect();

	if(m_bConnect == TRUE)
	{
		SetCtrlConnectIcon(TRUE);

		SetTimer(0, 500, NULL);
	}
	else
	{
		AfxMessageBox(_T("연결에 실패했습니다."));
	}
}


void CPlcSetupDlg::OnBnClickedBtnDisconnect()
{
	if(m_bConnect == TRUE)
	{
		KillTimer(0);

		m_pCtrlPLC->Disconnect();
		SetCtrlConnectIcon(FALSE);
	}
}


void CPlcSetupDlg::OnBnClickedBtnPlcSave()
{
	::g_PLCManager.SavePLCMap();
	::g_ConfigManager.SaveConfig();
}


void CPlcSetupDlg::OnBnClickedBtnPlcLoad()
{
	::g_PLCManager.LoadPLCMap(::g_PLCManager.m_strPath);
	::g_ConfigManager.LoadConfig();

	InitPropertyGrid();

	SetPLC2VisionGrid();
	SetVision2PLCGrid();
}

void CPlcSetupDlg::OnGridPLC2VisionEditEnd(NMHDR * pNotifyStruct, LRESULT * result)
{
	NM_GRIDVIEW* pNotify = (NM_GRIDVIEW*)pNotifyStruct;
	int row = pNotify->iRow;
	int col = pNotify->iColumn;

	auto &params = ::g_PLCManager.m_sPLC;

	if(row < 0 || row > params.nPLC2VisionCnt || col < 0 || col > 6)
	{
		return;
	}

	CString strParam(m_gridPLC2Vision.GetItemText(row, col));
	long value = (long)_ttol(strParam);

	switch(col)
	{
	case 0:
		break;
	case 1:
		params.vecsPLC2VisionData[row - 1].strName = (CString)strParam;
		break;
	case 2:
		params.vecsPLC2VisionData[row - 1].nAddr = (long)value;
		break;
	case 3:
		if(strParam == _T("BIT"))
			params.vecsPLC2VisionData[row - 1].nType = (long)PLC_MEMORY_TYPE::BIT;
		else if(strParam == _T("WORD"))
			params.vecsPLC2VisionData[row - 1].nType = (long)PLC_MEMORY_TYPE::WORD;
		else if(strParam == _T("INT"))
			params.vecsPLC2VisionData[row - 1].nType = (long)PLC_MEMORY_TYPE::INT;
		else if (strParam == _T("DOUBLE"))
			params.vecsPLC2VisionData[row - 1].nType = (long)PLC_MEMORY_TYPE::DOUBLE;
		break;
	case 4:
		params.vecsPLC2VisionData[row - 1].nBitNum = (long)value;
		break;
	case 5:
		params.vecsPLC2VisionData[row - 1].sValue = (short)value;
		break;
	default:
		break;
	}

	Invalidate(FALSE);

	*result = 0;
}


void CPlcSetupDlg::OnGridVision2PLCEditEnd(NMHDR * pNotifyStruct, LRESULT * result)
{
	NM_GRIDVIEW* pNotify = (NM_GRIDVIEW*)pNotifyStruct;
	int row = pNotify->iRow;
	int col = pNotify->iColumn;

	auto &params = ::g_PLCManager.m_sPLC;

	if(row < 0 || row > params.nVision2PLCCnt || col < 0 || col > 6)
	{
		return;
	}

	CString strParam(m_gridVision2PLC.GetItemText(row, col));
	long value = (long)_ttol(strParam);

	switch(col)
	{
	case 0:
		break;
	case 1:
		params.vecsVision2PLCData[row - 1].strName = (CString)strParam;
		break;
	case 2:
		params.vecsVision2PLCData[row - 1].nAddr = (long)value;
		break;
	case 3:
		if(strParam == _T("BIT"))
			params.vecsVision2PLCData[row - 1].nType = (long)PLC_MEMORY_TYPE::BIT;
		else if(strParam == _T("WORD"))
			params.vecsVision2PLCData[row - 1].nType = (long)PLC_MEMORY_TYPE::WORD;
		else if (strParam == _T("INT"))
			params.vecsVision2PLCData[row - 1].nType = (long)PLC_MEMORY_TYPE::INT;
		else if (strParam == _T("DOUBLE"))
			params.vecsVision2PLCData[row - 1].nType = (long)PLC_MEMORY_TYPE::DOUBLE;
		break;
	case 4:
		params.vecsVision2PLCData[row - 1].nBitNum = (long)value;
		break;
	case 5:
		params.vecsVision2PLCData[row - 1].sValue = (short)value;
		break;
	default:
		break;
	}

	Invalidate(FALSE);

	*result = 0;
}


void CPlcSetupDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
	case 0:
	{
		if(m_bConnect == TRUE)
		{
			m_pCtrlPLC->ReadPLC2VisionData();
			m_pCtrlPLC->WriteVision2PLCData();

			SetPLC2VisionGrid();
			SetVision2PLCGrid();
		}
	}
	break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CPlcSetupDlg::OnBnClickedBtnStart()
{
	SetTimer(0, 50, NULL);
}


void CPlcSetupDlg::OnBnClickedBtnStop()
{
	KillTimer(0);
}
