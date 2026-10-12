#pragma once

class CDlgLog;
class CDlgMain1;
class CDlgMain2;
class CDlgPLCMonitering;

// CDlgVisionMain 대화 상자

class CDlgVisionMain : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgVisionMain)

public:
	CDlgVisionMain(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgVisionMain();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_VISION_MAIN };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgLog* m_pDlgLog;
	CDlgMain1* m_pDlgMain1;
	CDlgMain2* m_pDlgMain2;
	CDlgPLCMonitering* m_pDlgPLCMonitering;

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;

	CString m_strCurPath;
	CString m_strCurRecipePath;
	CString m_strCurRecipeName;
	CString m_strMessage;
	BOOL m_bMessageDisplaying;
	BOOL m_bLinkTest;
	BOOL m_bReceiveRecipeOpen;

	BOOL LoadRecipe(const LPCTSTR szPathName);
	BOOL SaveRecipe(const LPCTSTR szPathName);
	BOOL ImageOpen(const CString strPath, const long camindex = 0, const long divindex = 0);
	BOOL ImageSave(const CString strPath, const long camindex = 0, const long divindex = 0);
	void ShowSetupDialog();
	void ShowUserLoginDialog();

	void ShowUser(const long index);
	CString SetVersion(const CString strRecipeName, BOOL bDisplay = TRUE);

	LRESULT OnUpdateImage(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateResult(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateGrabStatus(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateInspStatus(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateLaserStatus(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateCtrlStatus(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateRecipeUpdate(WPARAM wParam, LPARAM lParam);
	
	afx_msg void OnFileNewrecipe();
	afx_msg void OnFileRecipeopen();
	afx_msg void OnFileRecipesave();
	afx_msg void OnFileReciperesist();
	afx_msg void OnImageMenu1Imageopen();
	afx_msg void OnImageMenu1Imagesave();
	afx_msg void OnCalibrationCalibration();
	afx_msg void OnLogShowlog();
	afx_msg void OnImageMenu2Imageopen2();
	afx_msg void OnImageMenu2Imagesave2();
	afx_msg void OnInspectionInspectionitemsetup();
	afx_msg void OnInspectionBatchinspection();
	afx_msg void OnInspectionInspectionitemmatch();

	///////////////////////////////////////////////////////////////
	// CrashDump 관련...
	static void UserExceptionHandlerPre();
	static void UserExceptionHandlerPost();
	///////////////////////////////////////////////////////////////
	afx_msg void OnCommunicationPlcmonitering();
};
