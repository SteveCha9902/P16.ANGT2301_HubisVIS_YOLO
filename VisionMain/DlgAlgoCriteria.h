#pragma once


// CDlgAlgoCriteria 대화 상자
class CDlgSetup;

class CDlgAlgoCriteria : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgAlgoCriteria)

public:
	CDlgAlgoCriteria(CDlgSetup* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgAlgoCriteria();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_ALGO_CRITERIA };
//#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	CDlgSetup* m_pParent;
	CImageDisplay* m_pImageDisplay;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;
	long m_nCurItemIndex;

	CMFCPropertyGridCtrl m_propertyGridCriteria;
	
	void UpdateBlob(CString str, _variant_t value);
	void UpdateBlob2(CString str, _variant_t value);
	void UpdateEdge(CString str, _variant_t value);
	void SetCriteria(long algoSequence, BOOL &bResult);
	void InitPropertyGrid();
	void InitPropertyGridBlob();
	void InitPropertyGridBlob2();
	void InitPropertyGridEdge();

	afx_msg LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);
};
