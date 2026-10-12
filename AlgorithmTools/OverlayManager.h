#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API COverlayManager
{
public:
	COverlayManager();
	COverlayManager(const COverlayManager &Src);
	virtual ~COverlayManager();

	COverlayManager& operator = (const COverlayManager &Src);

public:
	void		GetStepUse(long nStep, BOOL& o_bMU, BOOL& o_bIU);
	void		SetStepUse(long nStep, BOOL bMU, BOOL bIU);

	COLORREF	GetStepColor(long nStep);
	void		GetStepState(long nStep, BOOL &bMD, BOOL &bID, COLORREF &crColor);
	void		SetStepState(long nStep, BOOL bMD, BOOL bID, COLORREF crColor);
	void		SetAllStepDrawDisable();
	void		SetStepDrawEnable(long nStep, BOOL bDraw);

	long		size(long nStep);
	long		size(long nStep, long nType);
	void		AddRect(long nStep, long nLeft, long nTop, long nRight, long nBottom);
	void		AddRect(long nStep, const CRect &rect);
	void		AddRect(long nStep, const CRect pRect[], long nNum);
	void		AddRect(long nStep, const std::vector<CRect> rects);
	void		AddCrPoint(long nStep, long x, long y);
	void		AddCrPoint(long nStep, const CPoint &point);
	void		AddCrPoint(long nStep, const CPoint pPoint[], long nNum);
	void		AddCrPoint(long nStep, const std::vector<CPoint> &points);
	void		AddPoint(long nStep, long x, long y);
	void		AddPoint(long nStep, const CPoint &point);
	void		AddPoint(long nStep, CPoint *pPoint, long nNum);
	void		AddPoint(long nStep, const std::vector<CPoint> &points);
	void		AddLine(long nStep, long x1, long y1, long x2, long y2);
	void		AddLine(long nStep, CPoint pt1, CPoint pt2);
	void		AddString(long nStep, long x, long y, LPCTSTR szText, long nFontSize);
	void		AddString(long nStep, CPoint pt, LPCTSTR szText, long nFontSize);
	void		AddEllipse(long nStep, long nLeft, long nTop, long nRight, long nBottom);
	void		AddEllipse(long nStep, const CRect &rect);
	void		AddEllipse(long nStep, const CRect pRect[], long nNum);
	void		AddEllipse(long nStep, const std::vector<CRect> rects);

	long			AddStep(long nStep, LPCTSTR szStepName, COLORREF color = RGB(0, 255, 0));
	long			GetStepN();
	CString		GetStepName(long nStep);
	void*			GetData(long nStep, long i_nRead, long &o_nType, BOOL bAllType);
	void		ClearAll(BOOL bStepClear);
	void		ClearData(long nStep);
	void		ShowDebugResult(CImageDisplay* pImageDisplay, long nStep);

	std::vector <SDebugResult>	vecOverlay;
};

extern ALGORITHM_TOOLS_API COverlayManager g_OverlayManager;