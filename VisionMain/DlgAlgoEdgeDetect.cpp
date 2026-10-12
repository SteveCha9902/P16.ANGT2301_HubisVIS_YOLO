// DlgAlgoEdgeDetect.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgAlgoEdgeDetect.h"
#include "afxdialogex.h"

#include "DlgSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgAlgoEdgeDetect 대화 상자

IMPLEMENT_DYNAMIC(CDlgAlgoEdgeDetect, CDialogEx)

CDlgAlgoEdgeDetect::CDlgAlgoEdgeDetect(CDlgSetup* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_ALGO_EDGE_DETECT, pParent)
	, m_pParent(pParent)
	, m_pImageDisplay(pParent->m_pImageDisplay)
	, m_nImageWidth(pParent->m_nImageWidth)
	, m_nImageHeight(pParent->m_nImageHeight)
	, m_nImageSize(pParent->m_nImageSize)
	, m_nCurItemIndex(pParent->m_nLastestSelectedInspItemIndex)
{

}

CDlgAlgoEdgeDetect::~CDlgAlgoEdgeDetect()
{
}

void CDlgAlgoEdgeDetect::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_EDGEDETECT, m_propertyGridEdgeDetect);
}

BEGIN_MESSAGE_MAP(CDlgAlgoEdgeDetect, CDialogEx)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgAlgoEdgeDetect::OnAfxWmPropertyChanged)
END_MESSAGE_MAP()


// CDlgAlgoEdgeDetect 메시지 처리기


BOOL CDlgAlgoEdgeDetect::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if(m_nCurItemIndex == -1)
	{
		m_nCurItemIndex = 0;
	}

	InitPropertyGrid();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CDlgAlgoEdgeDetect::InitPropertyGrid()
{
	CString strTemp;

	// Data 갱신을 위한 초기화 - SHK 2023.12.27
	m_propertyGridEdgeDetect.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridEdgeDetect.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	// Inspection Item 통합 - SHK 2023.12.27
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sAlign;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ROI
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupROI = new CMFCPropertyGridProperty(_T("ROI"));
	m_propertyGridEdgeDetect.AddProperty(pGroupROI);

	// Deflate ROI Width
	CMFCPropertyGridProperty* pGroupROI_DeflateROIWidth = new CMFCPropertyGridProperty(_T("Deflate ROI Width"), params.nDeflateROIWidth, _T(""));
	pGroupROI->AddSubItem(pGroupROI_DeflateROIWidth);

	// Deflate ROI Height
	CMFCPropertyGridProperty* pGroupROI_DeflateROIHeight = new CMFCPropertyGridProperty(_T("Deflate ROI Height"), params.nDeflateROIHeight, _T(""));
	pGroupROI->AddSubItem(pGroupROI_DeflateROIHeight);

	// Direction
	switch(params.sEdgeDetectParam.nDirection)
	{
	case (long)DIRECTION::LEFT: strTemp = _T("Left"); break;
	case (long)DIRECTION::TOP: strTemp = _T("Top"); break;
	case (long)DIRECTION::RIGHT: strTemp = _T("Right"); break;
	case (long)DIRECTION::BOTTOM: strTemp = _T("Bottom"); break;
	}

	CMFCPropertyGridProperty* pGroupROI_Direction = new CMFCPropertyGridProperty(_T("Direction"), strTemp, _T(""));
	pGroupROI_Direction->AddOption(_T("Left"));
	pGroupROI_Direction->AddOption(_T("Top"));
	pGroupROI_Direction->AddOption(_T("Right"));
	pGroupROI_Direction->AddOption(_T("Bottom"));
	pGroupROI->AddSubItem(pGroupROI_Direction);

	// Search Direction
	switch(params.sEdgeDetectParam.nSearchDirection)
	{
	case (long)DIRECTION::LEFT: strTemp = _T("Right->Left"); break;
	case (long)DIRECTION::UP: strTemp = _T("Bottom->Top"); break;
	case (long)DIRECTION::RIGHT: strTemp = _T("Left->Right"); break;
	case (long)DIRECTION::DOWN: strTemp = _T("Top->Bottom"); break;
	}

	CMFCPropertyGridProperty* pGroupROI_SearchDirection = new CMFCPropertyGridProperty(_T("Search Direction"), strTemp, _T(""));
	pGroupROI_SearchDirection->AddOption(_T("Right->Left"));
	pGroupROI_SearchDirection->AddOption(_T("Bottom->Top"));
	pGroupROI_SearchDirection->AddOption(_T("Left->Right"));
	pGroupROI_SearchDirection->AddOption(_T("Top->Bottom"));
	pGroupROI->AddSubItem(pGroupROI_SearchDirection);

	// Edge Direction
	switch(params.sEdgeDetectParam.nEdgeDirection)
	{
	case (long)PI_ED_DIR_BOTH: strTemp = _T("Both"); break;
	case (long)PI_ED_DIR_RISING: strTemp = _T("Rising"); break;
	case (long)PI_ED_DIR_FALLING: strTemp = _T("Falling"); break;
	}

	CMFCPropertyGridProperty* pGroupROI_EdgeDirection = new CMFCPropertyGridProperty(_T("Edge Direction"), strTemp, _T(""));
	pGroupROI_EdgeDirection->AddOption(_T("Both"));
	pGroupROI_EdgeDirection->AddOption(_T("Rising"));
	pGroupROI_EdgeDirection->AddOption(_T("Falling"));
	pGroupROI->AddSubItem(pGroupROI_EdgeDirection);

	// First Edge
	strTemp = params.sEdgeDetectParam.bFirstEdge == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupROI_FirstEdge = new CMFCPropertyGridProperty(_T("First Edge"), strTemp, _T(""));
	pGroupROI_FirstEdge->AddOption(_T("TRUE"));
	pGroupROI_FirstEdge->AddOption(_T("FALSE"));
	pGroupROI->AddSubItem(pGroupROI_FirstEdge);

	// Sample Rate
	CMFCPropertyGridProperty* pGroupROI_SampleRate = new CMFCPropertyGridProperty(_T("Sample Rate"), params.sEdgeDetectParam.nSampleRate, _T(""));
	pGroupROI->AddSubItem(pGroupROI_SampleRate);

	// Edge Threshold
	CMFCPropertyGridProperty* pGroupROI_EdgeThreshold = new CMFCPropertyGridProperty(_T("Edge Threshold"), params.sEdgeDetectParam.fEdgeThreshold, _T(""));
	pGroupROI->AddSubItem(pGroupROI_EdgeThreshold);

	// Use Edge Filtering
	strTemp = params.sEdgeDetectParam.bUseEdgeFiltering == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupROI_UseEdgeFiltering = new CMFCPropertyGridProperty(_T("Use Edge Edge Filtering"), strTemp, _T(""));
	pGroupROI_UseEdgeFiltering->AddOption(_T("TRUE"));
	pGroupROI_UseEdgeFiltering->AddOption(_T("FALSE"));
	pGroupROI->AddSubItem(pGroupROI_UseEdgeFiltering);

	// Edge Filtering Range
	CMFCPropertyGridProperty* pGroupROI_EdgeFilteringRange = new CMFCPropertyGridProperty(_T("Edge Filtering Range"), params.sEdgeDetectParam.nEdgeFilteringRange, _T(""));
	pGroupROI->AddSubItem(pGroupROI_EdgeFilteringRange);

	// NG Edge Erase
	strTemp = params.sEdgeDetectParam.bNGEdgeErase == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupROI_NGEdgeErase = new CMFCPropertyGridProperty(_T("NG Edge Erase"), strTemp, _T(""));
	pGroupROI_NGEdgeErase->AddOption(_T("Erase"));
	pGroupROI_NGEdgeErase->AddOption(_T("Replace Avg."));
	pGroupROI->AddSubItem(pGroupROI_NGEdgeErase);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ignore ROI
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupIgnoreROI = new CMFCPropertyGridProperty(_T("Ignore ROI"));
	m_propertyGridEdgeDetect.AddProperty(pGroupIgnoreROI);

	CMFCPropertyGridProperty* pGroupIgnoreROI_IgnoreROI = new CMFCPropertyGridProperty(_T("Ignore ROI Num"), params.nIgnoreROINum, _T(""));
	pGroupIgnoreROI->AddSubItem(pGroupIgnoreROI_IgnoreROI);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridEdgeDetect.AddProperty(pGroupEtc);
}

LRESULT CDlgAlgoEdgeDetect::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	CMFCPropertyGridProperty *pProp = (CMFCPropertyGridProperty*)lParam;

	if(pProp == nullptr)
		return 0;

	PropertyChanged(pProp->GetName(), pProp->GetValue());
	m_pParent->UpdateAlgoEdgeDetect();
	m_pParent->ShowResultOverlay((long)ALGORITHM_SEQUENCE::EDGE_DETECT, (long)EDGE_DETECT_SEQUENCE::EDGE_DETECT);

	return 0;
}

void CDlgAlgoEdgeDetect::PropertyChanged(CString str, _variant_t value)
{
	CString strTemp;

	// Inspection Item 통합 - SHK 2023.12.27
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sAlign;

	if(str == _T("Deflate ROI Width"))
	{
		params.nDeflateROIWidth = (long)value;
	}
	else if(str == _T("Deflate ROI Height"))
	{
		params.nDeflateROIHeight = (long)value;
	}
	else if(str == _T("Direction"))
	{
		if((CString)value == _T("Left"))
		{
			params.sEdgeDetectParam.nDirection = (long)HUBSDK::DIRECTION::LEFT;
		}
		else if((CString)value == _T("Top"))
		{
			params.sEdgeDetectParam.nDirection = (long)HUBSDK::DIRECTION::TOP;
		}
		else if((CString)value == _T("Right"))
		{
			params.sEdgeDetectParam.nDirection = (long)HUBSDK::DIRECTION::RIGHT;
		}
		else if((CString)value == _T("Bottom"))
		{
			params.sEdgeDetectParam.nDirection = (long)HUBSDK::DIRECTION::BOTTOM;
		}
	}
	else if(str == _T("Search Direction"))
	{
		if((CString)value == _T("Right->Left"))
		{
			params.sEdgeDetectParam.nSearchDirection = (long)HUBSDK::DIRECTION::LEFT;
		}
		else if((CString)value == _T("Bottom->Top"))
		{
			params.sEdgeDetectParam.nSearchDirection = (long)HUBSDK::DIRECTION::UP;
		}
		else if((CString)value == _T("Left->Right"))
		{
			params.sEdgeDetectParam.nSearchDirection = (long)HUBSDK::DIRECTION::RIGHT;
		}
		else if((CString)value == _T("Top->Bottom"))
		{
			params.sEdgeDetectParam.nSearchDirection = (long)HUBSDK::DIRECTION::DOWN;
		}
	}
	else if(str == _T("Edge Direction"))
	{
		if((CString)value == _T("Both"))
		{
			params.sEdgeDetectParam.nEdgeDirection = PI_ED_DIR_BOTH;
		}
		else if((CString)value == _T("Rising"))
		{
			params.sEdgeDetectParam.nEdgeDirection = PI_ED_DIR_RISING;
		}
		else if((CString)value == _T("Falling"))
		{
			params.sEdgeDetectParam.nEdgeDirection = PI_ED_DIR_FALLING;
		}
	}
	else if(str == _T("First Edge"))
	{
		if((CString)value == _T("TRUE"))
		{
			params.sEdgeDetectParam.bFirstEdge = TRUE;
		}
		else if((CString)value == _T("FALSE"))
		{
			params.sEdgeDetectParam.bFirstEdge = FALSE;
		}
	}
	else if(str == _T("Sample Rate"))
	{
		params.sEdgeDetectParam.nSampleRate = (long)value;
	}
	else if(str == _T("Edge Threshold"))
	{
		params.sEdgeDetectParam.fEdgeThreshold = (float)value;
	}
	else if(str == _T("Use Edge Edge Filtering"))
	{
		if((CString)value == _T("TRUE"))
		{
			params.sEdgeDetectParam.bUseEdgeFiltering = TRUE;
		}
		else if((CString)value == _T("FALSE"))
		{
			params.sEdgeDetectParam.bUseEdgeFiltering = FALSE;
		}
	}
	else if(str == _T("Edge Filtering Range"))
	{
		params.sEdgeDetectParam.nEdgeFilteringRange = (long)value;
	}
	else if(str == _T("NG Edge Erase"))
	{
		if((CString)value == _T("Erase"))
		{
			params.sEdgeDetectParam.bNGEdgeErase = TRUE;
		}
		else if((CString)value == _T("Replace Avg."))
		{
			params.sEdgeDetectParam.bNGEdgeErase = FALSE;
		}
	}
	else if(str == _T("Ignore ROI Num"))
	{
		params.nIgnoreROINum = (long)value;

		long newSize = params.nIgnoreROINum;
		long oldSize = (long)params.vecrtIgnoreROI.size();

		if(newSize > oldSize)
		{
			for(long idx = 0; idx < newSize - oldSize; idx++)
			{
				params.vecrtIgnoreROI.push_back(CRect(100, 100, 200, 200));
			}
		}
		else if(newSize < oldSize)
		{
			params.vecrtIgnoreROI.resize(newSize);
		}
	}
}
