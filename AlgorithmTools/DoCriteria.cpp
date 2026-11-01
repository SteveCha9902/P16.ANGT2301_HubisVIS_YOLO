#include "pch.h"
#include "Inspection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CInspection::DoCriteria(long itemIndex, BOOL &bResult)
{
	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[itemIndex].algoSequence;

	m_vecCriteriaResult.clear();
	m_vecYOLOMatchBox.clear(); // 이전 항목의 YOLO 표시가 남지 않도록 (SetCriteriaYOLO에서 다시 채움)

	switch(algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
	{
		SetCriteriaEdge(itemIndex, bResult);
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
	{
		SetCriteriaBlob(itemIndex, bResult);
	}
	break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:
	{
		// 250627 sjpark 후검사 결과 판정 YOLO / 룰베이스 선택
		if (g_ConfigManager.m_bUseAfterInspectionDL == TRUE)
		{
			SetCriteriaYOLO(itemIndex, bResult);
		}	
		else
		{
			SetCriteriaBlob2(itemIndex, bResult);
		}
	}
	break;
	default:
		break;
	}

	BOOL bsizeNG(FALSE);
	switch (g_RecipeManager.m_vecsInspItem[itemIndex].nInspType)	
	{
	case 1: //(long)InspType::INSP_AC :
		if (m_vecResult.size() < g_RecipeManager.m_sBusbarInfoParam.nACCount) 
		{
			bsizeNG = TRUE;
		}
		break;
	case 2://(long)InspType::INSP_DC:
		if (m_vecResult.size() < g_RecipeManager.m_sBusbarInfoParam.nDCCount)
		{
			bsizeNG = TRUE;
		}
	}

	BOOL bNG(FALSE);

	m_nOKCount = 0;
	m_nNGCount = 0;

	for(long idx = 0; idx < m_vecResult.size(); idx++)
	{
		if(m_vecCriteriaResult[idx].nErr == INSP_ERR_OK)
		{
			m_nOKCount++;
		}
		else
		{
			bNG = TRUE;
			m_nNGCount++;
		}
	}
	if (itemIndex == 0)
	{
		::g_Inspection.m_lTotalResult = 1;//LYS 241016 totalresult 초기화
		for (int i = 0; i < ::g_RecipeManager.m_nInspItemNum; i++) 
		{
			::g_InspectionManager.m_bResult[itemIndex] = TRUE;
		}

	}

	//if (bNG == TRUE || m_vecResult.size() <= 0)
	if(bNG == TRUE|| m_vecResult.size()<=0 || bsizeNG ==TRUE)
	{
		m_strTotalResult = _T("NG");
		::g_InspectionManager.m_bResult[itemIndex] = 0;
	}
	else
	{
		m_strTotalResult = _T("OK");
		::g_InspectionManager.m_bResult[itemIndex] = 1;
	}

	if (g_InspectionManager.m_bResult[itemIndex] == 0) 
	{
		::g_Inspection.m_lTotalResult= 2;//NG LYS241011
	}
	DebugOverlay(itemIndex, DEBUG_CRITERIA_RESULT);
	return TRUE;
}


BOOL CInspection::SetCriteriaBlob(long itemIndex, BOOL &bResult)
{
	auto criteria = ::g_RecipeManager.m_vecsInspItem[itemIndex].sCriteria;
	auto camIndex = ::g_RecipeManager.m_vecsInspItem[itemIndex].visionIndex;

	for(long idx = 0; idx < (long)m_vecResult.size(); idx++)
	{
		SCriteriaParams value;
		value = criteria;
		long nAlarm = ALARM_OK; // 측정점별 알람 (전역 공유 시 이전 NG 알람이 OK 항목에 남는 문제 수정)

		auto result = m_vecResult[idx];
		auto blobRect = m_vecBlobInfoList[idx].m_roi;

		// Blob 크기로 양불판정
		if((blobRect.Width() > criteria.nBlobWidthMin && blobRect.Width() < criteria.nBlobWidthMax) &&
			(blobRect.Height() > criteria.nBlobHeightMin && blobRect.Height() < criteria.nBlobHeightMax))
		{
			value.nErr = INSP_ERR_OK;
			m_sInspBlobResult.bResult = TRUE;
		}
		else
		{
			value.nErr = INSP_ERR;
			m_sInspBlobResult.bResult = FALSE;
			m_strNGtype = "Blob Size Error";
			::g_Log.Log(_T("Inspe specErr :Blob "));
			nAlarm = ALARM_CRI_BLOB;
		}

		// Blob 벽에서 얼마나 떨어져 있는지를 검사

		// 250401 PSJ Between 추가 주석처리 - 아직 덜 지웠어서 추가 주석
		/*if(value.nErr == INSP_ERR_OK)
		{
			auto distStartX = fabs(blobRect.left - result.ptBlobStart.x) * ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
			auto distStartY = fabs(blobRect.bottom - result.ptBlobStart.y) * ::g_CalibrationManager.m_fPixelResolutionY[camIndex];
			auto distEndX = fabs(blobRect.right - result.ptBlobEnd.x) * ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
			auto distEndY = fabs(blobRect.bottom - result.ptBlobEnd.y) * ::g_CalibrationManager.m_fPixelResolutionY[camIndex];

			if((distStartX > criteria.fBetweenDistMinX && distStartX < criteria.fBetweenDistMaxX) &&
				(distStartY > criteria.fBetweenDistMinY && distStartY < criteria.fBetweenDistMaxY) &&
				(distEndX > criteria.fBetweenDistMinX && distEndX < criteria.fBetweenDistMaxX) &&
				(distEndY > criteria.fBetweenDistMinY && distEndY < criteria.fBetweenDistMaxY))
			{
				value.nErr = INSP_ERR_OK;
				m_sInspBlobResult.bResult = TRUE;
			}
			else
			{
				value.nErr = INSP_ERR;
				m_sInspBlobResult.bResult = FALSE;
				m_strNGtype = "Between Distance Error";
				::g_Log.Log(_T("Inspe specErr : between "));
				::g_DataSaveManager.m_nAlarm = ALARM_CRI_BETWEEN;
			}
		}*/

		// Length 판단...
		if(value.nErr == INSP_ERR_OK)
		{
			if(result.fBlobLength > criteria.fLengthMin && result.fBlobLength < criteria.fLengthMax)
			{
				value.nErr = INSP_ERR_OK;
				m_sInspBlobResult.bResult = TRUE;
			}
			else
			{
				value.nErr = INSP_ERR;
				m_sInspBlobResult.bResult = FALSE;
				m_strNGtype = "Line Length Error";
				::g_Log.Log(_T("Inspe specErr : Length "));
				nAlarm = ALARM_CRI_LENGTH;
			}
		}

		// Anlge 판단...
		if(value.nErr == INSP_ERR_OK)
		{
			if(result.fBlobAngle == 0.f) // 0도이면 무조건 OK
			{
				value.nErr = INSP_ERR_OK;
				m_sInspBlobResult.bResult = TRUE;
			}
			else
			{
				if(result.fBlobAngle > criteria.fAngleMin && result.fBlobAngle < criteria.fAngleMax)
				{
					value.nErr = INSP_ERR_OK;
					m_sInspBlobResult.bResult = TRUE;
				}
				else
				{
					value.nErr = INSP_ERR;
					m_sInspBlobResult.bResult = FALSE;
					m_strNGtype = "Line Angle Error";
					::g_Log.Log(_T("Inspe specErr : Angle "));
					nAlarm = ALARM_CRI_ANGLE;
				}
			}
		}

		m_vecCriteriaResult.push_back(value);

		if(nAlarm != ALARM_OK)
			OverlayAlarm(idx, nAlarm, result);
	}

	if (m_vecResult.size() == 0)  // sjpark
	{
		SCriteriaParams value;
		value = criteria;

		value.nErr = INSP_ERR;
		m_sInspEdgeResult.bResult = FALSE;
		m_strNGtype = "Threshold Error";
		::g_Log.Log(_T("Inspe specErr :Threshold 재설정 필요"));

		m_strTotalResult = _T("NG");
		::g_InspectionManager.m_bResult[itemIndex] = 0;
	}

	return TRUE;
}

BOOL CInspection::SetCriteriaBlob2(long itemIndex, BOOL &bResult)
{
	auto criteria = ::g_RecipeManager.m_vecsInspItem[itemIndex].sCriteria;
	auto camIndex = ::g_RecipeManager.m_vecsInspItem[itemIndex].visionIndex;

	for(long idx = 0; idx < (long)m_vecResult.size(); idx++)
	{
		SCriteriaParams value;
		value = criteria;
		long nAlarm = ALARM_OK; // 측정점별 알람 (전역 공유 시 이전 NG 알람이 OK 항목에 남는 문제 수정)

		auto result = m_vecResult[idx];
		auto blobRect = m_vecBlobInfoList2[idx].m_roi;

		result.fWeldWidth = (float)blobRect.Width() * ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
		result.fWeldHeight = (float)blobRect.Height() * ::g_CalibrationManager.m_fPixelResolutionY[camIndex];

		// Blob 크기로 양불판정
		if((result.fWeldWidth > criteria.fWeldWidthMin && result.fWeldWidth < criteria.fWeldWidthMax) &&
			(result.fWeldHeight > criteria.fWeldHeightMin && result.fWeldHeight < criteria.fWeldHeightMax))
		{
			value.nErr = INSP_ERR_OK;
			m_sInspBlobResult.bResult = TRUE;
		}
		else
		{
			value.nErr = INSP_ERR;
			m_sInspBlobResult.bResult = FALSE;
			::g_Log.Log(_T("Inspe specErr : Blob2 "));
			nAlarm = ALARM_CRI_BLOB2;
		}

		m_vecCriteriaResult.push_back(value);
		if (nAlarm != ALARM_OK)
			OverlayAlarm(idx, nAlarm, result);
	}

	if (m_vecResult.size() == 0)  // sjpark
	{
		SCriteriaParams value;
		value = criteria;

		value.nErr = INSP_ERR;
		m_sInspEdgeResult.bResult = FALSE;
		m_strNGtype = "Threshold Error";
		::g_Log.Log(_T("Inspe specErr :Threshold 재설정 필요"));


		m_strTotalResult = _T("NG");
		::g_InspectionManager.m_bResult[itemIndex] = 0;
	}

	return TRUE;
}

namespace
{
	// 두 영역의 겹침 비율 = 교집합 면적 / 작은 쪽 면적 (0 ~ 1)
	float GetOverlapRatio(CRect a, CRect b)
	{
		a.NormalizeRect();
		b.NormalizeRect();

		CRect inter;
		if (inter.IntersectRect(&a, &b) == FALSE)
			return 0.f;

		double areaA = (double)a.Width() * a.Height();
		double areaB = (double)b.Width() * b.Height();
		double smaller = (areaA < areaB) ? areaA : areaB;
		if (smaller <= 0.)
			return 0.f;

		return (float)((double)inter.Width() * inter.Height() / smaller);
	}
}

// 250627 sjpark  SetCriteriaBlob2 벤치마킹
// P16: 검사 시점에 YOLO 서버를 호출하고, Blob2 측정점마다 위치가 겹치는 welding 박스를 찾아 그 크기로 판정한다.
//      서버가 타원을 보내면(segmentation 모델) 타원을 감싸는 회전 박스의 가로·세로로 판정한다.
//      (기존: 호출 없이 박스를 인덱스 순서로만 대응 → 박스 부족 시 범위 초과, 위치 불일치)
BOOL CInspection::SetCriteriaYOLO(long itemIndex, BOOL &bResult)
{
	auto criteria = ::g_RecipeManager.m_vecsInspItem[itemIndex].sCriteria;
	auto camIndex = ::g_RecipeManager.m_vecsInspItem[itemIndex].visionIndex;

	BOOL bYOLOOK = RequestYOLO(itemIndex);
	if (bYOLOOK == FALSE && ::g_ConfigManager.m_bYOLOFailFallback == TRUE)
	{
		::g_Log.Log(_T("[YOLO] Item %d : YOLO 실패 - 룰베이스(Blob2) 판정으로 대체"), itemIndex);
		return SetCriteriaBlob2(itemIndex, bResult);
	}

	std::vector<BOOL> vecBoxUsed(m_vecYOLOResultBox.size(), FALSE);
	m_vecYOLOMatchBox.assign(m_vecResult.size(), YOLOBox());

	for (long idx = 0; idx < (long)m_vecResult.size(); idx++)
	{
		SCriteriaParams value;
		value = criteria;
		long nAlarm = ALARM_OK; // 측정점별 알람 (전역 공유 시 이전 NG 알람이 OK 항목에 남는 문제 수정)

		auto &result = m_vecResult[idx];

		// 측정점 영역과 가장 많이 겹치는 미사용 welding 박스
		long nMatchBox = -1;
		float fBestRatio = 0.f;
		for (long box = 0; box < (long)m_vecYOLOResultBox.size(); box++)
		{
			if (vecBoxUsed[box] == TRUE)
				continue;

			float fRatio = GetOverlapRatio(result.rtROI, m_vecYOLOResultBox[box].GetRect());
			if (fRatio >= ::g_ConfigManager.m_fYOLOMatchRatio && fRatio > fBestRatio)
			{
				fBestRatio = fRatio;
				nMatchBox = box;
			}
		}

		if (nMatchBox < 0)
		{
			value.nErr = INSP_ERR;
			m_sInspBlobResult.bResult = FALSE;
			::g_Log.Log(_T("Inspe specErr : YOLO Item %d Point %d - %s"), itemIndex, idx, bYOLOOK ? _T("welding 박스 없음") : _T("YOLO 실패"));
			nAlarm = ALARM_CRI_BLOB2;
		}
		else
		{
			vecBoxUsed[nMatchBox] = TRUE;
			const auto &boundingBox = m_vecYOLOResultBox[nMatchBox];
			m_vecYOLOMatchBox[idx] = boundingBox;
			const double resX = ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
			const double resY = ::g_CalibrationManager.m_fPixelResolutionY[camIndex];

			// 결과 화면·CSV에도 판정에 쓴 YOLO 크기가 나가도록 측정 결과에 반영
			if (boundingBox.hasEllipse == TRUE)
			{
				// 타원을 감싸는 회전 박스의 변 길이. 기울어진 축은 X/Y 해상도를 축 방향 성분으로 합성해 mm로 바꾼다.
				// ew 축 방향 (cos, sin), eh 축 방향 (-sin, cos)
				const double kDegToRad = 3.14159265358979323846 / 180.0;
				const double c = cos(boundingBox.eangle * kDegToRad);
				const double s = sin(boundingBox.eangle * kDegToRad);
				result.fWeldWidth = (float)(boundingBox.ew * sqrt((resX * c) * (resX * c) + (resY * s) * (resY * s)));
				result.fWeldHeight = (float)(boundingBox.eh * sqrt((resX * s) * (resX * s) + (resY * c) * (resY * c)));
			}
			else
			{
				result.fWeldWidth = (float)(fabs(boundingBox.x2 - boundingBox.x1) * resX);
				result.fWeldHeight = (float)(fabs(boundingBox.y2 - boundingBox.y1) * resY);
			}

			// YOLO 박스 크기로 양불판정
			if ((result.fWeldWidth > criteria.fWeldWidthMin && result.fWeldWidth < criteria.fWeldWidthMax) &&
				(result.fWeldHeight > criteria.fWeldHeightMin && result.fWeldHeight < criteria.fWeldHeightMax))
			{
				value.nErr = INSP_ERR_OK;
				m_sInspBlobResult.bResult = TRUE;
			}
			else
			{
				value.nErr = INSP_ERR;
				m_sInspBlobResult.bResult = FALSE;
				::g_Log.Log(_T("Inspe specErr : YOLO Item %d Point %d W:%.3f H:%.3f %s angle:%.1f conf:%.2f"), itemIndex, idx, result.fWeldWidth, result.fWeldHeight,
					boundingBox.hasEllipse ? _T("ellipse") : _T("box"), boundingBox.hasEllipse ? boundingBox.eangle : 0.f, boundingBox.confidence);
				nAlarm = ALARM_CRI_BLOB2;
			}
		}

		// 옵션: 측정점 영역에 spatter가 있으면 NG
		if (value.nErr == INSP_ERR_OK && ::g_ConfigManager.m_bYOLOSpatterNG == TRUE)
		{
			for (const auto &spatter : m_vecYOLOSpatterBox)
			{
				if (GetOverlapRatio(result.rtROI, spatter.GetRect()) > 0.f)
				{
					value.nErr = INSP_ERR;
					m_sInspBlobResult.bResult = FALSE;
					::g_Log.Log(_T("Inspe specErr : YOLO Item %d Point %d spatter conf:%.2f"), itemIndex, idx, spatter.confidence);
					nAlarm = ALARM_CRI_BLOB2;
					break;
				}
			}
		}

		m_vecCriteriaResult.push_back(value);
		if (nAlarm != ALARM_OK)
			OverlayAlarm(idx, nAlarm, result);
	}

	// 결과 글자(Width/Height mm)는 DoResult에서 Blob2 크기로 먼저 그려졌으므로, 판정에 쓴 YOLO 크기로 다시 그린다.
	// (DEBUG_RESULT_RESULT는 지우고 m_vecResult로 다시 그린다. 같이 그려지는 알람 글자는 같은 위치에 겹쳐 그려진다)
	if (m_vecResult.size() != 0)
		DebugOverlay(itemIndex, DEBUG_RESULT_RESULT);

	if (m_vecResult.size() == 0)  // sjpark
	{
		SCriteriaParams value;
		value = criteria;

		value.nErr = INSP_ERR;
		m_sInspEdgeResult.bResult = FALSE;
		m_strNGtype = "Threshold Error";
		::g_Log.Log(_T("Inspe specErr :Threshold 재설정 필요"));


		m_strTotalResult = _T("NG");
		::g_InspectionManager.m_bResult[itemIndex] = 0;
	}

	return TRUE;
}

BOOL CInspection::SetCriteriaEdge(long itemIndex, BOOL &bResult)
{
	CString strTemp;
	auto criteria = ::g_RecipeManager.m_vecsInspItem[itemIndex].sCriteria;
	auto camIndex = ::g_RecipeManager.m_vecsInspItem[itemIndex].visionIndex;

	for (long idx = 0; idx < (long)m_vecResult.size(); idx++)
	{
		SCriteriaParams value;
		value = criteria;
		long nAlarm = ALARM_OK; // 측정점별 알람 (전역 공유 시 이전 NG 알람이 OK 항목에 남는 문제 수정)

			auto result = m_vecResult[idx];
			auto blobRect = m_vecBlobInfoList[idx].m_roi;

			 
			// Blob 크기로 양불판정
			if ((blobRect.Width() > criteria.nBlobWidthMin && blobRect.Width() < criteria.nBlobWidthMax) &&
				(blobRect.Height() > criteria.nBlobHeightMin && blobRect.Height() < criteria.nBlobHeightMax))
			{
				value.nErr = INSP_ERR_OK;
				m_sInspEdgeResult.bResult = TRUE;
			}
			else
			{
				value.nErr = INSP_ERR;
				m_sInspEdgeResult.bResult = FALSE;
				m_strNGtype = "Blob Size  Error";
				::g_Log.Log(_T("Inspe specErr :Blob "));
				nAlarm = ALARM_BLOB;
			}

			// Top Busbar Height 앙불판정
			if (value.nErr == INSP_ERR_OK)
			{
				if (result.fEdgeTopBusbarHeight > criteria.fTopBusbarHeightMin && result.fEdgeTopBusbarHeight < criteria.fTopBusbarHeightMax)
				{
					value.nErr = INSP_ERR_OK;
					m_sInspEdgeResult.bResult = TRUE;
				}
				else
				{
					value.nErr = INSP_ERR;
					m_sInspEdgeResult.bResult = FALSE;
					m_strNGtype = "Top Busbar Height Error";
					::g_Log.Log(_T("Inspe specErr : Height "));
					nAlarm = ALARM_CRI_HEIGHT;
				}
			}
			// 250102 Between 기능 임시 제거
			//// Blob 벽에서 얼마나 떨어져 있는지를 검사
			//if (value.nErr == INSP_ERR_OK)
			//{
			//	auto distStartX = abs(blobRect.left - result.ptStartResult.x) * ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
			//	auto distStartY = abs(blobRect.bottom - result.ptStartResult.y) * ::g_CalibrationManager.m_fPixelResolutionY[camIndex];
			//	auto distEndX = abs(blobRect.right - result.ptEndResult.x) * ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
			//	auto distEndY = abs(blobRect.bottom - result.ptEndResult.y) * ::g_CalibrationManager.m_fPixelResolutionY[camIndex];

			//	if ((distStartX > criteria.fBetweenDistMinX && distStartX < criteria.fBetweenDistMaxX) &&
			//		(distStartY > criteria.fBetweenDistMinY && distStartY < criteria.fBetweenDistMaxY) &&
			//		(distEndX > criteria.fBetweenDistMinX && distEndX < criteria.fBetweenDistMaxX) &&
			//		(distEndY > criteria.fBetweenDistMinY && distEndY < criteria.fBetweenDistMaxY))
			//	{
			//		value.nErr = INSP_ERR_OK;
			//		m_sInspEdgeResult.bResult = TRUE;
			//	}
			//	else
			//	{
			//		value.nErr = INSP_ERR;
			//		m_sInspEdgeResult.bResult = FALSE;
			//		m_strNGtype = "Between Distance Error";
			//		::g_Log.Log(_T("Inspe specErr : Between "));
			//		::g_DataSaveManager.m_nAlarm = ALARM_CRI_BETWEEN;
			//	}
			//}

			// Length 판단...
			if (value.nErr == INSP_ERR_OK)
			{
				if (result.fEdgeLength > criteria.fLengthMin && result.fEdgeLength < criteria.fLengthMax)
				{
					value.nErr = INSP_ERR_OK;
					m_sInspEdgeResult.bResult = TRUE;
				}
				else
				{
					value.nErr = INSP_ERR;
					m_sInspEdgeResult.bResult = FALSE;
					m_strNGtype = "Line Length Error";
					::g_Log.Log(_T("Inspe specErr : Length "));
					nAlarm = ALARM_CRI_LENGTH;
				}
			}

			// Anlge 판단...
			if (value.nErr == INSP_ERR_OK)
			{
				if (result.fEdgeAngle == 0.f) // 0도이면 무조건 OK
				{
					value.nErr = INSP_ERR_OK;
					m_sInspEdgeResult.bResult = TRUE;
				}
				else
				{
					if (result.fEdgeAngle > criteria.fAngleMin && result.fEdgeAngle < criteria.fAngleMax)
					{
						value.nErr = INSP_ERR_OK;
						m_sInspEdgeResult.bResult = TRUE;
					}
					else
					{
						value.nErr = INSP_ERR;
						m_sInspEdgeResult.bResult = FALSE;
						m_strNGtype = "Line Angle Error";
						::g_Log.Log(_T("Inspe specErr : Angle "));
						nAlarm = ALARM_CRI_ANGLE;
					}
				}
			}

		if (nAlarm != ALARM_OK)
			OverlayAlarm(idx, nAlarm, result);


		m_vecCriteriaResult.push_back(value);
		//if (m_sInspEdgeResult.bResult == FALSE) //LYS 에러타입 디스플레이
		//{
		//	auto pt = CPoint(0, result.rtBlobROI.bottom + 300);
		//	strTemp.Format(_T("%s"), m_strNGtype);

		//	m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, strTemp, 12/*125*/);
		//	pt = CPoint(result.rtBlobROI.left + 10, result.rtBlobROI.bottom + 300);
		//	strTemp.Format(_T("%s"), m_strNGtype);
		//	m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, strTemp, 12/*125*/);
		//}
			
	}
	
	if (m_vecResult.size() == 0)  // sjpark
	{
		SCriteriaParams value;
		value = criteria;

		value.nErr = INSP_ERR;
		m_sInspEdgeResult.bResult = FALSE;
		m_strNGtype = "Threshold Error";
		::g_Log.Log(_T("Inspe specErr :Threshold 재설정 필요"));


		m_strTotalResult = _T("NG");
		::g_InspectionManager.m_bResult[itemIndex] = 0;
	}
	

	return TRUE;
}
