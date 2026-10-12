#pragma once


// CDlgRecipeResist 대화 상자

class CDlgVisionMain;

class CDlgRecipeResist : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgRecipeResist)

public:
	CDlgRecipeResist(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgRecipeResist();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_RECIPE_RESIST };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

public:
	CDlgVisionMain* m_pParent;
	CListBox m_listSaveList;
	CListBox m_listUseList;

	std::vector<CString> m_vecstrSave;

	void GetSaveList();
	void UpdateSaveList();
	void UpdateUseList();

	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonAllClear();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnLbnDblclkListSaveRecipe();
	afx_msg void OnLbnDblclkListUseRecipe();
};
