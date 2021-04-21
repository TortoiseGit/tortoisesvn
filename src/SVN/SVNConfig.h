// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2010, 2012-2015, 2018, 2021 - TortoiseSVN

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

#pragma once
#include "TSVNPath.h"
#pragma warning(push)
#include "apr_hash.h"
#include "apr_tables.h"
#pragma warning(pop)

/**
 * \ingroup SVN
 * A small wrapper for the Subversion configurations.
 */
class SVNConfig
{
private:
    SVNConfig();
    ~SVNConfig();
    // prevent cloning
    SVNConfig(const SVNConfig&) = delete;
    SVNConfig& operator=(const SVNConfig&) = delete;

public:
    static SVNConfig& Instance()
    {
        if (m_pInstance == nullptr)
            m_pInstance = new SVNConfig();
        return *m_pInstance;
    }

    /**
     * Returns the configuration
     */
    apr_hash_t* GetConfig(apr_pool_t* pool);

    /**
     * Reloads the configuration
     */
    void Refresh();

    /**
     * Returns the error object from the initialization, or nullptr if there wasn't any
     */
    svn_error_t* GetError() const
    {
        return m_err;
    }

    /**
     * Reads the global ignore patterns which will be used later in
     * MatchIgnorePattern().
     * \return TRUE if the function is successful
     */
    BOOL GetDefaultIgnores();

    /**
     * Reads the global ignore patterns from svnconfig, and the svn:global-ignores property from the \c path
     * which will be used later in MatchIgnorePattern().
     * \return TRUE if the function is successful
     */
    BOOL GetWCIgnores(const CTSVNPath& path);

    /**
     * Checks if the \c name matches a pattern in the array of
     * ignore patterns.
     * \param name the name to check
     * \return TRUE if the name matches a pattern, FALSE if it doesn't.
     */
    BOOL MatchIgnorePattern(const CString& name) const;

    BOOL KeepLocks() const;

    BOOL ConfigGetBool(const char* section, const char* option, bool defbool) const;

    const CTSVNPath& GetLastWCIgnorePath() const { return m_lastWcIgnorePath; }

private:
    bool SetUpSSH() const;

    apr_pool_t*             m_parentPool;
    apr_pool_t*             m_pool;
    apr_pool_t*             m_wcIgnoresPool;
    apr_hash_t*             m_config;
    apr_array_header_t*     m_patterns;
    svn_error_t*            m_err;
    CComAutoCriticalSection m_critSec;
    static SVNConfig*       m_pInstance;
    CTSVNPath               m_lastWcIgnorePath;
};
