// ResizableSheetState.h: interface for the CResizableSheetState class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESIZABLESHEETSTATE_H__1F7B9232_4DA6_4CB7_9649_620A2CBB87DA__INCLUDED_)
#define AFX_RESIZABLESHEETSTATE_H__1F7B9232_4DA6_4CB7_9649_620A2CBB87DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizableWndState.h"

class CResizableSheetState : public CResizableWndState  
{
protected:
	// non-zero if successful
	BOOL LoadPage(LPCTSTR pszName);
	BOOL SavePage(LPCTSTR pszName);

	virtual CWnd* GetResizableWnd() const = 0;

public:
	CResizableSheetState();
	virtual ~CResizableSheetState();
};

#endif // !defined(AFX_RESIZABLESHEETSTATE_H__1F7B9232_4DA6_4CB7_9649_620A2CBB87DA__INCLUDED_)
