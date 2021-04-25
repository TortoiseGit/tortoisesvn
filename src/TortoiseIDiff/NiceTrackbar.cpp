// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2008, 2011, 2021 - TortoiseSVN

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
#include "NiceTrackbar.h"
#include <CommCtrl.h>
#include <WindowsX.h>

LRESULT CALLBACK CNiceTrackbar::NiceTrackbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto self = reinterpret_cast<CNiceTrackbar*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
        case WM_LBUTTONDOWN:
            self->m_dragging    = true;
            self->m_dragChanged = false;
            SetCapture(hwnd);
            //SetFocus(hwnd);
            if (self->SetThumb(lParam))
            {
                self->m_dragChanged = true;
                self->PostMessageToParent(TB_THUMBTRACK);
            }
            return 0;
        case WM_MOUSEMOVE:
            if (self->m_dragging)
            {
                if (self->SetThumb(lParam))
                {
                    self->m_dragChanged = true;
                    self->PostMessageToParent(TB_THUMBTRACK);
                }
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (self->m_dragging)
            {
                self->m_dragging = false;
                ReleaseCapture();
                if (self->SetThumb(lParam))
                {
                    self->PostMessageToParent(TB_ENDTRACK);
                    self->m_dragChanged = true;
                }
                if (self->m_dragChanged)
                {
                    self->PostMessageToParent(TB_THUMBPOSITION);
                    self->m_dragChanged = false;
                }
                return 0;
            }
            break;
        case WM_CAPTURECHANGED:
            if (self->m_dragging)
            {
                self->m_dragging = false;
                return 0;
            }
            break;
        case WM_DESTROY:
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(self->m_origProc));
            break;
    }
    return CallWindowProc(self->m_origProc, hwnd, message, wParam, lParam);
}

void CNiceTrackbar::ConvertTrackbarToNice(HWND window)
{
    m_window = window;

    // setup this pointer
    SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // subclass it
    m_origProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(NiceTrackbarProc)));
}

bool CNiceTrackbar::SetThumb(LPARAM lParamPoint) const
{
    POINT     point = {GET_X_LPARAM(lParamPoint), GET_Y_LPARAM(lParamPoint)};
    const int nMin  = static_cast<int>(SendMessage(m_window, TBM_GETRANGEMIN, 0, 0l));
    const int nMax  = static_cast<int>(SendMessage(m_window, TBM_GETRANGEMAX, 0, 0l));
    RECT      rc;
    SendMessage(m_window, TBM_GETCHANNELRECT, 0, reinterpret_cast<LPARAM>(&rc));
    double ratio;
    if (GetWindowLong(m_window, GWL_STYLE) & TBS_VERT)
    {
        // note: for vertical trackbar, it still returns the rectangle as if it was horizontal
        ratio = static_cast<double>(point.y - rc.left) / (rc.right - rc.left);
    }
    else
    {
        ratio = static_cast<double>(point.x - rc.left) / (rc.right - rc.left);
    }

    int nNewPos = static_cast<int>(nMin + (nMax - nMin) * ratio + 0.5); // round the result to go to the nearest tick mark

    const bool changed = (nNewPos != static_cast<int>(SendMessage(m_window, TBM_GETPOS, 0, 0)));
    if (changed)
    {
        SendMessage(m_window, TBM_SETPOS, TRUE, nNewPos);
    }
    return changed;
}

void CNiceTrackbar::PostMessageToParent(int tbCode) const
{
    HWND parent = GetParent(m_window);
    if (parent)
    {
        int  pos  = static_cast<int>(SendMessage(m_window, TBM_GETPOS, 0, 0));
        bool vert = (GetWindowLong(m_window, GWL_STYLE) & TBS_VERT) != 0;
        PostMessage(parent, vert ? WM_VSCROLL : WM_HSCROLL, static_cast<WPARAM>((pos << 16) | tbCode), reinterpret_cast<LPARAM>(m_window));
    }
}
