// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
    : m_crasher(L"TortoiseSVN", false)
    , regDiffLater(L"Software\\TortoiseMerge\\DiffLater", L"")
{
    m_State = state;

    m_cRef = 0L;
    InterlockedIncrement(&g_cRefThisDll);

    {
        AutoLocker lock(g_csGlobalCOMGuard);
        g_shellObjects.Insert(this);
    }

    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
            ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES
    };
    InitCommonControlsEx(&used);
    LoadLangDll();
}

CShellExt::~CShellExt()
{
    AutoLocker lock(g_csGlobalCOMGuard);
    InterlockedDecrement(&g_cRefThisDll);
    g_shellObjects.Erase(this);
}

void LoadLangDll()
{
    if ((g_langid != g_ShellCache.GetLangID())&&((g_langTimeout == 0)||(g_langTimeout < GetTickCount64())))
    {
        g_langid = g_ShellCache.GetLangID();
        DWORD langId = g_langid;
        TCHAR langDll[MAX_PATH * 4] = { 0 };
        HINSTANCE hInst = NULL;
        TCHAR langdir[MAX_PATH] = {0};
        char langdirA[MAX_PATH] = {0};
        if (GetModuleFileName(g_hmodThisDll, langdir, _countof(langdir))==0)
            return;
        if (GetModuleFileNameA(g_hmodThisDll, langdirA, _countof(langdirA))==0)
            return;
        TCHAR * dirpoint = _tcsrchr(langdir, '\\');
        char * dirpointA = strrchr(langdirA, '\\');
        if (dirpoint)
            *dirpoint = 0;
        if (dirpointA)
            *dirpointA = 0;
        dirpoint = _tcsrchr(langdir, '\\');
        dirpointA = strrchr(langdirA, '\\');
        if (dirpoint)
            *dirpoint = 0;
        if (dirpointA)
            *dirpointA = 0;
        strcat_s(langdirA, "\\Languages");
        bindtextdomain ("subversion", langdirA);

        BOOL bIsWow = FALSE;
        IsWow64Process(GetCurrentProcess(), &bIsWow);

        do
        {
            if (bIsWow)
                _stprintf_s(langDll, _T("%s\\Languages\\TortoiseProc32%lu.dll"), langdir, langId);
            else
                _stprintf_s(langDll, _T("%s\\Languages\\TortoiseProc%lu.dll"), langdir, langId);
            BOOL versionmatch = TRUE;

            struct TRANSARRAY
            {
                WORD wLanguageID;
                WORD wCharacterSet;
            };

            DWORD dwReserved,dwBufferSize;
            dwBufferSize = GetFileVersionInfoSize((LPTSTR)langDll,&dwReserved);

            if (dwBufferSize > 0)
            {
                std::unique_ptr<char[]> buffer(new char[dwBufferSize]);

                if (buffer.get() != 0)
                {
                    UINT        nInfoSize = 0;
                    UINT        nFixedLength = 0;
                    LPSTR       lpVersion = NULL;
                    VOID*       lpFixedPointer;

                    if (GetFileVersionInfo((LPTSTR)langDll,
                        dwReserved,
                        dwBufferSize,
                        buffer.get()))
                    {
                        // Query the current language
                        if (VerQueryValue( buffer.get(),
                            _T("\\VarFileInfo\\Translation"),
                            &lpFixedPointer,
                            &nFixedLength))
                        {
                            TRANSARRAY* lpTransArray = (TRANSARRAY*) lpFixedPointer;

                            TCHAR       strLangProductVersion[MAX_PATH];
                            _stprintf_s(strLangProductVersion, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
                                lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

                            if (VerQueryValue(buffer.get(),
                                (LPTSTR)strLangProductVersion,
                                (LPVOID *)&lpVersion,
                                &nInfoSize))
                            {
                                versionmatch = (_tcscmp((LPCTSTR)lpVersion, _T(STRPRODUCTVER)) == 0);
                            }

                        }
                    }
                } // if (buffer.get() != 0)
            } // if (dwBufferSize > 0)
            else
                versionmatch = FALSE;

            if (versionmatch)
                hInst = LoadLibrary(langDll);
            if (hInst != NULL)
            {
                if (g_hResInst != g_hmodThisDll)
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
        } while ((hInst == NULL) && (langId != 0));
        if (hInst == NULL)
        {
            // either the dll for the selected language is not present, or
            // it is the wrong version.
            // fall back to English and set a timeout so we don't retry
            // to load the language dll too often
            if (g_hResInst != g_hmodThisDll)
                FreeLibrary(g_hResInst);
            g_hResInst = g_hmodThisDll;
            g_langid = 1033;
            // set a timeout of 10 seconds
            if (g_ShellCache.GetLangID() != 1033)
                g_langTimeout = GetTickCount64() + 10000;
        }
        else
            g_langTimeout = 0;
    } // if (g_langid != g_ShellCache.GetLangID())
}

tstring GetAppDirectory()
{
    tstring path;
    DWORD len = 0;
    DWORD bufferlen = MAX_PATH;     // MAX_PATH is not the limit here!
    do
    {
        bufferlen += MAX_PATH;      // MAX_PATH is not the limit here!
        std::unique_ptr<TCHAR[]> pBuf(new TCHAR[bufferlen]);
        len = GetModuleFileName(g_hmodThisDll, pBuf.get(), bufferlen);
        path = tstring(pBuf.get(), len);
    } while(len == bufferlen);
    path = path.substr(0, path.rfind('\\') + 1);

    return path;
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    if(ppv == 0)
        return E_POINTER;

    *ppv = NULL;

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
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
}

// IPersistFile members
STDMETHODIMP CShellExt::GetClassID(CLSID *pclsid)
{
    if(pclsid == 0)
        return E_POINTER;
    *pclsid = CLSID_TortoiseSVN_UNCONTROLLED;
    return S_OK;
}

STDMETHODIMP CShellExt::Load(LPCOLESTR /*pszFileName*/, DWORD /*dwMode*/)
{
    return S_OK;
}

// ICopyHook member
UINT __stdcall CShellExt::CopyCallback(HWND hWnd, UINT wFunc, UINT wFlags, LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs)
{
    __try
    {
        return CopyCallback_Wrap(hWnd, wFunc, wFlags, pszSrcFile, dwSrcAttribs, pszDestFile, dwDestAttribs);
    }
    __except(GetExceptionCode() == 0xc0000194/*EXCEPTION_POSSIBLE_DEADLOCK*/ ? EXCEPTION_CONTINUE_EXECUTION : CCrashReport::Instance().SendReport(GetExceptionInformation()))
    {
    }
    return IDYES;
}

UINT __stdcall CShellExt::CopyCallback_Wrap(HWND /*hWnd*/, UINT wFunc, UINT /*wFlags*/, LPCTSTR pszSrcFile, DWORD /*dwSrcAttribs*/, LPCTSTR /*pszDestFile*/, DWORD /*dwDestAttribs*/)
{
    switch (wFunc)
    {
    case FO_MOVE:
    case FO_DELETE:
    case FO_RENAME:
        if (pszSrcFile && pszSrcFile[0])
        {
            if (g_ShellCache.IsPathAllowed(pszSrcFile))
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
