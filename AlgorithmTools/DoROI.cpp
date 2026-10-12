#include "pch.h"
#include "Inspection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CInspection::DoSetROI(long itemIndex)
{
	auto param = ::g_RecipeManager.m_vecsInspItem[itemIndex].sROI;

	//BYTE* srcImage = nullptr;
	//BYTE* procImage = nullptr;
	//BYTE* maskImage = nullptr;
	//BYTE* roiImage = nullptr;
	//BYTE* roiProcImage = nullptr;

	//// Camera Index¿¡ µû¸¥ Image
	//auto InspItem = ::g_RecipeManager.m_vecsInspItem[itemIndex];
	//switch(InspItem.visionIndex)
	//{
	//case (long)CAMERA_INDEX::CAMERA1:
	//	srcImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL1);
	//	roiImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL1_ROI);
	//	maskImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL1_MASK);

	//	break;
	//case (long)CAMERA_INDEX::CAMERA2:
	//	srcImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL2);
	//	roiImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL2_ROI);
	//	maskImage = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL2_MASK);
	//	break;
	//}

	//if(param.bUseROI)
	//{
	//	if(param.bUseApplyMaskROI)
	//	{
	//		memset(roiImage, 0, m_nImageSize);

	//		for(long idx = 0; idx < param.nSearchROINum; idx++)
	//		{
	//			HUBIP::ImageCopy(srcImage, m_nImageWidth, m_nImageHeight, param.vecrtSearchROI[idx], roiImage);
	//		}

	//		for(long idx = 0; idx < param.nIgnoreROINum; idx++)
	//		{
	//			HUBIP::ImageFill(param.vecrtIgnoreROI[idx], m_nImageWidth, m_nImageHeight, 0, roiImage);
	//		}
	//	}
	//	else
	//	{
	//		memcpy(roiImage, srcImage, m_nImageSize);
	//	}
	//}
	//else
	//{
	//	memcpy(roiImage, srcImage, m_nImageSize);
	//}

	DebugOverlayClear(DEBUG_EDGE_DETECT_SEARCH_ROI);
	DebugOverlayClear(DEBUG_EDGE_DETECT_IGNORE_ROI);

	DebugOverlay(itemIndex, DEBUG_ROI_SEARCH_ROI); // Debug Overlay...
	DebugOverlay(itemIndex, DEBUG_ROI_IGNORE_ROI);

	return TRUE;
}