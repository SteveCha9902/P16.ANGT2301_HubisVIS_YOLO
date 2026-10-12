#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class CDlgVisionMain;

class ALGORITHM_TOOLS_API CSocketManager
{
public:
	CSocketManager();
	~CSocketManager();

	CDlgVisionMain* m_pParent;

	BOOL m_bTryingServerConnect;
	BOOL m_bConnect;

	void SetParent(CDlgVisionMain* pParent);
	void SetLaserSocketInfo(const CString ip, const long port, const long idx);

	void InitThread();
	void FreeThread();

	///////////////////////////////////////////////////////////////
	// Thread ฐüทร
	struct SClientControl
	{
		HANDLE m_hSignal_Start;
		HANDLE m_hSignal_Kill;
		HANDLE m_Handle_Thread;
	};
	SClientControl m_Control_Client;

	SOCKET m_sock;
	
	long m_nPort;
	CString m_strIP;
	long m_nSocketVecIdx;

	SDataInfo m_sDataInfo;

	static UINT ThreadClientFunc(LPVOID pParam);
	void ThreadClient();
	///////////////////////////////////////////////////////////////

	void MessageProcess(SDataInfo &cursDataInfo);
	void SendToData(SDataInfo &sDataInfo);

	void SendMsg_SendResult();
	void SendMsg_Alive();
};

extern ALGORITHM_TOOLS_API std::vector<CSocketManager> g_SocketManager;