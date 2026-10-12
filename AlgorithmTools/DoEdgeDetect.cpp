#include "pch.h"
#include "Inspection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CInspection::DoEdgeDetect(long itemIndex)
{
	//auto InspItem = ::g_RecipeManager.m_vecsInspItem[itemIndex];
	//auto InspRoi = ::g_RecipeManager.m_vecsInspItem[itemIndex].sROI;

	BYTE* src = nullptr;
	BYTE* threshold = nullptr;
	BYTE* proc = nullptr;

	// Camera Index에 따른 Image
	switch(itemIndex)
	{
	case (long)IMAGE::ORIGINAL11://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_EDGE_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL12://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_EDGE_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL13://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_EDGE_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL21://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_EDGE_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL22://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_EDGE_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL23://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_EDGE_PROC);
	}
	break;
	}

	if (m_pAlgorithm->DoProcImage(threshold, m_nImageWidth, m_nImageHeight, ::g_RecipeManager.m_vecsInspItem[itemIndex].sProc[1], proc) == FALSE) { return FALSE; }
	//if(m_pAlgorithm->DoProcImage(src, m_nImageWidth, m_nImageHeight, ::g_RecipeManager.m_vecsInspItem[itemIndex].sProc[1], proc) == FALSE)	{		return FALSE;	}

	DebugOverlayClear(DEBUG_EDGE_DETECT_SEARCH_ROI);
	DebugOverlayClear(DEBUG_EDGE_DETECT_IGNORE_ROI);
	DebugOverlayClear(DEBUG_EDGE_DETECT_FOUND_EDGE);
	DebugOverlayClear(DEBUG_EDGE_DETECT_INTERSECTION_POINTS);

	EdgeDetect(src, proc, itemIndex);

	return TRUE;
}

BOOL CInspection::EdgeDetect(const BYTE* src, const BYTE* proc, const long itemIndex)
{
	auto align = ::g_RecipeManager.m_vecsInspItem[itemIndex].sAlign;
	auto direction = align.sEdgeDetectParam.nDirection;
	auto edgeDirection = align.sEdgeDetectParam.nEdgeDirection;
	auto searchDirection = align.sEdgeDetectParam.nSearchDirection; // PSJ   누락된 변수 추가
	auto firstEdge = align.sEdgeDetectParam.bFirstEdge;
	auto edgeThreshold = align.sEdgeDetectParam.fEdgeThreshold;
	auto useEdgeFiltering = align.sEdgeDetectParam.bUseEdgeFiltering;
	auto edgeFilteringRange = align.sEdgeDetectParam.nEdgeFilteringRange;
	auto ngEdgeErase = align.sEdgeDetectParam.bNGEdgeErase;

	auto blobCount = (long)m_vecBlobInfoList.size();

	// ROI...
	m_vecrtSearchROI.resize(blobCount);
	for(long idx = 0; idx < blobCount; idx++)
	{
		auto blobRect = m_vecBlobInfoList[idx].m_roi;
		if (!(itemIndex % 2)) { blobRect.bottom = LONG(blobRect.bottom - align.nDeflateROIHeight*2.2); }
		blobRect.DeflateRect(align.nDeflateROIWidth, align.nDeflateROIHeight);

		m_vecrtSearchROI[idx] = blobRect; // edge를 아우르는 ROI
	}

	DebugOverlay(itemIndex, DEBUG_EDGE_DETECT_SEARCH_ROI); // Debug Overlay...
	DebugOverlay(itemIndex, DEBUG_EDGE_DETECT_IGNORE_ROI);

	// Blob ROI 내 Edge Detect 실시...
	m_vecptEdge.clear();
	m_vecptEdge.resize(blobCount);

	auto oldEdgeThreshold = m_pAlgorithm->GetEdgeThreshold();
	m_pAlgorithm->SetEdgeThreshold(edgeThreshold);

	for(long idx = 0; idx < blobCount; idx++)
	{
		auto roi = m_vecrtSearchROI[idx];

		if(direction == (long)HUBSDK::DIRECTION::TOP || direction == (long)HUBSDK::DIRECTION::BOTTOM)
		{
			if (searchDirection == 1) // BOTTOM->TOP
			{
				for (long x = roi.left; x < roi.right; x++)
				{
					CPoint ptStart(x, roi.top);
					CPoint ptEnd(x, roi.bottom);

					m_pAlgorithm->CalcEdgeDetect(const_cast<BYTE *>(proc), m_nImageWidth, m_nImageHeight, ptEnd, ptStart, edgeDirection, firstEdge, m_vecptEdge[idx]);
				}
			}

			if (searchDirection == 3) // TOP->BOTTOM
			{
				for (long x = roi.left; x < roi.right; x++)
				{
					CPoint ptStart(x, roi.top);
					CPoint ptEnd(x, roi.bottom);

					m_pAlgorithm->CalcEdgeDetect(const_cast<BYTE *>(proc), m_nImageWidth, m_nImageHeight, ptStart, ptEnd, edgeDirection, firstEdge, m_vecptEdge[idx]);
				}
			}
		}
		else if(direction == (long)HUBSDK::DIRECTION::LEFT || direction == (long)HUBSDK::DIRECTION::RIGHT)
		{
			for(long y = roi.top; y < roi.bottom; y++)
			{
				CPoint ptStart(roi.left, y);
				CPoint ptEnd(roi.right, y);

				m_pAlgorithm->CalcEdgeDetect(const_cast<BYTE *>(proc), m_nImageWidth, m_nImageHeight, ptStart, ptEnd, edgeDirection, firstEdge, m_vecptEdge[idx]);
			}
		}
	}

	m_pAlgorithm->SetEdgeThreshold(oldEdgeThreshold);

	// Ignore ROI... (ROI에 포함된 Edge는 지운다.)
	for(long idx = 0; idx < (long)m_vecrtIgnoreROI.size(); idx++)
	{
		auto ignore = m_vecrtIgnoreROI[idx];

		for(long blobRoi = 0; blobRoi < (long)m_vecptEdge.size(); blobRoi++)
		{
			for(long edge = 0; edge < (long)m_vecptEdge[blobRoi].size(); edge++)
			{
				if(ignore.PtInRect(m_vecptEdge[blobRoi][edge]) == TRUE)
				{
					m_vecptEdge[blobRoi].erase(m_vecptEdge[blobRoi].begin() + edge);

					edge--;
				}
			}
		}
	}

	// Edge Filtering (Edge 평균+offset 벗어나는 Edge들을 처리)
	if(useEdgeFiltering == TRUE)
	{
		for(long idx = 0; idx < (long)m_vecptEdge.size(); idx++)
		{
			m_pAlgorithm->CalcEdgeFiltering(m_vecptEdge[idx], direction, edgeFilteringRange, ngEdgeErase);
		}
	}

	DebugOverlay(itemIndex, DEBUG_EDGE_DETECT_FOUND_EDGE);

	return TRUE;
}
