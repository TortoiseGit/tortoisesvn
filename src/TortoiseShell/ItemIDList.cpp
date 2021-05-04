// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2006, 2009, 2011-2016, 2021 - TortoiseSVN

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
#include "ShellExt.h"
#include "ItemIDList.h"

ItemIDList::ItemIDList(PCUITEMID_CHILD item, PCUIDLIST_RELATIVE parent)
    : m_item(item)
    , m_parent(parent)
    , m_count(-1)
{
}

ItemIDList::ItemIDList(PCIDLIST_ABSOLUTE item)
    : m_item(reinterpret_cast<PCUITEMID_CHILD>(item))
    , m_parent(nullptr)
    , m_count(-1)
{
}

ItemIDList::~ItemIDList()
{
}

int ItemIDList::size() const
{
    if (m_count == -1)
    {
        m_count = 0;
        if (m_item)
        {
            LPCSHITEMID ptr = &m_item->mkid;
            while (ptr != nullptr && ptr->cb != 0)
            {
                ++m_count;
                LPBYTE byte = reinterpret_cast<LPBYTE>(const_cast<LPSHITEMID>(ptr));
                byte += ptr->cb;
                ptr = reinterpret_cast<LPCSHITEMID>(byte);
            }
        }
    }
    return m_count;
}

LPCSHITEMID ItemIDList::get(int index) const
{
    int count = 0;

    if (m_item == nullptr)
        return nullptr;
    LPCSHITEMID ptr = &m_item->mkid;
    if (ptr == nullptr)
        return nullptr;
    while (ptr->cb != 0)
    {
        if (count == index)
            break;

        ++count;
        LPBYTE byte = reinterpret_cast<LPBYTE>(const_cast<LPSHITEMID>(ptr));
        byte += ptr->cb;
        ptr = reinterpret_cast<LPCSHITEMID>(byte);
    }
    return ptr;
}

std::wstring ItemIDList::toString(bool resolveLibraries /*= true*/) const
{
    CComPtr<IShellFolder> shellFolder;
    CComPtr<IShellFolder> parentFolder;
    std::wstring          ret;

    if (FAILED(::SHGetDesktopFolder(&shellFolder)))
        return ret;
    if (!m_parent || FAILED(shellFolder->BindToObject(m_parent, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&parentFolder))))
        parentFolder = shellFolder;

    STRRET   name;
    wchar_t* szDisplayName = nullptr;
    if ((parentFolder != nullptr) && (m_item != nullptr))
    {
        if (FAILED(parentFolder->GetDisplayNameOf(m_item, SHGDN_NORMAL | SHGDN_FORPARSING, &name)))
            return ret;
        if (FAILED(StrRetToStr(&name, m_item, &szDisplayName)))
            return ret;
    }
    if (!szDisplayName)
        return ret;
    ret = szDisplayName;
    CoTaskMemFree(szDisplayName);

    if (!((resolveLibraries) && (wcsncmp(ret.c_str(), L"::{", 3) == 0)))
        return ret;

    CComPtr<IShellLibrary> pLib;
    if (FAILED(pLib.CoCreateInstance(CLSID_ShellLibrary, nullptr, CLSCTX_INPROC_SERVER)))
        return ret;

    CComPtr<IShellItem> psiLibrary;
    if (FAILED(SHCreateItemFromParsingName(ret.c_str(), nullptr, IID_PPV_ARGS(&psiLibrary))))
        return ret;

    if (FAILED(pLib->LoadLibraryFromItem(psiLibrary, STGM_READ | STGM_SHARE_DENY_NONE)))
        return ret;

    CComPtr<IShellItem> psiSaveLocation;
    if (FAILED(pLib->GetDefaultSaveFolder(DSFT_DETECT, IID_PPV_ARGS(&psiSaveLocation))))
        return ret;

    PWSTR pszName = nullptr;
    if (SUCCEEDED(psiSaveLocation->GetDisplayName(SIGDN_FILESYSPATH, &pszName)))
    {
        ret = pszName;
        CoTaskMemFree(pszName);
    }

    return ret;
}

PCUITEMID_CHILD ItemIDList::operator&() const
{
    return m_item;
}
