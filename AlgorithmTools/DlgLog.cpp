// DlgLog.cpp: 구현 파일
//

#include "pch.h"
#include "AlgorithmTools.h"
#include "DlgLog.h"
#include "afxdialogex.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgLog 대화 상자

IMPLEMENT_DYNAMIC(CDlgLog, CDialogEx)

CDlgLog::CDlgLog(CLog* pLog, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_LOG, pParent)
	, m_pParent(pLog)
{

}

CDlgLog::~CDlgLog()
{
	::DeleteCriticalSection(&m_csLog);
}

void CDlgLog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LOG, m_logBox);
}


BEGIN_MESSAGE_MAP(CDlgLog, CDialogEx)
	ON_MESSAGE(UM_UPDATE_LOG, &CDlgLog::OnUpdateLog)
	ON_BN_CLICKED(IDC_BTN_SAVE_LOG, &CDlgLog::OnBnClickedBtnSaveLog)
	ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CDlgLog::OnBnClickedBtnClearLog)
END_MESSAGE_MAP()


// CDlgLog 메시지 처리기

BOOL CDlgLog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	::InitializeCriticalSection(&m_csLog);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Log 관련
LRESULT CDlgLog::OnUpdateLog(WPARAM wParam, LPARAM lParam)
{
	::EnterCriticalSection(&m_csLog);
	std::list<CString> pendingLog;
	std::swap(pendingLog, m_pendingLog);
	::LeaveCriticalSection(&m_csLog);

	for(const auto &str : pendingLog)
	{
		m_logBox.InsertString(0, str);

		if(m_logBox.GetCount() > 2000)
		{
			m_logBox.DeleteString(m_logBox.GetCount() - 1);
		}
	}

	return 1L;
}

void CDlgLog::SaveLog()
{
	CString strFolder;
	strFolder.Format(_T("%s\\Log"), m_pParent->GetCurPath());

	CreateDirectory(strFolder, nullptr);

	CFile file;
	CTime tmLog = CTime::GetCurrentTime();

	CString strFilePath;
	strFilePath.Format(_T("%s\\%s.txt"), strFolder, tmLog.Format(_T("%Y%m%d_%H%M%S")));
	if(file.Open(strFilePath, CFile::modeCreate | CFile::modeWrite))
	{
		for(long nRow = 0; nRow < m_logBox.GetCount(); nRow++)
		{
			CString strText;
			m_logBox.GetText(nRow, strText);
			strText += _T("\r\n");
			file.Write(strText, strText.GetLength() * sizeof(TCHAR));
		}
		file.Close();
		CString strMsg;
		strMsg.Format(_T("Save : %s"), strFilePath);
		AfxMessageBox(strMsg);
	}
}

void CDlgLog::ClearLog()
{
	m_logBox.ResetContent();
}

void CDlgLog::OnBnClickedBtnSaveLog()
{
	SaveLog();
}


void CDlgLog::OnBnClickedBtnClearLog()
{
	ClearLog();
}
