#pragma once


// CDlgIndexSetup 대화 상자

class CDlgIndexSetup : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgIndexSetup)

public:
	CDlgIndexSetup(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgIndexSetup();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_INDEX_SETUP };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CComboBox m_cmbVisionIndex;
	long m_nVisionIndex;
	CComboBox m_cmbDivisionIndex;
	long m_nDivisionIndex;

	afx_msg void OnBnClickedOk();
};
