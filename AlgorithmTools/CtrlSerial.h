#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class CSerialComm;

class ALGORITHM_TOOLS_API CCtrlSerial
{
public:
	CCtrlSerial();
	~CCtrlSerial();

	CSerialComm* m_pSerialComm;

	BOOL Open(UINT nPort, UINT nBaudrate, UINT nDataBits, UINT nStopBits, UINT nParity);
	void Close();
	BOOL IllumOn(const long channel, const long illumValue);
	BOOL IllumOff(const long channel);
	void SetIllum(const CString strIllum);
	BYTE GetIllum(const CString strIllum);
	BOOL IsOpen();
	CString GetIllumValue(const long illum);
};

extern ALGORITHM_TOOLS_API CCtrlSerial g_CtrlSerial;

