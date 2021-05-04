// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2009, 2011, 2013-2015, 2018, 2021 - TortoiseSVN

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
#include "IconMenu.h"
#include "registry.h"
#include "LoadIconEx.h"

CIconMenu::CIconMenu()
    : CMenu()
{
    m_bShowIcons = !!static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\ShowAppContextMenuIcons", TRUE));
}

CIconMenu::~CIconMenu()
{
    for (auto it = m_iconHandles.cbegin(); it != m_iconHandles.cend(); ++it)
        DestroyIcon(it->second);
}

BOOL CIconMenu::CreateMenu()
{
    if (__super::CreateMenu() == FALSE)
        return FALSE;

    SetMenuStyle();

    return TRUE;
}

BOOL CIconMenu::CreatePopupMenu()
{
    if (__super::CreatePopupMenu() == FALSE)
        return FALSE;

    SetMenuStyle();

    return TRUE;
}

BOOL CIconMenu::SetMenuStyle()
{
    MENUINFO menuInfo = {0};
    menuInfo.cbSize   = sizeof(menuInfo);
    menuInfo.fMask    = MIM_STYLE | MIM_APPLYTOSUBMENUS;
    menuInfo.dwStyle  = MNS_CHECKORBMP;

    SetMenuInfo(&menuInfo);

    return TRUE;
}

BOOL CIconMenu::AppendMenuIcon(UINT_PTR nIDNewItem, LPCWSTR lpszNewItem, UINT uIcon /* = 0 */)
{
    wchar_t menuTextBuffer[255] = {0};
    wcscpy_s(menuTextBuffer, lpszNewItem);

    if ((uIcon == 0) || (!m_bShowIcons))
        return CMenu::AppendMenu(MF_STRING | MF_ENABLED, nIDNewItem, menuTextBuffer);

    MENUITEMINFO info   = {0};
    info.cbSize         = sizeof(info);
    info.fMask          = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_BITMAP;
    info.fType          = MFT_STRING;
    info.wID            = static_cast<UINT>(nIDNewItem);
    info.dwTypeData     = menuTextBuffer;
    info.hbmpItem       = m_bitmapUtils.IconToBitmapPARGB32(AfxGetResourceHandle(), uIcon);
    m_icons[nIDNewItem] = uIcon;

    return InsertMenuItem(static_cast<UINT>(nIDNewItem), &info);
}

BOOL CIconMenu::AppendMenuIcon(UINT_PTR nIDNewItem, UINT_PTR nNewItem, UINT uIcon /* = 0 */)
{
    CString temp;
    temp.LoadString(static_cast<UINT>(nNewItem));

    return AppendMenuIcon(nIDNewItem, temp, uIcon);
}

BOOL CIconMenu::AppendMenuIcon(UINT_PTR nIDNewItem, UINT_PTR nNewItem, HICON hIcon)
{
    CString temp;
    temp.LoadString(static_cast<UINT>(nNewItem));

    wchar_t menuTextBuffer[255] = {0};
    wcscpy_s(menuTextBuffer, temp);

    if ((hIcon == nullptr) || (!m_bShowIcons))
        return CMenu::AppendMenu(MF_STRING | MF_ENABLED, nIDNewItem, menuTextBuffer);

    MENUITEMINFO info         = {0};
    info.cbSize               = sizeof(info);
    info.fMask                = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_BITMAP;
    info.fType                = MFT_STRING;
    info.wID                  = static_cast<UINT>(nIDNewItem);
    info.dwTypeData           = menuTextBuffer;
    info.hbmpItem             = m_bitmapUtils.IconToBitmapPARGB32(hIcon);
    m_iconHandles[nIDNewItem] = hIcon;

    return InsertMenuItem(static_cast<UINT>(nIDNewItem), &info);
}

void CIconMenu::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if ((lpDrawItemStruct == nullptr) || (lpDrawItemStruct->CtlType != ODT_MENU))
        return; //not for a menu
    HICON hIcon        = nullptr;
    bool  bDestroyIcon = true;
    int   iconWidth    = GetSystemMetrics(SM_CXSMICON);
    int   iconHeight   = GetSystemMetrics(SM_CYSMICON);
    if (m_iconHandles.find(lpDrawItemStruct->itemID) != m_iconHandles.end())
    {
        hIcon        = m_iconHandles[lpDrawItemStruct->itemID];
        bDestroyIcon = false;
    }
    else
        hIcon = LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(m_icons[lpDrawItemStruct->itemID]), iconWidth, iconHeight);
    if (hIcon == nullptr)
        return;
    DrawIconEx(lpDrawItemStruct->hDC,
               lpDrawItemStruct->rcItem.left - iconWidth,
               lpDrawItemStruct->rcItem.top + (lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - iconHeight) / 2,
               hIcon, iconWidth, iconHeight,
               0, nullptr, DI_NORMAL);
    if (bDestroyIcon)
        DestroyIcon(hIcon);
}

void CIconMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    if (lpMeasureItemStruct == nullptr)
        return;
    lpMeasureItemStruct->itemWidth += 2;
    if (lpMeasureItemStruct->itemHeight < 16)
        lpMeasureItemStruct->itemHeight = 16;
}
