// CDlgLogin.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgLogin.h"
#include "afxdialogex.h"

#include "DlgUser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgLogin 대화 상자

IMPLEMENT_DYNAMIC(CDlgLogin, CDialogEx)

CDlgLogin::CDlgLogin(CDlgUser* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_LOGIN, pParent)
	, m_pParent(pParent)
	, m_nIndex(m_pParent->m_nSelectedUserIndex)
{

}

CDlgLogin::~CDlgLogin()
{
}

void CDlgLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgLogin, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDlgLogin::OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgLogin 메시지 처리기


BOOL CDlgLogin::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString strTemp;
	
	auto User = ::g_UserManager.m_vecUser;
	switch (std::get<0>(User[m_nIndex]))
	{
	case 0:
		strTemp.Format(_T("Admin"));
		break;
	case 1:
		strTemp.Format(_T("Engineer"));
		break;
	case 2:
		strTemp.Format(_T("Operator"));
		break;
	}
	SetDlgItemText(IDC_EDIT_USER_AUTHORITY, strTemp);

	SetDlgItemText(IDC_EDIT_USER_NAME, std::get<1>(User[m_nIndex]));
	
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CDlgLogin::OnBnClickedOk()
{
	CString strTemp;

	GetDlgItemText(IDC_EDIT_PASSWORD, strTemp);

	auto User = ::g_UserManager.m_vecUser;

	if (strTemp != std::get<2>(User[m_nIndex]))
	{
		AfxMessageBox(_T("Your password is incorrect."));
		return;
	}

	CDialogEx::OnOK();
}
