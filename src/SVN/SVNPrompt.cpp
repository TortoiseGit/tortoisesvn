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
#include "SVNPrompt.h"

#include "TortoiseProc.h"
#include "svn.h"
#include "UnicodeUtils.h"
#include <shlwapi.h>
#include "DirFileList.h"
#include "MessageBox.h"

BOOL SVNPrompt::Prompt(CString& info, BOOL hide, CString promptphrase) 
{
	CPromptDlg dlg;
	dlg.SetHide(hide);
	dlg.m_info = promptphrase;
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

BOOL SVNPrompt::SimplePrompt(CString& username, CString& password) 
{
	CSimplePrompt dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		username = dlg.m_sUsername;
		password = dlg.m_sPassword;
		SaveAuthentication(dlg.m_bSaveAuthentication);
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

svn_error_t* SVNPrompt::userprompt(svn_auth_cred_username_t **cred, void *baton, const char *realm, apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;
	svn_auth_cred_username_t *ret = (svn_auth_cred_username_t *)apr_pcalloc (pool, sizeof (*ret));
	CString username;
	CString temp;
	temp.LoadString(IDS_AUTH_USERNAME);
	if (svn->Prompt(username, FALSE, temp))
	{
		ret->username = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(username));
		*cred = ret;
		return SVN_NO_ERROR;
	} // if (svn->UserPrompt(infostring, CString(realm))
	CStringA temp1;
	temp1.LoadString(IDS_SVN_USERCANCELLED);
	return svn_error_create(SVN_ERR_CANCELLED, NULL, temp1);
}

svn_error_t* SVNPrompt::simpleprompt(svn_auth_cred_simple_t **cred, void *baton, const char *realm, const char *username, apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;
	svn_auth_cred_simple_t *ret = (svn_auth_cred_simple_t *)apr_pcalloc (pool, sizeof (*ret));
	CString UserName = CUnicodeUtils::GetUnicode(username);
	CString PassWord;
	if (svn->SimplePrompt(UserName, PassWord))
	{
		ret->username = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(UserName));
		ret->password = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(PassWord));
		*cred = ret;
		return SVN_NO_ERROR;
	} // if (svn->userprompt(username, password))
	CStringA temp;
	temp.LoadString(IDS_SVN_USERCANCELLED);
	return svn_error_create(SVN_ERR_CANCELLED, NULL, temp);
}

svn_error_t* SVNPrompt::sslserverprompt(svn_auth_cred_server_ssl_t **cred, void *baton, int failures_in, apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;

	BOOL prev = FALSE;

	CString msg;
	msg.LoadString(IDS_ERR_SSL_VALIDATE);
	msg += _T("\n");
	CString temp;
	if (failures_in & SVN_AUTH_SSL_UNKNOWNCA)
	{
		temp.LoadString(IDS_ERR_SSL_UNKNOWNCA);
		msg += temp;
		prev = TRUE;
	} // if (failures_in & SVN_AUTH_SSL_UNKNOWNCA)
	if (failures_in & SVN_AUTH_SSL_CNMISMATCH)
	{
		if (prev)
			msg += _T("\n");
		temp.LoadString(IDS_ERR_SSL_CNMISMATCH);
		msg += temp;
		prev = TRUE;
	} // if (failures_in & SVN_AUTH_SSL_CNMISMATCH)
	if (failures_in & (SVN_AUTH_SSL_EXPIRED | SVN_AUTH_SSL_NOTYETVALID))
	{
		if (prev)
			msg += _T("\n");
		temp.LoadString(IDS_ERR_SSL_EXPIREDORNOTYETVALID);
		msg += temp;
		prev = TRUE;
	} // if (failures_in & (SVN_AUTH_SSL_EXPIRED | SVN_AUTH_SSL_NOTYETVALID))
	if (prev)
		msg += _T("\n");
	temp.LoadString(IDS_SSL_ACCEPTQUESTION);
	msg += temp;
	if (CMessageBox::Show(svn->hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION)==IDYES)
	{
		*cred = (svn_auth_cred_server_ssl_t*)apr_pcalloc (pool, sizeof (**cred));
		(*cred)->failures_allow = failures_in;
		return SVN_NO_ERROR;
	} // if (CMessageBox::Show(NULL, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION)==IDOK)
	*cred = NULL;
	CStringA temp1;
	temp1.LoadString(IDS_SVN_USERCANCELLED);
	return svn_error_create(SVN_ERR_CANCELLED, NULL, temp1);
}

svn_error_t* SVNPrompt::sslclientprompt(svn_auth_cred_client_ssl_t **cred, void *baton, apr_pool_t *pool)
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
	ofn.lpstrFilter = _T("Certificates\0*.p12;*.pkcs12\0All\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	temp.LoadString(IDS_SSL_CLIENTCERTIFICATEFILENAME);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		filename = CString(ofn.lpstrFile);
		cert_file = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(filename));
		/* Build and return the credentials. */
		*cred = (svn_auth_cred_client_ssl_t*)apr_pcalloc (pool, sizeof (**cred));
		(*cred)->cert_file = cert_file;
		return SVN_NO_ERROR;
	} // if (GetOpenFileName(&ofn)==TRUE) 

	CStringA temp1;
	temp1.LoadString(IDS_SVN_USERCANCELLED);
	return svn_error_create(SVN_ERR_CANCELLED, NULL, temp1);
}

svn_error_t* SVNPrompt::sslpwprompt(svn_auth_cred_client_ssl_pass_t **cred, void *baton, apr_pool_t *pool)
{
	SVN * svn = (SVN *)baton;
	svn_auth_cred_client_ssl_pass_t *ret = (svn_auth_cred_client_ssl_pass_t *)apr_pcalloc (pool, sizeof (*ret));
	CString password;
	CString temp;
	temp.LoadString(IDS_AUTH_PASSWORD);
	if (svn->Prompt(password, TRUE, temp))
	{
		ret->password = apr_pstrdup(pool, CUnicodeUtils::GetUTF8(password));
		*cred = ret;
		return SVN_NO_ERROR;
	} // if (svn->UserPrompt(infostring, CString(realm))
	*cred = NULL;
	CStringA temp1;
	temp1.LoadString(IDS_SVN_USERCANCELLED);
	return svn_error_create(SVN_ERR_CANCELLED, NULL, temp1);
}

