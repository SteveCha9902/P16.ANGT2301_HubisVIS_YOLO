
// VisionSPCDlg.h: 헤더 파일
//

#pragma once

// CVisionSPCDlg 대화 상자
class CVisionSPCDlg : public CDialogEx
{
// 생성입니다.
public:
	CVisionSPCDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CVisionSPCDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VISIONSPC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CImageDisplay* m_pImageDisplay;
	CListCtrl m_listFile;
	CListCtrl m_listResult;
	CDateTimeCtrl m_date_Start;
	CDateTimeCtrl m_date_End;
	CTabCtrl tabImage;

	CString m_strPath;

	std::vector<CString> m_vecstrFileList;
	std::vector<CString> m_vecstrResultList;
	std::vector<CString> m_vecstrImagePath;
	std::vector<CString> m_vecstrDate;

	std::vector<BYTE*> m_vecpyRed;
	std::vector<BYTE*> m_vecpyGreen;
	std::vector<BYTE*> m_vecpyBlue;

	CString m_strModel;
	CString m_strBarcode;

	long m_nImageNum;
	long m_nImageWidth;
	long m_nImageHeight;
	long m_nCurrIndex;

	void InitFileListControl();
	void InitResultListControl();
	void InitTabImage();

	void SetFileListControl();
	void SetResultListControl();
	void SetTabImage();
	void UpdateImageDisplay();

	void DoSearch(CString strPath);
	BOOL ResultFileOpen();
	void DivideToken(std::vector<CString> &vecResult, const CString &strSource, CString strToken);
	BOOL ResultImageOpen();

	afx_msg void OnTcnSelchangeTabImage(NMHDR *pNMHDR, LRESULT *pResult); // Image 변경 시켜줘야함
	afx_msg void OnNMClickListFileList(NMHDR *pNMHDR, LRESULT *pResult); // Open할 파일 인덱스 수정해줘야함
	afx_msg void OnBnClickedBtnSearch(); //해당 조건에 맞는 File list 뿌려줘야함
	afx_msg void OnBnClickedBtnOpen(); // 선택된 인덱스 결과 오픈 해야함 -> 이미지랑 csv
};
