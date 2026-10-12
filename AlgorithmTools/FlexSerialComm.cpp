#include "pch.h"
#include "FlexSerialComm.h"

UINT nDefDevice[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
UINT nDefBaudrate[] = {2400, 4800, 9600, 19200, 38400, 57600, 115200};
UINT nDefDataBits[] = {7, 8};
UINT nDefStopBits[] = {0, 1, 2};
UINT nDefParityBits[] = {0, 1, 2};

CSerialComm::CSerialComm()
{
	m_pComm = new CFlexLibComm;

	m_bOpened = FALSE;
}

CSerialComm::~CSerialComm()
{
	delete m_pComm;
}

BOOL CSerialComm::IsOpened()
{
	return m_bOpened;
}

//----------------------------------------------------------------------
BOOL CSerialComm::Open(UINT nPort, UINT nBaudrate, UINT nDataBits, UINT nStopBits, UINT nParity)
{
	BOOL rtn = FALSE;

	m_pComm->SetCommPort(nPort, nBaudrate, nDataBits, nStopBits, nParity);
	m_pComm->CreateComm();

	if(FALSE == m_pComm->OpenCommPort(m_pComm))
	{
		m_pComm->fConnected = FALSE;
		rtn = FALSE;
	}
	else
	{
		m_pComm->fConnected = TRUE;
		rtn = TRUE;
	}

	m_bOpened = rtn;

	return rtn;
}
//----------------------------------------------------------------------
void CSerialComm::Close()
{
	if(m_pComm->fConnected)
		m_pComm->CloseConnection();
}
//----------------------------------------------------------------------

void CSerialComm::Set(CString data, long size)
{
	char cSendData[100];
	
	memset(cSendData, 0, sizeof(cSendData));

	WideCharToMultiByte(CP_ACP, 0, data, -1, cSendData, 100, NULL, NULL); // String º¯È¯...

	m_pComm->WriteCommBlock(cSendData, size);
}

