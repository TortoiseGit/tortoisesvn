// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2010, 2012-2015 - TortoiseSVN

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
#include "SVNGlobal.h"
#include "TSVNPath.h"
#pragma warning(push)
#include "svn_client.h"
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
    SVNConfig(const SVNConfig&){}
    SVNConfig(void);
    ~SVNConfig(void);
public:
    static SVNConfig& Instance()
    {
        if (m_pInstance == NULL)
            m_pInstance = new SVNConfig();
        return *m_pInstance;
    }

    /**
     * Returns the configuration
     */
    apr_hash_t * GetConfig(apr_pool_t * pool);

    /**
     * Reloads the configuration
     */
    void Refresh();

    /**
     * Returns the error object from the initialization, or nullptr if there wasn't any
     */
    svn_error_t * GetError() const
    {
        return Err;
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
    BOOL MatchIgnorePattern(const CString& name);

    BOOL KeepLocks();

    BOOL ConfigGetBool(const char * section, const char * option, bool defbool);

private:
    bool SetUpSSH();

    apr_pool_t *                parentpool;
    apr_pool_t *                m_pool;
    apr_pool_t *                wcignorespool;
    apr_hash_t *                config;
    apr_array_header_t *        patterns;
    svn_error_t *               Err;
    CComCriticalSection         m_critSec;
    static SVNConfig *          m_pInstance;
};
