// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010 - TortoiseSVN

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
#include "Tooltip.h"

/**
 * \ingroup Utils
 * Implements an edit control used for showing paths. If the path
 * gets longer than the edit control, the path is shown compacted.
 * But copying the path text to the clipboard always copies the
 * full (not compacted) path string.
 */
class CPathEdit : public CEdit
{
    DECLARE_DYNAMIC(CPathEdit)

public:
    CPathEdit();
    virtual ~CPathEdit();
    void    SetBold();

protected:
    DECLARE_MESSAGE_MAP()
public:
    virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
    CString     m_sRealText;
    bool        m_bInternalCall;
    bool        m_bBold;
    CFont       m_boldFont;
    void        FitPathToWidth(CString& path);
    CFont *     GetFont();
    CToolTips   m_tooltips;
};
