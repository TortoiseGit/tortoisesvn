// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once

class CHyperLink : public CStatic
{
public:
    CHyperLink();
    virtual ~CHyperLink();

public:
    enum UnderLineOptions 
	{ 
		ulHover = -1, 
		ulNone = 0, 
		ulAlways = 1
	};

public:
    void		SetURL(CString strURL);
    CString		GetURL();

    void		SetColors(COLORREF crLinkColor, COLORREF crHoverColor = -1);
    COLORREF	GetLinkColor();
    COLORREF	GetHoverColor();

    void		SetUnderline(int nUnderline = ulHover);
    int			GetUnderline();

public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
protected:
    virtual void PreSubclassWindow();

protected:
    HINSTANCE	GotoURL(LPCTSTR url);
    void		SetDefaultCursor();

protected:
    COLORREF	m_crLinkColor;			///< Hyperlink color
    COLORREF	m_crHoverColor;			///< Hover color
    BOOL		m_bOverControl;			///< cursor over control?
    int			m_nUnderline;			///< underline hyperlink?
    CString		m_strURL;				///< hyperlink URL
    CFont		m_UnderlineFont;		///< Font for underline display
    CFont		m_StdFont;				///< Standard font
    HCURSOR		m_hLinkCursor;			///< Cursor for hyperlink
    CToolTipCtrl m_ToolTip;				///< The tooltip
    UINT		m_nTimerID;

protected:
    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnClicked();
    DECLARE_MESSAGE_MAP()
};
