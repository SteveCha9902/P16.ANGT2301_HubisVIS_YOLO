// CDlgCalibration.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgCalibration.h"
#include "afxdialogex.h"

#include "DlgVisionMain.h"
#include "DlgMain1.h"
#include "DlgMain2.h"

#include "DlgAlgoThreshold.h"
#include "DlgAlgoBlob.h"
#include "DlgAlgoCalibration.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgCalibration 대화 상자

IMPLEMENT_DYNAMIC(CDlgCalibration, CDialogEx)

CDlgCalibration::CDlgCalibration(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_CALIBRATION, pParent)
	, m_pParent(pParent)
	, m_pImageDisplay(nullptr)
	, m_pbyImage(::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11))
	, m_pbyThreshold(::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH))
	, m_nImageWidth(pParent->m_nImageWidth)
	, m_nImageHeight(pParent->m_nImageHeight)
	, m_nImageSize(pParent->m_nImageSize)
{

}

CDlgCalibration::~CDlgCalibration()
{
	delete m_pImageDisplay;
	delete m_pDlgAlgoThreshold;
	delete m_pDlgAlgoBlob;
	delete m_pDlgAlgoCalibration;
}

void CDlgCalibration::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_CAMERA, m_tabCamera);
	DDX_Control(pDX, IDC_TAB_ALGORITHM, m_tabAlgorithm);
}

BEGIN_MESSAGE_MAP(CDlgCalibration, CDialogEx)
	ON_WM_HSCROLL()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CAMERA, &CDlgCalibration::OnTcnSelchangeTabCamera)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_ALGORITHM, &CDlgCalibration::OnTcnSelchangeTabAlgorithm)
	ON_BN_CLICKED(IDC_BTN_ORIGINAL_IMAGE, &CDlgCalibration::OnBnClickedBtnOriginalImage)
	ON_BN_CLICKED(IDC_BTN_THRESHOLD_IMAGE, &CDlgCalibration::OnBnClickedBtnThresholdImage)
	ON_BN_CLICKED(IDC_BTN_SAVE, &CDlgCalibration::OnBnClickedBtnSave)
	ON_BN_CLICKED(IDC_BTN_DO_CALIBRATION, &CDlgCalibration::OnBnClickedBtnDoCalibration)
END_MESSAGE_MAP()

// CDlgCalibration 메시지 처리기

BOOL CDlgCalibration::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 2D Viewer
	m_pImageDisplay = new CImageDisplay;
	m_pImageDisplay->Initialize(IDC_STATIC_LIVE, this, RGB(50, 50, 50));
	m_pImageDisplay->SetCrossLineColor(RGB(255, 0, 0));
	m_pImageDisplay->SetCustomLineColor(RGB(255, 0, 0));
	m_pImageDisplay->SetImage(m_pbyImage, m_nImageWidth, m_nImageHeight, m_nImageWidth);

	// UI 선택 따라 Camera Tab 추가
	for(long idx = 0; idx <= ::g_ConfigManager.m_nMainUI; idx++)
	{
		m_tabCamera.InsertItem(idx, ::g_ConfigManager.m_strCameraName[idx]);
	}
	m_tabCamera.SetCurSel(0);

	m_tabAlgorithm.InsertItem(0, _T("Threshold"));
	m_tabAlgorithm.InsertItem(1, _T("Blob"));
	m_tabAlgorithm.InsertItem(2, _T("Calibration"));

	m_sThresholdParams.Init();
	m_sBlobParams.Init();
	m_sROIParams.Init();

	CRect rtTabAlgorithm;
	GetDlgItem(IDC_TAB_ALGORITHM)->GetWindowRect(rtTabAlgorithm);
	ScreenToClient(rtTabAlgorithm);

	// Algorithm UI 생성...
	m_pDlgAlgoThreshold = new CDlgAlgoThreshold(nullptr, this);
	m_pDlgAlgoThreshold->Create(CDlgAlgoThreshold::IDD, this);
	m_pDlgAlgoThreshold->SetParent(this);
	m_pDlgAlgoThreshold->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoThreshold->ShowWindow(SW_SHOW);

	m_pDlgAlgoBlob = new CDlgAlgoBlob(nullptr, this);
	m_pDlgAlgoBlob->Create(CDlgAlgoBlob::IDD, this);
	m_pDlgAlgoBlob->SetParent(this);
	m_pDlgAlgoBlob->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoBlob->ShowWindow(SW_HIDE);

	m_pDlgAlgoCalibration = new CDlgAlgoCalibration(this);
	m_pDlgAlgoCalibration->Create(CDlgAlgoCalibration::IDD, this);
	m_pDlgAlgoCalibration->SetParent(this);
	m_pDlgAlgoCalibration->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoCalibration->ShowWindow(SW_HIDE);

	InitUI();

	return TRUE;
}

void CDlgCalibration::OnTcnSelchangeTabCamera(NMHDR * pNMHDR, LRESULT * pResult)
{
	long nCurSel = m_tabCamera.GetCurSel();

	switch(nCurSel)
	{
	case (long)CAMERA_INDEX::CAMERA1:
	{
		m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH);
	}
	break;
	case (long)CAMERA_INDEX::CAMERA2:
	{
		m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH);
	}
	break;
	}

	m_pImageDisplay->Overlay_RemoveAll();
	m_pImageDisplay->SetImage(m_pbyImage, m_nImageWidth, m_nImageHeight, m_nImageWidth);

	*pResult = 0;
}

void CDlgCalibration::OnTcnSelchangeTabAlgorithm(NMHDR * pNMHDR, LRESULT * pResult)
{
	SetAlgoDialog();

	*pResult = 0;
}

void CDlgCalibration::OnBnClickedBtnOriginalImage()
{
	m_pImageDisplay->SetImage(m_pbyImage, m_nImageWidth, m_nImageHeight, m_nImageWidth);
}

void CDlgCalibration::OnBnClickedBtnThresholdImage()
{
	m_pImageDisplay->SetImage(m_pbyThreshold, m_nImageWidth, m_nImageHeight, m_nImageWidth);
}

void CDlgCalibration::InitUI()
{
	UpdateImageDisplay();
}

void CDlgCalibration::SetAlgoDialog()
{
	long nCurSel = m_tabAlgorithm.GetCurSel();

	m_pDlgAlgoThreshold->ShowWindow(SW_HIDE);
	m_pDlgAlgoBlob->ShowWindow(SW_HIDE);
	m_pDlgAlgoCalibration->ShowWindow(SW_HIDE);

	switch (nCurSel)
	{
	case 0:
		m_pDlgAlgoThreshold->ShowWindow(SW_SHOW);
		m_pDlgAlgoThreshold->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
		break;
	case 1:
		m_pDlgAlgoBlob->ShowWindow(SW_SHOW);
		m_pDlgAlgoBlob->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
		break;
	case 2:
		m_pDlgAlgoCalibration->ShowWindow(SW_SHOW);
		m_pDlgAlgoCalibration->UpdateUI();
		break;
	}

	ShowResultOverlay(nCurSel);
}

void CDlgCalibration::UpdateAlgoBlob()
{
	// 데이터 처리는 Inspection에서 - SHK 2023.12.27
	::g_Algorithm.DoThresholdImage(m_pbyImage, m_nImageWidth, m_nImageHeight, m_sROIParams, m_sThresholdParams, m_pbyThreshold, FALSE); // PSJ 임시로 FALSE처리

	auto &blobInfo = ::g_InspectionManager.m_pInspection->m_vecBlobInfoList;
	::g_Algorithm.DoDetectBlob(m_pbyImage, m_pbyThreshold, m_nImageWidth, m_nImageHeight, CRect(0, 0, m_nImageWidth, m_nImageHeight), blobInfo, m_sBlobParams, m_nBlobCount);

	CString strTemp;

	m_pImageDisplay->SetImage(m_pbyThreshold, m_nImageWidth, m_nImageHeight, m_nImageWidth);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug Overlay...
	::g_InspectionManager.m_pInspection->m_pOverlayManager->ClearData(DEBUG_BLOB_FOUND_BLOB);
	for (long idx = 0; idx < m_nBlobCount; idx++)
	{
		strTemp.Format(_T("%d"), idx + 1);
		auto rtBlob = blobInfo[idx].m_roi;

		::g_InspectionManager.m_pInspection->m_pOverlayManager->AddRect(DEBUG_BLOB_FOUND_BLOB, rtBlob);
		::g_InspectionManager.m_pInspection->m_pOverlayManager->AddString(DEBUG_BLOB_FOUND_BLOB, CPoint(rtBlob.right, rtBlob.bottom), strTemp, 125);
		::g_InspectionManager.m_pInspection->m_pOverlayManager->AddEllipse(DEBUG_BLOB_FOUND_BLOB, CRect(rtBlob.CenterPoint().x - 5, rtBlob.CenterPoint().y - 5, rtBlob.CenterPoint().x + 5, rtBlob.CenterPoint().y + 5));
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////
}

void CDlgCalibration::UpdateImageDisplay()
{
	// Image Buffer 변경 - SHK 2023.12.27
	long nCurSel = m_tabCamera.GetCurSel();

	switch (nCurSel)
	{
	case (long)CAMERA_INDEX::CAMERA1:
	{
		m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH);
		m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12);
		m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH);
		m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13);
		m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH);
	}
	break;
	case (long)CAMERA_INDEX::CAMERA2:
	{
		m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH);
		m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22);
		m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH);
		m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23);
		m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH);
	}
	break;
	}

	m_pImageDisplay->SetImage(m_pbyImage, m_nImageWidth, m_nImageHeight, m_nImageWidth);
}
void CDlgCalibration::ShowResultOverlay(const long seqIndex)
{
	m_pImageDisplay->Overlay_RemoveAll();

	auto overlayManager = ::g_InspectionManager.m_pInspection->m_pOverlayManager;

	long startIdx(0), endIdx(0);

	for (long idx = 0; idx <= (long)seqIndex; idx++)
	{
		switch (idx)
		{
		case (long)DETECT_BLOB_SEQUENCE::ROI:
			startIdx = DEBUG_ROI_BEGIN;
			endIdx = DEBUG_ROI_END;
			break;
		case (long)DETECT_BLOB_SEQUENCE::THRESHOLD:
			startIdx = DEBUG_THRESHOLD_BEGIN;
			endIdx = DEBUG_THRESHOLD_END;
			break;
		case (long)DETECT_BLOB_SEQUENCE::BLOB:
			startIdx = DEBUG_BLOB_BEGIN;
			endIdx = DEBUG_BLOB_END;
			break;
		case (long)DETECT_BLOB_SEQUENCE::RESULT:
			startIdx = DEBUG_RESULT_BEGIN;
			endIdx = DEBUG_RESULT_END;
			break;
		case (long)DETECT_BLOB_SEQUENCE::CRITERIA:
			startIdx = DEBUG_CRITERIA_BEGIN;
			endIdx = DEBUG_CRITERIA_END;
			break;
		}

		for (long overlay = startIdx; overlay <= endIdx; overlay++)
		{
			overlayManager->ShowDebugResult(m_pImageDisplay, overlay);
		}
	}

	m_pImageDisplay->Overlay_Show(TRUE);
}

void CDlgCalibration::OnBnClickedBtnSave()
{
	long nCurSel = m_tabCamera.GetCurSel();

	::g_CalibrationManager.m_fPixelResolutionX[nCurSel] = m_fPixelResolutionX;
	::g_CalibrationManager.m_fPixelResolutionY[nCurSel] = m_fPixelResolutionX;

	::g_CalibrationManager.SaveCalibration();
	::g_CalibrationManager.LoadCalibration();
}

void CDlgCalibration::OnBnClickedBtnDoCalibration()
{
	m_pDlgAlgoCalibration->DoCalibration();
}
