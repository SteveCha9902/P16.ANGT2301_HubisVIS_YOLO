#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CAlgorithm
{
public:
	CAlgorithm();
	~CAlgorithm();

	EdgeDetect* m_pEdgeDetect;

	// Blob 알고리즘
	BOOL AllocateBlobBuffer();
	BOOL DestroyBlobBuffer();
	BOOL Blob(BYTE* src, const long imageWidth, const long imageHeight, const CRect roi, const BOOL b4Neighbor, const long minBlobSize, const long maxBlobCount, HUBSDK::BlobInfo* blobInfo, long &blobCount, long* labelImage);
	BOOL BlobMerge(const long imageWidth, const long imageHeight, long* label, const long maxBlobCount, const float mergeDistance, HUBSDK::BlobInfo* blobInfo, long &nBlobMergeCount);
	BOOL DoDetectBlob(BYTE* src, BYTE* threshold, const long imageWidth, const long imageHeight, const CRect roi, std::vector<HUBSDK::BlobInfo>& blobInfoList, SBlobParams params, long &blobCount);

	// Edge Detect 알고리즘
	void CalcEdgeDetect(BYTE* image, long imageWidth, long imageHeight, CPoint ptStart, CPoint ptEnd, long edgeDir, BOOL firstEdge, std::vector<CPoint> &vecptEdge);
	void CalcEdgeFiltering(std::vector<CPoint> &vecptEdge, const long edgeDir, const long edgeFilteringRange, const BOOL bErase = TRUE);
	float GetEdgeThreshold();
	void SetEdgeThreshold(const float threshold);
	BOOL CalcEdgeDetect(BYTE* image, long imageWidth, long imageHeight, std::vector<SEdgeDetectParam> vecsSearchROI, long direction, std::vector<CRect> vecrtRoi, std::vector<long> paramIndex, std::vector<CRect> vecrtIgnore, std::vector<CPoint> &vecptEdge);
	BOOL DoLineFitting(std::vector<CPoint> vecptEdge[(long)HUBSDK::DIRECTION::END], CPoint ptStart[(long)HUBSDK::DIRECTION::END], CPoint ptEnd[(long)HUBSDK::DIRECTION::END], float fA[(long)HUBSDK::DIRECTION::END], float fB[(long)HUBSDK::DIRECTION::END], float fC[(long)HUBSDK::DIRECTION::END]);
	BOOL DoCalcIntersection(long nImageWidth, long nImageHeight, float fA[(long)HUBSDK::DIRECTION::END], float fB[(long)HUBSDK::DIRECTION::END], float fC[(long)HUBSDK::DIRECTION::END], CPoint* ptIntersection);
	BOOL GetPinAngle(const CPoint ptIntersection[(long)HUBSDK::RECT_CORNER::END], const CPoint ptStart[(long)HUBSDK::DIRECTION::END], const CPoint ptEnd[(long)HUBSDK::DIRECTION::END], CPoint &ptResultStart, CPoint &ptResultEnd, float &fResultAngle, BOOL &bVertical);
	BOOL GetGrayToColorImage(BYTE* src, const long imageWidth, const long imageHeight, const long bpp, const CRect roi, CImage* dst);

	BOOL DoProcImage(BYTE* src, long imageWidth, long imageHeight, SProc procParams, BYTE* dst);
	BOOL DoThresholdImage(BYTE* src, long imageWidth, long imageHeight, SROIParams roiParams, SThresholdParams thresholdParams, BYTE* dst, int nReInspection);
	BOOL DoThresholdImage2(BYTE* src, long imageWidth, long imageHeight, SROIParams roiParams, SThresholdParams thresholdParams, BYTE* dst);


	BYTE CalculateOtsuThreshold(const long* histogram);
	BYTE CalculateTriangleThreshold(const long* histogram);
	void ApplyVignetteEffect(const BYTE* src, long imageWidth, long imageHeight, CRect roi, BYTE* dst, float strength);
	void *m_bufferHandle;	
	long m_SumPixelValue, m_SumPixelCount, m_AveragePixelValue, m_test, m_dir;
	CPoint m_adaptiveVariable1, m_adaptiveVariable2;
	CRect m_adaptiveROI;
	long GetAveragePixelValue() { return m_AveragePixelValue; }
	long GetReInspectionDirection(int nThresholdValue)
	{
		return (m_AveragePixelValue > nThresholdValue) ? -1 : 1; // Average가 더 크면, 기존 설정값을 같이 높인다 // -1 : 1 조건 탐색중
	}
};

extern ALGORITHM_TOOLS_API CAlgorithm g_Algorithm;
