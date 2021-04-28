// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2015, 2020-2021 - TortoiseSVN

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
    SVNRev  revision;
    SVNRev  pegRevision;
    bool    isSVNParentPath = false;

    bool operator==(const SRepositoryInfo& rhs) const
    {
        return (root == rhs.root) && (uuid == rhs.uuid) && (revision.IsEqual(rhs.revision)) && (pegRevision.IsEqual(rhs.pegRevision)) && isSVNParentPath == rhs.isSVNParentPath;
    }

    bool operator!=(const SRepositoryInfo& rhs) const
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
        : m_kind(svn_node_none)
        , m_size(0)
        , m_hasProps(false)
        , m_isExternal(false)
        , m_unversioned(false)
        , m_externalPosition(-1)
        , m_createdRev(SVN_IGNORED_REVNUM)
        , m_time(0)
        , m_isDavComment(false)
        , m_lockCreationDate(0)
        , lockExpirationDate(0)
    {
    }
    CItem(const CString&         path,
          const CString&         externalRelPath,
          svn_node_kind_t        kind,
          svn_filesize_t         size,
          bool                   hasProps,
          svn_revnum_t           createdRev,
          apr_time_t             time,
          const CString&         author,
          const CString&         lockToken,
          const CString&         lockOwner,
          const CString&         lockComment,
          bool                   isDavComment,
          apr_time_t             lockCreationDate,
          apr_time_t             lockExpirationDate,
          const CString&         absolutePath,
          const SRepositoryInfo& repository)

        : m_path(path)
        , m_externalRelPath(externalRelPath)
        , m_kind(kind)
        , m_size(size)
        , m_hasProps(hasProps)
        , m_isExternal(!externalRelPath.IsEmpty())
        , m_unversioned(false)
        , m_externalPosition(m_isExternal ? Levels(externalRelPath) : -1)
        , m_createdRev(createdRev)
        , m_time(time)
        , m_author(author)
        , m_lockToken(lockToken)
        , m_lockOwner(lockOwner)
        , m_lockComment(lockComment)
        , m_isDavComment(isDavComment)
        , m_lockCreationDate(lockCreationDate)
        , lockExpirationDate(lockExpirationDate)
        , m_absolutePath(absolutePath)
        , m_repository(repository)
    {
    }

    static int Levels(const CString& relPath)
    {
        LPCWSTR start = relPath;
        LPCWSTR end   = start + relPath.GetLength();
        return static_cast<int>(std::count(start, end, '/'));
    }

public:
    CString         m_path;
    CString         m_externalRelPath;
    svn_node_kind_t m_kind;
    svn_filesize_t  m_size;
    bool            m_hasProps;
    bool            m_isExternal;
    bool            m_unversioned;

    /// number of levels up the local path hierarchy to find the external spec.
    /// -1, if this is not an external
    int             m_externalPosition;
    svn_revnum_t    m_createdRev;
    apr_time_t      m_time;
    CString         m_author;
    CString         m_lockToken;
    CString         m_lockOwner;
    CString         m_lockComment;
    bool            m_isDavComment;
    apr_time_t      m_lockCreationDate;
    apr_time_t      lockExpirationDate;
    CString         m_absolutePath;
    SRepositoryInfo m_repository;
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

        CTSVNPath    path;
        apr_uint32_t dirent;
        SVNRev       pegRevision;

        /// additional qeuery parameters

        SRepositoryInfo repository;

        /// here, the result will be placed once we are done

        std::deque<CItem> result;

        /// will be empty, iff \ref result is valid

        CString error;

        /// in case of a redirect

        CString redirectedUrl;

        /// copy copying supported

        CQuery(const CQuery&) = delete;
        CQuery& operator=(const CQuery&) = delete;

        /// not meant to be destroyed directly

        ~CQuery() override{};

    public:
        /// auto-schedule upon construction

        CQuery(const CTSVNPath& path, const SVNRev& pegRevision, apr_uint32_t dirent, const SRepositoryInfo& repository);

        /// parameter access

        const CTSVNPath& GetPath() const;
        const SVNRev&    GetRevision() const;
        const SVNRev&    GetPegRevision() const;

        /// result access. Automatically waits for execution to be finished.

        const std::deque<CItem>& GetResult();
        const CString&           GetError();
        const CString&           GetRedirectedUrl() const { return redirectedUrl; }
        bool                     Succeeded();
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

        void InternalExecute() override;

        /// not meant to be destroyed directly

        ~CExternalsQuery() override{};

    public:
        /// auto-schedule upon construction

        CExternalsQuery(const CTSVNPath&       path,
                        const SVNRev&          pegRevision,
                        const SRepositoryInfo& repository,
                        bool                   runSilently,
                        async::CJobScheduler*  scheduler);
    };

    /**
     * \ingroup TortoiseProc
     * Encapsulates an asynchroneous, interruptible SVN list request.
     */

    class CListQuery : public CQuery
        , private SVN
    {
    private:
        /// will be set, if includeExternals has been specified

        CExternalsQuery* externalsQuery;

        /// externals that apply to some sub-tree found while filling result

        std::deque<CItem> subPathExternals;

        /// callback from the SVN::List() method which stores all the information

        BOOL ReportList(const CString& listPath, svn_node_kind_t kind,
                        svn_filesize_t size, bool hasProps, svn_revnum_t createdRev,
                        apr_time_t time, const CString& author, const CString& lockToken,
                        const CString& lockowner, const CString& lockcomment,
                        bool isDavComment, apr_time_t lockCreationDate,
                        apr_time_t lockExpirationDate, const CString& absolutePath,
                        const CString& externalParentUrl, const CString& externalTarget) override;

        /// early termination

        BOOL Cancel() override;

    protected:
        /// actual job code: just call \ref SVN::List

        void InternalExecute() override;

        /// not meant to be destroyed directly

        ~CListQuery() override;

    public:
        /// auto-schedule upon construction

        CListQuery(const CTSVNPath&       path,
                   const SVNRev&          pegRevision,
                   const SRepositoryInfo& repository,
                   apr_uint32_t           dirent,
                   bool                   includeExternals,
                   bool                   runSilently,
                   async::CJobScheduler*  scheduler);

        /// cancel the svn:externals sub query as well

        void Terminate() override;

        /// access additional results

        const std::deque<CItem>& GetSubPathExternals();

        /// true, if this query has been run silently and
        /// some error occurred.

        bool ShouldBeRerun();
    };

    /// folder content at specific revisions

    struct SPathAndRev
    {
        CTSVNPath    path;
        svn_revnum_t rev;
        svn_revnum_t pegRev;

        SPathAndRev(const CTSVNPath& path, svn_revnum_t pegrev, svn_revnum_t rev)
            : path(path)
            , rev(rev)
            , pegRev(pegrev == SVNRev::REV_UNSPECIFIED ? rev : pegrev)
        {
        }

        bool operator<(const SPathAndRev& rhs) const
        {
            int pathDiff = CTSVNPath::CompareWithCase(path, rhs.path);
            return (pathDiff < 0) || ((pathDiff == 0) && ((rev < rhs.rev) || ((rev == rhs.rev) && (pegRev < rhs.pegRev))));
        }
    };

    std::map<SPathAndRev, CListQuery*> queries;

    /// maximum number of outstanding SVN requests.
    /// The higher this number is, the longer we will
    /// keep the server under load.

    enum
    {
        MAX_QUEUE_DEPTH = 100
    };

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

    static CTSVNPath EscapeUrl(const CString& url);

    /// data lookup utility:
    /// auto-insert & return query

    CListQuery* FindQuery(const CString& url, const SVNRev& pegRev, const SRepositoryInfo& repository, apr_uint32_t dirent, bool includeExternals);

    /// copy copying supported

    CRepositoryLister(const CRepositoryLister&) = delete;
    CRepositoryLister& operator=(const CRepositoryLister&) = delete;

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

    void Enqueue(const CString&         url,
                 const SVNRev&          pegRev,
                 const SRepositoryInfo& repository,
                 apr_uint32_t           dirent,
                 bool                   includeExternals,
                 bool                   runSilently = true);

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

    void Refresh(const SVNRev& revision);

    /// like \ref Refresh() but may only affect those nodes in the
    /// sub-tree starting at the specified \ref url.

    void RefreshSubTree(const SVNRev& revision, const CString& url);

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

    CString GetList(const CString& url, const SVNRev& pegRev, const SRepositoryInfo& repository, apr_uint32_t dirent, bool includeExternals, std::deque<CItem>& items, CString& redirUrl);

    /// get an already stored query result, if available.
    /// Otherwise, get the list directly.
    /// \ref externalsRelPath must be set to the sub-tree path
    /// for which we want to find externals. If this is empty
    /// all externals will be reported as immediate children.
    /// \returns the error or an empty string

    CString AddSubTreeExternals(const CString& url, const SVNRev& pegRev, const SRepositoryInfo& repository, const CString& externalsRelPath, std::deque<CItem>& items);
};
