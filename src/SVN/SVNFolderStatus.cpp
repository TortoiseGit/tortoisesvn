// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014, 2016, 2021 - TortoiseSVN

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
#include "SVNConfig.h"
#include "SmartHandle.h"

extern ShellCache g_shellCache;

// get / auto-alloc a string "copy"

const char* StringPool::GetString(const char* value)
{
    // special case: NULL pointer

    if (value == nullptr)
    {
        return emptyString;
    }

    // do we already have a string with the desired value?

    auto iter = pool.find(value);
    if (iter != pool.end())
    {
        // yes -> return it
        return *iter;
    }

    // no -> add one

    const char* newString = _strdup(value);
    if (newString)
    {
        pool.insert(newString);
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

    for (auto iter = pool.begin(), end = pool.end(); iter != end; ++iter)
    {
        free(static_cast<void*>(const_cast<char*>(*iter)));
    }

    // remove pointers from pool

    pool.clear();
}

CTSVNPath SVNFolderStatus::folderPath;

SVNFolderStatus::SVNFolderStatus()
    : m_nCounter(0)
    , m_timeStamp(0)
    , dirStatus(nullptr)
    , m_mostRecentStatus(nullptr)
{
    emptyString[0]             = 0;
    invalidStatus.author       = emptyString;
    invalidStatus.askedCounter = -1;
    invalidStatus.status       = svn_wc_status_none;
    invalidStatus.url          = emptyString;
    invalidStatus.rev          = -1;
    invalidStatus.owner        = emptyString;
    invalidStatus.needsLock    = false;
    invalidStatus.treeConflict = false;

    sCacheKey.reserve(MAX_PATH);

    rootPool = svn_pool_create(NULL);

    m_hInvalidationEvent = CreateEvent(nullptr, FALSE, FALSE, L"TortoiseSVNCacheInvalidationEvent");
}

SVNFolderStatus::~SVNFolderStatus()
{
    svn_pool_destroy(rootPool);
}

const FileStatusCacheEntry* SVNFolderStatus::BuildCache(const CTSVNPath& filePath, BOOL bIsFolder, BOOL bDirectFolder)
{
    svn_client_ctx_t* localCtx = nullptr;
    apr_pool_t*       pool     = nullptr;
    svn_error_t*      err      = nullptr; // If svn_client_status comes out through catch(...), err would else be unassigned

    //dont' build the cache if an instance of TortoiseProc is running
    //since this could interfere with svn commands running (concurrent
    //access of the .svn directory).
    if (g_shellCache.BlockStatus())
    {
        CAutoGeneralHandle TSVNMutex = ::CreateMutex(nullptr, FALSE, L"TortoiseProc.exe");
        if (TSVNMutex != nullptr)
        {
            if (::GetLastError() == ERROR_ALREADY_EXISTS)
            {
                return &invalidStatus;
            }
        }
    }

    pool = svn_pool_create(rootPool); // create the memory pool

    ClearCache();
    svn_error_clear(svn_client_create_context2(&localCtx, nullptr, pool));
    // set up the configuration
    // Note: I know this is an 'expensive' call, but without this, ignores
    // done in the global ignore pattern won't show up.
    if (g_unversionedOvlLoaded)
        localCtx->config = SVNConfig::Instance().GetConfig(pool);

    // strings pools are unused, now -> we may clear them

    authors.clear();
    urls.clear();
    owners.clear();

    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": building cache for %s\n", filePath.GetWinPath());
    if (bIsFolder)
    {
        if (bDirectFolder)
        {
            // initialize record members
            dirStat.rev          = -1;
            dirStat.status       = svn_wc_status_none;
            dirStat.author       = authors.GetString(nullptr);
            dirStat.url          = urls.GetString(nullptr);
            dirStat.owner        = owners.GetString(nullptr);
            dirStat.askedCounter = SVNFOLDERSTATUS_CACHETIMES;
            dirStat.needsLock    = false;
            dirStat.treeConflict = false;
            dirStat.lock         = nullptr;

            dirStatus                   = nullptr;
            svn_revnum_t       youngest = SVN_INVALID_REVNUM;
            svn_opt_revision_t rev;
            rev.kind = svn_opt_revision_unspecified;
            try
            {
                folderPath             = filePath.GetDirectory();
                const char* svnapipath = folderPath.GetSVNApiPath(pool);
                if (svnapipath && svnapipath[0])
                {
                    err = svn_client_status6(&youngest,
                                             localCtx,
                                             svnapipath,
                                             &rev,
                                             svn_depth_empty, // depth
                                             TRUE,            // get all
                                             FALSE,           // check out-of-date
                                             true,            // check working copy
                                             TRUE,            // no ignore
                                             FALSE,           // ignore externals
                                             TRUE,            // depth as sticky
                                             nullptr,
                                             findFolderStatus,
                                             this,
                                             pool);
                }
                else
                    dirStatus = nullptr;
            }
            catch (...)
            {
                dirStatus = nullptr;
            }

            if (dirStatus)
            {
                dirStat.author = authors.GetString(dirStatus->changed_author);
                dirStat.url    = authors.GetString(dirStatus->repos_relpath);
                dirStat.rev    = dirStatus->changed_rev;
                if (dirStatus->lock)
                    dirStat.owner = owners.GetString(dirStatus->lock->owner);

                dirStat.status       = dirStatus->node_status;
                dirStat.treeConflict = dirStatus->conflicted != 0;
                dirStat.lock         = dirStatus->lock;
            }
            m_cache[filePath.GetWinPath()] = dirStat;
            m_timeStamp                    = GetTickCount64();
            svn_error_clear(err);
            svn_pool_destroy(pool); //free allocated memory
            return &dirStat;
        }
    } // if (bIsFolder)

    m_nCounter = 0;

    //Fill in the cache with
    //all files inside the same folder as the asked file/folder is
    //since subversion can do this in one step
    localCtx->auth_baton = nullptr;

    svn_revnum_t       youngest = SVN_INVALID_REVNUM;
    svn_opt_revision_t rev;
    rev.kind = svn_opt_revision_unspecified;
    try
    {
        CTSVNPath   dirPath    = filePath.GetDirectory();
        const char* svnApiPath = dirPath.GetSVNApiPath(pool);
        if (svnApiPath && svnApiPath[0])
        {
            err = svn_client_status6(&youngest,
                                     localCtx,
                                     svnApiPath,
                                     &rev,
                                     svn_depth_immediates, // depth
                                     TRUE,                 // get all
                                     FALSE,                // check out-of-date
                                     TRUE,                 // check working copy
                                     TRUE,                 // no ignore
                                     FALSE,                // ignore externals
                                     TRUE,                 // depth as sticky
                                     nullptr,
                                     fillStatusMap,
                                     this,
                                     pool);
        }
        else
        {
            svn_pool_destroy(pool);
            return &invalidStatus;
        }
    }
    catch (...)
    {
    }

    // Error present if function is not under version control
    if (err != nullptr)
    {
        svn_error_clear(err);
        svn_pool_destroy(pool); //free allocated memory
        return &invalidStatus;
    }

    svn_error_clear(err);
    svn_pool_destroy(pool); //free allocated memory
    m_timeStamp                       = GetTickCount64();
    const FileStatusCacheEntry*   ret = nullptr;
    FileStatusMap::const_iterator iter;
    if ((iter = m_cache.find(filePath.GetWinPath())) != m_cache.end())
    {
        ret                = &iter->second;
        m_mostRecentPath   = filePath;
        m_mostRecentStatus = ret;
    }
    else
    {
        // for SUBST'ed drives, Subversion doesn't return a path with a backslash
        // e.g. G:\ but only G: when fetching the status. So search for that
        // path too before giving up.
        // This is especially true when right-clicking directly on a SUBST'ed
        // drive to get the context menu
        if (wcslen(filePath.GetWinPath()) == 3)
        {
            if ((iter = m_cache.find(static_cast<LPCWSTR>(filePath.GetWinPathString().Left(2)))) != m_cache.end())
            {
                ret                = &iter->second;
                m_mostRecentPath   = filePath;
                m_mostRecentStatus = ret;
            }
        }
    }
    if (ret)
        return ret;
    return &invalidStatus;
}

ULONGLONG SVNFolderStatus::GetTimeoutValue() const
{
    ULONGLONG timeout = SVNFOLDERSTATUS_CACHETIMEOUT;
    ULONGLONG factor  = static_cast<ULONGLONG>(m_cache.size()) / 200UL;
    if (factor == 0)
        factor = 1;
    return factor * timeout;
}

const FileStatusCacheEntry* SVNFolderStatus::GetFullStatus(const CTSVNPath& filePath, BOOL bIsFolder, BOOL bColumnProvider)
{
    const FileStatusCacheEntry* ret = nullptr;

    BOOL bHasAdminDir = g_shellCache.IsVersioned(filePath.GetWinPath(), !!bIsFolder, false);

    //no overlay for unversioned folders
    if ((!bColumnProvider) && (!bHasAdminDir))
        return &invalidStatus;
    //for the SVNStatus column, we have to check the cache to see
    //if it's not just unversioned but ignored
    ret = GetCachedItem(filePath);
    if ((ret) && (ret->status == svn_wc_status_unversioned) && (bIsFolder) && (bHasAdminDir))
    {
        // an 'unversioned' folder, but with an ADMIN dir --> nested layout!
        ret = BuildCache(filePath, bIsFolder, TRUE);
        if (ret)
            return ret;
        else
            return &invalidStatus;
    }
    if (ret)
        return ret;

    //if it's not in the cache and has no admin dir, then we assume
    //it's not ignored too
    if ((bColumnProvider) && (!bHasAdminDir))
        return &invalidStatus;
    ret = BuildCache(filePath, bIsFolder);
    if (ret)
        return ret;
    else
        return &invalidStatus;
}

const FileStatusCacheEntry* SVNFolderStatus::GetCachedItem(const CTSVNPath& filepath)
{
    sCacheKey.assign(filepath.GetWinPath());
    FileStatusMap::const_iterator iter;
    const FileStatusCacheEntry*   retVal;

    if (m_mostRecentPath.IsEquivalentTo(CTSVNPath(sCacheKey.c_str())))
    {
        // We've hit the same result as we were asked for last time
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": fast cache hit for %s\n", filepath.GetWinPath());
        retVal = m_mostRecentStatus;
    }
    else if ((iter = m_cache.find(sCacheKey)) != m_cache.end())
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": cache found for %s\n", filepath.GetWinPath());
        retVal             = &iter->second;
        m_mostRecentStatus = retVal;
        m_mostRecentPath   = CTSVNPath(sCacheKey.c_str());
    }
    else
    {
        retVal = nullptr;
    }

    if (retVal != nullptr)
    {
        // We found something in a cache - check that the cache is not timed-out or force-invalidated
        ULONGLONG now = GetTickCount64();

        if ((now >= m_timeStamp) && ((now - m_timeStamp) > GetTimeoutValue()))
        {
            // Cache is timed-out
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Cache timed-out\n");
            ClearCache();
            retVal = nullptr;
        }
        else if (WaitForSingleObject(m_hInvalidationEvent, 0) == WAIT_OBJECT_0)
        {
            // TortoiseProc has just done something which has invalidated the cache
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Cache invalidated\n");
            ClearCache();
            retVal = nullptr;
        }
        return retVal;
    }
    return nullptr;
}

svn_error_t* SVNFolderStatus::fillStatusMap(void* baton, const char* path, const svn_client_status_t* status, apr_pool_t* /*pool*/)
{
    SVNFolderStatus*     stat  = static_cast<SVNFolderStatus*>(baton);
    FileStatusMap*       cache = &stat->m_cache;
    FileStatusCacheEntry s;
    if (status)
    {
        s.author = stat->authors.GetString(status->changed_author);
        s.url    = stat->urls.GetString(status->repos_relpath);
        s.rev    = status->changed_rev;
        if (status->lock)
            s.owner = stat->owners.GetString(status->lock->owner);
        s.status       = SVNStatus::GetMoreImportant(s.status, status->node_status);
        s.lock         = status->repos_lock;
        s.treeConflict = (status->conflicted != 0);
    }
    std::wstring str;
    if (path)
    {
        str = CUnicodeUtils::StdGetUnicode(path);
        std::replace(str.begin(), str.end(), '/', '\\');
    }
    else
        str = L" ";
    (*cache)[str] = s;

    return nullptr;
}

svn_error_t* SVNFolderStatus::findFolderStatus(void* baton, const char* path, const svn_client_status_t* status, apr_pool_t* /*pool*/)
{
    SVNFolderStatus* stat = static_cast<SVNFolderStatus*>(baton);
    if ((stat) && (stat->folderPath.IsEquivalentTo(CTSVNPath(CString(path)))))
    {
        stat->dirStatus = status;
    }

    return nullptr;
}

void SVNFolderStatus::ClearCache()
{
    m_cache.clear();
    m_mostRecentStatus = nullptr;
    m_mostRecentPath.Reset();
    // If we're about to rebuild the cache, there's no point hanging on to
    // an event which tells us that it's invalid
    ResetEvent(m_hInvalidationEvent);
}
