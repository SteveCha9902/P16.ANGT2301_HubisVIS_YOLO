#pragma once


// CDlgAlgoBlob 대화 상자
class CDlgSetup;
class CDlgCalibration;

class CDlgAlgoBlob : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgAlgoBlob)

public:
	CDlgAlgoBlob(CDlgSetup* pParent = nullptr, CDlgCalibration* pCalibration = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgAlgoBlob();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_ALGO_BLOB };
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

	CMFCPropertyGridCtrl m_propertyGridBlob;
	CListCtrl m_listBlobInfo;

	void InitPropertyGrid();
	void UpdateBlobList();
	void ClearBlobList();

	afx_msg void OnBnClickedBtnUpdatebloblist();
	afx_msg void OnLvnItemchangedListBloblist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);
};
