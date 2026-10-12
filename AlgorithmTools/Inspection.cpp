#include "pch.h"
#include "Inspection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CInspection g_Inspection;

CInspection::CInspection()
	: m_nImageWidth(0)
	, m_nImageHeight(0)
	, m_nImageSize(0)
	, m_nThreadIndex(0)
	, m_pAlgorithm(new CAlgorithm)
	, m_pOverlayManager(new COverlayManager)
	, m_nAlarm(ALARM_OK)
	, m_bReInspection(FALSE) // PSJ
{
	InitParameters();
}

CInspection::~CInspection()
{
	delete m_pAlgorithm;
	delete m_pOverlayManager;
}

void CInspection::InitParameters()
{
	m_nBlobTotalCount = 0;
	m_nBlobTotalCount2 = 0;
	m_nBlobIdx = -1;
	m_vecBlobInfoList.clear();
	m_vecBlobInfoList2.clear();
	m_vecResult.clear();

	m_strTotalResult = _T("");
	m_nOKCount = 0;
	m_nNGCount = 0;
	m_nAlarm = ALARM_OK;

	m_vecptEdge.clear();
	m_vecrtSearchROI.clear();
	m_vecptStart.clear();
	m_vecptEnd.clear();
	m_vecnTopBusbarHeight.clear();
}

void CInspection::SetImageInfo(const long imageWidth, const long imageHeight, const long threadIndex)
{
	// Image ?뺣낫 ?ㅼ젙 諛??뚮씪誘명꽣 珥덇린??- SHK 2023.12.27
	m_nImageWidth = imageWidth;
	m_nImageHeight = imageHeight;
	m_nImageSize = m_nImageWidth * m_nImageHeight;
	m_nThreadIndex = threadIndex;

	InitParameters();
}

BOOL CInspection::DoInspection(const long visionIndex, const long divIndex, const BOOL bRawImageSave, const BOOL bResultDataSave, const BOOL bResultImageSave)
{

	// 寃?????명듃 ?쒖옉 ???대뜑 珥덇린??
	if(bRawImageSave == TRUE || bResultDataSave == TRUE || bResultImageSave == TRUE)
	{
		if (visionIndex == 0 && divIndex == 0)
		{
			if (::g_DataSaveManager.m_strBarcode == _T(""))
			{
				::g_DataSaveManager.m_strBarcode = _T("BarcodeEmpty");
			}

			// 諛붿퐫?쒓? 諛붾뚯뿀?쇰㈃ ?대뜑 珥덇린??
			if (::g_DataSaveManager.m_strCurrentBarcode != ::g_DataSaveManager.m_strBarcode)
			{
				::g_DataSaveManager.m_strCurrentSaveFolder = _T("");  // ????대뜑 珥덇린??
			}
			
			::g_DataSaveManager.m_strCurrentBarcode = ::g_DataSaveManager.m_strBarcode;
		}

		// ?쒓컙 ?뺣낫 ?앹꽦...
		COleDateTime currentTime;
		currentTime = COleDateTime::GetCurrentTime();
		::g_DataSaveManager.m_strTime = currentTime.Format(_T("%Y%m%d_%H%M%S")); // ?꾩썡?? ?쒕텇珥?
		::g_DataSaveManager.m_nCamIdx = visionIndex;
		::g_DataSaveManager.m_nDivIdx = divIndex;
	}

auto matchItem = ::g_RecipeManager.m_vecInspItemMatch;

	long itemIndex(0); // 寃??Item Index
	std::vector<long> vecnSubItemIndex;
	for(long idx = 0; idx < (long)matchItem.size(); idx++)
	{
		if(matchItem[idx].visionIndex == visionIndex && matchItem[idx].divIndex == divIndex)
		{
			itemIndex = matchItem[idx].matchingIndex;
			vecnSubItemIndex = matchItem[idx].vecsSubItemIndex;
		}
	}

	//// 250712 濡쒖슦?곗씠???????대컢 蹂寃?sjpark
	if (bRawImageSave == TRUE && g_ConfigManager.m_bUseAfterInspectionDL == false) // 자동 시퀀스는 ScanManager -> DataSave queue 경로로만 RAW 저장
		::g_DataSaveManager.RawImageSave(TRUE);

	BOOL bResult = FALSE;
	if(EachInspection(itemIndex, bResult) == FALSE)
	{
		::g_Log.Log(_T("Inspection(%d) : Each Inspection Fail!!"), m_nThreadIndex);

		return FALSE;
	}

	if(DoSubItemInspection(vecnSubItemIndex) == FALSE)
	{
		::g_Log.Log(_T("Inspection(%d) : SubItem Inspection Fail!!"), m_nThreadIndex);

		return FALSE;
	}

	// Result Data Save...
	if(bResultDataSave == TRUE)
	{
		::g_DataSaveManager.ResultDataSave(bResultDataSave);
	}

	return TRUE;
}

BOOL CInspection::DoSubItemInspection(std::vector<long> vecnSubItemIndex)
{
	for(long idx = 0; idx < (long)vecnSubItemIndex.size(); idx++)
	{
		auto itemIndex = vecnSubItemIndex[idx];

		BOOL bResult = FALSE;
		if(EachInspection(itemIndex, bResult) == FALSE)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CInspection::EachInspection(long itemIndex, BOOL &bResult)
{
	::g_Log.Log(_T("==>> CInspection::EachInspection In......"));
	auto inspItem = ::g_RecipeManager.m_vecsInspItem[itemIndex];

	BOOL bReturn = FALSE;
	m_bReInspection = 0;

	m_nAlarm = ALARM_OK; // ?뚮엺 珥덇린??

	DebugOverlayClear(DEBUG_RESULT_OK);
	DebugOverlayClear(DEBUG_RESULT_NG);

	switch(inspItem.algoSequence)
	{
	case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
		bReturn = DoInspectionEdgeAlgo(itemIndex, bResult);
		
		if(::g_RecipeManager.m_vecsInspItem[itemIndex].sThreshold.bUseReInspection)
		{
			// DoCriteria Result == FALSE硫?Retry 濡쒖쭅 ?곸슜
			if (::g_InspectionManager.m_bResult[itemIndex] == FALSE)
			{
				m_bReInspection = 1;
				OverlayAlarmClear(DEBUG_RESULT_NG); // 湲곗〈 Overlay Reset 
				m_nAlarm = ALARM_OK; // Reset

				::g_Log.Log(_T("Re-Inspection. Item Index : %d"), itemIndex);
				// ?ш???濡쒖쭅 ?ㅼ뼱媛붿쓣 寃쎌슦 - 20 Threshold ?곸슜 PSJ
				if (DoInspectionEdgeAlgo(itemIndex, bResult) == FALSE)
				{
					if (::g_InspectionManager.m_bResult[itemIndex] == FALSE)
						::g_Log.Log(_T("Re-Inspection Fail. Item Index : %d"), itemIndex);
				}
			}

			// DoCriteria Result == FALSE硫?Retry 濡쒖쭅 ?곸슜
			if (::g_InspectionManager.m_bResult[itemIndex] == FALSE)
			{
				m_bReInspection = 2;
				OverlayAlarmClear(DEBUG_RESULT_NG); // 湲곗〈 Overlay Reset 
				m_nAlarm = ALARM_OK; // Reset

				::g_Log.Log(_T("Re-Inspection. Item Index : %d"), itemIndex);
				// ?ш???濡쒖쭅 ?ㅼ뼱媛붿쓣 寃쎌슦 - 20 Threshold ?곸슜 PSJ
				if (DoInspectionEdgeAlgo(itemIndex, bResult) == FALSE)
				{
					if (::g_InspectionManager.m_bResult[itemIndex] == FALSE)
						::g_Log.Log(_T("Re-Inspection Fail. Item Index : %d"), itemIndex);
				}
			}
		}
		//if (bReturn == FALSE) OverlayAlarm(itemIndex, ALARM_EDGE);
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
		bReturn = DoInspectionBlobAlgo(itemIndex, bResult);
		//if (bReturn == FALSE) OverlayAlarm(itemIndex, ALARM_BLOB);
		break;
	case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2:	
		bReturn = DoInspectionBlob2Algo(itemIndex, bResult);
		//if (bReturn == FALSE) OverlayAlarm(itemIndex, ALARM_BLOB2);
		break;
	default:
		break;
	}
	DebugOverlay(itemIndex, DEBUG_RESULT); // 250620 sjpark
	return bReturn;
}

BOOL CInspection::DoInspectionBlobAlgo(long itemIndex, BOOL &bResult)
{
	::g_Log.Log(_T("Inspection(%d) : Blob Algorithm Start"), m_nThreadIndex);

	if(DoSetROI(itemIndex) == FALSE)
	{
		::g_Log.Log(_T("Inspection(%d) : Blob Algorithm::DoSetROI Fail!!"), m_nThreadIndex);

		return FALSE;
	}

	if(DoThresholdImage(itemIndex) == FALSE)
	{
		m_nAlarm = ALARM_THRESHOLD;
		::g_Log.Log(_T("Inspection(%d) : Blob Algorithm::DoThresholdImage Fail!!"), m_nThreadIndex);

		return FALSE;
	}

	if(DoDetectBlob(itemIndex) == FALSE)
	{
		m_nAlarm = ALARM_BLOB;
		::g_Log.Log(_T("Inspection(%d) : Blob Algorithm::DoDetectBlob Fail!!"), m_nThreadIndex);

		if (DoResult(itemIndex) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Blob Algorithm::DoResult Fail!!"), m_nThreadIndex);

			return FALSE;
		}

		if (DoCriteria(itemIndex, bResult) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Blob Algorithm::DoCriteria Fail!!"), m_nThreadIndex);

			return FALSE;
		}
		return FALSE;
	}
	else
	{
		if (DoResult(itemIndex) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Blob Algorithm::DoResult Fail!!"), m_nThreadIndex);

			return FALSE;
		}

		if (DoCriteria(itemIndex, bResult) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Blob Algorithm::DoCriteria Fail!!"), m_nThreadIndex);

			return FALSE;
		}

		::g_Log.Log(_T("Inspection(%d) : Blob Algorithm Finish"), m_nThreadIndex);

		return TRUE;
	}
}

BOOL CInspection::DoInspectionBlob2Algo(long itemIndex, BOOL &bResult)
{
	::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm Start"), m_nThreadIndex);

	if(DoSetROI(itemIndex) == FALSE)
	{
		::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm::DoSetROI Fail!!"), m_nThreadIndex);

		return FALSE;
	}

	if(DoThresholdImage(itemIndex) == FALSE)//		if (DoThresholdImage2(itemIndex) == FALSE)
	{
		m_nAlarm = ALARM_THRESHOLD;
		::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm::DoThresholdImage Fail!!"), m_nThreadIndex);

		return FALSE;
	}

	if(DoDetectBlob(itemIndex) == FALSE)
	{
		m_nAlarm = ALARM_BLOB;
		::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm::DoDetectBlob Fail!!"), m_nThreadIndex);

		return FALSE;
	}

	if(DoDetectBlob2(itemIndex) == FALSE)
	{
		m_nAlarm = ALARM_BLOB2;
		::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm::DoDetectBlob2 Fail!!"), m_nThreadIndex);

		if (DoResult(itemIndex) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm::DoResult Fail!!"), m_nThreadIndex);

			return FALSE;
		}

		if (DoCriteria(itemIndex, bResult) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm::DoCriteria Fail!!"), m_nThreadIndex);

			return FALSE;
		}

		::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm Finish"), m_nThreadIndex);

		return FALSE;
	}
	else
	{
		if (DoResult(itemIndex) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm::DoResult Fail!!"), m_nThreadIndex);

			return FALSE;
		}

		if (DoCriteria(itemIndex, bResult) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm::DoCriteria Fail!!"), m_nThreadIndex);

			return FALSE;
		}

		::g_Log.Log(_T("Inspection(%d) : Blob2 Algorithm Finish"), m_nThreadIndex);

		return TRUE;
	}
}

BOOL CInspection::DoInspectionEdgeAlgo(long itemIndex, BOOL &bResult)
{
	::g_Log.Log(_T("Inspection(%d) : Edge Algorithm Start"), m_nThreadIndex);
	
	if(DoSetROI(itemIndex) == FALSE)
	{
		::g_Log.Log(_T("Inspection(%d) : Edge Algorithm::DoSetROI Fail!!"), m_nThreadIndex);
		return FALSE;
	}

	if(DoThresholdImage(itemIndex) == FALSE)
	{
		m_nAlarm = ALARM_THRESHOLD;
		::g_Log.Log(_T("Inspection(%d) : Edge Algorithm::DoThresholdImage Fail!!"), m_nThreadIndex);
		return FALSE;
	}

	if(DoDetectBlob(itemIndex) == FALSE)
	{
		m_nAlarm = ALARM_BLOB;
		::g_Log.Log(_T("Inspection(%d) : Edge Algorithm::DoDetectBlob Fail!!"), m_nThreadIndex);
		return FALSE;
	}

	if(DoEdgeDetect(itemIndex) == FALSE)
	{
		m_nAlarm = ALARM_EDGE;
		::g_Log.Log(_T("Inspection(%d) : Edge Algorithm::DoEdgeDetect Fail!!"), m_nThreadIndex);
		if (DoResult(itemIndex) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Edge Algorithm::DoResult Fail!!"), m_nThreadIndex);
			return FALSE;
		}

		if (DoCriteria(itemIndex, bResult) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Edge Algorithm::DoCriteria Fail!!"), m_nThreadIndex);
			return FALSE;
		}
		return FALSE;
	}
	else
	{
		if (DoResult(itemIndex) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Edge Algorithm::DoResult Fail!!"), m_nThreadIndex);
			return FALSE;
		}

		if (DoCriteria(itemIndex, bResult) == FALSE)
		{
			::g_Log.Log(_T("Inspection(%d) : Edge Algorithm::DoCriteria Fail!!"), m_nThreadIndex);
			return FALSE;
		}

		::g_Log.Log(_T("Inspection(%d) : Edge Algorithm Finish"), m_nThreadIndex);

		return TRUE;
	}
}

void CInspection::OverlayAlarm(int idx, int alarmCode, SResultParams result)
{
	CString str;
	int offsetX = 0;
	int offsetY = 0;
	//auto pt = CPoint(result.rtROI.left + offsetX, result.rtROI.bottom + offsetY); // result 醫뚰몴
	auto pt = CPoint(700 + offsetX, 1600 + offsetY); // result 醫뚰몴
	auto pt_criteria = CPoint(800 + offsetX, 1700 + offsetY); // result 醫뚰몴



	// sjpark 260309 Error ?띿뒪????젣 ?붿껌 / 而щ윭 蹂寃?DEBUG_RESULT_NG -> DEBUG_RESULT_OK
	switch (alarmCode)
	{
	case ALARM_OK:
		// pass

		break;
	case ALARM_UNKNOWN:
		str.Format(_T("Alarm : Unknown "));
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt, str, 12/*125*/);
		break;

	case ALARM_THRESHOLD:
		str.Format(_T("Alarm : Threshold "));
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt, str, 12/*125*/);
		break;

	case ALARM_BLOB:
		str.Format(_T("Alarm : Blob "));
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt, str, 12/*125*/);
		break;

	case ALARM_BLOB2:
		str.Format(_T("Alarm : Blob2 "));
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt, str, 12/*125*/);
		break;

	case ALARM_EDGE:
		str.Format(_T("Alarm : Edge Detection "));
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt, str, 12/*125*/);
		break;
	case ALARM_CRI_BLOB:
		str.Format(_T("Alarm : Criteria - Blob Size ")); 
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_BLOB2:
		str.Format(_T("Alarm : Criteria - Blob2 Size ")); // sjpark 260309 Error ?띿뒪????젣 ?붿껌
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_BETWEEN:
		str.Format(_T("Alarm : Criteria - Between ")); // sjpark 260309 Error ?띿뒪????젣 ?붿껌
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_LENGTH:
		str.Format(_T("Alarm : Criteria - Length ")); // sjpark 260309 Error ?띿뒪????젣 ?붿껌
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_HEIGHT:
		str.Format(_T("Alarm : Criteria - Height ")); // sjpark 260309 Error ?띿뒪????젣 ?붿껌
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_ANGLE:
		str.Format(_T("Alarm : Criteria - Angle ")); // sjpark 260309 Error ?띿뒪????젣 ?붿껌
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_INSP_FAIL:
		str.Format(_T("Alarm : Inspection ")); // sjpark 260309 Error ?띿뒪????젣 ?붿껌
		m_pOverlayManager->AddString(DEBUG_RESULT_OK, pt, str, 12/*125*/);
		break;
	case ALARM_END:
		break;

	default:
		break;
	}
	
}
void CInspection::OverlayAlarm(int idx, int alarmCode)
{
	CString str;
	int offsetX = 0;
	int offsetY = 0;
	auto pt = CPoint(800 + offsetX, 1600 + offsetY); // result 醫뚰몴
	auto pt_criteria = CPoint(800 + offsetX, 1600 + offsetY); // result 醫뚰몴

	switch (alarmCode)
	{
	case ALARM_OK:
		// pass

		break;
	case ALARM_UNKNOWN:
		str.Format(_T("Alarm : Unknown "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, str, 12/*125*/);
		break;

	case ALARM_THRESHOLD:
		str.Format(_T("Alarm : Threshold "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, str, 12/*125*/);
		break;

	case ALARM_BLOB:
		str.Format(_T("Alarm : Blob "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, str, 12/*125*/);
		break;

	case ALARM_BLOB2:
		str.Format(_T("Alarm : Blob2 "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, str, 12/*125*/);
		break;

	case ALARM_EDGE:
		str.Format(_T("Alarm : Edge Detection "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, str, 12/*125*/);
		break;
	case ALARM_CRI_BLOB:
		str.Format(_T("Alarm : Criteria - Blob Size Error "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_BLOB2:
		str.Format(_T("Alarm : Criteria - Blob2 Size Error "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_BETWEEN:
		str.Format(_T("Alarm : Criteria - Between Error "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_LENGTH:
		str.Format(_T("Alarm : Criteria - Length Error "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_HEIGHT:
		str.Format(_T("Alarm : Criteria - Height Error "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_CRI_ANGLE:
		str.Format(_T("Alarm : Criteria - Angle Error "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt_criteria, str, 12/*125*/);
		break;
	case ALARM_INSP_FAIL:
		str.Format(_T("Alarm : Inspection Fail "));
		m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, str, 12/*125*/);
		break;
	case ALARM_END:
		break;

	default:
		break;
	}

}
void CInspection::OverlayAlarmClear(int alarmCode)
{
	m_pOverlayManager->ClearData(alarmCode);
}
void CInspection::DebugOverlay(const long itemIndex, const long debugIndex)
{
	CString strTemp;
	//::g_Log.Log(_T("==>> CInspection::DebugOverlay In ......(itemIndex = %d,   debugIndex = %d"), itemIndex, debugIndex);
	auto roiParams = ::g_RecipeManager.m_vecsInspItem[itemIndex].sROI;
	auto thresholdParams = ::g_RecipeManager.m_vecsInspItem[itemIndex].sThreshold;
	auto blobParams = ::g_RecipeManager.m_vecsInspItem[itemIndex].sBlob;
	auto blob2Params = ::g_RecipeManager.m_vecsInspItem[itemIndex].sBlob2;
	auto alignParams = ::g_RecipeManager.m_vecsInspItem[itemIndex].sAlign;
	auto criteriaParams = ::g_RecipeManager.m_vecsInspItem[itemIndex].sCriteria;
	auto resultParams = ::g_RecipeManager.m_vecsInspItem[itemIndex].sResult;
	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[itemIndex].algoSequence;

	switch(debugIndex)
	{
	case DEBUG_ROI_SEARCH_ROI:
	{
		m_pOverlayManager->ClearData(DEBUG_ROI_SEARCH_ROI);
		if(roiParams.bUseROI)
		{
			for(long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				m_pOverlayManager->AddRect(DEBUG_ROI_SEARCH_ROI, roiParams.vecrtSearchROI[idx]);
			}
		}
	}
	break;
	case DEBUG_ROI_IGNORE_ROI:
	{
		m_pOverlayManager->ClearData(DEBUG_ROI_IGNORE_ROI);
		if(roiParams.bUseROI)
		{
			for(long idx = 0; idx < roiParams.nIgnoreROINum; idx++)
			{
				m_pOverlayManager->AddRect(DEBUG_ROI_IGNORE_ROI, roiParams.vecrtIgnoreROI[idx]);
			}
		}
	}
	break;
	case DEBUG_THRESHOLD_SEARCH_ROI:
	{
	}
	break;
	case DEBUG_THRESHOLD_IGNORE_ROI:
	{
	}
	break;
	case DEBUG_EDGE_DETECT_SEARCH_ROI:
	{
		m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_SEARCH_ROI);

		for(long idx = 0; idx < (long)m_vecrtSearchROI.size(); idx++)
		{
			m_pOverlayManager->AddRect(DEBUG_EDGE_DETECT_SEARCH_ROI, m_vecrtSearchROI[idx]);
		}
	}
	break;
	case DEBUG_EDGE_DETECT_IGNORE_ROI:
	{
		m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_IGNORE_ROI);

		for(long idx = 0; idx < alignParams.nIgnoreROINum; idx++)
		{
			m_pOverlayManager->AddRect(DEBUG_EDGE_DETECT_IGNORE_ROI, alignParams.vecrtIgnoreROI[idx]);
		}
	}
	break;
	case DEBUG_EDGE_DETECT_FOUND_EDGE:
	{
		m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_FOUND_EDGE);

		for(long idx = 0; idx < (long)m_vecptEdge.size(); idx++)
		{
			m_pOverlayManager->AddPoint(DEBUG_EDGE_DETECT_FOUND_EDGE, m_vecptEdge[idx]);
		}
	}
	break;
	case DEBUG_EDGE_DETECT_INTERSECTION_POINTS:
	{
		m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_INTERSECTION_POINTS);

		for(long idx = 0; idx < (long)m_vecptStart.size(); idx++)
		{
			if(m_vecptStart[idx] != CPoint(0, 0))
			{
				m_pOverlayManager->AddCrPoint(DEBUG_EDGE_DETECT_INTERSECTION_POINTS, m_vecptStart[idx]);
			}
		}

		for(long idx = 0; idx < (long)m_vecptEnd.size(); idx++)
		{
			if(m_vecptEnd[idx] != CPoint(0, 0))
			{
				m_pOverlayManager->AddCrPoint(DEBUG_EDGE_DETECT_INTERSECTION_POINTS, m_vecptEnd[idx]);
			}
		}
	}
	break;
	case DEBUG_BLOB_FOUND_BLOB:
	{
		m_pOverlayManager->ClearData(DEBUG_BLOB_FOUND_BLOB);
		for(long idx = 0; idx < m_nBlobTotalCount; idx++)
		{
			strTemp.Format(_T("%d"), idx + 1);
			auto rtBlob = m_vecBlobInfoList[idx].m_roi;

			m_pOverlayManager->AddRect(DEBUG_BLOB_FOUND_BLOB, rtBlob);
			m_pOverlayManager->AddString(DEBUG_BLOB_FOUND_BLOB, CPoint(rtBlob.right, rtBlob.bottom), strTemp, 12/*125*/);
			m_pOverlayManager->AddCrPoint(DEBUG_BLOB_FOUND_BLOB, rtBlob.CenterPoint());
		}
	}
	break;
	case DEBUG_BLOB2_FOUND_BLOB:
	{
		m_pOverlayManager->ClearData(DEBUG_BLOB2_FOUND_BLOB);
		for(long idx = 0; idx < m_nBlobTotalCount2; idx++)
		{
			strTemp.Format(_T("%d"), idx + 1);
			auto rtBlob = m_vecBlobInfoList2[idx].m_roi;

			m_pOverlayManager->AddRect(DEBUG_BLOB2_FOUND_BLOB, rtBlob);
			m_pOverlayManager->AddString(DEBUG_BLOB2_FOUND_BLOB, CPoint(rtBlob.right, rtBlob.bottom), strTemp, 12/*125*/);
			m_pOverlayManager->AddCrPoint(DEBUG_BLOB2_FOUND_BLOB, rtBlob.CenterPoint());
		}
	}
	break;
	case DEBUG_RESULT_RESULT:
	{
		m_pOverlayManager->ClearData(DEBUG_RESULT_RESULT);
		if (m_vecResult.size() != 0) // 寃곌낵 踰≫꽣媛 鍮꾩뼱?덉? ?딆쑝硫?
		{
			for (long idx = 0; idx < m_vecResult.size(); idx++)
			{
				auto result = m_vecResult[idx];

				if (algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB)
				{
					m_pOverlayManager->AddRect(DEBUG_RESULT_RESULT, result.rtROI);
					m_pOverlayManager->AddLine(DEBUG_RESULT_RESULT, result.ptBlobStart, result.ptBlobEnd);

					if (result.ptBlobStart != CPoint(0, 0) && result.ptBlobEnd != CPoint(0, 0))
					{
						m_pOverlayManager->AddLine(DEBUG_RESULT_RESULT, result.ptBlobStart, result.ptBlobEnd);

						// Top Busbar Height
						auto pt = CPoint(result.rtROI.left, result.rtROI.top - 350);
						strTemp.Format(_T("Height")); // Y Offset
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
						pt = CPoint(result.rtROI.left + 400, result.rtROI.top - 350);
						strTemp.Format(_T("%.3fmm"), result.fBlobTopBusbarHeight);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

						// Length
						pt = CPoint(result.rtROI.left, result.rtROI.top - 250);
						strTemp.Format(_T("Length"));
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
						pt = CPoint(result.rtROI.left + 400, result.rtROI.top - 250);
						strTemp.Format(_T("%.3fmm"), result.fBlobLength);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

						// Angle ?쒖떆
						//pt = CPoint(result.rtROI.right + 100, result.rtROI.top - 150);
						//strTemp.Format(_T("Angle"));
						//m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
						//pt = CPoint(result.rtROI.right + 500, result.rtROI.top - 150);
						//strTemp.Format(_T("%.3fmm"), result.fBlobAngle);
						//m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
						
					}
				if (m_nAlarm != ALARM_OK)
					OverlayAlarm(idx, m_nAlarm, result);
				}
				else if (algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB2)
				{
					m_pOverlayManager->AddRect(DEBUG_RESULT_RESULT, result.rtROI);

					// Weld Width
					auto pt = CPoint(0, 0); // 珥덇린??

					// 250402 PSJ ?띿뒪???ㅻ쾭?덉씠 泥섎━
					if (itemIndex % 2 == 0)  // DC Text Overlay
					{
						if (idx == 0) // 泥?踰덉㎏ 寃곌낵 諛뺤뒪??寃쎌슦
						{
							pt = CPoint(0, 1300); //result.rtROI.bottom + 100
							strTemp.Format(_T("Width"), result.fWeldWidth);
							m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
							pt = CPoint(0, 1400); // result.rtROI.bottom + 200
							strTemp.Format(_T("Height"), result.fWeldHeight);
							m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
						}
						pt = CPoint(result.rtROI.left, 1300); //result.rtROI.bottom+100
						strTemp.Format(_T("%.3fmm"), result.fWeldWidth);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

						pt = CPoint(result.rtROI.left, 1400); //result.rtROI.bottom + 200
						strTemp.Format(_T("%.3fmm"), result.fWeldHeight);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

					}
					else // AC Text Overlay
					{
						pt = CPoint(0, result.rtROI.top - 350);
						strTemp.Format(_T("Width"), result.fWeldWidth);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
						pt = CPoint(0, result.rtROI.top - 250);
						strTemp.Format(_T("Height"), result.fWeldHeight);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
						
						pt = CPoint(result.rtROI.left, result.rtROI.top - 350);
						strTemp.Format(_T("%.3fmm"), result.fWeldWidth);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

						pt = CPoint(result.rtROI.left, result.rtROI.top - 250);
						strTemp.Format(_T("%.3fmm"), result.fWeldHeight);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
					}
					//if (idx == 0) // TODO : ?ㅻ쾭?덉씠 ?꾩튂 ?섏젙 ?꾩슂
					//{
					//	auto pt = CPoint(0, result.rtROI.bottom);
					//	strTemp.Format(_T("Width"), result.fWeldWidth);
					//	m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
					//	pt = CPoint(0, result.rtROI.bottom + 100);
					//	strTemp.Format(_T("Height"), result.fWeldHeight);
					//	m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
					//}

					//pt = CPoint(result.rtROI.left, result.rtROI.bottom);
					//strTemp.Format(_T("%.3fmm"), result.fWeldWidth);
					//m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

					//pt = CPoint(result.rtROI.left, result.rtROI.bottom + 100);
					//strTemp.Format(_T("%.3fmm"), result.fWeldHeight);
					//m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

					if (m_nAlarm != ALARM_OK)
					{
						OverlayAlarm(idx, m_nAlarm, result); // 
					}
				}
				else if (algoSequence == (long)ALGORITHM_SEQUENCE::EDGE_DETECT)
				{
					//	m_pOverlayManager->AddCrPoint(DEBUG_RESULT_RESULT, result.ptStartResult);
					//	m_pOverlayManager->AddCrPoint(DEBUG_RESULT_RESULT, result.ptEndResult);

					if (result.ptStartResult != CPoint(0, 0) && result.ptEndResult != CPoint(0, 0)
						) // OK?쇰븣留?overlay ?섎룄濡?異붽? 議곌굔 ?ｊ린 sjpark
					{
						m_pOverlayManager->AddRect(DEBUG_RESULT_RESULT, result.rtROI);
						m_pOverlayManager->AddLine(DEBUG_RESULT_RESULT, result.ptStartResult, result.ptEndResult);

						
						// Top Busbar Height
						// ?꾩떆濡?二쇱꽍泥섎━
						auto pt = CPoint(result.rtBlobROI.left, result.rtBlobROI.bottom);

						if (idx == 0)
						{
							auto pt = CPoint(0, 1300);
							strTemp.Format(_T("Height"));
							m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
							pt = CPoint(0, 1400);
							strTemp.Format(_T("Length"));
							m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
							pt = CPoint(0, 1500);
							strTemp.Format(_T("Angle"));
							m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
						}

						pt = CPoint(result.rtBlobROI.left + 10, 1300); //result.rtBlobROI.bottom
						strTemp.Format(_T("%.3fmm"), result.fEdgeTopBusbarHeight);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

						// Length
						pt = CPoint(result.rtBlobROI.left + 10, 1400); //result.rtBlobROI.bottom + 100
						strTemp.Format(_T("%.3fmm"), result.fEdgeLength);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);

						// Angle ?쒖떆
						pt = CPoint(result.rtBlobROI.left + 10, 1500); //result.rtBlobROI.bottom + 200
						strTemp.Format(_T("%.3f"), result.fEdgeAngle);
						m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
					}
					if (m_nAlarm != ALARM_OK)
						OverlayAlarm(idx, m_nAlarm, result);
				}
			}
		}
		else
		{
			// Result Vector媛 鍮꾩뼱?덉쓣 寃쎌슦 寃쎈낫 ?쒖떆
			OverlayAlarm(0, ALARM_INSP_FAIL); // 寃???ㅽ뙣, 0踰?寃곌낵 諛뺤뒪??寃쎈낫 ?쒖떆

			m_strTotalResult = "NG";
			::g_InspectionManager.m_bResult[itemIndex] = 0;
		}
	}
	break;
	case DEBUG_CRITERIA_RESULT:
	{
		m_pOverlayManager->ClearData(DEBUG_CRITERIA_RESULT);
		
		// 1. ?쒖떆 李쎌뿉 寃곌낵 ?쒖떆 (OK/NG)
		// 2. ?대떦 ?쒖떆 李쎌뿉??NG??寃쎌슦 NG ????쒖떆 (criteria)

		//if (::g_InspectionManager.m_bResult[itemIndex] == 0)
		//{
		//	auto pt = CPoint(800, 1800);
		//	strTemp.Format(_T("NG : %s"), m_strNGtype);
		//	m_pOverlayManager->AddString(DEBUG_RESULT_RESULT, pt, strTemp, 12/*125*/);
		//}
	}
	break;
	case DEBUG_RESULT:
	{

		BOOL bResult = TRUE;
		if (m_vecResult.size() != 0) // 寃곌낵 踰≫꽣媛 鍮꾩뼱?덉? ?딆쑝硫?
		{
			if (algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB)
			{
				for (long idx = 0; idx < m_vecResult.size(); idx++)
				{
					auto blobRect = m_vecBlobInfoList[idx].m_roi;
					auto result = m_vecResult[idx];

					if (m_vecCriteriaResult[idx].nErr == INSP_ERR_OK)
					{
						m_pOverlayManager->AddRect(DEBUG_RESULT_OK, m_vecResult[idx].rtROI);
						m_pOverlayManager->AddLine(DEBUG_RESULT_OK, m_vecResult[idx].ptBlobStart, m_vecResult[idx].ptBlobEnd);
					}
					else
					{
						// sjpark 260309 ?뚮엺 硫붿떆吏 ??젣 ?붿껌 DEBUG_RESULT_NG -> DEBUG_RESULT_OK
						::g_Log.Log(_T("[Test] DETECT_BLOB NG Overlay"));
						m_pOverlayManager->AddRect(DEBUG_RESULT_OK, m_vecResult[idx].rtROI);
						m_pOverlayManager->AddLine(DEBUG_RESULT_OK, m_vecResult[idx].ptBlobStart, m_vecResult[idx].ptBlobEnd);

						// sjpark 260309 ?뚮엺 硫붿떆吏 ??젣 ?붿껌
						//auto pt = CPoint(result.rtBlobROI.left - 150, result.rtBlobROI.top - 150);
						//strTemp.Format(_T("Blob Insp NG"), result.fEdgeLength);
						//m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, strTemp, 18/*125*/);
					}
				}
			}
			else if (algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB2)
			{
				for (long idx = 0; idx < m_vecResult.size(); idx++)
				{

					auto result = m_vecResult[idx];

					if (m_vecCriteriaResult[idx].nErr == INSP_ERR_OK)
					{
						m_pOverlayManager->AddRect(DEBUG_RESULT_OK, m_vecResult[idx].rtROI);

						if (m_nAlarm != ALARM_OK)
							OverlayAlarm(idx, m_nAlarm, result);
					}
					else
					{
						::g_Log.Log(_T("[Test] DETECT_BLOB2 NG Overlay"));
						m_pOverlayManager->AddRect(DEBUG_RESULT_NG, m_vecResult[idx].rtROI);

						// sjpark 260309 ?뚮엺 硫붿떆吏 ??젣 ?붿껌
						//auto pt = CPoint(500, 500);
						//strTemp.Format(_T("Blob2 Insp NG"), result.fEdgeLength);
						//m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, strTemp, 18/*125*/);
					}
				}
			}
			else if (algoSequence == (long)ALGORITHM_SEQUENCE::EDGE_DETECT)
			{
				for (long idx = 0; idx < m_vecResult.size(); idx++)
				{
					auto blobRect = m_vecBlobInfoList[idx].m_roi;
					auto result = m_vecResult[idx];

					if (m_vecCriteriaResult[idx].nErr == INSP_ERR_OK)
					{
						m_pOverlayManager->AddRect(DEBUG_RESULT_OK, blobRect);
						m_pOverlayManager->AddLine(DEBUG_RESULT_OK, m_vecResult[idx].ptStartResult, m_vecResult[idx].ptEndResult);

					}
					else
					{
						::g_Log.Log(_T("[Test] Edge Detect NG Overlay"));
						// sjpark 260309 ?뚮엺 硫붿떆吏 ??젣 ?붿껌 DEBUG_RESULT_NG -> DEBUG_RESULT_OK
						m_pOverlayManager->AddRect(DEBUG_RESULT_OK, blobRect);
						m_pOverlayManager->AddLine(DEBUG_RESULT_OK, m_vecResult[idx].ptStartResult, m_vecResult[idx].ptEndResult);

						//auto pt = CPoint(result.rtBlobROI.left - 150, result.rtBlobROI.top - 150);
						//strTemp.Format(_T("Edge 寃??NG"), result.fEdgeLength);
						//m_pOverlayManager->AddString(DEBUG_RESULT_NG, pt, strTemp, 18/*125*/);

					}
				}
			}
		}
		else
		{
			// Result Vector媛 鍮꾩뼱?덉쓣 寃쎌슦 寃쎈낫 ?쒖떆
			OverlayAlarm(0, ALARM_INSP_FAIL); // 寃???ㅽ뙣, // sjpark
			
			m_strTotalResult = "NG";
			::g_InspectionManager.m_bResult[itemIndex] = 0;
		}
	}
	break;
	}
}

void CInspection::DebugOverlayClear(const long debugIndex)
{
	switch(debugIndex)
	{
	case DEBUG_ROI_SEARCH_ROI:
		m_pOverlayManager->ClearData(DEBUG_ROI_SEARCH_ROI);
		break;
	case DEBUG_ROI_IGNORE_ROI:
		m_pOverlayManager->ClearData(DEBUG_ROI_IGNORE_ROI);
		break;
	case DEBUG_THRESHOLD_SEARCH_ROI:
		m_pOverlayManager->ClearData(DEBUG_THRESHOLD_SEARCH_ROI);
		break;
	case DEBUG_THRESHOLD_IGNORE_ROI:
		m_pOverlayManager->ClearData(DEBUG_THRESHOLD_IGNORE_ROI);
		break;
	case DEBUG_EDGE_DETECT_SEARCH_ROI:
		m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_SEARCH_ROI);
		break;
	case DEBUG_EDGE_DETECT_IGNORE_ROI:
		m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_IGNORE_ROI);
		break;
	case DEBUG_EDGE_DETECT_FOUND_EDGE:
		m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_FOUND_EDGE);
		break;
	case DEBUG_EDGE_DETECT_INTERSECTION_POINTS:
		m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_INTERSECTION_POINTS);
		break;
	case DEBUG_BLOB_FOUND_BLOB:
		m_pOverlayManager->ClearData(DEBUG_BLOB_FOUND_BLOB);
		break;
	case DEBUG_BLOB2_FOUND_BLOB:
		m_pOverlayManager->ClearData(DEBUG_BLOB2_FOUND_BLOB);
		break;
	case DEBUG_RESULT_RESULT:
		m_pOverlayManager->ClearData(DEBUG_RESULT_RESULT);
		break;
	case DEBUG_CRITERIA_RESULT:
		m_pOverlayManager->ClearData(DEBUG_CRITERIA_RESULT);
		break;
	case DEBUG_RESULT:
		m_pOverlayManager->ClearData(DEBUG_RESULT_OK);
		m_pOverlayManager->ClearData(DEBUG_RESULT_NG);
		break;
	}
}

void CInspection::DebugOverlayAllClear()
{
	m_pOverlayManager->ClearData(DEBUG_ROI_SEARCH_ROI);
	m_pOverlayManager->ClearData(DEBUG_ROI_IGNORE_ROI);
	m_pOverlayManager->ClearData(DEBUG_THRESHOLD_SEARCH_ROI);
	m_pOverlayManager->ClearData(DEBUG_THRESHOLD_IGNORE_ROI);
	m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_SEARCH_ROI);
	m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_IGNORE_ROI);
	m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_FOUND_EDGE);
	m_pOverlayManager->ClearData(DEBUG_EDGE_DETECT_INTERSECTION_POINTS);
	m_pOverlayManager->ClearData(DEBUG_BLOB_FOUND_BLOB);
	m_pOverlayManager->ClearData(DEBUG_BLOB2_FOUND_BLOB);
	m_pOverlayManager->ClearData(DEBUG_RESULT_RESULT);
	m_pOverlayManager->ClearData(DEBUG_CRITERIA_RESULT);
	m_pOverlayManager->ClearData(DEBUG_RESULT_OK);
	m_pOverlayManager->ClearData(DEBUG_RESULT_NG);
}


#include <Ws2tcpip.h>
bool CInspection::InitSocketClient()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;

	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sock == INVALID_SOCKET)
		return false;
	DWORD dwSocketTimeoutMs = 3000;
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwSocketTimeoutMs, sizeof(dwSocketTimeoutMs));
	setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwSocketTimeoutMs, sizeof(dwSocketTimeoutMs));

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

	if (connect(m_sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return false;

	m_connected = true;
	return true;
}
void CInspection::CleanupSocket()
{
	if (m_sock != INVALID_SOCKET)
	{
		closesocket(m_sock);
		WSACleanup();
		m_sock = INVALID_SOCKET;
		m_connected = false;
	}
}

BOOL CInspection::SocketYOLO_SingleCall(const CString& imagePath)
{
	// 1. ?뚯폆 ?앹꽦
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) return FALSE;
	DWORD dwSocketTimeoutMs = 3000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwSocketTimeoutMs, sizeof(dwSocketTimeoutMs));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwSocketTimeoutMs, sizeof(dwSocketTimeoutMs));

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

	if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(sock);
		return FALSE;
	}

	// 2. ?대?吏 寃쎈줈 ?꾩넚 (newline ?ы븿)
	std::string imgPathStr = CT2A(imagePath);
	std::string msgWithNewline = imgPathStr + "\n";
	int sent = send(sock, msgWithNewline.c_str(), (int)msgWithNewline.size(), 0);
	// shutdown(sock, SD_SEND); // ?꾩넚 ?꾨즺 ???뚯폆 ?リ린! (?곗씠???꾩넚 ?꾨즺)
	if (sent == SOCKET_ERROR) {
		closesocket(sock);
		return FALSE;
	}

	// 3. ?묐떟 ?곗씠???섏떊 (newline ?ы븿)
	std::string jsonStr;
	char buffer[1024];
	int bytesReceived = 0;
	while (true)
	{
		bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (bytesReceived <= 0) break;
		buffer[bytesReceived] = '\0';
		jsonStr += buffer;
		if (!jsonStr.empty() && jsonStr.back() == '\n') break;
	}
	closesocket(sock); // ?뚯폆 ?リ린

	if (jsonStr.empty()) return FALSE;

	// 4. ?곗씠???뚯떛 ??鍮?踰≫꽣 珥덇린??
	m_vecYOLOResultBox.clear();

	// 5. '\n' ?쒓굅 (newline 臾몄옄媛 ?ы븿???곗씠?곗쓽 寃쎌슦 泥섎━)
	size_t newlinePos = jsonStr.find('\n');
	if (newlinePos != std::string::npos)
		jsonStr = jsonStr.substr(0, newlinePos);

	// 6. CString 蹂??諛??뚯떛
	CStringA ansiStr(jsonStr.c_str());
	CString resultYOLO(ansiStr);
	ParseYOLOResult(resultYOLO);

	return TRUE;
}


std::vector<YOLOBox> CInspection::ParseYOLOResult(const CString & jsonStr)
{
	CStringA ansiStr(jsonStr);  // CString??ANSI 臾몄옄?대줈 蹂??
	std::string input = ansiStr;

	size_t pos = 0;
	while ((pos = input.find("\"class\"")) != std::string::npos)
	{
		YOLOBox box;

		// class
		pos = input.find("\"class\"");
		size_t start = input.find("\"", pos + 7) + 1;
		size_t end = input.find("\"", start);
		box.class_name = CString(input.substr(start, end - start).c_str());

		if (box.class_name == "welding")
		{
			// confidence
			pos = input.find("\"confidence\"", end);
			start = input.find(":", pos) + 1;
			end = input.find(",", start);
			box.confidence = static_cast<float>(atof(input.substr(start, end - start).c_str()));

			// box [x1, y1, x2, y2]
			pos = input.find("[", end) + 1;
			end = input.find(",", pos);
			box.x1 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

			pos = end + 1;
			end = input.find(",", pos);
			box.y1 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

			pos = end + 1;
			end = input.find(",", pos);
			box.x2 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

			pos = end + 1;
			end = input.find("]", pos);
			box.y2 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

			// 250627 sjpark ?좊ː??0.85 ?댁긽??寃쎌슦 寃곌낵???ы븿
			if (box.confidence >= 0.85)
			{
				m_vecYOLOResultBox.push_back(box);
			}
		}
		if (box.class_name == "spatter")
		{
			// confidence
			pos = input.find("\"confidence\"", end);
			start = input.find(":", pos) + 1;
			end = input.find(",", start);
			box.confidence = static_cast<float>(atof(input.substr(start, end - start).c_str()));

			// box [x1, y1, x2, y2]
			pos = input.find("[", end) + 1;
			end = input.find(",", pos);
			box.x1 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

			pos = end + 1;
			end = input.find(",", pos);
			box.y1 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

			pos = end + 1;
			end = input.find(",", pos);
			box.x2 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

			pos = end + 1;
			end = input.find("]", pos);
			box.y2 = static_cast<float>(atof(input.substr(pos, end - pos).c_str()));

			// 250627 sjpark ?좊ː??0.85 ?댁긽??寃쎌슦 寃곌낵???ы븿
			if (box.confidence >= 0.85)
			{
				// TODO : YOLOSpatterBox 寃곌낵 諛뺤뒪 珥덇린??諛?寃곌낵 踰≫꽣??異붽?
				m_vecYOLOSpatterBox.push_back(box);
			}
		}
		

		// ?곗씠???앷퉴吏 諛섎났
		input = input.substr(end);
	}

	//for (const auto& boxInstance : m_vecYOLOResultBox)
	//{
	//	CString debug;
	//	debug.Format(_T("Class: %s, Conf: %.2f, Box: %.1f, %.1f, %.1f, %.1f"),
	//		boxInstance.class_name, boxInstance.confidence, boxInstance.x1, boxInstance.y1, boxInstance.x2, boxInstance.y2);
	//	AfxMessageBox(debug);
	//}

	return m_vecYOLOResultBox;
}
