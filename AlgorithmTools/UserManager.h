#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CUserManager
{
public:
	CUserManager();
	~CUserManager();

	CRegistry* m_pRegistry;

	CString m_strUser;
	int m_nPreLoginUserIndex;
	CString m_strPreLoginUserID;

	std::vector<std::tuple<int, CString, CString>> m_vecUser;

	void InitParameters();
	BOOL LoadUserData();
	BOOL SaveUserData();
};

extern ALGORITHM_TOOLS_API CUserManager g_UserManager;
