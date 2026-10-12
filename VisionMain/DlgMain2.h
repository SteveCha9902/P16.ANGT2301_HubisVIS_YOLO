#pragma once

#define MAX_DISPLAY_NUM	6
#define MAX_CAMERA_NUM	2
// CDlgMain2 대화 상자

// 250619 sjpark YOLO Bound box 정보 구조체 선언
//struct YOLOBox {
//	CString class_name;
//	float confidence;
//	float x1, y1, x2, y2;
//};

class CDlgVisionMain;

class CDlgMain2 : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgMain2)

public:
	CDlgMain2(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgMain2();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_MAIN2 };
//#endif

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgVisionMain* m_pParent;
	CImageDisplay* m_pImageDisplay[MAX_DISPLAY_NUM];
	CCtrlCamera* m_pCtrlCamera[MAX_CAMERA_NUM];

	HICON m_hiconConnect;
	HICON m_hiconDisConnect;
	CStatic m_IconCtrl;
	CStatic m_IconLaser;
	CStatic m_IconCam[MAX_CAMERA_NUM];
	CStatic m_IconIllum;
	CStatic m_IconGrab[5];
	CStatic m_IconInsp[10];
	CLabel m_lblResult[MAX_DISPLAY_NUM+1];
	CLabel m_lblUser;
	CLabel m_lblRecipe;
	CGridCtrl m_gridResult;

	BOOL m_bConnectCamera[MAX_CAMERA_NUM];
	BOOL m_bConnectSerial;
	BOOL m_bResult[MAX_DISPLAY_NUM+1];
	CFont m_Font;

	long m_nImageWidth[MAX_DISPLAY_NUM];
	long m_nImageHeight[MAX_DISPLAY_NUM];
	long m_nImageSize[MAX_DISPLAY_NUM];
	long m_nVisionIndex;
	long m_nDivIndex;

	void InitResultGrid();
	void SetResultGrid();
	void UpdateResultGrid(const BOOL bManual, const long visionIndex, const long divIndex, const long threadIndex);
	void SetImage(const long camIndex, const long divIndex);
	void SetImageOverlay(const long camIndex, const long inspIndex);
	void SetRealTime(const CString strTime);
	void SetResult(const long dispIndex, const CString strResult);
	void SetUser(const long index);
	void SetRecipe(const CString strRecipeName);
	void SetCtrlConnectIcon(const BOOL bConnect);
	void SetLaserConnectIcon(const BOOL bConnect);
	void SetCamConnectIcon(const BOOL bConnect, const long index = 0);
	void SetIllumConnectIcon(const BOOL bConnect);
	void SetGrabConnectIcon(const BOOL bGrab, const long index = 0);
	void SetInspConnectIcon(const BOOL bInspection, const long index = 0);

	afx_msg void OnBnClickedBtnSetup();
	afx_msg void OnBnClickedBtnInspection();
	afx_msg void OnBnClickedBtnUserLogin();

	void InspectionCam1(int iCam, int iIdx);
	void InspectionCam2(int iCam, int iIdx);

	///////////////////////////////////////////////////////////////
	// Log 관련
	CRITICAL_SECTION m_csLog;
	std::list<CString> m_pendingLog;
	CListBox m_logBox;
	afx_msg LRESULT OnUpdateLog(WPARAM wParam, LPARAM lParam);
	//////////////////////////////////////////////////////////////
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnTcnSelchangeTabViewer(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnTestBatch();
	
	void ReadYOLOSharedMemory();

	// 250619 sjpark    Socket 관련 내용 추가 시작
	//
private:
	SOCKET m_sock = INVALID_SOCKET;
	bool m_connected = FALSE;
public:
	bool InitSocketClient();
	void CleanupSocket();
	BOOL SocketYOLO(const CString& imagePath);
	std::vector<YOLOBox> ParseYOLOResult(const CString& jsonStr);
	std::vector<YOLOBox> m_vecYOLOResultBox;
	// 250619 sjpark    Socket 관련 내용 추가 끝

	CButton m_btnSetup; // PSJ
	CLabel m_lblTotalResult;
};
