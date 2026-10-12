#pragma once


// CDlgUser 대화 상자

class CDlgVisionMain;

class CDlgUser : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgUser)

public:
	CDlgUser(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgUser();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_USER };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgVisionMain* m_pParent;

	CListCtrl m_listUser;

	long m_nSelectedUserIndex;
	long m_nCurUserIndex;

	void InitListControl();
	void SetListControl();

	afx_msg void OnBnClickedBtnAdd();
	afx_msg void OnBnClickedBtnDelete();
	afx_msg void OnLvnItemchangedListUser(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnLogin();
	CLabel m_lblCurUser;
	void SetCurUser(const long index);
	void UpdateUI();
};
