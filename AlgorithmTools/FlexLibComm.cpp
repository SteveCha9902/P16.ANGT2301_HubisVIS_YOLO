#include "pch.h"
#include "FlexLibComm.h"

#define	TIME_OUT						80


#ifdef		_DEBUG_
#define new	DEBUG_NEW
#undef		THIS_FILE
static char THIS_FILE[] = _FILE_;
#endif


DWORD CommWatchProc(LPVOID lpData);

IMPLEMENT_DYNCREATE(CFlexLibComm, CObject);

unsigned char RcvData[MAXBLOCK + 1];



CFlexLibComm::CFlexLibComm()
{
	idCommDev = NULL;
	fConnected = FALSE;
}

CFlexLibComm::~CFlexLibComm()
{

}

void CFlexLibComm::SetCommPort(int port, DWORD rate, BYTE bytesize, BYTE stop, BYTE parity)
{
	bPort = port;
	dwBaudRate = rate;
	bByteSize = bytesize;
	bStopBits = stop;
	bParity = parity;
}

BOOL CFlexLibComm::CreateComm(void)
{
	osWrite.Offset = osWrite.OffsetHigh = 0;
	osRead.Offset = osRead.OffsetHigh = 0;
	osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(osRead.hEvent == NULL) return FALSE;

	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(osWrite.hEvent == NULL) 
	{
		CloseHandle(osRead.hEvent);
		return FALSE;
	}
	return TRUE;
}

BOOL CFlexLibComm::DestroyComm(void)
{
	if(fConnected) 
		CloseConnection();

	CloseHandle(osRead.hEvent);
	CloseHandle(osWrite.hEvent);

	return TRUE;
}

BOOL CFlexLibComm::OpenCommPort(LPVOID lpData)
{
	char szPort[20];
	BOOL fRetVal;
	COMMTIMEOUTS CommTimeOuts;

	memset(szPort, 0, sizeof(szPort));
	wsprintf((LPWSTR)szPort, _T("\\\\.\\COM%d"), bPort);

	idCommDev = CreateFile((LPWSTR)szPort, GENERIC_READ | GENERIC_WRITE, 0, 0, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL); 

	if(idCommDev == INVALID_HANDLE_VALUE) 
		return FALSE;

	SetCommMask(idCommDev, EV_RXFLAG);

	SetupComm(idCommDev, 8192, 8192);
	PurgeComm(idCommDev, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 1000;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 0;

	SetCommTimeouts(idCommDev, &CommTimeOuts);

	fRetVal = SetupConnection();

	if(fRetVal) 
	{
		fConnected = TRUE;
		CWinThread* pThread = AfxBeginThread((AFX_THREADPROC)CommWatchProc, (LPVOID)this);
		pThread->m_bAutoDelete = TRUE;
	}
	else 
	{
		fConnected = FALSE;
		CloseHandle(idCommDev);
	}

	return fRetVal;
}

BOOL CFlexLibComm::SetupConnection(void)
{
	DCB dcb;

	FillMemory(&dcb, sizeof(DCB), 0);
	dcb.DCBlength = sizeof(DCB);

	GetCommState(idCommDev, &dcb);

	dcb.BaudRate = dwBaudRate;
	dcb.ByteSize = bByteSize;
	dcb.Parity = bParity;
	//------------------------------
	if(bStopBits == 1)
 		dcb.StopBits = ONESTOPBIT;
	else if(bStopBits == 2)
		dcb.StopBits = TWOSTOPBITS;
	else
		dcb.StopBits = ONESTOPBIT;
	//------------------------------
	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fInX = dcb.fOutX = 0;
	dcb.XonLim = dcb.XoffLim = 100;
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.EvtChar = '\r';

	return SetCommState(idCommDev, &dcb);
}

BOOL CFlexLibComm::CloseConnection(void)
{
	fConnected = FALSE;
	SetCommMask(idCommDev, 0);
	EscapeCommFunction(idCommDev, CLRDTR);
	PurgeComm(idCommDev, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	CloseHandle(idCommDev);

	return TRUE;
}

BOOL CFlexLibComm::WriteCommBlock(LPSTR lpszBlock, DWORD dwBytesToWrite)
{
	DWORD dwBytesWritten;

	PurgeComm(idCommDev, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	if(!WriteFile(idCommDev, lpszBlock, dwBytesToWrite, &dwBytesWritten, &osWrite)) 
	{
		if(GetLastError() != ERROR_IO_PENDING) 
			return FALSE;
	}
	return TRUE;
}

int CFlexLibComm::ReadCommBlock(LPSTR lpszBlock, int nMaxLength)
{
	COMSTAT ComStat;
	DWORD dwLength;
	DWORD dwError, dwErrorFlags;

	ClearCommError(idCommDev, &dwErrorFlags, &ComStat);
	dwLength = ComStat.cbInQue;

	if(dwLength > 0) 
	{
		if(!ReadFile(idCommDev, lpszBlock, dwLength, &dwLength, &osRead)) 
		{
			if(GetLastError() == ERROR_IO_PENDING) 
			{
				while(!GetOverlappedResult(idCommDev, &osRead, &dwLength, TRUE))
				{
					dwError = GetLastError();

					if (dwError != ERROR_IO_INCOMPLETE)
					{
						ClearCommError(idCommDev, &dwErrorFlags, &ComStat);
						break;
					}
				}
			}
			else
			{
				dwLength = 0;
				ClearCommError(idCommDev, &dwErrorFlags, &ComStat);
			}
		}
	}
	return (int)dwLength;
}
//----------------------------------------------------------------------
void CFlexLibComm::OnCommDataRcv(void)
{
}
//----------------------------------------------------------------------
void CFlexLibComm::SetHwnd(HWND hWnd)
{
	m_hWnd = hWnd;
}
//----------------------------------------------------------------------
DWORD CommWatchProc(LPVOID lpData)
{
//	DWORD dwEvtMask;
	OVERLAPPED os;
	CFlexLibComm* npComm = (CFlexLibComm*)lpData;
	UINT nLength = 0;	//i, 
//	CHAR InData[MAXBLOCK + 1];//	UCHAR Data;
	char sDataBuff[MAXBLOCK + 1];
	UINT nByteNum = 0;
//	double dTestTimer;
	CString sTestTimer;
	int nBoardSeqNo = 0;
	int	nBaordDataCnt = 0;

	if((npComm == NULL) || !npComm->IsKindOf(RUNTIME_CLASS(CFlexLibComm))) 
		return (DWORD) -1;

	memset(&os, 0, sizeof(OVERLAPPED));
	os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	memset(sDataBuff, 0, sizeof(sDataBuff));

	if(os.hEvent == NULL) 
	{
		MessageBox(NULL, _T("Failed to create event for thread"), _T("Comm Error!"), MB_ICONEXCLAMATION | MB_OK);
		return (DWORD) -1;
	}

	if(!SetCommMask(npComm->idCommDev, EV_RXCHAR)) 
		return 1;

	//while(npComm->fConnected) 
	//{
	//	dwEvtMask = 0;
	//	memset(InData, 0, MAXBLOCK);

	//	WaitCommEvent(npComm->idCommDev, &dwEvtMask, NULL);

	//	if((dwEvtMask & EV_RXCHAR) == EV_RXCHAR) 
	//	{
	//		if((nLength = npComm->ReadCommBlock((LPSTR)InData, MAXBLOCK)) > 0) 
	//		{
	//			if(npComm->bPort == nDefDevice[CGlobal::GetInstance()->getBoardCommPort() ] )
	//			{
	//				for(i = 0; i < nLength; i++)
	//				{
	//					Data = (UCHAR)InData[i];

	//					switch (nBoardSeqNo)
	//					{
	//					case 0:
	//						if(Data == 0x02)
	//						{
	//							//CGlobal::GetInstance()->DataTimer.Start();
	//							nBoardSeqNo = 10;
	//						}
	//						else
	//						{
	//							memset(sDataBuff, 0, sizeof(sDataBuff) );
	//							nBaordDataCnt = 0;
	//							nBoardSeqNo = 0;
	//						}
	//						break;
	//					case 10:
	//						if(Data == 0x0D)
	//						{
	//							nBoardSeqNo = 100;
	//						}
	//						else
	//						{
	//							if(nBaordDataCnt < MAXBLOCK)
	//							{
	//								sDataBuff[nBaordDataCnt] = Data;
	//								nBaordDataCnt++;
	//							}
	//							else
	//							{
	//								memset(sDataBuff, 0, sizeof(sDataBuff) );
	//								nBaordDataCnt = 0;
	//								nBoardSeqNo = 0;
	//								CGlobal::GetInstance()->nCommErrCount++;
	//								if(CGlobal::GetInstance()->nCommErrCount > 3)
	//								{
	//									CGlobal::GetInstance()->nCommErrCount = 0;
	//									ShowError(Ecode_Comm_Alarm);
	//								}

	//							}
	//						}
	//						break;
	//					case 100:
	//						npComm->nLength = nBaordDataCnt;
	//						npComm->SetLoadcellReadData(sDataBuff);
	//						//dTestTimer = CGlobal::GetInstance()->DataTimer.Now();
	//						//sTestTimer.Format("%f", dTestTimer);
	//						//g_pMainDlg->axTestTimer.SetText(sTestTimer);
	//						memset(sDataBuff, 0, sizeof(sDataBuff) );
	//						nBaordDataCnt = 0;
	//						npComm->nLength  = 0;
	//						nBoardSeqNo = 0;
	//						break;
	//					}
	//				}
	//			}
	//		}
	//	}
	//}
	CloseHandle(os.hEvent); 
	return (DWORD)0;
}
//----------------------------------------------------------------------
