#include "pch.h"
#include "SocketManager.h"
#include "../VisionMain/DlgVisionMain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BUFFER_SIZE 10000

std::vector <CSocketManager> g_SocketManager;

CSocketManager::CSocketManager()
	: m_pParent(nullptr)
	, m_nSocketVecIdx(0)
	, m_sock(INVALID_SOCKET)
{
	m_bTryingServerConnect = FALSE;
	m_bConnect = FALSE;
}

CSocketManager::~CSocketManager()
{
}

void CSocketManager::InitThread()
{
	// Client Thread 초기화
	m_Control_Client.m_hSignal_Start = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_Client.m_hSignal_Kill = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Control_Client.m_Handle_Thread = INVALID_HANDLE_VALUE;

	HANDLE hClientThread = ::AfxBeginThread(ThreadClientFunc, (void *)this)->m_hThread;
	::DuplicateHandle(::GetCurrentProcess(), hClientThread, ::GetCurrentProcess(), &m_Control_Client.m_Handle_Thread, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);
}

void CSocketManager::FreeThread()
{

	// 모든 쓰레드에게 일을 그만하라고 알린다.
	::SetEvent(m_Control_Client.m_hSignal_Kill);
	if(m_sock != INVALID_SOCKET)
	{
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	WSACleanup();

	// 쓰레드가 종료되기를 기다린다.
	DWORD dwWaitRet = ::WaitForSingleObject(m_Control_Client.m_Handle_Thread, 5000);
	if(dwWaitRet != WAIT_OBJECT_0)
	{
		::g_Log.Log(_T("[THREAD_DIAG][ERROR] Laser socket thread stop timeout - Index:%d Ret:%lu"), m_nSocketVecIdx, dwWaitRet);
	}

	// 관련 핸들을 모두 닫는다.
	::CloseHandle(m_Control_Client.m_hSignal_Kill);
	::CloseHandle(m_Control_Client.m_Handle_Thread);
	::CloseHandle(m_Control_Client.m_hSignal_Start);
}

void CSocketManager::SetParent(CDlgVisionMain* pParent)
{
	m_pParent = pParent;
}

void CSocketManager::SetLaserSocketInfo(const CString ip, const long port, const long idx)
{
	m_strIP = ip;
	m_nPort = port;
	m_nSocketVecIdx = idx;
}


// 클라이언트 스레드 함수
UINT CSocketManager::ThreadClientFunc(LPVOID pParam)
{
	CSocketManager *pProcessClientThread = (CSocketManager *)pParam;

	pProcessClientThread->ThreadClient();

	return 0;
}

// 클라이언트 스레드
void CSocketManager::ThreadClient()
{
	// 클라이언트 Thread Handle
	HANDLE pHandles[2] = {m_Control_Client.m_hSignal_Start, m_Control_Client.m_hSignal_Kill};

	::g_Log.Log(_T("Socket Client Thread : Thread Ready"));

	while(1)
	{
		DWORD dwRet = ::WaitForMultipleObjects(2, pHandles, FALSE, INFINITE);

		// 클라이언트 시작
		if(dwRet == WAIT_OBJECT_0)
		{
			m_bTryingServerConnect = TRUE;

			// 윈속 초기화
			WSADATA wsa;
			if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
			{
				return;
			}

			// socket()
			m_sock = socket(AF_INET, SOCK_STREAM, 0);
			DWORD dwSocketTimeoutMs = 3000;
			setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwSocketTimeoutMs, sizeof(dwSocketTimeoutMs));
			setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwSocketTimeoutMs, sizeof(dwSocketTimeoutMs));

			// connect()
			SOCKADDR_IN serveraddr;
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons((unsigned short)m_nPort);
			serveraddr.sin_addr.s_addr = inet_addr((CStringA)m_strIP);

			int retval = connect(m_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if(retval == SOCKET_ERROR)
			{
				m_bConnect = FALSE;
				m_bTryingServerConnect = FALSE;

				::g_Log.Log(_T("Socket Client Thread : Attempt to connect to %s:%d failed, server not found."), m_strIP, m_nPort);

				// 아이콘 상태 변경
				::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_LASER_STATUS, 0, FALSE);
			}
			else
			{
				m_bConnect = TRUE;
				m_pParent->KillTimer(LASER_CONNECT);

				::g_Log.Log(_T("Socket Client Thread : Connected to %s:%d."), m_strIP, m_nPort);

				// 아이콘 상태 변경
				::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_LASER_STATUS, 0, TRUE);

				m_sDataInfo.Init();

				// 서버와 데이터 통신
				while(1)
				{
					char buf[BUFFER_SIZE]; // 데이터 송수신 버퍼

					// 데이터 받기
					retval = recv(m_sock, buf, sizeof(buf) - 1, 0);
					if(retval != SOCKET_ERROR && retval != 0 && retval < BUFFER_SIZE)
					{
						// 받은 데이터 출력
						buf[retval] = '\0';

						m_sDataInfo = (SDataInfo &)buf;

						MessageProcess(m_sDataInfo);
					}
					else
					{
						m_bConnect = FALSE;
						if(retval == SOCKET_ERROR && WSAGetLastError() == WSAETIMEDOUT)
						{
							::g_Log.Log(_T("[SOCKET_DIAG] Laser receive idle timeout - connection retained Index:%d"), m_nSocketVecIdx);
							continue;
						}
						m_bTryingServerConnect = FALSE;

						// 접속이 끊겼을 경우 다시 타이머를 시작해서 서버를 찾는다.
						m_pParent->SetTimer(LASER_CONNECT, ::g_ConfigManager.m_nLaserAutoConnectTimes, NULL);

						// 아이콘 상태 변경
						::PostMessage(m_pParent->GetSafeHwnd(), UM_UPDATE_LASER_STATUS, 0, FALSE);

						break;
					}
				}
			}
		}
		else
		{
			m_bConnect = FALSE;
			break;
		}
	}
}

void CSocketManager::SendToData(SDataInfo &sDataInfo)
{
	Sleep(10);

	send(m_sock, (char*)&sDataInfo, sizeof(sDataInfo), 0);
}

// 메시지 처리
void CSocketManager::MessageProcess(SDataInfo &sDataInfo)
{
	switch(sDataInfo.messageIndex)
	{
	case (long)MSG_L2V::L2V_DATA_REQUEST:
		::g_Log.Log(_T("Socket Client Thread : MSG_L2V::L2V_DATA_REQUEST"));
		SendMsg_SendResult();
		break;
	case (long)MSG_L2V::L2V_RECIVE_DONE:
		::g_Log.Log(_T("Socket Client Thread : MSG_L2V::L2V_RECIVE_DONE"));
		break;
	case (long)MSG_L2V::L2V_ALIVE:
		::g_Log.Log(_T("Socket Client Thread : MSG_L2V::L2V_ALIVE"));
		SendMsg_Alive();
		break;
	default:
		::g_Log.Log(_T("Socket Client Thread : Invalid Message"));
		break;
	}
}

// Send Message
void CSocketManager::SendMsg_SendResult()
{
	if(::g_ConfigManager.m_bUseLaserTCPIP == FALSE || m_bConnect == FALSE)
		return;

	SDataInfo sDataInfo;
	sDataInfo.Init();

	long camCount(0);
	long nSendStartIdx(0);
	long nSendEndIdx(0);
	switch(::g_ConfigManager.m_nMainUI)
	{
	case (long)MAINUI::UI_FOR_SINGLE_CAMERA:
		camCount = 1;
		break;
	case (long)MAINUI::UI_FOR_DUAL_CAMERA:
		camCount = 2;
		break;
	}

	nSendStartIdx = m_nSocketVecIdx;
	nSendEndIdx = (camCount / ::g_ConfigManager.m_nLaserTCPIPNum) * (m_nSocketVecIdx + 1);

	long index(0);
	BOOL bResult = TRUE;
	for(long i = nSendStartIdx; i < nSendEndIdx; i++)
	{
		for(long j = 0; j < ::g_ConfigManager.m_nDivisionIndex; j++)
		{
			sDataInfo.startPointX[index] = ::g_InspectionManager.m_vecsInspEdgeResult[i][j].ptStart.x;
			sDataInfo.startPointY[index] = ::g_InspectionManager.m_vecsInspEdgeResult[i][j].ptStart.y;
			sDataInfo.endPointX[index] = ::g_InspectionManager.m_vecsInspEdgeResult[i][j].ptEnd.x;
			sDataInfo.endPointY[index] = ::g_InspectionManager.m_vecsInspEdgeResult[i][j].ptEnd.y;
			sDataInfo.dx[index] = ::g_InspectionManager.m_vecsInspEdgeResult[i][j].fdx;
			sDataInfo.dy[index] = ::g_InspectionManager.m_vecsInspEdgeResult[i][j].fdy;
			sDataInfo.angle[index] = ::g_InspectionManager.m_vecsInspEdgeResult[i][j].fAngle;
			
			if (bResult == TRUE)
			{
				bResult = ::g_InspectionManager.m_vecsInspEdgeResult[i][j].bResult;
			}

			index++;
		}
	}
	
	CString strTemp = ::g_PLCManager.GetBarcode();
	long length = strTemp.GetLength();
	if (length == 0)
	{
		strTemp = _T("BarcodeEmpty");
	}
	strcpy_s(sDataInfo.sBarcode, sizeof(sDataInfo.sBarcode), CStringA(strTemp));


	sDataInfo.status = bResult;
	sDataInfo.messageIndex = (long)MSG_V2L::V2L_SEND_RESULT;

	SendToData(sDataInfo);

	::g_Log.Log(_T("Socket Client Thread : MSG_V2L::V2L_SEND_RESULT - Success"));
}

void CSocketManager::SendMsg_Alive()
{
	if(::g_ConfigManager.m_bUseLaserTCPIP == FALSE || m_bConnect == FALSE)
		return;

	SDataInfo sDataInfo;
	sDataInfo.Init();

	sDataInfo.status = 1;
	sDataInfo.messageIndex = (long)MSG_V2L::V2L_ALIVE;

	SendToData(sDataInfo);

	::g_Log.Log(_T("Socket Client Thread : MSG_V2L::V2L_ALIVE - Success"));
}
