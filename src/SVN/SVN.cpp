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
#include "TortoiseProc.h"
#include "ProgressDlg.h"
#include "svn.h"
#include "svn_sorts.h"
#include "client.h"
#include "UnicodeUtils.h"
#include "DirFileEnum.h"
#include "TSVNPath.h"
#include "ShellUpdater.h"
#include "Registry.h"
#include "SVNHelpers.h"
#include "SVNStatus.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "ShellFileOp.h"
#include "SVNAdminDir.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SVN::SVN(void) :
	m_progressWnd(0),
	m_pProgressDlg(NULL),
	m_bShowProgressBar(false),
	progress_total(0),
	progress_lastprogress(0),
	progress_lasttotal(0)
{
	parentpool = svn_pool_create(NULL);
	svn_utf_initialize(parentpool);
	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", parentpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", parentpool);
	svn_client_create_context(&m_pctx, parentpool);

	Err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	// set up the configuration
	if (Err == 0)
		Err = svn_config_get_config (&(m_pctx->config), g_pConfigDir, parentpool);

	if (Err != 0)
	{
		::MessageBox(NULL, this->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_pool_destroy (pool);
		svn_pool_destroy (parentpool);
		exit(-1);
	} // if (Err != 0) 

	// set up authentication
	m_prompt.Init(parentpool, m_pctx);

	m_pctx->log_msg_func2 = svn_cl__get_log_message;
	m_pctx->log_msg_baton2 = logMessage("");
	m_pctx->notify_func2 = notify;
	m_pctx->notify_baton2 = this;
	m_pctx->notify_func = NULL;
	m_pctx->notify_baton = NULL;
	m_pctx->cancel_func = cancel;
	m_pctx->cancel_baton = this;
	m_pctx->progress_func = progress_func;
	m_pctx->progress_baton = this;

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	if (tsvn_ssh.IsEmpty())
		tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get (m_pctx->config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}
}

SVN::~SVN(void)
{
	svn_pool_destroy (parentpool);
}

CString SVN::CheckConfigFile()
{
	svn_client_ctx_t 			ctx;
	SVNPool						pool;
	svn_error_t *				Err;

	memset (&ctx, 0, sizeof (ctx));

	Err = svn_config_ensure(NULL, pool);
	// set up the configuration
	if (Err == 0)
		Err = svn_config_get_config (&(ctx.config), g_pConfigDir, pool);
	CString msg;
	CString temp;
	if (Err != NULL)
	{
		svn_error_t * ErrPtr = Err;
		msg = CUnicodeUtils::GetUnicode(ErrPtr->message);
		while (ErrPtr->child)
		{
			ErrPtr = ErrPtr->child;
			msg += _T("\n");
			msg += CUnicodeUtils::GetUnicode(ErrPtr->message);
		}
		if (!temp.IsEmpty())
		{
			msg += _T("\n") + temp;
		}
	}
	return msg;
}

#pragma warning(push)
#pragma warning(disable: 4100)	// unreferenced formal parameter

BOOL SVN::Cancel() {return FALSE;}

BOOL SVN::Notify(const CTSVNPath& path, svn_wc_notify_action_t action, 
				svn_node_kind_t kind, const CString& mime_type, 
				svn_wc_notify_state_t content_state, 
				svn_wc_notify_state_t prop_state, svn_revnum_t rev,
				const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
				svn_error_t * err, apr_pool_t * pool) {return TRUE;};
BOOL SVN::Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions) {return TRUE;};
BOOL SVN::BlameCallback(LONG linenumber, svn_revnum_t revision, const CString& author, const CString& date, const CStringA& line) {return TRUE;}
svn_error_t* SVN::DiffSummarizeCallback(const CTSVNPath& path, svn_client_diff_summarize_kind_t kind, bool propchanged, svn_node_kind_t node) {return SVN_NO_ERROR;}
#pragma warning(pop)

struct log_msg_baton2
{
  const char *message;  /* the message. */
  const char *message_encoding; /* the locale/encoding of the message. */
  const char *base_dir; /* the base directory for an external edit. UTF-8! */
  const char *tmpfile_left; /* the tmpfile left by an external edit. UTF-8! */
  apr_pool_t *pool; /* a pool. */
};

CString SVN::GetLastErrorMessage(int wrap /* = 80 */)
{
	return GetErrorString(Err, wrap);
}

CString SVN::GetErrorString(svn_error_t * Err, int wrap /* = 80 */)
{
	CString msg;
	CString temp;
	char errbuf[256];

	if (Err != NULL)
	{
		svn_error_t * ErrPtr = Err;
		if (ErrPtr->message)
			msg = CUnicodeUtils::GetUnicode(ErrPtr->message);
		else
		{
			/* Is this a Subversion-specific error code? */
			if ((ErrPtr->apr_err > APR_OS_START_USEERR)
				&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
				msg = svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf));
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
					msg = CUnicodeUtils::GetUnicode(err_string);
				}
			}
		}
		msg = CStringUtils::LinesWrap(msg, wrap);
		while (ErrPtr->child)
		{
			ErrPtr = ErrPtr->child;
			msg += _T("\n");
			if (ErrPtr->message)
				temp = CUnicodeUtils::GetUnicode(ErrPtr->message);
			else
			{
				/* Is this a Subversion-specific error code? */
				if ((ErrPtr->apr_err > APR_OS_START_USEERR)
					&& (ErrPtr->apr_err <= APR_OS_START_CANONERR))
					temp = svn_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf));
				/* Otherwise, this must be an APR error code. */
				else
				{
					svn_error_t *temp_err = NULL;
					const char * err_string = NULL;
					temp_err = svn_utf_cstring_to_utf8(&err_string, apr_strerror (ErrPtr->apr_err, errbuf, sizeof (errbuf)-1), ErrPtr->pool);
					if (temp_err)
					{
						svn_error_clear (temp_err);
						temp = _T("Can't recode error string from APR");
					}
					else
					{
						temp = CUnicodeUtils::GetUnicode(err_string);
					}
				}
			}
			temp = CStringUtils::LinesWrap(temp, wrap);
			msg += temp;
		}
		temp.Empty();
		switch (Err->apr_err)
		{
		case SVN_ERR_BAD_FILENAME:
		case SVN_ERR_BAD_URL:
			// please check the path or URL you've entered.
			temp.LoadString(IDS_SVNERR_CHECKPATHORURL);
			break;
		case SVN_ERR_WC_LOCKED:
		case SVN_ERR_WC_NOT_LOCKED:
			// do a "cleanup"
			temp.LoadString(IDS_SVNERR_RUNCLEANUP);
			break;
		case SVN_ERR_WC_NOT_UP_TO_DATE:
		case SVN_ERR_RA_OUT_OF_DATE:
		case SVN_ERR_FS_TXN_OUT_OF_DATE:
			// do an update first
			temp.LoadString(IDS_SVNERR_UPDATEFIRST);
			break;
		case SVN_ERR_WC_CORRUPT:
		case SVN_ERR_WC_CORRUPT_TEXT_BASE:
			// do a "cleanup". If that doesn't work you need to do a fresh checkout.
			temp.LoadString(IDS_SVNERR_CLEANUPORFRESHCHECKOUT);
			break;
		default:
			break;
		}
		if ((Err->apr_err == SVN_ERR_FS_PATH_NOT_LOCKED)||
			(Err->apr_err == SVN_ERR_FS_NO_SUCH_LOCK)||
			(Err->apr_err == SVN_ERR_RA_NOT_LOCKED))
		{
			temp.LoadString(IDS_SVNERR_UNLOCKFAILEDNOLOCK);
		}
		else if (SVN_ERR_IS_UNLOCK_ERROR(Err))
		{
			temp.LoadString(IDS_SVNERR_UNLOCKFAILED);
		}
		if (!temp.IsEmpty())
		{
			msg += _T("\n") + temp;
		}
		return msg;
	}
	return _T("");
}

BOOL SVN::Checkout(const CTSVNPath& moduleName, const CTSVNPath& destPath, SVNRev pegrev, SVNRev revision, BOOL recurse, BOOL bIgnoreExternals)
{
	SVNPool subpool(pool);
	Err = svn_client_checkout2 (NULL,			// we don't need the resulting revision
								moduleName.GetSVNApiPath(),
								destPath.GetSVNApiPath(),
								pegrev,
								revision,
								recurse,
								bIgnoreExternals,
								m_pctx,
								subpool );

	if(Err != NULL)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL SVN::Remove(const CTSVNPathList& pathlist, BOOL force, CString message)
{
	// svn_client_delete needs to run on a sub-pool, so that after it's run, the pool
	// cleanups get run.  For example, after a failure do to an unforced delete on 
	// a modified file, if you don't do a cleanup, the WC stays locked
	SVNPool subPool(pool);
	svn_commit_info_t *commit_info = svn_create_commit_info(subPool);
	message.Replace(_T("\r"), _T(""));
	m_pctx->log_msg_baton2 = logMessage(CUnicodeUtils::GetUTF8(message));

	Err = svn_client_delete2 (&commit_info, MakePathArray(pathlist), force,
							  m_pctx,
							  subPool);
	if(Err != NULL)
	{
		return FALSE;
	}

	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
	{
		for (int i=0; i<pathlist.GetCount(); ++i)
			Notify(pathlist[i], svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision, NULL, svn_wc_notify_lock_state_unchanged, NULL, pool);
	}

	for(int nPath = 0; nPath < pathlist.GetCount(); nPath++)
	{
		if (!pathlist[nPath].IsDirectory())
		{
			SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_FLUSHNOWAIT, pathlist[nPath].GetWinPath(), NULL);
		}
	}

	return TRUE;
}

BOOL SVN::Revert(const CTSVNPathList& pathlist, BOOL recurse)
{
	TRACE("Reverting list of %d files\n", pathlist.GetCount());
	SVNPool subpool(pool);

	Err = svn_client_revert (MakePathArray(pathlist), recurse, m_pctx, subpool);

	if(Err != NULL)
	{
		return FALSE;
	}

	return TRUE;
}


BOOL SVN::Add(const CTSVNPathList& pathList, BOOL recurse, BOOL force /* = FALSE */, BOOL no_ignore /* = FALSE */)
{
	for(int nItem = 0; nItem < pathList.GetCount(); nItem++)
	{
		TRACE(_T("add file %s\n"), pathList[nItem].GetWinPath());

		SVNPool subpool(pool);
		Err = svn_client_add3 (pathList[nItem].GetSVNApiPath(), recurse, force, no_ignore, m_pctx, subpool);
		if(Err != NULL)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL SVN::Update(const CTSVNPathList& pathList, SVNRev revision, BOOL recurse, BOOL ignoreexternals)
{
	SVNPool(localpool);
	Err = svn_client_update2(NULL,
							MakePathArray(pathList),
							revision,
							recurse,
							ignoreexternals,
							m_pctx,
							localpool);

	if(Err != NULL)
	{
		return FALSE;
	}

	return TRUE;
}

svn_revnum_t SVN::Commit(const CTSVNPathList& pathlist, CString message, BOOL recurse, BOOL keep_locks)
{
	svn_commit_info_t *commit_info = svn_create_commit_info(pool);

	message.Replace(_T("\r"), _T(""));
	m_pctx->log_msg_baton2 = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_commit3 (&commit_info, 
							MakePathArray(pathlist), 
							recurse,
							keep_locks,
							m_pctx,
							pool);
	m_pctx->log_msg_baton2 = logMessage("");
	if(Err != NULL)
	{
		return 0;
	}

	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
	{
		Notify(CTSVNPath(), svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision, NULL, svn_wc_notify_lock_state_unchanged, NULL, pool);
		return commit_info->revision;
	}

	return -1;
}

BOOL SVN::Copy(const CTSVNPath& srcPath, const CTSVNPath& destPath, SVNRev revision, CString logmsg)
{
	SVNPool subpool(pool);

	svn_commit_info_t *commit_info = svn_create_commit_info(subpool);
	logmsg.Replace(_T("\r"), _T(""));
	if (logmsg.IsEmpty())
		m_pctx->log_msg_baton2 = logMessage(CUnicodeUtils::GetUTF8(CString(_T("made a copy"))));
	else
		m_pctx->log_msg_baton2 = logMessage(CUnicodeUtils::GetUTF8(logmsg));
	Err = svn_client_copy3 (&commit_info,
							srcPath.GetSVNApiPath(),
							revision,
							destPath.GetSVNApiPath(),
							m_pctx,
							subpool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(destPath, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision, NULL, svn_wc_notify_lock_state_unchanged, NULL, pool);

	return TRUE;
}

BOOL SVN::Move(const CTSVNPath& srcPath, const CTSVNPath& destPath, BOOL force, CString message)
{
	SVNPool subpool(pool);

	svn_commit_info_t *commit_info = svn_create_commit_info(subpool);
	message.Replace(_T("\r"), _T(""));
	m_pctx->log_msg_baton2 = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_move4 (&commit_info,
							srcPath.GetSVNApiPath(),
							destPath.GetSVNApiPath(),
							force,
							m_pctx,
							subpool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(destPath, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision, NULL, svn_wc_notify_lock_state_unchanged, NULL, pool);

	return TRUE;
}

BOOL SVN::MakeDir(const CTSVNPathList& pathlist, CString message)
{
	svn_commit_info_t *commit_info = svn_create_commit_info(pool);
	message.Replace(_T("\r"), _T(""));
	m_pctx->log_msg_baton2 = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_mkdir2 (&commit_info,
							 MakePathArray(pathlist),
							 m_pctx,
							 pool);
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
	{
		for (int i=0; i<pathlist.GetCount(); ++i)
			Notify(pathlist[i], svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision, NULL, svn_wc_notify_lock_state_unchanged, NULL, pool);
	}

	return TRUE;
}

BOOL SVN::CleanUp(const CTSVNPath& path)
{
	Err = svn_client_cleanup (path.GetSVNApiPath(), m_pctx, pool);

	if(Err != NULL)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL SVN::Resolve(const CTSVNPath& path, BOOL recurse)
{
	SVNPool subpool(pool);
	Err = svn_client_resolved (path.GetSVNApiPath(),
								recurse,
								m_pctx,
								subpool);
	if(Err != NULL)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL SVN::Export(const CTSVNPath& srcPath, const CTSVNPath& destPath, SVNRev pegrev, SVNRev revision, BOOL force, BOOL bIgnoreExternals, HWND hWnd, BOOL extended)
{
	if (revision.IsWorking())
	{
		if (g_SVNAdminDir.IsAdminDirPath(srcPath.GetWinPath()))
			return FALSE;
		// files are special!
		if (!srcPath.IsDirectory())
		{
			CopyFile(srcPath.GetWinPath(), destPath.GetWinPath(), FALSE);
			return TRUE;
		}
		// our own "export" function with a callback and the ability to export
		// unversioned items too
		// BUGBUG: If a folder is marked as deleted, we export that folder too!
		if (extended)
		{
			CString srcfile;
			CShellFileOp fop;
			CDirFileEnum lister(srcPath.GetWinPathString());
			fop.AddSourceFile(srcPath.GetWinPath());
			fop.AddDestFile(destPath.GetWinPath());
			while (lister.NextFile(srcfile, NULL))
			{
				if (g_SVNAdminDir.IsAdminDirPath(srcfile))
					continue;
				if (!fop.AddSourceFile(srcfile))
				{
					Err = svn_error_create(NULL, NULL, CStringA(MAKEINTRESOURCE(IDS_ERR_NOTENOUGHMEMORY)));
					return FALSE;
				}
				CString destination = destPath.GetWinPathString() + _T("\\") + srcfile.Mid(srcPath.GetWinPathString().GetLength());
				destination.Replace(_T("\\\\"), _T("\\"));
				if (!fop.AddDestFile(destination))
				{
					Err = svn_error_create(NULL, NULL, CStringA(MAKEINTRESOURCE(IDS_ERR_NOTENOUGHMEMORY)));
					return FALSE;
				}
			}
			fop.SetOperationFlags(FO_COPY, hWnd, FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR | FOF_NO_CONNECTED_ELEMENTS | FOF_NORECURSION);
			fop.SetProgressDlgTitle(IDS_PROC_EXPORT_3);
			fop.Start();
			if (fop.AnyOperationsAborted())
			{
				Err = svn_error_create(NULL, NULL, CStringA(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED)));
				return FALSE;
			}
		}
		else
		{
			CTSVNPath statusPath;
			svn_wc_status2_t * s;
			SVNStatus status;
			CShellFileOp fop;
			if ((s = status.GetFirstFileStatus(srcPath, statusPath))!=0)
			{
				if (SVNStatus::GetMoreImportant(s->text_status, svn_wc_status_unversioned)!=svn_wc_status_unversioned)
				{
					CString src = statusPath.GetWinPathString();
					fop.AddSourceFile(src);
					CString destination = destPath.GetWinPathString() + _T("\\") + src.Mid(srcPath.GetWinPathString().GetLength());
					fop.AddDestFile(destination);
				}
				while ((s = status.GetNextFileStatus(statusPath))!=0)
				{
					if ((s->text_status == svn_wc_status_unversioned)||
						(s->text_status == svn_wc_status_ignored)||
						(s->text_status == svn_wc_status_none)||
						(s->text_status == svn_wc_status_missing)||
						(s->text_status == svn_wc_status_deleted))
						continue;
					
					CString src = statusPath.GetWinPathString();
					if (!fop.AddSourceFile(src))
					{
						Err = svn_error_create(NULL, NULL, CStringA(MAKEINTRESOURCE(IDS_ERR_NOTENOUGHMEMORY)));
						return FALSE;
					}
					CString destination = destPath.GetWinPathString() + _T("\\") + src.Mid(srcPath.GetWinPathString().GetLength());
					if (!fop.AddDestFile(destination))
					{
						Err = svn_error_create(NULL, NULL, CStringA(MAKEINTRESOURCE(IDS_ERR_NOTENOUGHMEMORY)));
						return FALSE;
					}
				}
				fop.SetOperationFlags(FO_COPY, hWnd, FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR | FOF_NO_CONNECTED_ELEMENTS | FOF_NORECURSION);
				fop.SetProgressDlgTitle(IDS_PROC_EXPORT_3);
				fop.Start();
				if (fop.AnyOperationsAborted())
				{
					Err = svn_error_create(NULL, NULL, CStringA(MAKEINTRESOURCE(IDS_SVN_USERCANCELLED)));
					return FALSE;
				}
			}
			else
			{
				Err = svn_error_create(status.m_err->apr_err, status.m_err, NULL);
				return FALSE;
			}
		} // else from if (extended)
	}
	else
	{
		Err = svn_client_export3(NULL,		//no resulting revision needed
			srcPath.GetSVNApiPath(),
			destPath.GetSVNApiPath(),
			pegrev,
			revision,
			force,
			bIgnoreExternals,
			TRUE,
			NULL,
			m_pctx,
			pool);
		if(Err != NULL)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL SVN::Switch(const CTSVNPath& path, const CTSVNPath& url, SVNRev revision, BOOL recurse)
{
	SVNPool subpool(pool);
	Err = svn_client_switch(NULL,
							path.GetSVNApiPath(),
							url.GetSVNApiPath(),
							revision,
							recurse,
							m_pctx,
							subpool);
	if(Err != NULL)
	{
		return FALSE;
	}
	
	return TRUE;
}

BOOL SVN::Import(const CTSVNPath& path, const CTSVNPath& url, CString message, BOOL recurse, BOOL no_ignore)
{
	svn_commit_info_t *commit_info = svn_create_commit_info(pool);
	message.Replace(_T("\r"), _T(""));
	m_pctx->log_msg_baton2 = logMessage(CUnicodeUtils::GetUTF8(message));
	Err = svn_client_import2 (&commit_info,
							path.GetSVNApiPath(),
							url.GetSVNApiPath(),
							!recurse,
							no_ignore,
							m_pctx,
							pool);
	m_pctx->log_msg_baton2 = logMessage("");
	if(Err != NULL)
	{
		return FALSE;
	}
	if (commit_info && SVN_IS_VALID_REVNUM (commit_info->revision))
		Notify(path, svn_wc_notify_update_completed, svn_node_none, _T(""), svn_wc_notify_state_unknown, svn_wc_notify_state_unknown, commit_info->revision, NULL, svn_wc_notify_lock_state_unchanged, NULL, pool);
	return TRUE;
}

BOOL SVN::Merge(const CTSVNPath& path1, SVNRev revision1, const CTSVNPath& path2, SVNRev revision2, const CTSVNPath& localPath, BOOL force, BOOL recurse, BOOL ignoreanchestry, BOOL dryrun)
{
	SVNPool subpool(pool);

	Err = svn_client_merge2(path1.GetSVNApiPath(),
							revision1,
							path2.GetSVNApiPath(),
							revision2,
							localPath.GetSVNApiPath(),
							recurse,
							ignoreanchestry,
							force,
							dryrun,
							NULL,
							m_pctx,
							subpool);
	if(Err != NULL)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL SVN::PegMerge(const CTSVNPath& source, SVNRev revision1, SVNRev revision2, SVNRev pegrevision, const CTSVNPath& destpath, BOOL force, BOOL recurse, BOOL ignoreancestry, BOOL dryrun)
{
	SVNPool subpool(pool);

	Err = svn_client_merge_peg2 (source.GetSVNApiPath(),
		revision1,
		revision2,
		pegrevision,
		destpath.GetSVNApiPath(),
		recurse,
		ignoreancestry,
		force,
		dryrun,
		NULL,
		m_pctx,
		subpool);
	if(Err != NULL)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL SVN::Diff(const CTSVNPath& path1, SVNRev revision1, const CTSVNPath& path2, SVNRev revision2, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, BOOL ignorecontenttype,  CString options, bool bAppend, const CTSVNPath& outputfile)
{
	return Diff(path1, revision1, path2, revision2, recurse, ignoreancestry, nodiffdeleted, ignorecontenttype, options, bAppend, outputfile, CTSVNPath());
}

BOOL SVN::Diff(const CTSVNPath& path1, SVNRev revision1, const CTSVNPath& path2, SVNRev revision2, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, BOOL ignorecontenttype,  CString options, bool bAppend, const CTSVNPath& outputfile, const CTSVNPath& errorfile)
{
	BOOL del = FALSE;
	apr_file_t * outfile;
	apr_file_t * errfile;
	apr_array_header_t *opts;

	SVNPool localpool(pool);

	opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, localpool);

	apr_int32_t flags = APR_WRITE | APR_CREATE | APR_BINARY;
	if (bAppend)
		flags |= APR_APPEND;
	else
		flags |= APR_TRUNCATE;
	Err = svn_io_file_open (&outfile, outputfile.GetSVNApiPath(),
							flags,
							APR_OS_DEFAULT, localpool);
	if (Err)
		return FALSE;

	CTSVNPath workingErrorFile;
	if (errorfile.IsEmpty())
	{
		workingErrorFile = CTempFiles::Instance().GetTempFilePath(true);
		del = TRUE;
	}
	else
	{
		workingErrorFile = errorfile;
	}

	Err = svn_io_file_open (&errfile, workingErrorFile.GetSVNApiPath(),
							APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
							APR_OS_DEFAULT, localpool);
	if (Err)
		return FALSE;

	Err = svn_client_diff3 (opts,
						   path1.GetSVNApiPath(),
						   revision1,
						   path2.GetSVNApiPath(),
						   revision2,
						   recurse,
						   ignoreancestry,
						   nodiffdeleted,
						   ignorecontenttype,
						   APR_LOCALE_CHARSET,
						   outfile,
						   errfile,
						   m_pctx,
						   localpool);
	if (Err)
	{
		return FALSE;
	}
	if (del)
	{
		svn_io_remove_file (workingErrorFile.GetSVNApiPath(), localpool);
	}
	return TRUE;
}

BOOL SVN::PegDiff(const CTSVNPath& path, SVNRev pegrevision, SVNRev startrev, SVNRev endrev, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, BOOL ignorecontenttype,  CString options, const CTSVNPath& outputfile)
{
	return PegDiff(path, pegrevision, startrev, endrev, recurse, ignoreancestry, nodiffdeleted, ignorecontenttype, options, outputfile, CTSVNPath());
}

BOOL SVN::PegDiff(const CTSVNPath& path, SVNRev pegrevision, SVNRev startrev, SVNRev endrev, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, BOOL ignorecontenttype,  CString options, const CTSVNPath& outputfile, const CTSVNPath& errorfile)
{
	BOOL del = FALSE;
	apr_file_t * outfile;
	apr_file_t * errfile;
	apr_array_header_t *opts;

	SVNPool localpool(pool);

	opts = svn_cstring_split (CUnicodeUtils::GetUTF8(options), " \t\n\r", TRUE, localpool);

	Err = svn_io_file_open (&outfile, outputfile.GetSVNApiPath(),
		APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
		APR_OS_DEFAULT, localpool);
	if (Err)
		return FALSE;

	CTSVNPath workingErrorFile;
	if (errorfile.IsEmpty())
	{
		workingErrorFile = CTempFiles::Instance().GetTempFilePath(true);
		del = TRUE;
	}
	else
	{
		workingErrorFile = errorfile;
	}

	Err = svn_io_file_open (&errfile, workingErrorFile.GetSVNApiPath(),
		APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY,
		APR_OS_DEFAULT, localpool);
	if (Err)
		return FALSE;

	Err = svn_client_diff_peg3 (opts,
		path.GetSVNApiPath(),
		pegrevision,
		startrev,
		endrev,
		recurse,
		ignoreancestry,
		nodiffdeleted,
		ignorecontenttype,
		APR_LOCALE_CHARSET,
		outfile,
		errfile,
		m_pctx,
		localpool);
	if (Err)
	{
		return FALSE;
	}
	if (del)
	{
		svn_io_remove_file (workingErrorFile.GetSVNApiPath(), localpool);
	}
	return TRUE;
}

bool SVN::DiffSummarize(const CTSVNPath& path1, SVNRev rev1, const CTSVNPath& path2, SVNRev rev2, bool recurse, bool ignoreancestry)
{
	SVNPool localpool(pool);
	Err = svn_client_diff_summarize(path1.GetSVNApiPath(), rev1,
									path2.GetSVNApiPath(), rev2,
									recurse, ignoreancestry,
									summarize_func, this,
									m_pctx, localpool);
	if(Err != NULL)
	{
		return false;
	}
	return true;
}

bool SVN::DiffSummarizePeg(const CTSVNPath& path, SVNRev peg, SVNRev rev1, SVNRev rev2, bool recurse, bool ignoreancestry)
{
	SVNPool localpool(pool);
	Err = svn_client_diff_summarize_peg(path.GetSVNApiPath(), peg, rev1, rev2,
										recurse, ignoreancestry,
										summarize_func, this,
										m_pctx, localpool);
	if(Err != NULL)
	{
		return false;
	}
	return true;
}

BOOL SVN::ReceiveLog(const CTSVNPathList& pathlist, SVNRev revisionPeg, SVNRev revisionStart, SVNRev revisionEnd, int limit, BOOL changed, BOOL strict /* = FALSE */)
{
	SVNPool localpool(pool);
	Err = svn_client_log3 (MakePathArray(pathlist), 
						revisionPeg,
						revisionStart, 
						revisionEnd, 
						limit,
						changed,
						strict,
						logReceiver,	// log_message_receiver
						(void *)this, m_pctx, localpool);

	if(Err != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL SVN::Cat(const CTSVNPath& url, SVNRev pegrevision, SVNRev revision, const CTSVNPath& localpath)
{
	apr_file_t * file;
	svn_stream_t * stream;
	apr_status_t status;
	SVNPool localpool(pool);

	CTSVNPath fullLocalPath(localpath);
	if (fullLocalPath.IsDirectory())
	{
		fullLocalPath.AppendPathString(url.GetFileOrDirectoryName());
	}
	::DeleteFile(fullLocalPath.GetWinPath());

	status = apr_file_open(&file, fullLocalPath.GetSVNApiPath(), APR_WRITE | APR_CREATE | APR_TRUNCATE, APR_OS_DEFAULT, localpool);
	if (status)
	{
		Err = svn_error_wrap_apr(status, NULL);
		return FALSE;
	}
	stream = svn_stream_from_aprfile(file, localpool);

	Err = svn_client_cat2(stream, url.GetSVNApiPath(), pegrevision, revision, m_pctx, localpool);

	apr_file_close(file);
	if (Err != NULL)
		return FALSE;
	return TRUE;
}

BOOL SVN::CreateRepository(CString path, CString fstype)
{
	svn_repos_t * repo;
	svn_error_t * err;
	apr_hash_t *config;

	SVNPool localpool;

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", localpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", localpool);
	
	apr_hash_t *fs_config = apr_hash_make (localpool);;

	apr_hash_set (fs_config, SVN_FS_CONFIG_BDB_TXN_NOSYNC,
		APR_HASH_KEY_STRING, "0");

	apr_hash_set (fs_config, SVN_FS_CONFIG_BDB_LOG_AUTOREMOVE,
		APR_HASH_KEY_STRING, "1");

	err = svn_config_get_config (&config, g_pConfigDir, localpool);
	if (err != NULL)
	{
		return FALSE;
	}
	const char * fs_type = apr_pstrdup(localpool, CStringA(fstype));
	apr_hash_set (fs_config, SVN_FS_CONFIG_FS_TYPE,
		APR_HASH_KEY_STRING,
		fs_type);
	err = svn_repos_create(&repo, MakeSVNUrlOrPath(path), NULL, NULL, config, fs_config, localpool);
	if (err != NULL)
	{
		return FALSE;
	} // if (err != NULL) 
	return TRUE;
}

BOOL SVN::Blame(const CTSVNPath& path, SVNRev startrev, SVNRev endrev, SVNRev peg, bool ignoremimetype)
{
	svn_diff_file_options_t * options = svn_diff_file_options_create(pool);
	options->ignore_space = svn_diff_file_ignore_space_none;
	options->ignore_eol_style = true;
	// Subversion < 1.4 silently changed a revision WC to BASE. Due to a bug
	// report this was changed: now Subversion returns an error 'not implemented'
	// since it actually blamed the BASE file and not the working copy file.
	// Until that's implemented, we 'fall back' here to the old behavior and
	// just change and REV_WC to REV_BASE.
	if (peg.IsWorking())
		peg = SVNRev::REV_BASE;
	if (startrev.IsWorking())
		startrev = SVNRev::REV_BASE;
	if (endrev.IsWorking())
		endrev = SVNRev::REV_BASE;
	Err = svn_client_blame3 ( path.GetSVNApiPath(),
							 peg,
							 startrev,  
							 endrev,
							 options,
							 ignoremimetype,
							 blameReceiver,  
							 (void *)this,  
							 m_pctx,  
							 pool );

	if(Err != NULL)
	{
		return FALSE;
	}
	return TRUE;
}

svn_error_t* SVN::blameReceiver(void* baton,
								apr_off_t line_no,
								svn_revnum_t revision,
								const char * author,
								const char * date,
								const char * line,
								apr_pool_t * pool)
{
	svn_error_t * error = NULL;
	CString author_native;
	CStringA line_native;
	TCHAR date_native[SVN_DATE_BUFFER] = {0};

	SVN * svn = (SVN *)baton;

	if (author)
		author_native = CUnicodeUtils::GetUnicode(author);
	if (line)
		line_native = line;

	if (date && date[0])
	{
		//Convert date to a format for humans.
		apr_time_t time_temp;

		error = svn_time_from_cstring (&time_temp, date, pool);
		if (error)
			return error;

		formatDate(date_native, time_temp, true);
	}
	else
		_tcscat_s(date_native, SVN_DATE_BUFFER, _T("(no date)"));

	if (!svn->BlameCallback((LONG)line_no, revision, author_native, date_native, line_native))
	{
		return svn_error_create(SVN_ERR_CANCELLED, NULL, "error in blame callback");
	}
	return error;
}

BOOL SVN::Lock(const CTSVNPathList& pathList, BOOL bStealLock, const CString& comment /* = CString( */)
{
	Err = svn_client_lock(MakePathArray(pathList), CUnicodeUtils::GetUTF8(comment), bStealLock, m_pctx, pool);
	return (Err == NULL);	
}

BOOL SVN::Unlock(const CTSVNPathList& pathList, BOOL bBreakLock)
{
	Err = svn_client_unlock(MakePathArray(pathList), bBreakLock, m_pctx, pool);
	return (Err == NULL);
}

svn_error_t* SVN::summarize_func(const svn_client_diff_summarize_t *diff, void *baton, apr_pool_t * /*pool*/)
{
	SVN * svn = (SVN *)baton;
	if (diff)
	{
		CTSVNPath path = CTSVNPath(CUnicodeUtils::GetUnicode(diff->path));
		return svn->DiffSummarizeCallback(path, diff->summarize_kind, !!diff->prop_changed, diff->node_kind);
	}
	return SVN_NO_ERROR;
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
	TCHAR date_native[SVN_DATE_BUFFER] = {0};
	CString author_native;
	CString msg_native;
	DWORD actions = 0;

	SVN * svn = (SVN *)baton;
	author_native = CUnicodeUtils::GetUnicode(author);
	apr_time_t time_temp = NULL;

	if (date && date[0])
	{
		//Convert date to a format for humans.
		error = svn_time_from_cstring (&time_temp, date, pool);
		if (error)
			return error;

		formatDate(date_native, time_temp);
	}
	else
		_tcscat_s(date_native, SVN_DATE_BUFFER, _T("(no date)"));

	if (msg == NULL)
		msg = "";

	msg_native = CUnicodeUtils::GetUnicode(msg);
	int filechanges = 0;
	BOOL copies = FALSE;
	LogChangedPathArray * arChangedPaths = new LogChangedPathArray;
	try
	{
		if (ch_paths)
		{
			CString sModifiedStatus, sReplacedStatus, sAddStatus, sDeleteStatus;
			sModifiedStatus.LoadString(IDS_SVNACTION_MODIFIED);
			sReplacedStatus.LoadString(IDS_SVNACTION_REPLACED);
			sAddStatus.LoadString(IDS_SVNACTION_ADD);
			sDeleteStatus.LoadString(IDS_SVNACTION_DELETE);
			apr_array_header_t *sorted_paths;
			sorted_paths = svn_sort__hash(ch_paths, svn_sort_compare_items_as_paths, pool);
			filechanges = sorted_paths->nelts;
			for (int i = 0; i < sorted_paths->nelts; i++)
			{
				LogChangedPath * changedpath = new LogChangedPath;
				svn_sort__item_t *item = &(APR_ARRAY_IDX (sorted_paths, i, svn_sort__item_t));
				CString path_native;
				const char *path = (const char *)item->key;
				svn_log_changed_path_t *log_item = (svn_log_changed_path_t *)apr_hash_get (ch_paths, item->key, item->klen);
				path_native = MakeUIUrlOrPath(path);
				changedpath->sPath = path_native;
				switch (log_item->action)
				{
				case 'M':
					changedpath->sAction = sModifiedStatus;
					actions |= LOGACTIONS_MODIFIED;
					break;
				case 'R':
					changedpath->sAction = sReplacedStatus;
					actions |= LOGACTIONS_REPLACED;
					break;
				case 'A':
					changedpath->sAction = sAddStatus;
					actions |= LOGACTIONS_ADDED;
					break;
				case 'D':
					changedpath->sAction = sDeleteStatus;
					actions |= LOGACTIONS_DELETED;
				default:
					break;
				}
				if (log_item->copyfrom_path && SVN_IS_VALID_REVNUM (log_item->copyfrom_rev))
				{
					changedpath->sCopyFromPath = MakeUIUrlOrPath(log_item->copyfrom_path);
					changedpath->lCopyFromRev = log_item->copyfrom_rev;
					copies = TRUE;
				}
				else
				{
					changedpath->lCopyFromRev = 0;
				}
				arChangedPaths->Add(changedpath);
			} // for (int i = 0; i < sorted_paths->nelts; i++) 
		} // if (ch_paths)
	}
	catch (CMemoryException * e)
	{
		e->Delete();
	}
#pragma warning(push)
#pragma warning(disable: 4127)	// conditional expression is constant
	SVN_ERR (svn->cancel(baton));
#pragma warning(pop)

	if (svn->Log(rev, author_native, date_native, msg_native, arChangedPaths, time_temp, filechanges, copies, actions))
	{
		return error;
	}
	return error;
}

void SVN::notify( void *baton,
				 const svn_wc_notify_t *notify,
				 apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;

	CTSVNPath tsvnPath;
	tsvnPath.SetFromSVN(notify->path);

	CString mime;
	if (notify->mime_type)
		mime = CUnicodeUtils::GetUnicode(notify->mime_type);

	svn->Notify(tsvnPath, notify->action, notify->kind, 
				mime, notify->content_state, 
				notify->prop_state, notify->revision, 
				notify->lock, notify->lock_state, notify->err, pool);
}

svn_error_t* SVN::cancel(void *baton)
{
	SVN * svn = (SVN *)baton;
	if ((svn->Cancel())||((svn->m_pProgressDlg)&&(svn->m_pProgressDlg->HasUserCancelled())))
	{
		CStringA temp;
		temp.LoadString(IDS_SVN_USERCANCELLED);
		return svn_error_create(SVN_ERR_CANCELLED, NULL, temp);
	}
	return SVN_NO_ERROR;
}

void * SVN::logMessage (const char * message, char * baseDirectory)
{
	log_msg_baton2* baton = (log_msg_baton2 *) apr_palloc (pool, sizeof (*baton));
	baton->message = apr_pstrdup(pool, message);
	baton->base_dir = baseDirectory ? baseDirectory : "";

	baton->message_encoding = NULL;
	baton->tmpfile_left = NULL;
	baton->pool = pool;

	return baton;
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
	SVN::preparePath(url);
}

void	SVN::preparePath(CString &path)
{
	path.Trim();
	path.TrimRight(_T("/\\"));			//remove trailing slashes
	path.Replace('\\','/');
	//workaround for Subversions UNC-path bug
	if (path.Left(10).CompareNoCase(_T("file://///"))==0)
	{
		path.Replace(_T("file://///"), _T("file:///\\"));
	}
	else if (path.Left(9).CompareNoCase(_T("file:////"))==0)
	{
		path.Replace(_T("file:////"), _T("file:///\\"));
	}
}

svn_error_t* svn_cl__get_log_message (const char **log_msg,
									const char **tmp_file,
									const apr_array_header_t * /*commit_items*/,
									void *baton, 
									apr_pool_t * pool)
{
	log_msg_baton2 *lmb = (log_msg_baton2 *) baton;
	*tmp_file = NULL;
	if (lmb->message)
	{
		*log_msg = apr_pstrdup (pool, lmb->message);
	}

	return SVN_NO_ERROR;
}

CString SVN::GetURLFromPath(const CTSVNPath& path)
{
	const char * URL;
	Err = get_url_from_target(&URL, path.GetSVNApiPath());
	if (Err)
		return _T("");
	if (URL==NULL)
		return _T("");
	return CString(URL);
}

CString SVN::GetUIURLFromPath(const CTSVNPath& path)
{
	const char * URL;
	Err = get_url_from_target(&URL, path.GetSVNApiPath());
	if (Err)
		return _T("");
	if (URL==NULL)
		return _T("");
	return MakeUIUrlOrPath(URL);
}

svn_error_t * SVN::get_url_from_target (const char **URL, const char *target)
{
	svn_wc_adm_access_t *adm_access;          
	const svn_wc_entry_t *entry;  
	const char * canontarget = svn_path_canonicalize(target, pool);
	svn_boolean_t is_url = svn_path_is_url (canontarget);

	if (is_url)
		*URL = apr_pstrdup(pool, canontarget);

	else
	{
#pragma warning(push)
#pragma warning(disable: 4127)	// conditional expression is constant
		SVN_ERR (svn_wc_adm_probe_open2 (&adm_access, NULL, canontarget,
			FALSE, 0, pool));
		SVN_ERR (svn_wc_entry (&entry, canontarget, adm_access, FALSE, pool));
		SVN_ERR (svn_wc_adm_close (adm_access));
#pragma warning(pop)

		*URL = entry ? entry->url : NULL;
	}

	return SVN_NO_ERROR;
}

CString SVN::GetUUIDFromPath(const CTSVNPath& path)
{
	const char * UUID;
	if (PathIsURL(path.GetSVNPathString()))
	{
		Err = svn_client_uuid_from_url(&UUID, path.GetSVNApiPath(), m_pctx, pool);
	}
	else
	{
		Err = get_uuid_from_target(&UUID, path.GetSVNApiPath());
	}
	if (Err)
		return _T("");
	if (UUID==NULL)
		return _T("");
	CString ret = CString(UUID);
	return ret;
}

svn_error_t * SVN::get_uuid_from_target (const char **UUID, const char *target)
{
	svn_wc_adm_access_t *adm_access;          
#pragma warning(push)
#pragma warning(disable: 4127)	// conditional expression is constant
	SVN_ERR (svn_wc_adm_probe_open2 (&adm_access, NULL, target,
		FALSE, 0, pool));
	SVN_ERR (svn_client_uuid_from_path(UUID, target, adm_access, m_pctx, pool));
	SVN_ERR (svn_wc_adm_close (adm_access));
#pragma warning(pop)

	return SVN_NO_ERROR;
}

BOOL SVN::Ls(const CTSVNPath& url, SVNRev pegrev, SVNRev revision, CStringArray& entries, BOOL extended, BOOL recursive, BOOL escaped)
{
	entries.RemoveAll();
	SVNPool subpool(pool);

	apr_hash_t* hash = apr_hash_make(subpool);
	apr_hash_t* lockhash = apr_hash_make(subpool);

	Err = svn_client_ls3(&hash,
						&lockhash,
						url.GetSVNApiPath(),
						pegrev,
						revision,
						recursive, 
						m_pctx,
						subpool);
	if (Err != NULL)
	{
		return FALSE;
	}

	apr_hash_index_t *hi;
    svn_dirent_t* val;
	const char* key;
	CStringA sKey;
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
		if (escaped)
		{
			sKey = key;
			// the '%' char isn't automatically escaped by CPathUtils::PathEscape(),
			// so we have to do that here manually.
			sKey.Replace("%", "%25");
			temp = temp + CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(sKey));
		}
		else
		{
			temp = temp + MakeUIUrlOrPath(key);
		}
		if (extended)
		{
			CString author, revnum, size, dateval;
			TCHAR date_native[SVN_DATE_BUFFER];
			author = CUnicodeUtils::GetUnicode(val->last_author);
			revnum.Format(_T("%u"), val->created_rev);
			if (val->kind != svn_node_dir)
				size.Format(_T("%u KB"), (val->size+1023)/1024);
			formatDate(date_native, val->time, true);
			dateval.Format(_T("%I64u"), val->time);
			temp = temp + _T("\t") + revnum + _T("\t") + author + _T("\t") + size + _T("\t") + date_native + _T("\t") + dateval;;
			svn_lock_t * lock = (svn_lock_t *)apr_hash_get(lockhash, key, APR_HASH_KEY_STRING);
			if (lock)
			{
				// we have lock information for this item.
				temp += _T("\t");
				if (lock->owner)
				{
					temp += lock->owner;
				}
				temp += _T("\t");
				if (lock->comment)
				{
					temp += lock->comment;
				}
			}
		}
		entries.Add(temp);
    }
	return Err == NULL;
}

BOOL SVN::Relocate(const CTSVNPath& path, const CTSVNPath& from, const CTSVNPath& to, BOOL recurse)
{
	Err = svn_client_relocate(
				path.GetSVNApiPath(), 
				from.GetSVNApiPath(), 
				to.GetSVNApiPath(), 
				recurse, m_pctx, pool);
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
		if (PathFileExists(url))
		{
			Err = svn_repos_open (&pRepos, MakeSVNUrlOrPath(url), pool);
			if ((Err)&&(Err->apr_err == SVN_ERR_FS_BERKELEY_DB))
				return TRUE;
			if (Err == NULL)
				return TRUE;
		}
	}

	return Err == NULL;
}

BOOL SVN::IsBDBRepository(CString url)
{
	preparePath(url);
	url = url.Mid(7);
	url.TrimLeft('/');
	while (!url.IsEmpty())
	{
		if (PathIsDirectory(url + _T("/db")))
		{
			if (PathFileExists(url + _T("/db/fs-type")))
				return FALSE;
			return TRUE;
		}
		if (url.ReverseFind('/')>=0)
			url = url.Left(url.ReverseFind('/'));
		else
			url.Empty();
	}
	return FALSE;
}

CString SVN::GetRepositoryRoot(const CTSVNPath& url)
{
	const char * returl;
	svn_ra_session_t *ra_session;

	SVNPool localpool(pool);

	// make sure the url is canonical.
	const char * goodurl = svn_path_canonicalize(url.GetSVNApiPath(), localpool);
	
	/* use subpool to create a temporary RA session */
	Err = svn_client_open_ra_session (&ra_session, goodurl, m_pctx, localpool);
	if (Err)
		return _T("");
	
	Err = svn_ra_get_repos_root(ra_session, &returl, localpool);
	if (Err)
		return _T("");

	return CString(returl);
}

svn_revnum_t SVN::GetHEADRevision(const CTSVNPath& url)
{
	svn_ra_session_t *ra_session;
	const char * urla;
	svn_revnum_t rev;

	SVNPool localpool(pool);
	if (!url.IsUrl())
		SVN::get_url_from_target(&urla, url.GetSVNApiPath());
	else
	{
		// make sure the url is canonical.
		const char * goodurl = svn_path_canonicalize(url.GetSVNApiPath(), localpool);
		urla = goodurl;
	}
	if (urla == NULL)
		return -1;

	/* use subpool to create a temporary RA session */
	Err = svn_client_open_ra_session (&ra_session, urla, m_pctx, localpool);
	if (Err)
		return -1;

	Err = svn_ra_get_latest_revnum(ra_session, &rev, localpool);
	if (Err)
		return -1;
	return rev;
}

BOOL SVN::GetRootAndHead(const CTSVNPath& path, CTSVNPath& url, svn_revnum_t& rev)
{
	svn_ra_session_t *ra_session;
	const char * urla;
	const char * returl;

	SVNPool localpool(pool);
	if (!path.IsUrl())
		SVN::get_url_from_target(&urla, path.GetSVNApiPath());
	else
	{
		// make sure the url is canonical.
		urla = svn_path_canonicalize(path.GetSVNApiPath(), localpool);
	}

	if (urla == NULL)
		return FALSE;

	/* use subpool to create a temporary RA session */
	Err = svn_client_open_ra_session (&ra_session, urla, m_pctx, localpool);
	if (Err)
		return FALSE;

	Err = svn_ra_get_latest_revnum(ra_session, &rev, localpool);
	if (Err)
		return FALSE;

	Err = svn_ra_get_repos_root(ra_session, &returl, localpool);
	if (Err)
		return FALSE;
		
	url.SetFromSVN(CUnicodeUtils::GetUnicode(returl));
	
	return TRUE;
}

BOOL SVN::GetLocks(const CTSVNPath& url, std::map<CString, SVNLock> * locks)
{
	svn_ra_session_t *ra_session;

	SVNPool localpool(pool);
	apr_hash_t * hash = apr_hash_make(localpool);

	/* use subpool to create a temporary RA session */
	Err = svn_client_open_ra_session (&ra_session, url.GetSVNApiPath(), m_pctx, localpool);
	if (Err != NULL)
		return FALSE;

	Err = svn_ra_get_locks(ra_session, &hash, "", localpool);
	if (Err != NULL)
		return FALSE;
	apr_hash_index_t *hi;
	svn_lock_t* val;
	const char* key;
	for (hi = apr_hash_first(localpool, hash); hi; hi = apr_hash_next(hi)) 
	{
		apr_hash_this(hi, (const void**)&key, NULL, (void**)&val);
		if (val)
		{
			SVNLock lock;
			lock.comment = CUnicodeUtils::GetUnicode(val->comment);
			lock.creation_date = val->creation_date/1000000L;
			lock.expiration_date = val->expiration_date/1000000L;
			lock.owner = CUnicodeUtils::GetUnicode(val->owner);
			lock.path = CUnicodeUtils::GetUnicode(val->path);
			lock.token = CUnicodeUtils::GetUnicode(val->token);
			CString sKey = CUnicodeUtils::GetUnicode(key);
			(*locks)[sKey] = lock;
		}
	}
	return TRUE;
}

svn_revnum_t SVN::RevPropertySet(CString sName, CString sValue, CString sURL, SVNRev rev)
{
	svn_revnum_t set_rev;
	svn_string_t*	pval;
	sValue.Replace(_T("\r"), _T(""));
	pval = svn_string_create (CUnicodeUtils::GetUTF8(sValue), pool);
	Err = svn_client_revprop_set(MakeSVNUrlOrPath(sName), 
									pval, 
									MakeSVNUrlOrPath(sURL), 
									rev, 
									&set_rev, 
									FALSE, 
									m_pctx, 
									pool);
	if (Err)
		return 0;
	return set_rev;
}

CString SVN::RevPropertyGet(CString sName, CString sURL, SVNRev rev)
{
	svn_string_t *propval;
	svn_revnum_t set_rev;
	Err = svn_client_revprop_get(MakeSVNUrlOrPath(sName), &propval, MakeSVNUrlOrPath(sURL), rev, &set_rev, m_pctx, pool);
	if (Err)
		return _T("");
	if (propval==NULL)
		return _T("");
	if (propval->len <= 0)
		return _T("");
	return CUnicodeUtils::GetUnicode(propval->data);
}

CTSVNPath SVN::GetPristinePath(const CTSVNPath& wcPath)
{
	svn_error_t * err;
	SVNPool localpool;

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", localpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", localpool);

	const char* pristinePath = NULL;
	CTSVNPath returnPath;

	err = svn_wc_get_pristine_copy_path(svn_path_internal_style(wcPath.GetSVNApiPath(), localpool), &pristinePath, localpool);

	if (err != NULL)
	{
		return returnPath;
	}
	if (pristinePath != NULL)
	{
		returnPath.SetFromSVN(pristinePath);
	}
	return returnPath;
}

BOOL SVN::GetTranslatedFile(CTSVNPath& sTranslatedFile, const CTSVNPath sFile, BOOL bForceRepair /*= TRUE*/)
{
	svn_wc_adm_access_t *adm_access;          
	svn_error_t * err;
	SVNPool localpool;

	const char * deststr = NULL;
	svn_utf_cstring_to_utf8(&deststr, "dummy", localpool);
	svn_utf_cstring_from_utf8(&deststr, "dummy", localpool);

	const char * translatedPath = NULL;
	CStringA temp = sFile.GetSVNApiPath();
	const char * originPath = svn_path_canonicalize(temp, localpool);
	err = svn_wc_adm_probe_open2 (&adm_access, NULL, originPath, FALSE, 0, localpool);
	if (err)
		return FALSE;
	err = svn_wc_translated_file((const char **)&translatedPath, originPath, adm_access, bForceRepair, localpool);
	svn_wc_adm_close(adm_access);
	if (err)
		return FALSE;
	
	sTranslatedFile.SetFromUnknown(MakeUIUrlOrPath(translatedPath));
	return (translatedPath != originPath);
}

BOOL SVN::PathIsURL(const CString& path)
{
	return svn_path_is_url(MakeSVNUrlOrPath(path));
} 

void SVN::formatDate(TCHAR date_native[], apr_time_t& date_svn, bool force_short_fmt)
{
	date_native[0] = '\0';
	apr_time_exp_t exploded_time = {0};
	
	SYSTEMTIME systime;
	TCHAR timebuf[SVN_DATE_BUFFER];
	TCHAR datebuf[SVN_DATE_BUFFER];

	LCID locale = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
	locale = MAKELCID(locale, SORT_DEFAULT);

	apr_time_exp_lt (&exploded_time, date_svn);
	
	systime.wDay = (WORD)exploded_time.tm_mday;
	systime.wDayOfWeek = (WORD)exploded_time.tm_wday;
	systime.wHour = (WORD)exploded_time.tm_hour;
	systime.wMilliseconds = (WORD)(exploded_time.tm_usec/1000);
	systime.wMinute = (WORD)exploded_time.tm_min;
	systime.wMonth = (WORD)exploded_time.tm_mon+1;
	systime.wSecond = (WORD)exploded_time.tm_sec;
	systime.wYear = (WORD)exploded_time.tm_year+1900;
	if (force_short_fmt || CRegDWORD(_T("Software\\TortoiseSVN\\LogDateFormat")) == 1)
	{
		GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, SVN_DATE_BUFFER);
		GetTimeFormat(locale, 0, &systime, NULL, timebuf, SVN_DATE_BUFFER);
		_tcsncat_s(date_native, SVN_DATE_BUFFER, datebuf, SVN_DATE_BUFFER);
		_tcsncat_s(date_native, SVN_DATE_BUFFER, _T(" "), SVN_DATE_BUFFER);
		_tcsncat_s(date_native, SVN_DATE_BUFFER, timebuf, SVN_DATE_BUFFER);
	}
	else
	{
		GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, SVN_DATE_BUFFER);
		GetTimeFormat(locale, 0, &systime, NULL, timebuf, SVN_DATE_BUFFER);
		_tcsncat_s(date_native, SVN_DATE_BUFFER, timebuf, SVN_DATE_BUFFER);
		_tcsncat_s(date_native, SVN_DATE_BUFFER, _T(", "), SVN_DATE_BUFFER);
		_tcsncat_s(date_native, SVN_DATE_BUFFER, datebuf, SVN_DATE_BUFFER);
	}
}

CStringA SVN::MakeSVNUrlOrPath(const CString& UrlOrPath)
{
	CStringA url = CUnicodeUtils::GetUTF8(UrlOrPath);
	if (svn_path_is_url(url))
	{
		url = CPathUtils::PathEscape(url);
	}
	return url;
}

CString SVN::MakeUIUrlOrPath(CStringA UrlOrPath)
{
	if (svn_path_is_url(UrlOrPath))
	{
		CPathUtils::Unescape(UrlOrPath.GetBuffer());
		UrlOrPath.ReleaseBuffer();
	}
	CString url = CUnicodeUtils::GetUnicode(UrlOrPath);
	return url;
}

BOOL SVN::EnsureConfigFile()
{
	svn_error_t * err;
	SVNPool localpool;
	err = svn_config_ensure(NULL, localpool);
	if (err)
	{
		return FALSE;
	}
	return TRUE;
}

void SVN::UseIEProxySettings(apr_hash_t * cfg)
{
	CStringA exceptions;
	CStringA port;
	CStringA proxy;
	CStringA temp;
	const char * valuep;
	INTERNET_PER_CONN_OPTION_LIST    List;
	INTERNET_PER_CONN_OPTION         Option[5];
	unsigned long                    nSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);

	Option[0].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
	Option[1].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
	Option[2].dwOption = INTERNET_PER_CONN_FLAGS;
	Option[3].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
	Option[4].dwOption = INTERNET_PER_CONN_PROXY_SERVER;

	List.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
	List.pszConnection = NULL;
	List.dwOptionCount = 5;
	List.dwOptionError = 0;
	List.pOptions = Option;

	CString server = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-host"), _T(""));
	if (!server.IsEmpty())
		return;

	if(!InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &List, &nSize))
		return;

	if((Option[2].Value.dwValue & PROXY_TYPE_AUTO_PROXY_URL) == PROXY_TYPE_AUTO_PROXY_URL)
		goto ERROR_LABEL;

	if((Option[2].Value.dwValue & PROXY_TYPE_AUTO_DETECT) == PROXY_TYPE_AUTO_DETECT)
		goto ERROR_LABEL;

	exceptions = CStringA(Option[3].Value.pszValue);
	exceptions.Replace(';', ',');
	proxy = CStringA(Option[4].Value.pszValue);
	if (proxy.Find(';')>=0)
	{
		if (proxy.Find("http=")>=0)
		{
			temp = proxy.Mid(proxy.Find("http=")+5);
			temp = temp.Left(temp.Find(';'));
		}
		if ((temp.IsEmpty())&&(proxy.Find("https=")>=0))
		{
			temp = proxy.Mid(proxy.Find("https=")+6);
			temp = temp.Left(temp.Find(';'));
		}
		proxy = temp;
	}
	if (proxy.Find(':')>=0)
	{
		port = proxy.Mid(proxy.Find(':')+1);
		proxy = proxy.Left(proxy.Find(':'));
	}
	if (proxy.IsEmpty())
		goto ERROR_LABEL;

	svn_config_t * opt = (svn_config_t *)apr_hash_get (cfg, SVN_CONFIG_CATEGORY_SERVERS,
		APR_HASH_KEY_STRING);
	svn_config_get(opt, &valuep, SVN_CONFIG_SECTION_GLOBAL, "http-proxy-exceptions", "");
	exceptions += ",";
	exceptions += valuep;
	svn_config_set(opt, SVN_CONFIG_SECTION_GLOBAL, "http-proxy-exceptions", exceptions);
	
	svn_config_set(opt, SVN_CONFIG_SECTION_GLOBAL, "http-proxy-host", proxy);
	
	svn_config_set(opt, SVN_CONFIG_SECTION_GLOBAL, "http-proxy-port", port);



ERROR_LABEL:
	if(Option[0].Value.pszValue != NULL)
		GlobalFree(Option[0].Value.pszValue);

	if(Option[3].Value.pszValue != NULL)
		GlobalFree(Option[3].Value.pszValue);

	if(Option[4].Value.pszValue != NULL)
		GlobalFree(Option[4].Value.pszValue);
	return;
}

/** 
* Set the parent window of an authentication prompt dialog
*/
void SVN::SetPromptParentWindow(HWND hWnd)
{
	m_prompt.SetParentWindow(hWnd);
}
/** 
* Set the MFC Application object for a prompt dialog
*/
void SVN::SetPromptApp(CWinApp* pWinApp)
{
	m_prompt.SetApp(pWinApp);
}

apr_array_header_t * SVN::MakePathArray(const CTSVNPathList& pathList)
{
	apr_array_header_t *targets = apr_array_make (pool,pathList.GetCount(),sizeof(const char *));

	for(int nItem = 0; nItem < pathList.GetCount(); nItem++)
	{
		const char * target = apr_pstrdup (pool, pathList[nItem].GetSVNApiPath());
		(*((const char **) apr_array_push (targets))) = target;
	}
	return targets;
}

void SVN::SetAndClearProgressInfo(HWND hWnd)
{
	m_progressWnd = hWnd;
	m_pProgressDlg = NULL;
	progress_total = 0;
	progress_lastprogress = 0;
	progress_lasttotal = 0;
	progress_lastTicks = GetTickCount();
}

void SVN::SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar/* = false*/)
{
	m_progressWnd = NULL;
	m_pProgressDlg = pProgressDlg;
	progress_total = 0;
	progress_lastprogress = 0;
	progress_lasttotal = 0;
	progress_lastTicks = GetTickCount();
	m_bShowProgressBar = bShowProgressBar;
}

CString SVN::GetSummarizeActionText(svn_client_diff_summarize_kind_t kind)
{
	CString sAction;
	switch (kind)
	{
	case svn_client_diff_summarize_kind_normal:
		sAction.LoadString(IDS_SVN_SUMMARIZENORMAL);
		break;
	case svn_client_diff_summarize_kind_added:
		sAction.LoadString(IDS_SVN_SUMMARIZEADDED);
		break;
	case svn_client_diff_summarize_kind_modified:
		sAction.LoadString(IDS_SVN_SUMMARIZEMODIFIED);
		break;
	case svn_client_diff_summarize_kind_deleted:
		sAction.LoadString(IDS_SVN_SUMMARIZEDELETED);
		break;
	}
	return sAction;
}

void SVN::progress_func(apr_off_t progress, apr_off_t total, void *baton, apr_pool_t * /*pool*/)
{
	SVN * pSVN = (SVN*)baton;
	if ((pSVN==0)||((pSVN->m_progressWnd == 0)&&(pSVN->m_pProgressDlg == 0)))
		return;
	apr_off_t delta = progress;
	if ((progress >= pSVN->progress_lastprogress)&&(total == pSVN->progress_lasttotal))
		delta = progress - pSVN->progress_lastprogress;
	pSVN->progress_lastprogress = progress;
	pSVN->progress_lasttotal = total;
	
	DWORD ticks = GetTickCount();
	pSVN->progress_vector.push_back(delta);
	pSVN->progress_total += delta;
	//ATLTRACE("progress = %I64d, total = %I64d, delta = %I64d, overall total is : %I64d\n", progress, total, delta, pSVN->progress_total);
	if ((pSVN->progress_lastTicks + 1000) < ticks)
	{
		double divby = (double(ticks - pSVN->progress_lastTicks)/1000.0);
		if (divby == 0)
			divby = 1;
		pSVN->m_SVNProgressMSG.overall_total = pSVN->progress_total;
		pSVN->m_SVNProgressMSG.progress = progress;
		pSVN->m_SVNProgressMSG.total = total;
		pSVN->progress_lastTicks = ticks;
		apr_off_t average = 0;
		for (std::vector<apr_off_t>::iterator it = pSVN->progress_vector.begin(); it != pSVN->progress_vector.end(); ++it)
		{
			average += *it;
		}
		average = apr_off_t(double(average) / divby);
		pSVN->m_SVNProgressMSG.BytesPerSecond = average;
		if (average < 1024)
			pSVN->m_SVNProgressMSG.SpeedString.Format(_T("%ld Bytes/s"), average);
		else
		{
			double averagekb = (double)average / 1024.0;
			pSVN->m_SVNProgressMSG.SpeedString.Format(_T("%.2f kBytes/s"), averagekb);
		}
		if (pSVN->m_progressWnd)
			SendMessage(pSVN->m_progressWnd, WM_SVNPROGRESS, 0, (LPARAM)&pSVN->m_SVNProgressMSG);
		else if (pSVN->m_pProgressDlg)
		{
			if ((pSVN->m_bShowProgressBar && (progress > 1000) && (total > 0)))
				pSVN->m_pProgressDlg->SetProgress64(progress, total);
			else
				pSVN->m_pProgressDlg->SetProgress64(0, 0);

			CString sTotal;
			CString temp;
			if (pSVN->m_SVNProgressMSG.overall_total < 1024)
				sTotal.Format(IDS_SVN_PROGRESS_TOTALBYTESTRANSFERRED, pSVN->m_SVNProgressMSG.overall_total);
			else if (pSVN->m_SVNProgressMSG.overall_total < 1200000)
				sTotal.Format(IDS_SVN_PROGRESS_TOTALTRANSFERRED, pSVN->m_SVNProgressMSG.overall_total / 1024);
			else
				sTotal.Format(IDS_SVN_PROGRESS_TOTALMBTRANSFERRED, (double)((double)pSVN->m_SVNProgressMSG.overall_total / 1024000.0));
			temp.Format(IDS_SVN_PROGRESS_TOTALANDSPEED, sTotal, pSVN->m_SVNProgressMSG.SpeedString);

			pSVN->m_pProgressDlg->SetLine(2, temp);
		}
		pSVN->progress_vector.clear();
	}
	return;
}

