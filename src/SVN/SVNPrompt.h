// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Tim Kemp and Stefan Kueng

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

class SVNPrompt
{
public:
	SVNPrompt();
	virtual ~SVNPrompt();

public:
	void SetParentWindow(HWND hWnd)				{ m_hParentWnd = hWnd; }
	void SetApp(CWinApp* pApp)					{ m_app = pApp; }
	void Init(apr_pool_t *pool, svn_client_ctx_t* ctx);

private:
	BOOL Prompt(CString& info, BOOL hide, CString promptphrase, BOOL& may_save);
	BOOL SimplePrompt(CString& username, CString& password, const CString& Realm, BOOL& may_save);

	static svn_error_t* userprompt(svn_auth_cred_username_t **cred, void *baton, const char *realm, svn_boolean_t may_save, apr_pool_t *pool);
	static svn_error_t* simpleprompt(svn_auth_cred_simple_t **cred, void *baton, const char *realm, const char *username, svn_boolean_t may_save, apr_pool_t *pool);
	static svn_error_t* sslserverprompt(svn_auth_cred_ssl_server_trust_t **cred_p, void *baton, const char *realm, apr_uint32_t failures, const svn_auth_ssl_server_cert_info_t *cert_info, svn_boolean_t may_save, apr_pool_t *pool);
	static svn_error_t* sslclientprompt(svn_auth_cred_ssl_client_cert_t **cred, void *baton, const char * realm, svn_boolean_t may_save, apr_pool_t *pool);
	static svn_error_t* sslpwprompt(svn_auth_cred_ssl_client_cert_pw_t **cred, void *baton, const char * realm, svn_boolean_t may_save, apr_pool_t *pool);

	static UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
	
private:
	svn_auth_baton_t *			auth_baton;
	CString						m_server;
	CWinApp *					m_app;
	HWND						m_hParentWnd;

};
