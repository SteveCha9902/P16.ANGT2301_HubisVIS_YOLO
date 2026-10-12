// DlgAlgoROI.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgAlgoROI.h"
#include "afxdialogex.h"

#include "DlgSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgAlgoROI 대화 상자

IMPLEMENT_DYNAMIC(CDlgAlgoROI, CDialogEx)

CDlgAlgoROI::CDlgAlgoROI(CDlgSetup* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_ALGO_ROI, pParent)
	, m_pParent(pParent)
	, m_pImageDisplay(pParent->m_pImageDisplay)
	, m_nImageWidth(pParent->m_nImageWidth)
	, m_nImageHeight(pParent->m_nImageHeight)
	, m_nImageSize(pParent->m_nImageSize)
	, m_nCurItemIndex(pParent->m_nLastestSelectedInspItemIndex)
{

}

CDlgAlgoROI::~CDlgAlgoROI()
{
}

void CDlgAlgoROI::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_ROI, m_propertyGridROI);
}

BEGIN_MESSAGE_MAP(CDlgAlgoROI, CDialogEx)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgAlgoROI::OnAfxWmPropertyChanged)
END_MESSAGE_MAP()


// CDlgAlgoROI 메시지 처리기


BOOL CDlgAlgoROI::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	InitPropertyGrid();

	SetROI();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

LRESULT CDlgAlgoROI::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	CMFCPropertyGridProperty *pProp = (CMFCPropertyGridProperty*)lParam;

	if (pProp == nullptr)
		return 0;

	CString strTemp;
	CString str = pProp->GetName();
	_variant_t value = pProp->GetValue();

	auto GroupROI = m_propertyGridROI.GetProperty(0);
	auto GroupROI_SearchROI = GroupROI->GetSubItem(1);
	auto GroupROI_IgnoreROI = GroupROI->GetSubItem(2);

	// Inspection Item 통합 - SHK 2023.12.27
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sROI;

	if (str == _T("Use ROI"))
	{
		if ((CString)value == _T("TRUE"))
		{
			params.bUseROI = TRUE;
			//GroupROI_ApplyMaskROI->Enable(TRUE);
			GroupROI_SearchROI->Enable(TRUE);
			GroupROI_IgnoreROI->Enable(TRUE);
		}
		else if ((CString)value == _T("FALSE"))
		{
			params.bUseROI = FALSE;
			//GroupROI_ApplyMaskROI->Enable(FALSE);
			GroupROI_SearchROI->Enable(FALSE);
			GroupROI_IgnoreROI->Enable(FALSE);
		}
	}
	else if (str == _T("Search ROI Num"))
	{
		params.nSearchROINum = (long)value;

		long newSize = params.nSearchROINum;
		long oldSize = (long)params.vecrtSearchROI.size();

		if (newSize > oldSize)
		{
			for (long idx = 0; idx < newSize - oldSize; idx++)
			{
				params.vecrtSearchROI.push_back(CRect(100, 100, 200, 200));
			}
		}
		else if (newSize < oldSize)
		{
			params.vecrtSearchROI.resize(newSize);
		}
	}
	else if (str == _T("Ignore ROI Num"))
	{
		params.nIgnoreROINum = (long)value;

		long newSize = params.nIgnoreROINum;
		long oldSize = (long)params.vecrtIgnoreROI.size();

		if (newSize > oldSize)
		{
			for (long idx = 0; idx < newSize - oldSize; idx++)
			{
				params.vecrtIgnoreROI.push_back(CRect(100, 100, 200, 200));
			}
		}
		else if (newSize < oldSize)
		{
			params.vecrtIgnoreROI.resize(newSize);
		}
	}

	m_pParent->UpdateAlgoROI();

	return 0;
}

void CDlgAlgoROI::InitPropertyGrid()
{
	CString strTemp;

	// Data 갱신을 위한 초기화 - SHK 2023.12.27
	m_propertyGridROI.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridROI.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	// Inspection Item 통합 - SHK 2023.12.27
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sROI;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ROI.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupROI = new CMFCPropertyGridProperty(_T("ROI"));
	m_propertyGridROI.AddProperty(pGroupROI);

	strTemp = params.bUseROI == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupROI_UseROI = new CMFCPropertyGridProperty(_T("Use ROI"), strTemp, _T(""));
	pGroupROI_UseROI->AddOption(_T("TRUE"));
	pGroupROI_UseROI->AddOption(_T("FALSE"));
	pGroupROI->AddSubItem(pGroupROI_UseROI);

	CMFCPropertyGridProperty* pGroupROI_SearchROI = new CMFCPropertyGridProperty(_T("Search ROI Num"), params.nSearchROINum, _T(""));
	pGroupROI->AddSubItem(pGroupROI_SearchROI);

	CMFCPropertyGridProperty* pGroupROI_IgnoreROI = new CMFCPropertyGridProperty(_T("Ignore ROI Num"), params.nIgnoreROINum, _T(""));
	pGroupROI->AddSubItem(pGroupROI_IgnoreROI);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridROI.AddProperty(pGroupEtc);
}

void CDlgAlgoROI::SetROI()
{
	// Inspection Item 통합 - SHK 2023.12.27
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sROI;

	m_pImageDisplay->ROI_RemoveAll();
	m_pImageDisplay->Overlay_RemoveAll();

	CRect imageROI = CRect(0, 0, m_nImageWidth, m_nImageHeight);

	CString strName;

	for (long idx = 0; idx < params.nSearchROINum; idx++)
	{
		strName.Format(_T("Search %d"), idx + 1);

		auto &roi = params.vecrtSearchROI[idx];

		HUBSDK::ImageCondition::VerifyROI2(imageROI, roi);

		m_pImageDisplay->ROI_Add(strName, roi, RGB(255, 0, 0), TRUE, TRUE);
	}

	for (long idx = 0; idx < params.nIgnoreROINum; idx++)
	{
		strName.Format(_T("Ignore %d"), idx + 1);

		auto &roi = params.vecrtIgnoreROI[idx];

		HUBSDK::ImageCondition::VerifyROI2(imageROI, roi);

		m_pImageDisplay->ROI_Add(strName, roi, RGB(255, 255, 0), TRUE, TRUE);
	}

	m_pImageDisplay->ROI_Show(TRUE);
	m_pImageDisplay->Overlay_Show(TRUE);
}

void CDlgAlgoROI::GetROI()
{
	// Inspection Item 통합 - SHK 2023.12.27
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sROI;

	m_pImageDisplay->Overlay_RemoveAll();

	long count(0);

	for (long idx = 0; idx < params.nSearchROINum; idx++)
	{
		m_pImageDisplay->ROI_Get(count, params.vecrtSearchROI[idx]); count++;
	}

	for (long idx = 0; idx < params.nIgnoreROINum; idx++)
	{
		m_pImageDisplay->ROI_Get(count, params.vecrtIgnoreROI[idx]); count++;
	}

	m_pImageDisplay->Overlay_Show(TRUE);
}
