// ResizableLayout.h: interface for the CResizableLayout class.
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

#if !defined(AFX_RESIZABLELAYOUT_H__INCLUDED_)
#define AFX_RESIZABLELAYOUT_H__INCLUDED_

#include <afxtempl.h>
#include "ResizableMsgSupport.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// special type for layout anchors
typedef struct tagANCHOR
{
	int cx;
	int cy;

	tagANCHOR() {}

	tagANCHOR(int x, int y)
	{
		cx = x;
		cy = y;
	}

} ANCHOR, *PANCHOR, *LPANCHOR;

// define common constants
const ANCHOR
	TOP_LEFT(0,0), TOP_CENTER(50,0), TOP_RIGHT(100,0),
	MIDDLE_LEFT(0,50), MIDDLE_CENTER(50,50), MIDDLE_RIGHT(100,50),
	BOTTOM_LEFT(0,100), BOTTOM_CENTER(50,100), BOTTOM_RIGHT(100,100);

// holds a window layout settings
typedef struct tagLAYOUTINFO
{
	HWND hWnd;
	UINT nCallbackID;

	TCHAR sWndClass[MAX_PATH];

	// upper-left corner
	ANCHOR anchorTypeTL;
	SIZE sizeMarginTL;
	
	// bottom-right corner
	ANCHOR anchorTypeBR;
	SIZE sizeMarginBR;

	// custom window support
	BOOL bMsgSupport;
	RESIZEPROPERTIES properties;

	tagLAYOUTINFO() : hWnd(NULL), nCallbackID(0), bMsgSupport(FALSE)
	{
		sWndClass[0] = 0;
	}

	tagLAYOUTINFO(HWND hwnd, ANCHOR tl_type, SIZE tl_margin, 
		ANCHOR br_type, SIZE br_margin)
		:
		hWnd(hwnd), nCallbackID(0), bMsgSupport(FALSE),
		anchorTypeTL(tl_type), sizeMarginTL(tl_margin),
		anchorTypeBR(br_type), sizeMarginBR(br_margin)
	{
		sWndClass[0] = 0;
	}

} LAYOUTINFO, *PLAYOUTINFO, *LPLAYOUTINFO;

// layout manager implementation
class CResizableLayout
{
private:
	// list of repositionable controls
	CMap<HWND, HWND, POSITION, POSITION> m_mapLayout;
	CList<LAYOUTINFO, LAYOUTINFO&> m_listLayout;
	CList<LAYOUTINFO, LAYOUTINFO&> m_listLayoutCB;

	// used for clipping
	HRGN m_hOldClipRgn;
	int m_nOldClipRgn;

	// used for anti-flickering
	RECT m_rectClientBefore;
	BOOL m_bNoRecursion;

	void ClipChildWindow(const LAYOUTINFO &layout, CRgn* pRegion) const;

	void CalcNewChildPosition(const LAYOUTINFO &layout,
		const CRect &rectParent, CRect &rectChild, UINT& uFlags) const;

protected:
	// override to initialize resize properties (clipping, refresh)
	virtual void InitResizeProperties(LAYOUTINFO& layout) const;

	// override to specify clipping for unsupported windows
	virtual BOOL LikesClipping(const LAYOUTINFO &layout) const;

	// override to specify refresh for unsupported windows
	virtual BOOL NeedsRefresh(const LAYOUTINFO &layout,
		const CRect &rectOld, const CRect &rectNew) const;

	// clip out child windows from the given DC (returns if clipping done)
	BOOL ClipChildren(CDC* pDC, BOOL bUndo);

	// get the clipping region (without clipped child windows)
	void GetClippingRegion(CRgn* pRegion) const;
	
	// override for scrollable or expanding parent windows
	virtual void GetTotalClientRect(LPRECT lpRect) const;

	// add anchors to a control, given its HWND
	void AddAnchor(HWND hWnd, ANCHOR anchorTypeTL, ANCHOR anchorTypeBR);

	// add anchors to a control, given its HWND
	void AddAnchor(HWND hWnd, ANCHOR anchorTypeTL)
	{
		AddAnchor(hWnd, anchorTypeTL, anchorTypeTL);
	}

	// add anchors to a control, given its ID
	void AddAnchor(UINT nID, ANCHOR anchorTypeTL, ANCHOR anchorTypeBR)
	{
		AddAnchor(::GetDlgItem(GetResizableWnd()->GetSafeHwnd(), nID),
			anchorTypeTL, anchorTypeBR);
	}

	// add anchors to a control, given its ID
	void AddAnchor(UINT nID, ANCHOR anchorTypeTL)
	{
		AddAnchor(::GetDlgItem(GetResizableWnd()->GetSafeHwnd(), nID),
			anchorTypeTL, anchorTypeTL);
	}

	// add a callback (control ID or HWND is unknown or may change)
	UINT AddAnchorCallback();

	// get rect of an anchored window, given the parent's client area
	BOOL GetAnchorPosition(HWND hWnd, const CRect &rectParent,
		CRect &rectChild, UINT* lpFlags = NULL) const
	{
		POSITION pos;
		if (!m_mapLayout.Lookup(hWnd, pos))
			return FALSE;

		UINT uTmpFlags;
		CalcNewChildPosition(m_listLayout.GetAt(pos), rectParent, rectChild,
			(lpFlags != NULL) ? (*lpFlags) : uTmpFlags);
		return TRUE;
	}

	// get rect of an anchored window, given the parent's client area
	BOOL GetAnchorPosition(UINT nID, const CRect &rectParent,
		CRect &rectChild, UINT* lpFlags = NULL) const
	{
		return GetAnchorPosition(::GetDlgItem(GetResizableWnd()->GetSafeHwnd(), nID),
			rectParent, rectChild, lpFlags);
	}

	// get margins surrounding a child window at the given size
	BOOL GetAnchorMargins(HWND hWnd, const CSize &sizeChild, CRect &rectMargins) const;

	// get margins surrounding a child window at the given size
	BOOL GetAnchorMargins(UINT nID, const CSize &sizeChild, CRect &rectMargins) const
	{
		return GetAnchorMargins(::GetDlgItem(GetResizableWnd()->GetSafeHwnd(), nID),
			sizeChild, rectMargins);
	}

	// remove an anchored control from the layout, given its HWND
	BOOL RemoveAnchor(HWND hWnd)
	{
		POSITION pos;
		if (!m_mapLayout.Lookup(hWnd, pos))
			return FALSE;

		m_listLayout.RemoveAt(pos);
		return m_mapLayout.RemoveKey(hWnd);
	}

	// remove an anchored control from the layout, given its HWND
	BOOL RemoveAnchor(UINT nID)
	{
		return RemoveAnchor(::GetDlgItem(GetResizableWnd()->GetSafeHwnd(), nID));
	}

	// reset layout content
	void RemoveAllAnchors()
	{
		m_mapLayout.RemoveAll();
		m_listLayout.RemoveAll();
		m_listLayoutCB.RemoveAll();
	}

	// adjust children's layout, when parent's size changes
	void ArrangeLayout() const;

	// override to provide dynamic control's layout info
	virtual BOOL ArrangeLayoutCallback(LAYOUTINFO& layout) const;

	// override to provide the parent window
	virtual CWnd* GetResizableWnd() const = 0;

	// enhance anti-flickering
	void HandleNcCalcSize(BOOL bAfterDefault, LPNCCALCSIZE_PARAMS lpncsp, LRESULT& lResult);
	
	// enable resizable style for top level windows
	void MakeResizable(LPCREATESTRUCT lpCreateStruct);

public:
	CResizableLayout()
	{
		m_bNoRecursion = FALSE;
		m_hOldClipRgn = ::CreateRectRgn(0,0,0,0);
	}

	virtual ~CResizableLayout()
	{
		// just for safety
		RemoveAllAnchors();
	}
};

#endif // !defined(AFX_RESIZABLELAYOUT_H__INCLUDED_)
