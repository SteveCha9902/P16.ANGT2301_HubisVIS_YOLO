// DlgMain1.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgVisionMain.h"
#include "DlgMain1.h"
#include "afxdialogex.h"

#include "DlgIndexSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgMain1 대화 상자

IMPLEMENT_DYNAMIC(CDlgMain1, CDialogEx)

CDlgMain1::CDlgMain1(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_MAIN1, pParent)
	, m_pParent(pParent)
	, m_pImageDisplay(nullptr)
	, m_pCtrlCamera(nullptr)
	, m_bConnectCamera(FALSE)
	, m_bConnectSerial(FALSE)
	, m_bResult(FALSE)
	, m_nImageWidth(0)
	, m_nImageHeight(0)
	, m_nImageSize(0)
	, m_nVisionIndex(0)
	, m_nDivIndex(0)
{

}

CDlgMain1::~CDlgMain1()
{
	::DeleteCriticalSection(&m_csLog);

	delete m_pImageDisplay;

	if(m_pCtrlCamera != nullptr)
	{
		delete m_pCtrlCamera;
		m_pCtrlCamera = nullptr;
	}
}

void CDlgMain1::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MAIN_LOG, m_logBox);
	DDX_Control(pDX, IDC_STATIC_CTRL, m_IconCtrl);
	DDX_Control(pDX, IDC_STATIC_LASER, m_IconLaser);
	DDX_Control(pDX, IDC_STATIC_CAM, m_IconCam);
	DDX_Control(pDX, IDC_STATIC_ILLUM, m_IconIllum);
	DDX_Control(pDX, IDC_STATIC_GRAB, m_IconGrab);
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
	DDX_Control(pDX, IDC_STATIC_RESULT, m_lblResult);
	DDX_Control(pDX, IDC_STATIC_USER, m_lblUser);
}


BEGIN_MESSAGE_MAP(CDlgMain1, CDialogEx)
	ON_MESSAGE(UM_UPDATE_LOG, &CDlgMain1::OnUpdateLog)
	ON_BN_CLICKED(IDC_BTN_SETUP, &CDlgMain1::OnBnClickedBtnSetup)
	ON_BN_CLICKED(IDC_BTN_INSPECTION, &CDlgMain1::OnBnClickedBtnInspection)
	ON_BN_CLICKED(IDC_BTN_USER_LOGIN, &CDlgMain1::OnBnClickedBtnUserLogin)
END_MESSAGE_MAP()


// CDlgMain1 메시지 처리기

BOOL CDlgMain1::OnInitDialog()
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

	// 2D Viewer
	m_pImageDisplay = new CImageDisplay;
	m_pImageDisplay->Initialize(IDC_STATIC_IMAGE_VIEWER, this, RGB(50, 50, 50));
	m_pImageDisplay->SetCrossLineColor(RGB(255, 0, 0));
	m_pImageDisplay->SetCustomLineColor(RGB(255, 0, 0));

	// Hardware 연결...
	if(::g_ConfigManager.m_bHardwareExist == TRUE)
	{
		m_pCtrlCamera = new CCtrlCamera; // Camera 생성 - SHK 2024.01.02

		// 카메라 연결...
		if(m_pCtrlCamera->Open() == TRUE)
		{
			m_nImageWidth = m_pCtrlCamera->GetImageWidth();
			m_nImageHeight = m_pCtrlCamera->GetImageHeight();
			m_nImageSize = m_nImageWidth * m_nImageHeight;

			SetCamConnectIcon(TRUE);
		}
		else
		{
			::AfxMessageBox(_T("Camera Open Failed..."));

			return FALSE;
		}

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

		if(m_bConnectSerial == FALSE) // 연결 실패...
		{
			::AfxMessageBox(_T("Serial Connect Failed..."));

			return FALSE;
		}

		SetIllumConnectIcon(TRUE);
	}
	else
	{
		m_nImageWidth = ::g_ConfigManager.m_nImageWidth[0];
		m_nImageHeight = ::g_ConfigManager.m_nImageHeight[0];
		m_nImageSize = m_nImageWidth * m_nImageHeight;
	}

	//// Recipe open...
	//m_pParent->m_strCurRecipePath = ::g_ConfigManager.m_strPrevRecipeFilePath;
	//m_pParent->m_strCurRecipeName = REMOVE_FOLDER_FROM_PATHNAME(m_pParent->m_strCurRecipePath);
	//if(m_pParent->m_strCurRecipeName != _T(""))
	//{
	//	m_pParent->LoadRecipe(m_pParent->m_strCurRecipePath);
	//}

	// 메모리 생성...
	::g_MemoryManager.SetImageSize(m_nImageWidth, m_nImageHeight);

	SetResult(0, _T("NOT MEASURED"));
	SetUser(::g_UserManager.m_nPreLoginUserIndex);

	::g_Log.Log(_T("Hubis Vision Inspection System Started!"));

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

BOOL CDlgMain1::PreTranslateMessage(MSG* pMsg)
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

void CDlgMain1::SetImage()
{
	m_pImageDisplay->Overlay_RemoveAll();

	BYTE* image = ::g_MemoryManager.GetByteImage();

	m_pImageDisplay->SetImage(image, m_nImageWidth, m_nImageHeight, m_nImageWidth);

	m_pImageDisplay->Overlay_Show(TRUE);
}

void CDlgMain1::SetImageOverlay(const long inspIndex)
{
	m_pImageDisplay->Overlay_RemoveAll();

	auto overlayManager = ::g_InspectionManager.m_vecpInspection[inspIndex]->m_pOverlayManager;
	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[inspIndex].algoSequence;
	
	for (long overlay = DEBUG_RESULT; overlay <= DEBUG_RESULT_NG; overlay++)
	{
		overlayManager->ShowDebugResult(m_pImageDisplay, overlay);
	}

	m_pImageDisplay->Overlay_Show(TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Real Time 관련...
void CDlgMain1::SetRealTime(const CString strTime)
{
	SetDlgItemText(IDC_STATIC_TIME, strTime);
}
///////////////////////////////////////////////////////////////////////////////////////////////////

void CDlgMain1::SetResult(const long camIndex, const CString strResult)
{
	CString strInspResult;

	if(strResult != _T(""))
	{
		if(m_bResult)
		{
			strInspResult = _T("/") + strResult;
		}
		else
		{
			strInspResult = strResult + _T("/");
		}
	}

	m_lblResult.SetText(strInspResult);
	m_lblResult.SetFontSize(40);
	m_lblResult.SetFontBold(TRUE);
	m_lblResult.SetTextColor(RGB(255, 0, 0));
	m_lblResult.SetBorder(TRUE);

	m_bResult = !m_bResult;
}

void CDlgMain1::SetUser(const long index)
{
	CString strUser;

	strUser = std::get<1>(::g_UserManager.m_vecUser[index]);
	m_lblUser.SetText(strUser);
	m_lblUser.SetFontSize(25);
	m_lblUser.SetFontBold(TRUE);
	m_lblUser.SetTextColor(RGB(0, 0, 255));
	m_lblUser.SetBorder(TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Icon 관련...
void CDlgMain1::SetCtrlConnectIcon(const BOOL bConnect)
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

void CDlgMain1::SetLaserConnectIcon(const BOOL bConnect)
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

void CDlgMain1::SetCamConnectIcon(const BOOL bConnect)
{
	if(bConnect)
	{
		m_IconCam.SetIcon(m_hiconConnect);
	}
	else
	{
		m_IconCam.SetIcon(m_hiconDisConnect);
	}
}

void CDlgMain1::SetIllumConnectIcon(const BOOL bConnect)
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

void CDlgMain1::SetGrabConnectIcon(const BOOL bGrab)
{
	if(bGrab)
	{
		m_IconGrab.SetIcon(m_hiconConnect);
	}
	else
	{
		m_IconGrab.SetIcon(m_hiconDisConnect);
	}
}

void CDlgMain1::SetInspConnectIcon(const BOOL bInspection, const long index)
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
// Log 관련...
LRESULT CDlgMain1::OnUpdateLog(WPARAM wParam, LPARAM lParam)
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
void CDlgMain1::OnBnClickedBtnSetup()
{
	m_pParent->ShowSetupDialog();
}

void CDlgMain1::OnBnClickedBtnInspection()
{
	CDlgIndexSetup DlgIndexSetup;
	if (DlgIndexSetup.DoModal() == IDOK)
	{
		m_nVisionIndex = DlgIndexSetup.m_nVisionIndex;
		m_nDivIndex = DlgIndexSetup.m_nDivisionIndex;

		::g_InspectionManager.m_pInspection->DoInspection(m_nVisionIndex, m_nDivIndex, TRUE, TRUE, TRUE);

		auto overlayManager = ::g_InspectionManager.m_pInspection->m_pOverlayManager;

		m_pImageDisplay->Overlay_RemoveAll();

		for (long overlay = DEBUG_RESULT; overlay <= DEBUG_RESULT_NG; overlay++)
		{
			overlayManager->ShowDebugResult(m_pImageDisplay, overlay);
		}

		m_pImageDisplay->Overlay_Show(TRUE);

		SetResult(m_nVisionIndex, ::g_InspectionManager.m_pInspection->m_strTotalResult);
		::g_Log.Log(_T("m_nVisionIndex : %d  // Result"), m_nVisionIndex, ::g_InspectionManager.m_pInspection->m_strTotalResult);

	}
}

void CDlgMain1::OnBnClickedBtnUserLogin()
{
	m_pParent->ShowUserLoginDialog();

	::g_UserManager.SaveUserData();
}
///////////////////////////////////////////////////////////////////////////////////////////////////

