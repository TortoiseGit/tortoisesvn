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

#pragma once
#ifdef _MFC_VER
#include "PromptDlg.h"
#endif
#include <windows.h>
#include "resource.h"
#include <tchar.h>
#include <Shlwapi.h>

#include "apr_general.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_sorts.h"
#include "svn_path.h"
#include "svn_wc.h"
#include "svn_utf.h"
#include "svn_config.h"



/**
 * \ingroup TortoiseShell
 * Provides Subversion commands, some of them as static methods for easier and
 * faster use.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 *
 * \version 1.0
 * first version
 *
 * \date 10-10-2002
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class SVNStatus
{
public:
	SVNStatus(void);
	~SVNStatus(void);

#ifdef _MFC_VER
	virtual BOOL Prompt(CString& info, CString prompt, BOOL hide);
#endif

	/**
	 * Reads the Subversion text status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * Only the status of the text entry is returned, properties
	 * are ignored.
	 * 
	 * \param path the pathname of the entry
	 * \return the status
	 */
	static svn_wc_status_kind GetTextStatus(const TCHAR * path);

	/**
	 * Reads the Subversion status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * If the status of the text and property part are different
	 * then the higher value is returned.
	 */
	static svn_wc_status_kind GetAllStatus(const TCHAR * path);

	/**
	 * Reads the Subversion text status of the working copy entry and all its
	 * subitems. The resulting status is determined by using priorities for each
	 * status. The status with the highest priority is then returned.
	 * Only the status of the text entry is returned, properties
	 * are ignored.
	 * \remark Use this method only after checking that the entry is a directory. Using this
	 * method for files is ineffective and slow.
	 * 
	 * \param path the pathname of the entry
	 * \return the status
	 */
	static svn_wc_status_kind GetTextStatusRecursive(const TCHAR * path);

	/**
	 * Reads the Subversion status of the working copy entry and all its
	 * subitems. The resulting status is determined by using priorities for
	 * each status. The status with the highest priority is then returned.
	 * If the status of the text and property part are different then
	 * the one with the higher priority is returned.
	 */
	static svn_wc_status_kind GetAllStatusRecursive(const TCHAR * path);

	/**
	 * Returns the status which is more "important" of the two statuses specified.
	 * This is used for the "recursive" status functions on folders - i.e. which status
	 * should be returned for a folder which has several files with different statuses
	 * in it.
	 */	 	 	 	 	
	static svn_wc_status_kind GetMoreImportant(svn_wc_status_kind status1, svn_wc_status_kind status2);
	
	static BOOL IsImportant(svn_wc_status_kind status) {return (GetMoreImportant(svn_wc_status_added, status)==status);}
	/**
	 * Reads the Subversion text status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * The result is stored in the public member variable status.
	 * 
	 * \param path the pathname of the entry
	 * \param update true if the status should be updated with the repository. Default is false.
	 * \return If update is set to true the HEAD revision of the repository is returned. If update is false then -1 is returned.
	 * \remark If the return value is -2 then the status could not be obtained.
	 */
	svn_revnum_t GetStatus(const TCHAR * path, bool update = false);


	/**
	 * Returns a string representation of a Subversion status.
	 * \param status the status enum
	 * \param string a string representation
	 */
	static void GetStatusString(svn_wc_status_kind status, TCHAR * string);
	static void GetStatusString(HINSTANCE hInst, svn_wc_status_kind status, TCHAR * string, int size, WORD lang);

	/**
	 * Returns the status of the first file of the given path. Use GetNextFileStatus() to obtain
	 * the status of the next file in the list.
	 * \param path the path of the folder from where the status list should be obtained
	 * \param retPath the path of the file for which the status was returned
	 * \param update set this to true if you want the status to be updated with the repository (needs network access)
	 * \return the status
	 */
	svn_wc_status_t * GetFirstFileStatus(const TCHAR * path, const TCHAR ** retPath, bool update = false);
	/**
	 * Returns the status of the next file in the filelist. If no more files are in the list then NULL is returned.
	 * See GetFirstFileStatus() for details.
	 */
	svn_wc_status_t * GetNextFileStatus(const TCHAR ** retPath);

	/**
	 * This member variable hold the status of the last call to GetStatus().
	 */
	svn_wc_status_t *			status;				///< the status result of GetStatus()

#ifdef _MFC_VER
	CString GetLastErrorMsg();
#endif

private:
	apr_pool_t *				m_parentpool;
	apr_pool_t *				m_pool;
	svn_auth_baton_t *			m_auth_baton;
	svn_client_ctx_t 			m_ctx;
	svn_error_t *				m_err;
#ifdef _MFC_VER
	static svn_error_t* prompt(char **info, 
					const char *prompt, 
					svn_boolean_t hide, 
					void *baton, 
					apr_pool_t *pool);
#endif
	static int GetStatusRanking(svn_wc_status_kind status);
	//for GetFirstFileStatus and GetNextFileStatus
	apr_hash_t *				m_statushash;
	apr_array_header_t *		m_statusarray;
	int							m_statushashindex;

#pragma warning(push)
#pragma warning(disable: 4200)
	struct STRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)	// C4200

	static int LoadStringEx(HINSTANCE hInstance, UINT uID, LPCTSTR lpBuffer, int nBufferMax, WORD wLanguage);

};

