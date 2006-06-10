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
#include "SVNPrompt.h"
#include "PromptDlg.h"
#include "SimplePrompt.h"
#include "Dlgs.h"
#include "Registry.h"
#include "TortoiseProc.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "StringUtils.h"

SVNPrompt::SVNPrompt()
{
	m_app = NULL;
	m_hParentWnd = NULL;
}

SVNPrompt::~SVNPrompt()
{
}

void SVNPrompt::Init(apr_pool_t *pool, svn_client_ctx_t* ctx)
{
	// set up authentication

	svn_auth_provider_object_t *provider;

	/* The whole list of registered providers */
	apr_array_header_t *providers = apr_array_make (pool, 11, sizeof (svn_auth_provider_object_t *));

	/* The main disk-caching auth providers, for both
	'username/password' creds and 'username' creds.  */
	svn_auth_get_windows_simple_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_auth_get_simple_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_auth_get_username_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* The server-cert, client-cert, and client-cert-password providers. */
	svn_auth_get_ssl_server_trust_file_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_auth_get_ssl_client_cert_file_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_auth_get_ssl_client_cert_pw_file_provider (&provider, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Two prompting providers, one for username/password, one for
	just username. */
	svn_auth_get_simple_prompt_provider (&provider, (svn_auth_simple_prompt_func_t)simpleprompt, this, 3, /* retry limit */ pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_auth_get_username_prompt_provider (&provider, (svn_auth_username_prompt_func_t)userprompt, this, 3, /* retry limit */ pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Three prompting providers for server-certs, client-certs,
	and client-cert-passphrases.  */
	svn_auth_get_ssl_server_trust_prompt_provider (&provider, sslserverprompt, this, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_auth_get_ssl_client_cert_prompt_provider (&provider, sslclientprompt, this, 2, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
	svn_auth_get_ssl_client_cert_pw_prompt_provider (&provider, sslpwprompt, this, 2, pool);
	APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

	/* Build an authentication baton to give to libsvn_client. */
	svn_auth_open (&auth_baton, providers, pool);
	ctx->auth_baton = auth_baton;
}

BOOL SVNPrompt::Prompt(CString& info, BOOL hide, CString promptphrase, BOOL& may_save) 
{
	CPromptDlg dlg;
	dlg.SetHide(hide);
	dlg.m_info = promptphrase;
	dlg.m_hParentWnd = m_hParentWnd;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		info = dlg.m_sPass;
		may_save = dlg.m_saveCheck;
		if (m_app)
			m_app->DoWaitCursor(0);
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
	}
	return FALSE;
}

BOOL SVNPrompt::SimplePrompt(CString& username, CString& password, const CString& Realm, BOOL& may_save) 
{
	CSimplePrompt dlg;
	dlg.m_hParentWnd = m_hParentWnd;
	dlg.m_sRealm = Realm;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		username = dlg.m_sUsername;
		password = dlg.m_sPassword;
		may_save = dlg.m_bSaveAuthentication;
		if (m_app)
			m_app->DoWaitCursor(0);
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
	}
	return FALSE;
}

svn_error_t* SVNPrompt::userprompt(svn_auth_cred_username_t **cred, void *baton, const char * /*realm*/, svn_boolean_t may_save, apr_pool_t *pool)
{
	SVNPrompt * svn = (SVNPrompt *)baton;
	svn_auth_cred_username_t *ret = (svn_auth_cred_username_t *)apr_pcalloc (pool, sizeof (*ret));
	CString username;
	CString temp;
	temp.LoadString(IDS_AUTH_USERNAME);
	if (svn->Prompt(username, FALSE, temp, may_save))
	{
		ret->username = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(username));
		ret->may_save = may_save;
		*cred = ret;
	}
	else
		*cred = NULL;
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

svn_error_t* SVNPrompt::simpleprompt(svn_auth_cred_simple_t **cred, void *baton, const char * realm, const char *username, svn_boolean_t may_save, apr_pool_t *pool)
{
	SVNPrompt * svn = (SVNPrompt *)baton;
	svn_auth_cred_simple_t *ret = (svn_auth_cred_simple_t *)apr_pcalloc (pool, sizeof (*ret));
	CString UserName = CUnicodeUtils::GetUnicode(username);
	CString PassWord;
	CString Realm = CUnicodeUtils::GetUnicode(realm);
	if (svn->SimplePrompt(UserName, PassWord, Realm, may_save))
	{
		ret->username = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(UserName));
		ret->password = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(PassWord));
		ret->may_save = may_save;
		*cred = ret;
	}
	else
		*cred = NULL;
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

svn_error_t* SVNPrompt::sslserverprompt(svn_auth_cred_ssl_server_trust_t **cred_p, void *baton, const char *realm, apr_uint32_t failures, const svn_auth_ssl_server_cert_info_t *cert_info, svn_boolean_t may_save, apr_pool_t *pool)
{
	SVNPrompt * svn = (SVNPrompt *)baton;

	BOOL prev = FALSE;

	CString msg;
	msg.Format(IDS_ERR_SSL_VALIDATE, CUnicodeUtils::GetUnicode(realm));
	msg += _T("\n");
	CString temp;
	if (failures & SVN_AUTH_SSL_UNKNOWNCA)
	{
		temp.Format(IDS_ERR_SSL_UNKNOWNCA, CUnicodeUtils::GetUnicode(cert_info->fingerprint), CUnicodeUtils::GetUnicode(cert_info->issuer_dname));
		msg += temp;
		prev = TRUE;
	}
	if (failures & SVN_AUTH_SSL_CNMISMATCH)
	{
		if (prev)
			msg += _T("\n");
		temp.Format(IDS_ERR_SSL_CNMISMATCH, CUnicodeUtils::GetUnicode(cert_info->hostname));
		msg += temp;
		prev = TRUE;
	}
	if (failures & SVN_AUTH_SSL_NOTYETVALID)
	{
		if (prev)
			msg += _T("\n");
		temp.Format(IDS_ERR_SSL_NOTYETVALID, CUnicodeUtils::GetUnicode(cert_info->valid_from));
		msg += temp;
		prev = TRUE;
	}
	if (failures & SVN_AUTH_SSL_EXPIRED)
	{
		if (prev)
			msg += _T("\n");
		temp.Format(IDS_ERR_SSL_EXPIRED, CUnicodeUtils::GetUnicode(cert_info->valid_until));
		msg += temp;
		prev = TRUE;
	}
	if (prev)
		msg += _T("\n");
	temp.LoadString(IDS_SSL_ACCEPTQUESTION);
	msg += temp;
	if (may_save)
	{
		CString sAcceptAlways, sAcceptTemp, sReject;
		sAcceptAlways.LoadString(IDS_SSL_ACCEPTALWAYS);
		sAcceptTemp.LoadString(IDS_SSL_ACCEPTTEMP);
		sReject.LoadString(IDS_SSL_REJECT);
		int ret = 0;
		ret = CMessageBox::Show(svn->m_hParentWnd, msg, _T("TortoiseSVN"), MB_DEFBUTTON3, IDI_QUESTION, sAcceptAlways, sAcceptTemp, sReject);
		if (ret == 1)
		{
			*cred_p = (svn_auth_cred_ssl_server_trust_t*)apr_pcalloc (pool, sizeof (**cred_p));
			(*cred_p)->may_save = TRUE;
			(*cred_p)->accepted_failures = failures;
		} 
		else if (ret == 2)
		{
			*cred_p = (svn_auth_cred_ssl_server_trust_t*)apr_pcalloc (pool, sizeof (**cred_p));
			(*cred_p)->may_save = FALSE;
		}
		else
			*cred_p = NULL;
	}
	else
	{
		if (CMessageBox::Show(svn->m_hParentWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION)==IDYES)
		{
			*cred_p = (svn_auth_cred_ssl_server_trust_t*)apr_pcalloc (pool, sizeof (**cred_p));
			(*cred_p)->may_save = FALSE;
		}
		else
			*cred_p = NULL;
	}
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

svn_error_t* SVNPrompt::sslclientprompt(svn_auth_cred_ssl_client_cert_t **cred, void *baton, const char * realm, svn_boolean_t /*may_save*/, apr_pool_t *pool)
{
	SVNPrompt * svn = (SVNPrompt *)baton;
	const char *cert_file = NULL;

	CString filename;
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = svn->m_hParentWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	CString sFilter;
	sFilter.LoadString(IDS_CERTIFICATESFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
	// Replace '|' delimeters with '\0's
	TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
	while (ptr != pszFilters)
	{
		if (*ptr == '|')
			*ptr = '\0';
		ptr--;
	}
	ofn.lpstrFilter = pszFilters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	temp.LoadString(IDS_SSL_CLIENTCERTIFICATEFILENAME);
	CStringUtils::RemoveAccelerators(temp);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_EXPLORER;
	ofn.lpfnHook = SVNPrompt::OFNHookProc;

	// Display the Open dialog box. 
	svn->m_server.Empty();
	if (GetOpenFileName(&ofn)==TRUE)
	{
		filename = CString(ofn.lpstrFile);
		cert_file = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(filename));
		/* Build and return the credentials. */
		*cred = (svn_auth_cred_ssl_client_cert_t*)apr_pcalloc (pool, sizeof (**cred));
		(*cred)->cert_file = cert_file;
		(*cred)->may_save = ((ofn.Flags & OFN_READONLY)!=0);

		if ((*cred)->may_save)
		{
			CString regpath = _T("Software\\tigris.org\\Subversion\\Servers\\");
			CString groups = regpath;
			groups += _T("groups\\");
			CString server = CString(realm);
			int f1 = server.Find('<')+9;
			int len = server.Find(':', 10)-f1;
			server = server.Mid(f1, len);
			svn->m_server = server;
			groups += server;
			CRegString server_groups = CRegString(groups);
			server_groups = server;
			regpath += server;
			regpath += _T("\\ssl-client-cert-file");
			CRegString client_cert_filepath_reg = CRegString(regpath);
			client_cert_filepath_reg = filename;
		}
	}
	else
		*cred = NULL;
	delete [] pszFilters;
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

UINT_PTR CALLBACK SVNPrompt::OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	switch (uiMsg)
	{

	case WM_NOTIFY:
        // look for the CDN_INITDONE notification telling you Windows
        // has has finished initializing the dialog box
		LPOFNOTIFY lpofn = (LPOFNOTIFY)lParam;
		if (lpofn->hdr.code == CDN_INITDONE)
		{
			temp.LoadString(IDS_SSL_SAVE_CERTPATH);
			SendMessage(::GetParent(hdlg), CDM_SETCONTROLTEXT, chx1, (LPARAM)(LPCTSTR)temp);
			return TRUE;
		}
	} 
	return FALSE;
}

svn_error_t* SVNPrompt::sslpwprompt(svn_auth_cred_ssl_client_cert_pw_t **cred, void *baton, const char * realm, svn_boolean_t may_save, apr_pool_t *pool)
{
	SVNPrompt* svn = (SVNPrompt *)baton;
	svn_auth_cred_ssl_client_cert_pw_t *ret = (svn_auth_cred_ssl_client_cert_pw_t *)apr_pcalloc (pool, sizeof (*ret));
	CString password;
	CString temp;
	temp.LoadString(IDS_AUTH_PASSWORD);
	if (svn->Prompt(password, TRUE, temp, may_save))
	{
		ret->password = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(password));
		ret->may_save = may_save;
		*cred = ret;
		if (!svn->m_server.IsEmpty())
		{
			if ((*cred)->may_save)
			{
				CString regpath = _T("Software\\tigris.org\\Subversion\\Servers\\");
				CString groups = regpath + _T("groups\\");
				CString server = CString(realm);
				int f1 = server.Find('<')+9;
				int len = server.Find(':', 10)-f1;
				server = server.Mid(f1, len);
				svn->m_server = server;
				groups += svn->m_server;
				CRegString server_groups = CRegString(groups);
				server_groups = svn->m_server;
				regpath += svn->m_server;
				regpath += _T("\\ssl-client-cert-password");
				CRegString client_cert_password_reg = CRegString(regpath);
				client_cert_password_reg = CString(ret->password);
			}
		}
	}
	else
		*cred = NULL;
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

