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

CTheme::CTheme()
    : m_bLoaded(false)
    , m_dark(false)
    , m_lastThemeChangeCallbackId(0)
    , m_regDarkTheme(REGSTRING_DARKTHEME, 0) // define REGSTRING_DARKTHEME on app level as e.g. L"Software\\TortoiseMerge\\DarkTheme"
    , m_bDarkModeIsAllowed(false)
{
}

CTheme::~CTheme()
{
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
    m_dark = !!m_regDarkTheme;
    IsDarkModeAllowed();
    m_bLoaded = true;
}

COLORREF CTheme::GetThemeColor(COLORREF clr) const
{
    if (m_dark)
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

bool CTheme::IsDarkModeAllowed()
{
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

void CTheme::SetDarkTheme(bool b /*= true*/)
{
    if (!m_bDarkModeIsAllowed)
        return;
    if (m_dark != b)
    {
        m_dark         = b;
        m_regDarkTheme = b ? 1 : 0;
        for (auto& cb : m_themeChangeCallbacks)
            cb.second();
    }
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
