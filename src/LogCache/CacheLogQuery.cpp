// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

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

#include "CacheLogQuery.h"

#include "Containers/CachedLogInfo.h"
#include "Access/CopyFollowingLogIterator.h"
#include "Access/StrictLogIterator.h"
#include "LogCachePool.h"
#include "RepositoryInfo.h"

#include "svn_client.h"
#include "svn_opt.h"

#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "TSVNPath.h"
#include "SVN.h"
#include "SVNInfo.h"
#include "SVNError.h"

///////////////////////////////////////////////////////////////
// CLogOptions
///////////////////////////////////////////////////////////////
// construction
///////////////////////////////////////////////////////////////

CCacheLogQuery::CLogOptions::CLogOptions ( bool strictNodeHistory
                                         , ILogReceiver* receiver
                                         , bool includeChanges
                                         , bool includeMerges
                                         , bool includeStandardRevProps
                                         , bool includeUserRevProps
                                         , const TRevPropNames& userRevProps)
    : strictNodeHistory (strictNodeHistory)
    , receiver (receiver)
    , includeChanges (includeChanges)
    , includeMerges (includeMerges)
    , includeStandardRevProps (includeStandardRevProps)
    , includeUserRevProps (includeUserRevProps)
    , userRevProps (userRevProps)
    , presenceMask (CRevisionInfoContainer::HAS_CHANGEDPATHS)
    , revsOnly (   !includeChanges
                && !includeStandardRevProps
                && !includeUserRevProps)
{
    if (includeStandardRevProps)
        presenceMask = CRevisionInfoContainer::HAS_STANDARD_REVPROPS;
    if (includeUserRevProps)
        presenceMask |= CRevisionInfoContainer::HAS_USERREVPROPS;
}

CCacheLogQuery::CLogOptions::CLogOptions ( const CLogOptions& rhs
                                         , ILogReceiver* receiver)
    : strictNodeHistory (rhs.strictNodeHistory)
    , receiver (receiver)
    , includeChanges (rhs.includeChanges)
    , includeMerges (rhs.includeMerges)
    , includeStandardRevProps (rhs.includeStandardRevProps)
    , includeUserRevProps (rhs.includeUserRevProps)
    , userRevProps (rhs.userRevProps)
    , presenceMask (rhs.presenceMask)
    , revsOnly (rhs.revsOnly)
{
}

///////////////////////////////////////////////////////////////
// utility methods
///////////////////////////////////////////////////////////////

ILogIterator* CCacheLogQuery::CLogOptions::CreateIterator
    ( CCachedLogInfo* cache
    , revision_t startRevision
    , const CDictionaryBasedTempPath& startPath) const
{
    return strictNodeHistory
        ? static_cast<ILogIterator*>
            (new CStrictLogIterator ( cache
                                    , startRevision
                                    , startPath))
        : static_cast<ILogIterator*>
            (new CCopyFollowingLogIterator ( cache
                                           , startRevision
                                           , startPath));
}

///////////////////////////////////////////////////////////////
// CDataAvailable
///////////////////////////////////////////////////////////////
// initialize & pre-compile
///////////////////////////////////////////////////////////////

CCacheLogQuery::CDataAvailable::CDataAvailable ( CCachedLogInfo* cache
                                               , const CLogOptions& options)
    : revisions (cache->GetRevisions())
    , logInfo (cache->GetLogInfo())

    // for iteration, we always need the changed path list!

    , requiredMask (  options.GetPresenceMask()
                    | CRevisionInfoContainer::HAS_CHANGEDPATHS)
{
}

///////////////////////////////////////////////////////////////
// all required log info has been cached?
///////////////////////////////////////////////////////////////

bool CCacheLogQuery::CDataAvailable::operator() (ILogIterator* iterator) const
{
    return operator()(iterator->GetRevision());
};

bool CCacheLogQuery::CDataAvailable::operator() (revision_t revision) const
{
    index_t logIndex = revisions [revision];
    if (logIndex == NO_INDEX)
        return false;

    return (logInfo.GetPresenceFlags (logIndex) & requiredMask)
        == requiredMask;
};


///////////////////////////////////////////////////////////////
// CLogFiller
///////////////////////////////////////////////////////////////
// utility method
///////////////////////////////////////////////////////////////

void CCacheLogQuery::CLogFiller::MergeFromUpdateCache()
{
    if (!updateData->IsEmpty())
    {
        cache->Update (*updateData);
        updateData->Clear();
    }
}

///////////////////////////////////////////////////////////////
// if there is a gap in the log, mark it
///////////////////////////////////////////////////////////////

void CCacheLogQuery::CLogFiller::AutoAddSkipRange (revision_t revision)
{
    if ((firstNARevision > revision) && (currentPath.get() != NULL))
    {
        // due to only the parent path being renamed, the currentPath
        // may not have shown up in the log -> don't mark the range
        // as N/A for some *parent*.

        if (currentPath->IsFullyCachedPath())
        {
            cache->AddSkipRange ( currentPath->GetBasePath()
                                , revision+1
                                , firstNARevision - revision);
        }
        else
        {
            // we must fill this range! Otherwise, we will stumble
            // over this gap and will try to fetch the data again
            // to no avail ... causing an endless loop.

            MakeRangeIterable ( currentPath->GetBasePath()
                              , revision+1
                              , firstNARevision - revision);
        }
    }
}

///////////////////////////////////////////////////////////////
// make sure, we can iterate over the given range for the given path
///////////////////////////////////////////////////////////////

void CCacheLogQuery::CLogFiller::MakeRangeIterable ( const CDictionaryBasedPath& path
                                                   , revision_t startRevision
                                                   , revision_t count)
{
    // update the cache before parsing its content

    MergeFromUpdateCache();

    // trim the range to cover only missing data
    // (we may already have enough information
    // to iterate over the whole range -> don't
    // ask the server in that case)

    CStrictLogIterator iterator ( cache
                                , startRevision + count-1
                                , CDictionaryBasedTempPath (path));

    iterator.Retry();
    while (   !iterator.EndOfPath()
           && !iterator.DataIsMissing()
           && (iterator.GetRevision() >= startRevision))
    {
        iterator.Advance();
    }

    if (iterator.EndOfPath() || !iterator.DataIsMissing())
        return;

    // o.k., some data is missing. Ask for it.

    CLogFiller (repositoryInfoCache).FillLog ( cache
                                             , URL
                                             , uuid
                                             , svnQuery
                                             , iterator.GetRevision()
                                             , startRevision
                                             , CDictionaryBasedTempPath (path)
                                             , 0
                                             , CLogOptions());
}

// cache data

void CCacheLogQuery::CLogFiller::WriteToCache
    ( TChangedPaths* changes
    , revision_t revision
    , const StandardRevProps* stdRevProps
    , UserRevPropArray* userRevProps)
{
    // If it is not yet in cache, add it to the cache directly.
    // Otherwise it is a modification of an existing revision
    // -> collect them and update cache later in one go
    //    for maximum performance.

    CCachedLogInfo* targetCache = cache->GetRevisions()[revision] == NO_INDEX
                                ? cache
                                : updateData;

    // create the revision entry

    std::string author;
    std::string message;
    __time64_t timeStamp = 0;

    if (stdRevProps)
    {
        author = (const char*)CUnicodeUtils::GetUTF8 (stdRevProps->GetAuthor());
        message = (const char*)CUnicodeUtils::GetUTF8 (stdRevProps->GetMessage());
        timeStamp = stdRevProps->GetTimeStamp();
    }

    char presenceMask = 0;
    if (stdRevProps != NULL)
        presenceMask = CRevisionInfoContainer::HAS_STANDARD_REVPROPS;
    if (changes != NULL)
        presenceMask |= CRevisionInfoContainer::HAS_CHANGEDPATHS;
    if (userRevProps != NULL)
        presenceMask |= CRevisionInfoContainer::HAS_USERREVPROPS;

    targetCache->Insert ( revision
                        , author
                        , message
                        , timeStamp
                        , presenceMask);

    // add all changes

    if (changes != NULL)
    {
        for (size_t i = 0, count = changes->size(); i < count; ++i)
        {
            const SChangedPath& change = (*changes)[i];

            CRevisionInfoContainer::TChangeAction action
                = (CRevisionInfoContainer::TChangeAction)(change.action * 4);
            std::string path
                = (const char*)CUnicodeUtils::GetUTF8 (change.path);
            std::string copyFromPath
                = (const char*)CUnicodeUtils::GetUTF8 (change.copyFromPath);
            revision_t copyFromRevision
                = change.copyFromRev == 0
                ? NO_REVISION
                : static_cast<revision_t>(change.copyFromRev);

            targetCache->AddChange ( action
                                   , static_cast<node_kind_t>(change.nodeKind)
                                   , path
                                   , copyFromPath
                                   , copyFromRevision
                                   , change.text_modified
                                   , change.props_modified);
        }
    }

    // add user revprops

    if (userRevProps != NULL)
    {
        for (INT_PTR i = 0, count = userRevProps->GetCount(); i < count; ++i)
        {
            const UserRevProp& revprop = userRevProps->GetAt (i);

            std::string name
                = (const char*)CUnicodeUtils::GetUTF8 (revprop.GetName());
            std::string value
                = (const char*)CUnicodeUtils::GetUTF8 (revprop.GetValue());

            targetCache->AddRevProp (name, value);
        }
    }

    // update our path info

    if (!currentPath->IsFullyCachedPath())
    {
        if (!updateData->IsEmpty())
            MergeFromUpdateCache();

        currentPath->RepeatLookup();
    }

    // mark the gap and update the current path

    AutoAddSkipRange (revision);
}

// implement ILogReceiver

void CCacheLogQuery::CLogFiller::ReceiveLog
    ( TChangedPaths* changes
    , svn_revnum_t rev
    , const StandardRevProps* stdRevProps
    , UserRevPropArray* userRevProps
    , bool mergesFollow)
{
    try
    {
        // so far, it has not be our fault

        receiverError = false;

        // one entry more that we received

        ++receiveCount;

        // store the data we just received

        revision_t revision = static_cast<revision_t>(rev);
        WriteToCache (changes, rev, stdRevProps, userRevProps);

        // maybe the revision was known before but we had no changes info
        // -> we received it now
        // -> update the cache in that case
        // (if it was not known before, WriteToCache added it directly)

        index_t index = cache->GetRevisions()[revision];
        if ((  cache->GetLogInfo().GetPresenceFlags (index)
             & CRevisionInfoContainer::HAS_CHANGEDPATHS) == 0)
        {
            MergeFromUpdateCache();

            // repeated lookup should not be necessary as the updateCache
            // should contain only revisions that are already known by cache

            assert (index == cache->GetRevisions()[revision]);
        }

        // due to renames / copies, we may continue on a different path

        if (  (cache->GetLogInfo().GetSumChanges (index)
            & CRevisionInfoContainer::ACTION_ADDED) != 0)
        {
            // create the appropriate iterator to follow the potential path change

            std::auto_ptr<CLogIteratorBase> iterator
                (  options.GetStrictNodeHistory()
                 ? static_cast<CLogIteratorBase*>
                    (new CStrictLogIterator (cache, revision, *currentPath))
                 : static_cast<CLogIteratorBase*>
                    (new CCopyFollowingLogIterator (cache, revision, *currentPath)));

            // now, iterate as usual

            iterator->Advance();
            if (iterator->EndOfPath())
                currentPath.reset();
            else
                *currentPath = iterator->GetPath();
        }

        // the first revision we may not have information about is the one
        // immediately preceding the one we just received from the server

        firstNARevision = revision-1;

        // hand on to the original log receiver.
        // Even if there is no receiver, track the oldest revision
        // we received so we can update the skip ranges properly.

        oldestReported = min (oldestReported, revision);
        if (options.GetReceiver() != NULL)
        {
            if (options.GetRevsOnly())
            {
                options.GetReceiver()->ReceiveLog ( NULL
                                                  , rev
                                                  , NULL
                                                  , NULL
                                                  , mergesFollow);
            }
            else
            {
                options.GetReceiver()->ReceiveLog ( changes
                                                  , rev
                                                  , stdRevProps
                                                  , userRevProps
                                                  , mergesFollow);
            }
        }
    }
    catch (...)
    {
        // this was (probably) not a genuine SVN error

        receiverError = true;
        throw;
    }
}

// default construction / destruction

CCacheLogQuery::CLogFiller::CLogFiller (CRepositoryInfo* repositoryInfoCache)
    : cache (NULL)
    , updateData (new CCachedLogInfo())
    , repositoryInfoCache (repositoryInfoCache)
    , svnQuery (NULL)
    , firstNARevision ((revision_t)NO_REVISION)
    , oldestReported ((revision_t)NO_REVISION)
    , receiverError (false)
    , receiveCount (0)
{
}

CCacheLogQuery::CLogFiller::~CLogFiller()
{
    delete updateData;
}

// actually call SVN
// return the last revision sent to the receiver

revision_t
CCacheLogQuery::CLogFiller::FillLog ( CCachedLogInfo* cache
                                    , const CStringA& URL
                                    , CString uuid
                                    , ILogQuery* svnQuery
                                    , revision_t startRevision
                                    , revision_t endRevision
                                    , const CDictionaryBasedTempPath& startPath
                                    , int limit
                                    , const CLogOptions& options)
{
    this->cache = cache;
    this->URL = URL;
    this->uuid = uuid;
    this->svnQuery = svnQuery;
    this->options = options;
    this->receiveCount = 0;

    firstNARevision = startRevision;
    oldestReported = (revision_t)NO_REVISION;
    currentPath.reset (new CDictionaryBasedTempPath (startPath));

    // full path to be passed to SVN.
    // don't append a trailing "/", if the path is empty (i.e. root)

    CTSVNPath path;
    if (startPath.IsRoot())
        path.SetFromSVN (URL);
    else
        path.SetFromSVN (URL + startPath.GetPath().c_str());

    CString root = CUnicodeUtils::GetUnicode (URL);

    try
    {
        svnQuery->Log ( CTSVNPathList (path)
                      , static_cast<long>(startRevision)
                      , static_cast<long>(startRevision)
                      , static_cast<long>(endRevision)
                      , limit
                      , options.GetStrictNodeHistory()
                      , this
                      , true
                      , false
                      , options.GetIncludeStandardRevProps()
                      , options.GetIncludeUserRevProps()
                      , TRevPropNames());
    }
    catch (SVNError& e)
    {
        // if the problem was caused by SVN and the user wants
        // to go off-line, swallow the error

        if (   receiverError
            || e.GetCode() == SVN_ERR_CANCELLED
            || e.GetCode() == SVN_ERR_FS_NOT_FOUND  // deleted paths etc.
            || !repositoryInfoCache->IsOffline (uuid, root, true))
        {
            // we want to cache whatever data we could receive so far ..

            MergeFromUpdateCache();

            // cancel SVN op

            throw;
        }
    }

    // update the cache with the data we may have received

    MergeFromUpdateCache();

    // update skip ranges etc. if we are still connected

    if (!repositoryInfoCache->IsOffline (uuid, root, false))
    {
        // do we miss some data at the end of the log?
        // (no-op, if end-of-log was reached;
        //  only valid for a bounded log, i.e. limit != 0)

        // if we haven't received *any* data, there is no log info
        // for this path even if we haven't been following renames
        // (we will not get here in case of an error or user cancel)

        bool limitReached = (limit > 0) && (receiveCount >= limit);
        if ((receiveCount == 0) || !limitReached)
        {
            AutoAddSkipRange (max (endRevision, (revision_t)1)-1);
        }
    }

    return min (oldestReported, firstNARevision+1);
}

///////////////////////////////////////////////////////////////
// CMergeLogger
///////////////////////////////////////////////////////////////
// implement ILogReceiver
///////////////////////////////////////////////////////////////

void CCacheLogQuery::CMergeLogger::ReceiveLog
    ( TChangedPaths* changes
    , svn_revnum_t rev
    , const StandardRevProps* stdRevProps
    , UserRevPropArray* userRevProps
    , bool mergesFollow)
{
    // we want to receive revision numbers and "mergesFollow" only

    assert (changes == NULL);
    assert (stdRevProps == NULL);
    assert (userRevProps == NULL);

    UNREFERENCED_PARAMETER(changes);
    UNREFERENCED_PARAMETER(stdRevProps);
    UNREFERENCED_PARAMETER(userRevProps);

    // special case: end of merge list?

    if (rev == SVN_INVALID_REVNUM)
    {
        if (options.GetReceiver() != NULL)
        {
            options.GetReceiver()->ReceiveLog ( NULL
                                              , NO_REVISION
                                              , NULL
                                              , NULL
                                              , mergesFollow);
        }
    }
    else
    {
        parentQuery->LogRevision ( static_cast<revision_t>(rev)
                                 , options
                                 , mergesFollow);
    }
}

///////////////////////////////////////////////////////////////
// construction
///////////////////////////////////////////////////////////////

CCacheLogQuery::CMergeLogger::CMergeLogger ( CCacheLogQuery* parentQuery
                                           , const CLogOptions& options)
    : parentQuery (parentQuery)
    , options (options)
{
}

///////////////////////////////////////////////////////////////
// CCacheLogQuery
///////////////////////////////////////////////////////////////
// when asking SVN to fill our cache, we want it to cover
// the missing revisions as well as the ones that we already
// know to have no info for this path. Otherwise, we may end
// up creating a lot of queries for paths that are seldom
// modified.
///////////////////////////////////////////////////////////////

revision_t
CCacheLogQuery::NextAvailableRevision ( const CDictionaryBasedTempPath& path
                                      , revision_t startRevision
                                      , revision_t endRevision
                                      , const CDataAvailable& dataAvailable) const
{
    const CRevisionIndex& revisions = cache->GetRevisions();
    revision_t lastRevisionToCheck = min ( endRevision
                                         , revisions.GetFirstRevision());

    while ((startRevision >= endRevision) && (startRevision != NO_REVISION))
    {
        // skip known revisions that are irrelevant for path

        CStrictLogIterator iterator (cache, startRevision, path);
        iterator.Retry();
        startRevision = iterator.GetRevision();

        // found the next cache entry for this path?

        if (dataAvailable (&iterator))
            return startRevision+1;

        // skip N/A revisions

        while (   (startRevision >= lastRevisionToCheck)
               && (startRevision != NO_REVISION)
               && (!dataAvailable (startRevision)))
        {
            --startRevision;
        }
    }

    // there is no cached data available for this path and revision range

    return endRevision-1;
}

// Determine an end-revision that would fill many cache gaps efficiently

revision_t CCacheLogQuery::FindOldestGap ( const CDictionaryBasedTempPath& path
                                         , revision_t startRevision
                                         , revision_t endRevision
                                         , const CDataAvailable& dataAvailable) const
{
    // consider the following trade-off:
    // 1 server round trip takes about as long as receiving
    // 50 log entries and / or 200 changes
    //
    // -> filling cache data gaps of up to 50 revisions
    //    (or 200 changes) apart with a single fetch should
    //    result in close-to-optimal log performance.

    enum
    {
        RECEIVE_TO_ROUNTRIP_TRADEOFF_REVS = 50,
        RECEIVE_TO_ROUNTRIP_TRADEOFF_CHANGES = 200
    };

    // find the next "long" section of consecutive cached log info

    revision_t lastMissing = startRevision;

    const CRevisionInfoContainer& logInfo = cache->GetLogInfo();
    const CRevisionIndex& revisions = cache->GetRevisions();

    // length of the current sequence of known data

    size_t knownSequenceLength = 0;
    size_t knownSequenceChanges = 0;

    // scan cached history data

    CCopyFollowingLogIterator iterator (cache, startRevision, path);
    iterator.Retry();

    while (   (startRevision >= endRevision)
           && (startRevision != NO_REVISION)
           && (knownSequenceLength < RECEIVE_TO_ROUNTRIP_TRADEOFF_REVS)
           && (knownSequenceChanges < RECEIVE_TO_ROUNTRIP_TRADEOFF_CHANGES))
    {
        // skip known revisions that are irrelevant for path

        startRevision = iterator.GetRevision();
        if( startRevision == NO_REVISION )
            break;

        // found the next cache entry for this path?

        if (!dataAvailable (&iterator))
        {
            // move the "last gap" pointer,
            // restart the sequence counting (until we found the first existing)

            lastMissing = startRevision;

            // try next revision

            iterator.SetRevision (startRevision-1);
            iterator.Retry();
        }
        else
        {
            // another known entry

            ++knownSequenceLength;

            index_t revIndex = revisions[startRevision];
            knownSequenceChanges += logInfo.GetChangesEnd (revIndex)
                                  - logInfo.GetChangesBegin (revIndex);

            // follow history

            iterator.Advance();
        }
    }

    // fetch everything down to and including the lowest missing revision
    // scanned (e.g. either the end of the log or before the first large
    // consecutive block of cached revision data for this path)

    return lastMissing == NO_REVISION ? 0 : lastMissing;
}

// ask SVN to fill the log -- at least a bit
// Possibly, it will stop long before endRevision and limit!

revision_t CCacheLogQuery::FillLog ( revision_t startRevision
                                   , revision_t endRevision
                                   , const CDictionaryBasedTempPath& startPath
                                   , int limit
                                   , const CLogOptions& options
                                   , const CDataAvailable& dataAvailable)
{
    // don't try to get a full log; just enough to continue our search

    revision_t nextAvailable = NextAvailableRevision ( startPath
                                                     , startRevision
                                                     , endRevision
                                                     , dataAvailable);

    // propose a (possibly) different end-revision to cover more gaps
    // within the desired range (receiving duplicate intermediate
    // log info is less expensive than starting a new log query)

    revision_t cacheOptimalEndRevision = FindOldestGap ( startPath
                                                       , startRevision
                                                       , endRevision
                                                       , dataAvailable);

    // extend the requested range, if that is probably more efficient
    // (fill many small gaps at once)

    endRevision = min (nextAvailable+1, cacheOptimalEndRevision);

    // now, fill the cache (somewhat) and forward to the receiver

    return CLogFiller(repositoryInfoCache)
               .FillLog ( cache
                        , URL
                        , uuid
                        , svnQuery
                        , startRevision
                        , max (min (startRevision, endRevision), (revision_t)0)
                        , startPath
                        , limit
                        , options);
}

// fill the receiver's change list buffer

void CCacheLogQuery::GetChanges
    ( TChangedPaths& result
    , CPathToStringMap& pathToStringMap
    , CRevisionInfoContainer::CChangesIterator first
    , const CRevisionInfoContainer::CChangesIterator& last)
{
    for (; first != last; ++first)
    {
        result.push_back (SChangedPath());

        SChangedPath& entry = result.back();
        entry.path = pathToStringMap.AsString (first.GetPath());
        entry.nodeKind = static_cast<svn_node_kind_t>(first->GetPathType());
        entry.action = (DWORD)first.GetAction() / 4;

        if (first.HasFromPath() && (first.GetFromRevision() != NO_REVISION))
        {
            entry.copyFromPath = pathToStringMap.AsString (first.GetFromPath());
            entry.copyFromRev = first.GetFromRevision();
        }
        else
        {
            entry.copyFromRev = 0;
        }
    }
}

// fill the receiver's user rev-prop list buffer

void CCacheLogQuery::GetUserRevProps
    ( UserRevPropArray& result
    , CRevisionInfoContainer::CUserRevPropsIterator first
    , const CRevisionInfoContainer::CUserRevPropsIterator& last
    , const TRevPropNames& userRevProps)
{
    TRevPropNames::const_iterator begin = userRevProps.begin();
    TRevPropNames::const_iterator end = userRevProps.end();

    for (; first != last; ++first)
    {
        CString name = CUnicodeUtils::GetUnicode (first.GetName());
        CString value = CUnicodeUtils::GetUnicode (first.GetValue().c_str());

        // add to output list,
        // if it matches the filter (or if there is no filter)

        if (userRevProps.empty() || (std::find (begin, end, name) != end))
            result.Add (name, value);
    }
}

void CCacheLogQuery::SendToReceiver ( revision_t revision
                                    , const CLogOptions& options
                                    , bool mergesFollow)
{
    // special cases

    if (options.GetReceiver() == NULL)
        return;

    if (options.GetRevsOnly())
    {
        // just notify the receiver that we made some progress

        options.GetReceiver()->ReceiveLog ( NULL
                                          , revision
                                          , NULL
                                          , NULL
                                          , mergesFollow);
        return;
    }

    // access to the cached log info for this revision

    index_t logIndex = cache->GetRevisions()[revision];
    const CRevisionInfoContainer& logInfo = cache->GetLogInfo();

    // change list

    TChangedPaths changes;
    if (options.GetIncludeChanges())
    {
        CRevisionInfoContainer::CChangesIterator first
            (logInfo.GetChangesBegin (logIndex));
        CRevisionInfoContainer::CChangesIterator last
            (logInfo.GetChangesEnd (logIndex));

        changes.reserve (last - first);
        GetChanges (changes, pathToStringMap, first, last);
    }

    // standard revprops

    StandardRevProps* standardRevProps = NULL;
    if (options.GetIncludeStandardRevProps())
    {
        // author

        index_t authorID = logInfo.GetAuthorID (logIndex);
        TID2String::const_iterator iter = authorToStringMap.find (authorID);
        if (iter == authorToStringMap.end())
        {
            CString author
                = CUnicodeUtils::GetUnicode (logInfo.GetAuthor (logIndex));
            authorToStringMap.insert (authorID, author);
            iter = authorToStringMap.find (authorID);
        }

        const CString& author = *iter;

        // comment

        logInfo.GetComment (logIndex, scratch);
        CString message = CUnicodeUtils::UTF8ToUTF16 (scratch);

        // time stamp

        __time64_t timeStamp = logInfo.GetTimeStamp (logIndex);

        // create the actual object

        standardRevProps = new (alloca (sizeof (StandardRevProps)))
                            StandardRevProps (author, message, timeStamp);
    }

    // user revprops

    UserRevPropArray userRevProps;
    if (options.GetIncludeUserRevProps())
        GetUserRevProps ( userRevProps
                        , logInfo.GetUserRevPropsBegin (logIndex)
                        , logInfo.GetUserRevPropsEnd (logIndex)
                        , options.GetUserRevProps());

    // now, send the data to the receiver

    options.GetReceiver()
        ->ReceiveLog ( options.GetIncludeChanges()
                           ? &changes
                           : NULL
                     , revision
                     , standardRevProps
                     , options.GetIncludeUserRevProps()
                           ? &userRevProps
                           : NULL
                     , mergesFollow);

    // clean-up

    if (standardRevProps)
        standardRevProps->~StandardRevProps();
}

// clear string translating caches

void CCacheLogQuery::ResetObjectTranslations()
{
    authorToStringMap.clear();
    pathToStringMap.Clear();
}

// log from cache w/o merge history. Auto-fill cache if data is missing.

void CCacheLogQuery::InternalLog ( revision_t startRevision
                                 , revision_t endRevision
                                 , const CDictionaryBasedTempPath& startPath
                                 , int limit
                                 , const CLogOptions& options)
{
    // clear string translation caches

    ResetObjectTranslations();

    // create the right iterator

    std::auto_ptr<ILogIterator> iterator
        (options.CreateIterator ( cache
                                , startRevision
                                , startPath));

    // what data we need

    CDataAvailable dataAvailable (cache, options);

    // find first suitable entry or cache gap

    iterator->Retry();

    // report starts at startRevision or earlier revision

    revision_t lastReported = startRevision+1;

    // we cannot receive logs for rev < 0

    if (endRevision < 0)
        endRevision = 0;

    // crawl & update the cache, report entries found

    while ((iterator->GetRevision() >= endRevision) && !iterator->EndOfPath())
    {
        if (!dataAvailable (iterator.get()))
        {
            // special case:
            // the path seems not to be from this repository.
            // At least, no "add" has been found for it.

            if (   (iterator->GetRevision() == 0)
                && !iterator->GetPath().IsRoot())
            {
                // we have to stop @rev 0, then

                return;
            }

            // we must not fetch revisions twice
            // (this may cause an indefinite loop)

            assert (iterator->GetRevision() < lastReported);
            if (iterator->GetRevision() >= lastReported)
            {
                return;
            }

            // don't try to fetch data when in "disconnected" mode

            if (repositoryInfoCache->IsOffline (uuid, root, false))
            {
                // just skip unknown revisions
                // (we already warned the use that this might
                // result bogus results)

                iterator->ToNextAvailableData();
            }
            else
            {
                // our cache is incomplete -> fill it.
                // Report entries immediately to the receiver
                // (as to allow the user to cancel this action).

                lastReported = FillLog ( iterator->GetRevision()
                                       , endRevision
                                       , iterator->GetPath()
                                       , limit
                                       , options
                                       , dataAvailable);
            }

            // the current iterator position should contain data now.
            // continue looking for the next *relevant* entry.

            iterator->Retry();
        }
        else
        {
            // found an entry. Report it if not already done.

            revision_t revision = iterator->GetRevision();
            if (revision < lastReported)
                SendToReceiver (revision, options, false);

            // enough?

            if ((limit != 0) && (--limit == 0))
                return;
            else
                iterator->Advance();
        }
    }
}

void CCacheLogQuery::InternalLogWithMerge ( revision_t startRevision
                                          , revision_t endRevision
                                          , const CDictionaryBasedTempPath& startPath
                                          , int limit
                                          , const CLogOptions& options)
{
    // clear string translation caches

    ResetObjectTranslations();

    // this object will only receive the revision numbers
    // and give us a callback to add the other info from cache
    // (auto-fill the latter)

    CMergeLogger logger (this, options);

    // fetch revisions only but include merge children

    CTSVNPath path;
    if (startPath.IsRoot())
        path.SetFromSVN (URL);
    else
        path.SetFromSVN (URL + startPath.GetPath().c_str());

    svnQuery->Log ( CTSVNPathList (path)
                  , static_cast<long>(startRevision)
                  , static_cast<long>(startRevision)
                  , static_cast<long>(endRevision)
                  , limit
                  , options.GetStrictNodeHistory()
                  , &logger
                  , false
                  , true
                  , false
                  , false
                  , TRevPropNames());
}

// follow copy history until the startRevision is reached

CDictionaryBasedTempPath CCacheLogQuery::TranslatePegRevisionPath
    ( revision_t pegRevision
    , revision_t startRevision
    , const CDictionaryBasedTempPath& startPath)
{
    CCopyFollowingLogIterator iterator (cache, pegRevision, startPath);
    iterator.Retry();

    while ((iterator.GetRevision() > startRevision) && !iterator.EndOfPath())
    {
        if (iterator.DataIsMissing())
        {
            // don't try to fetch data when in "disconnected" mode

            if (repositoryInfoCache->IsOffline (uuid, root, false))
            {
                // just skip unknown revisions
                // (we already warned the use that this might
                // result bogus results)

                iterator.ToNextAvailableData();
            }
            else
            {
                // our cache is incomplete -> fill it.
                // Report entries immediately to the receiver
                // (as to allow the user to cancel this action).

                CLogOptions options (false);
                FillLog ( iterator.GetRevision()
                        , startRevision
                        , iterator.GetPath()
                        , 0
                        , options
                        , CDataAvailable (cache, options));
            }

            // the current iterator position should contain data now.
            // continue looking for the next *relevant* entry.

            iterator.Retry();
        }
        else
        {
            iterator.Advance();
        }
    }

    return iterator.GetPath();
}

// extract the repository-relative path of the URL / file name
// and open the cache

CDictionaryBasedTempPath CCacheLogQuery::GetRelativeRepositoryPath
    (const CTSVNPath& url)
{
    // URL and / or uuid may be unknown if there is no repository list entry
    // (e.g. this is a temp. cache object) and there is no server connection

    assert (uuid.IsEmpty() == URL.IsEmpty());
    if (uuid.IsEmpty())
    {
        // we can't cache the data -> return an invalid path

        return CDictionaryBasedTempPath (NULL);
    }

    // load / create cache

    if (caches != NULL)
    {
        cache = caches->GetCache (uuid, root);
    }
    if ((caches == NULL)||(cache == NULL))
    {
        delete tempCache;
        tempCache = new CCachedLogInfo(L"");

        cache = tempCache;
    }

    // get path object
    // (URLs are always escaped, so we must unescape them)

    CStringA svnURLPath = CUnicodeUtils::GetUTF8 (url.GetSVNPathString());

    // the initial url can be in the format file:///\, but the
    // repository root returned would still be file://
    // to avoid string length comparison faults, we adjust
    // the repository root here to match the initial url

    if (URL.Left(9).CompareNoCase("file:///\\") == 0)
        URL.Delete (7, 2);
    if (svnURLPath.Left(9).CompareNoCase("file:///\\") == 0)
        svnURLPath.Delete (7, 2);

    CStringA relPath = svnURLPath.Mid (URL.GetLength());
    relPath = CPathUtils::PathUnescape (relPath);

    const CPathDictionary* paths = &cache->GetLogInfo().GetPaths();
    return CDictionaryBasedTempPath (paths, (const char*)relPath);
}

// utility method: we throw that error in several places

void CCacheLogQuery::ThrowBadRevision() const
{
    throw SVNError ( SVN_ERR_CLIENT_BAD_REVISION
                   , "Invalid revision passed to Log().");
}

// decode special revisions:
// base / head must be initialized with NO_REVISION
// and will be used to cache these values.

revision_t CCacheLogQuery::DecodeRevision ( const CTSVNPath& path
                                          , const CTSVNPath& url
                                          , const SVNRev& revision
                                          , const SVNRev& peg) const
{
    if (!revision.IsValid())
        ThrowBadRevision();

    // efficiently decode standard cases: revNum, HEAD, BASE/WORKING

    revision_t result = (revision_t)NO_REVISION;
    switch (revision.GetKind())
    {
    case svn_opt_revision_number:
        {
            result = static_cast<LONG>(revision);
            break;
        }

    case svn_opt_revision_head:
        {
            result = repositoryInfoCache->GetHeadRevision (uuid, url);
            if (result == NO_REVISION)
                throw SVNError (repositoryInfoCache->GetLastError());

            break;
        }

    case svn_opt_revision_date:
        {
            // find latest revision before the given date

            const CRevisionIndex& revisions = cache->GetRevisions();
            result = cache->FindRevisionByDate (revision.GetDate());
            CString URL = url.GetSVNPathString();
            bool offline = repositoryInfoCache->IsOffline (uuid, URL, false);

            // special case: date is before revision 1 / first cached revision

            if (   (result == NO_REVISION)
                && ((revisions.GetFirstCachedRevision() < 2) || offline))
            {
                // we won't get anyting better than this:

                result = 0;
                break;
            }

            // don't ask any more questions if we are off-line:
            // we will have found the highest *cached* revision
            // before the specified date

            if (offline)
            {
                assert (revision != NO_REVISION);
                break;
            }

            // verify that this is the limiting revision,
            // i.e that the next one is beyond the specified date

            if (result != NO_REVISION)
            {
                // are we missing the next revision?

                // This code is not optimal. However, we cannot use
                // the skip delta info because we don't know the
                // actual path *in that revision*.

                if (revisions [result+1] == NO_INDEX)
                {
                    // is it HEAD?

                    if (revisions.GetLastCachedRevision() > result+1)
                    {
                        result = (revision_t)NO_REVISION;
                    }
                    else
                    {
                        revision_t head
                            = repositoryInfoCache->GetHeadRevision (uuid, url);

                        if (result != head)
                            result = (revision_t)NO_REVISION;
                    }
                }
            }

            // let SVN translate the date into a revision

            if (result == NO_REVISION)
            {
                // first attempt: ask directly for that revision

                SVNInfo info;
                const SVNInfoData * baseInfo
                    = info.GetFirstFileInfo (path, peg, revision);

                if (baseInfo != NULL)
                {
                    result = static_cast<LONG>(baseInfo->rev);
                    break;
                }

                // was it just the revision being out of bound?

                if (info.GetError()->apr_err == SVN_ERR_CLIENT_UNRELATED_RESOURCES)
                {
                    // this will happen for dates in the future (post-HEAD)
                    // as long as the URL is valid.
                    // -> we are propably at the bottom end

                    result = 0;
                    break;
                }

                // (Probably) a server access errror. Retry off-line.

                if (repositoryInfoCache->IsOffline (uuid, URL, true))
                    return DecodeRevision (path, url, revision, peg);
                else
                    throw SVNError(info.GetError());
            }

            break;
        }

    default:
        {
            SVNInfo info;
            const SVNInfoData * baseInfo
                = info.GetFirstFileInfo (path, peg, revision);
            if (baseInfo == NULL)
                throw SVNError(info.GetError());

            result = static_cast<LONG>(baseInfo->rev);
        }
    }

    // did we actually get a valid revision?

    if (result == NO_REVISION)
        ThrowBadRevision();

    return result;
}

// get the (exactly) one path from targets
// throw an exception, if there are none or more than one

CTSVNPath CCacheLogQuery::GetPath (const CTSVNPathList& targets) const
{
    if (targets.GetCount() != 1)
        throw SVNError ( SVN_ERR_INCORRECT_PARAMS
                       , "Must specify exactly one path to get the log from.");

    // GetURLFromPath() always returns the URL escaped, so we have to escape the url we
    // get from the client too.
    return targets [0].IsUrl()
        ? CTSVNPath (CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(targets [0].GetSVNPathString()))))
        : targets [0];
}

// construction / destruction

CCacheLogQuery::CCacheLogQuery (CLogCachePool* caches, ILogQuery* svnQuery)
    : caches (caches)
    , repositoryInfoCache (&caches->GetRepositoryInfo())
    , cache (NULL)
    , tempCache (NULL)
    , URL()
    , svnQuery (svnQuery)
{
}

CCacheLogQuery::CCacheLogQuery (SVN& svn, ILogQuery* svnQuery)
    : caches (NULL)
    , repositoryInfoCache (NULL)
    , cache (NULL)
    , tempCache (NULL)
    , URL()
    , svnQuery (svnQuery)
{
    repositoryInfoCache = new CRepositoryInfo (svn, CString());
}

CCacheLogQuery::~CCacheLogQuery(void)
{
    // temporary cache objects?

    delete tempCache;

    if (caches == NULL)
        delete repositoryInfoCache;
}

// query a section from log for multiple paths
// (special revisions, like "HEAD", supported)

void CCacheLogQuery::Log ( const CTSVNPathList& targets
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
                         , const TRevPropNames& userRevProps)
{
    // the path to log for

    CTSVNPath path = GetPath (targets);

    // resolve respository URL and UUID

    root = repositoryInfoCache->GetRepositoryRootAndUUID (path, uuid);
    URL = CUnicodeUtils::GetUTF8 (root);

    // get the URL for that path

    CTSVNPath url = path.IsUrl()
        ? path
        : CTSVNPath (repositoryInfoCache->GetSVN().GetURLFromPath (path));

    // load cache and translate the path
    // (don't get the repo info from SVN, if it had to be fetched from the server
    //  -> let GetRelativeRepositoryPath() use our repository property cache)

    CDictionaryBasedTempPath repoPath = GetRelativeRepositoryPath (url);
    if (!repoPath.IsValid())
        return;

    // decode revisions
    // makes also sure that these aren't NO_REVISION values

    revision_t startRevision = DecodeRevision (path, url, start, peg_revision);
    revision_t endRevision = DecodeRevision (path, url, end, peg_revision);

    // The svn_client_log3() API defaults the peg revision to HEAD for URLs
    // and WC for local paths if it isn't set explicitly.

    SVNRev temp = peg_revision;
    if (!peg_revision.IsValid())
        temp = path.IsUrl() ? SVNRev::REV_HEAD : SVNRev::REV_WC;

    revision_t pegRevision = DecodeRevision (path, url, temp, peg_revision);

    // order revisions

    if (endRevision > startRevision)
        std::swap (endRevision, startRevision);

    if (pegRevision < startRevision)
        pegRevision = startRevision;

    // find the path to start from

    CDictionaryBasedTempPath startPath
        = TranslatePegRevisionPath ( pegRevision
                                   , startRevision
                                   , repoPath);

    // do it

    CLogOptions options ( strictNodeHistory
                        , receiver
                        , includeChanges
                        , includeMerges
                        , includeStandardRevProps
                        , includeUserRevProps
                        , userRevProps);

    if (includeMerges)
        InternalLogWithMerge ( startRevision
                             , endRevision
                             , startPath
                             , limit
                             , options);
    else
        InternalLog ( startRevision
                    , endRevision
                    , startPath
                    , limit
                    , options);
}

// relay the content of a single revision to the receiver
// (if the latter is not NULL)

void CCacheLogQuery::LogRevision ( revision_t revision
                                 , const CLogOptions& options
                                 , bool mergesFollow)
{
    // make sure the data is in our cache

    CDataAvailable dataAvailable (cache, options);
    if (!dataAvailable (revision))
    {
        // we will fetch the next ~100 revs at the repository root
        // but we will not send them to the receiver, yet (cache fill only)

        CLogOptions fillOptions (options, NULL);
        CDictionaryBasedTempPath root ( &cache->GetLogInfo().GetPaths()
                                      , std::string());

        FillLog ( revision
                , 0
                , root
                , 100
                , fillOptions
                , dataAvailable);
    }

    // send it to the receiver

    SendToReceiver (revision, options, mergesFollow);
}

// access to the cache

CCachedLogInfo* CCacheLogQuery::GetCache() const
{
    assert (cache != NULL);
    return cache;
}

// get the repository root URL

const CStringA& CCacheLogQuery::GetRootURL() const
{
    assert (!URL.IsEmpty());
    return URL;
}

// could we get at least some data

bool CCacheLogQuery::GotAnyData() const
{
    return cache != NULL;
}

// for tempCaches: write content to "real" cache files
// (no-op if this is does not use a temp. cache)

void CCacheLogQuery::UpdateCache (CCacheLogQuery* targetQuery) const
{
    // resolve URL

    CTSVNPath path;
    path.SetFromSVN (URL);

    CString uuid = repositoryInfoCache->GetRepositoryUUID (path);

    // UUID may be unknown if there is no repository list entry
    // (e.g. this is a temp. cache object) and there is no server connection

    if (uuid.IsEmpty())
        return;

    // load / create cache and merge it with our results

    assert(!uuid.IsEmpty());

    CLogCachePool* caches
        = targetQuery->repositoryInfoCache->GetSVN().GetLogCachePool();
    CCachedLogInfo* cache
        = caches->GetCache (uuid, CUnicodeUtils::GetUnicode (URL));
    if ((cache != this->cache) && (this->cache != NULL))
    {
        cache->Update (*this->cache);

        //

        targetQuery->cache = cache;
        targetQuery->uuid = uuid;
        targetQuery->URL = URL;
    }
}
