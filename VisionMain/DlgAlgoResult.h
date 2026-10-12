#pragma once


// CDlgResult 대화 상자
class CDlgSetup;

class CDlgAlgoResult : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgAlgoResult)

public:
	CDlgAlgoResult(CDlgSetup* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgAlgoResult();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_ALGO_RESULT };
//#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgSetup* m_pParent;
	CImageDisplay* m_pImageDisplay;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;
	long m_nCurItemIndex;

	CMFCPropertyGridCtrl m_propertyGridResult;

	void UpdateBlob(CString str, _variant_t value);
	void UpdateBlob2(CString str, _variant_t value);
	void UpdateEdge(CString str, _variant_t value);
	void SetResult(long algoSequence);
	void InitPropertyGrid();
	void InitPropertyGridBlob();
	void InitPropertyGridBlob2();
	void InitPropertyGridEdge();

	afx_msg LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);
};
