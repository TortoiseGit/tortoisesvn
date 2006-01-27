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
#include "StdAfx.h"
#include "resource.h"
#include "client.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "Utils.h"
#include "SVN.h"
#include "TSVNPath.h"
#include ".\revisiongraph.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CRevisionGraph::CRevisionGraph(void) :
	m_bCancelled(FALSE)
{
	memset (&m_ctx, 0, sizeof (m_ctx));
	parentpool = svn_pool_create(NULL);
	svn_utf_initialize(parentpool);

	Err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	graphpool = svn_pool_create(parentpool);
	// set up the configuration
	if (Err == 0)
		Err = svn_config_get_config (&(m_ctx.config), g_pConfigDir, pool);

	if (Err != 0)
	{
		::MessageBox(NULL, this->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_pool_destroy (pool);
		svn_pool_destroy (graphpool);
		svn_pool_destroy (parentpool);
		exit(-1);
	} // if (Err != 0) 

	// set up authentication
	m_prompt.Init(pool, &m_ctx);

	m_ctx.cancel_func = cancel;
	m_ctx.cancel_baton = this;

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	if (tsvn_ssh.IsEmpty())
		tsvn_ssh = CUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_ctx.config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}
}

CRevisionGraph::~CRevisionGraph(void)
{
	svn_pool_destroy (parentpool);
	for (EntryPtrsIterator it = m_mapEntryPtrs.begin(); it != m_mapEntryPtrs.end(); ++it)
	{
		CRevisionEntry * e = it->second;
		for (int j=0; j<e->sourcearray.GetCount(); ++j)
		{
			delete (source_entry*)e->sourcearray.GetAt(j);
		}
		delete e;
	}
	m_mapEntryPtrs.clear();
	m_arEntryPtrs.RemoveAll();
}

BOOL CRevisionGraph::ProgressCallback(CString /*text*/, CString /*text2*/, DWORD /*done*/, DWORD /*total*/) {return TRUE;}

svn_error_t* CRevisionGraph::cancel(void *baton)
{
	CRevisionGraph * me = (CRevisionGraph *)baton;
	if (me->m_bCancelled)
	{
		CStringA temp;
		temp.LoadString(IDS_SVN_USERCANCELLED);
		return svn_error_create(SVN_ERR_CANCELLED, NULL, temp);
	}
	return SVN_NO_ERROR;
}

svn_error_t* CRevisionGraph::logDataReceiver(void* baton, 
								  apr_hash_t* ch_paths, 
								  svn_revnum_t rev, 
								  const char* author, 
								  const char* date, 
								  const char* msg, 
								  apr_pool_t* localPool)
{
// put all data we receive into an array for later use.
	svn_error_t * error = NULL;
	log_entry * e = NULL;
	CRevisionGraph * me = (CRevisionGraph *)baton;

	e = (log_entry *)apr_pcalloc (me->pool, sizeof(log_entry));
	if (e==NULL)
	{
		return svn_error_create(APR_OS_START_SYSERR + ERROR_NOT_ENOUGH_MEMORY, NULL, NULL);
	}
	if (date && date[0])
	{
		//Convert date to a format for humans.
		error = svn_time_from_cstring (&e->time, date, localPool);
		if (error)
			return error;
	}
	if (author)
		e->author = apr_pstrdup(me->pool, author);
	else
		e->author = 0;
	if (ch_paths)
	{
		//create a copy of the hash
		e->ch_paths = apr_hash_make(me->pool);
		apr_hash_index_t* hi;
		for (hi = apr_hash_first (me->pool, ch_paths); hi; hi = apr_hash_next (hi))
		{
			const char * key;
			const char * keycopy;
			svn_log_changed_path_t *val;
			svn_log_changed_path_t *valcopy = (svn_log_changed_path_t *)apr_pcalloc (me->pool, sizeof(svn_log_changed_path_t));
			apr_hash_this(hi, (const void**)&key, NULL, (void**)&val);
			keycopy = apr_pstrdup(me->pool, key);
			valcopy->copyfrom_path = apr_pstrdup(me->pool, val->copyfrom_path);
			valcopy->action = val->action;
			valcopy->copyfrom_rev = val->copyfrom_rev;
			apr_hash_set(e->ch_paths, keycopy, APR_HASH_KEY_STRING, valcopy);
		}
	}
	else
		e->ch_paths = 0;

	if (msg)
		e->msg = apr_pstrdup(me->pool, msg);
	else
		e->msg = 0;
	e->rev = rev;
	if (me->m_lHeadRevision < rev)
		me->m_lHeadRevision = rev;
	if (rev)
	{
		CString temp, temp2;
		temp.LoadString(IDS_REVGRAPH_PROGGETREVS);
		temp2.Format(IDS_REVGRAPH_PROGCURRENTREV, rev);
		if (!me->ProgressCallback(temp, temp2, me->m_lHeadRevision - rev, me->m_lHeadRevision))
		{
			me->m_bCancelled = TRUE;
			CStringA temp3;
			temp3.LoadString(IDS_SVN_USERCANCELLED);
			error = svn_error_create(SVN_ERR_CANCELLED, NULL, temp3);
			return error;
		}
	}
	APR_ARRAY_PUSH(me->m_logdata, log_entry *) = e;
	return SVN_NO_ERROR;
}

BOOL CRevisionGraph::FetchRevisionData(CString path)
{
	// prepare the path for Subversion
	SVN::preparePath(path);
	CStringA url = CUnicodeUtils::GetUTF8(path);

	// convert a working copy path into an URL if necessary
	if (!svn_path_is_url(url))
	{
		//not an url, so get the URL from the working copy path first
		svn_wc_adm_access_t *adm_access;          
		const svn_wc_entry_t *entry;  
		const char * canontarget = svn_path_canonicalize(url, pool);
#pragma warning(push)
#pragma warning(disable: 4127)	// conditional expression is constant
			Err = svn_wc_adm_probe_open2 (&adm_access, NULL, canontarget,
				FALSE, 0, pool);
			if (Err) return FALSE;
			Err =  svn_wc_entry (&entry, canontarget, adm_access, FALSE, pool);
			if (Err) return FALSE;
			Err = svn_wc_adm_close (adm_access);
			if (Err) return FALSE;
#pragma warning(pop)

			url = entry ? entry->url : "";
	}
	url = CUtils::PathEscape(url);

	// we have to get the log from the repository root
	CTSVNPath urlpath;
	urlpath.SetFromSVN(url);
	SVN svn;
	m_sRepoRoot = svn.GetRepositoryRoot(urlpath);
	url = m_sRepoRoot;
	if (m_sRepoRoot.IsEmpty())
	{
		Err = svn.Err;
		return FALSE;
	}
	
	apr_array_header_t *targets = apr_array_make (pool, 1, sizeof (const char *));
	const char * target = apr_pstrdup (pool, url);
	(*((const char **) apr_array_push (targets))) = target;

	m_logdata = apr_array_make(pool, 100, sizeof(log_entry *));
	m_lHeadRevision = -1;
	Err = svn_client_log (targets, 
		SVNRev(SVNRev::REV_HEAD), 
		SVNRev(0), 
		TRUE,
		FALSE,
		logDataReceiver,
		(void *)this, &m_ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CRevisionGraph::AnalyzeRevisionData(CString path, bool bShowAll /* = false */)
{
	if (m_logdata == NULL)
		return FALSE;

	svn_pool_destroy(graphpool);
	graphpool = svn_pool_create(parentpool);

	for (EntryPtrsIterator it = m_mapEntryPtrs.begin(); it != m_mapEntryPtrs.end(); ++it)
	{
		CRevisionEntry * e = it->second;
		for (int j=0; j<e->sourcearray.GetCount(); ++j)
		{
			delete (source_entry*)e->sourcearray.GetAt(j);
		}
		delete e;
	}
	m_mapEntryPtrs.clear();
	m_arEntryPtrs.RemoveAll();


	SVN::preparePath(path);
	CStringA url = CUnicodeUtils::GetUTF8(path);

	// convert a working copy path into an URL if necessary
	if (!svn_path_is_url(url))
	{
		//not an url, so get the URL from the working copy path first
		svn_wc_adm_access_t *adm_access;          
		const svn_wc_entry_t *entry;  
		const char * canontarget = svn_path_canonicalize(url, graphpool);
#pragma warning(push)
#pragma warning(disable: 4127)	// conditional expression is constant
		Err = svn_wc_adm_probe_open2 (&adm_access, NULL, canontarget,
			FALSE, 0, graphpool);
		if (Err) return FALSE;
		Err =  svn_wc_entry (&entry, canontarget, adm_access, FALSE, graphpool);
		if (Err) return FALSE;
		Err = svn_wc_adm_close (adm_access);
		if (Err) return FALSE;
#pragma warning(pop)

		url = entry ? entry->url : "";
	}

	CUtils::Unescape(url.GetBuffer(url.GetLength()+1));
	url.ReleaseBuffer();

	CStringA sRepoRoot = m_sRepoRoot;
	CUtils::Unescape(sRepoRoot.GetBuffer(sRepoRoot.GetLength()+1));
	sRepoRoot.ReleaseBuffer();
	url = url.Mid(sRepoRoot.GetLength());
	m_nRecurseLevel = 0;
	BuildForwardCopies();
	
	// in case our path was renamed and had a different name in the past,
	// we have to find out that name now, because we will analyse the data
	// from lower to higher revisions
	
	CStringA realurl = url;
	svn_revnum_t initialrev = 0;
	for (svn_revnum_t currentrev = m_lHeadRevision; currentrev > 0; --currentrev)
	{
		log_entry * logentry = APR_ARRAY_IDX(m_logdata, m_lHeadRevision-currentrev, log_entry*);
		if ((logentry)&&(logentry->ch_paths))
		{
			apr_hash_index_t* hi;
			for (hi = apr_hash_first (graphpool, logentry->ch_paths); hi; hi = apr_hash_next (hi))
			{
				const char * key;
				svn_log_changed_path_t *val;
				apr_hash_this(hi, (const void**)&key, NULL, (void**)&val);
				if ((val->action == 'A')&&(IsParentOrItself(key, realurl)))
				{
					if (val->copyfrom_path)
					{
						CStringA child = realurl.Mid(strlen(key));
						currentrev = val->copyfrom_rev+1;
						realurl = val->copyfrom_path;
						realurl += child;
					}
					else if (strcmp(key, realurl)==0)
						initialrev = logentry->rev;
				}
			}
		}
	}
	
	if (AnalyzeRevisions(realurl, initialrev, bShowAll))
	{
		return Cleanup(realurl);
	}
	return FALSE;
}

bool CRevisionGraph::BuildForwardCopies()
{
	// * go through the revisions from top to bottom
	// * for every entry which has a 'copyfrom' entry,
	//   find the source and add this revision as a 'copyto' entry.
	for (svn_revnum_t currentrev=m_lHeadRevision; currentrev >= 0; --currentrev)
	{
		log_entry * logentry = APR_ARRAY_IDX(m_logdata, m_lHeadRevision-currentrev, log_entry*);
		if ((logentry)&&(logentry->ch_paths))
		{
			ASSERT(logentry->rev == currentrev);
			apr_hash_index_t* hi;
			for (hi = apr_hash_first (graphpool, logentry->ch_paths); hi; hi = apr_hash_next (hi))
			{
				const char * key;
				svn_log_changed_path_t *val;
				apr_hash_this(hi, (const void**)&key, NULL, (void**)&val);
				CRevisionEntry::Action action;
				switch (val->action)
				{
				case 'A':
					if (val->copyfrom_path)
						action = CRevisionEntry::addedwithhistory;
					else
						action = CRevisionEntry::added;
					break;
				case 'D':
					action = CRevisionEntry::deleted;
					break;
				case 'R':
					action = CRevisionEntry::replaced;
					break;
				case 'M':
					action = CRevisionEntry::modified;
					break;
				default:
					action = CRevisionEntry::nothing;
					break;
				}
				if (val->copyfrom_path)
				{
					// yes, this entry was copied from somewhere
					CRevisionEntry * reventry = GetRevisionEntry(key, logentry->rev);
					reventry->action = action;
					SetCopyTo(val->copyfrom_path, val->copyfrom_rev, key, currentrev);
				}
				else if (val->action == 'D')
				{
					CRevisionEntry * reventry = GetRevisionEntry(key, logentry->rev);
					reventry->action = action;
				}
				else if (val->action == 'R')
				{
					CRevisionEntry * reventry = GetRevisionEntry(key, logentry->rev);
					reventry->action = action;
				}
				else
				{
					CRevisionEntry * reventry = GetRevisionEntry(key, logentry->rev);
					reventry->action = action;
				}
			}
		}
	}
	return true;
}

bool CRevisionGraph::AnalyzeRevisions(CStringA url, svn_revnum_t startrev, bool bShowAll)
{
#define TRACELEVELSPACE {for (int traceloop = 1; traceloop < m_nRecurseLevel; traceloop++) TRACE(_T(" "));}
	m_nRecurseLevel++;
	TRACELEVELSPACE;
	TRACE(_T("Analyzing %s  - recurse level %d\n"), (LPCTSTR)CString(url), m_nRecurseLevel);

	bool bRenamed = false;
	bool bDeleted = false;
	CRevisionEntry * lastchangedreventry = NULL;
	CRevisionEntry * firstchangedreventry = NULL;
	for (svn_revnum_t currentrev=startrev; currentrev <= m_lHeadRevision; ++currentrev)
	{
		// show progress info
		if (m_nRecurseLevel==1)
		{
			CString temp, temp2;
			temp.LoadString(IDS_REVGRAPH_PROGANALYZE);
			temp2.Format(IDS_REVGRAPH_PROGANALYZEREV, currentrev);
			if (!ProgressCallback(temp, temp2, currentrev, m_lHeadRevision))
			{
				CStringA temp3;
				temp3.LoadString(IDS_SVN_USERCANCELLED);
				Err = svn_error_create(SVN_ERR_CANCELLED, NULL, temp3);
				return FALSE;
			}
		}

		CRevisionEntry * reventry = NULL;
		// find all entries with this revision
		for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(currentrev); it != m_mapEntryPtrs.upper_bound(currentrev); ++it)
		{
			reventry = it->second;
			if (reventry->revision == currentrev)
			{
				// we have an entry
				if (IsParentOrItself(reventry->url, url))
				{
					if ((bDeleted)&&(reventry->action != CRevisionEntry::added))
						continue;
					bool bIsSame = (strcmp(reventry->url, url) == 0);
					if ((bDeleted)&&(!bIsSame))
						continue;

					bDeleted = false;
					reventry->level = m_nRecurseLevel;
					if (reventry->action == CRevisionEntry::deleted)
					{
						char * newname = GetRename(reventry->url, currentrev);
						if (newname)
						{
							// we got renamed in this revision.
							TRACELEVELSPACE;
							TRACE(_T("%s renamed to %s in revision %ld\n"), (LPCTSTR)CString(url), (LPCTSTR)CString(newname), currentrev);
							url = newname;
							ATLASSERT(reventry->sourcearray.GetCount()==0);
							if (reventry == lastchangedreventry)
							{
								lastchangedreventry = NULL;
								firstchangedreventry = NULL;
							}
							delete reventry;
							it->second = NULL;
							EntryPtrsIterator it2 = it;
							if (it != m_mapEntryPtrs.begin())
							{
								--it;
								m_mapEntryPtrs.erase(it2);
							}
							else
							{
								m_mapEntryPtrs.erase(it2);
								it = m_mapEntryPtrs.begin();
							}
							bRenamed = true;
							continue;
						}
						
						// BUGBUG: if the entry was deleted and has an url matching
						// IsParentOrItself, that still doesn't mean that
						// *our* entry was deleted. It may be another entry
						// which was copied there with the same name
						TRACELEVELSPACE;
						TRACE(_T("%s deleted in revision %ld\n"), (LPCTSTR)CString(url), currentrev);
						CStringA child = url.Mid(strlen(reventry->url));
						if (!child.IsEmpty())
						{
							reventry->url = apr_pstrcat(graphpool, reventry->url, (LPCSTR)child, NULL);
						}
						reventry->bUsed = true;
						bDeleted = true;
						continue;
					}
					if (reventry->action == CRevisionEntry::replaced)
					{
						reventry->bUsed = true;
					}
					CStringA child = url.Mid(strlen(reventry->url));
					if (reventry->action != CRevisionEntry::added)
					{
						if (!child.IsEmpty())
						{
							reventry->url = apr_pstrcat(graphpool, reventry->url, (LPCSTR)child, NULL);
						}
					}
					// and the entry is for us
					reventry->bUsed = true;
					if (bRenamed)
					{
						reventry->action = CRevisionEntry::renamed;
						bRenamed = false;
					}
					if (reventry->sourcearray.GetCount() > 0)
					{
						// the entry is a source of a copy
						reventry->bUsed = true;
						for (INT_PTR copytoindex=0; copytoindex<reventry->sourcearray.GetCount(); ++copytoindex)
						{
							source_entry * sentry = (source_entry*)reventry->sourcearray[copytoindex];
							// follow all copy to targets
							CStringA targetURL = sentry->pathto + url.Mid(strlen(reventry->realurl));
							AnalyzeRevisions(targetURL, sentry->revisionto, bShowAll);
						}
					}
					if (bIsSame)
					{
						if ((bDeleted)||(reventry->action == CRevisionEntry::deleted)||(reventry->action == CRevisionEntry::replaced))
							continue;
						if (bShowAll)
						{
							reventry->bUsed = true;
						}
						if (lastchangedreventry)
						{
							if (reventry->revision > lastchangedreventry->revision)
								lastchangedreventry = reventry;
						}
						else
							lastchangedreventry = reventry;
						if (firstchangedreventry)
						{
							if (reventry->revision < firstchangedreventry->revision)
								firstchangedreventry = reventry;
						}
						else
							firstchangedreventry = reventry;
					}
				}
				else if (IsParentOrItself(url, reventry->url))
				{
					if ((bDeleted)||(reventry->action == CRevisionEntry::deleted)||(reventry->action == CRevisionEntry::replaced))
						continue;
					if (bShowAll)
					{
						reventry->bUsed = true;
						reventry->url = apr_pstrdup(graphpool, url);
					}
					if (lastchangedreventry)
					{
						if (reventry->revision > lastchangedreventry->revision)
							lastchangedreventry = reventry;
					}
					else
						lastchangedreventry = reventry;
					if (firstchangedreventry)
					{
						if (reventry->revision < firstchangedreventry->revision)
							firstchangedreventry = reventry;
					}
					else
						firstchangedreventry = reventry;
				}
			}
		}
	}
	if ((lastchangedreventry)&&(!lastchangedreventry->bUsed))
	{
		lastchangedreventry->bUsed = true;
		lastchangedreventry->action = CRevisionEntry::lastcommit;
		lastchangedreventry->level = m_nRecurseLevel;
		lastchangedreventry->url = apr_pstrdup(graphpool, url);
	}
	if ((firstchangedreventry)&&(!firstchangedreventry->bUsed))
	{
		firstchangedreventry->bUsed = true;
		firstchangedreventry->action = CRevisionEntry::lastcommit;
		firstchangedreventry->level = m_nRecurseLevel;
		firstchangedreventry->url = apr_pstrdup(graphpool, url);
	}
	m_nRecurseLevel--;
	return true;
}

bool CRevisionGraph::SetCopyTo(const char * copyfrom_path, svn_revnum_t copyfrom_rev, const char * copyto_path, svn_revnum_t copyto_rev)
{
	CRevisionEntry * reventry = GetRevisionEntry(copyfrom_path, copyfrom_rev);
	ATLASSERT(reventry);
	if (reventry->action == CRevisionEntry::deleted)
	{
		// the entry was deleted! So it can't be the source of a copy operation
		reventry = new CRevisionEntry();
		reventry->revision = copyfrom_rev;
		reventry->url = apr_pstrdup(graphpool, copyfrom_path);
		log_entry * logentry = APR_ARRAY_IDX(m_logdata, m_lHeadRevision-copyfrom_rev, log_entry*);
		if (logentry)
		{
			ASSERT(logentry->rev == copyfrom_rev);
			reventry->author = logentry->author;
			reventry->message = logentry->msg;
			reventry->date = logentry->time;
		}
		m_mapEntryPtrs.insert(EntryPair(reventry->revision, reventry));
	}
	source_entry * sentry = new source_entry;
	sentry->pathto = apr_pstrdup(graphpool, copyto_path);
	sentry->revisionto = copyto_rev;
	reventry->sourcearray.Add(sentry);
	return true;
}

CRevisionEntry * CRevisionGraph::GetRevisionEntry(const char * path, svn_revnum_t rev, bool bCreate /* = true */)
{
	CRevisionEntry * reventry = NULL;
	for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(rev); it != m_mapEntryPtrs.upper_bound(rev); ++it)
	{
		CRevisionEntry * rentry = it->second;
		if (rentry->revision == rev)
		{
			if (strcmp(rentry->url, path)==0)
			{
				reventry = rentry;
				break;
			}
			else
			{
				// urls didn't match!
				// check if the url has been renamed in this revision, and if
				// yes, return this entry anyway
				if (rentry->action == CRevisionEntry::addedwithhistory)
				{
					for (INT_PTR j=0; j<rentry->sourcearray.GetCount(); ++j)
					{
						source_entry * sentry = (source_entry*)rentry->sourcearray[j];
						if (strcmp(sentry->pathto, path)==0)
						{
							reventry = rentry;
							break;
						}
					}
				}
			}
		}
	}
	if ((bCreate)&&(reventry == NULL))
	{
		reventry = new CRevisionEntry();
		reventry->revision = rev;
		reventry->url = apr_pstrdup(graphpool, path);
		reventry->realurl = apr_pstrdup(graphpool, path);
		log_entry * logentry = APR_ARRAY_IDX(m_logdata, m_lHeadRevision-rev, log_entry*);
		if (logentry)
		{
			ASSERT(logentry->rev == rev);
			reventry->author = logentry->author;
			reventry->message = logentry->msg;
			reventry->date = logentry->time;
		}
		m_mapEntryPtrs.insert(EntryPair(reventry->revision, reventry));
	}
	return reventry;
}

bool CRevisionGraph::Cleanup(CStringA url)
{
	// step one: remove all entries which aren't marked as in use
	for (EntryPtrsIterator it = m_mapEntryPtrs.begin(); it != m_mapEntryPtrs.end(); ++it)
	{
		CRevisionEntry * reventry = it->second;
		if (!reventry->bUsed)
		{
			// delete unused entry
			for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
				delete (source_entry*)reventry->sourcearray[j];
			delete reventry;
			reventry = NULL;
			EntryPtrsIterator it2 = it;
			if (it != m_mapEntryPtrs.begin())
			{
				--it;
				m_mapEntryPtrs.erase(it2);
			}
			else
			{
				m_mapEntryPtrs.erase(it2);
				it = m_mapEntryPtrs.begin();
			}
		}
	}	
	for (EntryPtrsIterator it = m_mapEntryPtrs.begin(); it != m_mapEntryPtrs.end(); ++it)
	{
		CRevisionEntry * reventry = it->second;
		m_arEntryPtrs.Add(reventry);
	}
	// step two: sort the entries
	qsort(m_arEntryPtrs.GetData(), m_arEntryPtrs.GetSize(), sizeof(CRevisionEntry *), (GENERICCOMPAREFN)SortCompareRevUrl);
	
	// step three: combine entries with the same revision and url
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		if (i<m_arEntryPtrs.GetCount()-1)
		{
			CRevisionEntry * reventry2 = (CRevisionEntry*)m_arEntryPtrs[i+1];
			if ((reventry2->revision == reventry->revision)&&(strcmp(reventry->url, reventry2->url)==0))
			{
				reventry->action = reventry->action == CRevisionEntry::nothing ? reventry2->action : CRevisionEntry::nothing;
				reventry->level = min(reventry->level, reventry2->level);
				for (INT_PTR si=0; si<reventry2->sourcearray.GetCount(); ++si)
				{
					reventry->sourcearray.Add(reventry2->sourcearray[si]);
				}
				for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(reventry2->revision); it != m_mapEntryPtrs.upper_bound(reventry2->revision); ++it)
				{
					if (it->second == reventry2)
					{
						delete reventry2;
						m_mapEntryPtrs.erase(it);
						m_arEntryPtrs.RemoveAt(i+1);
						break;
					}
				}
				i=-1;
			}
		}
	}

	// step four: adjust the entry levels
	qsort(m_arEntryPtrs.GetData(), m_arEntryPtrs.GetSize(), sizeof(CRevisionEntry *), (GENERICCOMPAREFN)SortCompareRevLevels);
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		if (i<m_arEntryPtrs.GetCount()-1)
		{
			CRevisionEntry * reventry2 = (CRevisionEntry*)m_arEntryPtrs[i+1];
			if ((reventry2->revision == reventry->revision)&&(reventry2->level == reventry->level))
			{
				reventry2->level++;
				qsort(m_arEntryPtrs.GetData(), m_arEntryPtrs.GetSize(), sizeof(CRevisionEntry *), (GENERICCOMPAREFN)SortCompareRevLevels);
				i=-1;
			}
		}
	}
	
	// step five: connect entries with the same name and the same level
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		for (INT_PTR j=i-1; j>=0; --j)
		{
			CRevisionEntry * preventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(j);
			if ((reventry->level == preventry->level)&&(strcmp(reventry->url, preventry->url)==0))
			{
				// if an entry is added, then we don't connect anymore
				if ((preventry->action == CRevisionEntry::added)||(preventry->action == CRevisionEntry::addedwithhistory))
					break;
				// same level and url, now connect those two
				// but first check if they're not already connected!
				BOOL bConnected = FALSE;
				if ((reventry->action != CRevisionEntry::deleted)&&
					(preventry->action != CRevisionEntry::added)&&
					(preventry->action != CRevisionEntry::addedwithhistory)&&
					(preventry->action != CRevisionEntry::replaced))
				{
					for (INT_PTR k=0; k<reventry->sourcearray.GetCount(); ++k)
					{
						source_entry * s_entry = (source_entry *)reventry->sourcearray[k];
						if ((s_entry->revisionto == preventry->revision)&&(strcmp(s_entry->pathto, preventry->url)==0))
							bConnected = TRUE;
					}
					for (INT_PTR k=0; k<preventry->sourcearray.GetCount(); ++k)
					{
						source_entry * s_entry = (source_entry *)preventry->sourcearray[k];
						if ((s_entry->revisionto == reventry->revision)&&(strcmp(s_entry->pathto, reventry->url)==0))
							bConnected = TRUE;
					}
					if (!bConnected)
					{
						source_entry * sentry = new source_entry;
						sentry->pathto = preventry->url;
						sentry->revisionto = preventry->revision;
						reventry->sourcearray.Add(sentry);
						break;
					}
				}
			}
		}
	}
	
	// step six: sort the connections in each revision
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		qsort(reventry->sourcearray.GetData(), reventry->sourcearray.GetSize(), sizeof(source_entry *), (GENERICCOMPAREFN)SortCompareSourceEntry);
	}
	
	return true;
}

int CRevisionGraph::SortCompareRevUrl(const void * pElem1, const void * pElem2)
{
	CRevisionEntry * entry1 = *((CRevisionEntry**)pElem1);
	CRevisionEntry * entry2 = *((CRevisionEntry**)pElem2);
	if (entry2->revision == entry1->revision)
	{
		return strcmp(entry2->url, entry1->url);
	}
	return (entry2->revision - entry1->revision);
}

int CRevisionGraph::SortCompareRevLevels(const void * pElem1, const void * pElem2)
{
	CRevisionEntry * entry1 = *((CRevisionEntry**)pElem1);
	CRevisionEntry * entry2 = *((CRevisionEntry**)pElem2);
	if (entry2->revision == entry1->revision)
	{
		if (entry1->level == entry2->level)
			return strcmp(entry1->url, entry2->url);
		return (entry1->level - entry2->level);
	}
	return (entry2->revision - entry1->revision);
}

int CRevisionGraph::SortCompareSourceEntry(const void * pElem1, const void * pElem2)
{
	source_entry * entry1 = *((source_entry**)pElem1);
	source_entry * entry2 = *((source_entry**)pElem2);
	if (entry1->revisionto == entry2->revisionto)
	{
		return strcmp(entry2->pathto, entry1->pathto);
	}
	return (entry1->revisionto - entry2->revisionto);
}

bool CRevisionGraph::IsParentOrItself(const char * parent, const char * child)
{
	size_t len = strlen(parent);
	if (strncmp(parent, child, len)==0)
	{
		if ((child[len]=='/')||(child[len]==0))
			return true;
	}
	return false;
}

CString CRevisionGraph::GetLastErrorMessage()
{
	return SVN::GetErrorString(Err);
}

char * CRevisionGraph::GetRename(const char * url, LONG rev)
{
	log_entry * logentry = APR_ARRAY_IDX(m_logdata, m_lHeadRevision-rev, log_entry*);
	if (logentry)
	{
		apr_hash_index_t* hashindex;
		for (hashindex = apr_hash_first (graphpool, logentry->ch_paths); hashindex; hashindex = apr_hash_next (hashindex))
		{
			const char * key;
			svn_log_changed_path_t *val;
			apr_hash_this(hashindex, (const void**)&key, NULL, (void**)&val);
			if ((val->copyfrom_path)&&(IsParentOrItself(val->copyfrom_path, url)))
			{
				// check if copyfrom_path was removed in this revision
				apr_hash_index_t* hashindex2;
				for (hashindex2 = apr_hash_first (graphpool, logentry->ch_paths); hashindex2; hashindex2 = apr_hash_next (hashindex2))
				{
					const char * key2;
					svn_log_changed_path_t *val2;
					apr_hash_this(hashindex2, (const void**)&key2, NULL, (void**)&val2);
					if ((val2->action=='D')&&(strcmp(key2, val->copyfrom_path)==0))
					{
						CStringA child = url;
						child = child.Mid(strlen(key));
						return apr_pstrcat(graphpool, key, (const char *)child, NULL);
					}
				}	
			}
		}
	}
	return NULL;
}

#ifdef DEBUG
void CRevisionGraph::PrintDebugInfo()
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * entry = (CRevisionEntry *)m_arEntryPtrs[i];
		ATLTRACE("-------------------------------\n");
		ATLTRACE("entry        : %s\n", entry->url);
		ATLTRACE("revision     : %ld\n", entry->revision);
		ATLTRACE("action       : %d\n", entry->action);
		ATLTRACE("level        : %d\n", entry->level);
		for (INT_PTR k=0; k<entry->sourcearray.GetCount(); ++k)
		{
			source_entry * sentry = (source_entry *)entry->sourcearray[k];
			ATLTRACE("pathto       : %s\n", sentry->pathto);
			ATLTRACE("revisionto   : %ld\n", sentry->revisionto);
		}
	}
		ATLTRACE("*******************************\n");
}
#endif
