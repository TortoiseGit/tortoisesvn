// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2020 - TortoiseSVN

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
#include "Theme.h"
#include "SimpleIni.h"
#include "resource.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "DarkModeHelper.h"
#include <Uxtheme.h>
#include <vssym32.h>
#include <richedit.h>

constexpr COLORREF darkBkColor   = 0x101010;
constexpr COLORREF darkTextColor = 0xFFFFFF;

HBRUSH CTheme::s_backBrush = nullptr;

CTheme::CTheme()
    : m_bLoaded(false)
    , m_dark(false)
    , m_lastThemeChangeCallbackId(0)
    , m_regDarkTheme(REGSTRING_DARKTHEME, 0) // define REGSTRING_DARKTHEME on app level as e.g. L"Software\\TortoiseMerge\\DarkTheme"
    , m_bDarkModeIsAllowed(false)
    , m_isHighContrastMode(false)
    , m_isHighContrastModeDark(false)
{
}

CTheme::~CTheme()
{
    if (s_backBrush)
        DeleteObject(s_backBrush);
}

CTheme& CTheme::Instance()
{
    static CTheme instance;
    if (!instance.m_bLoaded)
        instance.Load();

    return instance;
}

void CTheme::Load()
{
    IsDarkModeAllowed();
    OnSysColorChanged();
    m_dark    = !!m_regDarkTheme && IsDarkModeAllowed();
    m_bLoaded = true;
}

bool CTheme::IsHighContrastMode() const
{
    return m_isHighContrastMode;
}

bool CTheme::IsHighContrastModeDark() const
{
    return m_isHighContrastModeDark;
}

COLORREF CTheme::GetThemeColor(COLORREF clr, bool fixed /*= false*/) const
{
    if (m_dark || (fixed && m_isHighContrastModeDark))
    {
        float h, s, l;
        CTheme::RGBtoHSL(clr, h, s, l);
        l = 100.0f - l;
        return CTheme::HSLtoRGB(h, s, l);
    }

    return clr;
}

int CTheme::RegisterThemeChangeCallback(ThemeChangeCallback&& cb)
{
    ++m_lastThemeChangeCallbackId;
    int nextThemeCallBackId = m_lastThemeChangeCallbackId;
    m_themeChangeCallbacks.emplace(nextThemeCallBackId, std::move(cb));
    return nextThemeCallBackId;
}

bool CTheme::RemoveRegisteredCallback(int id)
{
    auto foundIt = m_themeChangeCallbacks.find(id);
    if (foundIt != m_themeChangeCallbacks.end())
    {
        m_themeChangeCallbacks.erase(foundIt);
        return true;
    }
    return false;
}

bool CTheme::SetThemeForDialog(HWND hWnd, bool bDark)
{
    DarkModeHelper::Instance().AllowDarkModeForWindow(hWnd, bDark);
    if (bDark)
    {
        SetWindowSubclass(hWnd, MainSubclassProc, 1234, (DWORD_PTR)&s_backBrush);
    }
    else
    {
        RemoveWindowSubclass(hWnd, MainSubclassProc, 1234);
    }
    EnumChildWindows(hWnd, AdjustThemeForChildrenProc, bDark ? TRUE : FALSE);
    ::RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
    return true;
}

BOOL CTheme::AdjustThemeForChildrenProc(HWND hwnd, LPARAM lParam)
{
    DarkModeHelper::Instance().AllowDarkModeForWindow(hwnd, (BOOL)lParam);
    TCHAR szWndClassName[MAX_PATH] = {0};
    GetClassName(hwnd, szWndClassName, _countof(szWndClassName));
    if (lParam)
    {
        if ((wcscmp(szWndClassName, WC_LISTVIEW) == 0) || (wcscmp(szWndClassName, WC_LISTBOX) == 0))
        {
            // theme "Explorer" also gets the scrollbars with dark mode, but the hover color
            // is the blueish from the bright mode.
            // theme "ItemsView" has the hover color the same as the windows explorer (grayish),
            // but then the scrollbars are not drawn for dark mode.
            // theme "DarkMode_Explorer" doesn't paint a hover color at all.
            //
            // Also, the group headers are not affected in dark mode and therefore the group texts are
            // hardly visible.
            //
            // so use "Explorer" for now. The downside of the bluish hover color isn't that bad,
            // except in situations where both a treeview and a listview are on the same dialog
            // at the same time (e.g. repobrowser) - then the difference is unfortunately very
            // noticeable...
            SetWindowTheme(hwnd, L"Explorer", nullptr);
            auto header = ListView_GetHeader(hwnd);
            DarkModeHelper::Instance().AllowDarkModeForWindow(header, (BOOL)lParam);
            HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
            if (hTheme)
            {
                COLORREF color;
                if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
                {
                    ListView_SetTextColor(hwnd, color);
                }
                if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
                {
                    ListView_SetTextBkColor(hwnd, color);
                    ListView_SetBkColor(hwnd, color);
                }
                CloseThemeData(hTheme);
            }
            auto hTT = ListView_GetToolTips(hwnd);
            if (hTT)
            {
                DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, (BOOL)lParam);
                SetWindowTheme(hTT, L"Explorer", nullptr);
            }
            SetWindowSubclass(hwnd, ListViewSubclassProc, 1234, (DWORD_PTR)&s_backBrush);
        }
        else if (wcscmp(szWndClassName, WC_HEADER) == 0)
        {
            SetWindowTheme(hwnd, L"ItemsView", nullptr);
        }
        else if (wcscmp(szWndClassName, WC_BUTTON) == 0)
        {
            auto style = GetWindowLongPtr(hwnd, GWL_STYLE) & 0x0F;
            if ((style & BS_GROUPBOX) == BS_GROUPBOX)
                SetWindowTheme(hwnd, L"", L"");
            else if (style == BS_CHECKBOX || style == BS_AUTOCHECKBOX || style == BS_3STATE || style == BS_AUTO3STATE || style == BS_RADIOBUTTON || style == BS_AUTORADIOBUTTON)
                SetWindowTheme(hwnd, L"", L"");
            else if (FAILED(SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr)))
                SetWindowTheme(hwnd, L"Explorer", nullptr);
        }
        else if (wcscmp(szWndClassName, WC_STATIC) == 0)
        {
            SetWindowTheme(hwnd, L"", L"");
        }
        else if (wcscmp(szWndClassName, L"SysDateTimePick32") == 0)
        {
            SetWindowTheme(hwnd, L"", L"");
            SendMessage(hwnd, DTM_SETMCCOLOR, MCSC_BACKGROUND, RGB(0, 0, 0));
        }
        else if ((wcscmp(szWndClassName, WC_COMBOBOXEX) == 0) ||
                 (wcscmp(szWndClassName, WC_COMBOBOX) == 0))
        {
            SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
            HWND hCombo = hwnd;
            if (wcscmp(szWndClassName, WC_COMBOBOXEX) == 0)
            {
                SendMessage(hwnd, CBEM_SETWINDOWTHEME, 0, (LPARAM)L"DarkMode_Explorer");
                hCombo = (HWND)SendMessage(hwnd, CBEM_GETCOMBOCONTROL, 0, 0);
            }
            if (hCombo)
            {
                SetWindowSubclass(hCombo, ComboBoxSubclassProc, 1234, (DWORD_PTR)&s_backBrush);
                COMBOBOXINFO info = {0};
                info.cbSize       = sizeof(COMBOBOXINFO);
                if (SendMessage(hCombo, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
                {
                    DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndList, (BOOL)lParam);
                    DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndItem, (BOOL)lParam);
                    DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndCombo, (BOOL)lParam);

                    SetWindowTheme(info.hwndList, L"DarkMode_Explorer", nullptr);
                    SetWindowTheme(info.hwndItem, L"DarkMode_Explorer", nullptr);
                    SetWindowTheme(info.hwndCombo, L"DarkMode_Explorer", nullptr);
                }
            }
        }
        else if (wcscmp(szWndClassName, WC_TREEVIEW) == 0)
        {
            SetWindowTheme(hwnd, L"Explorer", nullptr);
            HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
            if (hTheme)
            {
                COLORREF color;
                if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
                {
                    TreeView_SetTextColor(hwnd, color);
                }
                if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
                {
                    TreeView_SetBkColor(hwnd, color);
                }
                CloseThemeData(hTheme);
            }
            auto hTT = TreeView_GetToolTips(hwnd);
            if (hTT)
            {
                DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, (BOOL)lParam);
                SetWindowTheme(hTT, L"Explorer", nullptr);
            }
        }
        else if (wcsncmp(szWndClassName, L"RICHEDIT", 8) == 0)
        {
            SetWindowTheme(hwnd, L"Explorer", nullptr);
            CHARFORMAT2 format = {0};
            format.cbSize      = sizeof(CHARFORMAT2);
            format.dwMask      = CFM_COLOR | CFM_BACKCOLOR;
            format.crTextColor = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT));
            format.crBackColor = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOW));
            SendMessage(hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
            SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, (LPARAM)format.crBackColor);
        }
        else if (FAILED(SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr)))
            SetWindowTheme(hwnd, L"Explorer", nullptr);
    }
    else
    {
        if (wcscmp(szWndClassName, WC_LISTVIEW) == 0)
        {
            SetWindowTheme(hwnd, L"Explorer", nullptr);
            HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
            if (hTheme)
            {
                COLORREF color;
                if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
                {
                    ListView_SetTextColor(hwnd, color);
                }
                if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
                {
                    ListView_SetTextBkColor(hwnd, color);
                    ListView_SetBkColor(hwnd, color);
                }
                CloseThemeData(hTheme);
            }
            auto hTT = ListView_GetToolTips(hwnd);
            if (hTT)
            {
                DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, (BOOL)lParam);
                SetWindowTheme(hTT, L"Explorer", nullptr);
            }
            RemoveWindowSubclass(hwnd, ListViewSubclassProc, 1234);
        }
        else if (wcscmp(szWndClassName, L"ComboBoxEx32") == 0)
        {
            SetWindowTheme(hwnd, L"Explorer", nullptr);
            SendMessage(hwnd, CBEM_SETWINDOWTHEME, 0, (LPARAM)L"Explorer");
            auto hCombo = (HWND)SendMessage(hwnd, CBEM_GETCOMBOCONTROL, 0, 0);
            if (hCombo)
            {
                COMBOBOXINFO info = {0};
                info.cbSize       = sizeof(COMBOBOXINFO);
                if (SendMessage(hCombo, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
                {
                    DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndList, (BOOL)lParam);
                    DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndItem, (BOOL)lParam);
                    DarkModeHelper::Instance().AllowDarkModeForWindow(info.hwndCombo, (BOOL)lParam);

                    SetWindowTheme(info.hwndList, L"Explorer", nullptr);
                    SetWindowTheme(info.hwndItem, L"Explorer", nullptr);
                    SetWindowTheme(info.hwndCombo, L"Explorer", nullptr);

                    HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
                    if (hTheme)
                    {
                        COLORREF color;
                        if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
                        {
                            ListView_SetTextColor(info.hwndList, color);
                        }
                        if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
                        {
                            ListView_SetTextBkColor(info.hwndList, color);
                            ListView_SetBkColor(info.hwndList, color);
                        }
                        CloseThemeData(hTheme);
                    }

                    RemoveWindowSubclass(info.hwndList, ListViewSubclassProc, 1234);
                }
                RemoveWindowSubclass(hCombo, ComboBoxSubclassProc, 1234);
            }
        }
        else if (wcscmp(szWndClassName, WC_TREEVIEW) == 0)
        {
            SetWindowTheme(hwnd, L"Explorer", nullptr);
            HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
            if (hTheme)
            {
                COLORREF color;
                if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
                {
                    TreeView_SetTextColor(hwnd, color);
                }
                if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
                {
                    TreeView_SetBkColor(hwnd, color);
                }
                CloseThemeData(hTheme);
            }
            auto hTT = TreeView_GetToolTips(hwnd);
            if (hTT)
            {
                DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, (BOOL)lParam);
                SetWindowTheme(hTT, L"Explorer", nullptr);
            }
        }
        else if (wcsncmp(szWndClassName, L"RICHEDIT", 8) == 0)
        {
            SetWindowTheme(hwnd, L"Explorer", nullptr);
            CHARFORMAT2 format = {0};
            format.cbSize      = sizeof(CHARFORMAT2);
            format.dwMask      = CFM_COLOR | CFM_BACKCOLOR;
            format.crTextColor = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOWTEXT));
            format.crBackColor = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_WINDOW));
            SendMessage(hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
            SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, (LPARAM)format.crBackColor);
        }
        else
            SetWindowTheme(hwnd, L"Explorer", nullptr);
    }
    return TRUE;
}

LRESULT CTheme::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW)
            {
                LPNMCUSTOMDRAW nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
                switch (nmcd->dwDrawStage)
                {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;
                    case CDDS_ITEMPREPAINT:
                    {
                        HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
                        if (hTheme)
                        {
                            COLORREF color;
                            if (SUCCEEDED(::GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
                            {
                                SetTextColor(nmcd->hdc, color);
                            }
                            CloseThemeData(hTheme);
                        }
                        return CDRF_DODEFAULT;
                    }
                }
            }
        }
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CTheme::ComboBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSCROLLBAR:
        {
            auto hbrBkgnd = (HBRUSH*)dwRefData;
            HDC  hdc      = reinterpret_cast<HDC>(wParam);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, darkTextColor);
            SetBkColor(hdc, darkBkColor);
            if (!*hbrBkgnd)
                *hbrBkgnd = CreateSolidBrush(darkBkColor);
            return reinterpret_cast<LRESULT>(*hbrBkgnd);
        }
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CTheme::MainSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSCROLLBAR:
        {
            auto hbrBkgnd = (HBRUSH*)dwRefData;
            HDC  hdc      = reinterpret_cast<HDC>(wParam);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, darkTextColor);
            SetBkColor(hdc, darkBkColor);
            if (!*hbrBkgnd)
                *hbrBkgnd = CreateSolidBrush(darkBkColor);
            return reinterpret_cast<LRESULT>(*hbrBkgnd);
        }
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CTheme::OnSysColorChanged()
{
    m_isHighContrastModeDark = false;
    m_isHighContrastMode     = false;
    HIGHCONTRAST hc          = {sizeof(HIGHCONTRAST)};
    SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, FALSE);
    if ((hc.dwFlags & HCF_HIGHCONTRASTON) != 0)
    {
        m_isHighContrastMode = true;
        // check if the high contrast mode is dark
        float h1, h2, s1, s2, l1, l2;
        RGBtoHSL(::GetSysColor(COLOR_WINDOWTEXT), h1, s1, l1);
        RGBtoHSL(::GetSysColor(COLOR_WINDOW), h2, s2, l2);
        m_isHighContrastModeDark = l2 < l1;
    }
    m_dark = !!m_regDarkTheme && IsDarkModeAllowed();
}

bool CTheme::IsDarkModeAllowed()
{
    if (IsHighContrastMode())
        return false;

    if (m_bLoaded)
        return m_bDarkModeIsAllowed;
    // we only allow the dark mode for Win10 1809 and later,
    // because on earlier versions it would look really, really ugly!
    m_bDarkModeIsAllowed              = false;
    auto                      version = CPathUtils::GetVersionFromFile(L"uiribbon.dll");
    std::vector<std::wstring> tokens;
    stringtok(tokens, version, false, L".");
    if (tokens.size() == 4)
    {
        auto major = std::stol(tokens[0]);
        auto minor = std::stol(tokens[1]);
        auto micro = std::stol(tokens[2]);
        //auto build = std::stol(tokens[3]);

        // the windows 10 update 1809 has the version
        // number as 10.0.17763.1
        if (major > 10)
            m_bDarkModeIsAllowed = true;
        else if (major == 10)
        {
            if (minor > 0)
                m_bDarkModeIsAllowed = true;
            else if (micro > 17762)
                m_bDarkModeIsAllowed = true;
        }
    }
    return m_bDarkModeIsAllowed;
}

void CTheme::SetDarkTheme(bool b /*= true*/, bool force /*= false*/)
{
    if (!m_bDarkModeIsAllowed && !force)
        return;
    if (force || m_dark != b)
    {
        bool highContrast = IsHighContrastMode();
        m_dark            = b && !highContrast;
        if (!highContrast)
            m_regDarkTheme = b ? 1 : 0;
        for (auto& cb : m_themeChangeCallbacks)
            cb.second();
    }
}

/// returns true if dark theme is enabled. If false, then the normal theme is active.

bool CTheme::IsDarkTheme() const
{
    return m_dark;
}

void CTheme::RGBToHSB(COLORREF rgb, BYTE& hue, BYTE& saturation, BYTE& brightness)
{
    BYTE   r      = GetRValue(rgb);
    BYTE   g      = GetGValue(rgb);
    BYTE   b      = GetBValue(rgb);
    BYTE   minRGB = min(min(r, g), b);
    BYTE   maxRGB = max(max(r, g), b);
    BYTE   delta  = maxRGB - minRGB;
    double l      = maxRGB;
    double s      = 0.0;
    double h      = 0.0;
    if (maxRGB == 0)
    {
        hue        = 0;
        saturation = 0;
        brightness = 0;
        return;
    }
    if (maxRGB)
        s = (255.0 * delta) / maxRGB;

    if (BYTE(s) != 0)
    {
        if (r == maxRGB)
            h = 0 + 43 * double(g - b) / delta;
        else if (g == maxRGB)
            h = 85 + 43 * double(b - r) / delta;
        else if (b == maxRGB)
            h = 171 + 43 * double(r - g) / delta;
    }
    else
        h = 0.0;

    hue        = BYTE(h);
    saturation = BYTE(s);
    brightness = BYTE(l);
}

void CTheme::RGBtoHSL(COLORREF color, float& h, float& s, float& l)
{
    const float r_percent = float(GetRValue(color)) / 255;
    const float g_percent = float(GetGValue(color)) / 255;
    const float b_percent = float(GetBValue(color)) / 255;

    float max_color = 0;
    if ((r_percent >= g_percent) && (r_percent >= b_percent))
        max_color = r_percent;
    else if ((g_percent >= r_percent) && (g_percent >= b_percent))
        max_color = g_percent;
    else if ((b_percent >= r_percent) && (b_percent >= g_percent))
        max_color = b_percent;

    float min_color = 0;
    if ((r_percent <= g_percent) && (r_percent <= b_percent))
        min_color = r_percent;
    else if ((g_percent <= r_percent) && (g_percent <= b_percent))
        min_color = g_percent;
    else if ((b_percent <= r_percent) && (b_percent <= g_percent))
        min_color = b_percent;

    float L = 0, S = 0, H = 0;

    L = (max_color + min_color) / 2;

    if (max_color == min_color)
    {
        S = 0;
        H = 0;
    }
    else
    {
        auto d = max_color - min_color;
        if (L < .50)
            S = d / (max_color + min_color);
        else
            S = d / ((2.0f - max_color) - min_color);

        if (max_color == r_percent)
            H = (g_percent - b_percent) / d;

        else if (max_color == g_percent)
            H = 2.0f + (b_percent - r_percent) / d;

        else if (max_color == b_percent)
            H = 4.0f + (r_percent - g_percent) / d;
    }
    H = H * 60;
    if (H < 0)
        H += 360;
    s = S * 100;
    l = L * 100;
    h = H;
}

static float HSLtoRGB_Subfunction(float temp1, float temp2, float temp3)
{
    if ((temp3 * 6) < 1)
        return (temp2 + (temp1 - temp2) * 6 * temp3) * 100;
    else if ((temp3 * 2) < 1)
        return temp1 * 100;
    else if ((temp3 * 3) < 2)
        return (temp2 + (temp1 - temp2) * (.66666f - temp3) * 6) * 100;
    else
        return temp2 * 100;
}

COLORREF CTheme::HSLtoRGB(float h, float s, float l)
{
    if (s == 0)
    {
        BYTE t = BYTE(l / 100 * 255);
        return RGB(t, t, t);
    }
    const float L     = l / 100;
    const float S     = s / 100;
    const float H     = h / 360;
    const float temp1 = (L < .50) ? L * (1 + S) : L + S - (L * S);
    const float temp2 = 2 * L - temp1;
    float       temp3 = 0;
    temp3             = H + .33333f;
    if (temp3 > 1)
        temp3 -= 1;
    const float pcr = HSLtoRGB_Subfunction(temp1, temp2, temp3);
    temp3           = H;
    const float pcg = HSLtoRGB_Subfunction(temp1, temp2, temp3);
    temp3           = H - .33333f;
    if (temp3 < 0)
        temp3 += 1;
    const float pcb = HSLtoRGB_Subfunction(temp1, temp2, temp3);
    BYTE        r   = BYTE(pcr / 100 * 255);
    BYTE        g   = BYTE(pcg / 100 * 255);
    BYTE        b   = BYTE(pcb / 100 * 255);
    return RGB(r, g, b);
}
