
// ConfigSetupDlg.h: 헤더 파일
//

#pragma once

class CConfigManager;

enum class CONFIG : long
{
	HARDWARE_EXIST,
	MAIN_UI,
	CAMERA_NO,
	CAMERA_TYPE,
	CAMERA_NAME,
	CAMERA_IP,
	CAMERA_MODE,
	IMAGE_WIDTH,
	IMAGE_HEIGHT,
	DIST_CAMERA_TO_CAMERA,
	DIVISION_INDEX,
	MULTI_ILLUMINATION,
	LED_PORT,
	LED_BAUDRATE,
	USE_LASER_TCPIP,
	LASER_TCPIP_NUM,
	LASER_TCPIP_NO,
	LASER_IP,
	LASER_PORT,
	LASER_CONNECT_TIME,
	USE_PLC_TCPIP,
	PLC_IP,
	PLC_PORT,
	PLC_CONNECT_TIME,
	RAW_IMAGE_SAVE_PATH,
	RESULT_IMAGE_SAVE_PATH,
	RESULT_DATA_SAVE_PATH,
	USE_AUTO_DELETE,
	AUTO_DELETE_DAYS,
	AUTO_DELETE_TIMES,
};

// CConfigSetupDlg 대화 상자
class CConfigSetupDlg : public CDialogEx
{
// 생성입니다.
public:
	CConfigSetupDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONFIGSETUP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


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
	CMFCPropertyGridCtrl m_propertyGridConfig;
	CConfigManager m_ConfigManager;

	CString m_strPath;
	BOOL m_bChange;

	void InitPropertyGrid();

	afx_msg void OnBnClickedOk();

protected:
	afx_msg LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnBnClickedBtnLoad();
};
