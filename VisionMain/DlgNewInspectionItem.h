#pragma once


// CDlgNewInspectionItem 대화 상자

class CDlgVisionMain;

class CDlgNewInspectionItem : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgNewInspectionItem)

public:
	CDlgNewInspectionItem(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgNewInspectionItem();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_NEW_INSPECTION_ITEM };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CDlgVisionMain* m_pParent;

	CString m_strName;
	CComboBox m_cmbVisionIndex;
	long m_nVisionIndex;
	CComboBox m_cmbDivisionIndex;
	long m_nDivisionIndex;
	CComboBox m_cmbAlgoSequence;
	long m_nAlgoSequence;
	long m_nMinIndex;

	afx_msg void OnBnClickedOk();
};
