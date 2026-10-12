// DlgLiveIllum.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgVisionMain.h"
#include "DlgInspItemSetup.h"
#include "afxdialogex.h"

#include "DlgMain1.h"
#include "DlgMain2.h"
#include "DlgNewInspectionItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgInspItemSetup 대화 상자

IMPLEMENT_DYNAMIC(CDlgInspItemSetup, CDialogEx)

CDlgInspItemSetup::CDlgInspItemSetup(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_INSP_ITEM_SETUP, pParent)
	, m_pParent(pParent)
	, m_pImageDisplay(nullptr)
	, m_nLastestSelectedInspItemIndex(-1)
{

}

CDlgInspItemSetup::~CDlgInspItemSetup()
{
	delete m_pImageDisplay;
}

void CDlgInspItemSetup::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_CAMERA, m_tabCamera);
	DDX_Control(pDX, IDC_SCROLLBAR_CH1, m_scrollbarCh[0]);
	DDX_Control(pDX, IDC_SCROLLBAR_CH2, m_scrollbarCh[1]);
	DDX_Control(pDX, IDC_SCROLLBAR_CH3, m_scrollbarCh[2]);
	DDX_Control(pDX, IDC_SCROLLBAR_CH4, m_scrollbarCh[3]);
	DDX_Control(pDX, IDC_LIST_INSPECTION_ITEM, m_listInspectionItem);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_ITEM_PROPERTY, m_propertyGridItemProperty);
}


BEGIN_MESSAGE_MAP(CDlgInspItemSetup, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BTN_LIVE, &CDlgInspItemSetup::OnBnClickedBtnLive)
	ON_BN_CLICKED(IDC_BTN_STOP, &CDlgInspItemSetup::OnBnClickedBtnStop)
	ON_BN_CLICKED(IDC_BTN_CH1_ZERO, &CDlgInspItemSetup::OnBnClickedBtnCh1Zero)
	ON_BN_CLICKED(IDC_BTN_CH1_FULL, &CDlgInspItemSetup::OnBnClickedBtnCh1Full)
	ON_BN_CLICKED(IDC_BTN_CH2_ZERO, &CDlgInspItemSetup::OnBnClickedBtnCh2Zero)
	ON_BN_CLICKED(IDC_BTN_CH2_FULL, &CDlgInspItemSetup::OnBnClickedBtnCh2Full)
	ON_BN_CLICKED(IDC_BTN_CH3_ZERO, &CDlgInspItemSetup::OnBnClickedBtnCh3Zero)
	ON_BN_CLICKED(IDC_BTN_CH3_FULL, &CDlgInspItemSetup::OnBnClickedBtnCh3Full)
	ON_BN_CLICKED(IDC_BTN_CH4_ZERO, &CDlgInspItemSetup::OnBnClickedBtnCh4Zero)
	ON_BN_CLICKED(IDC_BTN_CH4_FULL, &CDlgInspItemSetup::OnBnClickedBtnCh4Full)
	ON_BN_CLICKED(IDC_BTN_ILLUM_ON, &CDlgInspItemSetup::OnBnClickedBtnIllumOn)
	ON_BN_CLICKED(IDC_BTN_ILLUM_OFF, &CDlgInspItemSetup::OnBnClickedBtnIllumOff)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CAMERA, &CDlgInspItemSetup::OnTcnSelchangeTabCamera)
	ON_COMMAND(IDM_INSP_ITEM_ADD, &CDlgInspItemSetup::OnInspItemAdd)
	ON_COMMAND(IDM_INSP_ITEM_DEL, &CDlgInspItemSetup::OnInspItemDel)
	ON_COMMAND(IDM_INSP_ITEM_COPY, &CDlgInspItemSetup::OnInspItemCopy)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_INSPECTION_ITEM, &CDlgInspItemSetup::OnLvnItemchangedListInspectionItem)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_INSPECTION_ITEM, &CDlgInspItemSetup::OnNMRClickListInspectionItem)
	ON_BN_CLICKED(IDC_BTN_RECIPE_SAVE, &CDlgInspItemSetup::OnBnClickedBtnRecipeSave)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgInspItemSetup::OnAfxWmPropertyChanged)
END_MESSAGE_MAP()


// CDlgInspItemSetup 메시지 처리기

BOOL CDlgInspItemSetup::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 2D Viewer
	m_pImageDisplay = new CImageDisplay;
	m_pImageDisplay->Initialize(IDC_STATIC_LIVE, this, RGB(50, 50, 50));
	m_pImageDisplay->SetCrossLineColor(RGB(255, 0, 0));
	m_pImageDisplay->SetCustomLineColor(RGB(255, 0, 0));
	
	// UI 선택 따라 Camera Tab 추가
	for(long idx = 0; idx <= ::g_ConfigManager.m_nMainUI; idx++)
	{
		m_tabCamera.InsertItem(idx, ::g_ConfigManager.m_strCameraName[idx]);
	}

	// Live 화면 출력을 위한 Camera 추가 - SHK 2023.12.27
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
	{
		m_pCtrlCamera[0] = m_pParent->m_pDlgMain1->m_pCtrlCamera;
	}
	break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
	{
		for (long idx = 0; idx < 2; idx++)
		{
			m_pCtrlCamera[idx] = m_pParent->m_pDlgMain2->m_pCtrlCamera[idx];
		}
	}
	break;
	}

	m_tabCamera.SetCurSel(0);
	InitInspectionItemListControl();
	SetInspectionItemListControl();

	m_nLastestSelectedInspItemIndex = 0;

	auto inspItem = ::g_RecipeManager.m_vecsInspItem;

	// 선택한 Item만 Check 표시 하기
	for (long idx = 0; idx < (long)inspItem.size(); idx++)
	{
		if (m_nLastestSelectedInspItemIndex == idx)
		{
			m_listInspectionItem.SetCheck(idx, TRUE);
		}
		else
		{
			m_listInspectionItem.SetCheck(idx, FALSE);
		}
	}

	InitUI();
	InitPropertyGrid();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CDlgInspItemSetup::InitUI()
{
	UpdateData(TRUE);

	CString strTemp;

	if(m_nLastestSelectedInspItemIndex > -1)
	{
		auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];
		unsigned short *illum = nullptr;

		if(::g_ConfigManager.m_bMultiIllumination == TRUE)
		{
			illum = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex].sIllum.m_usIllum;
		}
		else
		{
			illum = ::g_RecipeManager.m_sIllum.m_usIllum;
		}

		for(long idx = 0; idx < ILLUM_CHANNEL_MAX; idx++)
		{
			m_scrollbarCh[idx].SetScrollRange(0, 255);
			m_scrollbarCh[idx].SetScrollPos(illum[idx], TRUE);
			SetDlgItemInt(IDC_EDIT_CH1 + idx, illum[idx]);
		}

		m_tabCamera.SetCurSel(inspItem.visionIndex);

		UpdateImageDisplay(); // Live 화면 출력- SHK 2023.12.27
	}

	UpdateData(FALSE);
}

LRESULT CDlgInspItemSetup::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	CMFCPropertyGridProperty *pProp = (CMFCPropertyGridProperty*)lParam;

	if(pProp == nullptr)
		return 0;

	CString strTemp;
	CString str = pProp->GetName();
	_variant_t value = pProp->GetValue();

	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];
	auto &busbarInfo = ::g_RecipeManager.m_sBusbarInfoParam;

	if(str == _T("Insp. Item Name"))
	{
		params.strName = (CString)value;
	}
	else if(str == _T("Vision Index"))
	{
		params.visionIndex = (long)value;
	}
	else if(str == _T("Division Index"))
	{
		params.divIndex = (long)value;
	}
	else if(str == _T("Algorithm Sequence"))
	{
		if((CString)value == _T("Edge Detect"))
		{
			params.algoSequence = (long)ALGORITHM_SEQUENCE::EDGE_DETECT;
		}
		else if((CString)value == _T("Detect Blob"))
		{
			params.algoSequence = (long)ALGORITHM_SEQUENCE::DETECT_BLOB;
		}
		else if((CString)value == _T("Detect Blob2"))
		{
			params.algoSequence = (long)ALGORITHM_SEQUENCE::DETECT_BLOB2;
		}
	}
	else if(str == _T("Inspection Division"))
	{
		if((CString)value == _T("NONE"))
		{
			busbarInfo.nInspectionDivision = (long)INSPECTION_DIVISION::NONE;
		}
		else if((CString)value == _T("BEFORE"))
		{
			busbarInfo.nInspectionDivision = (long)INSPECTION_DIVISION::BEFORE;
		}
		else if((CString)value == _T("AFTER"))
		{
			busbarInfo.nInspectionDivision = (long)INSPECTION_DIVISION::AFTER;
		}

		m_pParent->m_pDlgMain2->SetResultGrid();
	}
	else if(str == _T("DC Count"))
	{
		busbarInfo.nDCCount = (long)value;
	}
	else if(str == _T("AC Count"))
	{
		busbarInfo.nACCount = (long)value;
	}
	else if(str == _T("Grab Count"))
	{
		busbarInfo.nGrabCount = (long)value;
	}
	else if (str == _T("Insp Type"))
	{
		if ((CString)value == _T("NONE"))
		{
			params.nInspType = (long)InspType::INSP_None;
		}
		else if ((CString)value == _T("AC"))
		{
			params.nInspType = (long)InspType::INSP_AC;
		}
		else if ((CString)value == _T("DC"))
		{
			params.nInspType = (long)InspType::INSP_DC;
		}
	}
	//Vision & div index 변경시 MatchItem도 새로 맞춰서 변경해줘야한다
	auto &inspItemMatch = ::g_RecipeManager.m_vecInspItemMatch;
	for (long idx = 0; idx < inspItemMatch.size(); idx++)
	{
		if (inspItemMatch[idx].visionIndex == params.visionIndex && inspItemMatch[idx].divIndex == params.divIndex)
		{
 			inspItemMatch[idx].matchingIndex = m_nLastestSelectedInspItemIndex;
		}
	}

	SetInspectionItemListControl();

	return 0;
}

void CDlgInspItemSetup::InitPropertyGrid()
{
	CString strTemp;
	CString strTemp2;

	m_propertyGridItemProperty.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 150;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridItemProperty.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	auto &params = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex];
	auto &busbarInfo = ::g_RecipeManager.m_sBusbarInfoParam;

	long camCount(0);
	switch (::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		camCount = 1;
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		camCount = 2;
		break;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Busbar Info.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupBusbarInfo = new CMFCPropertyGridProperty(_T("Busbar Info."));
	m_propertyGridItemProperty.AddProperty(pGroupBusbarInfo);

	switch(busbarInfo.nInspectionDivision)
	{
	case 0: strTemp.Format(_T("NONE")); break;
	case 1: strTemp.Format(_T("BEFORE")); break;
	case 2: strTemp.Format(_T("AFTER")); break;
	}
	switch (params.nInspType)
	{
	case 0: strTemp2.Format(_T("NONE")); break;
	case 1: strTemp2.Format(_T("AC")); break;
	case 2: strTemp2.Format(_T("DC")); break;
	}
	CMFCPropertyGridProperty* pGroupBusbarInfo_InspectionDivision = new CMFCPropertyGridProperty(_T("Inspection Division"), strTemp, _T(""));
	pGroupBusbarInfo_InspectionDivision->AddOption(_T("NONE"));
	pGroupBusbarInfo_InspectionDivision->AddOption(_T("BEFORE"));
	pGroupBusbarInfo_InspectionDivision->AddOption(_T("AFTER"));
	pGroupBusbarInfo->AddSubItem(pGroupBusbarInfo_InspectionDivision);

	CMFCPropertyGridProperty* pGroupBusbarInfo_DCCount = new CMFCPropertyGridProperty(_T("DC Count"), busbarInfo.nDCCount, _T(""));
	pGroupBusbarInfo->AddSubItem(pGroupBusbarInfo_DCCount);

	CMFCPropertyGridProperty* pGroupBusbarInfo_ACCount = new CMFCPropertyGridProperty(_T("AC Count"), busbarInfo.nACCount, _T(""));
	pGroupBusbarInfo->AddSubItem(pGroupBusbarInfo_ACCount);

	CMFCPropertyGridProperty* pGroupBusbarInfo_GrabCount = new CMFCPropertyGridProperty(_T("Grab Count"), busbarInfo.nGrabCount, _T(""));
	pGroupBusbarInfo->AddSubItem(pGroupBusbarInfo_GrabCount);

	CMFCPropertyGridProperty* pGroupProperty_InspType = new CMFCPropertyGridProperty(_T("Insp Type"), strTemp2, _T(""));
	pGroupProperty_InspType->AddOption(_T("NONE"));
	pGroupProperty_InspType->AddOption(_T("AC"));
	pGroupProperty_InspType->AddOption(_T("DC"));
	pGroupBusbarInfo->AddSubItem(pGroupProperty_InspType);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Property
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupProperty = new CMFCPropertyGridProperty(_T("Property"));
	m_propertyGridItemProperty.AddProperty(pGroupProperty);

	CMFCPropertyGridProperty* pGroupProperty_ItemName = new CMFCPropertyGridProperty(_T("Insp. Item Name"), params.strName, _T(""));
	pGroupProperty->AddSubItem(pGroupProperty_ItemName);

	CMFCPropertyGridProperty* pGroupProperty_VisionIndex = new CMFCPropertyGridProperty(_T("Vision Index"), params.visionIndex, _T(""));
	for (long idx = 0; idx < camCount; idx++)
	{
		strTemp.Format(_T("%d"), idx);
		pGroupProperty_VisionIndex->AddOption(strTemp);
	}
	pGroupProperty->AddSubItem(pGroupProperty_VisionIndex);

	CMFCPropertyGridProperty* pGroupProperty_DivisionIndex = new CMFCPropertyGridProperty(_T("Division Index"), params.divIndex, _T(""));
	for (long idx = 0; idx < ::g_ConfigManager.m_nDivisionIndex; idx++)
	{
		strTemp.Format(_T("%d"), idx);
		pGroupProperty_DivisionIndex->AddOption(strTemp);
	}
	pGroupProperty->AddSubItem(pGroupProperty_DivisionIndex);

	CMFCPropertyGridProperty* pGroupProperty_Algorithm = new CMFCPropertyGridProperty(_T("Algorithm Sequence"), gl_szStrAlgoSequenceName[params.algoSequence], _T(""));
	for(long idx = 0 ; idx < (long)ALGORITHM_SEQUENCE::END; idx++)
	{
		pGroupProperty_Algorithm->AddOption(gl_szStrAlgoSequenceName[idx]);
	}
	pGroupProperty->AddSubItem(pGroupProperty_Algorithm);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridItemProperty.AddProperty(pGroupEtc);
}

void CDlgInspItemSetup::OnTimer(UINT_PTR nIDEvent)
{
	// Live 화면 출력 - SHK 2023.12.27
	long camIndex = (long)nIDEvent;
	BYTE* image = nullptr;

	switch(camIndex)
	{
	case (long)CAMERA_INDEX::CAMERA1:
		image = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		break;
	case (long)CAMERA_INDEX::CAMERA2:
		image = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		break;
	}

	m_pCtrlCamera[camIndex]->Grab1Shot();

	DWORD dwGrabDone = ::WaitForSingleObject(m_pCtrlCamera[camIndex]->GetGrabDoneHandle(), 1000);

	if (dwGrabDone == WAIT_OBJECT_0)
	{
		memcpy(image, m_pCtrlCamera[camIndex]->GetImage(), m_pParent->m_nImageSize);

		m_pImageDisplay->SetImage(image, m_pParent->m_nImageWidth, m_pParent->m_nImageHeight, m_pParent->m_nImageWidth);
	}
}

void CDlgInspItemSetup::OnClose()
{
	KillTimer(0);
	KillTimer(1);

	IllumOff();

	CDialog::OnClose();
}

void CDlgInspItemSetup::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int nID = pScrollBar->GetDlgCtrlID();

	UpdateData(TRUE);

	int step = 1;
	int Mstep = 10;
	int nNext, nmin, nmax;

	pScrollBar->GetScrollRange(&nmin, &nmax);

	switch(nSBCode)
	{
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNext = nPos;
		if(nNext > nmax) nNext = nmax;
		else if(nNext < nmin) nNext = nmin;
		pScrollBar->SetScrollPos(nNext);
		break;
	case SB_LINEDOWN:
		nNext = pScrollBar->GetScrollPos();
		if(nNext + step <= nmax) {
			pScrollBar->SetScrollPos(nNext + step);
			nNext += step;
		}
		break;
	case SB_LINEUP:
		nNext = pScrollBar->GetScrollPos();
		if(nNext - step >= nmin) {
			pScrollBar->SetScrollPos(nNext - step);
			nNext -= step;
		}
		break;
	case SB_PAGEDOWN:
		nNext = pScrollBar->GetScrollPos();
		if(nNext + Mstep <= nmax) {
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
		if(nNext - Mstep >= nmin) {
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

	if(m_nLastestSelectedInspItemIndex == -1)
	{
		return;
	}

	unsigned short *illum = nullptr;
	if(::g_ConfigManager.m_bMultiIllumination == TRUE)
	{
		illum = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex].sIllum.m_usIllum;
	}
	else
	{
		illum = ::g_RecipeManager.m_sIllum.m_usIllum;
	}

	switch(nID)
	{
	case IDC_SCROLLBAR_CH1:
		illum[0] = pScrollBar->GetScrollPos();
		SetDlgItemInt(IDC_EDIT_CH1, illum[0]);
		::g_CtrlSerial.IllumOn(0, illum[0]);
		break;
	case IDC_SCROLLBAR_CH2:
		illum[1] = pScrollBar->GetScrollPos();
		SetDlgItemInt(IDC_EDIT_CH2, illum[1]);
		::g_CtrlSerial.IllumOn(1, illum[1]);
		break;
	case IDC_SCROLLBAR_CH3:
		illum[2] = pScrollBar->GetScrollPos();
		SetDlgItemInt(IDC_EDIT_CH3, illum[2]);
		::g_CtrlSerial.IllumOn(2, illum[2]);
		break;
	case IDC_SCROLLBAR_CH4:
		illum[3] = pScrollBar->GetScrollPos();
		SetDlgItemInt(IDC_EDIT_CH4, illum[3]);
		::g_CtrlSerial.IllumOn(3, illum[3]);
		break;
	}

	UpdateData(FALSE);

	Invalidate(FALSE);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CDlgInspItemSetup::IllumOn()
{
	if(m_nLastestSelectedInspItemIndex == -1)
	{
		return;
	}

	unsigned short *illum = nullptr;
	if(::g_ConfigManager.m_bMultiIllumination == TRUE)
	{
		illum = ::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex].sIllum.m_usIllum;
	}
	else
	{
		illum = ::g_RecipeManager.m_sIllum.m_usIllum;
	}

	for(long ch = 0; ch < ILLUM_CHANNEL_MAX; ch++)
	{
		::g_CtrlSerial.IllumOn(ch, illum[ch]);
	}
}

void CDlgInspItemSetup::IllumOff()
{
	for(long ch = 0; ch < ILLUM_CHANNEL_MAX; ch++)
	{
		::g_CtrlSerial.IllumOff(ch);
	}
}

void CDlgInspItemSetup::SetEachIllum(const long ch, const BYTE value)
{
	UpdateData(TRUE);

	if(m_nLastestSelectedInspItemIndex == -1)
	{
		return;
	}

	unsigned short illum(0);
	
	m_scrollbarCh[ch].SetScrollPos(value);

	illum = (BYTE)m_scrollbarCh[ch].GetScrollPos();

	if(::g_ConfigManager.m_bMultiIllumination == TRUE)
	{
		::g_RecipeManager.m_vecsInspItem[m_nLastestSelectedInspItemIndex].sIllum.m_usIllum[ch] = illum;
	}
	else
	{
		::g_RecipeManager.m_sIllum.m_usIllum[ch] = illum;
	}

	m_scrollbarCh[ch].SetScrollPos(illum);

	SetDlgItemInt(IDC_EDIT_CH1 + ch, illum);

	::g_CtrlSerial.IllumOn(ch, illum);

	UpdateData(FALSE);
}


void CDlgInspItemSetup::OnBnClickedBtnLive()
{
	long cameraIndex = m_tabCamera.GetCurSel();

	IllumOn();

	SetTimer(cameraIndex, 50, NULL);
}


void CDlgInspItemSetup::OnBnClickedBtnStop()
{
	long cameraIndex = m_tabCamera.GetCurSel();

	KillTimer(cameraIndex);

	IllumOff();
}


void CDlgInspItemSetup::OnBnClickedBtnCh1Zero()
{
	SetEachIllum(0, 0);
}


void CDlgInspItemSetup::OnBnClickedBtnCh1Full()
{
	SetEachIllum(0, 255);
}


void CDlgInspItemSetup::OnBnClickedBtnCh2Zero()
{
	SetEachIllum(1, 0);
}


void CDlgInspItemSetup::OnBnClickedBtnCh2Full()
{
	SetEachIllum(1, 255);
}


void CDlgInspItemSetup::OnBnClickedBtnCh3Zero()
{
	SetEachIllum(2, 0);
}


void CDlgInspItemSetup::OnBnClickedBtnCh3Full()
{
	SetEachIllum(2, 255);
}


void CDlgInspItemSetup::OnBnClickedBtnCh4Zero()
{
	SetEachIllum(3, 0);
}


void CDlgInspItemSetup::OnBnClickedBtnCh4Full()
{
	SetEachIllum(3, 255);
}


void CDlgInspItemSetup::OnBnClickedBtnIllumOn()
{
	IllumOn();
}


void CDlgInspItemSetup::OnBnClickedBtnIllumOff()
{
	IllumOff();
}


void CDlgInspItemSetup::OnTcnSelchangeTabCamera(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Live 화면 출력 - SHK 2023.12.27
	UpdateImageDisplay();

	*pResult = 0;
}

void CDlgInspItemSetup::OnBnClickedBtnRecipeSave()
{
	m_pParent->SaveRecipe(m_pParent->m_strCurRecipePath);
}

void CDlgInspItemSetup::UpdateImageDisplay()
{
	// Live 화면 출력 - SHK 2023.12.27
	long cameraIndex = m_tabCamera.GetCurSel();

	BYTE* image = nullptr;

	switch (cameraIndex)
	{
	case (long)CAMERA_INDEX::CAMERA1:
		image = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		break;
	case (long)CAMERA_INDEX::CAMERA2:
		image = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		break;
	}

	m_pImageDisplay->SetImage(image, m_pParent->m_nImageWidth, m_pParent->m_nImageHeight, m_pParent->m_nImageWidth);
}


void CDlgInspItemSetup::InitInspectionItemListControl()
{
	m_listInspectionItem.SetExtendedStyle(m_listInspectionItem.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_listInspectionItem.InsertColumn(0, _T(""), LVCFMT_CENTER, 20);
	m_listInspectionItem.InsertColumn(1, _T("No."), LVCFMT_CENTER, 40);
	m_listInspectionItem.InsertColumn(2, _T("Name"), LVCFMT_CENTER, 150);
	m_listInspectionItem.InsertColumn(3, _T("Vision Index"), LVCFMT_CENTER, 100);
	m_listInspectionItem.InsertColumn(4, _T("Div. Index"), LVCFMT_CENTER, 100);
	m_listInspectionItem.InsertColumn(5, _T("Algorithm Sequence"), LVCFMT_CENTER, 200);
}

void CDlgInspItemSetup::SetInspectionItemListControl()
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

void CDlgInspItemSetup::OnInspItemAdd()
{
	auto &inspItem = ::g_RecipeManager.m_vecsInspItem;

	CDlgNewInspectionItem DlgNewInspectionItem;
	if(DlgNewInspectionItem.DoModal() == IDOK)
	{
		if(DlgNewInspectionItem.m_strName != _T(""))
		{
			for(auto item : inspItem)
			{
				if(item.strName == DlgNewInspectionItem.m_strName)
				{
					::AfxMessageBox(_T("Same Inspection item name exists."));
					return;
				}
			}

			SInspItem item;
			item.Init();
			item.strName = DlgNewInspectionItem.m_strName;
			item.visionIndex = DlgNewInspectionItem.m_nVisionIndex;
			item.divIndex = DlgNewInspectionItem.m_nDivisionIndex;
			item.algoSequence = DlgNewInspectionItem.m_nAlgoSequence;

			inspItem.push_back(item);

			//Item 추가시 MatchIndex 자동 매칭
			auto &inspItemMatch = ::g_RecipeManager.m_vecInspItemMatch;
			for (long idx = 0; idx < inspItemMatch.size(); idx++)
			{
				for (long insp = 0; insp < inspItem.size(); insp++)
				{
					if (inspItemMatch[idx].visionIndex == inspItem[insp].visionIndex && inspItemMatch[idx].divIndex == inspItem[insp].divIndex)
					{
						inspItemMatch[idx].matchingIndex = insp;
					}
				}
			}

			SetInspectionItemListControl();
		}
	}
}

void CDlgInspItemSetup::OnInspItemDel()
{
	if(m_nLastestSelectedInspItemIndex == -1)
	{
		return;
	}

	auto &inspItem = ::g_RecipeManager.m_vecsInspItem;

	if((long)inspItem.size() <= m_nLastestSelectedInspItemIndex)
	{
		return;
	}

	//Item 삭제시 MatchIndex 0으로 초기화
	auto &inspItemMatch = ::g_RecipeManager.m_vecInspItemMatch;
	for (long idx = 0; idx < inspItemMatch.size(); idx++)
	{
		if (inspItemMatch[idx].visionIndex == inspItem[m_nLastestSelectedInspItemIndex].visionIndex && inspItemMatch[idx].divIndex == inspItem[m_nLastestSelectedInspItemIndex].divIndex)
		{
			inspItemMatch[idx].matchingIndex = 0;
		}
	}

	inspItem.erase(inspItem.begin() + m_nLastestSelectedInspItemIndex);

	SetInspectionItemListControl();
}

void CDlgInspItemSetup::OnInspItemCopy()
{
	CString strTemp;

	if(m_nLastestSelectedInspItemIndex == -1)
	{
		return;
	}

	auto &inspItem = ::g_RecipeManager.m_vecsInspItem;

	SInspItem item;

	item.Init();
	strTemp.Format(_T("%s_copy"), inspItem[m_nLastestSelectedInspItemIndex].strName);
	item.strName = strTemp;
	item.visionIndex = inspItem[m_nLastestSelectedInspItemIndex].visionIndex;
	item.divIndex = inspItem[m_nLastestSelectedInspItemIndex].divIndex;
	item.algoSequence = inspItem[m_nLastestSelectedInspItemIndex].algoSequence;
	item.sIllum = inspItem[m_nLastestSelectedInspItemIndex].sIllum;
	item.nInspType = inspItem[m_nLastestSelectedInspItemIndex].nInspType;
	inspItem.push_back(item);

	SetInspectionItemListControl();
}

void CDlgInspItemSetup::OnLvnItemchangedListInspectionItem(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	long inspItemIndex = m_listInspectionItem.GetNextItem(-1, LVNI_SELECTED);

	if(inspItemIndex == -1)
	{
		return;
	}

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

	IllumOn();

	InitPropertyGrid();

	*pResult = 0;
}

void CDlgInspItemSetup::OnNMRClickListInspectionItem(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	CMenu menu;

	BOOL ret = menu.CreatePopupMenu();

	long index = m_listInspectionItem.GetNextItem(-1, LVNI_SELECTED);

	if(index == -1)
	{
		menu.AppendMenu(MF_STRING, IDM_INSP_ITEM_ADD, _T("Add Item ..."));
	}
	else
	{
		m_nLastestSelectedInspItemIndex = index;

		menu.AppendMenu(MF_STRING, IDM_INSP_ITEM_ADD, _T("Add Item ..."));
		menu.AppendMenu(MF_STRING, IDM_INSP_ITEM_DEL, _T("Delete Item ..."));
		menu.AppendMenu(MF_STRING, IDM_INSP_ITEM_COPY, _T("Copy Item ..."));
	}

	CmdRouteMenu(this, &menu);

	CPoint point;
	GetCursorPos(&point);

	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

	*pResult = 0;
}

void CDlgInspItemSetup::CmdRouteMenu(CWnd* pWnd, CMenu* pPopupMenu)
{
	CCmdUI state;
	state.m_pMenu = pPopupMenu;
	state.m_pParentMenu = pPopupMenu;
	state.m_nIndexMax = pPopupMenu->GetMenuItemCount();

	for(state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++)
	{
		state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);

		// menu separator or invalid cmd - ignore it

		if(state.m_nID == 0) continue;

		if(state.m_nID == (UINT)-1)
		{
			// possibly a popup menu, route to child menu if so

			CMenu* pSub = pPopupMenu->GetSubMenu(state.m_nIndex);
			if(pSub) CmdRouteMenu(pWnd, pSub);
		}
		else
		{
			// normal menu item, Auto disable if command is 

			// _not_ a system command.

			state.m_pSubMenu = NULL;
			state.DoUpdate(pWnd, FALSE);
		}
	}
}
