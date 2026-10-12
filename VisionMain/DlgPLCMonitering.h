#pragma once

class CCtrlPLC;

// CDlgPLCMonitering 대화 상자

class CDlgPLCMonitering : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgPLCMonitering)

public:
	CDlgPLCMonitering(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgPLCMonitering();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_PLC_MONITERING };
//#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CCtrlPLC* m_pCtrlPLC;

	long m_nCurPLC;
	BOOL m_bConnect;

	CGridCtrl m_gridPLC2Vision;
	CGridCtrl m_gridVision2PLC;

	void StartRepeatReadData();
	void EndRepeatReadData();
	void InitPLC2VisionGrid();
	void SetPLC2VisionGrid();
	void SetPLC2VisionGridUpdate();
	void InitVision2PLCGrid();
	void SetVision2PLCGrid();
	void SetVision2PLCGridUpdate();

	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
