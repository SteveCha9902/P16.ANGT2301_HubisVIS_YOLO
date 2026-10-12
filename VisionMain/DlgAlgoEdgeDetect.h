#pragma once


// CDlgAlgoEdgeDetect 대화 상자
class CDlgSetup;

class CDlgAlgoEdgeDetect : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgAlgoEdgeDetect)

public:
	CDlgAlgoEdgeDetect(CDlgSetup* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgAlgoEdgeDetect();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_ALGO_EDGE_DETECT };
//#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	afx_msg LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	CDlgSetup* m_pParent;
	CImageDisplay* m_pImageDisplay;

	CMFCPropertyGridCtrl m_propertyGridEdgeDetect;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;
	long m_nCurItemIndex;

	void InitPropertyGrid();
	void PropertyChanged(CString str, _variant_t value);
};
