// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2006, 2008, 2013-2015, 2021 - TortoiseSVN

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
#include <assert.h>
#include "LangDll.h"
#include "../version.h"

#include <memory>

#pragma comment(lib, "Version.lib")

CLangDll::CLangDll()
{
    m_hInstance = nullptr;
}

CLangDll::~CLangDll()
{
}

HINSTANCE CLangDll::Init(LPCWSTR appName, unsigned long langID)
{
    wchar_t langPath[MAX_PATH] = {0};
    wchar_t sVer[MAX_PATH]     = {0};
    wcscpy_s(sVer, TEXT(STRPRODUCTVER));
    GetModuleFileName(nullptr, langPath, _countof(langPath));
    wchar_t* pSlash = wcsrchr(langPath, '\\');
    if (!pSlash)
        return m_hInstance;

    *pSlash = 0;
    pSlash  = wcsrchr(langPath, '\\');
    if (!pSlash)
        return m_hInstance;

    *pSlash = 0;
    wcscat_s(langPath, L"\\Languages\\");
    assert(m_hInstance == NULL);
    do
    {
        wchar_t langDllPath[MAX_PATH] = {0};
        swprintf_s(langDllPath, L"%s%s%lu.dll", langPath, appName, langID);

        m_hInstance = LoadLibrary(langDllPath);

        if (!DoVersionStringsMatch(sVer, langDllPath))
        {
            FreeLibrary(m_hInstance);
            m_hInstance = nullptr;
        }
        if (!m_hInstance)
        {
            DWORD lid = SUBLANGID(langID);
            lid--;
            if (lid > 0)
                langID = MAKELANGID(PRIMARYLANGID(langID), lid);
            else
                langID = 0;
        }
    } while (!m_hInstance && (langID != 0));

    return m_hInstance;
}

void CLangDll::Close()
{
    if (!m_hInstance)
        return;

    FreeLibrary(m_hInstance);
    m_hInstance = nullptr;
}

bool CLangDll::DoVersionStringsMatch(LPCWSTR sVer, LPCWSTR langDll) const
{
    struct Transarray
    {
        WORD wLanguageID;
        WORD wCharacterSet;
    };

    DWORD dwReserved   = 0;
    DWORD dwBufferSize = GetFileVersionInfoSize(const_cast<LPWSTR>(langDll), &dwReserved);
    if (dwBufferSize == 0)
        return false;

    auto pBuffer = std::make_unique<BYTE[]>(dwBufferSize);
    if (!pBuffer)
        return false;

    UINT        nInfoSize                       = 0;
    UINT        nFixedLength                    = 0;
    LPSTR       lpVersion                       = nullptr;
    VOID*       lpFixedPointer                  = nullptr;
    Transarray* lpTransArray                    = nullptr;
    wchar_t     strLangProductVersion[MAX_PATH] = {0};

    if (!GetFileVersionInfo(const_cast<LPWSTR>(langDll), dwReserved, dwBufferSize, pBuffer.get()))
        return false;

    VerQueryValue(pBuffer.get(), L"\\VarFileInfo\\Translation", &lpFixedPointer, &nFixedLength);
    lpTransArray = static_cast<Transarray*>(lpFixedPointer);

    swprintf_s(strLangProductVersion, L"\\StringFileInfo\\%04x%04x\\ProductVersion", lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

    VerQueryValue(pBuffer.get(), static_cast<LPWSTR>(strLangProductVersion), reinterpret_cast<LPVOID*>(&lpVersion), &nInfoSize);
    if (lpVersion && nInfoSize)
        return (wcscmp(sVer, reinterpret_cast<LPCWSTR>(lpVersion)) == 0);
    return false;
}
