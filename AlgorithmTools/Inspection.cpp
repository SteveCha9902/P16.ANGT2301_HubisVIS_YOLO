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
namespace
{
	// P16: YOLO 타원과 이를 감싸는 회전 박스(판정 W/H)를 그린다. OverlayManager에 회전 도형이 없어 선분으로 근사한다.
	void AddYOLOEllipseOverlay(COverlayManager* pOverlay, long nStep, const YOLOBox& box)
	{
		if (pOverlay == nullptr || box.hasEllipse == FALSE)
			return;

		const double kPi = 3.14159265358979323846;
		const double c = cos(box.eangle * kPi / 180.0);
		const double s = sin(box.eangle * kPi / 180.0);
		const double hw = box.ew / 2.0, hh = box.eh / 2.0;
		// 타원 축 좌표 (a: ew 축, b: eh 축) → 영상 좌표
		auto toImage = [&](double a, double b)
		{
			return CPoint((long)floor(box.ecx + a * c - b * s + 0.5), (long)floor(box.ecy + a * s + b * c + 0.5));
		};

		const CPoint corners[4] = { toImage(-hw, -hh), toImage(hw, -hh), toImage(hw, hh), toImage(-hw, hh) };
		for (int i = 0; i < 4; i++)
			pOverlay->AddLine(nStep, corners[i], corners[(i + 1) % 4]);

		const int kSegments = 48;
		CPoint prev = toImage(hw, 0.0);
		for (int i = 1; i <= kSegments; i++)
		{
			const double t = 2.0 * kPi * i / kSegments;
			CPoint cur = toImage(hw * cos(t), hh * sin(t));
			pOverlay->AddLine(nStep, prev, cur);
			prev = cur;
		}
	}
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

					// YOLO 판정이면 판정에 쓴 타원·회전 박스도 함께 표시
					const YOLOBox* pYOLOBox = (idx < (long)m_vecYOLOMatchBox.size()) ? &m_vecYOLOMatchBox[idx] : nullptr;

					if (m_vecCriteriaResult[idx].nErr == INSP_ERR_OK)
					{
						m_pOverlayManager->AddRect(DEBUG_RESULT_OK, m_vecResult[idx].rtROI);
						if (pYOLOBox != nullptr)
							AddYOLOEllipseOverlay(m_pOverlayManager, DEBUG_RESULT_OK, *pYOLOBox);

						if (m_nAlarm != ALARM_OK)
							OverlayAlarm(idx, m_nAlarm, result);
					}
					else
					{
						::g_Log.Log(_T("[Test] DETECT_BLOB2 NG Overlay"));
						m_pOverlayManager->AddRect(DEBUG_RESULT_NG, m_vecResult[idx].rtROI);
						if (pYOLOBox != nullptr)
							AddYOLOEllipseOverlay(m_pOverlayManager, DEBUG_RESULT_NG, *pYOLOBox);

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

// P16 후검사 YOLO 서버 연동
// 프로토콜(기존 서버 유지): 영상 경로 + "\n" 전송 → JSON 한 줄 + "\n" 수신.
// 박스 좌표는 전체 영상 기준 [x1, y1, x2, y2].
// 결과 벡터는 호출 시작 시 비워서, 실패해도 이전 시료의 박스가 판정에 쓰이지 않게 한다.
BOOL CInspection::SocketYOLO_SingleCall(const CString& imagePath)
{
	m_vecYOLOResultBox.clear();
	m_vecYOLOSpatterBox.clear();

	const size_t kMaxResponseBytes = 1024 * 1024; // 비정상 응답으로 버퍼가 계속 커지지 않도록 제한
	const DWORD dwTimeoutMs = (DWORD)((::g_ConfigManager.m_nYOLOTimeoutMs > 100) ? ::g_ConfigManager.m_nYOLOTimeoutMs : 100);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		::g_Log.Log(_T("[YOLO] WSAStartup Fail"));
		return FALSE;
	}

	std::string jsonStr;
	BOOL bReceived = FALSE;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		::g_Log.Log(_T("[YOLO] socket Fail (%d)"), WSAGetLastError());
	}
	else
	{
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTimeoutMs, sizeof(dwTimeoutMs));
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwTimeoutMs, sizeof(dwTimeoutMs));

		sockaddr_in serverAddr = {};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons((u_short)::g_ConfigManager.m_nYOLOServerPort);
		CStringA strServerIP(::g_ConfigManager.m_strYOLOServerIP);

		// connect는 SO_SNDTIMEO가 적용되지 않으므로 non-blocking + select로 timeout을 건다
		BOOL bConnected = FALSE;
		if (inet_pton(AF_INET, strServerIP, &serverAddr.sin_addr) != 1)
		{
			::g_Log.Log(_T("[YOLO] Invalid Server IP : %s"), (LPCTSTR)::g_ConfigManager.m_strYOLOServerIP);
		}
		else
		{
			u_long nonBlocking = 1;
			ioctlsocket(sock, FIONBIO, &nonBlocking);

			if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == 0)
			{
				bConnected = TRUE;
			}
			else if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				fd_set writeSet, exceptSet;
				FD_ZERO(&writeSet);
				FD_ZERO(&exceptSet);
				FD_SET(sock, &writeSet);
				FD_SET(sock, &exceptSet);
				timeval tv = { (long)(dwTimeoutMs / 1000), (long)((dwTimeoutMs % 1000) * 1000) };
				if (select(0, nullptr, &writeSet, &exceptSet, &tv) > 0 && FD_ISSET(sock, &writeSet))
				{
					bConnected = TRUE;
				}
			}

			nonBlocking = 0;
			ioctlsocket(sock, FIONBIO, &nonBlocking);

			if (bConnected == FALSE)
			{
				::g_Log.Log(_T("[YOLO] Connect Fail %s:%d"), (LPCTSTR)::g_ConfigManager.m_strYOLOServerIP, ::g_ConfigManager.m_nYOLOServerPort);
			}
		}

		if (bConnected == TRUE)
		{
			// 영상 경로 전송 (기존 서버와 같은 ANSI 경로 + newline)
			std::string msgWithNewline = std::string(CT2A(imagePath)) + "\n";
			BOOL bSent = TRUE;
			size_t sentTotal = 0;
			while (sentTotal < msgWithNewline.size())
			{
				int sent = send(sock, msgWithNewline.c_str() + sentTotal, (int)(msgWithNewline.size() - sentTotal), 0);
				if (sent == SOCKET_ERROR || sent == 0)
				{
					::g_Log.Log(_T("[YOLO] Send Fail (%d)"), WSAGetLastError());
					bSent = FALSE;
					break;
				}
				sentTotal += (size_t)sent;
			}

			// 응답 수신 (newline까지)
			if (bSent == TRUE)
			{
				char buffer[1024];
				while (true)
				{
					int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
					if (bytesReceived <= 0)
					{
						if (bytesReceived < 0)
							::g_Log.Log(_T("[YOLO] Receive Fail/Timeout (%d)"), WSAGetLastError());
						break;
					}
					jsonStr.append(buffer, (size_t)bytesReceived);

					size_t newlinePos = jsonStr.find('\n');
					if (newlinePos != std::string::npos)
					{
						jsonStr.resize(newlinePos);
						bReceived = TRUE;
						break;
					}
					if (jsonStr.size() > kMaxResponseBytes)
					{
						::g_Log.Log(_T("[YOLO] Response too large (%u bytes)"), (unsigned int)jsonStr.size());
						jsonStr.clear();
						break;
					}
				}

				// newline 없이 서버가 연결을 닫은 경우도 받은 데이터는 사용한다 (기존 동작 유지)
				if (bReceived == FALSE && jsonStr.empty() == false)
					bReceived = TRUE;
			}
		}

		closesocket(sock);
	}

	WSACleanup();

	if (bReceived == FALSE || jsonStr.empty())
		return FALSE;

	if (jsonStr.find("\"error\"") != std::string::npos)
	{
		::g_Log.Log(_T("[YOLO] Server Error : %s"), (LPCTSTR)CString(jsonStr.substr(0, 200).c_str()));
		return FALSE;
	}

	ParseYOLOResult(CString(jsonStr.c_str()));

	return TRUE;
}

BOOL CInspection::RequestYOLO(long itemIndex)
{
	m_vecYOLOResultBox.clear();
	m_vecYOLOSpatterBox.clear();

	// DoDetectBlob2와 같은 원본 영상 (itemIndex == IMAGE::ORIGINALxx)
	if (itemIndex < (long)IMAGE::ORIGINAL11 || itemIndex > (long)IMAGE::ORIGINAL23)
	{
		::g_Log.Log(_T("[YOLO] Item %d : invalid image index"), itemIndex);
		return FALSE;
	}

	BYTE* src = ::g_MemoryManager.GetByteImage(itemIndex);
	if (src == nullptr || m_nImageWidth <= 0 || m_nImageHeight <= 0)
	{
		::g_Log.Log(_T("[YOLO] Item %d : empty image (%d x %d)"), itemIndex, m_nImageWidth, m_nImageHeight);
		return FALSE;
	}

	CString strWorkPath = ::g_ConfigManager.m_strYOLOWorkPath;
	strWorkPath.Trim();
	if (strWorkPath.IsEmpty())
	{
		TCHAR szTempPath[MAX_PATH] = { 0 };
		if (GetTempPath(MAX_PATH, szTempPath) == 0)
		{
			::g_Log.Log(_T("[YOLO] GetTempPath Fail"));
			return FALSE;
		}
		strWorkPath.Format(_T("%sHubisVIS_YOLO"), szTempPath);
	}
	strWorkPath.TrimRight(_T("\\"));
	::g_DataSaveManager.CreateDirectoryRecursive(strWorkPath);

	// 검사 스레드·항목별 고정 파일명: 동시 검사 시 서로 덮어쓰지 않고, 매 시료 덮어써서 쌓이지 않는다
	CString strImagePath;
	strImagePath.Format(_T("%s\\yolo_t%d_item%d.bmp"), strWorkPath, m_nThreadIndex, itemIndex);
	DeleteFile(strImagePath); // 이전 시료 파일이 남아 저장 실패를 놓치지 않도록 먼저 지운다

	DWORD dwBeginTick = GetTickCount();
	HUBSDK::ImageInterface::ImageSaveBMP(strImagePath, src, m_nImageWidth, m_nImageHeight, 8, CRect(0, 0, m_nImageWidth, m_nImageHeight), TRUE);
	if (PathFileExists(strImagePath) == FALSE)
	{
		::g_Log.Log(_T("[YOLO] Item %d : image save fail %s"), itemIndex, (LPCTSTR)strImagePath);
		return FALSE;
	}
	DWORD dwSaveElapsed = GetTickCount() - dwBeginTick;

	BOOL bReturn = SocketYOLO_SingleCall(strImagePath);
	DWORD dwTotalElapsed = GetTickCount() - dwBeginTick;

	::g_Log.Log(_T("[YOLO] Item %d : %s welding:%d spatter:%d save:%lu ms total:%lu ms"), itemIndex, bReturn ? _T("OK") : _T("FAIL"),
		(int)m_vecYOLOResultBox.size(), (int)m_vecYOLOSpatterBox.size(), (unsigned long)dwSaveElapsed, (unsigned long)dwTotalElapsed);

	return bReturn;
}

namespace
{
	// "class" 키부터 다음 "class" 키 전까지의 한 항목에서 class, confidence, 첫 [x1, y1, x2, y2]와 선택 항목 ellipse를 읽는다.
	BOOL ParseYOLOItem(const std::string& item, std::string& className, YOLOBox& box)
	{
		size_t pos = item.find(':');
		if (pos == std::string::npos) return FALSE;
		size_t start = item.find('"', pos);
		if (start == std::string::npos) return FALSE;
		size_t end = item.find('"', start + 1);
		if (end == std::string::npos) return FALSE;
		className = item.substr(start + 1, end - start - 1);

		pos = item.find("\"confidence\"");
		if (pos == std::string::npos) return FALSE;
		pos = item.find(':', pos);
		if (pos == std::string::npos) return FALSE;
		box.confidence = static_cast<float>(atof(item.c_str() + pos + 1));

		start = item.find('[');
		if (start == std::string::npos) return FALSE;
		end = item.find(']', start);
		if (end == std::string::npos) return FALSE;

		std::string coords = item.substr(start + 1, end - start - 1);
		if (sscanf_s(coords.c_str(), "%f , %f , %f , %f", &box.x1, &box.y1, &box.x2, &box.y2) != 4) return FALSE;

		// 선택 항목: box 뒤의 "ellipse": [cx, cy, w, h, angle]. 없거나 깨졌으면 박스만 쓴다.
		box.hasEllipse = FALSE;
		pos = item.find("\"ellipse\"", end);
		if (pos != std::string::npos)
		{
			start = item.find('[', pos);
			size_t ellipseEnd = (start == std::string::npos) ? std::string::npos : item.find(']', start);
			if (ellipseEnd != std::string::npos)
			{
				std::string values = item.substr(start + 1, ellipseEnd - start - 1);
				if (sscanf_s(values.c_str(), "%f , %f , %f , %f , %f", &box.ecx, &box.ecy, &box.ew, &box.eh, &box.eangle) == 5
					&& box.ew > 0.f && box.eh > 0.f)
					box.hasEllipse = TRUE;
			}
		}

		return TRUE;
	}
}

std::vector<YOLOBox> CInspection::ParseYOLOResult(const CString & jsonStr)
{
	CStringA ansiStr(jsonStr);
	std::string input = ansiStr;

	const std::string kClassKey = "\"class\"";
	const float fMinConfidence = ::g_ConfigManager.m_fYOLOConfidence;
	long nSkipped = 0;

	size_t pos = input.find(kClassKey);
	while (pos != std::string::npos)
	{
		size_t next = input.find(kClassKey, pos + kClassKey.size());
		std::string item = (next == std::string::npos) ? input.substr(pos) : input.substr(pos, next - pos);
		pos = next;

		YOLOBox box;
		std::string className;
		if (ParseYOLOItem(item.substr(kClassKey.size()), className, box) == FALSE)
		{
			nSkipped++;
			continue;
		}

		if (box.confidence < fMinConfidence)
			continue;

		box.class_name = CString(className.c_str());
		if (className == "welding")
			m_vecYOLOResultBox.push_back(box);
		else if (className == "spatter")
			m_vecYOLOSpatterBox.push_back(box);
	}

	if (nSkipped > 0)
		::g_Log.Log(_T("[YOLO] Parse : %d malformed item(s) skipped"), nSkipped);

	return m_vecYOLOResultBox;
}
