// DlgPLCMonitering.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgPLCMonitering.h"
#include "../AlgorithmTools/CtrlPLC.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgPLCMonitering 대화 상자

IMPLEMENT_DYNAMIC(CDlgPLCMonitering, CDialogEx)

CDlgPLCMonitering::CDlgPLCMonitering(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_PLC_MONITERING, pParent)
	, m_nCurPLC(0)
	, m_bConnect(FALSE)
{
}

CDlgPLCMonitering::~CDlgPLCMonitering()
{
}

void CDlgPLCMonitering::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GRID_PLC_TO_VISION, m_gridPLC2Vision);
	DDX_Control(pDX, IDC_GRID_VISION_TO_PLC, m_gridVision2PLC);
}


BEGIN_MESSAGE_MAP(CDlgPLCMonitering, CDialogEx)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CDlgPLCMonitering 메시지 처리기

BOOL CDlgPLCMonitering::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString strTemp;

	InitPLC2VisionGrid();
	SetPLC2VisionGrid();

	InitVision2PLCGrid();
	SetVision2PLCGrid();

	m_bConnect = ::g_PLCSocketManager.m_pCtrlPLC->IsConnect();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CDlgPLCMonitering::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
	case 0:
	{
		if(m_bConnect == TRUE)
		{
			// PLC polling thread owns PLC read/state parsing.
			// Monitoring UI only displays the latest cached snapshot to avoid racing m_bStart/job/div/barcode.
			SetPLC2VisionGridUpdate();
			SetVision2PLCGridUpdate();
		}
	}
	break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CDlgPLCMonitering::StartRepeatReadData()
{
	SetTimer(0, 500, NULL);
}

void CDlgPLCMonitering::EndRepeatReadData()
{
	KillTimer(0);
}

void CDlgPLCMonitering::InitPLC2VisionGrid()
{
	auto param = ::g_PLCManager.m_sPLC;

	m_gridPLC2Vision.GetDefaultCell(FALSE, TRUE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridPLC2Vision.GetDefaultCell(TRUE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridPLC2Vision.GetDefaultCell(FALSE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridPLC2Vision.SetGridLineColor(RGB(0, 0, 0));
	m_gridPLC2Vision.SetDefCellMargin(0);
	m_gridPLC2Vision.SetFixedColumnSelection(FALSE);
	m_gridPLC2Vision.SetFixedRowSelection(FALSE);
	m_gridPLC2Vision.SetSingleColSelection(FALSE);
	m_gridPLC2Vision.SetSingleRowSelection(FALSE);
	m_gridPLC2Vision.SetTrackFocusCell(FALSE);
	m_gridPLC2Vision.SetRowResize(FALSE);
	m_gridPLC2Vision.SetColumnResize(FALSE);

	m_gridPLC2Vision.SetColumnCount(6);
	m_gridPLC2Vision.SetFixedRowCount(1);
	m_gridPLC2Vision.SetFixedColumnCount(6);
	m_gridPLC2Vision.SetColumnWidth(0, 30);
	m_gridPLC2Vision.SetColumnWidth(1, 200);
	m_gridPLC2Vision.SetColumnWidth(2, 100);
	m_gridPLC2Vision.SetColumnWidth(3, 70);
	m_gridPLC2Vision.SetColumnWidth(4, 50);
	m_gridPLC2Vision.SetColumnWidth(5, 50);

	m_gridPLC2Vision.SetItemText(0, 0, _T("No."));
	m_gridPLC2Vision.SetItemText(0, 1, _T("Name"));
	m_gridPLC2Vision.SetItemText(0, 2, _T("Address"));
	m_gridPLC2Vision.SetItemText(0, 3, _T("Type"));
	m_gridPLC2Vision.SetItemText(0, 4, _T("Bit"));
	m_gridPLC2Vision.SetItemText(0, 5, _T("Value"));

	m_gridPLC2Vision.DeleteNonFixedRows();
	m_gridPLC2Vision.SetRowCount(param.nPLC2VisionCnt + 1);
}

void CDlgPLCMonitering::SetPLC2VisionGrid()
{
	CString strTemp;

	auto param = ::g_PLCManager.m_sPLC;

	for(long idx = 0; idx < param.nPLC2VisionCnt; idx++)
	{
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 0, _T("%d"), idx + 1);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 1, _T("%s"), param.vecsPLC2VisionData[idx].strName);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 2, _T("%d"), param.vecsPLC2VisionData[idx].nAddr);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 4, _T("%d"), param.vecsPLC2VisionData[idx].nBitNum);

		switch(param.vecsPLC2VisionData[idx].nType)
		{
		case (long)PLC_MEMORY_TYPE::BIT:
		{
			m_gridPLC2Vision.SetItemTextFmt(idx + 1, 3, _T("BIT"));
			if(param.vecsPLC2VisionData[idx].sValue == 0)
				strTemp.Format(_T("OFF"));
			else
				strTemp.Format(_T("ON"));
		}
		break;
		case (long)PLC_MEMORY_TYPE::INT:
		{
			m_gridPLC2Vision.SetItemTextFmt(idx + 1, 3, _T("INT"));
			strTemp.Format(_T("%d"), param.vecsPLC2VisionData[idx].sValue);
		}
		break;
		case (long)PLC_MEMORY_TYPE::WORD:
		{
			m_gridPLC2Vision.SetItemTextFmt(idx + 1, 3, _T("WORD"));
			char pString[3] = {0,};
			memcpy(pString, &param.vecsPLC2VisionData[idx].sValue, 2);
			strTemp.Format(_T("%s"), (CString)pString);
		}
		break;
		case (long)PLC_MEMORY_TYPE::DOUBLE:
		{
			m_gridPLC2Vision.SetItemTextFmt(idx + 1, 3, _T("DOUBLE"));
			strTemp.Format(_T("%d"), param.vecsPLC2VisionData[idx].nValue);
		}
		break;
		}

		COLORREF color(RGB(0, 0, 0));
		if(strTemp == _T("ON") || strTemp != _T("0"))
		{
			if(strTemp == _T("OFF") || strTemp == _T(""))
			{
				color = RGB(255, 0, 0);
			}
			else
			{
				color = RGB(0, 255, 0);
			}
		}
		else if(strTemp == _T("0"))
		{
			color = RGB(255, 0, 0);
		}

		m_gridPLC2Vision.SetItemBkColour(idx + 1, 5, color);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 5, strTemp);
	}

	Invalidate(FALSE);
}

void CDlgPLCMonitering::SetPLC2VisionGridUpdate()
{
	CString strTemp;

	auto param = ::g_PLCManager.m_sPLC;

	for(long idx = 0; idx < param.nPLC2VisionCnt; idx++)
	{
		switch(param.vecsPLC2VisionData[idx].nType)
		{
		case (long)PLC_MEMORY_TYPE::BIT:
		{
			if(param.vecsPLC2VisionData[idx].sValue == 0)
				strTemp.Format(_T("OFF"));
			else
				strTemp.Format(_T("ON"));
		}
		break;
		case (long)PLC_MEMORY_TYPE::INT:
		{
			strTemp.Format(_T("%d"), param.vecsPLC2VisionData[idx].sValue);
		}
		break;
		case (long)PLC_MEMORY_TYPE::WORD:
		{
			char pString[3] = {0,};
			memcpy(pString, &param.vecsPLC2VisionData[idx].sValue, 2 );
			strTemp.Format(_T("%s"), (CString)pString);
		}
		break;
		case (long)PLC_MEMORY_TYPE::DOUBLE:
		{
			strTemp.Format(_T("%d"), param.vecsPLC2VisionData[idx].nValue);
		}
		break;
		}

		COLORREF color(RGB(0, 0, 0));
		if(strTemp == _T("ON") || strTemp != _T("0"))
		{
			if(strTemp == _T("OFF") || strTemp == _T(""))
			{
				color = RGB(255, 0, 0);
			}
			else
			{
				color = RGB(0, 255, 0);
			}
		}
		else if(strTemp == _T("0"))
		{
			color = RGB(255, 0, 0);
		}

		m_gridPLC2Vision.SetItemBkColour(idx + 1, 5, color);
		m_gridPLC2Vision.SetItemTextFmt(idx + 1, 5, strTemp);
	}

	Invalidate(FALSE);
}

void CDlgPLCMonitering::InitVision2PLCGrid()
{
	auto param = ::g_PLCManager.m_sPLC;

	m_gridVision2PLC.GetDefaultCell(FALSE, TRUE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridVision2PLC.GetDefaultCell(TRUE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridVision2PLC.GetDefaultCell(FALSE, FALSE)->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	m_gridVision2PLC.SetGridLineColor(RGB(0, 0, 0));
	m_gridVision2PLC.SetDefCellMargin(0);
	m_gridVision2PLC.SetFixedColumnSelection(FALSE);
	m_gridVision2PLC.SetFixedRowSelection(FALSE);
	m_gridVision2PLC.SetSingleColSelection(FALSE);
	m_gridVision2PLC.SetSingleRowSelection(FALSE);
	m_gridVision2PLC.SetTrackFocusCell(FALSE);
	m_gridVision2PLC.SetRowResize(FALSE);
	m_gridVision2PLC.SetColumnResize(FALSE);

	m_gridVision2PLC.SetColumnCount(6);
	m_gridVision2PLC.SetFixedRowCount(1);
	m_gridVision2PLC.SetFixedColumnCount(6);
	m_gridVision2PLC.SetColumnWidth(0, 30);
	m_gridVision2PLC.SetColumnWidth(1, 200);
	m_gridVision2PLC.SetColumnWidth(2, 100);
	m_gridVision2PLC.SetColumnWidth(3, 70);
	m_gridVision2PLC.SetColumnWidth(4, 50);
	m_gridVision2PLC.SetColumnWidth(5, 50);

	m_gridVision2PLC.SetItemText(0, 0, _T("No."));
	m_gridVision2PLC.SetItemText(0, 1, _T("Name"));
	m_gridVision2PLC.SetItemText(0, 2, _T("Address"));
	m_gridVision2PLC.SetItemText(0, 3, _T("Type"));
	m_gridVision2PLC.SetItemText(0, 4, _T("Bit"));
	m_gridVision2PLC.SetItemText(0, 5, _T("Value"));

	m_gridVision2PLC.DeleteNonFixedRows();
	m_gridVision2PLC.SetRowCount(param.nVision2PLCCnt + 1);
}

void CDlgPLCMonitering::SetVision2PLCGrid()
{
	CString strTemp;

	auto param = ::g_PLCManager.m_sPLC;

	for(long idx = 0; idx < param.nVision2PLCCnt; idx++)
	{
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 0, _T("%d"), idx + 1);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 1, _T("%s"), param.vecsVision2PLCData[idx].strName);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 2, _T("%d"), param.vecsVision2PLCData[idx].nAddr);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 4, _T("%d"), param.vecsVision2PLCData[idx].nBitNum);

		switch(param.vecsVision2PLCData[idx].nType)
		{
		case (long)PLC_MEMORY_TYPE::BIT:
		{
			m_gridVision2PLC.SetItemTextFmt(idx + 1, 3, _T("BIT"));
			if(param.vecsVision2PLCData[idx].sValue == 0)
				strTemp.Format(_T("OFF"));
			else
				strTemp.Format(_T("ON"));
		}
		break;
		case (long)PLC_MEMORY_TYPE::INT:
		{
			m_gridVision2PLC.SetItemTextFmt(idx + 1, 3, _T("INT"));
			strTemp.Format(_T("%d"), param.vecsVision2PLCData[idx].sValue);
		}
		break;
		case (long)PLC_MEMORY_TYPE::WORD:
		{
			m_gridVision2PLC.SetItemTextFmt(idx + 1, 3, _T("WORD"));
			char pString[3] = {0,};
			memcpy(pString, &param.vecsVision2PLCData[idx].sValue, 2);
			strTemp.Format(_T("%s"), (CString)pString);
		}
		break;
		case (long)PLC_MEMORY_TYPE::DOUBLE:
		{
			m_gridVision2PLC.SetItemTextFmt(idx + 1, 3, _T("DOUBLE"));
			strTemp.Format(_T("%d"), param.vecsVision2PLCData[idx].nValue);
		}
		break;
		}

		COLORREF color(RGB(0, 0, 0));
		if(strTemp == _T("ON") || strTemp != _T("0"))
		{
			if(strTemp == _T("OFF") || strTemp == _T(""))
			{
				color = RGB(255, 0, 0);
			}
			else
			{
				color = RGB(0, 255, 0);
			}
		}
		else if(strTemp == _T("0"))
		{
			color = RGB(255, 0, 0);
		}

		m_gridVision2PLC.SetItemBkColour(idx + 1, 5, color);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 5, strTemp);
	}

	Invalidate(FALSE);
}

void CDlgPLCMonitering::SetVision2PLCGridUpdate()
{
	CString strTemp;

	auto param = ::g_PLCManager.m_sPLC;

	for(long idx = 0; idx < param.nVision2PLCCnt; idx++)
	{
		switch(param.vecsVision2PLCData[idx].nType)
		{
		case (long)PLC_MEMORY_TYPE::BIT:
		{
			if(param.vecsVision2PLCData[idx].sValue == 0)
				strTemp.Format(_T("OFF"));
			else
				strTemp.Format(_T("ON"));
		}
		break;
		case (long)PLC_MEMORY_TYPE::INT:
		{
			strTemp.Format(_T("%d"), param.vecsVision2PLCData[idx].sValue);
		}
		break;
		case (long)PLC_MEMORY_TYPE::WORD:
		{
			char pString[3] = {0,};
			memcpy(pString, &param.vecsVision2PLCData[idx].sValue, 2);
			strTemp.Format(_T("%s"), (CString)pString);
		}
		break;
		case (long)PLC_MEMORY_TYPE::DOUBLE:
		{
			strTemp.Format(_T("%d"), param.vecsVision2PLCData[idx].nValue);
		}
		break;
		}

		COLORREF color(RGB(0, 0, 0));
		if(strTemp == _T("ON") || strTemp != _T("0"))
		{
			if(strTemp == _T("OFF") || strTemp == _T(""))
			{
				color = RGB(255, 0, 0);
			}
			else
			{
				color = RGB(0, 255, 0);
			}
		}
		else if(strTemp == _T("0"))
		{
			color = RGB(255, 0, 0);
		}

		m_gridVision2PLC.SetItemBkColour(idx + 1, 5, color);
		m_gridVision2PLC.SetItemTextFmt(idx + 1, 5, strTemp);
	}

	Invalidate(FALSE);
}
