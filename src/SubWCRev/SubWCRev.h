// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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

#include "apr_pools.h"
#include "svn_error.h"
#include "svn_client.h"
#include "svn_path.h"

#define URL_BUF 2048
#define OWNER_BUF   2048
#define COMMENT_BUF 4096

/**
 * \ingroup SubWCRev
 * This structure is used as a part of the status baton for WC crawling
 * and contains all the information we are collecting regarding the lock status.
 */
typedef struct SubWcLockData_t
{
    BOOL NeedsLocks;            // TRUE if a lock can be applied in generally; if FALSE, the values of the other parms in this struct are invalid
    BOOL IsLocked;              // TRUE if the file or folder is locked
    char Owner[OWNER_BUF];      // the username which owns the lock
    char Comment[COMMENT_BUF];  // lock comment
    apr_time_t CreationDate;    // when lock was made

} SubWcLockData_t;

/**
 * \ingroup SubWCRev
 * This structure is used as the status baton for WC crawling
 * and contains all the information we are collecting.
 */
struct SubWCRev_t
{
    SubWCRev_t()
        : MinRev(0)
        , MaxRev(0)
        , CmtRev(0)
        , CmtDate(0)
        , HasMods(FALSE)
        , HasUnversioned(FALSE)
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
        SecureZeroMemory(Url, sizeof(Url));
        SecureZeroMemory(RootUrl, sizeof(RootUrl));
        SecureZeroMemory(Author, sizeof(Author));
        SecureZeroMemory(&LockData, sizeof(LockData));
    }
    svn_revnum_t MinRev;    // Lowest update revision found
    svn_revnum_t MaxRev;    // Highest update revision found
    svn_revnum_t CmtRev;    // Highest commit revision found
    apr_time_t CmtDate;     // Date of highest commit revision
    BOOL HasMods;           // True if local modifications found
    BOOL HasUnversioned;    // True if unversioned items found
    BOOL bFolders;          // If TRUE, status of folders is included
    BOOL bExternals;        // If TRUE, status of externals is included
    BOOL bExternalsNoMixedRevision; // If TRUE, externals set to an explicit revision lead not to an mixed revision error
    BOOL bHexPlain;         // If TRUE, revision numbers are output in HEX
    BOOL bHexX;             // If TRUE, revision numbers are output in HEX with '0x'
    char Url[URL_BUF];      // URL of working copy
    char RootUrl[URL_BUF];  // url of the repository root
    char Author[URL_BUF];   // The author of the wcPath
    BOOL  bIsSvnItem;           // True if the item is under SVN
    SubWcLockData_t LockData;   // Data regarding the lock of the file
    BOOL  bIsExternalsNotFixed; // True if one external is not fixed to a specified revision
    BOOL  bIsExternalMixed; // True if one external, which is fixed has not the explicit revision set
    BOOL  bIsTagged;   // True if working copy URL contains "tags" keyword
    std::set<std::string> ignorepatterns;   // a list of file patterns to ignore
};

/**
 * \ingroup SubWCRev
 * This structure is used as a part of the status baton for crawling externals
 * and contains the information about the externals path and revision status.
 */
typedef struct SubWcExtData_t
{
    const char * Path;            // The name of the directory (absolute path) into which this external should be checked out
    svn_opt_revision_t Revision;  // What revision to check out.
} SubWcExtData_t;

/**
 * \ingroup SubWCRev
 * Collects all the SubWCRev_t structures in an array.
 */
typedef struct SubWCRev_StatusBaton_t
{
    SubWCRev_t * SubStat;
    std::vector<SubWcExtData_t> * extarray;
    apr_pool_t *pool;
    svn_wc_context_t * wc_ctx;
} SubWCRev_StatusBaton_t;

/**
 * \ingroup SubWCRev
 * Callback function when fetching the Subversion status
 */
svn_error_t *
svn_status (       const char *path,
                   void *status_baton,
                   svn_boolean_t no_ignore,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

void LoadIgnorePatterns(const char * wc, SubWCRev_t * SubStat);
