// DlgSetup.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgVisionMain.h"
#include "DlgSetup.h"
#include "afxdialogex.h"

#include "DlgMain1.h"
#include "DlgMain2.h"

#include "DlgAlgoROI.h"
#include "DlgAlgoThreshold.h"
#include "DlgAlgoEdgeDetect.h"
#include "DlgAlgoBlob.h"
#include "DlgAlgoResult.h"
#include "DlgAlgoCriteria.h"

#include "DlgImageProcManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgSetup 대화 상자

IMPLEMENT_DYNAMIC(CDlgSetup, CDialogEx)

CDlgSetup::CDlgSetup(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_SETUP, pParent)
	, m_pParent(pParent)
	, m_pImageDisplay(nullptr)
	, m_pDlgAlgoROI(nullptr)
	, m_pDlgAlgoThreshold(nullptr)
	, m_pDlgAlgoEdgeDetect(nullptr)
	, m_pDlgAlgoBlob(nullptr)
	, m_pDlgAlgoResult(nullptr)
	, m_pDlgAlgoCriteria(nullptr)
	, m_nLastestSelectedInspItemIndex(-1)
	, m_bResult(FALSE)
	, m_pbyImage(::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11))
	, m_pbyMask(::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_MASK))
	, m_pbyThresholdProc(::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH_PROC))
	, m_pbyThreshold(::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH))
	, m_pbyEdgeProc(::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_EDGE_PROC))
	, m_pbyBlobProc(::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_BLOB_PROC))
	, m_nImageWidth(pParent->m_nImageWidth)
	, m_nImageHeight(pParent->m_nImageHeight)
	, m_nImageSize(pParent->m_nImageSize)
{
}

CDlgSetup::~CDlgSetup()
{
	delete m_pImageDisplay;
	delete m_pDlgAlgoROI;
	delete m_pDlgAlgoThreshold;
	delete m_pDlgAlgoEdgeDetect;
	delete m_pDlgAlgoBlob;
	delete m_pDlgAlgoResult;
	delete m_pDlgAlgoCriteria;
}

void CDlgSetup::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_INSPECTION_ITEM, m_listInspectionItem);
	DDX_Control(pDX, IDC_TAB_CAMERA, m_tabCamera);
	DDX_Control(pDX, IDC_TAB_ALGORITHM, m_tabAlgorithm);
	DDX_Control(pDX, IDC_STATIC_EACH_RESULT, m_lblEachResult);
}

BEGIN_MESSAGE_MAP(CDlgSetup, CDialogEx)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_INSPECTION_ITEM, &CDlgSetup::OnLvnItemchangedListInspectionItem)
	ON_BN_CLICKED(IDC_BTN_RECIPE_SAVE, &CDlgSetup::OnBnClickedBtnRecipeSave)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_ALGORITHM, &CDlgSetup::OnTcnSelchangeTabAlgorithm)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CAMERA, &CDlgSetup::OnTcnSelchangeTabCamera)
	ON_BN_CLICKED(IDC_BTN_INSPECTION, &CDlgSetup::OnBnClickedBtnInspection)
	ON_MESSAGE(ImageDisplayEvent::UM_ROI_CHANGED, &CDlgSetup::OnROIChanged)
	ON_BN_CLICKED(IDC_BTN_ORIGINAL_IMAGE, &CDlgSetup::OnBnClickedBtnOriginalImage)
	ON_BN_CLICKED(IDC_BTN_THRESHOLD_IMAGE, &CDlgSetup::OnBnClickedBtnThresholdImage)
	ON_BN_CLICKED(IDC_BTN_PROC_IMAGE, &CDlgSetup::OnBnClickedBtnProcImage)
	ON_BN_CLICKED(IDC_BTN_IMAGE_PROC_MANAGER, &CDlgSetup::OnBnClickedBtnImageProcManager)
END_MESSAGE_MAP()

// CDlgSetup 메시지 처리기

BOOL CDlgSetup::OnInitDialog()
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

	InitInspectionItemListControl();
	SetInspectionItemListControl();

	m_nLastestSelectedInspItemIndex = 0;

	auto inspItem = ::g_RecipeManager.m_vecsInspItem;

	// 선택한 Item만 Check 표시 하기
	for(long idx = 0; idx < (long)inspItem.size(); idx++)
	{
		if(m_nLastestSelectedInspItemIndex == idx)
		{
			m_listInspectionItem.SetCheck(idx, TRUE);
		}
		else
		{
			m_listInspectionItem.SetCheck(idx, FALSE);
		}
	}

	CRect rtTabAlgorithm;
	GetDlgItem(IDC_TAB_ALGORITHM)->GetWindowRect(rtTabAlgorithm);
	ScreenToClient(rtTabAlgorithm);

	// Algorithm UI 생성...
	m_pDlgAlgoROI = new CDlgAlgoROI(this);
	m_pDlgAlgoROI->Create(CDlgAlgoROI::IDD, this);
	m_pDlgAlgoROI->SetParent(this);
	m_pDlgAlgoROI->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoROI->ShowWindow(SW_HIDE);

	m_pDlgAlgoThreshold = new CDlgAlgoThreshold(this);
	m_pDlgAlgoThreshold->Create(CDlgAlgoThreshold::IDD, this);
	m_pDlgAlgoThreshold->SetParent(this);
	m_pDlgAlgoThreshold->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoThreshold->ShowWindow(SW_HIDE);

	m_pDlgAlgoEdgeDetect = new CDlgAlgoEdgeDetect(this);
	m_pDlgAlgoEdgeDetect->Create(CDlgAlgoEdgeDetect::IDD, this);
	m_pDlgAlgoEdgeDetect->SetParent(this);
	m_pDlgAlgoEdgeDetect->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoEdgeDetect->ShowWindow(SW_HIDE);

	m_pDlgAlgoBlob = new CDlgAlgoBlob(this);
	m_pDlgAlgoBlob->Create(CDlgAlgoBlob::IDD, this);
	m_pDlgAlgoBlob->SetParent(this);
	m_pDlgAlgoBlob->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoBlob->ShowWindow(SW_HIDE);

	m_pDlgAlgoResult = new CDlgAlgoResult(this);
	m_pDlgAlgoResult->Create(CDlgAlgoResult::IDD, this);
	m_pDlgAlgoResult->SetParent(this);
	m_pDlgAlgoResult->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoResult->ShowWindow(SW_HIDE);

	m_pDlgAlgoCriteria = new CDlgAlgoCriteria(this);
	m_pDlgAlgoCriteria->Create(CDlgAlgoCriteria::IDD, this);
	m_pDlgAlgoCriteria->SetParent(this);
	m_pDlgAlgoCriteria->SetWindowPos(nullptr, rtTabAlgorithm.left, rtTabAlgorithm.bottom, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
	m_pDlgAlgoCriteria->ShowWindow(SW_HIDE);

	InitUI();
	SetInspectionResult(_T("NOT MEASURED")); // EachResult 추가 - SHK 2023.12.21

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CDlgSetup::InitUI()
{
	UpdateData(TRUE);

	CString strTemp;

	if(m_nLastestSelectedInspItemIndex > -1)
	{
		auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];

		m_tabAlgorithm.DeleteAllItems();
		switch(inspItem.algoSequence)
		{
		case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
		{
			for(long idx = 0; idx < (long)EDGE_DETECT_SEQUENCE::END; idx++)
			{
				m_tabAlgorithm.InsertItem(idx, gl_szStrEdgeDetect2SequenceName[idx]);
			}
		}
		break;
		case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
		{
			for(long idx = 0; idx < (long)DETECT_BLOB_SEQUENCE::END; idx++)
			{
				m_tabAlgorithm.InsertItem(idx, gl_szStrDetectBlobSequenceName[idx]);
			}
		}
		break;
		case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
		{
			for(long idx = 0; idx < (long)DETECT_BLOB2_SEQUENCE::END; idx++)
			{
				m_tabAlgorithm.InsertItem(idx, gl_szStrDetectBlob2SequenceName[idx]);
			}
		}
		break;
		}
		m_tabAlgorithm.SetCurSel(0);

		m_tabCamera.SetCurSel(inspItem.visionIndex); // Camera Tab 화면 갱신 - SHK 2023.12.27
		UpdateImageDisplay(); // Camera Tab 화면 갱신 - SHK 2023.12.27

		SetAlgoDialog();
	}

	UpdateData(FALSE);
}

void CDlgSetup::OnTcnSelchangeTabCamera(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Camera Tab 화면 갱신 - SHK 2023.12.27
	UpdateImageDisplay();
	m_tabAlgorithm.SetCurSel(0);

	SetAlgoDialog();

	*pResult = 0;
}

void CDlgSetup::OnBnClickedBtnOriginalImage()
{
	m_pImageDisplay->SetImage(m_pbyImage, m_nImageWidth, m_nImageHeight, m_nImageWidth);
}

void CDlgSetup::OnBnClickedBtnThresholdImage()
{
	m_pImageDisplay->SetImage(m_pbyThresholdProc, m_nImageWidth, m_nImageHeight, m_nImageWidth);
}

void CDlgSetup::OnBnClickedBtnRecipeSave()
{
	m_pParent->SaveRecipe(m_pParent->m_strCurRecipePath);
	m_pParent->LoadRecipe(m_pParent->m_strCurRecipePath); // 레시피 세팅할때 저장하고 로드도 바로 해줘야 실제 레시피로 반영됨. 250117 PSJ
	//::AfxMessageBox(_T("Save Complete..."));
}

void CDlgSetup::OnBnClickedBtnInspection()
{
	if(IsCTRLPressed() == TRUE)
	{
		BOOL bReturn = FALSE;
		BOOL bResult = FALSE;
		bReturn = ::g_InspectionManager.m_pInspection->EachInspection(m_nLastestSelectedInspItemIndex, bResult);

		auto overlayManager = ::g_InspectionManager.m_pInspection->m_pOverlayManager;
		auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];

		ShowResultOverlay(inspItem.algoSequence, 4);

		for(long overlay = DEBUG_RESULT_BEGIN; overlay <= DEBUG_RESULT_END; overlay++)
		{
			overlayManager->ShowDebugResult(m_pImageDisplay, overlay);
		}

		for(long overlay = DEBUG_RESULT; overlay <= DEBUG_RESULT_NG; overlay++)
		{
			overlayManager->ShowDebugResult(m_pImageDisplay, overlay);
		}

		m_pImageDisplay->Overlay_Show(TRUE);
		m_pImageDisplay->ROI_Show(FALSE);

		SetInspectionResult(::g_InspectionManager.m_pInspection->m_strTotalResult);
	}
	else
	{
		auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];

		long nCurSel = m_tabAlgorithm.GetCurSel();

		switch(inspItem.algoSequence)
		{
		case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
		{
			switch(nCurSel)
			{
			case (long)EDGE_DETECT_SEQUENCE::ROI:
				UpdateAlgoROI();
				break;
			case (long)EDGE_DETECT_SEQUENCE::THRESHOLD:
				UpdateAlgoThreshold();
				break;
			case (long)EDGE_DETECT_SEQUENCE::BLOB:
				UpdateAlgoBlob();
				m_pDlgAlgoBlob->UpdateBlobList();
				break;
			case (long)EDGE_DETECT_SEQUENCE::EDGE_DETECT:
				UpdateAlgoEdgeDetect();
				break;
			case (long)EDGE_DETECT_SEQUENCE::RESULT:
				UpdateAlgoResult();
				break;
			case (long)EDGE_DETECT_SEQUENCE::CRITERIA:
				UpdateAlgoCriteria();
				break;
			}
		}
		break;
		case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
		{
			switch(nCurSel)
			{
			case (long)DETECT_BLOB_SEQUENCE::ROI:
				UpdateAlgoROI();
				break;
			case (long)DETECT_BLOB_SEQUENCE::THRESHOLD:
				UpdateAlgoThreshold();
				break;
			case (long)DETECT_BLOB_SEQUENCE::BLOB:
				UpdateAlgoBlob();
				m_pDlgAlgoBlob->UpdateBlobList();
				break;
			case (long)DETECT_BLOB_SEQUENCE::RESULT:
				UpdateAlgoResult();
				break;
			case (long)DETECT_BLOB_SEQUENCE::CRITERIA:
				UpdateAlgoCriteria();
				break;
			}
		}
		break;
		case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
		{
			switch(nCurSel)
			{
			case (long)DETECT_BLOB2_SEQUENCE::ROI:
				UpdateAlgoROI();
				break;
			case (long)DETECT_BLOB2_SEQUENCE::THRESHOLD:
				UpdateAlgoThreshold();
				break;
			case (long)DETECT_BLOB2_SEQUENCE::BLOB:
				UpdateAlgoBlob();
				m_pDlgAlgoBlob->UpdateBlobList();
				break;
			case (long)DETECT_BLOB2_SEQUENCE::BLOB2:
				UpdateAlgoBlob2();
				m_pDlgAlgoBlob->UpdateBlobList();
				break;
			case (long)DETECT_BLOB2_SEQUENCE::RESULT:
				UpdateAlgoResult();
				break;
			case (long)DETECT_BLOB2_SEQUENCE::CRITERIA:
				UpdateAlgoCriteria();
				break;
			}
		}
		break;
		}

		ShowResultOverlay(inspItem.algoSequence, nCurSel);

		SetInspectionResult(::g_InspectionManager.m_pInspection->m_strTotalResult);
	}
}

void CDlgSetup::InitInspectionItemListControl()
{
	m_listInspectionItem.SetExtendedStyle(m_listInspectionItem.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_listInspectionItem.InsertColumn(0, _T(""), LVCFMT_CENTER, 20);
	m_listInspectionItem.InsertColumn(1, _T("No."), LVCFMT_CENTER, 40);
	m_listInspectionItem.InsertColumn(2, _T("Name"), LVCFMT_CENTER, 150);
	m_listInspectionItem.InsertColumn(3, _T("Vision Index"), LVCFMT_CENTER, 100);
	m_listInspectionItem.InsertColumn(4, _T("Div. Index"), LVCFMT_CENTER, 100);
	m_listInspectionItem.InsertColumn(5, _T("Algorithm Sequence"), LVCFMT_CENTER, 200);
}

void CDlgSetup::SetInspectionItemListControl()
{
	CString strTemp;

	// List 에 있는 내용을 모두 지운다.
	m_listInspectionItem.DeleteAllItems();

	auto inspItem = ::g_RecipeManager.m_vecsInspItem;

	for(long insp = 0; insp < (long)inspItem.size(); insp++)
	{
		m_listInspectionItem.InsertItem(insp, _T(""));

		strTemp.Format(_T("%d"), insp + 1);
		m_listInspectionItem.SetItemText(insp, 1, strTemp);
		m_listInspectionItem.SetItemText(insp, 2, inspItem[insp].strName);
		strTemp.Format(_T("%d"), inspItem[insp].visionIndex);
		m_listInspectionItem.SetItemText(insp, 3, strTemp);
		strTemp.Format(_T("%d"), inspItem[insp].divIndex);
		m_listInspectionItem.SetItemText(insp, 4, strTemp);
		m_listInspectionItem.SetItemText(insp, 5, gl_szStrAlgoSequenceName[inspItem[insp].algoSequence]);
	}
}

// EachResult 추가 - SHK 2023.12.21
void CDlgSetup::SetInspectionResult(const CString strResult)
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

	m_lblEachResult.SetText(strInspResult);
	m_lblEachResult.SetFontSize(22);
	m_lblEachResult.SetFontBold(TRUE);
	if(strResult == _T("OK"))
	{
		m_lblEachResult.SetTextColor(RGB(0, 255, 0));
	}
	else
	{
		m_lblEachResult.SetTextColor(RGB(255, 0, 0));
	}
	m_lblEachResult.SetBorder(TRUE);

	m_bResult = !m_bResult;
}

void CDlgSetup::UpdateAlgoROI()
{
	m_pDlgAlgoROI->SetROI();

	auto params = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex].sROI;

	::g_InspectionManager.m_pInspection->DoSetROI(m_nLastestSelectedInspItemIndex);

	m_pImageDisplay->SetImage(m_pbyImage, m_nImageWidth, m_nImageHeight, m_nImageWidth);

	if(params.bUseROI == TRUE)
	{
		m_pImageDisplay->ROI_Show(TRUE);
	}
	else
	{
		m_pImageDisplay->ROI_Show(FALSE);
	}
}

void CDlgSetup::UpdateAlgoThreshold()
{
	m_pImageDisplay->ROI_RemoveAll();

	// 데이터 처리는 Inspection에서 - SHK 2023.12.27
	::g_InspectionManager.m_pInspection->DoSetROI(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoThresholdImage(m_nLastestSelectedInspItemIndex);

	m_pImageDisplay->SetImage(m_pbyThreshold, m_nImageWidth, m_nImageHeight, m_nImageWidth);
}

void CDlgSetup::UpdateAlgoBlob()
{
	// 데이터 처리는 Inspection에서 - SHK 2023.12.27
	::g_InspectionManager.m_pInspection->DoSetROI(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoThresholdImage(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoDetectBlob(m_nLastestSelectedInspItemIndex);

	m_pImageDisplay->SetImage(m_pbyBlobProc, m_nImageWidth, m_nImageHeight, m_nImageWidth);
}

void CDlgSetup::UpdateAlgoBlob2()
{
	// 데이터 처리는 Inspection에서 - SHK 2023.12.27
	::g_InspectionManager.m_pInspection->DoSetROI(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoThresholdImage(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoDetectBlob(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoDetectBlob2(m_nLastestSelectedInspItemIndex);

	m_pImageDisplay->SetImage(m_pbyBlobProc2, m_nImageWidth, m_nImageHeight, m_nImageWidth);
}

void CDlgSetup::UpdateAlgoEdgeDetect()
{
	m_pImageDisplay->ROI_Show(FALSE);

	// 데이터 처리는 Inspection에서 - SHK 2023.12.27
	::g_InspectionManager.m_pInspection->DoSetROI(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoThresholdImage(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoDetectBlob(m_nLastestSelectedInspItemIndex);
	::g_InspectionManager.m_pInspection->DoEdgeDetect(m_nLastestSelectedInspItemIndex);

	m_pImageDisplay->SetImage(m_pbyEdgeProc, m_nImageWidth, m_nImageHeight, m_nImageWidth);

	//m_pImageDisplay->Overlay_Show(TRUE);
}

void CDlgSetup::UpdateAlgoCriteria()
{
	m_pImageDisplay->ROI_RemoveAll();
	m_pImageDisplay->Overlay_RemoveAll();

	auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];
	long nCurSel = m_tabAlgorithm.GetCurSel();

	BOOL bResult = FALSE;

	//m_pImageDisplay->Overlay_RemoveAll();

	m_pDlgAlgoCriteria->SetCriteria(inspItem.algoSequence, bResult);

	//m_pImageDisplay->Overlay_Show(TRUE);
}

void CDlgSetup::UpdateAlgoResult()
{
	m_pImageDisplay->ROI_RemoveAll();

	auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];

	m_pDlgAlgoResult->SetResult(inspItem.algoSequence);
}

void CDlgSetup::ShowResultOverlay(const long sequence, const long seqIndex)
{
	m_pImageDisplay->Overlay_RemoveAll();

	auto overlayManager = ::g_InspectionManager.m_pInspection->m_pOverlayManager;

	long startIdx(0), endIdx(0);

	switch(sequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
	{
		for(long idx = 0; idx <= (long)seqIndex; idx++)
		{
			switch(idx)
			{
			case (long)EDGE_DETECT_SEQUENCE::ROI:
				startIdx = DEBUG_ROI_BEGIN;
				endIdx = DEBUG_ROI_END;
				break;
			case (long)EDGE_DETECT_SEQUENCE::THRESHOLD:
				startIdx = DEBUG_THRESHOLD_BEGIN;
				endIdx = DEBUG_THRESHOLD_END;
				break;
			case (long)EDGE_DETECT_SEQUENCE::BLOB:
				startIdx = DEBUG_BLOB_BEGIN;
				endIdx = DEBUG_BLOB_END;
				break;
			case (long)EDGE_DETECT_SEQUENCE::EDGE_DETECT:
				startIdx = DEBUG_EDGE_DETECT_BEGIN;
				endIdx = DEBUG_EDGE_DETECT_END;
				break;
			case (long)EDGE_DETECT_SEQUENCE::RESULT:
				startIdx = DEBUG_RESULT_BEGIN;
				endIdx = DEBUG_RESULT_END;
				break;
			case (long)EDGE_DETECT_SEQUENCE::CRITERIA:
				startIdx = DEBUG_CRITERIA_BEGIN;
				endIdx = DEBUG_CRITERIA_END;
				break;
			}

			for(long overlay = startIdx; overlay <= endIdx; overlay++)
			{
				overlayManager->ShowDebugResult(m_pImageDisplay, overlay);
			}
		}
		break;
	}
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		for(long idx = 0; idx <= (long)seqIndex; idx++)
		{
			switch(idx)
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

			for(long overlay = startIdx; overlay <= endIdx; overlay++)
			{
				overlayManager->ShowDebugResult(m_pImageDisplay, overlay);
			}
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		for(long idx = 0; idx <= (long)seqIndex; idx++)
		{
			switch(idx)
			{
			case (long)DETECT_BLOB2_SEQUENCE::ROI:
				startIdx = DEBUG_ROI_BEGIN;
				endIdx = DEBUG_ROI_END;
				break;
			case (long)DETECT_BLOB2_SEQUENCE::THRESHOLD:
				startIdx = DEBUG_THRESHOLD_BEGIN;
				endIdx = DEBUG_THRESHOLD_END;
				break;
			case (long)DETECT_BLOB2_SEQUENCE::BLOB:
				startIdx = DEBUG_BLOB_BEGIN;
				endIdx = DEBUG_BLOB_END;
				break;
			case (long)DETECT_BLOB2_SEQUENCE::BLOB2:
				startIdx = DEBUG_BLOB2_BEGIN;
				endIdx = DEBUG_BLOB2_END;
				break;
			case (long)DETECT_BLOB2_SEQUENCE::RESULT:
				startIdx = DEBUG_RESULT_BEGIN;
				endIdx = DEBUG_RESULT_END;
				break;
			case (long)DETECT_BLOB2_SEQUENCE::CRITERIA:
				startIdx = DEBUG_CRITERIA_BEGIN;
				endIdx = DEBUG_CRITERIA_END;
				break;
			}

			for(long overlay = startIdx; overlay <= endIdx; overlay++)
			{
				overlayManager->ShowDebugResult(m_pImageDisplay, overlay);
			}
		}
	}
	break;
	}

	m_pImageDisplay->Overlay_Show(TRUE);
}

void CDlgSetup::OnLvnItemchangedListInspectionItem(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	long inspItemIndex = m_listInspectionItem.GetNextItem(-1, LVNI_SELECTED);

	if(inspItemIndex == -1)
	{
		return;
	}

	::g_InspectionManager.m_pInspection->m_pOverlayManager->ClearAll(FALSE);
	m_pDlgAlgoBlob->ClearBlobList();

	m_nLastestSelectedInspItemIndex = inspItemIndex;

	auto inspItem = ::g_RecipeManager.m_vecsInspItem;

	// 선택한 Item만 Check 표시 하기
	for(long idx = 0; idx < (long)inspItem.size(); idx++)
	{
		if(inspItemIndex == idx)
		{
			m_listInspectionItem.SetCheck(idx, TRUE);
		}
		else
		{
			m_listInspectionItem.SetCheck(idx, FALSE);
		}
	}

	InitUI();

	*pResult = 0;
}

void CDlgSetup::OnTcnSelchangeTabAlgorithm(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	SetAlgoDialog();

}

void CDlgSetup::SetAlgoDialog()
{
	auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];

	long nCurSel = m_tabAlgorithm.GetCurSel();

	m_pDlgAlgoROI->ShowWindow(SW_HIDE);
	m_pDlgAlgoThreshold->ShowWindow(SW_HIDE);
	m_pDlgAlgoEdgeDetect->ShowWindow(SW_HIDE);
	m_pDlgAlgoBlob->ShowWindow(SW_HIDE);
	m_pDlgAlgoResult->ShowWindow(SW_HIDE);
	m_pDlgAlgoCriteria->ShowWindow(SW_HIDE);

	switch(inspItem.algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
	{
		switch(nCurSel)
		{
		case (long)EDGE_DETECT_SEQUENCE::ROI:
			m_pDlgAlgoROI->ShowWindow(SW_SHOW);
			m_pDlgAlgoROI->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
			UpdateAlgoROI();
			break;
		case (long)EDGE_DETECT_SEQUENCE::THRESHOLD:
			m_pDlgAlgoThreshold->ShowWindow(SW_SHOW);
			m_pDlgAlgoThreshold->InitPropertyGrid();
			m_pDlgAlgoThreshold->UpdateUI(); // 데이터 갱신 - SHK 2023.12.27
			//UpdateAlgoThreshold();
			break;
		case (long)EDGE_DETECT_SEQUENCE::BLOB:
			m_pDlgAlgoBlob->ShowWindow(SW_SHOW);
			m_pDlgAlgoBlob->InitPropertyGrid();
			//m_pDlgAlgoBlob->UpdateBlobList();
			//UpdateAlgoBlob();
			break;
		case (long)EDGE_DETECT_SEQUENCE::EDGE_DETECT:
			m_pDlgAlgoEdgeDetect->ShowWindow(SW_SHOW);
			m_pDlgAlgoEdgeDetect->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
			//UpdateAlgoEdgeDetect();
			break;
		case (long)EDGE_DETECT_SEQUENCE::RESULT:
			m_pDlgAlgoResult->ShowWindow(SW_SHOW);
			m_pDlgAlgoResult->InitPropertyGrid();
			//UpdateAlgoResult();
			break;
		case (long)EDGE_DETECT_SEQUENCE::CRITERIA:
			m_pDlgAlgoCriteria->ShowWindow(SW_SHOW);
			m_pDlgAlgoCriteria->InitPropertyGrid();
			//UpdateAlgoCriteria();
			break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		switch(nCurSel)
		{
		case (long)DETECT_BLOB_SEQUENCE::ROI:
			m_pDlgAlgoROI->ShowWindow(SW_SHOW);
			m_pDlgAlgoROI->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
			UpdateAlgoROI();
			break;
		case (long)DETECT_BLOB_SEQUENCE::THRESHOLD:
			m_pDlgAlgoThreshold->ShowWindow(SW_SHOW);
			m_pDlgAlgoThreshold->InitPropertyGrid();
			m_pDlgAlgoThreshold->UpdateUI(); // 데이터 갱신 - SHK 2023.12.27
			//UpdateAlgoThreshold();
			break;
		case (long)DETECT_BLOB_SEQUENCE::BLOB:
			m_pDlgAlgoBlob->ShowWindow(SW_SHOW);
			m_pDlgAlgoBlob->InitPropertyGrid();
			//m_pDlgAlgoBlob->UpdateBlobList();
			//UpdateAlgoBlob();
			break;
		case (long)DETECT_BLOB_SEQUENCE::RESULT:
			m_pDlgAlgoResult->ShowWindow(SW_SHOW);
			m_pDlgAlgoResult->InitPropertyGrid();
			//UpdateAlgoResult();
			break;
		case (long)DETECT_BLOB_SEQUENCE::CRITERIA:
			m_pDlgAlgoCriteria->ShowWindow(SW_SHOW);
			m_pDlgAlgoCriteria->InitPropertyGrid();
			//UpdateAlgoCriteria();
			break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		switch(nCurSel)
		{
		case (long)DETECT_BLOB2_SEQUENCE::ROI:
			m_pDlgAlgoROI->ShowWindow(SW_SHOW);
			m_pDlgAlgoROI->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
			UpdateAlgoROI();
			break;
		case (long)DETECT_BLOB2_SEQUENCE::THRESHOLD:
			m_pDlgAlgoThreshold->ShowWindow(SW_SHOW);
			m_pDlgAlgoThreshold->InitPropertyGrid();
			m_pDlgAlgoThreshold->UpdateUI(); // 데이터 갱신 - SHK 2023.12.27
			//UpdateAlgoThreshold();
			break;
		case (long)DETECT_BLOB2_SEQUENCE::BLOB:
		case (long)DETECT_BLOB2_SEQUENCE::BLOB2:
			m_pDlgAlgoBlob->ShowWindow(SW_SHOW);
			m_pDlgAlgoBlob->InitPropertyGrid();
			//m_pDlgAlgoBlob->UpdateBlobList();
			//UpdateAlgoBlob();
			break;
		case (long)DETECT_BLOB2_SEQUENCE::RESULT:
			m_pDlgAlgoResult->ShowWindow(SW_SHOW);
			m_pDlgAlgoResult->InitPropertyGrid();
			//UpdateAlgoResult();
			break;
		case (long)DETECT_BLOB2_SEQUENCE::CRITERIA:
			m_pDlgAlgoCriteria->ShowWindow(SW_SHOW);
			m_pDlgAlgoCriteria->InitPropertyGrid();
			//UpdateAlgoCriteria();
			break;
		}
	}
	break;
	}

	//ShowResultOverlay(inspItem.algoSequence, nCurSel);
}

void CDlgSetup::GetProcData(short * pProc, long procSize, short * pProcParams, long procParamsSize, SProcImageParams& sProc)
{
	//auto params = &::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex].sAlign;
	//auto &procParams = params->sProcParam;

	sProc.nImageProcNum = procSize;

	sProc.m_vecnProc.clear();
	sProc.m_vecnProc.resize(procSize);
	for(long idx = 0; idx < procSize; idx++)
	{
		sProc.m_vecnProc[idx] = pProc[idx];
	}

	sProc.m_vecnProcParams.clear();
	sProc.m_vecnProcParams.resize(procParamsSize);
	for(long idx = 0; idx < procParamsSize; idx++)
	{
		sProc.m_vecnProcParams[idx] = pProcParams[idx];
	}
}

void CDlgSetup::UpdateProcImage()
{
	BYTE* src = nullptr;
	BYTE* dst = nullptr;
	auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];
	long nCurSel = m_tabAlgorithm.GetCurSel();
	long nProcIndex = 0;

	switch(inspItem.algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
	{
		switch(nCurSel)
		{
		case (long)EDGE_DETECT_SEQUENCE::ROI:
			break;
		case (long)EDGE_DETECT_SEQUENCE::THRESHOLD:
			src = m_pbyImage;
			dst = m_pbyThresholdProc;
			nProcIndex = 0;
			break;
		case (long)EDGE_DETECT_SEQUENCE::BLOB:
			src = m_pbyThreshold;
			dst = m_pbyBlobProc;
			nProcIndex = 1;
			break;
		case (long)EDGE_DETECT_SEQUENCE::EDGE_DETECT:
			src = m_pbyImage;
			dst = m_pbyEdgeProc;
			nProcIndex = 1;
			break;
		case (long)EDGE_DETECT_SEQUENCE::RESULT:
		case (long)EDGE_DETECT_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		switch(nCurSel)
		{
		case (long)DETECT_BLOB_SEQUENCE::ROI:
			break;
		case (long)DETECT_BLOB_SEQUENCE::THRESHOLD:
			src = m_pbyImage;
			dst = m_pbyThresholdProc;
			nProcIndex = 0;
			break;
		case (long)DETECT_BLOB_SEQUENCE::BLOB:
			src = m_pbyThreshold;
			dst = m_pbyBlobProc;
			nProcIndex = 1;
			break;
		case (long)DETECT_BLOB_SEQUENCE::RESULT:
		case (long)DETECT_BLOB_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		switch(nCurSel)
		{
		case (long)DETECT_BLOB2_SEQUENCE::ROI:
			break;
		case (long)DETECT_BLOB2_SEQUENCE::THRESHOLD:
			src = m_pbyImage;
			dst = m_pbyThresholdProc;
			nProcIndex = 0;
			break;
		case (long)DETECT_BLOB2_SEQUENCE::BLOB:
			src = m_pbyThreshold;
			dst = m_pbyBlobProc;
			nProcIndex = 1;
			break;
		case (long)DETECT_BLOB2_SEQUENCE::BLOB2:
			src = m_pbyThreshold;
			dst = m_pbyBlobProc2;
			nProcIndex = 1;
			break;
		case (long)DETECT_BLOB2_SEQUENCE::RESULT:
		case (long)DETECT_BLOB2_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	}

	if(src != nullptr && dst != nullptr)
	{
		auto params = inspItem.sProc[nProcIndex];
		::g_InspectionManager.m_pInspection->m_pAlgorithm->DoProcImage(src, m_nImageWidth, m_nImageHeight, params, dst);

		m_pImageDisplay->SetImage(dst, m_nImageWidth, m_nImageHeight, m_nImageWidth);
	}
}

void CDlgSetup::UpdateImageDisplay()
{
	// Image Buffer 변경 - SHK 2023.12.27
	long nCurSel = m_tabCamera.GetCurSel();

	switch (m_nLastestSelectedInspItemIndex)
	{
		case (long)IMAGE::ORIGINAL11:
		{
			m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
			m_pbyMask = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_MASK);
			m_pbyThresholdProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH_PROC);
			m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH);
			m_pbyEdgeProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_EDGE_PROC);
			m_pbyBlobProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_BLOB_PROC);
			m_pbyBlobProc2 = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_BLOB_PROC2);
		}
		break;
		case (long)IMAGE::ORIGINAL12:
		{
			m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12);
			m_pbyMask = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_MASK);
			m_pbyThresholdProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH_PROC);
			m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH);
			m_pbyEdgeProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_EDGE_PROC);
			m_pbyBlobProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_BLOB_PROC);
			m_pbyBlobProc2 = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_BLOB_PROC2);
		}
		break;
		case (long)IMAGE::ORIGINAL13:
		{
			m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13);
			m_pbyMask = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_MASK);
			m_pbyThresholdProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH_PROC);
			m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH);
			m_pbyEdgeProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_EDGE_PROC);
			m_pbyBlobProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_BLOB_PROC);
			m_pbyBlobProc2 = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_BLOB_PROC2);
		}
		break;
		case (long)IMAGE::ORIGINAL21:
		{
			m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
			m_pbyMask = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_MASK);
			m_pbyThresholdProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH_PROC);
			m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH);
			m_pbyEdgeProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_EDGE_PROC);
			m_pbyBlobProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_BLOB_PROC);
			m_pbyBlobProc2 = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_BLOB_PROC2);
		}
		break;
		case (long)IMAGE::ORIGINAL22:
		{
			m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22);
			m_pbyMask = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_MASK);
			m_pbyThresholdProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH_PROC);
			m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH);
			m_pbyEdgeProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_EDGE_PROC);
			m_pbyBlobProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_BLOB_PROC);
			m_pbyBlobProc2 = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_BLOB_PROC2);
		}
		break;
		case (long)IMAGE::ORIGINAL23:
		{
			m_pbyImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23);
			m_pbyMask = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_MASK);
			m_pbyThresholdProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH_PROC);
			m_pbyThreshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH);
			m_pbyEdgeProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_EDGE_PROC);
			m_pbyBlobProc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_BLOB_PROC);
			m_pbyBlobProc2 = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_BLOB_PROC2);
		}
		break;
	}

	m_pImageDisplay->SetImage(m_pbyImage, m_nImageWidth, m_nImageHeight, m_nImageWidth);

	m_pImageDisplay->Overlay_RemoveAll();
}

long CDlgSetup::GetItemIndex()
{
	return m_nLastestSelectedInspItemIndex;
}

LRESULT CDlgSetup::OnROIChanged(WPARAM wParam, LPARAM lParam)
{
	auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];

	long nCurSel = m_tabAlgorithm.GetCurSel();

	switch(inspItem.algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
	{
		switch(nCurSel)
		{
		case (long)EDGE_DETECT_SEQUENCE::ROI:
			m_pDlgAlgoROI->GetROI();
			m_pDlgAlgoROI->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
			UpdateAlgoROI();
			break;
		case (long)EDGE_DETECT_SEQUENCE::THRESHOLD:
			break;
		case (long)EDGE_DETECT_SEQUENCE::BLOB:
			break;
		case (long)EDGE_DETECT_SEQUENCE::EDGE_DETECT:
			m_pDlgAlgoEdgeDetect->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
			UpdateAlgoEdgeDetect();
			break;
		case (long)EDGE_DETECT_SEQUENCE::RESULT:
			break;
		case (long)EDGE_DETECT_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		switch(nCurSel)
		{
		case (long)DETECT_BLOB_SEQUENCE::ROI:
			m_pDlgAlgoROI->GetROI();
			m_pDlgAlgoROI->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
			UpdateAlgoROI();
			break;
		case (long)DETECT_BLOB_SEQUENCE::THRESHOLD:
			break;
		case (long)DETECT_BLOB_SEQUENCE::BLOB:
			break;
		case (long)DETECT_BLOB_SEQUENCE::RESULT:
			break;
		case (long)DETECT_BLOB_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		switch(nCurSel)
		{
		case (long)DETECT_BLOB2_SEQUENCE::ROI:
			m_pDlgAlgoROI->GetROI();
			m_pDlgAlgoROI->InitPropertyGrid(); // 데이터 갱신 - SHK 2023.12.27
			UpdateAlgoROI();
			break;
		case (long)DETECT_BLOB2_SEQUENCE::THRESHOLD:
			break;
		case (long)DETECT_BLOB2_SEQUENCE::BLOB:
			break;
		case (long)DETECT_BLOB2_SEQUENCE::BLOB2:
			break;
		case (long)DETECT_BLOB2_SEQUENCE::RESULT:
			break;
		case (long)DETECT_BLOB2_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	}

	ShowResultOverlay(inspItem.algoSequence, nCurSel);

	return 0;
}

void CDlgSetup::OnBnClickedBtnProcImage()
{
	UpdateProcImage();
}

void CDlgSetup::OnBnClickedBtnImageProcManager()
{
	BYTE* src = nullptr;
	BYTE* dst = nullptr;
	auto inspItem = &::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];
	long nCurSel = m_tabAlgorithm.GetCurSel();
	long nProcIndex = 0;

	switch(inspItem->algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
	{
		switch(nCurSel)
		{
		case (long)EDGE_DETECT_SEQUENCE::ROI:
			break;
		case (long)EDGE_DETECT_SEQUENCE::THRESHOLD:
			src = m_pbyImage;
			dst = m_pbyThresholdProc;
			nProcIndex = 0;
			break;
		case (long)EDGE_DETECT_SEQUENCE::BLOB:
			src = m_pbyThreshold;
			dst = m_pbyBlobProc;
			nProcIndex = 1;
			break;
		case (long)EDGE_DETECT_SEQUENCE::EDGE_DETECT:
			src = m_pbyImage;
			dst = m_pbyEdgeProc;
			nProcIndex = 1;
			break;
		case (long)EDGE_DETECT_SEQUENCE::RESULT:
		case (long)EDGE_DETECT_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		switch(nCurSel)
		{
		case (long)DETECT_BLOB_SEQUENCE::ROI:
			break;
		case (long)DETECT_BLOB_SEQUENCE::THRESHOLD:
			src = m_pbyImage;
			dst = m_pbyThresholdProc;
			nProcIndex = 0;
			break;
		case (long)DETECT_BLOB_SEQUENCE::BLOB:
			src = m_pbyThreshold;
			dst = m_pbyBlobProc;
			nProcIndex = 1;
			break;
		case (long)DETECT_BLOB_SEQUENCE::RESULT:
		case (long)DETECT_BLOB_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		switch(nCurSel)
		{
		case (long)DETECT_BLOB2_SEQUENCE::ROI:
			break;
		case (long)DETECT_BLOB2_SEQUENCE::THRESHOLD:
			src = m_pbyImage;
			dst = m_pbyThresholdProc;
			nProcIndex = 0;
			break;
		case (long)DETECT_BLOB2_SEQUENCE::BLOB:
			src = m_pbyThreshold;
			dst = m_pbyBlobProc;
			nProcIndex = 1;
			break;
		case (long)DETECT_BLOB2_SEQUENCE::BLOB2:
			src = m_pbyThreshold;
			dst = m_pbyBlobProc2;
			nProcIndex = 2;
			break;
		case (long)DETECT_BLOB2_SEQUENCE::RESULT:
		case (long)DETECT_BLOB2_SEQUENCE::CRITERIA:
			break;
		}
	}
	break;
	}

	if(src != nullptr && dst != nullptr)
	{
		auto &procParams = inspItem->sProc[nProcIndex];
		const long paramSize = (long)procParams.m_vecnProc.size();

		CDlgImageProcManager* DlgImageProcManager;

		DlgImageProcManager = new CDlgImageProcManager(src, m_nImageWidth, m_nImageHeight, paramSize > 0 ? &procParams.m_vecnProc[0] : nullptr, paramSize > 0 ? &procParams.m_vecnProcParams[0] : nullptr, paramSize);

		if(DlgImageProcManager->DoModal() == IDOK)
		{
			GetProcData(DlgImageProcManager->GetProc(), DlgImageProcManager->GetProcSize(), DlgImageProcManager->GetProcParams(), DlgImageProcManager->GetProcParamsSize(), procParams);

			UpdateProcImage();
		}

		delete DlgImageProcManager;
	}
}
