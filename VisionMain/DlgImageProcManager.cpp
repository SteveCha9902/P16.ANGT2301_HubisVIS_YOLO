// DlgImageProcManager.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgImageProcManager.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CDlgImageProcManager 대화 상자

IMPLEMENT_DYNAMIC(CDlgImageProcManager, CDialog)

void swap(short &a, short &b)
{
	short temp = a;
	a = b;
	b = temp;
}

CDlgImageProcManager::CDlgImageProcManager(BYTE* pbySrc, const long nImageWidth, const long nImageHeight, short* pProc, short* pProcParams, const long procCount, CWnd* pParent /*=NULL*/)
	: CDialog(IDD_DLG_IMAGE_PROC_MANAGER, pParent)
{
	SetData(pbySrc, nImageWidth, nImageHeight, pProc, pProcParams, procCount);
}

CDlgImageProcManager::~CDlgImageProcManager()
{
	AllBufferDelete();

	delete m_pImageDisplay;
}

void CDlgImageProcManager::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_ALGORITHM, m_lstAlgorithm);
	DDX_Control(pDX, IDC_LIST_APPLY, m_listApplyList);
}


BEGIN_MESSAGE_MAP(CDlgImageProcManager, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_UP, &CDlgImageProcManager::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &CDlgImageProcManager::OnBnClickedButtonDown)
	ON_LBN_DBLCLK(IDC_LIST_ALGORITHM, &CDlgImageProcManager::OnLbnDblclkListAlgorithm)
	ON_LBN_DBLCLK(IDC_LIST_APPLY, &CDlgImageProcManager::OnLbnDblclkListApplyList)
	ON_BN_CLICKED(IDOK, &CDlgImageProcManager::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDlgImageProcManager::OnBnClickedCancel)
	ON_LBN_SELCHANGE(IDC_LIST_APPLY, &CDlgImageProcManager::OnLbnSelchangeListApply)
	ON_EN_CHANGE(IDC_EDIT_PARAMETER, &CDlgImageProcManager::OnEnChangeEditParameter)
	ON_BN_CLICKED(IDC_BUTTON_ALL_CLEAR, &CDlgImageProcManager::OnBnClickedButtonAllClear)
	ON_BN_CLICKED(IDC_BUTTON_ORIGINAL, &CDlgImageProcManager::OnBnClickedButtonOriginal)
	ON_BN_CLICKED(IDC_BUTTON_PROCIMAGE, &CDlgImageProcManager::OnBnClickedButtonProcimage)
END_MESSAGE_MAP()


// CDlgImageProcManager 메시지 처리기
BOOL CDlgImageProcManager::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString strTemp;

	m_pImageDisplay->Initialize(IDC_COMBINE_PREVIEW, this);

	for(long idx = (long)HUBSDK::IMAGE_PROC::BEGIN; idx < (long)HUBSDK::IMAGE_PROC::END; idx++)
	{
		m_lstAlgorithm.AddString(HUBSDK::gl_szStrAlgoName[idx]);
	}

	UpdateApplyList();

	strTemp.Format(_T("0"));
	GetDlgItem(IDC_EDIT_PARAMETER)->SetWindowTextW(strTemp);
	GetDlgItem(IDC_EDIT_PARAMETER)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CDlgImageProcManager::SetData(BYTE* pbySrc, const long nImageWidth, const long nImageHeight, short* pProc, short* pProcParams, const long procCount)
{
	m_pImageDisplay = new CImageDisplay;

	m_pbySrc = pbySrc;
	m_nImageWidth = nImageWidth;
	m_nImageHeight = nImageHeight;

	m_vecnProc.clear();
	m_vecnProcParams.clear();
	m_vecnProc.resize(procCount);
	m_vecnProcParams.resize(procCount);

	for(long proc = 0; proc < procCount; proc++)
	{
		m_vecnProc[proc] = pProc[proc];
		m_vecnProcParams[proc] = pProcParams[proc];
	}

	m_pbyBuffer = new BYTE[m_nImageWidth * m_nImageHeight];
	m_pbyDst = new BYTE[m_nImageWidth * m_nImageHeight];
}

void CDlgImageProcManager::UpdateApplyList()
{
	CString strTemp;

	m_listApplyList.ResetContent();

	for(long idx = 0; idx < (long)m_vecnProc.size(); idx++)
	{
		auto procIndex = m_vecnProc[idx];

		if(procIndex == (long)HUBSDK::IMAGE_PROC::OFFSET || procIndex == (long)HUBSDK::IMAGE_PROC::CONTRAST || procIndex == (long)HUBSDK::IMAGE_PROC::ADAPTIVE_THRESHOLD)
		{
			strTemp.Format(_T("%s(%d)"), HUBSDK::gl_szStrAlgoName[procIndex], m_vecnProcParams[idx]);
		}
		else
		{
			strTemp.Format(_T("%s"), HUBSDK::gl_szStrAlgoName[procIndex]);
		}

		m_listApplyList.AddString(strTemp);
	}

	DrawPreViewImage();
}

void CDlgImageProcManager::OnBnClickedButtonUp()
{
	long nCurSel = m_listApplyList.GetCurSel();

	if(nCurSel <= 0)
		return;

	swap(m_vecnProc[nCurSel], m_vecnProc[nCurSel - 1]);
	swap(m_vecnProcParams[nCurSel], m_vecnProcParams[nCurSel - 1]);

	UpdateApplyList();

	m_listApplyList.SetCurSel(nCurSel - 1);
}

void CDlgImageProcManager::OnBnClickedButtonDown()
{
	long nCurSel = m_listApplyList.GetCurSel();

	if(nCurSel < 0)
		return;

	if(nCurSel >= m_vecnProc.size() - 1)
		return;

	swap(m_vecnProc[nCurSel], m_vecnProc[nCurSel + 1]);
	swap(m_vecnProcParams[nCurSel], m_vecnProcParams[nCurSel + 1]);

	UpdateApplyList();

	m_listApplyList.SetCurSel(nCurSel + 1);
}

void CDlgImageProcManager::OnBnClickedButtonAllClear()
{
	m_vecnProc.clear();
	m_vecnProcParams.clear();

	UpdateApplyList();
}


void CDlgImageProcManager::OnLbnDblclkListAlgorithm()
{
	short nAlgo = (short)m_lstAlgorithm.GetCurSel();
	short nDefault = 0;

	if(nAlgo >= (long)HUBSDK::IMAGE_PROC::BEGIN && nAlgo < (long)HUBSDK::IMAGE_PROC::END)
	{
		m_vecnProc.push_back(nAlgo);
		m_vecnProcParams.push_back(nDefault);
	}

	UpdateApplyList();
}

void CDlgImageProcManager::OnLbnDblclkListApplyList()
{
	long nCurSel = m_listApplyList.GetCurSel();

	if(nCurSel < 0)
	{
		return;
	}

	m_vecnProc.erase(m_vecnProc.begin() + nCurSel);
	m_vecnProcParams.erase(m_vecnProcParams.begin() + nCurSel);

	UpdateApplyList();
}

void CDlgImageProcManager::OnLbnSelchangeListApply()
{
	CString strTemp;

	long nCurSel = m_listApplyList.GetCurSel();

	if(nCurSel < 0)
	{
		return;
	}

	auto procIndex = m_vecnProc[nCurSel];

	if(procIndex == (long)HUBSDK::IMAGE_PROC::OFFSET || procIndex == (long)HUBSDK::IMAGE_PROC::CONTRAST || procIndex == (long)HUBSDK::IMAGE_PROC::ADAPTIVE_THRESHOLD)
	{
		strTemp.Format(_T("%d"), m_vecnProcParams[nCurSel]);

		GetDlgItem(IDC_EDIT_PARAMETER)->SetWindowTextW(strTemp);
		GetDlgItem(IDC_EDIT_PARAMETER)->EnableWindow(TRUE);
	}
	else
	{
		strTemp.Format(_T("0"));

		GetDlgItem(IDC_EDIT_PARAMETER)->SetWindowTextW(strTemp);
		GetDlgItem(IDC_EDIT_PARAMETER)->EnableWindow(FALSE);
	}

	m_listApplyList.SetCurSel(nCurSel);
}

void CDlgImageProcManager::DrawPreViewImage()
{
	HUBIP::ImageCopy(m_pbySrc, m_nImageWidth, m_nImageHeight, m_pbyDst);

	if((long)m_vecnProc.size() > 0 && (long)m_vecnProcParams.size() > 0)
	{
		HUBIP::ImageProcessingManage(m_pbySrc, m_nImageWidth, m_nImageHeight, CRect(0, 0, m_nImageWidth, m_nImageHeight), &m_vecnProc[0], &m_vecnProcParams[0], (long)m_vecnProc.size(), m_pbyDst);
	}

	m_pImageDisplay->SetImage(m_pbyDst, m_nImageWidth, m_nImageHeight, m_nImageWidth, FALSE);

	m_pImageDisplay->ShowWindow(SW_SHOW);
}

void CDlgImageProcManager::OnBnClickedOk()
{
	AllBufferDelete();

	delete[] m_pbyDst;

	CDialog::OnOK();
}


void CDlgImageProcManager::OnBnClickedCancel()
{
	AllBufferDelete();

	CDialog::OnCancel();
}

void CDlgImageProcManager::AllBufferDelete()
{
	if(m_pbyBuffer != nullptr)
	{
		delete[] m_pbyBuffer;
		m_pbyBuffer = nullptr;
	}

	if(m_pbyDst != nullptr)
	{
		delete[] m_pbyDst;
		m_pbyDst = nullptr;
	}
}

short* CDlgImageProcManager::GetProc()
{
	return (long)m_vecnProc.size() ? &m_vecnProc[0] : nullptr;
}

long CDlgImageProcManager::GetProcSize()
{
	return (long)m_vecnProc.size();
}

short* CDlgImageProcManager::GetProcParams()
{
	return (long)m_vecnProcParams.size() ? &m_vecnProcParams[0] : nullptr;
}

long CDlgImageProcManager::GetProcParamsSize()
{
	return (long)m_vecnProcParams.size();
}


void CDlgImageProcManager::OnEnChangeEditParameter()
{
	CString strTemp;

	long nCurSel = m_listApplyList.GetCurSel();

	if(nCurSel < 0)
	{
		return;
	}

	auto procIndex = m_vecnProc[nCurSel];

	if(procIndex == (long)HUBSDK::IMAGE_PROC::OFFSET || procIndex == (long)HUBSDK::IMAGE_PROC::CONTRAST || procIndex == (long)HUBSDK::IMAGE_PROC::ADAPTIVE_THRESHOLD)
	{
		GetDlgItem(IDC_EDIT_PARAMETER)->GetWindowTextW(strTemp);

		m_vecnProcParams[nCurSel] = (short)_ttol(strTemp);
	}

	UpdateApplyList();

	m_listApplyList.SetCurSel(nCurSel);
}



void CDlgImageProcManager::OnBnClickedButtonOriginal()
{
	m_pImageDisplay->SetImage(m_pbySrc, m_nImageWidth, m_nImageHeight, m_nImageWidth, FALSE);
}


void CDlgImageProcManager::OnBnClickedButtonProcimage()
{
	m_pImageDisplay->SetImage(m_pbyDst, m_nImageWidth, m_nImageHeight, m_nImageWidth, FALSE);
}
