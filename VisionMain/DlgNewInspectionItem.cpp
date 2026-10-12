// DlgNewInspectionItem.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgVisionMain.h"
#include "DlgNewInspectionItem.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgNewInspectionItem 대화 상자

IMPLEMENT_DYNAMIC(CDlgNewInspectionItem, CDialogEx)

CDlgNewInspectionItem::CDlgNewInspectionItem(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_NEW_INSPECTION_ITEM, pParent)
	, m_pParent(pParent)
	, m_strName(_T(""))
{
}

CDlgNewInspectionItem::~CDlgNewInspectionItem()
{
}

void CDlgNewInspectionItem::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_INSPECTION_ITEM_NAME, m_strName);
	DDX_Control(pDX, IDC_CMB_VISION_INDEX, m_cmbVisionIndex);
	DDX_Control(pDX, IDC_CMB_DIVISION_INDEX, m_cmbDivisionIndex);
	DDX_Control(pDX, IDC_CMB_ALGO_SEQUENCE, m_cmbAlgoSequence);
}


BEGIN_MESSAGE_MAP(CDlgNewInspectionItem, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDlgNewInspectionItem::OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgNewInspectionItem 메시지 처리기


BOOL CDlgNewInspectionItem::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString strTemp;

	auto inspItem = ::g_RecipeManager.m_vecsInspItem;

	m_cmbVisionIndex.ResetContent();
	m_cmbDivisionIndex.ResetContent();
	m_cmbAlgoSequence.ResetContent();

	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		m_cmbVisionIndex.AddString(_T("Camera1"));
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		m_cmbVisionIndex.AddString(_T("Camera1"));
		m_cmbVisionIndex.AddString(_T("Camera2"));
		break;
	}
	m_cmbVisionIndex.SetCurSel(0);

	for(long idx = 0; idx < ::g_ConfigManager.m_nDivisionIndex; idx++)
	{
		strTemp.Format(_T("Div. %d"), idx);
		m_cmbDivisionIndex.AddString(strTemp);
	}
	m_cmbDivisionIndex.SetCurSel(0);

	for(long idx = 0; idx <= (long)ALGORITHM_SEQUENCE::DETECT_BLOB2; idx++)
	{
		m_cmbAlgoSequence.AddString(gl_szStrAlgoSequenceName[idx]);
	}
	m_cmbAlgoSequence.SetCurSel(0);

	return TRUE;
}

void CDlgNewInspectionItem::OnBnClickedOk()
{
	UpdateData(TRUE);

	m_nVisionIndex = m_cmbVisionIndex.GetCurSel();
	m_nDivisionIndex = m_cmbDivisionIndex.GetCurSel();
	m_nAlgoSequence = m_cmbAlgoSequence.GetCurSel();

	CDialogEx::OnOK();
}
