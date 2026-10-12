#pragma once

#ifdef ALGORITHM_TOOLS_EXPORT
#define ALGORITHM_TOOLS_API	__declspec(dllexport)
#else
#define ALGORITHM_TOOLS_API	__declspec(dllimport)
#endif

class ALGORITHM_TOOLS_API CRecipeManager
{
public:
	CRecipeManager();
	~CRecipeManager();
	CRITICAL_SECTION m_csRecipe;

	long m_nInspItemNum;
	SBusbarInfoParam m_sBusbarInfoParam;
	std::vector<SInspItem> m_vecsInspItem;
	long m_nInspItemMatchNum;
	std::vector<SMatchItem> m_vecInspItemMatch;
	SIllumParams m_sIllum;
	void CalcMatchSize();

	void InitParameters();
	BOOL LoadRecipeParameters(const CString strRecipePathName);
	BOOL SaveRecipeParameters(const CString strRecipePathName);
	CString m_currentRecipeName;
};

extern ALGORITHM_TOOLS_API CRecipeManager g_RecipeManager;
