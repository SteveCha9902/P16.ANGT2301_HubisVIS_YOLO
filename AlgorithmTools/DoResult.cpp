#include "pch.h"
#include "Inspection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CInspection::DoResult(long itemIndex)
{
	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[itemIndex].algoSequence;

	m_vecResult.clear();

	switch(algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			DoCalcEdgeResult(itemIndex);
			SetResultEdge(itemIndex);
		}
		break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			DoCalcBlobResult(itemIndex);
			SetResultBlob(itemIndex);
		}
		break;
		}
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		switch(::g_ConfigManager.m_nCustomer)
		{
		case (long)CUSTOMER::BORGWARNER:
		{
			DoCalcBlob2Result(itemIndex);
			SetResultBlob2(itemIndex);
		}
		break;
		}
	}
	break;
	default:
		break;
	}

	return TRUE;
}

BOOL CInspection::DoCalcEdgeResult(long itemIndex)
{
	CString strTemp;

	auto &params = ::g_RecipeManager.m_vecsInspItem[itemIndex].sResult;
	auto &result = m_vecResult;
	auto camIndex = ::g_RecipeManager.m_vecsInspItem[itemIndex].visionIndex;

	auto resolutionMean = (::g_CalibrationManager.m_fPixelResolutionX[camIndex] + ::g_CalibrationManager.m_fPixelResolutionY[camIndex]) * 0.5f;
	auto blobCount = (long)m_vecBlobInfoList.size();

	m_vecptStart.clear();
	m_vecptEnd.clear();
	m_vecnTopBusbarHeight.clear();
	m_vecptStart.resize(blobCount);
	m_vecptEnd.resize(blobCount);
	m_vecnTopBusbarHeight.resize(blobCount);

	std::vector<float> vecfA(blobCount);
	std::vector<float> vecfB(blobCount);
	std::vector<float> vecfC(blobCount);
	std::vector<float> vecfBlobLeftA(blobCount);
	std::vector<float> vecfBlobLeftB(blobCount);
	std::vector<float> vecfBlobLeftC(blobCount);
	std::vector<float> vecfBlobRightA(blobCount);
	std::vector<float> vecfBlobRightB(blobCount);
	std::vector<float> vecfBlobRightC(blobCount);

	// Line Fitting...
	for(long idx = 0; idx < blobCount; idx++)
	{
		auto blobRect = m_vecBlobInfoList[idx].m_roi;
		auto edgeCount = (long)m_vecptEdge[idx].size();

		float *pfX = new float[edgeCount];
		float *pfY = new float[edgeCount];

		for(long pt = 0; pt < (long)m_vecptEdge[idx].size(); pt++)
		{
			pfX[pt] = (float)m_vecptEdge[idx][pt].x;
			pfY[pt] = (float)m_vecptEdge[idx][pt].y;
		}

		// Line Fitting
		if(HUBSDK::Geometry::LineFitting(edgeCount, pfX, pfY, vecfA[idx], vecfB[idx], vecfC[idx]) != retStatus::success)
		{
			DebugOverlay(itemIndex, DEBUG_RESULT_RESULT); // Debug Overlay...
			return FALSE;
		}
		
		pfX[0] = (float)blobRect.left;
		pfY[0] = (float)blobRect.top;
		pfX[1] = (float)blobRect.left;
		pfY[1] = (float)blobRect.bottom;

		// Line Fitting
		if(HUBSDK::Geometry::LineFitting(2, pfX, pfY, vecfBlobLeftA[idx], vecfBlobLeftB[idx], vecfBlobLeftC[idx]) != retStatus::success)
		{
			return FALSE;
		}

		pfX[0] = (float)blobRect.right;
		pfY[0] = (float)blobRect.top;
		pfX[1] = (float)blobRect.right;
		pfY[1] = (float)blobRect.bottom;

		// Line Fitting
		if(HUBSDK::Geometry::LineFitting(2, pfX, pfY, vecfBlobRightA[idx], vecfBlobRightB[idx], vecfBlobRightC[idx]) != retStatus::success)
		{
			return FALSE;
		}

		delete[] pfX;
		pfX = NULL;
		delete[] pfY;
		pfY = NULL;
	}

	// Calc. Intersection Point...
	for(long idx = 0; idx < blobCount; idx++)
	{
		HUBSDK::Geometry::GetCrossPointByTwoStraightLines(
			vecfBlobLeftA[idx], vecfBlobLeftB[idx], vecfBlobLeftC[idx], vecfA[idx], vecfB[idx], vecfC[idx],
			m_vecptStart[idx].x, m_vecptStart[idx].y);

		HUBSDK::Geometry::GetCrossPointByTwoStraightLines(
			vecfBlobRightA[idx], vecfBlobRightB[idx], vecfBlobRightC[idx], vecfA[idx], vecfB[idx], vecfC[idx],
			m_vecptEnd[idx].x, m_vecptEnd[idx].y);
	}

	DebugOverlay(itemIndex, DEBUG_EDGE_DETECT_INTERSECTION_POINTS); // Debug Overlay...

	// 찾은 포인트에서 Blob Bottom 까지의 길이를 계산한다.
	for(long idx = 0; idx < blobCount; idx++)
	{
		CPoint ptMean = (m_vecptStart[idx] + m_vecptEnd[idx]);
		ptMean.x /= 2;
		ptMean.y /= 2;

		auto blobRect = m_vecBlobInfoList[idx].m_roi;
		CPoint blobBtmCenter(blobRect.CenterPoint().x, blobRect.bottom);

		float dist(0.f);
		HUBSDK::Geometry::GetDistance_PointToPoint(ptMean, blobBtmCenter, dist);

		m_vecnTopBusbarHeight[idx] = dist * resolutionMean;
		//m_vecnTopBusbarHeight[idx] = static_cast<long>(std::round(dist * resolutionMean)); // PSJ 정수 -> 실수형 으로 변경
	}

	// Result Offset 적용...
	for(long idx = 0; idx < blobCount; idx++)
	{
		m_vecptStart[idx].x += (long)(params.fEdgeStartPtXOffset / ::g_CalibrationManager.m_fPixelResolutionX[camIndex]);
		m_vecptStart[idx].y += (long)(params.fEdgeStartPtYOffset / ::g_CalibrationManager.m_fPixelResolutionY[camIndex]);

		m_vecptEnd[idx].x += (long)(params.fEdgeEndPtXOffset / ::g_CalibrationManager.m_fPixelResolutionX[camIndex]);
		m_vecptEnd[idx].y += (long)(params.fEdgeEndPtYOffset / ::g_CalibrationManager.m_fPixelResolutionY[camIndex]);
	}

	for(long idx = 0; idx < blobCount; idx++)
	{
		SResultParams value;
		value.Init();

		value.ptStartResult = m_vecptStart[idx];
		value.ptEndResult = m_vecptEnd[idx];
		value.rtBlobROI = m_vecBlobInfoList[idx].m_roi;
		value.fEdgeTopBusbarHeight = float(m_vecnTopBusbarHeight[idx]);

		HUBSDK::Geometry::GetDistance_PointToPoint(value.ptStartResult, value.ptEndResult, value.fEdgeLength);
		value.fEdgeLength *= resolutionMean;

		HUBSDK::Geometry::Get1LineAngle(value.ptStartResult, value.ptEndResult, value.fEdgeAngle);
		if(value.fEdgeAngle < 0.f)
		{
			value.fEdgeAngle += 90.f;
		}
		else if(value.fEdgeAngle > 0.f)
		{
			value.fEdgeAngle -= 90.f;
		}
		value.fEdgeAngle = fabs(value.fEdgeAngle);

		result.push_back(value);
	}

	DebugOverlay(itemIndex, DEBUG_RESULT_RESULT); // Debug Overlay...

	return TRUE;
}

BOOL CInspection::DoCalcBlobResult(long itemIndex)
{
	CString strTemp;

	auto &params = ::g_RecipeManager.m_vecsInspItem[itemIndex].sResult;
	auto &result = m_vecResult;

	auto camIndex = params.visionIndex;
	auto resolutionMean = (::g_CalibrationManager.m_fPixelResolutionX[camIndex] + ::g_CalibrationManager.m_fPixelResolutionY[camIndex]) * 0.5f;

	auto blobCount = (long)m_vecBlobInfoList.size();

	for(long idx = 0; idx < blobCount; idx++)
	{
		SResultParams value;
		value.Init();

		auto info = m_vecBlobInfoList[idx];

		value.nBlobIndex = idx;
		value.rtROI = info.m_roi;
		value.fArea = (float)info.m_area;

		value.ptBlobStart.x = (long)((float)value.rtROI.left + params.fBlobStartPtXOffset / ::g_CalibrationManager.m_fPixelResolutionX[camIndex]);
		value.ptBlobStart.y = (long)((float)value.rtROI.top + params.fBlobStartPtYOffset / ::g_CalibrationManager.m_fPixelResolutionY[camIndex]);

		value.ptBlobEnd.x = (long)((float)value.rtROI.right - params.fBlobEndPtXOffset / ::g_CalibrationManager.m_fPixelResolutionX[camIndex]);
		value.ptBlobEnd.y = (long)((float)value.rtROI.top + params.fBlobEndPtYOffset / ::g_CalibrationManager.m_fPixelResolutionY[camIndex]);

		// 임시 Angle으로 0할당
		value.fBlobAngle = 0;

		m_sInspBlobResult.bResult = TRUE;

		HUBSDK::Geometry::GetDistance_PointToPoint(value.ptBlobStart, value.ptBlobEnd, value.fBlobLength);
		value.fBlobLength *= resolutionMean;

		value.fBlobTopBusbarHeight = params.fBlobStartPtYOffset; // PSJ

		HUBSDK::Geometry::Get1LineAngle(value.ptBlobStart, value.ptBlobEnd, value.fBlobAngle);
		if(value.fBlobAngle < 0.f)
		{
			value.fBlobAngle += 90.f;
		}
		else if(value.fBlobAngle > 0.f)
		{
			value.fBlobAngle -= 90.f;
		}
		value.fBlobAngle = fabs(value.fBlobAngle);

		m_vecResult.push_back(value);
	}

	DebugOverlay(itemIndex, DEBUG_RESULT_RESULT); // Debug Overlay...

	return TRUE;
}

BOOL CInspection::DoCalcBlob2Result(long itemIndex)
{
	CString strTemp;

	auto &result = m_vecResult;

	auto camIndex = ::g_RecipeManager.m_vecsInspItem[itemIndex].visionIndex;
	auto blobCount = (long)m_vecBlobInfoList2.size();

	for(long idx = 0; idx < blobCount; idx++)
	{
		SResultParams value;
		value.Init();

		auto info = m_vecBlobInfoList2[idx];

		value.nBlobIndex = idx;
		value.rtROI = info.m_roi;
		value.fArea = (float)info.m_area;
		value.fWeldWidth = info.m_roi.Width() * ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
		value.fWeldHeight = info.m_roi.Height() * ::g_CalibrationManager.m_fPixelResolutionY[camIndex];

		m_sInspBlobResult.bResult = TRUE;

		m_vecResult.push_back(value);
	}

	DebugOverlay(itemIndex, DEBUG_RESULT_RESULT); // Debug Overlay...

	return TRUE;
}

BOOL CInspection::SetResultBlob(long itemIndex)
{
	return TRUE;
}

BOOL CInspection::SetResultBlob2(long itemIndex)
{
	return TRUE;
}

BOOL CInspection::SetResultEdge(long itemIndex)
{
	return TRUE;
}


