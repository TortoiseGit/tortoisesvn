// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2015, 2018, 2020-2021 - TortoiseSVN
// Copyright (C) 2015-2016 - TortoiseGit

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
#include <CommCtrl.h>
#include "TortoiseIDiff.h"
#include "resource.h"
#include "MainWindow.h"
#include "AboutDlg.h"
#include "TaskbarUUID.h"
#include "DPIAware.h"
#include "LoadIconEx.h"
#include "registry.h"
#include "Theme.h"
#include "DarkModeHelper.h"
#include "ResString.h"

#pragma comment(lib, "comctl32.lib")

std::wstring CMainWindow::m_sLeftPicPath;
std::wstring CMainWindow::m_sLeftPicTitle;

std::wstring CMainWindow::m_sRightPicPath;
std::wstring CMainWindow::m_sRightPicTitle;

const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

CMainWindow::CMainWindow(HINSTANCE hInstance, const WNDCLASSEX* wcx)
    : CWindow(hInstance, wcx)
    , m_themeCallbackId(0)
    , m_hwndTb(nullptr)
    , m_hToolbarImgList(nullptr)
    , m_picWindow1(hInstance)
    , m_picWindow2(hInstance)
    , m_picWindow3(hInstance)
    , m_bShowInfo(false)
    , m_transparentColor(::GetSysColor(COLOR_WINDOW))
    , m_oldX(-4)
    , m_oldY(-4)
    , m_bMoved(false)
    , m_bDragMode(false)
    , m_bDrag2(false)
    , m_nSplitterPos(100)
    , m_nSplitterPos2(200)
    , m_bSelectionMode(false)
    , m_bOverlap(false)
    , m_bVertical(false)
    , m_bLinkedPositions(true)
    , m_bFitWidths(false)
    , m_bFitHeights(false)
    , m_blendType(CPicWindow::BLEND_ALPHA)
{
    SetWindowTitle(static_cast<LPCWSTR>(ResString(hResource, IDS_APP_TITLE)));
}

bool CMainWindow::RegisterAndCreateWindow()
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with default parameters
    wcx.cbSize      = sizeof(WNDCLASSEX);
    wcx.style       = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = CWindow::stWinMsgHandler;
    wcx.cbClsExtra  = 0;
    wcx.cbWndExtra  = 0;
    wcx.hInstance   = hResource;
    wcx.hCursor     = LoadCursor(nullptr, IDC_SIZEWE);
    ResString clsName(hResource, IDS_APP_TITLE);
    wcx.lpszClassName = clsName;
    wcx.hIcon         = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_TORTOISEIDIFF), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcx.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_3DFACE + 1));
    if (m_selectionPaths.empty())
        wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
    else
        wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF2);
    wcx.hIconSm = LoadIconEx(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
    if (RegisterWindow(&wcx))
    {
        if (Create(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, nullptr))
        {
            UpdateWindow(m_hwnd);
            return true;
        }
    }
    return false;
}

void CMainWindow::PositionChildren(RECT* clientrect /* = nullptr */)
{
    RECT tbRect;
    if (!clientrect)
        return;
    const auto splitterBorder = CDPIAware::Instance().Scale(*this, SPLITTER_BORDER);
    SendMessage(m_hwndTb, TB_AUTOSIZE, 0, 0);
    GetWindowRect(m_hwndTb, &tbRect);
    LONG tbHeight = tbRect.bottom - tbRect.top - 1;
    HDWP hdwp     = BeginDeferWindowPos(3);
    if (m_bOverlap && m_selectionPaths.empty())
    {
        SetWindowPos(m_picWindow1, nullptr, clientrect->left, clientrect->top + tbHeight, clientrect->right - clientrect->left, clientrect->bottom - clientrect->top - tbHeight, SWP_SHOWWINDOW);
    }
    else
    {
        if (m_bVertical)
        {
            if (m_selectionPaths.size() != 3)
            {
                // two image windows
                RECT child;
                child.left   = clientrect->left;
                child.top    = clientrect->top + tbHeight;
                child.right  = clientrect->right;
                child.bottom = m_nSplitterPos - (splitterBorder / 2);
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow1, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.top    = m_nSplitterPos + (splitterBorder / 2);
                child.bottom = clientrect->bottom;
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow2, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
            else
            {
                // three image windows
                RECT child;
                child.left   = clientrect->left;
                child.top    = clientrect->top + tbHeight;
                child.right  = clientrect->right;
                child.bottom = m_nSplitterPos - (splitterBorder / 2);
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow1, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.top    = m_nSplitterPos + (splitterBorder / 2);
                child.bottom = m_nSplitterPos2 - (splitterBorder / 2);
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow2, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.top    = m_nSplitterPos2 + (splitterBorder / 2);
                child.bottom = clientrect->bottom;
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow3, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
        }
        else
        {
            if (m_selectionPaths.size() != 3)
            {
                // two image windows
                RECT child;
                child.left   = clientrect->left;
                child.top    = clientrect->top + tbHeight;
                child.right  = m_nSplitterPos - (splitterBorder / 2);
                child.bottom = clientrect->bottom;
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow1, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.left  = m_nSplitterPos + (splitterBorder / 2);
                child.right = clientrect->right;
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow2, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
            else
            {
                // three image windows
                RECT child;
                child.left   = clientrect->left;
                child.top    = clientrect->top + tbHeight;
                child.right  = m_nSplitterPos - (splitterBorder / 2);
                child.bottom = clientrect->bottom;
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow1, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.left  = m_nSplitterPos + (splitterBorder / 2);
                child.right = m_nSplitterPos2 - (splitterBorder / 2);
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow2, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.left  = m_nSplitterPos2 + (splitterBorder / 2);
                child.right = clientrect->right;
                if (hdwp)
                    hdwp = DeferWindowPos(hdwp, m_picWindow3, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
        }
    }
    if (hdwp)
        EndDeferWindowPos(hdwp);
    m_picWindow1.SetTransparentColor(m_transparentColor);
    m_picWindow2.SetTransparentColor(m_transparentColor);
    m_picWindow3.SetTransparentColor(m_transparentColor);
    InvalidateRect(*this, nullptr, FALSE);
}

LRESULT CALLBACK CMainWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == TaskBarButtonCreated)
    {
        setUuidOverlayIcon(hwnd);
    }
    switch (uMsg)
    {
        case WM_CREATE:
        {
            m_hwnd = hwnd;
            m_picWindow1.RegisterAndCreateWindow(hwnd);
            m_picWindow2.RegisterAndCreateWindow(hwnd);
            if (m_selectionPaths.empty())
            {
                m_picWindow1.SetPic(m_sLeftPicPath, m_sLeftPicTitle, true);
                m_picWindow2.SetPic(m_sRightPicPath, m_sRightPicTitle, false);

                m_picWindow1.SetOtherPicWindow(&m_picWindow2);
                m_picWindow2.SetOtherPicWindow(&m_picWindow1);
            }
            else
            {
                m_picWindow3.RegisterAndCreateWindow(hwnd);

                m_picWindow1.SetPic(m_selectionPaths[FileTypeMine], m_selectionTitles[FileTypeMine], false);
                m_picWindow2.SetPic(m_selectionPaths[FileTypeBase], m_selectionTitles[FileTypeBase], false);
                m_picWindow3.SetPic(m_selectionPaths[FileTypeTheirs], m_selectionTitles[FileTypeTheirs], false);
            }

            m_picWindow1.SetSelectionMode(!m_selectionPaths.empty());
            m_picWindow2.SetSelectionMode(!m_selectionPaths.empty());
            m_picWindow3.SetSelectionMode(!m_selectionPaths.empty());

            CreateToolbar();
            // center the splitter
            RECT rect;
            GetClientRect(hwnd, &rect);
            if (m_selectionPaths.size() != 3)
            {
                m_nSplitterPos  = (rect.right - rect.left) / 2;
                m_nSplitterPos2 = 0;
            }
            else
            {
                m_nSplitterPos  = (rect.right - rect.left) / 3;
                m_nSplitterPos2 = (rect.right - rect.left) * 2 / 3;
            }

            PositionChildren(&rect);
            m_picWindow1.FitImageInWindow();
            m_picWindow2.FitImageInWindow();
            m_picWindow3.FitImageInWindow();

            m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
                [this]() {
                    SetTheme(CTheme::Instance().IsDarkTheme());
                });
            SetTheme(CTheme::Instance().IsDarkTheme());
        }
        break;
        case WM_COMMAND:
        {
            return DoCommand(LOWORD(wParam), lParam);
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            RECT        rect{};

            ::GetClientRect(*this, &rect);
            HDC hdc = BeginPaint(hwnd, &ps);
            SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
            ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);
            EndPaint(hwnd, &ps);
        }
        break;
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi       = reinterpret_cast<MINMAXINFO*>(lParam);
            mmi->ptMinTrackSize.x = WINDOW_MINWIDTH;
            mmi->ptMinTrackSize.y = WINDOW_MINHEIGHT;
            return 0;
        }
        case WM_SIZE:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            if (m_bVertical)
            {
                RECT tbRect;
                GetWindowRect(m_hwndTb, &tbRect);
                LONG tbHeight = tbRect.bottom - tbRect.top - 1;
                if (m_selectionPaths.size() != 3)
                {
                    m_nSplitterPos  = (rect.bottom - rect.top) / 2 + tbHeight;
                    m_nSplitterPos2 = 0;
                }
                else
                {
                    m_nSplitterPos  = (rect.bottom - rect.top) / 3 + tbHeight;
                    m_nSplitterPos2 = (rect.bottom - rect.top) * 2 / 3 + tbHeight;
                }
            }
            else
            {
                if (m_selectionPaths.size() != 3)
                {
                    m_nSplitterPos  = (rect.right - rect.left) / 2;
                    m_nSplitterPos2 = 0;
                }
                else
                {
                    m_nSplitterPos  = (rect.right - rect.left) / 3;
                    m_nSplitterPos2 = (rect.right - rect.left) * 2 / 3;
                }
            }
            PositionChildren(&rect);
        }
        break;
        case WM_SETCURSOR:
        {
            if (reinterpret_cast<HWND>(wParam) == *this)
            {
                RECT  rect;
                POINT pt;
                GetClientRect(*this, &rect);
                GetCursorPos(&pt);
                ScreenToClient(*this, &pt);
                if (PtInRect(&rect, pt))
                {
                    if (m_bVertical)
                    {
                        HCURSOR hCur = LoadCursor(nullptr, IDC_SIZENS);
                        SetCursor(hCur);
                    }
                    else
                    {
                        HCURSOR hCur = LoadCursor(nullptr, IDC_SIZEWE);
                        SetCursor(hCur);
                    }
                    return TRUE;
                }
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        case WM_LBUTTONDOWN:
            Splitter_OnLButtonDown(hwnd, uMsg, wParam, lParam);
            break;
        case WM_LBUTTONUP:
            Splitter_OnLButtonUp(hwnd, uMsg, wParam, lParam);
            break;
        case WM_CAPTURECHANGED:
            Splitter_CaptureChanged();
            break;
        case WM_MOUSEMOVE:
            Splitter_OnMouseMove(hwnd, uMsg, wParam, lParam);
            break;
        case WM_MOUSEWHEEL:
        {
            // find out if the mouse cursor is over one of the views, and if
            // it is, pass the mouse wheel message to that view
            POINT pt;
            DWORD ptW = GetMessagePos();
            pt.x      = GET_X_LPARAM(ptW);
            pt.y      = GET_Y_LPARAM(ptW);
            RECT rect;
            GetWindowRect(m_picWindow1, &rect);
            if (PtInRect(&rect, pt))
            {
                m_picWindow1.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
            }
            else
            {
                GetWindowRect(m_picWindow2, &rect);
                if (PtInRect(&rect, pt))
                {
                    m_picWindow2.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
                }
                else
                {
                    GetWindowRect(m_picWindow3, &rect);
                    if (PtInRect(&rect, pt))
                    {
                        m_picWindow3.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
                    }
                }
            }
        }
        break;
        case WM_MOUSEHWHEEL:
        {
            // find out if the mouse cursor is over one of the views, and if
            // it is, pass the mouse wheel message to that view
            POINT pt;
            DWORD ptW = GetMessagePos();
            pt.x      = GET_X_LPARAM(ptW);
            pt.y      = GET_Y_LPARAM(ptW);
            RECT rect;
            GetWindowRect(m_picWindow1, &rect);
            if (PtInRect(&rect, pt))
            {
                m_picWindow1.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam) | MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
            }
            else
            {
                GetWindowRect(m_picWindow2, &rect);
                if (PtInRect(&rect, pt))
                {
                    m_picWindow2.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam) | MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
                }
                else
                {
                    GetWindowRect(m_picWindow3, &rect);
                    if (PtInRect(&rect, pt))
                    {
                        m_picWindow3.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam) | MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
                    }
                }
            }
        }
        break;
        case WM_NOTIFY:
        {
            LPNMHDR pNMHDR = reinterpret_cast<LPNMHDR>(lParam);
            if (pNMHDR->code == TTN_GETDISPINFO)
            {
                LPTOOLTIPTEXT lpttt = reinterpret_cast<LPNMTTDISPINFOW>(lParam);
                lpttt->hinst        = hResource;

                // Specify the resource identifier of the descriptive
                // text for the given button.
                wchar_t      stringBuf[MAX_PATH] = {0};
                MENUITEMINFO mii;
                mii.cbSize     = sizeof(MENUITEMINFO);
                mii.fMask      = MIIM_TYPE;
                mii.dwTypeData = stringBuf;
                mii.cch        = _countof(stringBuf);
                GetMenuItemInfo(GetMenu(*this), static_cast<UINT>(lpttt->hdr.idFrom), FALSE, &mii);
                wcscpy_s(lpttt->lpszText, 80, stringBuf);
            }
        }
        break;
        case WM_DESTROY:
            bWindowClosed = TRUE;
            PostQuitMessage(0);
            break;
        case WM_CLOSE:
            CTheme::Instance().RemoveRegisteredCallback(m_themeCallbackId);
            m_themeCallbackId = 0;
            ImageList_Destroy(m_hToolbarImgList);
            ::DestroyWindow(m_hwnd);
            break;
        case WM_SYSCOLORCHANGE:
            CTheme::Instance().OnSysColorChanged();
            CTheme::Instance().SetDarkTheme(CTheme::Instance().IsDarkTheme(), true);
            break;
        case WM_DPICHANGED:
        {
            CDPIAware::Instance().Invalidate();
            const RECT* rect = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(*this, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            ::RedrawWindow(*this, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
};

LRESULT CMainWindow::DoCommand(int id, LPARAM lParam)
{
    switch (id)
    {
        case ID_FILE_OPEN:
        {
            if (OpenDialog())
            {
                m_picWindow1.SetPic(m_sLeftPicPath, L"", true);
                m_picWindow2.SetPic(m_sRightPicPath, L"", false);
                if (m_bOverlap)
                {
                    m_picWindow1.SetSecondPic(m_picWindow2.GetPic(), m_sRightPicTitle, m_sRightPicPath);
                }
                else
                {
                    m_picWindow1.SetSecondPic();
                }
                RECT rect;
                GetClientRect(*this, &rect);
                PositionChildren(&rect);
                m_picWindow1.FitImageInWindow();
                m_picWindow2.FitImageInWindow();
            }
        }
        break;
        case ID_VIEW_IMAGEINFO:
        {
            m_bShowInfo  = !m_bShowInfo;
            HMENU hMenu  = GetMenu(*this);
            UINT  uCheck = MF_BYCOMMAND;
            uCheck |= m_bShowInfo ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_IMAGEINFO, uCheck);

            m_picWindow1.ShowInfo(m_bShowInfo);
            m_picWindow2.ShowInfo(m_bShowInfo);
            m_picWindow3.ShowInfo(m_bShowInfo);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize  = sizeof(TBBUTTONINFO);
            tbi.dwMask  = TBIF_STATE;
            tbi.fsState = m_bShowInfo ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_IMAGEINFO, reinterpret_cast<LPARAM>(&tbi));
        }
        break;
        case ID_VIEW_OVERLAPIMAGES:
        {
            m_bOverlap   = !m_bOverlap;
            HMENU hMenu  = GetMenu(*this);
            UINT  uCheck = MF_BYCOMMAND;
            uCheck |= m_bOverlap ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_OVERLAPIMAGES, uCheck);
            uCheck |= ((m_blendType == CPicWindow::BLEND_ALPHA) && m_bOverlap) ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_BLENDALPHA, uCheck);
            UINT uEnabled = MF_BYCOMMAND;
            uEnabled |= m_bOverlap ? MF_ENABLED : MF_DISABLED | MF_GRAYED;
            EnableMenuItem(hMenu, ID_VIEW_BLENDALPHA, uEnabled);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize  = sizeof(TBBUTTONINFO);
            tbi.dwMask  = TBIF_STATE;
            tbi.fsState = m_bOverlap ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_OVERLAPIMAGES, reinterpret_cast<LPARAM>(&tbi));

            tbi.fsState = ((m_blendType == CPicWindow::BLEND_ALPHA) && m_bOverlap) ? TBSTATE_CHECKED : 0;
            if (m_bOverlap)
                tbi.fsState |= TBSTATE_ENABLED;
            else
                tbi.fsState = 0;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_BLENDALPHA, reinterpret_cast<LPARAM>(&tbi));

            if (m_bOverlap)
                tbi.fsState = 0;
            else
                tbi.fsState = m_bVertical ? TBSTATE_ENABLED | TBSTATE_CHECKED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_ARRANGEVERTICAL, reinterpret_cast<LPARAM>(&tbi));

            if (m_bOverlap)
            {
                m_bLinkedPositions = true;
                m_picWindow1.LinkPositions(m_bLinkedPositions);
                m_picWindow2.LinkPositions(m_bLinkedPositions);
                tbi.fsState = TBSTATE_CHECKED;
            }
            else
                tbi.fsState = m_bLinkedPositions ? TBSTATE_ENABLED | TBSTATE_CHECKED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_LINKIMAGESTOGETHER, reinterpret_cast<LPARAM>(&tbi));

            ShowWindow(m_picWindow2, m_bOverlap ? SW_HIDE : SW_SHOW);

            if (m_bOverlap)
            {
                m_picWindow1.StopTimer();
                m_picWindow2.StopTimer();
                m_picWindow1.SetSecondPic(m_picWindow2.GetPic(), m_sRightPicTitle, m_sRightPicPath,
                                          m_picWindow2.GetHPos(), m_picWindow2.GetVPos());
                m_picWindow1.SetBlendAlpha(m_blendType, 0.5f);
            }
            else
            {
                m_picWindow1.SetSecondPic();
            }
            m_picWindow1.SetOverlapMode(m_bOverlap);
            m_picWindow2.SetOverlapMode(m_bOverlap);

            RECT rect;
            GetClientRect(*this, &rect);
            PositionChildren(&rect);

            return 0;
        }
        case ID_VIEW_BLENDALPHA:
        {
            if (m_blendType == CPicWindow::BLEND_ALPHA)
                m_blendType = CPicWindow::BLEND_XOR;
            else
                m_blendType = CPicWindow::BLEND_ALPHA;

            HMENU hMenu  = GetMenu(*this);
            UINT  uCheck = MF_BYCOMMAND;
            uCheck |= ((m_blendType == CPicWindow::BLEND_ALPHA) && m_bOverlap) ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_BLENDALPHA, uCheck);
            UINT uEnabled = MF_BYCOMMAND;
            uEnabled |= m_bOverlap ? MF_ENABLED : MF_DISABLED | MF_GRAYED;
            EnableMenuItem(hMenu, ID_VIEW_BLENDALPHA, uEnabled);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize  = sizeof(TBBUTTONINFO);
            tbi.dwMask  = TBIF_STATE;
            tbi.fsState = ((m_blendType == CPicWindow::BLEND_ALPHA) && m_bOverlap) ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_BLENDALPHA, reinterpret_cast<LPARAM>(&tbi));
            m_picWindow1.SetBlendAlpha(m_blendType, m_picWindow1.GetBlendAlpha());
            PositionChildren();
        }
        break;
        case ID_VIEW_TRANSPARENTCOLOR:
        {
            static COLORREF customColors[16] = {0};
            CHOOSECOLOR     ccDlg            = {0};
            ccDlg.lStructSize                = sizeof(ccDlg);
            ccDlg.hwndOwner                  = m_hwnd;
            ccDlg.rgbResult                  = m_transparentColor;
            ccDlg.lpCustColors               = customColors;
            ccDlg.Flags                      = CC_RGBINIT | CC_FULLOPEN;
            if (ChooseColor(&ccDlg))
            {
                m_transparentColor = ccDlg.rgbResult;
                m_picWindow1.SetTransparentColor(m_transparentColor);
                m_picWindow2.SetTransparentColor(m_transparentColor);
                m_picWindow3.SetTransparentColor(m_transparentColor);
                // The color picker takes the focus and we don't get it back.
                ::SetFocus(m_picWindow1);
            }
        }
        break;
        case ID_VIEW_FITIMAGEWIDTHS:
        {
            m_bFitWidths = !m_bFitWidths;
            m_picWindow1.FitWidths(m_bFitWidths);
            m_picWindow2.FitWidths(m_bFitWidths);
            m_picWindow3.FitWidths(m_bFitWidths);

            HMENU hMenu  = GetMenu(*this);
            UINT  uCheck = MF_BYCOMMAND;
            uCheck |= m_bFitWidths ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_FITIMAGEWIDTHS, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize  = sizeof(TBBUTTONINFO);
            tbi.dwMask  = TBIF_STATE;
            tbi.fsState = m_bFitWidths ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_FITIMAGEWIDTHS, reinterpret_cast<LPARAM>(&tbi));
        }
        break;
        case ID_VIEW_FITIMAGEHEIGHTS:
        {
            m_bFitHeights = !m_bFitHeights;
            m_picWindow1.FitHeights(m_bFitHeights);
            m_picWindow2.FitHeights(m_bFitHeights);
            m_picWindow3.FitHeights(m_bFitHeights);

            HMENU hMenu  = GetMenu(*this);
            UINT  uCheck = MF_BYCOMMAND;
            uCheck |= m_bFitHeights ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_FITIMAGEHEIGHTS, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize  = sizeof(TBBUTTONINFO);
            tbi.dwMask  = TBIF_STATE;
            tbi.fsState = m_bFitHeights ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_FITIMAGEHEIGHTS, reinterpret_cast<LPARAM>(&tbi));
        }
        break;
        case ID_VIEW_LINKIMAGESTOGETHER:
        {
            m_bLinkedPositions = !m_bLinkedPositions;
            m_picWindow1.LinkPositions(m_bLinkedPositions);
            m_picWindow2.LinkPositions(m_bLinkedPositions);
            m_picWindow3.LinkPositions(m_bLinkedPositions);

            HMENU hMenu  = GetMenu(*this);
            UINT  uCheck = MF_BYCOMMAND;
            uCheck |= m_bLinkedPositions ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_LINKIMAGESTOGETHER, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize  = sizeof(TBBUTTONINFO);
            tbi.dwMask  = TBIF_STATE;
            tbi.fsState = m_bLinkedPositions ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_LINKIMAGESTOGETHER, reinterpret_cast<LPARAM>(&tbi));
        }
        break;
        case ID_VIEW_ALPHA0:
            m_picWindow1.SetBlendAlpha(m_blendType, 0.0f);
            break;
        case ID_VIEW_ALPHA255:
            m_picWindow1.SetBlendAlpha(m_blendType, 1.0f);
            break;
        case ID_VIEW_ALPHA127:
            m_picWindow1.SetBlendAlpha(m_blendType, 0.5f);
            break;
        case ID_VIEW_ALPHATOGGLE:
            m_picWindow1.ToggleAlpha();
            break;
        case ID_VIEW_FITIMAGESINWINDOW:
        {
            m_picWindow1.FitImageInWindow();
            m_picWindow2.FitImageInWindow();
            m_picWindow3.FitImageInWindow();
        }
        break;
        case ID_VIEW_ORININALSIZE:
        {
            m_picWindow1.SetZoom(100, false);
            m_picWindow2.SetZoom(100, false);
            m_picWindow3.SetZoom(100, false);
            m_picWindow1.CenterImage();
            m_picWindow2.CenterImage();
            m_picWindow3.CenterImage();
        }
        break;
        case ID_VIEW_ZOOMIN:
        {
            m_picWindow1.Zoom(true, false);
            if ((!(m_bFitWidths || m_bFitHeights)) && (!m_bOverlap))
            {
                m_picWindow2.Zoom(true, false);
                m_picWindow3.Zoom(true, false);
            }
        }
        break;
        case ID_VIEW_ZOOMOUT:
        {
            m_picWindow1.Zoom(false, false);
            if ((!(m_bFitWidths || m_bFitHeights)) && (!m_bOverlap))
            {
                m_picWindow2.Zoom(false, false);
                m_picWindow3.Zoom(false, false);
            }
        }
        break;
        case ID_VIEW_ARRANGEVERTICAL:
        {
            m_bVertical = !m_bVertical;
            RECT rect;
            GetClientRect(*this, &rect);
            if (m_bVertical)
            {
                RECT tbRect;
                GetWindowRect(m_hwndTb, &tbRect);
                LONG tbHeight = tbRect.bottom - tbRect.top - 1;
                if (m_selectionPaths.size() != 3)
                {
                    m_nSplitterPos  = (rect.bottom - rect.top) / 2 + tbHeight;
                    m_nSplitterPos2 = 0;
                }
                else
                {
                    m_nSplitterPos  = (rect.bottom - rect.top) / 3 + tbHeight;
                    m_nSplitterPos2 = (rect.bottom - rect.top) * 2 / 3 + tbHeight;
                }
            }
            else
            {
                if (m_selectionPaths.size() != 3)
                {
                    m_nSplitterPos  = (rect.right - rect.left) / 2;
                    m_nSplitterPos2 = 0;
                }
                else
                {
                    m_nSplitterPos  = (rect.right - rect.left) / 3;
                    m_nSplitterPos2 = (rect.right - rect.left) * 2 / 3;
                }
            }
            HMENU hMenu  = GetMenu(*this);
            UINT  uCheck = MF_BYCOMMAND;
            uCheck |= m_bVertical ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_ARRANGEVERTICAL, uCheck);
            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize  = sizeof(TBBUTTONINFO);
            tbi.dwMask  = TBIF_STATE;
            tbi.fsState = m_bVertical ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(m_hwndTb, TB_SETBUTTONINFO, ID_VIEW_ARRANGEVERTICAL, reinterpret_cast<LPARAM>(&tbi));

            PositionChildren(&rect);
        }
        break;
        case ID_ABOUT:
        {
            CAboutDlg dlg(*this);
            dlg.DoModal(hInst, IDD_ABOUT, *this);
        }
        break;
        case SELECTBUTTON_ID:
        {
            HWND hSource = reinterpret_cast<HWND>(lParam);
            if (m_picWindow1 == hSource)
            {
                if (!m_selectionResult.empty())
                    CopyFile(m_selectionPaths[FileTypeMine].c_str(), m_selectionResult.c_str(), FALSE);
                PostQuitMessage(FileTypeMine);
            }
            if (m_picWindow2 == hSource)
            {
                if (!m_selectionResult.empty())
                    CopyFile(m_selectionPaths[FileTypeBase].c_str(), m_selectionResult.c_str(), FALSE);
                PostQuitMessage(FileTypeBase);
            }
            if (m_picWindow3 == hSource)
            {
                if (!m_selectionResult.empty())
                    CopyFile(m_selectionPaths[FileTypeTheirs].c_str(), m_selectionResult.c_str(), FALSE);
                PostQuitMessage(FileTypeTheirs);
            }
        }
        break;
        case ID_VIEW_DARKMODE:
        {
            CTheme::Instance().SetDarkTheme(!CTheme::Instance().IsDarkTheme());
        }
        break;
        case IDM_EXIT:
            ::PostQuitMessage(0);
            return 0;
        default:
            break;
    };
    return 1;
}

// splitter stuff
void CMainWindow::DrawXorBar(HDC hdc, int x1, int y1, int width, int height)
{
    static WORD dotPatternBmp[8] =
        {
            0x0055, 0x00aa, 0x0055, 0x00aa,
            0x0055, 0x00aa, 0x0055, 0x00aa};

    HBITMAP hbm = CreateBitmap(8, 8, 1, 1, dotPatternBmp);
    HBRUSH  hbr = CreatePatternBrush(hbm);

    SetBrushOrgEx(hdc, x1, y1, nullptr);
    HBRUSH hBrushOld = static_cast<HBRUSH>(SelectObject(hdc, hbr));

    PatBlt(hdc, x1, y1, width, height, PATINVERT);

    SelectObject(hdc, hBrushOld);

    DeleteObject(hbr);
    DeleteObject(hbm);
}

LRESULT CMainWindow::Splitter_OnLButtonDown(HWND hwnd, UINT /*iMsg*/, WPARAM /*wParam*/, LPARAM lParam)
{
    POINT pt;
    RECT  rect;
    RECT  clientRect;

    pt.x = static_cast<short>(LOWORD(lParam)); // horizontal position of cursor
    pt.y = static_cast<short>(HIWORD(lParam));

    GetClientRect(hwnd, &clientRect);
    GetWindowRect(hwnd, &rect);
    POINT zero = {0, 0};
    ClientToScreen(hwnd, &zero);
    OffsetRect(&clientRect, zero.x - rect.left, zero.y - rect.top);

    ClientToScreen(hwnd, &pt);
    // find out which drag bar is used
    m_bDrag2 = false;
    if (!m_selectionPaths.empty())
    {
        RECT pic2Rect;
        GetWindowRect(m_picWindow2, &pic2Rect);
        if (m_bVertical)
        {
            if (pic2Rect.bottom <= pt.y)
                m_bDrag2 = true;
        }
        else
        {
            if (pic2Rect.right <= pt.x)
                m_bDrag2 = true;
        }
    }

    //convert the mouse coordinates relative to the top-left of
    //the window
    pt.x -= rect.left;
    pt.y -= rect.top;

    //same for the window coordinates - make them relative to 0,0
    OffsetRect(&rect, -rect.left, -rect.top);

    if (pt.x < 0)
        pt.x = 0;
    if (pt.x > rect.right - 4)
        pt.x = rect.right - 4;
    if (pt.y < 0)
        pt.y = 0;
    if (pt.y > rect.bottom - 4)
        pt.y = rect.bottom - 4;

    m_bDragMode = true;

    SetCapture(hwnd);

    HDC hdc = GetWindowDC(hwnd);
    if (m_bVertical)
        DrawXorBar(hdc, clientRect.left, pt.y + 2, clientRect.right - clientRect.left - 2, 4);
    else
        DrawXorBar(hdc, pt.x + 2, clientRect.top, 4, clientRect.bottom - clientRect.top - 2);
    ReleaseDC(hwnd, hdc);

    m_oldX = pt.x;
    m_oldY = pt.y;

    return 0;
}

void CMainWindow::Splitter_CaptureChanged()
{
    m_bDragMode = false;
}

void CMainWindow::SetTheme(bool bDark)
{
    m_transparentColor = ::GetSysColor(COLOR_WINDOW);
    m_picWindow1.SetTransparentColor(m_transparentColor);
    m_picWindow2.SetTransparentColor(m_transparentColor);
    m_picWindow3.SetTransparentColor(m_transparentColor);
    if (bDark)
    {
        DarkModeHelper::Instance().AllowDarkModeForApp(TRUE);

        DarkModeHelper::Instance().AllowDarkModeForWindow(*this, TRUE);
        SetClassLongPtr(*this, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetStockObject(BLACK_BRUSH)));
        if (FAILED(SetWindowTheme(*this, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(*this, L"Explorer", nullptr);
        DarkModeHelper::Instance().AllowDarkModeForWindow(m_hwndTb, TRUE);
        //SetClassLongPtr(hwndTB, GCLP_HBRBACKGROUND, (LONG_PTR)GetStockObject(BLACK_BRUSH));
        if (FAILED(SetWindowTheme(m_hwndTb, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(m_hwndTb, L"Explorer", nullptr);
        BOOL                                        darkFlag = TRUE;
        DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data     = {DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag)};
        DarkModeHelper::Instance().SetWindowCompositionAttribute(*this, &data);
        DarkModeHelper::Instance().FlushMenuThemes();
        DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
    }
    else
    {
        SetClassLongPtr(*this, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
        SetWindowTheme(*this, L"Explorer", nullptr);
        //SetClassLongPtr(hwndTB, GCLP_HBRBACKGROUND, (LONG_PTR)GetSysColorBrush(COLOR_3DFACE));
        SetWindowTheme(m_hwndTb, L"Explorer", nullptr);
        DarkModeHelper::Instance().AllowDarkModeForWindow(*this, FALSE);
        DarkModeHelper::Instance().AllowDarkModeForWindow(m_hwndTb, FALSE);
        BOOL                                        darkFlag = FALSE;
        DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data     = {DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag)};
        DarkModeHelper::Instance().SetWindowCompositionAttribute(*this, &data);
        DarkModeHelper::Instance().FlushMenuThemes();
        DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
        DarkModeHelper::Instance().AllowDarkModeForApp(FALSE);
    }
    DarkModeHelper::Instance().RefreshTitleBarThemeColor(*this, bDark);

    HMENU hMenu  = GetMenu(*this);
    UINT  uCheck = MF_BYCOMMAND;
    uCheck |= CTheme::Instance().IsDarkTheme() ? MF_CHECKED : MF_UNCHECKED;
    CheckMenuItem(hMenu, ID_VIEW_DARKMODE, uCheck);
    UINT uEnabled = MF_BYCOMMAND;
    uEnabled |= CTheme::Instance().IsDarkModeAllowed() ? MF_ENABLED : MF_DISABLED;
    EnableMenuItem(hMenu, ID_VIEW_DARKMODE, uEnabled);

    ::RedrawWindow(*this, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

LRESULT CMainWindow::Splitter_OnLButtonUp(HWND hwnd, UINT /*iMsg*/, WPARAM /*wParam*/, LPARAM lParam)
{
    RECT rect;
    RECT clientRect;

    POINT pt;
    pt.x = static_cast<short>(LOWORD(lParam)); // horizontal position of cursor
    pt.y = static_cast<short>(HIWORD(lParam));

    if (m_bDragMode == FALSE)
        return 0;

    const auto borderSm = CDPIAware::Instance().Scale(*this, 2);
    const auto borderL  = CDPIAware::Instance().Scale(*this, 4);

    GetClientRect(hwnd, &clientRect);
    GetWindowRect(hwnd, &rect);
    POINT zero = {0, 0};
    ClientToScreen(hwnd, &zero);
    OffsetRect(&clientRect, zero.x - rect.left, zero.y - rect.top);

    ClientToScreen(hwnd, &pt);
    pt.x -= rect.left;
    pt.y -= rect.top;

    OffsetRect(&rect, -rect.left, -rect.top);

    if (pt.x < 0)
        pt.x = 0;
    if (pt.x > rect.right - borderL)
        pt.x = rect.right - borderL;
    if (pt.y < 0)
        pt.y = 0;
    if (pt.y > rect.bottom - borderL)
        pt.y = rect.bottom - borderL;

    HDC hdc = GetWindowDC(hwnd);
    if (m_bVertical)
        DrawXorBar(hdc, clientRect.left, m_oldY + borderSm, clientRect.right - clientRect.left - borderSm, borderL);
    else
        DrawXorBar(hdc, m_oldX + borderSm, clientRect.top, borderL, clientRect.bottom - clientRect.top - borderSm);
    ReleaseDC(hwnd, hdc);

    m_oldX = pt.x;
    m_oldY = pt.y;

    m_bDragMode = false;

    //convert the splitter position back to screen coords.
    GetWindowRect(hwnd, &rect);
    pt.x += rect.left;
    pt.y += rect.top;

    //now convert into CLIENT coordinates
    ScreenToClient(hwnd, &pt);
    GetClientRect(hwnd, &rect);
#define MINWINSIZE 10
    if (m_bVertical)
    {
        if (m_selectionPaths.size() != 3)
        {
            m_nSplitterPos = pt.y;
        }
        else
        {
            if (m_bDrag2)
            {
                if (pt.y < (m_nSplitterPos + MINWINSIZE))
                    pt.y = m_nSplitterPos + MINWINSIZE;
                m_nSplitterPos2 = pt.y;
            }
            else
            {
                if (pt.y > (m_nSplitterPos2 - MINWINSIZE))
                    pt.y = m_nSplitterPos2 - MINWINSIZE;
                m_nSplitterPos = pt.y;
            }
        }
    }
    else
    {
        if (m_selectionPaths.size() != 3)
        {
            m_nSplitterPos = pt.x;
        }
        else
        {
            if (m_bDrag2)
            {
                if (pt.x < (m_nSplitterPos + MINWINSIZE))
                    pt.x = m_nSplitterPos + MINWINSIZE;
                m_nSplitterPos2 = pt.x;
            }
            else
            {
                if (pt.x > (m_nSplitterPos2 - MINWINSIZE))
                    pt.x = m_nSplitterPos2 - MINWINSIZE;
                m_nSplitterPos = pt.x;
            }
        }
    }

    ReleaseCapture();

    //position the child controls
    PositionChildren(&rect);
    return 0;
}

LRESULT CMainWindow::Splitter_OnMouseMove(HWND hwnd, UINT /*iMsg*/, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    RECT clientRect;

    POINT pt;

    if (m_bDragMode == FALSE)
        return 0;

    const auto borderSm = CDPIAware::Instance().Scale(*this, 2);
    const auto borderL  = CDPIAware::Instance().Scale(*this, 4);

    pt.x = static_cast<short>(LOWORD(lParam)); // horizontal position of cursor
    pt.y = static_cast<short>(HIWORD(lParam));

    GetClientRect(hwnd, &clientRect);
    GetWindowRect(hwnd, &rect);
    POINT zero = {0, 0};
    ClientToScreen(hwnd, &zero);
    OffsetRect(&clientRect, zero.x - rect.left, zero.y - rect.top);

    //convert the mouse coordinates relative to the top-left of
    //the window
    ClientToScreen(hwnd, &pt);
    pt.x -= rect.left;
    pt.y -= rect.top;

    //same for the window coordinates - make them relative to 0,0
    OffsetRect(&rect, -rect.left, -rect.top);

    if (pt.x < 0)
        pt.x = 0;
    if (pt.x > rect.right - borderL)
        pt.x = rect.right - borderL;
    if (pt.y < 0)
        pt.y = 0;
    if (pt.y > rect.bottom - borderL)
        pt.y = rect.bottom - borderL;

    if ((wParam & MK_LBUTTON) && ((m_bVertical && (pt.y != m_oldY)) || (!m_bVertical && (pt.x != m_oldX))))
    {
        HDC hdc = GetWindowDC(hwnd);

        if (m_bVertical)
        {
            DrawXorBar(hdc, clientRect.left, m_oldY + borderSm, clientRect.right - clientRect.left - borderSm, borderL);
            DrawXorBar(hdc, clientRect.left, pt.y + borderSm, clientRect.right - clientRect.left - borderSm, borderL);
        }
        else
        {
            DrawXorBar(hdc, m_oldX + borderSm, clientRect.top, borderL, clientRect.bottom - clientRect.top - borderSm);
            DrawXorBar(hdc, pt.x + borderSm, clientRect.top, borderL, clientRect.bottom - clientRect.top - borderSm);
        }

        ReleaseDC(hwnd, hdc);

        m_oldX = pt.x;
        m_oldY = pt.y;
    }

    return 0;
}

bool CMainWindow::OpenDialog() const
{
    return (DialogBox(hResource, MAKEINTRESOURCE(IDD_OPEN), *this, reinterpret_cast<DLGPROC>(OpenDlgProc)) == IDOK);
}

BOOL CALLBACK CMainWindow::OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            CTheme::Instance().SetThemeForDialog(hwndDlg, CTheme::Instance().IsDarkTheme());
            // center on the parent window
            HWND hParentWnd = ::GetParent(hwndDlg);
            RECT parentrect, childrect, centeredrect;
            GetWindowRect(hParentWnd, &parentrect);
            GetWindowRect(hwndDlg, &childrect);
            centeredrect.left   = parentrect.left + ((parentrect.right - parentrect.left - childrect.right + childrect.left) / 2);
            centeredrect.right  = centeredrect.left + (childrect.right - childrect.left);
            centeredrect.top    = parentrect.top + ((parentrect.bottom - parentrect.top - childrect.bottom + childrect.top) / 2);
            centeredrect.bottom = centeredrect.top + (childrect.bottom - childrect.top);
            SetWindowPos(hwndDlg, nullptr, centeredrect.left, centeredrect.top, centeredrect.right - centeredrect.left, centeredrect.bottom - centeredrect.top, SWP_SHOWWINDOW);

            if (!m_sLeftPicPath.empty())
                SetDlgItemText(hwndDlg, IDC_LEFTIMAGE, m_sLeftPicPath.c_str());
            SetFocus(hwndDlg);
        }
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_LEFTBROWSE:
                {
                    wchar_t path[MAX_PATH] = {0};
                    if (AskForFile(hwndDlg, path))
                    {
                        SetDlgItemText(hwndDlg, IDC_LEFTIMAGE, path);
                    }
                }
                break;
                case IDC_RIGHTBROWSE:
                {
                    wchar_t path[MAX_PATH] = {0};
                    if (AskForFile(hwndDlg, path))
                    {
                        SetDlgItemText(hwndDlg, IDC_RIGHTIMAGE, path);
                    }
                }
                break;
                case IDOK:
                {
                    wchar_t path[MAX_PATH] = {0};
                    if (!GetDlgItemText(hwndDlg, IDC_LEFTIMAGE, path, _countof(path)))
                        *path = 0;
                    m_sLeftPicPath = path;
                    if (!GetDlgItemText(hwndDlg, IDC_RIGHTIMAGE, path, _countof(path)))
                        *path = 0;
                    m_sRightPicPath = path;
                }
                    [[fallthrough]];
                case IDCANCEL:
                    CTheme::Instance().SetThemeForDialog(hwndDlg, false);
                    EndDialog(hwndDlg, wParam);
                    return TRUE;
            }
    }
    return FALSE;
}

bool CMainWindow::AskForFile(HWND owner, wchar_t* path)
{
    OPENFILENAME ofn = {0}; // common dialog box structure
    // Initialize OPENFILENAME
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner   = owner;
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ResString sTitle(::hResource, IDS_OPENIMAGEFILE);
    ofn.lpstrTitle    = sTitle;
    ofn.Flags         = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    ofn.hInstance     = ::hResource;
    wchar_t filters[] = L"Images\0*.wmf;*.jpg;*jpeg;*.bmp;*.gif;*.png;*.ico;*.dib;*.emf;*.webp\0All (*.*)\0*.*\0\0";
    ofn.lpstrFilter   = filters;
    ofn.nFilterIndex  = 1;
    // Display the Open dialog box.
    if (GetOpenFileName(&ofn) == FALSE)
    {
        return false;
    }
    return true;
}

bool CMainWindow::CreateToolbar()
{
    // Ensure that the common control DLL is loaded.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    m_hwndTb = CreateWindowEx(TBSTYLE_EX_DOUBLEBUFFER,
                              TOOLBARCLASSNAME,
                              static_cast<LPCWSTR>(nullptr),
                              WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                              0, 0, 0, 0,
                              *this,
                              reinterpret_cast<HMENU>(IDC_TORTOISEIDIFF),
                              hResource,
                              nullptr);
    if (m_hwndTb == INVALID_HANDLE_VALUE)
        return false;

    SendMessage(m_hwndTb, TB_BUTTONSTRUCTSIZE, static_cast<WPARAM>(sizeof(TBBUTTON)), 0);

    TBBUTTON tbb[14]{};
    // create an imagelist containing the icons for the toolbar
    auto imgSize      = CDPIAware::Instance().Scale(*this, 24);
    m_hToolbarImgList = ImageList_Create(imgSize, imgSize, ILC_COLOR32 | ILC_MASK | ILC_HIGHQUALITYSCALE, 12, 4);
    if (!m_hToolbarImgList)
        return false;
    int   index = 0;
    HICON hIcon = nullptr;
    if (m_selectionPaths.empty())
    {
        hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_OVERLAP), imgSize, imgSize);
        tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_OVERLAPIMAGES;
        tbb[index].fsState   = TBSTATE_ENABLED;
        tbb[index].fsStyle   = BTNS_BUTTON;
        tbb[index].dwData    = 0;
        tbb[index++].iString = 0;

        hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_BLEND), imgSize, imgSize);
        tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_BLENDALPHA;
        tbb[index].fsState   = 0;
        tbb[index].fsStyle   = BTNS_BUTTON;
        tbb[index].dwData    = 0;
        tbb[index++].iString = 0;

        tbb[index].iBitmap   = 0;
        tbb[index].idCommand = 0;
        tbb[index].fsState   = TBSTATE_ENABLED;
        tbb[index].fsStyle   = BTNS_SEP;
        tbb[index].dwData    = 0;
        tbb[index++].iString = 0;

        hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_LINK), imgSize, imgSize);
        tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_LINKIMAGESTOGETHER;
        tbb[index].fsState   = TBSTATE_ENABLED | TBSTATE_CHECKED;
        tbb[index].fsStyle   = BTNS_BUTTON;
        tbb[index].dwData    = 0;
        tbb[index++].iString = 0;

        hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_FITWIDTHS), imgSize, imgSize);
        tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_FITIMAGEWIDTHS;
        tbb[index].fsState   = TBSTATE_ENABLED;
        tbb[index].fsStyle   = BTNS_BUTTON;
        tbb[index].dwData    = 0;
        tbb[index++].iString = 0;

        hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_FITHEIGHTS), imgSize, imgSize);
        tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_FITIMAGEHEIGHTS;
        tbb[index].fsState   = TBSTATE_ENABLED;
        tbb[index].fsStyle   = BTNS_BUTTON;
        tbb[index].dwData    = 0;
        tbb[index++].iString = 0;

        tbb[index].iBitmap   = 0;
        tbb[index].idCommand = 0;
        tbb[index].fsState   = TBSTATE_ENABLED;
        tbb[index].fsStyle   = BTNS_SEP;
        tbb[index].dwData    = 0;
        tbb[index++].iString = 0;
    }

    hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_FITINWINDOW), imgSize, imgSize);
    tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_FITIMAGESINWINDOW;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_ORIGSIZE), imgSize, imgSize);
    tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ORININALSIZE;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_ZOOMIN), imgSize, imgSize);
    tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ZOOMIN;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_ZOOMOUT), imgSize, imgSize);
    tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ZOOMOUT;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    tbb[index].iBitmap   = 0;
    tbb[index].idCommand = 0;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_SEP;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_IMGINFO), imgSize, imgSize);
    tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_IMAGEINFO;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    hIcon                = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_VERTICAL), imgSize, imgSize);
    tbb[index].iBitmap   = ImageList_AddIcon(m_hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ARRANGEVERTICAL;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    SendMessage(m_hwndTb, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(m_hToolbarImgList));
    SendMessage(m_hwndTb, TB_ADDBUTTONS, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(reinterpret_cast<LPTBBUTTON>(&tbb)));
    SendMessage(m_hwndTb, TB_AUTOSIZE, 0, 0);
    ShowWindow(m_hwndTb, SW_SHOW);
    return true;
}

void CMainWindow::SetSelectionImage(FileType ft, const std::wstring& path, const std::wstring& title)
{
    m_selectionPaths[ft]  = path;
    m_selectionTitles[ft] = title;
}
