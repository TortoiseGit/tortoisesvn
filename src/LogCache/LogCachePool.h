// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007, 2013-2014, 2021. 2023 - TortoiseSVN

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
#pragma once

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class SVN;

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{
///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CCachedLogInfo;
class CRepositoryInfo;

/**
 * Central storage for all repository specific log caches currently in use. New
 * caches will be created automatically.
 *
 * Currently, there is no method to delete unused cache files or to selectively
 * remove cache info from RAM.
 *
 * The filenames of the log caches are the repository UUIDs.
 */
// cppcheck-suppress noCopyConstructor
class CLogCachePool
{
private:
    /// where the log cache files are stored

    CString          cacheFolderPath;

    /// cached repository properties

    CRepositoryInfo* repositoryInfo;

    /// cache per repository (file name)

    using TCaches = std::map<CString, CCachedLogInfo*>;
    static TCaches caches;
    static long    instanceCount;

    /// utility

    static bool    FileExists(const std::wstring& filePath);

    /// minimize memory usage

    void           Clear() const;

    /// remove small, unused caches

    void           AutoRemoveUnused() const;

    CLogCachePool() = delete;

public:
    /// construction / destruction
    /// (Flush() on destruction)

    CLogCachePool(SVN& svn, const CString& cacheFolderPath);
    virtual ~CLogCachePool();

    /// auto-create and return cache for given repository

    CCachedLogInfo*                 GetCache(const CString& uuid, const CString& root) const;

    /// cached repository info

    CRepositoryInfo&                GetRepositoryInfo() const;

    /// return the size of the repository cache file
    /// (returns 0 for new files)

    size_t                          FileSize(const CString& uuid, const CString& root) const;

    /// delete a cache along with all file(s)

    void                            DropCache(const CString& uuid, const CString& root) const;

    /// other data access

    const CString&                  GetCacheFolderPath() const;

    /// return as URL -> UUID map

    std::multimap<CString, CString> GetRepositoryURLs() const;

    /// cache management

    /// write all changes to disk

    void                            Flush() const;

    /// release all locks
    static void                     ReleaseLocks();

    /// has log caching been enabled?

    static bool                     IsEnabled();
};

///////////////////////////////////////////////////////////////
// other data access
///////////////////////////////////////////////////////////////

inline const CString& CLogCachePool::GetCacheFolderPath() const
{
    return cacheFolderPath;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

} // namespace LogCache
