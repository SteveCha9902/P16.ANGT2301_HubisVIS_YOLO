#pragma once

#include <pylon/PylonIncludes.h>
#include <pylon/gige/PylonGigEIncludes.h>
#include <pylon/FeaturePersistence.h>

typedef Pylon::CBaslerGigEInstantCamera GigECamera_t;

enum eMODE_GIGE
{
	MODE_SOFTTRIGGER_GIGE,				// Soft-Trigger mode
	MODE_HARDTRIGGER_GIGE,				// Hard-Trigger mode
	MODE_CONTINUE_GIGE					// Free Run mode
};

enum eEvents_GIGE
{
	eExposureEndEvent_GIGE = 100,
	eEventOverrunEvent_GIGE = 200
};


using namespace Pylon;

/********************************************************************************/
/* CGigECamera class                                                            */
/* CGigECamera class                                                            */
/********************************************************************************/
class CGigECamera
	: public CImageEventHandler, public CBaslerGigECameraEventHandler, public CConfigurationEventHandler
{
	/********************************************************************************/
	/* etc                                                                          */
public:
	static bool					PersistentIpAddr(CString sSerialNumber, CString sIpAddr, CString sSubnetMask, CString sGateway);
	/* etc                                                                          */
	/********************************************************************************/

	/********************************************************************************/
	/* Constructor & Destructor                                                     */
public:
	CGigECamera(CString sIpAddr, int nContext = 0);
	~CGigECamera(void);
	/* Constructor & Destructor                                                     */
	/********************************************************************************/

	/********************************************************************************/
	/* Basic functions                                                              */
private:
	PylonAutoInitTerm			m_PylonAutoInitTerm;
public:
	bool						Open(void);
	bool						GrabStart();
	void						Stop(void);
	void						Close(void);
	bool						SetMode(eMODE_GIGE nMode);
	void						SoftTriggerSend();

	bool						IsGrabbing(void);

public:
	void						SetAOI(int64_t nX, int64_t nY, int64_t nWidth, int64_t nHeight);
	/* Basic functions                                                              */
	/********************************************************************************/

	/********************************************************************************/
	/* Expert functions                                                             */
public:
	void						SetExpose(int64_t nValue);
	void						SetPacketSize(int64_t nValue);
	/* Expert functions                                                             */
	/********************************************************************************/

	/********************************************************************************/
	/* for Events                                                                   */
public:
	CImageFormatConverter		converter;
	virtual void				OnImageGrabbed(CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult);
	virtual void				OnCameraEvent(CBaslerGigEInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* pNode);
	virtual void				OnOpened(CInstantCamera& camera);
	virtual void				OnCameraDeviceRemoved(CInstantCamera& camera);
	/* for Events                                                                   */
	/********************************************************************************/

	/********************************************************************************/
	/* Attribute : for Image                                                        */
private:
	byte*						m_pBuffer;
	byte*						m_pBufferC;
	/* Attribute : for Image                                                        */
	/********************************************************************************/

	/********************************************************************************/
	/* Attribute                                                                    */
private:
	CString						m_sIpAddr;

	GigECamera_t*					m_pBaslerCamera;
	int							m_nContext;
	int							m_nMode;
	HWND						m_hView;
	/* Attribute                                                                    */
	/********************************************************************************/


	/********************************************************************************/
	/* for Debug                                                                    */
private:
	long						m_nLogIndex;
public:
	void						AddLog(CString arg_log);
	void						AddLog(CString arg_category, CString arg_function, CString arg_msg);
	/* for Debug                                                                    */
	/********************************************************************************/

	float			m_fFPS;
	LARGE_INTEGER		TimerFrequency; // 타이머 빈도수를 저장한다.
	LARGE_INTEGER		StartTime; // 프레임 카운트가 0일때 시간이 저장되는 변수

	int 			m_nGrabCount;

public:
	BYTE* m_pbyImage;
	BOOL m_bOpen;
	BOOL m_bLive;
	HANDLE	 m_hGrabDone;

	BOOL &IsOpen();
	long GetWidth();
	long GetHeight();
	void SetNodeMap(char* szPathFile);
	BYTE* GetImage();
	HANDLE GetGrabDoneHandle();
};

