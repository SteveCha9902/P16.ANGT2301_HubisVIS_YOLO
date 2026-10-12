// DlgIndexSetup.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgIndexSetup.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgIndexSetup 대화 상자

IMPLEMENT_DYNAMIC(CDlgIndexSetup, CDialogEx)

CDlgIndexSetup::CDlgIndexSetup(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_INDEX_SETUP, pParent)
{

}

CDlgIndexSetup::~CDlgIndexSetup()
{
}

void CDlgIndexSetup::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMB_VISION_INDEX, m_cmbVisionIndex);
	DDX_Control(pDX, IDC_CMB_DIVISION_INDEX, m_cmbDivisionIndex);
}


BEGIN_MESSAGE_MAP(CDlgIndexSetup, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDlgIndexSetup::OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgIndexSetup 메시지 처리기

BOOL CDlgIndexSetup::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString strTemp;

	m_cmbVisionIndex.ResetContent();
	m_cmbDivisionIndex.ResetContent();

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

	return TRUE;
}

void CDlgIndexSetup::OnBnClickedOk()
{
	UpdateData(TRUE);

	m_nVisionIndex = m_cmbVisionIndex.GetCurSel();
	m_nDivisionIndex = m_cmbDivisionIndex.GetCurSel();

	CDialogEx::OnOK();
}
