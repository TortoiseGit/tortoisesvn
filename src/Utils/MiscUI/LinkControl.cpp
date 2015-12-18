// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009, 2012-2015 - TortoiseSVN

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
#include "LinkControl.h"
#include "CommonAppUtils.h"

const UINT CLinkControl::LK_LINKITEMCLICKED
= ::RegisterWindowMessage(L"LK_LINKITEMCLICKED");

CLinkControl::CLinkControl(void)
    : m_bOverControl(false)
{
}

CLinkControl::~CLinkControl(void)
{
    /*
    * No need to call DestroyCursor() for cursors acquired through
    * LoadCursor().
    */
    m_NormalFont.DeleteObject();
    m_UnderlineFont.DeleteObject();
}

void CLinkControl::PreSubclassWindow()
{
    CStatic::PreSubclassWindow();

    ModifyStyle(0, SS_NOTIFY);

    m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND); // Load Windows' hand cursor
    if (!m_hLinkCursor)    // if not available, use the standard Arrow cursor
    {
        m_hLinkCursor = ::LoadCursor(NULL, IDC_ARROW);
    }

    // Create an updated font by adding an underline.
    CFont* pFont = GetFont();
    if (!pFont)
    {
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (hFont == NULL)
            hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
        if (hFont)
            pFont = CFont::FromHandle(hFont);
    }
    ASSERT(pFont->GetSafeHandle());

    LOGFONT lf;
    pFont->GetObject(sizeof(lf), &lf);
    lf.lfWeight = FW_BOLD;
    m_NormalFont.CreateFontIndirect(&lf);

    lf.lfUnderline = TRUE;
    m_UnderlineFont.CreateFontIndirect(&lf);

    SetFont(&m_NormalFont, FALSE);

    CCommonAppUtils::SetAccProperty(GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    UpdateAccState();
}

BEGIN_MESSAGE_MAP(CLinkControl, CStatic)
    ON_WM_SETCURSOR()
    ON_WM_MOUSEMOVE()
    ON_WM_CAPTURECHANGED()
    ON_WM_ERASEBKGND()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_GETDLGCODE()
    ON_WM_KEYDOWN()
    ON_CONTROL_REFLECT(STN_CLICKED, OnClicked)
    ON_WM_ENABLE()
END_MESSAGE_MAP()

void CLinkControl::OnMouseMove(UINT /*nFlags*/, CPoint pt)
{
    if (m_bOverControl)
    {
        RECT rect;
        GetClientRect(&rect);

        if (!PtInRect(&rect, pt))
            ReleaseCapture();
    }
    else
    {
        m_bOverControl = TRUE;
        SetFont(&m_UnderlineFont, FALSE);
        InvalidateRect(NULL, FALSE);
        SetCapture();
    }
}

BOOL CLinkControl::OnSetCursor(CWnd* /*pWnd*/, UINT /*nHitTest*/, UINT /*message*/)
{
    ::SetCursor(m_hLinkCursor);
    return TRUE;
}

void CLinkControl::OnCaptureChanged(CWnd * /*pWnd*/)
{
    m_bOverControl = FALSE;
    SetFont(&m_NormalFont, FALSE);
    InvalidateRect(NULL, FALSE);
}

void CLinkControl::OnSetFocus(CWnd* pOldWnd)
{
    DrawFocusRect();
    __super::OnSetFocus(pOldWnd);
}

void CLinkControl::OnKillFocus(CWnd* pOldWnd)
{
    DrawFocusRect();
    __super::OnKillFocus(pOldWnd);
}

void CLinkControl::DrawFocusRect()
{
    HWND hwndParent = GetParent()->GetSafeHwnd();

    if (hwndParent)
    {
        // calculate where to draw focus rectangle, in screen coords
        RECT rc;
        GetWindowRect(&rc);

        InflateRect(&rc, 1, 1);                  // add one pixel all around
        ::ScreenToClient(hwndParent, (LPPOINT)&rc);
        ::ScreenToClient(hwndParent, ((LPPOINT)&rc) + 1);
        HDC dcParent = ::GetDC(hwndParent);
        ::DrawFocusRect(dcParent, &rc);
        ::ReleaseDC(hwndParent, dcParent);
    }
}

void CLinkControl::UpdateAccState()
{
    DWORD state = STATE_SYSTEM_READONLY;
    if (!IsWindowEnabled())
        state |= STATE_SYSTEM_UNAVAILABLE;

    CCommonAppUtils::SetAccProperty(GetSafeHwnd(), PROPID_ACC_STATE, state);
}

UINT CLinkControl::OnGetDlgCode()
{
    UINT dlgCode = CStatic::OnGetDlgCode();
    const MSG *pMsg = CWnd::GetCurrentMessage();

    // we want all keys to get the return key
    dlgCode |= DLGC_WANTALLKEYS;
    dlgCode |= DLGC_BUTTON;
    // but we don't want the tab key since that should be used in dialogs
    // to switch the focus
    dlgCode &= ~DLGC_WANTTAB;
    dlgCode &= ~DLGC_STATIC;

    if (pMsg->lParam &&
        ((MSG *)pMsg->lParam)->message == WM_KEYDOWN &&
        ((MSG *)pMsg->lParam)->wParam == VK_TAB)
    {
        dlgCode &= ~DLGC_WANTMESSAGE;
    }

    return dlgCode;
}

void CLinkControl::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
    if (nChar == VK_SPACE || nChar == VK_RETURN)
    {
        ::PostMessage(GetParent()->GetSafeHwnd(), LK_LINKITEMCLICKED,
                      (WPARAM)GetSafeHwnd(), (LPARAM)0);
    }
}

void CLinkControl::OnClicked()
{
    ::PostMessage(GetParent()->GetSafeHwnd(), LK_LINKITEMCLICKED,
                  (WPARAM)GetSafeHwnd(), (LPARAM)0);
}

void CLinkControl::OnEnable(BOOL enabled)
{
    CStatic::OnEnable(enabled);

    UpdateAccState();
}

BOOL CLinkControl::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (message)
    {
    case BM_CLICK:
        ::PostMessage(GetParent()->GetSafeHwnd(), LK_LINKITEMCLICKED,
                      (WPARAM)GetSafeHwnd(), (LPARAM)0);
        break;
    }

    return CStatic::OnWndMsg(message, wParam, lParam, pResult);
}
