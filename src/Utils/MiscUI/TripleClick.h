// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2012, 2014, 2021 - TortoiseSVN

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
 * Helper to catch tripple mouse clicks.
 */
class CTripleClick
{
public:
    CTripleClick()
        : m_lastDblClickMsg(0)
        , m_lastDblClickTime(0)
    {
    }
    virtual ~CTripleClick() {}

    virtual void OnLButtonTrippleClick(UINT /*nFlags*/, CPoint /*point*/) {}
    virtual void OnMButtonTrippleClick(UINT /*nFlags*/, CPoint /*point*/) {}
    virtual void OnRButtonTrippleClick(UINT /*nFlags*/, CPoint /*point*/) {}

    BOOL RelayTrippleClick(MSG* pMsg)
    {
        if ((pMsg->message == WM_LBUTTONDBLCLK) ||
            (pMsg->message == WM_MBUTTONDBLCLK) ||
            (pMsg->message == WM_RBUTTONDBLCLK))
        {
            m_lastDblClickMsg  = pMsg->message;
            m_lastDblClickTime = GetTickCount64();
        }
        else if (
            ((pMsg->message == WM_LBUTTONDOWN) && (m_lastDblClickMsg == WM_LBUTTONDBLCLK)) ||
            ((pMsg->message == WM_MBUTTONDOWN) && (m_lastDblClickMsg == WM_MBUTTONDBLCLK)) ||
            ((pMsg->message == WM_RBUTTONDOWN) && (m_lastDblClickMsg == WM_RBUTTONDBLCLK)))
        {
            if ((GetTickCount64() - GetDoubleClickTime()) < m_lastDblClickTime)
            {
                m_lastDblClickTime = 0;
                m_lastDblClickMsg  = 0;
                CPoint pt;
                pt.x = GET_X_LPARAM(pMsg->lParam);
                pt.y = GET_Y_LPARAM(pMsg->lParam);
                switch (pMsg->message)
                {
                    case WM_LBUTTONDOWN:
                        OnLButtonTrippleClick(static_cast<UINT>(pMsg->wParam), pt);
                        break;
                    case WM_MBUTTONDOWN:
                        OnMButtonTrippleClick(static_cast<UINT>(pMsg->wParam), pt);
                        break;
                    case WM_RBUTTONDOWN:
                        OnRButtonTrippleClick(static_cast<UINT>(pMsg->wParam), pt);
                        break;
                }
                return TRUE;
            }
        }
        return FALSE;
    }

private:
    UINT      m_lastDblClickMsg;
    ULONGLONG m_lastDblClickTime;
};
