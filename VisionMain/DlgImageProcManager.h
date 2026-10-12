#pragma once


// CDlgImageProcManager 대화 상자

class CDlgImageProcManager : public CDialog
{
	DECLARE_DYNAMIC(CDlgImageProcManager)

public:
	CDlgImageProcManager(BYTE* pbySrc, const long nImageWidth, const long nImageHeight, short* pProc, short* pProcParams, const long procCount, CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CDlgImageProcManager();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_IMAGE_PROC_MANAGER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:
	CImageDisplay *m_pImageDisplay;

	CTabCtrl m_tabFrame;
	CListBox m_lstAlgorithm;
	CListBox m_listApplyList;

	long m_nImageWidth;
	long m_nImageHeight;
	BYTE* m_pbySrc;
	BYTE* m_pbyBuffer;
	BYTE* m_pbyDst;

	short nCurFrameID;
	short nAuxFrameID;
	short nCombineType;

	std::vector<short> m_vecnProc;
	std::vector<short> m_vecnProcParams;

	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnLbnDblclkListAlgorithm();
	afx_msg void OnLbnDblclkListApplyList();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

public:
	void SetData(BYTE* pbySrc, const long nImageWidth, const long nImageHeight, short* pProc, short* pProcParams, const long procCount);
	void UpdateApplyList();
	void DrawPreViewImage();
	void AllBufferDelete();
	short* GetProc();
	short* GetProcParams();
	long GetProcSize();
	long GetProcParamsSize();

	afx_msg void OnLbnSelchangeListApply();
	afx_msg void OnEnChangeEditParameter();
	afx_msg void OnBnClickedButtonAllClear();
	afx_msg void OnBnClickedButtonOriginal();
	afx_msg void OnBnClickedButtonProcimage();
};
