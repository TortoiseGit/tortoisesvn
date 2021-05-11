// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009, 2012-2014, 2018, 2021 - TortoiseSVN

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
#include <Windows.h>
#include "Debug.h"

const HINSTANCE NIL = reinterpret_cast<HINSTANCE>(static_cast<char *>(nullptr) - 1);

static HINSTANCE hInst = nullptr;

static HINSTANCE          hTortoiseSVN       = nullptr;
static LPFNGETCLASSOBJECT pDllGetClassObject = nullptr;
static LPFNCANUNLOADNOW   pDllCanUnloadNow   = nullptr;

static wchar_t debugDllPath[MAX_PATH] = {0};

static BOOL DebugActive()
{
    static const WCHAR tsvnRootKey[]         = L"Software\\TortoiseSVN";
    static const WCHAR debugShellValue[]     = L"DebugShell";
    static const WCHAR debugShellPathValue[] = L"DebugShellPath";

    DWORD bDebug = 0;

    HKEY  hKey   = HKEY_CURRENT_USER;
    LONG  result = ERROR;
    DWORD type   = REG_DWORD;
    DWORD len    = sizeof(DWORD);

    BOOL bDebugActive = FALSE;

    TRACE(L"DebugActive() - Enter\n");

    if (IsDebuggerPresent())
    {
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, tsvnRootKey, 0, KEY_READ, &hKey);
        if (result == ERROR_SUCCESS)
        {
            result = RegQueryValueEx(hKey, debugShellValue, nullptr, &type, reinterpret_cast<BYTE *>(&bDebug), &len);
            if ((result == ERROR_SUCCESS) && (type == REG_DWORD) && (len == sizeof(DWORD)) && bDebug)
            {
                TRACE(L"DebugActive() - debug active\n");
                bDebugActive = TRUE;
                len          = sizeof(wchar_t) * MAX_PATH;
                type         = REG_SZ;
                result       = RegQueryValueEx(hKey, debugShellPathValue, nullptr, &type, reinterpret_cast<BYTE *>(debugDllPath), &len);
                if ((result == ERROR_SUCCESS) && (type == REG_SZ) && bDebug)
                {
                    TRACE(L"DebugActive() - debug path set\n");
                }
            }

            RegCloseKey(hKey);
        }
    }
    else
    {
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, tsvnRootKey, 0, KEY_READ, &hKey);
        if (result == ERROR_SUCCESS)
        {
            len    = sizeof(wchar_t) * MAX_PATH;
            type   = REG_SZ;
            result = RegQueryValueEx(hKey, debugShellPathValue, nullptr, &type, reinterpret_cast<BYTE *>(debugDllPath), &len);
            if ((result == ERROR_SUCCESS) && (type == REG_SZ) && bDebug)
            {
                TRACE(L"DebugActive() - debug path set\n");
            }

            RegCloseKey(hKey);
        }
    }

    TRACE(L"WantRealVersion() - Exit\n");
    return bDebugActive;
}

/**
 * \ingroup TortoiseShell
 * Check whether to load the full TortoiseSVN.dll or not.
 */
static BOOL WantRealVersion()
{
    static const WCHAR tsvnRootKey[]       = L"Software\\TortoiseSVN";
    static const WCHAR explorerOnlyValue[] = L"LoadDllOnlyInExplorer";

    static const WCHAR explorerEnvPath[] = L"%SystemRoot%\\explorer.exe";

    DWORD bExplorerOnly        = 0;
    WCHAR moduleName[MAX_PATH] = {0};

    HKEY  hKey = HKEY_CURRENT_USER;
    DWORD type = REG_DWORD;
    DWORD len  = sizeof(DWORD);

    BOOL bWantReal = TRUE;

    TRACE(L"WantRealVersion() - Enter\n");

    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, tsvnRootKey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
    if (result == ERROR_SUCCESS)
    {
        result = RegQueryValueEx(hKey, explorerOnlyValue, nullptr, &type, reinterpret_cast<BYTE *>(&bExplorerOnly), &len);
        if ((result == ERROR_SUCCESS) && (type == REG_DWORD) && (len == sizeof(DWORD)) && bExplorerOnly)
        {
            TRACE(L"WantRealVersion() - Explorer Only\n");

            // check if the current process is in fact the explorer
            len = GetModuleFileName(nullptr, moduleName, _countof(moduleName));
            if (len)
            {
                TRACE(L"Process is %s\n", moduleName);

                WCHAR explorerPath[MAX_PATH] = {0};
                len                          = ExpandEnvironmentStrings(explorerEnvPath, explorerPath, _countof(explorerPath));
                if (len && (len <= _countof(explorerPath)))
                {
                    TRACE(L"Explorer path is %s\n", explorerPath);
                    bWantReal = !lstrcmpi(moduleName, explorerPath);
                }

                // we also have to allow the verclsid.exe process - that process determines
                // first whether the shell is allowed to even use an extension.
                len = lstrlen(moduleName);
                if ((len > 13) && (lstrcmpi(&moduleName[len - 13], L"\\verclsid.exe") == 0))
                    bWantReal = TRUE;
            }
        }

        RegCloseKey(hKey);
    }

    TRACE(L"WantRealVersion() - Exit\n");
    return bWantReal;
}

static void LoadRealLibrary()
{
    static const char getClassObject[] = "DllGetClassObject";
    static const char canUnloadNow[]   = "DllCanUnloadNow";

    WCHAR     moduleName[MAX_PATH] = {0};
    DWORD     len                  = 0;
    HINSTANCE hUseInst             = hInst;
    debugDllPath[0]                = 0;

    if (hTortoiseSVN)
        return;

    if (!WantRealVersion())
    {
        TRACE(L"LoadRealLibrary() - Bypass\n");
        hTortoiseSVN = NIL;
        return;
    }
    // if HKCU\Software\TortoiseSVN\DebugShell is set, load the dlls from the location of the current process
    // which is for our debug purposes an instance of usually TortoiseProc. That way we can force the load
    // of the debug dlls.
    if (DebugActive())
        hUseInst = nullptr;
    len = GetModuleFileName(hUseInst, moduleName, _countof(moduleName));
    if (!len)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        hTortoiseSVN = NIL;
        return;
    }

    // truncate the string at the last '\' char
    while (len > 0)
    {
        --len;
        if (moduleName[len] == '\\')
        {
            moduleName[len] = '\0';
            break;
        }
    }
    if (len == 0)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        hTortoiseSVN = NIL;
        return;
    }
#ifdef _WIN64
    lstrcat(moduleName, L"\\TortoiseSVN.dll");
#else
    lstrcat(moduleName, L"\\TortoiseSVN32.dll");
#endif
    if (debugDllPath[0])
        lstrcpy(moduleName, debugDllPath);
    TRACE(L"LoadRealLibrary() - Load %s\n", moduleName);

    hTortoiseSVN = LoadLibraryEx(moduleName, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hTortoiseSVN)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        hTortoiseSVN = NIL;
        return;
    }

    TRACE(L"LoadRealLibrary() - Success\n");
    pDllGetClassObject = nullptr;
    pDllCanUnloadNow   = nullptr;
    pDllGetClassObject = reinterpret_cast<LPFNGETCLASSOBJECT>(GetProcAddress(hTortoiseSVN, getClassObject));
    if (pDllGetClassObject == nullptr)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        FreeLibrary(hTortoiseSVN);
        hTortoiseSVN = NIL;
        return;
    }
    pDllCanUnloadNow = reinterpret_cast<LPFNCANUNLOADNOW>(GetProcAddress(hTortoiseSVN, canUnloadNow));
    if (pDllCanUnloadNow == nullptr)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        FreeLibrary(hTortoiseSVN);
        hTortoiseSVN = NIL;
        return;
    }
}

static void UnloadRealLibrary()
{
    if (!hTortoiseSVN)
        return;

    if (hTortoiseSVN != NIL)
        FreeLibrary(hTortoiseSVN);

    hTortoiseSVN       = nullptr;
    pDllGetClassObject = nullptr;
    pDllCanUnloadNow   = nullptr;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID reserved)
{
#ifdef _DEBUG
    // If no debugger is present, then don't load the dll.
    // This prevents other apps from loading the dll and locking it.

    BOOL    bInShellTest      = FALSE;
    wchar_t buf[MAX_PATH + 1] = {0}; // MAX_PATH ok, the test really is for debugging anyway.
    DWORD   pathLength        = GetModuleFileName(nullptr, buf, MAX_PATH);

    UNREFERENCED_PARAMETER(reserved);

    if (pathLength >= 14)
    {
        if ((lstrcmpi(&buf[pathLength - 14], L"\\ShellTest.exe")) == 0)
        {
            bInShellTest = TRUE;
        }
        if ((_wcsicmp(&buf[pathLength - 13], L"\\verclsid.exe")) == 0)
        {
            bInShellTest = TRUE;
        }
    }

    if (!IsDebuggerPresent() && !bInShellTest)
    {
        TRACE(L"In debug load preventer\n");
        return FALSE;
    }
#else
    UNREFERENCED_PARAMETER(reserved);
#endif

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            hInst           = hInstance;
            debugDllPath[0] = 0;
            break;

            /*case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;*/
    }

    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE(L"DllGetClassObject() - Enter\n");

    LoadRealLibrary();
    if (!pDllGetClassObject)
    {
        if (ppv)
            *ppv = nullptr;

        TRACE(L"DllGetClassObject() - Bypass\n");
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    TRACE(L"DllGetClassObject() - Forward\n");
    return pDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
    TRACE(L"DllCanUnloadNow() - Enter\n");

    if (pDllCanUnloadNow)
    {
        TRACE(L"DllCanUnloadNow() - Forward\n");
        HRESULT result = pDllCanUnloadNow();
        if (result != S_OK)
            return result;
    }

    TRACE(L"DllCanUnloadNow() - Unload\n");
    UnloadRealLibrary();
    return S_OK;
}
