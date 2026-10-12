#pragma once

#include "resource.h"

// CDlgMain1 대화 상자

class CDlgVisionMain;

class CDlgMain1 : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgMain1)

public:
	CDlgMain1(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgMain1();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_MAIN1 };
//#endif

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgVisionMain* m_pParent;
	CImageDisplay* m_pImageDisplay;
	CCtrlCamera* m_pCtrlCamera;

	HICON m_hiconConnect;
	HICON m_hiconDisConnect;
	CStatic m_IconCtrl;
	CStatic m_IconLaser;
	CStatic m_IconCam;
	CStatic m_IconIllum;
	CStatic m_IconGrab;
	CStatic m_IconInsp[10];
	CLabel m_lblResult;
	CLabel m_lblUser;
	CFont m_Font;

	BOOL m_bConnectCamera;
	BOOL m_bConnectSerial;
	BOOL m_bResult;
	long m_nVisionIndex;
	long m_nDivIndex;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;

	void SetImage();
	void SetImageOverlay(const long inspIndex);
	void SetRealTime(const CString strTime);
	void SetResult(const long camIndex, const CString strResult);
	void SetUser(const long index);
	void SetCtrlConnectIcon(const BOOL bConnect);
	void SetLaserConnectIcon(const BOOL bConnect);
	void SetCamConnectIcon(const BOOL bConnect);
	void SetIllumConnectIcon(const BOOL bConnect);
	void SetGrabConnectIcon(const BOOL bGrab);
	void SetInspConnectIcon(const BOOL bInspection, const long index = 0);

	afx_msg void OnBnClickedBtnSetup();
	afx_msg void OnBnClickedBtnInspection();
	afx_msg void OnBnClickedBtnUserLogin();

	///////////////////////////////////////////////////////////////
	// Log 관련
	CRITICAL_SECTION m_csLog;
	std::list<CString> m_pendingLog;
	CListBox m_logBox;
	afx_msg LRESULT OnUpdateLog(WPARAM wParam, LPARAM lParam);
	//////////////////////////////////////////////////////////////
};
