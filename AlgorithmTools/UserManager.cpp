#include "pch.h"
#include "UserManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CUserManager g_UserManager;

CUserManager::CUserManager()
	: m_strUser(_T("USER\\"))
{
	m_pRegistry = new CRegistry(_T("HUBIS"));
}

CUserManager::~CUserManager()
{
	delete m_pRegistry;
}

void CUserManager::InitParameters()
{
	// User Registry 구조
	//  =Key= |  =Key= | =Key= |  =Value=
	//  HUBIS |- SETUP |		- PreLoginID
	//        |- USER  |-  ID  |- Password
	//						   |- Authority

	m_nPreLoginUserIndex = 0;
	m_strPreLoginUserID = _T("User"); // HUBIS -> User 로 변경 PSJ
	m_vecUser.clear();
}

BOOL CUserManager::LoadUserData()
{
	if(m_pRegistry->ReadString(_T("Setup"), _T("Pre Login User"), m_strPreLoginUserID) == FALSE)
		return FALSE;

	int nItemNum(0), nValueNum(0);
	m_pRegistry->GetNum(m_strUser, nItemNum, nValueNum);
	m_vecUser.resize(nItemNum);

	CString* strKey = nullptr;
	strKey = new CString[nItemNum];

	m_pRegistry->ReadAllKey(m_strUser, strKey, nItemNum);

	for (long idx = 0; idx < nItemNum; idx++)
	{
		std::get<1>(m_vecUser[idx]) = strKey[idx];
		CString strTemp = m_strUser + std::get<1>(m_vecUser[idx]);

		if(m_pRegistry->ReadInt(strTemp, _T("Authority"), std::get<0>(m_vecUser[idx])) == FALSE)
			break;

		if(m_pRegistry->ReadString(strTemp, _T("Password"), std::get<2>(m_vecUser[idx])) == FALSE)
			break;

		if (m_strPreLoginUserID == std::get<1>(m_vecUser[idx]))
			m_nPreLoginUserIndex = idx;
	}

	delete[] strKey;

	return TRUE;
}

BOOL CUserManager::SaveUserData()
{
	CString strTemp;
	m_pRegistry->WriteString(_T("SETUP"), _T("Pre Login User"), m_strPreLoginUserID);

	if (m_vecUser.size() > 0)
	{
		for (long idx = 0; idx < m_vecUser.size(); idx++)
		{
			strTemp = m_strUser + std::get<1>(m_vecUser[idx]);
			m_pRegistry->WriteInt(strTemp, _T("Authority"), std::get<0>(m_vecUser[idx]));
			m_pRegistry->WriteString(strTemp, _T("Password"), std::get<2>(m_vecUser[idx]));
		}
	}
	else
	{
		m_vecUser.push_back(std::make_tuple(2, _T("User"), _T("1234"))); // (0, "HUBIS", "1234") -> (2, "User", "")로 변경
		strTemp = m_strUser + m_strPreLoginUserID;
		m_pRegistry->WriteInt(strTemp, _T("Authority"), 2);
		m_pRegistry->WriteString(strTemp, _T("Password"), _T("1234")); // default password 1234가 안맞으면 안열리고, 창 나갈때 Crash dump도 발생함.

		/*m_vecUser.push_back(std::make_tuple(0, _T("Admin"), _T("1")));
		strTemp = m_strUser + m_strPreLoginUserID;
		m_pRegistry->WriteInt(strTemp, _T("Authority"), 2);
		m_pRegistry->WriteString(strTemp, _T("Password"), _T("1"));*/
	}

	return TRUE;
}
