#if !defined(AFX_RESIZABLEMDIFRAME_H__4BA07057_6EF3_43A4_A80B_A24FA3A8B5C7__INCLUDED_)
#define AFX_RESIZABLEMDIFRAME_H__4BA07057_6EF3_43A4_A80B_A24FA3A8B5C7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResizableMDIFrame.h : header file
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
// CResizableMDIFrame frame

class CResizableMDIFrame : public CMDIFrameWnd, public CResizableMinMax,
						public CResizableState
{
	DECLARE_DYNCREATE(CResizableMDIFrame)
protected:
	CResizableMDIFrame();		// protected constructor used by dynamic creation

// Attributes
protected:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizableMDIFrame)
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CResizableMDIFrame();

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
	//{{AFX_MSG(CResizableMDIFrame)
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESIZABLEMDIFRAME_H__4BA07057_6EF3_43A4_A80B_A24FA3A8B5C7__INCLUDED_)
