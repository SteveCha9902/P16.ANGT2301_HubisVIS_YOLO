#include "pch.h"
#include "GigECamera.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace GenApi;
using namespace Basler_GigECameraParams;

bool CGigECamera::PersistentIpAddr(CString sSerialNumber, CString sIpAddr, CString sSubnetMask, CString sGateway)
{

	IGigETransportLayer* const gigeTL = dynamic_cast<IGigETransportLayer*>(CTlFactory::GetInstance().CreateTl(BaslerGigEDeviceClass));

	if(gigeTL)
	{
		CString sMacAddr = _T("");
		DeviceInfoList_t dil;
		int nCamera = gigeTL->EnumerateAllDevices(dil);
		CTlFactory& TlFactory = CTlFactory::GetInstance();

		for(int i = 0; i < nCamera; i++)
		{
			CString sDevSerialNumber = (CString)(String_t)dil[i].GetSerialNumber();
			if(sDevSerialNumber == sSerialNumber)
			{
				CBaslerGigEDeviceInfo &bdi = static_cast<CBaslerGigEDeviceInfo&>(dil[i]);
				sMacAddr = bdi.GetMacAddress();
			}
		}

		if(sMacAddr != _T(""))
		{
			gigeTL->ForceIp((String_t)sMacAddr.GetBuffer(), (String_t)sIpAddr.GetBuffer(), (String_t)sSubnetMask.GetBuffer(), "0.0.0.0");

			CBaslerGigEDeviceInfo devInfo;
			devInfo.SetIpAddress((String_t)sIpAddr.GetBuffer());
			devInfo.SetSubnetMask((String_t)sSubnetMask.GetBuffer());

			GigECamera_t camera;
			camera.Attach(TlFactory.CreateDevice(devInfo));
			camera.Open();

			camera.ChangeIpConfiguration(true, true);
			camera.SetPersistentIpAddress((String_t)sIpAddr.GetBuffer(), (String_t)sSubnetMask.GetBuffer(), (String_t)sGateway.GetBuffer());
			camera.Close();
		}
	}

	return true;
}

CGigECamera::CGigECamera(CString sIpAddr, int nContext)
	: m_nLogIndex(0)
	, m_pBuffer(NULL)
	, m_pBufferC(NULL)
	, m_bOpen(FALSE)
	, m_pbyImage(nullptr)
	, m_bLive(FALSE)
	, m_hGrabDone(nullptr)
{
	m_pBaslerCamera = NULL;
	m_nMode = MODE_SOFTTRIGGER_GIGE;

	m_sIpAddr = sIpAddr;
	m_nContext = nContext;

	m_hView = NULL;

	m_fFPS = 0;
	m_nGrabCount = 0;
	QueryPerformanceFrequency(&TimerFrequency);
	QueryPerformanceCounter(&StartTime);
}

CGigECamera::~CGigECamera(void)
{
	if(m_pbyImage != nullptr)
		delete[] m_pbyImage;

	Close();
}

bool CGigECamera::Open(void)
{
	bool bRet = false;

	/********************************************************************************/
	/* Set Device Info                                                              */
	CTlFactory& tlFactory = CTlFactory::GetInstance();
	CBaslerGigEDeviceInfo di;
	di.SetIpAddress(String_t(m_sIpAddr.GetBuffer()));
	/* Set Device Info                                                              */
	/********************************************************************************/

	try
	{
		IPylonDevice* pIPylonDevice = tlFactory.CreateDevice(di);
		m_pBaslerCamera = new GigECamera_t(pIPylonDevice);
		m_pBaslerCamera->SetCameraContext(m_nContext);
		m_pBaslerCamera->RegisterImageEventHandler(this, RegistrationMode_Append, Cleanup_None);
		m_pBaslerCamera->RegisterConfiguration(this, RegistrationMode_ReplaceAll, Cleanup_None);

		m_pBaslerCamera->Open();


		bRet = true;

		m_pbyImage = new BYTE[m_pBaslerCamera->Width.GetValue() * m_pBaslerCamera->Height.GetValue()];

		m_hGrabDone = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	catch(...)
	{
		Close();
		bRet = false;
	}

	return bRet;
}

bool CGigECamera::GrabStart()
{
	bool res = false;

	if(m_pBaslerCamera != NULL)
	{
		try
		{
			m_pBaslerCamera->StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);

			res = true;

			m_bLive = TRUE;

			//열거 :
			//GrabStrategy_OneByOne.			: 이미지 순서대로 처리
			//GrabStrategy_LatestImageOnly.	: 마지막 이미지만 저장
			//GrabStrategy_UpcomingImage.		: 입력버퍼에 계속 갱신처리
		}
		catch(GenICam::GenericException ex)
		{
			AddLog(_T("Error"), _T("CGigECamera::Grab"), (CString)CStringA(ex.GetDescription()));
		}
		catch(...)//catch (CException* e)
		{
			AddLog(_T("Error"), _T("CGigECamera::Grab"), _T("failed : StartGrabbing"));
		}
	}

	return res;
}

void CGigECamera::Stop(void)
{
	if(m_pBaslerCamera != NULL)
	{
		try
		{
			if(IsGrabbing())
			{
				m_pBaslerCamera->StopGrabbing();

				m_bLive = FALSE;
			}
		}
		catch(...)
		{
			AddLog(_T("Error"), _T("CGigECamera::Stop"), _T("failed : StopGrabbing"));
		}
	}
}

void CGigECamera::Close(void)
{
	if(m_pBaslerCamera != NULL)
	{
		Stop();

		m_pBaslerCamera->Close();

		m_pBaslerCamera->DeregisterImageEventHandler(this);
		m_pBaslerCamera->DeregisterConfiguration(this);

		delete m_pBaslerCamera;
		m_pBaslerCamera = NULL;

	}
}

bool CGigECamera::IsGrabbing(void)
{
	bool res = false;

	if(m_pBaslerCamera != NULL)
	{
		try
		{
			res = m_pBaslerCamera->IsGrabbing();
		}
		catch(...)
		{
		}
	}

	return res;
}

bool CGigECamera::SetMode(eMODE_GIGE nMode)
{
	bool bRet = false;

	if(m_pBaslerCamera != NULL)
	{
		INodeMap &Control = m_pBaslerCamera->GetNodeMap();
		CEnumerationPtr triggerSelector(Control.GetNode("TriggerSelector"));
		CEnumerationPtr triggerMode(Control.GetNode("TriggerMode"));
		CEnumerationPtr triggerSource(Control.GetNode("TriggerSource"));

		switch(nMode)
		{
		case MODE_SOFTTRIGGER_GIGE:
			triggerMode->FromString("On");
			triggerSource->FromString("Software");
			bRet = true;
			break;

		case MODE_HARDTRIGGER_GIGE:
			triggerMode->FromString("On");
			triggerSource->FromString("Line1");
			bRet = true;
			break;

		case MODE_CONTINUE_GIGE:
			triggerSelector->FromString("FrameStart");
			triggerMode->FromString("Off");
			bRet = true;
			break;
		}

		m_nMode = nMode;
	}

	return bRet;
}

void CGigECamera::OnImageGrabbed(CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult)
{
	CString strTemp;
	static long index = 0;

	if(ptrGrabResult->GrabSucceeded() == true)
	{
		int nWidth = ptrGrabResult->GetWidth();
		int nHeight = ptrGrabResult->GetHeight();

		memcpy(m_pbyImage, (BYTE *)ptrGrabResult->GetBuffer(), nWidth * nHeight);

		m_pBaslerCamera->StopGrabbing(); // 한번만 Grab 하고 멈춘다.

		::SetEvent(m_hGrabDone);
	}
}

void CGigECamera::SoftTriggerSend()
{
	if(m_pBaslerCamera != NULL)
	{
		m_pBaslerCamera->ExecuteSoftwareTrigger();
	}
}

void CGigECamera::OnCameraEvent(CBaslerGigEInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* pNode)
{
	CString str;
	str.Format(_T("%d"), camera.GetCameraContext());
	switch(userProvidedId)
	{
	case eExposureEndEvent_GIGE:
		break;

	default:
		break;
	}
}

void CGigECamera::OnCameraDeviceRemoved(CInstantCamera& camera)
{
	AddLog(_T("Error"), _T("CGigECamera::OnCameraDeviceRemoved"), _T("Camera Disconnected."));
	TRACE("OnCameraDeviceRemoved\n");
}

void CGigECamera::SetExpose(int64_t nValue)
{
	if(m_pBaslerCamera != NULL)
		m_pBaslerCamera->ExposureTimeRaw.SetValue(nValue);
}

void CGigECamera::SetPacketSize(int64_t nValue)
{
	if(m_pBaslerCamera != NULL)
	{
		INodeMap &Control = m_pBaslerCamera->GetNodeMap();
		CIntegerPtr packetSize(Control.GetNode("GevSCPSPacketSize"));

		packetSize->SetValue(nValue);
	}
}

void CGigECamera::SetAOI(int64_t nX, int64_t nY, int64_t nWidth, int64_t nHeight)
{
	if(m_pBaslerCamera != NULL)
	{
		if(m_pBaslerCamera->IsOpen() == true)
		{
			bool bGrabbing = m_pBaslerCamera->IsGrabbing();

			if(bGrabbing == true)
			{
				m_pBaslerCamera->StopGrabbing();
			}
			m_pBaslerCamera->OffsetX.SetValue(nX);
			m_pBaslerCamera->OffsetY.SetValue(nY);
			m_pBaslerCamera->Width.SetValue(nWidth);
			m_pBaslerCamera->Height.SetValue(nHeight);
			//m_pBaslerCamera->PixelFormat.SetValue(PixelFormat_BayerBG8);
			m_pBaslerCamera->PixelFormat.SetValue(PixelFormat_Mono8);
			m_pBaslerCamera->AcquisitionFrameRateEnable.SetValue(false);

			if(bGrabbing == true)
			{
				m_pBaslerCamera->StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
			}
		}
	}
}

void CGigECamera::AddLog(CString arg_log)
{
	TRACE(arg_log);
	TRACE("\n");
}

void CGigECamera::AddLog(CString arg_category, CString arg_function, CString arg_msg)
{
	CString sTmp;
	sTmp.Format(_T("[%06d] [%-20s]: [%s] > %s"), m_nLogIndex++, arg_category, arg_function, arg_msg);

	AddLog(sTmp);
}

void CGigECamera::OnOpened(CInstantCamera& camera)
{
	using namespace GenApi;

	try
	{
		// Get the camera control object.
		INodeMap &Control = camera.GetNodeMap();

		// Get the trigger selector node.
		CEnumerationPtr triggerSelector(Control.GetNode("TriggerSelector"));
		// Get the trigger mode node.
		CEnumerationPtr triggerMode(Control.GetNode("TriggerMode"));

		// Check the available camera trigger mode(s) to select the appropriate one: acquisition start trigger mode 
		// (used by previous cameras, i.e. for cameras supporting only the legacy image acquisition control mode;
		// do not confuse with acquisition start command) or frame start trigger mode
		// (used by newer cameras, i.e.for cameras using the standard image acquisition control mode;
		// equivalent to the acquisition start trigger mode in the leagcy image acquisition control mode).
		bool frameStartAvailable = false;
		bool acquisitionStartAvailable = false;
		{
			// Frame start trigger mode available?
			GenApi::IEnumEntry* frameStart = triggerSelector->GetEntryByName("FrameStart");
			frameStartAvailable = frameStart && IsAvailable(frameStart);

			// Acquisition start trigger mode available?
			GenApi::IEnumEntry* acquisitionStart = triggerSelector->GetEntryByName("AcquisitionStart");
			acquisitionStartAvailable = acquisitionStart && IsAvailable(acquisitionStart);
		}

		// Preselect the trigger mode for image acquisition.
		const char* triggerSelectorValue = "FrameStart";

		// Check to see if the camera implements the acquisition start trigger mode only.
		if(acquisitionStartAvailable && !frameStartAvailable)
		{
			// Camera uses the acquisition start trigger as the only trigger mode.
			triggerSelector->FromString("AcquisitionStart");
			triggerMode->FromString("On");
			triggerSelectorValue = "AcquisitionStart";
		}
		else
		{
			// Camera may have the acquisition start trigger mode and the frame start trigger mode implemented.
			// In this case, the acquisition trigger mode must be switched off.
			if(acquisitionStartAvailable)
			{
				triggerSelector->FromString("AcquisitionStart");
				triggerMode->FromString("Off");
			}
			// To trigger each single frame by software or external hardware trigger: Enable the frame start trigger mode.
			assert(frameStartAvailable); //Frame start trigger mode must be available here.
			triggerSelector->FromString("FrameStart");
			//triggerMode->FromString("On");
			triggerMode->FromString("Off");
		}

		// Note: The trigger selector must be set to the appropriate trigger mode 
		// before setting the trigger source or issuing software triggers.
		// Frame start trigger mode for newer cameras (i.e. for cameras supporting the standard image acquisition control mode).
		triggerSelector->FromString(triggerSelectorValue);

		// The trigger source must be set to 'Software'.
		CEnumerationPtr(Control.GetNode("TriggerSource"))->FromString("Software");

		//// Alternative hardware trigger configuration:
		//// This configuration can be copied and modified to create a hardware trigger configuration.
		//// Remove setting the 'TriggerSource' to 'Software' (see above) and 
		//// use the commented lines as a starting point.
		//// The camera user's manual contains more information about available configurations.
		//// The pylon Viewer tool can be used to test the selected settings first.

		//// The trigger source must be set to the trigger input, e.g. 'Line1'.
		//CEnumerationPtr(Control.GetNode("TriggerSource"))->FromString("Line1");

		////The trigger activation must be set to e.g. 'RisingEdge'.
		//CEnumerationPtr(Control.GetNode("TriggerActivation"))->FromString("RisingEdge");

		// Enable the continuous acquisition mode.
		CEnumerationPtr(Control.GetNode("AcquisitionMode"))->FromString("Continuous");
	}
	catch(GenICam::GenericException& e)
	{
		throw RUNTIME_EXCEPTION("Could not apply configuration. GenICam::GenericException caught in OnOpened method msg=%hs", e.what());
	}
	catch(std::exception& e)
	{
		throw RUNTIME_EXCEPTION("Could not apply configuration. std::exception caught in OnOpened method msg=%hs", e.what());
	}
	catch(...)
	{
		throw RUNTIME_EXCEPTION("Could not apply configuration. Unknown exception caught in OnOpened method.");
	}
}

BOOL &CGigECamera::IsOpen()
{
	return m_bOpen;
}

long CGigECamera::GetWidth()
{
	if(m_pBaslerCamera == nullptr)
		return 0;

	return (long)m_pBaslerCamera->Width.GetValue();
}

long CGigECamera::GetHeight()
{
	if(m_pBaslerCamera == nullptr)
		return 0;

	return (long)m_pBaslerCamera->Height.GetValue();
}

void CGigECamera::SetNodeMap(char* szPathFile)
{
	CFeaturePersistence::Load(szPathFile, &m_pBaslerCamera->GetNodeMap(), TRUE);
}

BYTE* CGigECamera::GetImage()
{
	return m_pbyImage;
}

HANDLE CGigECamera::GetGrabDoneHandle()
{
	return m_hGrabDone;
}