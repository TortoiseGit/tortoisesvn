// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2011 - TortoiseSVN

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
#include "StdAfx.h"
#include ".\cacheddirectory.h"
#include "SVNHelpers.h"
#include "SVNStatusCache.h"
#include "SVNStatus.h"
#include "UnicodeUtils.h"
#include "SmartHandle.h"
#include <set>

#define CACHEDIRECTORYDISKVERSION 3

CCachedDirectory::CCachedDirectory(void)
    : m_wcDbFileTime(0)
    , m_bCurrentFullStatusValid(false)
    , m_currentFullStatus(svn_wc_status_none)
    , m_mostImportantFileStatus(svn_wc_status_none)
    , m_pCtx(NULL)
    , m_FetchingStatus(FALSE)
{
}

CCachedDirectory::~CCachedDirectory(void)
{
}

CCachedDirectory::CCachedDirectory(const CTSVNPath& directoryPath)
    : m_wcDbFileTime(0)
    , m_bCurrentFullStatusValid(false)
    , m_currentFullStatus(svn_wc_status_none)
    , m_mostImportantFileStatus(svn_wc_status_none)
    , m_pCtx(NULL)
    , m_FetchingStatus(FALSE)
{
    ATLASSERT(directoryPath.IsDirectory() || !PathFileExists(directoryPath.GetWinPath()));

    m_directoryPath = directoryPath;
}

BOOL CCachedDirectory::SaveToDisk(FILE * pFile)
{
    AutoLocker lock(m_critSec);
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;

    unsigned int value = CACHEDIRECTORYDISKVERSION;
    WRITEVALUETOFILE(value);    // 'version' of this save-format
    value = (int)m_entryCache.size();
    WRITEVALUETOFILE(value);    // size of the cache map
    // now iterate through the maps and save every entry.
    for (CacheEntryMap::iterator I = m_entryCache.begin(); I != m_entryCache.end(); ++I)
    {
        const CStringA& key = I->first;
        value = key.GetLength();
        WRITEVALUETOFILE(value);
        if (value)
        {
            if (fwrite((LPCSTR)key, sizeof(char), value, pFile)!=value)
                return false;
            if (!I->second.SaveToDisk(pFile))
                return false;
        }
    }
    value = (int)m_childDirectories.size();
    WRITEVALUETOFILE(value);
    for (ChildDirStatus::iterator I = m_childDirectories.begin(); I != m_childDirectories.end(); ++I)
    {
        value = I->first.GetLength();
        WRITEVALUETOFILE(value);
        if (value)
        {
            if (fwrite((LPCSTR)I->first, sizeof(char), value, pFile)!=value)
                return false;
            svn_wc_status_kind status = I->second;
            WRITEVALUETOFILE(status);
        }
    }
    WRITEVALUETOFILE(m_wcDbFileTime);
    value = m_directoryPath.GetWinPathString().GetLength();
    WRITEVALUETOFILE(value);
    if (value)
    {
        if (fwrite(m_directoryPath.GetWinPath(), sizeof(TCHAR), value, pFile)!=value)
            return false;
    }
    if (!m_ownStatus.SaveToDisk(pFile))
        return false;
    WRITEVALUETOFILE(m_currentFullStatus);
    WRITEVALUETOFILE(m_mostImportantFileStatus);
    return true;
}

BOOL CCachedDirectory::LoadFromDisk(FILE * pFile)
{
    AutoLocker lock(m_critSec);
#define LOADVALUEFROMFILE(x) if (fread(&x, sizeof(x), 1, pFile)!=1) return false;
    try
    {
        unsigned int value = 0;
        LOADVALUEFROMFILE(value);
        if (value != CACHEDIRECTORYDISKVERSION)
            return false;       // not the correct version

        int mapsize = 0;
        LOADVALUEFROMFILE(mapsize);
        for (int i=0; i<mapsize; ++i)
        {
            LOADVALUEFROMFILE(value);
            if (value > MAX_PATH)
                return false;
            if (value)
            {
                CStringA sKey;
                if (fread(sKey.GetBuffer(value+1), sizeof(char), value, pFile)!=value)
                {
                    sKey.ReleaseBuffer(0);
                    return false;
                }
                sKey.ReleaseBuffer(value);
                CStatusCacheEntry entry;
                if (!entry.LoadFromDisk(pFile))
                    return false;
                m_entryCache[sKey] = entry;
            }
        }
        LOADVALUEFROMFILE(mapsize);
        for (int i=0; i<mapsize; ++i)
        {
            LOADVALUEFROMFILE(value);
            if (value > MAX_PATH)
                return false;
            if (value)
            {
                CStringA sPath;
                if (fread(sPath.GetBuffer(value), sizeof(char), value, pFile)!=value)
                {
                    sPath.ReleaseBuffer(0);
                    return false;
                }
                sPath.ReleaseBuffer(value);
                svn_wc_status_kind status;
                LOADVALUEFROMFILE(status);
                m_childDirectories[sPath] = status;
            }
        }
        LOADVALUEFROMFILE(m_wcDbFileTime);
        LOADVALUEFROMFILE(value);
        if (value > MAX_PATH)
            return false;
        if (value)
        {
            CString sPath;
            if (fread(sPath.GetBuffer(value+1), sizeof(TCHAR), value, pFile)!=value)
            {
                sPath.ReleaseBuffer(0);
                return false;
            }
            sPath.ReleaseBuffer(value);
            m_directoryPath.SetFromWin(sPath);
        }
        if (!m_ownStatus.LoadFromDisk(pFile))
            return false;

        LOADVALUEFROMFILE(m_currentFullStatus);
        LOADVALUEFROMFILE(m_mostImportantFileStatus);
    }
    catch ( CAtlException )
    {
        return false;
    }
    return true;

}

CStatusCacheEntry CCachedDirectory::GetCacheStatusForMember( const CTSVNPath& path )
{
    AutoLocker lock(m_critSec);
    CStringA strCacheKey = GetCacheKey(path);
    CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
    if(itMap != m_entryCache.end())
    {
        return itMap->second;
    }
    return CStatusCacheEntry();
}

CStatusCacheEntry CCachedDirectory::GetStatusForMember(const CTSVNPath& path, bool bRecursive,  bool bFetch /* = true */)
{
    CStringA strCacheKey;
    bool bThisDirectoryIsUnversioned = false;
    bool bRequestForSelf = false;
    if(path.IsEquivalentToWithoutCase(m_directoryPath))
    {
        bRequestForSelf = true;
    }

    // In all most circumstances, we ask for the status of a member of this directory.
    ATLASSERT(m_directoryPath.IsEquivalentToWithoutCase(path.GetContainingDirectory()) || bRequestForSelf);

    bool wcDbFileTimeChanged = false;
    long long dbFileTime = CSVNStatusCache::Instance().WCRoots()->GetDBFileTime(m_directoryPath);
    wcDbFileTimeChanged = (m_wcDbFileTime != dbFileTime);

    if ( !wcDbFileTimeChanged )
    {
        if(m_wcDbFileTime == 0)
        {
            // We are a folder which is not in a working copy
            bThisDirectoryIsUnversioned = true;
            m_ownStatus.SetStatus(NULL, false, false);

            // If a user removes the .svn directory, we get here with m_entryCache
            // not being empty, but still us being unversioned
            if (!m_entryCache.empty())
            {
                m_entryCache.clear();
            }
            ATLASSERT(m_entryCache.empty());

            // However, a member *DIRECTORY* might be the top of WC
            // so we need to ask them to get their own status
            if(!path.IsDirectory())
            {
                if ((PathFileExists(path.GetWinPath()))||(bRequestForSelf))
                    return CStatusCacheEntry();
                // the entry doesn't exist anymore!
                // but we can't remove it from the cache here:
                // the GetStatusForMember() method is called only with a read
                // lock and not a write lock!
                // So mark it for crawling, and let the crawler remove it
                // later
                CSVNStatusCache::Instance().AddFolderForCrawling(path.GetContainingDirectory());
                return CStatusCacheEntry();
            }
            else
            {
                // If we're in the special case of a directory being asked for its own status
                // and this directory is unversioned, then we should just return that here
                if(bRequestForSelf)
                {
                    return CStatusCacheEntry();
                }
            }
        }

        if (CSVNStatusCache::Instance().GetDirectoryCacheEntryNoCreate(path) != NULL)
        {
            // We don't have directory status in our cache
            // Ask the directory if it knows its own status
            CCachedDirectory * dirEntry = CSVNStatusCache::Instance().GetDirectoryCacheEntry(path);
            if ((dirEntry)&&(dirEntry->IsOwnStatusValid()))
            {
                // To keep recursive status up to date, we'll request that children are all crawled again
                // We have to do this because the directory watcher isn't very reliable (especially under heavy load)
                // and also has problems with SUBSTed drives.
                // If nothing has changed in those directories, this crawling is fast and only
                // accesses two files for each directory.
                if (bRecursive)
                {
                    AutoLocker lock(dirEntry->m_critSec);
                    ChildDirStatus::const_iterator it;
                    for(it = dirEntry->m_childDirectories.begin(); it != dirEntry->m_childDirectories.end(); ++it)
                    {
                        CTSVNPath newpath;
                        CString winPath = CUnicodeUtils::GetUnicode (it->first);
                        newpath.SetFromWin(winPath, true);

                        CSVNStatusCache::Instance().AddFolderForCrawling(newpath);
                    }
                }

                return dirEntry->GetOwnStatus(bRecursive);
            }
        }
        else
        {
            {
                // if we currently are fetching the status of the directory
                // we want the status for, we just return an empty entry here
                // and don't wait for that fetching to finish.
                // That's because fetching the status can take a *really* long
                // time (e.g. if a commit is also in progress on that same
                // directory), and we don't want to make the explorer appear
                // to hang.
                if ((!bFetch)&&(m_FetchingStatus))
                {
                    if (m_directoryPath.IsAncestorOf(path))
                    {
                        m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
                        return GetCacheStatusForMember(path);
                    }
                }
            }
            // Look up a file in our own cache
            AutoLocker lock(m_critSec);
            strCacheKey = GetCacheKey(path);
            CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
            if(itMap != m_entryCache.end())
            {
                // We've hit the cache - check for timeout
                if(!itMap->second.HasExpired((long)GetTickCount()))
                {
                    if(itMap->second.DoesFileTimeMatch(path.GetLastWriteTime()))
                    {
                        if ((itMap->second.GetEffectiveStatus()!=svn_wc_status_missing)||(!PathFileExists(path.GetWinPath())))
                        {
                            // Note: the filetime matches after a modified has been committed too.
                            // So in that case, we would return a wrong status (e.g. 'modified' instead
                            // of 'normal') here.
                            return itMap->second;
                        }
                    }
                }
            }
        }
    }
    else
    {
        if ((!bFetch)&&(m_FetchingStatus))
        {
            if (m_directoryPath.IsAncestorOf(path))
            {
                // returning empty status (status fetch in progress)
                // also set the status to 'none' to have the status change and
                // the shell updater invoked in the crawler
                m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
                CSVNStatusCache::Instance().AddFolderForCrawling(m_directoryPath.GetDirectory());
                return GetCacheStatusForMember(path);
            }
        }
        // if we're fetching the status for the explorer,
        // we don't refresh the status but use the one
        // we already have (to save time and make the explorer
        // more responsive in stress conditions).
        // We leave the refreshing to the crawler.
        if ((!bFetch)&&(m_wcDbFileTime))
        {
            CSVNStatusCache::Instance().AddFolderForCrawling(m_directoryPath.GetDirectory());
            return GetCacheStatusForMember(path);
        }
        AutoLocker lock(m_critSec);
        m_entryCache.clear();
        strCacheKey = GetCacheKey(path);
    }

    // We've not got this item in the cache - let's add it
    // We never bother asking SVN for the status of just one file, always for its containing directory

    if (g_SVNAdminDir.IsAdminDirPath(path.GetWinPathString()))
    {
        // We're being asked for the status of an .SVN directory
        // It's not worth asking for this
        return CStatusCacheEntry();
    }

    {
        if ((!bFetch)&&(m_FetchingStatus))
        {
            if (m_directoryPath.IsAncestorOf(path))
            {
                m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
                return GetCacheStatusForMember(path);
            }
        }
    }

    {
        AutoLocker lock(m_critSec);
        m_mostImportantFileStatus = svn_wc_status_none;
        m_childDirectories.clear();
        m_entryCache.clear();
        m_ownStatus.SetStatus(NULL, false, false);
    }
    if(!bThisDirectoryIsUnversioned)
    {
        if (!SvnUpdateMembersStatus())
        {
            m_wcDbFileTime = 0;
            return CStatusCacheEntry();
        }
    }
    // Now that we've refreshed our SVN status, we can see if it's
    // changed the 'most important' status value for this directory.
    // If it has, then we should tell our parent
    UpdateCurrentStatus();

    m_wcDbFileTime = dbFileTime;
    if (path.IsDirectory())
    {
        CCachedDirectory * dirEntry = CSVNStatusCache::Instance().GetDirectoryCacheEntry(path);
        if ((dirEntry)&&(dirEntry->IsOwnStatusValid()))
        {
            //CSVNStatusCache::Instance().AddFolderForCrawling(path);
            return dirEntry->GetOwnStatus(bRecursive);
        }

        // If the status *still* isn't valid here, it means that
        // the current directory is unversioned, and we shall need to ask its children for info about themselves
        if ((dirEntry)&&(dirEntry != this))
            return dirEntry->GetStatusForMember(path,bRecursive);
        // add the path for crawling: if it's really unversioned, the crawler will
        // only check for the admin dir and do nothing more. But if it is
        // versioned (could happen in a nested layout) the crawler will update its
        // status correctly
        CSVNStatusCache::Instance().AddFolderForCrawling(path);
        return CStatusCacheEntry();
    }
    else
    {
        CacheEntryMap::iterator itMap = m_entryCache.find(strCacheKey);
        if(itMap != m_entryCache.end())
        {
            return itMap->second;
        }
    }

    AddEntry(path, NULL, false, false);
    return CStatusCacheEntry();
}

void
CCachedDirectory::AddEntry(const CTSVNPath& path, const svn_client_status_t* pSVNStatus, bool needsLock, bool forceNormal)
{
    svn_wc_status_kind nodestatus = forceNormal ? svn_wc_status_normal : (pSVNStatus ? pSVNStatus->node_status : svn_wc_status_none);
    if(path.IsDirectory())
    {
        // no lock here:
        // AutoLocker lock(m_critSec);
        // because GetDirectoryCacheEntry() can try to obtain a write lock
        CCachedDirectory * childDir = CSVNStatusCache::Instance().GetDirectoryCacheEntry(path);
        if (childDir)
        {
            if ((childDir->GetCurrentFullStatus() != svn_wc_status_ignored)||(pSVNStatus==NULL)||(nodestatus != svn_wc_status_unversioned))
                childDir->m_ownStatus.SetStatus(pSVNStatus, needsLock, forceNormal);
            childDir->m_ownStatus.SetKind(svn_node_dir);
        }
    }
    else
    {
        AutoLocker lock(m_critSec);
        CStringA cachekey = GetCacheKey(path);
        CacheEntryMap::iterator entry_it = m_entryCache.lower_bound(cachekey);
        if (entry_it != m_entryCache.end() && entry_it->first == cachekey)
        {
            if (pSVNStatus)
            {
                if (entry_it->second.GetEffectiveStatus() > svn_wc_status_none &&
                    entry_it->second.GetEffectiveStatus() != nodestatus)
                {
                    CSVNStatusCache::Instance().UpdateShell(path);
                    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": shell update for %s\n"), path.GetWinPath());
                }
            }
        }
        else
        {
            entry_it = m_entryCache.insert(entry_it, std::make_pair(cachekey, CStatusCacheEntry()));
        }
        entry_it->second = CStatusCacheEntry(pSVNStatus, needsLock, path.GetLastWriteTime(), forceNormal);
    }
}


CStringA
CCachedDirectory::GetCacheKey(const CTSVNPath& path)
{
    // All we put into the cache as a key is just the end portion of the pathname
    // There's no point storing the path of the containing directory for every item
    CString lastElement
        = path.GetWinPathString().Mid(m_directoryPath.GetWinPathString().GetLength());
    return CUnicodeUtils::GetUTF8 (lastElement);
}

CString
CCachedDirectory::GetFullPathString(const CStringA& cacheKey)
{
    return m_directoryPath.GetWinPathString()
        + CUnicodeUtils::GetUnicode (cacheKey);
}


bool
CCachedDirectory::SvnUpdateMembersStatus()
{
    if (InterlockedExchange(&m_FetchingStatus, TRUE))
        return false;

    svn_opt_revision_t revision;
    revision.kind = svn_opt_revision_unspecified;

    SVNPool subPool(CSVNStatusCache::Instance().m_svnHelp.Pool());
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": stat for %s\n"), m_directoryPath.GetWinPath());

    const char * svnapipath = m_directoryPath.GetSVNApiPath(subPool);
    if ((svnapipath == 0)||(svnapipath[0] == 0))
    {
        InterlockedExchange(&m_FetchingStatus, FALSE);
        m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
        CSVNStatusCache::Instance().BlockPath(m_directoryPath, true, 5);
        return false;
    }
    m_pCtx = CSVNStatusCache::Instance().m_svnHelp.ClientContext(subPool);
    svn_error_t * pErr = nullptr;
    if (m_pCtx)
    {
        pErr = svn_client_status5 (
                                   NULL,
                                   m_pCtx,
                                   svnapipath,
                                   &revision,
                                   svn_depth_immediates,
                                   TRUE,       // get all
                                   FALSE,      // update
                                   TRUE,       // no ignores
                                   FALSE,      // ignore externals
                                   TRUE,       // depth as sticky
                                   NULL,       // changelists
                                   GetStatusCallback,
                                   this,
                                   subPool
                                   );

        svn_wc_context_destroy(m_pCtx->wc_ctx);
        m_pCtx->wc_ctx = NULL;
        m_pCtx = NULL;
    }
    else
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": error creating client context!\n");
        m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
        // Since we only assume a none status here due to the fact that we couldn't get a client context,
        // make sure that this status times out soon.
        CSVNStatusCache::Instance().m_folderCrawler.BlockPath(m_directoryPath, 20);
        CSVNStatusCache::Instance().AddFolderForCrawling(m_directoryPath);
    }
    InterlockedExchange(&m_FetchingStatus, FALSE);
    if (pErr)
    {
        // Handle an error
        // The most likely error on a folder is that it's not part of a WC
        // In most circumstances, this will have been caught earlier,
        // but in some situations, we'll get this error.
        // If we allow ourselves to fall on through, then folders will be asked
        // for their own status, and will set themselves as unversioned, for the
        // benefit of future requests
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": svn_cli_stat error '%s'\n", pErr->message);
        // No assert here! Since we _can_ get here, an assertion is not an option!
        // Reasons to get here:
        // - renaming a folder with many sub folders --> results in "not a working copy" if the revert
        //   happens between our checks and the svn_client_status() call.
        // - reverting a move/copy --> results in "not a working copy" (as above)
        switch (pErr->apr_err)
        {
        case SVN_ERR_WC_NOT_WORKING_COPY:
        case SVN_ERR_WC_PATH_NOT_FOUND:
            {
                m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
                CSVNStatusCache::Instance().BlockPath(m_directoryPath, true);
            }
            break;
        case SVN_ERR_WC_NOT_FILE:
        case SVN_ERR_WC_CORRUPT:
        case SVN_ERR_WC_CORRUPT_TEXT_BASE:
        case SVN_ERR_WC_UNSUPPORTED_FORMAT:
        case SVN_ERR_WC_DB_ERROR:
        case SVN_ERR_WC_MISSING:
        case SVN_ERR_WC_PATH_UNEXPECTED_STATUS:
        case SVN_ERR_WC_UPGRADE_REQUIRED:
        case SVN_ERR_WC_CLEANUP_REQUIRED:
            {
                m_currentFullStatus = m_mostImportantFileStatus = svn_wc_status_none;
            }
            break;
        default:
            {
                // Since we only assume a none status here due to svn_client_status()
                // returning an error, make sure that this status times out soon.
                CSVNStatusCache::Instance().m_folderCrawler.BlockPath(m_directoryPath, 20);
                CSVNStatusCache::Instance().AddFolderForCrawling(m_directoryPath);
            }
            break;
        }
        svn_error_clear(pErr);

        return false;
    }

    RefreshMostImportant(false);
    return true;
}

svn_error_t * CCachedDirectory::GetStatusCallback(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t * pool)
{
    CCachedDirectory* pThis = (CCachedDirectory*)baton;

    if (path == NULL)
        return SVN_NO_ERROR;

    CTSVNPath svnPath;
    bool forceNormal = false;
    bool needsLock = false;

    const svn_wc_status_kind nodeStatus = status->node_status;
    if(status->versioned)
    {
        if ((nodeStatus != svn_wc_status_none)&&(nodeStatus != svn_wc_status_ignored))
            svnPath.SetFromSVN(path, (status->kind == svn_node_dir));
        else
            svnPath.SetFromSVN(path);

        if(svnPath.IsDirectory())
        {
            if(!svnPath.IsEquivalentToWithoutCase(pThis->m_directoryPath))
            {
                // Make sure we know about this child directory
                // This initial status value is likely to be overwritten from below at some point
                svn_wc_status_kind s = nodeStatus;
                if (status->conflicted)
                    s = SVNStatus::GetMoreImportant(s, svn_wc_status_conflicted);
                CCachedDirectory * cdir = CSVNStatusCache::Instance().GetDirectoryCacheEntryNoCreate(svnPath);
                if (cdir)
                {
                    // This child directory is already in our cache!
                    // So ask this dir about its recursive status
                    svn_wc_status_kind st = SVNStatus::GetMoreImportant(s, cdir->GetCurrentFullStatus());
                    pThis->SetChildStatus(svnPath, st);
                }
                else
                {
                    // the child directory is not in the cache. Create a new entry for it in the cache which is
                    // initially 'unversioned'. But we added that directory to the crawling list above, which
                    // means the cache will be updated soon.
                    CSVNStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
                    pThis->SetChildStatus(svnPath, s);
                }
            }
        }
        else
        {
            // only fetch the svn:needs-lock property if the status of this file is 'normal', because
            // if the status is something else, the needs-lock overlay won't show up anyway
            if ((pThis->m_pCtx)&&(nodeStatus == svn_wc_status_normal))
            {
                const svn_string_t * value = NULL;
                svn_error_t * err = svn_wc_prop_get2(&value, pThis->m_pCtx->wc_ctx, path, "svn:needs-lock", pool, pool);
                if ((err==NULL) && value)
                    needsLock = true;
                if (err)
                    svn_error_clear(err);
            }
        }
    }
    else
    {
        if ((status->kind != svn_node_unknown)&&(status->kind != svn_node_none))
            svnPath.SetFromSVN(path, status->kind == svn_node_dir);
        else
            svnPath.SetFromSVN(path);

        // Subversion returns no 'entry' field for versioned folders if they're
        // part of another working copy (nested layouts).
        // So we have to make sure that such an 'unversioned' folder really
        // is unversioned.
        if (((nodeStatus == svn_wc_status_unversioned)||(nodeStatus == svn_wc_status_ignored))&&(!svnPath.IsEquivalentToWithoutCase(pThis->m_directoryPath))&&(svnPath.IsDirectory()))
        {
            if (svnPath.IsWCRoot())
            {
                CSVNStatusCache::Instance().AddFolderForCrawling(svnPath);
                // Mark the directory as 'versioned' (status 'normal' for now).
                // This initial value will be overwritten from below some time later
                pThis->SetChildStatus(svnPath, svn_wc_status_normal);
                // Make sure the entry is also in the cache
                CSVNStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
                // also mark the status in the status object as normal
                forceNormal = true;
            }
            else
            {
                pThis->SetChildStatus(svnPath, nodeStatus);
            }
        }
        else if (nodeStatus == svn_wc_status_external)
        {
            if ((status->kind == svn_node_dir) || (svnPath.IsDirectory()))
            {
                CSVNStatusCache::Instance().AddFolderForCrawling(svnPath);
                // Mark the directory as 'versioned' (status 'normal' for now).
                // This initial value will be overwritten from below some time later
                pThis->SetChildStatus(svnPath, svn_wc_status_normal);
                // we have added a directory to the child-directory list of this
                // directory. We now must make sure that this directory also has
                // an entry in the cache.
                CSVNStatusCache::Instance().GetDirectoryCacheEntry(svnPath);
                // also mark the status in the status object as normal
                forceNormal = true;
            }
        }
        else
        {
            if (svnPath.IsDirectory())
            {
                svn_wc_status_kind s = nodeStatus;
                if (status->conflicted)
                    s = SVNStatus::GetMoreImportant(s, svn_wc_status_conflicted);
                pThis->SetChildStatus(svnPath, s);
            }
        }
    }

    pThis->AddEntry(svnPath, status, needsLock, forceNormal);

    return SVN_NO_ERROR;
}

bool
CCachedDirectory::IsOwnStatusValid() const
{
    return m_ownStatus.HasBeenSet() &&
           !m_ownStatus.HasExpired(GetTickCount()) &&
           // 'external' isn't a valid status. That just
           // means the folder is not part of the current working
           // copy but it still has its own 'real' status
           m_ownStatus.GetEffectiveStatus()!=svn_wc_status_external &&
           m_ownStatus.IsKindKnown();
}

void CCachedDirectory::Invalidate()
{
    m_ownStatus.Invalidate();
}

svn_wc_status_kind CCachedDirectory::CalculateRecursiveStatus()
{
    // Combine our OWN folder status with the most important of our *FILES'* status.
    svn_wc_status_kind retVal = SVNStatus::GetMoreImportant(m_mostImportantFileStatus, m_ownStatus.GetEffectiveStatus());

    if ((retVal != svn_wc_status_modified)&&(retVal != m_ownStatus.GetEffectiveStatus()))
    {
        if ((retVal == svn_wc_status_added)||(retVal == svn_wc_status_deleted)||(retVal == svn_wc_status_missing))
            retVal = svn_wc_status_modified;
    }

    // Now combine all our child-directories status

    AutoLocker lock(m_critSec);
    ChildDirStatus::const_iterator it;
    for (it = m_childDirectories.begin(); it != m_childDirectories.end(); ++it)
    {
        retVal = SVNStatus::GetMoreImportant(retVal, it->second);

        if ( ((it->second == svn_wc_status_none)||(it->second == svn_wc_status_unversioned)) &&
            (retVal == svn_wc_status_normal)&&(CSVNStatusCache::Instance().IsUnversionedAsModified()))
        {
            retVal = svn_wc_status_modified;
        }

        if ((retVal != svn_wc_status_modified)&&(retVal != m_ownStatus.GetEffectiveStatus()))
        {
            if ((retVal == svn_wc_status_added)||(retVal == svn_wc_status_deleted)||(retVal == svn_wc_status_missing))
                retVal = svn_wc_status_modified;
        }
    }

    return retVal;
}

// Update our composite status and deal with things if it's changed
void CCachedDirectory::UpdateCurrentStatus()
{
    svn_wc_status_kind newStatus = CalculateRecursiveStatus();

    if ((newStatus != m_currentFullStatus)&&(m_ownStatus.IsVersioned()))
    {
        if ((m_currentFullStatus != svn_wc_status_none)&&(m_ownStatus.GetEffectiveStatus() != svn_wc_status_ignored))
        {
            // Our status has changed - tell the shell
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Dir %s, status change from %d to %d\n"), m_directoryPath.GetWinPath(), m_currentFullStatus, newStatus);
            CSVNStatusCache::Instance().UpdateShell(m_directoryPath);
        }
        if (m_ownStatus.GetEffectiveStatus() != svn_wc_status_ignored)
            m_currentFullStatus = newStatus;
        else
            m_currentFullStatus = svn_wc_status_ignored;
    }
    // And tell our parent, if we've got one...
    // we tell our parent *always* about our status, even if it hasn't
    // changed. This is to make sure that the parent has really our current
    // status - the parent can decide itself if our status has changed
    // or not.
    CTSVNPath parentPath = m_directoryPath.GetContainingDirectory();
    if(!parentPath.IsEmpty())
    {
        CCachedDirectory * cachedDir = CSVNStatusCache::Instance().GetDirectoryCacheEntry(parentPath);
        if (cachedDir)
            cachedDir->UpdateChildDirectoryStatus(m_directoryPath, m_currentFullStatus);
    }
}


// Receive a notification from a child that its status has changed
void CCachedDirectory::UpdateChildDirectoryStatus(const CTSVNPath& childDir, svn_wc_status_kind childStatus)
{
    CStringA winPath = CUnicodeUtils::GetUTF8 (childDir.GetWinPathString());
    svn_wc_status_kind currentStatus = svn_wc_status_none;
    {
        AutoLocker lock(m_critSec);
        auto it = m_childDirectories.find(winPath);
        if (it == m_childDirectories.end())
            return; // this is not a child, or at least not a child connected to the parent
        currentStatus = it->second;
    }
    if ((currentStatus != childStatus)||(!IsOwnStatusValid()))
    {
        SetChildStatus(winPath, childStatus);
        UpdateCurrentStatus();
    }
}

void CCachedDirectory::SetChildStatus(const CTSVNPath& childDir, svn_wc_status_kind childStatus)
{
    CStringA winPath = CUnicodeUtils::GetUTF8(childDir.GetWinPathString());
    SetChildStatus(winPath, childStatus);
}

void CCachedDirectory::SetChildStatus(const CStringA& winPath, svn_wc_status_kind childStatus)
{
    AutoLocker lock(m_critSec);
    m_childDirectories[winPath] = childStatus;
}

CStatusCacheEntry CCachedDirectory::GetOwnStatus(bool bRecursive)
{
    // Don't return recursive status if we're unversioned ourselves.
    if(bRecursive && m_ownStatus.GetEffectiveStatus() > svn_wc_status_unversioned)
    {
        CStatusCacheEntry recursiveStatus(m_ownStatus);
        UpdateCurrentStatus();
        recursiveStatus.ForceStatus(m_currentFullStatus);
        return recursiveStatus;
    }
    else
    {
        return m_ownStatus;
    }
}

void CCachedDirectory::RefreshStatus(bool bRecursive)
{
    // Make sure that our own status is up-to-date
    GetStatusForMember(m_directoryPath,bRecursive);

    CTSVNPathList updatePathList;
    CTSVNPathList crawlPathList;
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": RefreshStatus for %s\n"), m_directoryPath.GetWinPath());

    DWORD now = GetTickCount();
    {
        // get the file write times with FindFirstFile/FindNextFile since those
        // APIs only access the folder, not each file individually.
        // This reduces the disk access a *lot*.
        std::map<CStringA, ULONGLONG> filetimes;
        WIN32_FIND_DATA FindFileData;
        CAutoFindFile hFind = FindFirstFile(m_directoryPath.GetWinPathString() + L"\\*.*", &FindFileData);
        if (hFind)
        {
            while (FindNextFile(hFind, &FindFileData))
            {
                if ( (wcscmp(FindFileData.cFileName, L"..")==0) ||
                     (wcscmp(FindFileData.cFileName, L".")==0) )
                     continue;
                ULARGE_INTEGER ft;
                ft.LowPart = FindFileData.ftLastWriteTime.dwLowDateTime;
                ft.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;

                CStringA nameUTF8 = CUnicodeUtils::GetUTF8(FindFileData.cFileName);
                filetimes[nameUTF8] = ft.QuadPart;
            }
            hFind.CloseHandle(); // explicit close handle to shorten its life time
        }

        AutoLocker lock(m_critSec);
        // We also need to check if all our file members have the right date on them
        for (CacheEntryMap::iterator itMembers = m_entryCache.begin(); itMembers != m_entryCache.end(); ++itMembers)
        {
            if ((itMembers->first)&&(!itMembers->first.IsEmpty()))
            {
                CTSVNPath filePath (GetFullPathString (itMembers->first));
                if (!filePath.IsEquivalentToWithoutCase(m_directoryPath))
                {
                    // we only have file members in our entry cache
                    ATLASSERT(!itMembers->second.IsDirectory());

                    auto ftIt = filetimes.find(itMembers->first.Mid(1));
                    if (ftIt != filetimes.end())
                    {
                        ULONGLONG ft = ftIt->second;
                        if ((itMembers->second.HasExpired(now))||(!itMembers->second.DoesFileTimeMatch(ft)))
                        {
                            // We need to request this item as well
                            updatePathList.AddPath(filePath);
                        }
                    }
                }
            }
        }

        if (bRecursive)
        {
            // crawl all sub folders too! Otherwise a change deep inside the
            // tree which has changed won't get propagated up the tree.
            for(ChildDirStatus::const_iterator it = m_childDirectories.begin(); it != m_childDirectories.end(); ++it)
            {
                CTSVNPath path;
                CString winPath = CUnicodeUtils::GetUnicode (it->first);
                path.SetFromWin (winPath, true);

                crawlPathList.AddPath(path);
            }
        }
    }

    for (int i = 0; i < updatePathList.GetCount(); ++i)
        GetStatusForMember(updatePathList[i], bRecursive);

    for (int i = 0; i < crawlPathList.GetCount(); ++i)
        CSVNStatusCache::Instance().AddFolderForCrawling(crawlPathList[i]);
}

void CCachedDirectory::RefreshMostImportant(bool bUpdateShell /* = true */)
{
    CacheEntryMap::iterator itMembers;
    svn_wc_status_kind newStatus = m_ownStatus.GetEffectiveStatus();
    for (itMembers = m_entryCache.begin(); itMembers != m_entryCache.end(); ++itMembers)
    {
        newStatus = SVNStatus::GetMoreImportant(newStatus, itMembers->second.GetEffectiveStatus());
        if (((itMembers->second.GetEffectiveStatus() == svn_wc_status_unversioned)||(itMembers->second.GetEffectiveStatus() == svn_wc_status_none))
            &&(CSVNStatusCache::Instance().IsUnversionedAsModified()))
        {
            // treat unversioned files as modified
            if (newStatus != svn_wc_status_added)
                newStatus = SVNStatus::GetMoreImportant(newStatus, svn_wc_status_modified);
        }
    }
    if (bUpdateShell && newStatus != m_mostImportantFileStatus)
    {
        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": status change of path %s\n"), m_directoryPath.GetWinPath());
        CSVNStatusCache::Instance().UpdateShell(m_directoryPath);
    }
    m_mostImportantFileStatus = newStatus;
}

