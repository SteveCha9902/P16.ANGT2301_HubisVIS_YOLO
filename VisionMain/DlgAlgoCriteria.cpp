// DlgAlgoCriteria.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgAlgoCriteria.h"
#include "afxdialogex.h"

#include "DlgSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgAlgoCriteria 대화 상자

IMPLEMENT_DYNAMIC(CDlgAlgoCriteria, CDialogEx)

CDlgAlgoCriteria::CDlgAlgoCriteria(CDlgSetup* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_ALGO_CRITERIA, pParent)
	, m_pParent(pParent)
	, m_pImageDisplay(pParent->m_pImageDisplay)
	, m_nImageWidth(pParent->m_nImageWidth)
	, m_nImageHeight(pParent->m_nImageHeight)
	, m_nImageSize(pParent->m_nImageSize)
	, m_nCurItemIndex(pParent->m_nLastestSelectedInspItemIndex)
{

}

CDlgAlgoCriteria::~CDlgAlgoCriteria()
{
}

void CDlgAlgoCriteria::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_CRITERIA, m_propertyGridCriteria);
}


BEGIN_MESSAGE_MAP(CDlgAlgoCriteria, CDialogEx)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgAlgoCriteria::OnAfxWmPropertyChanged)
END_MESSAGE_MAP()


// CDlgAlgoCriteria 메시지 처리기


BOOL CDlgAlgoCriteria::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitPropertyGrid();

	return TRUE;
}

LRESULT CDlgAlgoCriteria::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
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
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			UpdateEdge(str, value);
		}
		break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			UpdateBlob(str, value);
		}
		break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			UpdateBlob2(str, value);
		}
		break;
		}
	}
	break;
	}

	return 0;
}

void CDlgAlgoCriteria::InitPropertyGrid()
{
	m_nCurItemIndex = m_pParent->GetItemIndex();
	switch(::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			InitPropertyGridEdge();
		}
		break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			InitPropertyGridBlob();
		}
		break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			InitPropertyGridBlob2();
		}
		break;
		}
	}
	break;
	}

	UpdateData(FALSE);
}

void CDlgAlgoCriteria::InitPropertyGridBlob()
{
	CString strTemp;

	// Data 갱신을 위한 초기화
	m_propertyGridCriteria.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridCriteria.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sCriteria;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Blob Criteria.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupBlob = new CMFCPropertyGridProperty(_T("Blob Criteria"));
	m_propertyGridCriteria.AddProperty(pGroupBlob);

	CMFCPropertyGridProperty* pGroupBlob_BlobWidthMin = new CMFCPropertyGridProperty(_T("Blob Width Min"), params.nBlobWidthMin, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_BlobWidthMin);

	CMFCPropertyGridProperty* pGroupBlob_BlobWidthMax = new CMFCPropertyGridProperty(_T("Blob Width Max"), params.nBlobWidthMax, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_BlobWidthMax);

	CMFCPropertyGridProperty* pGroupBlob_BlobHeightMin = new CMFCPropertyGridProperty(_T("Blob Height Min"), params.nBlobHeightMin, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_BlobHeightMin);

	CMFCPropertyGridProperty* pGroupBlob_BlobHeightMax = new CMFCPropertyGridProperty(_T("Blob Height Max"), params.nBlobHeightMax, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_BlobHeightMax);

	// Between 검사항목 표기 제외 PSJ
	/*CMFCPropertyGridProperty* pGroupBlob_BetweenDistMinX = new CMFCPropertyGridProperty(_T("Between Dist. Min. X(mm)"), params.fBetweenDistMinX, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_BetweenDistMinX);

	CMFCPropertyGridProperty* pGroupBlob_BetweenDistMaxX = new CMFCPropertyGridProperty(_T("Between Dist. Max. X(mm)"), params.fBetweenDistMaxX, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_BetweenDistMaxX);

	CMFCPropertyGridProperty* pGroupBlob_BetweenDistMinY = new CMFCPropertyGridProperty(_T("Between Dist. Min. Y(mm)"), params.fBetweenDistMinY, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_BetweenDistMinY);

	CMFCPropertyGridProperty* pGroupBlob_BetweenDistMaxY = new CMFCPropertyGridProperty(_T("Between Dist. Max. Y(mm)"), params.fBetweenDistMaxY, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_BetweenDistMaxY);*/

	CMFCPropertyGridProperty* pGroupBlob_LengthMin = new CMFCPropertyGridProperty(_T("Length Min(mm)"), params.fLengthMin, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_LengthMin);

	CMFCPropertyGridProperty* pGroupBlob_LengthMax = new CMFCPropertyGridProperty(_T("Length Max(mm)"), params.fLengthMax, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_LengthMax);

	CMFCPropertyGridProperty* pGroupBlob_AngleMin = new CMFCPropertyGridProperty(_T("Angle Min(mm)"), params.fAngleMin, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_AngleMin);

	CMFCPropertyGridProperty* pGroupBlob_AngleMax = new CMFCPropertyGridProperty(_T("Angle Max(mm)"), params.fAngleMax, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_AngleMax);

	CMFCPropertyGridProperty* pGroupBlob_TopBusbarHeightMin = new CMFCPropertyGridProperty(_T("Top Busbar Height Min(mm)"), params.fTopBusbarHeightMin, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_TopBusbarHeightMin);

	CMFCPropertyGridProperty* pGroupBlob_TopBusbarHeightMax = new CMFCPropertyGridProperty(_T("Top Busbar Height Max(mm)"), params.fTopBusbarHeightMax, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_TopBusbarHeightMax);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridCriteria.AddProperty(pGroupEtc);
}

void CDlgAlgoCriteria::InitPropertyGridBlob2()
{
	CString strTemp;

	// Data 갱신을 위한 초기화
	m_propertyGridCriteria.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridCriteria.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sCriteria;
	//params.Init();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Weld Criteria.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupWeld = new CMFCPropertyGridProperty(_T("Weld Criteria"));
	m_propertyGridCriteria.AddProperty(pGroupWeld);

	CMFCPropertyGridProperty* pGroupWeld_WeldWidthMin = new CMFCPropertyGridProperty(_T("Weld Width Min"), params.fWeldWidthMin, _T(""));
	pGroupWeld->AddSubItem(pGroupWeld_WeldWidthMin);

	CMFCPropertyGridProperty* pGroupWeld_WeldWidthMax = new CMFCPropertyGridProperty(_T("Weld Width Max"), params.fWeldWidthMax, _T(""));
	pGroupWeld->AddSubItem(pGroupWeld_WeldWidthMax);

	CMFCPropertyGridProperty* pGroupWeld_WeldHeightMin = new CMFCPropertyGridProperty(_T("Weld Height Min"), params.fWeldHeightMin, _T(""));
	pGroupWeld->AddSubItem(pGroupWeld_WeldHeightMin);

	CMFCPropertyGridProperty* pGroupWeld_WeldHeightMax = new CMFCPropertyGridProperty(_T("Weld Height Max"), params.fWeldHeightMax, _T(""));
	pGroupWeld->AddSubItem(pGroupWeld_WeldHeightMax);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridCriteria.AddProperty(pGroupEtc);
}

void CDlgAlgoCriteria::InitPropertyGridEdge()
{
	CString strTemp;

	// Data 갱신을 위한 초기화
	m_propertyGridCriteria.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridCriteria.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	m_nCurItemIndex = m_pParent->GetItemIndex();
	auto params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sCriteria;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Edge Criteria.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEdge = new CMFCPropertyGridProperty(_T("Edge Criteria"));
	m_propertyGridCriteria.AddProperty(pGroupEdge);

	CMFCPropertyGridProperty* pGroupEdge_BlobWidthMin = new CMFCPropertyGridProperty(_T("Blob Width Min"), params.nBlobWidthMin, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_BlobWidthMin);

	CMFCPropertyGridProperty* pGroupEdge_BlobWidthMax = new CMFCPropertyGridProperty(_T("Blob Width Max"), params.nBlobWidthMax, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_BlobWidthMax);

	CMFCPropertyGridProperty* pGroupEdge_BlobHeightMin = new CMFCPropertyGridProperty(_T("Blob Height Min"), params.nBlobHeightMin, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_BlobHeightMin);

	CMFCPropertyGridProperty* pGroupEdge_BlobHeightMax = new CMFCPropertyGridProperty(_T("Blob Height Max"), params.nBlobHeightMax, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_BlobHeightMax);

	// Between 검사항목 표기 제외 PSJ
	/*CMFCPropertyGridProperty* pGroupEdge_BetweenDistMinX = new CMFCPropertyGridProperty(_T("Between Dist. Min. X(mm)"), params.fBetweenDistMinX, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_BetweenDistMinX);

	CMFCPropertyGridProperty* pGroupEdge_BetweenDistMaxX = new CMFCPropertyGridProperty(_T("Between Dist. Max. X(mm)"), params.fBetweenDistMaxX, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_BetweenDistMaxX);

	CMFCPropertyGridProperty* pGroupEdge_BetweenDistMinY = new CMFCPropertyGridProperty(_T("Between Dist. Min. Y(mm)"), params.fBetweenDistMinY, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_BetweenDistMinY);

	CMFCPropertyGridProperty* pGroupEdge_BetweenDistMaxY = new CMFCPropertyGridProperty(_T("Between Dist. Max. Y(mm)"), params.fBetweenDistMaxY, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_BetweenDistMaxY);*/

	CMFCPropertyGridProperty* pGroupEdge_LengthMin = new CMFCPropertyGridProperty(_T("Length Min(mm)"), params.fLengthMin, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_LengthMin);

	CMFCPropertyGridProperty* pGroupEdge_LengthMax = new CMFCPropertyGridProperty(_T("Length Max(mm)"), params.fLengthMax, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_LengthMax);

	CMFCPropertyGridProperty* pGroupEdge_AngleMin = new CMFCPropertyGridProperty(_T("Angle Min(mm)"), params.fAngleMin, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_AngleMin);

	CMFCPropertyGridProperty* pGroupEdge_AngleMax = new CMFCPropertyGridProperty(_T("Angle Max(mm)"), params.fAngleMax, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_AngleMax);

	CMFCPropertyGridProperty* pGroupEdge_TopBusbarHeightMin = new CMFCPropertyGridProperty(_T("Top Busbar Height Min(mm)"), params.fTopBusbarHeightMin, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_TopBusbarHeightMin);

	CMFCPropertyGridProperty* pGroupEdge_TopBusbarHeightMax = new CMFCPropertyGridProperty(_T("Top Busbar Height Max(mm)"), params.fTopBusbarHeightMax, _T(""));
	pGroupEdge->AddSubItem(pGroupEdge_TopBusbarHeightMax);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridCriteria.AddProperty(pGroupEtc);
}

void CDlgAlgoCriteria::UpdateBlob(CString str, _variant_t value)
{
	m_nCurItemIndex = m_pParent->GetItemIndex();

	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sCriteria;

	if(str == _T("Blob Width Min"))
	{
		params.nBlobWidthMin = (long)value;
	}
	else if(str == _T("Blob Width Max"))
	{
		params.nBlobWidthMax = (long)value;
	}
	else if(str == _T("Blob Height Min"))
	{
		params.nBlobHeightMin = (long)value;
	}
	else if(str == _T("Blob Height Max"))
	{
		params.nBlobHeightMax = (long)value;
	}
	else if(str == _T("Between Dist. Min. X(mm)"))
	{
		params.fBetweenDistMinX = (float)value;
	}
	else if(str == _T("Between Dist. Max. X(mm)"))
	{
		params.fBetweenDistMaxX = (float)value;
	}
	else if(str == _T("Between Dist. Min. Y(mm)"))
	{
		params.fBetweenDistMinY = (float)value;
	}
	else if(str == _T("Between Dist. Max. Y(mm)"))
	{
		params.fBetweenDistMaxY = (float)value;
	}
	else if(str == _T("Length Min(mm)"))
	{
		params.fLengthMin = (float)value;
	}
	else if(str == _T("Length Max(mm)"))
	{
		params.fLengthMax = (float)value;
	}
	else if(str == _T("Angle Min(mm)"))
	{
		params.fAngleMin = (float)value;
	}
	else if(str == _T("Angle Max(mm)"))
	{
		params.fAngleMax = (float)value;
	}
	else if(str == _T("Top Busbar Height Min(mm)"))
	{
		params.fTopBusbarHeightMin = (float)value;
	}
	else if(str == _T("Top Busbar Height Max(mm)"))
	{
		params.fTopBusbarHeightMax = (float)value;
	}
}

void CDlgAlgoCriteria::UpdateBlob2(CString str, _variant_t value)
{
	m_nCurItemIndex = m_pParent->GetItemIndex();

	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sCriteria;

	if(str == _T("Weld Width Min"))
	{
		params.fWeldWidthMin = (float)value;
	}
	else if(str == _T("Weld Width Max"))
	{
		params.fWeldWidthMax = (float)value;
	}
	else if(str == _T("Weld Height Min"))
	{
		params.fWeldHeightMin = (float)value;
	}
	else if(str == _T("Weld Height Max"))
	{
		params.fWeldHeightMax = (float)value;
	}
}

void CDlgAlgoCriteria::UpdateEdge(CString str, _variant_t value)
{
	m_nCurItemIndex = m_pParent->GetItemIndex();

	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sCriteria;

	if(str == _T("Blob Width Min"))
	{
		params.nBlobWidthMin = (long)value;
	}
	else if(str == _T("Blob Width Max"))
	{
		params.nBlobWidthMax = (long)value;
	}
	else if(str == _T("Blob Height Min"))
	{
		params.nBlobHeightMin = (long)value;
	}
	else if(str == _T("Blob Height Max"))
	{
		params.nBlobHeightMax = (long)value;
	}
	else if(str == _T("Between Dist. Min. X(mm)"))
	{
		params.fBetweenDistMinX = (float)value;
	}
	else if(str == _T("Between Dist. Max. X(mm)"))
	{
		params.fBetweenDistMaxX = (float)value;
	}
	else if(str == _T("Between Dist. Min. Y(mm)"))
	{
		params.fBetweenDistMinY = (float)value;
	}
	else if(str == _T("Between Dist. Max. Y(mm)"))
	{
		params.fBetweenDistMaxY = (float)value;
	}
	else if(str == _T("Length Min(mm)"))
	{
		params.fLengthMin = (float)value;
	}
	else if(str == _T("Length Max(mm)"))
	{
		params.fLengthMax = (float)value;
	}
	else if(str == _T("Angle Min(mm)"))
	{
		params.fAngleMin = (float)value;
	}
	else if(str == _T("Angle Max(mm)"))
	{
		params.fAngleMax = (float)value;
	}
	else if(str == _T("Top Busbar Height Min(mm)"))
	{
		params.fTopBusbarHeightMin = (float)value;
	}
	else if(str == _T("Top Busbar Height Max(mm)"))
	{
		params.fTopBusbarHeightMax = (float)value;
	}
}


void CDlgAlgoCriteria::SetCriteria(long algoSequence, BOOL &bResult)
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
		::g_InspectionManager.m_pInspection->DoCriteria(m_nCurItemIndex, bResult);
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
		::g_InspectionManager.m_pInspection->DoSetROI(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoThresholdImage(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoDetectBlob(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoResult(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoCriteria(m_nCurItemIndex, bResult);
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
		::g_InspectionManager.m_pInspection->DoSetROI(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoThresholdImage(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoDetectBlob(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoDetectBlob2(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoResult(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoCriteria(m_nCurItemIndex, bResult);
		break;
	default:
		break;
	}
}
