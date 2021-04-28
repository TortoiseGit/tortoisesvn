﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2008, 2020-2021 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

/**
 * \ingroup Utils
 * Hyperlink control
 */
class CHyperLink : public CStatic
{
public:
    CHyperLink();
    ~CHyperLink() override;

public:
    enum UnderLineOptions
    {
        UlHover  = -1,
        UlNone   = 0,
        UlAlways = 1
    };

public:
    void    SetURL(CString strURL);
    CString GetURL() const;

    void     SetColors(COLORREF crLinkColor, COLORREF crHoverColor = -1);
    COLORREF GetLinkColor() const;
    COLORREF GetHoverColor() const;

    void SetUnderline(int nUnderline = UlHover);
    int  GetUnderline() const;

public:
    BOOL PreTranslateMessage(MSG* pMsg) override;
    BOOL DestroyWindow() override;

protected:
    void PreSubclassWindow() override;

protected:
    static HINSTANCE GotoURL(LPCTSTR url);
    void             SetDefaultCursor();

protected:
    COLORREF     m_crLinkColor;   ///< Hyperlink color
    COLORREF     m_crHoverColor;  ///< Hover color
    BOOL         m_bOverControl;  ///< cursor over control?
    int          m_nUnderline;    ///< underline hyperlink?
    CString      m_strURL;        ///< hyperlink URL
    CFont        m_underlineFont; ///< Font for underline display
    CFont        m_stdFont;       ///< Standard font
    HCURSOR      m_hLinkCursor;   ///< Cursor for hyperlink
    CToolTipCtrl m_toolTip;       ///< The tooltip
    UINT         m_nTimerID;

protected:
    afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
    afx_msg BOOL   OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void   OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void   OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL   OnEraseBkgnd(CDC* pDC);
    afx_msg void   OnClicked();
    afx_msg void   OnSysColorChange();
    DECLARE_MESSAGE_MAP()
};
