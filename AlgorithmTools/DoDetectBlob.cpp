#include "pch.h"
#include "Inspection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CInspection::DoDetectBlob(long itemIndex)
{
	CString strTemp;

	auto InspItem = ::g_RecipeManager.m_vecsInspItem[itemIndex];
	auto roiParams = InspItem.sROI;
	auto blobParams = InspItem.sBlob;

	// Camera Index에 따른 Image
	BYTE* src = nullptr;
	BYTE* threshold = nullptr;
	BYTE* proc = nullptr;

	switch(itemIndex)
	{
	case (long)IMAGE::ORIGINAL11://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_BLOB_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL12://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_BLOB_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL13://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_BLOB_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL21://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_BLOB_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL22://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_BLOB_PROC);
	}
	break;
	case (long)IMAGE::ORIGINAL23://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_BLOB_PROC);
	}
	break;
	}

	if (m_pAlgorithm->DoProcImage(threshold, m_nImageWidth, m_nImageHeight, ::g_RecipeManager.m_vecsInspItem[itemIndex].sProc[1], proc) == FALSE)
	{
		return FALSE;
	}
	
	m_vecBlobInfoList.clear();
	for(long idx = 0; idx < roiParams.nSearchROINum; idx++)
	{
		auto roi = roiParams.vecrtSearchROI[idx];
		roi.DeflateRect(blobParams.nDeflateLeft, blobParams.nDeflateTop, blobParams.nDeflateRight, blobParams.nDeflateBottom);

		std::vector<BlobInfo> vecsBlobList;

		if(m_pAlgorithm->DoDetectBlob(src, proc, m_nImageWidth, m_nImageHeight, roi, vecsBlobList, InspItem.sBlob, m_nBlobTotalCount) == FALSE)
		{
			OverlayAlarm(itemIndex, ALARM_BLOB);
			return FALSE;
		}

		for(long blob = 0; blob < (long)vecsBlobList.size(); blob++)
		{
			m_vecBlobInfoList.push_back(vecsBlobList[blob]);
		}
	}

	m_nBlobTotalCount = (long)m_vecBlobInfoList.size();

	DebugOverlay(itemIndex, DEBUG_BLOB_FOUND_BLOB);

	return TRUE;
}

BOOL CInspection::DoDetectBlob2(long itemIndex)
{
	CString strTemp;

	auto InspItem = ::g_RecipeManager.m_vecsInspItem[itemIndex];
	auto blob2Params = InspItem.sBlob2;

	// Camera Index에 따른 Image
	BYTE* src = nullptr;
	BYTE* threshold = nullptr;
	BYTE* proc = nullptr;
	switch(itemIndex)
	{
	case (long)IMAGE::ORIGINAL11://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL11_BLOB_PROC2);
	}
	break;
	case (long)IMAGE::ORIGINAL12://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL12_BLOB_PROC2);
	}
	break;
	case (long)IMAGE::ORIGINAL13://CAMERA_INDEX::CAMERA1:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL13_BLOB_PROC2);
	}
	break;
	case (long)IMAGE::ORIGINAL21://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL21_BLOB_PROC2);
	}
	break;
	case (long)IMAGE::ORIGINAL22://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL22_BLOB_PROC2);
	}
	break;
	case (long)IMAGE::ORIGINAL23://CAMERA_INDEX::CAMERA2:
	{
		src = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23);
		threshold = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_TH);
		proc = ::g_MemoryManager.GetByteImage((long)IMAGE::ORIGINAL23_BLOB_PROC2);
	}
	break;
	}

	if(m_pAlgorithm->DoProcImage(threshold, m_nImageWidth, m_nImageHeight, ::g_RecipeManager.m_vecsInspItem[itemIndex].sProc[2], proc) == FALSE)
	{
		return FALSE;
	}

	// BLOB1 의 Blob ROI 내 에서 Blob을 실시한다.
	m_vecBlobInfoList2.clear();
	for(long idx = 0; idx < (long)m_vecBlobInfoList.size(); idx++)
	{
		auto roi = m_vecBlobInfoList[idx].m_roi;
		//if (!(itemIndex % 2)) { roi.top = roi.bottom - 370; }

		roi.DeflateRect(blob2Params.nDeflateLeft, blob2Params.nDeflateTop, blob2Params.nDeflateRight, blob2Params.nDeflateBottom);
		
		std::vector<BlobInfo> vecsBlobList;
		if (roi.bottom - roi.top < 0) //LYS 241111 후검사예외처리
		{
			::g_Log.Log(_T("DoDetectBlob2 bot-top erorr"));
			OverlayAlarm(itemIndex, ALARM_BLOB2);
			return FALSE;
		}
		if(m_pAlgorithm->DoDetectBlob(src, proc, m_nImageWidth, m_nImageHeight, roi, vecsBlobList, InspItem.sBlob2, m_nBlobTotalCount2) == FALSE)
		{
			OverlayAlarm(itemIndex, ALARM_BLOB2);
			return FALSE;
		}

		for(long blob = 0; blob < (long)vecsBlobList.size(); blob++)
		{
			m_vecBlobInfoList2.push_back(vecsBlobList[blob]);
		}
	}

	m_nBlobTotalCount2 = (long)m_vecBlobInfoList2.size();

	DebugOverlay(itemIndex, DEBUG_BLOB2_FOUND_BLOB); // Debug Overlay...

	return TRUE;
}
