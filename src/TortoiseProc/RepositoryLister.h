// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2015 - TortoiseSVN

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

#include "registry.h"

#include "JobBase.h"
#include "JobScheduler.h"
#include "SchedulerSuspension.h"

/**
 * \ingroup TortoiseProc
 * structure that contains all information necessary to access a repository.
 */

struct SRepositoryInfo
{
    CString root;
    CString uuid;
    SVNRev revision;
    SVNRev peg_revision;

    bool operator== (const SRepositoryInfo& rhs) const
    {
        return (root == rhs.root)
            && (uuid == rhs.uuid)
            && (revision.IsEqual (rhs.revision))
            && (peg_revision.IsEqual (rhs.peg_revision));
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
        , external_position(-1)
        , created_rev(SVN_IGNORED_REVNUM)
        , time(0)
        , is_dav_comment(false)
        , lock_creationdate(0)
        , lock_expirationdate(0)
        , unversioned(false)
    {
    }
    CItem
        ( const CString& path
        , const CString& external_rel_path
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
        , const CString& absolutepath
        , const SRepositoryInfo& repository)

        : path (path)
        , external_rel_path (external_rel_path)
        , kind (kind)
        , size (size)
        , has_props (has_props)
        , is_external (!external_rel_path.IsEmpty())
        , external_position (is_external ? Levels (external_rel_path) : -1)
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
        , unversioned(false)
    {
    }

    static int Levels (const CString& relPath)
    {
        LPCTSTR start = relPath;
        LPCTSTR end = start + relPath.GetLength();
        return static_cast<int>(std::count (start, end, '/'));
    }

public:
    CString             path;
    CString             external_rel_path;
    svn_node_kind_t     kind;
    svn_filesize_t      size;
    bool                has_props;
    bool                is_external;
    bool                unversioned;

    /// number of levels up the local path hierarchy to find the external spec.
    /// -1, if this is not an external
    int                 external_position;
    svn_revnum_t        created_rev;
    apr_time_t          time;
    CString             author;
    CString             locktoken;
    CString             lockowner;
    CString             lockcomment;
    bool                is_dav_comment;
    apr_time_t          lock_creationdate;
    apr_time_t          lock_expirationdate;
    CString             absolutepath;
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
        apr_uint32_t dirent;
        SVNRev pegRevision;

        /// additional qeuery parameters

        SRepositoryInfo repository;

        /// here, the result will be placed once we are done

        std::deque<CItem> result;

        /// will be empty, iff \ref result is valid

        CString error;

        /// in case of a redirect

        CString redirectedUrl;

        /// copy copying supported

        CQuery (const CQuery&);
        CQuery& operator=(const CQuery&);

        /// not meant to be destroyed directly

        virtual ~CQuery() {};

    public:

        /// auto-schedule upon construction

        CQuery ( const CTSVNPath& path
               , const SVNRev& pegRevision
               , apr_uint32_t dirent
               , const SRepositoryInfo& repository);

        /// parameter access

        const CTSVNPath& GetPath() const;
        const SVNRev& GetRevision() const;
        const SVNRev& GetPegRevision() const;

        /// result access. Automatically waits for execution to be finished.

        const std::deque<CItem>& GetResult();
        const CString& GetError();
        const CString& GetRedirectedUrl() {return redirectedUrl;}
        bool Succeeded();
    };

    /**
     * \ingroup TortoiseProc
     * Encapsulates an asynchroneous, interruptible svn:externals request.
     */

    class CExternalsQuery : public CQuery
    {
    private:

        /// if set, suppress all user prompts

        bool runSilently;

    protected:

        /// actual job code: fetch externals and parse them

        virtual void InternalExecute() override;

        /// not meant to be destroyed directly

        virtual ~CExternalsQuery() {};

    public:

        /// auto-schedule upon construction

        CExternalsQuery ( const CTSVNPath& path
                        , const SVNRev& pegRevision
                        , const SRepositoryInfo& repository
                        , bool runSilently
                        , async::CJobScheduler* scheduler);
    };

    /**
     * \ingroup TortoiseProc
     * Encapsulates an asynchroneous, interruptible SVN list request.
     */

    class CListQuery : public CQuery, private SVN
    {
    private:

        /// will be set, if includeExternals has been specified

        CExternalsQuery* externalsQuery;

        /// externals that apply to some sub-tree found while filling result

        std::deque<CItem> subPathExternals;

        /// callback from the SVN::List() method which stores all the information

        virtual BOOL ReportList(const CString& path_, svn_node_kind_t kind,
            svn_filesize_t size, bool has_props, svn_revnum_t created_rev,
            apr_time_t time, const CString& author, const CString& locktoken,
            const CString& lockowner, const CString& lockcomment,
            bool is_dav_comment, apr_time_t lock_creationdate,
            apr_time_t lock_expirationdate, const CString& absolutepath,
            const CString& externalParentUrl, const CString& externalTarget) override;

        /// early termination

        virtual BOOL Cancel() override;

    protected:

        /// actual job code: just call \ref SVN::List

        virtual void InternalExecute() override;

        /// not meant to be destroyed directly

        virtual ~CListQuery();

    public:

        /// auto-schedule upon construction

        CListQuery ( const CTSVNPath& path
                   , const SVNRev& pegRevision
                   , const SRepositoryInfo& repository
                   , apr_uint32_t dirent
                   , bool includeExternals
                   , bool runSilently
                   , async::CJobScheduler* scheduler);

        /// cancel the svn:externals sub query as well

        virtual void Terminate() override;

        /// access additional results

        const std::deque<CItem>& GetSubPathExternals();

        /// true, if this query has been run silently and
        /// some error occurred.

        bool ShouldBeRerun();
    };

    /// folder content at specific revisions

    struct SPathAndRev
    {
        CTSVNPath path;
        svn_revnum_t rev;
        svn_revnum_t peg_rev;

        SPathAndRev ( const CTSVNPath& path
                    , svn_revnum_t peg_rev
                    , svn_revnum_t rev)
            : path (path)
            , rev (rev)
            , peg_rev (peg_rev == SVNRev::REV_UNSPECIFIED ? rev : peg_rev)
        {
        }

        bool operator<(const SPathAndRev& rhs) const
        {
            int pathDiff = CTSVNPath::CompareWithCase (path, rhs.path);
            return (pathDiff < 0)
                || (   (pathDiff == 0)
                    && (   (rev < rhs.rev)
                        || (   (rev == rhs.rev)
                            && (peg_rev < rhs.peg_rev))));
        }
    };

    typedef std::map<SPathAndRev, CListQuery*> TQueries;

    TQueries queries;

    /// maximum number of outstanding SVN requests.
    /// The higher this number is, the longer we will
    /// keep the server under load.

    enum { MAX_QUEUE_DEPTH = 100 };

    /// move superseded queries here
    /// (so they can finish quietly without us waiting for them)

    std::vector<CQuery*> dumpster;

    /// sync. access

    async::CCriticalSection mutex;

    /// the job queue to execute the list requests

    async::CJobScheduler scheduler;

    /// cleanup utilities

    void CompactDumpster();
    void ClearDumpster();

    /// parameter encoding utility

    static CTSVNPath EscapeUrl (const CString& url);

    /// data lookup utility:
    /// auto-insert & return query

    CListQuery* FindQuery
        ( const CString& url
        , const SVNRev& pegRev
        , const SRepositoryInfo& repository
        , apr_uint32_t dirent
        , bool includeExternals);

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
    /// there is no query for that \ref url and revision yet.
    /// It should be set to @a false only if it is certain that
    /// no externals have been defined for that URL and revision.
    /// All queries that shall be run in background without any
    /// user interaction must set @a runSilently.

    void Enqueue ( const CString& url
                 , const SVNRev& pegRev
                 , const SRepositoryInfo& repository
                 , apr_uint32_t dirent
                 , bool includeExternals
                 , bool runSilently = true);

    /// remove all unfinished entries from the job queue

    void Cancel();

    /// wait for all jobs to be finished

    void WaitForJobsToFinish();

    /// return a RAII object that suspends job scheduling for its
    /// lifetime (jobs already being processed will not be affected)

    std::unique_ptr<async::CSchedulerSuspension> SuspendJobs();

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
    /// \ref treePath is the relative path from the repository root
    /// dir to the given URL as shown in the repo browser tree,
    /// including all intermittend externals (i.e. the rel. path
    /// in a w/c if we checked out at the root). If this is empty
    /// all externals will be reported as immediate children.
    /// \ref includeExternals will be ignored, if there is already
    /// a query running or result available.
    /// It should be set to @a false only if it is certain that
    /// no externals have been defined for that URL and revision.
    /// \returns the error or an empty string

    CString GetList ( const CString& url
                    , const SVNRev& pegRev
                    , const SRepositoryInfo& repository
                    , apr_uint32_t dirent
                    , bool includeExternals
                    , std::deque<CItem>& items
                    , CString& redirUrl);

    /// get an already stored query result, if available.
    /// Otherwise, get the list directly.
    /// \ref externalsRelPath must be set to the sub-tree path
    /// for which we want to find externals. If this is empty
    /// all externals will be reported as immediate children.
    /// \returns the error or an empty string

    CString AddSubTreeExternals
        ( const CString& url
        , const SVNRev& pegRev
        , const SRepositoryInfo& repository
        , const CString& externalsRelPath
        , std::deque<CItem>& items);
};

