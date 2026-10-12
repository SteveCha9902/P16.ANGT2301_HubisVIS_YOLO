#pragma once

class CDlgVisionMain;

#include "resource.h"
// CDlgLog 대화 상자

class CDlgLog : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgLog)

public:
	CDlgLog(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgLog();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_MAIN_LOG };
//#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgVisionMain* m_pParent;
};
