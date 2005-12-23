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

#pragma once

#include "SVNPrompt.h"
#include "SVNRev.h"
#include "SVNGlobal.h"

class CProgressDlg;
class CTSVNPath;
class CTSVNPathList;

svn_error_t * svn_cl__get_log_message (const char **log_msg,
									const char **tmp_file,
									const apr_array_header_t * commit_items,
									void *baton, apr_pool_t * pool);
#define SVN_PROGRESS_QUEUE_SIZE 10
#define SVN_DATE_BUFFER 260

/**
 * \ingroup SVN
 * This class provides all Subversion commands as methods and adds some helper
 * methods to the Subversion commands. Also provides virtual methods for the
 * callbacks Subversion uses. This class can't be used directly but must be
 * inherited if the callbacks are used.
 *
 * \remark Please note that all URLs passed to any of the methods of this class
 * must not be escaped! Escaping and unescaping of URLs is done automatically.
 * The drawback of this is that valid pathnames with sequences looking like escaped
 * chars may not work correctly under certain circumstances.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 10-20-2002
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class SVN
{
public:
	SVN(void);
	~SVN(void);

	struct LogChangedPath
	{
		CString sPath;
		CString sCopyFromPath;
		svn_revnum_t lCopyFromRev;
		CString sAction;
	};
#	define LOGACTIONS_MODIFIED	0x00000001
#	define LOGACTIONS_REPLACED	0x00000002
#	define LOGACTIONS_ADDED		0x00000004
#	define LOGACTIONS_DELETED	0x00000008
	typedef CArray<LogChangedPath*, LogChangedPath*> LogChangedPathArray;
	virtual BOOL Cancel();
	virtual BOOL Notify(const CTSVNPath& path, svn_wc_notify_action_t action, 
							svn_node_kind_t kind, const CString& mime_type, 
							svn_wc_notify_state_t content_state, 
							svn_wc_notify_state_t prop_state, svn_revnum_t rev,
							const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
							svn_error_t * err, apr_pool_t * pool);
	virtual BOOL Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions);
	virtual BOOL BlameCallback(LONG linenumber, svn_revnum_t revision, const CString& author, const CString& date, const CStringA& line);

	struct SVNLock
	{
		CString path;
		CString token;
		CString owner;
		CString comment;
		__time64_t creation_date;
		__time64_t expiration_date;
	};
	
	struct SVNProgress
	{
		apr_off_t progress;			///< operation progress
		apr_off_t total;			///< operation progress
		apr_off_t overall_total;	///< total bytes transferred, use SetAndClearProgressInfo() to reset this
		apr_off_t BytesPerSecond;	///< Speed in bytes per second
		CString	  SpeedString;		///< String for speed. Either "xxx Bytes/s" or "xxx kBytes/s"
	};
	
	/**
	 * If a method of this class returns FALSE then you can
	 * get the detailed error message with this method.
	 * \return the error message string
	 */
	CString GetLastErrorMessage(int wrap = 80);
	/** 
	 * Checkout a working copy of moduleName at revision, using destPath as the root
	 * directory of the newly checked out working copy
	 * \param moduleName the path/url of the repository
	 * \param destPath the path to the local working copy
	 * \param revision the revision number to check out
	 * \param pegrev the peg revision to use
	 * \param recurse TRUE if you want to check out all subdirs and files (recommended)
	 * \param bIgnoreExternals if TRUE, do not check out externals
	 */
	BOOL Checkout(const CTSVNPath& moduleName, const CTSVNPath& destPath, SVNRev pegrev, SVNRev revision, BOOL recurse, BOOL bIgnoreExternals);
	/**
	 * If pathlist contains an URL, use the MESSAGE to immediately attempt 
	 * to commit a deletion of the URL from the repository. 
	 * Else, schedule a working copy path for removal from the repository.
	 * path's parent must be under revision control. This is just a
	 * \b scheduling operation.  No changes will happen to the repository until
	 * a commit occurs.  This scheduling can be removed with
	 * Revert(). If path is a file it is immediately removed from the
	 * working copy. If path is a directory it will remain in the working copy
	 * but all the files, and all unversioned items, it contains will be
	 * removed. If force is not set then this operation will fail if path
	 * contains locally modified and/or unversioned items. If force is set such
	 * items will be deleted.
	 *
	 * \param pathlist a list of files/directories to delete
	 * \param force if TRUE, all files including those not versioned are deleted. If FALSE the operation
	 * will fail if a directory contains unversioned files or if the file itself is not versioned.
	 */
	BOOL Remove(const CTSVNPathList& pathlist, BOOL force, CString message = _T(""));
	/**
	 * Reverts a list of files/directories to its pristine state. I.e. its reverted to the state where it
	 * was last updated with the repository.
	 * \param path the file/directory to revert
	 * \param recurse 
	 */
	BOOL Revert(const CTSVNPathList& pathlist, BOOL recurse);
	/**
	 * Schedule a working copy path for addition to the repository.
	 * path's parent must be under revision control already, but path is
	 * not. If recursive is set, then assuming path is a directory, all
	 * of its contents will be scheduled for addition as well.
	 * Important:  this is a \b scheduling operation.  No changes will
	 * happen to the repository until a commit occurs.  This scheduling
	 * can be removed with Revert().
	 *
	 * \param path the file/directory to add 
	 * \param recurse 
	 * \param force if TRUE, then an adding an already versioned folder will add
	 *              all unversioned files in it (in combination with \a recurse)
	 * \param no_ignore if FALSE, then don't add ignored files.
	 */
	BOOL Add(const CTSVNPathList& pathList, BOOL recurse, BOOL force = FALSE, BOOL no_ignore = FALSE);
	/**
	 * Update working tree path to revision.
	 * \param pathList the files/directories to update
	 * \param revision the revision the local copy should be updated to or -1 for HEAD
	 * \param recurse 
	 * \param ignoreexternals if TRUE, don't update externals
	 */
	BOOL Update(const CTSVNPathList& pathList, SVNRev revision, BOOL recurse, BOOL ignoreexternals);
	/**
	 * Commit file or directory path into repository, using message as
	 * the log message.
	 * 
	 * If no error is returned and revision is set to -1, then the 
	 * commit was a no-op; nothing needed to be committed.
	 *
	 * If 0 is returned then the commit failed. Use GetLastErrorMessage()
	 * to get detailed error information.
	 *
	 * \param path the file/directory to commit
	 * \param message a log message describing the changes you made
	 * \param recurse 
	 * \param keep_locks if TRUE, the locks are not removed on commit
	 * \return the resulting revision number.
	 */
	svn_revnum_t Commit(const CTSVNPathList& pathlist, CString message, BOOL recurse, BOOL keep_locks);
	/**
	 * Copy srcPath to destPath.
	 * 
	 * srcPath must be a file or directory under version control, or the
	 * URL of a versioned item in the repository.  If srcPath is a URL,
	 * revision is used to choose the revision from which to copy the
	 * srcPath. destPath must be a file or directory under version
	 * control, or a repository URL, existent or not.
	 *
	 * If either srcPath or destPath are URLs, immediately attempt 
	 * to commit the copy action in the repository. 
	 * 
	 * If neither srcPath nor destPath is a URL, then this is just a
	 * variant of Add(), where the destPath items are scheduled
	 * for addition as copies.  No changes will happen to the repository
	 * until a commit occurs.  This scheduling can be removed with
	 * Revert().
	 *
	 * \param srcPath source path
	 * \param destPath destination path
	 * \return the new revision number
	 */
	BOOL Copy(const CTSVNPath& srcPath, const CTSVNPath& destPath, SVNRev revision, CString logmsg = _T(""));
	/**
	 * Move srcPath to destPath.
	 * 
	 * srcPath must be a file or directory under version control and 
	 * destPath must also be a working copy path (existent or not).
	 * This is a scheduling operation.  No changes will happen to the
	 * repository until a commit occurs.  This scheduling can be removed
	 * with Revert().  If srcPath is a file it is removed from
	 * the working copy immediately.  If srcPath is a directory it will
	 * remain in the working copy but all the files and unversioned items
	 * it contains will be removed.
	 *
	 * If srcPath contains locally modified and/or unversioned items and
	 * force is not set, the copy will fail. If force is set such items
	 * will be removed.
	 * 
	 * \param srcPath source path
	 * \param destPath destination path
	 * \param force 
	 */
	BOOL Move(const CTSVNPath& srcPath, const CTSVNPath& destPath, BOOL force, CString message = _T(""));
	/**
	 * If path is a URL, use the message to immediately
	 * attempt to commit the creation of the directory URL in the
	 * repository.
	 * Else, create the directory on disk, and attempt to schedule it for
	 * addition.
	 * 
	 * \param path 
	 * \param message 
	 */
	BOOL MakeDir(const CTSVNPathList& pathlist, CString message);
	/**
	 * Recursively cleanup a working copy directory DIR, finishing any
	 * incomplete operations, removing lockfiles, etc.
	 * \param path the file/directory to clean up
	 */
	BOOL CleanUp(const CTSVNPath& path);
	/**
	 * Remove the 'conflicted' state on a working copy PATH.  This will
	 * not semantically resolve conflicts;  it just allows path to be
	 * committed in the future.
	 * If recurse is set, recurse below path, looking for conflicts to
	 * resolve.
	 * If path is not in a state of conflict to begin with, do nothing.
	 * 
	 * \param path the path to resolve
	 * \param recurse 
	 */
	BOOL Resolve(const CTSVNPath& path, BOOL recurse);
	/**
	 * Export the contents of either a subversion repository or a subversion 
	 * working copy into a 'clean' directory (meaning a directory with no 
	 * administrative directories).

	 * \param srcPath	either the path the working copy on disk, or a url to the
	 *					repository you wish to export.
	 * \param destPath	the path to the directory where you wish to create the exported tree.
	 * \param revision	the revision that should be exported, which is only used 
	 *					when exporting from a repository.
	 * \param force		TRUE if existing files should be overwritten
	 */
	BOOL Export(const CTSVNPath& srcPath, const CTSVNPath& destPath, SVNRev pegrev, SVNRev revision, BOOL force = TRUE, BOOL bIgnoreExternals = FALSE, HWND hWnd = NULL, BOOL extended = FALSE);
	/**
	 * Switch working tree path to URL at revision
	 *
	 * Summary of purpose: this is normally used to switch a working
	 * directory over to another line of development, such as a branch or
	 * a tag.  Switching an existing working directory is more efficient
	 * than checking out URL from scratch.
	 *
	 * \param path the path of the working directory
	 * \param url the url of the repository
	 * \param revision the revision number to switch to
	 * \param recurse 
	 */
	BOOL Switch(const CTSVNPath& path, const CTSVNPath& url, SVNRev revision, BOOL recurse);
	/**
	 * Import file or directory path into repository directory url at
	 * head and using LOG_MSG as the log message for the (implied)
	 * commit.
	 * If path is a directory, the contents of that directory are
	 * imported, under a new directory named newEntry under url; or if
	 * newEntry is null, then the contents of path are imported directly
	 * into the directory identified by url.  Note that the directory path
	 * itself is not imported -- that is, the basename of path is not part
	 * of the import.
	 *
	 * If path is a file, that file is imported as newEntry (which may
	 * not be null).
	 *
	 * In all cases, if newEntry already exists in url, return error.
	 *
	 * \param path		the file/directory to import
	 * \param url		the url to import to
	 * \param message	log message used for the 'commit'
	 * \param recurse 
	 * \param no_ignore	If no_ignore is FALSE, don't add files or directories that match ignore patterns.
	 */
	BOOL Import(const CTSVNPath& path, const CTSVNPath& url, CString message, BOOL recurse, BOOL no_ignore);
	/**
	 * Merge changes from path1/revision1 to path2/revision2 into the
	 * working-copy path localPath.  path1 and path2 can be either
	 * working-copy paths or URLs.
	 *
	 * By "merging", we mean:  apply file differences and schedule 
	 * additions & deletions when appopriate.
	 *
	 * path1 and path2 must both represent the same node kind -- that is,
	 * if path1 is a directory, path2 must also be, and if path1 is a
	 * file, path2 must also be.
	 * 
	 * If recurse is true (and the paths are directories), apply changes
	 * recursively; otherwise, only apply changes in the current
	 * directory.
	 *
	 * If force is not set and the merge involves deleting locally modified or
	 * unversioned items the operation will fail.  If force is set such items
	 * will be deleted.
	 *
	 * \param path1		first path
	 * \param revision1	first revision
	 * \param path2		second path
	 * \param revision2 second revision
	 * \param localPath destination path
	 * \param force		see description
	 * \param recurse 
	 */
	BOOL Merge(const CTSVNPath& path1, SVNRev revision1, const CTSVNPath& path2, SVNRev revision2, const CTSVNPath& localPath, BOOL force, BOOL recurse, BOOL ignoreanchestry = FALSE, BOOL dryrun = FALSE);

	BOOL PegMerge(const CTSVNPath& source, SVNRev revision1, SVNRev revision2, SVNRev pegrevision, const CTSVNPath& destpath, BOOL force, BOOL recurse, BOOL ignoreancestry = FALSE, BOOL dryrun = FALSE);
	/**
	 * Produce diff output which describes the delta between \a path1/\a revision1 and \a path2/\a revision2
	 * Print the output of the diff to \a outputfile, and any errors to \a errorfile. \a path1 
	 * and \a path2 can be either working-copy paths or URLs.
	 * \a path1 and \a path2 must both represent the same node kind -- that
	 * is, if \a path1 is a directory, \a path2 must also be, and if \a path1
	 * is a file, \a path2 must also be.  (Currently, \a path1 and \a path2
	 * must be the exact same path)
	 * 
	 * If \a recurse is true (and the \a paths are directories) this will be a
	 * recursive operation.
	 *
	 * Use \a ignore_ancestry to control whether or not items being
	 * diffed will be checked for relatedness first.  Unrelated items
	 * are typically transmitted to the editor as a deletion of one thing
	 * and the addition of another, but if this flag is \c TRUE,
	 * unrelated items will be diffed as if they were related.
	 * 
	 * If \a no_diff_deleted is true, then no diff output will be
	 * generated on deleted files.
	 * 
	 * \a diff_options (an array of <tt>const char *</tt>) is used to pass 
	 * additional command line options to the diff processes invoked to compare files.
	 *
	 * \remark - the use of two overloaded functions rather than default parameters is to avoid the
	 * CTSVNPath constructor (and hence #include) being visible in this header file
	 */
	BOOL Diff(const CTSVNPath& path1, SVNRev revision1, const CTSVNPath& path2, SVNRev revision2, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, BOOL ignorecontenttype, CString options, bool bAppend, const CTSVNPath& outputfile, const CTSVNPath& errorfile);
	BOOL Diff(const CTSVNPath& path1, SVNRev revision1, const CTSVNPath& path2, SVNRev revision2, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, BOOL ignorecontenttype,  CString options, bool bAppend, const CTSVNPath& outputfile);

	/**
	 * Produce diff output which describes the delta between the filesystem object \a path in 
	 * peg revision \a pegrevision, as it changed between \a startrev and \a endrev. 
	 * Print the output of the diff to outputfile, and any errors to errorfile. 
	 * \a path can be either a working-copy path or URL.
	 *
	 * All other options are handled identically to Diff().
	 */
	BOOL PegDiff(const CTSVNPath& path, SVNRev pegrevision, SVNRev startrev, SVNRev endrev, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, BOOL ignorecontenttype,  CString options, const CTSVNPath& outputfile, const CTSVNPath& errorfile);
	BOOL PegDiff(const CTSVNPath& path, SVNRev pegrevision, SVNRev startrev, SVNRev endrev, BOOL recurse, BOOL ignoreancestry, BOOL nodiffdeleted, BOOL ignorecontenttype,  CString options, const CTSVNPath& outputfile);

	/**
	 * fires the Log-event on each log message from revisionStart
	 * to revisionEnd inclusive (but never fires the event
	 * on a given log message more than once).
	 * To receive the messages you need to listen to Log() events.
	 *
	 * \param path the file/directory to get the log of
	 * \param revisionStart	the revision to start the logs from
	 * \param revisionEnd the revision to stop the logs
	 * \param changed TRUE if the log should follow changed paths 
	 */
	BOOL ReceiveLog(const CTSVNPathList& pathlist, SVNRev revisionStart, SVNRev revisionEnd, int limit, BOOL changed, BOOL strict = FALSE);
	
	/**
	 * Checks out a file with \a revision to \a localpath.
	 * \param revision the revision of the file to checkout
	 * \param localpath the place to store the file
	 */
	BOOL Cat(const CTSVNPath& url, SVNRev pegrevision, SVNRev revision, const CTSVNPath& localpath);

	/**
	 * Lists the sub directories under a repository URL. The returned strings start with either
	 * a "d" if the entry is a directory, a "f" if it is a file or a "u" if the type of the 
	 * entry is unknown. So to retrieve only the name of the entry you have to cut off the
	 * first character of the string.
	 *
	 * If \a extended is set to TRUE, all entries are provided in extended format. In this case,
	 * not only the name of the entry, but also a number of additional information is provided
	 * in the returned string array. Additional data includes the revision number and the author
	 * of the last change, the file size and the date of the last change (in this order). Each
	 * item is separated by a TAB '\t' character from the previous one.
	 *
	 * \note The "extended" entry format is surely not a good solution for general purpose, and
	 * should be revised in the future. But it is exactly what is needed for the upcoming Repository
	 * Browser.
	 *
	 * \param url url to the repository you wish to ls.
	 * \param pegrev peg revision
	 * \param revision	the revision that you want to explore
	 * \param entries CStringArray of subdirectories
	 * \param extended Set to TRUE for entries in extended format (see above)
	 * \param recursive Set this to TRUE to get all entries recursively
	 */
	BOOL Ls(const CTSVNPath& url, SVNRev pegrev, SVNRev revision, CStringArray& entries, BOOL extended = FALSE, BOOL recursive = FALSE, BOOL escaped = FALSE);

	/**
	 * Relocates a working copy to a new/changes repository URL. Use this function
	 * if the URL of the repository server has changed.
	 * \param path path to the working copy
	 * \param from the old URL of the repository
	 * \param to the new URL of the repository
	 * \param recurse TRUE if the operation should be recursive
	 * \return TRUE if successful
	 */
	BOOL Relocate(const CTSVNPath& path, const CTSVNPath& from, const CTSVNPath& to, BOOL recurse);

	/**
	 * Determine the author for each line in a file (blame the changes on someone).
	 * The result is given in the callback function BlameCallback()
	 *
	 * \param path the path or url of the file
	 * \param startrev the revision from which the check is done from
	 * \param endrev the end revision where the check is stopped
	 */
	BOOL Blame(const CTSVNPath& path, SVNRev startrev, SVNRev endrev);
	
	/**
	 * Lock a file for exclusive use so no other users are allowed to edit
	 * the same file. A commit of a locked file is rejected if the lock isn't
	 * owned by the committer himself.
	 * \param pathList a list of filepaths to lock
	 * \param bStealLock if TRUE, an already existing lock is overwritten
	 * \param comment a comment to assign to the lock. Only used by svnserve!
	 */
	BOOL Lock(const CTSVNPathList& pathList, BOOL bStealLock, const CString& comment = CString());
	
	/**
	 * Removes existing locks from files.
	 * \param pathList a list of filepaths to remove the lock from
	 * \param bBreakLock if TRUE, the locks are removed even if the committer
	 * isn't the owner of the locks!
	 */
	BOOL Unlock(const CTSVNPathList& pathList, BOOL bBreakLock);
	
	/**
	 * Checks if a windows path is a local repository
	 */
	BOOL IsRepository(const CString& strPath);

	/**
	 * Finds the repository root of a given url. 
	 * \return The root url or an empty string
	 */
	CString GetRepositoryRoot(const CTSVNPath& url);

	/**
	 * Checks if a file:/// url points to a BDB repository.
	 */
	static BOOL IsBDBRepository(CString url);

	/**
	 * Returns the HEAD revision of the URL or WC-Path.
	 * Or -1 if the function failed.
	 */
	svn_revnum_t GetHEADRevision(const CTSVNPath& url);

	BOOL GetRootAndHead(const CTSVNPath& path, CTSVNPath& url, svn_revnum_t& rev);

	/**
	 * Set the revision property \a sName to the new value \a sValue.
	 * \param sURL the URL of the file/folder
	 * \param rev the revision number to change the revprop
	 * \return the actual revision number the property value was set
	 */
	svn_revnum_t RevPropertySet(CString sName, CString sValue, CString sURL, SVNRev rev);

	/**
	 * Reads the revision property \a sName and returns its value.
	 * \param sURL the URL of the file/folder
	 * \param rev the revision number
	 * \return the value of the property
	 */
	CString	RevPropertyGet(CString sName, CString sURL, SVNRev rev);

	/**
	 * Fetches all locks for \a url and the paths below.
	 * \remark The CString key in the map is the absolute path in
	 * the repository of the lock. It is \b not an absolute URL, the
	 * repository root part is stripped off!
	 */
	BOOL GetLocks(const CTSVNPath& url, std::map<CString, SVNLock> * locks);	

	CString GetURLFromPath(const CTSVNPath& path);
	CString GetUUIDFromPath(const CTSVNPath& path);

	static CString CheckConfigFile();

	/**
	 * Creates a repository at the specified location.
	 * \param path where the repository should be created
	 * \return TRUE if operation was successful
	 */
	static BOOL CreateRepository(CString path, CString fstype = _T("bdb"));

	/**
	 * Convert Windows Path to Local Repository URL
	 */
	static void PathToUrl(CString &path);

	/**
	 * Convert Local Repository URL to Windows Path
	 */
	static void UrlToPath(CString &url);

	/** 
	 * Returns the path to the text-base file of the working copy file.
	 * If no text base exists for the file then the returned string is empty.
	 */
	static CTSVNPath GetPristinePath(const CTSVNPath& wcPath);

	/**
	 * Returns the path to a translated version of sFile. If no translation of the
	 * file is needed, then sTranslatedFile will be the same as sFile. If translation
	 * is needed, then sTranslatedFile points to a translated file. The caller is
	 * responsible to delete that translated file after use!
	 * \param bForceRepair Repair any inconsistent line endings.
	 * \return TRUE if a translation was needed and the file in sTranslatedFile needs deleting after use
	 */
	static BOOL GetTranslatedFile(CTSVNPath& sTranslatedFile, const CTSVNPath sFile, BOOL bForceRepair = TRUE);

	/**
	 * convert path to a subversion path (replace '\' with '/')
	 */
	static void preparePath(CString &path);

	/**
	 * Checks if a given path is a valid URL.
	 */	 	 	 	
	static BOOL PathIsURL(const CString& path);

	/**
	 * Creates the Subversion config file if it doesn't already exist.
	 */
	static BOOL EnsureConfigFile();

	/** 
	* Set the parent window of an authentication prompt dialog
	*/
	void SetPromptParentWindow(HWND hWnd);
	/** 
	* Set the MFC Application object for a prompt dialog
	*/
	void SetPromptApp(CWinApp* pWinApp);

	void SetAndClearProgressInfo(HWND hWnd);
	void SetAndClearProgressInfo(CProgressDlg * pProgressDlg, bool bShowProgressBar = false);
	
	static CString GetErrorString(svn_error_t * Err, int wrap = 80);
	static CStringA MakeSVNUrlOrPath(const CString& UrlOrPath);
	static CString MakeUIUrlOrPath(CStringA UrlOrPath);
	static void formatDate(TCHAR date_native[], apr_time_t& date_svn, bool force_short_fmt = false);

	static void UseIEProxySettings(apr_hash_t * cfg);
	svn_error_t *				Err;			///< Global error object struct
private:
	svn_client_ctx_t * 			m_pctx;
	apr_hash_t *				statushash;
	apr_array_header_t *		statusarray;
	svn_wc_status_t *			status;
	apr_pool_t *				parentpool;
	apr_pool_t *				pool;			///< memory pool
	svn_opt_revision_t			rev;			///< subversion revision. used by getRevision()
	SVNPrompt					m_prompt;

	svn_opt_revision_t *	getRevision (svn_revnum_t revNumber);
	void * logMessage (const char * message, char * baseDirectory = NULL);

	// Convert a TSVNPathList into an array of SVN paths
	apr_array_header_t * MakePathArray(const CTSVNPathList& pathList);

	svn_error_t * get_url_from_target (const char **URL, const char *target);
	svn_error_t * get_uuid_from_target (const char **UUID, const char *target);
	static svn_error_t* cancel(void *baton);
	static void notify( void *baton,
						const svn_wc_notify_t *notify,
						apr_pool_t *pool);
	static svn_error_t* logReceiver(void* baton, 
					apr_hash_t* ch_paths, 
					svn_revnum_t rev, 
					const char* author, 
					const char* date, 
					const char* msg, 
					apr_pool_t* pool);
	static svn_error_t* blameReceiver(void* baton,
					apr_off_t line_no,
					svn_revnum_t revision,
					const char * author,
					const char * date,
					const char * line,
					apr_pool_t * pool);

	static void progress_func(apr_off_t progress, apr_off_t total, void *baton, apr_pool_t *pool);
	SVNProgress		m_SVNProgressMSG;
	HWND			m_progressWnd;
	CProgressDlg *	m_pProgressDlg;
	bool			m_progressWndIsCProgress;
	bool			m_bShowProgressBar;
	apr_off_t	progress_total;
	apr_off_t	progress_averagehelper;
	apr_off_t	progress_lastprogress;
	apr_off_t	progress_lasttotal;
	DWORD		progress_lastTicks;
	std::vector<apr_off_t> progress_vector;

};

static UINT WM_SVNPROGRESS = RegisterWindowMessage(_T("TORTOISESVN_SVNPROGRESS_MSG"));

