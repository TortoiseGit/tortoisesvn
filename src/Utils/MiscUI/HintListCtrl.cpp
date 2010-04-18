// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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

#include "stdafx.h"
#include "MyMemDC.h"
#include "HintListCtrl.h"

CHintListCtrl::CHintListCtrl() : CListCtrl()
    , pProvider(NULL)
{
}

CHintListCtrl::~CHintListCtrl()
{
}

void CHintListCtrl::ShowText(const CString& sText, bool forceupdate /* = false*/)
{
    m_sText = sText;
    Invalidate();
    if (forceupdate)
        UpdateWindow();
}

void CHintListCtrl::ClearText()
{
    m_sText.Empty();
    Invalidate();
}

BEGIN_MESSAGE_MAP(CHintListCtrl, CListCtrl)
    ON_WM_PAINT()
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, &CHintListCtrl::OnToolTipText)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, &CHintListCtrl::OnToolTipText)
END_MESSAGE_MAP()

void CHintListCtrl::OnPaint()
{
    Default();
    if (!m_sText.IsEmpty())
    {
        COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
        COLORREF clrTextBk;
        if (IsWindowEnabled())
            clrTextBk = ::GetSysColor(COLOR_WINDOW);
        else
            clrTextBk = ::GetSysColor(COLOR_3DFACE);

        CRect rc;
        GetClientRect(&rc);
        CHeaderCtrl* pHC;
        pHC = GetHeaderCtrl();
        if (pHC != NULL)
        {
            CRect rcH;
            rcH.SetRectEmpty();
            pHC->GetItemRect(0, &rcH);
            rc.top += rcH.bottom;
        }
        CDC* pDC = GetDC();
        {
            CMyMemDC memDC(pDC, &rc);

            memDC.SetTextColor(clrText);
            memDC.SetBkColor(clrTextBk);
            memDC.FillSolidRect(rc, clrTextBk);
            rc.top += 10;
            CGdiObject * oldfont = memDC.SelectStockObject(DEFAULT_GUI_FONT);
            memDC.DrawText(m_sText, rc, DT_CENTER | DT_VCENTER |
                DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);
            memDC.SelectObject(oldfont);
        }
        ReleaseDC(pDC);
    }
}

INT_PTR CHintListCtrl::OnToolHitTest(CPoint point, TOOLINFO * pTI) const 
{
    if (pProvider == NULL)
        return -1;

    LVHITTESTINFO lvhitTestInfo;

    lvhitTestInfo.pt    = point;

    int nItem = ListView_SubItemHitTest(
        this->m_hWnd,
        &lvhitTestInfo);
    int nSubItem = lvhitTestInfo.iSubItem;

    UINT nFlags =   lvhitTestInfo.flags;

    //nFlags is 0 if the SubItemHitTest fails
    //Therefore, 0 & <anything> will equal false
    if (nFlags & LVHT_ONITEM)
    {
        //If it did fall on a list item,
        //and it was also hit one of the
        //item specific sub-areas we wish to show tool tips for

        //Get the client (area occupied by this control
        RECT rcClient;
        GetClientRect( &rcClient );

        //Fill in the TOOLINFO structure
        pTI->hwnd = m_hWnd;
        pTI->uId = (UINT)((nItem<<10)+(nSubItem&0x3ff)+1);
        pTI->lpszText = LPSTR_TEXTCALLBACK;
        pTI->rect = rcClient;

        return pTI->uId; //By returning a unique value per listItem,
        //we ensure that when the mouse moves over another list item,
        //the tooltip will change
    }
    else
    {
        //Otherwise, we aren't interested, so let the message propagate
        return -1;
    }
}

BOOL CHintListCtrl::OnToolTipText(UINT /*id*/, NMHDR * pNMHDR, LRESULT * pResult)
{
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

    if (pProvider == NULL)
        return FALSE;

    // Ignore messages from the built in tooltip, we are processing them internally
    if( (pNMHDR->idFrom == (UINT)m_hWnd) &&
        ( ((pNMHDR->code == TTN_NEEDTEXTA) && (pTTTA->uFlags & TTF_IDISHWND)) ||
        ((pNMHDR->code == TTN_NEEDTEXTW) && (pTTTW->uFlags & TTF_IDISHWND)) ) )
    {
            return FALSE;
    }

    *pResult = 0;

    //Get the mouse position
    const MSG* pMessage;
    pMessage = GetCurrentMessage();

    CPoint pt;
    pt = pMessage->pt;
    ScreenToClient(&pt);

    // Check if the point falls onto a list item
    LVHITTESTINFO lvhitTestInfo;
    lvhitTestInfo.pt = pt;

    int nItem = SubItemHitTest(&lvhitTestInfo);

    if (lvhitTestInfo.flags & LVHT_ONITEM)
    {
        // we want multiline tooltips
        ::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, INT_MAX);
        CString strTipText = pProvider->GetToolTipText(nItem, lvhitTestInfo.iSubItem);

        //Deal with UNICODE
#ifndef _UNICODE
        if (pNMHDR->code == TTN_NEEDTEXTA)
            lstrcpyn(pTTTA->szText, strTipText, 80);
        else
            _mbstowcsz(pTTTW->szText, strTipText, 80);
#else
        if (pNMHDR->code == TTN_NEEDTEXTA)
            _wcstombsz(pTTTA->szText, strTipText, 80);
        else
            lstrcpyn(pTTTW->szText, strTipText, 80);
#endif
        return TRUE;    //We found a tool tip,
        //tell the framework this message has been handled
    }

    return FALSE; //We didn't handle the message,
    //let the framework continue propagating the message
}

