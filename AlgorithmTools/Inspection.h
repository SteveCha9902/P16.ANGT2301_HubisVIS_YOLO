#pragma once

#define INSP_DATA_COUNT 10

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

// 250619 sjpark YOLO Bound box 정보 구조체 선언
struct YOLOBox {
	CString class_name;
	float confidence;
	float x1, y1, x2, y2;

	// P16: segmentation 모델이면 서버가 박스 뒤에 타원 [cx, cy, w, h, angle]을 보낸다 (픽셀, 각도는 도).
	// ew는 가로축에 가까운 축의 길이, eh는 그 수직 축의 길이, eangle은 ew 축의 기울기(-45 ~ 45).
	// 타원이 없으면(검출 모델·mock) hasEllipse가 FALSE이고 판정은 박스 크기를 쓴다.
	BOOL hasEllipse = FALSE;
	float ecx = 0.f, ecy = 0.f, ew = 0.f, eh = 0.f, eangle = 0.f;

	CRect GetRect() const
	{
		return CRect((int)x1, (int)y1, (int)x2, (int)y2);
	}
};

struct YOLOThreadParam
{
	SOCKET sock;
	CString imagePath;
	CInspection* pThis;
};

class COverlayManager;

class ALGORITHM_TOOLS_API CInspection
{
public:
	CInspection();
	~CInspection();

	CAlgorithm* m_pAlgorithm;
	COverlayManager* m_pOverlayManager;

	// Image 정보 설정 - SHK 2023.12.27
	void SetImageInfo(const long imageWidth, const long imageHeight, const long threadIndex);

	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;
	long m_nThreadIndex;

	void InitParameters();
	BOOL DoInspection(const long visionIndex, const long divIndex, const BOOL bRawImageSave = FALSE, const BOOL bResultDataSave = FALSE, const BOOL bResultImageSave = FALSE);
	BOOL DoSubItemInspection(std::vector<long> vecnSubItemIndex);
	BOOL EachInspection(long itemIndex, BOOL &bResult);
	BOOL DoInspectionBlobAlgo(long itemIndex, BOOL &bResult);
	BOOL DoInspectionBlob2Algo(long itemIndex, BOOL &bResult);
	BOOL DoInspectionEdgeAlgo(long itemIndex, BOOL &bResult);
	void OverlayAlarm(int idx, int alarmCode, SResultParams result);
	void OverlayAlarm(int idx, int alarmCode);
	void OverlayAlarmClear(int alarmCode);
	void DebugOverlay(const long itemIndex, const long debugIndex);
	void DebugOverlayClear(const long debugIndex);
	void DebugOverlayAllClear();

	// ROI 진행 - SHK 2023.12.27
	BOOL DoSetROI(long itemIndex);
	
	// Threshold 진행 - SHK 2023.12.27
	BOOL DoThresholdImage(long itemIndex);
	BOOL DoThresholdImage2(long itemIndex);

	// Blob 진행 - SHK 2023.12.27
	std::vector<BlobInfo> m_vecBlobInfoList;
	long m_nBlobTotalCount;
	BOOL DoDetectBlob(long itemIndex);

	// Blob2 - ehjang
	std::vector<BlobInfo> m_vecBlobInfoList2;
	long m_nBlobTotalCount2;
	BOOL DoDetectBlob2(long itemIndex);

	// EdgeDetect - ehjang
	std::vector<std::vector<CPoint>> m_vecptEdge;
	std::vector<CRect> m_vecrtSearchROI;
	std::vector<CRect> m_vecrtIgnoreROI;
	std::vector<CPoint> m_vecptStart;
	std::vector<CPoint> m_vecptEnd;
	std::vector<float> m_vecnTopBusbarHeight; // PSJ  long -> float으로 변경

	std::vector<SInspEdgeResult> m_vecsEdgeResult;

	BOOL DoEdgeDetect(long itemIndex);
	BOOL EdgeDetect(const BYTE* src, const BYTE* proc, const long itemIndex);

	std::vector<SResultParams> m_vecResult;
	long m_nOKCount;
	long m_nNGCount;
	CString m_strTotalResult;
	CString m_strNGtype;
	int m_nAlarm;

	SInspEdgeResult m_sInspEdgeResult;
	SInspBlobResult m_sInspBlobResult;
	
	// Result
	BOOL DoCalcEdgeResult(long itemIndex);
	BOOL DoCalcBlobResult(long itemIndex);
	BOOL DoCalcBlob2Result(long itemIndex);

	BOOL DoResult(long itemIndex);
	BOOL SetResultBlob(long itemIndex);
	BOOL SetResultBlob2(long itemIndex);
	BOOL SetResultEdge(long itemIndex);

	// Criteria
	long m_nBlobIdx;
	std::vector<SCriteriaParams> m_vecCriteriaResult;
	BOOL DoCriteria(long itemIndex, BOOL &bResult);
	BOOL SetCriteriaBlob(long itemIndex, BOOL &bResult);
	BOOL SetCriteriaBlob2(long itemIndex, BOOL &bResult);
	BOOL SetCriteriaEdge(long itemIndex, BOOL &bResult);

	BOOL m_lTotalResult;
	int m_bReInspection; // PSJ
	
	// 250619 sjpark YOLO Socket 파트 추가 (Main -> Inspection 알고리즘 단으로 이동)
	SOCKET m_sock = INVALID_SOCKET;
	bool m_connected = FALSE;

	bool InitSocketClient();// 사라질 예정
	void CleanupSocket(); // 사라질 예정
	std::vector<YOLOBox> ParseYOLOResult(const CString & jsonStr);
	std::vector<YOLOBox> m_vecYOLOResultBox;
	std::vector<YOLOBox> m_vecYOLOSpatterBox; // 250630 sjpark YOLO Spatter Box
	std::vector<YOLOBox> m_vecYOLOMatchBox; // P16: m_vecResult 측정점별로 판정에 쓴 welding 박스 (매칭 없으면 기본값). 결과 화면 타원·회전 박스 표시용
	BOOL SocketYOLO_SingleCall(const CString& imagePath);
	BOOL RequestYOLO(long itemIndex); // 현재 검사 영상을 임시 BMP로 저장 후 YOLO 서버 호출 (결과: m_vecYOLOResultBox / m_vecYOLOSpatterBox)
	BOOL SetCriteriaYOLO(long itemIndex, BOOL &bResult);
	CString m_currentImageName;
};

extern ALGORITHM_TOOLS_API CInspection g_Inspection;