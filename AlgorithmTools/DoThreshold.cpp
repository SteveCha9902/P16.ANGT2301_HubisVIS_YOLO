#include "pch.h"
#include "Inspection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CInspection::DoThresholdImage(long itemIndex)
{
	BYTE* src = nullptr;
	BYTE* dst = nullptr;
	BYTE* proc = nullptr;

	// Camera Index에 따른 Image
	auto InspItem = ::g_RecipeManager.m_vecsInspItem[itemIndex];
	auto InspRoi = ::g_RecipeManager.m_vecsInspItem[itemIndex].sROI;
	auto InspThreshold = ::g_RecipeManager.m_vecsInspItem[itemIndex].sThreshold;

	switch(itemIndex)
	{
	case (long)IMAGE::ORIGINAL11://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL12:// (long)CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL13://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL21://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL22://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL23://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH);
	}
	break;
	}

	//ROI 작아지면 기존 데이터가 남음
	memset(dst, 0, m_nImageSize);

	if (m_pAlgorithm->DoProcImage(src, m_nImageWidth, m_nImageHeight, ::g_RecipeManager.m_vecsInspItem[itemIndex].sProc[0], proc) == FALSE)
	{
		return FALSE;
	}

	if (::g_RecipeManager.m_vecsInspItem[itemIndex].sThreshold.bUseThreshold)
	{
		// Threshold 적용
		if (m_pAlgorithm->DoThresholdImage(proc, m_nImageWidth, m_nImageHeight, InspRoi, ::g_RecipeManager.m_vecsInspItem[itemIndex].sThreshold , dst, m_bReInspection) == FALSE) // 재검사시 -20? 우선 -로 진행해보는 쪽으로 진행 +20?
		{
			OverlayAlarm(itemIndex, ALARM_THRESHOLD);
			return FALSE;
		}
	}
	else
	{
		memcpy(dst, proc, m_nImageSize);
	}

	return TRUE;
}

BOOL CInspection::DoThresholdImage2(long itemIndex)
{
	BYTE* src = nullptr;
	BYTE* dst = nullptr;
	BYTE* proc = nullptr;

	// Camera Index에 따른 Image
	auto InspItem = ::g_RecipeManager.m_vecsInspItem[itemIndex];
	auto InspRoi = ::g_RecipeManager.m_vecsInspItem[itemIndex].sROI;
	auto InspThreshold = ::g_RecipeManager.m_vecsInspItem[itemIndex].sThreshold;

	switch (itemIndex)
	{
	case (long)IMAGE::ORIGINAL11://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL12:// (long)CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL13://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL21://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL22://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH);
	}
	break;
	case (long)IMAGE::ORIGINAL23://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH_PROC);
		dst = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH);
	}
	break;
	}

	//ROI 작아지면 기존 데이터가 남음
	memset(dst, 0, m_nImageSize);

	if (m_pAlgorithm->DoProcImage(src, m_nImageWidth, m_nImageHeight, ::g_RecipeManager.m_vecsInspItem[itemIndex].sProc[0], proc) == FALSE)
	{
		return FALSE;
	}

	if (::g_RecipeManager.m_vecsInspItem[itemIndex].sThreshold.bUseThreshold)
	{
		// Threshold 적용
		if (m_pAlgorithm->DoThresholdImage2(proc, m_nImageWidth, m_nImageHeight, InspRoi, ::g_RecipeManager.m_vecsInspItem[itemIndex].sThreshold, dst) == FALSE)
		{
			return FALSE;
		}
	}
	else
	{
		memcpy(dst, proc, m_nImageSize);
	}

	return TRUE;
}