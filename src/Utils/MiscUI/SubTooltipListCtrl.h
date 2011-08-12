// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2011 - TortoiseSVN

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

class IListCtrlTooltipProvider
{
public:
    virtual CString GetToolTipText(int nItem, int nSubItem) = 0;
};

/**
 * \ingroup Utils
 * Extends the list control to provide sub-item tooltips.
 */
class CSubTooltipListCtrl : public CListCtrl
{
public:
    CSubTooltipListCtrl();
    ~CSubTooltipListCtrl();

    void SetTooltipProvider(IListCtrlTooltipProvider * provider) {pProvider = provider;}

    DECLARE_DYNAMIC(CSubTooltipListCtrl)

protected:
    DECLARE_MESSAGE_MAP()
    virtual afx_msg BOOL OnToolTipText(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
    virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO * pTI) const;

private:
    IListCtrlTooltipProvider * pProvider;
};
