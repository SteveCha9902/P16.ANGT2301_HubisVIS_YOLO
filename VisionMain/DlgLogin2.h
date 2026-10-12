#pragma once


// CDlgLogin2 대화 상자
class CDlgVisionMain;

class CDlgLogin2 : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgLogin2)

public:
	CDlgLogin2(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgLogin2();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_LOGIN2 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CDlgVisionMain* m_pParent;
	afx_msg void OnBnClickedOk2();
};
