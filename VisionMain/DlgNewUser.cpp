// CDlgNewUser.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgNewUser.h"
#include "afxdialogex.h"

#include "DlgUser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgNewUser 대화 상자

IMPLEMENT_DYNAMIC(CDlgNewUser, CDialogEx)

CDlgNewUser::CDlgNewUser(CDlgUser* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_NEW_USER, pParent)
{

}

CDlgNewUser::~CDlgNewUser()
{
}

void CDlgNewUser::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMB_USER_AUTHORITY, m_cmbUserAuthority);
}


BEGIN_MESSAGE_MAP(CDlgNewUser, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDlgNewUser::OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgNewUser 메시지 처리기


BOOL CDlgNewUser::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	m_cmbUserAuthority.ResetContent();
	m_cmbUserAuthority.AddString(_T("Admin"));
	m_cmbUserAuthority.AddString(_T("Engineer"));
	m_cmbUserAuthority.AddString(_T("Operator"));

	m_cmbUserAuthority.SetCurSel(0);

	return TRUE; 				 
}


void CDlgNewUser::OnBnClickedOk()
{
	UpdateData(TRUE);

	m_nAuthority = m_cmbUserAuthority.GetCurSel();

	GetDlgItemText(IDC_EDIT_USER_NAME, m_strName);
	auto User = ::g_UserManager.m_vecUser;
	for (long idx = 0; idx < User.size(); idx++)
	{
		if (m_strName == std::get<1>(User[idx]))
		{
			AfxMessageBox(_T("The same name exists."));
			return;
		}
	}

	GetDlgItemText(IDC_EDIT_USER_PASSWORD, m_strPassword);

	CDialogEx::OnOK();
}
