#pragma once


// CDlgCalibration 대화 상자

class CDlgVisionMain;
class CDlgAlgoThreshold;
class CDlgAlgoBlob;
class CDlgAlgoCalibration;

class CDlgCalibration : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgCalibration)

public:
	CDlgCalibration(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgCalibration();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_CALIBRATION };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgVisionMain* m_pParent;
	CImageDisplay* m_pImageDisplay;
	CTabCtrl m_tabCamera;
	CTabCtrl m_tabAlgorithm;

	CDlgAlgoThreshold* m_pDlgAlgoThreshold;
	CDlgAlgoBlob* m_pDlgAlgoBlob;
	CDlgAlgoCalibration* m_pDlgAlgoCalibration;

	BYTE* m_pbyImage;
	BYTE* m_pbyThreshold;

	SThresholdParams m_sThresholdParams;
	SBlobParams m_sBlobParams;
	SROIParams m_sROIParams;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;
	long m_nBlobCount;

	void InitUI();
	void SetAlgoDialog();
	void UpdateAlgoBlob();
	void UpdateImageDisplay(); // Image Buffer 변경 및 데이터 갱신 - SHK 2023.12.27

	float m_fPixelResolutionX;
	float m_fPixelResolutionY;
	
	//void DoCalibration();
	void ShowResultOverlay(const long seqIndex);

	afx_msg void OnTcnSelchangeTabCamera(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTcnSelchangeTabAlgorithm(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnBnClickedBtnOriginalImage();
	afx_msg void OnBnClickedBtnThresholdImage();
	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnBnClickedBtnDoCalibration();
};
