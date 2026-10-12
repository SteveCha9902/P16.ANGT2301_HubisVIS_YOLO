#pragma once

class CDlgCalibration;

// CDlgAlgoCalibration 대화 상자

class CDlgAlgoCalibration : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgAlgoCalibration)

public:
	CDlgAlgoCalibration(CDlgCalibration* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgAlgoCalibration();

// 대화 상자 데이터입니다.
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_ALGO_CALIBRATION };
//#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	CDlgCalibration* m_pParent;

	float m_fAvgBlobWidth;
	float m_fAvgBlobHeight;
	float m_fPixelResolutionX;
	float m_fPixelResolutionY;

	void DoCalibration();
	virtual BOOL OnInitDialog();
	void UpdateUI();
};
