// DlgMain2.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgVisionMain.h"
#include "DlgMain2.h"
#include "afxdialogex.h"

#include "DlgIndexSetup.h"

#include <windows.h> // sjpark 250617 공유메모리 작업
#include <iostream>  // sjpark 250617 공유메모리 작업


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgMain2 대화 상자

IMPLEMENT_DYNAMIC(CDlgMain2, CDialogEx)

CDlgMain2::CDlgMain2(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_MAIN2, pParent)
	, m_pParent(pParent)
	, m_bConnectSerial(FALSE)
	, m_nVisionIndex(0)
	, m_nDivIndex(0)
{
	for(long idx = 0 ; idx < 2; idx++)
	{
		m_pImageDisplay[idx*3+0] = nullptr;
		m_nImageWidth  [idx*3+0] = 0;
		m_nImageHeight [idx*3+0] = 0;
		m_nImageSize   [idx*3+0] = 0;
		m_pImageDisplay[idx*3+1] = nullptr;
		m_nImageWidth  [idx*3+1] = 0;
		m_nImageHeight [idx*3+1] = 0;
		m_nImageSize   [idx*3+1] = 0;
		m_pImageDisplay[idx*3+2] = nullptr;
		m_nImageWidth  [idx*3+2] = 0;
		m_nImageHeight [idx*3+2] = 0;
		m_nImageSize   [idx*3+2] = 0;

		m_pCtrlCamera[idx] = nullptr;
		m_bConnectCamera[idx] = FALSE;
		//m_bResult[idx] = FALSE;
	}
	for (int i = 0; i < MAX_DISPLAY_NUM + 1; i++)
	{
		m_bResult[i] = TRUE;
	}
	//sjpark 250625 테스트 추가
	//InitSocketClient();
}

CDlgMain2::~CDlgMain2()
{
	::DeleteCriticalSection(&m_csLog);

	for (long idx = 0; idx < MAX_DISPLAY_NUM; idx++)
	{
		delete m_pImageDisplay[idx];
	}

	for (long idx = 0; idx < 2; idx++)
	{
		if(m_pCtrlCamera[idx] != nullptr)
		{
			delete m_pCtrlCamera[idx];
			m_pCtrlCamera[idx] = nullptr;
		}
	}
	//sjpark 250625 테스트 추가
	//CleanupSocket();
}

void CDlgMain2::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MAIN_LOG, m_logBox);
	DDX_Control(pDX, IDC_STATIC_CTRL, m_IconCtrl);
	DDX_Control(pDX, IDC_STATIC_LASER, m_IconLaser);
	DDX_Control(pDX, IDC_STATIC_CAM1, m_IconCam[0]);
	DDX_Control(pDX, IDC_STATIC_CAM2, m_IconCam[1]);
	DDX_Control(pDX, IDC_STATIC_ILLUM, m_IconIllum);
	DDX_Control(pDX, IDC_STATIC_GRAB1, m_IconGrab[0]);
	DDX_Control(pDX, IDC_STATIC_GRAB2, m_IconGrab[1]);
	DDX_Control(pDX, IDC_STATIC_GRAB3, m_IconGrab[2]);
	DDX_Control(pDX, IDC_STATIC_GRAB4, m_IconGrab[3]);
	DDX_Control(pDX, IDC_STATIC_GRAB5, m_IconGrab[4]);
	DDX_Control(pDX, IDC_STATIC_INSP1, m_IconInsp[0]);
	DDX_Control(pDX, IDC_STATIC_INSP2, m_IconInsp[1]);
	DDX_Control(pDX, IDC_STATIC_INSP3, m_IconInsp[2]);
	DDX_Control(pDX, IDC_STATIC_INSP4, m_IconInsp[3]);
	DDX_Control(pDX, IDC_STATIC_INSP5, m_IconInsp[4]);
	DDX_Control(pDX, IDC_STATIC_INSP6, m_IconInsp[5]);
	DDX_Control(pDX, IDC_STATIC_INSP7, m_IconInsp[6]);
	DDX_Control(pDX, IDC_STATIC_INSP8, m_IconInsp[7]);
	DDX_Control(pDX, IDC_STATIC_INSP9, m_IconInsp[8]);
	DDX_Control(pDX, IDC_STATIC_INSP10, m_IconInsp[9]);
	DDX_Control(pDX, IDC_STATIC_RESULT1, m_lblResult[0]);
	DDX_Control(pDX, IDC_STATIC_RESULT2, m_lblResult[1]);
	DDX_Control(pDX, IDC_STATIC_RESULT3, m_lblResult[2]);
	DDX_Control(pDX, IDC_STATIC_RESULT4, m_lblResult[3]);
	DDX_Control(pDX, IDC_STATIC_RESULT5, m_lblResult[4]);
	DDX_Control(pDX, IDC_STATIC_RESULT6, m_lblResult[5]);
	DDX_Control(pDX, IDC_STATIC_RESULT7, m_lblResult[6]);
	DDX_Control(pDX, IDC_STATIC_RECIPE, m_lblRecipe);
	DDX_Control(pDX, IDC_STATIC_USER, m_lblUser);
	DDX_Control(pDX, IDC_GRID_RESULT, m_gridResult);
	DDX_Control(pDX, IDC_BTN_SETUP, m_btnSetup);
	DDX_Control(pDX, IDC_STATIC_TOTAL_RESULT_TXT, m_lblTotalResult);
}

BEGIN_MESSAGE_MAP(CDlgMain2, CDialogEx)
	ON_MESSAGE(UM_UPDATE_LOG, &CDlgMain2::OnUpdateLog)
	ON_BN_CLICKED(IDC_BTN_SETUP, &CDlgMain2::OnBnClickedBtnSetup)
	ON_BN_CLICKED(IDC_BTN_INSPECTION, &CDlgMain2::OnBnClickedBtnInspection)
	ON_BN_CLICKED(IDC_BTN_USER_LOGIN, &CDlgMain2::OnBnClickedBtnUserLogin)
	//ON_BN_CLICKED(IDC_BUTTON1, &CDlgMain2::OnBnClickedButton1)
	//ON_BN_CLICKED(IDC_BUTTON2, &CDlgMain2::OnBnClickedButton2)
	//ON_BN_CLICKED(IDC_BUTTON3, &CDlgMain2::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BTN_TEST_BATCH, &CDlgMain2::OnBnClickedBtnTestBatch)
END_MESSAGE_MAP()

// CDlgMain2 메시지 처리기

BOOL CDlgMain2::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	::InitializeCriticalSection(&m_csLog);
	
	CString strTemp;

	// Font 설정
	CFont* currFont = this->GetFont();
	LOGFONT logFont;
	currFont->GetLogFont(&logFont);

	logFont.lfHeight = 30;
	logFont.lfWeight = FW_BOLD;
	m_Font.CreateFontIndirectW(&logFont);
	GetDlgItem(IDC_STATIC_TIME)->SetFont(&m_Font);

	m_hiconConnect = AfxGetApp()->LoadIcon(IDI_ICON_CONNECT);
	m_hiconDisConnect = AfxGetApp()->LoadIcon(IDI_ICON_DISCONNECT);

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
	
	// 2D Viewer, Camera Connect
	for(long idx = 0; idx < camCount; idx++)
	{	
		m_pImageDisplay[idx*3+0] = new CImageDisplay;
		m_pImageDisplay[idx*3+0]->Initialize(IDC_STATIC_IMAGE_VIEWER11 + idx*3+0, this, RGB(50, 50, 50));
		m_pImageDisplay[idx*3+0]->SetCrossLineColor(RGB(255, 0, 0));
		m_pImageDisplay[idx*3+0]->SetCustomLineColor(RGB(255, 0, 0));
		m_pImageDisplay[idx*3+1] = new CImageDisplay;
		m_pImageDisplay[idx*3+1]->Initialize(IDC_STATIC_IMAGE_VIEWER11 + idx*3+1, this, RGB(50, 50, 50));
		m_pImageDisplay[idx*3+1]->SetCrossLineColor(RGB(255, 0, 0));
		m_pImageDisplay[idx*3+1]->SetCustomLineColor(RGB(255, 0, 0));
		m_pImageDisplay[idx*3+2] = new CImageDisplay;
		m_pImageDisplay[idx*3+2]->Initialize(IDC_STATIC_IMAGE_VIEWER11 + idx*3+2, this, RGB(50, 50, 50));
		m_pImageDisplay[idx*3+2]->SetCrossLineColor(RGB(255, 0, 0));
		m_pImageDisplay[idx*3+2]->SetCustomLineColor(RGB(255, 0, 0));

		// Hardware 연결...
		if(::g_ConfigManager.m_bHardwareExist == TRUE)
		{
			m_pCtrlCamera[idx] = new CCtrlCamera;

			// 카메라 연결...
			// Gige Camear IP 추가 - SHK 2023.12.27
			if(m_pCtrlCamera[idx]->Open(::g_ConfigManager.m_strCameraIP[idx]) == TRUE)
			{
				m_nImageWidth[idx*3+0] = m_pCtrlCamera[idx]->GetImageWidth();
				m_nImageHeight[idx*3+0]= m_pCtrlCamera[idx]->GetImageHeight();
				m_nImageSize[idx*3+0]  = m_nImageWidth[idx] * m_nImageHeight[idx];

				m_nImageWidth[idx * 3 + 1] = m_pCtrlCamera[idx]->GetImageWidth();
				m_nImageHeight[idx * 3 + 1] = m_pCtrlCamera[idx]->GetImageHeight();
				m_nImageSize[idx * 3 + 1] = m_nImageWidth[idx] * m_nImageHeight[idx];

				m_nImageWidth[idx * 3 + 2] = m_pCtrlCamera[idx]->GetImageWidth();
				m_nImageHeight[idx * 3 + 2] = m_pCtrlCamera[idx]->GetImageHeight();
				m_nImageSize[idx * 3 + 2] = m_nImageWidth[idx] * m_nImageHeight[idx];

				SetCamConnectIcon(TRUE, idx);
			}
			else if(::g_ConfigManager.m_bPLCAutoPass != FALSE)
			{
				// PLCAutoPass=1: 카메라가 없어도 초기화를 계속하고, 검사는 Master 이미지(ScanManager)로 대체한다
				::g_Log.Log(_T("[CAMERA_DIAG][WARN] Camera%d Open Failed - continue with Master image [Applying AutoPass]"), idx + 1);

				m_nImageWidth [idx*3+0] = ::g_ConfigManager.m_nImageWidth[idx];
				m_nImageHeight[idx*3+0] = ::g_ConfigManager.m_nImageHeight[idx];
				m_nImageSize  [idx*3+0] = m_nImageWidth[idx] * m_nImageHeight[idx];
				m_nImageWidth [idx*3+1] = ::g_ConfigManager.m_nImageWidth[idx];
				m_nImageHeight[idx*3+1] = ::g_ConfigManager.m_nImageHeight[idx];
				m_nImageSize  [idx*3+1] = m_nImageWidth[idx] * m_nImageHeight[idx];
				m_nImageWidth [idx*3+2] = ::g_ConfigManager.m_nImageWidth[idx];
				m_nImageHeight[idx*3+2] = ::g_ConfigManager.m_nImageHeight[idx];
				m_nImageSize  [idx*3+2] = m_nImageWidth[idx] * m_nImageHeight[idx];

				SetCamConnectIcon(FALSE, idx);
			}
			else
			{
				strTemp.Format(_T("Camera%d Open Failed..."), idx + 1);

				::AfxMessageBox(strTemp);

				return FALSE;
			}
		}
		else
		{
			m_nImageWidth [idx*3+0] = ::g_ConfigManager.m_nImageWidth[idx];
			m_nImageHeight[idx*3+0]= ::g_ConfigManager.m_nImageHeight[idx];
			m_nImageSize  [idx*3+0]  = m_nImageWidth[idx] * m_nImageHeight[idx];
			m_nImageWidth [idx*3+1] = ::g_ConfigManager.m_nImageWidth[idx];
			m_nImageHeight[idx*3+1]= ::g_ConfigManager.m_nImageHeight[idx];
			m_nImageSize  [idx*3+1]  = m_nImageWidth[idx] * m_nImageHeight[idx];
			m_nImageWidth [idx*3+2] = ::g_ConfigManager.m_nImageWidth[idx];
			m_nImageHeight[idx*3+2]= ::g_ConfigManager.m_nImageHeight[idx];
			m_nImageSize  [idx*3+2]  = m_nImageWidth[idx] * m_nImageHeight[idx];
		}

		SetResult(idx*3+0, _T("NOT MEASURED"));
		SetResult(idx*3+1, _T("NOT MEASURED"));
		SetResult(idx*3+2, _T("NOT MEASURED"));
	}

	// 메모리 생성...
	if(m_nImageWidth[0] != m_nImageWidth[1] || m_nImageHeight[0] != m_nImageHeight[1])
	{
		// image size 큰 쪽으로 메모리를 생성한다...
		auto imageWidth = m_nImageWidth[0] > m_nImageWidth[1] ? m_nImageWidth[0] : m_nImageWidth[1];
		auto imageHeight = m_nImageHeight[0] > m_nImageHeight[1] ? m_nImageHeight[0] : m_nImageHeight[1];

		::g_MemoryManager.SetImageSize(imageWidth, imageHeight);
	}
	else
	{
		::g_MemoryManager.SetImageSize(m_nImageWidth[0], m_nImageHeight[0]);
	}

	// Hardware 연결...
	if(::g_ConfigManager.m_bHardwareExist == TRUE)
	{
		// 조명
		if(::g_CtrlSerial.IsOpen() == FALSE)
		{
			::g_CtrlSerial.Close();
		}

		UINT comPort = (UINT)::g_ConfigManager.m_nLEDComPort;
		UINT baudRate = (UINT)::g_ConfigManager.m_nLEDComBaudRate;
		UINT byteSize = 8;
		UINT stopBits = 1;
		UINT parity = 0;

		m_bConnectSerial = ::g_CtrlSerial.Open(comPort, baudRate, byteSize, stopBits, parity);

		if(m_bConnectSerial == FALSE && ::g_ConfigManager.m_bPLCAutoPass != FALSE)
		{
			// PLCAutoPass=1: 조명이 없어도 초기화를 계속한다 (검사는 Master 이미지로 대체)
			::g_Log.Log(_T("[CAMERA_DIAG][WARN] Serial Connect Failed - continue with Master image [Applying AutoPass]"));
		}
		else if(m_bConnectSerial == FALSE) // 연결 실패...
		{
			::AfxMessageBox(_T("Serial Connect Failed..."));

			return FALSE;
		}
		else
		{
			SetIllumConnectIcon(TRUE);
		}
	}
	
	SetUser(::g_UserManager.m_nPreLoginUserIndex);

	// 정상 접속 했을 경우 Ready 신호 On
	if(::g_ConfigManager.m_bHardwareExist == TRUE)
	{
		if (m_pCtrlCamera[0]->IsOpen() == TRUE && m_pCtrlCamera[1]->IsOpen() == TRUE && m_bConnectSerial == TRUE)
		{
			::g_PLCManager.m_sPLC.vecsVision2PLCData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_RESP_ReadyInspection].sValue = (long)BORGWARNER::DATA::ON;
			::g_PLCSocketManager.m_pCtrlPLC->WriteVision2PLCData();
		}
	}

	InitResultGrid();
	SetResultGrid();

	::g_Log.Log(_T("Hubis Vision Inspection System Started!"));

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

BOOL CDlgMain2::PreTranslateMessage(MSG* pMsg)
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

	return CDialog::PreTranslateMessage(pMsg);
}

void CDlgMain2::InitResultGrid()
{
	m_gridResult.GetDefaultCell(FALSE, TRUE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridResult.GetDefaultCell(TRUE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridResult.GetDefaultCell(FALSE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridResult.SetGridLineColor(RGB(0, 0, 0));
	m_gridResult.SetDefCellMargin(0);
	m_gridResult.SetFixedColumnSelection(FALSE);
	m_gridResult.SetFixedRowSelection(FALSE);
	m_gridResult.SetSingleColSelection(FALSE);
	m_gridResult.SetSingleRowSelection(FALSE);
	m_gridResult.SetTrackFocusCell(FALSE);
	m_gridResult.SetRowResize(FALSE);
	m_gridResult.SetColumnResize(FALSE);
}

void CDlgMain2::SetResultGrid()
{
	CString strTemp;

	switch(::g_ConfigManager.m_nCustomer)
	{
	case (long)CUSTOMER::BORGWARNER:
	{
		auto busbarInfo = ::g_RecipeManager.m_sBusbarInfoParam;
		auto busbarCount = busbarInfo.nDCCount * busbarInfo.nGrabCount + busbarInfo.nACCount * busbarInfo.nGrabCount;
		long criteriaItemCount(0);

		if(busbarInfo.nInspectionDivision == (long)INSPECTION_DIVISION::BEFORE)
		{
			criteriaItemCount = 5;
		}
		else if(busbarInfo.nInspectionDivision == (long)INSPECTION_DIVISION::AFTER)
		{
			criteriaItemCount = 2;
		}
		else
		{
			return;
		}

		// Items + Header
		m_gridResult.SetRowCount(criteriaItemCount + 1);
		m_gridResult.SetColumnCount(busbarCount + 1);
		m_gridResult.SetFixedRowCount(1);
		m_gridResult.SetFixedColumnCount(1);
		m_gridResult.SetColumnWidth(0, 72);
		m_gridResult.SetRowHeight(0, 25);

		for(long idx = 1; idx < criteriaItemCount + 1; idx++)
		{
			m_gridResult.SetRowHeight(idx, 45);
		}

		m_gridResult.SetItemText(0, 0, _T("Insp.Item"));
		m_gridResult.SetItemText(0, 1, _T("DC1-1"));
		m_gridResult.SetItemText(0, 2, _T("DC1-2"));
		m_gridResult.SetItemText(0, 3, _T("DC1-3"));
		m_gridResult.SetItemText(0, 4, _T("AC1-1"));
		m_gridResult.SetItemText(0, 5, _T("DC2-1"));
		m_gridResult.SetItemText(0, 6, _T("DC2-2"));
		m_gridResult.SetItemText(0, 7, _T("DC2-3"));
		m_gridResult.SetItemText(0, 8, _T("AC2-1"));
		m_gridResult.SetItemText(0, 9, _T("DC3-1"));
		m_gridResult.SetItemText(0, 10,_T("DC3-2"));
		m_gridResult.SetItemText(0, 11,_T("DC3-3"));
		m_gridResult.SetItemText(0, 12,_T("AC3-1"));

		if(busbarInfo.nInspectionDivision == (long)INSPECTION_DIVISION::BEFORE)
		{
			m_gridResult.SetItemText(1, 0, _T("Start X, Y"));
			m_gridResult.SetItemText(2, 0, _T("End X, Y"));
			m_gridResult.SetItemText(3, 0, _T("Height"));
			m_gridResult.SetItemText(4, 0, _T("Length"));
			m_gridResult.SetItemText(5, 0, _T("Angle"));
		}
		else if(busbarInfo.nInspectionDivision == (long)INSPECTION_DIVISION::AFTER)
		{
			m_gridResult.SetItemText(1, 0, _T("Width"));
			m_gridResult.SetItemText(2, 0, _T("Height"));
		}

		for(long row = 1; row < criteriaItemCount + 1; row++)
		{
			for(long col = 1; col < busbarCount + 1; col++)
			{
				m_gridResult.SetItemState(row, col, GVIS_READONLY);
			}
		}
	}
	break;
	}

	Invalidate(FALSE);
}

void CDlgMain2::UpdateResultGrid(const BOOL bManual, const long visionIndex, const long divIndex, const long threadIndex)
{
	CString strTemp;

	::g_Log.Log(_T("CDlgMain2::UpdateResultGrid( visionindex : [%d]  divindex : [%d]"), visionIndex, divIndex);
	std::vector<SResultParams> vecResult;

	switch(::g_ConfigManager.m_nCustomer)
	{
	case (long)CUSTOMER::BORGWARNER:
	{
		auto matchItem = ::g_RecipeManager.m_vecInspItemMatch;

		long itemIndex(0); // 검사할 Item Index
		for(long idx = 0; idx < (long)matchItem.size(); idx++)
		{
			if(matchItem[idx].visionIndex == visionIndex && matchItem[idx].divIndex == divIndex)
			{
				itemIndex = matchItem[idx].matchingIndex;
			}
		}

		auto algoSequence = ::g_RecipeManager.m_vecsInspItem[itemIndex].algoSequence;
		auto busbarInfo = ::g_RecipeManager.m_sBusbarInfoParam;
		auto multiple = busbarInfo.nDCCount + busbarInfo.nACCount;

		if(bManual == TRUE)
		{
			vecResult = ::g_InspectionManager.m_pInspection->m_vecResult;
		}
		else
		{
			vecResult = ::g_InspectionManager.m_vecpInspection[threadIndex]->m_vecResult;
		}

		if(busbarInfo.nInspectionDivision == (long)INSPECTION_DIVISION::BEFORE)
		{
			switch(visionIndex)
			{
			case (long)CAMERA_INDEX::CAMERA1:
			{
				if ((long)vecResult.size() <= 0)//LYS 검사결과가 0개면 모두다0으로
				{
					for (long idx = 0; idx < ::g_RecipeManager.m_sBusbarInfoParam.nDCCount; idx++)//LYS240904
					{
						m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + 1, _T("0"));
						m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + 1, _T("0"));
						m_gridResult.SetItemTextFmt(3, (divIndex * multiple) + idx + 1, _T("0"));
						m_gridResult.SetItemTextFmt(4, (divIndex * multiple) + idx + 1, _T("0"));
						m_gridResult.SetItemTextFmt(5, (divIndex * multiple) + idx + 1, _T("0"));

						m_gridResult.SetTextColor(RGB(255, 0, 0));

					}
				}
				else
				{
					for (long idx = 0; idx < (long)vecResult.size(); idx++)
					{
						CPoint start(0, 0), end(0, 0);
						float topBusbarHeight(0.f);
						switch (algoSequence)
						{
						case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
							start = vecResult[idx].ptStartResult;
							end = vecResult[idx].ptEndResult;
							//topBusbarHeight = vecResult[idx].fEdgeTopBusbarHeight;

							strTemp.Format(_T("%d, %d"), start.x, start.y);
							m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
							strTemp.Format(_T("%d, %d"), end.x, end.y);
							m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
							strTemp.Format(_T("%.3f"), vecResult[idx].fEdgeTopBusbarHeight);
							m_gridResult.SetItemTextFmt(3, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
							strTemp.Format(_T("%.3f"), vecResult[idx].fEdgeLength);
							m_gridResult.SetItemTextFmt(4, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
							strTemp.Format(_T("%.3f"), vecResult[idx].fEdgeAngle);
							m_gridResult.SetItemTextFmt(5, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);

							break;
						case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
							start = vecResult[idx].ptBlobStart;
							end = vecResult[idx].ptBlobEnd;
							//opBusbarHeight = vecResult[idx].fBlobTopBusbarHeight;

							strTemp.Format(_T("%d, %d"), start.x, start.y);
							m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
							strTemp.Format(_T("%d, %d"), end.x, end.y);
							m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
							strTemp.Format(_T("%.3f"), vecResult[idx].fBlobTopBusbarHeight);
							m_gridResult.SetItemTextFmt(3, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
							strTemp.Format(_T("%.3f"), vecResult[idx].fBlobLength);
							m_gridResult.SetItemTextFmt(4, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
							strTemp.Format(_T("%.3f"), vecResult[idx].fBlobAngle);
							m_gridResult.SetItemTextFmt(5, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);

							break;
						}

						if (m_bResult[visionIndex * 2 + divIndex] != TRUE)
						{
							m_gridResult.SetTextColor(RGB(255, 0, 0));
						}
						else
						{

							m_gridResult.SetTextColor(RGB(0, 0, 0));
						}

					}
				}
				
			}
			break;
			case (long)CAMERA_INDEX::CAMERA2:
			{
				if ((long)vecResult.size() <= 0)
				{
					for (long idx = 0; idx < ::g_RecipeManager.m_sBusbarInfoParam.nACCount; idx++)//LYS240904
					{
						m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + multiple, _T("0"));
						m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + multiple, _T("0"));
						m_gridResult.SetItemTextFmt(3, (divIndex * multiple) + idx + multiple, _T("0"));
						m_gridResult.SetItemTextFmt(4, (divIndex * multiple) + idx + multiple, _T("0"));
						m_gridResult.SetItemTextFmt(5, (divIndex * multiple) + idx + multiple, _T("0"));
						m_gridResult.SetTextColor(RGB(255, 0, 0));
					}
				}
				for(long idx = 0; idx < (long)vecResult.size(); idx++)
				{
					CPoint start(0, 0), end(0, 0);
					float topBusbarHeight(0.f);
					switch(algoSequence)
					{
					case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
						start = vecResult[idx].ptStartResult;
						end = vecResult[idx].ptEndResult;
						topBusbarHeight = vecResult[idx].fEdgeTopBusbarHeight;

						strTemp.Format(_T("%d, %d"), start.x, start.y);
						m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
						strTemp.Format(_T("%d, %d"), end.x, end.y);
						m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
						strTemp.Format(_T("%.3f"), vecResult[idx].fEdgeTopBusbarHeight);
						m_gridResult.SetItemTextFmt(3, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
						strTemp.Format(_T("%.3f"), vecResult[idx].fEdgeLength);
						m_gridResult.SetItemTextFmt(4, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
						strTemp.Format(_T("%.3f"), vecResult[idx].fEdgeAngle);
						m_gridResult.SetItemTextFmt(5, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);

						break;
					case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
						start = vecResult[idx].ptBlobStart;
						end = vecResult[idx].ptBlobEnd;
						topBusbarHeight = vecResult[idx].fBlobTopBusbarHeight;

						strTemp.Format(_T("%d, %d"), start.x, start.y);
						m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
						strTemp.Format(_T("%d, %d"), end.x, end.y);
						m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
						strTemp.Format(_T("%.3f"), vecResult[idx].fBlobTopBusbarHeight);
						m_gridResult.SetItemTextFmt(3, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
						strTemp.Format(_T("%.3f"), vecResult[idx].fBlobLength);
						m_gridResult.SetItemTextFmt(4, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
						strTemp.Format(_T("%.3f"), vecResult[idx].fBlobAngle);
						m_gridResult.SetItemTextFmt(5, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);

						break;
					}
					if (m_bResult[visionIndex * 2 + divIndex] != TRUE)
					{
						m_gridResult.SetTextColor(RGB(255, 0, 0));
					}
					else
					{

						m_gridResult.SetTextColor(RGB(0, 0, 0));
					}

				}
			}
			break;
			}
		}
		else if(busbarInfo.nInspectionDivision == (long)INSPECTION_DIVISION::AFTER)
		{
			switch(visionIndex)
			{
			case (long)CAMERA_INDEX::CAMERA1:
			{
				if ((long)vecResult.size() <= 0)
				{
					for (long idx = 0; idx < ::g_RecipeManager.m_sBusbarInfoParam.nDCCount; idx++)//LYS240904
					{
						m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + 1, _T("0"));
						m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + 1, _T("0"));
						m_gridResult.SetTextColor(RGB(255, 0, 0));
					}
				}
				for(long idx = 0; idx < (long)vecResult.size(); idx++)
				{
					switch(algoSequence)
					{
					case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
						break;
					case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
						break;
					case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
						break;
					}

					//strTemp.Format(_T("%d, %d"), start.x, start.y);
					//m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
					//strTemp.Format(_T("%d, %d"), end.x, end.y);
					//m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
					strTemp.Format(_T("%.3f"), vecResult[idx].fWeldWidth);
					m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
					strTemp.Format(_T("%.3f"), vecResult[idx].fWeldHeight);
					m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + 1, _T("%s"), strTemp);
				}
			}
			break;
			case (long)CAMERA_INDEX::CAMERA2:
			{
				if ((long)vecResult.size() <= 0)
				{
					for (long idx = 0; idx < ::g_RecipeManager.m_sBusbarInfoParam.nACCount; idx++)//LYS240904
					{
						m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + multiple, _T("0"));
						m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + multiple, _T("0"));
						m_gridResult.SetTextColor(RGB(255, 0, 0));
					}
				}
				for(long idx = 0; idx < (long)vecResult.size(); idx++)
				{
					switch(algoSequence)
					{
					case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
						break;
					case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
						break;
					case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
						break;
					}

					//strTemp.Format(_T("%d, %d"), start.x, start.y);
					//m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
					//strTemp.Format(_T("%d, %d"), end.x, end.y);
					//m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
					strTemp.Format(_T("%.3f"), vecResult[idx].fWeldWidth);
					m_gridResult.SetItemTextFmt(1, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
					strTemp.Format(_T("%.3f"), vecResult[idx].fWeldHeight);
					m_gridResult.SetItemTextFmt(2, (divIndex * multiple) + idx + multiple, _T("%s"), strTemp);
				}
			}
			break;
			}
		}
	}
	break;
	}

	//250613 sjpark 결과 데이터 저장
	//::g_Log.LogResult(_T("%s%s%s%s"), vecResult, NULL, NULL, NULL);

	Invalidate(FALSE);
}

void CDlgMain2::SetImage(const long camIndex, const long divIndex)
{
	m_nVisionIndex = camIndex;
	m_nDivIndex = divIndex;
	int nDispIndex = m_nVisionIndex + m_nDivIndex * 2;

	m_pImageDisplay[nDispIndex]->Overlay_RemoveAll();

	BYTE* image = ::g_MemoryManager.GetByteImage(nDispIndex);

	m_pImageDisplay[nDispIndex]->SetImage(image, m_nImageWidth[nDispIndex], m_nImageHeight[nDispIndex], m_nImageWidth[nDispIndex]);

	m_pImageDisplay[nDispIndex]->Overlay_Show(TRUE);

	//m_pImageDisplay[index]->Overlay_RemoveAll();
	//BYTE* image = ::g_MemoryManager.GetByteImage(index);
	//m_pImageDisplay[index]->SetImage(image, m_nImageWidth[index], m_nImageHeight[index], m_nImageWidth[index]);
	//m_pImageDisplay[index]->Overlay_Show(TRUE);
}

void CDlgMain2::SetImageOverlay(const long camIndex, const long inspIndex)
{
	int nDispIndex = m_nVisionIndex + m_nDivIndex * 2;
	m_pImageDisplay[nDispIndex]->Overlay_RemoveAll();

	auto overlayManager = ::g_InspectionManager.m_vecpInspection[inspIndex]->m_pOverlayManager;

	for (long overlay = DEBUG_RESULT_BEGIN; overlay <= DEBUG_RESULT_END; overlay++)
	{
		overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], overlay);
	}

	for (long overlay = DEBUG_RESULT; overlay <= DEBUG_RESULT_NG; overlay++)
	{
		overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], overlay);
	}
	//for (long overlay = DEBUG_RESULT_RESULT; overlay <= DEBUG_RESULT_END; overlay++)
	//{
	//	overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], overlay);
	//}
	//
	//// 주석 복구 sjpark
	//// 임시 전체 결과 받아서 처리
	//BOOL bResult = TRUE;
	//if (bResult)
	//{
	//	overlayManager->ShowDebugResult(m_pImageDisplay[camIndex], DEBUG_RESULT_OK);
	//}
	//else
	//{
	//	overlayManager->ShowDebugResult(m_pImageDisplay[camIndex], DEBUG_RESULT_NG);
	//}

	m_pImageDisplay[nDispIndex]->Overlay_Show(TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Real Time 관련...
void CDlgMain2::SetRealTime(const CString strTime)
{
	SetDlgItemText(IDC_STATIC_TIME, strTime);
}
///////////////////////////////////////////////////////////////////////////////////////////////////

void CDlgMain2::SetResult(const long dispIndex, const CString strResult)
{
	CString strInspResult;
	COLORREF color(RGB(255, 0, 0));

	m_bResult[dispIndex] = TRUE;


	if(strResult == _T("OK"))
	{
		color = RGB(0, 255, 0);
	}

	if (strResult != _T(""))
	{
		if (m_bResult[dispIndex])
		{
			strInspResult = _T("/") + strResult;
		}
		else
		{
			strInspResult = strResult + _T("/");
		}
	}
	if (strResult == "NG")
	{
		m_bResult[dispIndex] = 0;
	}


	m_lblResult[dispIndex].SetText(strInspResult);
	if( strResult == "NOT MEASURED") m_lblResult[dispIndex].SetFontSize(14);
	else m_lblResult[dispIndex].SetFontSize(22);
	m_lblResult[dispIndex].SetFontBold(TRUE);
	m_lblResult[dispIndex].SetTextColor(color);
	m_lblResult[dispIndex].SetBorder(TRUE);

	m_lblResult[6].SetText(_T(""));
	m_bResult[6] = !m_bResult[dispIndex];
	// 하나라도 FALSE인 경우 var7을 FALSE로 설정
	if (m_bResult[0] == false || m_bResult[1] == false || m_bResult[2] == false ||
		m_bResult[3] == false || m_bResult[4] == false || m_bResult[5] == false) {
		m_bResult[6] = false;

		m_lblResult[6].SetText(_T("NG"));
		m_lblResult[6].SetFontSize(60);
		m_lblResult[6].SetFontBold(TRUE);
		m_lblResult[6].SetTextColor(RGB(255, 0, 0));
		m_lblResult[6].SetBorder(TRUE);
		
	}
	else {
		m_bResult[6] = true;  // 모두 TRUE인 경우 var7을 TRUE로 설정


			m_lblResult[6].SetText(_T("OK"));
			m_lblResult[6].SetFontSize(60);
			m_lblResult[6].SetFontBold(TRUE);
			m_lblResult[6].SetTextColor(RGB(0, 255, 0));
			m_lblResult[6].SetBorder(TRUE);
	}

	// 기타 UI 수정 PSJ 
	m_lblTotalResult.SetFontSize(30);
	m_lblTotalResult.SetFontBold(TRUE);
}

void CDlgMain2::SetUser(const long index)
{
	CString strUser;

	strUser = std::get<1>(::g_UserManager.m_vecUser[index]);
	m_lblUser.SetText(strUser);
	m_lblUser.SetFontSize(25);
	m_lblUser.SetFontBold(TRUE);
	m_lblUser.SetTextColor(RGB(0, 0, 255));
	m_lblUser.SetBorder(TRUE);

	// UserName이 Admin이 아니면 Setup 비활성화 PSJ
	if (strUser == _T("User")) m_btnSetup.EnableWindow(FALSE);
	else m_btnSetup.EnableWindow(TRUE);
}

void CDlgMain2::SetRecipe(const CString strRecipeName)
{
	m_lblRecipe.SetText(strRecipeName);
	m_lblRecipe.SetFontSize(30);
	m_lblRecipe.SetFontBold(TRUE);
	m_lblRecipe.SetTextColor(RGB(0, 0, 0));
	m_lblRecipe.SetBorder(TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Icon 관련...
void CDlgMain2::SetCtrlConnectIcon(const BOOL bConnect)
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

void CDlgMain2::SetLaserConnectIcon(const BOOL bConnect)
{
	if(bConnect)
	{
		m_IconLaser.SetIcon(m_hiconConnect);
	}
	else
	{
		m_IconLaser.SetIcon(m_hiconDisConnect);
	}
}

void CDlgMain2::SetCamConnectIcon(const BOOL bConnect, const long index)
{
	if(bConnect)
	{
		m_IconCam[index].SetIcon(m_hiconConnect);
	}
	else
	{
		m_IconCam[index].SetIcon(m_hiconDisConnect);
	}
}

void CDlgMain2::SetIllumConnectIcon(const BOOL bConnect)
{
	if(bConnect)
	{
		m_IconIllum.SetIcon(m_hiconConnect);
	}
	else
	{
		m_IconIllum.SetIcon(m_hiconDisConnect);
	}
}

void CDlgMain2::SetGrabConnectIcon(const BOOL bGrab, const long index)
{
	if(bGrab)
	{
		m_IconGrab[index].SetIcon(m_hiconConnect);
	}
	else
	{
		m_IconGrab[index].SetIcon(m_hiconDisConnect);
	}
}

void CDlgMain2::SetInspConnectIcon(const BOOL bInspection, const long index)
{
	if(bInspection)
	{
		m_IconInsp[index].SetIcon(m_hiconConnect);
	}
	else
	{
		m_IconInsp[index].SetIcon(m_hiconDisConnect);
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Log 관련
LRESULT CDlgMain2::OnUpdateLog(WPARAM wParam, LPARAM lParam)
{
	::EnterCriticalSection(&m_csLog);
	std::list<CString> pendingLog;
	std::swap(pendingLog, m_pendingLog);
	::LeaveCriticalSection(&m_csLog);

	for(const auto &str : pendingLog)
	{
		m_logBox.InsertString(0, str);

		if(m_logBox.GetCount() > 2000)
		{
			m_logBox.DeleteString(m_logBox.GetCount() - 1);
		}
	}

	return 1L;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Buttons...
void CDlgMain2::OnBnClickedBtnSetup()
{
	m_pParent->ShowSetupDialog();
}

void CDlgMain2::OnBnClickedBtnInspection()
{
	CDlgIndexSetup DlgIndexSetup;
	//if (DlgIndexSetup.DoModal() == IDOK)
	{
		//m_nVisionIndex = DlgIndexSetup.m_nVisionIndex;
		m_nDivIndex = DlgIndexSetup.m_nDivisionIndex;
		InspectionCam1(0, 0);
		::g_DataSaveManager.RawImageSave(TRUE);
		
		/*
		BOOL yoloResult[6];
		yoloResult[0] = ::g_Inspection.SocketYOLO_SingleCall(::g_DataSaveManager.m_strCurrentImageName);
		CString test = ::g_Inspection.m_vecYOLOResultBox[2].class_name;
		float confidence = ::g_Inspection.m_vecYOLOResultBox[2].confidence;
		float x1 = ::g_Inspection.m_vecYOLOResultBox[2].x1;
		float y1 = ::g_Inspection.m_vecYOLOResultBox[2].y1;
		float x2 = ::g_Inspection.m_vecYOLOResultBox[2].x2;
		float y2 = ::g_Inspection.m_vecYOLOResultBox[2].y2;
		*/
		
		InspectionCam2(1, 0);
		::g_DataSaveManager.RawImageSave(TRUE);
		//yoloResult[1] = ::g_Inspection.SocketYOLO(::g_DataSaveManager.m_strCurrentImageName);


		Sleep(100);
		InspectionCam1(0, 1);
		::g_DataSaveManager.RawImageSave(TRUE);
		//yoloResult[2] = ::g_Inspection.SocketYOLO(::g_DataSaveManager.m_strCurrentImageName);

		InspectionCam2(1, 1);
		::g_DataSaveManager.RawImageSave(TRUE);
		//yoloResult[3] = ::g_Inspection.SocketYOLO(::g_DataSaveManager.m_strCurrentImageName);


		Sleep(100);
		InspectionCam1(0, 2);
		::g_DataSaveManager.RawImageSave(TRUE);
		//yoloResult[4] = ::g_Inspection.SocketYOLO(::g_DataSaveManager.m_strCurrentImageName);

		InspectionCam2(1, 2);
		::g_DataSaveManager.RawImageSave(TRUE);
		//yoloResult[5] = ::g_Inspection.SocketYOLO(::g_DataSaveManager.m_strCurrentImageName);

		

		//::g_InspectionManager.m_pInspection->DoInspection(m_nVisionIndex, m_nDivIndex, TRUE, TRUE, TRUE);

		//auto overlayManager = ::g_InspectionManager.m_pInspection->m_pOverlayManager;

		//m_pImageDisplay[m_nVisionIndex]->Overlay_RemoveAll();

		//for(long overlay = DEBUG_RESULT_BEGIN; overlay <= DEBUG_RESULT_END; overlay++)
		//{
		//	overlayManager->ShowDebugResult(m_pImageDisplay[m_nVisionIndex], overlay);
		//}

		//for (long overlay = DEBUG_RESULT; overlay <= DEBUG_RESULT_NG; overlay++)
		//{
		//	overlayManager->ShowDebugResult(m_pImageDisplay[m_nVisionIndex], overlay);
		//}

		//m_pImageDisplay[m_nVisionIndex]->Overlay_Show(TRUE);

		//SetResult(m_nVisionIndex, ::g_InspectionManager.m_pInspection->m_strTotalResult);

		//// 결과 Report Grid 부분 출력
		//UpdateResultGrid(TRUE, m_nVisionIndex, m_nDivIndex, 0);

		//::g_PLCManager.SetResultData_forBorgwarner(::g_InspectionManager.m_pInspection->m_vecResult,::g_InspectionManager.m_pInspection->m_vecCriteriaResult, m_nDivIndex);
	}
	// 250617 sjpark YOLO
	//if (FALSE)
	//{
	//	// Test 라인
	//	BOOL yoloResult = SocketYOLO(::g_DataSaveManager.m_strCurrentImageName);
	//}
}

void CDlgMain2::InspectionCam1(int iCam, int iIdx)
{
	m_nVisionIndex = iCam;
	m_nDivIndex = iIdx;
	::g_Log.Log(_T("CDlgMain2::InspectionCam1( m_nVisionIndex : [%d]  m_nDivIndex : [%d]"), m_nVisionIndex, m_nDivIndex);
	int nDispIndex = m_nVisionIndex + m_nDivIndex*2;

	BOOL bOK = ::g_InspectionManager.m_pInspection->DoInspection(m_nVisionIndex, m_nDivIndex, TRUE, TRUE, TRUE);

	auto overlayManager = ::g_InspectionManager.m_pInspection->m_pOverlayManager;

	m_pImageDisplay[nDispIndex]->Overlay_RemoveAll();

	for (long overlay = DEBUG_RESULT_BEGIN; overlay <= DEBUG_RESULT_END; overlay++)
	{
		overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], overlay);
	}

	for (long overlay = DEBUG_RESULT; overlay <= DEBUG_RESULT_NG; overlay++)
	{
		overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], overlay);
	}

	m_pImageDisplay[nDispIndex]->Overlay_Show(TRUE);
	// SetImageOverlay(m_nVisionIndex, m_nDivIndex);
	if(::g_ConfigManager.m_bManualAutoPass == FALSE)	// Manual로 하는 부분도 적용 - 250228 PSJ
		SetResult(nDispIndex, ::g_InspectionManager.m_pInspection->m_strTotalResult);
	else 
		SetResult(nDispIndex, _T("OK"));

	// 결과 Report Grid 부분 출력
	UpdateResultGrid(TRUE, m_nVisionIndex, m_nDivIndex, 0);

	::g_PLCManager.SetResultData_forBorgwarner(::g_InspectionManager.m_pInspection->m_vecResult, ::g_InspectionManager.m_pInspection->m_vecCriteriaResult, m_nDivIndex);

}
void CDlgMain2::InspectionCam2(int iCam, int iIdx)
{
	m_nVisionIndex = iCam;
	m_nDivIndex = iIdx;
	::g_Log.Log(_T("CDlgMain2::InspectionCam2( m_nVisionIndex : [%d]  m_nDivIndex : [%d]"), m_nVisionIndex, m_nDivIndex);
	int nDispIndex = m_nVisionIndex + m_nDivIndex * 2;

	::g_InspectionManager.m_pInspection->DoInspection(m_nVisionIndex, m_nDivIndex, TRUE, TRUE, TRUE);

	auto overlayManager = ::g_InspectionManager.m_pInspection->m_pOverlayManager;

	m_pImageDisplay[nDispIndex]->Overlay_RemoveAll();

	for (long overlay = DEBUG_RESULT_BEGIN; overlay <= DEBUG_RESULT_END; overlay++)
	{
		overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], overlay);
	}

	for (long overlay = DEBUG_RESULT; overlay <= DEBUG_RESULT_NG; overlay++)
	{
		overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], overlay);
	}

	//for (long overlay = DEBUG_RESULT_RESULT; overlay <= DEBUG_RESULT_END; overlay++)
	//{
	//	overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], overlay);
	//}

	//// 주석 복구 sjpark
	//// 임시 전체 결과 받아서 처리
	//BOOL bResult = TRUE;
	//if (bResult)
	//{
	//	overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], DEBUG_RESULT_OK);
	//}
	//else
	//{
	//	overlayManager->ShowDebugResult(m_pImageDisplay[nDispIndex], DEBUG_RESULT_NG);
	//}

	
	m_pImageDisplay[nDispIndex]->Overlay_Show(TRUE);
	if (::g_ConfigManager.m_bManualAutoPass == FALSE)	// Manual로 하는 부분도 적용 - 250228 PSJ
		SetResult(nDispIndex, ::g_InspectionManager.m_pInspection->m_strTotalResult); // 기존
	else
		SetResult(nDispIndex, _T("OK")); // 강제 OK

	// 결과 Report Grid 부분 출력
	UpdateResultGrid(TRUE, m_nVisionIndex, m_nDivIndex, 0);

	::g_PLCManager.SetResultData_forBorgwarner(::g_InspectionManager.m_pInspection->m_vecResult, ::g_InspectionManager.m_pInspection->m_vecCriteriaResult, m_nDivIndex);

}

void CDlgMain2::OnBnClickedBtnUserLogin()
{
	GetDlgItem(IDC_BTN_SETUP)->EnableWindow();
	/*m_pParent->ShowUserLoginDialog();

	::g_UserManager.SaveUserData();*/
}
///////////////////////////////////////////////////////////////////////////////////////////////////

void CDlgMain2::OnBnClickedButton1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	::g_PLCSocketManager.MessageProcessSimul(0, 0, 0, true);//(int nCameraIndex, int nDivIndex, int nJobNumber, bool bScanDone)
	
	//CString strPath1 = L"E:\\01.WorkSpace\\08.HubisVIS_debug\\HubisVIS\\x64\\Debug\\Recipe\\BDC.bmp";
	//CString strPath2 = L"E:\\01.WorkSpace\\08.HubisVIS_debug\\HubisVIS\\x64\\Debug\\Recipe\\BAC.bmp";

	////m_pParent->ImageOpen();

	//m_pParent->ImageOpen(strPath1);
	//m_pParent->ImageOpen(strPath2, 1);
	InspectionCam1(0, 0);
	InspectionCam2(1, 0);
}

void CDlgMain2::OnBnClickedButton2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	::g_PLCSocketManager.MessageProcessSimul(0, 0, 0, true);
	InspectionCam1(0, 1);
	InspectionCam2(1, 1);
}

void CDlgMain2::OnBnClickedButton3()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	::g_PLCSocketManager.MessageProcessSimul(0, 0, 0, true);
	InspectionCam1(0, 2);
	InspectionCam2(1, 2);	
}

void CDlgMain2::OnBnClickedBtnTestBatch()
{
	InspectionCam1(0, 0);
	InspectionCam2(1, 0);

	InspectionCam1(0, 1);
	InspectionCam1(1, 1);

	InspectionCam1(0, 2);
	InspectionCam1(1, 2);
	
}

void CDlgMain2::ReadYOLOSharedMemory()
{
	// 1. 추론할 이미지 경로 지정
	CString imagePath = _T("D:\\YOLOv8\\1_0_After.bmp");  // 절대경로로 지정
	std::string imgPathStr = CT2A(imagePath);  // CString → std::string 변환

	// 2. 공유 메모리 이름과 크기 설정
	//const char* shmName = "yolo_shm";  // Python과 정확히 일치해야 함
	LPCWSTR shmName = L"yolo_shm";  // 유니코드 문자열
	const int shmSize = 65536;

	// 3. 공유 메모리 열기 (Python에서 먼저 생성됨)
	HANDLE hMapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, shmName);
	if (hMapFile == NULL)
	{
		AfxMessageBox(_T("공유 메모리 열기 실패 (yolo_infer.exe 먼저 실행되어 있어야 함)"));
		return;
	}

	// 4. 공유 메모리 맵핑
	LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, shmSize);
	if (pBuf == NULL)
	{
		CloseHandle(hMapFile);
		AfxMessageBox(_T("Memory Map Open Failed"));
		return;
	}

	// 5. 공유 메모리에 이미지 경로 쓰기
	ZeroMemory(pBuf, shmSize);  // 이전 내용 제거
	memcpy(pBuf, imgPathStr.c_str(), imgPathStr.size());  // 경로 쓰기

	// 6. 정리
	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);

	AfxMessageBox(_T("YOLO 추론 요청을 전송했습니다."));

}

#include <Ws2tcpip.h>
bool CDlgMain2::InitSocketClient()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;

	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sock == INVALID_SOCKET)
		return false;
	DWORD dwSocketTimeoutMs = 3000;
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwSocketTimeoutMs, sizeof(dwSocketTimeoutMs));
	setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwSocketTimeoutMs, sizeof(dwSocketTimeoutMs));

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

	if (connect(m_sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return false;

	m_connected = true;
	return true;
}
void CDlgMain2::CleanupSocket()
{
	if (m_sock != INVALID_SOCKET)
	{
		closesocket(m_sock);
		WSACleanup();
		m_sock = INVALID_SOCKET;
		m_connected = false;
	}
}

// YOLO 테스트용 임시코드
BOOL CDlgMain2::SocketYOLO(const CString& imagePath)
{
	m_vecYOLOResultBox.clear(); // YOLO 결과 초기화

	if (!m_connected || m_sock == INVALID_SOCKET)
	{
		AfxMessageBox(_T("Socket Not Connected."));
		return FALSE;
	}

	// 1. CString → std::string → char*
	std::string imgPathStr = CT2A(imagePath);
	const char* msg = imgPathStr.c_str();

	// 2. 이미지 경로 전송
	int sent = send(m_sock, msg, (int)strlen(msg), 0);
	if (sent == SOCKET_ERROR)
	{
		AfxMessageBox(_T("Socket Send Failed."));
		return FALSE;
	}

	// 3. 서버 응답 수신 (끝에 '\n'까지 반복 수신)
	std::string jsonStr;
	char buffer[1024];
	int bytesReceived = 0;

	while (true)
	{
		bytesReceived = recv(m_sock, buffer, sizeof(buffer) - 1, 0);
		if (bytesReceived <= 0)
		{
			AfxMessageBox(_T("서버 응답 수신 실패"));
			return FALSE;
		}
		buffer[bytesReceived] = '\0';
		jsonStr += buffer;

		// \n 도착 시 루프 종료
		if (jsonStr.find('\n') != std::string::npos)
			break;
	}

	// '\n' 제거
	size_t newlinePos = jsonStr.find('\n');
	if (newlinePos != std::string::npos)
		jsonStr = jsonStr.substr(0, newlinePos);

	// CString 변환 후 파싱
	CStringA ansiStr(jsonStr.c_str());
	CString resultYOLO(ansiStr);
	ParseYOLOResult(resultYOLO);

	return TRUE;
}

std::vector<YOLOBox> CDlgMain2::ParseYOLOResult(const CString & jsonStr)
{
	CStringA ansiStr(jsonStr);  // CString → ANSI 문자열
	std::string input = ansiStr;

	size_t pos = 0;
	while ((pos = input.find("\"class\"")) != std::string::npos)
	{
		YOLOBox box;

		// class
		pos = input.find("\"class\"");
		size_t start = input.find("\"", pos + 7) + 1;
		size_t end = input.find("\"", start);
		box.class_name = CString(input.substr(start, end - start).c_str());

		// confidence
		pos = input.find("\"confidence\"", end);
		start = input.find(":", pos) + 1;
		end = input.find(",", start);
		box.confidence = static_cast<float>(atof(input.substr(start, end - start).c_str()));

		// box [x1, y1, x2, y2]
		pos = input.find("[", end) + 1;
		end = input.find(",", pos);
		box.x1 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

		pos = end + 1;
		end = input.find(",", pos);
		box.y1 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

		pos = end + 1;
		end = input.find(",", pos);
		box.x2 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

		pos = end + 1;
		end = input.find("]", pos);
		box.y2 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

		m_vecYOLOResultBox.push_back(box);

		// 다음 항목으로 이동
		input = input.substr(end);
	}

	for (const auto& boxInstance : m_vecYOLOResultBox)
	{
		CString debug;
		debug.Format(_T("Class: %s, Conf: %.2f, Box: %.1f, %.1f, %.1f, %.1f"),
			boxInstance.class_name, boxInstance.confidence, boxInstance.x1, boxInstance.y1, boxInstance.x2, boxInstance.y2);
		AfxMessageBox(debug);
	}

	return m_vecYOLOResultBox;
}
