// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2006-2014 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once
#include <map>
#include "registry.h"
#include "TSVNPath.h"
#include "SVNRev.h"

class ProjectProperties;

/**
 * \ingroup TortoiseProc
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
    post_update_hook,
    issue_tracker_hook,
    pre_connect_hook,
    manual_precommit,
    check_commit_hook
} hooktype;

/**
 * \ingroup TortoiseProc
 * helper class, used as the key to the std::map we store
 * the data for the client hook scripts in.
 */
class hookkey
{
public:
    hooktype        htype;
    CTSVNPath       path;

    bool operator < (const hookkey& hk) const
    {
        if (htype == hk.htype)
            return (path < hk.path);
        else
            return htype < hk.htype;
    }
};

/**
 * \ingroup TortoiseProc
 * helper struct, used as the value to the std::map we
 * store the data for the client hook scripts in.
 */
typedef struct hookcmd
{
    CString         commandline;    ///< command line to be executed
    bool            bWait;          ///< wait until the hook exited
    bool            bShow;          ///< show hook executable window
    bool            bEnforce;       ///< hook can not be skipped
    bool            bApproved;      ///< user explicitly approved
    bool            bStored;        ///< use decision is stored in reg
    CString         sRegKey;
} hookcmd;

typedef std::map<hookkey, hookcmd>::iterator hookiterator;

/**
 * \ingroup TortoiseProc
 * Singleton class which deals with the client hook scripts.
 */
class CHooks : public std::map<hookkey, hookcmd>
{
private:
    CHooks();
    ~CHooks();
    void AddPathParam(CString& sCmd, const CTSVNPathList& pathList);
    void AddDepthParam(CString& sCmd, svn_depth_t depth);
    void AddCWDParam(CString& sCmd, const CTSVNPathList& pathList);
    void AddErrorParam(CString& sCmd, const CString& error);
    void AddParam(CString& sCmd, const CString& param);
    CTSVNPath AddMessageFileParam(CString& sCmd, const CString& message);
public:
    /// Create the singleton. Call this at the start of the program.
    static bool         Create();
    /// Returns the singleton instance
    static CHooks&      Instance();
    /// Destroys the singleton object. Call this at the end of the program.
    static void         Destroy();

public:
    /// Saves the hook script information to the registry.
    bool                Save();
    /**
     * Removes the hook script identified by \c key. To make the change persistent
     * call Save().
     */
    bool                Remove(hookkey key);
    /**
     * Adds a new hook script. To make the change persistent, call Save().
     */
    void                Add(hooktype ht, const CTSVNPath& Path, LPCTSTR szCmd,
                            bool bWait, bool bShow, bool bEnforce);

    /// returns the string representation of the hook type.
    static CString      GetHookTypeString(hooktype t);
    /// returns the hooktype from a string representation of the same.
    static hooktype     GetHookType(const CString& s);

    /// Add hook script data from project properties
    void                SetProjectProperties(const CTSVNPath& wcPath, const ProjectProperties& pp);

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
    bool                StartUpdate(HWND hWnd, const CTSVNPathList& pathList, DWORD& exitcode,
                                    CString& error);
    /**
     * Executes the Pre-Update-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param depth the depth of the commit
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
    bool                PreUpdate(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth,
                                    SVNRev rev, DWORD& exitcode, CString& error);
    /**
     * Executes the Post-Update-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param depth the depth of the commit
     * \param rev the revision the update was done to
     * \param updatedList list of paths that were touched by the update in some way
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
    bool                PostUpdate(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth,
                                    SVNRev rev, const CTSVNPathList& updatedList, DWORD& exitcode, CString& error);

    /**
     * Executes the Start-Commit-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param message a commit message
     * \param exitcode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     * \remark the string "%PATHS% in the command line of the hook script is
     * replaced with the path to a temporary file which contains a list of files
     * in \c pathList, separated by newlines. The hook script can parse this
     * file to get all the paths the commit is about to be done on.
     * The string %MESSAGEFILE% is replaced with path to temporary file containing
     * \c message. If the script finishes successfully, contents of this file
     * is read back into \c message parameter.
     */
    bool                StartCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message,
                                    DWORD& exitcode, CString& error);
    /**
     * Executes the Start-Commit-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param message a commit message
     * \param exitcode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     * \remark the string "%PATHS% in the command line of the hook script is
     * replaced with the path to a temporary file which contains a list of files
     * in \c pathList, separated by newlines. The hook script can parse this
     * file to get all the paths the commit is about to be done on.
     * The string %MESSAGEFILE% is replaced with path to temporary file containing
     * \c message. If the script finishes successfully, contents of this file
     * is read back into \c message parameter.
     */
    bool                CheckCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message,
                                    DWORD& exitcode, CString& error);
    /**
     * Executes the Pre-Commit-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param depth the depth of the commit
     * \param message the commit message
     * \param exitcode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     * \remark the string "%PATHS% in the command line of the hook script is
     * replaced with the path to a temporary file which contains a list of files
     * in \c pathList, separated by newlines. The hook script can parse this
     * file to get all the paths the update is about to be done on.
     * The string "%DEPTH%" is replaced with the numerical value (string) of the
     * svn_depth_t parameter. See the Subversion source documentation about the
     * values.
     */
    bool                PreCommit(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth,
                                    CString& message, DWORD& exitcode,
                                    CString& error);
    /**
     * Executes the Manual Pre-Commit-Hook
     * \param pathList a list of paths that are checked in the commit dialog
     * \param message the commit message if there already is one
     * \param exitcode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     * \remark the string "%PATHS% in the command line of the hook script is
     * replaced with the path to a temporary file which contains a list of files
     * in \c pathList, separated by newlines. The hook script can parse this
     * file to get all the paths the update is about to be done on.
     */
    bool                ManualPreCommit(HWND hWnd, const CTSVNPathList& pathList,
                                        CString& message, DWORD& exitcode,
                                        CString& error);
    /**
     * Executes the Post-Commit-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param depth the depth of the commit
     * \param message the commit message
     * \param rev the revision the commit was done to
     * \param exitcode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     * \remark the string "%PATHS% in the command line of the hook script is
     * replaced with the path to a temporary file which contains a list of files
     * in \c pathList, separated by newlines. The hook script can parse this
     * file to get all the paths the commit is about to be done on.
     * The string "%DEPTH%" is replaced with the numerical value (string) of the
     * svn_depth_t parameter. See the Subversion source documentation about the
     * values.
     */
    bool                PostCommit(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth,
                                    SVNRev rev, const CString& message,
                                    DWORD& exitcode, CString& error);

    /**
     * Executed just before a command is executed that accesses the repository.
     * Can be used to start a tool to connect to a VPN.
     * \param pathList a list of paths to look for the hook scripts
     * \remark The script is only executed once. Subsequent calls within 5 minutes
     * are ignored for performance reasons since TSVN dialogs often call many
     * SVN functions that contact a repository.
     */
    bool                PreConnect(const CTSVNPathList& pathList);

    /**
     * Returns cc true if the hook(s) \c t for the paths given in \c pathList
     * cannot be ignored, i.e. if the user configured it as "forced execution".
     * \param pathList a list of paths to look for the hook scripts
     */
    bool                IsHookExecutionEnforced(hooktype t, const CTSVNPathList& pathList);

    /**
     * Returns true if the hook \c t for the paths given in \c pathList
     * exists.
     */
    bool                IsHookPresent(hooktype t, const CTSVNPathList& pathList);

private:
    /**
     * Starts a new process, specified in \c cmd.
     * \param error the data the process writes to stderr
     * \param bWait if true, then this method waits until the created process has finished. If false, then the return
     * value will always be 0 and \c error will be an empty string.
     * \param bShow set to true if the process should be started visible.
     * \return the exit code of the process if \c bWait is true, zero otherwise.
     */
    DWORD               RunScript(CString cmd, const CTSVNPathList& paths, CString& error, bool bWait, bool bShow);
    /**
     * Find the hook script information for the hook type \c t which matches a
     * path in \c pathList.
     */
    hookiterator        FindItem(hooktype t, const CTSVNPathList& pathList);

    /**
     * Parses the hook information from a project property string
     */
    bool                ParseAndInsertProjectProperty(hooktype t, const CString& strhooks, const CTSVNPath& wcPath,
                                                      const CString& rootPath, const CString& rootUrl,
                                                      const CString& repoRootUrl);

    /**
     * Checks whether the hook script has been validated already and
     * if not, asks the user to validate it.
     */
    bool                ApproveHook(HWND hWnd, hookiterator it);

    static CHooks *     m_pInstance;
    ULONGLONG           m_lastPreConnectTicks;
    bool                m_PathsConvertedToUrls;
    CTSVNPath           m_wcRootPath;
};
