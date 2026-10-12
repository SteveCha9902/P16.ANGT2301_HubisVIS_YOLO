#pragma once


// CDlgInspItemSetup 대화 상자

class CDlgVisionMain;

enum enumFloatingMenuItem
{
	IDM_INSP_ITEM_ADD = 1000,
	IDM_INSP_ITEM_DEL,
	IDM_INSP_ITEM_COPY,
};

class CDlgInspItemSetup : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgInspItemSetup)

public:
	CDlgInspItemSetup(CDlgVisionMain* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgInspItemSetup();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_INSP_ITEM_SETUP };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CDlgVisionMain* m_pParent;
	CImageDisplay* m_pImageDisplay;
	CCtrlCamera* m_pCtrlCamera[2]; // Live 화면 출력을 위한 Camera 추가 - SHK 2023.12.27

	CTabCtrl m_tabCamera;
	CScrollBar m_scrollbarCh[ILLUM_CHANNEL_MAX];
	CListCtrl m_listInspectionItem;
	long m_nLastestSelectedInspItemIndex;
	CMFCPropertyGridCtrl m_propertyGridItemProperty;

	void InitUI();
	void InitPropertyGrid();
	void InitInspectionItemListControl();
	void SetInspectionItemListControl();
	void IllumOn();
	void IllumOff();
	void SetEachIllum(const long ch, const BYTE value);
	void CmdRouteMenu(CWnd* pWnd, CMenu* pPopupMenu);
	void UpdateImageDisplay(); // Live 화면 출력 - SHK 2023.12.27

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedBtnLive();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnBnClickedBtnCh1Zero();
	afx_msg void OnBnClickedBtnCh1Full();
	afx_msg void OnBnClickedBtnCh2Zero();
	afx_msg void OnBnClickedBtnCh2Full();
	afx_msg void OnBnClickedBtnCh3Zero();
	afx_msg void OnBnClickedBtnCh3Full();
	afx_msg void OnBnClickedBtnCh4Zero();
	afx_msg void OnBnClickedBtnCh4Full();
	afx_msg void OnBnClickedBtnIllumOn();
	afx_msg void OnBnClickedBtnIllumOff();
	afx_msg void OnTcnSelchangeTabCamera(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnInspItemAdd();
	afx_msg void OnInspItemDel();
	afx_msg void OnInspItemCopy();
	afx_msg void OnLvnItemchangedListInspectionItem(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickListInspectionItem(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnRecipeSave();
	afx_msg LRESULT OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam);
	
};
