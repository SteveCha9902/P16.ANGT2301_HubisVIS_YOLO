#pragma once


// CDlgInspItemMatch 대화 상자

class CDlgVisionMain;

class CDlgInspItemMatch : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgInspItemMatch)

public:
	CDlgInspItemMatch(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgInspItemMatch();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_INSP_ITEM_MATCH };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

public:
	CDlgVisionMain* m_pParent;

	CListCtrl m_listInspItemMatch;
	CComboBox m_cmbInspItem;
	CCheckListBox m_listSubItem;

	int m_nMatchItemIndex;
	int m_nSubItem;

	void InitListControl();
	void SetListControl();
	void InitSubItemList();
	void SetSubItemList();

	afx_msg void OnNMDblclkListInspItemMatch(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeCmbInspItem();
	afx_msg void OnLvnItemchangedListInspItemMatch(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLbnSelchangeListSubItem();
	afx_msg void OnBnClickedBtnRecipeSave();
};
