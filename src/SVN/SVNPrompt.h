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
#pragma once
#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_sorts.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_repos.h"
#include "svn_string.h"
#include "svn_config.h"
#include "svn_time.h"
#include "svn_subst.h"
#include "svn_auth.h"


#include "PromptDlg.h"
#include "SimplePrompt.h"

class SVNPrompt
{
public:
	svn_client_ctx_t 	ctx;

	virtual void SaveAuthentication(BOOL save) = 0;
	virtual BOOL Prompt(CString& info, BOOL hide, CString promptphrase);
	virtual BOOL SimplePrompt(CString& username, CString& password);

	static svn_error_t* userprompt(svn_auth_cred_username_t **cred, void *baton, const char *realm, apr_pool_t *pool);
	static svn_error_t* simpleprompt(svn_auth_cred_simple_t **cred, void *baton, const char *realm, const char *username, apr_pool_t *pool);
	static svn_error_t* sslserverprompt(svn_auth_cred_server_ssl_t **cred, void *baton, int failures_in, apr_pool_t *pool);
	static svn_error_t* sslclientprompt(svn_auth_cred_client_ssl_t **cred, void *baton, apr_pool_t *pool);
	static svn_error_t* sslpwprompt(svn_auth_cred_client_ssl_pass_t **cred, void *baton, apr_pool_t *pool);

	CWinApp *					m_app;
	HWND						hWnd;

};