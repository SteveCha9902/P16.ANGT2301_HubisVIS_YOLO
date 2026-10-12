// CDlgUser.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgUser.h"
#include "afxdialogex.h"

#include "DlgVisionMain.h"
#include "DlgNewUser.h"
#include "DlgLogin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgUser 대화 상자

IMPLEMENT_DYNAMIC(CDlgUser, CDialogEx)

CDlgUser::CDlgUser(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_USER, pParent)
	, m_pParent(pParent)
	, m_nSelectedUserIndex(-1)
	, m_nCurUserIndex(-1)
{

}

CDlgUser::~CDlgUser()
{
}

void CDlgUser::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_USER, m_listUser);
	DDX_Control(pDX, IDC_STATIC_CUR_USER, m_lblCurUser);
}

BEGIN_MESSAGE_MAP(CDlgUser, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_ADD, &CDlgUser::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDC_BTN_DELETE, &CDlgUser::OnBnClickedBtnDelete)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_USER, &CDlgUser::OnLvnItemchangedListUser)
	ON_BN_CLICKED(IDC_BTN_LOGIN, &CDlgUser::OnBnClickedBtnLogin)
END_MESSAGE_MAP()


// CDlgUser 메시지 처리기


BOOL CDlgUser::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitListControl();
	SetListControl();

	m_nCurUserIndex = ::g_UserManager.m_nPreLoginUserIndex;
	m_nSelectedUserIndex = m_nCurUserIndex;

	auto User = ::g_UserManager.m_vecUser;

	// 선택한 Item만 Check 표시 하기
	for (long idx = 0; idx < (long)User.size(); idx++)
	{
		if (m_nCurUserIndex == idx)
		{
			m_listUser.SetCheck(idx, TRUE);
		}
		else
		{
			m_listUser.SetCheck(idx, FALSE);
		}
	}

	SetCurUser(m_nCurUserIndex);
	UpdateUI();

	return TRUE;
}


BOOL CDlgUser::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
		case VK_ESCAPE:
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CDlgUser::InitListControl()
{
	m_listUser.SetExtendedStyle(m_listUser.GetExtendedStyle() | /*LVS_EX_CHECKBOXES |*/ LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_listUser.InsertColumn(0, _T("No."), LVCFMT_CENTER, 40);
	m_listUser.InsertColumn(1, _T("Authority"), LVCFMT_CENTER, 100);
	m_listUser.InsertColumn(2, _T("Name"), LVCFMT_CENTER, 100);
}

void CDlgUser::SetListControl()
{
	CString strTemp;
	m_listUser.DeleteAllItems();

	auto User = ::g_UserManager.m_vecUser;

	for (long idx = 0; idx < User.size(); idx++)
	{
		strTemp.Format(_T("%d"), idx + 1);
		m_listUser.InsertItem(idx, strTemp);

		switch (std::get<0>(User[idx]))
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
		m_listUser.SetItemText(idx, 1, strTemp);

		m_listUser.SetItemText(idx, 2, std::get<1>(User[idx]));
	}
}


void CDlgUser::OnBnClickedBtnAdd()
{
	CDlgNewUser dlg(this);
	if (dlg.DoModal() == IDOK)
	{
		auto &User = ::g_UserManager.m_vecUser;

		User.push_back(std::make_tuple(dlg.m_nAuthority, dlg.m_strName, dlg.m_strPassword));
	}

	SetListControl();
}


void CDlgUser::OnBnClickedBtnDelete()
{
	if (m_nSelectedUserIndex == -1)
	{
		return;
	}

	auto &User = ::g_UserManager.m_vecUser;

	if ((long)User.size() <= m_nSelectedUserIndex)
	{
		return;
	}

	if (m_nCurUserIndex == m_nSelectedUserIndex)
	{
		AfxMessageBox(_T("Logged-in users cannot be deleted."));
		return;
	}

	::g_UserManager.m_pRegistry->DeleteKey(_T("USER"), std::get<1>(User[m_nSelectedUserIndex]));

	User.erase(User.begin() + m_nSelectedUserIndex);

	SetListControl();
}


void CDlgUser::OnLvnItemchangedListUser(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	long userIndex = m_listUser.GetNextItem(-1, LVNI_SELECTED);

	if (userIndex == -1)
	{
		return;
	}

	m_nSelectedUserIndex = userIndex;

	auto User = ::g_UserManager.m_vecUser;

	// 선택한 Item만 Check 표시 하기
	for (long idx = 0; idx < (long)User.size(); idx++)
	{
		if (userIndex == idx)
		{
			m_listUser.SetCheck(idx, TRUE);
		}
		else
		{
			m_listUser.SetCheck(idx, FALSE);
		}
	}

	*pResult = 0;
}


void CDlgUser::OnBnClickedBtnLogin()
{
	CDlgLogin dlg(this);

	if (dlg.DoModal() == IDOK)
	{
		m_nCurUserIndex = dlg.m_nIndex;
		SetCurUser(m_nCurUserIndex);

		m_pParent->ShowUser(m_nCurUserIndex);

		UpdateUI();

		::g_UserManager.m_strPreLoginUserID = std::get<1>(::g_UserManager.m_vecUser[m_nCurUserIndex]);
		//::g_UserManager.m_nPreLoginUserIndex = m_nCurUserIndex;
	}
}

void CDlgUser::SetCurUser(const long index)
{
	CString strUser;

	strUser = std::get<1>(::g_UserManager.m_vecUser[index]);
	m_lblCurUser.SetText(strUser);
	m_lblCurUser.SetFontSize(15);
	m_lblCurUser.SetFontBold(TRUE);
	m_lblCurUser.SetTextColor(RGB(0, 0, 255));
	m_lblCurUser.SetBorder(TRUE);
}

void CDlgUser::UpdateUI()
{
	auto User = ::g_UserManager.m_vecUser;

	if (std::get<0>(User[m_nSelectedUserIndex]) == 0)
	{
		GetDlgItem(IDC_BTN_ADD)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_DELETE)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_BTN_ADD)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_DELETE)->EnableWindow(FALSE);
	}


}
