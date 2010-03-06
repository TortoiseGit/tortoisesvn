// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010 - TortoiseSVN

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
#include "RepositoryLister.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "Resource.h"

#include "SVNProperties.h"
#include "SVNError.h"
#include "SVNHelpers.h"

#include "RepositoryInfo.h"

/////////////////////////////////////////////////////////////////////
// CRepositoryLister::CQuery
/////////////////////////////////////////////////////////////////////

// auto-schedule upon construction

CRepositoryLister::CQuery::CQuery 
    ( const CTSVNPath& path
    , const SRepositoryInfo& repository)
    : path (path)
    , repository (repository)
{
}

// wait for termination

CRepositoryLister::CQuery::~CQuery()
{
	// stop execution asap (or prevent execution at all)

    Terminate();

	// ensure we make the transition to "done"
	// and wait for that transition to be made

    WaitUntilDone(true);
}

// parameter access

const CTSVNPath& CRepositoryLister::CQuery::GetPath() const
{
    return path;
}

const SVNRev& CRepositoryLister::CQuery::GetRevision() const
{
    return repository.revision;
}

// result access. Automatically waits for execution to be finished.

const std::deque<CItem>& CRepositoryLister::CQuery::GetResult()
{
    WaitUntilDone (true);
    return result;
}

const CString& CRepositoryLister::CQuery::GetError()
{
    WaitUntilDone (true);

    if (error.IsEmpty() && HasBeenTerminated())
        error.LoadString (IDS_REPOBROWSE_ERR_CANCEL);

    return error;
}

bool CRepositoryLister::CQuery::Succeeded()
{
    return GetError().IsEmpty();
}

/////////////////////////////////////////////////////////////////////
// CRepositoryLister::CListQuery
/////////////////////////////////////////////////////////////////////

// callback from the SVN::List() method which stores all the information

BOOL CRepositoryLister::CListQuery::ReportList
    ( const CString& path
    , svn_node_kind_t kind
    , svn_filesize_t size
    , bool has_props
    , svn_revnum_t created_rev
    , apr_time_t time
    , const CString& author
    , const CString& locktoken
    , const CString& lockowner
    , const CString& lockcomment
    , bool is_dav_comment
    , apr_time_t lock_creationdate
    , apr_time_t lock_expirationdate
    , const CString& absolutepath)
{
    // skip the parent path

    if (path.IsEmpty())
    {
        // terminate with an error if this was actually a file

        return kind == svn_node_dir ? TRUE : FALSE;
    }

    // store dir entry

	bool abspath_has_slash = (absolutepath.GetAt(absolutepath.GetLength()-1) == '/');
    CString relPath = absolutepath + (abspath_has_slash ? _T("") : _T("/"));
    CItem entry
        ( path
		, CString()
        , kind
        , size
        , has_props
        , created_rev
        , time
        , author
        , locktoken
        , lockowner
        , lockcomment
        , is_dav_comment
        , lock_creationdate
        , lock_expirationdate
        , repository.root + relPath + path
        , repository);

    result.push_back (entry);

    return TRUE;
}

// early termination

BOOL CRepositoryLister::CListQuery::Cancel()
{
    return HasBeenTerminated();
}

// actual job code: just call \ref SVN::List

void CRepositoryLister::CListQuery::InternalExecute()
{
    if (!List (path, GetRevision(), GetRevision(), svn_depth_immediates, true))
    {
        // something went wrong or query was cancelled
        // -> store error, clear results and terminate sub-queries

        if (externalsQuery.get() != NULL)
            externalsQuery->Terminate();

        result.clear();

        error = GetLastErrorMessage();
        if (error.IsEmpty())
            error.LoadString (IDS_REPOBROWSE_ERR_UNKNOWN);
    }
    else 
    {
        // add results from the sub-query

        if (externalsQuery.get() != NULL)
            if (externalsQuery->Succeeded())
            {
                const std::deque<CItem>& externals 
                    = externalsQuery->GetResult();

				for ( std::deque<CItem>::const_iterator iter = externals.begin()
					, end = externals.end()
					; iter != end
					; ++iter)
				{
					if (iter->external_position == 0)
						result.push_back (*iter);
					else
						subPathExternals.push_back (*iter);
				}
            }
            else
            {
                result.clear();
                error = externalsQuery->GetError();
            }
    }
}

// auto-schedule upon construction

CRepositoryLister::CListQuery::CListQuery 
    ( const CTSVNPath& path
    , const SRepositoryInfo& repository
    , bool includeExternals
    , async::CJobScheduler* scheduler)
    : CQuery (path, repository)
    , externalsQuery 
        (includeExternals
            ? new CExternalsQuery (path, repository, scheduler)
            : NULL)
{
    Schedule (false, scheduler);
}

// wait for termination

CRepositoryLister::CListQuery::~CListQuery()
{
	// call overwritten method (base class destructor won't see it)

	Terminate();
}

// cancel the svn:externals sub query as well

void CRepositoryLister::CListQuery::Terminate()
{
    CQuery::Terminate();

    if (externalsQuery.get() != NULL)
        externalsQuery->Terminate();
}

// access additional results

const std::deque<CItem>& CRepositoryLister::CListQuery::GetSubPathExternals()
{
    WaitUntilDone (true);
    return subPathExternals;
}

/////////////////////////////////////////////////////////////////////
// CRepositoryLister::CExternalsQuery
/////////////////////////////////////////////////////////////////////

// actual job code: fetch externals and parse them

void CRepositoryLister::CExternalsQuery::InternalExecute()
{
    // fetch externals definition for the given path

    static const std::string svnExternals (SVN_PROP_EXTERNALS);

	SVNProperties properties (path, GetRevision(), false);

    std::string externals;
    for (int i = 0, count = properties.GetCount(); i < count; ++i)
        if (properties.GetItemName (i) == svnExternals)
            externals = properties.GetItemValue (i);

    // none there?

    if (externals.empty())
        return;

    // parse externals

    SVNPool pool;

    apr_array_header_t* parsedExternals = NULL;
    SVNError svnError 
        (svn_wc_parse_externals_description3 
            ( &parsedExternals
            , path.GetSVNApiPath (pool)
            , externals.c_str()
            , TRUE
            , pool));

    if (svnError.GetCode() == 0)
    {
        SVN svn;

        // copy the parser output

	    for (long i=0; i < parsedExternals->nelts; ++i)
	    {
		    svn_wc_external_item2_t * external 
                = APR_ARRAY_IDX( parsedExternals, i, svn_wc_external_item2_t*);

            if (   (external != NULL)
                && (external->peg_revision.kind == svn_opt_revision_head))
		    {
                // get target repositiory 

				CStringA absoluteURL 
					= CPathUtils::GetAbsoluteURL 
						( external->url
						, CUnicodeUtils::GetUTF8 (repository.root)
						, CUnicodeUtils::GetUTF8 (path.GetSVNPathString()));

                SRepositoryInfo externalRepository;

                CTSVNPath url;
                url.SetFromSVN (absoluteURL);
				externalRepository.root 
					= svn.GetRepositoryRootAndUUID (url, externalRepository.uuid, true);
                externalRepository.revision = external->revision;

                // add the new entry

				CString relPath = CUnicodeUtils::GetUnicode (external->target_dir);
                CItem entry
					( relPath.Mid (relPath.ReverseFind ('/') + 1)
					, relPath

                      // even file externals will be treated as folders because
                      // this is a safe default
                    , svn_node_dir
                    , 0

                      // actually, we don't know for sure whether the target
                      // URL has props but its the safe default to say 'yes'
                    , true  

                      // explicit revision, HEAD or DATE
                    , external->revision.kind == svn_opt_revision_number
                        ? external->revision.value.number
                        : SVN_IGNORED_REVNUM

                      // explicit revision, HEAD or DATE
                    , external->revision.kind == svn_opt_revision_date
                        ? external->revision.value.date
                        : 0
                    , CString()
                    , CString()
                    , CString()
                    , CString()
                    , false
                    , 0
                    , 0
                    , CPathUtils::PathUnescape 
                        (CUnicodeUtils::GetUnicode (absoluteURL))
                    , externalRepository);

                result.push_back (entry);
		    }
	    }
    }
    else
    {
        error = CUnicodeUtils::GetUnicode (svnError.GetMessage());
    }
}

// auto-schedule upon construction

CRepositoryLister::CExternalsQuery::CExternalsQuery 
    ( const CTSVNPath& path
    , const SRepositoryInfo& repository
    , async::CJobScheduler* scheduler)
    : CQuery (path, repository)
{
    Schedule (false, scheduler);
}

/////////////////////////////////////////////////////////////////////
// CRepositoryLister
/////////////////////////////////////////////////////////////////////

// cleanup utilities

void CRepositoryLister::CompactDumpster()
{
    typedef std::vector<CQuery*>::iterator IT;
    IT target = dumpster.begin();
    IT end = dumpster.end();

    for (IT iter = target; iter != end; ++iter)
        if ((*iter)->GetStatus() == async::IJob::done)
            delete *iter;
        else
            *(target++) = *iter;

    dumpster.erase (target, end);
}

void CRepositoryLister::ClearDumpster()
{
    std::for_each ( dumpster.begin()
                  , dumpster.end()
				  , std::bind2nd (std::mem_fun1 (&CQuery::WaitUntilDone), false));

    CompactDumpster();
}

// parameter encoding utility

CTSVNPath CRepositoryLister::EscapeUrl (const CString& url)
{
    CStringA prepUrl = CUnicodeUtils::GetUTF8 (CTSVNPath(url).GetSVNPathString());
	prepUrl.Replace("%", "%25");
	return CTSVNPath (CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(prepUrl)));
}

// simple construction

CRepositoryLister::CRepositoryLister()
    : scheduler (32, 0, true, false)
    , fetchingExternalsEnabled (_T("Software\\TortoiseSVN\\ShowExternalsInBrowser"), TRUE)
{
}

// wait for all jobs to finish before returning

CRepositoryLister::~CRepositoryLister()
{
    Refresh();
    ClearDumpster();
}

// we probably will call \ref GetList() on that \ref url soon

void CRepositoryLister::Enqueue 
    ( const CString& url
    , const SRepositoryInfo& repository
    , bool includeExternals)
{
    CTSVNPath escapedURL = EscapeUrl (url);
    includeExternals &= (DWORD)fetchingExternalsEnabled == TRUE;

    async::CCriticalSectionLock lock (mutex);

    TPathAndRev key (escapedURL, repository.revision);
    if (queries.find (key) == queries.end())
	{
		// trim list of SVN requests

		if (scheduler.GetQueueDepth() > MAX_QUEUE_DEPTH)
		{
			// reduce the number of SVN requests to half the limit by 
			// extracting the oldest (and least probably needed) ones

			std::vector<async::IJob*> removed 
				= scheduler.RemoveJobFromQueue (MAX_QUEUE_DEPTH / 2, true);

			// we can only delete CListQuery instances
			// -> (temp.) re-add all others (most of them will be
			// terminated with the parent CListQuery and since they
			// are at the front of the queue, they get done quickly)
			
			// we must not delete queries here because they may have 
			// references between them (list query owns externals query)

			std::vector<CListQuery*> deletedQueries; 
			deletedQueries.reserve (removed.size());

			for (size_t i = 0, count = removed.size(); i < count; ++i)
			{
				CListQuery* query = dynamic_cast<CListQuery*>(removed[i]);
				if (query == NULL)
					scheduler.Schedule (removed[i], false);
				else
					deletedQueries.push_back (query);
			}

			// CListQuery* may now be deleted

			for (size_t i = 0, count = deletedQueries.size(); i < count; ++i)
			{
				// remove query from list of "valid" queries
				// (note: there might be a new query for the same key)

				CListQuery* query = deletedQueries[i];

				TPathAndRev key (query->GetPath(), query->GetRevision());
				TQueries::iterator iter = queries.find (key);
				if ((iter != queries.end()) && (iter->second == query))
					queries.erase (iter);

				// destroy query

				delete query;
			}

			// if the dumpster has not been empty before for some reason,
			// then the removed queries might already have been dumped.
			// Now, they would be dumped twice -> fix that.

			std::sort (dumpster.begin(), dumpster.end());
			std::sort (deletedQueries.begin(), deletedQueries.end());
			dumpster.erase 
				( std::set_difference 
					( dumpster.begin(), dumpster.end()
				    , deletedQueries.begin(), deletedQueries.end()
					, dumpster.begin())
				, dumpster.end());
		}

		// add the query whose result we will probably need

        queries[key] = new CListQuery ( escapedURL
                                      , repository
                                      , includeExternals
                                      , &scheduler);
	}
}

// remove all unfinished entries from the job queue

void CRepositoryLister::Cancel()
{
    async::CCriticalSectionLock lock (mutex);

    // move all unfinsiehd queries to the dumpster

    for ( TQueries::iterator iter = queries.begin(); iter != queries.end(); )
    {
        if (iter->second->GetStatus() != async::IJob::done)
        {
            iter->second->Terminate();
            dumpster.push_back (iter->second);
            iter = queries.erase (iter);
        }
        else
        {
            ++iter;
        }
    }

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

// don't return results from previous or still running requests
// the next time \ref GetList() gets called

void CRepositoryLister::Refresh()
{
    async::CCriticalSectionLock lock (mutex);

    // move all revision-specific queries to the dumpster

    for ( TQueries::iterator iter = queries.begin()
        , end = queries.end()
        ; iter != end
        ; ++iter)
    {
        iter->second->Terminate();
        dumpster.push_back (iter->second);
    }

    queries.clear();

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

// invalidate only those entries that belong to the given \ref revision

void CRepositoryLister::Refresh (const SVNRev& revision)
{
    async::CCriticalSectionLock lock (mutex);

    // move all HEAD queries for a specific revision the dumpster

    for (TQueries::iterator iter = queries.begin(); iter != queries.end(); )
    {
        if (iter->first.second == revision)
        {
            iter->second->Terminate();
            dumpster.push_back (iter->second);
            iter = queries.erase (iter);
        }
        else
        {
            ++iter;
        }
    }

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

// like \ref Refresh() but may only affect those nodes in the 
// sub-tree starting at the specified \ref url.

void CRepositoryLister::RefreshSubTree 
    ( const SVNRev& revision
    , const CString& url)
{
    CTSVNPath escapedURL = EscapeUrl (url);

    async::CCriticalSectionLock lock (mutex);

    // move all HEAD queries for a specific revision the dumpster

    for (TQueries::iterator iter = queries.begin(); iter != queries.end(); )
    {
        if (   (iter->first.second == revision)
            && (   (escapedURL == iter->first.first)
                || escapedURL.IsAncestorOf (iter->first.first)))
        {
            iter->second->Terminate();
            dumpster.push_back (iter->second);
            iter = queries.erase (iter);
        }
        else
        {
            ++iter;
        }
    }

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

CRepositoryLister::CListQuery* CRepositoryLister::FindQuery 
    ( const CString& url
    , const SRepositoryInfo& repository
	, bool includeExternals)
{
	// ensure there is a suitable query

	Enqueue (url, repository, includeExternals);

	// return that query

    TPathAndRev key (EscapeUrl (url), repository.revision);
    return queries[key];
}

CString CRepositoryLister::GetList 
    ( const CString& url
    , const SRepositoryInfo& repository
    , bool includeExternals
    , std::deque<CItem>& items)
{
    async::CCriticalSectionLock lock (mutex);

    // find that query

	CListQuery* query = FindQuery (url, repository, includeExternals);

    // wait for the results to come in and return them
	// get "ordinary" list plus direct externals 

	items = query->GetResult();
	return query->GetError();
}

CString CRepositoryLister::AddSubTreeExternals 
    ( const CString& url
    , const SRepositoryInfo& repository
    , const CString& externalsRelPath
    , std::deque<CItem>& items)
{
    async::CCriticalSectionLock lock (mutex);

    // auto-create / find that query

	CListQuery* query = FindQuery (url, repository, true);

	// add unfiltered externals?

	typedef std::deque<CItem>::const_iterator TI;
	TI begin = query->GetSubPathExternals().begin();
	TI end = query->GetSubPathExternals().end();

	if (externalsRelPath.IsEmpty())
	{
		std::copy (begin, end, std::back_inserter (items));
	}
	else
	{
		// filtered externals

		int levels = CItem::Levels (externalsRelPath);
		for (TI iter = begin; iter != end; ++iter)
			if (   (iter->external_position == levels)
				&& (iter->external_rel_path.Find (externalsRelPath) == 0))
			{
				items.push_back (*iter);
			}
	}

	return query->GetError();
}