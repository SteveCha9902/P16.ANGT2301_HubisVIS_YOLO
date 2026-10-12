// DlgLogin2.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgLogin2.h"
#include "afxdialogex.h"

#include "DlgVisionMain.h"

// CDlgLogin2 대화 상자

IMPLEMENT_DYNAMIC(CDlgLogin2, CDialogEx)

CDlgLogin2::CDlgLogin2(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_LOGIN2, pParent)
{

}

CDlgLogin2::~CDlgLogin2()
{
}

void CDlgLogin2::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgLogin2, CDialogEx)
	ON_BN_CLICKED(IDOK2, &CDlgLogin2::OnBnClickedOk2)
END_MESSAGE_MAP()


// CDlgLogin2 메시지 처리기


void CDlgLogin2::OnBnClickedOk2()
{
	CString strTemp;

	GetDlgItemText(IDC_EDIT_PASSWORD, strTemp);
	if (strTemp == _T("1"))
	{
		g_UserManager.m_strPreLoginUserID = "1";
	}
}
