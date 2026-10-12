#include "pch.h"
#include "OverlayManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

COverlayManager g_OverlayManager;

COverlayManager::COverlayManager()
{
	AddStep(DEBUG_ROI_SEARCH_ROI, gl_szStrDebugName[DEBUG_ROI_SEARCH_ROI], RGB(255, 0, 0));
	AddStep(DEBUG_ROI_IGNORE_ROI, gl_szStrDebugName[DEBUG_ROI_IGNORE_ROI], RGB(255, 255, 0));

	AddStep(DEBUG_EDGE_DETECT_SEARCH_ROI, gl_szStrDebugName[DEBUG_EDGE_DETECT_SEARCH_ROI], RGB(0, 255, 0));
	AddStep(DEBUG_EDGE_DETECT_IGNORE_ROI, gl_szStrDebugName[DEBUG_EDGE_DETECT_IGNORE_ROI], RGB(255, 255, 0));
	AddStep(DEBUG_EDGE_DETECT_FOUND_EDGE, gl_szStrDebugName[DEBUG_EDGE_DETECT_FOUND_EDGE], RGB(0, 255, 0));
	AddStep(DEBUG_EDGE_DETECT_INTERSECTION_POINTS, gl_szStrDebugName[DEBUG_EDGE_DETECT_INTERSECTION_POINTS], RGB(0, 255, 0));

	AddStep(DEBUG_THRESHOLD_SEARCH_ROI, gl_szStrDebugName[DEBUG_THRESHOLD_SEARCH_ROI], RGB(255, 0, 0));
	AddStep(DEBUG_THRESHOLD_IGNORE_ROI, gl_szStrDebugName[DEBUG_THRESHOLD_IGNORE_ROI], RGB(255, 255, 0));

	AddStep(DEBUG_BLOB_FOUND_BLOB, gl_szStrDebugName[DEBUG_BLOB_FOUND_BLOB], RGB(0, 255, 0));

	AddStep(DEBUG_BLOB2_FOUND_BLOB, gl_szStrDebugName[DEBUG_BLOB2_FOUND_BLOB], RGB(0, 255, 0));

	AddStep(DEBUG_RESULT_RESULT, gl_szStrDebugName[DEBUG_RESULT_RESULT], RGB(255, 255, 255)); // 기존 빨강(255, 0, 0) ->  초록(0,255,0)으로 변경

	AddStep(DEBUG_CRITERIA_RESULT, gl_szStrDebugName[DEBUG_CRITERIA_RESULT], RGB(255, 255, 0));
	
	AddStep(DEBUG_RESULT_OK, _T("Result OK"), RGB(0,255,0));
	AddStep(DEBUG_RESULT_NG, _T("Result NG"), RGB(0,255,0)); //260309 sjpark 기존 빨간 메시지도 초록색으로 변경 요청

	AddStep(DEBUG_INVALID , _T("Invalid"));
	SetStepState(DEBUG_INVALID, TRUE, TRUE, RGB(255, 0, 0));

	ASSERT(GetStepN() == DEBUG_TOTAL_END);
}

COverlayManager::COverlayManager(const COverlayManager &Src)
{
	vecOverlay = Src.vecOverlay;
}

COverlayManager::~COverlayManager()
{
	vecOverlay.clear();
}

COverlayManager& COverlayManager::operator = (const COverlayManager &srcOjb)
{
	vecOverlay = srcOjb.vecOverlay;

	return *this;
}

void COverlayManager::AddRect(long nStep, long nLeft, long nTop, long nRight, long nBottom)
{
	vecOverlay[nStep].vecRect.push_back(CRect(nLeft, nTop, nRight, nBottom));
}

void COverlayManager::AddRect(long nStep, const CRect &rect)
{
	vecOverlay[nStep].vecRect.push_back(rect);
}

void COverlayManager::AddRect(long nStep, const CRect pRect[], long nNum)
{
	std::vector<CRect> &vecRect = vecOverlay[nStep].vecRect;

	vecRect.insert(vecRect.end(), pRect, pRect+nNum);
}

void COverlayManager::AddRect(long nStep, const std::vector<CRect> rects)
{
	std::vector<CRect> &vecRect = vecOverlay[nStep].vecRect;

	vecRect.insert(vecRect.end(), rects.begin(), rects.end());
}

void COverlayManager::AddCrPoint(long nStep, long x, long y)
{
	vecOverlay[nStep].vecCrPoint.push_back(CPoint(x, y));
}

void COverlayManager::AddCrPoint(long nStep, const CPoint &point)
{
	vecOverlay[nStep].vecCrPoint.push_back(point);
}

void COverlayManager::AddCrPoint(long nStep, const CPoint pPoint[], long nNum)
{
	std::vector<CPoint> &vecCrPoint = vecOverlay[nStep].vecCrPoint;

	vecCrPoint.insert(vecCrPoint.end(), pPoint, pPoint+nNum);
}

void COverlayManager::AddCrPoint(long nStep, const std::vector<CPoint> &points)
{
	std::vector<CPoint> &vecCrPoint = vecOverlay[nStep].vecCrPoint;

	vecCrPoint.insert(vecCrPoint.end(), points.begin(), points.end());
}

void COverlayManager::AddPoint(long nStep, long x, long y)
{
	AddPoint(nStep, CPoint(x, y));
}

void COverlayManager::AddPoint(long nStep, CPoint *pPoint, long nNum)
{
	for(long n=0;n<nNum;n++)
	{
		AddPoint(nStep,pPoint[n]);
	}
}

void COverlayManager::AddPoint(long nStep, const std::vector<CPoint> &points)
{
	std::vector<CPoint> &vecPoint = vecOverlay[nStep].vecPoint;

	vecPoint.insert(vecPoint.end(), points.begin(), points.end());
}

void COverlayManager::AddPoint(long nStep, const CPoint &point)
{
	vecOverlay[nStep].vecPoint.push_back(point);
}

void COverlayManager::AddLine(long nStep, long x1, long y1, long x2, long y2)
{
	vecOverlay[nStep].vecLine.push_back(SOverlay_Line(x1, y1, x2, y2));
}

void COverlayManager::AddLine(long nStep, CPoint pt1, CPoint pt2)
{
	vecOverlay[nStep].vecLine.push_back(SOverlay_Line(pt1, pt2));
}

void COverlayManager::AddString(long nStep, long x, long y, LPCTSTR szText, long nFontSize)
{
	vecOverlay[nStep].vecString.push_back(SOverlay_String(x,y, szText, nFontSize));
}

void COverlayManager::AddString(long nStep, CPoint pt, LPCTSTR szText, long nFontSize)
{
	vecOverlay[nStep].vecString.push_back(SOverlay_String(pt, szText, nFontSize));
}

void COverlayManager::AddEllipse(long nStep, long nLeft, long nTop, long nRight, long nBottom)
{
	vecOverlay[nStep].vecEllipse.push_back(CRect(nLeft, nTop, nRight, nBottom));
}

void COverlayManager::AddEllipse(long nStep, const CRect &rect)
{
	vecOverlay[nStep].vecEllipse.push_back(rect);
}

void COverlayManager::AddEllipse(long nStep, const CRect pRect[], long nNum)
{
	for(long n = 0; n < nNum; n++)
	{
		AddEllipse(nStep, pRect[n]);
	}
}

void COverlayManager::AddEllipse(long nStep, const std::vector<CRect> rects)
{
	std::vector<CRect> &vecEllipse = vecOverlay[nStep].vecEllipse;

	vecEllipse.insert(vecEllipse.end(), rects.begin(), rects.end());
}

void* COverlayManager::GetData(long nStep, long i_nRead, long &o_nType, BOOL bAllType)
{
	if(nStep >= (long)vecOverlay.size()) return NULL;

	if(bAllType)
	{
		if(i_nRead >= size(nStep)) return NULL;

		long nType = 0;

		while(1)
		{
			if(i_nRead >= size(nStep, nType))
			{
				i_nRead-= size(nStep, nType);
				nType++;
			}
			else
			{
				break;
			}
		}

		o_nType = nType;

		switch(nType)
		{
		case INTERMEDIATE_RECT:			return (void *)(&vecOverlay[nStep].vecRect[i_nRead]);	
		case INTERMEDIATE_CRPOINT:		return (void *)(&vecOverlay[nStep].vecCrPoint[i_nRead]);
		case INTERMEDIATE_POINT:		return (void *)(&vecOverlay[nStep].vecPoint[i_nRead]);	
		case INTERMEDIATE_LINE:		return (void *)(&vecOverlay[nStep].vecLine[i_nRead]);	
		case INTERMEDIATE_STRING:		return (void *)(&vecOverlay[nStep].vecString[i_nRead]);	
		case INTERMEDIATE_ELLIPSE:		return (void *)(&vecOverlay[nStep].vecEllipse[i_nRead]);
		}

		ASSERT(!"여기오는 경우도 있는감?");
	}
	else
	{
		if(i_nRead >= size(nStep, o_nType)) return NULL;

		switch(o_nType)
		{
		case INTERMEDIATE_RECT:			return (void *)(&vecOverlay[nStep].vecRect[i_nRead]);	
		case INTERMEDIATE_CRPOINT:		return (void *)(&vecOverlay[nStep].vecCrPoint[i_nRead]);
		case INTERMEDIATE_POINT:		return (void *)(&vecOverlay[nStep].vecPoint[i_nRead]);	
		case INTERMEDIATE_LINE:		return (void *)(&vecOverlay[nStep].vecLine[i_nRead]);	
		case INTERMEDIATE_STRING:		return (void *)(&vecOverlay[nStep].vecString[i_nRead]);	
		case INTERMEDIATE_ELLIPSE:		return (void *)(&vecOverlay[nStep].vecEllipse[i_nRead]);
		}
	}

	return NULL;
}

long COverlayManager::AddStep(long nStep, LPCTSTR szStepName, COLORREF color)
{
	long nTotalStepNum = (long)vecOverlay.size();

	if(nStep >= nTotalStepNum)
	{
		vecOverlay.resize(nStep+1);
	}

	vecOverlay[nStep].strStepName = szStepName;
	vecOverlay[nStep].bManualDraw = FALSE;
	vecOverlay[nStep].bInLineDraw = FALSE;
	vecOverlay[nStep].bManualUse = TRUE;
	vecOverlay[nStep].bInLineUse = TRUE;
	vecOverlay[nStep].crColor = color;

	return nStep;
}
long COverlayManager::GetStepN()
{
	return (long)vecOverlay.size();
}

CString COverlayManager::GetStepName(long nStep)
{
	return vecOverlay[nStep].strStepName;
}

void COverlayManager::ClearAll(BOOL bStepClear)
{
	if(bStepClear) vecOverlay.clear();
	else
	{
		for(long n = 0; n < (long)vecOverlay.size(); n++)
		{
			ClearData(n);
		}
	}
}

void COverlayManager::ClearData(long nStep)
{
	vecOverlay[nStep].vecRect.clear();
	vecOverlay[nStep].vecCrPoint.clear();
	vecOverlay[nStep].vecPoint.clear();
	vecOverlay[nStep].vecLine.clear();
	vecOverlay[nStep].vecString.clear();
	vecOverlay[nStep].vecEllipse.clear();
	vecOverlay[nStep].bManualUse = TRUE;
	vecOverlay[nStep].bInLineUse = TRUE;
}

long COverlayManager::size(long nStep)
{
	long return_size = 0;

	return_size += (long)vecOverlay[nStep].vecRect.size();
	return_size += (long)vecOverlay[nStep].vecCrPoint.size();
	return_size += (long)vecOverlay[nStep].vecPoint.size();
	return_size += (long)vecOverlay[nStep].vecLine.size();
	return_size += (long)vecOverlay[nStep].vecString.size();
	return_size += (long)vecOverlay[nStep].vecEllipse.size();

	return return_size;
}

long COverlayManager::size(long nStep, long nType)
{
	switch(nType)
	{
	case INTERMEDIATE_RECT:		return (long)vecOverlay[nStep].vecRect.size();		
	case INTERMEDIATE_CRPOINT:	return (long)vecOverlay[nStep].vecCrPoint.size();
	case INTERMEDIATE_POINT:	return (long)vecOverlay[nStep].vecPoint.size();
	case INTERMEDIATE_LINE:	return (long)vecOverlay[nStep].vecLine.size();
	case INTERMEDIATE_STRING:	return (long)vecOverlay[nStep].vecString.size();
	case INTERMEDIATE_ELLIPSE:	return (long)vecOverlay[nStep].vecEllipse.size();
	}

	return 0;
}

COLORREF COverlayManager::GetStepColor(long nStep)
{
	return vecOverlay[nStep].crColor;
}

void COverlayManager::GetStepState(long nStep, BOOL &bMD, BOOL &bID, COLORREF &crColor)
{
	bMD = vecOverlay[nStep].bManualDraw;
	bID = vecOverlay[nStep].bInLineDraw;
	crColor = vecOverlay[nStep].crColor;
}


void COverlayManager::SetStepState(long nStep, BOOL bMD, BOOL bID, COLORREF crColor)
{
	vecOverlay[nStep].bManualDraw = bMD;
	vecOverlay[nStep].bInLineDraw = bID;
	vecOverlay[nStep].crColor = crColor;
}

void COverlayManager::SetAllStepDrawDisable()
{
	for(long n=0;n < (long)vecOverlay.size();n++)
	{
		vecOverlay[n].bManualDraw = FALSE;
		vecOverlay[n].bInLineDraw = FALSE;
	}
}

void COverlayManager::SetStepDrawEnable(long nStep, BOOL bDraw)
{
	vecOverlay[nStep].bManualDraw = bDraw;
	vecOverlay[nStep].bInLineDraw = bDraw;
}

void COverlayManager::SetStepUse(long nStep, BOOL bMU, BOOL bIU)
{
	vecOverlay[nStep].bManualUse = bMU;
	vecOverlay[nStep].bInLineUse = bIU;
}

void COverlayManager::GetStepUse(long nStep, BOOL& o_bMU, BOOL& o_bIU)
{
	o_bMU = vecOverlay[nStep].bManualUse;
	o_bIU = vecOverlay[nStep].bInLineUse;
}

void COverlayManager::ShowDebugResult(CImageDisplay* pImageDisplay, long nStep)
{
	long nType;
	long nOverlay = 0;

	COLORREF color = GetStepColor(nStep);
	void *data = NULL;

	while(data = GetData(nStep, nOverlay, nType, TRUE))
	{
		switch(nType)
		{
		case INTERMEDIATE_RECT:			pImageDisplay->Overlay_AddRectangle(*(CRect *)data, color);
			break;
		case INTERMEDIATE_CRPOINT:		pImageDisplay->Overlay_AddCrPoint(*(CPoint *)data, color);
			break;
		case INTERMEDIATE_POINT:		pImageDisplay->Overlay_AddPoint(*(CPoint *)data, color);
			break;
		case INTERMEDIATE_LINE:
		{
			SOverlay_Line &sdata = *(SOverlay_Line *)data;
			pImageDisplay->Overlay_AddLine(sdata.ptStart, sdata.ptEnd, color);
		}
		break;
		case INTERMEDIATE_STRING:
		{
			SOverlay_String &sdata = *(SOverlay_String *)data;
			pImageDisplay->Overlay_AddText(sdata.ptXY, sdata.strText, color, 125, _T("Tahoma"));
		}
		break;
		case INTERMEDIATE_ELLIPSE:
			pImageDisplay->Overlay_AddEllipse2(*(CRect *)data, color);
			break;
		}

		nOverlay++;
	}
}
