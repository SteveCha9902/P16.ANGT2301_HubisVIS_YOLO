#pragma once


#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class CCtrlPLC_Melsec;

class ALGORITHM_TOOLS_API CCtrlPLC
{
public:
	CCtrlPLC();
	~CCtrlPLC();

public:
	CCtrlPLC_Melsec* m_pMelsec;

	void Initialize();
	BOOL IsConnect();
	BOOL Connect();
	BOOL Disconnect();
	
	short* m_pReadPLC2VisionData;
	short* m_pWritePLC2VisionData;
	short* m_pReadVision2PLCData;
	short* m_pWriteVision2PLCData;
	int* m_pWriteVision2PLCData_Double;
	
	// PLC -> Vision Data Read, Write관련(전체 읽기, 쓰기)
	BOOL ReadPLC2VisionData();
	BOOL WritePLC2VisionData();

	// Vision -> PLC Data Read, Write관련(전체 읽기, 쓰기)
	BOOL ReadVision2PLCData();
	BOOL WriteVision2PLCData();

	// 개별 Address 에 읽기, 쓰기
	BOOL ReadData(SPLCData &data);
	BOOL WriteData(SPLCData &data, const short value);
	BOOL WriteDataDouble(SPLCData &data, const long value);
	BOOL WriteDatasDouble(std::vector<SPLCData> vecData, long nStartAddr);

	BOOL GetPLCData(short* pData, long nStartAddr, std::vector<SPLCData> &vecData);
	BOOL GetPLCData(short* pData, SPLCData &data);
	BOOL SetPLCData(std::vector<SPLCData> vecData, long nStartAddr, short* pData);
	BOOL SetPLCData(std::vector<SPLCData> vecData, long nStartAddr, int* pData);
	BOOL SetPLCData(SPLCData data, short &pData);
	BOOL SetPLCDataDouble(SPLCData data, int &pData);
	BOOL SetPLCDatasDouble(std::vector<SPLCData> vecData, int* pData);
};

