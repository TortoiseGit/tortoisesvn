// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2019, 2021 - TortoiseSVN

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
#include <vector>
#include <set>
#include <tuple>
#include <string>

#include "apr_pools.h"
#include "svn_client.h"
#include "svn_path.h"

#define URL_BUF     2048
#define OWNER_BUF   2048
#define COMMENT_BUF 4096

/**
 * \ingroup SubWCRev
 * This structure is used as a part of the status baton for WC crawling
 * and contains all the information we are collecting regarding the lock status.
 */
struct SubWcLockDataT
{
    BOOL       needsLocks = FALSE;     // TRUE if a lock can be applied in generally; if FALSE, the values of the other parms in this struct are invalid
    BOOL       isLocked   = FALSE;     // TRUE if the file or folder is locked
    char       owner[OWNER_BUF]{};     // the username which owns the lock
    char       comment[COMMENT_BUF]{}; // lock comment
    apr_time_t creationDate = 0;       // when lock was made
};

/**
 * \ingroup SubWCRev
 * This structure is used as the status baton for WC crawling
 * and contains all the information we are collecting.
 */
struct SubWCRevT
{
    SubWCRevT()
        : minRev(0)
        , maxRev(0)
        , cmtRev(0)
        , cmtDate(0)
        , hasMods(FALSE)
        , hasUnversioned(FALSE)
        , bFolders(FALSE)
        , bExternals(FALSE)
        , bExternalsNoMixedRevision(FALSE)
        , bHexPlain(FALSE)
        , bHexX(FALSE)
        , bIsSvnItem(FALSE)
        , bIsExternalsNotFixed(FALSE)
        , bIsExternalMixed(FALSE)
        , bIsTagged(FALSE)
    {
        SecureZeroMemory(url, sizeof(url));
        SecureZeroMemory(rootUrl, sizeof(rootUrl));
        SecureZeroMemory(author, sizeof(author));
        SecureZeroMemory(&lockData, sizeof(lockData));
    }
    svn_revnum_t                              minRev;                    // Lowest update revision found
    svn_revnum_t                              maxRev;                    // Highest update revision found
    svn_revnum_t                              cmtRev;                    // Highest commit revision found
    apr_time_t                                cmtDate;                   // Date of highest commit revision
    BOOL                                      hasMods;                   // True if local modifications found
    BOOL                                      hasUnversioned;            // True if unversioned items found
    BOOL                                      bFolders;                  // If TRUE, status of folders is included
    BOOL                                      bExternals;                // If TRUE, status of externals is included
    BOOL                                      bExternalsNoMixedRevision; // If TRUE, externals set to an explicit revision lead not to an mixed revision error
    BOOL                                      bHexPlain;                 // If TRUE, revision numbers are output in HEX
    BOOL                                      bHexX;                     // If TRUE, revision numbers are output in HEX with '0x'
    char                                      url[URL_BUF];              // URL of working copy
    char                                      rootUrl[URL_BUF];          // url of the repository root
    char                                      author[URL_BUF];           // The author of the wcPath
    BOOL                                      bIsSvnItem;                // True if the item is under SVN
    SubWcLockDataT                            lockData;                  // Data regarding the lock of the file
    BOOL                                      bIsExternalsNotFixed;      // True if one external is not fixed to a specified revision
    BOOL                                      bIsExternalMixed;          // True if one external, which is fixed has not the explicit revision set
    BOOL                                      bIsTagged;                 // True if working copy URL contains "tags" keyword
    std::set<std::tuple<std::string, size_t>> ignorePatterns;            // a list of file patterns to ignore
};

/**
 * \ingroup SubWCRev
 * This structure is used as a part of the status baton for crawling externals
 * and contains the information about the externals path and revision status.
 */
struct SubWcExtDataT
{
    const char *       path;     // The name of the directory (absolute path) into which this external should be checked out
    svn_opt_revision_t revision; // What revision to check out.
};

/**
 * \ingroup SubWCRev
 * Collects all the SubWCRev_t structures in an array.
 */
struct SubWCRevStatusBatonT
{
    SubWCRevT *                  subStat;
    std::vector<SubWcExtDataT> *extArray;
    apr_pool_t *                 pool;
    svn_wc_context_t *           wcCtx;
};

/**
 * \ingroup SubWCRev
 * Callback function when fetching the Subversion status
 */
svn_error_t *
    svnStatus(const char *      path,
              void *            statusBaton,
              svn_boolean_t     noIgnore,
              svn_client_ctx_t *ctx,
              apr_pool_t *      pool);

void loadIgnorePatterns(const char *wc, SubWCRevT *subStat);
