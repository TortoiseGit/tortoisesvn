// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

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

#include "./Containers/LogCacheGlobals.h"
#include "./ConnectionState.h"

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
private:

    /**
     * Contains all "header" data for a repository.
     */

    struct SPerRepositoryInfo
    {
        /// the repository root URL

        CString root;

        /// Universal Unique Identifier for a cached repository

        CString uuid;

        /// cached repository file

        CString fileName;

        /// path we used to ask SVN for the head revision

        CString headURL;

        /// the answer we got

        revision_t headRevision;

        /// when we asked the last time

        __time64_t headLookupTime;

        /// flag to control the repository access

        ConnectionState connectionState;
    };

    class CData
    {
    private:
        
        /// per-repository properties

        typedef std::vector<SPerRepositoryInfo*> TData;
        TData data;

        /// several indices for faster access

        typedef std::multimap<CString, SPerRepositoryInfo*> TPartialIndex;
        TPartialIndex uuidIndex;
        TPartialIndex urlIndex;

        typedef std::map<std::pair<CString, CString>, SPerRepositoryInfo*> TFullIndex;
        TFullIndex fullIndex;

        /**
         * File version identifiers.
         **/

        enum
        {
            VERSION = 20081023,
            MIN_FILENAME_VERSION = VERSION,
            MIN_COMPATIBLE_VERSION = 20071023
        };

        // a lookup utility that scans an index range

        CString FindRoot 
            ( TPartialIndex::const_iterator begin
            , TPartialIndex::const_iterator end
            , const CString& url) const;

    public:

        /// construction / destruction

        CData();
        ~CData();

        /// lookup (using current rules setting);
        /// pass empty strings for unknown values.

        CString FindRoot (const CString& uuid, const CString& url) const;
        SPerRepositoryInfo* Lookup (const CString& uuid, const CString& root) const;

        /// modification

        void Add (const SPerRepositoryInfo& info);
        void Remove (SPerRepositoryInfo* info);

        /// read / write file

        void Load (const CString& fileName);
        void Save (const CString& fileName) const;
        void Clear();

        /// status info

        bool empty() const;

        /// data access

        const SPerRepositoryInfo* const * begin() const;
        const SPerRepositoryInfo* const * end() const;
    };

    /// cached repository properties

    static CData data;

    /// has the data been modified

    bool modified;

    /// where to store the cached data

    CString cacheFolder;

    /// use this instance for all SVN access

    SVN& svn;

    /// read the dump file

    void Load();

    /// does the user want to be this repository off-line?

    bool IsOffline (SPerRepositoryInfo* info);

    /// try to get the HEAD revision from the log cache

    void SetHeadFromCache (SPerRepositoryInfo* iter);

public:

    /// construction / destruction: auto-load and save

    CRepositoryInfo (SVN& svn, const CString& cacheFolderPath);
    ~CRepositoryInfo();

    /// look-up and ask SVN if the info is not in cache. 
    /// cache the result.

	CString GetRepositoryRoot (const CTSVNPath& url);
	CString GetRepositoryUUID (const CTSVNPath& url);
	CString GetRepositoryRootAndUUID (const CTSVNPath& url, CString& uuid);

    revision_t GetHeadRevision (CString uuid, const CTSVNPath& url);

    /// make sure, we will ask the repository for the HEAD

    void ResetHeadRevision (const CString& uuid, const CString& root);

    /// is the repository offline? 
	/// Don't modify the state if autoSet is false.

    bool IsOffline (const CString& uuid, const CString& url, bool autoSet);

    /// get the connection state (uninterpreted)

    ConnectionState GetConnectionState (const CString& uuid, const CString& url);

    /// remove a specific entry.
    /// Parameters must be copied because they may stem from the
    /// info object being deleted.

    void DropEntry (CString uuid, CString url);

    /// write all changes to disk

    void Flush();

    /// clear cache

    void Clear();

    /// get the owning SVN instance

    SVN& GetSVN() const;

    /// access to the result of the last SVN operation

    const svn_error_t* GetLastError() const;

    /// construct the dump file name

    CString GetFileName() const;

    /// for statistics

    friend class CLogCacheStatistics;
    friend class CLogCachePool;
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

