// ResizableSheetState.cpp: implementation of the CResizableSheetState class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResizableSheetState.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResizableSheetState::CResizableSheetState()
{

}

CResizableSheetState::~CResizableSheetState()
{

}

// used to save/restore active page
// either in the registry or a private .INI file
// depending on your application settings

#define ACTIVEPAGE_ENT 	_T("ActivePage")

BOOL CResizableSheetState::SavePage(LPCTSTR pszName)
{
	// saves active page index, or the initial page if problems
	// cannot use GetActivePage, because it always fails

	CPropertySheet* pSheet = DYNAMIC_DOWNCAST(CPropertySheet, GetResizableWnd());
	if (pSheet == NULL)
		return FALSE;

	int page = pSheet->m_psh.nStartPage;
	CTabCtrl *pTab = pSheet->GetTabControl();
	if (pTab != NULL) 
		page = pTab->GetCurSel();
	if (page < 0)
		page = pSheet->m_psh.nStartPage;

	CString data, id;
	_itot(page, data.GetBuffer(10), 10);
	id = CString(pszName) + ACTIVEPAGE_ENT;
	return WriteState(id, data);
}

BOOL CResizableSheetState::LoadPage(LPCTSTR pszName)
{
	// restore active page, zero (the first) if not found

	CString data, id;
	id = CString(pszName) + ACTIVEPAGE_ENT;
	if (!ReadState(id, data))
		return FALSE;
	
	int page = _ttoi(data);
	CPropertySheet* pSheet = DYNAMIC_DOWNCAST(CPropertySheet, GetResizableWnd());
	if (pSheet != NULL)
		return pSheet->SetActivePage(page);
	return FALSE;
}
