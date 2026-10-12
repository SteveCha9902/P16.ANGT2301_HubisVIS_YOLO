#pragma once


// CDlgNewUser 대화 상자

class CDlgUser;

class CDlgNewUser : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgNewUser)

public:
	CDlgNewUser(CDlgUser* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgNewUser();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_NEW_USER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgUser* m_pParent;
	CComboBox m_cmbUserAuthority;

	long m_nAuthority;
	CString m_strName;
	CString m_strPassword;

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
