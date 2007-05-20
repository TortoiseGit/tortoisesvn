// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - Stefan Kueng

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
#include "resource.h"
#include "client.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "SVN.h"
#include "TSVNPath.h"
#include ".\revisiongraph.h"
#include "SVNError.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CRevisionGraph::CRevisionGraph(void) : m_bCancelled(FALSE)
	, m_FilterMinRev(-1)
	, m_FilterMaxRev(-1)
{
	memset (&m_ctx, 0, sizeof (m_ctx));
	parentpool = svn_pool_create(NULL);

	Err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	// set up the configuration
	if (Err == 0)
		Err = svn_config_get_config (&(m_ctx.config), g_pConfigDir, pool);

	if (Err != 0)
	{
		::MessageBox(NULL, this->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_pool_destroy (pool);
		svn_pool_destroy (parentpool);
		exit(-1);
	}

	// set up authentication
	m_prompt.Init(pool, &m_ctx);

	m_ctx.cancel_func = cancel;
	m_ctx.cancel_baton = this;

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	if (tsvn_ssh.IsEmpty())
		tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
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
	for (TLogDataMap::iterator iter = m_logdata.begin(), end = m_logdata.end(); iter != end; ++iter)
	{
		LogChangedPathArray& changes = *iter->second->changes;
		for (INT_PTR i = 0, count = changes.GetCount(); i < count; ++i)
			delete changes.GetAt(i);

		delete iter->second;
	}

	m_mapEntryPtrs.clear();
	m_arEntryPtrs.RemoveAll();
}

bool CRevisionGraph::SetFilter(svn_revnum_t minrev, svn_revnum_t maxrev, const CString& sPathFilter)
{
	m_FilterMinRev = minrev;
	m_FilterMaxRev = maxrev;
	m_filterpaths.clear();
	// the filtered paths are separated by an '*' char, since that char is illegal in paths and urls
	if (!sPathFilter.IsEmpty())
	{
		int pos = sPathFilter.Find('*');
		if (pos)
		{
			CString sTemp = sPathFilter;
			while (pos>=0)
			{
				m_filterpaths.insert((LPCWSTR)sTemp.Left(pos));
				sTemp = sTemp.Mid(pos+1);
				pos = sTemp.Find('*');
			}
			m_filterpaths.insert((LPCWSTR)sTemp);
		}
		else
			m_filterpaths.insert((LPCWSTR)sPathFilter);
	}
	return true;
}

BOOL CRevisionGraph::ProgressCallback(CString /*text*/, CString /*text2*/, DWORD /*done*/, DWORD /*total*/) {return TRUE;}

svn_error_t* CRevisionGraph::cancel(void *baton)
{
	CRevisionGraph * me = (CRevisionGraph *)baton;
	if (me->m_bCancelled)
	{
		CString temp;
		temp.LoadString(IDS_SVN_USERCANCELLED);
		return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
	}
	return SVN_NO_ERROR;
}

// implement ILogReceiver

void CRevisionGraph::ReceiveLog ( LogChangedPathArray* changes
								, svn_revnum_t rev
								, const CString& author
								, const apr_time_t& timeStamp
								, const CString& message)
{
	// create new entry

	std::auto_ptr<log_entry> entry (new log_entry);
	if (entry.get() == NULL)
	{
		throw SVNError (svn_error_create (APR_OS_START_SYSERR + ERROR_NOT_ENOUGH_MEMORY, NULL, NULL));
	}

	// fill it

	entry->changes.reset (changes);
	entry->rev = rev;
	entry->author = author;
	entry->timeStamp = timeStamp;
	entry->message = message;

	// update internal data

	if (m_lHeadRevision < rev)
		m_lHeadRevision = rev;

	// update progress bar and check for user pressing "Cancel" somewhere

	if (rev > 0)
	{
		CString temp, temp2;
		temp.LoadString(IDS_REVGRAPH_PROGGETREVS);
		temp2.Format(IDS_REVGRAPH_PROGCURRENTREV, rev);
		if (!ProgressCallback (temp, temp2, m_lHeadRevision - rev, m_lHeadRevision))
		{
			m_bCancelled = TRUE;
			throw SVNError (cancel (this));
		}
	}

	// finally, store the log info 

	m_logdata[rev] = entry.release();
}

BOOL CRevisionGraph::FetchRevisionData(CString path)
{
	// set some text on the progress dialog, before we wait
	// for the log operation to start
	CString temp;
	temp.LoadString(IDS_REVGRAPH_PROGGETREVS);
	ProgressCallback(temp, _T(""), 0, 1);

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
	url = CPathUtils::PathEscape(url);

	// we have to get the log from the repository root
	CTSVNPath urlpath;
	urlpath.SetFromSVN(url);

	SVN svn;
	m_sRepoRoot = svn.GetRepositoryRoot(urlpath);
	url = m_sRepoRoot;
	urlpath.SetFromSVN(url);

	if (m_sRepoRoot.IsEmpty())
	{
		Err = svn.Err;
		return FALSE;
	}

	m_lHeadRevision = -1;
	try
	{
		CSVNLogQuery svnQuery (&m_ctx, pool);
		CCacheLogQuery cacheQuery (svn.GetLogCachePool(), &svnQuery);

		CRegStdWORD useLogCache (_T("Software\\TortoiseSVN\\UseLogCache"), FALSE);
		ILogQuery* query = useLogCache != FALSE
						 ? static_cast<ILogQuery*>(&cacheQuery)
						 : static_cast<ILogQuery*>(&svnQuery);

		query->Log ( CTSVNPathList (urlpath)
				   , SVNRev(SVNRev::REV_HEAD)
				   , SVNRev(SVNRev::REV_HEAD)
				   , SVNRev(0)
				   , 0
				   , false
				   , this);
	}
	catch (SVNError& e)
	{
		Err = svn_error_create (e.GetCode(), NULL, e.GetMessage());
		return FALSE;
	}

	return TRUE;
}

BOOL CRevisionGraph::AnalyzeRevisionData(CString path, bool bShowAll /* = false */, bool bArrangeByPath /* = false */)
{
	if (m_logdata.empty())
		return FALSE;

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
	m_maxurllength = 0;
	m_maxurl.Empty();
	m_numRevisions = 0;
	m_maxlevel = 0;

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

	CPathUtils::Unescape(url.GetBuffer(url.GetLength()+1));
	url.ReleaseBuffer();

	CStringA sRepoRoot = m_sRepoRoot;
	CPathUtils::Unescape(sRepoRoot.GetBuffer(sRepoRoot.GetLength()+1));
	sRepoRoot.ReleaseBuffer();
	url = url.Mid(sRepoRoot.GetLength());
	m_nRecurseLevel = 0;
	BuildForwardCopies();
	
	// in case our path was renamed and had a different name in the past,
	// we have to find out that name now, because we will analyze the data
	// from lower to higher revisions
	
	CString realurl = SVN::MakeUIUrlOrPath (url);
	svn_revnum_t initialrev = 0;
	for (svn_revnum_t currentrev = m_lHeadRevision; currentrev > 0; --currentrev)
	{
		TLogDataMap::iterator iter = m_logdata.find (currentrev);
		if (iter != m_logdata.end())
		{
			log_entry * logentry = iter->second;
			for (INT_PTR i = 0, count = logentry->changes->GetCount(); i < count; ++i)
			{
				const LogChangedPath* change = logentry->changes->GetAt (i);
				if (   (change->action == LOGACTIONS_ADDED) 
					&& (IsParentOrItself (change->sPath, realurl)))
				{
					if (!change->sCopyFromPath.IsEmpty())
					{
						CString child = realurl.Mid(change->sPath.GetLength());
						currentrev = change->lCopyFromRev+1;
						realurl = change->sCopyFromPath;
						realurl += child;
					}
					else if (change->sPath == realurl)
					{
						initialrev = logentry->rev;
						// this is where our entry got added
						// break out of the loop, we don't need
						// to go further back.
						currentrev = 0;
						break;
					}
				}
			}
		}
	}

	CRevisionEntry * reventry = GetRevisionEntry(realurl, initialrev, true);
	if (reventry)
	{
		if (reventry->action == CRevisionEntry::nothing)
		{
			reventry->action = CRevisionEntry::initial;
			reventry->bUsed = true;
		}
	}

	if (AnalyzeRevisions(realurl, initialrev, bShowAll))
	{
		return Cleanup(bArrangeByPath);
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
		TLogDataMap::iterator iter = m_logdata.find (currentrev);
		if (iter != m_logdata.end())
		{
			log_entry * logentry = iter->second;
			ASSERT(logentry->rev == currentrev);

			for (INT_PTR i = 0, count = logentry->changes->GetCount(); i < count; ++i)
			{
				const LogChangedPath* change = logentry->changes->GetAt (i);

				CRevisionEntry::Action action;
				switch (change->action)
				{
				case LOGACTIONS_ADDED:
					if (!change->sCopyFromPath.IsEmpty())
						action = CRevisionEntry::addedwithhistory;
					else
						action = CRevisionEntry::added;
					break;
				case LOGACTIONS_DELETED:
					action = CRevisionEntry::deleted;
					break;
				case LOGACTIONS_REPLACED:
					action = CRevisionEntry::replaced;
					break;
				case LOGACTIONS_MODIFIED:
					action = CRevisionEntry::modified;
					break;
				default:
					action = CRevisionEntry::nothing;
					break;
				}

				CRevisionEntry * reventry = GetRevisionEntry (change->sPath, logentry->rev);
				reventry->action = action;

				if (!change->sCopyFromPath.IsEmpty())
				{
					// yes, this entry was copied from somewhere
					SetCopyTo ( change->sCopyFromPath
						      , change->lCopyFromRev
							  , change->sPath
							  , logentry->rev);
				}
			}
		}
	}
	return true;
}

bool CRevisionGraph::AnalyzeRevisions (CString url, svn_revnum_t startrev, bool bShowAll)
{
#define TRACELEVELSPACE {for (int traceloop = 1; traceloop < m_nRecurseLevel; traceloop++) TRACE(_T(" "));}
	m_nRecurseLevel++;
	TRACELEVELSPACE;
	TRACE(_T("Analyzing %s  - recurse level %d\n"), (LPCTSTR)CString(url), m_nRecurseLevel);

	bool bRenamed = false;
	bool bDeleted = false;
	CRevisionEntry * lastchangedreventry = NULL;
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
				CString temp3;
				temp3.LoadString(IDS_SVN_USERCANCELLED);
				Err = svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp3));
				return FALSE;
			}
		}

		CRevisionEntry * reventry = NULL;
		// find all entries with this revision
		for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(currentrev); it != m_mapEntryPtrs.upper_bound(currentrev); )
		{
			reventry = it->second;
			if (reventry->revision == currentrev)
			{
				// we have an entry
				if (IsParentOrItself(reventry->url, url))
				{
					if ((bDeleted)&&(reventry->action != CRevisionEntry::added))
					{
						++it;
						continue;
					}
					bool bIsSame = (reventry->url == url);
					if ((bDeleted)&&(!bIsSame))
					{
						++it;
						continue;
					}

					bDeleted = false;
					reventry->level = m_nRecurseLevel;
					if (reventry->action == CRevisionEntry::deleted)
					{
						CString newname = GetRename(reventry->url, currentrev);
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
							}
							delete reventry;
							if (it == m_mapEntryPtrs.begin())
							{
								m_mapEntryPtrs.erase(it);
								it = m_mapEntryPtrs.begin();
							}
							else
							{
								EntryPtrsIterator it2 = it;
								--it2;
								m_mapEntryPtrs.erase(it);
								it = it2;
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
						CString child = url.Mid (reventry->url.GetLength());
						if (!child.IsEmpty())
						{
							reventry->url += child;
						}
						reventry->bUsed = true;
						bDeleted = true;
						++it;
						continue;
					}
					if (reventry->action == CRevisionEntry::replaced)
					{
						reventry->bUsed = true;
					}
					CString child = url.Mid (reventry->url.GetLength());
					if (reventry->action != CRevisionEntry::added)
					{
						if (!child.IsEmpty())
						{
							reventry->url += child;
						}
					}
					// and the entry is for us
					if (reventry->action != CRevisionEntry::modified)
					{
						reventry->bUsed = true;
						if (reventry->action == CRevisionEntry::nothing)
							reventry->action = CRevisionEntry::source;
					}
					if (bRenamed)
					{
						reventry->bUsed = true;
						reventry->action = CRevisionEntry::renamed;
						bRenamed = false;
					}
					if 	((reventry->action != CRevisionEntry::modified)&&
						(reventry->action != CRevisionEntry::lastcommit)&&
						(reventry->sourcearray.GetCount() > 0))
					{
						// the entry is a source of a copy
						reventry->bUsed = true;
						if (reventry->action == CRevisionEntry::nothing)
							reventry->action = CRevisionEntry::source;
						for (INT_PTR copytoindex=0; copytoindex<reventry->sourcearray.GetCount(); ++copytoindex)
						{
							source_entry * sentry = (source_entry*)reventry->sourcearray[copytoindex];
							// follow all copy to targets
							CString targetURL = sentry->pathto + url.Mid (reventry->realurl.GetLength());
							AnalyzeRevisions(targetURL, sentry->revisionto, bShowAll);
						}
					}
					if (bIsSame)
					{
						if ((bDeleted)||(reventry->action == CRevisionEntry::deleted)||(reventry->action == CRevisionEntry::replaced))
						{
							++it;
							continue;
						}
						if (lastchangedreventry)
						{
							if (reventry->revision > lastchangedreventry->revision)
								lastchangedreventry = reventry;
						}
						else
							lastchangedreventry = reventry;
						if (bShowAll)
						{
							if (!reventry->bUsed)
							{
								reventry->bUsed = true;
								reventry->action = CRevisionEntry::modified;
								reventry->level = m_nRecurseLevel;
								reventry->url = url;
							}
						}
					}
				}
				else if (IsParentOrItself(url, reventry->url))
				{
					if ((bDeleted)||(reventry->action == CRevisionEntry::deleted)||(reventry->action == CRevisionEntry::replaced))
					{
						++it;
						continue;
					}
					if (lastchangedreventry)
					{
						if (reventry->revision > lastchangedreventry->revision)
							lastchangedreventry = reventry;
					}
					else
						lastchangedreventry = reventry;
					if (bShowAll)
					{
						if (!reventry->bUsed)
						{
							reventry->bUsed = true;
							reventry->action = CRevisionEntry::modified;
							reventry->level = m_nRecurseLevel;
							reventry->url = url;
						}
					}
				}
			}
			++it;
		}
	}
	if ((lastchangedreventry)&&((!lastchangedreventry->bUsed)||(lastchangedreventry->action == CRevisionEntry::nothing)))
	{
		// This is the last entry for that url. That means after this revision, there are no more
		// entries shown for this url.
		lastchangedreventry->bUsed = true;
		lastchangedreventry->action = CRevisionEntry::lastcommit;
		lastchangedreventry->level = m_nRecurseLevel;
		lastchangedreventry->url = url;
	}
	m_nRecurseLevel--;
	return true;
}

bool CRevisionGraph::SetCopyTo(const CString& uiCopyFromPath, svn_revnum_t copyfrom_rev, const CString& copyto_path, svn_revnum_t copyto_rev)
{
	CRevisionEntry * reventry = GetRevisionEntry (uiCopyFromPath, copyfrom_rev);
	ATLASSERT(reventry);
	if (reventry->action == CRevisionEntry::deleted)
	{
		// the entry was deleted! So it can't be the source of a copy operation
		reventry = new CRevisionEntry();
		reventry->revision = copyfrom_rev;
		reventry->url = uiCopyFromPath;

		TLogDataMap::iterator iter = m_logdata.find (copyfrom_rev);
		if (iter != m_logdata.end())
		{
			log_entry * logentry = iter->second;

			ASSERT(logentry->rev == copyfrom_rev);
			reventry->author = logentry->author;
			reventry->message = logentry->message;
			reventry->date = logentry->timeStamp;
		}
		m_mapEntryPtrs.insert(EntryPair(reventry->revision, reventry));
	}
	source_entry * sentry = new source_entry;
	sentry->pathto = copyto_path;
	sentry->revisionto = copyto_rev;
	reventry->sourcearray.Add(sentry);
	return true;
}

CRevisionEntry * CRevisionGraph::GetRevisionEntry(const CString& path, svn_revnum_t rev, bool bCreate /* = true */)
{
	CRevisionEntry * reventry = NULL;
	for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(rev); it != m_mapEntryPtrs.upper_bound(rev); ++it)
	{
		CRevisionEntry * rentry = it->second;
		if (rentry->revision == rev)
		{
			if (rentry->url == path)
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
						if (sentry->pathto == path)
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
		reventry->url = path;
		reventry->realurl = path;

		TLogDataMap::iterator iter = m_logdata.find (rev);
		if (iter != m_logdata.end())
		{
			log_entry * logentry = iter->second;

			ASSERT(logentry->rev == rev);
			reventry->author = logentry->author;
			reventry->message = logentry->message;
			reventry->date = logentry->timeStamp;
		}
		m_mapEntryPtrs.insert(EntryPair(reventry->revision, reventry));
	}
	return reventry;
}

bool CRevisionGraph::Cleanup(bool bArrangeByPath)
{
	// step one: remove all entries which aren't marked as in use
	for (EntryPtrsIterator it = m_mapEntryPtrs.begin(); it != m_mapEntryPtrs.end();)
	{
		CRevisionEntry * reventry = it->second;
		if (!reventry->bUsed)
		{
			// delete unused entry
			for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
				delete (source_entry*)reventry->sourcearray[j];
			delete reventry;
			reventry = NULL;
			if (it == m_mapEntryPtrs.begin())
			{
				m_mapEntryPtrs.erase(it);
				it = m_mapEntryPtrs.begin();
			}
			else
			{
				EntryPtrsIterator it2 = it;
				--it2;
				m_mapEntryPtrs.erase(it);
				it = it2;
			}
		}
		else
		{
			// remove entries which are filtered out
			bool bAdd = true;
			if (m_filterpaths.size() > 0)
			{
				for (std::set<std::wstring>::iterator fi = m_filterpaths.begin(); fi != m_filterpaths.end(); ++fi)
				{
					if (reventry->url.Find (fi->c_str()) >= 0)
					{
						bAdd = false;
						break;
					}
				}
			}
			if ((m_FilterMinRev > 0)&&(m_FilterMaxRev > 0))
			{
				if ((reventry->revision < m_FilterMinRev)||(reventry->revision > m_FilterMaxRev))
					bAdd = false;
			}
			if (!bAdd)
			{
				// delete unused entry
				for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
					delete (source_entry*)reventry->sourcearray[j];
				delete reventry;
				reventry = NULL;
				if (it == m_mapEntryPtrs.begin())
				{
					m_mapEntryPtrs.erase(it);
					it = m_mapEntryPtrs.begin();
				}
				else
				{
					EntryPtrsIterator it2 = it;
					--it2;
					m_mapEntryPtrs.erase(it);
					it = it2;
				}
			}
			else
				++it;
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
			if ((reventry2->revision == reventry->revision)&&(reventry->url == reventry2->url))
			{
				if (reventry->action == CRevisionEntry::nothing)
				{
					reventry->action = reventry2->action;
				}
				else if (reventry->action == CRevisionEntry::source)
				{
					if (reventry2->action != CRevisionEntry::nothing)
						reventry->action = reventry2->action;
				}
				else if (reventry->action == CRevisionEntry::lastcommit)
				{
					if ((reventry2->action != CRevisionEntry::nothing)&&(reventry2->action != CRevisionEntry::source))
						reventry2->action = reventry2->action;
				}
				reventry->level = min(reventry->level, reventry2->level);
				for (INT_PTR si=0; si<reventry2->sourcearray.GetCount(); ++si)
				{
					reventry->sourcearray.Add(reventry2->sourcearray[si]);
				}
				for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(reventry2->revision); it != m_mapEntryPtrs.upper_bound(reventry2->revision); )
				{
					if (it->second == reventry2)
					{
						delete reventry2;
						if (it == m_mapEntryPtrs.begin())
						{
							m_mapEntryPtrs.erase(it);
							it = m_mapEntryPtrs.begin();
						}
						else
						{
							EntryPtrsIterator it2 = it;
							--it2;
							m_mapEntryPtrs.erase(it);
							it = it2;
						}
						m_arEntryPtrs.RemoveAt(i+1);
						break;
					}
					else
						++it;
				}
				i=-1;
			}
		}
	}

	// step two and a half: rearrange the nodes by path if requested
	if (bArrangeByPath)
	{
		// arraning the nodes by path:
		// each URL gets its own column, as long as there are more than just
		// one node of that URL.

		struct view
		{
			int count;
			int level;
		};
		std::map<std::wstring, view> pathmap;

		// go through all nodes, and count the number of nodes for each URL
		// don't assign a valid level for those yet
		for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
		{
			CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
			if (reventry->url)
			{
				std::wstring surl(reventry->url);
				std::map<std::wstring, view>::iterator findit = pathmap.lower_bound(surl);
				if (findit == pathmap.end() || findit->first != surl)
				{
					findit = pathmap.insert(findit, std::make_pair(surl, view()));
					findit->second.count = 0;
					findit->second.level = 0;
				}
				// that URL already is in the map, so just increase its count
				findit->second.count++;
			}
		}

		// since the map is ordered alphabetically by URL, we assign each URL
		// a level according to its order. This makes sure that e.g. all tags
		// get on the same level, because they will all be ordered after each
		// other in the map.
		int proplevel=1;
		bool bLastWasCountEqualOne = false;
		for (std::map<std::wstring,view>::iterator it = pathmap.begin(); it != pathmap.end(); ++it)
		{
			if (it->second.count > 1)
			{
				if (bLastWasCountEqualOne)
					++proplevel;
				bLastWasCountEqualOne = false;
				it->second.level = proplevel++;
			}
			else
			{
				it->second.level = proplevel;
				bLastWasCountEqualOne = true;
			}
		}

		// ordering the nodes alphabetically by URL doesn't look that good
		// in the graph, because 'branch' comes before 'tags' and only then
		// comes 'trunk'.
		// So we reorder the nodes again, but this time by revisions an URL
		// appears first. That way the graph looks a lot nicer.
		
		std::map<int, int> levelmap;	// Assigns each alphabetical level a real level
		std::map<std::wstring, view>::iterator lev = pathmap.begin();
		int reallevel = 1;
		for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
		{
			CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
			if ((lev = pathmap.find((LPCWSTR)reventry->url)) != pathmap.end())
			{
				std::map<int, int>::iterator levelit = levelmap.lower_bound(lev->second.level);
				if (levelit == levelmap.end() || levelit->first != lev->second.level)
				{
					levelit = levelmap.insert(levelit, std::make_pair(lev->second.level, reallevel));
					++reallevel;
				}
				reventry->level = levelit->second;
			}
			else
				ATLASSERT(FALSE);
		}
	}

	// step four: connect entries with the same name and the same level
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		for (INT_PTR j=i-1; j>=0; --j)
		{
			CRevisionEntry * preventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(j);
			if ((reventry->level == preventry->level)&&(reventry->url == preventry->url))
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
						if ((s_entry->revisionto == preventry->revision)&&(s_entry->pathto == preventry->url))
							bConnected = TRUE;
					}
					for (INT_PTR k=0; k<preventry->sourcearray.GetCount(); ++k)
					{
						source_entry * s_entry = (source_entry *)preventry->sourcearray[k];
						if ((s_entry->revisionto == reventry->revision)&&(s_entry->pathto ==  reventry->url))
							bConnected = TRUE;
					}
					if (!bConnected)
					{
						source_entry * sentry = new source_entry;
						sentry->pathto = preventry->url;
						sentry->revisionto = preventry->revision;
						reventry->sourcearray.Add(sentry);
						if (reventry->action == CRevisionEntry::lastcommit)
							reventry->action = CRevisionEntry::source;
						break;
					}
				}
			}
		}
	}

	// step five: adjust the entry levels
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
		return wcscmp(entry2->url, entry1->url);
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
			return wcscmp (entry1->url, entry2->url);
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
		return wcscmp(entry2->pathto, entry1->pathto);
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

bool CRevisionGraph::IsParentOrItself(const wchar_t * parent, const wchar_t * child)
{
	size_t len = wcslen(parent);
	if (wcsncmp(parent, child, len)==0)
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

CString CRevisionGraph::GetRename(const CString& url, LONG rev)
{
	TLogDataMap::iterator iter = m_logdata.find (rev);
	if (iter != m_logdata.end())
	{
		log_entry * logentry = iter->second;

		for (INT_PTR i = 0, count = logentry->changes->GetCount(); i < count; ++i)
		{
			const LogChangedPath* change = logentry->changes->GetAt (i);

			if (   !change->sCopyFromPath.IsEmpty() 
				&& IsParentOrItself (change->sCopyFromPath, url))
			{
				// check if copyfrom_path was removed in this revision

				for (INT_PTR k = 0; k < count; ++k)
				{
					const LogChangedPath* change2 = logentry->changes->GetAt (k);

					if (   (change2->action == LOGACTIONS_DELETED)
						&& (change2->sPath == change->sCopyFromPath))
					{
						CString child = url.Mid (change2->sPath.GetLength());
						return change2->sPath + url;
					}
				}	
			}
		}
	}
	return CString();
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
