#pragma once


// CDlgSetup 대화 상자

class CDlgVisionMain;
class CDlgAlgoROI;
class CDlgAlgoThreshold;
class CDlgAlgoEdgeDetect;
class CDlgAlgoBlob;
class CDlgAlgoResult;
class CDlgAlgoCriteria;

class CDlgSetup : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgSetup)

public:
	CDlgSetup(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgSetup();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_SETUP };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	afx_msg LRESULT OnROIChanged(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	CDlgVisionMain* m_pParent;
	CImageDisplay* m_pImageDisplay;

	CDlgAlgoROI* m_pDlgAlgoROI;
	CDlgAlgoThreshold* m_pDlgAlgoThreshold;
	CDlgAlgoEdgeDetect* m_pDlgAlgoEdgeDetect;
	CDlgAlgoBlob* m_pDlgAlgoBlob;
	CDlgAlgoResult* m_pDlgAlgoResult;
	CDlgAlgoCriteria* m_pDlgAlgoCriteria;

	CTabCtrl m_tabCamera;
	CTabCtrl m_tabAlgorithm;
	CListCtrl m_listInspectionItem;
	long m_nLastestSelectedInspItemIndex;

	BYTE* m_pbyImage;
	BYTE* m_pbyMask;
	BYTE* m_pbyThresholdProc;
	BYTE* m_pbyThreshold;
	BYTE* m_pbyEdgeProc;
	BYTE* m_pbyBlobProc;
	BYTE* m_pbyBlobProc2;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;

	BOOL m_bResult; // EachResult 추가 - SHK 2023.12.21
	CLabel m_lblEachResult; // EachResult 추가 - SHK 2023.12.21

	void InitUI();
	void SetAlgoDialog();
	void InitInspectionItemListControl();
	void SetInspectionItemListControl();
	void SetInspectionResult(const CString strResult); // EachResult 추가 - SHK 2023.12.21
	void UpdateAlgoROI();
	void UpdateAlgoThreshold();
	void UpdateAlgoBlob();
	void UpdateAlgoBlob2();
	void UpdateAlgoEdgeDetect();
	void UpdateAlgoCriteria();
	void UpdateAlgoResult();
	void ShowResultOverlay(const long sequence, const long seqIndex);
	void GetProcData(short* pProc, long procSize, short* pProcParams, long procParamsSize, SProcImageParams &sProc);
	void UpdateProcImage();
	void UpdateImageDisplay(); // Image Buffer 변경 및 데이터 갱신 - SHK 2023.12.27
	long GetItemIndex();

	afx_msg void OnBnClickedBtnRecipeSave();
	afx_msg void OnTcnSelchangeTabCamera(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedListInspectionItem(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTcnSelchangeTabAlgorithm(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnInspection();
	afx_msg void OnBnClickedBtnOriginalImage();
	afx_msg void OnBnClickedBtnThresholdImage();
	afx_msg void OnBnClickedBtnProcImage();
	afx_msg void OnBnClickedBtnImageProcManager();
};
