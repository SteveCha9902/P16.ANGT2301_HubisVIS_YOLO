// DlgLog.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgVisionMain.h"
#include "DlgLog.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgLog 대화 상자

IMPLEMENT_DYNAMIC(CDlgLog, CDialogEx)

CDlgLog::CDlgLog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_MAIN_LOG, pParent)
{

}

CDlgLog::~CDlgLog()
{
}

void CDlgLog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgLog, CDialogEx)
END_MESSAGE_MAP()


// CDlgLog 메시지 처리기

BOOL CDlgLog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}
