// CDlgInspItemMatch.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgInspItemMatch.h"
#include "afxdialogex.h"

#include "DlgVisionMain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgInspItemMatch 대화 상자

IMPLEMENT_DYNAMIC(CDlgInspItemMatch, CDialogEx)

CDlgInspItemMatch::CDlgInspItemMatch(CDlgVisionMain* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_INSP_ITEM_MATCH, pParent)
	, m_pParent(pParent)
	, m_nMatchItemIndex(-1)
{

}

CDlgInspItemMatch::~CDlgInspItemMatch()
{
}

void CDlgInspItemMatch::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_INSP_ITEM_MATCH, m_listInspItemMatch);
	DDX_Control(pDX, IDC_CMB_INSP_ITEM, m_cmbInspItem);
	DDX_Control(pDX, IDC_LIST_SUB_ITEM, m_listSubItem);
}

BEGIN_MESSAGE_MAP(CDlgInspItemMatch, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_INSP_ITEM_MATCH, &CDlgInspItemMatch::OnNMDblclkListInspItemMatch)
	ON_CBN_SELCHANGE(IDC_CMB_INSP_ITEM, &CDlgInspItemMatch::OnCbnSelchangeCmbInspItem)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_INSP_ITEM_MATCH, &CDlgInspItemMatch::OnLvnItemchangedListInspItemMatch)
	ON_LBN_SELCHANGE(IDC_LIST_SUB_ITEM, &CDlgInspItemMatch::OnLbnSelchangeListSubItem)
	ON_BN_CLICKED(IDC_BTN_RECIPE_SAVE, &CDlgInspItemMatch::OnBnClickedBtnRecipeSave)
END_MESSAGE_MAP()


// CDlgInspItemMatch 메시지 처리기


BOOL CDlgInspItemMatch::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_cmbInspItem.SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
	m_cmbInspItem.ResetContent();

	auto inspItem = ::g_RecipeManager.m_vecsInspItem;
	for (long idx = 0; idx < inspItem.size(); idx++)
	{
		m_cmbInspItem.AddString(inspItem[idx].strName);
	}
	m_cmbInspItem.SetCurSel(0);

	InitListControl();
	SetListControl();

	InitSubItemList();
	SetSubItemList();

	return TRUE;
}

BOOL CDlgInspItemMatch::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
		case VK_RETURN:
		case VK_ESCAPE:
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CDlgInspItemMatch::InitListControl()
{
	m_listInspItemMatch.SetExtendedStyle(m_listInspItemMatch.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_listInspItemMatch.InsertColumn(0, _T(""), LVCFMT_CENTER, 20);
	m_listInspItemMatch.InsertColumn(1, _T("No."), LVCFMT_CENTER, 40);
	m_listInspItemMatch.InsertColumn(2, _T("Vision Index"), LVCFMT_CENTER, 90);
	m_listInspItemMatch.InsertColumn(3, _T("Div. Index"), LVCFMT_CENTER, 90);
	m_listInspItemMatch.InsertColumn(4, _T("InspItem"), LVCFMT_CENTER, 140);
}

void CDlgInspItemMatch::SetListControl()
{
	CString strTemp;
	m_listInspItemMatch.DeleteAllItems();

	auto inspItem = ::g_RecipeManager.m_vecsInspItem;
	auto inspItemMatch = ::g_RecipeManager.m_vecInspItemMatch;

	for (long idx = 0; idx < inspItemMatch.size(); idx++)
	{
		m_listInspItemMatch.InsertItem(idx, _T(""));

		strTemp.Format(_T("%d"), idx + 1);
		m_listInspItemMatch.SetItemText(idx, 1, strTemp);

		strTemp.Format(_T("%d"), inspItemMatch[idx].visionIndex);
		m_listInspItemMatch.SetItemText(idx, 2, strTemp);

		strTemp.Format(_T("%d"), inspItemMatch[idx].divIndex);
		m_listInspItemMatch.SetItemText(idx, 3, strTemp);

		m_listInspItemMatch.SetItemText(idx, 4, inspItem[inspItemMatch[idx].matchingIndex].strName);
	}
}

void CDlgInspItemMatch::InitSubItemList()
{
	auto inspItem = ::g_RecipeManager.m_vecsInspItem;

	m_listSubItem.ResetContent();

	for(long idx = 0; idx < (long)inspItem.size(); idx++)
	{
		m_listSubItem.InsertString(idx, inspItem[idx].strName);
		m_listSubItem.SetItemHeight(idx, 20);
	}
}

void CDlgInspItemMatch::SetSubItemList()
{
	if(m_nMatchItemIndex <= -1)
	{
		return;
	}

	auto matchItem = ::g_RecipeManager.m_vecInspItemMatch[m_nMatchItemIndex];

	for(long idx = 0; idx < m_listSubItem.GetCount(); idx++)
	{
		m_listSubItem.SetCheck(idx, FALSE);
	}

	for(long idx = 0; idx < (long)matchItem.vecsSubItemIndex.size(); idx++)
	{
		m_listSubItem.SetCheck(matchItem.vecsSubItemIndex[idx], TRUE);
	}

	UpdateData(TRUE);
}

void CDlgInspItemMatch::OnNMDblclkListInspItemMatch(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	LPNMITEMACTIVATE pNMITEM = (LPNMITEMACTIVATE)pNMHDR;

	m_nMatchItemIndex = pNMITEM->iItem;
	m_nSubItem = pNMITEM->iSubItem;

	if (pNMITEM->iItem != -1 && pNMITEM->iSubItem > 2)
	{
		if(m_nMatchItemIndex == -1)
		{
			return;
		}

		auto matchItem = ::g_RecipeManager.m_vecInspItemMatch;

		// 선택한 Item만 Check 표시 하기
		for(long idx = 0; idx < (long)matchItem.size(); idx++)
		{
			if(m_nMatchItemIndex == idx)
			{
				m_listInspItemMatch.SetCheck(idx, TRUE);
			}
			else
			{
				m_listInspItemMatch.SetCheck(idx, FALSE);
			}
		}

		CRect rect;
		if (pNMITEM->iSubItem == 0)
		{
			m_listInspItemMatch.GetItemRect(pNMITEM->iItem, rect, LVIR_BOUNDS);
			rect.right = rect.left + m_listInspItemMatch.GetColumnWidth(0);
		}
		else
		{
			m_listInspItemMatch.GetSubItemRect(pNMITEM->iItem, pNMITEM->iSubItem, LVIR_BOUNDS, rect);

		}
		m_listInspItemMatch.ClientToScreen(rect);
		this->ScreenToClient(rect);

		m_cmbInspItem.SetWindowPos(NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
	}

	*pResult = 0;
}

void CDlgInspItemMatch::OnCbnSelchangeCmbInspItem()
{
	long index = m_cmbInspItem.GetCurSel();
	auto inspItem = ::g_RecipeManager.m_vecsInspItem;
	auto &matchItem = ::g_RecipeManager.m_vecInspItemMatch[m_nMatchItemIndex];

	if (index > -1)
	{
		m_listInspItemMatch.SetItemText(m_nMatchItemIndex, m_nSubItem, inspItem[index].strName);
		matchItem.matchingIndex = index;
	}

	m_cmbInspItem.SetCurSel(0);
	m_cmbInspItem.SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
}



void CDlgInspItemMatch::OnLvnItemchangedListInspItemMatch(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	m_nMatchItemIndex = m_listInspItemMatch.GetNextItem(-1, LVNI_SELECTED);

	if(m_nMatchItemIndex == -1)
	{
		return;
	}

	auto matchItem = ::g_RecipeManager.m_vecInspItemMatch;

	// 선택한 Item만 Check 표시 하기
	for(long idx = 0; idx < (long)matchItem.size(); idx++)
	{
		if(m_nMatchItemIndex == idx)
		{
			m_listInspItemMatch.SetCheck(idx, TRUE);
		}
		else
		{
			m_listInspItemMatch.SetCheck(idx, FALSE);
		}
	}

	SetSubItemList();

	*pResult = 0;
}


void CDlgInspItemMatch::OnLbnSelchangeListSubItem()
{
	if(m_nMatchItemIndex <= -1)
	{
		return;
	}

	auto &matchItem = ::g_RecipeManager.m_vecInspItemMatch[m_nMatchItemIndex];
	auto &inspItem = ::g_RecipeManager.m_vecsInspItem[matchItem.matchingIndex];
	auto &subItem = matchItem.vecsSubItemIndex;
	long subItemIndex = m_listSubItem.GetCurSel();
	
	if(m_listSubItem.GetCheck(subItemIndex) == TRUE)
	{
		if(matchItem.matchingIndex == subItemIndex)
		{
			::AfxMessageBox(_T("Same items cannot be selected!!"));

			m_listSubItem.SetCheck(subItemIndex, FALSE);

			return;
		}
		else
		{
			subItem.push_back(subItemIndex);
		}
	}
	else
	{
		BOOL bExist(FALSE);
		long index(0);
		for(long idx = 0; idx < (long)subItem.size(); idx++)
		{
			if(subItem[idx] == subItemIndex)
			{
				bExist = TRUE;
				index = idx;
				break;
			}
		}

		if(bExist == TRUE)
		{
			subItem.erase(subItem.begin() + index);
		}
	}

	SetSubItemList();
}


void CDlgInspItemMatch::OnBnClickedBtnRecipeSave()
{
	m_pParent->SaveRecipe(m_pParent->m_strCurRecipePath);

	::AfxMessageBox(_T("Save Complete..."));
}
