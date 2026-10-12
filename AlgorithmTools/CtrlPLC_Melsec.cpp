#include "pch.h"
#include "CtrlPLC_Melsec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning(disable:4996)
// CEthernetPLC

/**************************************************************/
//won 170908 Melsec PLC Ŀ�´����̼� ��������

//Sing Data Get, Set�� ���� ��ɾ�?����
#define PLC_SEND_ETHERNET_ERROR_RESET				_T("500000FF03FF00000C009016170000")
#define PLC_SEND_ETHERNET_SINGLE_BIT_READ			_T("500000FF03FF000018009004010001")
#define PLC_SEND_ETHERNET_SINGLE_BIT_WRITE			_T("500000FF03FF000019009014010001")
#define PLC_SEND_ETHERNET_SINGLE_BYTE_WRITE			_T("500000FF03FF000020009014010001")
#define PLC_SEND_ETHERNET_SINGLE_WORD_READ			_T("500000FF03FF0000180090040300000100")
#define PLC_SEND_ETHERNET_SINGLE_WORD_WRITE			_T("500000FF03FF0000200090140200000001")
#define PLC_SEND_ETHERNET_SINGLE_DWORD_READ			_T("500000FF03FF0000180090040300000001")
#define PLC_SEND_ETHERNET_SINGLE_DWORD_WRITE		_T("500000FF03FF0000200090140200000001")

//Multi Data Get, Set�� ���� ��ɾ�?����
#define PLC_SEND_ETHERNET_HEADER					_T("500000FF03FF00")   //Send ���?
#define PLC_RECEIVE_HEADER							_T("D00000FF03FF0000") //Receive ���?
#define PLC_CPU_TIMER								_T("0090")             // CPU ���� Timer
#define PLC_CMD_BATCH_READ							_T("0401")             // CMD
#define PLC_CMD_RANDOM_READ							_T("0403")
#define PLC_CMD_BATCH_WRITE							_T("1401")
#define PLC_CMD_RANDOM_WRITE						_T("1402")
#define PLC_SUB_CMD									_T("0000")

//Buffer/fd Size �� Timeout ����
#define MAX_COMM_WORDDATA							960 // 960 �ȵǸ� 480 �õ� 
#define PLC_COMM_TIMEOUT							1000						//ms����
#define BUF_SIZE									4096
#define __FD_SETSIZE								1024

#define PLC_BITADDR									_T("M")
#define PLC_WORDADDR								_T("D")


//�� 20200512 Binary �����?���� ��ɾ�?����
#define PLC_BINARY_SEND_ETHERNET_SINGLE_WORD_READ   _T("500000FFFF03000C00100001040000")
#define PLC_BINARY_SEND_ETHERNET_SINGLE_WORD_WRITE  _T("500000FFFF03000E00100001140000")
#define PLC_BINARY_SEND_ETHERNET_SINGLE_DWORD_READ  _T("500000FFFF03000C001000030400000001")
#define PLC_BINARY_SEND_ETHERNET_SINGLE_DWORD_WRITE _T("500000FFFF030010001000021400000001")
#define PLC_BINARY_SEND_ETHERNET_BLOCK_DWORD_READ   _T("500000FFFF03000C00100003040000")
#define PLC_BINARY_SEND_ETHERNET_BLOCK_DWORD_WRITE  _T("500000FFFF03000E00100002140000")
#define PLC_BINARY_RECV_EHTERNET_HEADER			    _T("D00000FFFF0300")
#define PLC_BINARY_SEND_HEADER						_T("500000FFFF0300")
#define PLC_BINARY_CPU_TIMER						_T("1000")
#define PLC_BINARY_BATCH_WIRTE						_T("0114")
#define PLC_BINARY_BATCH_READ						_T("0104")
#define PLC_BINARY_RANDOM_WIRTE						_T("0214")
#define PLC_BINARY_RANDOM_READ						_T("0304")
#define	DEVICE_D									0xA8
#define	DEVICE_W									0xB4
#define	DEVICE_r									0xAF
#define	DEVICE_ZR									0xB0
#define	DEVICE_X									0x9C
#define	DEVICE_Y									0x9D
#define	DEVICE_M									0x90
#define	DEVICE_L									0x92
#define	DEVICE_s									0x98
#define	DEVICE_B									0xA0
#define	DEVICE_f									0x93
#define	DEVICE_TS									0xC1
#define	DEVICE_TN									0xC2
#define	DEVICE_CS									0xC4
#define	DEVICE_CC									0xC3
#define	DEVICE_CN									0xC5
#define PLC_DEVICE									DEVICE_D

/*************************************************************/

CCtrlPLC_Melsec::CCtrlPLC_Melsec()
{
	m_strPLCIP = _T("");
	m_nPLCPort = 0;

	m_bConnected = FALSE;
	m_bSendLog = FALSE;
	m_strReceiveData = _T("");
	//m_hWnd = NULL;
	m_bInitialized = FALSE;
}
CCtrlPLC_Melsec::CCtrlPLC_Melsec(CString strPLCIP, int nPLCPort)
{
	m_strPLCIP = _T("");
	m_nPLCPort = 0;

	m_bConnected = FALSE;
	m_bSendLog = FALSE;
	m_strReceiveData = _T("");
	//m_hWnd = NULL;
	m_bInitialized = FALSE;
	Initialize( strPLCIP, nPLCPort);
}
void CCtrlPLC_Melsec::Initialize(CString strPLCIP, int nPLCPort)
{

	SetPLCip(strPLCIP, nPLCPort);
	m_bConnected = FALSE;
	m_bSendLog = FALSE;
	m_strReceiveData = _T("");

	if (m_bInitialized == TRUE) return;

	m_bInitialized = TRUE;
	//this->CreateEx(0, AfxRegisterWndClass(CS_GLOBALCLASS), _T(""), 0, 0, 0, 0, 0, 0, 0);
}
BOOL CCtrlPLC_Melsec::SetPLCip(CString strPLCIP, int nPLCPort)
{
	if (IsConnectPLC() == TRUE)
	{
		return FALSE;
	}
	m_strPLCIP = strPLCIP;
	m_nPLCPort = nPLCPort;
	return TRUE;
}
CCtrlPLC_Melsec::~CCtrlPLC_Melsec()
{
	if (m_bConnected == TRUE)
		PLC_DisConnect();
}

// CEthernetPLC message handlers
BOOL CCtrlPLC_Melsec::PLC_Connect()
{
	if (m_bConnected == TRUE)
		return TRUE;
		//PLC_DisConnect();//��õ���?Disconnect?

	int nErrorStatus;

	WORD wVersionRequested = MAKEWORD(2, 2); // Winsock ���� ���� (2.2) ���?
	WSADATA wsaData;						//Winsock �ʱ�ȭ ���� ����
	struct sockaddr_in SockAddr;

	nErrorStatus = WSAStartup(wVersionRequested, &wsaData); // Winsock Initial processing

	if (nErrorStatus != 0)			//���� ó��
	{
		switch (nErrorStatus)
		{

		case WSASYSNOTREADY:
			m_strErr.Format(_T("Network subsystem is unavailable."));
			break;
		case WSAVERNOTSUPPORTED:
			m_strErr.Format(_T("Winsock.dll version out of range."));
			break;
		case WSAEINPROGRESS:
			m_strErr.Format(_T("Operation now in progress."));
			break;
		case WSAEPROCLIM:
			m_strErr.Format(_T("Too many processes."));
			break;
		case WSAEFAULT:
			m_strErr.Format(_T("Bad address. "));
			break;
		default:
			break;
		}
		SendLog(m_strErr);
		return FALSE;
	}

	m_fdPlcSock = (int)socket(AF_INET, SOCK_STREAM, 0); // Create socket
	if (m_fdPlcSock == INVALID_SOCKET)
	{
		m_strErr.Format(_T("Socket Create Fail!"));
		SendLog(m_strErr);
		return FALSE;
	}

	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = inet_addr((CStringA)m_strPLCIP);
	SockAddr.sin_port = htons(m_nPLCPort);

	int iRet = connect(m_fdPlcSock, (struct sockaddr *)&SockAddr, sizeof(SockAddr));
	if (iRet != 0)
	{
		int iErrCode = WSAGetLastError();
		m_strErr.Format(_T("Socket Connection Fail(%d)!"), iErrCode);
		SendLog(m_strErr);
		m_bConnected = FALSE;
		return FALSE;
	}
	else
		m_bConnected = TRUE;

	return TRUE;
}

BOOL CCtrlPLC_Melsec::PLC_SendMsg(CString Message, BOOL isRead)
{
	int	nTime = 0, nErrorStatus;
	int nMsglen, nSelectStatus;
	char SendBuf[BUF_SIZE];    // Send buffer.    //180111 unsigned char -> char
	char ReceiveBuf[BUF_SIZE]; // Receive buffer  //180111 unsigned char -> char
	if (m_bConnected == TRUE)
	{
		strcpy((char *)(SendBuf), (CStringA)Message);
		nMsglen = (int)strlen((char *)(SendBuf));
		if (send(m_fdPlcSock, (char *)(SendBuf), nMsglen, 0) == SOCKET_ERROR)
		{
			m_strErr.Format(_T("Data Sending Fail.."));
			SendLog(m_strErr);
			return FALSE;
		}
		//Time out.
		struct timeval sel_timeout;
		sel_timeout.tv_sec = 0;
		sel_timeout.tv_usec = PLC_COMM_TIMEOUT * 1000; // ms -> us ��ȯ

		fd_set fds_read;
		FD_ZERO(&fds_read);
		FD_SET(m_fdPlcSock, &fds_read);
		//
		nSelectStatus = select(m_fdPlcSock, &fds_read, NULL, NULL, &sel_timeout); //Receive ���� ��ȭ ����
		if (nSelectStatus > 0)	//Timeout, Error ó��
		{
			nMsglen = recv(m_fdPlcSock, (char*)(&ReceiveBuf[0]), BUF_SIZE, 0);
			if (nMsglen == 0)
			{
				m_strErr.Format(_T("Data Sending Fail.. Length = 0"));
				SendLog(m_strErr);
				return FALSE;
			}
			if (nMsglen == SOCKET_ERROR)
			{
				nErrorStatus = WSAGetLastError();
				m_strErr.Format(_T("Data Receive Fail.. Error Code : %d"), nErrorStatus);
				SendLog(m_strErr);
				return FALSE;
			}

			//skh Plc Read Buffer Receive Length Over Exception (1024 Over Receive ?) _180406
			if (nMsglen >= BUF_SIZE)
			{
				TRACE(_T("Plc Receive Buffer Size Max Over!!\n"));
				nMsglen = BUF_SIZE - 10;
				ReceiveBuf[nMsglen] = '\0';
				m_strReceiveData = (LPCSTR)(LPSTR)ReceiveBuf;
				TRACE(_T("%s\n"), m_strReceiveData);
				return FALSE;
			}

			ReceiveBuf[nMsglen] = '\0'; // Set NULL at the end of receive data

			if (isRead == TRUE)		//Read Message�� ��쿡��?Receive Msg ó��.
			{
				CString strData;
				strData = (LPCSTR)(LPSTR)ReceiveBuf;
				if (_T("04") == strData.Mid(16, 2)) return FALSE; //�߸��� ����̰ų�?PLC�� �������� �޼����� �������� �������?

				m_strReceiveData = strData;
			}

			return TRUE;
		}
		else if (nSelectStatus == 0)
		{
			m_strErr.Format(_T("Data Receive Time Out.."));
			SendLog(m_strErr);
			return FALSE;
		}
		else
		{
			m_strErr.Format(_T("Data Receive Error.."));
			SendLog(m_strErr);
			return FALSE;
		}
	}
	m_strErr.Format(_T("Data Sending Fail.. Connect Fail"));
	SendLog(m_strErr);

	return FALSE;
}

void CCtrlPLC_Melsec::SendLog(CString strLog, BOOL bRecieve)
{
	//_tracer(TRACE_ERROR, TRACE_C_EXT_COMM, strLog);
	//if (m_pMainWnd && m_bSendLog)
	//{
	//	m_pMainWnd->SendMessage(UM_PLC_MSG, (WPARAM)bRecieve, (LPARAM)&strLog);
	//}
}

BOOL CCtrlPLC_Melsec::PLC_DisConnect()
{
	if (shutdown(m_fdPlcSock, 2) != 0)
	{
		m_strErr.Format(_T("Socket Shutdown Fail.."));
		SendLog(m_strErr);
		return FALSE;
	}
	if (closesocket(m_fdPlcSock) != 0)
	{
		m_strErr.Format(_T("Socket Close Fail.."));
		SendLog(m_strErr);
		return FALSE;
	}
	WSACleanup(); //

	m_bConnected = FALSE;
	return TRUE;

}

BOOL CCtrlPLC_Melsec::SetBitM(int address, BOOL bON)
{
	BOOL bRet = FALSE;

	CString  strCmd, strHeader, strAddress;
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_SINGLE_BIT_WRITE);
	strAddress.Format(_T("%s*%06d%04d%01d"), PLC_BITADDR, address, 1, bON); //bit
	strCmd.Format(_T("%s%s"), strHeader, strAddress);

	m_cLock.Lock();
	bRet = PLC_SendMsg(strCmd, FALSE);
	m_cLock.Unlock();
	return bRet;
}

BOOL CCtrlPLC_Melsec::SetBitBlockM(int iFirstAddress, int nDataCount, BOOL* bValArray)
{
	CString strHeader, strCmdLength, strAddress, strDataCount, strData, strVal, strCmd;
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_HEADER); //Header
	strAddress.Format(_T("%s*%06d"), PLC_BITADDR, iFirstAddress); //���� Address
	strDataCount.Format(_T("%04x"), nDataCount);    //����̽�����?

	strData = _T("");
	for (int i = 0; i < nDataCount; i++)
	{
		strVal.Format(_T("%01d"), bValArray[i]);
		strData += strVal;
	}

	strCmd.Format(_T("%s%s0001%s%s%s"), PLC_CPU_TIMER, PLC_CMD_BATCH_WRITE, strAddress, strDataCount, strData);
	strCmdLength.Format(_T("%04x"), strCmd.GetLength());

	CString strSend;
	strSend = strHeader + strCmdLength + strCmd;

	BOOL bRet = FALSE;

	m_cLock.Lock();
	bRet = PLC_SendMsg(strSend, FALSE);
	m_cLock.Unlock();

	return bRet;
}

BOOL CCtrlPLC_Melsec::SetWordBlock(int iFirstAddress, int nDataCount, int* iValArray, int iStartOffset)
{
	CString strHeader, strCmdLength, strAddress, strDataCount, strData, strCmd;
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_HEADER); //Header
	strAddress.Format(_T("%s*%06d"), PLC_WORDADDR, iFirstAddress); //���� Address
	strDataCount.Format(_T("%04x"), nDataCount - iStartOffset);    //����̽�����?

	strData = _T("");
	for (int i = (0 + iStartOffset); i < nDataCount; i++)
	{
		CString strVal;
		wchar_t hVal[10] = { 0, };
		_ltow_s((long)iValArray[i], hVal, 16);
		strVal.Format(_T("%04s"), hVal);

		strData += strVal;
	}

	strCmd.Format(_T("%s%s%s%s%s%s"), PLC_CPU_TIMER, PLC_CMD_BATCH_WRITE, PLC_SUB_CMD, strAddress, strDataCount, strData);
	strCmdLength.Format(_T("%04x"), strCmd.GetLength());

	CString strSend;
	strSend = strHeader + strCmdLength + strCmd;

	BOOL ret = FALSE;
	m_cLock.Lock();
	ret = PLC_SendMsg(strSend, FALSE);
	m_cLock.Unlock();


	return ret;
}

BOOL CCtrlPLC_Melsec::SetWord(int iAddress, int val)
{
	CString strHeader, strCmdLength, strAddress, strDataCount, strCmd;
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_HEADER); //Header
	strAddress.Format(_T("%s*%06d"), PLC_WORDADDR, iAddress); //���� Address
	strDataCount.Format(_T("%04x"), 1);    //����̽�����?

	wchar_t hVal[10] = { 0, };
	_ltow_s((long)val, hVal, 16);
	CString strVal;
	strVal.Format(_T("%04s"), hVal);

	strCmd.Format(_T("%s%s%s%s%s%s"), PLC_CPU_TIMER, PLC_CMD_BATCH_WRITE, PLC_SUB_CMD, strAddress, strDataCount, strVal);
	strCmdLength.Format(_T("%04x"), strCmd.GetLength());

	CString strSend;
	strSend = strHeader + strCmdLength + strCmd;

	BOOL ret = FALSE;
	m_cLock.Lock();
	ret = PLC_SendMsg(strSend, FALSE);
	m_cLock.Unlock();


	return ret;
}

BOOL CCtrlPLC_Melsec::SetDoubleWord(int iAddress, int iVal)
{
	wchar_t hVal[10] = { 0, };
	CString strHeader, strAddress, strVal, strCmd;
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_SINGLE_DWORD_WRITE);
	strAddress.Format(_T("%s*%06d"), PLC_WORDADDR, iAddress);

	_ltow_s((long)iVal, hVal, 16);
	strVal.Format(_T("%08s"), hVal);
	strCmd = strHeader + strAddress + strVal;

	BOOL ret = FALSE;
	m_cLock.Lock();
	ret = PLC_SendMsg(strCmd, FALSE);
	m_cLock.Unlock();

	return ret;
}

BOOL CCtrlPLC_Melsec::SetDoubleWordBlock(int iFirstAddress, int nDataCount, int* iValArray)
{
	CString strHeader, strCmdLength, strDataCount, strDeviceData, strData, strCmd;
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_HEADER); //Header
	strDataCount.Format(_T("%04d"), nDataCount);          //����̽�����?

	strData = _T("");
	for (int i = 0; i < nDataCount; i++)
	{
		CString strVal;
		wchar_t hVal[10] = { 0, };
		_ltow_s((long)iValArray[i], hVal, 16);
		strVal.Format(_T("%08s"), hVal);
		strDeviceData.Format(_T("%s*%06d%s"), PLC_WORDADDR, iFirstAddress + (i * 2), strVal);
		strData += strDeviceData;
	}

	strCmd.Format(_T("%s%s%s%s%s"), PLC_CPU_TIMER, PLC_CMD_RANDOM_WRITE, PLC_SUB_CMD, strDataCount, strData);
	strCmdLength.Format(_T("%04x"), strCmd.GetLength());

	CString strSend;
	strSend = strHeader + strCmdLength + strCmd;

	BOOL ret = FALSE;
	m_cLock.Lock();
	ret = PLC_SendMsg(strSend, FALSE);
	m_cLock.Unlock();


	return ret;
}

int CCtrlPLC_Melsec::GetBitM(int address)
{
	CString strCmd, strHeader, strAddress;
	strAddress.Format(_T("%s*%06d%04x"), PLC_BITADDR, address, 1); //bit
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_SINGLE_BIT_READ);
	strCmd.Format(_T("%s%s"), strHeader, strAddress);

	int iVal = -1;
	m_cLock.Lock();
	if (PLC_SendMsg(strCmd, TRUE))
		CheckBit(m_strReceiveData, &iVal);
	m_cLock.Unlock();
	return iVal;
}

BOOL CCtrlPLC_Melsec::GetBitBlockM(int firstAddress, int nDataCount, int* iArray)
{
	CString strCmd, strHeader, strAddress;
	strAddress.Format(_T("%s*%06d%04x"), PLC_BITADDR, firstAddress, nDataCount);
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_SINGLE_BIT_READ);
	strCmd.Format(_T("%s%s"), strHeader, strAddress);

	BOOL bRet = FALSE;
	m_cLock.Lock();
	if (PLC_SendMsg(strCmd, TRUE))
		bRet = CheckBitArray(m_strReceiveData, nDataCount, iArray);
	m_cLock.Unlock();

	return bRet;
}

BOOL CCtrlPLC_Melsec::GetWord(int iAddress, int* iVal)
{
	CString strHeader, strAddress, strCmd;
	strHeader = PLC_SEND_ETHERNET_SINGLE_WORD_READ;
	strAddress.Format(_T("%s*%06d"), PLC_WORDADDR, iAddress);
	strCmd = strHeader + strAddress;

	BOOL bRet = FALSE;
	*iVal = -1;
	m_cLock.Lock();
	if (PLC_SendMsg(strCmd, TRUE))
		bRet = CheckWord(m_strReceiveData, iVal);
	m_cLock.Unlock();

	return bRet;
}

BOOL CCtrlPLC_Melsec::GetWordBlock(int iFirstAddress, int nDataCount, int* iReadArray)
{
	CString strHeader, strCmdLength, strAddress, strDataCount, strData, strCmd;
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_HEADER); //Header
	strAddress.Format(_T("%s*%06d"), PLC_WORDADDR, iFirstAddress); //���� Address
	strDataCount.Format(_T("%04x"), nDataCount);    //����̽�����?
	strCmd.Format(_T("%s%s0000%s%s"), PLC_CPU_TIMER, PLC_CMD_BATCH_READ, strAddress, strDataCount);
	strCmdLength.Format(_T("%04x"), strCmd.GetLength());

	CString strSend;
	strSend = strHeader + strCmdLength + strCmd;

	BOOL bRet = FALSE;
	m_cLock.Lock();
	if (PLC_SendMsg(strSend, TRUE))
		bRet = CheckWordArray(m_strReceiveData, nDataCount, iReadArray);
	m_cLock.Unlock();

	return bRet;
}

BOOL CCtrlPLC_Melsec::GetDoubleWord(int iAddress, int* iVal)
{
	CString strHeader, strAddress, strCmd;
	strHeader = PLC_SEND_ETHERNET_SINGLE_DWORD_READ;
	strAddress.Format(_T("%s*%06d"), PLC_WORDADDR, iAddress);
	strCmd = strHeader + strAddress;

	BOOL bRet = FALSE;
	*iVal = -1;
	m_cLock.Lock();
	if (PLC_SendMsg(strCmd, TRUE))
		bRet = CheckDWord(m_strReceiveData, iVal);
	m_cLock.Unlock();

	return bRet;
}

BOOL CCtrlPLC_Melsec::GetDoubleWordBlock(int iFirstAddress, int nDataCount, int* iReadArray)
{
	CString strHeader, strCmdLength, strAddress, strDataCount, strData, strCmd, strSend;
	strHeader.Format(_T("%s"), PLC_SEND_ETHERNET_HEADER); //Header
	strAddress.Format(_T("R*%06d"), iFirstAddress); //���� Address
	strDataCount.Format(_T("%04x"), nDataCount);    //����̽�����?

	strData = _T("");
	for (int i = 0; i < nDataCount; i++)
	{
		strAddress.Format(_T("%s*%06d"), PLC_WORDADDR, iFirstAddress + (i * 2));
		strData += strAddress;
	}

	strCmd.Format(_T("%s%s%s%s%s"), PLC_CPU_TIMER, PLC_CMD_RANDOM_READ, PLC_SUB_CMD, strDataCount, strData);
	strCmdLength.Format(_T("%04x"), strCmd.GetLength());
	strSend = strHeader + strCmdLength + strCmd;

	BOOL bRet = FALSE;
	m_cLock.Lock();
	if (PLC_SendMsg(strSend, TRUE))
		bRet = CheckDWordArray(m_strReceiveData, nDataCount, iReadArray);
	m_cLock.Unlock();


	return bRet;
}

BOOL CCtrlPLC_Melsec::PLCErrorReset()
{
	BOOL bret = FALSE;
	m_cLock.Lock();
	bret = PLC_SendMsg(PLC_SEND_ETHERNET_ERROR_RESET, FALSE);
	m_cLock.Unlock();

	return bret;
}

BOOL CCtrlPLC_Melsec::CheckBit(CString strReply, int* iBitData)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strBitVal;

	strHeader = strData.Mid(0, 16);
	if (PLC_RECEIVE_HEADER == strHeader)
	{
		strLength = strData.Mid(16, 2);
		strErrorCode = strData.Mid(18, 4);
		strBitVal = strData.Mid(22, 1);

		*iBitData = _tstoi(strBitVal);
		bAck = TRUE;
	}
	else
	{
		*iBitData = -1;
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckBitArray(CString strReply, int nCount, int* iBitArray)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strBitVal;

	strHeader = strData.Mid(0, 16);
	if (PLC_RECEIVE_HEADER == strHeader)
	{
		strLength = strData.Mid(16, 2);
		strErrorCode = strData.Mid(18, 4);
		strBitVal = strData.Mid(22, nCount);
		for (int i = 0; i < nCount; i++)
		{
			iBitArray[i] = _tstoi(strBitVal.Mid(i, 1));
		}
		bAck = TRUE;
	}
	else
	{
		memset(iBitArray, -1, sizeof(int)* nCount);
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckWord(CString strReply, int* iVal)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strVal;

	strHeader = strData.Mid(0, 16);
	if (PLC_RECEIVE_HEADER == strHeader)
	{
		strLength = strData.Mid(16, 2);
		strErrorCode = strData.Mid(18, 4);
		strVal = strData.Mid(22, 4);

		//Hexa to int
		wchar_t	*StopStr;
		*iVal = (int)wcstol(strVal, &StopStr, 16);

		bAck = TRUE;
	}
	else
	{
		*iVal = -1;
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckWordArray(CString strReply, int nCount, int* iValArray)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strVal;

	strHeader = strData.Mid(0, 16);
	if (PLC_RECEIVE_HEADER == strHeader)//L"D00000FF03FF0000"
	{
		strLength = strData.Mid(16, 2);
		strErrorCode = strData.Mid(18, 4);

		wchar_t	*StopStr;
		int nLength = (int)wcstol(strLength, &StopStr, 16);
		strVal = strData.Mid(22, nLength - 4);

		CString strData;
		int idx = 0;
		for (int i = 0; i < nCount; i++)
		{
			strData = strVal.Mid(idx, 4);
			//Hexa to int
			iValArray[i] = (int)wcstol(strData, &StopStr, 16);

			idx += 4;
		}
		bAck = TRUE;
	}
	else
	{
		memset(iValArray, -1, sizeof(int)*nCount);
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckDWord(CString strReply, int* iVal)
{
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strVal;
	BOOL bAck = FALSE;

	strHeader = strData.Mid(0, 16);
	if (PLC_RECEIVE_HEADER == strHeader)
	{
		strLength = strData.Mid(16, 2);
		strErrorCode = strData.Mid(18, 4);
		strVal = strData.Mid(22, 8);

		//Hexa to int
		wchar_t	*StopStr;
		*iVal = (int)wcstol(strVal, &StopStr, 16);

		bAck = TRUE;
	}
	else
	{
		*iVal = -1;
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckDWordArray(CString strReply, int nCount, int* iValArray)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strVal;

	strHeader = strData.Mid(0, 16);
	if (PLC_RECEIVE_HEADER == strHeader)
	{
		strLength = strData.Mid(16, 2);
		strErrorCode = strData.Mid(18, 4);

		wchar_t	*StopStr;
		int nLength = (int)wcstol(strLength, &StopStr, 16);
		strVal = strData.Mid(22, nLength - 4);

		CString strData;
		int idx = 0;
		for (int i = 0; i < nCount; i++)
		{
			strData = strVal.Mid(idx, 8);
			//Hexa to int
			iValArray[i] = (int)wcstol(strData, &StopStr, 16);

			idx += 8;
		}
		bAck = TRUE;
	}
	else
	{
		memset(iValArray, -1, sizeof(int)*nCount);
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckBinaryWord(CString strReply, short* iVal)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strVal;

	strHeader = strData.Mid(0, 14);
	strHeader.MakeUpper();
	if (PLC_BINARY_RECV_EHTERNET_HEADER == strHeader)
	{
		strLength = strData.Mid(14, 4);
		strErrorCode = strData.Mid(18, 4);
		strVal = strData.Mid(24, 2);
		strVal += strData.Mid(22, 2);

		//Hexa to int
		wchar_t	*StopStr;
		*iVal = (WORD)wcstol(strVal, &StopStr, 16);
		bAck = TRUE;
	}
	else
	{
		*iVal = -1;
	}
	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckBinaryWordArray(CString strReply, int nCount, short* iValArray)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strVal;

	strHeader = strData.Mid(0, 14);
	strHeader.MakeUpper();
	if (PLC_BINARY_RECV_EHTERNET_HEADER == strHeader)
	{
		strLength = strData.Mid(16, 2);
		strLength += strData.Mid(14, 2);
		strErrorCode = strData.Mid(16, 4);

		wchar_t	*StopStr;
		int nLength = (int)wcstol(strLength, &StopStr, 16);
		strVal = strData.Mid(22, nLength * 2 - 4);

		CString strData;
		int idx = 0;
		for (int i = 0; i < nCount; i++)
		{
			strData = strVal.Mid(idx + 2, 2);
			strData += strVal.Mid(idx, 2);
			//Hexa to int
			iValArray[i] = (WORD)wcstol(strData, &StopStr, 16);

			idx += 4;
		}
		bAck = TRUE;
	}
	else
	{
		memset(iValArray, -1, sizeof(int)*nCount);
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckBinaryDWord(CString strReply, int* iVal)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strVal;

	strHeader = strData.Mid(0, 14);
	strHeader.MakeUpper();

	short LO, HI;
	if (PLC_BINARY_RECV_EHTERNET_HEADER == strHeader)//L"D00000FF03FF0000"
	{
		strLength = strData.Mid(16, 2);
		strLength += strData.Mid(14, 2);
		strErrorCode = strData.Mid(16, 4);

		wchar_t	*StopStr;

		strVal = strData.Mid(24, 2);
		strVal += strData.Mid(22, 2);
		LO = (WORD)wcstol(strVal, &StopStr, 16);

		strVal = strData.Mid(28, 2);
		strVal += strData.Mid(26, 2);
		HI = (WORD)wcstol(strVal, &StopStr, 16);


		//�̷��Թۿ��ȵǳ���..
		*iVal = MAKELONG(LO, HI);

		bAck = TRUE;
	}
	else
	{
		*iVal = -1;
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::CheckBinaryDWordArray(CString strReply, int nCount, int* iValArray)
{
	BOOL bAck = FALSE;
	CString strData = strReply;
	CString strHeader, strLength, strErrorCode, strVal;

	strHeader = strData.Mid(0, 14);
	strHeader.MakeUpper();

	short LO, HI;
	if (PLC_BINARY_RECV_EHTERNET_HEADER == strHeader)
	{
		strLength = strData.Mid(16, 2);
		strLength += strData.Mid(14, 2);
		strErrorCode = strData.Mid(16, 4);

		wchar_t	*StopStr;
		int nLength = (int)wcstol(strLength, &StopStr, 16);
		strVal = strData.Mid(22, nLength * 2 - 4);

		CString strData;
		int idx = 0;
		for (int i = 0; i < nCount; i++)
		{
			strData = strVal.Mid(idx + 2, 2);
			strData += strVal.Mid(idx, 2);
			LO = (WORD)wcstol(strData, &StopStr, 16);

			strData = strVal.Mid(idx + 6, 2);
			strData += strVal.Mid(idx + 4, 2);
			HI = (WORD)wcstol(strData, &StopStr, 16);

			//Hexa to int
			iValArray[i] = MAKELONG(LO, HI);

			idx += 8;
		}
		bAck = TRUE;
	}
	else
	{
		memset(iValArray, -1, sizeof(int)*nCount);
	}

	return bAck;
}

BOOL CCtrlPLC_Melsec::PLC_SendBinaryMsg(char * Message, int nLen, BOOL isRead)
{
	int	nTime = 0, nErrorStatus;
	int nMsglen, nSelectStatus;
	char SendBuf[BUF_SIZE];    // Send buffer.    //180111 unsigned char -> char
	char ReceiveBuf[BUF_SIZE]; // Receive buffer  //180111 unsigned char -> char
	unsigned char ReceiveCoppyBuf[BUF_SIZE];
	memset(ReceiveBuf, 0, BUF_SIZE);

	if (m_bConnected == TRUE)
	{
		memcpy(SendBuf, Message, nLen);
		//strcpy((char *)(SendBuf), Message);
		//nMsglen = (int)strlen((char *)(SendBuf));
		nMsglen = nLen;
		if (send(m_fdPlcSock, (char*)SendBuf, nMsglen, 0) == SOCKET_ERROR)
		{
			m_strErr.Format(_T("Data Sending Fail.."));
			SendLog(m_strErr);
			return FALSE;
		}
		//Time out ������ ����
		struct timeval sel_timeout;
		sel_timeout.tv_sec = 0;
		sel_timeout.tv_usec = PLC_COMM_TIMEOUT * 1000; // ms -> us ��ȯ

		fd_set fds_read;
		FD_ZERO(&fds_read);
		FD_SET(m_fdPlcSock, &fds_read);
		//
		nSelectStatus = select(m_fdPlcSock, &fds_read, NULL, NULL, &sel_timeout); //Receive ���� ��ȭ ����
		if (nSelectStatus > 0)	//Timeout, Error ó��
		{
			nMsglen = recv(m_fdPlcSock, ReceiveBuf, sizeof(ReceiveBuf), 0);
			if (nMsglen == 0)
			{
				m_strErr.Format(_T("Data Sending Fail.. Length = 0"));
				SendLog(m_strErr);
				return FALSE;
			}
			if (nMsglen == SOCKET_ERROR)
			{
				nErrorStatus = WSAGetLastError();
				m_strErr.Format(_T("Data Receive Fail.. Error Code : %d"), nErrorStatus);
				SendLog(m_strErr);
				return FALSE;
			}

			//skh Plc Read Buffer Receive Length Over Exception (1024 Over Receive ?) _180406
			if (nMsglen >= BUF_SIZE)
			{
				TRACE(_T("Plc Receive Buffer Size Max Over!!\n"));
				nMsglen = BUF_SIZE - 10;
				ReceiveBuf[nMsglen] = '\0';
				m_strReceiveData = (LPCSTR)(LPSTR)ReceiveBuf;
				TRACE(_T("%s\n"), m_strReceiveData);
				return FALSE;
			}

			if (isRead == TRUE)		//Read Message�� ��쿡��?Receive Msg ó��.
			{
				//���� ������ ȹ�� ��.
				CString strData;
				memset(ReceiveCoppyBuf, 0, BUF_SIZE);
				memcpy(ReceiveCoppyBuf, ReceiveBuf, nMsglen);
				m_strReceiveData = L"";

				for (int i = 0; i < nMsglen; i++) {
					strData.Format(L"%02x", ReceiveCoppyBuf[i]);
					m_strReceiveData += strData;
				}
				strData = m_strReceiveData.Mid(18, 4);

				if (_T("0000") != strData)
					return FALSE; //�߸��� ����̰ų�?PLC�� �������� �޼����� �������� �������?
			}
			return TRUE;
		}
		else if (nSelectStatus == 0)
		{
			m_strErr.Format(_T("Data Receive Time Out.."));
			SendLog(m_strErr);
			return FALSE;
		}
		else
		{
			m_strErr.Format(_T("Data Receive Error.."));
			SendLog(m_strErr);
			return FALSE;
		}
	}
	m_strErr.Format(_T("Data Sending Fail.. Connect Fail"));
	SendLog(m_strErr);
	return FALSE;
}

BOOL  CCtrlPLC_Melsec::GetBinaryWord(int iAddress, short* iReadVal)
{
	CString strHeader, strAddress, strCmd, strTmp;
	strHeader.Format(_T("%s"), PLC_BINARY_SEND_ETHERNET_SINGLE_WORD_READ); //Header
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(iAddress), (byte)HIBYTE(iAddress));
	strHeader += strTmp;
	strTmp.Format(L"00%02x", PLC_DEVICE);
	strHeader += strTmp;
	strTmp.Format(L"%02x%02x", 1 % 256, 1 / 256);
	strHeader += strTmp;

	int nCnt = 0;
	wchar_t	*StopStr;
	char czSendData[BUF_SIZE];
	memset(czSendData, 0, BUF_SIZE);
	for (int i = 0; i < strHeader.GetLength(); i++) {
		strTmp = strHeader.Mid(nCnt, 2);
		czSendData[i] = (byte)wcstol(strTmp, &StopStr, 16);
		nCnt += 2;
	}
	nCnt /= 4; // ���嵥���� ����.


	BOOL bRet = FALSE;
	*iReadVal = -1;
	m_cLock.Lock();
	if (PLC_SendBinaryMsg(czSendData, nCnt, TRUE))
		bRet = CheckBinaryWord(m_strReceiveData, iReadVal);
	m_cLock.Unlock();

	return bRet;
}

BOOL  CCtrlPLC_Melsec::SetBinaryWord(int iAddress, short ival)
{
	CString strHeader, strAddress, strCmd, strTmp;
	strHeader.Format(_T("%s"), PLC_BINARY_SEND_ETHERNET_SINGLE_WORD_WRITE); //Header
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(iAddress), (byte)HIBYTE(iAddress));
	strHeader += strTmp;
	strTmp.Format(L"00%02x", PLC_DEVICE);
	strHeader += strTmp;
	strTmp.Format(L"%02x%02x", 1 % 256, 1 / 256);
	strHeader += strTmp;
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(ival), (byte)HIBYTE(ival));
	strHeader += strTmp;


	int nCnt = 0;
	wchar_t	*StopStr;
	char czSendData[BUF_SIZE];
	memset(czSendData, 0, BUF_SIZE);
	for (int i = 0; i < strHeader.GetLength(); i++) {
		strTmp = strHeader.Mid(nCnt, 2);
		czSendData[i] = (byte)wcstol(strTmp, &StopStr, 16);
		nCnt += 2;
	}
	nCnt /= 4; // ���嵥���� ����.

	BOOL ret = FALSE;
	m_cLock.Lock();
	ret = PLC_SendBinaryMsg(czSendData, nCnt, TRUE);
	m_cLock.Unlock();
	return ret;
}

BOOL  CCtrlPLC_Melsec::GetBinaryDoubleWord(int iAddress, int* iReadVal)
{
	CString strHeader, strAddress, strCmd, strTmp;
	strHeader.Format(L"%s", PLC_BINARY_SEND_ETHERNET_SINGLE_DWORD_READ); //Header
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(iAddress), (byte)HIBYTE(iAddress));
	strHeader += strTmp;
	strTmp.Format(L"00%02x", PLC_DEVICE);
	strHeader += strTmp;

	int nCnt = 0;
	wchar_t	*StopStr;
	char czSendData[BUF_SIZE];
	memset(czSendData, 0, BUF_SIZE);
	for (int i = 0; i < strHeader.GetLength(); i++) {
		strTmp = strHeader.Mid(nCnt, 2);
		czSendData[i] = (byte)wcstol(strTmp, &StopStr, 16);
		nCnt += 2;
	}
	nCnt /= 4; // ���嵥���� ����.

	BOOL bRet = FALSE;
	*iReadVal = -1;
	m_cLock.Lock();
	if (PLC_SendBinaryMsg(czSendData, nCnt, TRUE))
		bRet = CheckBinaryDWord(m_strReceiveData, iReadVal);
	m_cLock.Unlock();
	return bRet;
}

BOOL  CCtrlPLC_Melsec::SetBinaryDoubleWord(int iAddress, int iVal)
{
	CString strHeader, strAddress, strCmd, strTmp;
	strHeader.Format(_T("%s"), PLC_BINARY_SEND_ETHERNET_SINGLE_DWORD_WRITE); //Header
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(iAddress), (byte)HIBYTE(iAddress));
	strHeader += strTmp;
	strTmp.Format(L"00%02x", PLC_DEVICE);
	strHeader += strTmp;

	int nTemp[2];
	nTemp[0] = LOWORD(iVal);
	nTemp[1] = HIWORD(iVal);

	strTmp.Format(L"%02x%02x", (byte)LOBYTE(nTemp[0]), (byte)HIBYTE(nTemp[0]));
	strHeader += strTmp;
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(nTemp[1]), (byte)HIBYTE(nTemp[1]));
	strHeader += strTmp;

	int nCnt = 0;
	wchar_t	*StopStr;
	char czSendData[BUF_SIZE];
	memset(czSendData, 0, BUF_SIZE);
	for (int i = 0; i < strHeader.GetLength(); i++) {
		strTmp = strHeader.Mid(nCnt, 2);
		czSendData[i] = (byte)wcstol(strTmp, &StopStr, 16);
		nCnt += 2;
	}
	nCnt /= 4; // ���嵥���� ����.

	BOOL ret = FALSE;
	m_cLock.Lock();
	ret = PLC_SendBinaryMsg(czSendData, nCnt, FALSE);
	m_cLock.Unlock();
	return ret;
}

BOOL  CCtrlPLC_Melsec::GetBinaryWordBlock(int iFirstAddress, int nDataCount, short* iReadArray)
{
	CString strHeader, strAddress, strCmd, strTmp;
	int nLen = 12 + (nDataCount * 2);
	strHeader.Format(L"%s", PLC_BINARY_SEND_ETHERNET_SINGLE_WORD_READ); //Header
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(iFirstAddress), (byte)HIBYTE(iFirstAddress));
	strHeader += strTmp;
	strTmp.Format(L"00%02x", PLC_DEVICE);
	strHeader += strTmp;
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(nDataCount), (byte)HIBYTE(nDataCount));
	strHeader += strTmp;

	int nCnt = 0;
	wchar_t	*StopStr;
	char czSendData[BUF_SIZE];
	memset(czSendData, 0, BUF_SIZE);
	for (int i = 0; i < strHeader.GetLength(); i++) {
		strTmp = strHeader.Mid(nCnt, 2);
		czSendData[i] = (byte)wcstol(strTmp, &StopStr, 16);
		nCnt += 2;
	}
	nCnt /= 4; // ���嵥���� ����.

	BOOL bRet = FALSE;
	m_cLock.Lock();
	if (PLC_SendBinaryMsg(czSendData, nCnt, TRUE))
		bRet = CheckBinaryWordArray(m_strReceiveData, nDataCount, iReadArray);
	m_cLock.Unlock();
	return bRet;
}

BOOL  CCtrlPLC_Melsec::SetBinaryWordBlock(int iFirstAddress, int nDataCount, short* iValArray, int iStartOffset)
{
	CString strHeader, strAddress, strCmd, strTmp;
	int nLen = 12 + (nDataCount * 2);
	strHeader.Format(L"%s", PLC_BINARY_SEND_HEADER); //Header
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(nLen), (byte)HIBYTE(nLen));
	strHeader += strTmp;

	strHeader += PLC_BINARY_CPU_TIMER;
	strHeader += PLC_BINARY_BATCH_WIRTE;
	strHeader += PLC_SUB_CMD;

	strTmp.Format(L"%02x%02x", (byte)LOBYTE(iFirstAddress), (byte)HIBYTE(iFirstAddress));
	strHeader += strTmp;
	strTmp.Format(L"00%02x", PLC_DEVICE);
	strHeader += strTmp;
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(nDataCount), (byte)HIBYTE(nDataCount));
	strHeader += strTmp;
	for (int i = 0; i < nDataCount; i++) {
		strTmp.Format(L"%02x%02x", (byte)LOBYTE(iValArray[i]), (byte)HIBYTE(iValArray[i]));
		strHeader += strTmp;
	}


	int nCnt = 0;
	wchar_t	*StopStr;
	char czSendData[BUF_SIZE];
	memset(czSendData, 0, BUF_SIZE);
	for (int i = 0; i < strHeader.GetLength(); i++) {
		strTmp = strHeader.Mid(nCnt, 2);
		czSendData[i] = (byte)wcstol(strTmp, &StopStr, 16);
		nCnt += 2;
	}
	nCnt /= 4; // ���嵥���� ����.

	BOOL ret = FALSE;
	m_cLock.Lock();
	ret = PLC_SendBinaryMsg(czSendData, nCnt, TRUE);
	m_cLock.Unlock();
	return ret;
}

BOOL CCtrlPLC_Melsec::ReadPLC(int addr, int nWordCnt, short* pReadArray)
{

	if (nWordCnt > MAX_COMM_WORDDATA) {
		/*int iptr= PLC_READ_INSPECT;*/
		int iptr = addr;
		short *ptr = pReadArray;
		int nmax = nWordCnt / MAX_COMM_WORDDATA;
		for (int n = 0; n < nmax; n++) {
			if (!GetBinaryWordBlock(iptr, MAX_COMM_WORDDATA, ptr))
				return FALSE;
			iptr += MAX_COMM_WORDDATA;
			ptr += MAX_COMM_WORDDATA;
		}

		if (!GetBinaryWordBlock(iptr, nWordCnt % MAX_COMM_WORDDATA, ptr))
			return FALSE;
		return TRUE;

	}
	else
		return GetBinaryWordBlock(addr, nWordCnt, pReadArray);
	/*return GetBinaryWordBlock(PLC_READ_INSPECT, PLC_INSP_MAX_BUFF, m_rDataInsp);*/
}
BOOL  CCtrlPLC_Melsec::WritePLC(int addr, int nWordCnt, short* pWriteArray)
{
	if (nWordCnt > MAX_COMM_WORDDATA) {
		/*int iptr= PLC_READ_INSPECT;*/
		int iptr = addr;
		short *ptr = pWriteArray;
		int nmax = nWordCnt / MAX_COMM_WORDDATA;
		for (int n = 0; n < nmax; n++) {
			if (!SetBinaryWordBlock(iptr, MAX_COMM_WORDDATA, ptr))
				return FALSE;
			iptr += MAX_COMM_WORDDATA;
			ptr += MAX_COMM_WORDDATA;
		}

		if (!SetBinaryWordBlock(iptr, nWordCnt % MAX_COMM_WORDDATA, ptr))
			return FALSE;
		return TRUE;

	}
	else
		return SetBinaryWordBlock(addr, nWordCnt, pWriteArray);
}

BOOL  CCtrlPLC_Melsec::WritePLC(int addr, int nWordCnt, int* pWriteArray)
{
	if (nWordCnt > MAX_COMM_WORDDATA) {
		/*int iptr= PLC_READ_INSPECT;*/
		int iptr = addr;
		int *ptr = pWriteArray;
		int nmax = nWordCnt / MAX_COMM_WORDDATA;
		for (int n = 0; n < nmax; n++) {
			if (!SetBinaryDoubleWordBlock(iptr, MAX_COMM_WORDDATA, ptr))
				return FALSE;
			iptr += MAX_COMM_WORDDATA;
			ptr += MAX_COMM_WORDDATA;
		}

		if (!SetBinaryDoubleWordBlock(iptr, nWordCnt % MAX_COMM_WORDDATA, ptr))
			return FALSE;
		return TRUE;

	}
	else
		return SetBinaryDoubleWordBlock(addr, nWordCnt, pWriteArray);
}

//BOOL CCtrlPLC_Melsec::GetInpectionResults(short nNo, POINTF* pStartPos, POINTF* pEndPos)
//{
//
//	int inx = nNo * JOB_WELD_POINT_COUNT;  // 4
//	int no = inx * PLC_INSP_JUMP1; // 30
//	for (int n = 0; n < JOB_WELD_POINT_COUNT; n++)
//	{
//		int i = n * PLC_INSP_JUMP1;
//		// Start pos
//		pStartPos[n].x = ((float)GetRead_Insp(no + i + 0) / 100.0f);
//		pStartPos[n].y = ((float)GetRead_Insp(no + i + 1) / 100.0f);
//		// End Pos
//		pEndPos[n].x = ((float)GetRead_Insp(no + i + 2) / 100.0f);
//		pEndPos[n].y = ((float)GetRead_Insp(no + i + 3) / 100.0f);
//
//		_tracer(TRACE_NORMAL, nNo, L"Job No %d - %d, Start pos x : %5.2f, y : %5.2f, End pos x : %5.2f, y : %5.2f", nNo, n, pStartPos[n].x, pStartPos[n].y, pEndPos[n].x, pEndPos[n].y);
//	}
//	return TRUE;
//}


BOOL  CCtrlPLC_Melsec::SetBinaryDoubleWordBlock(int iFirstAddress, int nDataCount, int* iValArray)
{
	CString strHeader, strAddress, strCmd, strTmp;
	strHeader.Format(L"%s", PLC_BINARY_SEND_HEADER); //Header
	int nLen = 16 + ((nDataCount - 1) * 8);
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(nLen), (byte)HIBYTE(nLen));
	strHeader += strTmp;

	strHeader += PLC_BINARY_CPU_TIMER;
	strHeader += PLC_BINARY_RANDOM_WIRTE;
	strHeader += PLC_SUB_CMD;
	strTmp.Format(L"%04x", nDataCount);
	strHeader += strTmp;
	int nTemp[2];

	for (int i = 0; i < nDataCount; i++) {
		strTmp.Format(L"%02x%02x", (byte)LOBYTE(iFirstAddress + i * 2), (byte)HIBYTE(iFirstAddress + i * 2));
		strHeader += strTmp;
		strTmp.Format(L"00%02x", PLC_DEVICE);
		strHeader += strTmp;
		nTemp[0] = LOWORD(iValArray[i]);
		nTemp[1] = HIWORD(iValArray[i]);
		strTmp.Format(L"%02x%02x", (byte)LOBYTE(nTemp[0]), (byte)HIBYTE(nTemp[0]));
		strHeader += strTmp;
		strTmp.Format(L"%02x%02x", (byte)LOBYTE(nTemp[1]), (byte)HIBYTE(nTemp[1]));
		strHeader += strTmp;
	}

	int nCnt = 0;
	wchar_t	*StopStr;
	char czSendData[BUF_SIZE];
	memset(czSendData, 0, BUF_SIZE);
	for (int i = 0; i < strHeader.GetLength(); i++) {
		strTmp = strHeader.Mid(nCnt, 2);
		czSendData[i] = (byte)wcstol(strTmp, &StopStr, 16);
		nCnt += 2;
	}
	nCnt /= 4; // ���嵥���� ����.

	BOOL ret = FALSE;
	m_cLock.Lock();
	ret = PLC_SendBinaryMsg(czSendData, nCnt, TRUE);
	m_cLock.Unlock();
	return ret;
}

BOOL  CCtrlPLC_Melsec::GetBinaryDoubleWordBlock(int iFirstAddress, int nDataCount, int* iReadArray)
{
	CString strHeader, strAddress, strCmd, strTmp;
	strHeader.Format(L"%s", PLC_BINARY_SEND_HEADER); //Header
	int nLen = 12 + ((nDataCount - 1) * 4);
	strTmp.Format(L"%02x%02x", (byte)LOBYTE(nLen), (byte)HIBYTE(nLen));
	strHeader += strTmp;

	strHeader += PLC_BINARY_CPU_TIMER;
	strHeader += PLC_BINARY_RANDOM_READ;
	strHeader += PLC_SUB_CMD;
	strTmp.Format(L"%04x", nDataCount);
	strHeader += strTmp;

	for (int i = 0; i < nDataCount; i++) {
		strTmp.Format(L"%02x%02x", (byte)LOBYTE(iFirstAddress + i * 2), (byte)HIBYTE(iFirstAddress + i * 2));
		strHeader += strTmp;
		strTmp.Format(L"00%02x", PLC_DEVICE);
		strHeader += strTmp;

	}

	int nCnt = 0;
	wchar_t	*StopStr;
	char czSendData[BUF_SIZE];
	memset(czSendData, 0, BUF_SIZE);
	for (int i = 0; i < strHeader.GetLength(); i++) {
		strTmp = strHeader.Mid(nCnt, 2);
		czSendData[i] = (byte)wcstol(strTmp, &StopStr, 16);
		nCnt += 2;
	}
	nCnt /= 4; // ���嵥���� ����.

	BOOL bRet = FALSE;
	m_cLock.Lock();
	if (PLC_SendBinaryMsg(czSendData, nCnt, TRUE))
		bRet = CheckBinaryDWordArray(m_strReceiveData, nDataCount, iReadArray);
	m_cLock.Unlock();
	return bRet;
}



void CCtrlPLC_Melsec::GetWordToBit(short nVal, BOOL * bBit)
{
	for (int i = 0; i < 16; i++) {
		bBit[i] = (nVal & (1 << i)) >> i;
	}
}

void CCtrlPLC_Melsec::GetBitToWord(BOOL * bBit, short * nVal)
{
	short parsing = 1;
	*nVal = 0;
	for (int i = 0; i < 16; i++) {
		*nVal += (short)(bBit[i] * parsing);
		parsing *= 2;
	}
}

void CCtrlPLC_Melsec::GetAsciiCode(short * nVal, int nReadCnt, CString * sVal)
{
	long parsing = 1;
	BOOL bBit[16] = { 0, };
	int nDec[2] = { 0, };
	CString str("");
	(*sVal) = L"";
	for (int i = 0; i < nReadCnt; i++) {
		memset(bBit, 0, sizeof(bBit));
		memset(nDec, 0, sizeof(nDec));
		parsing = 1;
		for (int j = 0; j < 8; j++) {
			nDec[0] += ((nVal[i] & (1 << j)) >> j) * parsing;
			nDec[1] += ((nVal[i] & (1 << (j + 8))) >> (j + 8)) * parsing;
			parsing *= 2;
		}
		str.Format(L"%c%c", nDec[0], nDec[1]);
		(*sVal) += str;
	}
}

void CCtrlPLC_Melsec::SetDWordParsing(int nDecimalPoint, double dVal, int * nVal)
{
	BOOL bRet = FALSE;
	BOOL bNegative = FALSE;
	long parsing = 1;
	long lValue[2] = { 0, };
	double dTemp = dVal;

	if (dVal < 0) {
		bNegative = TRUE;
		dTemp = fabs(dVal);
	}

	long nSetValue = long(dTemp * pow(10, nDecimalPoint) + 0.5);

	for (int i = 0; i < 16; i++) {
		bRet = (nSetValue & (1 << i)) >> i;
		nVal[0] += bRet * parsing;

		bRet = (nSetValue & (1 << (i + 16))) >> (i + 16);
		nVal[1] += bRet * parsing;
		parsing *= 2;
	}

	if (bNegative) {
		if (nVal[0] != 0) nVal[0] *= -1;
		else			  nVal[1] *= -1;
	}
}

void CCtrlPLC_Melsec::GetDWordParsing(int nDecimalPoint, int * nVal, double *dVal)
{
	BOOL bRet = FALSE;
	BOOL bNegative = FALSE;
	long parsing = 1;
	BOOL bBit[32] = { 0, };
	long lRet = 0;

	if (nVal[0] < 0 || nVal[1] < 0) {
		bNegative = TRUE;
	}

	for (int i = 0; i < 16; i++) {
		bBit[i] = (nVal[0] & (1 << i)) >> i;
		bBit[i + 16] = (nVal[1] & (1 << i)) >> i;
	}

	for (int i = 0; i < 32; i++) {
		lRet += parsing * bBit[i];
		parsing *= 2;
	}

	if (bNegative) {
		*dVal = lRet / pow(10, nDecimalPoint) * -1;
	}
	else {
		*dVal = lRet / pow(10, nDecimalPoint);
	}
}