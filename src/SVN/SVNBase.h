// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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

/*
 * allow compilation even if the following structs have not been
 * defined yet (i.e. in case of missing includes).
 */

struct apr_pool_t;
struct svn_client_ctx_t;
struct svn_error_t;
struct apr_hash_t;
class CTSVNPath;

#ifdef _MFC_VER
#   ifndef CSTRING_AVAILABLE
#       define CSTRING_AVAILABLE
#   endif
#endif

/**
 * \ingroup SVN
 * base class for all SVN classes like SVN, SVNInfo, SVNStatus, ...
 */
class SVNBase
{
public:
    SVNBase();
    ~SVNBase();

#ifdef CSTRING_AVAILABLE
    /**
     * If a method of this class returns FALSE then you can
     * get the detailed error message with this method.
     * \return the error message string
     */
    CString GetLastErrorMessage(int wrap = 80);

    /**
     * Returns the string representation of the error object \c Err, wrapped
     * (if possible) at \c wrap chars.
     */
    static CString GetErrorString(svn_error_t * Err, int wrap = 80);
#endif

    void ClearSVNError() { svn_error_clear(Err); Err = NULL; }
    const svn_error_t * GetSVNError() const { return Err; }
    svn_client_ctx_t * GetSVNClientContext() const { return m_pctx; }

protected:
#ifdef CSTRING_AVAILABLE
    CString                     PostCommitErr;  ///< error string from post commit hook script
#endif
    svn_error_t *               Err;            ///< Global error object struct
    svn_client_ctx_t *          m_pctx;         ///< pointer to client context
};


