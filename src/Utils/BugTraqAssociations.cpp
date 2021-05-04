// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2009, 2012-2014, 2021 - TortoiseSVN

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
#include "BugTraqAssociations.h"

#include <initguid.h>

// {3494FA92-B139-4730-9591-01135D5E7831}
// ReSharper disable once CppInconsistentNaming
DEFINE_GUID(CATID_BugTraqProvider,
            0x3494fa92, 0xb139, 0x4730, 0x95, 0x91, 0x1, 0x13, 0x5d, 0x5e, 0x78, 0x31);

#define BUGTRAQ_ASSOCIATIONS_REGPATH L"Software\\TortoiseSVN\\BugTraq Associations"

CBugTraqAssociations::CBugTraqAssociations()
    : pProjectProvider(nullptr)
{
}

CBugTraqAssociations::~CBugTraqAssociations()
{
    for (auto it = m_inner.begin(); it != m_inner.end(); ++it)
        delete *it;
    delete pProjectProvider;
}

void CBugTraqAssociations::Load(LPCWSTR uuid /* = NULL */, LPCWSTR params /* = NULL */)
{
    HKEY hk;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH, 0, KEY_READ, &hk) != ERROR_SUCCESS)
    {
        if (uuid)
            m_providerUuid = uuid;
        if (params)
            providerParams = params;
        return;
    }

    for (DWORD dwIndex = 0; /* nothing */; ++dwIndex)
    {
        wchar_t szSubKey[MAX_PATH] = {0};
        DWORD   cchSubKey          = MAX_PATH;
        LSTATUS status             = RegEnumKeyEx(hk, dwIndex, szSubKey, &cchSubKey, nullptr, nullptr, nullptr, nullptr);
        if (status != ERROR_SUCCESS)
            break;

        HKEY hk2;
        if (RegOpenKeyEx(hk, szSubKey, 0, KEY_READ, &hk2) == ERROR_SUCCESS)
        {
            wchar_t szWorkingCopy[MAX_PATH] = {0};
            DWORD   cbWorkingCopy           = sizeof(szWorkingCopy);
            RegQueryValueEx(hk2, L"WorkingCopy", nullptr, nullptr, reinterpret_cast<LPBYTE>(szWorkingCopy), &cbWorkingCopy);

            wchar_t szClsid[MAX_PATH] = {0};
            DWORD   cbClsid           = sizeof(szClsid);
            RegQueryValueEx(hk2, L"Provider", nullptr, nullptr, reinterpret_cast<LPBYTE>(szClsid), &cbClsid);

            CLSID providerClsid;
            CLSIDFromString(szClsid, &providerClsid);

            DWORD cbParameters = 0;
            RegQueryValueEx(hk2, L"Parameters", nullptr, nullptr, static_cast<LPBYTE>(nullptr), &cbParameters);
            auto szParameters = std::make_unique<wchar_t[]>(cbParameters + 1LL);
            RegQueryValueEx(hk2, L"Parameters", nullptr, nullptr, reinterpret_cast<LPBYTE>(szParameters.get()), &cbParameters);
            szParameters[cbParameters] = 0;
            m_inner.push_back(new CBugTraqAssociation(szWorkingCopy, providerClsid, LookupProviderName(providerClsid), szParameters.get()));

            RegCloseKey(hk2);
        }
    }

    RegCloseKey(hk);
    if (uuid)
        m_providerUuid = uuid;
    if (params)
        providerParams = params;
}

void CBugTraqAssociations::Add(const CBugTraqAssociation &assoc)
{
    m_inner.push_back(new CBugTraqAssociation(assoc));
}

bool CBugTraqAssociations::FindProvider(const CTSVNPathList &pathList, CBugTraqAssociation *assoc)
{
    if (FindProviderForPathList(pathList, assoc))
        return true;

    if (pProjectProvider)
    {
        if (assoc)
            *assoc = *pProjectProvider;
        return true;
    }
    if (!m_providerUuid.IsEmpty())
    {
        CLSID providerClsid;
        CLSIDFromString(const_cast<LPOLESTR>(static_cast<LPCWSTR>(m_providerUuid)), &providerClsid);
        pProjectProvider = new CBugTraqAssociation(L"", providerClsid, L"bugtraq:provider", static_cast<LPCWSTR>(providerParams));
        if (pProjectProvider)
        {
            if (assoc)
                *assoc = *pProjectProvider;
            return true;
        }
    }
    return false;
}

bool CBugTraqAssociations::FindProviderForPathList(const CTSVNPathList &pathList, CBugTraqAssociation *assoc) const
{
    for (int i = 0; i < pathList.GetCount(); ++i)
    {
        CTSVNPath path = pathList[i];
        if (FindProviderForPath(path, assoc))
            return true;
    }

    return false;
}

bool CBugTraqAssociations::FindProviderForPath(CTSVNPath path, CBugTraqAssociation *assoc) const
{
    do
    {
        auto it = std::find_if(m_inner.begin(), m_inner.end(), FindByPathPred(path));
        if (it != m_inner.end())
        {
            if (assoc)
                *assoc = *(*it);
            return true;
        }

        path = path.GetContainingDirectory();
    } while (!path.IsEmpty());

    return false;
}

/* static */
CString CBugTraqAssociations::LookupProviderName(const CLSID &providerClsid)
{
    OLECHAR szClsid[40] = {0};
    StringFromGUID2(providerClsid, szClsid, _countof(szClsid));

    wchar_t szSubKey[MAX_PATH] = {0};
    swprintf_s(szSubKey, L"CLSID\\%ls", szClsid);

    CString providerName = CString(szClsid);

    HKEY hk;
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        wchar_t szClassName[MAX_PATH] = {0};
        DWORD   cbClassName           = sizeof(szClassName);

        if (RegQueryValueEx(hk, nullptr, nullptr, nullptr, reinterpret_cast<LPBYTE>(szClassName), &cbClassName) == ERROR_SUCCESS)
            providerName = CString(szClassName);

        RegCloseKey(hk);
    }

    return providerName;
}

LSTATUS RegSetValueFromCString(HKEY hKey, LPCWSTR lpValueName, CString str)
{
    LPCWSTR lpsz = str;
    DWORD   cb   = (str.GetLength() + 1) * sizeof(wchar_t);
    return RegSetValueEx(hKey, lpValueName, 0, REG_SZ, reinterpret_cast<const BYTE *>(lpsz), cb);
}

void CBugTraqAssociations::Save() const
{
    SHDeleteKey(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH);

    HKEY hk;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, BUGTRAQ_ASSOCIATIONS_REGPATH, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &hk, nullptr) != ERROR_SUCCESS)
        return;

    DWORD dwIndex = 0;
    for (auto it = begin(); it != end(); ++it)
    {
        wchar_t szSubKey[MAX_PATH] = {0};
        swprintf_s(szSubKey, L"%lu", dwIndex);

        HKEY hk2;
        if (RegCreateKeyEx(hk, szSubKey, 0, nullptr, 0, KEY_WRITE, nullptr, &hk2, nullptr) == ERROR_SUCCESS)
        {
            RegSetValueFromCString(hk2, L"Provider", (*it)->GetProviderClassAsString());
            RegSetValueFromCString(hk2, L"WorkingCopy", (*it)->GetPath().GetWinPath());
            RegSetValueFromCString(hk2, L"Parameters", (*it)->GetParameters());

            RegCloseKey(hk2);
        }

        ++dwIndex;
    }

    RegCloseKey(hk);
}

void CBugTraqAssociations::RemoveByPath(const CTSVNPath &path)
{
    auto it = std::find_if(m_inner.begin(), m_inner.end(), FindByPathPred(path));
    if (it != m_inner.end())
    {
        delete *it;
        m_inner.erase(it);
    }
}

CString CBugTraqAssociation::GetProviderClassAsString() const
{
    OLECHAR szTemp[40] = {0};
    StringFromGUID2(m_provider.clsid, szTemp, _countof(szTemp));

    return CString(szTemp);
}

/* static */
std::vector<CBugTraqProvider> CBugTraqAssociations::GetAvailableProviders()
{
    std::vector<CBugTraqProvider> results;

    CComPtr<ICatInformation> pCatInformation;
    HRESULT                  hr = pCatInformation.CoCreateInstance(CLSID_StdComponentCategoriesMgr, nullptr, CLSCTX_ALL);
    if (SUCCEEDED(hr))
    {
        CComPtr<IEnumGUID> pEnum;
        hr = pCatInformation->EnumClassesOfCategories(1, &CATID_BugTraqProvider, 0, nullptr, &pEnum);
        if (SUCCEEDED(hr))
        {
            HRESULT hrEnum;
            do
            {
                CLSID clsids[5]{};
                ULONG cClsids = 0;

                hrEnum = pEnum->Next(_countof(clsids), clsids, &cClsids);
                if (SUCCEEDED(hrEnum))
                {
                    for (ULONG i = 0; i < cClsids; ++i)
                    {
                        CBugTraqProvider provider;
                        provider.clsid = clsids[i];
                        provider.name  = LookupProviderName(clsids[i]);
                        results.push_back(provider);
                    }
                }
            } while (hrEnum == S_OK);
        }
    }
    return results;
}
