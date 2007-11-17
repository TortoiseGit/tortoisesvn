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
// necessary includes
///////////////////////////////////////////////////////////////

#include "logCacheGlobals.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CTSVNPath;

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

/**
 * Cache for frequently needed repository properties such as root URL
 * and UUID. Also, cache the last known head revision.
 *
 * However, there is a minor difference in how the head revision is
 * treated: if the head rev. of some parent path is already known,
 * it will be returned for every sub-path where SVN may return a
 * smaller number. However, this higher HEAD will still be valid 
 * for the path (i.e. it didn't get deleted etc.). To determine the
 * head *change*, call the SVN method directly.
 *
 * Mimic some methods of the SVN class. Results will automatically
 * be put into this cache.
 * 
 * Store to disk as "Repositories.dat" in the log cache folder.
 */

class CRepositoryInfo
{
private:

	/**
	 * Contains all "header" data for a repository.
	 */

    struct SPerRepositoryInfo
    {
        /// the repository root URL

        CString root;

        /// repository URL

        CString uuid;

        /// path we used to ask SVN for the head revision

        CString headURL;

        /// the answer we got

        revision_t headRevision;

        /// when we asked the last time

        __time64_t headLookupTime;
    };

    /// cached repository properties

    typedef std::map<CString, SPerRepositoryInfo> TData;
    TData data;

    /// has the data been modified

    bool modified;

    /// where to store the cached data

    CString cacheFolder;

    /// construct the dump file name

    CString GetFileName() const;

    /// read the dump file

    void Load();

    /// find cache entry (or data::end())

    TData::iterator Lookup (const CTSVNPath& url);

public:

    /// construction / destruction: auto-load and save

    CRepositoryInfo(const CString& cacheFolderPath);
    ~CRepositoryInfo(void);

    /// look-up and ask SVN if the info is not in cache. 
    /// cache the result.

	CString GetRepositoryRoot (const CTSVNPath& url);
	CString GetRepositoryRootAndUUID (const CTSVNPath& url, CString& sUUID);

    revision_t GetHeadRevision (const CTSVNPath& url);

    /// make sure, we will ask the repository for the HEAD

    void ResetHeadRevision (const CTSVNPath& url);

    /// find the root folder to a given UUID (e.g. URL for given cache file).
    /// Returns an empty string, if no suitable entry has been found.

    CString GetRootFromUUID (const CString& sUUID) const;

	/// remove a specific entry

    void DropEntry (const CString& sUUID);

	/// write all changes to disk

	void Flush();

    /// clear cache

    void Clear();

	/// for statistics

	friend class CLogCacheStatistics;
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

