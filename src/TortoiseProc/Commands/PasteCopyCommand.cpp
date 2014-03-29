// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008, 2010-2011, 2014 - TortoiseSVN

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
#include "PasteCopyCommand.h"

#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "AppUtils.h"
#include "RenameDlg.h"
#include "SVN.h"
#include "SVNStatus.h"
#include "ShellUpdater.h"

bool PasteCopyCommand::Execute()
{
    CString sDroppath = parser.GetVal(L"droptarget");
    CTSVNPath dropPath(sDroppath);
    ProjectProperties props;
    props.ReadProps(dropPath);
    if (dropPath.IsAdminDir())
        return FALSE;
    SVN svn;
    SVNStatus status;
    unsigned long count = 0;
    CString sNewName;
    pathList.RemoveAdminPaths();
    CProgressDlg progress;
    progress.SetTitle(IDS_PROC_COPYING);
    progress.SetTime(true);
    progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
    for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
    {
        const CTSVNPath& sourcePath = pathList[nPath];

        CTSVNPath fullDropPath = dropPath;
        if (sNewName.IsEmpty())
            fullDropPath.AppendPathString(sourcePath.GetFileOrDirectoryName());
        else
            fullDropPath.AppendPathString(sNewName);

        // Check for a drop-on-to-ourselves
        if (sourcePath.IsEquivalentTo(fullDropPath))
        {
            // Offer a rename
            progress.Stop();
            CRenameDlg dlg;
            dlg.SetInputValidator(this);
            m_renPath = sourcePath;
            dlg.m_windowtitle.Format(IDS_PROC_NEWNAMECOPY, (LPCTSTR)sourcePath.GetUIFileOrDirectoryName());
            if (dlg.DoModal() != IDOK)
            {
                return FALSE;
            }
            // rebuild the progress dialog
            progress.EnsureValid();
            progress.SetTitle(IDS_PROC_COPYING);
            progress.SetTime(true);
            progress.SetProgress(count, pathList.GetCount());
            progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
            // Rebuild the destination path, with the new name
            fullDropPath.SetFromUnknown(sDroppath);
            fullDropPath.AppendPathString(dlg.m_name);
        }

        svn_wc_status_kind s = status.GetAllStatus(sourcePath);
        if ((s == svn_wc_status_none)||(s == svn_wc_status_unversioned)||(s == svn_wc_status_ignored))
        {
            // source file is unversioned: move the file to the target, then add it
            CopyFile(sourcePath.GetWinPath(), fullDropPath.GetWinPath(), FALSE);
            if (!svn.Add(CTSVNPathList(fullDropPath), &props, svn_depth_infinity, true, true, false, true))
            {
                svn.ShowErrorDialog(GetExplorerHWND(), fullDropPath);
                return FALSE;       //get out of here
            }
            else
                CShellUpdater::Instance().AddPathForUpdate(fullDropPath);
        }
        else
        {
            if (!svn.Copy(CTSVNPathList(sourcePath), fullDropPath, SVNRev::REV_WC, SVNRev()))
            {
                svn.ShowErrorDialog(GetExplorerHWND(), sourcePath);
                return FALSE;       //get out of here
            }
            else
                CShellUpdater::Instance().AddPathForUpdate(fullDropPath);
        }
        count++;
        if (progress.IsValid())
        {
            progress.FormatPathLine(1, IDS_PROC_COPYINGPROG, sourcePath.GetWinPath());
            progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, fullDropPath.GetWinPath());
            progress.SetProgress(count, pathList.GetCount());
        }
        if ((progress.IsValid())&&(progress.HasUserCancelled()))
        {
            TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_SVN_USERCANCELLED), NULL, TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
            return false;
        }
    }
    return true;
}

CString PasteCopyCommand::Validate( const int /*nID*/, const CString& input )
{
    CString sError;

    CTSVNPath p = m_renPath.GetContainingDirectory();
    p.AppendPathString(input);
    if (input.IsEmpty())
        sError.LoadString(IDS_ERR_NOVALIDPATH);
    else if (PathFileExists(p.GetWinPath()))
        sError.LoadString(IDS_ERR_FILEEXISTS);

    return sError;
}
