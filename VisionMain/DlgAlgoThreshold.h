#pragma once


// CDlgAlgoThreshold 대화 상자
class CDlgSetup;
class CDlgCalibration;

class CDlgAlgoThreshold : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgAlgoThreshold)

public:
	CDlgAlgoThreshold(CDlgSetup* pParent = nullptr, CDlgCalibration* pCalibration = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgAlgoThreshold();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_ALGO_THRESHOLD };
//#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgSetup* m_pParent;
	CDlgCalibration* m_pCalibration;
	CImageDisplay* m_pImageDisplay;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;
	long m_nCurItemIndex;

	CMFCPropertyGridCtrl m_propertyGridThreshold;
	CScrollBar m_scrollbarThreshold;
	CScrollBar m_scrollbarThreshold_High;

	void InitPropertyGrid();
	void UpdateUI();
	void UpdateParameters();
	void UpdateImage();

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);
};
