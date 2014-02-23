// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009, 2012-2014 - TortoiseSVN

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
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <Windows.h>
#include <tchar.h>
#include "Debug.h"


const HINSTANCE NIL = (HINSTANCE)((char*)(0)-1);

static HINSTANCE hInst = NULL;

static HINSTANCE hTortoiseSVN = NULL;
static LPFNGETCLASSOBJECT pDllGetClassObject = NULL;
static LPFNCANUNLOADNOW pDllCanUnloadNow = NULL;




static BOOL DebugActive(void)
{
    static const WCHAR TSVNRootKey[]=L"Software\\TortoiseSVN";
    static const WCHAR ExplorerOnlyValue[]=L"DebugShell";


    DWORD bDebug = 0;

    HKEY hKey = HKEY_CURRENT_USER;
    LONG Result = ERROR;
    DWORD Type = REG_DWORD;
    DWORD Len = sizeof(DWORD);

    BOOL bDebugActive = FALSE;


    TRACE(L"DebugActive() - Enter\n");

    if (IsDebuggerPresent())
    {
        Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TSVNRootKey, 0, KEY_READ, &hKey);
        if (Result == ERROR_SUCCESS)
        {
            Result = RegQueryValueEx(hKey, ExplorerOnlyValue, NULL, &Type, (BYTE *)&bDebug, &Len);
            if ((Result == ERROR_SUCCESS) && (Type == REG_DWORD) && (Len == sizeof(DWORD)) && bDebug)
            {
                TRACE(L"DebugActive() - debug active\n");
                bDebugActive = TRUE;
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
static BOOL WantRealVersion(void)
{
    static const WCHAR TSVNRootKey[]=L"Software\\TortoiseSVN";
    static const WCHAR ExplorerOnlyValue[]=L"LoadDllOnlyInExplorer";

    static const WCHAR ExplorerEnvPath[]=L"%SystemRoot%\\explorer.exe";


    DWORD bExplorerOnly = 0;
    WCHAR ModuleName[MAX_PATH] = {0};

    HKEY hKey = HKEY_CURRENT_USER;
    DWORD Type = REG_DWORD;
    DWORD Len = sizeof(DWORD);

    BOOL bWantReal = TRUE;


    TRACE(L"WantRealVersion() - Enter\n");

    LONG Result = RegOpenKeyEx(HKEY_CURRENT_USER, TSVNRootKey, 0, KEY_READ, &hKey);
    if (Result == ERROR_SUCCESS)
    {
        Result = RegQueryValueEx(hKey, ExplorerOnlyValue, NULL, &Type, (BYTE *)&bExplorerOnly, &Len);
        if ((Result == ERROR_SUCCESS) && (Type == REG_DWORD) && (Len == sizeof(DWORD)) && bExplorerOnly)
        {
            TRACE(L"WantRealVersion() - Explorer Only\n");

            // check if the current process is in fact the explorer
            Len = GetModuleFileName(NULL, ModuleName, _countof(ModuleName));
            if (Len)
            {
                TRACE(L"Process is %s\n", ModuleName);

                WCHAR ExplorerPath[MAX_PATH] = {0};
                Len = ExpandEnvironmentStrings(ExplorerEnvPath, ExplorerPath, _countof(ExplorerPath));
                if (Len && (Len <= _countof(ExplorerPath)))
                {
                    TRACE(L"Explorer path is %s\n", ExplorerPath);
                    bWantReal = !lstrcmpi(ModuleName, ExplorerPath);
                }

                // we also have to allow the verclsid.exe process - that process determines
                // first whether the shell is allowed to even use an extension.
                Len = lstrlen(ModuleName);
                if ((Len > 13)&&(lstrcmpi(&ModuleName[Len-13], L"\\verclsid.exe") == 0))
                    bWantReal = TRUE;
            }
        }

        RegCloseKey(hKey);
    }

    TRACE(L"WantRealVersion() - Exit\n");
    return bWantReal;
}

static void LoadRealLibrary(void)
{
    static const char GetClassObject[] = "DllGetClassObject";
    static const char CanUnloadNow[] = "DllCanUnloadNow";

    WCHAR ModuleName[MAX_PATH] = {0};
    DWORD Len = 0;
    HINSTANCE hUseInst = hInst;

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
        hUseInst = NULL;
    Len = GetModuleFileName(hUseInst, ModuleName, _countof(ModuleName));
    if (!Len)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        hTortoiseSVN = NIL;
        return;
    }

    // truncate the string at the last '\' char
    while(Len > 0)
    {
        --Len;
        if (ModuleName[Len] == '\\')
        {
            ModuleName[Len] = '\0';
            break;
        }
    }
    if (Len == 0)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        hTortoiseSVN = NIL;
        return;
    }
#ifdef _WIN64
    lstrcat(ModuleName, L"\\TortoiseSVN.dll");
#else
    lstrcat(ModuleName, L"\\TortoiseSVN32.dll");
#endif
    TRACE(L"LoadRealLibrary() - Load %s\n", ModuleName);

    hTortoiseSVN = LoadLibraryEx(ModuleName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hTortoiseSVN)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        hTortoiseSVN = NIL;
        return;
    }

    TRACE(L"LoadRealLibrary() - Success\n");
    pDllGetClassObject = NULL;
    pDllCanUnloadNow = NULL;
    pDllGetClassObject = (LPFNGETCLASSOBJECT)GetProcAddress(hTortoiseSVN, GetClassObject);
    if (pDllGetClassObject == NULL)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        FreeLibrary(hTortoiseSVN);
        hTortoiseSVN = NIL;
        return;
    }
    pDllCanUnloadNow = (LPFNCANUNLOADNOW)GetProcAddress(hTortoiseSVN, CanUnloadNow);
    if (pDllCanUnloadNow == NULL)
    {
        TRACE(L"LoadRealLibrary() - Fail\n");
        FreeLibrary(hTortoiseSVN);
        hTortoiseSVN = NIL;
        return;
    }
}

static void UnloadRealLibrary(void)
{
    if (!hTortoiseSVN)
        return;

    if (hTortoiseSVN != NIL)
        FreeLibrary(hTortoiseSVN);

    hTortoiseSVN = NULL;
    pDllGetClassObject = NULL;
    pDllCanUnloadNow = NULL;
}


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
#ifdef _DEBUG
    // If no debugger is present, then don't load the dll.
    // This prevents other apps from loading the dll and locking it.

    BOOL bInShellTest = FALSE;
    TCHAR buf[MAX_PATH + 1] = { 0 };       // MAX_PATH ok, the test really is for debugging anyway.
    DWORD pathLength = GetModuleFileName(NULL, buf, MAX_PATH);

    UNREFERENCED_PARAMETER(Reserved);

    if (pathLength >= 14)
    {
        if ((lstrcmpi(&buf[pathLength-14], L"\\ShellTest.exe")) == 0)
        {
            bInShellTest = TRUE;
        }
        if ((_wcsicmp(&buf[pathLength-13], L"\\verclsid.exe")) == 0)
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
    UNREFERENCED_PARAMETER(Reserved);
#endif


    switch(Reason)
    {
    case DLL_PROCESS_ATTACH:
        hInst = hInstance;
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
            *ppv = NULL;

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
        HRESULT Result = pDllCanUnloadNow();
        if (Result != S_OK)
            return Result;
    }

    TRACE(L"DllCanUnloadNow() - Unload\n");
    UnloadRealLibrary();
    return S_OK;
}
