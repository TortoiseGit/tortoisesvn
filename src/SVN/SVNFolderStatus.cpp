// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "ShellExt.h"
#include "SVNFolderStatus.h"
#include "UnicodeUtils.h"
#include "..\TSVNCache\CacheInterface.h"
#include "SVNGlobal.h"
#include "SmartHandle.h"

extern ShellCache g_ShellCache;


// get / auto-alloc a string "copy"

const char* StringPool::GetString (const char* value)
{
    // special case: NULL pointer

    if (value == NULL)
    {
        return emptyString;
    }

    // do we already have a string with the desired value?

    pool_type::const_iterator iter = pool.find (value);
    if (iter != pool.end())
    {
        // yes -> return it
        return *iter;
    }

    // no -> add one

    const char* newString =  _strdup (value);
    if (newString)
    {
        pool.insert (newString);
    }
    else
        return emptyString;

    // .. and return it

    return newString;
}

// clear internal pool

void StringPool::clear()
{
    // delete all strings

    for (pool_type::iterator iter = pool.begin(), end = pool.end(); iter != end; ++iter)
    {
        free((void*)*iter);
    }

    // remove pointers from pool

    pool.clear();
}

CTSVNPath   SVNFolderStatus::folderpath;


SVNFolderStatus::SVNFolderStatus(void)
    : m_TimeStamp(0)
    , m_nCounter(0)
    , dirstatus(NULL)
{
    emptyString[0] = 0;
    invalidstatus.author = emptyString;
    invalidstatus.askedcounter = -1;
    invalidstatus.status = svn_wc_status_none;
    invalidstatus.url = emptyString;
    invalidstatus.rev = -1;
    invalidstatus.owner = emptyString;
    invalidstatus.needslock = false;
    invalidstatus.tree_conflict = false;

    sCacheKey.reserve(MAX_PATH);

    rootpool = svn_pool_create (NULL);

    m_hInvalidationEvent = CreateEvent(NULL, FALSE, FALSE, _T("TortoiseSVNCacheInvalidationEvent"));
}

SVNFolderStatus::~SVNFolderStatus(void)
{
    svn_pool_destroy(rootpool);
}

const FileStatusCacheEntry * SVNFolderStatus::BuildCache(const CTSVNPath& filepath, BOOL bIsFolder, BOOL bDirectFolder)
{
    svn_client_ctx_t *          localctx;
    apr_pool_t *                pool;
    svn_error_t *               err = NULL; // If svn_client_status comes out through catch(...), err would else be unassigned

    //dont' build the cache if an instance of TortoiseProc is running
    //since this could interfere with svn commands running (concurrent
    //access of the .svn directory).
    if (g_ShellCache.BlockStatus())
    {
        CAutoGeneralHandle TSVNMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseProc.exe"));
        if (TSVNMutex != NULL)
        {
            if (::GetLastError() == ERROR_ALREADY_EXISTS)
            {
                return &invalidstatus;
            }
        }
    }

    pool = svn_pool_create (rootpool);              // create the memory pool

    ClearCache();
    svn_error_clear(svn_client_create_context(&localctx, pool));
    // set up the configuration
    // Note: I know this is an 'expensive' call, but without this, ignores
    // done in the global ignore pattern won't show up.
    if (g_unversionedovlloaded)
        svn_error_clear(svn_config_get_config (&(localctx->config), g_pConfigDir, pool));

    // strings pools are unused, now -> we may clear them

    authors.clear();
    urls.clear();
    owners.clear();

    ATLTRACE2(_T("building cache for %s\n"), filepath);
    if (bIsFolder)
    {
        if (bDirectFolder)
        {
            // initialize record members
            dirstat.rev = -1;
            dirstat.status = svn_wc_status_none;
            dirstat.author = authors.GetString(NULL);
            dirstat.url = urls.GetString(NULL);
            dirstat.owner = owners.GetString(NULL);
            dirstat.askedcounter = SVNFOLDERSTATUS_CACHETIMES;
            dirstat.needslock = false;
            dirstat.tree_conflict = false;
            dirstat.lock = nullptr;

            dirstatus = NULL;
            svn_revnum_t youngest = SVN_INVALID_REVNUM;
            svn_opt_revision_t rev;
            rev.kind = svn_opt_revision_unspecified;
            try
            {
                folderpath = filepath;
                err = svn_client_status5 (&youngest,
                    localctx,
                    filepath.GetDirectory().GetSVNApiPath(pool),
                    &rev,
                    svn_depth_empty,    // depth
                    TRUE,               // get all
                    FALSE,              // update
                    TRUE,               // no ignore
                    FALSE,              // ignore externals
                    TRUE,           // depth as sticky
                    NULL,
                    findfolderstatus,
                    this,
                    pool);
            }
            catch ( ... )
            {
                dirstatus = NULL;
            }


            if (dirstatus)
            {
                dirstat.author = authors.GetString (dirstatus->changed_author);
                dirstat.url = authors.GetString (dirstatus->repos_relpath);
                dirstat.rev = dirstatus->changed_rev;
                if (dirstatus->lock)
                    dirstat.owner = owners.GetString(dirstatus->lock->owner);

                dirstat.status = dirstatus->node_status;
                dirstat.tree_conflict = dirstatus->conflicted != 0;
                dirstat.lock = dirstatus->lock;
            }
            m_cache[filepath.GetWinPath()] = dirstat;
            m_TimeStamp = GetTickCount();
            svn_error_clear(err);
            svn_pool_destroy (pool);                //free allocated memory
            return &dirstat;
        }
    } // if (bIsFolder)

    m_nCounter = 0;

    //Fill in the cache with
    //all files inside the same folder as the asked file/folder is
    //since subversion can do this in one step
    localctx->auth_baton = NULL;

    svn_revnum_t youngest = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    try
    {
        err = svn_client_status5 (&youngest,
            localctx,
            filepath.GetDirectory().GetSVNApiPath(pool),
            &rev,
            svn_depth_immediates,       // depth
            TRUE,                       // get all
            FALSE,                      // update
            TRUE,                       // no ignore
            FALSE,                      // ignore externals
            TRUE,           // depth as sticky
            NULL,
            fillstatusmap,
            this,
            pool);
    }
    catch ( ... )
    {
    }

    // Error present if function is not under version control
    if (err != NULL)
    {
        svn_error_clear(err);
        svn_pool_destroy (pool);                //free allocated memory
        return &invalidstatus;
    }

    svn_error_clear(err);
    svn_pool_destroy (pool);                //free allocated memory
    m_TimeStamp = GetTickCount();
    const FileStatusCacheEntry * ret = NULL;
    FileStatusMap::const_iterator iter;
    if ((iter = m_cache.find(filepath.GetWinPath())) != m_cache.end())
    {
        ret = &iter->second;
        m_mostRecentPath = filepath;
        m_mostRecentStatus = ret;
    }
    else
    {
        // for SUBST'ed drives, Subversion doesn't return a path with a backslash
        // e.g. G:\ but only G: when fetching the status. So search for that
        // path too before giving up.
        // This is especially true when right-clicking directly on a SUBST'ed
        // drive to get the context menu
        if (_tcslen(filepath.GetWinPath())==3)
        {
            if ((iter = m_cache.find((LPCTSTR)filepath.GetWinPathString().Left(2))) != m_cache.end())
            {
                ret = &iter->second;
                m_mostRecentPath = filepath;
                m_mostRecentStatus = ret;
            }
        }
    }
    if (ret)
        return ret;
    return &invalidstatus;
}

DWORD SVNFolderStatus::GetTimeoutValue()
{
    DWORD timeout = SVNFOLDERSTATUS_CACHETIMEOUT;
    DWORD factor = (DWORD)m_cache.size()/200;
    if (factor==0)
        factor = 1;
    return factor*timeout;
}

const FileStatusCacheEntry * SVNFolderStatus::GetFullStatus(const CTSVNPath& filepath, BOOL bIsFolder, BOOL bColumnProvider)
{
    const FileStatusCacheEntry * ret = NULL;

    BOOL bHasAdminDir = g_ShellCache.IsVersioned(filepath.GetWinPath(), !!bIsFolder, false);

    //no overlay for unversioned folders
    if ((!bColumnProvider)&&(!bHasAdminDir))
        return &invalidstatus;
    //for the SVNStatus column, we have to check the cache to see
    //if it's not just unversioned but ignored
    ret = GetCachedItem(filepath);
    if ((ret)&&(ret->status == svn_wc_status_unversioned)&&(bIsFolder)&&(bHasAdminDir))
    {
        // an 'unversioned' folder, but with an ADMIN dir --> nested layout!
        ret = BuildCache(filepath, bIsFolder, TRUE);
        if (ret)
            return ret;
        else
            return &invalidstatus;
    }
    if (ret)
        return ret;

    //if it's not in the cache and has no admin dir, then we assume
    //it's not ignored too
    if ((bColumnProvider)&&(!bHasAdminDir))
        return &invalidstatus;
    ret = BuildCache(filepath, bIsFolder);
    if (ret)
        return ret;
    else
        return &invalidstatus;
}

const FileStatusCacheEntry * SVNFolderStatus::GetCachedItem(const CTSVNPath& filepath)
{
    sCacheKey.assign(filepath.GetWinPath());
    FileStatusMap::const_iterator iter;
    const FileStatusCacheEntry *retVal;

    if(m_mostRecentPath.IsEquivalentTo(CTSVNPath(sCacheKey.c_str())))
    {
        // We've hit the same result as we were asked for last time
        ATLTRACE2(_T("fast cache hit for %s\n"), filepath);
        retVal = m_mostRecentStatus;
    }
    else if ((iter = m_cache.find(sCacheKey)) != m_cache.end())
    {
        ATLTRACE2(_T("cache found for %s\n"), filepath);
        retVal = &iter->second;
        m_mostRecentStatus = retVal;
        m_mostRecentPath = CTSVNPath(sCacheKey.c_str());
    }
    else
    {
        retVal = NULL;
    }

    if(retVal != NULL)
    {
        // We found something in a cache - check that the cache is not timed-out or force-invalidated
        DWORD now = GetTickCount();

        if ((now >= m_TimeStamp)&&((now - m_TimeStamp) > GetTimeoutValue()))
        {
            // Cache is timed-out
            ATLTRACE("Cache timed-out\n");
            ClearCache();
            retVal = NULL;
        }
        else if(WaitForSingleObject(m_hInvalidationEvent, 0) == WAIT_OBJECT_0)
        {
            // TortoiseProc has just done something which has invalidated the cache
            ATLTRACE("Cache invalidated\n");
            ClearCache();
            retVal = NULL;
        }
        return retVal;
    }
    return NULL;
}

svn_error_t* SVNFolderStatus::fillstatusmap(void * baton, const char * path, const svn_client_status_t * status, apr_pool_t * /*pool*/)
{
    SVNFolderStatus * Stat = (SVNFolderStatus *)baton;
    FileStatusMap * cache = &Stat->m_cache;
    FileStatusCacheEntry s = {  svn_wc_status_none,          // state
                                "",                          // author
                                "",                          // url
                                "",                          // owner
                                false,                       // needslock
                                -1,                          // rev
                                SVNFOLDERSTATUS_CACHETIMES,  // askedcounter
                                NULL,                        // lock
                                false,                       // tree_conflict;
                             };
    if (status)
    {
        s.author = Stat->authors.GetString(status->changed_author);
        s.url = Stat->urls.GetString(status->repos_relpath);
        s.rev = status->changed_rev;
        if (status->lock)
            s.owner = Stat->owners.GetString(status->lock->owner);
        s.status = SVNStatus::GetMoreImportant(s.status, status->node_status);
        s.lock = status->repos_lock;
        s.tree_conflict = (status->conflicted != 0);
    }
    tstring str;
    if (path)
    {
        str = CUnicodeUtils::StdGetUnicode(path);
        std::replace(str.begin(), str.end(), '/', '\\');
    }
    else
        str = _T(" ");
    (*cache)[str] = s;

    return SVN_NO_ERROR;
}

svn_error_t* SVNFolderStatus::findfolderstatus(void * baton, const char * path, const svn_client_status_t * status, apr_pool_t * /*pool*/)
{
    SVNFolderStatus * Stat = (SVNFolderStatus *)baton;
    if ((Stat)&&(Stat->folderpath.IsEquivalentTo(CTSVNPath(CString(path)))))
    {
        Stat->dirstatus = status;
    }

    return SVN_NO_ERROR;
}

void SVNFolderStatus::ClearCache()
{
    m_cache.clear();
    m_mostRecentStatus = NULL;
    m_mostRecentPath.Reset();
    // If we're about to rebuild the cache, there's no point hanging on to
    // an event which tells us that it's invalid
    ResetEvent(m_hInvalidationEvent);
}
