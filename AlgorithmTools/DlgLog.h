#pragma once

#include "resource.h"

class CLog;

// CDlgLog 대화 상자

class CDlgLog : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgLog)

public:
	CDlgLog(CLog* pLog, CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgLog();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_LOG };
//#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CLog* m_pParent;

	///////////////////////////////////////////////////////////////
	// Log 관련
	CRITICAL_SECTION m_csLog;
	std::list<CString> m_pendingLog;
	CListBox m_logBox;
	void SaveLog();
	void ClearLog();
	afx_msg LRESULT OnUpdateLog(WPARAM wParam, LPARAM lParam);
	//////////////////////////////////////////////////////////////
	afx_msg void OnBnClickedBtnSaveLog();
	afx_msg void OnBnClickedBtnClearLog();
};
