// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012, 2014-2015, 2021-2022 - TortoiseSVN

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
#include "Guids.h"
#include "ShellExtClassFactory.h"
#include "ShellObjects.h"
#include "SVNAdminDir.h"
#include "svn_dso.h"

volatile LONG      g_cRefThisDll = 0;       ///< reference count of this DLL.
HINSTANCE          g_hModThisDll = nullptr; ///< handle to this DLL itself.
int                g_cAprInit    = 0;
ShellCache         g_shellCache; ///< caching of registry entries, ...
DWORD              g_langId;
ULONGLONG          g_langTimeout = 0;
HINSTANCE          g_hResInst    = nullptr;
std::wstring       g_filePath;
svn_wc_status_kind g_fileStatus      = svn_wc_status_none; ///< holds the corresponding status to the file/dir above
bool               g_readOnlyOverlay = false;
bool               g_lockedOverlay   = false;

bool                g_normalOvlLoaded      = false;
bool                g_modifiedOvlLoaded    = false;
bool                g_conflictedOvlLoaded  = false;
bool                g_readonlyOvlLoaded    = false;
bool                g_deletedOvlLoaded     = false;
bool                g_lockedOvlLoaded      = false;
bool                g_addedOvlLoaded       = false;
bool                g_ignoredOvlLoaded     = false;
bool                g_unversionedOvlLoaded = false;
CComCriticalSection g_csGlobalComGuard;

LPCWSTR g_menuIDString = L"TortoiseSVN";

ShellObjects g_shellObjects;

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern "C" int APIENTRY
    DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /* lpReserved */)
{
    // NOTE: Do *NOT* call apr_initialize() or apr_terminate() here in DllMain(),
    // because those functions call LoadLibrary() indirectly through malloc().
    // And LoadLibrary() inside DllMain() is not allowed and can lead to unexpected
    // behavior and even may create dependency loops in the dll load order.
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        if (g_hModThisDll == nullptr)
        {
            g_csGlobalComGuard.Init();
        }

        // Extension DLL one-time initialization
        g_hModThisDll = hInstance;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // do not clean up memory here:
        // if an application doesn't release all COM objects
        // but still unloads the dll, cleaning up ourselves
        // will lead to crashes.
        // better to leak some memory than to crash other apps.
        // sometimes an application doesn't release all COM objects
        // but still unloads the dll.
        // in that case, we do it ourselves
        g_csGlobalComGuard.Term();
    }
    return 1; // ok
}

STDAPI DllCanUnloadNow()
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    if (ppvOut == nullptr)
        return E_POINTER;
    *ppvOut = nullptr;

    FileState state = FileStateInvalid;
    if (IsEqualIID(rclsid, CLSID_TortoiseSVN_UPTODATE))
        state = FileStateVersioned;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_MODIFIED))
        state = FileStateModified;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_CONFLICTING))
        state = FileStateConflict;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_UNCONTROLLED))
        state = FileStateUncontrolled;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_DROPHANDLER))
        state = FileStateDropHandler;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_DELETED))
        state = FileStateDeleted;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_READONLY))
        state = FileStateReadOnly;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_LOCKED))
        state = FileStateLockedOverlay;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_ADDED))
        state = FileStateAddedOverlay;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_IGNORED))
        state = FileStateIgnoredOverlay;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_UNVERSIONED))
        state = FileStateUnversionedOverlay;
    else if (IsEqualIID(rclsid, CLSID_TortoiseSVN_EXPLORERCOMMAND))
        state = FileStateVersioned;

    if (state != FileStateInvalid)
    {
        if (apr_initialize() == APR_SUCCESS)
        {
            svn_dso_initialize2();
            g_SVNAdminDir.Init();
            g_cAprInit++;

            CShellExtClassFactory *pcf = new (std::nothrow) CShellExtClassFactory(state);
            if (pcf == nullptr)
                return E_OUTOFMEMORY;
            // refcount currently set to 0
            const HRESULT hr = pcf->QueryInterface(riid, ppvOut);
            if (FAILED(hr))
                delete pcf;
            return hr;
        }
        return E_OUTOFMEMORY;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}
