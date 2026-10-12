// DlgResult.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgAlgoResult.h"
#include "afxdialogex.h"

#include "DlgSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgAlgoResult 대화 상자

IMPLEMENT_DYNAMIC(CDlgAlgoResult, CDialogEx)

CDlgAlgoResult::CDlgAlgoResult(CDlgSetup* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_ALGO_RESULT, pParent)
	, m_pParent(pParent)
	, m_pImageDisplay(pParent->m_pImageDisplay)
	, m_nImageWidth(pParent->m_nImageWidth)
	, m_nImageHeight(pParent->m_nImageHeight)
	, m_nImageSize(pParent->m_nImageSize)
	, m_nCurItemIndex(pParent->m_nLastestSelectedInspItemIndex)
{

}

CDlgAlgoResult::~CDlgAlgoResult()
{
}

void CDlgAlgoResult::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_RESULT, m_propertyGridResult);
}


BEGIN_MESSAGE_MAP(CDlgAlgoResult, CDialogEx)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgAlgoResult::OnAfxWmPropertyChanged)
END_MESSAGE_MAP()


// CDlgAlgoResult 메시지 처리기

BOOL CDlgAlgoResult::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitPropertyGrid();

	return TRUE;
}

LRESULT CDlgAlgoResult::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	CMFCPropertyGridProperty *pProp = (CMFCPropertyGridProperty*)lParam;

	if(pProp == nullptr)
		return 0;

	CString str = pProp->GetName();
	_variant_t value = pProp->GetValue();

	m_nCurItemIndex = m_pParent->GetItemIndex();
	switch(::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
		UpdateEdge(str, value);
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
		UpdateBlob(str, value);
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
		UpdateBlob2(str, value);
		break;
	}

	return 0;
}

void CDlgAlgoResult::InitPropertyGrid()
{
	m_nCurItemIndex = m_pParent->GetItemIndex();
	switch(::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
		InitPropertyGridEdge();
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
		InitPropertyGridBlob();
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
		InitPropertyGridBlob2();
		break;
	}

	UpdateData(FALSE);
}

void CDlgAlgoResult::InitPropertyGridEdge()
{
	CString strTemp;

	// Data 갱신을 위한 초기화
	m_propertyGridResult.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridResult.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sResult;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Edge Offset.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupIntersectionPointOffset = new CMFCPropertyGridProperty(_T("Intersection Point Offset"));
	m_propertyGridResult.AddProperty(pGroupIntersectionPointOffset);

	CMFCPropertyGridProperty* pGroupEdgeStartPtXOffset = new CMFCPropertyGridProperty(_T("Edge Start Pt. X Offset(mm)"), params.fEdgeStartPtXOffset, _T(""));
	pGroupIntersectionPointOffset->AddSubItem(pGroupEdgeStartPtXOffset);

	CMFCPropertyGridProperty* pGroupEdgeStartPtYOffset = new CMFCPropertyGridProperty(_T("Edge Start Pt. Y Offset(mm)"), params.fEdgeStartPtYOffset, _T(""));
	pGroupIntersectionPointOffset->AddSubItem(pGroupEdgeStartPtYOffset);

	CMFCPropertyGridProperty* pGroupEdgeEndPtXOffset = new CMFCPropertyGridProperty(_T("Edge End Pt. X Offset(mm)"), params.fEdgeEndPtXOffset, _T(""));
	pGroupIntersectionPointOffset->AddSubItem(pGroupEdgeEndPtXOffset);

	CMFCPropertyGridProperty* pGroupEdgeEndPtYOffset = new CMFCPropertyGridProperty(_T("Edge End Pt. Y Offset(mm)"), params.fEdgeEndPtYOffset, _T(""));
	pGroupIntersectionPointOffset->AddSubItem(pGroupEdgeEndPtYOffset);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridResult.AddProperty(pGroupEtc);
}

void CDlgAlgoResult::InitPropertyGridBlob()
{
	CString strTemp;

	// Data 갱신을 위한 초기화
	m_propertyGridResult.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridResult.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sResult;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Blob Offset.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupBlobOffset = new CMFCPropertyGridProperty(_T("Blob Offset"));
	m_propertyGridResult.AddProperty(pGroupBlobOffset);

	CMFCPropertyGridProperty* pGroupBlobStartPtXOffset = new CMFCPropertyGridProperty(_T("Blob Start PT. X Offset"), params.fBlobStartPtXOffset, _T(""));
	pGroupBlobOffset->AddSubItem(pGroupBlobStartPtXOffset);
	CMFCPropertyGridProperty* pGroupBlobStartPtYOffset = new CMFCPropertyGridProperty(_T("Blob Start PT. Y Offset"), params.fBlobStartPtYOffset, _T(""));
	pGroupBlobOffset->AddSubItem(pGroupBlobStartPtYOffset);
	CMFCPropertyGridProperty* pGroupBlobEndPtXOffset = new CMFCPropertyGridProperty(_T("Blob End PT. X Offset"), params.fBlobEndPtXOffset, _T(""));
	pGroupBlobOffset->AddSubItem(pGroupBlobEndPtXOffset);
	CMFCPropertyGridProperty* pGroupBlobEndPtYOffset = new CMFCPropertyGridProperty(_T("Blob End PT. Y Offset"), params.fBlobEndPtYOffset, _T(""));
	pGroupBlobOffset->AddSubItem(pGroupBlobEndPtYOffset);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridResult.AddProperty(pGroupEtc);
}

void CDlgAlgoResult::InitPropertyGridBlob2()
{
	CString strTemp;

	// Data 갱신을 위한 초기화
	m_propertyGridResult.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridResult.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sResult;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Result
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupResult = new CMFCPropertyGridProperty(_T("Result"));
	m_propertyGridResult.AddProperty(pGroupResult);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridResult.AddProperty(pGroupEtc);
}

void CDlgAlgoResult::UpdateBlob(CString str, _variant_t value)
{
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sResult;

	if (str == _T("Blob Start PT. X Offset"))
	{
		params.fBlobStartPtXOffset = (float)value;
	}
	else if (str == _T("Blob Start PT. Y Offset"))
	{
		params.fBlobStartPtYOffset = (float)value;
	}
	else if (str == _T("Blob End PT. X Offset"))
	{
		params.fBlobEndPtXOffset = (float)value;
	}
	else if (str == _T("Blob End PT. Y Offset"))
	{
		params.fBlobEndPtYOffset = (float)value;
	}
}

void CDlgAlgoResult::UpdateBlob2(CString str, _variant_t value)
{
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sResult;

	//if(str == _T("Blob dx Offset"))
	//{
	//	params.fBlobdxOffset = (float)value;
	//}
	//else if(str == _T("Blob dy Offset"))
	//{
	//	params.fBlobdyOffset = (float)value;
	//}
}

void CDlgAlgoResult::UpdateEdge(CString str, _variant_t value)
{
	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sResult;

	if(str == _T("Edge Start Pt. X Offset(mm)"))
	{
		params.fEdgeStartPtXOffset = (float)value;
	}
	else if(str == _T("Edge Start Pt. Y Offset(mm)"))
	{
		params.fEdgeStartPtYOffset = (float)value;
	}
	else if(str == _T("Edge End Pt. X Offset(mm)"))
	{
		params.fEdgeEndPtXOffset = (float)value;
	}
	else if(str == _T("Edge End Pt. Y Offset(mm)"))
	{
		params.fEdgeEndPtYOffset = (float)value;
	}
}


void CDlgAlgoResult::SetResult(long algoSequence)
{
	m_nCurItemIndex = m_pParent->GetItemIndex();

	switch(algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
		::g_InspectionManager.m_pInspection->DoSetROI(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoThresholdImage(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoDetectBlob(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoEdgeDetect(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoResult(m_nCurItemIndex);
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
		::g_InspectionManager.m_pInspection->DoSetROI(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoThresholdImage(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoDetectBlob(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoResult(m_nCurItemIndex);
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
		::g_InspectionManager.m_pInspection->DoSetROI(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoThresholdImage(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoDetectBlob(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoDetectBlob2(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoResult(m_nCurItemIndex);
		break;
	default:
		break;
	}
}
