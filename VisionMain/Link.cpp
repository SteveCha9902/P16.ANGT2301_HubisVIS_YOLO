#include "pch.h"
#include "Link.h"
#include "DlgVisionMain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLink::CLink()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

CLink::~CLink()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

INT_PTR CLink::DoModal()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDlgVisionMain dlg;

	return dlg.DoModal();
}
