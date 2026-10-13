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
			::g_DataSaveManager.m_nAlarm = ALARM_CRI_BLOB;
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
				::g_DataSaveManager.m_nAlarm = ALARM_CRI_LENGTH;
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
					::g_DataSaveManager.m_nAlarm = ALARM_CRI_ANGLE;
				}
			}
		}

		m_vecCriteriaResult.push_back(value);

		if(::g_DataSaveManager.m_nAlarm != ALARM_OK)
			OverlayAlarm(idx, ::g_DataSaveManager.m_nAlarm, result);
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
			::g_DataSaveManager.m_nAlarm = ALARM_CRI_BLOB2;
		}

		m_vecCriteriaResult.push_back(value);
		if (::g_DataSaveManager.m_nAlarm != ALARM_OK)
			OverlayAlarm(idx, ::g_DataSaveManager.m_nAlarm, result);
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

// 250627 sjpark  SetCriteriaBlob2 벤치마킹
BOOL CInspection::SetCriteriaYOLO(long itemIndex, BOOL &bResult)
{
	auto criteria = ::g_RecipeManager.m_vecsInspItem[itemIndex].sCriteria;
	auto camIndex = ::g_RecipeManager.m_vecsInspItem[itemIndex].visionIndex;

	for (long idx = 0; idx < (long)m_vecResult.size(); idx++)
	{
		SCriteriaParams value;
		value = criteria;

		auto result = m_vecResult[idx];
		auto boundingBoxRect = m_vecYOLOResultBox[idx];

		float yoloW = abs(boundingBoxRect.x2 - boundingBoxRect.x1);
		float yoloH = abs(boundingBoxRect.y2 - boundingBoxRect.y1);

		result.fWeldWidth = yoloW * ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
		result.fWeldHeight = yoloH * ::g_CalibrationManager.m_fPixelResolutionY[camIndex];

		// Blob 크기로 양불판정
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
			::g_Log.Log(_T("Inspe specErr : Blob2 "));
			::g_DataSaveManager.m_nAlarm = ALARM_CRI_BLOB2;
		}

		m_vecCriteriaResult.push_back(value);
		if (::g_DataSaveManager.m_nAlarm != ALARM_OK)
			OverlayAlarm(idx, ::g_DataSaveManager.m_nAlarm, result);
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

BOOL CInspection::SetCriteriaEdge(long itemIndex, BOOL &bResult)
{
	CString strTemp;
	auto criteria = ::g_RecipeManager.m_vecsInspItem[itemIndex].sCriteria;
	auto camIndex = ::g_RecipeManager.m_vecsInspItem[itemIndex].visionIndex;

	for (long idx = 0; idx < (long)m_vecResult.size(); idx++)
	{
		SCriteriaParams value;
		value = criteria;

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
				::g_DataSaveManager.m_nAlarm = ALARM_BLOB;
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
					::g_DataSaveManager.m_nAlarm = ALARM_CRI_HEIGHT;
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
					::g_DataSaveManager.m_nAlarm = ALARM_CRI_LENGTH;
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
						::g_DataSaveManager.m_nAlarm = ALARM_CRI_ANGLE;
					}
				}
			}

		if (::g_DataSaveManager.m_nAlarm != ALARM_OK)
			OverlayAlarm(idx, ::g_DataSaveManager.m_nAlarm, result);


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
