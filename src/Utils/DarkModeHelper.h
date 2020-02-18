// TortoiseSVN - a Windows shell extension for easy version control

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
#pragma once
#include <Uxtheme.h>

/// helper class for the Windows 10 dark mode
/// note: we use undocumented APIs here, so be careful!
class DarkModeHelper
{
public:
    static DarkModeHelper& Instance();

    bool CanHaveDarkMode();
    void AllowDarkModeForApp(BOOL allow);
    void AllowDarkModeForWindow(HWND hwnd, BOOL allow);
    BOOL ShouldAppsUseDarkMode();
    BOOL IsDarkModeAllowedForWindow(HWND hwnd);
    BOOL IsDarkModeAllowedForApp();
    BOOL ShouldSystemUseDarkMode();
    void RefreshImmersiveColorPolicyState();
    //void FlushMenuThemes();

private:
    DarkModeHelper();
    ~DarkModeHelper();

    typedef void(WINAPI* AllowDarkModeForAppFPN)(BOOL allow);
    typedef void(WINAPI* AllowDarkModeForWindowFPN)(HWND hwnd, BOOL allow);
    typedef BOOL(WINAPI* ShouldAppsUseDarkModeFPN)();
    typedef BOOL(WINAPI* IsDarkModeAllowedForWindowFPN)(HWND hwnd);
    typedef BOOL(WINAPI* IsDarkModeAllowedForAppFPN)();
    typedef BOOL(WINAPI* ShouldSystemUseDarkModeFPN)();
    typedef void(WINAPI* RefreshImmersiveColorPolicyStateFN)();
    //typedef void(WINAPI* FlushMenuThemesFN)();
    typedef HTHEME(WINAPI* OpenNCThemeDataFPN)(HWND hWnd, LPCWSTR pszClassList);

    AllowDarkModeForAppFPN             m_pAllowDarkModeForApp              = nullptr;
    AllowDarkModeForWindowFPN          m_pAllowDarkModeForWindow           = nullptr;
    ShouldAppsUseDarkModeFPN           m_pShouldAppsUseDarkMode            = nullptr;
    IsDarkModeAllowedForWindowFPN      m_pIsDarkModeAllowedForWindow       = nullptr;
    IsDarkModeAllowedForAppFPN         m_pIsDarkModeAllowedForApp          = nullptr;
    ShouldSystemUseDarkModeFPN         m_pShouldSystemUseDarkMode          = nullptr;
    RefreshImmersiveColorPolicyStateFN m_pRefreshImmersiveColorPolicyState = nullptr;
    //FlushMenuThemesFN                  m_pFlushMenuThemes                  = nullptr;
    HMODULE                            m_hUxthemeLib                       = nullptr;
    bool                               m_bCanHaveDarkMode                  = false;
};
