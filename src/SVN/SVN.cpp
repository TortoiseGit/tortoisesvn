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
//

#include "StdAfx.h"
#include "TortoiseProc.h"
#include "svn.h"
#include "UnicodeUtils.h"
#include <shlwapi.h>
#include "DirFileList.h"

SVN::SVN(void)
{
	m_app = NULL;
	hWnd = NULL;
	apr_initialize();
	memset (&ctx, 0, sizeof (ctx));
	parentpool = svn_pool_create(NULL);
	Err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	// set up the configuration
	if (Err == 0)
		Err = svn_config_get_config (&(ctx.config), NULL, pool);

	m_username = NULL;
	m_password = NULL;

	// set up authentication

	svn_auth_provider_object_t *provider;

	/* The whole list of registered providers */
	apr_array_header_t *providers = apr_array_make (pool, 11, sizeof (svn_auth_provider_object_t *));

	/* our own caching provider */
	get_simple_provider(&provider, pool);
	APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

	/* The main disk-caching auth providers, for both
	'username/password' creds and 'username' creds.  */
	svn_client_get_simple_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_username_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* The server-cert, client-cert, and client-cert-password providers. */
	svn_client_get_ssl_server_file_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_client_file_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_pw_file_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Two prompting providers, one for username/password, one for
	just username. */
	svn_client_get_simple_prompt_provider (&provider, (svn_auth_simple_prompt_func_t)simpleprompt, this, 2, /* retry limit */ pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_username_prompt_provider (&provider, (svn_auth_username_prompt_func_t)userprompt, this, 2, /* retry limit */ pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Three prompting providers for server-certs, client-certs,
	and client-cert-passphrases.  */
	svn_client_get_ssl_server_prompt_provider (&provider, (svn_auth_ssl_server_prompt_func_t)sslserverprompt, this, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_client_prompt_provider (&provider, (svn_auth_ssl_client_prompt_func_t)sslclientprompt, this, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_client_get_ssl_pw_prompt_provider (&provider, (svn_auth_ssl_pw_prompt_func_t)sslpwprompt, this, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Build an authentication baton to give to libsvn_client. */
	svn_auth_open (&auth_baton, providers, pool);
	ctx.auth_baton = auth_baton;

	//svn_auth_open (&auth_baton, providers, pool);

	//ctx.auth_baton = auth_baton;
	//ctx.prompt_func = (svn_client_prompt_t)prompt;
	//ctx.prompt_baton = this;
	ctx.log_msg_func = svn_cl__get_log_message;
	ctx.log_msg_baton = logMessage("");
	ctx.notify_func = notify;
	ctx.notify_baton = this;
	ctx.cancel_func = cancel;
	ctx.cancel_baton = this;

}

SVN::~SVN(void)
{
	svn_pool_destroy (pool);
	svn_pool_destroy (parentpool);
	apr_terminate();
}

void SVN::SaveAuthentication(BOOL save)
{
	if (save)
	{
		svn_auth_set_parameter(ctx.auth_baton, SVN_AUTH_PARAM_NO_AUTH_CACHE, NULL);
	}
	else
	{
		svn_auth_set_parameter(ctx.auth_baton, SVN_AUTH_PARAM_NO_AUTH_CACHE, (void *) "");
	}
}

BOOL SVN::Cancel() {return FALSE;};
BOOL SVN::Notify(CString path, svn_wc_notify_action_t action, svn_node_kind_t kind, CString myme_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, LONG rev) {return TRUE;};
BOOL SVN::Log(LONG rev, CString author, CString date, CString message, CString& cpaths) {return TRUE;};

struct log_msg_baton
{
  const char *message;  /* the message. */
  const char *message_encoding; /* the locale/encoding of the message. */
  const char *base_dir; /* the base directory for an external edit. UTF-8! */
  const char *tmpfile_left; /* the tmpfile left by an external edit. UTF-8! */
  apr_pool_t *pool; /* a pool. */
};

CString SVN::GetLastErrorMessage()
{
	CString msg;
	if (Err != NULL)
	{
		msg = Err->message;
		while (Err->child)
		{
			Err = Err->child;
			msg += "\n";
			msg += Err->message;
		}
		return msg;
	}
	return _T("");
}

BOOL SVN::Checkout(CString moduleName, CString destPath, LONG revision, BOOL recurse)
{
	preparePath(destPath);
	preparePath(moduleName);

	Err = svn_client_checkout (	CUnicodeUtils::GetUTF8(moduleName),
								CUnicodeUtils::GetUTF8(destPath),
								getRevision (revision),
								recurse,
								&ctx,
								pool );

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(destPath);

	return TRUE;
}

BOOL SVN::Remove(CString path, BOOL force)
{
	preparePath(path);

	svn_client_commit_info_t *commit_info = NULL;

	Err = svn_client_delete (&commit_info, target(path), force,
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}

	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Revert(CString path, BOOL recurse)
{
	preparePath(path);
	Err = svn_client_revert (CUnicodeUtils::GetUTF8(path), recurse, &ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}


BOOL SVN::Add(CString path, BOOL recurse)
{
	preparePath(path);
	Err = svn_client_add (CUnicodeUtils::GetUTF8(path), recurse, &ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Update(CString path, LONG revision, BOOL recurse)
{
	preparePath(path);
	Err = svn_client_update (CUnicodeUtils::GetUTF8(path),
							getRevision (revision),
							recurse,
							&ctx,
							pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}

LONG SVN::Commit(CString path, CString message, BOOL recurse)
{
	preparePath(path);
	svn_client_commit_info_t *commit_info = NULL;

	ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_commit (&commit_info, 
							target ((LPCTSTR)path), 
							!recurse,
							&ctx,
							pool);
	ctx.log_msg_baton = logMessage("");
	if(Err != NULL)
	{
		return 0;
	}
	
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);

	if(commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		return commit_info->revision;

	UpdateShell(path);
	return -1;
}

BOOL SVN::Copy(CString srcPath, CString destPath, LONG revision, CString logmsg)
{
	preparePath(srcPath);
	preparePath(destPath);
	svn_client_commit_info_t *commit_info = NULL;
	if (logmsg.IsEmpty())
		ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(_T("made a copy")));
	else
		ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(logmsg));
	Err = svn_client_copy (&commit_info,
							CUnicodeUtils::GetUTF8(srcPath),
							getRevision (revision),
							CUnicodeUtils::GetUTF8(destPath),
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(destPath, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);
	return TRUE;
}

BOOL SVN::Move(CString srcPath, CString destPath, BOOL force)
{
	preparePath(srcPath);
	preparePath(destPath);
	svn_client_commit_info_t *commit_info = NULL;

	Err = svn_client_move (&commit_info,
							CUnicodeUtils::GetUTF8(srcPath),
							getRevision (-1),
							CUnicodeUtils::GetUTF8(destPath),
							force,
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(destPath, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);

	UpdateShell(srcPath);
	UpdateShell(destPath);

	return TRUE;
}

BOOL SVN::MakeDir(CString path, CString message)
{
	preparePath(path);

	svn_client_commit_info_t *commit_info = NULL;

	Err = svn_client_mkdir (&commit_info,
							target(path),
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::CleanUp(CString path)
{
	preparePath(path);
	Err = svn_client_cleanup (CUnicodeUtils::GetUTF8(path), &ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Resolve(CString path, BOOL recurse)
{
	preparePath(path);
	Err = svn_client_resolved (CUnicodeUtils::GetUTF8(path),
								recurse,
								&ctx,
								pool);
	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Export(CString srcPath, CString destPath, LONG revision, BOOL force)
{
	preparePath(srcPath);
	preparePath(destPath);

	Err = svn_client_export (CUnicodeUtils::GetUTF8(srcPath),
							CUnicodeUtils::GetUTF8(destPath),
							getRevision (revision),
							force,
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL SVN::Switch(CString path, CString url, LONG revision, BOOL recurse)
{
	preparePath(path);
	preparePath(url);

	Err = svn_client_switch (CUnicodeUtils::GetUTF8(path),
							CUnicodeUtils::GetUTF8(url),
							getRevision (revision),
							recurse,
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	
	UpdateShell(path);

	return TRUE;
}

BOOL SVN::Import(CString path, CString url, CString message, BOOL recurse)
{
	preparePath(path);
	preparePath(url);

	svn_client_commit_info_t *commit_info = NULL;

	ctx.log_msg_baton = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_import (&commit_info,
							CUnicodeUtils::GetUTF8(path),
							CUnicodeUtils::GetUTF8(url),
							!recurse,
							&ctx,
							pool);
	ctx.log_msg_baton = logMessage("");
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision);
	return TRUE;
}

BOOL SVN::Merge(CString path1, LONG revision1, CString path2, LONG revision2, CString localPath, BOOL force, BOOL recurse, BOOL ignoreanchestry)
{
	svn_opt_revision_t revEnd;
	memset (&revEnd, 0, sizeof (revEnd));
	if(revision2 == -1)
	{
		revEnd.kind = svn_opt_revision_head;
		revision2 = 0;
	}
	else
	{
		revEnd.kind = svn_opt_revision_number;
	}
	revEnd.value.number = revision2;

	preparePath(path1);
	preparePath(path2);
	preparePath(localPath);

	Err = svn_client_merge (CUnicodeUtils::GetUTF8(path1),
							getRevision (revision1),
							CUnicodeUtils::GetUTF8(path2),
							&revEnd,
							CUnicodeUtils::GetUTF8(localPath),
							recurse,
							ignoreanchestry,
							force,
							false,		//no 'dry-run'
							&ctx,
							pool);
	if(Err != NULL)
	{
		return FALSE;
	}

	UpdateShell(localPath);

	return TRUE;
}

BOOL SVN::Diff(CString path1, LONG revision1, CString path2, LONG revision2, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, CString options, CString outputfile, CString errorfile)
{
	BOOL del = FALSE;
	apr_file_t * outfile;
	apr_file_t * errfile;
	apr_array_header_t *opts;

	opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, pool);

	svn_opt_revision_t revEnd;
	memset (&revEnd, 0, sizeof (revEnd));
	if(revision2 == REV_HEAD)
	{
		revEnd.kind = svn_opt_revision_head;
		revision2 = 0;
	} // if(revision2 == REV_HEAD) 
	else if (revision2 == REV_BASE)
	{
		revEnd.kind = svn_opt_revision_base;
		revision2 = 0;
	}
	else if (revision2 == REV_WC)
	{
		revEnd.kind = svn_opt_revision_working;
		revision2 = 0;
	}
	else
	{
		revEnd.kind = svn_opt_revision_number;
	}
	revEnd.value.number = revision2;

	preparePath(path1);
	preparePath(path2);
	preparePath(outputfile);
	preparePath(errorfile);

	Err = svn_io_file_open (&outfile, CUnicodeUtils::GetUTF8(outputfile),
							APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
							APR_OS_DEFAULT, pool);
	if (Err)
		return FALSE;

	if (errorfile.IsEmpty())
	{
		TCHAR path[MAX_PATH];
		TCHAR tempF[MAX_PATH];
		::GetTempPath (MAX_PATH, path);
		::GetTempFileName (path, _T("svn"), 0, tempF);
		errorfile = CString(tempF);
		del = TRUE;
	}

	Err = svn_io_file_open (&errfile, CUnicodeUtils::GetUTF8(errorfile),
							APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
							APR_OS_DEFAULT, pool);
	if (Err)
		return FALSE;

	Err = svn_client_diff (opts,
						   CUnicodeUtils::GetUTF8(path1),
						   getRevision(revision1),
						   CUnicodeUtils::GetUTF8(path2),
						   &revEnd,
						   recurse,
						   ignoreancestry,
						   nodiffdeleted,
						   outfile,
						   errfile,
						   &ctx,
						   pool);
	if (Err)
		return FALSE;
	if (del)
	{
		svn_io_remove_file (CUnicodeUtils::GetUTF8(errorfile), pool);
	}
	return TRUE;
}

BOOL SVN::ReceiveLog(CString path, LONG revisionStart, LONG revisionEnd, BOOL changed)
{
	svn_opt_revision_t revEnd;
	memset (&revEnd, 0, sizeof (revEnd));
	if(revisionEnd == -1)
	{
		revEnd.kind = svn_opt_revision_head;
		revisionEnd = 0;
	}
	else
	{
		revEnd.kind = svn_opt_revision_number;
	}
	revEnd.value.number = revisionEnd;

	preparePath(path);
	Err = svn_client_log (target ((LPCTSTR)path), 
						getRevision (revisionStart), 
						&revEnd, 
						changed,
						false,			// not strict by default (showing cp info)
						logReceiver,	// log_message_receiver
						(void *)this, &ctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL SVN::Cat(CString url, LONG revision, CString localpath)
{
	apr_file_t * file;
	svn_stream_t * stream;

	preparePath(url);

	DeleteFile(localpath);

	apr_file_open(&file, CUnicodeUtils::GetUTF8(localpath), APR_WRITE | APR_CREATE, APR_OS_DEFAULT, pool);
	stream = svn_stream_from_aprfile(file, pool);

	Err = svn_client_cat(stream, CUnicodeUtils::GetUTF8(url), getRevision(revision), &ctx, pool);

	apr_file_close(file);
	if (Err != NULL)
		return FALSE;
	return TRUE;
}

CString SVN::GetActionText(svn_wc_notify_action_t action, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state)
{
	CString temp = _T(" ");
	switch (action)
	{
		case svn_wc_notify_add:
		case svn_wc_notify_update_add:
		case svn_wc_notify_commit_added:
			temp.LoadString(IDS_SVNACTION_ADD);
			break;
		case svn_wc_notify_copy:
			temp.LoadString(IDS_SVNACTION_COPY);
			break;
		case svn_wc_notify_delete:
		case svn_wc_notify_update_delete:
		case svn_wc_notify_commit_deleted:
			temp.LoadString(IDS_SVNACTION_DELETE);
			break;
		case svn_wc_notify_restore:
			temp.LoadString(IDS_SVNACTION_RESTORE);
			break;
		case svn_wc_notify_revert:
			temp.LoadString(IDS_SVNACTION_REVERT);
			break;
		case svn_wc_notify_resolved:
			temp.LoadString(IDS_SVNACTION_RESOLVE);
			break;
		case svn_wc_notify_update_update:
			if ((content_state == svn_wc_notify_state_conflicted) || (prop_state == svn_wc_notify_state_conflicted))
				temp.LoadString(IDS_STATUSCONFLICTED);
			else if ((content_state == svn_wc_notify_state_merged) || (prop_state == svn_wc_notify_state_merged))
				temp.LoadString(IDS_STATUSMERGED);
			else
				temp.LoadString(IDS_SVNACTION_UPDATE);
			break;
		case svn_wc_notify_update_completed:
			temp.LoadString(IDS_SVNACTION_COMPLETED);
			break;
		case svn_wc_notify_update_external:
			temp.LoadString(IDS_SVNACTION_EXTERNAL);
			break;
		case svn_wc_notify_commit_modified:
			temp.LoadString(IDS_SVNACTION_MODIFIED);
			break;
		case svn_wc_notify_commit_replaced:
			temp.LoadString(IDS_SVNACTION_REPLACED);
			break;
		case svn_wc_notify_commit_postfix_txdelta:
			temp.LoadString(IDS_SVNACTION_POSTFIX);
			break;
		case svn_wc_notify_failed_revert:
			temp.LoadString(IDS_SVNACTION_FAILEDREVERT);
			break;
		case svn_wc_notify_status_completed:
		case svn_wc_notify_status_external:
			temp.LoadString(IDS_SVNACTION_STATUS);
			break;
		case svn_wc_notify_skip:
			temp.LoadString(IDS_SVNACTION_SKIP);
			break;
		default:
			return _T("???");
	}
	return temp;
}

BOOL SVN::CreateRepository(CString path)
{
	apr_pool_t * localpool;
	svn_repos_t * repo;
	svn_error_t * err;
	apr_initialize();
	localpool = svn_pool_create (NULL);

	err = svn_repos_create(&repo, CUnicodeUtils::GetUTF8(path), NULL, NULL, NULL, NULL, localpool);
	if (err != NULL)
	{
		svn_pool_destroy(localpool);
		return FALSE;
	}
	svn_pool_destroy(localpool);
	apr_terminate();
	return TRUE;
}



svn_error_t* SVN::logReceiver(void* baton, 
								apr_hash_t* ch_paths, 
								svn_revnum_t rev, 
								const char* author, 
								const char* date, 
								const char* msg, 
								apr_pool_t* pool)
{
	svn_error_t * error = NULL;
	TCHAR date_native[MAX_PATH] = {0};
	stdstring author_native;
	stdstring msg_native;

	SVN * svn = (SVN *)baton;
	author_native = CUnicodeUtils::GetUnicode(author);

	if (date && date[0])
	{
		//Convert date to a format for humans.
		apr_time_t time_temp;

		error = svn_time_from_cstring (&time_temp, date, pool);
		if (error)
			return error;
		__time64_t ttime = time_temp/1000000L;

		struct tm * newtime;
		SYSTEMTIME systime;
		TCHAR timebuf[MAX_PATH];
		TCHAR datebuf[MAX_PATH];

		LCID locale = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
		locale = MAKELCID(locale, SORT_DEFAULT);

		newtime = _localtime64(&ttime);

		systime.wDay = newtime->tm_mday;
		systime.wDayOfWeek = newtime->tm_wday;
		systime.wHour = newtime->tm_hour;
		systime.wMilliseconds = 0;
		systime.wMinute = newtime->tm_min;
		systime.wMonth = newtime->tm_mon+1;
		systime.wSecond = newtime->tm_sec;
		systime.wYear = newtime->tm_year+1900;
		GetDateFormat(locale, 0, &systime, NULL, datebuf, MAX_PATH);
		GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_PATH);
		_tcsncat(date_native, timebuf, MAX_PATH);
		_tcsncat(date_native, _T(", "), MAX_PATH);
		_tcsncat(date_native, datebuf, MAX_PATH);
	}
	else
		_tcscat(date_native, _T("(no date)"));

	if (msg == NULL)
		msg = "";

	msg_native = CUnicodeUtils::GetUnicode(msg);
	CString cpaths;
	try
	{
		cpaths.Preallocate(1000000);		//allocate 1M memory
		if (ch_paths)
		{
			apr_array_header_t *sorted_paths;
			sorted_paths = apr_hash_sorted_keys(ch_paths, svn_sort_compare_items_as_paths, pool);
			for (int i = 0; i < sorted_paths->nelts; i++)
			{
				svn_item_t *item = &(APR_ARRAY_IDX (sorted_paths, i, svn_item_t));
				stdstring path_native;
				const char *path = (const char *)item->key;
				svn_log_changed_path_t *log_item = (svn_log_changed_path_t *)apr_hash_get (ch_paths, item->key, item->klen);
				path_native = CUnicodeUtils::GetUnicode(path);
				CString temp;
				switch (log_item->action)
				{
				case 'M':
					temp.LoadString(IDS_SVNACTION_MODIFIED);
					break;
				case 'R':
					temp.LoadString(IDS_SVNACTION_REPLACED);
					break;
				case 'A':
					temp.LoadString(IDS_SVNACTION_ADD);
					break;
				case 'D':
					temp.LoadString(IDS_SVNACTION_DELETE);
				default:
					break;
				} // switch (temppath->action)
				if (!cpaths.IsEmpty())
					cpaths += _T("\r\n");
				cpaths += temp;
				cpaths += _T(" ");
				cpaths += path_native.c_str();
				if (i == 500)
				{
					temp.Format(_T("\r\n.... and %d more ..."), sorted_paths->nelts - 500);
					cpaths += temp;
					break;
				}
			} // for (int i = 0; i < sorted_paths->nelts; i++) 
		} // if (changed_paths)
	}
	catch (CMemoryException * e)
	{
		cpaths = _T("Memory Exception!");
		e->Delete();
	}

	SVN_ERR (svn->cancel(baton));

	if (svn->Log(rev, CString(author_native.c_str()), CString(date_native), CString(msg_native.c_str()), cpaths))
	{
		return error;
	}
	return error;
}

void SVN::notify( void *baton,
					const char *path,
					svn_wc_notify_action_t action,
					svn_node_kind_t kind,
					const char *mime_type,
					svn_wc_notify_state_t content_state,
					svn_wc_notify_state_t prop_state,
					svn_revnum_t revision)
{
	SVN * svn = (SVN *)baton;
	WCHAR buf[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, path, -1, buf, MAX_PATH);
	svn->Notify(CString(buf), (svn_wc_notify_action_t)action, kind, CString(mime_type), content_state, prop_state, revision);
}

svn_error_t* SVN::cancel(void *baton)
{
	SVN * svn = (SVN *)baton;
	if (svn->Cancel())
	{
		CStringA temp;
		temp.LoadString(IDS_SVN_USERCANCELLED);
		return svn_error_create(SVN_ERR_CANCELLED, NULL, temp);
	}
	return SVN_NO_ERROR;
}

void * SVN::logMessage (const char * message, char * baseDirectory)
{
	log_msg_baton* baton = (log_msg_baton *) apr_palloc (pool, sizeof (*baton));
	baton->message = apr_pstrdup(pool, message);
	baton->base_dir = baseDirectory ? baseDirectory : "";

	baton->message_encoding = NULL;
	baton->tmpfile_left = NULL;
	baton->pool = pool;

	return baton;
}

svn_opt_revision_t*	SVN::getRevision (long revNumber)
{
	memset (&rev, 0, sizeof (rev));
	if(revNumber == SVN::REV_HEAD)
	{
		rev.kind = svn_opt_revision_head;
		revNumber = 0;
	} // if(revNumber == SVN::REV_HEAD)
	else if (revNumber == SVN::REV_BASE)
	{
		rev.kind = svn_opt_revision_base;
		revNumber = 0;
	}
	else if (revNumber == SVN::REV_WC)
	{
		rev.kind = svn_opt_revision_working;
		revNumber = 0;
	}
	else
	{
		rev.kind = svn_opt_revision_number;
	}

	rev.value.number = revNumber;

	return &rev;
}

void SVN::PathToUrl(CString &path)
{
	path.Trim();
	// convert \ to /
	path.Replace('\\','/');
	// prepend file:///
	if (path.GetAt(0) == '/')
		path.Insert(0, _T("file://"));
	else
		path.Insert(0, _T("file:///"));
	path.TrimRight(_T("/\\"));			//remove trailing slashes
}

void SVN::UrlToPath(CString &url)
{
	//we have to convert paths like file:///c:/myfolder
	//to c:/myfolder
	//and paths like file:////mymachine/c/myfolder
	//to //mymachine/c/myfolder
	url.Trim();
	url.Replace('\\','/');
	url = url.Mid(7);
	if (url.GetAt(1) != '/')
		url = url.Mid(1);
	url.Replace('/','\\');
	url.TrimRight(_T("/\\"));			//remove trailing slashes
}

void	SVN::preparePath(CString &path)
{
	path.Trim();
	path.TrimRight(_T("/\\"));			//remove trailing slashes
	path.Replace('\\','/');
	if ((path.Left(7).CompareNoCase(_T("http://"))==0) ||
		(path.Left(8).CompareNoCase(_T("https://"))==0) ||
		(path.Left(6).CompareNoCase(_T("svn://"))==0) ||
		(path.Left(10).CompareNoCase(_T("svn-ssh://"))==0))
	{
		TCHAR buf[8192];
		DWORD len = 8192;
		UrlCanonicalize(path, buf, &len, URL_ESCAPE_UNSAFE);
		path = CString(buf);
	}
}

svn_error_t* svn_cl__get_log_message (const char **log_msg,
									const char **tmp_file,
									apr_array_header_t * commit_items,
									void *baton, 
									apr_pool_t * pool)
{
	log_msg_baton *lmb = (log_msg_baton *) baton;
	*tmp_file = NULL;
	if (lmb->message)
	{
		*log_msg = apr_pstrdup (pool, lmb->message);
	}

	return SVN_NO_ERROR;
}

apr_array_header_t * SVN::target (LPCTSTR path)
{
	CString p = CString(path);
	apr_array_header_t *targets = apr_array_make (pool,
												5,
												sizeof (const char *));

	int curPos= 0;

	CString resToken= p.Tokenize(_T(";"),curPos);
	while (resToken != _T(""))
	{
		const char * target = apr_pstrdup (pool, CUnicodeUtils::GetUTF8(resToken));
		(*((const char **) apr_array_push (targets))) = target;
		resToken= p.Tokenize(_T(";"),curPos);
	};
	return targets;
}

svn_error_t * SVN::get_url_from_target (const char **URL, const char *target)
{
	svn_wc_adm_access_t *adm_access;          
	const svn_wc_entry_t *entry;  
	svn_boolean_t is_url = svn_path_is_url (target);

	if (is_url)
		*URL = target;

	else
	{
		SVN_ERR (svn_wc_adm_probe_open (&adm_access, NULL, target,
			FALSE, FALSE, pool));
		SVN_ERR (svn_wc_entry (&entry, target, adm_access, FALSE, pool));
		SVN_ERR (svn_wc_adm_close (adm_access));

		*URL = entry ? entry->url : NULL;
	}

	return SVN_NO_ERROR;
}

BOOL SVN::Ls(CString url, LONG revision, CStringArray& entries)
{
	entries.RemoveAll();

	preparePath(url);

	apr_hash_t* hash = apr_hash_make(pool);

	Err = svn_client_ls(&hash, 
						CUnicodeUtils::GetUTF8(url),
						getRevision(revision),
						FALSE, 
						&ctx,
						pool);
	if (Err != NULL)
		return FALSE;

	apr_hash_index_t *hi;
    svn_dirent_t* val;
	const char* key;
    for (hi = apr_hash_first(pool, hash); hi; hi = apr_hash_next(hi)) 
	{
        apr_hash_this(hi, (const void**)&key, NULL, (void**)&val);
		CString temp;
		if (val->kind == svn_node_dir)
			temp = "d";
		else if (val->kind == svn_node_file)
			temp = "f";
		else
			temp = "u";
		temp = temp + CUnicodeUtils::GetUnicode(key);
		entries.Add(temp);
    }
	return Err == NULL;
}

BOOL SVN::Relocate(CString path, CString from, CString to, BOOL recurse)
{
	preparePath(path);
	preparePath(from);
	preparePath(to);
	Err = svn_client_relocate(CUnicodeUtils::GetUTF8(path), CUnicodeUtils::GetUTF8(from), CUnicodeUtils::GetUTF8(to), recurse, &ctx, pool);
	if (Err != NULL)
		return FALSE;
	return TRUE;
}

BOOL SVN::IsRepository(const CString& strUrl)
{
	svn_repos_t* pRepos;
	CString url = strUrl;
	preparePath(url);
	url += _T("/");
	int pos = url.GetLength();
	while ((pos = url.ReverseFind('/'))>=0)
	{
		url = url.Left(pos);
		Err = svn_repos_open (&pRepos, CUnicodeUtils::GetUTF8(url), pool);
		if ((Err)&&(Err->apr_err == SVN_ERR_FS_BERKELEY_DB))
			return TRUE;
		if (Err == NULL)
			return TRUE;
	}

	return Err == NULL;
}

CString SVN::GetRepositoryRoot(CString url)
{
	CString retUrl = url;
	preparePath(retUrl);
	CString findUrl = retUrl;
	int pos = findUrl.GetLength();
	CStringArray dummyarray;
	while ((pos = findUrl.ReverseFind('/'))>=0)
	{
		if (!Ls(findUrl, -1, dummyarray))
		{
			return retUrl;
		}
		retUrl = findUrl;
		findUrl = findUrl.Left(pos);
	} // while ((pos = findUrl.ReverseFind('/'))>=0) 
	return _T("");
}

CString SVN::GetPristinePath(CString wcPath)
{
	apr_pool_t * localpool;
	svn_error_t * err;
	apr_initialize();
	localpool = svn_pool_create (NULL);
	const char* pristinePath = NULL;
	CString temp;

	err = svn_wc_get_pristine_copy_path(svn_path_internal_style(CUnicodeUtils::GetUTF8(wcPath), localpool), (const char **)&pristinePath, localpool);

	if (err != NULL)
	{
		svn_pool_destroy(localpool);
		return temp;
	}
	if (pristinePath != NULL)
		temp = CString(pristinePath);
	svn_pool_destroy(localpool);
	apr_terminate();
	return temp;
}

void SVN::UpdateShell(CString path)
{
	//updating the left pane (tree view) of the explorer
	//is more difficult (if not impossible) than I thought.
	//Using SHChangeNotify() doesn't work at all. I found that
	//the shell receives the message, but then checks the files/folders
	//itself for changes. And since the folders which are shown
	//in the tree view haven't changed the icon-overlay is
	//not updated!
	//a workaround for this problem would be if this method would
	//rename the folders, do a SHChangeNotify(SHCNE_RMDIR, ...),
	//rename the folders back and do an SHChangeNotify(SHCNE_UPDATEDIR, ...)
	//
	//But I'm not sure if that is really a good workaround - it'll possibly
	//slows down the explorer and also causes more HD usage.
	//
	//So this method only updates the files and folders in the normal
	//explorer view by telling the explorer that the folder icon itself
	//has changed.
	preparePath(path);
	SHFILEINFO    sfi;
	SHGetFileInfo(
		(LPCTSTR)path, 
		FILE_ATTRIBUTE_DIRECTORY,
		&sfi, 
		sizeof(SHFILEINFO), 
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	SHFILEINFO    sfiopen;
	SHGetFileInfo(
		(LPCTSTR)path, 
		FILE_ATTRIBUTE_DIRECTORY,
		&sfiopen, 
		sizeof(SHFILEINFO), 
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES | SHGFI_OPENICON);

	SHChangeNotify(SHCNE_UPDATEIMAGE | SHCNF_FLUSH, SHCNF_DWORD, NULL, (LPCVOID)sfi.iIcon);
	SHChangeNotify(SHCNE_UPDATEIMAGE | SHCNF_FLUSH, SHCNF_DWORD, NULL, (LPCVOID)sfiopen.iIcon);
}

typedef struct
{
  /* the cred_kind being fetched (see svn_auth.h)*/
  const char *cred_kind;

  /* cache:  realmstring which identifies the credentials file */
  const char *realmstring;

  /* values retrieved from cache. */
  const char *username;
  const char *password;
  SVN * svn;

} provider_baton_t;

static const svn_auth_provider_t tsvn_simple_provider = {
	SVN_AUTH_CRED_SIMPLE,
		tsvn_simple_first_creds,
		NULL,
		tsvn_simple_save_creds
};

void SVN::get_simple_provider (svn_auth_provider_object_t **provider,
							   apr_pool_t *pool)
{
	svn_auth_provider_object_t *po = (svn_auth_provider_object_t *)apr_pcalloc (pool, sizeof(*po));
	provider_baton_t *pb = (provider_baton_t *)apr_pcalloc (pool, sizeof(*pb));
	pb->cred_kind = SVN_AUTH_CRED_SIMPLE;
	pb->svn = this;
	po->vtable = &tsvn_simple_provider;
	po->provider_baton = pb;
	*provider = po;
}

svn_error_t *
tsvn_simple_save_creds (svn_boolean_t *saved,
							 void *credentials,
							 void *provider_baton,
							 apr_hash_t *parameters,
							 apr_pool_t *pool)
{
	svn_auth_cred_simple_t *creds = (svn_auth_cred_simple_t *)credentials;
	provider_baton_t *pb = (provider_baton_t *)provider_baton;
	//SVN_ERR (save_creds (saved, pb, creds->username, creds->password, pool));
	((provider_baton_t *)provider_baton)->svn->m_username = apr_pstrdup(pool, creds->username);
	((provider_baton_t *)provider_baton)->svn->m_password = apr_pstrdup(pool, creds->password);
	return SVN_NO_ERROR;
}

svn_error_t *
tsvn_simple_first_creds (void **credentials,
							  void **iter_baton,
							  void *provider_baton,
							  apr_hash_t *parameters,
							  const char *realmstring,
							  apr_pool_t *pool)
{
	provider_baton_t *pb = (provider_baton_t *)provider_baton;

	if (realmstring)
		pb->realmstring = apr_pstrdup (pool, realmstring);

	svn_auth_cred_simple_t *creds = (svn_auth_cred_simple_t *)apr_pcalloc (pool, sizeof(*creds));
	creds->username = ((provider_baton_t *)provider_baton)->svn->m_username;
	creds->password = ((provider_baton_t *)provider_baton)->svn->m_password;
	if ((creds->username)&&(creds->password))
		*credentials = creds;
	else
		*credentials = NULL;

	*iter_baton = NULL;
	return SVN_NO_ERROR;
}

