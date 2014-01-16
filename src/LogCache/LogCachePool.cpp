// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012, 2014 - TortoiseSVN

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
#include "stdafx.h"
#include "LogCachePool.h"
#include "LogCacheSettings.h"
#include "Containers/CachedLogInfo.h"
#include "RepositoryInfo.h"

#include "DirFileEnum.h"
#include "PathUtils.h"
#include "SmartHandle.h"

// begin namespace LogCache

namespace LogCache
{

// use the same caches throughout this application
// (they are unique per computer anyway)

CLogCachePool::TCaches CLogCachePool::caches;
long CLogCachePool::instanceCount = 0;

// utility

bool CLogCachePool::FileExists (const std::wstring& filePath)
{
    return GetFileAttributes (filePath.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// minimize memory usage

void CLogCachePool::Clear()
{
    while (!caches.empty())
    {
        CCachedLogInfo* toDelete = caches.begin()->second;
        caches.erase (caches.begin());
        delete toDelete;
    }

    if (repositoryInfo != NULL)
        repositoryInfo->Clear();
}

// remove small, unused caches

void CLogCachePool::AutoRemoveUnused()
{
    std::set<CString> allFiles;
    std::set<CString> lockedCaches;
    std::set<CString> smallUnusedCaches;
    std::set<CString> oldLocks;

    // calculate the threshold for the last read access
    // (since the read time stamps have only day resolution
    // on FAT, be gratious when setting the limit)

    SYSTEMTIME nowSystemTime;
    GetSystemTime (&nowSystemTime);
    FILETIME nowFileTime = {0, 0};
    SystemTimeToFileTime (&nowSystemTime, &nowFileTime);

    __int64 now = *reinterpret_cast<__int64*>(&nowFileTime);
    __int64 ageLimit = now - 864000000000i64 * (CSettings::GetCacheDropAge() + 1);
    DWORD maxSize = CSettings::GetCacheDropMaxSize() * 1024;

    // find all files in the cache and fill the above sets

    static const CString lockExtension = L".lock";
    CString datFile
        = repositoryInfo->GetFileName().Mid (cacheFolderPath.GetLength());

    WIN32_FIND_DATA dirEntry;
    CAutoFindFile handle = FindFirstFile (cacheFolderPath + L"*.*", &dirEntry);
    if (handle)
    {
        do
        {
            CString fileName = dirEntry.cFileName;
            fileName.MakeLower();

            allFiles.insert (fileName);

            // process only caches that are not locked.
            // The repository list itself is not a repository cache

            if ((fileName.GetLength() > 2) && (fileName != datFile))
            {
                __int64 readTime
                    = *reinterpret_cast<__int64*>(&dirEntry.ftLastAccessTime);

                if (fileName.Right (5) == lockExtension)
                {
                    // remove obsolete locks
                    // (in case they had been left behind)

                    if (readTime < ageLimit)
                        oldLocks.insert (fileName);

                    // the cache is locked
                    // (so, it may take 2 runs to remove lock *and* cache)

                    lockedCaches.insert (fileName.Left (fileName.GetLength() - 5));
                }
                else
                {
                    // is this a cache we should try to remove?

                    if ((readTime < ageLimit) && (dirEntry.nFileSizeLow < maxSize))
                        smallUnusedCaches.insert (fileName);
                }
            }
        }
        while (FindNextFile (handle, &dirEntry));
    }

    // try to remove old locks
    // (will fail silently for active / open locks)

    typedef std::set<CString>::const_iterator CIT;
    for (CIT iter = oldLocks.begin(), end = oldLocks.end(); iter != end; ++iter)
        DeleteFile (cacheFolderPath + *iter);

    // remove small, unused caches that are not locked
    // (will fail silently for caches that are being read / written right now)

    std::set<CString> deletedCaches;
    for ( CIT iter = smallUnusedCaches.begin(), end = smallUnusedCaches.end()
        ; iter != end
        ; ++iter)
    {
        if (lockedCaches.find (*iter) == lockedCaches.end())
        {
            DeleteFile (cacheFolderPath + *iter);
            deletedCaches.insert (*iter);
        }
    }

    // update cache info list and also remove entries
    // that don't have a cache file anymore

    for (size_t i = repositoryInfo->data.size(); i > 0; --i)
    {
        // DropEntry may remove more than one entry per call

        if (i > repositoryInfo->data.size())
            continue;

        // entry still exists. Check whether we have to remove it

        const CRepositoryInfo::SPerRepositoryInfo* info
            = repositoryInfo->data[i-1];

        if (   (deletedCaches.find (info->fileName) != deletedCaches.end())
            || (allFiles.find (info->fileName) == allFiles.end()))
        {
            repositoryInfo->DropEntry (info->uuid, info->root);
        }
    }
}

// construction / destruction
// (Flush() on destruction)

CLogCachePool::CLogCachePool (SVN& svn, const CString& cacheFolderPath)
    : cacheFolderPath (cacheFolderPath)
    , repositoryInfo (new CRepositoryInfo (svn, cacheFolderPath))
{
    if (++instanceCount == 1)
        AutoRemoveUnused();
}

CLogCachePool::~CLogCachePool()
{
    delete repositoryInfo;
    repositoryInfo = NULL;

    if (--instanceCount == 0)
        Clear();
}

// auto-create and return cache for given repository

CCachedLogInfo* CLogCachePool::GetCache (const CString& uuid, const CString& root)
{
    // get / auto-create suitable entry in cache list

    CRepositoryInfo::SPerRepositoryInfo* info
        = repositoryInfo->data.AutoInsert (uuid, root);

    // cache hit?

    TCaches::const_iterator iter = caches.find (info->fileName);
    if (iter != caches.end())
        return iter->second;

    // load / create

    std::wstring fileName = (LPCTSTR)(cacheFolderPath + info->fileName);
    std::unique_ptr<CCachedLogInfo> cache (new CCachedLogInfo (fileName));

    cache->Load (CSettings::GetMaxFailuresUntilDrop());

    caches[info->fileName] = cache.get();

    // ready

    return cache.release();
}

// cached repository info

CRepositoryInfo& CLogCachePool::GetRepositoryInfo()
{
    return *repositoryInfo;
}

// return the size of the repository cache file
// (returns 0 for new files)

size_t CLogCachePool::FileSize (const CString& uuid, const CString& root)
{
    // return 0 for unknown caches

    CRepositoryInfo::SPerRepositoryInfo* info
        = repositoryInfo->data.Lookup (uuid, root);
    if (info == NULL)
        return 0;

    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesEx ( cacheFolderPath + info->fileName
                            , GetFileExInfoStandard
                            , &fileInfo) == FALSE)
        return 0;

    // return result

    return fileInfo.nFileSizeLow;
}

// delete a cache along with all file(s)

void CLogCachePool::DropCache (const CString& uuid, const CString& root)
{
    // delete cache object

    CRepositoryInfo::SPerRepositoryInfo* info
        = repositoryInfo->data.Lookup (uuid, root);
    if (info == NULL)
        return;

    TCaches::iterator iter = caches.find (info->fileName);
    if (iter != caches.end())
    {
        delete iter->second;
        caches.erase (iter);
    }

    // delete cache file

    std::wstring fileName = (LPCTSTR)(cacheFolderPath + info->fileName);
    if (FileExists (fileName))
        DeleteFile (fileName.c_str());

    if (FileExists (fileName + L".lock"))
        DeleteFile ((fileName + L".lock").c_str());

    // remove from cache info list

    repositoryInfo->DropEntry (uuid, root);
}

// other data access
// return as URL -> UUID map

std::multimap<CString, CString> CLogCachePool::GetRepositoryURLs() const
{
    std::multimap<CString, CString> result;

    for (size_t i = 0, count = repositoryInfo->data.size(); i != count; ++i)
    {
        const CRepositoryInfo::SPerRepositoryInfo* info
            = repositoryInfo->data[i];

        result.insert (std::make_pair (info->root, info->uuid));
    }

    return result;
}

// cache management

// write all changes to disk

void CLogCachePool::Flush()
{
    for ( TCaches::iterator iter = caches.begin(), end = caches.end()
        ; iter != end
        ; ++iter)
    {
        if (iter->second->IsModified())
        {
            try
            {
                iter->second->Save();
            }
            catch (...)
            {
                // cache file could not be written
                // (cache may be lost but that's o.k.)
            }
        }
    }

    repositoryInfo->Flush();
}

// has log caching been enabled?

bool CLogCachePool::IsEnabled() const
{
    return CSettings::GetEnabled();
}

// end namespace LogCache

}
