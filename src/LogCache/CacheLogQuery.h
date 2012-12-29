// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009,2011-2012 - TortoiseSVN

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

#include "ILogQuery.h"
#include "ILogReceiver.h"

#include "Containers/RevisionInfoContainer.h"

#include "PathToStringMap.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class SVNInfoData;
class CTSVNPath;
class SVN;

namespace LogCache
{
    class ILogIterator;

    class CLogCachePool;
    class CRevisionIndex;
    class CRevisionInfoContainer;
    class CCachedLogInfo;
    class CRepositoryInfo;
}

using namespace LogCache;

/**
 * Implements the ILogQuery interface on the log cache. It requires another
 * query to fill the gaps in the cache it may encounter. Those gaps will be
 * filled on-the-fly.
 */
class CCacheLogQuery : public ILogQuery
{
private:

    /** utility class to group all options that controls
     * the behavior of the log command. It's used to pass
     * those options through a single variable.
     */

    class CLogOptions
    {
    private:

        /// history tracing options

        bool strictNodeHistory;

        /// log target options

        ILogReceiver* receiver;

        /// log content options

        bool includeChanges;
        bool includeMerges;
        bool includeStandardRevProps;
        bool includeUserRevProps;
        TRevPropNames userRevProps;

        // derived properties

        char presenceMask;
        bool revsOnly;

    public:

        /// construction

        CLogOptions ( bool strictNodeHistory = true
                    , ILogReceiver* receiver = NULL
                    , bool includeChanges = true
                    , bool includeMerges = false
                    , bool includeStandardRevProps = true
                    , bool includeUserRevProps = false
                    , const TRevPropNames& userRevProps = TRevPropNames());
        CLogOptions ( const CLogOptions& rhs
                    , ILogReceiver* receiver);

        /// data access

        bool GetStrictNodeHistory() const;

        ILogReceiver* GetReceiver() const;

        bool GetIncludeChanges() const;
        bool GetIncludeMerges() const;
        bool GetIncludeStandardRevProps() const;
        bool GetIncludeUserRevProps() const;
        const TRevPropNames& GetUserRevProps() const;

        char GetPresenceMask() const;
        bool GetRevsOnly() const;

        /// utility methods

        ILogIterator* CreateIterator ( CCachedLogInfo* cache
                                     , revision_t startRevision
                                     , const CDictionaryBasedTempPath& startPath) const;
    };

    /**
     * Simple utility class that checks the revision pointed to
     * by the iterator for available cached log data.
     */

    class CDataAvailable
    {
    private:

        /// required to map revision -> log entry

        const CRevisionIndex& revisions;
        const CRevisionInfoContainer& logInfo;

        /// presence flags that must be set for the respective revision

        char requiredMask;

    public:

        /// initialize & pre-compile

        CDataAvailable ( CCachedLogInfo* cache
                       , const CLogOptions& options);

        /// all required log info has been cached?

        bool operator() (ILogIterator* iterator) const;
        bool operator() (revision_t revision) const;
    };

    /** utility class that implements the ILogReceiver interface
     */

    class CLogFiller : private ILogReceiver
    {
    private:

        /// cache to use & update
        /// collect *changed* data in updateData because updates are expensive
        /// (merge them into cache when necessary)
        CCachedLogInfo* cache;
        CCachedLogInfo* updateData;
        CRepositoryInfo* repositoryInfoCache;
        CStringA URL;
        CString uuid;

        /// connection to the SVN repository
        ILogQuery* svnQuery;

        /// path to log for and the begin of the current gap
        /// in the log for that path
        std::unique_ptr<CDictionaryBasedTempPath> currentPath;
        revision_t firstNARevision;

        /// the last revision we forwarded to the receiver.
        /// Will also be set, if receiver is NULL.
        revision_t oldestReported;

        /// log options (including receiver)
        CLogOptions options;

        /// don't ask for going off-line, if this is set
        bool receiverError;

        /// number of revisions received (i.e.: has limit been reached?)
        int receiveCount;

        /// utility method
        void MergeFromUpdateCache();

        /// if there is a gap in the log, mark it
        void AutoAddSkipRange (revision_t revision);

        /// make sure, we can iterator over the given range for the given path
        void MakeRangeIterable ( const CDictionaryBasedPath& path
                               , revision_t startRevision
                               , revision_t count);

        /// cache data
        void WriteToCache ( TChangedPaths* changes
                          , revision_t revision
                          , const StandardRevProps* stdRevProps
                          , UserRevPropArray* userRevProps);

        /// implement ILogReceiver
        virtual void ReceiveLog ( TChangedPaths* changes
                                , svn_revnum_t rev
                                , const StandardRevProps* stdRevProps
                                , UserRevPropArray* userRevProps
                                , const MergeInfo* mergeInfo) override;

    public:

        /// default construction / destruction
        explicit CLogFiller (CRepositoryInfo* repositoryInfoCache);
        ~CLogFiller();

        /// actually call SVN
        /// return the last revision sent to the receiver
        revision_t FillLog ( CCachedLogInfo* cache
                           , const CStringA& URL
                           , CString uuid
                           , ILogQuery* svnQuery
                           , revision_t startRevision
                           , revision_t endRevision
                           , const CDictionaryBasedTempPath& startPath
                           , int limit
                           , const CLogOptions& options);
    };

    /** utility class that receives (only) the revisions in the log
     * including merged revisions. The parent CCacheLogQuery is used
     * to add the other info from cache (auto-fill the cache if data
     * is missing) and send it to the receiver.
     */

    class CMergeLogger : public ILogReceiver
    {
    private:

        /// we will use the parent to actually update the cache
        /// and to send data to the receiver
        CCacheLogQuery* parentQuery;

        /// log options (including receiver)
        CLogOptions options;

        /// implement ILogReceiver
        virtual void ReceiveLog ( TChangedPaths* changes
                                , svn_revnum_t rev
                                , const StandardRevProps* stdRevProps
                                , UserRevPropArray* userRevProps
                                , const MergeInfo* mergeInfo) override;

    public:

        /// construction

        CMergeLogger ( CCacheLogQuery* parentQuery
                     , const CLogOptions& options);
    };

    /// we get our cache from here
    CLogCachePool* caches;
    CRepositoryInfo* repositoryInfoCache;

    /// cache to use & update
    CCachedLogInfo* cache;
    CStringA URL;
    CString root;
    CString uuid;

    /// used, if caches is NULL
    CCachedLogInfo* tempCache;

    /// connection to the SVN repository (may be NULL)
    ILogQuery* svnQuery;

    /// efficient map cached string / path -> CString
    typedef quick_hash_map<index_t, std::string> TID2String;

    TID2String authorToStringMap;
    CPathToStringMap pathToStringMap;

    /// used for temporary string objects to prevent frequent allocations
    std::string messageScratch;

    /// Determine the revision range to pass to SVN.
    revision_t NextAvailableRevision ( const CDictionaryBasedTempPath& path
                                     , revision_t firstMissingRevision
                                     , revision_t endRevision
                                     , const CDataAvailable& dataAvailable) const;

    /// Determine an end-revision that would fill many cache gaps efficiently
    revision_t FindOldestGap ( const CDictionaryBasedTempPath& path
                             , revision_t startRevision
                             , revision_t endRevision
                             , const CDataAvailable& dataAvailable) const;

    /// ask SVN to fill the log -- at least a bit
    /// Possibly, it will stop long before endRevision and limit!
    revision_t FillLog ( revision_t startRevision
                       , revision_t endRevision
                       , const CDictionaryBasedTempPath& startPath
                       , int limit
                       , const CLogOptions& options
                       , const CDataAvailable& dataAvailable);

    /// fill the receiver's user rev-prop list buffer
    void GetUserRevProps ( UserRevPropArray& result
                         , CRevisionInfoContainer::CUserRevPropsIterator first
                         , const CRevisionInfoContainer::CUserRevPropsIterator& last
                         , const TRevPropNames& userRevProps);

    /// relay an existing cache entry to the receiver
    void SendToReceiver ( revision_t revision
                        , const CLogOptions& options
                        , const MergeInfo* mergeInfo);

    /// clear string translating caches
    void ResetObjectTranslations();

    /// crawl the history and forward it to the receiver
    void InternalLog ( revision_t startRevision
                     , revision_t endRevision
                     , const CDictionaryBasedTempPath& startPath
                     , int limit
                     , const CLogOptions& options);

    void InternalLogWithMerge ( revision_t startRevision
                              , revision_t endRevision
                              , const CDictionaryBasedTempPath& startPath
                              , int limit
                              , const CLogOptions& options);

    /// follow copy history until the startRevision is reached
    CDictionaryBasedTempPath TranslatePegRevisionPath
        ( revision_t pegRevision
        , revision_t startRevision
        , const CDictionaryBasedTempPath& startPath);

    /// extract the repository-relative path of the URL / file name
    /// and open the cache
    CDictionaryBasedTempPath GetRelativeRepositoryPath
        ( const CTSVNPath& info);

    /// utility method: we throw that error in several places
    void ThrowBadRevision() const;

    /// decode special revisions:
    /// base / head must be initialized with NO_REVISION
    /// and will be used to cache these values.
    revision_t DecodeRevision ( const CTSVNPath& path
                              , const CTSVNPath& url
                              , const SVNRev& revision
                              , const SVNRev& peg) const;

    /// get the (exactly) one path from targets
    /// throw an exception, if there are none or more than one
    CTSVNPath GetPath (const CTSVNPathList& targets) const;

public:

    // construction / destruction

    CCacheLogQuery (CLogCachePool* caches, ILogQuery* svnQuery);
    CCacheLogQuery (SVN& svn, ILogQuery* svnQuery);
    virtual ~CCacheLogQuery(void);

    /// query a section from log for multiple paths
    /// (special revisions, like "HEAD", are supported)
    virtual void Log ( const CTSVNPathList& targets
                     , const SVNRev& peg_revision
                     , const SVNRev& start
                     , const SVNRev& end
                     , int limit
                     , bool strictNodeHistory
                     , ILogReceiver* receiver
                     , bool includeChanges
                     , bool includeMerges
                     , bool includeStandardRevProps
                     , bool includeUserRevProps
                     , const TRevPropNames& userRevProps) override;

    /// relay the content of a single revision to the receiver
    /// (if the latter is not NULL)
    void LogRevision ( revision_t revision
                     , const CLogOptions& options
                     , const MergeInfo* mergeInfo);

    /// access to the cache
    /// (only valid after calling Log())
    CCachedLogInfo* GetCache() const;

    /// get the repository root URL
    /// (only valid after calling Log())
    const CStringA& GetRootURL() const;

    /// could we get at least some data?
    /// (such as an empty log but still UUID and HEAD info)
    bool GotAnyData() const;

    /// for tempCaches: write content to "real" cache files
    /// (no-op if this does not use a temp. cache)
    void UpdateCache (CCacheLogQuery* targetQuery) const;

    /// utility function:
    /// fill the receiver's change list buffer
    static void GetChanges ( TChangedPaths& result
                           , CPathToStringMap& pathToStringMap
                           , CRevisionInfoContainer::CChangesIterator first
                           , const CRevisionInfoContainer::CChangesIterator& last);

};

/// Log options inline implementations for data access

inline bool CCacheLogQuery::CLogOptions::GetStrictNodeHistory() const
{
    return strictNodeHistory;
}

inline ILogReceiver* CCacheLogQuery::CLogOptions::GetReceiver() const
{
    return receiver;
}

inline bool CCacheLogQuery::CLogOptions::GetIncludeChanges() const
{
    return includeChanges;
}

inline bool CCacheLogQuery::CLogOptions::GetIncludeMerges() const
{
    return includeMerges;
}

inline bool CCacheLogQuery::CLogOptions::GetIncludeStandardRevProps() const
{
    return includeStandardRevProps;
}

inline bool CCacheLogQuery::CLogOptions::GetIncludeUserRevProps() const
{
    return includeUserRevProps;
}

inline const TRevPropNames& CCacheLogQuery::CLogOptions::GetUserRevProps() const
{
    return userRevProps;
}

inline char CCacheLogQuery::CLogOptions::GetPresenceMask() const
{
    return presenceMask;
}

inline bool CCacheLogQuery::CLogOptions::GetRevsOnly() const
{
    return revsOnly;
}
