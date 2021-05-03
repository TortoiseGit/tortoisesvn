// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2005-2008, 2010-2011, 2014, 2020-2021 - TortoiseSVN

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

#include "stdafx.h"
#include <shlwapi.h>
#include <objbase.h> // ATL base
#include <atlbase.h>
#include <assert.h>

#include "Register.h"

//
// Internal helper functions prototypes
//

// Set the given key and its value.
BOOL setKeyAndValue(const wchar_t* pszPath,
                    const wchar_t* szSubkey,
                    const wchar_t* szValue);

BOOL setValue(const wchar_t* szKey,
              const wchar_t* szEntry,
              const wchar_t* szValue);

// Convert a CLSID into a char string.
void CLSIDTochar(const CLSID& clsid,
                 wchar_t*     szCLSID,
                 int          length);

// Delete szKeyChild and all of its descendants.
LONG recursiveDeleteKey(HKEY hKeyParent, const wchar_t* szKeyChild);

////////////////////////////////////////////////////////
//
// Constants
//

// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39;

/////////////////////////////////////////////////////////
//
// Public function implementation
//

//
// Register the component in the registry.
//
HRESULT RegisterServer(HMODULE        hModule,        // DLL module handle
                       const CLSID&   clsid,          // Class ID
                       const wchar_t* szFriendlyName, // Friendly Name
                       const wchar_t* szVerIndProgID, // Programmatic
                       const wchar_t* szProgID,
                       const CLSID&   libid) //   Type lib ID
{
    // Get server location.
    wchar_t szModule[1024] = {0};
    ::GetModuleFileName(hModule,
                        szModule,
                        _countof(szModule) - 1);

    wcscat_s(szModule, L" /automation");
    // Convert the CLSID into a wchar_t.
    wchar_t szCLSID[CLSID_STRING_SIZE];
    CLSIDTochar(clsid, szCLSID, _countof(szCLSID));
    wchar_t szLibid[CLSID_STRING_SIZE];
    CLSIDTochar(libid, szLibid, _countof(szLibid));

    // Build the key CLSID\\{...}
    wchar_t szKey[64];
    wcscpy_s(szKey, L"CLSID\\");
    wcscat_s(szKey, szCLSID);

    // Add the CLSID to the registry.
    setKeyAndValue(szKey, nullptr, szFriendlyName);

    // Add the server filename subkey under the CLSID key.
    setKeyAndValue(szKey, L"LocalServer32", szModule);

    // Add the ProgID subkey under the CLSID key.
    setKeyAndValue(szKey, L"ProgID", szProgID);

    // Add the version-independent ProgID subkey under CLSID key.
    setKeyAndValue(szKey, L"VersionIndependentProgID", szVerIndProgID);

    // Add the typelib
    setKeyAndValue(szKey, L"TypeLib", szLibid);

    // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
    setKeyAndValue(szVerIndProgID, nullptr, szFriendlyName);
    setKeyAndValue(szVerIndProgID, L"CLSID", szCLSID);
    setKeyAndValue(szVerIndProgID, L"CurVer", szProgID);

    // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
    setKeyAndValue(szProgID, nullptr, szFriendlyName);
    setKeyAndValue(szProgID, L"CLSID", szCLSID);

    // add TypeLib keys
    wcscpy_s(szKey, L"TypeLib\\");
    wcscat_s(szKey, szLibid);

    // Add the CLSID to the registry.
    setKeyAndValue(szKey, nullptr, nullptr);
    wcscat_s(szKey, L"\\1.0");
    setKeyAndValue(szKey, nullptr, szFriendlyName);
    wcscat_s(szKey, L"\\0");
    setKeyAndValue(szKey, nullptr, nullptr);
    wcscat_s(szKey, L"\\win32");
    setKeyAndValue(szKey, nullptr, szModule);

    return S_OK;
}

void RegisterInterface(HMODULE        hModule,        // DLL module handle
                       const CLSID&   clsid,          // Class ID
                       const wchar_t* szFriendlyName, // Friendly Name
                       const CLSID&   libid,
                       const IID&     iid)
{
    // Get server location.
    wchar_t szModule[512] = {0};
    ::GetModuleFileName(hModule,
                        szModule,
                        _countof(szModule) - 1);

    // Convert the CLSID into a wchar_t.
    wchar_t szCLSID[CLSID_STRING_SIZE];
    CLSIDTochar(clsid, szCLSID, _countof(szCLSID));
    wchar_t szLibid[CLSID_STRING_SIZE];
    CLSIDTochar(libid, szLibid, _countof(szCLSID));
    wchar_t szIid[CLSID_STRING_SIZE];
    CLSIDTochar(iid, szIid, _countof(szCLSID));

    // Build the key Interface\\{...}
    wchar_t szKey[64];
    wcscpy_s(szKey, L"Interface\\");
    wcscat_s(szKey, szIid);

    // Add the value to the registry.
    setKeyAndValue(szKey, nullptr, szFriendlyName);

    wchar_t szKey2[MAX_PATH] = {0};
    wcscpy_s(szKey2, szKey);
    wcscat_s(szKey2, L"\\ProxyStubClsID");
    // Add the server filename subkey under the IID key.
    setKeyAndValue(szKey2, nullptr, L"{00020424-0000-0000-C000-000000000046}"); //IUnknown

    wcscpy_s(szKey2, szKey);
    wcscat_s(szKey2, L"\\ProxyStubClsID32");
    // Add the server filename subkey under the IID key.
    setKeyAndValue(szKey2, nullptr, L"{00020424-0000-0000-C000-000000000046}"); //IUnknown

    wcscpy_s(szKey2, szKey);
    wcscat_s(szKey2, L"\\TypeLib");
    // Add the server filename subkey under the CLSID key.
    setKeyAndValue(szKey2, nullptr, szLibid);

    setValue(szKey2, L"Version", L"1.0");
}

void UnregisterInterface(const IID& iid)
{
    wchar_t szIID[CLSID_STRING_SIZE];
    CLSIDTochar(iid, szIID, _countof(szIID));

    // Build the key Interface\\{...}
    wchar_t szKey[64];
    wcscpy_s(szKey, L"Interface\\");
    wcscat_s(szKey, szIID);

    recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(const CLSID&   clsid,          // Class ID
                      const wchar_t* szVerIndProgID, // Programmatic
                      const wchar_t* szProgID,
                      const CLSID&   libid) //   Type lib ID
{
    // Convert the CLSID into a wchar_t.
    wchar_t szCLSID[CLSID_STRING_SIZE];
    CLSIDTochar(clsid, szCLSID, _countof(szCLSID));

    // Build the key CLSID\\{...}
    wchar_t szKey[64];
    wcscpy_s(szKey, L"CLSID\\");
    wcscat_s(szKey, szCLSID);

    // Delete the CLSID Key - CLSID\{...}
    LONG lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
    assert((lResult == ERROR_SUCCESS) ||
           (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

    // Delete the version-independent ProgID Key.
    lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID);
    assert((lResult == ERROR_SUCCESS) ||
           (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

    // Delete the ProgID key.
    lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID);
    assert((lResult == ERROR_SUCCESS) ||
           (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

    wchar_t szLibid[CLSID_STRING_SIZE];
    CLSIDTochar(libid, szLibid, _countof(szLibid));

    wcscpy_s(szKey, L"TypeLib\\");
    wcscat_s(szKey, szLibid);

    // Delete the TypeLib Key - LIBID\{...}
    lResult = recursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);
    assert((lResult == ERROR_SUCCESS) ||
           (lResult == ERROR_FILE_NOT_FOUND)); // Subkey may not exist.

    return S_OK;
}

///////////////////////////////////////////////////////////
//
// Internal helper functions
//

// Convert a CLSID to a wchar_t string.
void CLSIDTochar(const CLSID& clsid,
                 wchar_t*     szCLSID,
                 int          length)
{
    assert(length >= CLSID_STRING_SIZE);
    // Get CLSID
    LPOLESTR wszCLSID = nullptr;
    StringFromCLSID(clsid, &wszCLSID);

    // Covert from wide characters to non-wide.
    wcscpy_s(szCLSID, length, wszCLSID);

    // Free memory.
    CoTaskMemFree(wszCLSID);
}

//
// Delete a key and all of its descendants.
//
LONG recursiveDeleteKey(HKEY           hKeyParent,   // Parent of key to delete
                        const wchar_t* lpszKeyChild) // Key to delete
{
    // Open the child.
    HKEY hKeyChild;
    LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild, 0,
                             KEY_ALL_ACCESS, &hKeyChild);
    if (lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    // Enumerate all of the descendants of this child.
    FILETIME time;
    wchar_t  szBuffer[256];
    DWORD    dwSize = _countof(szBuffer);
    while (RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, nullptr,
                        nullptr, nullptr, &time) == S_OK)
    {
        // Delete the descendants of this child.
        lRes = recursiveDeleteKey(hKeyChild, szBuffer);
        if (lRes != ERROR_SUCCESS)
        {
            // Cleanup before exiting.
            RegCloseKey(hKeyChild);
            return lRes;
        }
        dwSize = _countof(szBuffer);
    }

    // Close the child.
    RegCloseKey(hKeyChild);

    // Delete this child.
    return RegDeleteKey(hKeyParent, lpszKeyChild);
}

//
// Create a key and set its value.
//   - This helper function was borrowed and modified from
//     Kraig Brockschmidt's book Inside OLE.
//
BOOL setKeyAndValue(const wchar_t* szKey,
                    const wchar_t* szSubkey,
                    const wchar_t* szValue)
{
    HKEY    hKey;
    wchar_t szKeyBuf[1024];

    // Copy key name into buffer.
    wcscpy_s(szKeyBuf, szKey);

    // Add subkey name to buffer.
    if (szSubkey != nullptr)
    {
        wcscat_s(szKeyBuf, L"\\");
        wcscat_s(szKeyBuf, szSubkey);
    }

    // Create and open key and subkey.
    long lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                  szKeyBuf,
                                  0, nullptr, REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS, nullptr,
                                  &hKey, nullptr);
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Set the Value.
    if (szValue != nullptr)
    {
        RegSetValueEx(hKey, nullptr, 0, REG_SZ,
                      reinterpret_cast<BYTE*>(const_cast<wchar_t*>(szValue)),
                      static_cast<DWORD>((1 + wcslen(szValue)) * sizeof(wchar_t)));
    }

    RegCloseKey(hKey);
    return TRUE;
}

BOOL setValue(const wchar_t* szKey,
              const wchar_t* szEntry,
              const wchar_t* szValue)
{
    HKEY hKey;

    // Create and open key and subkey.
    long lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                                szKey,
                                0,
                                KEY_ALL_ACCESS,
                                &hKey);
    if (lResult != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Set the Value.
    if (szValue != nullptr)
    {
        RegSetValueEx(hKey, szEntry, 0, REG_SZ,
                      reinterpret_cast<BYTE*>(const_cast<wchar_t*>(szValue)),
                      static_cast<DWORD>((1 + wcslen(szValue)) * sizeof(wchar_t)));
    }

    RegCloseKey(hKey);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// TypeLib registration

HRESULT LoadTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib)
{
    ATLASSERT(pbstrPath != nullptr && ppTypeLib != nullptr);
    if (pbstrPath == nullptr || ppTypeLib == nullptr)
        return E_POINTER;

    *pbstrPath = nullptr;
    *ppTypeLib = nullptr;

    USES_CONVERSION;
    ATLASSERT(hInstTypeLib != nullptr);
    wchar_t szModule[MAX_PATH + 10] = {0};

    DWORD dwFLen = ::GetModuleFileName(hInstTypeLib, szModule, MAX_PATH);
    if (dwFLen == 0)
        return HRESULT_FROM_WIN32(::GetLastError());
    else if (dwFLen == MAX_PATH)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    // get the extension pointer in case of fail
    LPWSTR lpszExt = ::PathFindExtension(szModule);

    if (lpszIndex != nullptr)
        lstrcat(szModule, OLE2CT(lpszIndex));
    LPOLESTR lpszModule = T2OLE(szModule);
    HRESULT  hr         = ::LoadTypeLib(lpszModule, ppTypeLib);
    if (FAILED(hr))
    {
        // typelib not in module, try <module>.tlb instead
        lstrcpy(lpszExt, L".tlb");
        lpszModule = T2OLE(szModule);
        hr         = ::LoadTypeLib(lpszModule, ppTypeLib);
    }
    if (SUCCEEDED(hr))
    {
        *pbstrPath = ::SysAllocString(lpszModule);
        if (*pbstrPath == nullptr)
            hr = E_OUTOFMEMORY;
    }
    return hr;
}

static inline UINT WINAPI GetDirLen(LPCOLESTR lpszPathName)
{
    ATLASSERT(lpszPathName != nullptr);

    // always capture the complete file name including extension (if present)
    LPCOLESTR lpszTemp = lpszPathName;
    for (LPCOLESTR lpsz = lpszPathName; *lpsz != NULL;)
    {
        LPCOLESTR lp = CharNextW(lpsz);
        // remember last directory/drive separator
        if (*lpsz == OLESTR('\\') || *lpsz == OLESTR('/') || *lpsz == OLESTR(':'))
            lpszTemp = lp;
        lpsz = lp;
    }

    return static_cast<UINT>(lpszTemp - lpszPathName);
}

HRESULT RegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
    CComBSTR          bstrPath;
    CComPtr<ITypeLib> pTypeLib;
    HRESULT           hr = LoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
    if (SUCCEEDED(hr))
    {
        OLECHAR szDir[MAX_PATH] = {0};
        ocscpy_s(szDir, _countof(szDir), bstrPath);
        // If index is specified remove it from the path
        if (lpszIndex != nullptr)
        {
            size_t nLenPath  = ocslen(szDir);
            size_t nLenIndex = ocslen(lpszIndex);
            if (memcmp(szDir + nLenPath - nLenIndex, lpszIndex, nLenIndex) == 0)
                szDir[nLenPath - nLenIndex] = 0;
        }
        szDir[GetDirLen(szDir)] = 0;
        hr                      = ::RegisterTypeLib(pTypeLib, bstrPath, szDir);
    }
    return hr;
}

HRESULT UnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
    CComBSTR          bstrPath;
    CComPtr<ITypeLib> pTypeLib;
    HRESULT           hr = LoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
    if (SUCCEEDED(hr))
    {
        TLIBATTR* ptla = nullptr;
        hr             = pTypeLib->GetLibAttr(&ptla);
        if ((SUCCEEDED(hr)) && (ptla != nullptr))
        {
            hr = ::UnRegisterTypeLib(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);
            pTypeLib->ReleaseTLibAttr(ptla);
        }
    }
    return hr;
}
