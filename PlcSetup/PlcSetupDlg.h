
// PlcSetupDlg.h: 헤더 파일
//

#pragma once

class CCtrlPLC;

enum class PLC : long
{
	IP,
	PORT,
	START_READ_ADDRESS,
	READ_COUNT,
	START_WRITE_ADDRESS,
	WRITE_COUNT,
};

// CPlcSetupDlg 대화 상자
class CPlcSetupDlg : public CDialogEx
{
// 생성입니다.
public:
	CPlcSetupDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CPlcSetupDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PLCSETUP_DIALOG };
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
	CCtrlPLC* m_pCtrlPLC;

	HICON m_hiconConnect;
	HICON m_hiconDisConnect;
	CStatic m_IconCtrl;

	CMFCPropertyGridCtrl m_propertyGridPLC;
	CGridCtrl m_gridPLC2Vision;
	CGridCtrl m_gridVision2PLC;

	CString m_strPath;
	BOOL m_bChange;

	BOOL m_bConnect;

	void InitPropertyGrid();
	void InitPLC2VisionGrid();
	void SetPLC2VisionGrid();
	void InitVision2PLCGrid();
	void SetVision2PLCGrid();

	void SetCtrlConnectIcon(const BOOL bConnect);

	LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateCtrlStatus(WPARAM wParam, LPARAM lParam);
	afx_msg void OnGridPLC2VisionEditEnd(NMHDR* pNotifyStruct, LRESULT* result);
	afx_msg void OnGridVision2PLCEditEnd(NMHDR* pNotifyStruct, LRESULT* result);
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnDisconnect();
	afx_msg void OnBnClickedBtnPlcSave();
	afx_msg void OnBnClickedBtnPlcLoad();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnStart();
	afx_msg void OnBnClickedBtnStop();
};
