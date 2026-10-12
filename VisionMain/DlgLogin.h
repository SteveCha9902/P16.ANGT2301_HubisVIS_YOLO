#pragma once


// CDlgLogin 대화 상자

class CDlgUser;

class CDlgLogin : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgLogin)

public:
	CDlgLogin(CDlgUser* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgLogin();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_LOGIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgUser* m_pParent;
	long m_nIndex;

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
