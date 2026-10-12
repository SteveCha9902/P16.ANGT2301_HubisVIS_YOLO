#pragma once

#ifdef VISION_MAIN_DLL_EXPORT
#define VISION_MAIN_DLL_API	__declspec(dllexport)
#else
#define VISION_MAIN_DLL_API	__declspec(dllimport)
#endif

class VISION_MAIN_DLL_API CLink
{
public:
	CLink();
	~CLink();

	INT_PTR DoModal();
};

