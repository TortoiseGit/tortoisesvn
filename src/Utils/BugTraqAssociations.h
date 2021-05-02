// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008, 2021 - TortoiseSVN

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
#pragma once
#include "TSVNPath.h"

struct CBugTraqProvider
{
    CLSID   clsid;
    CString name;
};

class CBugTraqAssociation
{
    CTSVNPath        m_path;
    CBugTraqProvider m_provider;
    CString          m_parameters;

public:
    CBugTraqAssociation()
    {
        m_provider.clsid = GUID_NULL;
    }

    CBugTraqAssociation(LPCWSTR szWorkingCopy, const CLSID &providerClsid, LPCWSTR szProviderName, LPCWSTR szParameters)
        : m_path(szWorkingCopy)
        , m_parameters(szParameters)
    {
        m_provider.clsid = providerClsid;
        m_provider.name  = szProviderName;
    }

    const CTSVNPath &GetPath() const { return m_path; }
    CString          GetProviderName() const { return m_provider.name; }
    CLSID            GetProviderClass() const { return m_provider.clsid; }
    CString          GetProviderClassAsString() const;
    CString          GetParameters() const { return m_parameters; }
};

class CBugTraqAssociations
{
    std::vector<CBugTraqAssociation *> m_inner;

public:
    CBugTraqAssociations();
    ~CBugTraqAssociations();

    void Load(LPCWSTR uuid = nullptr, LPCWSTR params = nullptr);
    void Save() const;

    void Add(const CBugTraqAssociation &assoc);
    void RemoveByPath(const CTSVNPath &path);

    bool FindProvider(const CTSVNPathList &pathList, CBugTraqAssociation *assoc);

    std::vector<CBugTraqAssociation *>::const_iterator begin() const { return m_inner.begin(); }
    std::vector<CBugTraqAssociation *>::const_iterator end() const { return m_inner.end(); }

    static std::vector<CBugTraqProvider> GetAvailableProviders();
    static CString                       LookupProviderName(const CLSID &providerClsid);

private:
    bool FindProviderForPathList(const CTSVNPathList &pathList, CBugTraqAssociation *assoc) const;
    bool FindProviderForPath(CTSVNPath path, CBugTraqAssociation *assoc) const;

    struct FindByPathPred
    {
        const CTSVNPath &m_path;

        FindByPathPred(const CTSVNPath &path)
            : m_path(path)
        {
        }

        bool operator()(const CBugTraqAssociation *assoc) const
        {
            return (assoc->GetPath().IsEquivalentToWithoutCase(m_path));
        }
    };
    CString              m_providerUuid;
    CString              providerParams;
    CBugTraqAssociation *pProjectProvider;
};
