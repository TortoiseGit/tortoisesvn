// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2006-2016, 2021-2022 - TortoiseSVN

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
#include "TSVNPath.h"
#include "SVNRev.h"

class ProjectProperties;

/**
 * \ingroup TortoiseProc
 * enumeration of all client hook types
 */
enum HookType
{
    Unknown_Hook,
    Start_Commit_Hook,
    Pre_Commit_Hook,
    Post_Commit_Hook,
    Start_Update_Hook,
    Pre_Update_Hook,
    Post_Update_Hook,
    Pre_Connect_Hook,
    Manual_Precommit,
    Check_Commit_Hook,
    Pre_Lock_Hook,
    Post_Lock_Hook
};

/**
 * \ingroup TortoiseProc
 * helper class, used as the key to the std::map we store
 * the data for the client hook scripts in.
 */
class HookKey
{
public:
    HookType  hType;
    CTSVNPath path;

    bool      operator<(const HookKey& hk) const
    {
        if (hType == hk.hType)
            return (path < hk.path);
        else
            return hType < hk.hType;
    }
};

/**
 * \ingroup TortoiseProc
 * helper struct, used as the value to the std::map we
 * store the data for the client hook scripts in.
 */
struct HookCmd
{
    CString commandline; ///< command line to be executed
    bool    bWait;       ///< wait until the hook exited
    bool    bShow;       ///< show hook executable window
    bool    bEnforce;    ///< hook can not be skipped
    bool    bApproved;   ///< user explicitly approved
    bool    bStored;     ///< use decision is stored in reg
    CString sRegKey;
};

/**
 * \ingroup TortoiseProc
 * Singleton class which deals with the client hook scripts.
 */
class CHooks : public std::map<HookKey, HookCmd>
{
private:
    CHooks();
    ~CHooks();
    // prevent cloning
    CHooks(const CHooks&)                = delete;
    CHooks&     operator=(const CHooks&) = delete;

    static void AddPathParam(CString& sCmd, const CTSVNPathList& pathList);
    static void AddDepthParam(CString& sCmd, svn_depth_t depth);
    static void AddCWDParam(CString& sCmd, const CTSVNPathList& pathList);
    void        AddErrorParam(CString& sCmd, const CString& error) const;
    static void AddParam(CString& sCmd, const CString& param);
    CTSVNPath   AddMessageFileParam(CString& sCmd, const CString& message) const;

public:
    /// Create the singleton. Call this at the start of the program.
    static bool    Create();
    /// Returns the singleton instance
    static CHooks& Instance();
    /// Destroys the singleton object. Call this at the end of the program.
    static void    Destroy();

public:
    /// Saves the hook script information to the registry.
    bool            Save();
    /**
     * Removes the hook script identified by \c key. To make the change persistent
     * call Save().
     */
    bool            Remove(const HookKey& key);
    /**
     * Adds a new hook script. To make the change persistent, call Save().
     */
    void            Add(HookType ht, const CTSVNPath& path, LPCWSTR szCmd,
                        bool bWait, bool bShow, bool bEnforce);

    /// returns the string representation of the hook type.
    static CString  GetHookTypeString(HookType t);
    /// returns the HookType from a string representation of the same.
    static HookType GetHookType(const CString& s);

    /// Add hook script data from project properties
    void            SetProjectProperties(const CTSVNPath& wcPath, const ProjectProperties& pp);

    /**
     * Executes the Start-Update-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            StartUpdate(HWND hWnd, const CTSVNPathList& pathList, DWORD& exitCode,
                                CString& error);
    /**
     * Executes the Pre-Update-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param depth the depth of the commit
     * \param rev the revision the update is done to
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            PreUpdate(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth,
                              const SVNRev& rev, DWORD& exitCode, CString& error);
    /**
     * Executes the Post-Update-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param depth the depth of the commit
     * \param rev the revision the update was done to
     * \param updatedList list of paths that were touched by the update in some way
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            PostUpdate(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth,
                               const SVNRev& rev, const CTSVNPathList& updatedList, DWORD& exitCode, CString& error);

    /**
     * Executes the Start-Commit-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param message a commit message
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            StartCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message,
                                DWORD& exitCode, CString& error);
    /**
     * Executes the Start-Commit-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param message a commit message
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            CheckCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message,
                                DWORD& exitCode, CString& error);
    /**
     * Executes the Pre-Commit-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param depth the depth of the commit
     * \param message the commit message
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            PreCommit(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth,
                              CString& message, DWORD& exitCode,
                              CString& error);
    /**
     * Executes the Manual Pre-Commit-Hook
     * \param pathList a list of paths that are checked in the commit dialog
     * \param message the commit message if there already is one
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            ManualPreCommit(HWND hWnd, const CTSVNPathList& pathList,
                                    CString& message, DWORD& exitCode,
                                    CString& error);
    /**
     * Executes the Post-Commit-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param depth the depth of the commit
     * \param message the commit message
     * \param rev the revision the commit was done to
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            PostCommit(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth,
                               const SVNRev& rev, const CString& message,
                               DWORD& exitCode, CString& error);

    /**
     * Executed just before a command is executed that accesses the repository.
     * Can be used to start a tool to connect to a VPN.
     * \param pathList a list of paths to look for the hook scripts
     */
    bool            PreConnect(const CTSVNPathList& pathList);

    /**
     * Executes the Pre-Lock-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param lock true for locks, false for unlocks
     * \param steal true if the lock needs stealing
     * \param message the lock message
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            PreLock(HWND hWnd, const CTSVNPathList& pathList,
                            bool lock, bool steal, CString& message,
                            DWORD& exitCode, CString& error);
    /**
     * Executes the Post-Lock-Hook that first matches one of the paths in
     * \c pathList.
     * \param pathList a list of paths to look for the hook scripts
     * \param lock true for locks, false for unlocks
     * \param steal true if the lock needs stealing
     * \param message the lock message
     * \param exitCode on return, contains the exit code of the hook script
     * \param error the data the hook script outputs to stderr
     */
    bool            PostLock(HWND hWnd, const CTSVNPathList& pathList,
                             bool lock, bool steal, const CString& message,
                             DWORD& exitCode, CString& error);

    /**
     * Returns cc true if the hook(s) \c t for the paths given in \c pathList
     * cannot be ignored, i.e. if the user configured it as "forced execution".
     * \param pathList a list of paths to look for the hook scripts
     */
    bool            IsHookExecutionEnforced(HookType t, const CTSVNPathList& pathList);

    /**
     * Returns true if the hook \c t for the paths given in \c pathList
     * exists.
     */
    bool            IsHookPresent(HookType t, const CTSVNPathList& pathList);

private:
    /**
     * Starts a new process, specified in \c cmd.
     * \param error the data the process writes to stderr
     * \param bWait if true, then this method waits until the created process has finished. If false, then the return
     * value will always be 0 and \c error will be an empty string.
     * \param bShow set to true if the process should be started visible.
     * \return the exit code of the process if \c bWait is true, zero otherwise.
     */
    static DWORD                         RunScript(CString cmd, const CTSVNPathList& paths, CString& error, bool bWait, bool bShow);
    /**
     * Find the hook script information for the hook type \c t which matches a
     * path in \c pathList.
     */
    std::map<HookKey, HookCmd>::iterator FindItem(HookType t, const CTSVNPathList& pathList);

    /**
     * Parses the hook information from a project property string
     */
    bool                                 ParseAndInsertProjectProperty(HookType t, const CString& strhooks, const CTSVNPath& wcPath,
                                                                       const CString& rootPath, const CString& rootUrl,
                                                                       const CString& repoRootUrl);

    /**
     * Checks whether the hook script has been validated already and
     * if not, asks the user to validate it.
     */
    static bool                          ApproveHook(HWND hWnd, std::map<HookKey, HookCmd>::iterator it, DWORD& exitcode);
    static void                          handleHookApproveFailure(DWORD& exitCode, CString& error);

    static CHooks*                       m_pInstance;
    ULONGLONG                            m_lastPreConnectTicks;
    bool                                 m_pathsConvertedToUrls;
    CTSVNPath                            m_wcRootPath;
};
