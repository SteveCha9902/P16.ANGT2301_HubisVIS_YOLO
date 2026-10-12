#include "pch.h"
#include "RecipeManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRecipeManager g_RecipeManager;

CRecipeManager::CRecipeManager()
{
	InitializeCriticalSection(&m_csRecipe);
}

CRecipeManager::~CRecipeManager()
{
	DeleteCriticalSection(&m_csRecipe);
}

void CRecipeManager::CalcMatchSize()
{
	//Config에서 설정한 Camera 수와 Division 수에 따라서 Item Match 수정
	long camCount(0);
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		camCount = 1;
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		camCount = 2;
		break;
	}

	long newSize = camCount * ::g_ConfigManager.m_nDivisionIndex;
	long oldSize = m_nInspItemMatchNum;

	long count(0);
	SMatchItem item;
	item.Init();

	// 기존 Match Num이 0일 경우
	if(oldSize == 0)
	{
		for(long matchCam = 0; matchCam < camCount; matchCam++)
		{
			for(long matchDiv = 0; matchDiv < ::g_ConfigManager.m_nDivisionIndex; matchDiv++)
			{
				item.Init();
				item.visionIndex = matchCam;
				item.divIndex = matchDiv;
				m_vecInspItemMatch.push_back(item);
			}
		}
	}
	// Match Num이 늘어났을 경우 
	else if(newSize > oldSize)
	{
		for(long match = 0; match < newSize; match++)
		{
			if(match == m_vecInspItemMatch.size())
			{
				item.Init();

				if(m_vecInspItemMatch[match - 1].visionIndex + 1 < camCount)
				{
					if(m_vecInspItemMatch[match - 1].divIndex + 1 >= ::g_ConfigManager.m_nDivisionIndex)
					{
						item.visionIndex = m_vecInspItemMatch[match - 1].visionIndex + 1;
						item.divIndex = 0;
					}
					else
					{
						item.visionIndex = m_vecInspItemMatch[match - 1].visionIndex;
						item.divIndex = m_vecInspItemMatch[match - 1].divIndex + 1;
					}
				}
				else
				{
					item.visionIndex = m_vecInspItemMatch[match - 1].visionIndex;
					item.divIndex = m_vecInspItemMatch[match - 1].divIndex + 1;
				}

				m_vecInspItemMatch.push_back(item);
			}
			else
			{
				if(m_vecInspItemMatch[match].visionIndex < camCount)
				{
					if(m_vecInspItemMatch[match].divIndex == count)
					{
						count++;
					}
					else
					{
						if(count < ::g_ConfigManager.m_nDivisionIndex)
						{
							item.Init();
							item.visionIndex = m_vecInspItemMatch[match - 1].visionIndex;
							item.divIndex = count;

							m_vecInspItemMatch.insert(m_vecInspItemMatch.begin() + match, item);
						}
						else
						{
							count = 0;
						}

						match--;
					}
				}
				else
				{
					if(m_vecInspItemMatch[match].divIndex == count)
					{
						count++;
					}
					else
					{
						if(count < ::g_ConfigManager.m_nDivisionIndex)
						{
							item.Init();
							item.visionIndex = m_vecInspItemMatch[match - 1].visionIndex;
							item.divIndex = count;

							m_vecInspItemMatch.insert(m_vecInspItemMatch.begin() + match, item);
						}
						else
						{
							count = 0;
						}

						match--;
					}
				}
			}
		}

		m_vecInspItemMatch.resize(newSize);
	}
	// Match Num이 줄어들었을 경우
	else if(newSize < oldSize)
	{
		for(long match = 0; match < m_vecInspItemMatch.size(); match++)
		{
			if(m_vecInspItemMatch[match].visionIndex < camCount)
			{
				if(m_vecInspItemMatch[match].divIndex >= ::g_ConfigManager.m_nDivisionIndex)
				{
					m_vecInspItemMatch.erase(m_vecInspItemMatch.begin() + match);

					match--;
				}
			}
			else
			{
				m_vecInspItemMatch.erase(m_vecInspItemMatch.begin() + match);

				match--;
			}
		}
	}

	m_nInspItemMatchNum = (long)m_vecInspItemMatch.size();
}

void CRecipeManager::InitParameters()
{
	m_nInspItemNum = 1;
	SInspItem sInspItem;
	sInspItem.Init();
	m_vecsInspItem.clear(); // 검사 아이템 초기화 - SHK 2023.12.27
	m_vecsInspItem.push_back(sInspItem);

	m_nInspItemMatchNum = 0;
	m_vecInspItemMatch.clear();

	for(long vision = 0; vision < CAMERA_MAX_COUNT; vision++)
	{
		for(long div = 0; div < DIVISION_INDEX_MAX; div++)
		{
			SMatchItem item;
			item.Init();
			item.visionIndex = vision;
			item.divIndex = div;
			m_vecInspItemMatch.push_back(item);
		}
	}

	m_sIllum.Init();
	m_sBusbarInfoParam.Init();
}

BOOL CRecipeManager::LoadRecipeParameters(const CString strRecipePathName)
{
	if(strRecipePathName.IsEmpty())
	{
		::g_Log.Log(_T("[RECIPE_DIAG][ERROR] Recipe load rejected - empty path"));
		return FALSE;
	}

	EnterCriticalSection(&m_csRecipe); // 250307 PSJ  Load 스레드 락
	CString strTemp1, strTemp2, strTemp3;
	::g_Log.Log(_T("Recipe Load path   : %s >>===CRecipeManager::LoadRecipeParameters"), strRecipePathName);//LYS 241016 log추가
	m_currentRecipeName = strRecipePathName;
	if(strRecipePathName == _T(""))
	{
		return FALSE;
	}

	// m_sBusbarInfoParam
	HUBSDK::IniReadWriter::Read(strRecipePathName, _T("Busbar Info Param"), _T("Inspection Division"), &m_sBusbarInfoParam.nInspectionDivision, m_sBusbarInfoParam.nInspectionDivision);
	HUBSDK::IniReadWriter::Read(strRecipePathName, _T("Busbar Info Param"), _T("DC Count"), &m_sBusbarInfoParam.nDCCount, m_sBusbarInfoParam.nDCCount);
	HUBSDK::IniReadWriter::Read(strRecipePathName, _T("Busbar Info Param"), _T("AC Count"), &m_sBusbarInfoParam.nACCount, m_sBusbarInfoParam.nACCount);
	HUBSDK::IniReadWriter::Read(strRecipePathName, _T("Busbar Info Param"), _T("Grab Count"), &m_sBusbarInfoParam.nGrabCount, m_sBusbarInfoParam.nGrabCount);

	// m_vecInspItemMatch
	strTemp1.Format(_T("Insp Item Match"));
	HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Insp. Item Match Num"), &m_nInspItemMatchNum, m_nInspItemMatchNum);
	m_vecInspItemMatch.resize(m_nInspItemMatchNum);

	for(long match = 0; match < m_nInspItemMatchNum; match++)
	{
		strTemp2.Format(_T("Match%d Vision Index"), match);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].visionIndex, m_vecInspItemMatch[match].visionIndex);
		strTemp2.Format(_T("Match%d Division Index"), match);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].divIndex, m_vecInspItemMatch[match].divIndex);
		strTemp2.Format(_T("Match%d Matching Index"), match);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].matchingIndex, m_vecInspItemMatch[match].matchingIndex);

		strTemp2.Format(_T("Match%d Sub Item Num"), match);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].subItemNum, m_vecInspItemMatch[match].subItemNum);
		m_vecInspItemMatch[match].vecsSubItemIndex.resize(m_vecInspItemMatch[match].subItemNum);
		for(long sub = 0; sub < m_vecInspItemMatch[match].subItemNum; sub++)
		{
			strTemp2.Format(_T("Match%d Sub Item Index %d"), match, sub);
			HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].vecsSubItemIndex[sub], m_vecInspItemMatch[match].vecsSubItemIndex[sub]);
		}
	}

	CalcMatchSize();

	// m_vecsInspItem
	strTemp1.Format(_T("Insp Item"));	// InspItemNum 이름 추가 - SHK 2023.12.21
	HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Insp. Item Num"), &m_nInspItemNum, m_nInspItemNum);
	m_vecsInspItem.resize(m_nInspItemNum);
	for(long insp = 0; insp < m_nInspItemNum; insp++)
	{
		strTemp1.Format(_T("Insp. Item %d"), insp);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Insp. Name"), &m_vecsInspItem[insp].strName, m_vecsInspItem[insp].strName);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Insp. Vision"), &m_vecsInspItem[insp].visionIndex, m_vecsInspItem[insp].visionIndex); // Insp Item Vision 추가 - SHK 2023.12.26
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Insp. Div"), &m_vecsInspItem[insp].divIndex, m_vecsInspItem[insp].divIndex); // Insp Item Div 추가 - SHK 2023.12.26
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Insp. Sequence"), &m_vecsInspItem[insp].algoSequence, m_vecsInspItem[insp].algoSequence);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Insp. InspType"), &m_vecsInspItem[insp].nInspType, m_vecsInspItem[insp].nInspType);

		//Illum
		for(long ch = 0; ch < ILLUM_CHANNEL_MAX; ch++)
		{
			strTemp2.Format(_T("Illumination Ch%d"), ch);
			HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sIllum.m_usIllum[ch], m_vecsInspItem[insp].sIllum.m_usIllum[ch]);
		}

		//Proc
		for(long proc = 0; proc < 3; proc++)
		{
			strTemp2.Format(_T("%d_Image Proc. Num"), proc);
			HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sProc[proc].nImageProcNum, m_vecsInspItem[insp].sProc[proc].nImageProcNum);
			m_vecsInspItem[insp].sProc[proc].m_vecnProc.resize(m_vecsInspItem[insp].sProc[proc].nImageProcNum);
			for(long idx = 0; idx < m_vecsInspItem[insp].sProc[proc].nImageProcNum; idx++)
			{
				strTemp2.Format(_T("%d_Image_Proc_%d"), proc, idx);
				HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sProc[proc].m_vecnProc[idx], m_vecsInspItem[insp].sProc[proc].m_vecnProc[idx]);
			}

			m_vecsInspItem[insp].sProc[proc].m_vecnProcParams.resize(m_vecsInspItem[insp].sProc[proc].nImageProcNum);
			for(long idx = 0; idx < m_vecsInspItem[insp].sProc[proc].nImageProcNum; idx++)
			{
				strTemp2.Format(_T("%d_Image_Proc_Param_%d"), proc, idx);
				HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sProc[proc].m_vecnProcParams[idx], m_vecsInspItem[insp].sProc[proc].m_vecnProcParams[idx]);
			}
		}

		//ROI
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Use ROI"), &m_vecsInspItem[insp].sROI.bUseROI, m_vecsInspItem[insp].sROI.bUseROI);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Search ROI Num"), &m_vecsInspItem[insp].sROI.nSearchROINum, m_vecsInspItem[insp].sROI.nSearchROINum);
		m_vecsInspItem[insp].sROI.vecrtSearchROI.resize(m_vecsInspItem[insp].sROI.nSearchROINum);
		for(long idx = 0; idx < m_vecsInspItem[insp].sROI.nSearchROINum; idx++)
		{
			strTemp2.Format(_T("Search ROI_%d"), idx);
			HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sROI.vecrtSearchROI[idx], m_vecsInspItem[insp].sROI.vecrtSearchROI[idx]);
		}

		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Ignore ROI Num"), &m_vecsInspItem[insp].sROI.nIgnoreROINum, m_vecsInspItem[insp].sROI.nIgnoreROINum);
		m_vecsInspItem[insp].sROI.vecrtIgnoreROI.resize(m_vecsInspItem[insp].sROI.nIgnoreROINum);
		for(long idx = 0; idx < m_vecsInspItem[insp].sROI.nIgnoreROINum; idx++)
		{
			strTemp2.Format(_T("Ignore ROI_%d"), idx);
			HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sROI.vecrtIgnoreROI[idx], m_vecsInspItem[insp].sROI.vecrtIgnoreROI[idx]);
		}

		//Threshold
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Use Threshold"), &m_vecsInspItem[insp].sThreshold.bUseThreshold, m_vecsInspItem[insp].sThreshold.bUseThreshold);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Select Threshold"), &m_vecsInspItem[insp].sThreshold.nSelectThreshold, m_vecsInspItem[insp].sThreshold.nSelectThreshold);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Threshold Value"), &m_vecsInspItem[insp].sThreshold.nThresholdValue, m_vecsInspItem[insp].sThreshold.nThresholdValue);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Adaptive Offset"), &m_vecsInspItem[insp].sThreshold.nAdaptiveOffset, m_vecsInspItem[insp].sThreshold.nAdaptiveOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Hysteresis Low"), &m_vecsInspItem[insp].sThreshold.nHysteresisLow, m_vecsInspItem[insp].sThreshold.nHysteresisLow);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Hysteresis High"), &m_vecsInspItem[insp].sThreshold.nHysteresisHigh, m_vecsInspItem[insp].sThreshold.nHysteresisHigh);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("High Threshold Region Noise Filtering Size"), &m_vecsInspItem[insp].sThreshold.nHighThresholdRegionNoiseFilteringSize, m_vecsInspItem[insp].sThreshold.nHighThresholdRegionNoiseFilteringSize);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Threshold Reverse"), &m_vecsInspItem[insp].sThreshold.bThresholdReverse, m_vecsInspItem[insp].sThreshold.bThresholdReverse);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Left Margin"), &m_vecsInspItem[insp].sThreshold.nLeftMargin, m_vecsInspItem[insp].sThreshold.nLeftMargin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Right Margin"), &m_vecsInspItem[insp].sThreshold.nRightMargin, m_vecsInspItem[insp].sThreshold.nRightMargin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Threshold Offset"), &m_vecsInspItem[insp].sThreshold.nThresholdOffset, m_vecsInspItem[insp].sThreshold.nThresholdOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Vignette Strength"), &m_vecsInspItem[insp].sThreshold.fVignetteStrength, m_vecsInspItem[insp].sThreshold.fVignetteStrength);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Use Re-Inspection"), &m_vecsInspItem[insp].sThreshold.bUseReInspection, m_vecsInspItem[insp].sThreshold.bUseReInspection); // PSJ
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Re-Inspection Offset"), &m_vecsInspItem[insp].sThreshold.nReInspectionOffset, m_vecsInspItem[insp].sThreshold.nReInspectionOffset); // PSJ
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Use Adaptive Threshold"), &m_vecsInspItem[insp].sThreshold.bUseAdaptiveThreshold, m_vecsInspItem[insp].sThreshold.bUseAdaptiveThreshold); // PSJ
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Adaptive Threshold Offset"), &m_vecsInspItem[insp].sThreshold.nAdaptiveThresholdOffset, m_vecsInspItem[insp].sThreshold.nAdaptiveThresholdOffset); // PSJ

		//Blob
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Deflate Left"), &m_vecsInspItem[insp].sBlob.nDeflateLeft, m_vecsInspItem[insp].sBlob.nDeflateLeft);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Deflate Top"), &m_vecsInspItem[insp].sBlob.nDeflateTop, m_vecsInspItem[insp].sBlob.nDeflateTop);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Deflate Right"), &m_vecsInspItem[insp].sBlob.nDeflateRight, m_vecsInspItem[insp].sBlob.nDeflateRight);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Deflate Bottom"), &m_vecsInspItem[insp].sBlob.nDeflateBottom, m_vecsInspItem[insp].sBlob.nDeflateBottom);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Min Blob Size"), &m_vecsInspItem[insp].sBlob.nMinBlobSize, m_vecsInspItem[insp].sBlob.nMinBlobSize);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Max Blob Count"), &m_vecsInspItem[insp].sBlob.nMaxBlobCount, m_vecsInspItem[insp].sBlob.nMaxBlobCount);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob Merge"), &m_vecsInspItem[insp].sBlob.bBlobMerge, m_vecsInspItem[insp].sBlob.bBlobMerge);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob Merge Dist"), &m_vecsInspItem[insp].sBlob.nBlobMergeDist, m_vecsInspItem[insp].sBlob.nBlobMergeDist);

		//Blob2
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("(Blob2)Deflate Left"), &m_vecsInspItem[insp].sBlob2.nDeflateLeft, m_vecsInspItem[insp].sBlob2.nDeflateLeft);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("(Blob2)Deflate Top"), &m_vecsInspItem[insp].sBlob2.nDeflateTop, m_vecsInspItem[insp].sBlob2.nDeflateTop);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("(Blob2)Deflate Right"), &m_vecsInspItem[insp].sBlob2.nDeflateRight, m_vecsInspItem[insp].sBlob2.nDeflateRight);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("(Blob2)Deflate Bottom"), &m_vecsInspItem[insp].sBlob2.nDeflateBottom, m_vecsInspItem[insp].sBlob2.nDeflateBottom);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("(Blob2)Min Blob Size"), &m_vecsInspItem[insp].sBlob2.nMinBlobSize, m_vecsInspItem[insp].sBlob2.nMinBlobSize);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("(Blob2)Max Blob Count"), &m_vecsInspItem[insp].sBlob2.nMaxBlobCount, m_vecsInspItem[insp].sBlob2.nMaxBlobCount);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("(Blob2)Blob Merge"), &m_vecsInspItem[insp].sBlob2.bBlobMerge, m_vecsInspItem[insp].sBlob2.bBlobMerge);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("(Blob2)Blob Merge Dist"), &m_vecsInspItem[insp].sBlob2.nBlobMergeDist, m_vecsInspItem[insp].sBlob2.nBlobMergeDist);

		//EdgeDetect
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Deflate ROI Width"), &m_vecsInspItem[insp].sAlign.nDeflateROIWidth, m_vecsInspItem[insp].sAlign.nDeflateROIWidth);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Deflate ROI Height"), &m_vecsInspItem[insp].sAlign.nDeflateROIHeight, m_vecsInspItem[insp].sAlign.nDeflateROIHeight);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Direction"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nDirection, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nDirection);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Search Direction"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nSearchDirection, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nSearchDirection);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Edge Direction"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nEdgeDirection, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nEdgeDirection);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_First Edge"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bFirstEdge, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bFirstEdge);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_SampleRate"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nSampleRate, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nSampleRate);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Edge Threshold"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.fEdgeThreshold, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.fEdgeThreshold);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Use Edge Filtering"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bUseEdgeFiltering, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bUseEdgeFiltering);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Edge Filtering Range"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nEdgeFilteringRange, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nEdgeFilteringRange);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_NG Edge Erase"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bNGEdgeErase, m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bNGEdgeErase);

		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("EdgeDetect_Edge Ignore ROI Num"), &m_vecsInspItem[insp].sAlign.nIgnoreROINum, m_vecsInspItem[insp].sAlign.nIgnoreROINum);
		m_vecsInspItem[insp].sAlign.vecrtIgnoreROI.resize(m_vecsInspItem[insp].sAlign.nIgnoreROINum);
		for(long idx = 0; idx < m_vecsInspItem[insp].sAlign.nIgnoreROINum; idx++)
		{
			strTemp2.Format(_T("EdgeDetect_Edge Ignore ROI_%d"), idx);
			HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sAlign.vecrtIgnoreROI[idx], m_vecsInspItem[insp].sAlign.vecrtIgnoreROI[idx]);
		}

		// Result
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob Start Pt X Offset"), &m_vecsInspItem[insp].sResult.fBlobStartPtXOffset, m_vecsInspItem[insp].sResult.fBlobStartPtXOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob Start Pt Y Offset"), &m_vecsInspItem[insp].sResult.fBlobStartPtYOffset, m_vecsInspItem[insp].sResult.fBlobStartPtYOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob End Pt X Offset"), &m_vecsInspItem[insp].sResult.fBlobEndPtXOffset, m_vecsInspItem[insp].sResult.fBlobEndPtXOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob End Pt Y Offset"), &m_vecsInspItem[insp].sResult.fBlobEndPtYOffset, m_vecsInspItem[insp].sResult.fBlobEndPtYOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Edge Start Pt X Offset"), &m_vecsInspItem[insp].sResult.fEdgeStartPtXOffset, m_vecsInspItem[insp].sResult.fEdgeStartPtXOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Edge Start Pt Y Offset"), &m_vecsInspItem[insp].sResult.fEdgeStartPtYOffset, m_vecsInspItem[insp].sResult.fEdgeStartPtYOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Edge End Pt X Offset"), &m_vecsInspItem[insp].sResult.fEdgeEndPtXOffset, m_vecsInspItem[insp].sResult.fEdgeEndPtXOffset);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Edge End Pt Y Offset"), &m_vecsInspItem[insp].sResult.fEdgeEndPtYOffset, m_vecsInspItem[insp].sResult.fEdgeEndPtYOffset);

		// Criteria
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob Width Min"), &m_vecsInspItem[insp].sCriteria.nBlobWidthMin, m_vecsInspItem[insp].sCriteria.nBlobWidthMin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob Width Max"), &m_vecsInspItem[insp].sCriteria.nBlobWidthMax, m_vecsInspItem[insp].sCriteria.nBlobWidthMax);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob Height Min"), &m_vecsInspItem[insp].sCriteria.nBlobHeightMin, m_vecsInspItem[insp].sCriteria.nBlobHeightMin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Blob Height Max"), &m_vecsInspItem[insp].sCriteria.nBlobHeightMax, m_vecsInspItem[insp].sCriteria.nBlobHeightMax);



		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Weld Width. Min"), &m_vecsInspItem[insp].sCriteria.fWeldWidthMin , m_vecsInspItem[insp].sCriteria.fWeldWidthMin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Weld Width. Max"), &m_vecsInspItem[insp].sCriteria.fWeldWidthMax, m_vecsInspItem[insp].sCriteria.fWeldWidthMax);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Weld Height Min"), &m_vecsInspItem[insp].sCriteria.fWeldHeightMin, m_vecsInspItem[insp].sCriteria.fWeldHeightMin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Weld Height Max"), &m_vecsInspItem[insp].sCriteria.fWeldHeightMax, m_vecsInspItem[insp].sCriteria.fWeldHeightMax);

		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Between Dist. Min. X"), &m_vecsInspItem[insp].sCriteria.fBetweenDistMinX, m_vecsInspItem[insp].sCriteria.fBetweenDistMinX);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Between Dist. Max. X"), &m_vecsInspItem[insp].sCriteria.fBetweenDistMaxX, m_vecsInspItem[insp].sCriteria.fBetweenDistMaxX);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Between Dist. Min. Y"), &m_vecsInspItem[insp].sCriteria.fBetweenDistMinY, m_vecsInspItem[insp].sCriteria.fBetweenDistMinY);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Between Dist. Max. Y"), &m_vecsInspItem[insp].sCriteria.fBetweenDistMaxY, m_vecsInspItem[insp].sCriteria.fBetweenDistMaxY);

		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Length Min"), &m_vecsInspItem[insp].sCriteria.fLengthMin, m_vecsInspItem[insp].sCriteria.fLengthMin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Length Max"), &m_vecsInspItem[insp].sCriteria.fLengthMax, m_vecsInspItem[insp].sCriteria.fLengthMax);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Angle Min"), &m_vecsInspItem[insp].sCriteria.fAngleMin, m_vecsInspItem[insp].sCriteria.fAngleMin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Angle Max"), &m_vecsInspItem[insp].sCriteria.fAngleMax, m_vecsInspItem[insp].sCriteria.fAngleMax);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Top Busbar Height Min"), &m_vecsInspItem[insp].sCriteria.fTopBusbarHeightMin, m_vecsInspItem[insp].sCriteria.fTopBusbarHeightMin);
		HUBSDK::IniReadWriter::Read(strRecipePathName, strTemp1, _T("Top Busbar Height Max"), &m_vecsInspItem[insp].sCriteria.fTopBusbarHeightMax, m_vecsInspItem[insp].sCriteria.fTopBusbarHeightMax);
	}

	// m_sIllum
	for(long ch = 0; ch < ILLUM_CHANNEL_MAX; ch++)
	{
		strTemp1.Format(_T("Ch%d"), ch);
		HUBSDK::IniReadWriter::Read(strRecipePathName, _T("Illumination"), strTemp1, &m_sIllum.m_usIllum[ch], m_sIllum.m_usIllum[ch]);
	}

	LeaveCriticalSection(&m_csRecipe); // 250307 PSJ   Load 마친 후 스레드 락 해제
	return TRUE;
}

BOOL CRecipeManager::SaveRecipeParameters(const CString strRecipePathName)
{
	if(strRecipePathName.IsEmpty())
	{
		::g_Log.Log(_T("[RECIPE_DIAG][ERROR] Recipe save rejected - empty path"));
		return FALSE;
	}

	EnterCriticalSection(&m_csRecipe); // 250307 PSJ   Save 스레드 락
	CString strTemp1, strTemp2;
	::g_Log.Log(_T("Recipe Save path   : %s <<===CRecipeManager::SaveRecipeParameters"), strRecipePathName);//LYS 241016 log추가

	if(strRecipePathName == _T(""))
	{
		return FALSE;
	}
	
	// m_sBusbarInfoParam
	HUBSDK::IniReadWriter::Write(strRecipePathName, _T("Busbar Info Param"), _T("Inspection Division"), &m_sBusbarInfoParam.nInspectionDivision);
	HUBSDK::IniReadWriter::Write(strRecipePathName, _T("Busbar Info Param"), _T("DC Count"), &m_sBusbarInfoParam.nDCCount);
	HUBSDK::IniReadWriter::Write(strRecipePathName, _T("Busbar Info Param"), _T("AC Count"), &m_sBusbarInfoParam.nACCount);
	HUBSDK::IniReadWriter::Write(strRecipePathName, _T("Busbar Info Param"), _T("Grab Count"), &m_sBusbarInfoParam.nGrabCount);
	
	// m_vecInspItemMatch
	m_nInspItemMatchNum = (long)m_vecInspItemMatch.size();
	strTemp1.Format(_T("Insp Item Match"));
	HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Insp. Item Match Num"), &m_nInspItemMatchNum);
	for(long match = 0; match < m_nInspItemMatchNum; match++)
	{
		strTemp2.Format(_T("Match%d Vision Index"), match);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].visionIndex);
		strTemp2.Format(_T("Match%d Division Index"), match);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].divIndex);
		strTemp2.Format(_T("Match%d Matching Index"), match);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].matchingIndex);

		m_vecInspItemMatch[match].subItemNum = (long)m_vecInspItemMatch[match].vecsSubItemIndex.size();
		strTemp2.Format(_T("Match%d Sub Item Num"), match);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].subItemNum);
		for(long sub = 0; sub < m_vecInspItemMatch[match].subItemNum; sub++)
		{
			strTemp2.Format(_T("Match%d Sub Item Index %d"), match, sub);
			HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecInspItemMatch[match].vecsSubItemIndex[sub]);
		}
	}

	// m_vecsInspItem
	m_nInspItemNum = (long)m_vecsInspItem.size(); 	// InspItemNum 개수 업데이트 및 이름 추가 - SHK 2023.12.21
	strTemp1.Format(_T("Insp Item"));	// InspItemNum 개수 업데이트 및 이름 추가 - SHK 2023.12.21
	HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Insp. Item Num"), &m_nInspItemNum);
	for(long insp = 0; insp < m_nInspItemNum; insp++)
	{
		strTemp1.Format(_T("Insp. Item %d"), insp);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Insp. Name"), &m_vecsInspItem[insp].strName);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Insp. Vision"), &m_vecsInspItem[insp].visionIndex); // Insp Item Vision 추가 - SHK 2023.12.26
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Insp. Div"), &m_vecsInspItem[insp].divIndex); // Insp Item Div 추가 - SHK 2023.12.26
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Insp. Sequence"), &m_vecsInspItem[insp].algoSequence);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Insp. InspType"), &m_vecsInspItem[insp].nInspType);

		//Illum
		for(long ch = 0; ch < ILLUM_CHANNEL_MAX; ch++)
		{
			strTemp2.Format(_T("Illumination Ch%d"), ch);
			CString strTemp3;
			strTemp3.Format(_T("Illumination Ch%d = %d"), ch, m_vecsInspItem[insp].sIllum.m_usIllum[ch]);
			HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sIllum.m_usIllum[ch]);
		}

		//Proc
		for(long proc = 0; proc < 3; proc++)
		{
			strTemp2.Format(_T("%d_Image Proc. Num"), proc);
			HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sProc[proc].nImageProcNum);
			m_vecsInspItem[insp].sProc[proc].m_vecnProc.resize(m_vecsInspItem[insp].sProc[proc].nImageProcNum);
			for(long idx = 0; idx < m_vecsInspItem[insp].sProc[proc].nImageProcNum; idx++)
			{
				strTemp2.Format(_T("%d_Image_Proc_%d"), proc, idx);
				HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sProc[proc].m_vecnProc[idx]);
			}

			m_vecsInspItem[insp].sProc[proc].m_vecnProcParams.resize(m_vecsInspItem[insp].sProc[proc].nImageProcNum);
			for(long idx = 0; idx < m_vecsInspItem[insp].sProc[proc].nImageProcNum; idx++)
			{
				strTemp2.Format(_T("%d_Image_Proc_Param_%d"), proc, idx);
				HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sProc[proc].m_vecnProcParams[idx]);
			}
		}

		//ROI
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Use ROI"), &m_vecsInspItem[insp].sROI.bUseROI);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Search ROI Num"), &m_vecsInspItem[insp].sROI.nSearchROINum);
		m_vecsInspItem[insp].sROI.vecrtSearchROI.resize(m_vecsInspItem[insp].sROI.nSearchROINum);
		for(long idx = 0; idx < m_vecsInspItem[insp].sROI.nSearchROINum; idx++)
		{
			strTemp2.Format(_T("Search ROI_%d"), idx);
			HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sROI.vecrtSearchROI[idx]);
		}

		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Ignore ROI Num"), &m_vecsInspItem[insp].sROI.nIgnoreROINum);
		m_vecsInspItem[insp].sROI.vecrtIgnoreROI.resize(m_vecsInspItem[insp].sROI.nIgnoreROINum);
		for(long idx = 0; idx < m_vecsInspItem[insp].sROI.nIgnoreROINum; idx++)
		{
			strTemp2.Format(_T("Ignore ROI_%d"), idx);
			HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sROI.vecrtIgnoreROI[idx]);
		}

		//Threshold
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Use Threshold"), &m_vecsInspItem[insp].sThreshold.bUseThreshold);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Select Threshold"), &m_vecsInspItem[insp].sThreshold.nSelectThreshold);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Threshold Value"), &m_vecsInspItem[insp].sThreshold.nThresholdValue);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Adaptive Offset"), &m_vecsInspItem[insp].sThreshold.nAdaptiveOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Hysteresis Low"), &m_vecsInspItem[insp].sThreshold.nHysteresisLow);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Hysteresis High"), &m_vecsInspItem[insp].sThreshold.nHysteresisHigh);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("High Threshold Region Noise Filtering Size"), &m_vecsInspItem[insp].sThreshold.nHighThresholdRegionNoiseFilteringSize);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Threshold Reverse"), &m_vecsInspItem[insp].sThreshold.bThresholdReverse);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Left Margin"), &m_vecsInspItem[insp].sThreshold.nLeftMargin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Right Margin"), &m_vecsInspItem[insp].sThreshold.nRightMargin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Threshold Offset"), &m_vecsInspItem[insp].sThreshold.nThresholdOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Vignette Strength"), &m_vecsInspItem[insp].sThreshold.fVignetteStrength);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Use Re-Inspection"), &m_vecsInspItem[insp].sThreshold.bUseReInspection);// PSJ
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Re-Inspection Offset"), &m_vecsInspItem[insp].sThreshold.nReInspectionOffset); // PSJ
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Use Adaptive Threshold"), &m_vecsInspItem[insp].sThreshold.bUseAdaptiveThreshold);// PSJ
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Adaptive Threshold Offset"), &m_vecsInspItem[insp].sThreshold.nAdaptiveThresholdOffset); // PSJ

		//Blob
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Deflate Left"), &m_vecsInspItem[insp].sBlob.nDeflateLeft);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Deflate Top"), &m_vecsInspItem[insp].sBlob.nDeflateTop);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Deflate Right"), &m_vecsInspItem[insp].sBlob.nDeflateRight);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Deflate Bottom"), &m_vecsInspItem[insp].sBlob.nDeflateBottom);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Min Blob Size"), &m_vecsInspItem[insp].sBlob.nMinBlobSize);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Max Blob Count"), &m_vecsInspItem[insp].sBlob.nMaxBlobCount);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob Merge"), &m_vecsInspItem[insp].sBlob.bBlobMerge);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob Merge Dist"), &m_vecsInspItem[insp].sBlob.nBlobMergeDist);

		//Blob2
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("(Blob2)Deflate Left"), &m_vecsInspItem[insp].sBlob2.nDeflateLeft);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("(Blob2)Deflate Top"), &m_vecsInspItem[insp].sBlob2.nDeflateTop);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("(Blob2)Deflate Right"), &m_vecsInspItem[insp].sBlob2.nDeflateRight);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("(Blob2)Deflate Bottom"), &m_vecsInspItem[insp].sBlob2.nDeflateBottom);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("(Blob2)Min Blob Size"), &m_vecsInspItem[insp].sBlob2.nMinBlobSize);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("(Blob2)Max Blob Count"), &m_vecsInspItem[insp].sBlob2.nMaxBlobCount);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("(Blob2)Blob Merge"), &m_vecsInspItem[insp].sBlob2.bBlobMerge);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("(Blob2)Blob Merge Dist"), &m_vecsInspItem[insp].sBlob2.nBlobMergeDist);

		//EdgeDetect
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Deflate ROI Width"), &m_vecsInspItem[insp].sAlign.nDeflateROIWidth);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Deflate ROI Height"), &m_vecsInspItem[insp].sAlign.nDeflateROIHeight);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Direction"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nDirection);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Search Direction"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nSearchDirection);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Edge Direction"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nEdgeDirection);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_First Edge"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bFirstEdge);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_SampleRate"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nSampleRate);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Edge Threshold"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.fEdgeThreshold);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Use Edge Filtering"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bUseEdgeFiltering);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Edge Filtering Range"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.nEdgeFilteringRange);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_NG Edge Erase"), &m_vecsInspItem[insp].sAlign.sEdgeDetectParam.bNGEdgeErase);

		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("EdgeDetect_Edge Ignore ROI Num"), &m_vecsInspItem[insp].sAlign.nIgnoreROINum);
		m_vecsInspItem[insp].sAlign.vecrtIgnoreROI.resize(m_vecsInspItem[insp].sAlign.nIgnoreROINum);
		for(long idx = 0; idx < m_vecsInspItem[insp].sAlign.nIgnoreROINum; idx++)
		{
			strTemp2.Format(_T("EdgeDetect_Edge Ignore ROI_%d"), idx);
			HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, strTemp2, &m_vecsInspItem[insp].sAlign.vecrtIgnoreROI[idx]);
		}

		// Result
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob Start Pt X Offset"), &m_vecsInspItem[insp].sResult.fBlobStartPtXOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob Start Pt Y Offset"), &m_vecsInspItem[insp].sResult.fBlobStartPtYOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob End Pt X Offset"), &m_vecsInspItem[insp].sResult.fBlobEndPtXOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob End Pt Y Offset"), &m_vecsInspItem[insp].sResult.fBlobEndPtYOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Edge Start Pt X Offset"), &m_vecsInspItem[insp].sResult.fEdgeStartPtXOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Edge Start Pt Y Offset"), &m_vecsInspItem[insp].sResult.fEdgeStartPtYOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Edge End Pt X Offset"), &m_vecsInspItem[insp].sResult.fEdgeEndPtXOffset);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Edge End Pt Y Offset"), &m_vecsInspItem[insp].sResult.fEdgeEndPtYOffset);

		// Criteria
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob Width Min"), &m_vecsInspItem[insp].sCriteria.nBlobWidthMin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob Width Max"), &m_vecsInspItem[insp].sCriteria.nBlobWidthMax);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob Height Min"), &m_vecsInspItem[insp].sCriteria.nBlobHeightMin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Blob Height Max"), &m_vecsInspItem[insp].sCriteria.nBlobHeightMax);

		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Weld Width. Min"), &m_vecsInspItem[insp].sCriteria.fWeldWidthMin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Weld Width. Max"), &m_vecsInspItem[insp].sCriteria.fWeldWidthMax);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Weld Height Min"), &m_vecsInspItem[insp].sCriteria.fWeldHeightMin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Weld Height Max"), &m_vecsInspItem[insp].sCriteria.fWeldHeightMax);

		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Between Dist. Min. X"), &m_vecsInspItem[insp].sCriteria.fBetweenDistMinX);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Between Dist. Max. X"), &m_vecsInspItem[insp].sCriteria.fBetweenDistMaxX);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Between Dist. Min. Y"), &m_vecsInspItem[insp].sCriteria.fBetweenDistMinY);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Between Dist. Max. Y"), &m_vecsInspItem[insp].sCriteria.fBetweenDistMaxY);

		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Length Min"), &m_vecsInspItem[insp].sCriteria.fLengthMin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Length Max"), &m_vecsInspItem[insp].sCriteria.fLengthMax);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Angle Min"), &m_vecsInspItem[insp].sCriteria.fAngleMin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Angle Max"), &m_vecsInspItem[insp].sCriteria.fAngleMax);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Top Busbar Height Min"), &m_vecsInspItem[insp].sCriteria.fTopBusbarHeightMin);
		HUBSDK::IniReadWriter::Write(strRecipePathName, strTemp1, _T("Top Busbar Height Max"), &m_vecsInspItem[insp].sCriteria.fTopBusbarHeightMax);
	}

	// m_sIllum
	for(long ch = 0; ch < ILLUM_CHANNEL_MAX; ch++)
	{
		strTemp1.Format(_T("Ch%d"), ch);
		HUBSDK::IniReadWriter::Write(strRecipePathName, _T("Illumination"), strTemp1, &m_sIllum.m_usIllum[ch]);
	}

	LeaveCriticalSection(&m_csRecipe); // 250307 PSJ   Save 마친 후 스레드 락 해제
	return TRUE;
}
