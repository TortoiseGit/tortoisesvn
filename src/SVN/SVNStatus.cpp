// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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

#include "stdafx.h"
#include "resource.h"

#include "SVNStatus.h"
#include "UnicodeUtils.h"

SVNStatus::SVNStatus(void)
{
	apr_initialize();
	m_pool = svn_pool_create (NULL);				// create the memory pool
	svn_config_ensure(m_pool);

	memset (&m_ctx, 0, sizeof (m_ctx));

	// set up authentication
	svn_auth_provider_object_t *provider;

	/* The whole list of registered providers */
	apr_array_header_t *providers = apr_array_make (m_pool, 10, sizeof (svn_auth_provider_object_t *));

	/* The main disk-caching auth providers, for both
	'username/password' creds and 'username' creds.  */
	svn_client_get_simple_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_username_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* The server-cert, client-cert, and client-cert-password providers. */
	svn_client_get_ssl_server_file_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_client_file_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_pw_file_provider (&provider, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
#ifdef _MFC_VER
	/* Two prompting providers, one for username/password, one for
	just username. */
	svn_client_get_simple_prompt_provider (&provider, (svn_client_prompt_t)prompt, this, 2, /* retry limit */ m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_username_prompt_provider (&provider, (svn_client_prompt_t)prompt, this, 2, /* retry limit */ m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Three prompting providers for server-certs, client-certs,
	and client-cert-passphrases.  */
	svn_client_get_ssl_server_prompt_provider (&provider, (svn_client_prompt_t)prompt, this, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_client_prompt_provider (&provider, (svn_client_prompt_t)prompt, this, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_pw_prompt_provider (&provider, (svn_client_prompt_t)prompt, this, m_pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
#endif
	/* Build an authentication baton to give to libsvn_client. */
	svn_auth_open (&m_auth_baton, providers, m_pool);
	m_ctx.auth_baton = m_auth_baton;

#ifdef _MFC_VER
	m_ctx.prompt_func = (svn_client_prompt_t)prompt;
	m_ctx.prompt_baton = this;
#endif
	// set up the configuration
	svn_config_get_config (&(m_ctx.config), m_pool);
}

SVNStatus::~SVNStatus(void)
{
	svn_pool_destroy (m_pool);					// free the allocated memory
	apr_terminate();
}

#ifdef _MFC_VER
svn_error_t* SVNStatus::prompt(char **info, const char *prompt, svn_boolean_t hide, void *baton, apr_pool_t *pool)
{
	SVNStatus * svnstatus = (SVNStatus *)baton;
	CString infostring;
	if (svnstatus->Prompt(infostring, CString(prompt), hide))
	{
		SVN_ERR (svn_utf_cstring_to_utf8 ((const char **)info, CUnicodeUtils::GetUTF8(infostring), NULL, pool));
		return SVN_NO_ERROR;
	}
	CStringA temp;
	temp.LoadString(IDS_SVN_USERCANCELLED);
	return svn_error_create(SVN_ERR_CANCELLED, NULL, temp);
}

BOOL SVNStatus::Prompt(CString& info, CString prompt, BOOL hide) 
{
	CPromptDlg dlg;
	dlg.m_info = prompt;
	dlg.m_sPass = info;
	
	dlg.SetHide(hide);

	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		info = dlg.m_sPass;
		if (dlg.m_saveCheck)
		{
			svn_auth_set_parameter(m_ctx.auth_baton, SVN_AUTH_PARAM_NO_AUTH_CACHE, NULL);
		}
		else
		{
			svn_auth_set_parameter(m_ctx.auth_baton, SVN_AUTH_PARAM_NO_AUTH_CACHE, (void *) "");
		}
		return TRUE;
	}
	if (nResponse == IDABORT)
	{
		//the prompt dialog box could not be shown!
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION );
		// Free the buffer.
		LocalFree( lpMsgBuf );
	} // if (nResponse == IDABORT)
	return FALSE;
}
#endif //_MFC_VER


//static method
svn_wc_status_kind SVNStatus::GetTextStatus(const TCHAR * path)
{
	svn_wc_status_t *			status;
	apr_pool_t *				pool;
	svn_error_t *				err;
	const char *				internalpath;

	TCHAR						pathbuf[MAX_PATH];
	_tcscpy(pathbuf, path);
	if (!PathIsDirectory(path))
	{
		TCHAR * ptr = _tcsrchr(pathbuf, '\\');
		if (ptr == 0)
			ptr = _tcsrchr(pathbuf, '/');
		if (ptr == 0)
			return svn_wc_status_unversioned;
		*ptr = 0;
	}
	_tcscat(pathbuf, _T("\\.svn"));
	if (!PathFileExists(pathbuf))
		return svn_wc_status_unversioned;

	apr_initialize();
	pool = svn_pool_create (NULL);				// create the memory pool

	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(path).c_str(), pool);

	svn_revnum_t			youngest;
	youngest = SVN_INVALID_REVNUM;				//always get status from newest revision



	svn_wc_adm_access_t *adm_access;

	// Need to lock the tree as even a non-recursive status requires the
	// immediate directories to be locked. 
	err = svn_wc_adm_probe_open (&adm_access, NULL, internalpath, FALSE, FALSE, pool);

	if (err != NULL)
	{
		svn_pool_destroy (pool);				//free allocated memory
		apr_terminate();
		return svn_wc_status_unversioned;		//error occured, so treat entry as unversioned
	}
	// Ask the wc to give us a list of svn_wc_status_t structures.
	// These structures contain nothing but information found in the
	// working copy. 
	err = svn_wc_status (&status, internalpath, adm_access, pool);
	if (err != NULL)
	{
		svn_wc_adm_close (adm_access);			//unlock the directory
		svn_pool_destroy (pool);				//free allocated memory
		apr_terminate();
		return svn_wc_status_unversioned;		//error occured, so treat entry as unversioned
	}

	err = svn_wc_adm_close (adm_access);
	if (err != NULL)
	{
		svn_pool_destroy (pool);				//free allocated memory
		apr_terminate();
		return svn_wc_status_unversioned;		//error occured, so treat entry as unversioned
	}

	svn_wc_status_kind text = status->text_status;
	svn_pool_destroy(pool);
	apr_terminate();
	return text;
}

//static method
svn_wc_status_kind SVNStatus::GetTextStatusRecursive(const TCHAR * path)
{
	svn_auth_baton_t *			auth_baton;
	svn_client_ctx_t 			ctx;
	apr_hash_t *				statushash;
	svn_wc_status_kind			statuskind;
	apr_pool_t *				pool;
	svn_error_t *				err;
	const char *				internalpath;

	TCHAR						pathbuf[MAX_PATH];
	_tcscpy(pathbuf, path);
	if (!PathIsDirectory(path))
	{
		TCHAR * ptr = _tcsrchr(pathbuf, '\\');
		if (ptr == 0)
			ptr = _tcsrchr(pathbuf, '/');
		if (ptr == 0)
			return svn_wc_status_unversioned;
		*ptr = 0;
	}
	_tcscat(pathbuf, _T("\\.svn"));
	if (!PathFileExists(pathbuf))
		return svn_wc_status_unversioned;

	apr_initialize();
	pool = svn_pool_create (NULL);				// create the memory pool
	memset (&ctx, 0, sizeof (ctx));
	svn_config_ensure(pool);

	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(path).c_str(), pool);

	// set up authentication

    /* The whole list of registered providers */
    apr_array_header_t *providers
      = apr_array_make (pool, 1, sizeof (svn_auth_provider_object_t *));

    /* The main disk-caching auth providers, for both
       'username/password' creds and 'username' creds.  */
    svn_auth_provider_object_t *simple_wc_provider = (svn_auth_provider_object_t *)apr_pcalloc (pool, sizeof(*simple_wc_provider));

    svn_auth_provider_object_t *username_wc_provider = (svn_auth_provider_object_t *)apr_pcalloc (pool, sizeof(*username_wc_provider));

    svn_client_get_simple_provider (&(simple_wc_provider), pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = simple_wc_provider;

    svn_client_get_username_provider(&(username_wc_provider), pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = username_wc_provider;

	svn_auth_open (&auth_baton, providers, pool);

	// set up the configuration
	svn_config_get_config (&(ctx.config), pool);

	ctx.auth_baton = auth_baton;

	svn_revnum_t			youngest;
	youngest = SVN_INVALID_REVNUM;				//always get status from newest revision
	err = svn_client_status (&statushash,
							&youngest,
							internalpath,
							TRUE,				//descend to subitems
							1,
							false,				//don't update with repository
							1,
							&ctx,
							pool);

	// Error present if function is not under version control
	if ((err != NULL) || (apr_hash_count(statushash) == 0))
	{
		svn_pool_destroy (pool);				//free allocated memory
		return svn_wc_status_unversioned;	
	}

    apr_hash_index_t *hi;
	statuskind = svn_wc_status_unversioned;
    for (hi = apr_hash_first (pool, statushash); hi; hi = apr_hash_next (hi))
    {
		svn_wc_status_t * tempstatus;
		apr_hash_this(hi, NULL, NULL, (void **)&tempstatus);
		//check if this status has higher priority
		//to keep things easy the higher priority is also the higher enum value...
		if ((tempstatus->text_status > statuskind)&&(tempstatus->text_status != svn_wc_status_ignored))
		{
			statuskind = tempstatus->text_status;
		}
	}
	svn_pool_destroy (pool);				//free allocated memory
	apr_terminate();
	return statuskind;
}

//static method
svn_wc_status_kind SVNStatus::GetAllStatus(const TCHAR * path)
{
	svn_wc_status_t *			status;
	apr_pool_t *				pool;
	svn_error_t *				err;
	const char *				internalpath;

	TCHAR						pathbuf[MAX_PATH];
	_tcscpy(pathbuf, path);
	if (!PathIsDirectory(path))
	{
		TCHAR * ptr = _tcsrchr(pathbuf, '\\');
		if (ptr == 0)
			ptr = _tcsrchr(pathbuf, '/');
		if (ptr == 0)
			return svn_wc_status_unversioned;
		*ptr = 0;
	}
	_tcscat(pathbuf, _T("\\.svn"));
	if (!PathFileExists(pathbuf))
		return svn_wc_status_unversioned;

	apr_initialize();
	pool = svn_pool_create (NULL);				// create the memory pool

	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(path).c_str(), pool);

	svn_revnum_t			youngest;
	youngest = SVN_INVALID_REVNUM;				//always get status from newest revision


	svn_wc_adm_access_t *adm_access;

	// Need to lock the tree as even a non-recursive status requires the
	// immediate directories to be locked. 
	err = svn_wc_adm_probe_open (&adm_access, NULL, internalpath, FALSE, FALSE, pool);

	if (err != NULL)
	{
		svn_pool_destroy (pool);				//free allocated memory
		apr_terminate();
		return svn_wc_status_unversioned;		//error occured, so treat entry as unversioned
	}
	// Ask the wc to give us a list of svn_wc_status_t structures.
	// These structures contain nothing but information found in the
	// working copy. 
	err = svn_wc_status (&status, internalpath, adm_access, pool);
	if (err != NULL)
	{
		svn_wc_adm_close (adm_access);			//unlock the directory
		svn_pool_destroy (pool);				//free allocated memory
		apr_terminate();
		return svn_wc_status_unversioned;		//error occured, so treat entry as unversioned
	}

	err = svn_wc_adm_close (adm_access);
	if (err != NULL)
	{
		svn_pool_destroy (pool);				//free allocated memory
		apr_terminate();
		return svn_wc_status_unversioned;		//error occured, so treat entry as unversioned
	}

	svn_wc_status_kind text = status->text_status > status->prop_status ? status->text_status : status->prop_status;
	if (text == svn_wc_status_none)
		text = svn_wc_status_unversioned;
	svn_pool_destroy(pool);
	apr_terminate();
	return text;
}

//static method
svn_wc_status_kind SVNStatus::GetAllStatusRecursive(const TCHAR * path)
{
	svn_auth_baton_t *			auth_baton;
	svn_client_ctx_t 			ctx;
	apr_hash_t *				statushash;
	svn_wc_status_kind			statuskind;
	apr_pool_t *				pool;
	svn_error_t *				err;
	const char *				internalpath;

	TCHAR						pathbuf[MAX_PATH];
	_tcscpy(pathbuf, path);
	if (!PathIsDirectory(path))
	{
		TCHAR * ptr = _tcsrchr(pathbuf, '\\');
		if (ptr == 0)
			ptr = _tcsrchr(pathbuf, '/');
		if (ptr == 0)
			return svn_wc_status_unversioned;
		*ptr = 0;
	}
	_tcscat(pathbuf, _T("\\.svn"));
	if (!PathFileExists(pathbuf))
		return svn_wc_status_unversioned;

	apr_initialize();
	pool = svn_pool_create (NULL);				// create the memory pool
	memset (&ctx, 0, sizeof (ctx));
	svn_config_ensure(pool);

	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(path).c_str(), pool);

	// set up authentication

    /* The whole list of registered providers */
    apr_array_header_t *providers
      = apr_array_make (pool, 1, sizeof (svn_auth_provider_object_t *));

    /* The main disk-caching auth providers, for both
       'username/password' creds and 'username' creds.  */
    svn_auth_provider_object_t *simple_wc_provider = (svn_auth_provider_object_t *)apr_pcalloc (pool, sizeof(*simple_wc_provider));

    svn_auth_provider_object_t *username_wc_provider = (svn_auth_provider_object_t *)apr_pcalloc (pool, sizeof(*username_wc_provider));

    svn_client_get_simple_provider (&(simple_wc_provider), pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = simple_wc_provider;

    svn_client_get_username_provider(&(username_wc_provider), pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = username_wc_provider;

	svn_auth_open (&auth_baton, providers, pool);

	// set up the configuration
	svn_config_get_config (&(ctx.config), pool);

	ctx.auth_baton = auth_baton;

	svn_revnum_t			youngest;
	youngest = SVN_INVALID_REVNUM;				//always get status from newest revision
	err = svn_client_status (&statushash,
							&youngest,
							internalpath,
							TRUE,				//descend to subitems
							1,
							false,				//don't update with repository
							1,
							&ctx,
							pool);

	// Error present if function is not under version control
	if ((err != NULL) || (apr_hash_count(statushash) == 0))
	{
		svn_pool_destroy (pool);				//free allocated memory
		return svn_wc_status_unversioned;	
	}

    apr_hash_index_t *hi;
	statuskind = svn_wc_status_unversioned;
    for (hi = apr_hash_first (pool, statushash); hi; hi = apr_hash_next (hi))
    {
		svn_wc_status_t * tempstatus;
		apr_hash_this(hi, NULL, NULL, (void **)&tempstatus);
		//check if this status has higher priority
		//to keep things easy the higher priority is also the higher enum value...
		if ((tempstatus->text_status > statuskind)&&(tempstatus->text_status != svn_wc_status_ignored))
		{
			statuskind = tempstatus->text_status;
		}
		if ((tempstatus->prop_status > statuskind)&&(tempstatus->prop_status != svn_wc_status_ignored))
		{
			statuskind = tempstatus->prop_status;
		}
	}
	svn_pool_destroy (pool);				//free allocated memory
	apr_terminate();
	return statuskind;
}

svn_revnum_t SVNStatus::GetStatus(const TCHAR * path, bool update /* = false */)
{
	apr_hash_t *				statushash;
	apr_array_header_t *		statusarray;
	const svn_item_t*			item;
	svn_error_t *				err;
	const char *				internalpath;

	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(path).c_str(), m_pool);


	svn_revnum_t			youngest;
	youngest = SVN_INVALID_REVNUM;				//always get status from newest revision

	err = svn_client_status (&statushash,
							&youngest,
							internalpath,
							0,
							1,
							update,
							1,
							&m_ctx,
							m_pool);

	// Error present if function is not under version control
	if ((err != NULL) || (apr_hash_count(statushash) == 0))
	{
		return -2;	
	}

	// Convert the unordered hash to an ordered, sorted array
	statusarray = apr_hash_sorted_keys (statushash,
										svn_sort_compare_items_as_paths,
										m_pool);

	//only the first entry is needed (no recurse)
	item = &APR_ARRAY_IDX (statusarray, 0, const svn_item_t);
	
	status = (svn_wc_status_t *) item->value;

	
	return youngest;
}
svn_wc_status_t * SVNStatus::GetFirstFileStatus(const TCHAR * path, const TCHAR ** retPath, bool update)
{
	const svn_item_t*			item;
	svn_error_t *				err;
	const char *				internalpath;

	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(path).c_str(), m_pool);


	svn_revnum_t			youngest;
	youngest = SVN_INVALID_REVNUM;				//always get status from newest revision

	err = svn_client_status (&m_statushash,
							&youngest,
							internalpath,
							1,					//recurse
							1,					//getall
							update,
							1,					//no ignore
							&m_ctx,
							m_pool);

	// Error present if function is not under version control
	if ((err != NULL) || (apr_hash_count(m_statushash) == 0))
	{
		return NULL;	
	}

	// Convert the unordered hash to an ordered, sorted array
	m_statusarray = apr_hash_sorted_keys (m_statushash,
											svn_sort_compare_items_as_paths,
											m_pool);

	//only the first entry is needed (no recurse)
	m_statushashindex = 0;
	item = &APR_ARRAY_IDX (m_statusarray, m_statushashindex, const svn_item_t);
#ifdef UNICODE
	int len = MultiByteToWideChar(CP_UTF8, 0, (const char *)item->key, -1, 0, 0);
	*retPath = (const TCHAR *)apr_palloc(m_pool, len * sizeof(TCHAR));
	MultiByteToWideChar(CP_UTF8, 0, (const char *)item->key, -1, (LPWSTR)*retPath, len);
#else
	if (svn_utf_cstring_from_utf8 (retPath, (const char *)item->key, m_pool) != NULL)
		return NULL;
#endif
	return (svn_wc_status_t *) item->value;
}

svn_wc_status_t * SVNStatus::GetNextFileStatus(const TCHAR ** retPath)
{
	const svn_item_t*			item;

	if ((m_statushashindex+1) == apr_hash_count(m_statushash))
		return NULL;
	m_statushashindex++;

	item = &APR_ARRAY_IDX (m_statusarray, m_statushashindex, const svn_item_t);
#ifdef UNICODE
	int len = MultiByteToWideChar(CP_UTF8, 0, (const char *)item->key, -1, 0, 0);
	*retPath = (const TCHAR *)apr_palloc(m_pool, len * sizeof(TCHAR));
	MultiByteToWideChar(CP_UTF8, 0, (const char *)item->key, -1, (LPWSTR)*retPath, len);
#else
	if (svn_utf_cstring_from_utf8 (retPath, (const char *)item->key, m_pool) != NULL)
		return NULL;
#endif	
	return (svn_wc_status_t *) item->value;
}

void SVNStatus::GetStatusString(svn_wc_status_kind status, TCHAR * string)
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
		case svn_wc_status_absent:
			buf = _T("absent\0");
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
			buf = _T("conflicted\0");
			break;
		default:
			buf = _T("\0");
			break;
	}
	_stprintf(string, _T("%s"), buf);
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
		case svn_wc_status_absent:
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
		case svn_wc_status_obstructed:
			LoadStringEx(hInst, IDS_STATUSOBSTRUCTED, string, size, lang);
			break;
		default:
			LoadStringEx(hInst, IDS_STATUSNONE, string, size, lang);
			break;
	}
}

int SVNStatus::LoadStringEx(HINSTANCE hInstance, UINT uID, LPCTSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
	const STRINGRESOURCEIMAGE* pImage;
	const STRINGRESOURCEIMAGE* pImageEnd;
	ULONG nResourceSize;
	HGLOBAL hGlobal;
	UINT iIndex;
#ifndef UNICODE
	BOOL defaultCharUsed;
#endif
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

#ifdef UNICODE
	ret = pImage->nLength;
	wcsncpy((wchar_t *)lpBuffer, pImage->achString, pImage->nLength);
	(wchar_t)lpBuffer[ret] = 0;
#else
	ret = WideCharToMultiByte(CP_ACP, 0, pImage->achString, pImage->nLength, (LPSTR)lpBuffer, nBufferMax-1, ".", &defaultCharUsed);
	(TCHAR)lpBuffer[ret] = 0;
#endif
	return ret;
}

SVNFolderStatus::SVNFolderStatus(void)
{
	m_pStatusCache = NULL;
}

SVNFolderStatus::~SVNFolderStatus(void)
{
	if (m_pStatusCache != NULL)
		delete[] m_pStatusCache;
}

void SVNFolderStatus::BuildCache(LPCTSTR folderpath)
{
	svn_client_ctx_t 			ctx;
	apr_hash_t *				statushash;
	apr_pool_t *				pool;
	svn_error_t *				err;
	const char *				internalpath;
	m_nCacheCount = 0;
	apr_initialize();
	pool = svn_pool_create (NULL);				// create the memory pool
	memset (&ctx, 0, sizeof (ctx));
	memset (m_currentfolder, 0, sizeof(m_currentfolder));
	//we need to convert the path to subversion internal format
	//the internal format uses '/' instead of the windows '\'
	internalpath = svn_path_internal_style (CUnicodeUtils::StdGetUTF8(folderpath).c_str(), pool);

	_tcscpy(m_currentfolder, CUnicodeUtils::StdGetUnicode(internalpath).c_str());


	ctx.auth_baton = NULL;

	svn_revnum_t			youngest;
	youngest = SVN_INVALID_REVNUM;				//always get status from newest revision
	err = svn_client_status (&statushash,
							&youngest,
							internalpath,
							FALSE,				//don't descend to subitems
							1,
							false,				//don't update with repository
							1,
							&ctx,
							pool);

	// Error present if function is not under version control
	if ((err != NULL) || (apr_hash_count(statushash) == 0))
	{
		svn_pool_destroy (pool);				//free allocated memory
		return;	
	}

    apr_hash_index_t *hi;
	if (m_pStatusCache != NULL)
		delete[] m_pStatusCache;
	m_nCacheCount = apr_hash_count(statushash);
	m_pStatusCache = new filestatuscache[m_nCacheCount];
	int i=0;
    for (hi = apr_hash_first (pool, statushash); hi; hi = apr_hash_next (hi))
    {
		svn_wc_status_t * tempstatus;
		apr_hash_this(hi, NULL, NULL, (void **)&tempstatus);
		m_pStatusCache[i].status = svn_wc_status_unversioned;
		//check if this status has higher priority
		//to keep things easy the higher priority is also the higher enum value...
		if (m_pStatusCache[i].status < tempstatus->text_status)
			m_pStatusCache[i].status = tempstatus->text_status;
		if (m_pStatusCache[i].status < tempstatus->prop_status)
			m_pStatusCache[i].status = tempstatus->prop_status;
		m_pStatusCache[i].askedcounter = SVNFOLDERSTATUS_CACHETIMES;
		if (tempstatus->entry)
		{
			if (tempstatus->entry->kind == svn_node_dir)
			{
				if (tempstatus->entry->url)
					_tcscpy(m_pStatusCache[i].filename, _tcsrchr(CUnicodeUtils::StdGetUnicode(tempstatus->entry->url).c_str(), _T('/')) + sizeof(TCHAR));
				else
				{
					_tcscpy(m_pStatusCache[i].filename, _T(" "));
					m_pStatusCache[i].status = svn_wc_status_unversioned;
				}
			}
			else
			{
				if (tempstatus->entry->name)
					_tcscpy(m_pStatusCache[i].filename, CUnicodeUtils::StdGetUnicode(tempstatus->entry->name).c_str());
				else
				{
					_tcscpy(m_pStatusCache[i].filename, _T(" "));
					m_pStatusCache[i].status = svn_wc_status_unversioned;
				}
			}
			i++;
		}
	}
	svn_pool_destroy (pool);				//free allocated memory
	apr_terminate();
}

int SVNFolderStatus::FindFile(LPCTSTR filename)
{
	for (int i=0; i<m_nCacheCount; i++)
	{
		if (_tcsicmp(filename, m_pStatusCache[i].filename) == 0)
			return i;
	} // for (int i=0; i<m_nCacheCount; i++)
	return -1;		//not found!
}

BOOL SVNFolderStatus::IsCacheValid(LPCTSTR filename)
{
	int i = FindFile(filename);
	if (i>=0)
	{
		if (m_pStatusCache[i].askedcounter > 0)
			return TRUE;
	} // if (i>=0)
	return FALSE;
}

svn_wc_status_kind SVNFolderStatus::GetFileStatus(LPCTSTR filepath)
{
	TCHAR * filename;
	TCHAR * filepathnonconst = (LPTSTR)filepath;
	//first change the filename to 'internal' format
	for (UINT i=0; i<_tcsclen(filepath); i++)
	{
		if (filepath[i] == _T('\\'))
			filepathnonconst[i] = _T('/');
	} // for (int i=0; i<_tcsclen(filename); i++)
	filename = _tcsrchr(filepath, _T('/')) + 1;
	//check if the file is in the current folder
	if (_tcsnicmp(filepath, m_currentfolder, (filename-filepath)-1) == 0)
	{
		//now check if the cache is still valid
		if (m_pStatusCache != NULL)
		{
			for (int j=0; j<m_nCacheCount; j++)
			{
				if (_tcsicmp(filename, m_pStatusCache[j].filename) == 0)
				{
					if (m_pStatusCache[j].askedcounter--)
						return m_pStatusCache[j].status;
					break;
				}
			}
		} // if (m_pStatusCache != NULL)
	} // if (_tcsnicmp(filepath, m_currentfolder, (filepath - filename)) == 0)
	//we don't build the cache for directories. This
	//prevents us from building the cache for items in the folder view.
	if (PathIsDirectory(filepath))
		return SVNStatus::GetAllStatus(filepath);		//don't build the cache for dirs
	//also, don't build the cache for unversioned items
	if (SVNStatus::GetAllStatus(filepath) == svn_wc_status_unversioned)
		return svn_wc_status_unversioned;
	TCHAR buildpath[MAX_PATH] = {0};
	_tcsncpy(buildpath, filepath, filename-filepath);
	BuildCache(buildpath);
	for (int k=0; k<m_nCacheCount; k++)
	{
		if (_tcsicmp(filename, m_pStatusCache[k].filename) == 0)
			return m_pStatusCache[k].status;
	} // for (int k=0; k<m_nCacheCount; k++)
	return svn_wc_status_unversioned;
}
