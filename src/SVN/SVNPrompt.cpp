// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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

#include "TortoiseProc.h"
#include "svn.h"
#include "UnicodeUtils.h"
#include <shlwapi.h>
#include "MessageBox.h"

BOOL SVNPrompt::Prompt(CString& info, BOOL hide, CString promptphrase, BOOL& may_save) 
{
	CPromptDlg dlg;
	dlg.SetHide(hide);
	dlg.m_info = promptphrase;
	dlg.m_hParentWnd = this->hWnd;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		info = dlg.m_sPass;
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
	} // if (nResponse == IDABORT)
	return FALSE;
}

BOOL SVNPrompt::SimplePrompt(CString& username, CString& password, BOOL& may_save) 
{
	CSimplePrompt dlg;
	dlg.m_hParentWnd = this->hWnd;
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
	} // if (nResponse == IDABORT)
	return FALSE;
}

svn_error_t* SVNPrompt::userprompt(svn_auth_cred_username_t **cred, void *baton, const char *realm, svn_boolean_t may_save, apr_pool_t *pool)
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
	} // if (svn->UserPrompt(infostring, CString(realm))
	else
		*cred = NULL;
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

svn_error_t* SVNPrompt::simpleprompt(svn_auth_cred_simple_t **cred, void *baton, const char *realm, const char *username, svn_boolean_t may_save, apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;
	svn_auth_cred_simple_t *ret = (svn_auth_cred_simple_t *)apr_pcalloc (pool, sizeof (*ret));
	CString UserName = CUnicodeUtils::GetUnicode(username);
	CString PassWord;
	if (svn->SimplePrompt(UserName, PassWord, may_save))
	{
		ret->username = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(UserName));
		ret->password = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(PassWord));
		ret->may_save = may_save;
		*cred = ret;
	} // if (svn->userprompt(username, password))
	else
		*cred = NULL;
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

svn_error_t* SVNPrompt::sslserverprompt(svn_auth_cred_ssl_server_trust_t **cred_p, void *baton, const char *realm, apr_uint32_t failures, const svn_auth_ssl_server_cert_info_t *cert_info, svn_boolean_t may_save, apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;

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
	} // if (failures_in & SVN_AUTH_SSL_UNKNOWNCA)
	if (failures & SVN_AUTH_SSL_CNMISMATCH)
	{
		if (prev)
			msg += _T("\n");
		temp.Format(IDS_ERR_SSL_CNMISMATCH, CUnicodeUtils::GetUnicode(cert_info->hostname));
		msg += temp;
		prev = TRUE;
	} // if (failures_in & SVN_AUTH_SSL_CNMISMATCH)
	if (failures & SVN_AUTH_SSL_NOTYETVALID)
	{
		if (prev)
			msg += _T("\n");
		temp.Format(IDS_ERR_SSL_NOTYETVALID, CUnicodeUtils::GetUnicode(cert_info->valid_from));
		msg += temp;
		prev = TRUE;
	} // if (failures_in & (SVN_AUTH_SSL_EXPIRED | SVN_AUTH_SSL_NOTYETVALID))
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
		ret = CMessageBox::Show(svn->hWnd, msg, _T("TortoiseSVN"), MB_DEFBUTTON3, IDI_QUESTION, sAcceptAlways, sAcceptTemp, sReject);
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
		if (CMessageBox::Show(svn->hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION)==IDYES)
		{
			*cred_p = (svn_auth_cred_ssl_server_trust_t*)apr_pcalloc (pool, sizeof (**cred_p));
			(*cred_p)->may_save = FALSE;
		} // if (CMessageBox::Show(NULL, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION)==IDOK)
		else
			*cred_p = NULL;
	}
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

svn_error_t* SVNPrompt::sslclientprompt(svn_auth_cred_ssl_client_cert_t **cred, void *baton, const char * realm, svn_boolean_t may_save, apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;
	const char *cert_file = NULL;

	CString filename;
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	//ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
	ofn.hwndOwner = svn->hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	CString sFilter;
	sFilter.LoadString(IDS_CERTIFICATESFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy (pszFilters, sFilter);
	// Replace '|' delimeters with '\0's
	TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
	while (ptr != pszFilters)
	{
		if (*ptr == '|')
			*ptr = '\0';
		ptr--;
	} // while (ptr != pszFilters) 
	ofn.lpstrFilter = pszFilters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	temp.LoadString(IDS_SSL_CLIENTCERTIFICATEFILENAME);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		filename = CString(ofn.lpstrFile);
		cert_file = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(filename));
		/* Build and return the credentials. */
		*cred = (svn_auth_cred_ssl_client_cert_t*)apr_pcalloc (pool, sizeof (**cred));
		(*cred)->cert_file = cert_file;
		(*cred)->may_save = may_save;
	} // if (GetOpenFileName(&ofn)==TRUE) 
	else
		*cred = NULL;
	delete [] pszFilters;
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

svn_error_t* SVNPrompt::sslpwprompt(svn_auth_cred_ssl_client_cert_pw_t **cred, void *baton, const char * realm, svn_boolean_t may_save, apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;
	svn_auth_cred_ssl_client_cert_pw_t *ret = (svn_auth_cred_ssl_client_cert_pw_t *)apr_pcalloc (pool, sizeof (*ret));
	CString password;
	CString temp;
	temp.LoadString(IDS_AUTH_PASSWORD);
	if (svn->Prompt(password, TRUE, temp, may_save))
	{
		ret->password = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(password));
		ret->may_save = may_save;
		*cred = ret;
	} // if (svn->UserPrompt(infostring, CString(realm))
	else
		*cred = NULL;
	if (svn->m_app)
		svn->m_app->DoWaitCursor(0);
	return SVN_NO_ERROR;
}

