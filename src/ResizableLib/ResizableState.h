// ResizableState.h: interface for the CResizableState class.
//
/////////////////////////////////////////////////////////////////////////////
//
// This file is part of ResizableLib
// http://sourceforge.net/projects/resizablelib
//
// Copyright (C) 2000-2004 by Paolo Messina
// http://www.geocities.com/ppescher - mailto:ppescher@hotmail.com
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESIZABLESTATE_H__INCLUDED_)
#define AFX_RESIZABLESTATE_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CResizableState  
{
	static CString m_sDefaultStorePath;
	CString m_sStorePath;

protected:
	static LPCTSTR GetDefaultStateStore();
	static void SetDefaultStateStore(LPCTSTR szPath);

	LPCTSTR GetStateStore();
	void SetStateStore(LPCTSTR szPath);

	// overridable to customize state persistance method
	virtual BOOL ReadState(LPCTSTR szId, CString& rsState);
	virtual BOOL WriteState(LPCTSTR szId, LPCTSTR szState);

public:
	CResizableState();
	virtual ~CResizableState();
};

#endif // !defined(AFX_RESIZABLESTATE_H__INCLUDED_)
