#pragma once


// CDlgBatchInspection 대화 상자

class CDlgBatchInspection : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgBatchInspection)

public:
	CDlgBatchInspection(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgBatchInspection();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_BATCH_INSPECTION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
};
