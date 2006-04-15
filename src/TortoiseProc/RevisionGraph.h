// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once

#include "SVNPrompt.h"
#include <map>

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

struct log_entry
{
	apr_hash_t* ch_paths;
	svn_revnum_t rev;
	const char* author; 
	apr_time_t time;
	const char* msg; 
};

struct source_entry
{
	const char *	pathto;
	LONG			revisionto;
};

/**
 * \ingroup TortoiseProc
 * Helper class, representing a revision with all the required information
 * which we need to draw a revision graph.
 */
class CRevisionEntry
{
public:
	enum Action
	{
		nothing,
		modified,
		deleted,
		added,
		addedwithhistory,
		renamed,
		replaced,
		lastcommit,
		initial,
		source
	};
	//methods
	CRevisionEntry(void) : revision(0), url(NULL), realurl(NULL), author(NULL), date(0),
		message(NULL), action(nothing), level(1), bUsed(false),
		leftconnections(0),	rightconnections(0), bottomconnections(0),
		rightlines(0), bottomlines(0),
		leftconnectionsleft(0),	rightconnectionsleft(0), bottomconnectionsleft(0),
		rightlinesleft(0), bottomlinesleft(0) {};
	//members
	svn_revnum_t	revision;
	const char *	url;
	const char *	realurl;
	const char *	author;
	apr_time_t		date;
	const char *	message;
	Action			action;
	int				level;
	CPtrArray		sourcearray;
	bool			bUsed;
	int				leftconnections;
	int				rightconnections;
	int				bottomconnections;
	int				rightlines;
	int				bottomlines;

	int				leftconnectionsleft;
	int				rightconnectionsleft;
	int				bottomconnectionsleft;
	int				rightlinesleft;
	int				bottomlinesleft;
	std::set<INT_PTR>	connections;
	
	CRect			drawrect;
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
class CRevisionGraph
{
public:
	CRevisionGraph(void);
	~CRevisionGraph(void);
	BOOL						FetchRevisionData(CString path);
	BOOL						AnalyzeRevisionData(CString path, bool bShowAll = false, bool bArrangeByPath = false);
	virtual BOOL				ProgressCallback(CString text1, CString text2, DWORD done, DWORD total);
	
	
	CString						GetLastErrorMessage();
	static bool					IsParentOrItself(const char * parent, const char * child);
	CPtrArray					m_arEntryPtrs;
	size_t						m_maxurllength;
	CString						m_maxurl;
	int							m_maxlevel;
	svn_revnum_t				m_numRevisions;

	typedef std::multimap<svn_revnum_t, CRevisionEntry*>::iterator EntryPtrsIterator;
	typedef std::pair<svn_revnum_t, CRevisionEntry*> EntryPair;
	std::multimap<svn_revnum_t, CRevisionEntry*> m_mapEntryPtrs;
	CString						GetReposRoot() {return CString(m_sRepoRoot);}

	BOOL						m_bCancelled;
	apr_array_header_t *		m_logdata;
	apr_pool_t *				pool;			///< memory pool
	apr_pool_t *				graphpool;
	svn_client_ctx_t 			m_ctx;
	SVNPrompt					m_prompt;

private:
	bool						BuildForwardCopies();
	bool						AnalyzeRevisions(CStringA url, svn_revnum_t startrev, bool bShowAll);
	bool						Cleanup(CStringA url, bool bArrangeByPath);
	
	bool						SetCopyTo(const char * copyfrom_path, svn_revnum_t copyfrom_rev, 
											const char * copyto_path, svn_revnum_t copyto_rev);
	CRevisionEntry *			GetRevisionEntry(const char * path, svn_revnum_t rev, 
													bool bCreate = true);
	char *						GetRename(const char * url, LONG rev);

#ifdef DEBUG	
	void						PrintDebugInfo();
#endif

	static int __cdecl			SortCompareRevUrl(const void * pElem1, const void * pElem2);	///< sort callback function
	static int __cdecl			SortCompareRevLevels(const void * pElem1, const void * pElem2);	///< sort callback function
	static int __cdecl			SortCompareSourceEntry(const void * pElem1, const void * pElem2);	///< sort callback function
	CStringA					m_sRepoRoot;
	LONG						m_lHeadRevision;

	int							m_nRecurseLevel;
	svn_error_t *				Err;			///< Global error object struct
	apr_pool_t *				parentpool;
	static svn_error_t*			cancel(void *baton);
	static svn_error_t*			logDataReceiver(void* baton, 
												apr_hash_t* ch_paths, 
												svn_revnum_t rev, 
												const char* author, 
												const char* date, 
												const char* msg, 
												apr_pool_t* pool);

};
