// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "CleanupCommand.h"

#include "MessageBox.h"
#include "ProgressDlg.h"
#include "SVN.h"
#include "SVNGlobal.h"
#include "SVNAdminDir.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"
#include "CleanupDlg.h"
#include "SVNStatus.h"

bool CleanupCommand::Execute()
{
    bool bCleanup           = !!parser.HasKey(L"cleanup");
    bool bRevert            = !!parser.HasKey(L"revert");
    bool bDelUnversioned    = !!parser.HasKey(L"delunversioned");
    bool bRefreshShell      = !!parser.HasKey(L"refreshshell");

    if (!parser.HasKey(L"noui") && !parser.HasKey(L"nodlg"))
    {
        CCleanupDlg dlg;
        if (dlg.DoModal() != IDOK)
            return false;
        bCleanup            = !!dlg.m_bCleanup;
        bRevert             = !!dlg.m_bRevert;
        bDelUnversioned     = !!dlg.m_bDelUnversioned;
        bRefreshShell       = !!dlg.m_bRefreshShell;
    }

    if (!bCleanup && !bRevert && !bDelUnversioned && !bRefreshShell)
        return false;

    int actionTotal = 0;
    if (bCleanup)
        actionTotal += pathList.GetCount();
    if (bRevert || bDelUnversioned)
        actionTotal++;  // for fetching the status
    if (bRevert)
        actionTotal++;
    if (bDelUnversioned)
        actionTotal++;
    if (bRefreshShell)
        actionTotal += pathList.GetCount();
    int actionCounter = 0;
    CProgressDlg progress;
    progress.SetTitle(IDS_PROC_CLEANUP);
    progress.SetAnimation(IDR_CLEANUPANI);
    progress.FormatNonPathLine(2, IDS_PROC_CLEANUP_INFO1, L"");
    progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO2)));
    progress.SetProgress(actionCounter++, actionTotal);
    if (!parser.HasKey(_T("noprogressui")))
        progress.ShowModeless(GetExplorerHWND());

    CString strFailedString;
    bool bFailed = false;
    if (bCleanup)
    {
        for (int i=0; i<pathList.GetCount(); ++i)
        {
            SVN svn;
            progress.FormatPathLine(2, IDS_PROC_CLEANUP_INFO1, (LPCTSTR)pathList[i].GetFileOrDirectoryName());
            progress.SetProgress(actionCounter++, actionTotal);
            if (!svn.CleanUp(pathList[i]))
            {
                strFailedString = pathList[i].GetWinPathString();
                strFailedString += L"\n" + svn.GetLastErrorMessage();
                bFailed = true;
                break;
            }
        }
    }
    CTSVNPathList itemsToRevert, unversionedItems;
    if (!bFailed && (bRevert || bDelUnversioned))
    {
        progress.SetProgress(actionCounter++, actionTotal);
        progress.FormatPathLine(2, IDS_PROC_CLEANUP_INFOFETCHSTATUS, pathList.GetCommonRoot().GetWinPath());
        strFailedString = GetUnversionedAndRevertPaths(pathList, unversionedItems, itemsToRevert);
        bFailed = !strFailedString.IsEmpty();
    }
    if (!bFailed && bRevert)
    {
        progress.SetProgress(actionCounter++, actionTotal);
        progress.FormatNonPathLine(2, IDS_PROC_CLEANUP_INFOREVERT, pathList.GetCommonRoot().GetWinPath());
        if (itemsToRevert.GetCount())
        {
            CTSVNPathList revertItems = itemsToRevert;
            if (DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\RevertWithRecycleBin"), TRUE)))
                itemsToRevert.DeleteAllPaths(true, true);
            SVN svn;
            if (!svn.Revert(revertItems, CStringArray(), false))
            {
                // just assume the first path failed...
                strFailedString = pathList[0].GetWinPathString();
                strFailedString += L"\n" + svn.GetLastErrorMessage();
                bFailed = true;
            }
        }
    }
    if (!bFailed && bDelUnversioned)
    {
        progress.SetProgress(actionCounter++, actionTotal);
        progress.FormatPathLine(2, IDS_PROC_CLEANUP_INFODELUNVERSIONED, pathList.GetCommonRoot().GetWinPath());
        progress.SetProgress(actionCounter+=pathList.GetCount(), actionTotal);
        unversionedItems.DeleteAllPaths(true, false);
    }
    if (!bFailed && bRefreshShell)
    {
        // crawl the path downwards and send a change
        // notification for every directory to the shell. This will update the
        // overlays in the left tree view of the explorer.
        for (int i=0; i<pathList.GetCount(); ++i)
        {
            progress.SetProgress(actionCounter++, actionTotal);
            progress.FormatPathLine(2, IDS_PROC_CLEANUP_INFOREFRESHSHELL, pathList[i].GetWinPath());
            CDirFileEnum crawler(pathList[i].GetWinPathString());
            CString sPath;
            bool bDir = false;
            CTSVNPathList updateList;
            while (crawler.NextFile(sPath, &bDir, true))
            {
                if (bDir)
                {
                    if (!g_SVNAdminDir.IsAdminDirPath(sPath))
                        updateList.AddPath(CTSVNPath(sPath));
                }
            }
            updateList.AddPath(pathList[i]);
            CShellUpdater::Instance().AddPathsForUpdate(updateList);
            CShellUpdater::Instance().Flush();
            updateList.SortByPathname(true);
            for (INT_PTR j=0; j<updateList.GetCount(); ++j)
            {
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, updateList[j].GetWinPath(), NULL);
                ATLTRACE(_T("notify change for path %s\n"), updateList[j].GetWinPath());
            }
        }
    }
    progress.Stop();

    CString strMessage;
    if ( !bFailed )
    {
        CString sPaths;
        for (int i = 0; i < pathList.GetCount(); ++i)
        {
            sPaths += pathList[i].GetWinPathString() + L"\n";
        }
        CString tmp;
        tmp.Format(IDS_PROC_CLEANUPFINISHED, (LPCTSTR)sPaths);
        strMessage += tmp;
    }
    if ( !strFailedString.IsEmpty() )
    {
        if (!strMessage.IsEmpty())
            strMessage += _T("\n");
        CString tmp;
        tmp.Format(IDS_PROC_CLEANUPFINISHED_FAILED, (LPCTSTR)strFailedString);
        strMessage += tmp;
    }
    if (!parser.HasKey(_T("noui")))
        MessageBox(GetExplorerHWND(), strMessage, _T("TortoiseSVN"), MB_OK | (strFailedString.IsEmpty()?MB_ICONINFORMATION:MB_ICONERROR));

    return !bFailed;
}

CString CleanupCommand::GetUnversionedAndRevertPaths( const CTSVNPathList paths, CTSVNPathList& unversioned, CTSVNPathList& reverts )
{
    for (int i=0; i<paths.GetCount(); ++i)
    {
        SVNStatus status;
        CTSVNPath retPath;
        svn_client_status_t * s = status.GetFirstFileStatus(paths[i], retPath, false, svn_depth_infinity, false, true);
        if (s == NULL)
        {
            CString sErr = paths[i].GetWinPathString() + L"\n" + status.GetLastErrorMessage();
            return sErr;
        }
        switch (s->node_status)
        {
        case svn_wc_status_none:
        case svn_wc_status_unversioned:
        case svn_wc_status_ignored:
            unversioned.AddPath(retPath);
            break;
        case svn_wc_status_normal:
            break;
        default:
            reverts.AddPath(retPath);
            break;
        }
        while ((s = status.GetNextFileStatus(retPath))!=NULL)
        {
            switch (s->node_status)
            {
            case svn_wc_status_none:
            case svn_wc_status_unversioned:
            case svn_wc_status_ignored:
                unversioned.AddPath(retPath);
                break;
            case svn_wc_status_normal:
                break;
            default:
                reverts.AddPath(retPath);
                break;
            }
        }
    }
    return L"";
}
