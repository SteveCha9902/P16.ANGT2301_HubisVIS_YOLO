#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class CGigECamera;

class ALGORITHM_TOOLS_API CCtrlCamera
{
public:
	CCtrlCamera();
	~CCtrlCamera();

private:
	CGigECamera* m_pGigECamera;

	BOOL m_bOpen;
	long m_nImageWidth;
	long m_nImageHeight;
	long m_nImageSize;

public:
	BOOL Open();
	BOOL Open(CString strIP); // Gige Camera IP Ãß°¡ - SHK 2023.12.27
	BOOL Close();
	void SetSoftTrig();
	void Grab1Shot();
	void GrabStop();
	void GrabLive();
	void SetLive(bool bLive);
	long GetImageWidth();
	long GetImageHeight();
	long GetImageSize();
	void* GetImage();
	void* GetGrabberPtr();
	HANDLE GetGrabDoneHandle();
	BOOL IsOpen();
};
