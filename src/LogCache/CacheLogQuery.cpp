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
#include "StdAfx.h"

#include "CacheLogQuery.h"

#include "CachedLogInfo.h"
#include "CopyFollowingLogIterator.h"
#include "StrictLogIterator.h"
#include "LogCachePool.h"

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
    , presenceMask (0)
    , revsOnly (   !includeChanges 
                && !includeStandardRevProps 
                && !includeUserRevProps)
{
    if (includeStandardRevProps)
        presenceMask = CRevisionInfoContainer::HAS_STANDARD_REVPROPS;
    if (includeChanges)
        presenceMask |= CRevisionInfoContainer::HAS_CHANGEDPATHS;
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
    , requiredMask (options.GetPresenceMask())
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
// make sure, we can iterator over the given range for the given path
///////////////////////////////////////////////////////////////

void CCacheLogQuery::CLogFiller::MakeRangeIterable ( const CDictionaryBasedPath& path
												   , revision_t startRevision
												   , revision_t count)
{
    // update the cache before parsing its content

    cache->Update (*updateData);
    updateData->Clear();

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

	CLogFiller().FillLog ( cache
						 , URL
						 , svnQuery
						 , iterator.GetRevision()
						 , startRevision
						 , CDictionaryBasedTempPath (path)
						 , 0
                         , CLogOptions());
}

// cache data

void CCacheLogQuery::CLogFiller::WriteToCache 
    ( LogChangedPathArray* changes
    , revision_t revision
    , const StandardRevProps* stdRevProps
    , UserRevPropArray* userRevProps)
{
    // If it is not yet in cache, add it to the cache directly.
    // Otherwise it is a modification of an existing revision
    // -> collect them an update cache later for maximum performance.

    CCachedLogInfo* targetCache = cache->GetRevisions()[revision] == NO_INDEX
                                ? cache
                                : updateData;

	// create the revision entry

    std::string author;
    std::string message;
    __time64_t timeStamp = 0;

    if (stdRevProps)
    {
        author = (const char*)CUnicodeUtils::GetUTF8 (stdRevProps->author);
        message = (const char*)CUnicodeUtils::GetUTF8 (stdRevProps->message);
        timeStamp = stdRevProps->timeStamp;
    }

	targetCache->Insert ( revision
                        , author
                        , message
                        , timeStamp
                        , options.GetPresenceMask());

	// add all changes

    if (changes != NULL)
    {
	    for (INT_PTR i = 0, count = changes->GetCount(); i < count; ++i)
	    {
		    const LogChangedPath* change = changes->GetAt (i);

		    CRevisionInfoContainer::TChangeAction action 
			    = (CRevisionInfoContainer::TChangeAction)(change->action * 4);
		    std::string path 
			    = (const char*)SVN::MakeSVNUrlOrPath (change->sPath);
		    std::string copyFromPath 
			    = (const char*)SVN::MakeSVNUrlOrPath (change->sCopyFromPath);
		    revision_t copyFromRevision 
			    = change->lCopyFromRev == 0 
			    ? NO_REVISION 
			    : static_cast<revision_t>(change->lCopyFromRev);

		    targetCache->AddChange (action, path, copyFromPath, copyFromRevision);
	    }
    }

	// add use revprops

    if (userRevProps != NULL)
    {
	    for (INT_PTR i = 0, count = userRevProps->GetCount(); i < count; ++i)
	    {
		    const UserRevProp* revprop = userRevProps->GetAt (i);

            std::string name 
                = (const char*)CUnicodeUtils::GetUTF8 (revprop->name);
            std::string value 
                = (const char*)CUnicodeUtils::GetUTF8 (revprop->value);

            targetCache->AddUserRevProp (name, value);
	    }
    }

	// update our path info

	currentPath->RepeatLookup();

	// mark the gap and update the current path

	if (firstNARevision > revision)
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

// implement ILogReceiver

void CCacheLogQuery::CLogFiller::ReceiveLog 
    ( LogChangedPathArray* changes
    , svn_revnum_t rev
    , const StandardRevProps* stdRevProps
    , UserRevPropArray* userRevProps
    , bool mergesFollow)
{
    // store the data we just received

	revision_t revision = static_cast<revision_t>(rev);
    WriteToCache (changes, rev, stdRevProps, userRevProps);

	// due to renames / copies, we may continue on a different path

	if (!options.GetStrictNodeHistory())
	{
        // maybe the revision was known before but we had no changes info
        // -> we received them now
        // -> update the cache in that case

        index_t index = cache->GetRevisions()[revision];
        if ((  cache->GetLogInfo().GetPresenceFlags (index) 
             & CRevisionInfoContainer::HAS_CHANGEDPATHS) == 0)
        {
            cache->Update (*updateData);
            updateData->Clear();
        }

        // now, iterate as usual

		CCopyFollowingLogIterator iterator (cache, revision, *currentPath);
		iterator.Advance();

		*currentPath = iterator.GetPath();
	}

	// the first revision we may not have information about is the one
	// immediately preceding the on we just received from the server

	firstNARevision = revision-1;

	// hand on to the original log receiver

	if (options.GetReceiver() != NULL)
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

// default construction / destruction

CCacheLogQuery::CLogFiller::CLogFiller()
    : cache (NULL)
    , updateData (new CCachedLogInfo())
    , svnQuery (NULL)
    , firstNARevision (NO_REVISION)
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
								    , ILogQuery* svnQuery
								    , revision_t startRevision
									, revision_t endRevision
									, const CDictionaryBasedTempPath& startPath
									, int limit
									, const CLogOptions& options)
{
    this->cache = cache;
	this->URL = URL;
    this->svnQuery = svnQuery;
    this->options = options;

    firstNARevision = startRevision;
	currentPath.reset (new CDictionaryBasedTempPath (startPath));

    // full path to be passed to SVN.
    // don't append a trailing "/", if the path is empty (i.e. root)

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
				  , this
                  , true
                  , false
                  , options.GetIncludeStandardRevProps()
                  , options.GetIncludeUserRevProps()
                  , TRevPropNames());

    cache->Update (*updateData);
    updateData->Clear();

	if (firstNARevision == startRevision) 
	{
		// the log was empty

		if (currentPath->IsFullyCachedPath())
		{
			cache->AddSkipRange ( currentPath->GetBasePath()
								, endRevision
								, startRevision - endRevision + 1);
		}
		else
		{
			MakeRangeIterable ( currentPath->GetBasePath()
							  , endRevision
							  , startRevision - endRevision + 1);
		}
	}

	return firstNARevision+1;
}

///////////////////////////////////////////////////////////////
// CMergeLogger
///////////////////////////////////////////////////////////////
// implement ILogReceiver
///////////////////////////////////////////////////////////////

void CCacheLogQuery::CMergeLogger::ReceiveLog 
    ( LogChangedPathArray* changes
    , svn_revnum_t rev
    , const StandardRevProps* stdRevProps
    , UserRevPropArray* userRevProps
    , bool mergesFollow)
{
    // we want to receive revision numbers and "mergesFollow" only

    assert (changes == NULL);
    assert (stdRevProps == NULL);
    assert (userRevProps == NULL);

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
	revision_t lastRevisionToCheck = min ( endRevision
										 , revisions.GetFirstRevision());

    // length of the current sequence of known data

    size_t knownSequenceLength = 0;
    size_t knownSequenceChanges = 0;

	while (   (startRevision >= endRevision) 
           && (startRevision != NO_REVISION)
           && (knownSequenceLength < RECEIVE_TO_ROUNTRIP_TRADEOFF_REVS)
           && (knownSequenceChanges < RECEIVE_TO_ROUNTRIP_TRADEOFF_CHANGES))
	{
		// skip known revisions that are irrelevant for path

		CStrictLogIterator iterator (cache, startRevision, path);
		iterator.Retry();
		startRevision = iterator.GetRevision();

		// found the next cache entry for this path?

		if (!dataAvailable (&iterator))
		{
			// move the "last gap" pointer, 
            // restart the sequence counting (until we found the first existing)

			lastMissing = startRevision;
            knownSequenceLength = 0;
            knownSequenceChanges = 0;
		}
		else
		{
			// another known entry 

            ++knownSequenceLength;

            index_t revIndex = revisions[startRevision];
            knownSequenceChanges += logInfo.GetChangesEnd (revIndex) 
                                  - logInfo.GetChangesBegin (revIndex);
		}

        // try the next revision

        --startRevision;
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

	return CLogFiller().FillLog ( cache
								, URL
								, svnQuery
								, startRevision
								, max (min (startRevision, endRevision), 1)
								, startPath
								, limit
								, options);
}

// fill the receiver's change list buffer 

void CCacheLogQuery::GetChanges 
    ( LogChangedPathArray& result
    , CRevisionInfoContainer::CChangesIterator& first
    , CRevisionInfoContainer::CChangesIterator& last)
{
	for (; first != last; ++first)
	{
		// find the item in the hash

		std::auto_ptr<LogChangedPath> changedPath (new LogChangedPath);

		// extract the path name
        // use map to cache results (speed-up and reduce memory footprint)

        CDictionaryBasedPath path = first.GetPath();
        TID2String::const_iterator iter (pathToStringMap.find (path.GetIndex()));
        if (iter == pathToStringMap.end())
        {
    		changedPath->sPath = SVN::MakeUIUrlOrPath (path.GetPath().c_str());
            pathToStringMap.insert (path.GetIndex(), changedPath->sPath);
        }
        else
        {
            changedPath->sPath = *iter;
        }

		// decode the action

		CRevisionInfoContainer::TChangeAction action = first.GetAction();
		changedPath->action = (DWORD)action / 4;

		// decode copy-from info

		if (   first.HasFromPath()
			&& (first.GetFromRevision() != NO_REVISION))
		{
			std::string path = first.GetFromPath().GetPath();

			changedPath->lCopyFromRev = first.GetFromRevision();
			changedPath->sCopyFromPath 
				= SVN::MakeUIUrlOrPath (path.c_str());
		}
		else
		{
			changedPath->lCopyFromRev = 0;
		}

		result.Add (changedPath.release());
	} 
}

// fill the receiver's user rev-prop list buffer 

void CCacheLogQuery::GetUserRevProps 
    ( UserRevPropArray& result
    , CRevisionInfoContainer::CUserRevPropsIterator& first
	, CRevisionInfoContainer::CUserRevPropsIterator& last
    , const TRevPropNames& userRevProps)
{
    TRevPropNames::const_iterator begin = userRevProps.begin();
    TRevPropNames::const_iterator end = userRevProps.end();

	for (; first != last; ++first)
	{
		std::auto_ptr<UserRevProp> revProp (new UserRevProp);

        revProp->name = CUnicodeUtils::GetUnicode (first.GetName());
        revProp->value = CUnicodeUtils::GetUnicode (first.GetValue().c_str());

        // add to output list, 
        // if it matches the filter (or if there is no filter)

        if (   userRevProps.empty() 
            || std::find (begin, end, revProp->name) != end)
        {
		    result.Add (revProp.release());
        }
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
    }

    // access to the cached log info for this revision

    index_t logIndex = cache->GetRevisions()[revision];
    const CRevisionInfoContainer& logInfo = cache->GetLogInfo();

    // change list

    LogChangedPathArray changes;
    if (options.GetIncludeChanges())
        GetChanges ( changes
                   , logInfo.GetChangesBegin (logIndex)
                   , logInfo.GetChangesEnd (logIndex));

    // standard revprops

    StandardRevProps standardRevProps;
    if (options.GetIncludeStandardRevProps())
    {
        // author

        index_t authorID = logInfo.GetAuthorID (logIndex);
        TID2String::const_iterator iter = authorToStringMap.find (authorID);
        if (iter == authorToStringMap.end())
        {
            standardRevProps.author     
                = CUnicodeUtils::GetUnicode (logInfo.GetAuthor (logIndex));
            authorToStringMap.insert (authorID, standardRevProps.author);
        }
        else
        {
            standardRevProps.author = *iter;
        }

        // comment

		standardRevProps.message 
			= CUnicodeUtils::GetUnicode (logInfo.GetComment (logIndex).c_str());

        // time stamp

        standardRevProps.timeStamp = logInfo.GetTimeStamp (logIndex);
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
                     , options.GetIncludeStandardRevProps() 
                           ? &standardRevProps 
                           : NULL
                     , options.GetIncludeUserRevProps() 
                           ? &userRevProps 
                           : NULL
                     , mergesFollow);
}

// clear string translating caches

void CCacheLogQuery::ResetObjectTranslations()
{
    authorToStringMap.clear();
    pathToStringMap.clear();
}

// log from cach w/o merge history. Auto-fill cache if data is missing.

void CCacheLogQuery::InternalLog ( revision_t startRevision
								 , revision_t endRevision
								 , const CDictionaryBasedTempPath& startPath
								 , int limit
                                 , const CLogOptions& options)
{
    // clear string translating caches

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

	// we cannot receive logs for rev 0 (or earlier)

	if (endRevision < 1)
		endRevision = 1;

	// crawl & update the cache, report entries found

	while ((iterator->GetRevision() >= endRevision) && !iterator->EndOfPath())
	{
		if (!dataAvailable (iterator.get()))
		{
			// we must not fetch revisions twice
			// (this may cause an indefinite loop)

			assert (iterator->GetRevision() < lastReported);

			// our cache is incomplete -> fill it.
			// Report entries immediately to the receiver 
			// (as to allow the user to cancel this action).

			lastReported = FillLog ( iterator->GetRevision()
								   , endRevision
								   , iterator->GetPath()
								   , limit
                                   , options
                                   , dataAvailable);

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
    // clear string translating caches

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
            CLogOptions options (false);
			FillLog ( iterator.GetRevision()
					, startRevision
					, iterator.GetPath()
					, 0
					, options
                    , CDataAvailable (cache, options));
        }

		iterator.Advance();
	}

	return iterator.GetPath();
}

// extract the repository-relative path of the URL / file name
// and open the cache

CDictionaryBasedTempPath CCacheLogQuery::GetRelativeRepositoryPath (SVNInfoData& info)
{
	// resolve URL

	URL.Empty();
	if (info.reposUUID.IsEmpty())
	{
		URL = CUnicodeUtils::GetUTF8 
				(repositoryInfoCache->GetRepositoryRootAndUUID 
                    ( CTSVNPath (info.url)
					, info.reposUUID));
	}

	if (URL.IsEmpty())
		URL = CUnicodeUtils::GetUTF8 (info.reposRoot);

	// load / create cache

	assert(!info.reposUUID.IsEmpty());
	if (caches != NULL)
	{
		cache = caches->GetCache (info.reposUUID);
	}
	else
	{
		delete tempCache;
		tempCache = new CCachedLogInfo(L"");

		cache = tempCache;
	}

	// workaround for 1.2.x (and older) working copies

	if (URL.IsEmpty())
	{
		URL = CUnicodeUtils::GetUTF8 
				(repositoryInfoCache->GetRepositoryRoot (CTSVNPath (info.url)));
	}

	// get path object 
	// (URLs are always escaped, so we must unescape them)

	CStringA relPath = CUnicodeUtils::GetUTF8 (info.url).Mid (URL.GetLength());
	relPath = CPathUtils::PathUnescape (relPath);

	const CPathDictionary* paths = &cache->GetLogInfo().GetPaths();
	return CDictionaryBasedTempPath (paths, (const char*)relPath);
}

// get UUID & repository-relative path

SVNInfoData& CCacheLogQuery::GetRepositoryInfo ( const CTSVNPath& path
											   , const SVNRev& pegRevision
 								               , SVNInfoData& baseInfo
									           , SVNInfoData& headInfo) const
{
	// already known?

	if (baseInfo.IsValid())
		return baseInfo;

	if (headInfo.IsValid())
		return headInfo;

	// look it up

	SVNInfo info;
	if (path.IsUrl())
	{
		headInfo = *info.GetFirstFileInfo (path, pegRevision, pegRevision);
		return headInfo;
	}
	else
	{
		baseInfo = *info.GetFirstFileInfo (path, SVNRev(), SVNRev());
		return baseInfo;
	}
}

// decode special revisions:
// base / head must be initialized with NO_REVISION
// and will be used to cache these values.

revision_t CCacheLogQuery::DecodeRevision ( const CTSVNPath& path
			  							  , const SVNRev& revision
										  , SVNInfoData& baseInfo
										  , SVNInfoData& headInfo) const
{
	if (!revision.IsValid())
		throw SVNError ( SVN_ERR_CLIENT_BAD_REVISION
					   , "Invalid revision passed to Log().");

	// efficiently decode standard cases: revNum, HEAD, BASE/WORKING

	switch (revision.GetKind())
	{
	case svn_opt_revision_number:
		return static_cast<LONG>(revision);

	case svn_opt_revision_head:
		if (!headInfo.IsValid())
		{
			SVNInfo info;
			const SVNInfoData * pHeadInfo = info.GetFirstFileInfo (path, SVNRev(), revision);
			if (pHeadInfo)
				headInfo = *pHeadInfo;
			else
				throw SVNError(info.GetError());
		}
		return static_cast<LONG>(headInfo.rev);

	case svn_opt_revision_base:
	case svn_opt_revision_working:
		if (!baseInfo.IsValid())
		{
			SVNInfo info;
			const SVNInfoData * pBaseInfo = info.GetFirstFileInfo (path, SVNRev(), revision);
			if (pBaseInfo)
				baseInfo = *pBaseInfo;
			else
				throw SVNError(info.GetError());
		}

		return static_cast<LONG>(baseInfo.rev);
	}

	// more unusual cases:

	SVNInfo infoProvider;
	const SVNInfoData* info 
		= infoProvider.GetFirstFileInfo (path, SVNRev(), revision);
	if (info)
		return static_cast<LONG>(info->rev);
	throw SVNError(infoProvider.GetError());	
}

// get the (exactly) one path from targets
// throw an exception, if there are none or more than one

CTSVNPath CCacheLogQuery::GetPath (const CTSVNPathList& targets) const
{
	if (targets.GetCount() != 1)
		throw SVNError ( SVN_ERR_INCORRECT_PARAMS
					   , "Must specify exactly one path to get the log from.");

	return targets [0];
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

CCacheLogQuery::~CCacheLogQuery(void)
{
	delete tempCache;
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

	// decode revisions

	SVNInfoData baseInfo;
	SVNInfoData headInfo;

	revision_t startRevision = DecodeRevision (path, start, baseInfo, headInfo);
	revision_t endRevision = DecodeRevision (path, end, baseInfo, headInfo);

	// The svn_client_log3() API defaults the peg revision to HEAD for URLs 
	// and WC for local paths if it isn't set explicitly.

	revision_t pegRevision = 0;
	if (!peg_revision.IsValid())
	{
		if (path.IsUrl())
			pegRevision = DecodeRevision (path, SVNRev::REV_HEAD, baseInfo, headInfo);
		else
			pegRevision = DecodeRevision (path, SVNRev::REV_WC, baseInfo, headInfo);
	}
	else
		pegRevision = DecodeRevision (path, peg_revision, baseInfo, headInfo);

	// order revisions

	if (endRevision > startRevision)
		std::swap (endRevision, startRevision);

	if (pegRevision < startRevision)
		pegRevision = startRevision;

	// load cache and find path to start from
    // (don't get the repo info from SVN, if it had to be fetched from the server
    //  -> let GetRelativeRepositoryPath() use our repository property cache)

    SVNInfoData& info = path.IsUrl()
        ? headInfo 
        : GetRepositoryInfo (path, peg_revision, baseInfo, headInfo);

    if (info.url.IsEmpty())
        info.url = path.GetSVNPathString();

	CDictionaryBasedTempPath startPath 
		= TranslatePegRevisionPath ( pegRevision
								   , startRevision
								   , GetRelativeRepositoryPath (info));

	// do it 

    CLogOptions options (strictNodeHistory
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
        // we will fetch the next ~100 revs at the respository root
        // but we will not send them to the receiver, yet (cache fill only)

        CLogOptions fillOptions (options, NULL);
        CDictionaryBasedTempPath root ( &cache->GetLogInfo().GetPaths()
                                      , std::string());

        FillLog ( revision
			    , 1
			    , root
			    , 100
                , fillOptions
                , dataAvailable);
    }

    // send it to the receiver

    SendToReceiver (revision, options, mergesFollow);
}

// access to the cache

CCachedLogInfo* CCacheLogQuery::GetCache()
{
	assert (cache != NULL);
	return cache;
}
