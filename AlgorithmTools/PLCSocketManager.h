#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class CDlgVisionMain;
class CCtrlPLC;

class ALGORITHM_TOOLS_API CPLCSocketManager
{
public:
	CPLCSocketManager();
	~CPLCSocketManager();

	CDlgVisionMain* m_pParent;
	CCtrlPLC* m_pCtrlPLC;

	BOOL m_bTryingServerConnect;
	BOOL m_bConnect;
	CString m_strScanTime;
	long m_ScanCount; // 250307 PSJ  Scan 횟수 추가 기존 3회 Load하던 레시피 -> 1회만 Load 하기 위한 목적으로 생성

	void SetParent(CDlgVisionMain* pParent);
	void RequestLinkTestWrite(const BOOL bOn);

	void InitThread();
	void FreeThread();

	BOOL IsConnect();

	///////////////////////////////////////////////////////////////
	// Thread 관련
	struct SClientControl
	{
		HANDLE m_hSignal_Start;
		HANDLE m_hSignal_Kill;
		HANDLE m_Handle_Thread;
	};
	SClientControl m_Control_Client;

	SOCKET m_sock;

	//PLC에서 받아온 데이터 파싱한 것들
	CString m_strBarcode;
	CString m_strModel;
	long m_nCameraIndex;
	long m_nDivIndex;
	BOOL m_bStart;
	long m_nJobNumber;
	BOOL m_bScanDone;
	BOOL m_bInitialize;
	BOOL m_bPrevStartSignal;
	DWORD m_dwStartSignalOnTick;
	long m_nStartSignalSeq;
	BOOL m_bPrevMoveRespSignal;
	BOOL m_bMoveReqOn;
	DWORD m_dwMoveReqOnTick;
	DWORD m_dwMoveRespOnTick;
	volatile LONG m_nPendingLinkTestValue;

	struct SStartSignalContext
	{
		long seq;
		long jobNumber;
		long cameraIndex;
		long divIndex;
		CString barcode;
		CString model;
		DWORD reqOnTick;

		SStartSignalContext()
			: seq(0)
			, jobNumber(0)
			, cameraIndex(0)
			, divIndex(0)
			, reqOnTick(0)
		{
		}
	};


	static UINT ThreadClientFunc(LPVOID pParam);
	void ThreadClient();

	void MessageProcess();
	void MessageProcessSimul(int nCameraIndex, int nDivIndex, int nJobNumber, bool bScanDone);

	void ScanStart(const long cameraIndex, const long divIndex, const CString barcode = _T(""));
	///////////////////////////////////////////////////////////////
};

extern ALGORITHM_TOOLS_API CPLCSocketManager g_PLCSocketManager;
