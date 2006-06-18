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

#pragma once

#ifdef _MFC_VER
#	include "SVNPrompt.h"
#endif
#include "TSVNPath.h"

#pragma warning (push,1)
typedef std::basic_string<wchar_t> wide_string;
#ifdef UNICODE
#	define stdstring wide_string
#else
#	define stdstring std::string
#endif
#pragma warning (pop)

#define MAX_STATUS_STRING_LENGTH		100


/**
 * \ingroup SVN
 * Provides Subversion commands, some of them as static methods for easier and
 * faster use.
 */
class SVNStatus
{
public:
	SVNStatus(bool * pbCanceled = NULL);
	~SVNStatus(void);


	/**
	 * Reads the Subversion status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * If the status of the text and property part are different
	 * then the more important status is returned.
	 */
	static svn_wc_status_kind GetAllStatus(const CTSVNPath& path, BOOL recursive = FALSE);

	/**
	 * Reads the Subversion status of the working copy entry and all its
	 * subitems. The resulting status is determined by using priorities for
	 * each status. The status with the highest priority is then returned.
	 * If the status of the text and property part are different then
	 * the more important status is returned.
	 */
	static svn_wc_status_kind GetAllStatusRecursive(const CTSVNPath& path);

	/**
	 * Returns the status which is more "important" of the two statuses specified.
	 * This is used for the "recursive" status functions on folders - i.e. which status
	 * should be returned for a folder which has several files with different statuses
	 * in it.
	 */	 	 	 	 	
	static svn_wc_status_kind GetMoreImportant(svn_wc_status_kind status1, svn_wc_status_kind status2);
	
	/**
	 * Checks if a status is "important", i.e. if the status indicates that the user should know about it.
	 * E.g. a "normal" status is not important, but "modified" is.
	 * \param status the status to check
	 */
	static BOOL IsImportant(svn_wc_status_kind status) {return (GetMoreImportant(svn_wc_status_added, status)==status);}

	/**
	 * Reads the Subversion text status of the working copy entry. No
	 * recurse is done, even if the entry is a directory.
	 * The result is stored in the public member variable status.
	 * Use this method if you need detailed information about a file/folder, not just the raw status (like "normal", "modified").
	 * 
	 * \param path the pathname of the entry
	 * \param update true if the status should be updated with the repository. Default is false.
	 * \return If update is set to true the HEAD revision of the repository is returned. If update is false then -1 is returned.
	 * \remark If the return value is -2 then the status could not be obtained.
	 */
	svn_revnum_t GetStatus(const CTSVNPath& path, bool update = false, bool noignore = false, bool noexternals = false);

	/**
	 * Returns a string representation of a Subversion status.
	 * \param status the status enum
	 * \param string a string representation
	 */
	static void GetStatusString(svn_wc_status_kind status, size_t buflen, TCHAR * string);
	static void GetStatusString(HINSTANCE hInst, svn_wc_status_kind status, TCHAR * string, int size, WORD lang);

	/**
	 * Returns the status of the first file of the given path. Use GetNextFileStatus() to obtain
	 * the status of the next file in the list.
	 * \param path the path of the folder from where the status list should be obtained
	 * \param retPath the path of the file for which the status was returned
	 * \param update set this to true if you want the status to be updated with the repository (needs network access)
	 * \return the status
	 */
	svn_wc_status2_t * GetFirstFileStatus(const CTSVNPath& path, CTSVNPath& retPath, bool update = false, bool recurse = true, bool bNoIgnore = true);
	unsigned int GetFileCount() {return apr_hash_count(m_statushash);}
	unsigned int GetVersionedCount();
	/**
	 * Returns the status of the next file in the filelist. If no more files are in the list then NULL is returned.
	 * See GetFirstFileStatus() for details.
	 */
	svn_wc_status2_t * GetNextFileStatus(CTSVNPath& retPath);
	/**
	 * Checks if a path is an external folder.
	 * This is necessary since Subversion returns two entries for external folders: one with the status svn_wc_status_external
	 * and one with the 'real' status of that folder. GetFirstFileStatus() and GetNextFileStatus() only return the 'real'
	 * status, so with this method it's possible to check if the status also is svn_wc_status_external.
	 */
	bool IsExternal(const CTSVNPath& path);

	/**
	 * Clears the memory pool.
	 */
	void ClearPool();

	/**
	 * This member variable hold the status of the last call to GetStatus().
	 */
	svn_wc_status2_t *			status;				///< the status result of GetStatus()

	svn_revnum_t				headrev;			///< the head revision fetched with GetFirstStatus()

	bool *						m_pbCanceled;
#ifdef _MFC_VER
friend class SVN;	// So that SVN can get to our m_err
	/**
	 * Returns the last error message as a CString object.
	 */
	CString GetLastErrorMsg();

	/** 
	 * Set a list of paths which will be considered when calling GetFirstFileStatus.
	 * If a filter is set, then GetFirstFileStatus/GetNextFileStatus will only return items which are in the filter list
	 */
	void SetFilter(const CTSVNPathList& fileList);
	void ClearFilter();

#else
	/**
	 * Returns the last error message as a CString object.
	 */
	stdstring GetLastErrorMsg();
#endif


protected:
	apr_pool_t *				m_pool;			///< the memory pool
private:
	typedef struct sort_item
	{
		const void *key;
		apr_ssize_t klen;
		void *value;
	} sort_item;

	typedef struct hashbaton_t
	{
		SVNStatus*		pThis;
		apr_hash_t *	hash;
		apr_hash_t *	exthash;
	} hash_baton_t;

	svn_client_ctx_t * 			ctx;
	svn_wc_status_kind			m_allstatus;	///< used by GetAllStatus and GetAllStatusRecursive
	svn_error_t *				m_err;			///< Subversion error baton

#ifdef _MFC_VER
	SVNPrompt					m_prompt;
#endif

	/**
	 * Returns a numeric value indicating the importance of a status. 
	 * A higher number indicates a more important status.
	 */
	static int GetStatusRanking(svn_wc_status_kind status);

	/**
	 * Callback function which collects the raw status from a svn_client_status() function call
	 */
	static void getallstatus (void *baton, const char *path, svn_wc_status2_t *status);

	/**
	 * Callback function which stores the raw status from a svn_client_status() function call
	 * in a hashtable.
	 */
	static void getstatushash (void *baton, const char *path, svn_wc_status2_t *status);

	/**
	 * helper function to sort a hash to an array
	 */
	static apr_array_header_t * sort_hash (apr_hash_t *ht, int (*comparison_func) (const sort_item *,
										const sort_item *), apr_pool_t *pool);

	/**
	 * Callback function used by qsort() which does the comparison of two elements
	 */
	static int __cdecl sort_compare_items_as_paths (const sort_item *a, const sort_item *b);

	//for GetFirstFileStatus and GetNextFileStatus
	apr_hash_t *				m_statushash;
	apr_array_header_t *		m_statusarray;
	unsigned int				m_statushashindex;
	apr_hash_t *				m_externalhash;

#pragma warning(push)
#pragma warning(disable: 4200)
	struct STRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)	// C4200

	static int LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage);
	static svn_error_t* cancel(void *baton);

	// A sorted list of filenames (in SVN format, in lowercase) 
	// when this list is set, we only pick-up files during a GetStatus which are found in this list
	typedef std::vector<std::string> StdStrAVector;
	StdStrAVector m_filterFileList;
};

