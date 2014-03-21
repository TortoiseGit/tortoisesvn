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
#include "PasteMoveCommand.h"

#include "ProgressDlg.h"
#include "AppUtils.h"
#include "SVN.h"
#include "SVNStatus.h"
#include "RenameDlg.h"
#include "ShellUpdater.h"

bool PasteMoveCommand::Execute()
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
    pathList.RemoveAdminPaths();
    CString sNewName;
    CProgressDlg progress;
    progress.SetTitle(IDS_PROC_MOVING);
    progress.SetAnimation(IDR_MOVEANI);
    progress.SetTime(true);
    progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
    for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
    {
        CTSVNPath destPath;
        if (sNewName.IsEmpty())
            destPath = CTSVNPath(sDroppath+L"\\"+pathList[nPath].GetFileOrDirectoryName());
        else
            destPath = CTSVNPath(sDroppath+L"\\"+sNewName);
        if (destPath.Exists())
        {
            CString name = pathList[nPath].GetFileOrDirectoryName();
            if (!sNewName.IsEmpty())
                name = sNewName;
            progress.Stop();
            CRenameDlg dlg;
            dlg.m_name = name;
            dlg.SetInputValidator(this);
            m_renPath = pathList[nPath];
            dlg.m_windowtitle.Format(IDS_PROC_NEWNAMEMOVE, (LPCTSTR)name);
            if (dlg.DoModal() != IDOK)
            {
                return FALSE;
            }
            destPath.SetFromWin(sDroppath+L"\\"+dlg.m_name);
        }
        svn_wc_status_kind s = status.GetAllStatus(pathList[nPath]);
        if ((s == svn_wc_status_none)||(s == svn_wc_status_unversioned)||(s == svn_wc_status_ignored))
        {
            // source file is unversioned: move the file to the target, then add it
            MoveFile(pathList[nPath].GetWinPath(), destPath.GetWinPath());
            if (!svn.Add(CTSVNPathList(destPath), &props, svn_depth_infinity, true, true, false, true))
            {
                svn.ShowErrorDialog(GetExplorerHWND());
                return FALSE;       //get out of here
            }
            CShellUpdater::Instance().AddPathForUpdate(destPath);
        }
        else
        {
            if (!svn.Move(CTSVNPathList(pathList[nPath]), destPath))
            {
                svn.ShowErrorDialog(GetExplorerHWND());
                return FALSE;       //get out of here
            }
            else
                CShellUpdater::Instance().AddPathForUpdate(destPath);
        }
        count++;
        if (progress.IsValid())
        {
            progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, pathList[nPath].GetWinPath());
            progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, destPath.GetWinPath());
            progress.SetProgress(count, pathList.GetCount());
        }
        if ((progress.IsValid())&&(progress.HasUserCancelled()))
        {
            TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_SVN_USERCANCELLED), NULL, TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
            return FALSE;
        }
    }
    return true;
}

CString PasteMoveCommand::Validate( const int /*nID*/, const CString& input )
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
