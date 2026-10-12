// DlgBatchInspection.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgBatchInspection.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgBatchInspection 대화 상자

IMPLEMENT_DYNAMIC(CDlgBatchInspection, CDialogEx)

CDlgBatchInspection::CDlgBatchInspection(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_BATCH_INSPECTION, pParent)
{

}

CDlgBatchInspection::~CDlgBatchInspection()
{
}

void CDlgBatchInspection::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgBatchInspection, CDialogEx)
END_MESSAGE_MAP()


// CDlgBatchInspection 메시지 처리기
