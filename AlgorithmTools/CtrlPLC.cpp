#include "pch.h"
#include "CtrlPLC.h"

#include "CtrlPLC_Melsec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCtrlPLC::CCtrlPLC()
	: m_pMelsec(nullptr)
	, m_pReadPLC2VisionData(nullptr)
	, m_pWritePLC2VisionData(nullptr)
	, m_pReadVision2PLCData(nullptr)
	, m_pWriteVision2PLCData(nullptr)
	, m_pWriteVision2PLCData_Double(nullptr)
{
}

CCtrlPLC::~CCtrlPLC()
{
	Disconnect();

	delete m_pMelsec;
	delete[] m_pReadPLC2VisionData;
	delete[] m_pWritePLC2VisionData;
	delete[] m_pReadVision2PLCData;
	delete[] m_pWriteVision2PLCData;
	delete[] m_pWriteVision2PLCData_Double;
}

void CCtrlPLC::Initialize()
{
	m_pReadPLC2VisionData = new short[PLC_ADDRESS_MAX];
	m_pWritePLC2VisionData = new short[PLC_ADDRESS_MAX];
	m_pReadVision2PLCData = new short[PLC_ADDRESS_MAX];
	m_pWriteVision2PLCData = new short[PLC_ADDRESS_MAX];
	m_pWriteVision2PLCData_Double = new int[PLC_ADDRESS_MAX];

	memset(m_pReadPLC2VisionData, 0, sizeof(short) * PLC_ADDRESS_MAX);
	memset(m_pWritePLC2VisionData, 0, sizeof(short) * PLC_ADDRESS_MAX);
	memset(m_pReadVision2PLCData, 0, sizeof(short) * PLC_ADDRESS_MAX);
	memset(m_pWriteVision2PLCData, 0, sizeof(short) * PLC_ADDRESS_MAX);
	memset(m_pWriteVision2PLCData_Double, 0, sizeof(int) * PLC_ADDRESS_MAX);

	m_pMelsec = new CCtrlPLC_Melsec;
}

BOOL CCtrlPLC::IsConnect()
{
	return m_pMelsec->IsConnectPLC();
}

BOOL CCtrlPLC::Connect()
{
	m_pMelsec->Initialize(::g_ConfigManager.m_strPLCIP, ::g_ConfigManager.m_nPLCPort);

	if(m_pMelsec->PLC_Connect() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CCtrlPLC::Disconnect()
{
	if(m_pMelsec->PLC_DisConnect() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PLC -> Vision Data Read, Write관련
BOOL CCtrlPLC::ReadPLC2VisionData()
{
	auto &plc = ::g_PLCManager.m_sPLC;
	auto &data = plc.vecsPLC2VisionData;

	long dataCount = data[(long)data.size() - 1].nAddr - data[0].nAddr;
	for(long idx = 0; idx < (long)data.size(); idx++)
	{
		if(data[idx].nType == (long)PLC_MEMORY_TYPE::DOUBLE)
		{
			dataCount++;
		}
	}

	memset(m_pReadPLC2VisionData, 0, sizeof(short) * PLC_ADDRESS_MAX);

	//Config에서 설정한 주소와 수
	if(m_pMelsec->ReadPLC(plc.nPLC2VisionStartAddr, dataCount, m_pReadPLC2VisionData) == FALSE)
	{
		return FALSE;
	}

	GetPLCData(m_pReadPLC2VisionData, plc.nPLC2VisionStartAddr, plc.vecsPLC2VisionData);

	::g_PLCManager.GetReadData();

	return TRUE;
}

BOOL CCtrlPLC::WritePLC2VisionData()
{
	auto &data = ::g_PLCManager.m_sPLC;

	memset(m_pWritePLC2VisionData, 0, sizeof(short) * PLC_ADDRESS_MAX);

	SetPLCData(data.vecsPLC2VisionData, data.nPLC2VisionStartAddr, m_pWritePLC2VisionData);

	//Config에서 설정한 주소와 수
	if(m_pMelsec->WritePLC(data.nPLC2VisionStartAddr, data.nPLC2VisionCnt, m_pWritePLC2VisionData) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// Vision -> PLC Data Read, Write관련
BOOL CCtrlPLC::ReadVision2PLCData()
{
	auto &plc = ::g_PLCManager.m_sPLC;
	auto &data = plc.vecsVision2PLCData;

	long dataCount = data[(long)data.size() - 1].nAddr - data[0].nAddr;
	for(long idx = 0; idx < (long)data.size(); idx++)
	{
		if(data[idx].nType == (long)PLC_MEMORY_TYPE::DOUBLE)
		{
			dataCount++;
		}
	}

	memset(m_pReadVision2PLCData, 0, sizeof(short) * PLC_ADDRESS_MAX);

	//Config에서 설정한 주소와 수
	if(m_pMelsec->ReadPLC(plc.nVision2PLCStartAddr, dataCount, m_pReadVision2PLCData) == FALSE)
	{
		return FALSE;
	}

	GetPLCData(m_pReadVision2PLCData, plc.nVision2PLCStartAddr, plc.vecsVision2PLCData);

	::g_PLCManager.GetReadData();

	return TRUE;
}

BOOL CCtrlPLC::WriteVision2PLCData()
{
	switch(::g_ConfigManager.m_nCustomer)
	{
	case (long)CUSTOMER::BORGWARNER:
	{
		// 30300번대 가 Read, 30200번대가 Write, 31000번 부터 Data Write 이므로
		// 한번에 다 써버리면 Read Address에 Write 할 수 있어 나눠서 Write 한다.
		auto &data = ::g_PLCManager.m_sPLC;

		memset(m_pWriteVision2PLCData, 0, sizeof(short) * PLC_ADDRESS_MAX);

		SetPLCData(data.vecsVision2PLCData, data.nVision2PLCStartAddr, m_pWriteVision2PLCData);

		long startIndex = (long)BORGWARNER::V2P_ADDR_NAME::PLC_DO_LinkTest;
		long dataCount = (long)BORGWARNER::V2P_ADDR_NAME::PLC_OUT_JobNumber - startIndex + 1;

		//Config에서 설정한 주소와 수
		if(m_pMelsec->WritePLC(data.nVision2PLCStartAddr, dataCount, m_pWriteVision2PLCData) == FALSE)
		{
			return FALSE;
		}

		memset(m_pWriteVision2PLCData_Double, 0, sizeof(short) * PLC_ADDRESS_MAX);

		startIndex = ::g_PLCManager.m_sPLC.vecsVision2PLCData[(long)BORGWARNER::V2P_ADDR_NAME::ResultData_000].nAddr;
		dataCount = (long)BORGWARNER::V2P_ADDR_NAME::ResultData_181 - (long)BORGWARNER::V2P_ADDR_NAME::ResultData_000 + 1;

		SetPLCData(data.vecsVision2PLCData, startIndex, m_pWriteVision2PLCData_Double);

		if(m_pMelsec->WritePLC(startIndex, dataCount, m_pWriteVision2PLCData_Double) == FALSE)
		{
			return FALSE;
		}
	}
	break;
	default:
	{
		auto &data = ::g_PLCManager.m_sPLC;

		memset(m_pWriteVision2PLCData, 0, sizeof(short) * PLC_ADDRESS_MAX);

		SetPLCData(data.vecsVision2PLCData, data.nVision2PLCStartAddr, m_pWriteVision2PLCData);

		//Config에서 설정한 주소와 수
		if(m_pMelsec->WritePLC(data.nVision2PLCStartAddr, data.nVision2PLCCnt, m_pWriteVision2PLCData) == FALSE)
		{
			return FALSE;
		}
	}
	break;
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// 개별 Address 에 읽기, 쓰기
BOOL CCtrlPLC::ReadData(SPLCData &data)
{
	short buffer(0);

	//Config에서 설정한 주소와 수
	if(m_pMelsec->ReadPLC(data.nAddr, 1, &buffer) == FALSE)
	{
		return FALSE;
	}

	GetPLCData(&buffer, data);

	return TRUE;
}

BOOL CCtrlPLC::WriteData(SPLCData &data, const short value)
{
	short buffer(0);

	data.sValue = value;

	SetPLCData(data, buffer);

	//Config에서 설정한 주소와 수
	if(m_pMelsec->WritePLC(data.nAddr, 1, &buffer) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CCtrlPLC::WriteDataDouble(SPLCData &data, const long value)
{
	int buffer(0);

	data.nValue = value;

	SetPLCDataDouble(data, buffer);

	//Config에서 설정한 주소와 수
	if(m_pMelsec->WritePLC(data.nAddr, 1, &buffer) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CCtrlPLC::WriteDatasDouble(std::vector<SPLCData> vecData, long nStartAddr)
{
	auto dataCount = (long)vecData.size();

	memset(m_pWriteVision2PLCData_Double, 0, sizeof(short) * PLC_ADDRESS_MAX);

	SetPLCDatasDouble(vecData, m_pWriteVision2PLCData_Double);

	//Config에서 설정한 주소와 수
	if(m_pMelsec->WritePLC(nStartAddr, dataCount, m_pWriteVision2PLCData_Double) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////

BOOL CCtrlPLC::GetPLCData(short* pData, long nStartAddr, std::vector<SPLCData> &vecData)
{
	//PLC에서 받아온 Buffer를 Vector 값에 입력
	long bit = 0;

	for(long idx = 0; idx < vecData.size(); idx++)
	{
		if(vecData[idx].nType == (long)PLC_MEMORY_TYPE::BIT)
		{
			auto bit = pData[vecData[idx].nAddr - nStartAddr] & (1 << vecData[idx].nBitNum);

			vecData[idx].sValue = (bit == 0) ? 0 : 1;
		}
		else if(vecData[idx].nType == (long)PLC_MEMORY_TYPE::INT || vecData[idx].nType == (long)PLC_MEMORY_TYPE::WORD)
		{
			vecData[idx].sValue = pData[vecData[idx].nAddr - nStartAddr];
		}
		else if(vecData[idx].nType == (long)PLC_MEMORY_TYPE::DOUBLE)
		{
			long nValue(0);
			memcpy(&nValue, &pData[vecData[idx].nAddr - nStartAddr], 2 * sizeof(short));

			vecData[idx].nValue = (nValue);
		}
	}

	return TRUE;
}

BOOL CCtrlPLC::GetPLCData(short* pData, SPLCData &data)
{
	//PLC에서 받아온 Buffer를 Vector 값에 입력
	if(data.nType == (long)PLC_MEMORY_TYPE::BIT)
	{
		auto bit = pData[data.nAddr] & (1 << data.nBitNum);

		data.sValue = (bit == 0) ? 0 : 1;
	}
	else if(data.nType == (long)PLC_MEMORY_TYPE::INT || data.nType == (long)PLC_MEMORY_TYPE::WORD)
	{
		data.sValue = pData[data.nAddr];
	}
	else if(data.nType == (long)PLC_MEMORY_TYPE::DOUBLE)
	{
		long nValue(0);
		memcpy(&nValue, &pData[data.nAddr], 2 * sizeof(short));

		data.nValue = (nValue);
	}

	return TRUE;
}

BOOL CCtrlPLC::SetPLCData(std::vector<SPLCData> vecData, long nStartAddr, short* pData)
{
	//Vector 값을 PLC에서 전달할 Buffer에 입력
	for(long idx = 0; idx < vecData.size(); idx++)
	{
		if(vecData[idx].nType == (long)PLC_MEMORY_TYPE::BIT)
		{
			if(vecData[idx].sValue == 1)
				pData[vecData[idx].nAddr - nStartAddr] = pData[vecData[idx].nAddr - nStartAddr] | (1 << vecData[idx].nBitNum);
			else
				pData[vecData[idx].nAddr - nStartAddr] = pData[vecData[idx].nAddr - nStartAddr] & (~(1 << vecData[idx].nBitNum));
		}
		else if(vecData[idx].nType == (long)PLC_MEMORY_TYPE::INT || vecData[idx].nType == (long)PLC_MEMORY_TYPE::WORD)
		{
			pData[vecData[idx].nAddr - nStartAddr] = vecData[idx].sValue;
		}
		else if(vecData[idx].nType == (long)PLC_MEMORY_TYPE::DOUBLE)
		{
			pData[vecData[idx].nAddr - nStartAddr] = short(vecData[idx].nValue);
		}
	}

	return TRUE;
}

BOOL CCtrlPLC::SetPLCData(std::vector<SPLCData> vecData, long nStartAddr, int* pData)
{
	//Vector 값을 PLC에서 전달할 Buffer에 입력
	for (long idx = 0; idx < vecData.size(); idx++)
	{
		if (vecData[idx].nType == (long)PLC_MEMORY_TYPE::BIT)
		{
			if (vecData[idx].sValue == 1)
				pData[vecData[idx].nAddr - nStartAddr] = pData[vecData[idx].nAddr - nStartAddr] | (1 << vecData[idx].nBitNum);
			else
				pData[vecData[idx].nAddr - nStartAddr] = pData[vecData[idx].nAddr - nStartAddr] & (~(1 << vecData[idx].nBitNum));
		}
		else if (vecData[idx].nType == (long)PLC_MEMORY_TYPE::INT || vecData[idx].nType == (long)PLC_MEMORY_TYPE::WORD)
		{
			pData[vecData[idx].nAddr - nStartAddr] = vecData[idx].sValue;
		}
		else if (vecData[idx].nType == (long)PLC_MEMORY_TYPE::DOUBLE)
		{
			pData[vecData[idx].nAddr - nStartAddr] = vecData[idx].nValue;
		}
	}

	return TRUE;
}

BOOL CCtrlPLC::SetPLCData(SPLCData data, short &pData)
{
	if(data.nType == (long)PLC_MEMORY_TYPE::BIT)
	{
		if(data.sValue == 1)
			pData = pData | (1 << data.nBitNum);
		else
			pData = pData & (~(1 << data.nBitNum));
	}
	else if(data.nType == (long)PLC_MEMORY_TYPE::INT || data.nType == (long)PLC_MEMORY_TYPE::WORD)
	{
		pData = data.sValue;
	}
	else if(data.nType == (long)PLC_MEMORY_TYPE::DOUBLE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CCtrlPLC::SetPLCDataDouble(SPLCData data, int &pData)
{
	if(data.nType == (long)PLC_MEMORY_TYPE::DOUBLE)
	{
		pData = data.nValue;
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CCtrlPLC::SetPLCDatasDouble(std::vector<SPLCData> vecData, int* pData)
{
	//Vector 값을 PLC에서 전달할 Buffer에 입력
	for(long idx = 0; idx < vecData.size(); idx++)
	{
		pData[idx] = vecData[idx].nValue;
	}

	return TRUE;
}