#if !defined(AFX_RESIZABLEMDICHILD_H__EA9AE112_0E99_4D6E_B42B_A3BA9DE3756E__INCLUDED_)
#define AFX_RESIZABLEMDICHILD_H__EA9AE112_0E99_4D6E_B42B_A3BA9DE3756E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResizableMDIChild.h : header file
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000-2002 by Paolo Messina
// (http://www.geocities.com/ppescher - ppescher@yahoo.com)
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

#include "ResizableMinMax.h"
#include "ResizableState.h"

/////////////////////////////////////////////////////////////////////////////
// CResizableMDIChild frame

class CResizableMDIChild : public CMDIChildWnd, public CResizableMinMax,
						public CResizableState
{
	DECLARE_DYNCREATE(CResizableMDIChild)
protected:
	CResizableMDIChild();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizableMDIChild)
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CResizableMDIChild();

	BOOL EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly = FALSE);

	virtual CWnd* GetResizableWnd()
	{
		// make the layout know its parent window
		return this;
	};

private:
	// flags
	BOOL m_bEnableSaveRestore;
	BOOL m_bRectOnly;

	// internal status
	CString m_sSection;			// section name (identifies a parent window)

protected:
	// Generated message map functions
	//{{AFX_MSG(CResizableMDIChild)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESIZABLEMDICHILD_H__EA9AE112_0E99_4D6E_B42B_A3BA9DE3756E__INCLUDED_)
