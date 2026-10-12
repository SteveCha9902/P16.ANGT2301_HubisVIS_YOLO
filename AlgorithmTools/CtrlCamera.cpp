#include "pch.h"
#include "CtrlCamera.h"
#include "GigECamera.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCtrlCamera::CCtrlCamera()
	: m_pGigECamera(nullptr)
	, m_bOpen(FALSE)
	, m_nImageWidth(0)
	, m_nImageHeight(0)
	, m_nImageSize(0)
{
}

CCtrlCamera::~CCtrlCamera()
{
	if(m_bOpen == TRUE)
	{
		Close();
	}

	if(m_pGigECamera != nullptr)
	{
		delete m_pGigECamera;
		m_pGigECamera = nullptr;
	}
}

BOOL CCtrlCamera::Open()
{
	//BOOL bOpen(FALSE);

	if(m_pGigECamera != nullptr)
	{
		delete m_pGigECamera;
		m_pGigECamera = nullptr;
	}

	_putenv("PYLON_GIGE_HEARTBEAT=5000");

	m_pGigECamera = new CGigECamera(_T("192.168.4.2"));

	m_bOpen = m_pGigECamera->Open(); // 변수 변경 - SHK 2024.01.02

	if(m_bOpen == TRUE) // 변수 변경 - SHK 2024.01.02
	{
		m_nImageWidth = (long)m_pGigECamera->GetWidth();
		m_nImageHeight = (long)m_pGigECamera->GetHeight();
		m_nImageSize = m_nImageWidth * m_nImageHeight;
	}

	return m_bOpen;
}

// Gige Camera IP 추가 - SHK 2023.12.27
BOOL CCtrlCamera::Open(CString strIP)
{
	//BOOL bOpen(FALSE);

	if (m_pGigECamera != nullptr)
	{
		delete m_pGigECamera;
		m_pGigECamera = nullptr;
	}

	_putenv("PYLON_GIGE_HEARTBEAT=5000");

	m_pGigECamera = new CGigECamera(strIP);

	m_bOpen = m_pGigECamera->Open(); // 변수 변경 - SHK 2024.01.02

	if (m_bOpen == TRUE) // 변수 변경 - SHK 2024.01.02
	{
		m_nImageWidth = (long)m_pGigECamera->GetWidth();
		m_nImageHeight = (long)m_pGigECamera->GetHeight();
		m_nImageSize = m_nImageWidth * m_nImageHeight;
	}

	return m_bOpen;
}

BOOL CCtrlCamera::Close()
{
	m_pGigECamera->Close();

	return TRUE;
}

void CCtrlCamera::SetSoftTrig()
{
	m_pGigECamera->GrabStart();
	const DWORD dwStartTick = GetTickCount();

	while(1) // Event 호출(::WaitForSingleObject)로 수정 필요, 테스트 필요
	{
		if(GetTickCount() - dwStartTick >= 2000)
		{
			::g_Log.Log(_T("[CAMERA_DIAG][ERROR] SetSoftTrig timeout after 2000ms - forcing stop"));
			m_pGigECamera->Stop();
			break;
		}
		if(m_pGigECamera->IsGrabbing() == FALSE)
		{
			break;
		}

		Sleep(1);
	}
}

void CCtrlCamera::Grab1Shot()
{
	m_pGigECamera->GrabStart();

	const DWORD dwStartTick = GetTickCount();
	while(1) // Event 호출(::WaitForSingleObject)로 수정 필요, 테스트 필요
	{
		if(m_pGigECamera->IsGrabbing() == FALSE)
		{
			break;
		}
		if(GetTickCount() - dwStartTick >= 2000)
		{
			::g_Log.Log(_T("[CAMERA_DIAG][ERROR] Grab1Shot timeout after 2000ms - forcing stop"));
			m_pGigECamera->Stop();
			break;
		}

		Sleep(1);
	}
}

void CCtrlCamera::GrabStop()
{
	m_pGigECamera->Stop();
}

void CCtrlCamera::GrabLive()
{
	if(m_pGigECamera->m_bLive == FALSE)
	{
		m_pGigECamera->GrabStart();
		const DWORD dwStartTick = GetTickCount();

		while(1) // Event 호출(::WaitForSingleObject)로 수정 필요, 테스트 필요
		{
			if(m_pGigECamera->IsGrabbing() == FALSE)
			{
				break;
			}
			if(GetTickCount() - dwStartTick >= 2000)
			{
				::g_Log.Log(_T("[CAMERA_DIAG][ERROR] GrabLive timeout after 2000ms - forcing stop"));
				m_pGigECamera->Stop();
				break;
			}

			Sleep(1);
		}
	}
}

void CCtrlCamera::SetLive(bool bLive)
{
	m_pGigECamera->m_bLive = bLive;
}

long CCtrlCamera::GetImageWidth()
{
	return m_nImageWidth;
}

long CCtrlCamera::GetImageHeight()
{
	return m_nImageHeight;
}

long CCtrlCamera::GetImageSize()
{
	return m_nImageSize;
}

void* CCtrlCamera::GetImage()
{
	return m_pGigECamera->GetImage();

	return nullptr;
}

void* CCtrlCamera::GetGrabberPtr()
{
	return m_pGigECamera;
}

HANDLE CCtrlCamera::GetGrabDoneHandle()
{
	return m_pGigECamera->GetGrabDoneHandle();
}

BOOL CCtrlCamera::IsOpen()
{
	return m_bOpen;
}