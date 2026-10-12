// CDlgAlgoCalibration.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgAlgoCalibration.h"
#include "afxdialogex.h"

#include "DlgCalibration.h"

// CDlgAlgoCalibration 대화 상자

IMPLEMENT_DYNAMIC(CDlgAlgoCalibration, CDialogEx)

CDlgAlgoCalibration::CDlgAlgoCalibration(CDlgCalibration* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_ALGO_CALIBRATION, pParent)
	, m_pParent(pParent)
{

}

CDlgAlgoCalibration::~CDlgAlgoCalibration()
{
}

void CDlgAlgoCalibration::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgAlgoCalibration, CDialogEx)
END_MESSAGE_MAP()


// CDlgAlgoCalibration 메시지 처리기

void CDlgAlgoCalibration::DoCalibration()
{
	CString strTemp;
	float fReal_X = 0.f;
	float fReal_Y = 0.f;

	GetDlgItemText(IDC_EDIT_REAL_X, strTemp);
	fReal_X = (float)_ttof(strTemp);

	GetDlgItemText(IDC_EDIT_REAL_Y, strTemp);
	fReal_Y = (float)_ttof(strTemp);

	if (m_fAvgBlobWidth == 0.f || m_fAvgBlobHeight == 0.f)
	{
		AfxMessageBox(_T("Blob을 검출해주세요."));
		return;
	}

	if (fReal_X == 0.f || fReal_Y == 0.f)
	{
		AfxMessageBox(_T("실제 크기 값을 입력해주세요."));
		return;
	}

	// mm 단위로 저장
	m_pParent->m_fPixelResolutionX = fReal_X / m_fAvgBlobWidth;
	m_pParent->m_fPixelResolutionY = fReal_Y / m_fAvgBlobHeight;

	strTemp.Format(_T("%0.3f"), m_pParent->m_fPixelResolutionX * 1000);
	SetDlgItemText(IDC_EDIT_RESOLUTION_X, strTemp);

	strTemp.Format(_T("%0.3f"), m_pParent->m_fPixelResolutionY * 1000);
	SetDlgItemText(IDC_EDIT_RESOLUTION_Y, strTemp);
}

BOOL CDlgAlgoCalibration::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_fAvgBlobWidth = 0.f;
	m_fAvgBlobHeight = 0.f;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CDlgAlgoCalibration::UpdateUI()
{
	CString strTemp;
	auto &blobInfo = ::g_InspectionManager.m_pInspection->m_vecBlobInfoList;

	if (blobInfo.size() > 0)
	{
		for (long idx = 0; idx < blobInfo.size(); idx++)
		{
			m_fAvgBlobWidth += blobInfo[idx].m_roi.Width();
			m_fAvgBlobHeight += blobInfo[idx].m_roi.Height();
		}

		m_fAvgBlobWidth /= blobInfo.size();
		m_fAvgBlobHeight /= blobInfo.size();

		strTemp.Format(_T("%0.3f"), m_fAvgBlobWidth);
		SetDlgItemText(IDC_EDIT_IMAGE_X, strTemp);

		strTemp.Format(_T("%0.3f"), m_fAvgBlobHeight);
		SetDlgItemText(IDC_EDIT_IMAGE_Y, strTemp);
	}
}
