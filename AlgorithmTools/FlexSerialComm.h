#pragma once

#include "FlexLibComm.h"

class CSerialComm
{
public:
	CSerialComm();
	~CSerialComm();

	CFlexLibComm *m_pComm;

	BOOL m_bOpened;

	BOOL Open(UINT nPort, UINT nBaudrate, UINT nDataBits, UINT nStopBits, UINT nParity);
	void Close();

	void Set(CString data, long size);

	BOOL IsOpened();
};
