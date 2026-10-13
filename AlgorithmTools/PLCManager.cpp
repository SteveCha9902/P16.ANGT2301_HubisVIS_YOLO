#include "pch.h"
#include "PLCManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPLCManager g_PLCManager;

CPLCManager::CPLCManager()
	: m_strPath(_T(""))
{
	m_nJudge = 0;
}

CPLCManager::~CPLCManager()
{
}

void CPLCManager::SetCurPath(CString strCurPath)
{
	m_strCurPath = strCurPath;

	InitParameters();
}

void CPLCManager::InitParameters()
{
	m_strPath.Format(_T("%s\\Config\\PLCMap.ini"), m_strCurPath);

	//초기화
	m_sPLC.Init();

	m_nUseRecipeNum = 1;
	m_vecstrUseRecipe.clear();
	m_vecstrUseRecipe.resize(m_nUseRecipeNum);

	m_vecstrUseRecipe.push_back(_T("Default.recipe"));

	// 임시
	for(int i = 0; i < 3; i++)
	{
		m_fPLCPosX[i] = 0.f;
		m_fPLCPosY[i] = 0.f;
	}

}

BOOL CPLCManager::LoadPLCMap(const CString strLoadini)
{
	CString strPath;

	if(strLoadini == _T(""))
	{
		return FALSE;
	}

	return LoadParameters(strLoadini);
}

BOOL CPLCManager::SavePLCMap()
{
	SaveParameters(m_strPath);

	return TRUE;
}

BOOL CPLCManager::LoadParameters(const CString strPath)
{
	CString strTemp1, strTemp2;

	HUBSDK::IniReadWriter::Read(strPath, _T("PLC-Recipe"), _T("Use Recipe Num"), &m_nUseRecipeNum, m_nUseRecipeNum);

	m_vecstrUseRecipe.resize(m_nUseRecipeNum);

	for(long idx = 0; idx < m_nUseRecipeNum; idx++)
	{
		strTemp1.Format(_T("%d"), idx);
		HUBSDK::IniReadWriter::Read(strPath, _T("PLC-Recipe"), strTemp1, &m_vecstrUseRecipe[idx], m_vecstrUseRecipe[idx]);
	}

	HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), _T("PLC2Vision Start Address"), &m_sPLC.nPLC2VisionStartAddr, m_sPLC.nPLC2VisionStartAddr);
	HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), _T("PLC2Vision Count"), &m_sPLC.nPLC2VisionCnt, m_sPLC.nPLC2VisionCnt);

	m_sPLC.vecsPLC2VisionData.resize(m_sPLC.nPLC2VisionCnt);

	for(long data = 0; data < m_sPLC.nPLC2VisionCnt; data++)
	{
		strTemp2.Format(_T("Read_%d_Name"), data);
		HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsPLC2VisionData[data].strName, m_sPLC.vecsPLC2VisionData[data].strName);
		strTemp2.Format(_T("Read_%d_Address"), data);
		HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsPLC2VisionData[data].nAddr, m_sPLC.vecsPLC2VisionData[data].nAddr);
		strTemp2.Format(_T("Read_%d_Type"), data);
		HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsPLC2VisionData[data].nType, m_sPLC.vecsPLC2VisionData[data].nType);
		strTemp2.Format(_T("Read_%d_BitNum"), data);
		HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsPLC2VisionData[data].nBitNum, m_sPLC.vecsPLC2VisionData[data].nBitNum);
	}

	HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), _T("Vision2PLC Start Address"), &m_sPLC.nVision2PLCStartAddr, m_sPLC.nVision2PLCStartAddr);
	HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), _T("Vision2PLC Count"), &m_sPLC.nVision2PLCCnt, m_sPLC.nVision2PLCCnt);

	m_sPLC.vecsVision2PLCData.resize(m_sPLC.nVision2PLCCnt);

	for(long data = 0; data < m_sPLC.nVision2PLCCnt; data++)
	{
		strTemp2.Format(_T("Write_%d_Name"), data);
		HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsVision2PLCData[data].strName, m_sPLC.vecsVision2PLCData[data].strName);
		strTemp2.Format(_T("Write_%d_Address"), data);
		HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsVision2PLCData[data].nAddr, m_sPLC.vecsVision2PLCData[data].nAddr);
		strTemp2.Format(_T("Write_%d_Type"), data);
		HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsVision2PLCData[data].nType, m_sPLC.vecsVision2PLCData[data].nType);
		strTemp2.Format(_T("Write_%d_BitNum"), data);
		HUBSDK::IniReadWriter::Read(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsVision2PLCData[data].nBitNum, m_sPLC.vecsVision2PLCData[data].nBitNum);
	}

	return TRUE;
}

BOOL CPLCManager::SaveParameters(const CString strPath)
{
	CString strTemp1, strTemp2;

	m_nUseRecipeNum = (long)m_vecstrUseRecipe.size();

	HUBSDK::IniReadWriter::Write(strPath, _T("PLC-Recipe"), _T("Use Recipe Num"), &m_nUseRecipeNum);

	for(long idx = 0; idx < m_vecstrUseRecipe.size(); idx++)
	{
		strTemp1.Format(_T("%d"), idx);
		HUBSDK::IniReadWriter::Write(strPath, _T("PLC-Recipe"), strTemp1, &m_vecstrUseRecipe[idx]);
	}

	m_sPLC.nPLC2VisionCnt = (long)m_sPLC.vecsPLC2VisionData.size();

	HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), _T("PLC2Vision Start Address"), &m_sPLC.nPLC2VisionStartAddr);
	HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), _T("PLC2Vision Count"), &m_sPLC.nPLC2VisionCnt);

	for(long data = 0; data < m_sPLC.nPLC2VisionCnt; data++)
	{
		strTemp2.Format(_T("Read_%d_Name"), data);
		HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsPLC2VisionData[data].strName);
		strTemp2.Format(_T("Read_%d_Address"), data);
		HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsPLC2VisionData[data].nAddr);
		strTemp2.Format(_T("Read_%d_Type"), data);
		HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsPLC2VisionData[data].nType);
		strTemp2.Format(_T("Read_%d_BitNum"), data);
		HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsPLC2VisionData[data].nBitNum);
	}

	m_sPLC.nVision2PLCCnt = (long)m_sPLC.vecsVision2PLCData.size();

	HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), _T("Vision2PLC Start Address"), &m_sPLC.nVision2PLCStartAddr);
	HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), _T("Vision2PLC Count"), &m_sPLC.nVision2PLCCnt);

	for(long data = 0; data < m_sPLC.nVision2PLCCnt; data++)
	{
		strTemp2.Format(_T("Write_%d_Name"), data);
		HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsVision2PLCData[data].strName);
		strTemp2.Format(_T("Write_%d_Address"), data);
		HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsVision2PLCData[data].nAddr);
		strTemp2.Format(_T("Write_%d_Type"), data);
		HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsVision2PLCData[data].nType);
		strTemp2.Format(_T("Write_%d_BitNum"), data);
		HUBSDK::IniReadWriter::Write(strPath, _T("Vision PLC"), strTemp2, &m_sPLC.vecsVision2PLCData[data].nBitNum);
	}

	return TRUE;
}

BOOL CPLCManager::SaveUseRecipe(const CString strPath)
{
	CString strTemp1, strTemp2;

	m_nUseRecipeNum = (long)m_vecstrUseRecipe.size();

	HUBSDK::IniReadWriter::Write(strPath, _T("PLC-Recipe"), _T("Use Recipe Num"), &m_nUseRecipeNum);

	for(long idx = 0; idx < m_vecstrUseRecipe.size(); idx++)
	{
		strTemp1.Format(_T("%d"), idx);
		HUBSDK::IniReadWriter::Write(strPath, _T("PLC-Recipe"), strTemp1, &m_vecstrUseRecipe[idx]);
	}

	return TRUE;
}

void CPLCManager::GetReadData()
{
	::g_PLCSocketManager.m_strBarcode = GetBarcode();
	::g_PLCSocketManager.m_strModel = GetModel();
	::g_PLCSocketManager.m_nCameraIndex = GetCamIndex();
	::g_PLCSocketManager.m_nDivIndex = GetDivIndex() - 1;
	::g_PLCSocketManager.m_bStart = GetScanStart();
	::g_PLCSocketManager.m_nJobNumber = GetJobNumber();
	::g_PLCSocketManager.m_bScanDone = GetScanDone();
	::g_PLCSocketManager.m_bInitialize = GetInitialize();
}

void  CPLCManager::GetReadDataSimul(int nCameraIndex, int nDivIndex, int nJobNumber, bool bScanDone)
{
	::g_PLCSocketManager.m_strBarcode = _T("NoBCR");
	::g_PLCSocketManager.m_strModel = _T("NoBCR");
	::g_PLCSocketManager.m_nCameraIndex = nCameraIndex;
	::g_PLCSocketManager.m_nDivIndex = nDivIndex;
	::g_PLCSocketManager.m_bStart = FALSE;
	::g_PLCSocketManager.m_nJobNumber = nJobNumber;
	::g_PLCSocketManager.m_bScanDone = bScanDone;
	::g_PLCSocketManager.m_bInitialize = FALSE;
}

CString CPLCManager::GetBarcode()
{
	CString strBarcode = _T("");

	long startAdd = (long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_LotID_01;
	long endAdd = (long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_LotID_10;

	for(long idx = startAdd; idx <= endAdd; idx++)
	{
		char szString[STRING_MAX] = {0,};
		memcpy(szString, &m_sPLC.vecsPLC2VisionData[idx].sValue, 2);
		strBarcode += (CString)szString;
	}

	if(strBarcode == _T(""))
	{
		strBarcode = _T("BarcodeEmpty");
	}

	return strBarcode;
}

CString CPLCManager::GetModel()
{
	CString strModel = _T("");

	long startAdd = (long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_ModelName_01;
	long endAdd = (long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_ModelName_10;

	for(long idx = startAdd; idx <= endAdd; idx++)
	{
		char szString[STRING_MAX] = {0,};
		memcpy(szString, &m_sPLC.vecsPLC2VisionData[idx].sValue, 2);
		strModel += (CString)szString;
	}

	return strModel;
}

long CPLCManager::GetCamIndex()
{
	long nCameraIndex = (long)m_sPLC.vecsPLC2VisionData[(long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_CameraIndex].sValue;

	return nCameraIndex;
}

long CPLCManager::GetDivIndex()
{
	long nDivIndex = (long)m_sPLC.vecsPLC2VisionData[(long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_DivIndex].sValue;

	return nDivIndex;
}

BOOL CPLCManager::GetScanStart()
{
	BOOL bStart = (BOOL)m_sPLC.vecsPLC2VisionData[(long)BORGWARNER::P2V_ADDR_NAME::PLC_DI_REQ_Insp_Start].sValue;

	return bStart;
}

long CPLCManager::GetJobNumber()
{
	long nJobNumber = (long)m_sPLC.vecsPLC2VisionData[(long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_JobNumber].sValue;

	return nJobNumber;
}

BOOL CPLCManager::GetScanDone()
{
	BOOL bScanDone = (BOOL)m_sPLC.vecsPLC2VisionData[(long)BORGWARNER::P2V_ADDR_NAME::PLC_DI_RESP_Insp_Move].sValue;

	return bScanDone;
}

BOOL CPLCManager::GetInitialize()
{
	BOOL bInitialize = (BOOL)m_sPLC.vecsPLC2VisionData[(long)BORGWARNER::P2V_ADDR_NAME::PLC_DI_REQ_Initialize].sValue;

	return bInitialize;
}

////////////////////////////////////////////////////////////////////////////////
// 장비마다 결과 전송 주소 및 Data 가 다르므로 함수 분리
// Borgwarner 향 버스바 용점기 전용 SetResultData
void CPLCManager::SetResultData_forBorgwarner(std::vector<SResultParams> vecResult, std::vector<SCriteriaParams> vecCriteriaResult, int idivIndex)
{
	::g_Log.Log(_T("SetResultData_forBorgwarner Start"));
	if((long)vecResult.size() <=0)
	{
		return;
	}

	long camIndex = vecResult[0].visionIndex;
	long divIndex = vecResult[0].divIndex;

	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[camIndex].algoSequence;

	float fResolutionX = ::g_CalibrationManager.m_fPixelResolutionX[camIndex];
	float fResolutionY = ::g_CalibrationManager.m_fPixelResolutionY[camIndex];

	// PLC에서 받아온 스테이지 좌표
	float fStageX = m_sPLC.vecsPLC2VisionData[(long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_Scan_Pos_X].nValue / 10000.f;
	float fStageY = m_sPLC.vecsPLC2VisionData[(long)BORGWARNER::P2V_ADDR_NAME::PLC_IN_Scan_Pos_Y].nValue / 10000.f;
	//float fStageX = 100;
	//float fStageY = 100;

	float fCenterX = (float)::g_ConfigManager.m_nImageWidth[camIndex] / 2;
	float fCenterY = (float)::g_ConfigManager.m_nImageHeight[camIndex] / 2;

	float fOffsetY = ::g_ConfigManager.m_fDistToCamera;
	float fHalfOffsetY = fOffsetY / 2.f;
	if(camIndex == 1)
	{
		fHalfOffsetY *= -1.f;
	}

	// 임시
	m_fPLCPosX[divIndex] = fStageX;
	m_fPLCPosY[divIndex] = fStageY;

	// 임시
	// 기존 기준 위치...		float fX = 201.84f;		float fY = 833.86f;

	// 새 기준 위치
	float fX = 201.84f;
	float fY = 1233.86f;//float fY = 1033.86f;   //4733
	::g_Log.Log(_T("SetResultData_forBorgwarner 임시좌표 입력"));

	float fXPLC = m_fPLCPosX[0] + ((fX - fCenterX) * fResolutionX); // 임시 기준 위치 X 좌표
	float fYPLC = m_fPLCPosY[0] + ((fY - fCenterY) * fResolutionY);	// 임시 기준 위치 Y 좌표

	// 기존 버스바돠 동일하게 각 포인트마다 X, Y, Z만 보내도록 고정
	// DATA_COUNT == 6으로 고정됨 (start, end)
	// 2D 검사이므로 Z값은 존재하지 않아 Z값 작성 위치에는 0 작성
	// Start.x, Start.y, Start.z, End.x, End.y, End.z
	float fData[(long)BORGWARNER::DATA::DATA_COUNT] = {0,};

	// 임시 각 핀마다 간격 IWS와 30로 협의
	long dataCount(15);
	long startIdx(0);

	switch(camIndex)
	{
	case (long)CAMERA_INDEX::CAMERA1:
	{
		switch(divIndex)
		{
		case 0: startIdx = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_000; break;
		case 1: startIdx = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_045; break;
		case 2: startIdx = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_090; break;
		}
	}
	break;
	case (long)CAMERA_INDEX::CAMERA2:
	{
		switch(divIndex)
		{
		case 0: startIdx = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_135; break;
		case 1: startIdx = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_150; break;
		case 2: startIdx = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_165; break;
		}
	}
	break;
	}

	long startAddr = m_sPLC.vecsVision2PLCData[startIdx].nAddr;

	BOOL bResult = TRUE; // 1 : OK, 2 : NG

	// 임시
	// cam0 -> 1개짜리, cam1 -> 3개짜리
	std::vector<SPLCData> vecsVision2PLCData;
	long index(startIdx);
	if((long)vecResult.size() != 1 && (long)vecResult.size() != 3)
	{
		::g_Log.Log(_T("SetResultData_forBorgwarner ------------  bResult = FALSE"));
		bResult = FALSE;

		for(long i = 0; i < dataCount; i++)
		{
			SPLCData data;
			data.Init();

			data.nAddr = startAddr + i;
			data.nType = (long)PLC_MEMORY_TYPE::DOUBLE;
			data.nValue = 0;

			vecsVision2PLCData.push_back(data);
			m_sPLC.vecsVision2PLCData[index].nValue = data.nValue;

			index++;
		}
	}
	else
	{
		for(long idx = 0; idx < (long)vecResult.size(); idx++)
		{
			long dataIndex(0);

			CPoint start(0, 0), end(0, 0);
			float length(0.f), angle(0.f);
			switch(algoSequence)
			{
			case (long)ALGORITHM_SEQUENCE::EDGE_DETECT:
				start = vecResult[idx].ptStartResult;
				end = vecResult[idx].ptEndResult;
				length = vecResult[idx].fEdgeLength;
				angle = vecResult[idx].fEdgeAngle;
				break;
			case (long)ALGORITHM_SEQUENCE::DETECT_BLOB:
				start = vecResult[idx].ptBlobStart;//start = 1024;
				end = vecResult[idx].ptBlobEnd;//end = 2056;
				length = vecResult[idx].fBlobLength;
				angle = vecResult[idx].fBlobAngle;
				break;
			case (long)ALGORITHM_SEQUENCE::DETECT_BLOB2: // 후검사는 PLC에 전송하지 않는다.
				break;
			}

			bResult &= (vecCriteriaResult[idx].nErr == INSP_ERR_OK) ? TRUE : FALSE;

			::g_Log.Log(_T("====>start  : %d end : %d "), vecResult[idx].ptBlobStart, vecResult[idx].ptBlobEnd);

			fData[dataIndex] = fStageX + ((start.x - fCenterX) * fResolutionX);
			fData[dataIndex] = (fData[dataIndex] - fXPLC) * 100.f;
			dataIndex++;

			fData[dataIndex] = fStageY + ((start.y - fCenterY) * fResolutionY);
			if(camIndex == (long)CAMERA_INDEX::CAMERA2)
			{
				fData[dataIndex] = fData[dataIndex] + fOffsetY;
			}
			// 기존 기준 위치 기준 fData[dataIndex] = (fData[dataIndex] - fYPLC) * 100.f;
			fData[dataIndex] = (fYPLC - fData[dataIndex]) * 100.f;	// 새 기준위치 기준
			dataIndex++;

			fData[dataIndex] = 0;
			dataIndex++;

			fData[dataIndex] = fStageX + ((end.x - fCenterX) * fResolutionX);
			fData[dataIndex] = (fData[dataIndex] - fXPLC) * 100.f;
			dataIndex++;

			fData[dataIndex] = fStageY + ((end.y - fCenterY) * fResolutionY);
			if(camIndex == (long)CAMERA_INDEX::CAMERA2)
			{
				fData[dataIndex] = fData[dataIndex] + fOffsetY;
			}
			fData[dataIndex] = (fYPLC - fData[dataIndex]) * 100.f;	// 새 기준위치 기준
			// 기존 기준위치 기준 fData[dataIndex] = (fData[dataIndex] - fYPLC) * 100.f;

			dataIndex++;

			fData[dataIndex] = 0;
			dataIndex++;

			for(long i = 0; i < dataCount; i++)
			{
				SPLCData data;
				data.Init();

				if(0 <= i && i < dataIndex)
				{
					data.nAddr = startAddr + i;
					data.nType = (long)PLC_MEMORY_TYPE::DOUBLE;
					data.nValue = (long)fData[i];

					vecsVision2PLCData.push_back(data);
					m_sPLC.vecsVision2PLCData[index].nValue = data.nValue;
				}
				else
				{
					data.nAddr = startAddr + i;
					data.nType = (long)PLC_MEMORY_TYPE::DOUBLE;
					data.nValue = 0;

					vecsVision2PLCData.push_back(data);
					m_sPLC.vecsVision2PLCData[index].nValue = data.nValue;
				}
				index++;
			}
		}
	}

	short nResult = (bResult == TRUE) ? 1 : 2; // 1 : OK, 2 : NG 

	// 검사결과...
	// Data와 PLC Address 시작주소를 넘겨주면 한번에 쓰자
	::g_PLCSocketManager.m_pCtrlPLC->WriteDatasDouble(vecsVision2PLCData, startAddr);
	//////////////////////////////////////////////////////////////////////////////////////////////////
	/*m_nJudge = m_nJudge + nResult;

	if (camIndex == 0)
	{
	::g_Log.Log(_T("SetResultData_forBorgwarner:: cam1 -  m_nJudge = %d, nResult = %d, bResult = %d"), m_nJudge, nResult, bResult);
	::g_Log.Log(_T("SetResultData_forBorgwarner:: cam1 -  %d  = %d + %d"), m_nJudge, m_nJudge, nResult);
	}
	else
	{
		::g_Log.Log(_T("SetResultData_forBorgwarner:: cam2 -  m_nJudge = %d, nResult = %d, bResult = %d"), m_nJudge, nResult, bResult);
		::g_Log.Log(_T("SetResultData_forBorgwarner:: cam2 -  %d  = %d + %d"), m_nJudge, m_nJudge, nResult);
	}

	if (idivIndex == 0)
	{
		if (m_nJudge == 1)	nResult = 1;
	}
	else if (idivIndex == 1)
	{
		if (m_nJudge == 2)	nResult = 1;
	}
	else if (idivIndex == 2)
	{
		if (m_nJudge == 3)	nResult = 1;
		m_nJudge = 0;
	}

	if (camIndex == 0)
	{
		::g_Log.Log(_T("SetResultData_forBorgwarner:: cam1 -   --------------------------------"));
		::g_Log.Log(_T("SetResultData_forBorgwarner:: cam1 -   Reset m_nJudge = %d, Final nResult = %d"), m_nJudge, nResult);
	}
	else
	{
		::g_Log.Log(_T("SetResultData_forBorgwarner:: cam2 -   --------------------------------"));
		::g_Log.Log(_T("SetResultData_forBorgwarner:: cam2 -   Reset m_nJudge = %d, Final nResult = %d"), m_nJudge, nResult);
	}*/

	//////////////////////////////////////////////////////////////////////////////////////////////////


	// 검사결과...
	// Data와 PLC Address 시작주소를 넘겨주면 한번에 쓰자
	//::g_PLCSocketManager.m_pCtrlPLC->WriteDatasDouble(vecsVision2PLCData, startAddr);

	auto &plcData = m_sPLC.vecsVision2PLCData;
	if(::g_ConfigManager.m_bPLCAutoPass != FALSE) // PLC에 강제로 AutoPass 기능 적용 250228 PSJ
	{
		// 기존 방식
		::g_PLCSocketManager.m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_InspRet], (short)::g_Inspection.m_lTotalResult);
		::g_Log.Log(_T("PLC ::g_Inspection.m_lTotalResult, : %d"), ::g_Inspection.m_lTotalResult);
	}
	else
	{
		// Auto Pass 방식
		::g_PLCSocketManager.m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_InspRet], 1 /*(short)::g_Inspection.m_lTotalResult*/);
		::g_Log.Log(_T("PLC ::g_Inspection.m_lTotalResult, : %d [Applying AutoPass]"), ::g_Inspection.m_lTotalResult);
	}
	
	::g_PLCSocketManager.m_pCtrlPLC->WriteData(plcData[(long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_JobNumber], (short)::g_PLCSocketManager.m_nJobNumber);

}
////////////////////////////////////////////////////////////////////////////////