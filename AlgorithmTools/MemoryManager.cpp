#include "pch.h"
#include "MemoryManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMemoryManager g_MemoryManager;

CMemoryManager::CMemoryManager()
{
	m_nImageWidth = 0;
	m_nImageHeight = 0;

	ResetMemoryUse();
}

CMemoryManager::~CMemoryManager()
{
	Clear();
}

void CMemoryManager::Clear()
{
	for(long i = 0; i < (long)m_vecByteImage.size(); i++)
	{
		BYTE* pData = m_vecByteImage[i];
		delete[] pData;
	}
	m_vecByteImage.clear();
	for(long i = 0; i < (long)m_vecLongImage.size(); i++)
	{
		long* pData = m_vecLongImage[i];
		delete[] pData;
	}
	m_vecLongImage.clear();
	for(long i = 0; i < (long)m_vecFloatImage.size(); i++)
	{
		float* pData = m_vecFloatImage[i];
		delete[] pData;
	}
	m_vecFloatImage.clear();

	ResetMemoryUse();
}

void CMemoryManager::SetImageSize(const long imageWidth, const long imageHeight)
{
	if(m_nImageWidth != imageWidth || m_nImageHeight != imageHeight)
	{
		Clear();

		m_nImageWidth = imageWidth;
		m_nImageHeight = imageHeight;

		SetReserveByteImage((long)IMAGE::END);
		SetReserveLongImage(1);
	}
}

void CMemoryManager::ResetMemoryUse()
{
	m_nUseByteImage = 0;
	m_nUseLongImage = 0;
	m_nUseFloatImage = 0;
}

void CMemoryManager::SetReserveByteImage(long nNum)
{
	long i;
	for(i = (long)m_vecByteImage.size(); i < nNum; i++)
	{
		BYTE* pData = new BYTE[m_nImageWidth * m_nImageHeight];
		memset(pData, 0, m_nImageWidth * m_nImageHeight);
		m_vecByteImage.push_back(pData);
	}
}

void CMemoryManager::SetReserveLongImage(long nNum)
{
	long i;
	for(i = (long)m_vecLongImage.size(); i < nNum; i++)
	{
		long* pData = new long[m_nImageWidth * m_nImageHeight];
		memset(pData, 0, m_nImageWidth * m_nImageHeight * sizeof(long));
		m_vecLongImage.push_back(pData);
	}
}

void CMemoryManager::SetReserveFloatImage(long nNum)
{
	long i;
	for(i = (long)m_vecFloatImage.size(); i < nNum; i++)
	{
		float* pData = new float[m_nImageWidth * m_nImageHeight];
		memset(pData, 0, m_nImageWidth * m_nImageHeight * sizeof(float));
		m_vecFloatImage.push_back(pData);
	}
}

BYTE* CMemoryManager::GetNextByteImage()
{
	BYTE* pData;
	if(m_nUseByteImage >= (long)m_vecByteImage.size())
	{
		pData = new BYTE[m_nImageWidth * m_nImageHeight];
		memset(pData, 0, m_nImageWidth * m_nImageHeight);
		m_vecByteImage.push_back(pData);
	}

	pData = m_vecByteImage[m_nUseByteImage];

	m_nUseByteImage++;

	return pData;
}

long* CMemoryManager::GetNextLongImage()
{
	long* pData;
	if(m_nUseLongImage >= (long)m_vecLongImage.size())
	{
		pData = new long[m_nImageWidth * m_nImageHeight];
		memset(pData, 0, m_nImageWidth * m_nImageHeight * sizeof(long));
		m_vecLongImage.push_back(pData);
	}

	pData = m_vecLongImage[m_nUseLongImage];

	m_nUseLongImage++;

	return pData;
}

float* CMemoryManager::GetNextFloatImage()
{
	float* pData;
	if(m_nUseFloatImage >= (long)m_vecFloatImage.size())
	{
		pData = new float[m_nImageWidth * m_nImageHeight];
		memset(pData, 0, m_nImageWidth * m_nImageHeight * sizeof(float));
		m_vecFloatImage.push_back(pData);
	}

	pData = m_vecFloatImage[m_nUseFloatImage];

	m_nUseFloatImage++;

	return pData;
}

BYTE* CMemoryManager::GetByteImage(long nOrder)
{
	if(nOrder >= (long)m_vecByteImage.size())
	{
		SetReserveByteImage(nOrder + 1);
	}

	m_nUseByteImage = nOrder + 1;

	return m_vecByteImage[nOrder];
}

long* CMemoryManager::GetLongImage(long nOrder)
{
	if(nOrder >= (long)m_vecLongImage.size())
	{
		SetReserveLongImage(nOrder + 1);
	}

	m_nUseLongImage = nOrder + 1;

	return m_vecLongImage[nOrder];
}

float* CMemoryManager::GetFloatImage(long nOrder)
{
	if(nOrder >= (long)m_vecFloatImage.size())
	{
		SetReserveFloatImage(nOrder + 1);
	}

	m_nUseFloatImage = nOrder + 1;

	return m_vecFloatImage[nOrder];
}
