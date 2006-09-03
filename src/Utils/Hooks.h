// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2006 - Stefan Kueng

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
#include <map>
#include "registry.h"
#include "TSVNPath.h"
#include "SVNRev.h"

/**
 * enumeration of all client hook types
 */
typedef enum hooktype
{
	unknown_hook,
	start_commit_hook,
	pre_commit_hook,
	post_commit_hook,
	start_update_hook,
	pre_update_hook,
	post_update_hook
} hooktype;

/**
 * helper class, used as the key to the std::map we store
 * the data for the client hook scripts in.
 */
class hookkey
{
public:
	hooktype		htype;
	CTSVNPath		path;

	bool operator < (const hookkey& hk) const 
	{
		if (htype == hk.htype) 
			return (path < hk.path); 
		else 
			return htype < hk.htype;
	}
};

/**
 * helper struct, used as the value to the std::map we
 * store the data for the client hook scripts in.
 */
typedef struct hookcmd
{
	CString			commandline;
	bool			bWait;
	bool			bShow;
} hookcmd;

typedef std::map<hookkey, hookcmd>::iterator hookiterator;

/**
 * \ingroup Utils
 * Singleton class which deals with the client hook scripts.
 */
class CHooks : public std::map<hookkey, hookcmd>
{
private:
	CHooks();
	~CHooks();
public:
	/// Create the singleton. Call this at the start of the program.
	static bool			Create();
	/// Returns the singleton instance
	static CHooks&		Instance();
	/// Destroys the singleton object. Call this at the end of the program.
	static void			Destroy();

public:
	/// Saves the hook script information to the registry.
	bool				Save();
	/**
	 * Removes the hook script identified by \c key. To make the change persistent
	 * call Save().
	 */
	bool				Remove(hookkey key);
	/**
	 * Adds a new hook script. To make the change persistent, call Save().
	 */
	void				Add(hooktype ht, const CTSVNPath& Path, LPCTSTR szCmd, bool bWait, bool bShow);

	/// returns the string representation of the hook type.
	static CString		GetHookTypeString(hooktype t);
	/// returns the hooktype from a string representation of the same.
	static hooktype		GetHookType(const CString& s);

	/**
	 * Executes the Start-Update-Hook that first matches one of the paths in
	 * \c pathList.
	 * \param pathList a list of paths to look for the hook scripts
	 * \param exitcode on return, contains the exit code of the hook script
	 * \param error the data the hook script outputs to stderr
	 * \remark the string "%PATHS% in the command line of the hook script is 
	 * replaced with the path to a temporary file which contains a list of files
	 * in \c pathList, separated by newlines. The hook script can parse this
	 * file to get all the paths the update is about to be done on.
	 */
	bool				StartUpdate(const CTSVNPathList& pathList, DWORD& exitcode, CString& error);
	/**
	 * Executes the Pre-Update-Hook that first matches one of the paths in
	 * \c pathList.
	 * \param pathList a list of paths to look for the hook scripts
	 * \param bRecursive set to true if the update is done recursively
	 * \param rev the revision the update is done to
	 * \param exitcode on return, contains the exit code of the hook script
	 * \param error the data the hook script outputs to stderr
	 * \remark the string "%PATHS% in the command line of the hook script is 
	 * replaced with the path to a temporary file which contains a list of files
	 * in \c pathList, separated by newlines. The hook script can parse this
	 * file to get all the paths the update is about to be done on.
	 * The string "%RECURSIVE%" is replaced with either "recursive" or "nonrecursive" according
	 * to the \c bRecursive parameter. And the string "%REVISION%" is replaced with
	 * the string representation of \c rev.
	 */
	bool				PreUpdate(const CTSVNPathList& pathList, bool bRecursive, SVNRev rev, DWORD& exitcode, CString& error);
	/**
	 * Executes the Post-Update-Hook that first matches one of the paths in
	 * \c pathList.
	 * \param pathList a list of paths to look for the hook scripts
	 * \param bRecursive set to true if the update is done recursively
	 * \param rev the revision the update was done to
	 * \param exitcode on return, contains the exit code of the hook script
	 * \param error the data the hook script outputs to stderr
	 * \remark the string "%PATHS% in the command line of the hook script is 
	 * replaced with the path to a temporary file which contains a list of files
	 * in \c pathList, separated by newlines. The hook script can parse this
	 * file to get all the paths the update is about to be done on.
	 * The string "%RECURSIVE%" is replaced with either "recursive" or "nonrecursive" according
	 * to the \c bRecursive parameter. And the string "%REVISION%" is replaced with
	 * the string representation of \c rev.
	 */
	bool				PostUpdate(const CTSVNPathList& pathList, bool bRecursive, SVNRev rev, DWORD& exitcode, CString& error);

	/**
	 * Executes the Start-Commit-Hook that first matches one of the paths in
	 * \c pathList.
	 * \param pathList a list of paths to look for the hook scripts
	 * \param exitcode on return, contains the exit code of the hook script
	 * \param error the data the hook script outputs to stderr
	 * \remark the string "%PATHS% in the command line of the hook script is 
	 * replaced with the path to a temporary file which contains a list of files
	 * in \c pathList, separated by newlines. The hook script can parse this
	 * file to get all the paths the commit is about to be done on.
	 */
	bool				StartCommit(const CTSVNPathList& pathList, DWORD& exitcode, CString& error);
	/**
	 * Executes the Pre-Commit-Hook that first matches one of the paths in
	 * \c pathList.
	 * \param pathList a list of paths to look for the hook scripts
	 * \param bRecursive set to true if the commit is done recursively
	 * \param exitcode on return, contains the exit code of the hook script
	 * \param error the data the hook script outputs to stderr
	 * \remark the string "%PATHS% in the command line of the hook script is 
	 * replaced with the path to a temporary file which contains a list of files
	 * in \c pathList, separated by newlines. The hook script can parse this
	 * file to get all the paths the update is about to be done on.
	 * The string "%RECURSIVE%" is replaced with either "recursive" or "nonrecursive" according
	 * to the \c bRecursive parameter.
	 */
	bool				PreCommit(const CTSVNPathList& pathList, bool bRecursive, DWORD& exitcode, CString& error);
	/**
	 * Executes the Post-Commit-Hook that first matches one of the paths in
	 * \c pathList.
	 * \param pathList a list of paths to look for the hook scripts
	 * \param bRecursive set to true if the commit is done recursively
	 * \param rev the revision the commit was done to
	 * \param exitcode on return, contains the exit code of the hook script
	 * \param error the data the hook script outputs to stderr
	 * \remark the string "%PATHS% in the command line of the hook script is 
	 * replaced with the path to a temporary file which contains a list of files
	 * in \c pathList, separated by newlines. The hook script can parse this
	 * file to get all the paths the commit is about to be done on.
	 * The string "%RECURSIVE%" is replaced with either "recursive" or "nonrecursive" according
	 * to the \c bRecursive parameter. And the string "%REVISION%" is replaced with
	 * the string representation of \c rev.
	 */
	bool				PostCommit(const CTSVNPathList& pathList, bool bRecursive, SVNRev rev, DWORD& exitcode, CString& error);
private:
	/**
	 * Starts a new process, specified in \c cmd.
	 * \param error the data the process writes to stderr
	 * \param bWait if true, then this method waits until the created process has finished. If false, then the return
	 * value will always be 0 and \c error will be an empty string.
	 * \param bShow set to true if the process should be started visible.
	 * \return the exit code of the process if \c bWait is true, zero otherwise.
	 */
	DWORD				RunScript(CString cmd, CString& error, bool bWait, bool bShow);
	/**
	 * Find the hook script information for the hook type \c t which matches a
	 * path in \c pathList.
	 */
	hookiterator		FindItem(hooktype t, const CTSVNPathList& pathList);
	static CHooks *		m_pInstance;
};