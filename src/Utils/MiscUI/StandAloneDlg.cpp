// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2014-2015 - TortoiseSVN

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
#include "resource.h"
#include "StandAloneDlg.h"


const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

BEGIN_TEMPLATE_MESSAGE_MAP(CStandAloneDialogTmpl, BaseType, BaseType)
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_NCHITTEST()
    ON_WM_DWMCOMPOSITIONCHANGED()
    ON_REGISTERED_MESSAGE( TaskBarButtonCreated, OnTaskbarButtonCreated )
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CStandAloneDialog, CStandAloneDialogTmpl<CDialog>)
CStandAloneDialog::CStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
: CStandAloneDialogTmpl<CDialog>(nIDTemplate, pParentWnd)
{
}
BEGIN_MESSAGE_MAP(CStandAloneDialog, CStandAloneDialogTmpl<CDialog>)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CStateStandAloneDialog, CStandAloneDialogTmpl<CStateDialog>)
CStateStandAloneDialog::CStateStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
: CStandAloneDialogTmpl<CStateDialog>(nIDTemplate, pParentWnd)
{
}
BEGIN_MESSAGE_MAP(CStateStandAloneDialog, CStandAloneDialogTmpl<CStateDialog>)
END_MESSAGE_MAP()


IMPLEMENT_DYNAMIC(CResizableStandAloneDialog, CStandAloneDialogTmpl<CResizableDialog>)
CResizableStandAloneDialog::CResizableStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
    : CStandAloneDialogTmpl<CResizableDialog>(nIDTemplate, pParentWnd)
    , m_bVertical(false)
    , m_bHorizontal(false)
    , m_stickySize(CRegDWORD(L"Software\\TortoiseSVN\\DlgStickySize", 3))
{
}

BEGIN_MESSAGE_MAP(CResizableStandAloneDialog, CStandAloneDialogTmpl<CResizableDialog>)
    ON_WM_SIZING()
    ON_WM_MOVING()
    ON_WM_NCMBUTTONUP()
    ON_WM_NCRBUTTONUP()
END_MESSAGE_MAP()

void CResizableStandAloneDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
    m_bVertical = m_bVertical && (fwSide == WMSZ_LEFT || fwSide == WMSZ_RIGHT);
    m_bHorizontal = m_bHorizontal && (fwSide == WMSZ_TOP || fwSide == WMSZ_BOTTOM);
    if (m_nResizeBlock & DIALOG_BLOCKVERTICAL)
    {
        // don't allow the dialog to be changed in height
        switch (fwSide)
        {
            case WMSZ_BOTTOM:
            case WMSZ_BOTTOMLEFT:
            case WMSZ_BOTTOMRIGHT:
                pRect->bottom = pRect->top + m_height;
                break;
            case WMSZ_TOP:
            case WMSZ_TOPLEFT:
            case WMSZ_TOPRIGHT:
                pRect->top = pRect->bottom - m_height;
                break;
        }
    }
    if (m_nResizeBlock & DIALOG_BLOCKHORIZONTAL)
    {
        // don't allow the dialog to be changed in width
        switch (fwSide)
        {
            case WMSZ_RIGHT:
            case WMSZ_TOPRIGHT:
            case WMSZ_BOTTOMRIGHT:
                pRect->right = pRect->left + m_width;
                break;
            case WMSZ_LEFT:
            case WMSZ_TOPLEFT:
            case WMSZ_BOTTOMLEFT:
                pRect->left = pRect->right - m_width;
                break;
        }
    }
    CStandAloneDialogTmpl<CResizableDialog>::OnSizing(fwSide, pRect);
}

void CResizableStandAloneDialog::OnMoving(UINT fwSide, LPRECT pRect)
{
    m_bVertical = m_bHorizontal = false;
    if (pRect)
    {
        HMONITOR hMonitor = MonitorFromRect(pRect, MONITOR_DEFAULTTONEAREST);
        if (hMonitor)
        {
            MONITORINFO minfo = { 0 };
            minfo.cbSize = sizeof(minfo);
            if (GetMonitorInfo(hMonitor, &minfo))
            {
                int width = pRect->right - pRect->left;
                int heigth = pRect->bottom - pRect->top;
                if (abs(pRect->left - minfo.rcWork.left) < m_stickySize)
                {
                    pRect->left = minfo.rcWork.left;
                    pRect->right = pRect->left + width;
                }
                if (abs(pRect->right - minfo.rcWork.right) < m_stickySize)
                {
                    pRect->right = minfo.rcWork.right;
                    pRect->left = pRect->right - width;
                }
                if (abs(pRect->top - minfo.rcWork.top) < m_stickySize)
                {
                    pRect->top = minfo.rcWork.top;
                    pRect->bottom = pRect->top + heigth;
                }
                if (abs(pRect->bottom - minfo.rcWork.bottom) < m_stickySize)
                {
                    pRect->bottom = minfo.rcWork.bottom;
                    pRect->top = pRect->bottom - heigth;
                }
            }
        }
    }

    CStandAloneDialogTmpl<CResizableDialog>::OnMoving(fwSide, pRect);
}

void CResizableStandAloneDialog::OnNcMButtonUp(UINT nHitTest, CPoint point)
{
    WINDOWPLACEMENT windowPlacement;
    if ((nHitTest == HTMAXBUTTON) && GetWindowPlacement(&windowPlacement) && windowPlacement.showCmd == SW_SHOWNORMAL)
    {
        CRect rcWindowRect;
        GetWindowRect(&rcWindowRect);

        MONITORINFO mi = {0};
        mi.cbSize = sizeof(MONITORINFO);

        if (m_bVertical)
        {
            rcWindowRect.top = m_rcOrgWindowRect.top;
            rcWindowRect.bottom = m_rcOrgWindowRect.bottom;
        }
        else if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
            m_rcOrgWindowRect.top = rcWindowRect.top;
            m_rcOrgWindowRect.bottom = rcWindowRect.bottom;
            rcWindowRect.top = mi.rcWork.top;
            rcWindowRect.bottom = mi.rcWork.bottom;
        }
        m_bVertical = !m_bVertical;
        //m_bHorizontal = m_bHorizontal;
        MoveWindow(&rcWindowRect);
    }
    CStandAloneDialogTmpl<CResizableDialog>::OnNcMButtonUp(nHitTest, point);
}

void CResizableStandAloneDialog::OnNcRButtonUp(UINT nHitTest, CPoint point)
{
    WINDOWPLACEMENT windowPlacement;
    if ((nHitTest == HTMAXBUTTON) && GetWindowPlacement(&windowPlacement) && windowPlacement.showCmd == SW_SHOWNORMAL)
    {
        CRect rcWindowRect;
        GetWindowRect(&rcWindowRect);

        MONITORINFO mi = {0};
        mi.cbSize = sizeof(MONITORINFO);

        if (m_bHorizontal)
        {
            rcWindowRect.left = m_rcOrgWindowRect.left;
            rcWindowRect.right = m_rcOrgWindowRect.right;
        }
        else if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
            m_rcOrgWindowRect.left = rcWindowRect.left;
            m_rcOrgWindowRect.right = rcWindowRect.right;
            rcWindowRect.left = mi.rcWork.left;
            rcWindowRect.right = mi.rcWork.right;
        }
        //m_bVertical = m_bVertical;
        m_bHorizontal = !m_bHorizontal;
        MoveWindow(&rcWindowRect);
        // WORKAROUND
        // for some reasons, when the window is resized horizontally, its menu size is not get adjusted.
        // so, we force it to happen.
        SetMenu(GetMenu());
    }
    CStandAloneDialogTmpl<CResizableDialog>::OnNcRButtonUp(nHitTest, point);
}

void CResizableStandAloneDialog::OnCantStartThread()
{
    ::MessageBox(this->m_hWnd, (LPCTSTR)CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)), (LPCTSTR)CString(MAKEINTRESOURCE(IDS_APPNAME)), MB_OK | MB_ICONERROR);
}

bool CResizableStandAloneDialog::OnEnterPressed()
{
    if (GetAsyncKeyState(VK_CONTROL)&0x8000)
    {
        CWnd * pOkBtn = GetDlgItem(IDOK);
#ifdef ID_OK
        if (pOkBtn == NULL)
            pOkBtn = GetDlgItem(ID_OK);
#endif
        if ( pOkBtn && pOkBtn->IsWindowEnabled() )
        {
            if (DWORD(CRegStdDWORD(L"Software\\TortoiseSVN\\CtrlEnter", TRUE)))
                PostMessage(WM_COMMAND, IDOK);
        }
        return true;
    }
    return false;
}

BEGIN_MESSAGE_MAP(CStateDialog, CDialog)
    ON_WM_DESTROY()
END_MESSAGE_MAP()
