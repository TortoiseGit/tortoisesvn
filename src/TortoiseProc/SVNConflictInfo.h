// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseSVN

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

#include "SVNBase.h"

struct svn_client_conflict_t;
struct svn_client_conflict_option_t;

/**
* \ingroup SVN
* Wrapper for the svn_client_conflict_option API.
*/
class SVNConflictOption
{
    SVNConflictOption(const SVNConflictOption&);
    SVNConflictOption& operator=(SVNConflictOption&);
public:
    svn_client_conflict_option_id_t GetId() { return m_id; }
    CString GetDescription() { return m_description; }
    operator svn_client_conflict_option_t *() { return m_option; }

protected:
    SVNConflictOption(svn_client_conflict_option_t *option,
                      svn_client_conflict_option_id_t id,
                      const CString & description);

private:
    svn_client_conflict_option_t *m_option;
    svn_client_conflict_option_id_t m_id;
    CString m_description;

    friend class SVNConflictInfo;
};

/**
* \ingroup SVN
* Collection of SVNConflictOption.
*/
class SVNConflictOptions : public std::deque<std::unique_ptr<SVNConflictOption>>
{
public:
    SVNConflictOptions();
    ~SVNConflictOptions();

    apr_pool_t *GetPool() { return m_pool; }

private:
    apr_pool_t *m_pool;
};

/**
* \ingroup SVN
* Wrapper for the svn_client_conflict_t API.
*/
class SVNConflictInfo : public SVNBase
{
private:
    SVNConflictInfo(const SVNConflictInfo&);
    SVNConflictInfo& operator=(const SVNConflictInfo&);

public:
    SVNConflictInfo();
    ~SVNConflictInfo();

    bool Get(const CTSVNPath & path);
    bool HasTreeConflict();
    bool GetTreeConflictDescription(CString & incomingChangeDescription, CString & localChangeDescription);

    bool GetTreeResolutionOptions(SVNConflictOptions & result);

    operator svn_client_conflict_t *() { return m_conflict; }

protected:
    apr_pool_t *m_pool;
    svn_client_conflict_t *m_conflict;
};
