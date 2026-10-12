// CDlgRecipeResist.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgRecipeResist.h"
#include "afxdialogex.h"

#include "DlgVisionMain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgRecipeResist 대화 상자


IMPLEMENT_DYNAMIC(CDlgRecipeResist, CDialogEx)

void swap(CString &a, CString &b)
{
	CString temp = a;
	a = b;
	b = temp;
}

CDlgRecipeResist::CDlgRecipeResist(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_RECIPE_RESIST, pParent)
	, m_pParent(pParent)
{

}

CDlgRecipeResist::~CDlgRecipeResist()
{
}

void CDlgRecipeResist::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SAVE_RECIPE, m_listSaveList);
	DDX_Control(pDX, IDC_LIST_USE_RECIPE, m_listUseList);
}


BEGIN_MESSAGE_MAP(CDlgRecipeResist, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_UP, &CDlgRecipeResist::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &CDlgRecipeResist::OnBnClickedButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_ALL_CLEAR, &CDlgRecipeResist::OnBnClickedButtonAllClear)
	ON_BN_CLICKED(IDOK, &CDlgRecipeResist::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDlgRecipeResist::OnBnClickedCancel)
	ON_LBN_DBLCLK(IDC_LIST_SAVE_RECIPE, &CDlgRecipeResist::OnLbnDblclkListSaveRecipe)
	ON_LBN_DBLCLK(IDC_LIST_USE_RECIPE, &CDlgRecipeResist::OnLbnDblclkListUseRecipe)
END_MESSAGE_MAP()


// CDlgRecipeResist 메시지 처리기
BOOL CDlgRecipeResist::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	GetSaveList();
	UpdateSaveList();

	UpdateUseList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


BOOL CDlgRecipeResist::PreTranslateMessage(MSG* pMsg)
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

void CDlgRecipeResist::GetSaveList()
{
	CString strPath = REMOVE_FILE_FROM_PATHNAME(m_pParent->m_strCurRecipePath) + _T("\\*.recipe");

	CFileFind finder;

	BOOL bFind = finder.FindFile(strPath);

	CString strFile;

	m_vecstrSave.clear();

	while (bFind)
	{
		bFind = finder.FindNextFile();

		if (finder.IsDirectory() || finder.IsDots())
			continue;

		strFile = finder.GetFileName();

		m_vecstrSave.push_back(strFile);
	}
}

void CDlgRecipeResist::UpdateSaveList()
{
	CString strTemp;

	m_listSaveList.ResetContent();

	for (long idx = 0; idx < (long)m_vecstrSave.size(); idx++)
	{
		m_listSaveList.AddString(m_vecstrSave[idx]);
	}
}

void CDlgRecipeResist::UpdateUseList()
{
	CString strTemp;

	m_listUseList.ResetContent();

	for (long idx = 0; idx < (long)::g_PLCManager.m_vecstrUseRecipe.size(); idx++)
	{
		strTemp.Format(_T("%d : %s"), idx + 1, ::g_PLCManager.m_vecstrUseRecipe[idx]);
		m_listUseList.AddString(strTemp);
	}
}

void CDlgRecipeResist::OnBnClickedButtonUp()
{
	long nCurSel = m_listUseList.GetCurSel();

	if (nCurSel <= 0)
		return;

	swap(::g_PLCManager.m_vecstrUseRecipe[nCurSel], ::g_PLCManager.m_vecstrUseRecipe[nCurSel - 1]);

	UpdateUseList();

	m_listUseList.SetCurSel(nCurSel - 1);
}


void CDlgRecipeResist::OnBnClickedButtonDown()
{
	long nCurSel = m_listUseList.GetCurSel();

	if (nCurSel < 0)
		return;

	if (nCurSel >= ::g_PLCManager.m_vecstrUseRecipe.size() - 1)
		return;

	swap(::g_PLCManager.m_vecstrUseRecipe[nCurSel], ::g_PLCManager.m_vecstrUseRecipe[nCurSel + 1]);

	UpdateUseList();

	m_listUseList.SetCurSel(nCurSel + 1);
}


void CDlgRecipeResist::OnBnClickedButtonAllClear()
{
	::g_PLCManager.m_vecstrUseRecipe.clear();

	UpdateUseList();
}


void CDlgRecipeResist::OnBnClickedOk()
{
	::g_PLCManager.SaveUseRecipe(::g_PLCManager.m_strPath);

	CDialogEx::OnOK();
}


void CDlgRecipeResist::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}


void CDlgRecipeResist::OnLbnDblclkListSaveRecipe()
{
	long nCurSel = m_listSaveList.GetCurSel();

	if (nCurSel < -1)
	{
		return;
	}

	::g_PLCManager.m_vecstrUseRecipe.insert(::g_PLCManager.m_vecstrUseRecipe.end(), m_vecstrSave[nCurSel]);

	UpdateUseList();
}


void CDlgRecipeResist::OnLbnDblclkListUseRecipe()
{
	long nCurSel = m_listUseList.GetCurSel();

	if (nCurSel < 0)
	{
		return;
	}

	::g_PLCManager.m_vecstrUseRecipe.erase(::g_PLCManager.m_vecstrUseRecipe.begin() + nCurSel);

	UpdateUseList();
}