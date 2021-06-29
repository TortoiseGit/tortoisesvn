// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2016, 2018-2021 - TortoiseSVN
// Copyright (C) 2016 - TortoiseGit

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
#include "TortoiseIDiff.h"
#include "resource.h"
#include "PicWindow.h"
#include "ResString.h"

#include "../Utils/DPIAware.h"
#include "../Utils/LoadIconEx.h"
#include "../Utils/Theme.h"
#include "../Utils/DarkModeHelper.h"

#include <memory>
#include <shellapi.h>
#include <CommCtrl.h>

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "shell32.lib")

bool CPicWindow::RegisterAndCreateWindow(HWND hParent)
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with default parameters
    wcx.cbSize        = sizeof(WNDCLASSEX);
    wcx.style         = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc   = CWindow::stWinMsgHandler;
    wcx.cbClsExtra    = 0;
    wcx.cbWndExtra    = 0;
    wcx.hInstance     = hResource;
    wcx.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcx.lpszClassName = L"TortoiseIDiffPicWindow";
    wcx.hIcon         = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_TORTOISEIDIFF), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcx.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
    wcx.lpszMenuName  = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
    wcx.hIconSm       = LoadIconEx(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
    RegisterWindow(&wcx);
    if (CreateEx(WS_EX_ACCEPTFILES | WS_EX_CLIENTEDGE, WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, hParent))
    {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        CreateButtons();
        SetTheme(CTheme::Instance().IsDarkTheme());
        return true;
    }
    return false;
}

void CPicWindow::PositionTrackBar() const
{
    const auto sliderWidth = CDPIAware::Instance().Scale(*this, SLIDER_WIDTH);
    RECT       rc;
    GetClientRect(&rc);
    HWND slider = m_alphaSlider.GetWindow();
    if ((m_pSecondPic) && (m_blend == BLEND_ALPHA))
    {
        MoveWindow(slider, 0, rc.top - CDPIAware::Instance().Scale(*this, 4) + sliderWidth, sliderWidth, rc.bottom - rc.top - sliderWidth + CDPIAware::Instance().Scale(*this, 8), true);
        ShowWindow(slider, SW_SHOW);
        MoveWindow(m_hwndAlphaToggleBtn, 0, rc.top - CDPIAware::Instance().Scale(*this, 4), sliderWidth, sliderWidth, true);
        ShowWindow(m_hwndAlphaToggleBtn, SW_SHOW);
    }
    else
    {
        ShowWindow(slider, SW_HIDE);
        ShowWindow(m_hwndAlphaToggleBtn, SW_HIDE);
    }
}

LRESULT CALLBACK CPicWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACKMOUSEEVENT mevt;
    switch (uMsg)
    {
        case WM_CREATE:
        {
            // create a slider control
            CreateTrackbar(hwnd);
            ShowWindow(m_alphaSlider.GetWindow(), SW_HIDE);
            //Create the tooltips
            TOOLINFO ti;
            RECT     rect; // for client area coordinates

            m_hwndTT = CreateWindowEx(0,
                                      TOOLTIPS_CLASS,
                                      nullptr,
                                      WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      hwnd,
                                      nullptr,
                                      hResource,
                                      nullptr);

            SetWindowPos(m_hwndTT,
                         HWND_TOP,
                         0,
                         0,
                         0,
                         0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            ::GetClientRect(hwnd, &rect);

            ti.cbSize   = sizeof(TOOLINFO);
            ti.uFlags   = TTF_TRACK | TTF_ABSOLUTE;
            ti.hwnd     = hwnd;
            ti.hinst    = hResource;
            ti.uId      = 0;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            // ToolTip control will cover the whole window
            ti.rect.left   = rect.left;
            ti.rect.top    = rect.top;
            ti.rect.right  = rect.right;
            ti.rect.bottom = rect.bottom;

            SendMessage(m_hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
            SendMessage(m_hwndTT, TTM_SETMAXTIPWIDTH, 0, 600);
            m_nHSecondScrollPos = 0;
            m_nVSecondScrollPos = 0;
            m_themeCallbackId   = CTheme::Instance().RegisterThemeChangeCallback(
                [this]() {
                    SetTheme(CTheme::Instance().IsDarkTheme());
                });
        }
        break;
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            InvalidateRect(*this, nullptr, FALSE);
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            Paint(hwnd);
            break;
        case WM_SIZE:
            PositionTrackBar();
            SetupScrollBars();
            break;
        case WM_VSCROLL:
            if ((m_pSecondPic) && (reinterpret_cast<HWND>(lParam) == m_alphaSlider.GetWindow()))
            {
                if (LOWORD(wParam) == TB_THUMBTRACK)
                {
                    // while tracking, only redraw after 50 milliseconds
                    ::SetTimer(*this, TIMER_ALPHASLIDER, 50, nullptr);
                }
                else
                    SetBlendAlpha(m_blend, SendMessage(m_alphaSlider.GetWindow(), TBM_GETPOS, 0, 0) / 16.0f);
            }
            else
            {
                UINT nPos         = HIWORD(wParam);
                bool bForceUpdate = false;
                if (LOWORD(wParam) == SB_THUMBTRACK || LOWORD(wParam) == SB_THUMBPOSITION)
                {
                    // Get true 32-bit scroll position
                    SCROLLINFO si;
                    si.cbSize = sizeof(SCROLLINFO);
                    si.fMask  = SIF_TRACKPOS;
                    GetScrollInfo(*this, SB_VERT, &si);
                    nPos         = si.nTrackPos;
                    bForceUpdate = true;
                }

                OnVScroll(LOWORD(wParam), nPos);
                if (m_bLinkedPositions && m_pTheOtherPic)
                {
                    m_pTheOtherPic->OnVScroll(LOWORD(wParam), nPos);
                    if (bForceUpdate)
                        ::UpdateWindow(*m_pTheOtherPic);
                }
            }
            break;
        case WM_HSCROLL:
        {
            UINT nPos         = HIWORD(wParam);
            bool bForceUpdate = false;
            if (LOWORD(wParam) == SB_THUMBTRACK || LOWORD(wParam) == SB_THUMBPOSITION)
            {
                // Get true 32-bit scroll position
                SCROLLINFO si;
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask  = SIF_TRACKPOS;
                GetScrollInfo(*this, SB_VERT, &si);
                nPos         = si.nTrackPos;
                bForceUpdate = true;
            }

            OnHScroll(LOWORD(wParam), nPos);
            if (m_bLinkedPositions && m_pTheOtherPic)
            {
                m_pTheOtherPic->OnHScroll(LOWORD(wParam), nPos);
                if (bForceUpdate)
                    ::UpdateWindow(*m_pTheOtherPic);
            }
        }
        break;
        case WM_MOUSEWHEEL:
        {
            OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
        }
        break;
        case WM_MOUSEHWHEEL:
        {
            OnMouseWheel(GET_KEYSTATE_WPARAM(wParam) | MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
        }
        break;
        case WM_LBUTTONDOWN:
            SetFocus(*this);
            m_ptPanStart.x          = GET_X_LPARAM(lParam);
            m_ptPanStart.y          = GET_Y_LPARAM(lParam);
            m_startVScrollPos       = m_nVScrollPos;
            m_startHScrollPos       = m_nHScrollPos;
            m_startVSecondScrollPos = m_nVSecondScrollPos;
            m_startHSecondScrollPos = m_nHSecondScrollPos;
            m_bDragging             = true;
            SetCapture(*this);
            break;
        case WM_LBUTTONUP:
            m_bDragging = false;
            ReleaseCapture();
            InvalidateRect(*this, nullptr, FALSE);
            break;
        case WM_MOUSELEAVE:
            m_ptPanStart.x = -1;
            m_ptPanStart.y = -1;
            m_lastTTPos.x  = 0;
            m_lastTTPos.y  = 0;
            SendMessage(m_hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
            break;
        case WM_MOUSEMOVE:
        {
            mevt.cbSize      = sizeof(TRACKMOUSEEVENT);
            mevt.dwFlags     = TME_LEAVE;
            mevt.dwHoverTime = HOVER_DEFAULT;
            mevt.hwndTrack   = *this;
            ::TrackMouseEvent(&mevt);
            POINT pt = {static_cast<int>(LOWORD(lParam)), static_cast<int>(HIWORD(lParam))};
            if (pt.y < CDPIAware::Instance().Scale(*this, HEADER_HEIGHT))
            {
                ClientToScreen(*this, &pt);
                if ((abs(m_lastTTPos.x - pt.x) > 20) || (abs(m_lastTTPos.y - pt.y) > 10))
                {
                    m_lastTTPos = pt;
                    pt.x += 15;
                    pt.y += 15;
                    SendMessage(m_hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
                    TOOLINFO ti = {0};
                    ti.cbSize   = sizeof(TOOLINFO);
                    ti.hwnd     = *this;
                    ti.uId      = 0;
                    SendMessage(m_hwndTT, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));
                }
            }
            else
            {
                SendMessage(m_hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
                m_lastTTPos.x = 0;
                m_lastTTPos.y = 0;
            }
            if ((wParam & MK_LBUTTON) &&
                (m_ptPanStart.x >= 0) &&
                (m_ptPanStart.y >= 0))
            {
                // pan the image
                int xPos = GET_X_LPARAM(lParam);
                int yPos = GET_Y_LPARAM(lParam);

                if (wParam & MK_CONTROL)
                {
                    m_nHSecondScrollPos = m_startHSecondScrollPos + (m_ptPanStart.x - xPos);
                    m_nVSecondScrollPos = m_startVSecondScrollPos + (m_ptPanStart.y - yPos);
                }
                else if (wParam & MK_SHIFT)
                {
                    m_nHScrollPos = m_startHScrollPos + (m_ptPanStart.x - xPos);
                    m_nVScrollPos = m_startVScrollPos + (m_ptPanStart.y - yPos);
                }
                else
                {
                    m_nHSecondScrollPos = m_startHSecondScrollPos + (m_ptPanStart.x - xPos);
                    m_nVSecondScrollPos = m_startVSecondScrollPos + (m_ptPanStart.y - yPos);
                    m_nHScrollPos       = m_startHScrollPos + (m_ptPanStart.x - xPos);
                    m_nVScrollPos       = m_startVScrollPos + (m_ptPanStart.y - yPos);
                    if (!m_bLinkedPositions && m_pTheOtherPic)
                    {
                        // snap to the other picture borders
                        if (abs(m_nVScrollPos - m_pTheOtherPic->m_nVScrollPos) < 10)
                            m_nVScrollPos = m_pTheOtherPic->m_nVScrollPos;
                        if (abs(m_nHScrollPos - m_pTheOtherPic->m_nHScrollPos) < 10)
                            m_nHScrollPos = m_pTheOtherPic->m_nHScrollPos;
                    }
                }
                SetupScrollBars();
                InvalidateRect(*this, nullptr, TRUE);
                UpdateWindow(*this);
                if (m_pTheOtherPic && (m_bLinkedPositions) && ((wParam & MK_SHIFT) == 0))
                {
                    m_pTheOtherPic->m_nHScrollPos = m_nHScrollPos;
                    m_pTheOtherPic->m_nVScrollPos = m_nVScrollPos;
                    m_pTheOtherPic->SetupScrollBars();
                    InvalidateRect(*m_pTheOtherPic, nullptr, TRUE);
                    UpdateWindow(*m_pTheOtherPic);
                }
            }
        }
        break;
        case WM_SETCURSOR:
        {
            // we show a hand cursor if the image can be dragged,
            // and a hand-down cursor if the image is currently dragged
            if ((*this == reinterpret_cast<HWND>(wParam)) && (LOWORD(lParam) == HTCLIENT))
            {
                RECT rect;
                GetClientRect(&rect);
                LONG width  = m_picture.m_width;
                LONG height = m_picture.m_height;
                if (m_pSecondPic)
                {
                    width  = max(width, m_pSecondPic->m_width);
                    height = max(height, m_pSecondPic->m_height);
                }

                if ((GetKeyState(VK_LBUTTON) & 0x8000) || (HIWORD(lParam) == WM_LBUTTONDOWN))
                {
                    SetCursor(curHandDown);
                }
                else
                {
                    SetCursor(curHand);
                }
                return TRUE;
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        case WM_DROPFILES:
        {
            HDROP   hDrop                = reinterpret_cast<HDROP>(wParam);
            wchar_t szFileName[MAX_PATH] = {0};
            // we only use the first file dropped (if multiple files are dropped)
            if (DragQueryFile(hDrop, 0, szFileName, _countof(szFileName)))
            {
                SetPic(szFileName, L"", m_bMainPic);
                FitImageInWindow();
                InvalidateRect(*this, nullptr, TRUE);
            }
        }
        break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case LEFTBUTTON_ID:
                {
                    PrevImage();
                    if (m_bLinkedPositions && m_pTheOtherPic)
                        m_pTheOtherPic->PrevImage();
                    return 0;
                }
                case RIGHTBUTTON_ID:
                {
                    NextImage();
                    if (m_bLinkedPositions && m_pTheOtherPic)
                        m_pTheOtherPic->NextImage();
                    return 0;
                }
                case PLAYBUTTON_ID:
                {
                    m_bPlaying = !m_bPlaying;
                    Animate(m_bPlaying);
                    if (m_bLinkedPositions && m_pTheOtherPic)
                        m_pTheOtherPic->Animate(m_bPlaying);
                    return 0;
                }
                case ALPHATOGGLEBUTTON_ID:
                {
                    WORD msg = HIWORD(wParam);
                    switch (msg)
                    {
                        case BN_DOUBLECLICKED:
                        {
                            SendMessage(m_hwndAlphaToggleBtn, BM_SETSTATE, 1, 0);
                            SetTimer(*this, ID_ALPHATOGGLETIMER, 1000, nullptr);
                        }
                        break;
                        case BN_CLICKED:
                            KillTimer(*this, ID_ALPHATOGGLETIMER);
                            ToggleAlpha();
                            break;
                    }
                    return 0;
                }
                case BLENDALPHA_ID:
                {
                    m_blend = BLEND_ALPHA;
                    PositionTrackBar();
                    InvalidateRect(*this, nullptr, TRUE);
                }
                break;
                case BLENDXOR_ID:
                {
                    m_blend = BLEND_XOR;
                    PositionTrackBar();
                    InvalidateRect(*this, nullptr, TRUE);
                }
                break;
                case SELECTBUTTON_ID:
                {
                    SendMessage(GetParent(m_hwnd), WM_COMMAND, MAKEWPARAM(SELECTBUTTON_ID, SELECTBUTTON_ID), reinterpret_cast<LPARAM>(m_hwnd));
                }
                break;
            }
        }
        break;
        case WM_TIMER:
        {
            switch (wParam)
            {
                case ID_ANIMATIONTIMER:
                {
                    m_nCurrentFrame++;
                    if (m_nCurrentFrame > m_picture.GetNumberOfFrames(0))
                        m_nCurrentFrame = 1;
                    long delay = m_picture.SetActiveFrame(m_nCurrentFrame);
                    delay      = max(100, delay);
                    SetTimer(*this, ID_ANIMATIONTIMER, delay, nullptr);
                    InvalidateRect(*this, nullptr, FALSE);
                }
                break;
                case TIMER_ALPHASLIDER:
                {
                    SetBlendAlpha(m_blend, SendMessage(m_alphaSlider.GetWindow(), TBM_GETPOS, 0, 0) / 16.0f);
                    KillTimer(*this, TIMER_ALPHASLIDER);
                }
                break;
                case ID_ALPHATOGGLETIMER:
                {
                    ToggleAlpha();
                }
                break;
            }
        }
        break;
        case WM_NOTIFY:
        {
            LPNMHDR pNMHDR = reinterpret_cast<LPNMHDR>(lParam);
            if (pNMHDR->code == TTN_GETDISPINFO)
            {
                if (pNMHDR->hwndFrom == m_alphaSlider.GetWindow())
                {
                    LPTOOLTIPTEXT lpttt         = reinterpret_cast<LPNMTTDISPINFOW>(lParam);
                    lpttt->hinst                = hResource;
                    wchar_t stringBuf[MAX_PATH] = {0};
                    swprintf_s(stringBuf, L"%i%% alpha", static_cast<int>((SendMessage(m_alphaSlider.GetWindow(), TBM_GETPOS, 0, 0) / 16.0f * 100.0f)));
                    wcscpy_s(lpttt->lpszText, 80, stringBuf);
                }
                else if (pNMHDR->idFrom == reinterpret_cast<UINT_PTR>(m_hwndAlphaToggleBtn))
                {
                    swprintf_s(m_wszTip, static_cast<wchar_t const*>(ResString(hResource, IDS_ALPHABUTTONTT)), static_cast<int>((SendMessage(m_alphaSlider.GetWindow(), TBM_GETPOS, 0, 0) / 16.0f * 100.0f)));
                    if (pNMHDR->code == TTN_NEEDTEXTW)
                    {
                        NMTTDISPINFOW* pTttw = reinterpret_cast<NMTTDISPINFOW*>(pNMHDR);
                        pTttw->lpszText      = m_wszTip;
                    }
                    else
                    {
                        NMTTDISPINFOA* pTtta = reinterpret_cast<NMTTDISPINFOA*>(pNMHDR);
                        pTtta->lpszText      = m_szTip;
                        ::WideCharToMultiByte(CP_ACP, 0, m_wszTip, -1, m_szTip, 8192, nullptr, nullptr);
                    }
                }
                else
                {
                    BuildInfoString(m_wszTip, _countof(m_wszTip), true);
                    if (pNMHDR->code == TTN_NEEDTEXTW)
                    {
                        NMTTDISPINFOW* pTttw = reinterpret_cast<NMTTDISPINFOW*>(pNMHDR);
                        pTttw->lpszText      = m_wszTip;
                    }
                    else
                    {
                        NMTTDISPINFOA* pTtta = reinterpret_cast<NMTTDISPINFOA*>(pNMHDR);
                        pTtta->lpszText      = m_szTip;
                        ::WideCharToMultiByte(CP_ACP, 0, m_wszTip, -1, m_szTip, 8192, nullptr, nullptr);
                    }
                }
            }
        }
        break;
        case WM_DESTROY:
            DestroyIcon(m_hLeft);
            DestroyIcon(m_hRight);
            DestroyIcon(m_hPlay);
            DestroyIcon(m_hStop);
            bWindowClosed = TRUE;
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
};

void CPicWindow::NextImage()
{
    m_nCurrentDimension++;
    if (m_nCurrentDimension > m_picture.GetNumberOfDimensions())
        m_nCurrentDimension = m_picture.GetNumberOfDimensions();
    m_nCurrentFrame++;
    if (m_nCurrentFrame > m_picture.GetNumberOfFrames(0))
        m_nCurrentFrame = m_picture.GetNumberOfFrames(0);
    m_picture.SetActiveFrame(m_nCurrentFrame >= m_nCurrentDimension ? m_nCurrentFrame : m_nCurrentDimension);
    InvalidateRect(*this, nullptr, FALSE);
    PositionChildren();
}

void CPicWindow::PrevImage()
{
    m_nCurrentDimension--;
    if (m_nCurrentDimension < 1)
        m_nCurrentDimension = 1;
    m_nCurrentFrame--;
    if (m_nCurrentFrame < 1)
        m_nCurrentFrame = 1;
    m_picture.SetActiveFrame(m_nCurrentFrame >= m_nCurrentDimension ? m_nCurrentFrame : m_nCurrentDimension);
    InvalidateRect(*this, nullptr, FALSE);
    PositionChildren();
}

void CPicWindow::Animate(bool bStart) const
{
    if (bStart)
    {
        SendMessage(m_hwndPlayBtn, BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(m_hStop));
        SetTimer(*this, ID_ANIMATIONTIMER, 0, nullptr);
    }
    else
    {
        SendMessage(m_hwndPlayBtn, BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(m_hPlay));
        KillTimer(*this, ID_ANIMATIONTIMER);
    }
}

void CPicWindow::SetPic(const std::wstring& path, const std::wstring& title, bool bFirst)
{
    m_bMainPic = bFirst;
    m_picPath  = path;
    m_picTitle = title;
    m_picture.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    m_bValid      = m_picture.Load(m_picPath);
    m_nDimensions = m_picture.GetNumberOfDimensions();
    if (m_nDimensions)
        m_nFrames = m_picture.GetNumberOfFrames(0);
    if (m_bValid)
    {
        m_picScale = 100;
        PositionChildren();
        InvalidateRect(*this, nullptr, FALSE);
    }
}

void CPicWindow::DrawViewTitle(HDC hDC, RECT* rect)
{
    const auto headerHeight = CDPIAware::Instance().Scale(*this, HEADER_HEIGHT);
    HFONT      hFont        = nullptr;
    hFont                   = CreateFont(-CDPIAware::Instance().PointsToPixels(*this, m_pSecondPic ? 9 : 10), 0, 0, 0, FW_DONTCARE, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT hFontOld          = static_cast<HFONT>(SelectObject(hDC, static_cast<HGDIOBJ>(hFont)));

    RECT textrect;
    textrect.left   = rect->left;
    textrect.top    = rect->top;
    textrect.right  = rect->right;
    textrect.bottom = rect->top + headerHeight;
    if (HasMultipleImages())
        textrect.bottom += headerHeight;

    COLORREF crBk = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_SCROLLBAR));
    COLORREF crFg = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
    SetBkColor(hDC, crBk);
    ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &textrect, nullptr, 0, nullptr);

    if (GetFocus() == *this)
        DrawEdge(hDC, &textrect, EDGE_BUMP, BF_RECT);
    else
        DrawEdge(hDC, &textrect, EDGE_ETCHED, BF_RECT);

    SetTextColor(hDC, crFg);

    // use the path if no title is set.
    std::wstring* title = m_picTitle.empty() ? &m_picPath : &m_picTitle;

    std::wstring realTitle = *title;
    std::wstring imgNumString;

    if (HasMultipleImages())
    {
        wchar_t buf[MAX_PATH] = {0};
        if (m_nFrames > 1)
            swprintf_s(buf, static_cast<const wchar_t*>(ResString(hResource, IDS_DIMENSIONSANDFRAMES)), m_nCurrentFrame, m_nFrames);
        else
            swprintf_s(buf, static_cast<const wchar_t*>(ResString(hResource, IDS_DIMENSIONSANDFRAMES)), m_nCurrentDimension, m_nDimensions);
        imgNumString = buf;
    }

    SIZE stringSize;
    if (GetTextExtentPoint32(hDC, realTitle.c_str(), static_cast<int>(realTitle.size()), &stringSize))
    {
        int nStringLength = stringSize.cx;
        int textTop       = m_pSecondPic ? textrect.top + (headerHeight / 2) - stringSize.cy : textrect.top + (headerHeight / 2) - stringSize.cy / 2;

        RECT drawRC = textrect;
        drawRC.left = max(textrect.left + ((textrect.right - textrect.left) - nStringLength) / 2, 1);
        drawRC.top  = textTop;
        ::DrawText(hDC, realTitle.c_str(), static_cast<int>(realTitle.size()), &drawRC, DT_HIDEPREFIX | DT_NOPREFIX | DT_SINGLELINE);

        if (m_pSecondPic)
        {
            realTitle   = (m_picTitle2.empty() ? m_picPath2 : m_picTitle2);
            drawRC      = textrect;
            drawRC.left = max(textrect.left + ((textrect.right - textrect.left) - nStringLength) / 2, 1);
            drawRC.top  = textTop + stringSize.cy;
            ::DrawText(hDC, realTitle.c_str(), static_cast<int>(realTitle.size()), &drawRC, DT_HIDEPREFIX | DT_NOPREFIX | DT_SINGLELINE);
        }
    }
    if (HasMultipleImages())
    {
        if (GetTextExtentPoint32(hDC, imgNumString.c_str(), static_cast<int>(imgNumString.size()), &stringSize))
        {
            int nStringLength = stringSize.cx;

            RECT drawRC = textrect;
            drawRC.left = max(textrect.left + ((textrect.right - textrect.left) - nStringLength) / 2, 1);
            drawRC.top  = textrect.top + headerHeight + (headerHeight / 2) - stringSize.cy / 2;
            ::DrawText(hDC, imgNumString.c_str(), static_cast<int>(imgNumString.size()), &drawRC, DT_HIDEPREFIX | DT_NOPREFIX | DT_SINGLELINE);
        }
    }
    SelectObject(hDC, static_cast<HGDIOBJ>(hFontOld));
    DeleteObject(hFont);
}

void CPicWindow::SetupScrollBars() const
{
    RECT rect;
    GetClientRect(&rect);

    SCROLLINFO si = {sizeof(si)};

    si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;

    long width  = m_picture.m_width * m_picScale / 100;
    long height = m_picture.m_height * m_picScale / 100;
    if (m_pSecondPic && m_pTheOtherPic)
    {
        width  = max(width, m_pSecondPic->m_width * m_pTheOtherPic->GetZoom() / 100);
        height = max(height, m_pSecondPic->m_height * m_pTheOtherPic->GetZoom() / 100);
    }

    bool bShowHScrollBar = (m_nHScrollPos > 0);                                                  // left of pic is left of window
    bShowHScrollBar      = bShowHScrollBar || (width - m_nHScrollPos > rect.right);              // right of pic is outside right of window
    bShowHScrollBar      = bShowHScrollBar || (width + m_nHScrollPos > rect.right);              // right of pic is outside right of window
    bool bShowVScrollBar = (m_nVScrollPos > 0);                                                  // top of pic is above window
    bShowVScrollBar      = bShowVScrollBar || (height - m_nVScrollPos + rect.top > rect.bottom); // bottom of pic is below window
    bShowVScrollBar      = bShowVScrollBar || (height + m_nVScrollPos + rect.top > rect.bottom); // bottom of pic is below window

    // if the image is smaller than the window, we don't need the scrollbars
    ShowScrollBar(*this, SB_HORZ, bShowHScrollBar);
    ShowScrollBar(*this, SB_VERT, bShowVScrollBar);

    if (bShowVScrollBar)
    {
        si.nPos  = m_nVScrollPos;
        si.nPage = rect.bottom - rect.top;
        if (height < rect.bottom - rect.top)
        {
            if (m_nVScrollPos > 0)
            {
                si.nMin = 0;
                si.nMax = rect.bottom + m_nVScrollPos - rect.top;
            }
            else
            {
                si.nMin = m_nVScrollPos;
                si.nMax = static_cast<int>(height);
            }
        }
        else
        {
            if (m_nVScrollPos > 0)
            {
                si.nMin = 0;
                si.nMax = static_cast<int>(max(height, rect.bottom + m_nVScrollPos - rect.top));
            }
            else
            {
                si.nMin = 0;
                si.nMax = static_cast<int>(height - m_nVScrollPos);
            }
        }
        SetScrollInfo(*this, SB_VERT, &si, TRUE);
    }

    if (bShowHScrollBar)
    {
        si.nPos  = m_nHScrollPos;
        si.nPage = rect.right - rect.left;
        if (width < rect.right - rect.left)
        {
            if (m_nHScrollPos > 0)
            {
                si.nMin = 0;
                si.nMax = rect.right + m_nHScrollPos - rect.left;
            }
            else
            {
                si.nMin = m_nHScrollPos;
                si.nMax = static_cast<int>(width);
            }
        }
        else
        {
            if (m_nHScrollPos > 0)
            {
                si.nMin = 0;
                si.nMax = static_cast<int>(max(width, rect.right + m_nHScrollPos - rect.left));
            }
            else
            {
                si.nMin = 0;
                si.nMax = static_cast<int>(width - m_nHScrollPos);
            }
        }
        SetScrollInfo(*this, SB_HORZ, &si, TRUE);
    }

    PositionChildren();
}

void CPicWindow::OnVScroll(UINT nSbCode, UINT nPos)
{
    RECT rect;
    GetClientRect(&rect);

    switch (nSbCode)
    {
        case SB_BOTTOM:
            m_nVScrollPos = static_cast<LONG>(m_picture.GetHeight() * m_picScale / 100);
            break;
        case SB_TOP:
            m_nVScrollPos = 0;
            break;
        case SB_LINEDOWN:
            m_nVScrollPos++;
            break;
        case SB_LINEUP:
            m_nVScrollPos--;
            break;
        case SB_PAGEDOWN:
            m_nVScrollPos += (rect.bottom - rect.top);
            break;
        case SB_PAGEUP:
            m_nVScrollPos -= (rect.bottom - rect.top);
            break;
        case SB_THUMBPOSITION:
            m_nVScrollPos = nPos;
            break;
        case SB_THUMBTRACK:
            m_nVScrollPos = nPos;
            break;
        default:
            return;
    }
    LONG height = static_cast<LONG>(m_picture.GetHeight() * m_picScale / 100);
    if (m_pSecondPic)
    {
        height              = max(height, static_cast<LONG>(m_pSecondPic->GetHeight() * m_picScale / 100));
        m_nVSecondScrollPos = m_nVScrollPos;
    }
    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, nullptr, TRUE);
}

void CPicWindow::OnHScroll(UINT nSbCode, UINT nPos)
{
    RECT rect;
    GetClientRect(&rect);

    switch (nSbCode)
    {
        case SB_RIGHT:
            m_nHScrollPos = static_cast<LONG>(m_picture.GetWidth() * m_picScale / 100);
            break;
        case SB_LEFT:
            m_nHScrollPos = 0;
            break;
        case SB_LINERIGHT:
            m_nHScrollPos++;
            break;
        case SB_LINELEFT:
            m_nHScrollPos--;
            break;
        case SB_PAGERIGHT:
            m_nHScrollPos += (rect.right - rect.left);
            break;
        case SB_PAGELEFT:
            m_nHScrollPos -= (rect.right - rect.left);
            break;
        case SB_THUMBPOSITION:
            m_nHScrollPos = nPos;
            break;
        case SB_THUMBTRACK:
            m_nHScrollPos = nPos;
            break;
        default:
            return;
    }
    LONG width = static_cast<LONG>(m_picture.GetWidth() * m_picScale / 100);
    if (m_pSecondPic)
    {
        width               = max(width, static_cast<LONG>(m_pSecondPic->GetWidth() * m_picScale / 100));
        m_nHSecondScrollPos = m_nHScrollPos;
    }
    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, nullptr, TRUE);
}

void CPicWindow::OnMouseWheel(short fwKeys, short zDelta)
{
    RECT rect;
    GetClientRect(&rect);
    LONG width  = static_cast<long>(m_picture.m_width * m_picScale / 100);
    LONG height = static_cast<long>(m_picture.m_height * m_picScale / 100);
    if (m_pSecondPic)
    {
        width  = max(width, static_cast<long>(m_pSecondPic->m_width * m_picScale / 100));
        height = max(height, static_cast<long>(m_pSecondPic->m_height * m_picScale / 100));
    }
    if ((fwKeys & MK_SHIFT) && (fwKeys & MK_CONTROL) && (m_pSecondPic))
    {
        // ctrl+shift+wheel: change the alpha channel
        float a = m_blendAlpha;
        a -= static_cast<float>(zDelta) / 120.0f / 4.0f;
        if (a < 0.0f)
            a = 0.0f;
        else if (a > 1.0f)
            a = 1.0f;
        SetBlendAlpha(m_blend, a);
    }
    else if (fwKeys & MK_SHIFT)
    {
        // shift means scrolling sideways
        OnHScroll(SB_THUMBPOSITION, GetHPos() - zDelta);
        if ((m_bLinkedPositions) && (m_pTheOtherPic))
        {
            m_pTheOtherPic->OnHScroll(SB_THUMBPOSITION, m_pTheOtherPic->GetHPos() - zDelta);
        }
    }
    else if (fwKeys & MK_CONTROL)
    {
        // control means adjusting the scale factor
        Zoom(zDelta > 0, true);
        PositionChildren();
        InvalidateRect(*this, nullptr, FALSE);
        SetWindowPos(*this, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOREPOSITION | SWP_NOMOVE);
        UpdateWindow(*this);
        if ((m_bLinkedPositions || m_bOverlap) && m_pTheOtherPic)
        {
            m_pTheOtherPic->m_nHScrollPos = m_nHScrollPos;
            m_pTheOtherPic->m_nVScrollPos = m_nVScrollPos;
            m_pTheOtherPic->SetupScrollBars();
            InvalidateRect(*m_pTheOtherPic, nullptr, TRUE);
            UpdateWindow(*m_pTheOtherPic);
        }
    }
    else
    {
        OnVScroll(SB_THUMBPOSITION, GetVPos() - zDelta);
        if ((m_bLinkedPositions) && (m_pTheOtherPic))
        {
            m_pTheOtherPic->OnVScroll(SB_THUMBPOSITION, m_pTheOtherPic->GetVPos() - zDelta);
        }
    }
}

void CPicWindow::GetClientRect(RECT* pRect) const
{
    ::GetClientRect(*this, pRect);
    pRect->top += CDPIAware::Instance().Scale(*this, HEADER_HEIGHT);
    if (HasMultipleImages())
    {
        pRect->top += CDPIAware::Instance().Scale(*this, HEADER_HEIGHT);
    }
    if (m_pSecondPic)
        pRect->left += CDPIAware::Instance().Scale(*this, SLIDER_WIDTH);
}

void CPicWindow::GetClientRectWithScrollbars(RECT* pRect) const
{
    GetClientRect(pRect);
    ::GetWindowRect(*this, pRect);
    pRect->right  = pRect->right - pRect->left;
    pRect->bottom = pRect->bottom - pRect->top;
    pRect->top    = 0;
    pRect->left   = 0;
    pRect->top += CDPIAware::Instance().Scale(*this, HEADER_HEIGHT);
    if (HasMultipleImages())
    {
        pRect->top += CDPIAware::Instance().Scale(*this, HEADER_HEIGHT);
    }
    if (m_pSecondPic)
        pRect->left += CDPIAware::Instance().Scale(*this, SLIDER_WIDTH);
};

void CPicWindow::SetZoom(int zoom, bool centerMouse, bool inZoom)
{
    // Set the interpolation mode depending on zoom
    int oldPicScale      = m_picScale;
    int oldOtherPicScale = m_picScale;

    m_picture.SetInterpolationMode(InterpolationModeNearestNeighbor);
    if (m_pSecondPic)
        m_pSecondPic->SetInterpolationMode(InterpolationModeNearestNeighbor);

    if ((oldPicScale == 0) || (zoom == 0))
        return;

    m_picScale = zoom;

    if (m_pTheOtherPic && !inZoom)
    {
        if (m_bOverlap)
        {
            m_pTheOtherPic->SetZoom(zoom, centerMouse, true);
        }
        if (m_bFitHeights)
        {
            m_linkedHeight = 0;
            m_pTheOtherPic->SetZoomToHeight(m_picture.m_height * zoom / 100);
        }
        if (m_bFitWidths)
        {
            m_linkedWidth = 0;
            m_pTheOtherPic->SetZoomToWidth(m_picture.m_width * zoom / 100);
        }
    }

    // adjust the scrollbar positions according to the new zoom and the
    // mouse position: if possible, keep the pixel where the mouse pointer
    // is at the same position after the zoom
    if (!inZoom)
    {
        POINT cpos;
        DWORD ptW = GetMessagePos();
        cpos.x    = GET_X_LPARAM(ptW);
        cpos.y    = GET_Y_LPARAM(ptW);
        ScreenToClient(*this, &cpos);
        RECT clientRect;
        GetClientRect(&clientRect);
        if ((PtInRect(&clientRect, cpos)) && (centerMouse))
        {
            // the mouse pointer is over our window
            m_nHScrollPos = (m_nHScrollPos + cpos.x) * zoom / oldPicScale - cpos.x;
            m_nVScrollPos = (m_nVScrollPos + cpos.y) * zoom / oldPicScale - cpos.y;
            if (m_pTheOtherPic && m_bMainPic)
            {
                int otherzoom       = m_pTheOtherPic->GetZoom();
                m_nHSecondScrollPos = (m_nHSecondScrollPos + cpos.x) * otherzoom / oldOtherPicScale - cpos.x;
                m_nVSecondScrollPos = (m_nVSecondScrollPos + cpos.y) * otherzoom / oldOtherPicScale - cpos.y;
            }
        }
        else
        {
            m_nHScrollPos = (m_nHScrollPos + ((clientRect.right - clientRect.left) / 2)) * zoom / oldPicScale - ((clientRect.right - clientRect.left) / 2);
            m_nVScrollPos = (m_nVScrollPos + ((clientRect.bottom - clientRect.top) / 2)) * zoom / oldPicScale - ((clientRect.bottom - clientRect.top) / 2);
            if (m_pTheOtherPic && m_bMainPic)
            {
                int otherzoom       = m_pTheOtherPic->GetZoom();
                m_nHSecondScrollPos = (m_nHSecondScrollPos + ((clientRect.right - clientRect.left) / 2)) * otherzoom / oldOtherPicScale - ((clientRect.right - clientRect.left) / 2);
                m_nVSecondScrollPos = (m_nVSecondScrollPos + ((clientRect.bottom - clientRect.top) / 2)) * otherzoom / oldOtherPicScale - ((clientRect.bottom - clientRect.top) / 2);
            }
        }
    }

    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, nullptr, TRUE);
}

void CPicWindow::Zoom(bool in, bool centermouse)
{
    int zoomFactor;

    // Find correct zoom factor and quantize picscale
    if (m_picScale % 10)
    {
        m_picScale /= 10;
        m_picScale *= 10;
        if (!in)
            m_picScale += 10;
    }

    if (!in && m_picScale <= 20)
    {
        m_picScale = 10;
        zoomFactor = 0;
    }
    else if ((in && m_picScale < 100) || (!in && m_picScale <= 100))
    {
        zoomFactor = 10;
    }
    else if ((in && m_picScale < 200) || (!in && m_picScale <= 200))
    {
        zoomFactor = 20;
    }
    else
    {
        zoomFactor = 10;
    }

    // Set zoom
    if (in)
    {
        SetZoom(m_picScale + zoomFactor, centermouse);
    }
    else
    {
        SetZoom(m_picScale - zoomFactor, centermouse);
    }
}

void CPicWindow::FitImageInWindow()
{
    RECT rect;

    GetClientRectWithScrollbars(&rect);

    const auto border = CDPIAware::Instance().Scale(*this, 2);
    if (rect.right - rect.left)
    {
        int zoom = 100;
        if (((rect.right - rect.left) > m_picture.m_width + border) && ((rect.bottom - rect.top) > m_picture.m_height + border))
        {
            // image is smaller than the window
            zoom = 100;
        }
        else
        {
            // image is bigger than the window
            if (m_picture.m_width > 0 && m_picture.m_height > 0)
            {
                int xScale = (rect.right - rect.left - border) * 100 / m_picture.m_width;
                int yScale = (rect.bottom - rect.top - border) * 100 / m_picture.m_height;
                zoom       = min(yScale, xScale);
            }
        }
        if (m_pSecondPic)
        {
            if (((rect.right - rect.left) > m_pSecondPic->m_width + border) && ((rect.bottom - rect.top) > m_pSecondPic->m_height + border))
            {
                // image is smaller than the window
                if (m_pTheOtherPic)
                    m_pTheOtherPic->SetZoom(min(100, zoom), false);
            }
            else
            {
                // image is bigger than the window
                int xScale = (rect.right - rect.left - border) * 100 / m_pSecondPic->m_width;
                int yScale = (rect.bottom - rect.top - border) * 100 / m_pSecondPic->m_height;
                if (m_pTheOtherPic)
                    m_pTheOtherPic->SetZoom(min(yScale, xScale), false);
            }
            m_nHSecondScrollPos = 0;
            m_nVSecondScrollPos = 0;
        }
        SetZoom(zoom, false);
    }
    CenterImage();
    PositionChildren();
    InvalidateRect(*this, nullptr, TRUE);
}

void CPicWindow::CenterImage()
{
    RECT rect;
    GetClientRectWithScrollbars(&rect);
    const auto border = CDPIAware::Instance().Scale(*this, 2);
    long       width  = m_picture.m_width * m_picScale / 100 + border;
    long       height = m_picture.m_height * m_picScale / 100 + border;
    if (m_pSecondPic && m_pTheOtherPic)
    {
        width  = max(width, m_pSecondPic->m_width * m_pTheOtherPic->GetZoom() / 100 + border);
        height = max(height, m_pSecondPic->m_height * m_pTheOtherPic->GetZoom() / 100 + border);
    }

    bool bPicWidthBigger  = (static_cast<int>(width) > (rect.right - rect.left));
    bool bPicHeightBigger = (static_cast<int>(height) > (rect.bottom - rect.top));
    // set the scroll position so that the image is drawn centered in the window
    // if the window is bigger than the image
    if (!bPicWidthBigger)
    {
        m_nHScrollPos       = -((rect.right - rect.left + 4) - static_cast<int>(width)) / 2;
        m_nHSecondScrollPos = m_nHScrollPos;
    }
    if (!bPicHeightBigger)
    {
        m_nVScrollPos       = -((rect.bottom - rect.top + 4) - static_cast<int>(height)) / 2;
        m_nVSecondScrollPos = m_nVScrollPos;
    }
    SetupScrollBars();
}

void CPicWindow::FitWidths(bool bFit)
{
    m_bFitWidths = bFit;

    SetZoom(GetZoom(), false);
}

void CPicWindow::FitHeights(bool bFit)
{
    m_bFitHeights = bFit;

    SetZoom(GetZoom(), false);
}

void CPicWindow::ShowPicWithBorder(HDC hdc, const RECT& bounds, CPicture& pic, int scale)
{
    ::SetBkColor(hdc, GetTransparentThemedColor());
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &bounds, nullptr, 0, nullptr);

    RECT picrect;
    picrect.left = bounds.left - m_nHScrollPos;
    picrect.top  = bounds.top - m_nVScrollPos;
    if ((!m_bLinkedPositions || m_bOverlap) && (m_pTheOtherPic) && (&pic != &m_picture))
    {
        picrect.left = bounds.left - m_nHSecondScrollPos;
        picrect.top  = bounds.top - m_nVSecondScrollPos;
    }
    picrect.right  = (picrect.left + pic.m_width * scale / 100);
    picrect.bottom = (picrect.top + pic.m_height * scale / 100);

    if (m_bFitWidths && m_linkedWidth)
        picrect.right = picrect.left + m_linkedWidth;
    if (m_bFitHeights && m_linkedHeight)
        picrect.bottom = picrect.top + m_linkedHeight;

    pic.Show(hdc, picrect);

    const auto borderSize = CDPIAware::Instance().Scale(*this, 1);

    RECT border;
    border.left   = picrect.left - borderSize;
    border.top    = picrect.top - borderSize;
    border.right  = picrect.right + borderSize;
    border.bottom = picrect.bottom + borderSize;

    HPEN hPen    = CreatePen(PS_SOLID, 1, CTheme::Instance().GetThemeColor(GetSysColor(COLOR_3DDKSHADOW)));
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    MoveToEx(hdc, border.left, border.top, nullptr);
    LineTo(hdc, border.left, border.bottom);
    LineTo(hdc, border.right, border.bottom);
    LineTo(hdc, border.right, border.top);
    LineTo(hdc, border.left, border.top);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void CPicWindow::Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC         hdc;
    RECT        rect, fullRect;

    GetUpdateRect(hwnd, &rect, FALSE);
    if (IsRectEmpty(&rect))
        return;

    const auto sliderWidth = CDPIAware::Instance().Scale(*this, SLIDER_WIDTH);
    const auto border      = CDPIAware::Instance().Scale(*this, 4);
    ::GetClientRect(*this, &fullRect);
    hdc = BeginPaint(hwnd, &ps);
    {
        // Exclude the alpha control and button
        if ((m_pSecondPic) && (m_blend == BLEND_ALPHA))
            ExcludeClipRect(hdc, 0, m_infoRect.top - border, sliderWidth, m_infoRect.bottom + border);

        CMyMemDC memDC(hdc);
        if ((m_pSecondPic) && (m_blend != BLEND_ALPHA))
        {
            // erase the place where the alpha slider would be
            ::SetBkColor(memDC, GetTransparentThemedColor());
            RECT bounds = {0, m_infoRect.top - border, sliderWidth, m_infoRect.bottom + border};
            ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &bounds, nullptr, 0, nullptr);
        }

        GetClientRect(&rect);
        if (m_bValid)
        {
            ShowPicWithBorder(memDC, rect, m_picture, m_picScale);
            if (m_pSecondPic)
            {
                HDC     secondhdc  = CreateCompatibleDC(hdc);
                HBITMAP hBitmap    = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
                HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(secondhdc, hBitmap));
                SetWindowOrgEx(secondhdc, rect.left, rect.top, nullptr);

                if ((m_pSecondPic) && (m_blend != BLEND_ALPHA))
                {
                    // erase the place where the alpha slider would be
                    ::SetBkColor(secondhdc, GetTransparentThemedColor());
                    RECT bounds = {0, m_infoRect.top - border, sliderWidth, m_infoRect.bottom + border};
                    ::ExtTextOut(secondhdc, 0, 0, ETO_OPAQUE, &bounds, nullptr, 0, nullptr);
                }
                if (m_pTheOtherPic)
                    ShowPicWithBorder(secondhdc, rect, *m_pSecondPic, m_pTheOtherPic->GetZoom());

                if (m_blend == BLEND_ALPHA)
                {
                    BLENDFUNCTION blender;
                    blender.AlphaFormat         = 0;
                    blender.BlendFlags          = 0;
                    blender.BlendOp             = AC_SRC_OVER;
                    blender.SourceConstantAlpha = static_cast<BYTE>(m_blendAlpha * 255);
                    AlphaBlend(memDC,
                               rect.left,
                               rect.top,
                               rect.right - rect.left,
                               rect.bottom - rect.top,
                               secondhdc,
                               rect.left,
                               rect.top,
                               rect.right - rect.left,
                               rect.bottom - rect.top,
                               blender);
                }
                else if (m_blend == BLEND_XOR)
                {
                    BitBlt(memDC,
                           rect.left,
                           rect.top,
                           rect.right - rect.left,
                           rect.bottom - rect.top,
                           secondhdc,
                           rect.left,
                           rect.top,
                           //rect.right-rect.left,
                           //rect.bottom-rect.top,
                           SRCINVERT);
                    InvertRect(memDC, &rect);
                }
                SelectObject(secondhdc, hOldBitmap);
                DeleteObject(hBitmap);
                DeleteDC(secondhdc);
            }
            else if (m_bDragging && m_pTheOtherPic && !m_bLinkedPositions)
            {
                // when dragging, show lines indicating the position of the other image
                HPEN hPen    = CreatePen(PS_SOLID, 1, CTheme::Instance().GetThemeColor(GetSysColor(/*COLOR_ACTIVEBORDER*/ COLOR_HIGHLIGHT)));
                HPEN hOldPen = static_cast<HPEN>(SelectObject(memDC, hPen));
                int  xPos    = rect.left - m_pTheOtherPic->m_nHScrollPos - 1;
                MoveToEx(memDC, xPos, rect.top, nullptr);
                LineTo(memDC, xPos, rect.bottom);
                xPos = rect.left - m_pTheOtherPic->m_nHScrollPos + m_pTheOtherPic->m_picture.m_width * m_pTheOtherPic->GetZoom() / 100 + 1;
                if (m_bFitWidths && m_linkedWidth)
                    xPos = rect.left + m_pTheOtherPic->m_linkedWidth + 1;
                MoveToEx(memDC, xPos, rect.top, nullptr);
                LineTo(memDC, xPos, rect.bottom);

                int yPos = rect.top - m_pTheOtherPic->m_nVScrollPos - 1;
                MoveToEx(memDC, rect.left, yPos, nullptr);
                LineTo(memDC, rect.right, yPos);
                yPos = rect.top - m_pTheOtherPic->m_nVScrollPos + m_pTheOtherPic->m_picture.m_height * m_pTheOtherPic->GetZoom() / 100 + 1;
                if (m_bFitHeights && m_linkedHeight)
                    yPos = rect.top - m_pTheOtherPic->m_linkedHeight + 1;
                MoveToEx(memDC, rect.left, yPos, nullptr);
                LineTo(memDC, rect.right, yPos);

                SelectObject(memDC, hOldPen);
                DeleteObject(hPen);
            }

            int localSliderWidth = 0;
            if ((m_pSecondPic) && (m_blend == BLEND_ALPHA))
                localSliderWidth = sliderWidth;
            m_infoRect.left   = rect.left + border + localSliderWidth;
            m_infoRect.top    = rect.top;
            m_infoRect.right  = rect.right + localSliderWidth;
            m_infoRect.bottom = rect.bottom;

            SetBkColor(memDC, GetTransparentThemedColor());
            if (m_bShowInfo)
            {
                auto infoString = std::make_unique<wchar_t[]>(8192);
                BuildInfoString(infoString.get(), 8192, false);
                // set the font
                NONCLIENTMETRICS metrics = {0};
                metrics.cbSize           = sizeof(NONCLIENTMETRICS);
                SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
                metrics.lfStatusFont.lfHeight = static_cast<LONG>(CDPIAware::Instance().ScaleFactorSystemToWindow(*this) * metrics.lfStatusFont.lfHeight);
                HFONT hFont                   = CreateFontIndirect(&metrics.lfStatusFont);
                HFONT hFontOld                = static_cast<HFONT>(SelectObject(memDC, static_cast<HGDIOBJ>(hFont)));
                // find out how big the rectangle for the text has to be
                DrawText(memDC, infoString.get(), -1, &m_infoRect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_VCENTER | DT_CALCRECT);

                // the text should be drawn with a four pixel offset to the window borders
                m_infoRect.top    = rect.bottom - (m_infoRect.bottom - m_infoRect.top) - border;
                m_infoRect.bottom = rect.bottom - 4;

                // first draw an edge rectangle
                RECT edgerect;
                edgerect.left   = m_infoRect.left - border;
                edgerect.top    = m_infoRect.top - border;
                edgerect.right  = m_infoRect.right + border;
                edgerect.bottom = m_infoRect.bottom + border;
                ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &edgerect, nullptr, 0, nullptr);
                DrawEdge(memDC, &edgerect, EDGE_BUMP, BF_RECT | BF_SOFT);

                SetTextColor(memDC, CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT));
                DrawText(memDC, infoString.get(), -1, &m_infoRect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_VCENTER);
                SelectObject(memDC, static_cast<HGDIOBJ>(hFontOld));
                DeleteObject(hFont);
            }
        }
        else
        {
            SetBkColor(memDC, CTheme::Instance().IsDarkTheme() ? CTheme::darkBkColor : GetSysColor(COLOR_WINDOW));
            SetTextColor(memDC, CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT));
            ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);
            SIZE      stringsize;
            ResString str = ResString(hResource, IDS_INVALIDIMAGEINFO);

            // set the font
            NONCLIENTMETRICS metrics = {0};
            metrics.cbSize           = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
            metrics.lfStatusFont.lfHeight = static_cast<LONG>(CDPIAware::Instance().ScaleFactorSystemToWindow(*this) * metrics.lfStatusFont.lfHeight);
            HFONT hFont                   = CreateFontIndirect(&metrics.lfStatusFont);
            HFONT hFontOld                = static_cast<HFONT>(SelectObject(memDC, static_cast<HGDIOBJ>(hFont)));

            if (GetTextExtentPoint32(memDC, str, static_cast<int>(wcslen(str)), &stringsize))
            {
                int nStringLength = stringsize.cx;

                RECT drawRC = rect;
                drawRC.left = max(rect.left + ((rect.right - rect.left) - nStringLength) / 2, 1);
                drawRC.top  = rect.top + ((rect.bottom - rect.top) - stringsize.cy) / 2;
                ::DrawText(memDC, str, static_cast<int>(wcslen(str)), &drawRC, DT_HIDEPREFIX | DT_NOPREFIX | DT_SINGLELINE);
            }
            SelectObject(memDC, static_cast<HGDIOBJ>(hFontOld));
            DeleteObject(hFont);
        }
        DrawViewTitle(memDC, &fullRect);
    }
    EndPaint(hwnd, &ps);
}

bool CPicWindow::CreateButtons()
{
    // Ensure that the common control DLL is loaded.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    m_hwndLeftBtn = CreateWindowEx(0,
                                   L"BUTTON",
                                   static_cast<LPCWSTR>(nullptr),
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                   0, 0, 0, 0,
                                   static_cast<HWND>(*this),
                                   reinterpret_cast<HMENU>(LEFTBUTTON_ID),
                                   hResource,
                                   nullptr);
    if (m_hwndLeftBtn == INVALID_HANDLE_VALUE)
        return false;
    int iconWidth  = GetSystemMetrics(SM_CXSMICON);
    int iconHeight = GetSystemMetrics(SM_CYSMICON);
    m_hLeft        = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_BACKWARD), iconWidth, iconHeight);
    SendMessage(m_hwndLeftBtn, BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(m_hLeft));
    m_hwndRightBtn = CreateWindowEx(0,
                                    L"BUTTON",
                                    static_cast<LPCWSTR>(nullptr),
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                    0, 0, 0, 0,
                                    *this,
                                    reinterpret_cast<HMENU>(RIGHTBUTTON_ID),
                                    hResource,
                                    nullptr);
    if (m_hwndRightBtn == INVALID_HANDLE_VALUE)
        return false;
    m_hRight = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_FORWARD), iconWidth, iconHeight);
    SendMessage(m_hwndRightBtn, BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(m_hRight));
    m_hwndPlayBtn = CreateWindowEx(0,
                                   L"BUTTON",
                                   static_cast<LPCWSTR>(nullptr),
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                   0, 0, 0, 0,
                                   *this,
                                   reinterpret_cast<HMENU>(PLAYBUTTON_ID),
                                   hResource,
                                   nullptr);
    if (m_hwndPlayBtn == INVALID_HANDLE_VALUE)
        return false;
    m_hPlay = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_START), iconWidth, iconHeight);
    m_hStop = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_STOP), iconWidth, iconHeight);
    SendMessage(m_hwndPlayBtn, BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(m_hPlay));
    m_hwndAlphaToggleBtn = CreateWindowEx(0,
                                          L"BUTTON",
                                          static_cast<LPCWSTR>(nullptr),
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT | BS_NOTIFY | BS_PUSHLIKE,
                                          0, 0, 0, 0,
                                          static_cast<HWND>(*this),
                                          reinterpret_cast<HMENU>(ALPHATOGGLEBUTTON_ID),
                                          hResource,
                                          nullptr);
    if (m_hwndAlphaToggleBtn == INVALID_HANDLE_VALUE)
        return false;
    m_hAlphaToggle = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_ALPHATOGGLE), iconWidth, iconHeight);
    SendMessage(m_hwndAlphaToggleBtn, BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(m_hAlphaToggle));

    TOOLINFO ti = {0};
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd     = *this;
    ti.hinst    = hResource;
    ti.uId      = reinterpret_cast<UINT_PTR>(m_hwndAlphaToggleBtn);
    ti.lpszText = LPSTR_TEXTCALLBACK;
    // ToolTip control will cover the whole window
    ti.rect.left   = 0;
    ti.rect.top    = 0;
    ti.rect.right  = 0;
    ti.rect.bottom = 0;
    SendMessage(m_hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
    ResString sSelect(hResource, IDS_SELECT);
    m_hwndSelectBtn = CreateWindowEx(0,
                                     L"BUTTON",
                                     sSelect,
                                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     0, 0, 0, 0,
                                     *this,
                                     reinterpret_cast<HMENU>(SELECTBUTTON_ID),
                                     hResource,
                                     nullptr);
    if (m_hwndPlayBtn == INVALID_HANDLE_VALUE)
        return false;

    return true;
}

void CPicWindow::PositionChildren() const
{
    const auto headerHeight = CDPIAware::Instance().Scale(*this, HEADER_HEIGHT);
    const auto selBorder    = CDPIAware::Instance().Scale(*this, 100);
    RECT       rect;
    ::GetClientRect(*this, &rect);
    if (HasMultipleImages())
    {
        int iconWidth  = GetSystemMetrics(SM_CXSMICON);
        int iconHeight = GetSystemMetrics(SM_CYSMICON);
        SetWindowPos(m_hwndLeftBtn, HWND_TOP, rect.left + iconWidth / 4, rect.top + headerHeight + (headerHeight - iconHeight) / 2, iconWidth, iconHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        SetWindowPos(m_hwndRightBtn, HWND_TOP, rect.left + iconWidth + iconWidth / 2, rect.top + headerHeight + (headerHeight - iconHeight) / 2, iconWidth, iconHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        if (m_nFrames > 1)
            SetWindowPos(m_hwndPlayBtn, HWND_TOP, rect.left + iconWidth * 2 + iconWidth / 2, rect.top + headerHeight + (headerHeight - iconHeight) / 2, iconWidth, iconHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        else
            ShowWindow(m_hwndPlayBtn, SW_HIDE);
    }
    else
    {
        ShowWindow(m_hwndLeftBtn, SW_HIDE);
        ShowWindow(m_hwndRightBtn, SW_HIDE);
        ShowWindow(m_hwndPlayBtn, SW_HIDE);
    }
    if (m_bSelectionMode)
        SetWindowPos(m_hwndSelectBtn, HWND_TOP, rect.right - selBorder, rect.bottom - headerHeight, selBorder, headerHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    else
        ShowWindow(m_hwndSelectBtn, SW_HIDE);
    PositionTrackBar();
}

bool CPicWindow::HasMultipleImages() const
{
    return (((m_nDimensions > 1) || (m_nFrames > 1)) && (!m_pSecondPic));
}

void CPicWindow::CreateTrackbar(HWND hwndParent)
{
    m_hwndTrack = CreateWindowEx(
        0,                                                               // no extended styles
        TRACKBAR_CLASS,                                                  // class name
        L"Trackbar Control",                                             // title (caption)
        WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_TOOLTIPS | TBS_AUTOTICKS, // style
        10, 10,                                                          // position
        200, 30,                                                         // size
        hwndParent,                                                      // parent window
        reinterpret_cast<HMENU>(TRACKBAR_ID),                            // control identifier
        hInst,                                                           // instance
        nullptr                                                          // no WM_CREATE parameter
    );

    SendMessage(m_hwndTrack, TBM_SETRANGE,
                static_cast<WPARAM>(TRUE),             // redraw flag
                static_cast<LPARAM>(MAKELONG(0, 16))); // min. & max. positions
    SendMessage(m_hwndTrack, TBM_SETTIPSIDE,
                static_cast<WPARAM>(TBTS_TOP),
                static_cast<LPARAM>(0));

    m_alphaSlider.ConvertTrackbarToNice(m_hwndTrack);
}

void CPicWindow::BuildInfoString(wchar_t* buf, int size, bool bTooltip) const
{
    // Unfortunately, we need two different strings for the tooltip
    // and the info box. Because the tooltips use a different tab size
    // than ExtTextOut(), and to keep the output aligned we therefore
    // need two different strings.
    // Note: some translations could end up with two identical strings, but
    // in English we need two - even if we wouldn't need two in English, some
    // translation might then need two again.
    if (m_pSecondPic && m_pTheOtherPic)
    {
        swprintf_s(buf, size,
                   static_cast<wchar_t const*>(ResString(hResource, bTooltip ? IDS_DUALIMAGEINFOTT : IDS_DUALIMAGEINFO)),
                   m_picture.GetFileSizeAsText().c_str(), m_picture.GetFileSizeAsText(false).c_str(),
                   m_picture.m_width, m_picture.m_height,
                   m_picture.GetHorizontalResolution(), m_picture.GetVerticalResolution(),
                   m_picture.m_colorDepth,
                   static_cast<UINT>(GetZoom()),
                   m_pSecondPic->GetFileSizeAsText().c_str(), m_pSecondPic->GetFileSizeAsText(false).c_str(),
                   m_pSecondPic->m_width, m_pSecondPic->m_height,
                   m_pSecondPic->GetHorizontalResolution(), m_pSecondPic->GetVerticalResolution(),
                   m_pSecondPic->m_colorDepth,
                   static_cast<UINT>(m_pTheOtherPic->GetZoom()));
    }
    else
    {
        swprintf_s(buf, size,
                   static_cast<wchar_t const*>(ResString(hResource, bTooltip ? IDS_IMAGEINFOTT : IDS_IMAGEINFO)),
                   m_picture.GetFileSizeAsText().c_str(), m_picture.GetFileSizeAsText(false).c_str(),
                   m_picture.m_width, m_picture.m_height,
                   m_picture.GetHorizontalResolution(), m_picture.GetVerticalResolution(),
                   m_picture.m_colorDepth,
                   static_cast<UINT>(GetZoom()));
    }
}

void CPicWindow::SetZoomToWidth(long width)
{
    m_linkedWidth = width;
    if (m_picture.m_width)
    {
        int zoom = width * 100 / m_picture.m_width;
        SetZoom(zoom, false, true);
    }
}

void CPicWindow::SetZoomToHeight(long height)
{
    m_linkedHeight = height;
    if (m_picture.m_height)
    {
        int zoom = height * 100 / m_picture.m_height;
        SetZoom(zoom, false, true);
    }
}

void CPicWindow::SetTheme(bool bDark) const
{
    if (bDark)
    {
        DarkModeHelper::Instance().AllowDarkModeForWindow(*this, TRUE);
        CTheme::Instance().SetThemeForDialog(*this, true);
        SetClassLongPtr(*this, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetStockObject(BLACK_BRUSH)));
        if (FAILED(SetWindowTheme(*this, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(*this, L"Explorer", nullptr);
        DarkModeHelper::Instance().AllowDarkModeForWindow(m_hwndTrack, TRUE);
        SetClassLongPtr(m_hwndTrack, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetStockObject(BLACK_BRUSH)));
        if (FAILED(SetWindowTheme(m_hwndTrack, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(m_hwndTrack, L"Explorer", nullptr);
        if (FAILED(SetWindowTheme(m_hwndTT, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(m_hwndTT, L"Explorer", nullptr);

        if (FAILED(SetWindowTheme(m_hwndLeftBtn, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(m_hwndLeftBtn, L"Explorer", nullptr);
        if (FAILED(SetWindowTheme(m_hwndRightBtn, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(m_hwndRightBtn, L"Explorer", nullptr);
        if (FAILED(SetWindowTheme(m_hwndPlayBtn, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(m_hwndPlayBtn, L"Explorer", nullptr);
        if (FAILED(SetWindowTheme(m_hwndSelectBtn, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(m_hwndSelectBtn, L"Explorer", nullptr);
        if (FAILED(SetWindowTheme(m_hwndAlphaToggleBtn, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(m_hwndAlphaToggleBtn, L"Explorer", nullptr);
    }
    else
    {
        DarkModeHelper::Instance().AllowDarkModeForWindow(*this, FALSE);
        CTheme::Instance().SetThemeForDialog(*this, false);
        SetClassLongPtr(*this, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
        SetWindowTheme(*this, L"Explorer", nullptr);
        DarkModeHelper::Instance().AllowDarkModeForWindow(m_hwndTrack, FALSE);
        SetClassLongPtr(m_hwndTrack, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
        SetWindowTheme(m_hwndTrack, L"Explorer", nullptr);
        SetWindowTheme(m_hwndTT, L"Explorer", nullptr);

        SetWindowTheme(m_hwndLeftBtn, L"Explorer", nullptr);
        SetWindowTheme(m_hwndRightBtn, L"Explorer", nullptr);
        SetWindowTheme(m_hwndPlayBtn, L"Explorer", nullptr);
        SetWindowTheme(m_hwndSelectBtn, L"Explorer", nullptr);
        SetWindowTheme(m_hwndAlphaToggleBtn, L"Explorer", nullptr);
    }

    InvalidateRect(*this, nullptr, true);
}

COLORREF CPicWindow::GetTransparentThemedColor() const
{
    if (CTheme::Instance().IsDarkTheme())
    {
        if (m_transparentColor == 0xFFFFFF)
            return CTheme::darkBkColor;
    }
    return CTheme::Instance().GetThemeColor(m_transparentColor);
}
