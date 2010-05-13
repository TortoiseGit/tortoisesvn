// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2010 - TortoiseSVN

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
/**
 * \ingroup SVN
 * A small wrapper for the Subversion configurations.
 */
class SVNConfig
{
private:
    SVNConfig(const SVNConfig&){}
public:
    SVNConfig(void);
    ~SVNConfig(void);

    /**
     * Reads the global ignore patterns which will be used later in
     * MatchIgnorePattern().
     * \return TRUE if the function is successful
     */
    BOOL GetDefaultIgnores();

    /**
     * Checks if the \c name matches a pattern in the array of
     * ignore patterns.
     * \param name the name to check
     * \param *patterns the array of ignore patterns. Get this array with GetDefaultIgnores()
     * \return TRUE if the name matches a pattern, FALSE if it doesn't.
     */
    BOOL MatchIgnorePattern(const CString& name);

    BOOL KeepLocks();
private:
    apr_pool_t *                parentpool;
    apr_pool_t *                pool;           ///< memory pool
    svn_client_ctx_t *          ctx;
    apr_array_header_t *        patterns;

};
