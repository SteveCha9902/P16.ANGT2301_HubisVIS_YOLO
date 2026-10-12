#pragma once


// CDlgAlgoROI 대화 상자
class CDlgSetup;

class CDlgAlgoROI : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgAlgoROI)

public:
	CDlgAlgoROI(CDlgSetup* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgAlgoROI();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_ALGO_ROI };
//#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgSetup* m_pParent;
	CImageDisplay* m_pImageDisplay;

	CMFCPropertyGridCtrl m_propertyGridROI;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;
	long m_nCurItemIndex;

	void InitPropertyGrid();
	void SetROI();
	void GetROI();

	afx_msg LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);
};
