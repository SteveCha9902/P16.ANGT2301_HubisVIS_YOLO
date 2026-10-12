#include	<afx.h>

#ifndef		_COMM_H
#define		_COMM_H

#define		MAXBLOCK		4096
#define		MAX_PORT		8

#define		COM1			1
#define		COM2			2
#define		COM3			3
#define		COM4			4
#define		COM5			5
#define		COM6			6
#define		COM7			7
#define		COM8			8


class CFlexLibComm : public CObject
{
	DECLARE_DYNCREATE(CFlexLibComm)

public:
	HANDLE	idCommDev;	
	BOOL	fConnected;
	char	abIn[MAXBLOCK + 1];
	char	abScannerIn[MAXBLOCK + 1];
	UINT	nLength;
	UINT	dScannerLength;
	BYTE	fDataStatus;
	HWND	m_hWnd;
public:
	CFlexLibComm();
	~CFlexLibComm();
	void	SetCommPort(int port, DWORD rate, BYTE bytesize, BYTE stop, BYTE parity);
	BOOL	CreateComm(void);
	BOOL	DestroyComm(void);
	int		ReadCommBlock(LPSTR lpszBlock, int nMaxLength);
	BOOL	WriteCommBlock(LPSTR lpszBlock, DWORD dwBytesWrite);
	BOOL	OpenCommPort(LPVOID lpData);
	BOOL	SetupConnection(void);
	BOOL	CloseConnection(void);
	void	SetHwnd(HWND hWnd);
	void	PurgeCommPort(void);

	void	SetPcCommReadData(char *data);
	void	SetNmpCommReadData(char *data);
	void	SetLoadcellReadData(char *data);
	void	OnCommDataRcv(void);
	void	OnScaleCommDataRcv();
	void	SetScannerReadData(char * data);
	//Attributes
public:
	BYTE	bPort, bByteSize, bParity, bStopBits;
	DWORD	dwBaudRate;
	OVERLAPPED osWrite, osRead;
};

#endif

