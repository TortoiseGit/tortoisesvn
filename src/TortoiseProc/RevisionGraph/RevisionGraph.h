// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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

#include "SVN.h"
#include "SVNPrompt.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"
#include "PathClassificator.h"
#include "SearchPathTree.h"

#include <map>

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/**
 * \ingroup TortoiseProc
 * helper class which handles one log entry used in the revision graph
 */
class log_entry
{
public:

	std::auto_ptr<LogChangedPathArray> changes;
	svn_revnum_t rev;
	CString author;
	apr_time_t timeStamp;
	CString message;

	// default construction

	log_entry() {};

private:

	// copy is not allowed (due to std::auto_ptr)

	log_entry (const log_entry&);
	log_entry& operator= (const log_entry&);
};

class CRevisionEntry;

/**
 * \ingroup TortoiseProc
 * helper struct containing information about a copy operation in the revision graph
 */
struct SCopyInfo
{
	revision_t fromRevision;
	index_t fromPathIndex;
	revision_t toRevision;
	index_t toPathIndex;

	struct STarget
	{
		CRevisionEntry* source;
		CDictionaryBasedTempPath path;

		STarget ( CRevisionEntry* source
				, const CDictionaryBasedTempPath& path)
			: source (source)
			, path (path)
		{
		}
	};

	std::vector<STarget> targets;
};

/**
 * \ingroup TortoiseProc
 * Handles and analyzes log data to produce a revision graph.
 * 
 * Since Subversion only stores information where each entry is copied \b from
 * and not where it is copied \b to, the first thing we do here is crawl all
 * revisions and create separate CRevisionEntry objects where we store the
 * information where those are copied \b to.
 *
 * In a next step, we go again through all the CRevisionEntry objects to find
 * out if they are related to the path we're looking at. If they are, we mark
 * them as \b in-use.
 */
class CRevisionGraph : private ILogReceiver
{
public:

    struct SOptions
    {
        bool groupBranches;         // one row per revision, if false
        bool includeSubPathChanges; // "show all"
        bool oldestAtTop;           // start with latest revision (not first / oldest revision)
        bool showHEAD;              // show HEAD change for all branches
        bool exactCopySources;      // create a copy-source node, even if there was no change in that revision
        bool foldTags;              // show tags as property to the source - not as separate nodes
        bool reduceCrossLines;      // minimize places with lines crossing a node box
        bool removeDeletedOnes;     // remove all deleted branches and tags
        bool showWCRev;				// highlight the current WC revision if path points to a WC
    };

	CRevisionGraph(void);
	~CRevisionGraph(void);
	BOOL						FetchRevisionData(CString path, const SOptions& options);
	void						AnalyzeRevisionData(CString path, const SOptions& options);
	virtual BOOL				ProgressCallback(CString text1, CString text2, DWORD done, DWORD total);
	svn_revnum_t				GetHeadRevision() {return m_lHeadRevision;}
	bool						SetFilter(svn_revnum_t minrev, svn_revnum_t maxrev, const CString& sPathFilter);

	CString						GetLastErrorMessage();
	std::vector<CRevisionEntry*> m_entryPtrs;
	size_t						m_maxurllength;
	CString						m_maxurl;
	int							m_maxColumn;
	int							m_maxRow;

	std::auto_ptr<CSVNLogQuery> svnQuery;
	std::auto_ptr<CCacheLogQuery> query;
    std::auto_ptr<CPathClassificator> pathClassification;

    CString				        GetReposRoot();

	BOOL						m_bCancelled;

	apr_pool_t *				pool;			///< memory pool
	svn_client_ctx_t 			m_ctx;
	SVNPrompt					m_prompt;
	SVN							svn;

private:

    CRevisionEntry::TPool       nodePool;

	typedef std::vector<SCopyInfo*>::const_iterator TSCopyIterator;

	void						BuildForwardCopies();
    void                        InsertWCRevision (const CDictionaryBasedTempPath& wcPath);
	void						AnalyzeRevisions ( const CDictionaryBasedTempPath& url
												 , revision_t startrev
												 , const SOptions& options);
	void						AnalyzeRevisions ( revision_t revision
												 , CRevisionInfoContainer::CChangesIterator first
												 , CRevisionInfoContainer::CChangesIterator last
												 , CSearchPathTree* startNode
												 , bool bShowAll
												 , std::vector<CSearchPathTree*>& toRemove);
	void						AnalyzeChangesOnly ( revision_t revision
												   , CRevisionInfoContainer::CChangesIterator first
												   , CRevisionInfoContainer::CChangesIterator last
												   , CSearchPathTree* startNode);
	void						AddCopiedPaths ( revision_t revision
											   , CSearchPathTree* rootNode
											   , TSCopyIterator& lastToCopy);
	void						FillCopyTargets ( revision_t revision
											    , CSearchPathTree* rootNode
											    , TSCopyIterator& lastFromCopy
                                                , bool exactCopy);
    bool                        IsLatestCopySource ( const CCachedLogInfo* cache
                                                   , revision_t fromRevision
                                                   , revision_t toRevision
                                                   , const CDictionaryBasedPath& fromPath
                                                   , const CDictionaryBasedTempPath& currentPath);
    void                        AddMissingHeads (CSearchPathTree* rootNode);
    void                        MarkHeads (CSearchPathTree* rootNode);
    void                        AnalyzeHeadRevision ( revision_t revision
    									            , CRevisionInfoContainer::CChangesIterator first
	    								            , CRevisionInfoContainer::CChangesIterator last
		    							            , CSearchPathTree* searchNode
				    					            , std::vector<CSearchPathTree*>& toRemove);
	void						AssignColumns ( CRevisionEntry* start
											  , std::vector<int>& columnByRow
                                              , int column
											  , const SOptions& options);

    void                        FindReplacements();
    void                        ForwardClassification();
    void                        BackwardClassification (const SOptions& options);
    void                        RemoveDeletedOnes();
    void                        FoldTags ( CRevisionEntry * collectorNode
                                         , CRevisionEntry * entry
                                         , size_t depth);
    void                        FoldTags();
	void						ApplyFilter();
    void                        Compact();
	void						Optimize (const SOptions& options);

	int							AssignOneRowPerRevision();
	int							AssignOneRowPerBranchNode (CRevisionEntry* start, int row);
    void                        ReverseRowOrder (int maxRow);
	void						AssignCoordinates (const SOptions& options);
	void						Cleanup();
	void						ClearRevisionEntries();
	
	CStringA					m_sRepoRoot;
	revision_t					m_lHeadRevision;
	CStringA					m_wcURL;
	svn_revnum_t				m_wcRev;

	std::set<std::string>		m_filterpaths;
	svn_revnum_t				m_FilterMinRev;
	svn_revnum_t				m_FilterMaxRev;

	int							m_nRecurseLevel;
	svn_error_t *				Err;			///< Global error object struct
	apr_pool_t *				parentpool;
	static svn_error_t*			cancel(void *baton);

	std::vector<SCopyInfo>		copiesContainer;
	std::vector<SCopyInfo*>		copyToRelation;
	std::vector<SCopyInfo*>		copyFromRelation;

	// implement ILogReceiver

	void ReceiveLog ( LogChangedPathArray* changes
					, svn_revnum_t rev
                    , const StandardRevProps* stdRevProps
                    , UserRevPropArray* userRevProps
                    , bool mergesFollow);

};
