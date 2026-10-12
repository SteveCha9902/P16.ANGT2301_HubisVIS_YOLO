// DlgAlgoThreshold.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgAlgoThreshold.h"
#include "afxdialogex.h"

#include "DlgSetup.h"
#include "DlgCalibration.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgAlgoThreshold 대화 상자

IMPLEMENT_DYNAMIC(CDlgAlgoThreshold, CDialogEx)

CDlgAlgoThreshold::CDlgAlgoThreshold(CDlgSetup* pParent, CDlgCalibration* pCalibration /*=nullptr*/)
	: CDialogEx(IDD_DLG_ALGO_THRESHOLD, pParent)
	, m_pParent(pParent)
	, m_pCalibration(pCalibration)
{
	if (m_pParent != nullptr)
	{
		m_pImageDisplay = pParent->m_pImageDisplay;
		m_nImageWidth = pParent->m_nImageWidth;
		m_nImageHeight = pParent->m_nImageHeight;
		m_nImageSize = pParent->m_nImageSize;
		m_nCurItemIndex = pParent->m_nLastestSelectedInspItemIndex;
	}
	else
	{
		m_pImageDisplay = m_pCalibration->m_pImageDisplay;
		m_nImageWidth = m_pCalibration->m_nImageWidth;
		m_nImageHeight = m_pCalibration->m_nImageHeight;
		m_nImageSize = m_pCalibration->m_nImageSize;
		m_nCurItemIndex = 0;
	}
}

CDlgAlgoThreshold::~CDlgAlgoThreshold()
{
}

void CDlgAlgoThreshold::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCROLLBAR_THRESHOLD, m_scrollbarThreshold);
	DDX_Control(pDX, IDC_SCROLLBAR_THRESHOLD_HIGH, m_scrollbarThreshold_High);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_THRESHOLD, m_propertyGridThreshold);
}

BEGIN_MESSAGE_MAP(CDlgAlgoThreshold, CDialogEx)
	ON_WM_HSCROLL()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgAlgoThreshold::OnAfxWmPropertyChanged)
END_MESSAGE_MAP()

// CDlgAlgoThreshold 메시지 처리기

BOOL CDlgAlgoThreshold::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitPropertyGrid();
	UpdateUI();
	UpdateParameters();

	return TRUE;
}

void CDlgAlgoThreshold::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar * pScrollBar)
{
	int nID = pScrollBar->GetDlgCtrlID();
	SThresholdParams* params;

	// Inspection Item 통합 - SHK 2023.12.27
	if (m_pParent != nullptr)
	{
		m_nCurItemIndex = m_pParent->GetItemIndex();
		params = &::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold;
	}
	else
	{
		params = &m_pCalibration->m_sThresholdParams;
	}

	UpdateData(TRUE);

	int step = 1;
	int Mstep = 10;
	int nNext, nmin, nmax;

	pScrollBar->GetScrollRange(&nmin, &nmax);

	switch (nSBCode)
	{
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNext = nPos;
		if (nNext > nmax) nNext = nmax;
		else if (nNext < nmin) nNext = nmin;
		pScrollBar->SetScrollPos(nNext);
		break;
	case SB_LINEDOWN:
		nNext = pScrollBar->GetScrollPos();
		if (nNext + step <= nmax) {
			pScrollBar->SetScrollPos(nNext + step);
			nNext += step;
		}
		break;
	case SB_LINEUP:
		nNext = pScrollBar->GetScrollPos();
		if (nNext - step >= nmin) {
			pScrollBar->SetScrollPos(nNext - step);
			nNext -= step;
		}
		break;
	case SB_PAGEDOWN:
		nNext = pScrollBar->GetScrollPos();
		if (nNext + Mstep <= nmax) {
			pScrollBar->SetScrollPos(nNext + Mstep);
			nNext += Mstep;
		}
		else {
			nNext = nmax;
			pScrollBar->SetScrollPos(nNext);
		}
		break;
	case SB_PAGEUP:
		nNext = pScrollBar->GetScrollPos();
		if (nNext - Mstep >= nmin) {
			pScrollBar->SetScrollPos(nNext - Mstep);
			nNext -= Mstep;
		}
		else {
			nNext = nmin;
			pScrollBar->SetScrollPos(nNext);
		}
		break;
	default:
		return;
		break;
	}

	int pos = pScrollBar->GetScrollPos();

	switch (nID)
	{
	case IDC_SCROLLBAR_THRESHOLD:
		if (params->nSelectThreshold == (long)THRESHOLD::NORMAL)
		{
			params->nThresholdValue = pos;
			m_scrollbarThreshold.SetScrollPos(params->nThresholdValue, TRUE);
			SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nThresholdValue);
		}
		else if (params->nSelectThreshold == (long)THRESHOLD::ADAPTIVE)
		{
			params->nAdaptiveOffset = pos;
			m_scrollbarThreshold.SetScrollPos(params->nAdaptiveOffset, TRUE);
			SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nAdaptiveOffset);
		}
		else if (params->nSelectThreshold == (long)THRESHOLD::HYSTERISIS)
		{
			params->nHysteresisLow = pos;
			m_scrollbarThreshold.SetScrollPos(params->nHysteresisLow, TRUE);
			SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nHysteresisLow);
		}
		break;
	case IDC_SCROLLBAR_THRESHOLD_HIGH:
		if (params->nSelectThreshold == (long)THRESHOLD::HYSTERISIS)
		{
			params->nHysteresisHigh = pos;
			m_scrollbarThreshold_High.SetScrollPos(params->nHysteresisHigh, TRUE);
			SetDlgItemInt(IDC_EDIT_THRESHOLD_HIGH, params->nHysteresisHigh);
		}
		break;
	}

	//UpdateImage();

	UpdateData(FALSE);

	Invalidate(TRUE);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CDlgAlgoThreshold::UpdateImage()
{
	if (m_pParent != nullptr)
	{
		// Use the same processing path as the inspection. The old preview code
		// always applied the Normal threshold formula regardless of Method.
		m_nCurItemIndex = m_pParent->GetItemIndex();
		::g_InspectionManager.m_pInspection->DoSetROI(m_nCurItemIndex);
		::g_InspectionManager.m_pInspection->DoThresholdImage(m_nCurItemIndex);
		m_pImageDisplay->SetImage(m_pParent->m_pbyThreshold, m_nImageWidth, m_nImageHeight, m_nImageWidth);
	}
	else if (m_pCalibration != nullptr)
	{
		BYTE* src = m_pCalibration->m_pbyImage;
		BYTE* dst = m_pCalibration->m_pbyThreshold;
		const SThresholdParams& params = m_pCalibration->m_sThresholdParams;

		memset(dst, 0, m_nImageSize);
		if (params.bUseThreshold == TRUE)
		{
			::g_Algorithm.DoThresholdImage(src, m_nImageWidth, m_nImageHeight,
				m_pCalibration->m_sROIParams, params, dst, FALSE);
		}
		else
		{
			memcpy(dst, src, m_nImageSize);
		}

		m_pImageDisplay->SetImage(dst, m_nImageWidth, m_nImageHeight, m_nImageWidth);
	}
}

void CDlgAlgoThreshold::UpdateUI()
{
	CString strTemp;
	SThresholdParams* params;

	// Inspection Item 통합 - SHK 2023.12.27
	if (m_pParent != nullptr)
	{
		m_nCurItemIndex = m_pParent->GetItemIndex();
		params = &::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold;
	}
	else
	{
		params = &m_pCalibration->m_sThresholdParams;
	}

	UpdateData(TRUE);

	if (params->nSelectThreshold == (long)THRESHOLD::NORMAL)
	{
		m_scrollbarThreshold.SetScrollRange(0, 255);
		m_scrollbarThreshold.SetScrollPos(params->nThresholdValue, TRUE);
		SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nThresholdValue);

		GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
	}
	else if (params->nSelectThreshold == (long)THRESHOLD::ADAPTIVE)
	{
		m_scrollbarThreshold.SetScrollRange(-255, 255);
		m_scrollbarThreshold.SetScrollPos(params->nAdaptiveOffset, TRUE);
		SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nAdaptiveOffset);

		GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
	}
	else if (params->nSelectThreshold == (long)THRESHOLD::HYSTERISIS)
	{
		m_scrollbarThreshold.SetScrollRange(0, 255);
		m_scrollbarThreshold.SetScrollPos(params->nHysteresisLow, TRUE);
		SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nHysteresisLow);

		m_scrollbarThreshold_High.SetScrollRange(0, 255);
		m_scrollbarThreshold_High.SetScrollPos(params->nHysteresisHigh, TRUE);
		SetDlgItemInt(IDC_EDIT_THRESHOLD_HIGH, params->nHysteresisHigh);

		GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(TRUE);
	}
	else if (params->nSelectThreshold == (long)THRESHOLD::OTSU || params->nSelectThreshold == (long)THRESHOLD::TRIANGLE)
	{
		// Otsu 또는 Triangle 선택 시 UI 요소 비활성화
		GetDlgItem(IDC_SCROLLBAR_THRESHOLD)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_THRESHOLD)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
	}
	else if (params->nSelectThreshold == (long)THRESHOLD::VIGNETTE)
	{
		// 비네팅 효과 선택 시 슬라이더 비활성화
		GetDlgItem(IDC_SCROLLBAR_THRESHOLD)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_THRESHOLD)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
	}

	//UpdateImage();

	UpdateData(FALSE);
}

void CDlgAlgoThreshold::UpdateParameters()
{
	UpdateData(TRUE);

	SThresholdParams params;

	// Inspection Item 통합 - SHK 2023.12.27
	if (m_pParent != nullptr)
	{
		m_nCurItemIndex = m_pParent->GetItemIndex();
		params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold;
	}
	else
	{
		params = m_pCalibration->m_sThresholdParams;
	}

	if (params.nSelectThreshold == (long)THRESHOLD::NORMAL)
	{
		params.nThresholdValue = m_scrollbarThreshold.GetScrollPos();
	}
	else if (params.nSelectThreshold == (long)THRESHOLD::ADAPTIVE)
	{
		params.nAdaptiveOffset = m_scrollbarThreshold.GetScrollPos();
	}
	else if (params.nSelectThreshold == (long)THRESHOLD::HYSTERISIS)
	{
		params.nHysteresisLow = m_scrollbarThreshold.GetScrollPos();
		params.nHysteresisHigh = m_scrollbarThreshold_High.GetScrollPos();
	}
}

LRESULT CDlgAlgoThreshold::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	CMFCPropertyGridProperty *pProp = (CMFCPropertyGridProperty*)lParam;

	if(pProp == nullptr)
		return 0;

	CString strTemp;
	CString str = pProp->GetName();
	_variant_t value = pProp->GetValue();

	SThresholdParams* params;

	if (m_pParent != nullptr)
	{
		m_nCurItemIndex = m_pParent->GetItemIndex();
		params = &::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold;
	}
	else
	{
		params = &m_pCalibration->m_sThresholdParams;
	}
	
	if (str == _T("Use Threshold"))
	{
		params->bUseThreshold = (CString)value == _T("TRUE") ? TRUE : FALSE;
	}
	else if(str == _T("Method"))
	{
		if((CString)value == _T("Normal"))
		{
			params->nSelectThreshold = (long)THRESHOLD::NORMAL;

			m_scrollbarThreshold.SetScrollRange(0, 255);
			m_scrollbarThreshold.SetScrollPos(params->nThresholdValue, TRUE);
			SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nThresholdValue);

			GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
		}
		else if((CString)value == _T("Adaptive"))
		{
			params->nSelectThreshold = (long)THRESHOLD::ADAPTIVE;

			m_scrollbarThreshold.SetScrollRange(-255, 255);
			m_scrollbarThreshold.SetScrollPos(params->nAdaptiveOffset, TRUE);
			SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nAdaptiveOffset);

			GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
		}
		else if((CString)value == _T("Hysteresis"))
		{
			params->nSelectThreshold = (long)THRESHOLD::HYSTERISIS;

			m_scrollbarThreshold.SetScrollRange(0, 255);
			m_scrollbarThreshold.SetScrollPos(params->nHysteresisLow, TRUE);
			SetDlgItemInt(IDC_EDIT_THRESHOLD, params->nHysteresisLow);

			m_scrollbarThreshold_High.SetScrollRange(0, 255);
			m_scrollbarThreshold_High.SetScrollPos(params->nHysteresisHigh, TRUE);
			SetDlgItemInt(IDC_EDIT_THRESHOLD_HIGH, params->nHysteresisHigh);

			GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(TRUE);
		}
		else if ((CString)value == _T("Otsu"))
		{
			params->nSelectThreshold = (long)THRESHOLD::OTSU;

			// UI 요소 비활성화
			GetDlgItem(IDC_SCROLLBAR_THRESHOLD)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_THRESHOLD)->EnableWindow(FALSE);
			GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
		}
		else if ((CString)value == _T("Triangle"))
		{
			params->nSelectThreshold = (long)THRESHOLD::TRIANGLE;

			// UI 요소 비활성화
			GetDlgItem(IDC_SCROLLBAR_THRESHOLD)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_THRESHOLD)->EnableWindow(FALSE);
			GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
	}
		else if ((CString)value == _T("Vignette"))
		{
			params->nSelectThreshold = (long)THRESHOLD::VIGNETTE;

			GetDlgItem(IDC_SCROLLBAR_THRESHOLD)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_THRESHOLD)->EnableWindow(FALSE);
			GetDlgItem(IDC_SCROLLBAR_THRESHOLD_HIGH)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_THRESHOLD_HIGH)->EnableWindow(FALSE);
		}
	}
	else if (str == _T("Vignette Strength"))
	{
		float fStrength = (float)value;
		if (fStrength < 0.0f)
			fStrength = 0.0f;
		else if (fStrength > 10.0f)
			fStrength = 10.0f;

		params->fVignetteStrength = fStrength;
		pProp->SetValue((_variant_t)fStrength);
		::g_Log.Log(_T("[VIGNETTE_DIAG] Item:%d UseThreshold:%d Method:%d Strength:%.3f"),
			m_nCurItemIndex, params->bUseThreshold, params->nSelectThreshold, params->fVignetteStrength);
	}

	else if (str == _T("Use Re-Inspection"))
	{
		params->bUseReInspection = (CString)value == _T("TRUE") ? TRUE : FALSE; // PSJ
	}

	else if (str == _T("Use Adaptive Threshold"))
	{
		params->bUseAdaptiveThreshold = (CString)value == _T("TRUE") ? TRUE : FALSE; // PSJ
	}

	else if(str == _T("Reverse"))
	{
		params->bThresholdReverse = (CString)value == _T("TRUE") ? TRUE : FALSE;
	}
	else if(str == _T("High Threshold Region Noise Filtering Size"))
	{
		params->nHighThresholdRegionNoiseFilteringSize = (long)value;
	}

	else if (str == _T("Left Margin"))
	{
		params->nLeftMargin = (long)value;
		::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold.nLeftMargin = params->nLeftMargin;
	}
	else if (str == _T("Right Margin"))
	{
		params->nRightMargin = (long)value;
		::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold.nRightMargin = params->nRightMargin;
	}
	else if (str == _T("Threshold Offset"))
	{
		params->nThresholdOffset = (long)value;
		::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold.nThresholdOffset = params->nThresholdOffset;
		::g_Log.Log(_T("[THRESHOLD_GRADIENT_2D] Item:%d Base:%d AxisEdgeOffset:%d CornerOffset:%d"),
			m_nCurItemIndex, params->nThresholdValue, params->nThresholdOffset,
			params->nThresholdOffset * 2);
	}
	else if (str == _T("Re-Inspection Offset"))
	{
	params->nReInspectionOffset = (long)value;
	::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold.nReInspectionOffset = params->nReInspectionOffset;
	}
	else if (str == _T("Adaptive Threshold Offset"))
	{
	params->nAdaptiveThresholdOffset = (long)value;
	::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold.nAdaptiveThresholdOffset = params->nAdaptiveThresholdOffset;
	}

	UpdateImage();

	return 0;
}

void CDlgAlgoThreshold::InitPropertyGrid()
{
	CString strTemp;

	m_propertyGridThreshold.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridThreshold.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	SThresholdParams params;

	// Inspection Item 통합 - SHK 2023.12.27
	if (m_pParent != nullptr)
	{
		m_nCurItemIndex = m_pParent->GetItemIndex();
		params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sThreshold;
	}
	else
	{
		params = m_pCalibration->m_sThresholdParams;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Threshold
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupThreshold = new CMFCPropertyGridProperty(_T("Threshold"));
	m_propertyGridThreshold.AddProperty(pGroupThreshold);
		
	strTemp = params.bUseThreshold == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupThreshold_UseThreshold = new CMFCPropertyGridProperty(_T("Use Threshold"), strTemp, _T(""));
	pGroupThreshold_UseThreshold->AddOption(_T("TRUE"));
	pGroupThreshold_UseThreshold->AddOption(_T("FALSE"));
	pGroupThreshold->AddSubItem(pGroupThreshold_UseThreshold);

	CString strMethod;
	switch(params.nSelectThreshold)
	{
	case (long)THRESHOLD::NORMAL:
		strMethod = _T("Normal");
		break;
	case (long)THRESHOLD::ADAPTIVE:
		strMethod = _T("Adaptive");
		break;
	case (long)THRESHOLD::HYSTERISIS:
		strMethod = _T("Hysterisis");
		break;
	case (long)THRESHOLD::OTSU:
		strMethod = _T("Otsu");
		break;
	case (long)THRESHOLD::TRIANGLE:
		strMethod = _T("Triangle");
		break;
	case (long)THRESHOLD::VIGNETTE:
		strMethod = _T("Vignette");
		break;
	}

	CMFCPropertyGridProperty* pGroupThreshold_Method = new CMFCPropertyGridProperty(_T("Method"), strMethod, _T(""));
	pGroupThreshold_Method->AddOption(_T("Normal"));
	pGroupThreshold_Method->AddOption(_T("Adaptive"));
	pGroupThreshold_Method->AddOption(_T("Hysteresis"));
	pGroupThreshold_Method->AddOption(_T("Otsu"));
	pGroupThreshold_Method->AddOption(_T("Triangle"));
	pGroupThreshold_Method->AddOption(_T("Vignette"));
	pGroupThreshold->AddSubItem(pGroupThreshold_Method);

	strTemp = params.bThresholdReverse == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupThreshold_Reverse = new CMFCPropertyGridProperty(_T("Reverse"), strTemp, _T(""));
	pGroupThreshold_Reverse->AddOption(_T("TRUE"));
	pGroupThreshold_Reverse->AddOption(_T("FALSE"));
	pGroupThreshold->AddSubItem(pGroupThreshold_Reverse);

	CMFCPropertyGridProperty* pGroupThreshold_HighThresholdRegionNoiseFilteringSize = new CMFCPropertyGridProperty(_T("High Threshold Region Noise Filtering Size"), params.nHighThresholdRegionNoiseFilteringSize, _T(""));
	pGroupThreshold->AddSubItem(pGroupThreshold_HighThresholdRegionNoiseFilteringSize);

	// Add properties for LeftMargin
	CMFCPropertyGridProperty* pGroupThreshold_LeftMargin = new CMFCPropertyGridProperty(_T("Left Margin"), params.nLeftMargin, _T("Set the left margin for threshold adjustment."));
	pGroupThreshold->AddSubItem(pGroupThreshold_LeftMargin);

	// Add properties for Rigit Margin
	CMFCPropertyGridProperty* pGroupThreshold_RightMargin = new CMFCPropertyGridProperty(_T("Right Margin"), params.nRightMargin, _T("Set the right margin for threshold adjustment."));
	pGroupThreshold->AddSubItem(pGroupThreshold_RightMargin);

	CMFCPropertyGridProperty* pGroupThreshold_ThresholdOffset = new CMFCPropertyGridProperty(_T("Threshold Offset"), params.nThresholdOffset, _T("Set the offset value for side threshold adjustment."));
	pGroupThreshold->AddSubItem(pGroupThreshold_ThresholdOffset);

	CMFCPropertyGridProperty* pGroupThreshold_VignetteStrength = new CMFCPropertyGridProperty(_T("Vignette Strength"), params.fVignetteStrength, _T("Set the strength of the vignette effect."));
	pGroupThreshold->AddSubItem(pGroupThreshold_VignetteStrength);

	// PSJ
	strTemp = params.bUseReInspection == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupThreshold_ReInspection = new CMFCPropertyGridProperty(_T("Use Re-Inspection"), strTemp, _T("Set Using Re-Inspection."));
	pGroupThreshold_ReInspection->AddOption(_T("TRUE"));
	pGroupThreshold_ReInspection->AddOption(_T("FALSE"));
	pGroupThreshold->AddSubItem(pGroupThreshold_ReInspection);

	// PSJ
	CMFCPropertyGridProperty* pGroupThreshold_ReInspectionRatio = new CMFCPropertyGridProperty(_T("Re-Inspection Offset"), params.nReInspectionOffset, _T("Set the Re-Inspection Offset."));
	pGroupThreshold->AddSubItem(pGroupThreshold_ReInspectionRatio);

	// 250313 Adaptive Threshold PSJ
	strTemp = params.bUseAdaptiveThreshold == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupThreshold_AdaptiveThreshold = new CMFCPropertyGridProperty(_T("Use Adaptive Threshold"), strTemp, _T("Set Using Adaptive Threshold."));
	pGroupThreshold_AdaptiveThreshold->AddOption(_T("TRUE"));
	pGroupThreshold_AdaptiveThreshold->AddOption(_T("FALSE"));
	pGroupThreshold->AddSubItem(pGroupThreshold_AdaptiveThreshold);

	// 250313 Adaptive Threshold PSJ
	CMFCPropertyGridProperty* pGroupThreshold_AdaptiveThresholdRatio = new CMFCPropertyGridProperty(_T("Adaptive Threshold Offset"), params.nAdaptiveThresholdOffset, _T("Set the Adaptive Threshold Offset."));
	pGroupThreshold->AddSubItem(pGroupThreshold_AdaptiveThresholdRatio);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridThreshold.AddProperty(pGroupEtc);
}
