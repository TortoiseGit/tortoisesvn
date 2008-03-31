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

struct svn_error_t;

class CTSVNPath;
class SVN;

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
public:

    /**
     * Per-repository server connectivity state: Default is 
     * @a online, user can switch to one of the other states
     * upon connection failure. Refreshing a view will always 
     * reset to default.
     */

    enum ConnectionState 
    {
        /// call the server whenever necessary (default)

        online = 0,

        /// don't call the server, except when HEAD info needs to be refreshed

        tempOffline = 1,

        /// don't contact the server for any reason whatsoever

        offline = 2
    };

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

        /// flag to control the repository access

        ConnectionState connectionState;
    };

    /**
     * File version identifiers.
     **/

    enum
    {
        VERSION = 20071023,
        MIN_COMPATIBLE_VERSION = VERSION
    };

    /// cached repository properties
	/// map URL -> PerRepoInfo

    typedef std::map<CString, SPerRepositoryInfo> TData;
    TData data;

    /// has the data been modified

    bool modified;

    /// where to store the cached data

    CString cacheFolder;

    /// use this instance for all SVN access

    SVN& svn;

    /// read the dump file

    void Load();

    /// find cache entry (or data::end())

    TData::iterator Lookup (const CString& url);
    TData::iterator Lookup (const CTSVNPath& url);

    /// does the user want to be this repository off-line?

    bool IsOffline (SPerRepositoryInfo& info);

    /// try to get the HEAD revision from the log cache

    void SetHeadFromCache (SPerRepositoryInfo& iter);

public:

    /// construction / destruction: auto-load and save

    CRepositoryInfo (SVN& svn, const CString& cacheFolderPath);
    ~CRepositoryInfo();

    /// look-up and ask SVN if the info is not in cache. 
    /// cache the result.

	CString GetRepositoryRoot (const CTSVNPath& url);
	CString GetRepositoryUUID (const CTSVNPath& url);
	CString GetRepositoryRootAndUUID (const CTSVNPath& url, CString& sUUID);

    revision_t GetHeadRevision (const CTSVNPath& url);

    /// make sure, we will ask the repository for the HEAD

    void ResetHeadRevision (const CTSVNPath& url);

    /// find the root folder to a given UUID (e.g. URL for given cache file).
    /// Returns an empty string, if no suitable entry has been found.

    CString GetRootFromUUID (const CString& sUUID) const;

    /// do multiple URLs use this UUID?

    bool HasMultipleURLs (const CString& uuid) const;

	/// is the repository offline? 
	/// Don't modify the state if autoSet is false.

    bool IsOffline (const CString& url, bool autoSet);

    /// get the connection state (uninterpreted)

    ConnectionState GetConnectionState (const CString& uuid);

    /// remove a specific entry

    void DropEntry (const CString& sUUID);

	/// write all changes to disk

	void Flush();

    /// clear cache

    void Clear();

	/// get the owning SVN instance

	SVN& GetSVN() const;

    /// access to the result of the last SVN operation

    svn_error_t* GetLastError() const;

    /// construct the dump file name

    CString GetFileName() const;

    /// is this only temporary data?

    bool IsPermanent() const;

    /// for statistics

	friend class CLogCacheStatistics;
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

