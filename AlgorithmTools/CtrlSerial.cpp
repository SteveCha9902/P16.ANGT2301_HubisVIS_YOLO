#include "pch.h"
#include "CtrlSerial.h"
#include "FlexSerialComm.h"

CCtrlSerial g_CtrlSerial;

CCtrlSerial::CCtrlSerial()
	: m_pSerialComm(new CSerialComm)
{
}

CCtrlSerial::~CCtrlSerial()
{
	if(IsOpen() == TRUE)
	{
		for(long ch = 0; ch < ILLUM_CHANNEL_MAX; ch++)
		{
			IllumOff(ch + 1);
		}

		Close();
	}

	delete m_pSerialComm;
}


BOOL CCtrlSerial::Open(UINT nPort, UINT nBaudrate, UINT nDataBits, UINT nStopBits, UINT nParity)
{
	return m_pSerialComm->Open(nPort, nBaudrate, nDataBits, nStopBits, nParity);
}

void CCtrlSerial::Close()
{
	m_pSerialComm->Close();
}

BOOL CCtrlSerial::IllumOn(const long channel, const long illumValue)
{
	if(IsOpen() == FALSE)
	{
		return FALSE;
	}

	float factor = 255.f / 1023.f;
	long value = (long)((float)illumValue / factor);

	CString str;

	str.Format(_T("%c%d%s%s%c"), 0x02, channel, "w", GetIllumValue(value), 0x03);

	// Illum On
	SetIllum(str);

	return TRUE;
}

BOOL CCtrlSerial::IllumOff(const long channel)
{
	if(IsOpen() == FALSE)
	{
		return FALSE;
	}

	CString str;

	str.Format(_T("%c%d%s%s%c"), 0x02, channel, "w", GetIllumValue(0), 0x03);

	// Illum Off
	SetIllum(str);

	return TRUE;
}

void CCtrlSerial::SetIllum(const CString strIllum)
{
	if(IsOpen() == FALSE)
	{
		return;
	}

	BYTE pbyIllum[50];

	for(long i = 0; i < strIllum.GetLength(); i++)
	{
		pbyIllum[i] = (BYTE)strIllum[i];
	}
	pbyIllum[strIllum.GetLength()] = '\0';

	Sleep(20);

	m_pSerialComm->Set(strIllum, strIllum.GetLength());
}

BYTE CCtrlSerial::GetIllum(const CString strIllum)
{
	BYTE value(0);

	if(IsOpen() == FALSE)
	{
		return value;
	}


	return value;
}

BOOL CCtrlSerial::IsOpen()
{
	return m_pSerialComm->m_bOpened;
}

CString CCtrlSerial::GetIllumValue(const long illum)
{
	CString result;

	if (illum < 10)
	{
		result.Format(_T("000%d"), illum);
	}
	else if (illum >= 10 && illum < 100)
	{
		result.Format(_T("00%d"), illum);
	}
	else if (illum >= 100 && illum < 1000)
	{
		result.Format(_T("0%d"), illum);
	}
	else
	{
		result.Format(_T("%d"), illum);
	}

	return result;
}