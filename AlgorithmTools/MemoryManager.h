#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CMemoryManager
{
public:
	CMemoryManager();
	~CMemoryManager();

	void Clear();
	void SetImageSize(const long imageWidth, const long imageHeight);
	void ResetMemoryUse();
	void SetReserveByteImage(long nNum);
	void SetReserveLongImage(long nNum);
	void SetReserveFloatImage(long nNum);
	BYTE* GetNextByteImage();
	long* GetNextLongImage();
	float* GetNextFloatImage();
	BYTE* GetByteImage(long nOrder = 0);
	long* GetLongImage(long nOrder = 0);
	float* GetFloatImage(long nOrder = 0);

public:
	long m_nImageWidth;
	long m_nImageHeight;
	long m_nUseByteImage;
	long m_nUseLongImage;
	long m_nUseFloatImage;
	std::vector<BYTE*> m_vecByteImage;
	std::vector<long*> m_vecLongImage;
	std::vector<float*> m_vecFloatImage;
};

extern ALGORITHM_TOOLS_API CMemoryManager g_MemoryManager;