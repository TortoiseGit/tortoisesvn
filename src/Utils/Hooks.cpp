// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2018, 2021-2022 - TortoiseSVN
// Copyright (C) 2019 - TortoiseGit

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
#include "stdafx.h"
#include "Hooks.h"
#include "resource.h"
#include "registry.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "SVN.h"
#include "SVNHelpers.h"
#include "SmartHandle.h"

CHooks* CHooks::m_pInstance = nullptr;

CHooks::CHooks()
    : m_lastPreConnectTicks(0)
    , m_pathsConvertedToUrls(false)
{
}

CHooks::~CHooks(){};

bool CHooks::Create()
{
    if (m_pInstance == nullptr)
        m_pInstance = new CHooks();
    CRegString regHooks = CRegString(L"Software\\TortoiseSVN\\hooks");
    CString    strHooks = regHooks;
    // now fill the map with all the hooks defined in the string
    // the string consists of multiple lines, where one hook script is defined
    // as six lines:
    // line 1: the hook type
    // line 2: path to working copy where to apply the hook script
    // line 3: command line to execute
    // line 4: 'true' or 'false' for waiting for the script to finish
    // line 5: 'show' or 'hide' on how to start the hook script
    // line 6: 'enforce' on whether to ask the user for permission (optional)
    HookKey    key{};
    int        pos = 0;
    HookCmd    cmd{};
    while ((pos = strHooks.Find('\n')) >= 0)
    {
        // line 1
        key.hType = GetHookType(strHooks.Mid(0, pos));
        if (pos + 1 < strHooks.GetLength())
            strHooks = strHooks.Mid(pos + 1);
        else
            strHooks.Empty();
        bool bComplete = false;
        if ((pos = strHooks.Find('\n')) >= 0)
        {
            // line 2
            key.path = CTSVNPath(strHooks.Mid(0, pos));
            if (pos + 1 < strHooks.GetLength())
                strHooks = strHooks.Mid(pos + 1);
            else
                strHooks.Empty();
            if ((pos = strHooks.Find('\n')) >= 0)
            {
                // line 3
                cmd.commandline = strHooks.Mid(0, pos);
                if (pos + 1 < strHooks.GetLength())
                    strHooks = strHooks.Mid(pos + 1);
                else
                    strHooks.Empty();

                if ((pos = strHooks.Find('\n')) >= 0)
                {
                    // line 4
                    cmd.bWait = (strHooks.Mid(0, pos).CompareNoCase(L"true") == 0);
                    if (pos + 1 < strHooks.GetLength())
                        strHooks = strHooks.Mid(pos + 1);
                    else
                        strHooks.Empty();
                    if ((pos = strHooks.Find('\n')) >= 0)
                    {
                        // line 5
                        cmd.bShow = (strHooks.Mid(0, pos).CompareNoCase(L"show") == 0);
                        if (pos + 1 < strHooks.GetLength())
                            strHooks = strHooks.Mid(pos + 1);
                        else
                            strHooks.Empty();

                        cmd.bEnforce = false;
                        if ((pos = strHooks.Find('\n')) >= 0)
                        {
                            // line 6 (optional)
                            if (GetHookType(strHooks.Mid(0, pos)) == Unknown_Hook)
                            {
                                cmd.bEnforce = (strHooks.Mid(0, pos).CompareNoCase(L"enforce") == 0);
                                if (pos + 1 < strHooks.GetLength())
                                    strHooks = strHooks.Mid(pos + 1);
                                else
                                    strHooks.Empty();
                            }
                        }
                        cmd.bApproved = true; // user configured scripts are pre-approved
                        bComplete     = true;
                    }
                }
            }
        }
        if (bComplete)
        {
            m_pInstance->insert(std::pair<HookKey, HookCmd>(key, cmd));
        }
    }
    return true;
}

void CHooks::SetProjectProperties(const CTSVNPath& wcRootPath, const ProjectProperties& pp)
{
    m_wcRootPath   = wcRootPath;
    auto propsPath = pp.GetPropsPath().GetWinPathString();
    ParseAndInsertProjectProperty(Check_Commit_Hook, pp.sCheckCommitHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Pre_Commit_Hook, pp.sPreCommitHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Start_Commit_Hook, pp.sStartCommitHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Post_Commit_Hook, pp.sPostCommitHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Pre_Update_Hook, pp.sPreUpdateHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Start_Update_Hook, pp.sStartUpdateHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Post_Update_Hook, pp.sPostUpdateHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Manual_Precommit, pp.sManualPreCommitHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Pre_Lock_Hook, pp.sPreLockHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(Post_Lock_Hook, pp.sPostLockHook, wcRootPath, propsPath, pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
}

CHooks& CHooks::Instance()
{
    return *m_pInstance;
}

void CHooks::Destroy()
{
    delete m_pInstance;
}

bool CHooks::Save()
{
    CString strHooks;
    for (auto it = begin(); it != end(); ++it)
    {
        strHooks += GetHookTypeString(it->first.hType);
        strHooks += '\n';
        strHooks += it->first.path.GetWinPathString();
        strHooks += '\n';
        strHooks += it->second.commandline;
        strHooks += '\n';
        strHooks += (it->second.bWait ? L"true" : L"false");
        strHooks += '\n';
        strHooks += (it->second.bShow ? L"show" : L"hide");
        strHooks += '\n';
        strHooks += (it->second.bEnforce ? L"enforce" : L"ask");
        strHooks += '\n';
    }

    CRegString regHooks(L"Software\\TortoiseSVN\\hooks");
    regHooks = strHooks;
    if (regHooks.GetLastError() != ERROR_SUCCESS)
        return false;

    return true;
}

bool CHooks::Remove(const HookKey& key)
{
    return (erase(key) > 0);
}

void CHooks::Add(HookType ht, const CTSVNPath& path, LPCWSTR szCmd,
                 bool bWait, bool bShow, bool bEnforce)
{
    HookKey key;
    key.hType = ht;
    key.path  = path;
    auto it   = find(key);
    if (it != end())
        erase(it);

    HookCmd cmd;
    cmd.commandline = szCmd;
    cmd.bWait       = bWait;
    cmd.bShow       = bShow;
    cmd.bEnforce    = bEnforce;
    cmd.bApproved   = bEnforce;

    insert(std::pair<HookKey, HookCmd>(key, cmd));
}

CString CHooks::GetHookTypeString(HookType t)
{
    switch (t)
    {
        case Start_Commit_Hook:
            return L"start_commit_hook";
        case Check_Commit_Hook:
            return L"check_commit_hook";
        case Pre_Commit_Hook:
            return L"pre_commit_hook";
        case Post_Commit_Hook:
            return L"post_commit_hook";
        case Start_Update_Hook:
            return L"start_update_hook";
        case Pre_Update_Hook:
            return L"pre_update_hook";
        case Post_Update_Hook:
            return L"post_update_hook";
        case Pre_Connect_Hook:
            return L"pre_connect_hook";
        case Manual_Precommit:
            return L"manual_precommit_hook";
        case Pre_Lock_Hook:
            return L"pre_lock_hook";
        case Post_Lock_Hook:
            return L"post_lock_hook";
        case Unknown_Hook:
            break;
        default:
            break;
    }
    return L"";
}

HookType CHooks::GetHookType(const CString& s)
{
    if (s.Compare(L"start_commit_hook") == 0)
        return Start_Commit_Hook;
    if (s.Compare(L"check_commit_hook") == 0)
        return Check_Commit_Hook;
    if (s.Compare(L"pre_commit_hook") == 0)
        return Pre_Commit_Hook;
    if (s.Compare(L"post_commit_hook") == 0)
        return Post_Commit_Hook;
    if (s.Compare(L"start_update_hook") == 0)
        return Start_Update_Hook;
    if (s.Compare(L"pre_update_hook") == 0)
        return Pre_Update_Hook;
    if (s.Compare(L"post_update_hook") == 0)
        return Post_Update_Hook;
    if (s.Compare(L"pre_connect_hook") == 0)
        return Pre_Connect_Hook;
    if (s.Compare(L"manual_precommit_hook") == 0)
        return Manual_Precommit;
    if (s.Compare(L"pre_lock_hook") == 0)
        return Pre_Lock_Hook;
    if (s.Compare(L"post_lock_hook") == 0)
        return Post_Lock_Hook;
    return Unknown_Hook;
}

void CHooks::AddParam(CString& sCmd, const CString& param)
{
    sCmd += L" \"";
    sCmd += param;
    sCmd += L"\"";
}

void CHooks::AddPathParam(CString& sCmd, const CTSVNPathList& pathList)
{
    CTSVNPath tempPath = CTempFiles::Instance().GetTempFilePath(true);
    pathList.WriteToFile(tempPath.GetWinPathString(), true);
    AddParam(sCmd, tempPath.GetWinPathString());
}

void CHooks::AddCWDParam(CString& sCmd, const CTSVNPathList& pathList)
{
    CTSVNPath curDir = pathList.GetCommonRoot().GetDirectory();
    while (!curDir.IsEmpty() && !curDir.Exists())
        curDir = curDir.GetContainingDirectory();

    AddParam(sCmd, curDir.GetWinPathString());
}

void CHooks::AddDepthParam(CString& sCmd, svn_depth_t depth)
{
    CString sTemp;
    sTemp.Format(L"%d", depth);
    AddParam(sCmd, sTemp);
}

void CHooks::AddErrorParam(CString& sCmd, const CString& error) const
{
    CTSVNPath tempPath = CTempFiles::Instance().GetTempFilePath(true);
    CStringUtils::WriteStringToTextFile(tempPath.GetWinPath(), static_cast<LPCWSTR>(error));
    AddParam(sCmd, tempPath.GetWinPathString());
}

CTSVNPath CHooks::AddMessageFileParam(CString& sCmd, const CString& message) const
{
    CTSVNPath tempPath = CTempFiles::Instance().GetTempFilePath(true);
    CStringUtils::WriteStringToTextFile(tempPath.GetWinPath(), static_cast<LPCWSTR>(message));
    AddParam(sCmd, tempPath.GetWinPathString());
    return tempPath;
}

bool CHooks::StartCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message, DWORD& exitCode, CString& error)
{
    exitCode = 0;
    auto it  = FindItem(Start_Commit_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    CTSVNPath tempPath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitCode && !tempPath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(tempPath.GetWinPathString(), message);
    }
    return true;
}

bool CHooks::CheckCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message, DWORD& exitCode, CString& error)
{
    exitCode = 0;
    auto it  = FindItem(Check_Commit_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    CTSVNPath tempPath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitCode && !tempPath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(tempPath.GetWinPathString(), message);
    }
    return true;
}

bool CHooks::PreCommit(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth, CString& message, DWORD& exitCode, CString& error)
{
    auto it = FindItem(Pre_Commit_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddDepthParam(sCmd, depth);
    CTSVNPath tempPath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitCode && !tempPath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(tempPath.GetWinPathString(), message);
    }
    return true;
}

bool CHooks::ManualPreCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message, DWORD& exitCode, CString& error)
{
    auto it = FindItem(Manual_Precommit, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    CTSVNPath temppath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitCode && !temppath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(temppath.GetWinPathString(), message);
    }
    return true;
}

bool CHooks::PostCommit(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth, const SVNRev& rev, const CString& message, DWORD& exitCode, CString& error)
{
    auto it = FindItem(Post_Commit_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddDepthParam(sCmd, depth);
    AddMessageFileParam(sCmd, message);
    AddParam(sCmd, rev.ToString());
    AddErrorParam(sCmd, error);
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::StartUpdate(HWND hWnd, const CTSVNPathList& pathList, DWORD& exitCode, CString& error)
{
    auto it = FindItem(Start_Update_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::PreUpdate(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth, const SVNRev& rev, DWORD& exitCode, CString& error)
{
    auto it = FindItem(Pre_Update_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddDepthParam(sCmd, depth);
    AddParam(sCmd, rev.ToString());
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::PostUpdate(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth, const SVNRev& rev, const CTSVNPathList& updatedList, DWORD& exitCode, CString& error)
{
    auto it = FindItem(Post_Update_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddDepthParam(sCmd, depth);
    AddParam(sCmd, rev.ToString());
    AddErrorParam(sCmd, error);
    AddCWDParam(sCmd, pathList);
    AddPathParam(sCmd, updatedList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::PreConnect(const CTSVNPathList& pathList)
{
    if ((m_lastPreConnectTicks == 0) || ((GetTickCount64() - m_lastPreConnectTicks) > 5 * 60 * 1000))
    {
        auto it = FindItem(Pre_Connect_Hook, pathList);
        if (it == end())
        {
            if (!m_pathsConvertedToUrls && pathList.GetCount() && pathList[0].IsUrl())
            {
                SVN svn;
                for (auto ith = begin(); ith != end(); ++ith)
                {
                    if (ith->first.hType == Pre_Connect_Hook)
                    {
                        CString sUrl = svn.GetURLFromPath(ith->first.path);
                        HookKey hk;
                        hk.hType = Pre_Connect_Hook;
                        hk.path  = CTSVNPath(sUrl);
                        insert(std::pair<HookKey, HookCmd>(hk, ith->second));
                    }
                }
                m_pathsConvertedToUrls = true;
                it                     = FindItem(Pre_Connect_Hook, pathList);
                if (it == end())
                    return false;
            }
            else
                return false;
        }
        CString sCmd = it->second.commandline;
        CString error;
        RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
        m_lastPreConnectTicks = GetTickCount64();
        return true;
    }
    return false;
}

bool CHooks::PreLock(HWND hWnd, const CTSVNPathList& pathList, bool lock, bool steal, CString& message, DWORD& exitCode, CString& error)
{
    auto it = FindItem(Pre_Lock_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddParam(sCmd, lock ? L"true" : L"false");
    AddParam(sCmd, steal ? L"true" : L"false");
    CTSVNPath tempPath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitCode && !tempPath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(tempPath.GetWinPathString(), message);
    }
    return true;
}

bool CHooks::PostLock(HWND hWnd, const CTSVNPathList& pathList, bool lock, bool steal, const CString& message, DWORD& exitCode, CString& error)
{
    auto it = FindItem(Post_Lock_Hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it, exitCode))
    {
        handleHookApproveFailure(exitCode, error);
        return true;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddParam(sCmd, lock ? L"true" : L"false");
    AddParam(sCmd, steal ? L"true" : L"false");
    AddMessageFileParam(sCmd, message);
    AddErrorParam(sCmd, error);
    AddCWDParam(sCmd, pathList);
    exitCode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::IsHookExecutionEnforced(HookType t, const CTSVNPathList& pathList)
{
    auto it = FindItem(t, pathList);
    return it != end() && it->second.bEnforce;
}

bool CHooks::IsHookPresent(HookType t, const CTSVNPathList& pathList)
{
    auto it = FindItem(t, pathList);
    return it != end();
}

std::map<HookKey, HookCmd>::iterator CHooks::FindItem(HookType t, const CTSVNPathList& pathList)
{
    HookKey key;
    for (int i = 0; i < pathList.GetCount(); ++i)
    {
        CTSVNPath path = pathList[i];
        do
        {
            key.hType = t;
            key.path  = path;
            auto it   = find(key);
            if (it != end())
            {
                return it;
            }
            path = path.GetContainingDirectory();
        } while (!path.IsEmpty());
    }

    // try the wc root path
    key.hType = t;
    key.path  = m_wcRootPath;
    auto it   = find(key);
    if (it != end())
    {
        return it;
    }

    // look for a script with a path as '*'
    key.hType = t;
    key.path  = CTSVNPath(L"*");
    it        = find(key);
    if (it != end())
    {
        return it;
    }

    return end();
}

DWORD CHooks::RunScript(CString cmd, const CTSVNPathList& paths, CString& error, bool bWait, bool bShow)
{
    DWORD               exitCode = 0;
    SECURITY_ATTRIBUTES sa       = {0};
    sa.nLength                   = sizeof(sa);
    sa.bInheritHandle            = TRUE;

    CTSVNPath curDir             = paths.GetCommonRoot().GetDirectory();
    while (!curDir.IsEmpty() && !curDir.Exists())
        curDir = curDir.GetContainingDirectory();
    if (curDir.IsEmpty())
    {
        WCHAR buf[MAX_PATH] = {0};
        GetTempPath(MAX_PATH, buf);
        curDir.SetFromWin(buf, true);
    }

    CAutoFile hOut;
    CAutoFile hRedir;
    CAutoFile hErr;

    // clear the error string
    error.Empty();

    // Create Temp File for redirection
    wchar_t szTempPath[MAX_PATH] = {0};
    wchar_t szOutput[MAX_PATH]   = {0};
    wchar_t szErr[MAX_PATH]      = {0};
    GetTempPath(_countof(szTempPath), szTempPath);
    GetTempFileName(szTempPath, L"svn", 0, szErr);

    // setup redirection handles
    // output handle must be WRITE mode, share READ
    // redirect handle must be READ mode, share WRITE
    hErr = CreateFile(szErr, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);

    if (!hErr)
    {
        error = static_cast<LPCWSTR>(CFormatMessageWrapper());
        return static_cast<DWORD>(-1);
    }

    hRedir = CreateFile(szErr, GENERIC_READ, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

    if (!hRedir)
    {
        error = static_cast<LPCWSTR>(CFormatMessageWrapper());
        return static_cast<DWORD>(-1);
    }

    GetTempFileName(szTempPath, L"svn", 0, szOutput);
    hOut = CreateFile(szOutput, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);

    if (!hOut)
    {
        error = static_cast<LPCWSTR>(CFormatMessageWrapper());
        return static_cast<DWORD>(-1);
    }

    // setup startup info, set std out/err handles
    // hide window
    STARTUPINFO si         = {0};
    si.cb                  = sizeof(si);
    si.dwFlags             = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput          = hOut;
    si.hStdError           = hErr;
    si.wShowWindow         = bShow ? SW_SHOW : SW_HIDE;

    PROCESS_INFORMATION pi = {nullptr};
    if (!CreateProcess(nullptr, cmd.GetBuffer(), nullptr, nullptr, TRUE, 0, nullptr, curDir.IsEmpty() ? nullptr : curDir.GetWinPath(), &si, &pi))
    {
        const DWORD err = GetLastError(); // preserve the CreateProcess error
        error           = static_cast<LPCWSTR>(CFormatMessageWrapper(err));
        SetLastError(err);
        cmd.ReleaseBuffer();
        return static_cast<DWORD>(-1);
    }
    cmd.ReleaseBuffer();

    CloseHandle(pi.hThread);

    // wait for process to finish, capture redirection and
    // send it to the parent window/console
    if (bWait)
    {
        DWORD dw = 0;
        char  buf[10 * 1024]{};
        do
        {
            SecureZeroMemory(&buf, sizeof(buf));
            while (ReadFile(hRedir, &buf, sizeof(buf) - 1, &dw, nullptr))
            {
                if (dw == 0)
                    break;
                error += CString(CStringA(buf, dw));
                SecureZeroMemory(&buf, sizeof(buf));
            }
        } while (WaitForSingleObject(pi.hProcess, 100) != WAIT_OBJECT_0);

        // perform any final flushing
        while (ReadFile(hRedir, &buf, sizeof(buf) - 1, &dw, nullptr))
        {
            if (dw == 0)
                break;

            error += CString(CStringA(buf, dw));
            SecureZeroMemory(&buf, sizeof(buf));
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);
    }
    CloseHandle(pi.hProcess);
    DeleteFile(szOutput);
    DeleteFile(szErr);

    return exitCode;
}

bool CHooks::ParseAndInsertProjectProperty(HookType t, const CString& strhook, const CTSVNPath& wcRootPath, const CString& rootPath, const CString& rootUrl, const CString& repoRootUrl)
{
    if (strhook.IsEmpty())
        return false;
    // the string consists of multiple lines, where one hook script is defined
    // as four lines:
    // line 1: command line to execute
    // line 2: 'true' or 'false' for waiting for the script to finish
    // line 3: 'show' or 'hide' on how to start the hook script
    // line 4: 'force' on whether to ask the user for permission
    HookKey key{};
    HookCmd cmd{};

    key.hType   = t;
    key.path    = wcRootPath;

    int     pos = 0;
    CString temp;

    temp = strhook.Tokenize(L"\n", pos);
    if (!temp.IsEmpty())
    {
        ASSERT(t == GetHookType(temp));
        temp = strhook.Tokenize(L"\n", pos);
        if (!temp.IsEmpty())
        {
            int urlStart = temp.Find(L"%REPOROOT%");
            if (urlStart >= 0)
            {
                temp.Replace(L"%REPOROOT%", repoRootUrl);
                CString fullUrl = temp.Mid(urlStart);
                int     urlEnd  = -1;
                if ((urlStart > 0) && (temp[urlStart - 1] == '\"'))
                    urlEnd = temp.Find('\"', urlStart);
                else
                    urlEnd = temp.Find(' ', urlStart);
                if (urlEnd < 0)
                    urlEnd = temp.GetLength();
                fullUrl = temp.Mid(urlStart, urlEnd - urlStart);
                fullUrl.Replace('\\', '/');
                // now we have the full url of the script, e.g.
                // https://svn.osdn.net/svnroot/tortoisesvn/trunk/contrib/hook-scripts/client-side/checkyear.js

                CString sLocalPathUrl = rootUrl;
                CString sLocalPath    = rootPath;
                // find the lowest common ancestor of the local path url and the script url
                int     stripCount    = 0;
                while (fullUrl.Left(sLocalPathUrl.GetLength()).Compare(sLocalPathUrl))
                {
                    int sp = sLocalPathUrl.ReverseFind('/');
                    if (sp < 0)
                        return false;
                    sLocalPathUrl = sLocalPathUrl.Left(sp);
                    ++stripCount;
                    do
                    {
                        sp = sLocalPath.ReverseFind('\\');
                        if (sp < 0)
                        {
                            sLocalPath = rootPath;
                            for (int j = 0; j < stripCount; ++j)
                                sLocalPath = sLocalPath.Left(sLocalPath.ReverseFind('\\'));
                            break;
                        }
                        sLocalPath = sLocalPath.Left(sp);
                    } while (fullUrl.Left(sLocalPathUrl.GetLength()).Compare(sLocalPathUrl));
                }
                // now both sLocalPathUrl and sLocalPath can be used to construct
                // the path to the script
                CString partUrl = fullUrl.Mid(sLocalPathUrl.GetLength());
                if (partUrl.Find('/') == 0)
                    partUrl = partUrl.Mid(1);
                if (sLocalPath.ReverseFind('\\') == sLocalPath.GetLength() - 1 || sLocalPath.ReverseFind('/') == sLocalPath.GetLength() - 1)
                    sLocalPath = sLocalPath.Left(sLocalPath.GetLength() - 1);
                sLocalPath = sLocalPath + L"\\" + partUrl;
                sLocalPath.Replace('/', '\\');
                // now replace the full url in the command line with the local path
                temp.Replace(fullUrl, sLocalPath);
            }
            urlStart = temp.Find(L"%REPOROOT+%");
            if (urlStart >= 0)
            {
                CString temp2 = temp;
                CString sExt  = rootUrl.Mid(repoRootUrl.GetLength());
                CString sLocalPath;
                do
                {
                    temp                   = temp2;
                    CString repoRootUrlExt = repoRootUrl + sExt;
                    int     slp            = sExt.ReverseFind('/');
                    if (slp >= 0)
                        sExt = sExt.Left(sExt.ReverseFind('/'));
                    else if (sExt.IsEmpty())
                        return false;
                    else
                        sExt.Empty();
                    temp.Replace(L"%REPOROOT+%", repoRootUrlExt);
                    CString fullUrl = temp.Mid(urlStart);
                    int     urlend  = -1;
                    if ((urlStart > 0) && (temp[urlStart - 1] == '\"'))
                        urlend = temp.Find('\"', urlStart);
                    else
                        urlend = temp.Find(' ', urlStart);
                    if (urlend < 0)
                        urlend = temp.GetLength();
                    fullUrl = temp.Mid(urlStart, urlend - urlStart);
                    fullUrl.Replace('\\', '/');
                    // now we have the full url of the script, e.g.
                    // https://svn.osdn.net/svnroot/tortoisesvn/trunk/contrib/hook-scripts/client-side/checkyear.js

                    CString sLocalPathUrl = rootUrl;
                    sLocalPath            = rootPath;
                    // find the lowest common ancestor of the local path url and the script url
                    while (fullUrl.Left(sLocalPathUrl.GetLength()).Compare(sLocalPathUrl))
                    {
                        int sp = sLocalPathUrl.ReverseFind('/');
                        if (sp < 0)
                            return false;
                        sLocalPathUrl = sLocalPathUrl.Left(sp);

                        sp            = sLocalPath.ReverseFind('\\');
                        if (sp < 0)
                            return false;
                        sLocalPath = sLocalPath.Left(sp);
                    }
                    // now both sLocalPathUrl and sLocalPath can be used to construct
                    // the path to the script
                    CString partUrl = fullUrl.Mid(sLocalPathUrl.GetLength());
                    if (partUrl.Find('/') == 0)
                        partUrl = partUrl.Mid(1);
                    if (sLocalPath.ReverseFind('\\') == sLocalPath.GetLength() - 1 || sLocalPath.ReverseFind('/') == sLocalPath.GetLength() - 1)
                        sLocalPath = sLocalPath.Left(sLocalPath.GetLength() - 1);
                    sLocalPath = sLocalPath + L"\\" + partUrl;
                    sLocalPath.Replace('/', '\\');
                    // now replace the full url in the command line with the local path
                    temp.Replace(fullUrl, sLocalPath);
                } while (!PathFileExists(sLocalPath));
            }
            cmd.commandline = temp;
            temp            = strhook.Tokenize(L"\n", pos);
            if (!temp.IsEmpty())
            {
                cmd.bWait = (temp.CompareNoCase(L"true") == 0);
                temp      = strhook.Tokenize(L"\n", pos);
                if (!temp.IsEmpty())
                {
                    cmd.bShow = (temp.CompareNoCase(L"show") == 0);

                    temp.Format(L"%d%s", static_cast<int>(key.hType), static_cast<LPCWSTR>(cmd.commandline));
                    SVNPool pool;
                    cmd.sRegKey = L"Software\\TortoiseSVN\\approvedhooks\\" + SVN::GetChecksumString(svn_checksum_sha1, temp, pool);
                    CRegDWORD reg(cmd.sRegKey, 0);
                    cmd.bApproved = (static_cast<DWORD>(reg) != 0);
                    cmd.bStored   = reg.exists();

                    temp          = strhook.Tokenize(L"\n", pos);
                    cmd.bEnforce  = temp.CompareNoCase(L"enforce") == 0;

                    if (find(key) == end())
                    {
                        m_pInstance->insert(std::pair<HookKey, HookCmd>(key, cmd));
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool CHooks::ApproveHook(HWND hWnd, std::map<HookKey, HookCmd>::iterator it, DWORD& exitCode)
{
    if (it->second.bApproved || it->second.bStored)
    {
        exitCode = 0;
        return it->second.bApproved;
    }

    CString sQuestion;
    sQuestion.Format(IDS_HOOKS_APPROVE_TASK1, static_cast<LPCWSTR>(it->second.commandline));
    CTaskDialog taskDlg(sQuestion,
                        CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK2)),
                        L"TortoiseSVN",
                        0,
                        TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    taskDlg.AddCommandControl(101, CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK3)));
    taskDlg.AddCommandControl(102, CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK4)));
    taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskDlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK5)));
    taskDlg.SetVerificationCheckbox(false);
    taskDlg.SetDefaultCommandControl(102);
    taskDlg.SetMainIcon(TD_WARNING_ICON);
    auto ret = taskDlg.DoModal(hWnd);
    if (ret == IDCANCEL)
    {
        exitCode = 1;
        return false;
    }
    bool bApproved      = (ret == 101);
    bool bDoNotAskAgain = !!taskDlg.GetVerificationCheckboxState();

    if (bDoNotAskAgain)
    {
        CRegDWORD reg(it->second.sRegKey, 0);
        reg                  = bApproved ? 1 : 0;
        it->second.bStored   = true;
        it->second.bApproved = bApproved;
    }
    exitCode = 0;
    return bApproved;
}

void CHooks::handleHookApproveFailure(DWORD& exitCode, CString& error)
{
    if (exitCode == 1)
    {
        if (static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\HookCancelError", FALSE)) != 0)
            error.LoadString(IDS_SVN_USERCANCELLED);
        else
        {
            error.Empty();
            exitCode = 0;
        }
    }
    else
        error.LoadString(IDS_ERR_HOOKNOTAPPROVED);
}
