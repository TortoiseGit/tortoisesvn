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
#pragma once

#include "TSVNPath.h"
#include "SVN.h"

#include "Registry.h"

#include "JobBase.h"
#include "JobScheduler.h"

/**
 * \ingroup TortoiseProc
 * structure that contains all information necessary to access a repository.
 */

struct SRepositoryInfo
{
    CString root;
    CString uuid;
    SVNRev revision;

    bool operator== (const SRepositoryInfo& rhs) const
    {
        return (root == rhs.root) 
            && (uuid == rhs.uuid) 
            && ((LONG)revision == (LONG)rhs.revision);
    }

    bool operator!= (const SRepositoryInfo& rhs) const
    {
        return !operator==(rhs);
    }
};

/**
 * \ingroup TortoiseProc
 * helper class which holds all the information of an item (file or folder)
 * in the repository. The information gets filled by the svn_client_list()
 * callback.
 */
class CItem
{
public:
	CItem() 
        : kind(svn_node_none)
		, size(0)
		, has_props(false)
		, is_external(false)
		, created_rev(SVN_IGNORED_REVNUM)
		, time(0)
		, is_dav_comment(false)
		, lock_creationdate(0)
		, lock_expirationdate(0)
	{
	}
	CItem 
        ( const CString& path
		, svn_node_kind_t kind
		, svn_filesize_t size
		, bool has_props
        , bool is_external
		, svn_revnum_t created_rev
		, apr_time_t time
		, const CString& author
		, const CString& locktoken
		, const CString& lockowner
		, const CString& lockcomment
		, bool is_dav_comment
		, apr_time_t lock_creationdate
		, apr_time_t lock_expirationdate
		, const CString& absolutepath
        , const SRepositoryInfo& repository)

        : path (path)
		, kind (kind)
		, size (size)
		, has_props (has_props)
        , is_external (is_external)
		, created_rev (created_rev)
		, time (time)
		, author (author)
		, locktoken (locktoken)
		, lockowner (lockowner)
		, lockcomment (lockcomment)
		, is_dav_comment (is_dav_comment)
		, lock_creationdate (lock_creationdate)
		, lock_expirationdate (lock_expirationdate)
		, absolutepath (absolutepath)
        , repository (repository)
    {
	}
public:
	CString				path;
	svn_node_kind_t		kind;
	svn_filesize_t		size;
	bool				has_props;
    bool                is_external;
	svn_revnum_t		created_rev;
	apr_time_t			time;
	CString				author;
	CString				locktoken;
	CString				lockowner;
	CString				lockcomment;
	bool				is_dav_comment;
	apr_time_t			lock_creationdate;
	apr_time_t			lock_expirationdate;
	CString				absolutepath;			
    SRepositoryInfo     repository;
};

/**
 * \ingroup TortoiseProc
 */
class CRepositoryLister
{
private:

    /**
     * \ingroup TortoiseProc
     * Encapsulates an asynchroneous, interruptible SVN request.
     */

    class CQuery : public async::CJobBase
    {
    protected:

        /// qeuery parameters

        CTSVNPath path;
        SVNRev revision;

        /// here, the result will be placed once we are done

        std::deque<CItem> result;

        /// will be empty, iff \ref result is valid

        CString error;

        /// copy copying supported

        CQuery (const CQuery&);
        CQuery& operator=(const CQuery&);

    public:

        /// auto-schedule upon construction

        CQuery ( const CTSVNPath& path
               , const SVNRev& revision);

        /// wait for termination

        virtual ~CQuery();

        /// parameter access

        const CTSVNPath& GetPath() const;
        const SVNRev& GetRevision() const;

        /// result access. Automatically waits for execution to be finished.

        const std::deque<CItem>& GetResult();
        const CString& GetError();
        bool Succeeded();
    };

    /**
     * \ingroup TortoiseProc
     * Encapsulates an asynchroneous, interruptible svn:externals request.
     */

    class CExternalsQuery : public CQuery
    {
    protected:

        /// actual job code: fetch externals and parse them

        virtual void InternalExecute();

    public:

        /// auto-schedule upon construction

        CExternalsQuery ( const CTSVNPath& path
                        , const SVNRev& revision
                        , async::CJobScheduler* scheduler);
    };

    /**
     * \ingroup TortoiseProc
     * Encapsulates an asynchroneous, interruptible SVN list request.
     */

    class CListQuery : public CQuery, private SVN
    {
    private:

        /// additional qeuery parameters

	    SRepositoryInfo repository;

        /// will be set, if includeExternals has been specified

        std::auto_ptr<CExternalsQuery> externalsQuery;

	    /// callback from the SVN::List() method which stores all the information

	    virtual BOOL ReportList(const CString& path, svn_node_kind_t kind, 
		    svn_filesize_t size, bool has_props, svn_revnum_t created_rev, 
		    apr_time_t time, const CString& author, const CString& locktoken, 
		    const CString& lockowner, const CString& lockcomment, 
		    bool is_dav_comment, apr_time_t lock_creationdate, 
		    apr_time_t lock_expirationdate, const CString& absolutepath);

        /// early termination

        virtual BOOL Cancel();

    protected:

        /// actual job code: just call \ref SVN::List

        virtual void InternalExecute();

    public:

        /// auto-schedule upon construction

        CListQuery ( const CTSVNPath& path
            	   , const SRepositoryInfo& repository
                   , bool includeExternals
                   , async::CJobScheduler* scheduler);

        /// wait for termination

        virtual ~CListQuery();

        /// cancel the svn:externals sub query as well

        virtual void Terminate();
    };

    /// folder content at specific revisions

    typedef std::pair<CTSVNPath, svn_revnum_t> TPathAndRev;
    typedef std::map<TPathAndRev, CQuery*> TQueries;

    TQueries queries;

    /// move superseeded queries here
    /// (so they can finish quietly without us waiting for them)

    std::vector<CQuery*> dumpster;

    /// sync. access

    async::CCriticalSection mutex;

    /// the job queue to execute the list requests

    async::CJobScheduler scheduler;

    /// registry settings

    CRegDWORD fetchingExternalsEnabled;

    /// cleanup utilities

    void CompactDumpster();
    void ClearDumpster();

    /// parameter encoding utility

    static CTSVNPath EscapeUrl (const CString& url);

    /// copy copying supported

    CRepositoryLister (const CRepositoryLister&);
    CRepositoryLister& operator=(const CRepositoryLister&);

public:

    /// simple construction

    CRepositoryLister();

    /// wait for all jobs to finish before returning

    ~CRepositoryLister();

    /// we probably will call \ref GetList() on that \ref url soon.
    /// \ref includeExternals will only be taken into account if
    /// there is no query for that \ref url and resivision yet.
    /// It should be set to @a false only if it is certain that
    /// no externals have been defined for that URL and revision.

    void Enqueue ( const CString& url
                 , const SRepositoryInfo& repository
                 , bool includeExternals);

    /// remove all unfinished entries from the job queue

    void Cancel();

    /// don't return results from previous or still running requests
    /// the next time \ref GetList() gets called

    void Refresh();

    /// invalidate only those entries that belong to the given \ref revision

    void Refresh (const SVNRev& revision);

    /// like \ref Refresh() but may only affect those nodes in the 
    /// sub-tree starting at the specified \ref url.

    void RefreshSubTree (const SVNRev& revision, const CString& url);

    /// get an already stored query result, if available.
    /// Otherwise, get the list directly.
    /// \ref includeExternals will be ignored, if there is already
    /// a query running or result available.
    /// It should be set to @a false only if it is certain that
    /// no externals have been defined for that URL and revision.
    /// \returns the error or an empty string

    CString GetList ( const CString& url
                    , const SRepositoryInfo& repository
                    , bool includeExternals
                    , std::deque<CItem>& items);
};

