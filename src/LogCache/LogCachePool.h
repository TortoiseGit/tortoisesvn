// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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
class CLogCachePool
{
private:

    /// where the log cache files are stored

    CString cacheFolderPath;

    /// cached repository properties

    CRepositoryInfo* repositoryInfo;

    /// cache per repository (file name)

    typedef std::map<CString, CCachedLogInfo*> TCaches;
    static TCaches caches;
    static long instanceCount;

    /// utility

    static bool FileExists (const std::wstring& filePath);

    /// minimize memory usage

    void Clear();

    /// remove small, unused caches

    void AutoRemoveUnused();

public:

    /// construction / destruction
    /// (Flush() on destruction)

    CLogCachePool (SVN& svn, const CString& cacheFolderPath);
    virtual ~CLogCachePool(void);

    /// auto-create and return cache for given repository

    CCachedLogInfo* GetCache (const CString& uuid, const CString& root);

    /// cached repository info

    CRepositoryInfo& GetRepositoryInfo();

    /// return the size of the repository cache file
    /// (returns 0 for new files)

    size_t FileSize (const CString& uuid, const CString& root);

    /// delete a cache along with all file(s)

    void DropCache (const CString& uuid, const CString& root);

    /// other data access

    const CString& GetCacheFolderPath() const;

    /// return as URL -> UUID map

    std::multimap<CString, CString> GetRepositoryURLs() const;

    /// cache management

    /// write all changes to disk

    void Flush();

    /// has log caching been enabled?

    bool IsEnabled() const;
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

}

