// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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

// callback from the SVN::List() method which stores all the information

BOOL CRepositoryLister::CQuery::ReportList
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
	int slashpos = path.ReverseFind('/');
	bool abspath_has_slash = (absolutepath.GetAt(absolutepath.GetLength()-1) == '/');

    CItem entry
        ( path.Mid (slashpos+1)
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
        , repoRoot + absolutepath + (abspath_has_slash ? _T("") : _T("/")) + path);

    result.push_back (entry);

    return TRUE;
}

// early termination

BOOL CRepositoryLister::CQuery::Cancel()
{
    return cancelled;
}

// actual job code: just call \ref SVN::List

void CRepositoryLister::CQuery::InternalExecute()
{
    if (!cancelled)
	    if (!List (path, revision, revision, svn_depth_immediates, true))
            result.clear();
}

// auto-schedule upon construction

CRepositoryLister::CQuery::CQuery 
    ( const CTSVNPath& path
    , const CString& repoRoot
    , const SVNRev& revision
    , async::CJobScheduler* scheduler)
    : path (path)
    , repoRoot (repoRoot)
    , revision (revision)
    , cancelled (false)
{
    Schedule (false, scheduler);
}

// wait for termination

CRepositoryLister::CQuery::~CQuery()
{
    Terminate();
    WaitUntilDone();
}

// trigger cancellation

void CRepositoryLister::CQuery::Terminate()
{
    cancelled = true;
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

// simple construction

CRepositoryLister::CRepositoryLister()
    : scheduler (8, 0, true)
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
    ( const CTSVNPath& url
    , const CString& repoRoot
    , const SVNRev& revision)
{
    async::CCriticalSectionLock lock (mutex);

    if (revision.IsHead())
    {
        if (queryByPath.find (url) == queryByPath.end())
            queryByPath[url] 
                = new CQuery (url, repoRoot, revision, &scheduler);
    }
    else
    {
        TPathAndRev key (url, revision);
        if (queryByPathAndRev.find (key) == queryByPathAndRev.end())
            queryByPathAndRev[key]
                = new CQuery (url, repoRoot, revision, &scheduler);
    }
}

// don't return results from previous or still running requests
// the next time \ref GetList() gets called

void CRepositoryLister::Refresh()
{
    async::CCriticalSectionLock lock (mutex);

    // move all revision-specific queries to the dumpster

    for ( TQueryByPathAndRev::iterator iter = queryByPathAndRev.begin()
        , end = queryByPathAndRev.end()
        ; iter != end
        ; ++iter)
    {
        iter->second->Terminate();
        dumpster.push_back (iter->second);
    }

    queryByPathAndRev.clear();

    // move all HEAD queries to the dumpster & clear it

    Refresh (SVNRev::REV_HEAD);
}

// invalidate only those entries that belong to the given \ref revision

void CRepositoryLister::Refresh (const SVNRev& revision)
{
    async::CCriticalSectionLock lock (mutex);

    if (revision.IsHead())
    {
        // move all HEAD queries to the dumpster

        for ( TQueryByPath::iterator iter = queryByPath.begin()
            , end = queryByPath.end()
            ; iter != end
            ; ++iter)
        {
            iter->second->Terminate();
            dumpster.push_back (iter->second);
        }

        queryByPath.clear();
    }
    else
    {
        // move all HEAD queries for a specific revision the dumpster

        for ( TQueryByPathAndRev::iterator iter = queryByPathAndRev.begin()
            ; iter != queryByPathAndRev.end()
            ; )
        {
            if (iter->first.second == revision)
            {
                iter->second->Terminate();
                dumpster.push_back (iter->second);
                iter = queryByPathAndRev.erase (iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

// like \ref Refresh() but may only affect those nodes in the 
// sub-tree starting at the specified \ref url.

void CRepositoryLister::RefreshSubTree 
    ( const SVNRev& revision
    , const CTSVNPath& url)
{
    async::CCriticalSectionLock lock (mutex);

    if (revision.IsHead())
    {
        // move all HEAD queries to the dumpster

        for ( TQueryByPath::iterator iter = queryByPath.begin()
            , end = queryByPath.end()
            ; iter != end
            ; ++iter)
        {
            if ((url == iter->first) || url.IsAncestorOf (iter->first))
            {
                iter->second->Terminate();
                dumpster.push_back (iter->second);
                iter = queryByPath.erase (iter);
            }
            else
            {
                ++iter;
            }
        }

        queryByPath.clear();
    }
    else
    {
        // move all HEAD queries for a specific revision the dumpster

        for ( TQueryByPathAndRev::iterator iter = queryByPathAndRev.begin()
            ; iter != queryByPathAndRev.end()
            ; )
        {
            if (   (iter->first.second == revision)
                && (   (url == iter->first.first)
                    || url.IsAncestorOf (iter->first.first)))
            {
                iter->second->Terminate();
                dumpster.push_back (iter->second);
                iter = queryByPathAndRev.erase (iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    // finally destroy all entries that have already been processed

    CompactDumpster();
}

// get an already stored query result, if available.
// Otherwise, get the list directly

void CRepositoryLister::GetList 
    ( const CTSVNPath& url
    , const CString& repoRoot
    , const SVNRev& revision
    , std::deque<CItem>& items)
{
    async::CCriticalSectionLock lock (mutex);

    // lets see if there is already a suitable query that will
    // finish before the one that I could start right now

    if (revision.IsHead())
    {
        TQueryByPath::iterator iter = queryByPath.find (url);
        if (   (iter != queryByPath.end()) 
            && (iter->second->GetStatus() != async::IJob::waiting))
        {
            items = iter->second->GetResult();
            return;
        }
    }
    else
    {
        TPathAndRev key (url, revision);
        TQueryByPathAndRev::iterator iter = queryByPathAndRev.find (key);

        if (   (iter != queryByPathAndRev.end()) 
            && (iter->second->GetStatus() != async::IJob::waiting))
        {
            items = iter->second->GetResult();
            return;
        }
    }

    // query (quasi-)synchronously using the default queue

    items = CQuery (url, repoRoot, revision, NULL).GetResult();
}

