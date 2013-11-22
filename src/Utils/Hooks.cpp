// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2013 - TortoiseSVN

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
#include "MessageBox.h"

CHooks* CHooks::m_pInstance = NULL;

CHooks::CHooks()
    : m_lastPreConnectTicks(0)
    , m_PathsConvertedToUrls(false)
{
}

CHooks::~CHooks()
{
};

bool CHooks::Create()
{
    if (m_pInstance == NULL)
        m_pInstance = new CHooks();
    CRegString reghooks = CRegString(_T("Software\\TortoiseSVN\\hooks"));
    CString strhooks = reghooks;
    // now fill the map with all the hooks defined in the string
    // the string consists of multiple lines, where one hook script is defined
    // as six lines:
    // line 1: the hook type
    // line 2: path to working copy where to apply the hook script
    // line 3: command line to execute
    // line 4: 'true' or 'false' for waiting for the script to finish
    // line 5: 'show' or 'hide' on how to start the hook script
    // line 6: 'enforce' on whether to ask the user for permission (optional)
    hookkey key;
    int pos = 0;
    hookcmd cmd;
    while ((pos = strhooks.Find('\n')) >= 0)
    {
        // line 1
        key.htype = GetHookType(strhooks.Mid(0, pos));
        if (pos+1 < strhooks.GetLength())
            strhooks = strhooks.Mid(pos+1);
        else
            strhooks.Empty();
        bool bComplete = false;
        if ((pos = strhooks.Find('\n')) >= 0)
        {
            // line 2
            key.path = CTSVNPath(strhooks.Mid(0, pos));
            if (pos+1 < strhooks.GetLength())
                strhooks = strhooks.Mid(pos+1);
            else
                strhooks.Empty();
            if ((pos = strhooks.Find('\n')) >= 0)
            {
                // line 3
                cmd.commandline = strhooks.Mid(0, pos);
                if (pos+1 < strhooks.GetLength())
                    strhooks = strhooks.Mid(pos+1);
                else
                    strhooks.Empty();

                if ((pos = strhooks.Find('\n')) >= 0)
                {
                    // line 4
                    cmd.bWait = (strhooks.Mid(0, pos).CompareNoCase(_T("true"))==0);
                    if (pos+1 < strhooks.GetLength())
                        strhooks = strhooks.Mid(pos+1);
                    else
                        strhooks.Empty();
                    if ((pos = strhooks.Find('\n')) >= 0)
                    {
                        // line 5
                        cmd.bShow = (strhooks.Mid(0, pos).CompareNoCase(_T("show"))==0);
                        if (pos+1 < strhooks.GetLength())
                            strhooks = strhooks.Mid(pos+1);
                        else
                            strhooks.Empty();

                        cmd.bEnforce = false;
                        if ((pos = strhooks.Find('\n')) >= 0)
                        {
                            // line 6 (optional)
                            if (GetHookType(strhooks.Mid(0, pos)) == unknown_hook)
                            {
                                cmd.bEnforce = (strhooks.Mid(0, pos).CompareNoCase(_T("enforce"))==0);
                                if (pos+1 < strhooks.GetLength())
                                    strhooks = strhooks.Mid(pos+1);
                                else
                                    strhooks.Empty();
                            }
                        }
                        cmd.bApproved = true;   // user configured scripts are pre-approved
                        bComplete = true;
                    }
                }
            }
        }
        if (bComplete)
        {
            m_pInstance->insert(std::pair<hookkey, hookcmd>(key, cmd));
        }
    }
    return true;
}

void CHooks::SetProjectProperties( const CTSVNPath& wcRootPath, const ProjectProperties& pp )
{
    m_wcRootPath = wcRootPath;
    CString sLocalPath = pp.sRepositoryRootUrl;
    ParseAndInsertProjectProperty(check_commit_hook, pp.sCheckCommitHook, wcRootPath, pp.GetPropsPath().GetWinPathString(), pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(pre_commit_hook, pp.sPreCommitHook, wcRootPath, pp.GetPropsPath().GetWinPathString(), pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(start_commit_hook, pp.sStartCommitHook, wcRootPath, pp.GetPropsPath().GetWinPathString(), pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(post_commit_hook, pp.sPostCommitHook, wcRootPath, pp.GetPropsPath().GetWinPathString(), pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(pre_update_hook, pp.sPreUpdateHook, wcRootPath, pp.GetPropsPath().GetWinPathString(), pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(start_update_hook, pp.sStartUpdateHook, wcRootPath, pp.GetPropsPath().GetWinPathString(), pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(post_update_hook, pp.sPostUpdateHook, wcRootPath, pp.GetPropsPath().GetWinPathString(), pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
    ParseAndInsertProjectProperty(manual_precommit, pp.sManualPreCommitHook, wcRootPath, pp.GetPropsPath().GetWinPathString(), pp.sRepositoryPathUrl, pp.sRepositoryRootUrl);
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
    CString strhooks;
    for (hookiterator it = begin(); it != end(); ++it)
    {
        strhooks += GetHookTypeString(it->first.htype);
        strhooks += '\n';
        strhooks += it->first.path.GetWinPathString();
        strhooks += '\n';
        strhooks += it->second.commandline;
        strhooks += '\n';
        strhooks += (it->second.bWait ? L"true" : L"false");
        strhooks += '\n';
        strhooks += (it->second.bShow ? L"show" : L"hide");
        strhooks += '\n';
        strhooks += (it->second.bEnforce ? L"enforce" : L"ask");
        strhooks += '\n';
    }

    CRegString reghooks = CRegString(_T("Software\\TortoiseSVN\\hooks"));
    reghooks = strhooks;
    if (reghooks.GetLastError() != ERROR_SUCCESS)
        return false;

    return true;
}

bool CHooks::Remove(hookkey key)
{
    return (erase(key) > 0);
}

void CHooks::Add(hooktype ht, const CTSVNPath& Path, LPCTSTR szCmd,
                 bool bWait, bool bShow, bool bEnforce)
{
    hookkey key;
    key.htype = ht;
    key.path = Path;
    hookiterator it = find(key);
    if (it!=end())
        erase(it);

    hookcmd cmd;
    cmd.commandline = szCmd;
    cmd.bWait = bWait;
    cmd.bShow = bShow;
    cmd.bEnforce = bEnforce;
    cmd.bApproved = bEnforce;

    insert(std::pair<hookkey, hookcmd>(key, cmd));
}

CString CHooks::GetHookTypeString(hooktype t)
{
    switch (t)
    {
    case start_commit_hook:
        return _T("start_commit_hook");
    case check_commit_hook:
        return _T("check_commit_hook");
    case pre_commit_hook:
        return _T("pre_commit_hook");
    case post_commit_hook:
        return _T("post_commit_hook");
    case start_update_hook:
        return _T("start_update_hook");
    case pre_update_hook:
        return _T("pre_update_hook");
    case post_update_hook:
        return _T("post_update_hook");
    case pre_connect_hook:
        return _T("pre_connect_hook");
    case manual_precommit:
        return _T("manual_precommit_hook");
    }
    return _T("");
}

hooktype CHooks::GetHookType(const CString& s)
{
    if (s.Compare(_T("start_commit_hook"))==0)
        return start_commit_hook;
    if (s.Compare(_T("check_commit_hook"))==0)
        return check_commit_hook;
    if (s.Compare(_T("pre_commit_hook"))==0)
        return pre_commit_hook;
    if (s.Compare(_T("post_commit_hook"))==0)
        return post_commit_hook;
    if (s.Compare(_T("start_update_hook"))==0)
        return start_update_hook;
    if (s.Compare(_T("pre_update_hook"))==0)
        return pre_update_hook;
    if (s.Compare(_T("post_update_hook"))==0)
        return post_update_hook;
    if (s.Compare(_T("pre_connect_hook"))==0)
        return pre_connect_hook;
    if (s.Compare(_T("manual_precommit_hook"))==0)
        return manual_precommit;
    return unknown_hook;
}

void CHooks::AddParam(CString& sCmd, const CString& param)
{
    sCmd += _T(" \"");
    sCmd += param;
    sCmd += _T("\"");
}

void CHooks::AddPathParam(CString& sCmd, const CTSVNPathList& pathList)
{
    CTSVNPath temppath = CTempFiles::Instance().GetTempFilePath(true);
    pathList.WriteToFile(temppath.GetWinPathString(), true);
    AddParam(sCmd, temppath.GetWinPathString());
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
    sTemp.Format(_T("%d"), depth);
    AddParam(sCmd, sTemp);
}

void CHooks::AddErrorParam(CString& sCmd, const CString& error)
{
    CTSVNPath tempPath;
    tempPath = CTempFiles::Instance().GetTempFilePath(true);
    CStringUtils::WriteStringToTextFile(tempPath.GetWinPath(), (LPCTSTR)error);
    AddParam(sCmd, tempPath.GetWinPathString());
}

CTSVNPath CHooks::AddMessageFileParam(CString& sCmd, const CString& message)
{
    CTSVNPath tempPath;
    tempPath = CTempFiles::Instance().GetTempFilePath(true);
    CStringUtils::WriteStringToTextFile(tempPath.GetWinPath(), (LPCTSTR)message);
    AddParam(sCmd, tempPath.GetWinPathString());
    return tempPath;
}

bool CHooks::StartCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message, DWORD& exitcode, CString& error)
{
    exitcode = 0;
    hookiterator it = FindItem(start_commit_hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it))
    {
        exitcode = 1;
        error.LoadString(IDS_ERR_HOOKNOTAPPROVED);
        return false;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    CTSVNPath temppath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitcode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitcode && !temppath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(temppath.GetWinPathString(), message);
    }
    return true;
}

bool CHooks::CheckCommit(HWND hWnd, const CTSVNPathList& pathList, CString& message, DWORD& exitcode, CString& error)
{
    exitcode = 0;
    hookiterator it = FindItem(check_commit_hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it))
    {
        exitcode = 1;
        error.LoadString(IDS_ERR_HOOKNOTAPPROVED);
        return false;
    }
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    CTSVNPath temppath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitcode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitcode && !temppath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(temppath.GetWinPathString(), message);
    }
    return true;
}

bool CHooks::PreCommit(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth, CString& message, DWORD& exitcode, CString& error)
{
    hookiterator it = FindItem(pre_commit_hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it))
        return false;
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddDepthParam(sCmd, depth);
    CTSVNPath temppath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitcode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitcode && !temppath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(temppath.GetWinPathString(), message);
    }
    return true;
}

bool CHooks::ManualPreCommit( HWND hWnd, const CTSVNPathList& pathList, CString& message, DWORD& exitcode, CString& error )
{
    hookiterator it = FindItem(manual_precommit, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it))
        return false;
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    CTSVNPath temppath = AddMessageFileParam(sCmd, message);
    AddCWDParam(sCmd, pathList);
    exitcode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    if (!exitcode && !temppath.IsEmpty())
    {
        CStringUtils::ReadStringFromTextFile(temppath.GetWinPathString(), message);
    }
    return true;
}


bool CHooks::PostCommit(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth, SVNRev rev, const CString& message, DWORD& exitcode, CString& error)
{
    hookiterator it = FindItem(post_commit_hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it))
        return false;
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddDepthParam(sCmd, depth);
    AddMessageFileParam(sCmd, message);
    AddParam(sCmd, rev.ToString());
    AddErrorParam(sCmd, error);
    AddCWDParam(sCmd, pathList);
    exitcode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::StartUpdate(HWND hWnd, const CTSVNPathList& pathList, DWORD& exitcode, CString& error)
{
    hookiterator it = FindItem(start_update_hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it))
        return false;
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddCWDParam(sCmd, pathList);
    exitcode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::PreUpdate(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth, SVNRev rev, DWORD& exitcode, CString& error)
{
    hookiterator it = FindItem(pre_update_hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it))
        return false;
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddDepthParam(sCmd, depth);
    AddParam(sCmd, rev.ToString());
    AddCWDParam(sCmd, pathList);
    exitcode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::PostUpdate(HWND hWnd, const CTSVNPathList& pathList, svn_depth_t depth, SVNRev rev, const CTSVNPathList& updatedList, DWORD& exitcode, CString& error)
{
    hookiterator it = FindItem(post_update_hook, pathList);
    if (it == end())
        return false;
    if (!ApproveHook(hWnd, it))
        return false;
    CString sCmd = it->second.commandline;
    AddPathParam(sCmd, pathList);
    AddDepthParam(sCmd, depth);
    AddParam(sCmd, rev.ToString());
    AddErrorParam(sCmd, error);
    AddCWDParam(sCmd, pathList);
    AddPathParam(sCmd, updatedList);
    exitcode = RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
    return true;
}

bool CHooks::PreConnect(const CTSVNPathList& pathList)
{
    if ((m_lastPreConnectTicks == 0) || ((GetTickCount() - m_lastPreConnectTicks) > 5*60*1000))
    {
        hookiterator it = FindItem(pre_connect_hook, pathList);
        if (it == end())
        {
            if (!m_PathsConvertedToUrls && pathList.GetCount() && pathList[0].IsUrl())
            {
                SVN svn;
                for (hookiterator ith = begin(); ith != end(); ++ith)
                {
                    if (ith->first.htype == pre_connect_hook)
                    {
                        CString sUrl = svn.GetURLFromPath(ith->first.path);
                        hookkey hk;
                        hk.htype = pre_connect_hook;
                        hk.path = CTSVNPath(sUrl);
                        insert(std::pair<hookkey, hookcmd>(hk, ith->second));
                    }
                }
                m_PathsConvertedToUrls = true;
                it = FindItem(pre_connect_hook, pathList);
                if (it == end())
                    return false;
            }
            else
                return false;
        }
        CString sCmd = it->second.commandline;
        CString error;
        RunScript(sCmd, pathList, error, it->second.bWait, it->second.bShow);
        m_lastPreConnectTicks = GetTickCount();
        return true;
    }
    return false;
}

bool CHooks::IsHookExecutionEnforced(hooktype t, const CTSVNPathList& pathList)
{
    hookiterator it = FindItem(t, pathList);
    return it != end() && it->second.bEnforce;
}

bool CHooks::IsHookPresent( hooktype t, const CTSVNPathList& pathList )
{
    hookiterator it = FindItem(t, pathList);
    return it != end();
}

hookiterator CHooks::FindItem(hooktype t, const CTSVNPathList& pathList)
{
    hookkey key;
    for (int i=0; i<pathList.GetCount(); ++i)
    {
        CTSVNPath path = pathList[i];
        do
        {
            key.htype = t;
            key.path = path;
            hookiterator it = find(key);
            if (it != end())
            {
                return it;
            }
            path = path.GetContainingDirectory();
        } while(!path.IsEmpty());
    }

    // try the wc root path
    key.htype = t;
    key.path = m_wcRootPath;
    hookiterator it = find(key);
    if (it != end())
    {
        return it;
    }

    // look for a script with a path as '*'
    key.htype = t;
    key.path = CTSVNPath(_T("*"));
    it = find(key);
    if (it != end())
    {
        return it;
    }

    return end();
}

DWORD CHooks::RunScript(CString cmd, const CTSVNPathList& paths, CString& error, bool bWait, bool bShow)
{
    DWORD exitcode = 0;
    SECURITY_ATTRIBUTES sa;
    SecureZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    CTSVNPath curDir = paths.GetCommonRoot().GetDirectory();
    while (!curDir.IsEmpty() && !curDir.Exists())
        curDir = curDir.GetContainingDirectory();
    if (curDir.IsEmpty())
    {
        WCHAR buf[MAX_PATH] = {0};
        GetTempPath(MAX_PATH, buf);
        curDir.SetFromWin(buf, true);
    }

    CAutoFile hOut ;
    CAutoFile hRedir;
    CAutoFile hErr;

    // clear the error string
    error.Empty();

    // Create Temp File for redirection
    TCHAR szTempPath[MAX_PATH];
    TCHAR szOutput[MAX_PATH];
    TCHAR szErr[MAX_PATH];
    GetTempPath(_countof(szTempPath),szTempPath);
    GetTempFileName(szTempPath, _T("svn"), 0, szErr);

    // setup redirection handles
    // output handle must be WRITE mode, share READ
    // redirect handle must be READ mode, share WRITE
    hErr   = CreateFile(szErr, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY,    0);

    if (!hErr)
    {
        error = CFormatMessageWrapper();
        return (DWORD)-1;
    }

    hRedir = CreateFile(szErr, GENERIC_READ, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

    if (!hRedir)
    {
        error = CFormatMessageWrapper();
        return (DWORD)-1;
    }

    GetTempFileName(szTempPath, _T("svn"), 0, szOutput);
    hOut   = CreateFile(szOutput, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, 0);

    if (!hOut)
    {
        error = CFormatMessageWrapper();
        return (DWORD)-1;
    }

    // setup startup info, set std out/err handles
    // hide window
    STARTUPINFO si;
    SecureZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput  = hOut;
    si.hStdError   = hErr;
    si.wShowWindow = bShow ? SW_SHOW : SW_HIDE;

    PROCESS_INFORMATION pi;
    SecureZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL, cmd.GetBuffer(), NULL, NULL, TRUE, 0, NULL, curDir.IsEmpty() ? NULL : curDir.GetWinPath(), &si, &pi))
    {
        const DWORD err = GetLastError();  // preserve the CreateProcess error
        error = CFormatMessageWrapper(err);
        SetLastError(err);
        cmd.ReleaseBuffer();
        return (DWORD)-1;
    }
    cmd.ReleaseBuffer();

    CloseHandle(pi.hThread);

    // wait for process to finish, capture redirection and
    // send it to the parent window/console
    if (bWait)
    {
        DWORD dw;
        char buf[10*1024];
        do
        {
            SecureZeroMemory(&buf,sizeof(buf));
            while (ReadFile(hRedir, &buf, sizeof(buf)-1, &dw, NULL))
            {
                if (dw == 0)
                    break;
                error += CString(CStringA(buf,dw));
                SecureZeroMemory(&buf,sizeof(buf));
            }
        } while (WaitForSingleObject(pi.hProcess, 100) != WAIT_OBJECT_0);

        // perform any final flushing
        while (ReadFile(hRedir, &buf, sizeof(buf)-1, &dw, NULL))
        {
            if (dw == 0)
                break;

            error += CString(CStringA(buf, dw));
            SecureZeroMemory(&buf,sizeof(buf));
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitcode);
    }
    CloseHandle(pi.hProcess);
    DeleteFile(szOutput);
    DeleteFile(szErr);

    return exitcode;
}

bool CHooks::ParseAndInsertProjectProperty( hooktype t, const CString& strhook, const CTSVNPath& wcRootPath, const CString& rootPath, const CString& rootUrl, const CString& repoRootUrl )
{
    if (strhook.IsEmpty())
        return false;
    // the string consists of multiple lines, where one hook script is defined
    // as four lines:
    // line 1: command line to execute
    // line 2: 'true' or 'false' for waiting for the script to finish
    // line 3: 'show' or 'hide' on how to start the hook script
    // line 4: 'force' on whether to ask the user for permission
    hookkey key;
    hookcmd cmd;

    key.htype = t;
    key.path = wcRootPath;

    int pos = 0;
    CString temp;

    temp = strhook.Tokenize(_T("\n"), pos);
    if (!temp.IsEmpty())
    {
        ASSERT(t == GetHookType(temp));
        temp = strhook.Tokenize(_T("\n"), pos);
        if (!temp.IsEmpty())
        {
            int urlstart = temp.Find(L"%REPOROOT%");
            if (urlstart >= 0)
            {
                temp.Replace(L"%REPOROOT%", repoRootUrl);
                CString fullUrl = temp.Mid(urlstart);
                int urlend = -1;
                if ((urlstart > 0)&&(temp[urlstart-1]=='\"'))
                    urlend = temp.Find('\"', urlstart);
                else
                    urlend = temp.Find(' ', urlstart);
                if (urlend < 0)
                    urlend = temp.GetLength();
                fullUrl = temp.Mid(urlstart, urlend-urlstart);
                fullUrl.Replace('\\', '/');
                // now we have the full url of the script, e.g.
                // https://tortoisesvn.googlecode.com/svn/trunk/contrib/hook-scripts/client-side/checkyear.js

                CString sLocalPathUrl = rootUrl;
                CString sLocalPath = rootPath;
                // find the lowest common ancestor of the local path url and the script url
                while (fullUrl.Left(sLocalPathUrl.GetLength()).Compare(sLocalPathUrl))
                {
                    int sp = sLocalPathUrl.ReverseFind('/');
                    if (sp < 0)
                        return false;
                    sLocalPathUrl = sLocalPathUrl.Left(sp);

                    sp = sLocalPath.ReverseFind('\\');
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
            }
            urlstart = temp.Find(L"%REPOROOT+%");
            if (urlstart >= 0)
            {
                CString temp2 = temp;
                CString sExt = rootUrl.Mid(repoRootUrl.GetLength());
                CString sLocalPath;
                do
                {
                    temp = temp2;
                    CString repoRootUrlExt = repoRootUrl + sExt;
                    int slp = sExt.ReverseFind('/');
                    if (slp >= 0)
                        sExt = sExt.Left(sExt.ReverseFind('/'));
                    else if (sExt.IsEmpty())
                        return false;
                    else
                        sExt.Empty();
                    temp.Replace(L"%REPOROOT+%", repoRootUrlExt);
                    CString fullUrl = temp.Mid(urlstart);
                    int urlend = -1;
                    if ((urlstart > 0)&&(temp[urlstart-1]=='\"'))
                        urlend = temp.Find('\"', urlstart);
                    else
                        urlend = temp.Find(' ', urlstart);
                    if (urlend < 0)
                        urlend = temp.GetLength();
                    fullUrl = temp.Mid(urlstart, urlend-urlstart);
                    fullUrl.Replace('\\', '/');
                    // now we have the full url of the script, e.g.
                    // https://tortoisesvn.googlecode.com/svn/trunk/contrib/hook-scripts/client-side/checkyear.js

                    CString sLocalPathUrl = rootUrl;
                    sLocalPath = rootPath;
                    // find the lowest common ancestor of the local path url and the script url
                    while (fullUrl.Left(sLocalPathUrl.GetLength()).Compare(sLocalPathUrl))
                    {
                        int sp = sLocalPathUrl.ReverseFind('/');
                        if (sp < 0)
                            return false;
                        sLocalPathUrl = sLocalPathUrl.Left(sp);

                        sp = sLocalPath.ReverseFind('\\');
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
            temp = strhook.Tokenize(_T("\n"), pos);
            if (!temp.IsEmpty())
            {
                cmd.bWait = (temp.CompareNoCase(_T("true"))==0);
                temp = strhook.Tokenize(_T("\n"), pos);
                if (!temp.IsEmpty())
                {
                    cmd.bShow = (temp.CompareNoCase(_T("show"))==0);

                    temp.Format(L"%d%s", (int)key.htype, (LPCTSTR)cmd.commandline);
                    SVNPool pool;
                    cmd.sRegKey = L"Software\\TortoiseSVN\\approvedhooks\\" + SVN::GetChecksumString(svn_checksum_sha1, temp, pool);
                    CRegDWORD reg(cmd.sRegKey, 0);
                    cmd.bApproved = (DWORD(reg) != 0);
                    cmd.bStored = reg.exists();

                    temp = strhook.Tokenize(_T("\n"), pos);
                    cmd.bEnforce = temp.CompareNoCase(_T("enforce"))==0;

                    if (find(key) == end())
                    {
                        m_pInstance->insert(std::pair<hookkey, hookcmd>(key, cmd));
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool CHooks::ApproveHook( HWND hWnd, hookiterator it )
{
    if (it->second.bApproved || it->second.bStored)
        return it->second.bApproved;

    CString sQuestion;
    sQuestion.Format(IDS_HOOKS_APPROVE_TASK1, it->second.commandline);
    bool bApproved = false;
    bool bDoNotAskAgain = false;
    if (CTaskDialog::IsSupported())
    {
        CTaskDialog taskdlg(sQuestion,
                            CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_HOOKS_APPROVE_TASK5)));
        taskdlg.SetVerificationCheckbox(false);
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        bApproved = taskdlg.DoModal(hWnd) == 1;
        bDoNotAskAgain = !!taskdlg.GetVerificationCheckboxState();
    }
    else
    {
        UINT ret = TSVNMessageBox(hWnd, sQuestion, CString(MAKEINTRESOURCE(IDS_APPNAME)), MB_YESNO|MB_ICONWARNING|MB_DONOTASKAGAIN);
        bApproved = (ret & 0xFFFF) == IDYES;
        bDoNotAskAgain = (ret & MB_DONOTASKAGAIN)!=0;
    }

    if (bDoNotAskAgain)
    {
        CRegDWORD reg(it->second.sRegKey, 0);
        reg = bApproved ? 1 : 0;
        it->second.bStored = true;
    }
    it->second.bApproved = bApproved;
    return bApproved;
}

