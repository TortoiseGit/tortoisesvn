// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2016, 2018, 2021 - TortoiseSVN

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

// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
// ReSharper disable once CppUnusedIncludeDirective
#include <initguid.h>
#include "Guids.h"

#include "ShellExt.h"
#include "ShellObjects.h"
#include "../version.h"
#include "libintl.h"
#undef swprintf

extern ShellObjects g_shellObjects;

// *********************** CShellExt *************************
CShellExt::CShellExt(FileState state)
    : m_state(state)
    , itemStates(0)
    , itemStatesFolder(0)
    , space(0)
    , columnRev(0)
    , fileStatus(svn_wc_status_none)
    , regDiffLater(L"Software\\TortoiseMerge\\DiffLater", L"")
{
    m_cRef = 0L;
    InterlockedIncrement(&g_cRefThisDll);

    {
        AutoLocker lock(g_csGlobalComGuard);
        g_shellObjects.Insert(this);
    }

    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES};
    InitCommonControlsEx(&used);
    LoadLangDll();
}

CShellExt::~CShellExt()
{
    AutoLocker lock(g_csGlobalComGuard);
    InterlockedDecrement(&g_cRefThisDll);
    g_shellObjects.Erase(this);
}

void LoadLangDll()
{
    if ((g_langId != g_shellCache.GetLangID()) && ((g_langTimeout == 0) || (g_langTimeout < GetTickCount64())))
    {
        g_langId                        = g_shellCache.GetLangID();
        DWORD     langId                = g_langId;
        wchar_t   langDll[MAX_PATH * 4] = {0};
        HINSTANCE hInst                 = nullptr;
        wchar_t   langDir[MAX_PATH]     = {0};
        char      langDirA[MAX_PATH]    = {0};
        if (GetModuleFileName(g_hModThisDll, langDir, _countof(langDir)) == 0)
            return;
        if (GetModuleFileNameA(g_hModThisDll, langDirA, _countof(langDirA)) == 0)
            return;
        wchar_t* dirpoint  = wcsrchr(langDir, '\\');
        char*    dirpointA = strrchr(langDirA, '\\');
        if (dirpoint)
            *dirpoint = 0;
        if (dirpointA)
            *dirpointA = 0;
        dirpoint  = wcsrchr(langDir, '\\');
        dirpointA = strrchr(langDirA, '\\');
        if (dirpoint)
            *dirpoint = 0;
        if (dirpointA)
            *dirpointA = 0;
        strcat_s(langDirA, "\\Languages");
#ifdef ENABLE_NLS
        bindtextdomain("subversion", langDirA);
#endif
        BOOL bIsWow = FALSE;
        IsWow64Process(GetCurrentProcess(), &bIsWow);

        do
        {
            if (bIsWow)
                swprintf_s(langDll, L"%s\\Languages\\TortoiseProc32%lu.dll", langDir, langId);
            else
                swprintf_s(langDll, L"%s\\Languages\\TortoiseProc%lu.dll", langDir, langId);
            BOOL versionMatch = TRUE;

            struct Transarray
            {
                WORD wLanguageID;
                WORD wCharacterSet;
            };

            DWORD dwReserved   = 0;
            auto  dwBufferSize = GetFileVersionInfoSize(static_cast<LPWSTR>(langDll), &dwReserved);

            if (dwBufferSize > 0)
            {
                auto buffer = std::make_unique<char[]>(dwBufferSize);

                if (buffer.get() != nullptr)
                {
                    UINT  nInfoSize    = 0;
                    UINT  nFixedLength = 0;
                    LPSTR lpVersion    = nullptr;
                    VOID* lpFixedPointer;

                    if (GetFileVersionInfo(static_cast<LPWSTR>(langDll),
                                           dwReserved,
                                           dwBufferSize,
                                           buffer.get()))
                    {
                        // Query the current language
                        if (VerQueryValue(buffer.get(),
                                          L"\\VarFileInfo\\Translation",
                                          &lpFixedPointer,
                                          &nFixedLength))
                        {
                            Transarray* lpTransArray = static_cast<Transarray*>(lpFixedPointer);

                            wchar_t strLangProductVersion[MAX_PATH];
                            swprintf_s(strLangProductVersion, L"\\StringFileInfo\\%04x%04x\\ProductVersion",
                                       lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

                            if (VerQueryValue(buffer.get(),
                                              static_cast<LPWSTR>(strLangProductVersion),
                                              reinterpret_cast<LPVOID*>(&lpVersion),
                                              &nInfoSize))
                            {
                                versionMatch = (wcscmp(reinterpret_cast<LPCWSTR>(lpVersion), _T(STRPRODUCTVER)) == 0);
                            }
                        }
                    }
                } // if (buffer.get() != 0)
            }     // if (dwBufferSize > 0)
            else
                versionMatch = FALSE;

            if (versionMatch)
                hInst = LoadLibrary(langDll);
            if (hInst != nullptr)
            {
                if (g_hResInst != g_hModThisDll)
                    FreeLibrary(g_hResInst);
                g_hResInst = hInst;
            }
            else
            {
                DWORD lid = SUBLANGID(langId);
                lid--;
                if (lid > 0)
                {
                    langId = MAKELANGID(PRIMARYLANGID(langId), lid);
                }
                else
                    langId = 0;
            }
        } while ((hInst == nullptr) && (langId != 0));
        if (hInst == nullptr)
        {
            // either the dll for the selected language is not present, or
            // it is the wrong version.
            // fall back to English and set a timeout so we don't retry
            // to load the language dll too often
            if (g_hResInst != g_hModThisDll)
                FreeLibrary(g_hResInst);
            g_hResInst = g_hModThisDll;
            g_langId   = 1033;
            // set a timeout of 10 seconds
            if (g_shellCache.GetLangID() != 1033)
                g_langTimeout = GetTickCount64() + 10000;
        }
        else
            g_langTimeout = 0;
    } // if (g_langId != g_ShellCache.GetLangID())
}

std::wstring GetAppDirectory()
{
    std::wstring path;
    DWORD        len       = 0;
    DWORD        bufferLen = MAX_PATH; // MAX_PATH is not the limit here!
    do
    {
        bufferLen += MAX_PATH; // MAX_PATH is not the limit here!
        auto pBuf = std::make_unique<wchar_t[]>(bufferLen);
        len       = GetModuleFileName(g_hModThisDll, pBuf.get(), bufferLen);
        path      = std::wstring(pBuf.get(), len);
    } while (len == bufferLen);
    path = path.substr(0, path.rfind('\\') + 1);

    return path;
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (ppv == nullptr)
        return E_POINTER;

    *ppv = nullptr;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = static_cast<LPSHELLEXTINIT>(this);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppv = static_cast<LPCONTEXTMENU>(this);
    }
    else if (IsEqualIID(riid, IID_IContextMenu2))
    {
        *ppv = static_cast<LPCONTEXTMENU2>(this);
    }
    else if (IsEqualIID(riid, IID_IContextMenu3))
    {
        *ppv = static_cast<LPCONTEXTMENU3>(this);
    }
    else if (IsEqualIID(riid, IID_IShellIconOverlayIdentifier))
    {
        *ppv = static_cast<IShellIconOverlayIdentifier*>(this);
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppv = static_cast<LPSHELLPROPSHEETEXT>(this);
    }
    else if (IsEqualIID(riid, IID_IColumnProvider))
    {
        *ppv = static_cast<IColumnProvider*>(this);
    }
    else if (IsEqualIID(riid, IID_IShellCopyHook))
    {
        *ppv = static_cast<ICopyHook*>(this);
    }
    else if (IsEqualIID(riid, IID_IExplorerCommand))
    {
        *ppv = static_cast<IExplorerCommand*>(this);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppv = static_cast<IObjectWithSite*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CShellExt::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CShellExt::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;

    return 0L;
}

// IPersistFile members
STDMETHODIMP CShellExt::GetClassID(CLSID* pclsid)
{
    if (pclsid == nullptr)
        return E_POINTER;
    *pclsid = CLSID_TortoiseSVN_UNCONTROLLED;
    return S_OK;
}

STDMETHODIMP CShellExt::Load(LPCOLESTR /*pszFileName*/, DWORD /*dwMode*/)
{
    return S_OK;
}

// ICopyHook member
UINT __stdcall CShellExt::CopyCallback(HWND /*hWnd*/, UINT wFunc, UINT /*wFlags*/, LPCWSTR pszSrcFile, DWORD /*dwSrcAttribs*/, LPCWSTR /*pszDestFile*/, DWORD /*dwDestAttribs*/)
{
    switch (wFunc)
    {
        case FO_MOVE:
        case FO_DELETE:
        case FO_RENAME:
            if (pszSrcFile && pszSrcFile[0])
            {
                if (g_shellCache.IsPathAllowed(pszSrcFile))
                    m_remoteCacheLink.ReleaseLockForPath(CTSVNPath(pszSrcFile));
            }
            break;
        default:
            break;
    }

    // we could now wait a little bit to give the cache time to release the handles.
    // but the explorer/shell already retries any action for about two seconds
    // if it first fails. So if the cache hasn't released the handle yet, the explorer
    // will retry anyway, so we just leave here immediately.
    return IDYES;
}

// IObjectWithSite
HRESULT __stdcall CShellExt::SetSite(IUnknown* pUnkSite)
{
    m_site = pUnkSite;
    return S_OK;
}

HRESULT __stdcall CShellExt::GetSite(REFIID riid, void** ppvSite)
{
    return m_site.CopyTo(riid, ppvSite);
}
