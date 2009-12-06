// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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

/////////////////////////////////////////////////////////////////////
// CRepositoryLister::CQuery
/////////////////////////////////////////////////////////////////////

// auto-schedule upon construction

CRepositoryLister::CQuery::CQuery 
    ( const CTSVNPath& path
    , const SVNRev& revision)
    : path (path)
    , revision (revision)
{
}

// wait for termination

CRepositoryLister::CQuery::~CQuery()
{
    Terminate();
    WaitUntilDone();
}

// parameter access

const CTSVNPath& CRepositoryLister::CQuery::GetPath() const
{
    return path;
}

const SVNRev& CRepositoryLister::CQuery::GetRevision() const
{
    return revision;
}

// result access. Automatically waits for execution to be finished.

const std::deque<CItem>& CRepositoryLister::CQuery::GetResult()
{
    WaitUntilDone();
    return result;
}

const CString& CRepositoryLister::CQuery::GetError()
{
    WaitUntilDone();

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
        , kind
        , size
        , has_props
        , false
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
    if (!List (path, revision, revision, svn_depth_immediates, true))
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

                std::copy ( externals.begin()
                          , externals.end()
                          , std::back_inserter (result));
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
    : CQuery (path, repository.revision)
    , repository (repository)
    , externalsQuery 
        (includeExternals
            ? new CExternalsQuery (path, repository.revision, scheduler)
            : NULL)
{
    Schedule (false, scheduler);
}

// wait for termination

CRepositoryLister::CListQuery::~CListQuery()
{
}

// cancel the svn:externals sub query as well

void CRepositoryLister::CListQuery::Terminate()
{
    CQuery::Terminate();

    if (externalsQuery.get() != NULL)
        externalsQuery->Terminate();
}

/////////////////////////////////////////////////////////////////////
// CRepositoryLister::CExternalsQuery
/////////////////////////////////////////////////////////////////////

// actual job code: fetch externals and parse them

void CRepositoryLister::CExternalsQuery::InternalExecute()
{
    // fetch externals definition for the given path

    static const std::string svnExternals (SVN_PROP_EXTERNALS);

    SVNProperties properties (path, revision, false);

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

                SRepositoryInfo repository;

                CTSVNPath url;
                url.SetFromSVN (external->url);
                repository.root 
                    = svn.GetRepositoryRootAndUUID (url, repository.uuid);

                repository.revision = external->revision;

                // add the new entry

                CItem entry
                    ( CUnicodeUtils::GetUnicode (external->target_dir)

                      // even file external will be treated as folders because
                      // this is a safe default
                    , svn_node_dir
                    , 0

                      // actually, we don't know for sure whether the target
                      // URL has props but its the safe default to say 'yes'
                    , true  
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
                        (CUnicodeUtils::GetUnicode (external->url))
                    , repository);

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
    , const SVNRev& revision
    , async::CJobScheduler* scheduler)
    : CQuery (path, revision)
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
                  , std::mem_fun (&CQuery::WaitUntilDone));

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
    : scheduler (32, 0, true, true)
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
        queries[key] = new CListQuery ( escapedURL
                                      , repository
                                      , includeExternals
                                      , &scheduler);
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

// get an already stored query result, if available.
// Otherwise, get the list directly

CString CRepositoryLister::GetList 
    ( const CString& url
    , const SRepositoryInfo& repository
    , bool includeExternals
    , std::deque<CItem>& items)
{
    CTSVNPath escapedURL = EscapeUrl (url);
    includeExternals &= (DWORD)fetchingExternalsEnabled == TRUE;

    async::CCriticalSectionLock lock (mutex);

    // lets see if there is already a suitable query that will
    // finish before the one that I could start right now

    TPathAndRev key (escapedURL, repository.revision);
    TQueries::iterator iter = queries.find (key);

    if (   (iter != queries.end()) 
        && (iter->second->GetStatus() != async::IJob::waiting))
    {
        items = iter->second->GetResult();
        return iter->second->GetError();
    }

    // query (quasi-)synchronously using the default queue

    CListQuery query (escapedURL, repository, includeExternals, NULL);

    items = query.GetResult();
    return query.GetError();
}

