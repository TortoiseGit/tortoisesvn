// ResizableWndState.cpp: implementation of the CResizableWndState class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResizableWndState.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResizableWndState::CResizableWndState()
{

}

CResizableWndState::~CResizableWndState()
{

}

// used to save/restore window's size and position
// either in the registry or a private .INI file
// depending on your application settings

#define PLACEMENT_ENT	_T("WindowPlacement")
#define PLACEMENT_FMT 	_T("%d,%d,%d,%d,%d,%d,%d,%d")

BOOL CResizableWndState::SaveWindowRect(LPCTSTR pszName, BOOL bRectOnly)
{
	CString data, id;
	WINDOWPLACEMENT wp;

	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);
	if (!GetResizableWnd()->GetWindowPlacement(&wp))
		return FALSE;
	
	// use workspace coordinates
	RECT& rc = wp.rcNormalPosition;

	if (bRectOnly)	// save size/pos only (normal state)
	{
		data.Format(PLACEMENT_FMT, rc.left, rc.top,
			rc.right, rc.bottom, SW_SHOWNORMAL, 0, 0, 0);
	}
	else	// save also min/max state
	{
		data.Format(PLACEMENT_FMT, rc.left, rc.top,
			rc.right, rc.bottom, wp.showCmd, wp.flags,
			wp.ptMinPosition.x, wp.ptMinPosition.y);
	}

	id = CString(pszName) + PLACEMENT_ENT;
	return WriteState(id, data);
}

BOOL CResizableWndState::LoadWindowRect(LPCTSTR pszName, BOOL bRectOnly)
{
	CString data, id;
	WINDOWPLACEMENT wp;

	id = CString(pszName) + PLACEMENT_ENT;
	if (!ReadState(id, data))	// never saved before
		return FALSE;
	
	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);
	if (!GetResizableWnd()->GetWindowPlacement(&wp))
		return FALSE;

	// use workspace coordinates
	RECT& rc = wp.rcNormalPosition;

	if (_stscanf(data, PLACEMENT_FMT, &rc.left, &rc.top,
		&rc.right, &rc.bottom, &wp.showCmd, &wp.flags,
		&wp.ptMinPosition.x, &wp.ptMinPosition.y) == 8)
	{
		if (bRectOnly)	// restore size/pos only
		{
			wp.showCmd = SW_SHOWNORMAL;
			wp.flags = 0;
			return GetResizableWnd()->SetWindowPlacement(&wp);
		}
		else	// restore also min/max state
		{
			return GetResizableWnd()->SetWindowPlacement(&wp);
		}
	}
	return FALSE;
}
