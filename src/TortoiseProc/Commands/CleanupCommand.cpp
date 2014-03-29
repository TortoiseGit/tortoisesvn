// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2014 - TortoiseSVN

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
#include "CleanupCommand.h"

#include "SVN.h"
#include "SVNGlobal.h"
#include "SVNAdminDir.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"
#include "CleanupDlg.h"
#include "SVNStatus.h"
#include "SVNHelpers.h"
#include "../../TSVNCache/CacheInterface.h"
#include "RecycleBinDlg.h"

bool CleanupCommand::Execute()
{
    bool bCleanup           = !!parser.HasKey(L"cleanup");
    bool bRevert            = !!parser.HasKey(L"revert");
    bool bDelUnversioned    = !!parser.HasKey(L"delunversioned");
    bool bDelIgnored        = !!parser.HasKey(L"delignored");
    bool bRefreshShell      = !!parser.HasKey(L"refreshshell");
    bool bIncludeExternals  = !!parser.HasKey(L"externals");
    bool bBreakLocks        = !!parser.HasKey(L"breaklocks");
    bool bFixTimestamps     = !!parser.HasKey(L"fixtimestamps");
    bool bVacuum            = !!parser.HasKey(L"vacuum");

    if (!parser.HasKey(L"noui") && !parser.HasKey(L"nodlg"))
    {
        CCleanupDlg dlg;
        if (dlg.DoModal() != IDOK)
            return false;
        bCleanup            = !!dlg.m_bCleanup;
        bRevert             = !!dlg.m_bRevert;
        bDelUnversioned     = !!dlg.m_bDelUnversioned;
        bDelIgnored         = !!dlg.m_bDelIgnored;
        bRefreshShell       = !!dlg.m_bRefreshShell;
        bIncludeExternals   = !!dlg.m_bExternals;
        bBreakLocks         = !!dlg.m_bBreakLocks;
        bFixTimestamps      = !!dlg.m_bFixTimestamps;
        bVacuum             = !!dlg.m_bVacuum;
    }

    if (!bCleanup && !bRevert && !bDelUnversioned && !bDelIgnored && !bRefreshShell)
        return false;

    bool bUseTrash = DWORD(CRegDWORD(L"Software\\TortoiseSVN\\RevertWithRecycleBin", TRUE)) != 0;
    int actionTotal = 0;
    if (bCleanup)
        actionTotal += pathList.GetCount();
    if (bRevert || bDelUnversioned)
        actionTotal++;  // for fetching the status
    if (bRevert)
        actionTotal++;
    if (bDelUnversioned)
        actionTotal++;
    if (bDelIgnored)
        actionTotal++;
    if (bRefreshShell)
        actionTotal += pathList.GetCount();
    int actionCounter = 0;
    CProgressDlg progress;
    progress.SetTitle(IDS_PROC_CLEANUP);
    progress.FormatNonPathLine(2, IDS_PROC_CLEANUP_INFO1, L"");
    progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO2)));
    progress.SetProgress(actionCounter++, actionTotal);
    if (!parser.HasKey(L"noprogressui"))
        progress.ShowModeless(GetExplorerHWND());

    CString strFailedString;
    bool bFailed = false;
    bool bCleanupFailed = false;
    if (bCleanup)
    {
        bCleanupFailed = CleanupPaths(progress, actionCounter, actionTotal, bBreakLocks, bFixTimestamps, bVacuum, strFailedString);
    }
    CTSVNPathList itemsToRevert, unversionedItems, ignoredItems, externals;
    if (bRevert || bDelUnversioned || bDelIgnored || bIncludeExternals)
    {
        progress.SetProgress(actionCounter++, actionTotal);
        progress.FormatPathLine(2, IDS_PROC_CLEANUP_INFOFETCHSTATUS, pathList.GetCommonRoot().GetWinPath());
        strFailedString = GetCleanupPaths(pathList, unversionedItems, ignoredItems, itemsToRevert, bIncludeExternals, externals);
        bFailed = !strFailedString.IsEmpty();
    }
    if (bIncludeExternals)
    {
        actionTotal += externals.GetCount();
        for (int i=0; i<externals.GetCount(); ++i)
        {
            if (SVNHelper::IsVersioned(externals[i], true))
            {
                SVN svn;
                progress.FormatPathLine(2, IDS_PROC_CLEANUP_INFO1, (LPCTSTR)externals[i].GetFileOrDirectoryName());
                progress.SetProgress(actionCounter++, actionTotal);
                CBlockCacheForPath cacheBlock (externals[i].GetWinPath());
                if (!svn.CleanUp(externals[i], bBreakLocks, bFixTimestamps, true, bVacuum, false))
                {
                    strFailedString = externals[i].GetWinPathString();
                    strFailedString += L"\n" + svn.GetLastErrorMessage();
                    bFailed = true;
                    break;
                }
            }
        }
    }
    if (bCleanupFailed && bCleanup && bIncludeExternals && !bFailed)
    {
        bFailed = CleanupPaths(progress, actionCounter, actionTotal, bBreakLocks, bFixTimestamps, bVacuum, strFailedString);
    }
    if (!bFailed && bRevert)
    {
        progress.SetProgress(actionCounter++, actionTotal);
        progress.FormatNonPathLine(2, IDS_PROC_CLEANUP_INFOREVERT, pathList.GetCommonRoot().GetWinPath());
        if (itemsToRevert.GetCount())
        {
            CTSVNPathList revertItems = itemsToRevert;
            if (bUseTrash)
            {
                CRecycleBinDlg rec;
                rec.StartTime();
                int count = itemsToRevert.GetCount();
                itemsToRevert.DeleteAllPaths(bUseTrash, true, NULL);
                rec.EndTime(count);
            }
            SVN svn;
            if (!svn.Revert(revertItems, CStringArray(), false, false))
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
        progress.SetProgress(actionCounter++, actionTotal);
        HWND hErrorWnd = GetExplorerHWND();
        if (hErrorWnd == NULL)
            hErrorWnd = GetDesktopWindow();
        unversionedItems.DeleteAllPaths(bUseTrash, false, hErrorWnd);
    }
    if (!bFailed && bDelIgnored)
    {
        progress.SetProgress(actionCounter++, actionTotal);
        progress.FormatPathLine(2, IDS_PROC_CLEANUP_INFODELIGNORED, pathList.GetCommonRoot().GetWinPath());
        progress.SetProgress(actionCounter++, actionTotal);
        HWND hErrorWnd = GetExplorerHWND();
        if (hErrorWnd == NULL)
            hErrorWnd = GetDesktopWindow();
        ignoredItems.DeleteAllPaths(bUseTrash, false, hErrorWnd);
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
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": notify change for path %s\n", updateList[j].GetWinPath());
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
            strMessage += L"\n";
        CString tmp;
        tmp.Format(IDS_PROC_CLEANUPFINISHED_FAILED, (LPCTSTR)strFailedString);
        strMessage += tmp;
    }
    if (!parser.HasKey(L"noui"))
        MessageBox(GetExplorerHWND(), strMessage, L"TortoiseSVN", MB_OK | (strFailedString.IsEmpty()?MB_ICONINFORMATION:MB_ICONERROR));

    return !bFailed;
}

CString CleanupCommand::GetCleanupPaths( const CTSVNPathList& paths, CTSVNPathList& unversioned, CTSVNPathList& ignored, CTSVNPathList& reverts, bool includeExts, CTSVNPathList& externals )
{
    for (int i=0; i<paths.GetCount(); ++i)
    {
        SVNStatus status;
        CTSVNPath retPath;
        svn_client_status_t * s = status.GetFirstFileStatus(paths[i], retPath, false, svn_depth_infinity, true, !includeExts);
        if (s == NULL)
        {
            if ((pathList.GetCount() > 1) && !SVNHelper::IsVersioned(paths[i], false))
                continue;   // ignore failures for unversioned paths

            CString sErr = paths[i].GetWinPathString() + L"\n" + status.GetLastErrorMessage();
            return sErr;
        }
        switch (s->node_status)
        {
        case svn_wc_status_none:
        case svn_wc_status_unversioned:
            unversioned.AddPath(retPath);
            break;
        case svn_wc_status_ignored:
            ignored.AddPath(retPath);
            break;
        case svn_wc_status_normal:
            break;
        case svn_wc_status_external:
            externals.AddPath(retPath);
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
                unversioned.AddPath(retPath);
                break;
            case svn_wc_status_ignored:
                ignored.AddPath(retPath);
                break;
            case svn_wc_status_normal:
                break;
            case svn_wc_status_external:
                externals.AddPath(retPath);
                break;
            default:
                reverts.AddPath(retPath);
                break;
            }
        }
        std::set<CTSVNPath> extset;
        status.GetExternals(extset);
        for (auto it = extset.cbegin(); it != extset.cend(); ++it)
        {
            if (it->IsDirectory())
                externals.AddPath(*it);
        }
    }
    return L"";
}

bool CleanupCommand::CleanupPaths(CProgressDlg &progress, int &actionCounter, int actionTotal, bool bBreakLocks, bool bFixTimestamps, bool bVacuum, CString &strFailedString)
{
    bool bCleanupFailed = false;
    SVN svn;
    for (int i=0; i<pathList.GetCount(); ++i)
    {
        progress.FormatPathLine(2, IDS_PROC_CLEANUP_INFO1, (LPCTSTR)pathList[i].GetFileOrDirectoryName());
        progress.SetProgress(actionCounter++, actionTotal);
        CBlockCacheForPath cacheBlock (pathList[i].GetWinPath());
        if (!svn.CleanUp(pathList[i], bBreakLocks, bFixTimestamps, true, bVacuum, false))
        {
            if ((pathList.GetCount() > 1) && !SVNHelper::IsVersioned(pathList[i], false))
                continue;   // ignore failures for unversioned paths
            strFailedString = pathList[i].GetWinPathString();
            strFailedString += L"\n" + svn.GetLastErrorMessage();
            bCleanupFailed = true;
            break;
        }
    }
    if (pathList.GetCount()==1)
    {
        // also clean up the wc root
        CTSVNPath wcroot = svn.GetWCRootFromPath(pathList[0]);
        if (!wcroot.IsEquivalentToWithoutCase(pathList[0]))
        {
            bCleanupFailed = false;
            CBlockCacheForPath cacheBlock (wcroot.GetWinPath());
            if (!svn.CleanUp(wcroot, bBreakLocks, bFixTimestamps, true, bVacuum, false))
            {
                strFailedString = wcroot.GetWinPathString();
                strFailedString += L"\n" + svn.GetLastErrorMessage();
                bCleanupFailed = true;
            }
        }
    }
    return bCleanupFailed;
}
