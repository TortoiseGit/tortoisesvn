// ResizableWndState.h: interface for the CResizableWndState class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESIZABLEWNDSTATE_H__23E0B3B0_2B95_47AF_A73D_CA629855B40B__INCLUDED_)
#define AFX_RESIZABLEWNDSTATE_H__23E0B3B0_2B95_47AF_A73D_CA629855B40B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizableState.h"

class CResizableWndState : public CResizableState  
{
protected:
	// non-zero if successful
	BOOL LoadWindowRect(LPCTSTR pszName, BOOL bRectOnly);
	BOOL SaveWindowRect(LPCTSTR pszName, BOOL bRectOnly);

	virtual CWnd* GetResizableWnd() const = 0;

public:
	CResizableWndState();
	virtual ~CResizableWndState();
};

#endif // !defined(AFX_RESIZABLEWNDSTATE_H__23E0B3B0_2B95_47AF_A73D_CA629855B40B__INCLUDED_)
