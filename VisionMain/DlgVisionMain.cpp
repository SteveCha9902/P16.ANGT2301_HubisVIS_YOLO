// DlgVisionMain.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgVisionMain.h"
#include "afxdialogex.h"

#include "DlgLog.h"
#include "DlgMain1.h"
#include "DlgMain2.h"
#include "DlgBatchInspection.h"
#include "DlgInspItemSetup.h"
#include "DlgSetup.h"
#include "DlgCalibration.h"
#include "DlgInspItemMatch.h"
#include "DlgUser.h"
#include "DlgRecipeResist.h"
#include "DlgPLCMonitering.h"
#include "DlgLogin2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgVisionMain 대화 상자

IMPLEMENT_DYNAMIC(CDlgVisionMain, CDialogEx)

CDlgVisionMain::CDlgVisionMain(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_VISION_MAIN, pParent)
	, m_pDlgLog(nullptr)
	, m_pDlgMain1(nullptr)
	, m_pDlgMain2(nullptr)
	, m_pDlgPLCMonitering(nullptr)
	, m_nImageWidth(0)
	, m_nImageHeight(0)
	, m_nImageSize(0)
	, m_strCurPath(_T(""))
	, m_strCurRecipePath(_T(""))
	, m_strCurRecipeName(_T(""))
	, m_strMessage(_T(""))
	, m_bMessageDisplaying(FALSE)
	, m_bLinkTest(FALSE)
	, m_bReceiveRecipeOpen(FALSE)
{	

}

CDlgVisionMain::~CDlgVisionMain()
{
	delete m_pDlgLog;
	delete m_pDlgMain1;
	delete m_pDlgMain2;
	delete m_pDlgPLCMonitering;
}

void CDlgVisionMain::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgVisionMain, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_MESSAGE(UM_UPDATE_IMAGE, &CDlgVisionMain::OnUpdateImage)
	ON_MESSAGE(UM_UPDATE_RESULT, &CDlgVisionMain::OnUpdateResult)
	ON_MESSAGE(UM_UPDATE_GRAB_STATUS, &CDlgVisionMain::OnUpdateGrabStatus)
	ON_MESSAGE(UM_UPDATE_INSP_STATUS, &CDlgVisionMain::OnUpdateInspStatus)
	ON_MESSAGE(UM_UPDATE_LASER_STATUS, &CDlgVisionMain::OnUpdateLaserStatus)
	ON_MESSAGE(UM_UPDATE_CTRL_STATUS, &CDlgVisionMain::OnUpdateCtrlStatus)
	ON_MESSAGE(UM_UPDATE_RECIPE_UPDATE, &CDlgVisionMain::OnUpdateRecipeUpdate)
	ON_COMMAND(ID_FILE_MENU1_NEWRECIPE, &CDlgVisionMain::OnFileNewrecipe)
	ON_COMMAND(ID_FILE_MENU2_NEWRECIPE, &CDlgVisionMain::OnFileNewrecipe)
	ON_COMMAND(ID_FILE_MENU1_RECIPEOPEN, &CDlgVisionMain::OnFileRecipeopen)
	ON_COMMAND(ID_FILE_MENU2_RECIPEOPEN, &CDlgVisionMain::OnFileRecipeopen)
	ON_COMMAND(ID_FILE_MENU1_RECIPESAVE, &CDlgVisionMain::OnFileRecipesave)
	ON_COMMAND(ID_FILE_MENU2_RECIPESAVE, &CDlgVisionMain::OnFileRecipesave)
	ON_COMMAND(ID_FILE_MENU1_RECIPERESIST, &CDlgVisionMain::OnFileReciperesist)
	ON_COMMAND(ID_FILE_MENU2_RECIPERESIST, &CDlgVisionMain::OnFileReciperesist)
	ON_COMMAND(ID_IMAGE_MENU1_IMAGEOPEN, &CDlgVisionMain::OnImageMenu1Imageopen)
	ON_COMMAND(ID_IMAGE_MENU2_IMAGEOPEN1, &CDlgVisionMain::OnImageMenu1Imageopen)
	ON_COMMAND(ID_IMAGE_MENU1_IMAGESAVE, &CDlgVisionMain::OnImageMenu1Imagesave)
	ON_COMMAND(ID_IMAGE_MENU2_IMAGESAVE1, &CDlgVisionMain::OnImageMenu1Imagesave)
	ON_COMMAND(ID_IMAGE_MENU2_IMAGEOPEN2, &CDlgVisionMain::OnImageMenu2Imageopen2)
	ON_COMMAND(ID_IMAGE_MENU2_IMAGESAVE2, &CDlgVisionMain::OnImageMenu2Imagesave2)
	ON_COMMAND(ID_LOG_MENU1_SHOWLOG, &CDlgVisionMain::OnLogShowlog)
	ON_COMMAND(ID_LOG_MENU2_SHOWLOG, &CDlgVisionMain::OnLogShowlog)
	ON_COMMAND(ID_INSPECTION_MENU1_INSPECTIONITEMSETUP, &CDlgVisionMain::OnInspectionInspectionitemsetup)
	ON_COMMAND(ID_INSPECTION_MENU2_INSPECTIONITEMSETUP, &CDlgVisionMain::OnInspectionInspectionitemsetup)
	ON_COMMAND(ID_INSPECTION_MENU1_BATCHINSPECTION, &CDlgVisionMain::OnInspectionBatchinspection)
	ON_COMMAND(ID_INSPECTION_MENU2_BATCHINSPECTION, &CDlgVisionMain::OnInspectionBatchinspection)
	ON_COMMAND(ID_CALIBRATION_MENU2_CALIBRATION, &CDlgVisionMain::OnCalibrationCalibration)
	ON_COMMAND(ID_CALIBRATION_MENU1_CALIBRATION, &CDlgVisionMain::OnCalibrationCalibration)
	ON_COMMAND(ID_INSPECTION_MENU1_INSPECTIONITEMMATCH, &CDlgVisionMain::OnInspectionInspectionitemmatch)
	ON_COMMAND(ID_INSPECTION_MENU2_INSPECTIONITEMMATCH, &CDlgVisionMain::OnInspectionInspectionitemmatch)
	ON_COMMAND(ID_COMMUNICATION_MENU1_PLCMONITERING, &CDlgVisionMain::OnCommunicationPlcmonitering)
	ON_COMMAND(ID_COMMUNICATION_MENU2_PLCMONITERING, &CDlgVisionMain::OnCommunicationPlcmonitering)
END_MESSAGE_MAP()


// CDlgVisionMain 메시지 처리기

BOOL CDlgVisionMain::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString strTemp;

	// 현재 폴더 가져오기...
	CString strCurPath, strConfig, strRecipe;
	GetModuleFileName(nullptr, strCurPath.GetBuffer(_MAX_PATH + 1), _MAX_PATH);
	strCurPath.ReleaseBuffer();
	m_strCurPath = strCurPath.Left(strCurPath.ReverseFind(_T('\\')));
	strConfig.Format(_T("%s\\Config"), m_strCurPath);
	CreateDirectory(strConfig, nullptr);
	strRecipe.Format(_T("%s\\Recipe"), m_strCurPath);
	CreateDirectory(strRecipe, nullptr);

	// Log 창 생성
	m_pDlgLog = new CDlgLog(this);
	m_pDlgLog->Create(CDlgLog::IDD, this);
	m_pDlgLog->ShowWindow(SW_HIDE);

	::g_Log.SetCurPath(m_strCurPath);
	::g_Log.SetParent(this);
	::g_Log.OpenDialog(m_pDlgLog->GetSafeHwnd());

	// Config 초기화
	::g_ConfigManager.SetCurPath(m_strCurPath);
	::g_ConfigManager.InitParameters();
	::g_ConfigManager.LoadConfig();
	::g_ConfigManager.SaveConfig();
	CreateDirectory(::g_ConfigManager.m_strRawImageSavePath, nullptr);
	CreateDirectory(::g_ConfigManager.m_strResultImageSavePath, nullptr);
	CreateDirectory(::g_ConfigManager.m_strResultDataSavePath, nullptr);

	// Calibration 초기화
	::g_CalibrationManager.SetCurPath(m_strCurPath);
	::g_CalibrationManager.InitParameters();
	::g_CalibrationManager.LoadCalibration();
	::g_CalibrationManager.SaveCalibration();

	::g_UserManager.InitParameters();
	::g_UserManager.LoadUserData();
	::g_UserManager.SaveUserData();

	::g_PLCManager.SetCurPath(m_strCurPath);
	::g_PLCManager.InitParameters();
	if(::g_PLCManager.LoadPLCMap(::g_PLCManager.m_strPath) == FALSE)
	{
		::g_PLCManager.SavePLCMap();
	}

	// Crash dump
	static HUBSDK::CrashDump g_CrashDump(m_strCurPath + _T("\\MiniDump.dmp"), CDlgVisionMain::UserExceptionHandlerPre, CDlgVisionMain::UserExceptionHandlerPost);

	// Thread 생성
	::g_PLCSocketManager.SetParent(this);

	// Main UI 설정에 따라...
	CMenu menu;
	CRect rtMainRect;
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_pDlgMain1 = new CDlgMain1(this);
		m_pDlgMain1->Create(CDlgMain1::IDD, this);
		m_pDlgMain1->ShowWindow(SW_SHOW);
		m_pDlgMain1->GetClientRect(&rtMainRect);

		m_nImageWidth = m_pDlgMain1->m_nImageWidth;
		m_nImageHeight = m_pDlgMain1->m_nImageHeight;
		m_nImageSize = m_pDlgMain1->m_nImageSize;

		menu.LoadMenu(IDR_MENU1);
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_pDlgMain2 = new CDlgMain2(this);
		m_pDlgMain2->Create(CDlgMain2::IDD, this);
		m_pDlgMain2->ShowWindow(SW_SHOW);
		m_pDlgMain2->GetClientRect(&rtMainRect);

		m_nImageWidth = m_pDlgMain2->m_nImageWidth[0] > m_pDlgMain2->m_nImageWidth[1] ? m_pDlgMain2->m_nImageWidth[0] : m_pDlgMain2->m_nImageWidth[1];
		m_nImageHeight = m_pDlgMain2->m_nImageHeight[0] > m_pDlgMain2->m_nImageHeight[1] ? m_pDlgMain2->m_nImageHeight[0] : m_pDlgMain2->m_nImageHeight[1];
		m_nImageSize = m_nImageWidth * m_nImageHeight;

		menu.LoadMenu(IDR_MENU2);
		break;
	}

	// 최근 Recipe Open...
	::g_RecipeManager.InitParameters();
	m_strCurRecipePath = ::g_ConfigManager.m_strPrevRecipeFilePath;
	m_strCurRecipeName = REMOVE_FOLDER_FROM_PATHNAME(m_strCurRecipePath);

	if(LoadRecipe(m_strCurRecipePath) == FALSE)
	{
		strRecipe += _T("\\Default.recipe"); 	// Recipe 경로 수정 - SHK 2023.12.21
		SaveRecipe(strRecipe);
		LoadRecipe(strRecipe);
	}

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_pDlgMain2->SetResultGrid();
		break;
	}

	SetMenu(&menu);

	// CDlgVisionMain 의 Dialog 크기를 Main UI 에 맞게 변경하기 위해...
	CRect rtWndRect, rtClientRect;
	GetWindowRect(&rtWndRect);
	GetClientRect(&rtClientRect);
	long cx = rtWndRect.Width() - rtClientRect.Width();
	long cy = rtWndRect.Height() - rtClientRect.Height();

	SetWindowPos(NULL, rtMainRect.left, rtMainRect.top, rtMainRect.Width(), rtMainRect.Height(), SWP_NOMOVE);
	//SetWindowPos(NULL, rtMainRect.left, rtMainRect.top, rtMainRect.Width() + cx, rtMainRect.Height() + cy, SWP_NOMOVE);

	// Thread 생성
	::g_ScanManager.SetParent(this);
	::g_ScanManager.InitScanManageThread();
	::g_ScanManager.InitScanThread();
	::g_InspectionManager.SetParent(this);
	::g_InspectionManager.InitInspectionThread();
	::g_FileManager.InitThread();
	::g_DataSaveManager.InitThread();
	::g_DataSaveManager.SetImageSize(m_nImageWidth, m_nImageHeight);

	long nCam = 0;
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		nCam = 1; break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		nCam = 2; break;
	}

	::g_SocketManager.clear();
	long nSocketSize = ::g_ConfigManager.m_nLaserTCPIPNum;
	if(::g_ConfigManager.m_bUseLaserTCPIP == TRUE)
	{
		::g_SocketManager.resize(nSocketSize);
		for(int idx = 0; idx < nSocketSize; idx++)
		{
			CString ip = ::g_ConfigManager.m_strLaserIP[idx];
			long port = ::g_ConfigManager.m_nLaserPort[idx];

			::g_SocketManager[idx].SetParent(this);
			::g_SocketManager[idx].SetLaserSocketInfo(ip, port, idx);
		}
	}

	// 60초에 한번씩 Folder Delete 확인
	if(::g_ConfigManager.m_bUseDataAutoDelete == TRUE)
	{
		SetTimer(FOLDER_DELETE, ::g_ConfigManager.m_nAutoDeleteTimes, NULL);
	}

	// Laser S/W 접속...
	if(::g_ConfigManager.m_bUseLaserTCPIP == TRUE)
	{
		// TCP/IP Thread 생성...
		for(int i = 0; i < nSocketSize; i++)
		{
			::g_SocketManager[i].InitThread();
		}
		SetTimer(LASER_CONNECT, ::g_ConfigManager.m_nLaserAutoConnectTimes, NULL);
	}

	// PLC 접속...
	if(::g_ConfigManager.m_bUsePLCTCPIP == TRUE)
	{
		// TCP/IP Thread 생성...
		::g_PLCSocketManager.InitThread();

		if(::g_ConfigManager.m_bUsePLCTCPIP == TRUE &&
			::g_PLCSocketManager.m_bTryingServerConnect == FALSE) // 접속 시도 중일때는 다시 접속시도하지 말자.
		{
			// 접속 시작
			::SetEvent(::g_PLCSocketManager.m_Control_Client.m_hSignal_Start);
		}

		SetTimer(PLC_LINK, 500, NULL);
	}

	// PLC Monitering UI 생성...
	m_pDlgPLCMonitering = new CDlgPLCMonitering(this);
	m_pDlgPLCMonitering->Create(CDlgPLCMonitering::IDD, this);
	m_pDlgPLCMonitering->ShowWindow(SW_HIDE);

	// 실시간 시계 표시
	SetTimer(REAL_TIME, 1000, NULL);

	// 프로그램 버전 표시
	SetVersion(m_strCurRecipeName);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

BOOL CDlgVisionMain::PreTranslateMessage(MSG* pMsg)
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

void CDlgVisionMain::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
	case FOLDER_DELETE:
	{
		::SetEvent(::g_FileManager.m_Control_File_Delete.m_hSignal_Start);
	}
	break;
	case CTRL_CONNECT: // PLC 와 연결
	{
		if(::g_ConfigManager.m_bUsePLCTCPIP == TRUE &&
			::g_PLCSocketManager.m_bTryingServerConnect == FALSE) // 접속 시도 중일때는 다시 접속시도하지 말자.
		{
			// 접속 시작
			::SetEvent(::g_PLCSocketManager.m_Control_Client.m_hSignal_Start);
		}
	}
	break;
	case LASER_CONNECT: // Laser S/W 와 연결
	{
		long nSocketSize = ::g_ConfigManager.m_nLaserTCPIPNum;
		for(int i = 0; i < nSocketSize; i++)
		{
			if(::g_ConfigManager.m_bUseLaserTCPIP == TRUE &&
				::g_SocketManager[i].m_bTryingServerConnect == FALSE) // 접속 시도 중일때는 다시 접속시도하지 말자.
			{
				// 접속 시작
				::SetEvent(::g_SocketManager[i].m_Control_Client.m_hSignal_Start);
			}
		}
	}
	break;
	case PLC_MOINTERING:
	{
		if(m_pDlgPLCMonitering->IsWindowVisible() == FALSE)
		{
			m_pDlgPLCMonitering->EndRepeatReadData();

			KillTimer(PLC_MOINTERING);
		}
	}
	break;
	case PLC_LINK:
	{
		if(::g_PLCSocketManager.IsConnect() == TRUE)
		{
			::g_PLCSocketManager.RequestLinkTestWrite(m_bLinkTest == FALSE);

			m_bLinkTest = !m_bLinkTest;
		}
	}
	break;
	case REAL_TIME:
	{
		CTime time;
		time = CTime::GetCurrentTime();

		CString strTime = time.Format(_T("%Y-%m-%d %H:%M:%S"));

		switch(::g_ConfigManager.m_nMainUI)
		{
		case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
			m_pDlgMain1->SetRealTime(strTime);
			break;
		case (long)MAINUI::UI_FOR_DUAL_CAMERA:
			m_pDlgMain2->SetRealTime(strTime);
			break;
		}
	}
	break;
	}
}

void CDlgVisionMain::OnClose()
{
	int ret = ::AfxMessageBox(_T("Are you sure you want to quit?"), MB_YESNO);

	if(ret == IDYES)
	{

		if(::g_ConfigManager.m_bUseLaserTCPIP == TRUE)
		{
			long nSocketSize = ::g_ConfigManager.m_nLaserTCPIPNum;
			for(int i = 0; i < nSocketSize; i++)
			{
				::g_SocketManager[i].FreeThread();
			}
		}

		if(::g_ConfigManager.m_bUsePLCTCPIP == TRUE)
		{
			::g_PLCSocketManager.FreeThread();
		}
		KillTimer(PLC_LINK);
		KillTimer(CTRL_CONNECT);
		KillTimer(LASER_CONNECT);

		::g_ScanManager.FreeScanManageThread();
		::g_ScanManager.FreeScanThread();
		::g_InspectionManager.FreeInspectionThread();
		::g_FileManager.FreeThread();
		::g_DataSaveManager.FreeThread();

		::g_Log.Log(_T("Program Terminated!"));
	}
	else
	{
		return;
	}

	CDialog::OnClose();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Menu...
void CDlgVisionMain::OnFileNewrecipe()
{
	BOOL bOk(FALSE);

	CFileDialog Dlg(FALSE, _T("recipe"), m_strCurRecipeName, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Recipe File (*.recipe)|*.recipe||"), NULL, NULL, FALSE);

	if(Dlg.DoModal() == IDOK)
	{
		bOk = TRUE;
	}

	if(bOk == TRUE)
	{
		m_strCurRecipePath = Dlg.GetPathName();
		m_strCurRecipeName = Dlg.GetFileName();

		::g_RecipeManager.InitParameters();
		bOk = SaveRecipe(m_strCurRecipePath);
		bOk = LoadRecipe(m_strCurRecipePath);

		if(bOk == TRUE)
		{
			::AfxMessageBox(_T("New Recipe Save Complete!"), MB_ICONINFORMATION);
		}
		else
		{
			::AfxMessageBox(_T("New Recipe Save Failed!"), MB_ICONERROR);
		}
	}
}


void CDlgVisionMain::OnFileRecipeopen()
{
	BOOL bOk(FALSE);

	CFileDialog Dlg(TRUE, _T("recipe"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Recipe File (*.recipe)|*.recipe||"), NULL, NULL, FALSE);
	//CString RecipePath;
	//RecipePath.Format(_T("%s\\Recipe"), m_strCurPath);
	//Dlg.m_ofn.lpstrInitialDir = RecipePath;
	Dlg.m_ofn.lpstrInitialDir = _T("D:\\HubisVIS\\Recipe");
	if(Dlg.DoModal() == IDOK)
	{
		bOk = TRUE;
	}

	if(bOk == TRUE)
	{
		m_strCurRecipePath = Dlg.GetPathName();
		m_strCurRecipeName = Dlg.GetFileName();

		bOk = LoadRecipe(m_strCurRecipePath);

		if(bOk == FALSE)
		{
			return;
		}
	}	
}


void CDlgVisionMain::OnFileRecipesave()
{
	BOOL bOk(FALSE);

	CFileDialog Dlg(FALSE, _T("recipe"), m_strCurRecipeName, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Recipe File (*.recipe)|*.recipe||"), NULL, NULL, FALSE);
	//CString RecipePath;
	//RecipePath.Format(_T("%s\\Recipe"), m_strCurPath);
	//Dlg.m_ofn.lpstrInitialDir = RecipePath;//LYS 240902
	Dlg.m_ofn.lpstrInitialDir = _T("D:\\HubisVIS\\Recipe");
	if(Dlg.DoModal() == IDOK)
	{
		bOk = TRUE;
	}

	if(bOk == TRUE)
	{
		m_strCurRecipePath = Dlg.GetPathName();
		m_strCurRecipeName = Dlg.GetFileName();

		bOk = SaveRecipe(m_strCurRecipePath);
		bOk = LoadRecipe(m_strCurRecipePath);

		if(bOk == TRUE)
		{
			::AfxMessageBox(_T("Save Complete!"), MB_ICONINFORMATION);
		}
		else
		{
			::AfxMessageBox(_T("Save Failed!"), MB_ICONERROR);
		}

		::g_Log.Log(_T("============================================================================================================"), m_strCurRecipeName);
		::g_Log.Log(_T("Save Recipe====> 레시피 저장경로 : %s <======"), m_strCurRecipePath);
		::g_Log.Log(_T("Save Recipe====> 현재 열고 있는 레시피 이름 : %s <======"), m_strCurRecipeName);
		::g_Log.Log(_T("============================================================================================================"), m_strCurRecipeName);
	}
}

void CDlgVisionMain::OnFileReciperesist()
{
	CDlgRecipeResist dlg(this);
	dlg.DoModal();
}


void CDlgVisionMain::OnImageMenu1Imageopen()
{
	CFileDialog Dlg(TRUE, _T("bmp"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Bmp File (*.bmp)|*.bmp||"), NULL, NULL, FALSE);

	Dlg.m_ofn.lpstrInitialDir = ::g_ConfigManager.m_strRawImageSavePath;//LYS 240902
	if(Dlg.DoModal() == IDOK)
	{
		CString strPath = Dlg.GetPathName();
		int length = strPath.GetLength();
		CString strFile, strPath2;
		strPath2 = strPath.Left(length -11);
		for (int camindex = 0; camindex < 2; camindex++)
		{
			for (int divindex = 0; divindex < 3; divindex++)
			{
				strFile.Format(_T("%s\\%d_%d_raw.bmp"), strPath2, camindex, divindex);
				ImageOpen(strFile, camindex, divindex);
			}
		}
	}
}


void CDlgVisionMain::OnImageMenu1Imagesave()
{
	CFileDialog Dlg(FALSE, _T("bmp"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Bmp File (*.bmp)|*.bmp||"), NULL, NULL, FALSE);

	Dlg.m_ofn.lpstrInitialDir = ::g_ConfigManager.m_strRawImageSavePath;//LYS 240902
	if(Dlg.DoModal() == IDOK)
	{
		CString strPath = Dlg.GetPathName();

		ImageSave(strPath);

		::AfxMessageBox(_T("Image Save Complete..."));
	}
}

void CDlgVisionMain::OnImageMenu2Imageopen2()
{
	CFileDialog Dlg(TRUE, _T("bmp"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Bmp File (*.bmp)|*.bmp||"), NULL, NULL, FALSE);

	Dlg.m_ofn.lpstrInitialDir = ::g_ConfigManager.m_strRawImageSavePath;//LYS 240902
	if(Dlg.DoModal() == IDOK)
	{
		CString strPath = Dlg.GetPathName();

		ImageOpen(strPath, 1);
	}
}

void CDlgVisionMain::OnImageMenu2Imagesave2()
{
	CFileDialog Dlg(FALSE, _T("bmp"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("Bmp File (*.bmp)|*.bmp||"), NULL, NULL, FALSE);

	Dlg.m_ofn.lpstrInitialDir = ::g_ConfigManager.m_strRawImageSavePath;//LYS 240902
	if(Dlg.DoModal() == IDOK)
	{
		CString strPath = Dlg.GetPathName();

		ImageSave(strPath, 1);

		::AfxMessageBox(_T("Image Save Complete..."));
	}
}


void CDlgVisionMain::OnInspectionInspectionitemsetup()
{
	CDlgInspItemSetup dlg(this);
	dlg.DoModal();
}

void CDlgVisionMain::OnInspectionInspectionitemmatch()
{
	CDlgInspItemMatch dlg(this);
	dlg.DoModal();
}

void CDlgVisionMain::OnInspectionBatchinspection()
{
	CDlgBatchInspection dlg;
	dlg.DoModal();
}

void CDlgVisionMain::OnCalibrationCalibration()
{
	CDlgCalibration dlg(this);
	dlg.DoModal();
}

void CDlgVisionMain::OnCommunicationPlcmonitering()
{
	m_pDlgPLCMonitering->ShowWindow(SW_SHOW);
	m_pDlgPLCMonitering->StartRepeatReadData();

	SetTimer(PLC_MOINTERING, 1000, NULL);
}

void CDlgVisionMain::OnLogShowlog()
{
	m_pDlgLog->ShowWindow(SW_SHOW);
}
///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CDlgVisionMain::LoadRecipe(const LPCTSTR szPathName)
{
	CString strTemp;
	strTemp.Format(_T("%s"), szPathName);

	m_strCurRecipeName = REMOVE_FOLDER_FROM_PATHNAME(strTemp);//LYS 241011 PLC잡넘버 호출

	::g_Log.Log(_T("----------------------------------------------------------------------"), ::g_ConfigManager.m_strPrevRecipeFilePath);//LYS 241016 log추가
	::g_Log.Log(_T("Current Recipe Name : %s"), m_strCurRecipeName);//LYS 241016 log추가
	::g_Log.Log(_T("Recipe Load Path : %s"), szPathName);//LYS 241016 log추가
	::g_Log.Log(_T("----------------------------------------------------------------------"), ::g_ConfigManager.m_strPrevRecipeFilePath);//LYS 241016 log추가

	// 해당 파일이 있는지 검사한다...
	CFileFind Find;
	BOOL bRet = Find.FindFile(szPathName);
	
	if(bRet == TRUE)
	{
		::g_Log.Log(_T("Recipe FIle Find Success"));// 250712 sjpark 검사장비 정지 관련 log추가, 여기서 True 나와줘야 Recipe 정상 오픈
		::g_RecipeManager.InitParameters();
		if(::g_RecipeManager.LoadRecipeParameters(szPathName) == FALSE)
		{
			::g_Log.Log(_T("Load Recipe Parameter Set Fail"));// 250712 sjpark 검사장비 정지 관련 log추가, 여기서 True 나와줘야 Recipe 정상 오픈
			return FALSE;			
		}

		::g_ConfigManager.m_strPrevRecipeFilePath = szPathName;
		::g_ConfigManager.SaveConfig();

		switch(::g_ConfigManager.m_nMainUI)
		{
		case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
			break;
		case (long)MAINUI::UI_FOR_DUAL_CAMERA:
			m_pDlgMain2->SetRecipe(m_strCurRecipeName);
			break;
		}

		SetVersion(m_strCurRecipeName);

		m_pDlgMain2->SetResultGrid();
		return TRUE;
	}
	else
	{
		::g_Log.Log(_T("Recipe FIle Find Fail"));// 250712 sjpark 검사장비 정지 관련 log추가, 여기서 True 나와줘야 Recipe 정상 오픈
		m_pDlgMain2->SetResultGrid();
		return FALSE;
	}
}

BOOL CDlgVisionMain::SaveRecipe(const LPCTSTR szPathName)
{
	
	::g_RecipeManager.SaveRecipeParameters(szPathName);

	::g_ConfigManager.m_strPrevRecipeFilePath = szPathName;
	::g_Log.Log(_T("----------------------------------------------------------------------"), ::g_ConfigManager.m_strPrevRecipeFilePath);//LYS 241016 log추가
	::g_Log.Log(_T("Recipe save path   : %s"), ::g_ConfigManager.m_strPrevRecipeFilePath);//LYS 241016 log추가
	::g_Log.Log(_T("Current Recipe Name: %s"), m_strCurRecipeName);//LYS 241016 log추가
	::g_Log.Log(_T("----------------------------------------------------------------------"), ::g_ConfigManager.m_strPrevRecipeFilePath);//LYS 241016 log추가
	::g_ConfigManager.SaveConfig();

	::AfxMessageBox(_T("Save Complete..."));
	
	return TRUE;
}

BOOL CDlgVisionMain::ImageOpen(const CString strPath, const long camindex, const long divindex)
{
	long width(0), height(0), bpp(0);

	int index = camindex + divindex * 2;

	BYTE* pbyBuffer = nullptr;
	if(!HUBSDK::ImageInterface::ImageOpenBMP(strPath, pbyBuffer, m_nImageWidth * m_nImageHeight, &width, &height, &bpp))
	{
		return FALSE;
	}

	if(m_nImageWidth != width || m_nImageHeight != height)
	{
		m_nImageWidth = width;
		m_nImageHeight = height;
		m_nImageSize = width * height;
	}

	BYTE* image = ::g_MemoryManager.GetByteImage(index);

	// 불러온 image를 모든 buffer에 복사
	memcpy(image, pbyBuffer, m_nImageSize);

	HUBIPHelper::FreeMemory(pbyBuffer);

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_pDlgMain1->SetImage();
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_pDlgMain2->SetImage(camindex, divindex);
		break;
	}

	return TRUE;
}

BOOL CDlgVisionMain::ImageSave(const CString strPath, const long camindex, const long divindex)
{
	int index = camindex + divindex * 2;
	BYTE* image = ::g_MemoryManager.GetByteImage(index);

	HUBSDK::ImageInterface::ImageSaveBMP(strPath, image, m_nImageWidth, m_nImageHeight, 8, CRect(0, 0, m_nImageWidth, m_nImageHeight), TRUE);

	return TRUE;
}

void CDlgVisionMain::ShowSetupDialog()
{
	CDlgSetup dlg(this);
	dlg.DoModal();
}

void CDlgVisionMain::ShowUserLoginDialog()
{
	CDlgUser dlg(this);
	//CDlgLogin2 dlg(this);
	
	dlg.DoModal();
}

void CDlgVisionMain::ShowUser(const long index)
{
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_pDlgMain1->SetUser(index);
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_pDlgMain2->SetUser(index);
		break;
	}
}

CString CDlgVisionMain::SetVersion(const CString strRecipeName, BOOL bDisplay)
{
	// Version 정보 가져오기...
	TCHAR path[_MAX_PATH];

	GetModuleFileName(NULL, path, sizeof(path));

	CString strFilePath = path;
	CString strProductVersion;
	CString strTitle;

	// 회사이름, 검사 프로그램 이름, 버전정보 출력해주기...
	CFile file;
	CFileException fe;
	CString str;
	str.Format(L"%s\\ReleaseNote.txt", m_strCurPath);
	if(file.Open(str, CFile::modeRead, &fe))
	{
		const long nFileSize = 20;

		BYTE *pFileBuffer = new BYTE[nFileSize];
		file.Read(pFileBuffer, nFileSize);
		CString strVer;
		for(long x = 0; x < nFileSize; x++)
		{
			strVer += pFileBuffer[x];
		}
		if(strVer.Left(4).Compare(L"ver:") == 0)
		{
			long nEndPos = strVer.Find('\n');
			strProductVersion.Format(L"%s", strVer.Mid(4, nEndPos - 5));
		}
		delete[] pFileBuffer;
		file.Close();
	}

	strTitle.Format(_T("Hubis Vision Inspection System (ver. %s) - %s"), strProductVersion, strRecipeName);

	if(bDisplay == TRUE)
	{
		this->SetWindowText(strTitle);
	}

	return strProductVersion;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Message 관련...
LRESULT CDlgVisionMain::OnUpdateImage(WPARAM wParam, LPARAM lParam)
{
	long camIndex = (long)wParam;
	long divIndex = (long)lParam;

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_pDlgMain1->SetImage();
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:

		m_pDlgMain2->SetImage(camIndex, divIndex);
		break;
	}

	return 0L;
}

LRESULT CDlgVisionMain::OnUpdateResult(WPARAM wParam, LPARAM lParam)
{
	long camIndex = (long)wParam;
	long inspIndex = (long)LOWORD(lParam);
	long divIndex = (long)HIWORD(lParam);

	if(inspIndex < 0 || inspIndex >= (long)::g_InspectionManager.m_vecpInspection.size())
	{
		::g_Log.Log(_T("[INSP_DIAG][WARN] OnUpdateResult invalid inspIndex:%d Cam:%d Div:%d"), inspIndex, camIndex, divIndex);
		return 0L;
	}

	if((long)::g_InspectionManager.m_vecpInspection[inspIndex]->m_vecResult.size() <= 0)
	{
		return 0L;
	}

	
	long dispIndex = 0;
	::g_Log.Log(_T("\n\n=====>CDlgVisionMain::OnUpdateResult( camIndex : [%d]  divIndex : [%d] \n\n"), camIndex, divIndex);

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		dispIndex = camIndex;
		m_pDlgMain1->SetImageOverlay(inspIndex);
		m_pDlgMain1->SetResult(dispIndex, ::g_InspectionManager.m_vecpInspection[inspIndex]->m_strTotalResult);
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		dispIndex = camIndex + divIndex * 2;
		m_pDlgMain2->SetImageOverlay(camIndex, inspIndex); // 변경 중
		// 각 파트별로도 AutoPass 기능 추가 - 250228 PSJ
		if(::g_ConfigManager.m_bPLCAutoPass == FALSE) // PLC에서 Auto로 할 경우만 적용
			m_pDlgMain2->SetResult(dispIndex, ::g_InspectionManager.m_vecpInspection[inspIndex]->m_strTotalResult); // 기존
		else
			m_pDlgMain2->SetResult(dispIndex, _T("OK")); // AutoPass 기능 적용시 OK 로 강제로 적용되도록 수정


		// Main UI 에 정보표시...//LYS 241010 첫 위치일때 다른값다 초기화
		if (divIndex == 1) {
			m_pDlgMain2->InitResultGrid();
		}
		m_pDlgMain2->UpdateResultGrid(FALSE, camIndex, divIndex, inspIndex);
		break;
	}

	return 0L;
}

LRESULT CDlgVisionMain::OnUpdateGrabStatus(WPARAM wParam, LPARAM lParam)
{
	BOOL bGrab = (BOOL)wParam;
	long index = (long)lParam;

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_pDlgMain1->SetGrabConnectIcon(bGrab);
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_pDlgMain2->SetGrabConnectIcon(bGrab, index);
		break;
	}

	return 0L;
}

LRESULT CDlgVisionMain::OnUpdateInspStatus(WPARAM wParam, LPARAM lParam)
{
	BOOL bInspection = (BOOL)wParam;
	long index = (long)lParam;

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_pDlgMain1->SetInspConnectIcon(bInspection, index);
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_pDlgMain2->SetInspConnectIcon(bInspection, index);
		break;
	}

	return 0L;
}

LRESULT CDlgVisionMain::OnUpdateLaserStatus(WPARAM wParam, LPARAM lParam)
{
	BOOL bConnect = (long)lParam;

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_pDlgMain1->SetLaserConnectIcon(bConnect);
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_pDlgMain2->SetLaserConnectIcon(bConnect);
		break;
	}

	return 0L;
}

LRESULT CDlgVisionMain::OnUpdateCtrlStatus(WPARAM wParam, LPARAM lParam)
{
	BOOL bConnect = (long)lParam;

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_pDlgMain1->SetCtrlConnectIcon(bConnect);
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_pDlgMain2->SetCtrlConnectIcon(bConnect);
		break;
	}

	return 0L;
}

LRESULT CDlgVisionMain::OnUpdateRecipeUpdate(WPARAM wParam, LPARAM lParam)	// wParam = ScanCount, lParam = JobNumber
{
	long recipeIndex = (long)lParam; // m_nJobNumber - 1 ... Recipe Resist에서는 1번부터 시작함.
	CString strRecipe;
	strRecipe.Format(_T("%s\\Recipe\\%s"), m_strCurPath, ::g_PLCManager.m_vecstrUseRecipe[recipeIndex]);
	
	m_bReceiveRecipeOpen = LoadRecipe(strRecipe);	
	return 0L;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// CrashDump 관련...
void CDlgVisionMain::UserExceptionHandlerPre()
{
}

void CDlgVisionMain::UserExceptionHandlerPost()
{
	// 현재 폴더 가져오기...
	CString strCurPath;
	GetModuleFileName(nullptr, strCurPath.GetBuffer(_MAX_PATH + 1), _MAX_PATH);
	strCurPath.ReleaseBuffer();
	strCurPath = strCurPath.Left(strCurPath.ReverseFind(_T('\\')));

	CTime curTime(CTime::GetCurrentTime());

	CString strExe, strAppVersion;
	strExe.Format(_T("%s.exe"), ::GetExeName());

	HUBSDK::CFileVersion cFileVer;
	cFileVer.Open(strExe);
	strAppVersion = cFileVer.GetFileVersion2();

	CString strBackupCrashDump;
	strBackupCrashDump.Format(strCurPath + _T("\\HubisVIS_Ver%s_%04d-%02d-%02d-%02d-%02d.dmp"), strAppVersion, curTime.GetYear(), curTime.GetMonth(), curTime.GetDay(), curTime.GetHour(), curTime.GetMinute());

	CString strTemp;
	strTemp.Format(strCurPath + _T("\\MiniDump.dmp"));
	HUBSDK::CFileOperation::MoveFile(strTemp, LPCTSTR(strBackupCrashDump));
}
///////////////////////////////////////////////////////////////////////////////////////////////////
