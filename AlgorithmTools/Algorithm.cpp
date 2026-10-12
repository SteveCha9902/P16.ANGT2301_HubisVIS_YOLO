#include "pch.h"
#include "Algorithm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CAlgorithm g_Algorithm;

CAlgorithm::CAlgorithm()
{
	AllocateBlobBuffer();

	m_pEdgeDetect = new EdgeDetect;
}

CAlgorithm::~CAlgorithm()
{
	DestroyBlobBuffer();

	delete m_pEdgeDetect;
}

BOOL CAlgorithm::AllocateBlobBuffer()
{
	m_bufferHandle = nullptr;

	if(HUBIP::AllocateBlobBuffer(m_bufferHandle) != retStatus::success)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CAlgorithm::DestroyBlobBuffer()
{
	// Blob Buffer 해제
	if(m_bufferHandle != nullptr)
	{
		if(HUBIP::DestroyBlobBuffer(m_bufferHandle) != retStatus::success)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CAlgorithm::Blob(BYTE* src, const long imageWidth, const long imageHeight, const CRect roi, const BOOL b4Neighbor, const long minBlobSize, const long maxBlobCount, HUBSDK::BlobInfo* blobInfo, long &blobCount, long* labelImage)
{
	if(HUBIP::DetectBlob(src, imageWidth, imageHeight, roi, b4Neighbor, minBlobSize, maxBlobCount, m_bufferHandle, blobInfo, blobCount, labelImage) != retStatus::success)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CAlgorithm::BlobMerge(const long imageWidth, const long imageHeight, long* label, const long maxBlobCount, const float mergeDistance, HUBSDK::BlobInfo* blobInfo, long &nBlobMergeCount)
{
	if(HUBIP::BlobLink(imageWidth, imageHeight, label, maxBlobCount, mergeDistance, m_bufferHandle, blobInfo) != retStatus::success)
	{
		return FALSE;
	}

	if(HUBIP::BlobMerge(imageWidth, imageHeight, maxBlobCount, m_bufferHandle, label, blobInfo, nBlobMergeCount) != retStatus::success)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CAlgorithm::DoDetectBlob(BYTE * src, BYTE * threshold, const long imageWidth, const long imageHeight, const CRect roi, std::vector<HUBSDK::BlobInfo>& blobInfoList, SBlobParams params, long &blobCount)
{
	long* pnLabelImage = ::g_MemoryManager.GetLongImage(); // Label Image

	memset(pnLabelImage, 0, imageWidth * imageHeight * sizeof(long));

	long nBlobCount(0);
	long nBlobMergeCount(0);

	blobInfoList.resize(100000);

	// Blob 검출
	if(Blob(threshold, imageWidth, imageHeight, roi, FALSE, params.nMinBlobSize, params.nMaxBlobCount, &blobInfoList[0], nBlobCount, pnLabelImage) == FALSE)
	{
		return FALSE;
	}
	
	// Blob Merge
	if(params.bBlobMerge == TRUE && nBlobCount >= 2)
	{
		if(BlobMerge(imageWidth, imageHeight, pnLabelImage, nBlobCount, (float)params.nBlobMergeDist, &blobInfoList[0], nBlobMergeCount) == FALSE)
			return FALSE;

		nBlobCount = nBlobMergeCount;
	}

	blobInfoList.resize(nBlobCount);
	blobCount = nBlobCount;

	/*
	// 2,1,3 순으로 X축 데이터 정렬   24.1211
	//if (nBlobCount == 3)
	//{
	//	// 1. m_roi.left 기준으로 정렬
	//	std::sort(blobInfoList.begin(), blobInfoList.end(), [](const HUBSDK::BlobInfo& a, const HUBSDK::BlobInfo& b) {
	//		return a.m_roi.left < b.m_roi.left;
	//	});

	//	// 2. 두 번째로 큰 m_roi.left 값을 가진 BlobInfo를 찾음
	//	if (blobInfoList.size() >= 2) { // 데이터가 최소 2개 이상인지 확인
	//		size_t secondLargestIndex = blobInfoList.size() - 2;

	//		// 3. 두 번째로 큰 값을 맨 앞으로 이동
	//		HUBSDK::BlobInfo secondLargestBlob = blobInfoList[secondLargestIndex];

	//		// erase 및 insert 작업 (순서를 주의)
	//		blobInfoList.erase(blobInfoList.begin() + secondLargestIndex);
	//		blobInfoList.insert(blobInfoList.begin(), secondLargestBlob);
	//	}
	//}
	*/
	if(nBlobCount != 0)
	{
		// Blob 추가 정보
		double bgAverageValue(0.f); // Contrast 계산시 필요
		if(HUBIP::CalcBlobInfo(src, (BYTE)bgAverageValue, pnLabelImage, imageWidth, imageHeight, nBlobCount, &blobInfoList[0]) != retStatus::success)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CAlgorithm::GetGrayToColorImage(BYTE* src, const long imageWidth, const long imageHeight, const long bpp, const CRect roi, CImage* dst)
{
	BYTE* pbyColorImage = new BYTE[imageWidth * imageHeight * 3];
	memset(pbyColorImage, 0, imageWidth * imageHeight * 3);

	// Gray -> Color
	if(HUBIP::CombineRGB(src, src, src, imageWidth, imageHeight, CRect(0, 0, imageWidth, imageHeight), pbyColorImage) != retStatus::success)
	{
		return FALSE;
	}

	if(!HUBSDK::ImageInterface::ByteArrayToCImage(pbyColorImage, imageWidth, imageHeight, 3, dst))
	{
		return FALSE;
	}

	delete[] pbyColorImage;

	return TRUE;
}

void CAlgorithm::CalcEdgeDetect(BYTE* image, long imageWidth, long imageHeight, CPoint ptStart, CPoint ptEnd, long edgeDir, BOOL firstEdge, std::vector<CPoint> &vecptEdge)
{
	float fEdgeX = 0.f;
	float fEdgeY = 0.f;
	float fEdgeValue = 0.f;

	if(m_pEdgeDetect->CalcEdgePointFromImage(edgeDir, ptStart, ptEnd, image, imageWidth, imageHeight, fEdgeX, fEdgeY, fEdgeValue, firstEdge))
	{
		vecptEdge.push_back(CPoint((long)fEdgeX, (long)fEdgeY));
	}
}

void CAlgorithm::CalcEdgeFiltering(std::vector<CPoint> &vecptEdge, const long edgeDir, const long edgeFilteringRange, const BOOL bErase)
{
	if(edgeFilteringRange == 0)
	{
		return;
	}

	float fAvg = 0.f;

	switch(edgeDir)
	{
	case (long)HUBSDK::DIRECTION::LEFT:
	case (long)HUBSDK::DIRECTION::RIGHT:
	{
		for(long idx = 0; idx < (long)vecptEdge.size(); idx++)
		{
			fAvg += vecptEdge[idx].x;
		}
		fAvg /= (long)vecptEdge.size();
	}

	for(long idx = 0; idx < (long)vecptEdge.size(); idx++)
	{
		if(fAvg - edgeFilteringRange > vecptEdge[idx].x || fAvg + edgeFilteringRange < vecptEdge[idx].x)
		{
			if(bErase)
			{
				if((long)vecptEdge.size() > 2)
				{
					vecptEdge.erase(vecptEdge.begin() + idx);
					idx--;
				}
			}
			else
			{
				vecptEdge[idx].x = (long)fAvg;
			}
		}
	}

	break;
	case (long)HUBSDK::DIRECTION::TOP:
	case (long)HUBSDK::DIRECTION::BOTTOM:
	{
		for(long idx = 0; idx < (long)vecptEdge.size(); idx++)
		{
			fAvg += vecptEdge[idx].y;
		}
		fAvg /= (long)vecptEdge.size();

		for(long idx = 0; idx < (long)vecptEdge.size(); idx++)
		{
			if(fAvg - edgeFilteringRange > vecptEdge[idx].y || fAvg + edgeFilteringRange < vecptEdge[idx].y)
			{
				if(bErase)
				{
					if((long)vecptEdge.size() > 2)
					{
						vecptEdge.erase(vecptEdge.begin() + idx);
						idx--;
					}
				}
				else
				{
					vecptEdge[idx].y = (long)fAvg;
				}
			}
		}
	}
	break;
	}
}

float CAlgorithm::GetEdgeThreshold()
{
	return m_pEdgeDetect->GetMininumThreshold();
}

void CAlgorithm::SetEdgeThreshold(const float threshold)
{
	m_pEdgeDetect->SetMininumThreshold(threshold);
}

BOOL CAlgorithm::DoProcImage(BYTE * src, long imageWidth, long imageHeight, SProc procParams, BYTE * dst)
{
	if((long)procParams.m_vecnProc.size() > 0 && (long)procParams.m_vecnProcParams.size() > 0)
	{
		HUBIP::ImageProcessingManage(src, imageWidth, imageHeight, CRect(0, 0, imageWidth, imageHeight), &procParams.m_vecnProc[0], &procParams.m_vecnProcParams[0], (long)procParams.m_vecnProc.size(), dst);
	}
	else
	{
		memcpy(dst, src, imageWidth * imageHeight);
	}

	return TRUE;
}

BOOL CAlgorithm::DoThresholdImage(BYTE * src, long imageWidth, long imageHeight, SROIParams roiParams, SThresholdParams thresholdParams, BYTE * dst, int nReInspection)
{
	CRect roi = CRect(0, 0, 0, 0);
	BYTE* pbyTemp = nullptr;
	float fVignetteStrength = thresholdParams.fVignetteStrength;
	// 필요한 경우에만 pbyTemp 메모리 할당
	if (thresholdParams.nSelectThreshold == (long)THRESHOLD::HYSTERISIS)
	{
		pbyTemp = new BYTE[imageWidth * imageHeight];
		memset(pbyTemp, 0, imageWidth * imageHeight);
	}

	// 250327 PSJ 적응형 변수 초기화
	m_AveragePixelValue = 0; m_SumPixelCount = 0; m_SumPixelValue = 0;
	m_test = 0;
	m_dir = 0;

	// NORMAL Threshold 처리
	if(thresholdParams.nSelectThreshold == (long)THRESHOLD::NORMAL)
	{
		if(roiParams.bUseROI == TRUE)
		{
			// ROI가 활성화된 경우
			for(long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];

				// 250327 적응형 Threshold ROI 설정 PSJ
				m_adaptiveVariable1.x = roi.left;
				m_adaptiveVariable1.y = roi.bottom;
				m_adaptiveVariable2.x = roi.right;
				m_adaptiveVariable2.y = roi.bottom;
				m_adaptiveROI = { // 하단부 엣지 기준으로 상부 버스바 강제상수 ROI로 탐색(납작한 작은 사각형)
								m_adaptiveVariable1.x + 100,
								m_adaptiveVariable1.y - 500,
								m_adaptiveVariable2.x - 100,
								m_adaptiveVariable2.y - 450
				};

				for (int y = roi.top; y < roi.bottom; ++y)
				{
					for (int x = roi.left; x < roi.right; ++x)
					{
						int index = y * imageWidth + x;
						POINT tmp = { x, y };

						// TODO: zero divide issue 처리해야함.
						// ROI 내 평균 픽셀값 연산 정보 세팅 250327 PSJ 
						if (m_adaptiveROI.PtInRect(tmp) /*&& src[index] > thresholdParams.nThresholdValue*/)
						{
							m_SumPixelValue += src[index]; // ROI area 내 pixel 값 sum
							m_SumPixelCount++;
						}

						// 250328  ROI 마지막 지점에서 Average PixelValue 찾기
						if (x == m_adaptiveROI.right && y == m_adaptiveROI.bottom)
						{
							m_AveragePixelValue = m_SumPixelValue / m_SumPixelCount; // src[index]에 값이 pixel값으로 잘 들어가는지 확인 필요
							//m_test = GetReInspectionDirection(thresholdParams.nThresholdValue); // + , - 방향 설정
							//m_dir = GetReInspectionDirection(thresholdParams.nThresholdValue); // + , - 방향 설정
							m_dir = 1; // 250401 현재는 강제로 + 설정
							Sleep(1);

							
						}
					}
				}
				// 이거 여기서 하면 안됨.
				// thresholdParams.nThresholdValue = m_AveragePixelValue; // Threshold 변수 결정

				if (0)
				{
					// Two-dimensional threshold gradient:
					// ROI center = no offset. Left/right and top/bottom edges each apply
					// the full signed offset, so a corner applies twice the offset.
					const int roiWidth = roi.Width();
					const int centerLeft = roi.left + (roiWidth - 1) / 2;
					const int centerRight = roi.left + roiWidth / 2;
					const int maxHorizontalDistance = max(centerLeft - roi.left,
						(roi.right - 1) - centerRight);
					const int roiHeight = roi.Height();
					const int centerTop = roi.top + (roiHeight - 1) / 2;
					const int centerBottom = roi.top + roiHeight / 2;
					const int maxVerticalDistance = max(centerTop - roi.top,
						(roi.bottom - 1) - centerBottom);

					for (int y2 = roi.top; y2 < roi.bottom; ++y2)
					{
						for (int x2 = roi.left; x2 < roi.right; ++x2)
						{
							int index = y2 * imageWidth + x2;
							int distanceFromCenter = 0;
							if(x2 < centerLeft)
								distanceFromCenter = centerLeft - x2;
							else if(x2 > centerRight)
								distanceFromCenter = x2 - centerRight;
						 
							const float horizontalRatio = maxHorizontalDistance > 0
								? (float)distanceFromCenter / (float)maxHorizontalDistance
								: 0.0f;
							const float horizontalOffset = thresholdParams.nThresholdOffset * horizontalRatio;

							int verticalDistanceFromCenter = 0;
							if(y2 < centerTop)
								verticalDistanceFromCenter = centerTop - y2;
							else if(y2 > centerBottom)
								verticalDistanceFromCenter = y2 - centerBottom;

							const float verticalRatio = maxVerticalDistance > 0
								? (float)verticalDistanceFromCenter / (float)maxVerticalDistance
								: 0.0f;
							const float verticalOffset = thresholdParams.nThresholdOffset * verticalRatio;

							float effectiveThreshold = thresholdParams.nThresholdValue
								+ horizontalOffset + verticalOffset
								+ (nReInspection * thresholdParams.nReInspectionOffset * m_dir);
							//effectiveThreshold = min(max(effectiveThreshold, 0.0f), 255.0f);
							// 절대로 음수가 되지 않도록 보정
							effectiveThreshold = max(effectiveThreshold, 0.0f); 
							effectiveThreshold = min(effectiveThreshold, 255.0f);
						
							const BOOL isForeground = thresholdParams.bThresholdReverse == TRUE
								? src[index] <= effectiveThreshold
								: src[index] > effectiveThreshold;
							dst[index] = isForeground ? 255 : 0;
						}
					}
				}
				else
				{
					// Two-dimensional threshold gradient (Radial)

					const int roiWidth = roi.Width();
					const int centerLeft = roi.left + (roiWidth - 1) / 2;
					const int centerRight = roi.left + roiWidth / 2;
					const int maxHorizontalDistance = max(centerLeft - roi.left,
						(roi.right - 1) - centerRight);

					const int roiHeight = roi.Height();
					const int centerTop = roi.top + (roiHeight - 1) / 2;
					const int centerBottom = roi.top + roiHeight / 2;
					const int maxVerticalDistance = max(centerTop - roi.top,
						(roi.bottom - 1) - centerBottom);

					for (int y2 = roi.top; y2 < roi.bottom; ++y2)
					{
						for (int x2 = roi.left; x2 < roi.right; ++x2)
						{
							const int index = y2 * imageWidth + x2;

							// 가로 거리
							int horizontalDistance = 0;
							if (x2 < centerLeft)
								horizontalDistance = centerLeft - x2;
							else if (x2 > centerRight)
								horizontalDistance = x2 - centerRight;

							// 세로 거리
							int verticalDistance = 0;
							if (y2 < centerTop)
								verticalDistance = centerTop - y2;
							else if (y2 > centerBottom)
								verticalDistance = y2 - centerBottom;

							// 정규화
							float dx = (maxHorizontalDistance > 0)
								? (float)horizontalDistance / (float)maxHorizontalDistance
								: 0.0f;

							float dy = (maxVerticalDistance > 0)
								? (float)verticalDistance / (float)maxVerticalDistance
								: 0.0f;

							// 중심으로부터의 반경(0~1)
							float ratio = sqrtf(dx * dx + dy * dy);
							ratio = min(ratio, 1.0f);

							float effectiveThreshold =
								thresholdParams.nThresholdValue +
								thresholdParams.nThresholdOffset * ratio +
								(nReInspection * thresholdParams.nReInspectionOffset * m_dir);

							effectiveThreshold = max(0.0f, min(effectiveThreshold, 255.0f));

							const BOOL isForeground =
								thresholdParams.bThresholdReverse == TRUE
								? src[index] <= effectiveThreshold
								: src[index] > effectiveThreshold;

							dst[index] = isForeground ? 255 : 0;
						}
					}
				}
			}
		}
		else
		{
			// ROI가 비활성화된 경우 전체 이미지에 대해 처리
			roi = CRect(0, 0, imageWidth, imageHeight);

			for (int y = roi.top; y < roi.bottom; ++y)
			{
				for (int x = roi.left; x < roi.right; ++x)
				{
					int index = y * imageWidth + x;
					dst[index] = (src[index] > thresholdParams.nThresholdValue) ? 255 : 0;
				}
			}
		}
	}
	// ADAPTIVE Threshold 처리
	else if(thresholdParams.nSelectThreshold == (long)THRESHOLD::ADAPTIVE)
	{
		if(roiParams.bUseROI == TRUE)
		{
			// ROI가 활성화된 경우
			for(long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];

				// Adaptive Threshold 처리
				HUBIP::BinarizeAdaptive(src, imageWidth, imageHeight, roi, CSize(100, 100), thresholdParams.nAdaptiveOffset, dst);

				// Reverse 옵션이 활성화된 경우 반전 처리
				if(thresholdParams.bThresholdReverse == TRUE)
				{
					HUBIP::BitwiseNot(imageWidth, imageHeight, roi, dst);
				}
			}
		}
		else
		{
			// ROI가 비활성화된 경우 전체 이미지에 대해 처리
			roi = CRect(0, 0, imageWidth, imageHeight);

			HUBIP::BinarizeAdaptive(src, imageWidth, imageHeight, roi, CSize(100, 100), thresholdParams.nAdaptiveOffset, dst);

			if(thresholdParams.bThresholdReverse == TRUE)
			{
				HUBIP::BitwiseNot(imageWidth, imageHeight, roi, dst);
			}
		}
	}
	// HYSTERESIS Threshold 처리
	else if(thresholdParams.nSelectThreshold == (long)THRESHOLD::HYSTERISIS)
	{
		if(roiParams.bUseROI == TRUE)
		{
			// ROI가 활성화된 경우
			HUBIP::BinarizeHysteresis(src, imageWidth, imageHeight, CRect(0, 0, imageWidth, imageHeight),
				(BYTE)thresholdParams.nHysteresisLow, (BYTE)thresholdParams.nHysteresisHigh,
				false, (bool)thresholdParams.bThresholdReverse,
				thresholdParams.nHighThresholdRegionNoiseFilteringSize,
				imageWidth, imageHeight, pbyTemp, dst);

			memset(pbyTemp, 0, imageWidth * imageHeight);

			for(long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];

				// Hysteresis Threshold 적용
				HUBIP::BinarizationHysteresisForSetupImage(src, imageWidth, imageHeight, roi,
					(BYTE)thresholdParams.nHysteresisLow,
					(BYTE)thresholdParams.nHysteresisHigh, dst);

				HUBIP::BitwiseOr(dst, imageWidth, imageHeight, roi, pbyTemp);
			}

			memcpy(dst, pbyTemp, imageWidth * imageHeight);
		}
		else
		{
			// ROI가 비활성화된 경우 전체 이미지에 대해 처리
			HUBIP::BinarizeHysteresis(src, imageWidth, imageHeight, CRect(0, 0, imageWidth, imageHeight),
				(BYTE)thresholdParams.nHysteresisLow, (BYTE)thresholdParams.nHysteresisHigh,
				false, (bool)thresholdParams.bThresholdReverse,
				thresholdParams.nHighThresholdRegionNoiseFilteringSize,
				imageWidth, imageHeight, pbyTemp, dst);

			HUBIP::BinarizationHysteresisForSetupImage(src, imageWidth, imageHeight,
				(BYTE)thresholdParams.nHysteresisLow,
				(BYTE)thresholdParams.nHysteresisHigh, dst);
		}
	}
	// OTSU Threshold 처리 -> 메모리 관리는 아직 못했음..
	else if (thresholdParams.nSelectThreshold == (long)THRESHOLD::OTSU)
	{
		long* histogram = nullptr;

		if (roiParams.bUseROI == TRUE)
		{
			for (long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];

				// 히스토그램 생성
				histogram = nullptr;
				HUBIP::MakeHistogram(src, imageWidth, imageHeight, roi, false, histogram);

				// 오츠의 방법으로 임계값 계산
				BYTE otsuThreshold = CalculateOtsuThreshold(histogram);

				// 계산된 임계값으로 이진화
				HUBIP::Binarization(src, imageWidth, imageHeight, roi, otsuThreshold, thresholdParams.bThresholdReverse, dst);
			}
		}
		else
		{
			roi = CRect(0, 0, imageWidth, imageHeight);

			// 히스토그램 생성
			histogram = nullptr;
			HUBIP::MakeHistogram(src, imageWidth, imageHeight, roi, false, histogram);

			// 오츠의 방법으로 임계값 계산
			BYTE otsuThreshold = CalculateOtsuThreshold(histogram);

			// 계산된 임계값으로 이진화
			HUBIP::Binarization(src, imageWidth, imageHeight, roi, otsuThreshold, thresholdParams.bThresholdReverse, dst);
		}
	}

	// TRIANGLE Threshold 처리
	else if (thresholdParams.nSelectThreshold == (long)THRESHOLD::TRIANGLE)
	{
		long* histogram = nullptr;

		if (roiParams.bUseROI == TRUE)
		{
			for (long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];

				// 히스토그램 생성
				HUBIP::MakeHistogram(src, imageWidth, imageHeight, roi, false, histogram);

				// 삼각형 방법으로 임계값 계산
				BYTE triangleThreshold = CalculateTriangleThreshold(histogram);

				// 계산된 임계값으로 이진화
				HUBIP::Binarization(src, imageWidth, imageHeight, roi, triangleThreshold, thresholdParams.bThresholdReverse, dst);
			}
		}
		else
		{
			roi = CRect(0, 0, imageWidth, imageHeight);

			// 히스토그램 생성
			HUBIP::MakeHistogram(src, imageWidth, imageHeight, roi, false, histogram);

			// 삼각형 방법으로 임계값 계산
			BYTE triangleThreshold = CalculateTriangleThreshold(histogram);

			// 계산된 임계값으로 이진화
			HUBIP::Binarization(src, imageWidth, imageHeight, roi, triangleThreshold, thresholdParams.bThresholdReverse, dst);
		}
	}
	// VIGNETTE 효과 처리
	else if (thresholdParams.nSelectThreshold == (long)THRESHOLD::VIGNETTE)
	{
		if (roiParams.bUseROI == TRUE)
		{
			for (long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];

				// 비네팅 효과 적용
				ApplyVignetteEffect(src, imageWidth, imageHeight, roi, dst, fVignetteStrength);
			}
		}
		else
		{
			roi = CRect(0, 0, imageWidth, imageHeight);

			// 비네팅 효과 적용
			ApplyVignetteEffect(src, imageWidth, imageHeight, roi, dst, fVignetteStrength);
		}
	}
	else
	{
		// 선택된 임계값 방법이 없는 경우 원본 이미지를 그대로 복사
		memcpy(dst, src, imageWidth * imageHeight);
	}

	// 메모리 해제
	if (pbyTemp != nullptr)
	{
		delete[] pbyTemp;
		pbyTemp = nullptr;
	}

	return TRUE;
}

BOOL CAlgorithm::DoThresholdImage2(BYTE * src, long imageWidth, long imageHeight, SROIParams roiParams, SThresholdParams thresholdParams, BYTE * dst)
{
	BYTE* pbyTemp = new BYTE[imageWidth * imageHeight];
	CRect roi = CRect(0, 0, 0, 0);

	memset(pbyTemp, 0, imageWidth * imageHeight);

	int nThreshMargin = 5;

	if (thresholdParams.nSelectThreshold == (long)THRESHOLD::NORMAL)
	{
		if (roiParams.bUseROI == TRUE)
		{
			for (long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];
				HUBIP::Binarization(src, imageWidth, imageHeight, roi, (BYTE)thresholdParams.nThresholdValue+ nThreshMargin, (bool)thresholdParams.bThresholdReverse, dst);
			}
		}
		else
		{
			roi = CRect(0, 0, imageWidth, imageHeight);
			HUBIP::Binarization(src, imageWidth, imageHeight, roi, (BYTE)thresholdParams.nThresholdValue+ nThreshMargin, (bool)thresholdParams.bThresholdReverse, dst);
		}
	}
	else if (thresholdParams.nSelectThreshold == (long)THRESHOLD::ADAPTIVE)
	{
		if (roiParams.bUseROI == TRUE)
		{
			for (long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];
				HUBIP::BinarizeAdaptive(src, imageWidth, imageHeight, roi, CSize(100, 100), thresholdParams.nAdaptiveOffset, dst);

				if (thresholdParams.bThresholdReverse == TRUE)
				{
					HUBIP::BitwiseNot(imageWidth, imageHeight, roi, dst);
				}
			}
		}
		else
		{
			roi = CRect(0, 0, imageWidth, imageHeight);
			HUBIP::BinarizeAdaptive(src, imageWidth, imageHeight, roi, CSize(100, 100), thresholdParams.nAdaptiveOffset, dst);

			if (thresholdParams.bThresholdReverse == TRUE)
			{
				HUBIP::BitwiseNot(imageWidth, imageHeight, roi, dst);
			}
		}
	}
	else if (thresholdParams.nSelectThreshold == (long)THRESHOLD::HYSTERISIS)
	{
		if (roiParams.bUseROI == TRUE)
		{
			HUBIP::BinarizeHysteresis(src, imageWidth, imageHeight, CRect(0, 0, imageWidth, imageHeight), (BYTE)thresholdParams.nHysteresisLow, (BYTE)thresholdParams.nHighThresholdRegionNoiseFilteringSize, false, (bool)thresholdParams.bThresholdReverse, thresholdParams.nHighThresholdRegionNoiseFilteringSize, imageWidth, imageHeight, pbyTemp, dst);

			memset(pbyTemp, 0, imageWidth * imageHeight);

			for (long idx = 0; idx < roiParams.nSearchROINum; idx++)
			{
				roi = roiParams.vecrtSearchROI[idx];
				HUBIP::BinarizationHysteresisForSetupImage(src, imageWidth, imageHeight, roi, (BYTE)thresholdParams.nHysteresisLow, (BYTE)thresholdParams.nHysteresisHigh, dst);

				HUBIP::BitwiseOr(dst, imageWidth, imageHeight, roi, pbyTemp);
			}

			memcpy(dst, pbyTemp, imageWidth * imageHeight);
		}
		else
		{
			HUBIP::BinarizeHysteresis(src, imageWidth, imageHeight, CRect(0, 0, imageWidth, imageHeight), (BYTE)thresholdParams.nHysteresisLow, (BYTE)thresholdParams.nHighThresholdRegionNoiseFilteringSize, false, (bool)thresholdParams.bThresholdReverse, thresholdParams.nHighThresholdRegionNoiseFilteringSize, imageWidth, imageHeight, pbyTemp, dst);

			HUBIP::BinarizationHysteresisForSetupImage(src, imageWidth, imageHeight, (BYTE)thresholdParams.nHysteresisLow, (BYTE)thresholdParams.nHysteresisHigh, dst);
		}
	}

	delete pbyTemp;

	return TRUE;
}

BOOL CAlgorithm::CalcEdgeDetect(BYTE * image, long imageWidth, long imageHeight, std::vector<SEdgeDetectParam> vecsSearchROI, long direction, std::vector<CRect> vecrtRoi, std::vector<long> paramIndex, std::vector<CRect> vecrtIgnore, std::vector<CPoint>& vecptEdge)
{
	// Search ROI...
	for(long idx = 0; idx < (long)vecrtRoi.size(); idx++)
	{
		auto roi = vecrtRoi[idx];
		auto searchDirection = vecsSearchROI[paramIndex[idx]].nSearchDirection;
		auto edgeDirection = vecsSearchROI[paramIndex[idx]].nEdgeDirection;
		auto firstEdge = vecsSearchROI[paramIndex[idx]].bFirstEdge;
		auto sampleRate = vecsSearchROI[paramIndex[idx]].nSampleRate;
		auto edgeThreshold = vecsSearchROI[paramIndex[idx]].fEdgeThreshold;
		auto useEdgeFiltering = vecsSearchROI[paramIndex[idx]].bUseEdgeFiltering;
		auto edgeFilteringRange = vecsSearchROI[paramIndex[idx]].nEdgeFilteringRange;
		auto ngEdgeErase = vecsSearchROI[paramIndex[idx]].bNGEdgeErase;

		std::vector<CPoint> vecptStart;
		std::vector<CPoint> vecptEnd;

		if(searchDirection == (long)HUBSDK::DIRECTION::LEFT) // RIght -> Left
		{
			if(direction == (long)HUBSDK::DIRECTION::LEFT || direction == (long)HUBSDK::DIRECTION::RIGHT)
			{
				for(long y = roi.top; y < roi.bottom - sampleRate; y += sampleRate)
				{
					vecptStart.push_back(CPoint(roi.right - 1, y));
					vecptEnd.push_back(CPoint(roi.left, y));
				}
			}
		}
		else if(searchDirection == (long)HUBSDK::DIRECTION::UP) // Bottom -> Top
		{
			if(direction == (long)HUBSDK::DIRECTION::UP || direction == (long)HUBSDK::DIRECTION::DOWN)
			{
				for(long x = roi.left; x < roi.right - sampleRate; x += sampleRate)
				{
					vecptStart.push_back(CPoint(x, roi.bottom - 1));
					vecptEnd.push_back(CPoint(x, roi.top));
				}
			}
		}
		else if(searchDirection == (long)HUBSDK::DIRECTION::RIGHT) // Left -> Right
		{
			if(direction == (long)HUBSDK::DIRECTION::LEFT || direction == (long)HUBSDK::DIRECTION::RIGHT)
			{
				for(long y = roi.top; y < roi.bottom - sampleRate; y += sampleRate)
				{
					vecptStart.push_back(CPoint(roi.left, y));
					vecptEnd.push_back(CPoint(roi.right - 1, y));
				}
			}
		}
		else if(searchDirection == (long)HUBSDK::DIRECTION::DOWN) // Top -> Bottom
		{
			if(direction == (long)HUBSDK::DIRECTION::UP || direction == (long)HUBSDK::DIRECTION::DOWN)
			{
				for(long x = roi.left; x < roi.right - sampleRate; x += sampleRate)
				{
					vecptStart.push_back(CPoint(x, roi.top));
					vecptEnd.push_back(CPoint(x, roi.bottom - 1));
				}
			}
		}
		else // Edge Detect 실시하지 않는다.
		{
			;;
		}

		auto oldEdgeThreshold = GetEdgeThreshold();
		SetEdgeThreshold(edgeThreshold);

		for(long pt = 0; pt < (long)vecptStart.size(); pt++)
		{
			CalcEdgeDetect(image, imageWidth, imageHeight, vecptStart[pt], vecptEnd[pt], edgeDirection, firstEdge, vecptEdge);
		}

		SetEdgeThreshold(oldEdgeThreshold);

		// Ignore ROI... (ROI에 포함된 Edge는 지운다.)
		for(long idx = 0; idx < (long)vecrtIgnore.size(); idx++)
		{
			auto exist = vecrtIgnore[idx] & roi;
			if(exist.Width() == 0 || exist.Height() == 0)
			{
				continue;
			}

			for(long edge = 0; edge < (long)vecptEdge.size(); edge++)
			{
				if(vecrtIgnore[idx].PtInRect(vecptEdge[edge]) == TRUE)
				{
					vecptEdge.erase(vecptEdge.begin() + edge);

					edge--;
				}
			}
		}

		// Edge Filtering (Edge 평균+offset 벗어나는 Edge들을 처리)
		if(useEdgeFiltering == TRUE)
		{
			CalcEdgeFiltering(vecptEdge, direction, edgeFilteringRange, ngEdgeErase);
		}
	}

	if((long)vecptEdge.size() <= 0)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CAlgorithm::GetPinAngle(const CPoint ptIntersection[(long)HUBSDK::RECT_CORNER::END], const CPoint ptStart[(long)HUBSDK::DIRECTION::END], const CPoint ptEnd[(long)HUBSDK::DIRECTION::END], CPoint &ptResultStart, CPoint &ptResultEnd, float &fResultAngle, BOOL &bVertical)
{
	if (ptIntersection[(long)HUBSDK::RECT_CORNER::LT] != CPoint(0,0) && ptIntersection[(long)HUBSDK::RECT_CORNER::LB] != CPoint(0, 0))
	{
		bVertical = TRUE;

		ptResultStart = ptIntersection[(long)HUBSDK::RECT_CORNER::LT];
		ptResultEnd = ptIntersection[(long)HUBSDK::RECT_CORNER::LB];
	}
	else if (ptIntersection[(long)HUBSDK::RECT_CORNER::RT] != CPoint(0, 0) && ptIntersection[(long)HUBSDK::RECT_CORNER::RB] != CPoint(0, 0))
	{
		bVertical = TRUE;

		ptResultStart = ptIntersection[(long)HUBSDK::RECT_CORNER::RT];
		ptResultEnd = ptIntersection[(long)HUBSDK::RECT_CORNER::RB];
	}
	else if(ptIntersection[(long)HUBSDK::RECT_CORNER::LT] != CPoint(0, 0) && ptIntersection[(long)HUBSDK::RECT_CORNER::RT] != CPoint(0, 0))
	{
		ptResultStart = ptIntersection[(long)HUBSDK::RECT_CORNER::LT];
		ptResultEnd = ptIntersection[(long)HUBSDK::RECT_CORNER::RT];
	}
	else
	{
		ptResultStart = ptIntersection[(long)HUBSDK::RECT_CORNER::LB];
		ptResultEnd = ptIntersection[(long)HUBSDK::RECT_CORNER::RB];
	}
	

	long topIdx = (long)HUBSDK::DIRECTION::TOP;
	long bottomIdx = (long)HUBSDK::DIRECTION::BOTTOM;
	long leftIdx = (long)HUBSDK::DIRECTION::LEFT;
	long rightIdx = (long)HUBSDK::DIRECTION::RIGHT;

	CPoint ptAngleStart(0, 0), ptAngleEnd(0, 0);
	long angleStart(0), angleEnd(0);

	// 두 변 사이의 중심 선 구하기
	// 나중에 찾은 점들로 선 구하는거로 변경 필요
	if(bVertical == TRUE)
	{
		// 핀이 가로로 있는 경우
		angleStart = (long)(sqrt(pow(ptStart[bottomIdx].x - ptStart[topIdx].x, 2) + pow(ptStart[bottomIdx].y - ptStart[topIdx].y, 2)) / 2.f);
		angleEnd = (long)(sqrt(pow(ptEnd[bottomIdx].x - ptEnd[topIdx].x, 2) + pow(ptEnd[bottomIdx].y - ptEnd[topIdx].y, 2)) / 2.f);

		ptAngleStart.x = ptStart[topIdx].x;
		ptAngleStart.y = ptStart[topIdx].y + angleStart;
		ptAngleEnd.x = ptEnd[topIdx].x;
		ptAngleEnd.y = ptEnd[topIdx].y + angleEnd;

		HUBSDK::Geometry::GetAngleBetween2Lines(ptAngleStart, ptAngleEnd, CPoint(ptAngleStart.x, ptAngleEnd.y), fResultAngle);
	}
	else
	{
		// 핀이 세로로 있는 경우
		angleStart = (long)(sqrt(pow(ptStart[rightIdx].x - ptStart[leftIdx].x, 2) + pow(ptStart[rightIdx].y - ptStart[leftIdx].y, 2)) / 2.f);
		angleEnd = (long)(sqrt(pow(ptEnd[rightIdx].x - ptEnd[leftIdx].x, 2) + pow(ptEnd[rightIdx].y - ptEnd[leftIdx].y, 2)) / 2.f);

		ptAngleStart.x = ptStart[leftIdx].x + angleStart;
		ptAngleStart.y = ptStart[leftIdx].y;
		ptAngleEnd.x = ptEnd[leftIdx].x + angleEnd;
		ptAngleEnd.y = ptEnd[leftIdx].y;

		HUBSDK::Geometry::GetAngleBetween2Lines(ptAngleStart, ptAngleEnd, CPoint(ptAngleEnd.x, ptAngleStart.y), fResultAngle);
	}

	if(fResultAngle < 0.f)
	{
		fResultAngle += 360.f;
	}

	return TRUE;
}

BOOL CAlgorithm::DoLineFitting(std::vector<CPoint> vecptEdge[(long)HUBSDK::DIRECTION::END], CPoint ptStart[(long)HUBSDK::DIRECTION::END], CPoint ptEnd[(long)HUBSDK::DIRECTION::END], float fA[(long)HUBSDK::DIRECTION::END], float fB[(long)HUBSDK::DIRECTION::END], float fC[(long)HUBSDK::DIRECTION::END])
{
	for(long dir = 0; dir < (long)HUBSDK::DIRECTION::END; dir++)
	{
		ptStart[dir] = CPoint(0, 0);
		ptEnd[dir] = CPoint(0, 0);

		if((long)vecptEdge[dir].size() > 0)
		{
			float *ix = new float[vecptEdge[dir].size()];
			float *iy = new float[vecptEdge[dir].size()];

			for(long idx = 0; idx < vecptEdge[dir].size(); idx++)
			{
				ix[idx] = (float)vecptEdge[dir][idx].x;
				iy[idx] = (float)vecptEdge[dir][idx].y;
			}

			if(HUBSDK::Geometry::LineFitting_RemoveNoise((long)vecptEdge[dir].size() - 1, ix, iy, fA[dir], fB[dir], fC[dir]) != retStatus::success)
			{
				delete ix;
				delete iy;
				return FALSE;
			}

			if(dir == (long)HUBSDK::DIRECTION::TOP || dir == (long)HUBSDK::DIRECTION::BOTTOM)
			{
				ptStart[dir] = CPoint((long)ix[0], (long)(-fA[dir] / fB[dir] * ix[0] + -fC[dir] / fB[dir]));
				ptEnd[dir] = CPoint((long)ix[vecptEdge[dir].size() - 1], (long)(-fA[dir] / fB[dir] * ix[vecptEdge[dir].size() - 1] + -fC[dir] / fB[dir]));
			}
			else
			{
				ptStart[dir] = CPoint((long)(-fB[dir] / fA[dir] * iy[0] + -fC[dir] / fA[dir]), (long)iy[0]);
				ptEnd[dir] = CPoint((long)(-fB[dir] / fA[dir] * iy[vecptEdge[dir].size() - 1] + -fC[dir] / fA[dir]), (long)iy[vecptEdge[dir].size() - 1]);
			}

			delete ix;
			delete iy;
		}
	}

	return TRUE;
}

BOOL CAlgorithm::DoCalcIntersection(long nImageWidth, long nImageHeight, float fA[(long)HUBSDK::DIRECTION::END], float fB[(long)HUBSDK::DIRECTION::END], float fC[(long)HUBSDK::DIRECTION::END], CPoint* ptIntersection)
{
	// 교점 구하기
	HUBSDK::Geometry::GetCrossPointByTwoStraightLines(
		fA[(long)HUBSDK::DIRECTION::LEFT], fB[(long)HUBSDK::DIRECTION::LEFT], fC[(long)HUBSDK::DIRECTION::LEFT],
		fA[(long)HUBSDK::DIRECTION::TOP], fB[(long)HUBSDK::DIRECTION::TOP], fC[(long)HUBSDK::DIRECTION::TOP],
		ptIntersection[(long)HUBSDK::RECT_CORNER::LT].x, ptIntersection[(long)HUBSDK::RECT_CORNER::LT].y);

	HUBSDK::Geometry::GetCrossPointByTwoStraightLines(
		fA[(long)HUBSDK::DIRECTION::TOP], fB[(long)HUBSDK::DIRECTION::TOP], fC[(long)HUBSDK::DIRECTION::TOP],
		fA[(long)HUBSDK::DIRECTION::RIGHT], fB[(long)HUBSDK::DIRECTION::RIGHT], fC[(long)HUBSDK::DIRECTION::RIGHT],
		ptIntersection[(long)HUBSDK::RECT_CORNER::RT].x, ptIntersection[(long)HUBSDK::RECT_CORNER::RT].y);

	HUBSDK::Geometry::GetCrossPointByTwoStraightLines(
		fA[(long)HUBSDK::DIRECTION::RIGHT], fB[(long)HUBSDK::DIRECTION::RIGHT], fC[(long)HUBSDK::DIRECTION::RIGHT],
		fA[(long)HUBSDK::DIRECTION::BOTTOM], fB[(long)HUBSDK::DIRECTION::BOTTOM], fC[(long)HUBSDK::DIRECTION::BOTTOM],
		ptIntersection[(long)HUBSDK::RECT_CORNER::RB].x, ptIntersection[(long)HUBSDK::RECT_CORNER::RB].y);

	HUBSDK::Geometry::GetCrossPointByTwoStraightLines(
		fA[(long)HUBSDK::DIRECTION::BOTTOM], fB[(long)HUBSDK::DIRECTION::BOTTOM], fC[(long)HUBSDK::DIRECTION::BOTTOM],
		fA[(long)HUBSDK::DIRECTION::LEFT], fB[(long)HUBSDK::DIRECTION::LEFT], fC[(long)HUBSDK::DIRECTION::LEFT],
		ptIntersection[(long)HUBSDK::RECT_CORNER::LB].x, ptIntersection[(long)HUBSDK::RECT_CORNER::LB].y);

	// 찾은 교점이 영상을 넘어가면 Align 실패
	CRect rtImage = CRect(0, 0, nImageWidth, nImageHeight);
	for(long idx = 0; idx < (long)HUBSDK::RECT_CORNER::END; idx++)
	{
		if(rtImage.PtInRect(ptIntersection[idx]) == FALSE)
		{
			ptIntersection[idx] = CPoint(0, 0);
		}
	}

	return TRUE;
}

// thPark Otsu Threshold 계산!!
BYTE CAlgorithm::CalculateOtsuThreshold(const long* histogram)
{
	long total = 0;
	float sum = 0.0f;

	for (int i = 0; i < 256; ++i)
	{
		total += histogram[i];
		sum += i * histogram[i];
	}

	long sumB = 0;
	long wB = 0;
	long wF = 0;

	float varMax = 0.0f;
	BYTE threshold = 0;

	for (int t = 0; t < 256; ++t)
	{
		wB += histogram[t];
		if (wB == 0)
			continue;

		wF = total - wB;
		if (wF == 0)
			break;

		sumB += t * histogram[t];

		float mB = sumB / (float)wB;
		float mF = (sum - sumB) / (float)wF;

		float varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);

		if (varBetween > varMax)
		{
			varMax = varBetween;
			threshold = t;
		}
	}

	return threshold;
}

// thPark Triangle Threshold 계산!!
BYTE CAlgorithm::CalculateTriangleThreshold(const long* histogram)
{
	int left = 0;
	int right = 255;
	int maxIndex = 0;
	long maxValue = histogram[0];

	// 히스토그램에서 최대값 찾기
	for (int i = 1; i < 256; ++i)
	{
		if (histogram[i] > maxValue)
		{
			maxValue = histogram[i];
			maxIndex = i;
		}
	}

	// 히스토그램의 시작과 끝 결정
	for (int i = 0; i < 256; ++i)
	{
		if (histogram[i] > 0)
		{
			left = i;
			break;
		}
	}

	for (int i = 255; i >= 0; --i)
	{
		if (histogram[i] > 0)
		{
			right = i;
			break;
		}
	}

	bool inverted = false;
	const long* workingHistogram = histogram;

	// 좌우 중 어느 쪽이 더 긴지 결정
	if ((maxIndex - left) < (right - maxIndex))
	{
		inverted = true;

		// 히스토그램 반전
		long* invertedHistogram = new long[256];
		for (int i = 0; i < 256; ++i)
		{
			invertedHistogram[255 - i] = histogram[i];
		}
		workingHistogram = invertedHistogram;

		left = 255 - right;
		maxIndex = 255 - maxIndex;

	}

	// 삼각형의 직선 방정식 계산
	float norm = 0;
	for (int i = left; i <= maxIndex; ++i)
	{
		norm += workingHistogram[i];
	}

	float sum = 0;
	for (int i = left; i <= maxIndex; ++i)
	{
		sum += (maxIndex - i) * workingHistogram[i];
	}

	BYTE threshold = (BYTE)(sum / norm);
	if (inverted)
		threshold = 255 - threshold;

	if (inverted)
	{
		delete[] workingHistogram;
	}

	return threshold;
}

void CAlgorithm::ApplyVignetteEffect(const BYTE* src, long imageWidth, long imageHeight, CRect roi, BYTE* dst, float strength)
{
	if(src == nullptr || dst == nullptr || imageWidth <= 0 || imageHeight <= 0)
		return;

	roi.left = max(roi.left, 0L);
	roi.top = max(roi.top, 0L);
	roi.right = min(roi.right, imageWidth);
	roi.bottom = min(roi.bottom, imageHeight);
	if(roi.Width() <= 0 || roi.Height() <= 0)
		return;

	// 0 means no correction. Larger values monotonically increase edge brightening.
	strength = min(max(strength, 0.0f), 10.0f);
	if(strength <= 0.0f)
	{
		if(src != dst)
		{
			for(long y = roi.top; y < roi.bottom; y++)
			{
				const long offset = y * imageWidth + roi.left;
				memcpy(dst + offset, src + offset, roi.Width());
			}
		}
		return;
	}

	// 이미지 중심 좌표 계산
	float centerX = roi.left + (roi.Width() / 2.0f);
	float centerY = roi.top + (roi.Height() / 2.0f);

	// Build a lookup table from squared radius to correction weight. This avoids
	// calling sqrt/pow for every pixel on a 2448x2048 inspection image.
	const int nLutSize = 4096;
	float weightLut[nLutSize + 1];
	for(int idx = 0; idx <= nLutSize; idx++)
	{
		const float ratioSquared = (float)idx / (float)nLutSize;
		const float ratio = sqrtf(ratioSquared);
		weightLut[idx] = 1.0f - powf(1.0f - ratio, strength);
	}

	const float halfWidth = roi.Width() / 2.0f;
	const float halfHeight = roi.Height() / 2.0f;
	const float maxDistanceSquared = halfWidth * halfWidth + halfHeight * halfHeight;
	const float lutScale = (float)nLutSize / maxDistanceSquared;

	std::vector<float> xDistanceSquared((size_t)roi.Width());
	for(long x = roi.left; x < roi.right; x++)
	{
		const float dx = x - centerX;
		xDistanceSquared[(size_t)(x - roi.left)] = dx * dx;
	}

	for (int y = roi.top; y < roi.bottom; ++y)
	{
		const float dy = y - centerY;
		const float yDistanceSquared = dy * dy;
		for (int x = roi.left; x < roi.right; ++x)
		{
			int index = y * imageWidth + x;
			const float distanceSquared = xDistanceSquared[(size_t)(x - roi.left)] + yDistanceSquared;
			const int lutIndex = min((int)(distanceSquared * lutScale + 0.5f), nLutSize);
			const float weight = weightLut[lutIndex];

			// 픽셀 값 계산 (가장자리로 갈수록 밝아짐)
			float pixelValue = src[index] + (255.0f - src[index]) * weight;

			// 결과를 dst에 저장 (0 ~ 255 범위로 클램핑)
			dst[index] = (BYTE)(min(max(pixelValue, 0.0f), 255.0f));
		}
	}
}
