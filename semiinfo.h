#pragma once

#include <algorithm>
#include <vector>
#include <list>
#include <string.h>
#include <fstream>
#include <iostream>
#include <string>
#include <memory>
#include <tuple>

#include "Algorithm/ImageProcessing.h"
#include "Algorithm/ImageProcessingHelper.h"
#include "Algorithm/ImageCondition.h"
#include "Algorithm/Geometry.h"
#include "Algorithm/BlobInfo.h"
#include "Algorithm/EdgeDetect.h"
#include "BaseMethod/IniReadWriter.h"
#include "BaseMethod/ImageInterface.h"
#include "BaseMethod/ThreadedProgress.h"
#include "BaseMethod/Label.h"
#include "BaseMethod/CrashDump.h"
#include "BaseMethod/FileVersion.h"
#include "BaseMethod/FileOperation.h"
#include "BaseMethod/ShareMemory.h"
#include "BaseMethod/Registry.h"
#include "ImageViewer/ImageDisplay.h"
#include "ImageViewer/EventMessage.h"
#include "GridControl/GridCtrl.h"

#pragma comment(lib, "HubisSDK.Library.Algorithm.lib")
#pragma comment(lib, "HubisSDK.Library.BaseMethod.lib")
#pragma comment( lib, "HubisSDK.Library.ImageViewer.lib" )

#ifdef _DEBUG
#pragma comment(lib, "HubisSDK.Library.GridControl_d.lib")
#else
#pragma comment(lib, "HubisSDK.Library.GridControl.lib")
#endif

#define PI 3.141592
#define TwoPI 2 * PI

#define IsSHIFTPressed() ((GetKeyState(VK_SHIFT) & (1 << (sizeof(SHORT) * 8 - 1))) != 0)
#define IsCTRLPressed()  ((GetKeyState(VK_CONTROL) & (1 << (sizeof(SHORT) * 8 - 1))) != 0)

#define REMOVE_EXT_RECIPE_FROM_FILENAME(x)		( (x).Left ( (x).Find(_T(".recipe")) ) )
#define REMOVE_EXT_BMP_FROM_FILENAME(x)			( (x).Left ( (x).Find(_T(".bmp")) ) )
#define REMOVE_EXT_JPG_FROM_FILENAME(x)			( (x).Left ( (x).Find(_T(".jpg")) ) )
#define REMOVE_FILE_FROM_PATHNAME(x)				( (x).Left((x).ReverseFind('\\')) )
#define REMOVE_FOLDER_FROM_PATHNAME(x)			( (x).Right( (x).GetLength() - (x).ReverseFind('\\') - 1 ) )
#define REMOVE_EXTENSION_FROM_PATHNAME(x)		( (x).Right( (x).GetLength() - (x).ReverseFind('.') - 1) )
#define REMOVE_EXT_FROM_FILENAME(x)					( (x).Left ( (x).ReverseFind('.') ) )

#define SCAN_THREAD_COUNT 5
#define INSPECTION_THREAD_COUNT 10
#define CAMERA_MAX_COUNT 5
#define INSPECTION_ITEM_MAX_COUNT 20
#define ILLUM_CHANNEL_MAX 4
#define DIVISION_INDEX_MAX 10
#define VECTOR_SIZE(v) (long)v.size()
#define STRING_MAX 256
#define DATA_MAX 20
#define PLC_ADDRESS_MAX 1500

enum enumInspAlarm
{
	ALARM_OK = 1000,
	ALARM_UNKNOWN,
	ALARM_THRESHOLD,
	ALARM_BLOB,
	ALARM_BLOB2,
	ALARM_EDGE,

	ALARM_CRI_BLOB,
	ALARM_CRI_BLOB2,
	ALARM_CRI_BETWEEN,
	ALARM_CRI_LENGTH,
	ALARM_CRI_HEIGHT,
	ALARM_CRI_ANGLE,

	ALARM_INSP_FAIL,
	ALARM_END,
};

enum enumUserMessage
{
	UM_UPDATE_LOG = WM_USER + 1416,
	UM_UPDATE_MESSAGE,
	UM_UPDATE_IMAGE,
	UM_UPDATE_RESULT,
	UM_UPDATE_INSP_RESULT_VISION1,
	UM_UPDATE_INSP_RESULT_VISION2,
	UM_UPDATE_GRAB_STATUS,
	UM_UPDATE_INSP_STATUS,
	UM_UPDATE_CTRL_STATUS,
	UM_UPDATE_LASER_STATUS,
	UM_UPDATE_RECIPE_UPDATE,
};

enum class CAMERA_TYPE : long
{
	ACA244020GM, // Basler acA2440-20gm
};

enum class TRIGGER_MODE : long
{
	TRIGGER, // Trigger mode
	FREERUN, // Freerun mode
};

enum class CAMERA_INDEX : long
{
	CAMERA1,
	CAMERA2,
	CAMERA3,
	CAMERA4,
	CAMERA5,
};

enum class MAINUI : long
{
	UI_FOR_SINGLE_CAMERA,
	UI_FOR_DUAL_CAMERA,
};

enum class DATA_SAVE : long
{
	RAW_IMAGE,
	RESULT_IMAGE,
	RESULT_DATA_CSV,
};

enum class INSPECTION_DIVISION : long
{
	NONE,
	BEFORE,
	AFTER,
	END,
};

enum class ALGORITHM_SEQUENCE : long
{
	EDGE_DETECT,
	DETECT_BLOB,
	DETECT_BLOB2,
	END,
};

const LPCTSTR gl_szStrAlgoSequenceName[] =
{
	_T("Edge Detect"),
	_T("Detect Blob"),
	_T("Detect Blob2"),
};

enum class EDGE_DETECT_SEQUENCE : long
{
	ROI,
	THRESHOLD,
	BLOB,
	EDGE_DETECT,
	RESULT,
	CRITERIA,
	END,
};

const LPCTSTR gl_szStrEdgeDetect2SequenceName[] =
{
	_T("ROI"),
	_T("Threshold"),
	_T("Blob"),
	_T("Edge Detect"),
	_T("Result"),
	_T("Criteria"),
};

enum class DETECT_BLOB_SEQUENCE : long
{
	ROI,
	THRESHOLD,
	BLOB,
	RESULT,
	CRITERIA,
	END,
};

const LPCTSTR gl_szStrDetectBlobSequenceName[] =
{
	_T("ROI"),
	_T("Threshold"),
	_T("Blob"),
	_T("Result"),
	_T("Criteria"),
};

enum class DETECT_BLOB2_SEQUENCE : long
{
	ROI,
	THRESHOLD,
	BLOB,
	BLOB2,
	RESULT,
	CRITERIA,
	END,
};

const LPCTSTR gl_szStrDetectBlob2SequenceName[] =
{
	_T("ROI"),
	_T("Threshold"),
	_T("Blob"),
	_T("Blob2"),
	_T("Result"),
	_T("Criteria"),
};

enum class THRESHOLD : long
{
	NORMAL,
	ADAPTIVE,
	HYSTERISIS,
	OTSU,
	TRIANGLE,
	VIGNETTE,
};

enum enumTimerCheck
{
	CAMERA_LIVE,
	FOLDER_DELETE,
	CTRL_CONNECT,
	LASER_CONNECT,
	PLC_MOINTERING,
	PLC_LINK,
	REAL_TIME,
};

enum class IMAGE : long
{
	ORIGINAL11,
	ORIGINAL21,
	ORIGINAL12,
	ORIGINAL22,
	ORIGINAL13,
	ORIGINAL23,

	ORIGINAL11_MASK,
	ORIGINAL11_TH_PROC,
	ORIGINAL11_TH,
	ORIGINAL11_EDGE_PROC,
	ORIGINAL11_BLOB_PROC,
	ORIGINAL11_BLOB_PROC2,

	ORIGINAL21_MASK,
	ORIGINAL21_TH_PROC,
	ORIGINAL21_TH,
	ORIGINAL21_EDGE_PROC,
	ORIGINAL21_BLOB_PROC,
	ORIGINAL21_BLOB_PROC2,

	ORIGINAL12_MASK,
	ORIGINAL12_TH_PROC,
	ORIGINAL12_TH,
	ORIGINAL12_EDGE_PROC,
	ORIGINAL12_BLOB_PROC,
	ORIGINAL12_BLOB_PROC2,

	ORIGINAL22_MASK,
	ORIGINAL22_TH_PROC,
	ORIGINAL22_TH,
	ORIGINAL22_EDGE_PROC,
	ORIGINAL22_BLOB_PROC,
	ORIGINAL22_BLOB_PROC2,

	ORIGINAL13_MASK,
	ORIGINAL13_TH_PROC,
	ORIGINAL13_TH,
	ORIGINAL13_EDGE_PROC,
	ORIGINAL13_BLOB_PROC,
	ORIGINAL13_BLOB_PROC2,

	ORIGINAL23_MASK,
	ORIGINAL23_TH_PROC,
	ORIGINAL23_TH,
	ORIGINAL23_EDGE_PROC,
	ORIGINAL23_BLOB_PROC,
	ORIGINAL23_BLOB_PROC2,

	END,
};

enum class CUSTOMER : long
{
	BORGWARNER,
	SAMBO,
};

// 업체 추가시 마다 새로 추가해야함, 외부에서 이름 변경시 Define도 바꿔줘야함.
// 외부에서 이름 변경하고 여기서 변경 안해도 상관은 없지만, 순서가 바뀔 경우 똑같이 바꿔줘야함.
// 왠만하면 PLC와 협의 후 Fix 해서 사용할 것.
namespace BORGWARNER
{
	enum class DATA : long
	{
		ON = 1,
		OFF = 0,
		DATA_COUNT = 6,
	};

	enum class P2V_ADDR_NAME : long
	{
		PLC_DI_LinkTest,
		PLC_DI_REQ_Initialize,
		PLC_DI_REQ_TimeSync,
		PLC_DI_REQ_ReadyInspection,
		PLC_DI_ALARM,
		PLC_DI_Auto,
		PLC_DI_Manual,
		PLC_DI_REQ_Insp_InitScanPos,
		PLC_DI_REQ_Insp_Start,
		PLC_DI_REQ_Insp_Blank1,
		PLC_DI_REQ_Insp_Blank2,
		PLC_DI_RESP_Insp_Move,
		PLC_IN_SYNC_TIME_Year,
		PLC_IN_SYNC_TIME_Month,
		PLC_IN_SYNC_TIME_Day,
		PLC_IN_SYNC_TIME_Hour,
		PLC_IN_SYNC_TIME_Minute,
		PLC_IN_SYNC_TIME_Second,
		PLC_IN_InspMode,
		PLC_IN_PLC_Model_NO,
		PLC_IN_JobNumber,
		PLC_IN_CameraIndex,
		PLC_IN_DivIndex,
		PLC_IN_ModelName_01,
		PLC_IN_ModelName_02,
		PLC_IN_ModelName_03,
		PLC_IN_ModelName_04,
		PLC_IN_ModelName_05,
		PLC_IN_ModelName_06,
		PLC_IN_ModelName_07,
		PLC_IN_ModelName_08,
		PLC_IN_ModelName_09,
		PLC_IN_ModelName_10,
		PLC_IN_LotID_01,
		PLC_IN_LotID_02,
		PLC_IN_LotID_03,
		PLC_IN_LotID_04,
		PLC_IN_LotID_05,
		PLC_IN_LotID_06,
		PLC_IN_LotID_07,
		PLC_IN_LotID_08,
		PLC_IN_LotID_09,
		PLC_IN_LotID_10,
		PLC_IN_Scan_Pos_X,
		PLC_IN_Scan_Pos_Y,
		END,
	};

	enum class V2P_ADDR_NAME : long
	{
		PLC_DO_LinkTest,
		PLC_DO_RESP_Initialize,
		PLC_DO_RESP_TimeSync,
		PLC_DO_RESP_ReadyInspection,
		PLC_DO_ALARM,
		PLC_DO_Auto,
		PLC_DO_Manul,
		PLC_DO_RESP_Insp_InitScanPos,
		PLC_DO_RESP_Insp_Start,
		PLC_DO_RESP_Insp_End,
		PLC_DO_RESP_Insp_Stop,
		PLC_DO_REQ_Insp_Move,
		PLC_OUT_InspRet,
		PLC_OUT_JobNumber,
		ResultData_000,
		ResultData_001,
		ResultData_002,
		ResultData_003,
		ResultData_004,
		ResultData_005,
		ResultData_006,
		ResultData_007,
		ResultData_008,
		ResultData_009,
		ResultData_010,
		ResultData_011,
		ResultData_012,
		ResultData_013,
		ResultData_014,
		ResultData_015,
		ResultData_016,
		ResultData_017,
		ResultData_018,
		ResultData_019,
		ResultData_020,
		ResultData_021,
		ResultData_022,
		ResultData_023,
		ResultData_024,
		ResultData_025,
		ResultData_026,
		ResultData_027,
		ResultData_028,
		ResultData_029,
		ResultData_030,
		ResultData_031,
		ResultData_032,
		ResultData_033,
		ResultData_034,
		ResultData_035,
		ResultData_036,
		ResultData_037,
		ResultData_038,
		ResultData_039,
		ResultData_040,
		ResultData_041,
		ResultData_042,
		ResultData_043,
		ResultData_044,
		ResultData_045,
		ResultData_046,
		ResultData_047,
		ResultData_048,
		ResultData_049,
		ResultData_050,
		ResultData_051,
		ResultData_052,
		ResultData_053,
		ResultData_054,
		ResultData_055,
		ResultData_056,
		ResultData_057,
		ResultData_058,
		ResultData_059,
		ResultData_060,
		ResultData_061,
		ResultData_062,
		ResultData_063,
		ResultData_064,
		ResultData_065,
		ResultData_066,
		ResultData_067,
		ResultData_068,
		ResultData_069,
		ResultData_070,
		ResultData_071,
		ResultData_072,
		ResultData_073,
		ResultData_074,
		ResultData_075,
		ResultData_076,
		ResultData_077,
		ResultData_078,
		ResultData_079,
		ResultData_080,
		ResultData_081,
		ResultData_082,
		ResultData_083,
		ResultData_084,
		ResultData_085,
		ResultData_086,
		ResultData_087,
		ResultData_088,
		ResultData_089,
		ResultData_090,
		ResultData_091,
		ResultData_092,
		ResultData_093,
		ResultData_094,
		ResultData_095,
		ResultData_096,
		ResultData_097,
		ResultData_098,
		ResultData_099,
		ResultData_100,
		ResultData_101,
		ResultData_102,
		ResultData_103,
		ResultData_104,
		ResultData_105,
		ResultData_106,
		ResultData_107,
		ResultData_108,
		ResultData_109,
		ResultData_110,
		ResultData_111,
		ResultData_112,
		ResultData_113,
		ResultData_114,
		ResultData_115,
		ResultData_116,
		ResultData_117,
		ResultData_118,
		ResultData_119,
		ResultData_120,
		ResultData_121,
		ResultData_122,
		ResultData_123,
		ResultData_124,
		ResultData_125,
		ResultData_126,
		ResultData_127,
		ResultData_128,
		ResultData_129,
		ResultData_130,
		ResultData_131,
		ResultData_132,
		ResultData_133,
		ResultData_134,
		ResultData_135,
		ResultData_136,
		ResultData_137,
		ResultData_138,
		ResultData_139,
		ResultData_140,
		ResultData_141,
		ResultData_142,
		ResultData_143,
		ResultData_144,
		ResultData_145,
		ResultData_146,
		ResultData_147,
		ResultData_148,
		ResultData_149,
		ResultData_150,
		ResultData_151,
		ResultData_152,
		ResultData_153,
		ResultData_154,
		ResultData_155,
		ResultData_156,
		ResultData_157,
		ResultData_158,
		ResultData_159,
		ResultData_160,
		ResultData_161,
		ResultData_162,
		ResultData_163,
		ResultData_164,
		ResultData_165,
		ResultData_166,
		ResultData_167,
		ResultData_168,
		ResultData_169,
		ResultData_170,
		ResultData_171,
		ResultData_172,
		ResultData_173,
		ResultData_174,
		ResultData_175,
		ResultData_176,
		ResultData_177,
		ResultData_178,
		ResultData_179,
		ResultData_180,
		ResultData_181,
		END,
	};
}

//Inspection ErroCode - SHK - 2023.12.27
enum
{
	INSP_ERR_OK = 1,
	INSP_ERR = 0,
	INSP_NONE = -1,
};
enum class InspType
{
	INSP_None =0,
	INSP_AC = 1,
	INSP_DC =2,
};

typedef struct SIllumParams
{
	unsigned short m_usIllum[ILLUM_CHANNEL_MAX];

	void Init()
	{
		for (long idx = 0; idx < ILLUM_CHANNEL_MAX; idx++)
		{
			m_usIllum[idx] = 0;
		}
	}
}SIllumParams;

typedef struct SProcImageParams
{
	long nImageProcNum;
	std::vector<short> m_vecnProc;
	std::vector<short> m_vecnProcParams;

	void Init()
	{
		nImageProcNum = 0;
		m_vecnProc.clear();
		m_vecnProcParams.clear();
	}
}SProc;

typedef struct SBusbarInfoParam
{
	long nInspectionDivision;
	long nDCCount;
	long nACCount;
	long nGrabCount;

	void Init()
	{
		nInspectionDivision = (long)INSPECTION_DIVISION::NONE;
		nDCCount = 3;
		nACCount = 1;
		nGrabCount = 3;
	}
}SBusbarInfoParam;

typedef struct SEdgeDetectParam
{
	CRect rtSearchROI;
	long nDirection; // 0: Left, 1: Top, 2: Right, 3: Bottom
	long nSearchDirection; // 0: Left->Right, 1: Right->Left, 2: Top->Bottom, 3: Bottom->Top
	long nEdgeDirection; // 0, 1, 2(PI_ED_DIR_BOTH, PI_ED_DIR_RISING, PI_ED_DIR_FALLING)
	BOOL bFirstEdge;
	long nSampleRate;
	float fEdgeThreshold;
	BOOL bUseEdgeFiltering;
	long nEdgeFilteringRange;
	BOOL bNGEdgeErase; // TRUE: NG인 Edge를 지운다., FALSE: NG인 Edge를 평균값으로 대체한다.

	void Init()
	{
		rtSearchROI = CRect(100, 100, 500, 500);
		nDirection = 0;
		nSearchDirection = 0;
		nEdgeDirection = 0;
		bFirstEdge = FALSE;
		nSampleRate = 1;
		fEdgeThreshold = 5.f;
		bUseEdgeFiltering = FALSE;
		nEdgeFilteringRange = 10;
		bNGEdgeErase = TRUE;
	}
}SEdgeDetectParam;

typedef struct SAlignParams
{
	long nDeflateROIWidth;
	long nDeflateROIHeight;
	SEdgeDetectParam sEdgeDetectParam;
	long nIgnoreROINum;
	std::vector<CRect> vecrtIgnoreROI;

	void Init()
	{
		nDeflateROIWidth = 10;
		nDeflateROIHeight = 10;
		sEdgeDetectParam.Init();
		nIgnoreROINum = 0;
		vecrtIgnoreROI.clear();
	}
}SAlignParams;

typedef struct SBlobParams
{
	long nDeflateLeft;
	long nDeflateTop;
	long nDeflateRight;
	long nDeflateBottom;
	long nMinBlobSize;
	long nMaxBlobCount;
	BOOL bBlobMerge;
	long nBlobMergeDist;

	void Init()
	{
		nDeflateLeft = 0;
		nDeflateTop = 0;
		nDeflateRight = 0;
		nDeflateBottom = 0;
		nMinBlobSize = 100;
		nMaxBlobCount = 10000;
		bBlobMerge = FALSE;
		nBlobMergeDist = 1;
	}
}SBlobParams;

typedef struct SThresholdParams
{
	BOOL bUseThreshold;
	long nSelectThreshold;
	long nThresholdValue;
	long nAdaptiveOffset;
	long nHysteresisLow;
	long nHysteresisHigh;
	long nHighThresholdRegionNoiseFilteringSize;
	BOOL bThresholdReverse;
	long nLeftMargin; // Left margin for threshold adjustment
	long nRightMargin; // Right margin for threshold adjustment
	long nThresholdOffset; // Offset value for side threshold adjustment
	float fVignetteStrength;
	BOOL bUseReInspection; // PSJ
	long nReInspectionOffset; // PSJ
	BOOL bUseAdaptiveThreshold; // 250313 PSJ
	long nAdaptiveThresholdOffset; // 250313 PSJ

	void Init()
	{
		bUseThreshold = TRUE;
		nSelectThreshold = 0;
		nThresholdValue = 128;
		nAdaptiveOffset = 10;
		nHysteresisLow = 30;
		nHysteresisHigh = 225;
		nHighThresholdRegionNoiseFilteringSize = 10;
		bThresholdReverse = FALSE;
		nLeftMargin = 0; // Default margin
		nRightMargin = 0; // Default margin
		nThresholdOffset = 0; // Default offset for threshold adjustment
		fVignetteStrength = 0;
		bUseReInspection = FALSE;
		nReInspectionOffset = 0;
	}
}SThresholdParams;

typedef struct SROIParams
{
	BOOL bUseROI;
	long nSearchROINum;
	std::vector<CRect> vecrtSearchROI;
	long nIgnoreROINum;
	std::vector<CRect> vecrtIgnoreROI;

	void Init()
	{
		bUseROI = FALSE;
		nSearchROINum = 0;
		vecrtSearchROI.clear();
		nIgnoreROINum = 0;
		vecrtIgnoreROI.clear();
	}
}SROIParams;

//Criteria 임시 - SHK - 2023.12.27
typedef struct SCriteriaParams
{
	long nErr;
	long nBlobWidthMin;
	long nBlobWidthMax;
	long nBlobHeightMin;
	long nBlobHeightMax;
	float fWeldWidthMin; //mm
	float fWeldWidthMax; //mm
	float fWeldHeightMin; //mm
	float fWeldHeightMax; //mm
	float fBetweenDistMinX; //mm
	float fBetweenDistMaxX; //mm
	float fBetweenDistMinY; //mm
	float fBetweenDistMaxY; //mm
	float fLengthMin; //mm, 2Point distance
	float fLengthMax;
	float fAngleMin; // deg, 2Point Angle
	float fAngleMax;
	float fTopBusbarHeightMin; //mm
	float fTopBusbarHeightMax; //mm

	void Init()
	{
		nErr = INSP_NONE;
		nBlobWidthMin = 0;
		nBlobWidthMax = 500;
		nBlobHeightMin = 0;
		nBlobHeightMax = 500;
		fWeldWidthMin = 0.f;
		fWeldWidthMax = 5.f;
		fWeldHeightMin = 0.f;
		fWeldHeightMax = 5.f;
		fBetweenDistMinX = 0.f;
		fBetweenDistMaxX = 5.f;
		fBetweenDistMinY = 0.f;
		fBetweenDistMaxY = 5.f;
		fLengthMin = 0.f;
		fLengthMax = 10.f;
		fAngleMin = 0.f;
		fAngleMax = 360.f;
		fTopBusbarHeightMin = 5.f;
		fTopBusbarHeightMax = 10.f;
	}
}SCriteriaParams;

//Result 임시 - SHK - 2023.12.27
typedef struct SResultParams
{
	long visionIndex;	// Vision enum(Camera Index)(index 구분을 위한 추가)
	long divIndex;		// Vision 구분 Index			(index 구분을 위한 추가)
	
	// BOOL Vector든 각각이든 algosequence result 에 대한 데이터 저장 / 초기값은 TRUE로 줘야함. 실패한것만 바꿔줘야됨.
	//

	//Blob
	long nBlobIndex;
	CRect rtROI;
	float fWeldWidth; //mm
	float fWeldHeight; //mm
	float fArea;
	//float fBlobdxOffset;
	//float fBlobdyOffset;


	float fBlobStartPtXOffset; //mm
	float fBlobStartPtYOffset; //mm
	float fBlobEndPtXOffset; //mm
	float fBlobEndPtYOffset; //mm


	CPoint ptBlobStart;
	CPoint ptBlobEnd;
	float fBlobLength; //mm
	float fBlobAngle;
	float fBlobTopBusbarHeight; //mm
	float fBlobMargin;

	//Edge
	CPoint ptStartResult;
	CPoint ptEndResult;
	CRect rtBlobROI;
	float fEdgeStartPtXOffset; //mm
	float fEdgeStartPtYOffset; //mm
	float fEdgeEndPtXOffset; //mm
	float fEdgeEndPtYOffset; //mm
	float fEdgeLength; //mm
	float fEdgeAngle; //deg
	float fEdgeTopBusbarHeight; //mm
	
	void Init()
	{
		visionIndex = 0;
		divIndex = 0;

		nBlobIndex = 0;
		rtROI = CRect(0, 0, 0, 0);
		fWeldWidth = 0.f;
		fWeldHeight = 0.f;
		fArea = 0.f;
		ptBlobStart = CPoint(0, 0);
		ptBlobEnd = CPoint(0, 0);
		//fBlobdxOffset = 0.f;
		//fBlobdyOffset = 0.f;
		fBlobStartPtXOffset = 0.f;
		fBlobStartPtYOffset = 0.f;
		fBlobEndPtXOffset = 0.f;
		fBlobEndPtYOffset = 0.f;
		fBlobAngle = 0.f;
		fBlobLength = 0.f;
		fBlobTopBusbarHeight = 0.f;

		ptStartResult = CPoint(0, 0);
		ptEndResult = CPoint(0, 0);
		rtBlobROI = CRect(0, 0, 0, 0);
		fEdgeStartPtXOffset = 0.f;
		fEdgeStartPtYOffset = 0.f;
		fEdgeEndPtXOffset = 0.f;
		fEdgeEndPtYOffset = 0.f;
		fEdgeLength = 0.f;
		fEdgeAngle = 0.f;
		fEdgeTopBusbarHeight = 0.f;
	}
}SResultParams;

typedef struct SInspectionItem
{
	CString strName;
	long visionIndex; // Vision enum(Camera Index)
	long divIndex; // Vision 구분 Index(같은 카메라라고 해도 찍는 위치에 따라 검사 Recipe가 다름을 구분하기 위함)
	long algoSequence;
	SIllumParams sIllum;
	SProcImageParams sProc[6];
	SROIParams sROI;
	SThresholdParams sThreshold;
	SBlobParams sBlob;
	SBlobParams sBlob2;
	SAlignParams sAlign;
	SCriteriaParams sCriteria;
	SResultParams sResult;
	int nInspType;

	void Init()
	{
		strName = _T("Default");
		visionIndex = 0;
		divIndex = 0;
		nInspType = (long)InspType::INSP_None;
		algoSequence = (long)ALGORITHM_SEQUENCE::EDGE_DETECT;
		sProc[0].Init();
		sProc[1].Init();
		sProc[2].Init();
		sProc[3].Init();
		sProc[4].Init();
		sProc[5].Init();
		sIllum.Init();
		sROI.Init();
		sThreshold.Init();
		sBlob.Init();
		sBlob2.Init();
		sAlign.Init();
		sCriteria.Init();
		sResult.Init();
	}
}SInspItem;

typedef struct SMatchItem
{
	long visionIndex;
	long divIndex;
	long matchingIndex;
	long subItemNum;
	std::vector<long> vecsSubItemIndex;

	void Init()
	{
		visionIndex = 0;
		divIndex = 0;
		matchingIndex = 0;
		subItemNum = 0;
		vecsSubItemIndex.clear();
	}
}SMatchItem;

typedef struct SInspectionEdgeResult
{
	BOOL bResult;
	BOOL bVertical;
	CPoint ptStart;
	CPoint ptEnd;
	float fAngle;
	float fdx;
	float fdy;

	void Init()
	{
		bResult = FALSE;
		bVertical = FALSE;
		ptStart = CPoint(0, 0);
		ptEnd = CPoint(0, 0);
		fAngle = 0.0f;
		fdx = 0.0f;
		fdy = 0.0f;
	}
}SInspEdgeResult;


typedef struct SInspectionBlobResult
{
	BOOL bResult;

	void Init()
	{
		BOOL bResult = FALSE;
	}
}SInspBlobResult;

enum class PLC_MEMORY_TYPE : long
{
	BIT,
	INT,
	WORD,
	DOUBLE,
};

typedef struct SPLCDataParams
{
	CString strName;
	long nAddr;
	long nType;
	long nBitNum;
	short sValue;
	long nValue;

	void Init()
	{
		strName = _T(" ");
		nAddr = 0;
		nType = (long)PLC_MEMORY_TYPE::BIT;
		nBitNum = 0;
		sValue = 0;
		nValue = 0;
	}

}SPLCData;

typedef struct SPLCParams
{
	std::vector<SPLCData> vecsPLC2VisionData;
	std::vector<SPLCData> vecsVision2PLCData;
	long nPLC2VisionStartAddr;
	long nPLC2VisionCnt;
	long nVision2PLCStartAddr;
	long nVision2PLCCnt;

	void Init()
	{
		vecsPLC2VisionData.clear();
		vecsVision2PLCData.clear();
		nPLC2VisionStartAddr = 0;
		nPLC2VisionCnt = 0;
		nVision2PLCStartAddr = 0;
		nVision2PLCCnt = 0;
	}
}SPLC;

enum eIntermediate
{
	INTERMEDIATE_NONE,
	INTERMEDIATE_RECT,
	INTERMEDIATE_CRPOINT,
	INTERMEDIATE_POINT,
	INTERMEDIATE_LINE,
	INTERMEDIATE_STRING,
	INTERMEDIATE_ELLIPSE,
};

enum eDebug
{
	DEBUG_ROI_BEGIN,
	DEBUG_ROI_SEARCH_ROI = DEBUG_ROI_BEGIN,
	DEBUG_ROI_IGNORE_ROI,
	DEBUG_ROI_END = DEBUG_ROI_IGNORE_ROI,

	DEBUG_EDGE_DETECT_BEGIN,
	DEBUG_EDGE_DETECT_SEARCH_ROI = DEBUG_EDGE_DETECT_BEGIN,
	DEBUG_EDGE_DETECT_IGNORE_ROI,
	DEBUG_EDGE_DETECT_FOUND_EDGE,
	DEBUG_EDGE_DETECT_INTERSECTION_POINTS,
	DEBUG_EDGE_DETECT_END = DEBUG_EDGE_DETECT_INTERSECTION_POINTS,

	DEBUG_THRESHOLD_BEGIN,
	DEBUG_THRESHOLD_SEARCH_ROI = DEBUG_THRESHOLD_BEGIN,
	DEBUG_THRESHOLD_IGNORE_ROI,
	DEBUG_THRESHOLD_END = DEBUG_THRESHOLD_IGNORE_ROI,

	DEBUG_BLOB_BEGIN,
	DEBUG_BLOB_FOUND_BLOB = DEBUG_BLOB_BEGIN,
	DEBUG_BLOB_END = DEBUG_BLOB_FOUND_BLOB,

	DEBUG_BLOB2_BEGIN,
	DEBUG_BLOB2_FOUND_BLOB = DEBUG_BLOB2_BEGIN,
	DEBUG_BLOB2_END = DEBUG_BLOB2_FOUND_BLOB,

	DEBUG_RESULT_BEGIN,
	DEBUG_RESULT_RESULT = DEBUG_RESULT_BEGIN,
	DEBUG_RESULT_END = DEBUG_RESULT_RESULT,

	DEBUG_CRITERIA_BEGIN,
	DEBUG_CRITERIA_RESULT = DEBUG_CRITERIA_BEGIN,
	DEBUG_CRITERIA_END = DEBUG_CRITERIA_RESULT,

	DEBUG_RESULT,
	DEBUG_RESULT_OK = DEBUG_RESULT,
	DEBUG_RESULT_NG,

	DEBUG_INVALID,

	DEBUG_TOTAL_END,
};

const LPCTSTR gl_szStrDebugName[] =
{
	_T("[ROI] Search ROI"),
	_T("[ROI] Ignore ROI"),
	_T("[Edge Detect] Search ROI"),
	_T("[Edge Detect] Ignore ROI"),
	_T("[Edge Detect] Found Edge"),
	_T("[Edge Detect] Intersection Points"),
	_T("[Threshold] Search ROI"),
	_T("[Threshold] Ignore ROI"),
	_T("[Blob] Found Blob"),
	_T("[Blob2] Found Blob"),
	_T("[Resutl] Result"),
	_T("[Criteria] Result"),
};


struct SOverlay_String
{
	SOverlay_String(long i_x, long i_y, LPCTSTR szText, long nSize)
	{
		ptXY = CPoint(i_x, i_y);
		strText = szText;
		nFontSize = nSize;
	}

	SOverlay_String(CPoint pt, LPCTSTR szText, long nSize)
	{
		ptXY = pt;
		strText = szText;
		nFontSize = nSize;
	}

	void Init()
	{
		ptXY = CPoint(0, 0);
		strText = _T("");
		nFontSize = 10;
	}

	CPoint ptXY;
	CString strText;
	long nFontSize;
};

struct SOverlay_Line
{
	SOverlay_Line(long x1, long y1, long x2, long y2)
	{
		ptStart = CPoint(x1, y1);
		ptEnd = CPoint(x2, y2);
	}

	SOverlay_Line(CPoint pt1, CPoint pt2)
	{
		ptStart = pt1;
		ptEnd = pt2;
	}

	void Init()
	{
		ptStart = CPoint(0, 0);
		ptEnd = CPoint(0, 0);
	}

	CPoint ptStart;
	CPoint ptEnd;
};

struct SDebugResult
{
	std::vector<CRect>					vecRect;
	std::vector<CPoint>					vecCrPoint;
	std::vector<CPoint>					vecPoint;
	std::vector<SOverlay_Line>			vecLine;
	std::vector<SOverlay_String>		vecString;
	std::vector<CRect>					vecEllipse;

	CString								strStepName;
	BOOL								bManualDraw;
	BOOL								bInLineDraw;
	BOOL								bManualUse;
	BOOL								bInLineUse;
	COLORREF							crColor;
};

#pragma pack(push,1)

// [Laser SW -> Vision S/W]
enum class MSG_L2V : long
{
	L2V_DATA_REQUEST,			// 검사 결과 정보 요청
	L2V_RECIVE_DONE,			// 검사 결과 정보 받기 완료
	L2V_ALIVE,						// 상대방이 접속 중인지 확인 하기 위함, return : V2L_ALIVE
	L2V_END,
};

// [Vision S/W -> Laser SW]
enum class MSG_V2L : long
{
	V2L_SEND_RESULT,				// 검사 결과 정보 전송, return : L2V_RECIVE_DONE
	V2L_ALIVE,							// 상대방이 접속 중인지 확인 하기 위함, return : L2V_ALIVE
	V2L_END,
};

// [제어SW -> Vision S/W]
enum class MSG_H2V : long
{
	H2V_SCAN_START,			// Motion 이동 완료
	H2V_ALIVE,						// 상대방이 접속 중인지 확인 하기 위함, return : V2H_ALIVE
	H2V_END,
};

// [Vision S/W -> 제어SW]
enum class MSG_V2H : long
{
	V2H_SCAN_DONE,
	V2H_SCAN_FAIL,
	V2H_ALIVE,						// 상대방이 접속 중인지 확인 하기 위함, return : V2H_ALIVE
	V2H_END,
};


struct SDataInfo
{
	long messageIndex;			// 메시지 이름 (MSG_L2V, MSG_V2L)
	char sBarcode[STRING_MAX];		// 제품 바코드
	long status;				// 1: 성공, 0: 실패
	long startPointX[DATA_MAX];		// 용접 기준 위치(시료 끝 단) Start Point X
	long startPointY[DATA_MAX];		// 용접 기준 위치(시료 끝 단) Start Point Y
	long endPointX[DATA_MAX];		// 용접 기준 위치(시료 끝 단) End Point X
	long endPointY[DATA_MAX];		// 용접 기준 위치(시료 끝 단) End Point Y
	float dx[DATA_MAX];			// 이미지 좌표의 (0,0) 기준 벗어난 정도 X
	float dy[DATA_MAX];			// 이미지 좌표의 (0,0) 기준 벗어난 정도 Y
	float angle[DATA_MAX];			// Start/End Point를 찾는 Edge와 기준인 X/Y축과 틀어진 정도

	void Init()
	{
		messageIndex = 0;
		WideCharToMultiByte(CP_ACP, 0, _T(""), -1, sBarcode, STRING_MAX, NULL, NULL);
		status = 0;
		memset(startPointX, 0, DATA_MAX * sizeof(long));
		memset(startPointY, 0, DATA_MAX * sizeof(long));
		memset(endPointX, 0, DATA_MAX * sizeof(long));
		memset(endPointY, 0, DATA_MAX * sizeof(long));
		memset(dx, 0, DATA_MAX * sizeof(float));
		memset(dy, 0, DATA_MAX * sizeof(float));
		memset(angle, 0, DATA_MAX * sizeof(float));
	}
};

struct SPLCInfo
{
	long messageIndex;				// 메시지 이름 (MSG_H2V, MSG_V2H)
	long status;							// 1: 성공, 0: 실패
	long cameraIndex;
	long divIndex;

	void Init()
	{
		messageIndex = 0;
		status = 0;
		cameraIndex = 0;
		divIndex = 0;
	}
};

#pragma pack(pop)
