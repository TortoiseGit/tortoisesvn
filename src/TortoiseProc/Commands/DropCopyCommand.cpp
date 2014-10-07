// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2011, 2014 - TortoiseSVN

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
#include "DropCopyCommand.h"

#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "AppUtils.h"
#include "RenameDlg.h"
#include "SVN.h"
#include "ShellUpdater.h"

#define IDYESTOALL          19
#define IDNOTOALL           20

bool DropCopyCommand::Execute()
{
    CString sDroppath = parser.GetVal(L"droptarget");
    if (CTSVNPath(sDroppath).IsAdminDir())
        return FALSE;
    SVN svn;
    unsigned long count = 0;
    CString sNewName;
    pathList.RemoveAdminPaths();
    if ((parser.HasKey(L"rename"))&&(pathList.GetCount()==1))
    {
        // ask for a new name of the source item
        CRenameDlg renDlg;
        renDlg.SetInputValidator(this);
        renDlg.SetFileSystemAutoComplete();
        renDlg.m_windowtitle.LoadString(IDS_PROC_COPYRENAME);
        renDlg.m_name = pathList[0].GetFileOrDirectoryName();
        if (renDlg.DoModal() != IDOK)
        {
            return FALSE;
        }
        sNewName = renDlg.m_name;
    }
    CProgressDlg progress;
    progress.SetTitle(IDS_PROC_COPYING);
    progress.SetTime(true);
    progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
    UINT msgRet = IDNO;
    INT_PTR msgRetNonversioned = 0;
    for (int nPath = 0; nPath < pathList.GetCount(); nPath++)
    {
        const CTSVNPath& sourcePath = pathList[nPath];

        CTSVNPath fullDropPath(sDroppath);
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
            dlg.SetFileSystemAutoComplete();
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
        if (!svn.Copy(CTSVNPathList(sourcePath), fullDropPath, SVNRev::REV_WC, SVNRev()))
        {
            if ((svn.GetSVNError() && svn.GetSVNError()->apr_err == SVN_ERR_ENTRY_EXISTS) && (fullDropPath.Exists()))
            {
                if ((msgRet != IDYESTOALL) && (msgRet != IDNOTOALL))
                {
                    // target file already exists. Ask user if he wants to replace the file
                    CString sReplace;
                    sReplace.Format(IDS_PROC_REPLACEEXISTING, fullDropPath.GetWinPath());
                    CTaskDialog taskdlg(sReplace,
                                        CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK2)),
                                        L"TortoiseSVN",
                                        0,
                                        TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
                    taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK3)));
                    taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK4)));
                    taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                    taskdlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK5)));
                    taskdlg.SetVerificationCheckbox(false);
                    taskdlg.SetDefaultCommandControl(2);
                    taskdlg.SetMainIcon(TD_WARNING_ICON);
                    INT_PTR ret = taskdlg.DoModal(GetExplorerHWND());
                    if (ret == 1) // replace
                        msgRet = taskdlg.GetVerificationCheckboxState() ? IDYES : IDYESTOALL;
                    else
                        msgRet = taskdlg.GetVerificationCheckboxState() ? IDNO : IDNOTOALL;
                }

                if ((msgRet == IDYES) || (msgRet == IDYESTOALL))
                {
                    if (!svn.Remove(CTSVNPathList(fullDropPath), true, false))
                    {
                        fullDropPath.Delete(true);
                    }
                    if (!svn.Copy(CTSVNPathList(pathList[nPath]), fullDropPath, SVNRev::REV_WC, SVNRev()))
                    {
                        svn.ShowErrorDialog(GetExplorerHWND(), pathList[nPath]);
                        return FALSE;       //get out of here
                    }
                }
            }
            else
            {
                svn.ShowErrorDialog(GetExplorerHWND(), sourcePath);
                return FALSE;       //get out of here
            }
        }
        else if (svn.GetSVNError() && svn.GetSVNError()->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
        {
            INT_PTR ret = 0;
            if (msgRetNonversioned == 0)
            {
                CString sReplace;
                sReplace.Format(IDS_PROC_MOVEUNVERSIONED_TASK1, fullDropPath.GetWinPath());
                CTaskDialog taskdlg(sReplace,
                                    CString(MAKEINTRESOURCE(IDS_PROC_MOVEUNVERSIONED_TASK2)),
                                    L"TortoiseSVN",
                                    TDCBF_CANCEL_BUTTON,
                                    TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
                taskdlg.AddCommandControl(101, CString(MAKEINTRESOURCE(IDS_PROC_COPYUNVERSIONED_TASK3)));
                taskdlg.AddCommandControl(102, CString(MAKEINTRESOURCE(IDS_PROC_COPYUNVERSIONED_TASK4)));
                taskdlg.AddCommandControl(103, CString(MAKEINTRESOURCE(IDS_PROC_MOVEUNVERSIONED_TASK5)));
                taskdlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_MOVEUNVERSIONED_TASK6)));
                taskdlg.SetVerificationCheckbox(false);
                taskdlg.SetDefaultCommandControl(103);
                taskdlg.SetMainIcon(TD_WARNING_ICON);
                ret = taskdlg.DoModal(GetExplorerHWND());
                if (taskdlg.GetVerificationCheckboxState())
                    msgRetNonversioned = ret;
            }
            else
            {
                ret = msgRetNonversioned;
            }
            switch (ret)
            {
                case 101: // copy
                    CopyFile(pathList[nPath].GetWinPath(), fullDropPath.GetWinPath(), FALSE);
                    break;
                case 102: // copy and add
                    CopyFile(pathList[nPath].GetWinPath(), fullDropPath.GetWinPath(), FALSE);
                    if (!svn.Add(CTSVNPathList(fullDropPath), NULL, svn_depth_infinity, true, false, false, false))
                    {
                        svn.ShowErrorDialog(GetExplorerHWND(), fullDropPath);
                        return FALSE;       //get out of here
                    }
                    break;
                case 103: // skip
                default:
                    break;
            }
        }
        else
            CShellUpdater::Instance().AddPathForUpdate(fullDropPath);
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

CString DropCopyCommand::Validate(const int /*nID*/, const CString& input)
{
    CString sError;

    CString sDroppath = parser.GetVal(L"droptarget");
    if (input.IsEmpty())
        sError.LoadString(IDS_ERR_NOVALIDPATH);
    else if (!CTSVNPath(sDroppath+L"\\"+input).IsValidOnWindows())
        sError.LoadString(IDS_ERR_NOVALIDPATH);

    return sError;
}
