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

#include "stdafx.h"
#include "resource.h"
#include "..\TortoiseShell\resource.h"
#include "svn_config.h"
#include "SVNStatus.h"
#include "UnicodeUtils.h"
#include "SVNGlobal.h"
#ifdef _MFC_VER
#	include "SVN.h"
#	include "MessageBox.h"
#	include "registry.h"
#	include "TSVNPath.h"
#	include "PathUtils.h"
#endif

SVNStatus::SVNStatus(bool * pbCanceled)
{
	m_pool = svn_pool_create (NULL);

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", m_pool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", m_pool);
	
	svn_client_create_context(&ctx, m_pool);
	
	if (pbCanceled)
	{
		ctx->cancel_func = cancel;
		ctx->cancel_baton = pbCanceled;
	}

#ifdef _MFC_VER
	svn_config_ensure(NULL, m_pool);
	
	// set up authentication
	m_prompt.Init(m_pool, ctx);

	svn_utf_initialize(m_pool);

	// set up the configuration
	m_err = svn_config_get_config (&(ctx->config), g_pConfigDir, m_pool);

	if (m_err)
	{
		::MessageBox(NULL, this->GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_pool_destroy (m_pool);					// free the allocated memory
		exit(-1);
	} // if (m_err) 

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	if (tsvn_ssh.IsEmpty())
		tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get ((apr_hash_t *)ctx->config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}
#else
	svn_config_ensure(NULL, m_pool);
	svn_utf_initialize(m_pool);

	// set up the configuration
	m_err = svn_config_get_config (&(ctx->config), g_pConfigDir, m_pool);

#endif
}

SVNStatus::~SVNStatus(void)
{
	svn_pool_destroy (m_pool);					// free the allocated memory
}

void SVNStatus::ClearPool()
{
	svn_pool_clear(m_pool);
}

#ifdef _MFC_VER
CString SVNStatus::GetLastErrorMsg()
{
	return SVN::GetErrorString(m_err);
}
#else
stdstring SVNStatus::GetLastErrorMsg()
{
	stdstring msg;
	char errbuf[256];

	if (m_err != NULL)
	{
		svn_error_t * ErrPtr = m_err;
		if (ErrPtr->message)
		{
			msg = CUnicodeUtils::StdGetUnicode(ErrPtr->message);
		}
		else
		{
			/* Is this a Subversion-specific error code? */
			if ((ErrPtr->apr_err > APR_OS_START_USEERR)
				&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
				msg = CUnicodeUtils::StdGetUnicode(svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)));
			/* Otherwise, this must be an APR error code. */
			else
			{
				svn_error_t *temp_err = NULL;
				const char * err_string = NULL;
				temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
				if (temp_err)
				{
					svn_error_clear (temp_err);
					msg = _T("Can't recode error string from APR");
				}
				else
				{
					msg = CUnicodeUtils::StdGetUnicode(err_string);
				}
			}

		}

		while (ErrPtr->child)
		{
			ErrPtr = ErrPtr->child;
			msg += _T("\n");
			if (ErrPtr->message)
			{
				msg += CUnicodeUtils::StdGetUnicode(ErrPtr->message);
			}
			else
			{
				/* Is this a Subversion-specific error code? */
				if ((ErrPtr->apr_err > APR_OS_START_USEERR)
					&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
					msg += CUnicodeUtils::StdGetUnicode(svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)));
				/* Otherwise, this must be an APR error code. */
				else
				{
					svn_error_t *temp_err = NULL;
					const char * err_string = NULL;
					temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
					if (temp_err)
					{
						svn_error_clear (temp_err);
						msg += _T("Can't recode error string from APR");
					}
					else
					{
						msg += CUnicodeUtils::StdGetUnicode(err_string);
					}
				}

			}
		}
		return msg;
	} // if (m_err != NULL)
	return msg;
}
#endif

//static method
svn_wc_status_kind SVNStatus::GetAllStatus(const CTSVNPath& path, BOOL recursive)
{
	svn_client_ctx_t * 			ctx;
	svn_wc_status_kind			statuskind;
	apr_pool_t *				pool;
	svn_error_t *				err;
	BOOL						isDir;

	isDir = path.IsDirectory();
	if (!path.HasAdminDir())
		return svn_wc_status_unversioned;

	pool = svn_pool_create (NULL);				// create the memory pool
	svn_utf_initialize(pool);

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", pool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", pool);

	svn_client_create_context(&ctx, pool);

	svn_revnum_t youngest = SVN_INVALID_REVNUM;
	svn_opt_revision_t rev;
	rev.kind = svn_opt_revision_unspecified;
	statuskind = svn_wc_status_none;
	err = svn_client_status2 (&youngest,
							path.GetSVNApiPath(),
							&rev,
							getallstatus,
							&statuskind,
							recursive,	//descend
							TRUE,		//getall
							FALSE,		//update
							TRUE,		//noignore
							FALSE,		//ignore externals
							ctx,
							pool);

	// Error present
	if (err != NULL)
	{
		svn_pool_destroy (pool);				//free allocated memory
		return svn_wc_status_unversioned;	
	}

	svn_pool_destroy (pool);				//free allocated memory
	return statuskind;
}

//static method
svn_wc_status_kind SVNStatus::GetAllStatusRecursive(const CTSVNPath& path)
{
	return GetAllStatus(path, TRUE);
}

//static method
svn_wc_status_kind SVNStatus::GetMoreImportant(svn_wc_status_kind status1, svn_wc_status_kind status2)
{
	if (GetStatusRanking(status1) >= GetStatusRanking(status2))
		return status1;
	return status2;
}
//static private method
int SVNStatus::GetStatusRanking(svn_wc_status_kind status)
{
	switch (status)
	{
		case svn_wc_status_none:
			return 0;
		case svn_wc_status_unversioned:
			return 1;
		case svn_wc_status_ignored:
			return 2;
		case svn_wc_status_incomplete:
			return 4;
		case svn_wc_status_normal:
		case svn_wc_status_external:
			return 5;
		case svn_wc_status_added:
			return 6;
		case svn_wc_status_missing:
			return 7;
		case svn_wc_status_deleted:
			return 8;
		case svn_wc_status_replaced:
			return 9;
		case svn_wc_status_modified:
			return 10;
		case svn_wc_status_merged:
			return 11;
		case svn_wc_status_conflicted:
			return 12;
		case svn_wc_status_obstructed:
			return 13;
	} // switch (status)
	return 0;
}

svn_revnum_t SVNStatus::GetStatus(const CTSVNPath& path, bool update /* = false */, bool noignore /* = false */, bool noexternals /* = false */)
{
	apr_hash_t *				statushash;
	apr_hash_t *				exthash;
	apr_array_header_t *		statusarray;
	const sort_item*			item;
	
	statushash = apr_hash_make(m_pool);
	exthash = apr_hash_make(m_pool);
	svn_revnum_t youngest = SVN_INVALID_REVNUM;
	svn_opt_revision_t rev;
	rev.kind = svn_opt_revision_unspecified;
	struct hashbaton_t hashbaton;
	hashbaton.hash = statushash;
	hashbaton.exthash = exthash;
	hashbaton.pThis = this;
	m_err = svn_client_status2 (&youngest,
							path.GetSVNApiPath(),
							&rev,
							getstatushash,
							&hashbaton,
							FALSE,		//descend
							TRUE,		//getall
							update,		//update
							noignore,		//noignore
							noexternals,
							ctx,
							m_pool);


	// Error present if function is not under version control
	if ((m_err != NULL) || (apr_hash_count(statushash) == 0))
	{
		status = NULL;
		return -2;	
	}

	// Convert the unordered hash to an ordered, sorted array
	statusarray = sort_hash (statushash,
							  sort_compare_items_as_paths,
							  m_pool);

	//only the first entry is needed (no recurse)
	item = &APR_ARRAY_IDX (statusarray, 0, const sort_item);
	
	status = (svn_wc_status2_t *) item->value;
	
	return youngest;
}
svn_wc_status2_t * SVNStatus::GetFirstFileStatus(const CTSVNPath& path, CTSVNPath& retPath, bool update, bool recurse, bool bNoIgnore /* = true */)
{
	const sort_item*			item;

	m_statushash = apr_hash_make(m_pool);
	m_externalhash = apr_hash_make(m_pool);
	headrev = SVN_INVALID_REVNUM;
	svn_opt_revision_t rev;
	rev.kind = svn_opt_revision_unspecified;
	struct hashbaton_t hashbaton;
	hashbaton.hash = m_statushash;
	hashbaton.exthash = m_externalhash;
	hashbaton.pThis = this;
	m_statushashindex = 0;
	m_err = svn_client_status2 (&headrev,
							path.GetSVNApiPath(),
							&rev,
							getstatushash,
							&hashbaton,
							recurse,	//descend
							TRUE,		//getall
							update,		//update
							bNoIgnore,	//noignore
							FALSE,		//noexternals
							ctx,
							m_pool);


	// Error present if function is not under version control
	if ((m_err != NULL) || (apr_hash_count(m_statushash) == 0))
	{
		return NULL;	
	}

	// Convert the unordered hash to an ordered, sorted array
	m_statusarray = sort_hash (m_statushash,
								sort_compare_items_as_paths,
								m_pool);

	//only the first entry is needed (no recurse)
	m_statushashindex = 0;
	item = &APR_ARRAY_IDX (m_statusarray, m_statushashindex, const sort_item);
	retPath.SetFromSVN((const char*)item->key);
	return (svn_wc_status2_t *) item->value;
}

unsigned int SVNStatus::GetVersionedCount()
{
	unsigned int count = 0;
	const sort_item* item;
	for (unsigned int i=0; i<apr_hash_count(m_statushash); ++i)
	{
		item = &APR_ARRAY_IDX(m_statusarray, i, const sort_item);
		if (item)
		{
			if (SVNStatus::GetMoreImportant(((svn_wc_status_t *)item->value)->text_status, svn_wc_status_ignored)!=svn_wc_status_ignored)
				count++;				
		}
	}
	return count;
}

svn_wc_status2_t * SVNStatus::GetNextFileStatus(CTSVNPath& retPath)
{
	const sort_item*			item;

	if ((m_statushashindex+1) >= apr_hash_count(m_statushash))
		return NULL;
	m_statushashindex++;

	item = &APR_ARRAY_IDX (m_statusarray, m_statushashindex, const sort_item);
	retPath.SetFromSVN((const char*)item->key);
	return (svn_wc_status2_t *) item->value;
}

bool SVNStatus::IsExternal(const CTSVNPath& path)
{
	if (apr_hash_get(m_externalhash, path.GetSVNApiPath(), APR_HASH_KEY_STRING))
		return true;
	return false;
}

void SVNStatus::GetStatusString(svn_wc_status_kind status, size_t buflen, TCHAR * string)
{
	TCHAR * buf;
	switch (status)
	{
		case svn_wc_status_none:
			buf = _T("none\0");
			break;
		case svn_wc_status_unversioned:
			buf = _T("unversioned\0");
			break;
		case svn_wc_status_normal:
			buf = _T("normal\0");
			break;
		case svn_wc_status_added:
			buf = _T("added\0");
			break;
		case svn_wc_status_missing:
			buf = _T("missing\0");
			break;
		case svn_wc_status_deleted:
			buf = _T("deleted\0");
			break;
		case svn_wc_status_replaced:
			buf = _T("replaced\0");
			break;
		case svn_wc_status_modified:
			buf = _T("modified\0");
			break;
		case svn_wc_status_merged:
			buf = _T("merged\0");
			break;
		case svn_wc_status_conflicted:
			buf = _T("conflicted\0");
			break;
		case svn_wc_status_obstructed:
			buf = _T("obstructed\0");
			break;
		case svn_wc_status_ignored:
			buf = _T("ignored");
			break;
		case svn_wc_status_external:
			buf = _T("external");
			break;
		case svn_wc_status_incomplete:
			buf = _T("incomplete\0");
			break;
		default:
			buf = _T("\0");
			break;
	} // switch (status) 
	_stprintf_s(string, buflen, _T("%s"), buf);
}

void SVNStatus::GetStatusString(HINSTANCE hInst, svn_wc_status_kind status, TCHAR * string, int size, WORD lang)
{
	switch (status)
	{
		case svn_wc_status_none:
			LoadStringEx(hInst, IDS_STATUSNONE, string, size, lang);
			break;
		case svn_wc_status_unversioned:
			LoadStringEx(hInst, IDS_STATUSUNVERSIONED, string, size, lang);
			break;
		case svn_wc_status_normal:
			LoadStringEx(hInst, IDS_STATUSNORMAL, string, size, lang);
			break;
		case svn_wc_status_added:
			LoadStringEx(hInst, IDS_STATUSADDED, string, size, lang);
			break;
		case svn_wc_status_missing:
			LoadStringEx(hInst, IDS_STATUSABSENT, string, size, lang);
			break;
		case svn_wc_status_deleted:
			LoadStringEx(hInst, IDS_STATUSDELETED, string, size, lang);
			break;
		case svn_wc_status_replaced:
			LoadStringEx(hInst, IDS_STATUSREPLACED, string, size, lang);
			break;
		case svn_wc_status_modified:
			LoadStringEx(hInst, IDS_STATUSMODIFIED, string, size, lang);
			break;
		case svn_wc_status_merged:
			LoadStringEx(hInst, IDS_STATUSMERGED, string, size, lang);
			break;
		case svn_wc_status_conflicted:
			LoadStringEx(hInst, IDS_STATUSCONFLICTED, string, size, lang);
			break;
		case svn_wc_status_ignored:
			LoadStringEx(hInst, IDS_STATUSIGNORED, string, size, lang);
			break;
		case svn_wc_status_obstructed:
			LoadStringEx(hInst, IDS_STATUSOBSTRUCTED, string, size, lang);
			break;
		case svn_wc_status_external:
			LoadStringEx(hInst, IDS_STATUSEXTERNAL, string, size, lang);
			break;
		case svn_wc_status_incomplete:
			LoadStringEx(hInst, IDS_STATUSINCOMPLETE, string, size, lang);
			break;
		default:
			LoadStringEx(hInst, IDS_STATUSNONE, string, size, lang);
			break;
	} // switch (status) 
}

int SVNStatus::LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
	const STRINGRESOURCEIMAGE* pImage;
	const STRINGRESOURCEIMAGE* pImageEnd;
	ULONG nResourceSize;
	HGLOBAL hGlobal;
	UINT iIndex;
	int ret;

	HRSRC hResource =  FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(((uID>>4)+1)), wLanguage);
	if (!hResource)
	{
		//try the default language before giving up!
		hResource = FindResource(hInstance, MAKEINTRESOURCE(((uID>>4)+1)), RT_STRING);
		if (!hResource)
			return 0;
	}
	hGlobal = LoadResource(hInstance, hResource);
	if (!hGlobal)
		return 0;
	pImage = (const STRINGRESOURCEIMAGE*)::LockResource(hGlobal);
	if(!pImage)
		return 0;

	nResourceSize = ::SizeofResource(hInstance, hResource);
	pImageEnd = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+nResourceSize);
	iIndex = uID&0x000f;

	while ((iIndex > 0) && (pImage < pImageEnd))
	{
		pImage = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+(sizeof(STRINGRESOURCEIMAGE)+(pImage->nLength*sizeof(WCHAR))));
		iIndex--;
	}
	if (pImage >= pImageEnd)
		return 0;
	if (pImage->nLength == 0)
		return 0;

	ret = pImage->nLength;
	if (pImage->nLength > nBufferMax)
	{
		wcsncpy_s(lpBuffer, nBufferMax, pImage->achString, pImage->nLength-1);
		lpBuffer[nBufferMax-1] = 0;
	}
	else
	{
		wcsncpy_s(lpBuffer, nBufferMax, pImage->achString, pImage->nLength);
		lpBuffer[ret] = 0;
	}
	return ret;
}

void SVNStatus::getallstatus(void * baton, const char * /*path*/, svn_wc_status2_t * status)
{
	svn_wc_status_kind * s = (svn_wc_status_kind *)baton;
	*s = SVNStatus::GetMoreImportant(*s, status->text_status);
	*s = SVNStatus::GetMoreImportant(*s, status->prop_status);
}

void SVNStatus::getstatushash(void * baton, const char * path, svn_wc_status2_t * status)
{
	hashbaton_t * hash = (hashbaton_t *)baton;
	const StdStrAVector& filterList = hash->pThis->m_filterFileList;
	if (status->text_status == svn_wc_status_external)
	{
		apr_hash_set (hash->exthash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, (const void*)1);
		return;
	}
	if(filterList.size() > 0)
	{
		// We have a filter active - we're only interested in files which are in 
		// the filter  
		if(!binary_search(filterList.begin(), filterList.end(), path))
		{
			// This item is not in the filter - don't store it
			return;
		}
	}
	svn_wc_status2_t * statuscopy = svn_wc_dup_status2 (status, hash->pThis->m_pool);
	apr_hash_set (hash->hash, apr_pstrdup(hash->pThis->m_pool, path), APR_HASH_KEY_STRING, statuscopy);
}

apr_array_header_t * SVNStatus::sort_hash (apr_hash_t *ht,
										int (*comparison_func) (const SVNStatus::sort_item *, const SVNStatus::sort_item *),
										apr_pool_t *pool)
{
	apr_hash_index_t *hi;
	apr_array_header_t *ary;

	/* allocate an array with only one element to begin with. */
	ary = apr_array_make (pool, 1, sizeof(sort_item));

	/* loop over hash table and push all keys into the array */
	for (hi = apr_hash_first (pool, ht); hi; hi = apr_hash_next (hi))
	{
		sort_item *item = (sort_item*)apr_array_push (ary);

		apr_hash_this (hi, &item->key, &item->klen, &item->value);
	}

	/* now quicksort the array.  */
	qsort (ary->elts, ary->nelts, ary->elt_size,
		(int (*)(const void *, const void *))comparison_func);

	return ary;
}

int SVNStatus::sort_compare_items_as_paths (const sort_item *a, const sort_item *b)
{
	const char *astr, *bstr;

	astr = (const char*)a->key;
	bstr = (const char*)b->key;
	return svn_path_compare_paths (astr, bstr);
}

svn_error_t* SVNStatus::cancel(void *baton)
{
	volatile bool * canceled = (bool *)baton;
	if (*canceled)
	{
		CStringA temp;
		temp.LoadString(IDS_SVN_USERCANCELLED);
		return svn_error_create(SVN_ERR_CANCELLED, NULL, temp);
	}
	return SVN_NO_ERROR;
}

#ifdef _MFC_VER

// Set-up a filter to restrict the files which will have their status stored by a get-status
void SVNStatus::SetFilter(const CTSVNPathList& fileList)
{
	m_filterFileList.clear();
	for(int fileIndex = 0; fileIndex < fileList.GetCount(); fileIndex++)
	{
		m_filterFileList.push_back(fileList[fileIndex].GetSVNApiPath());
	}
	// Sort the list so that we can do binary searches
	std::sort(m_filterFileList.begin(), m_filterFileList.end());
}

void SVNStatus::ClearFilter()
{
	m_filterFileList.clear();
}

#endif // _MFC_VER

