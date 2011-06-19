// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2011 - TortoiseSVN

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
#include "DropCopyCommand.h"

#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "AppUtils.h"
#include "RenameDlg.h"
#include "SVN.h"
#include "ShellUpdater.h"

bool DropCopyCommand::Execute()
{
    CString sDroppath = parser.GetVal(_T("droptarget"));
    if (CTSVNPath(sDroppath).IsAdminDir())
        return FALSE;
    SVN svn;
    unsigned long count = 0;
    CString sNewName;
    pathList.RemoveAdminPaths();
    if ((parser.HasKey(_T("rename")))&&(pathList.GetCount()==1))
    {
        // ask for a new name of the source item
        CRenameDlg renDlg;
        renDlg.SetInputValidator(this);
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
    progress.SetAnimation(IDR_MOVEANI);
    progress.SetTime(true);
    progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
    UINT msgRet = IDNO;
    for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
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
            dlg.m_windowtitle.Format(IDS_PROC_NEWNAMECOPY, (LPCTSTR)sourcePath.GetUIFileOrDirectoryName());
            if (dlg.DoModal() != IDOK)
            {
                return FALSE;
            }
            // rebuild the progress dialog
            progress.EnsureValid();
            progress.SetTitle(IDS_PROC_COPYING);
            progress.SetAnimation(IDR_MOVEANI);
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
                    if (CTaskDialog::IsSupported())
                    {
                        CTaskDialog taskdlg(sReplace, 
                                            CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK2)), 
                                            L"TortoiseSVN",
                                            0,
                                            TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
                        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK3)));
                        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK4)));
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
                    else
                    {
                        msgRet = TSVNMessageBox(GetExplorerHWND(), sReplace, _T("TortoiseSVN"), MB_ICONQUESTION|MB_YESNO|MB_YESTOALL|MB_NOTOALL);
                    }
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
            TSVNMessageBox(GetExplorerHWND(), IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
            return false;
        }
    }
    return true;
}

CString DropCopyCommand::Validate(const int /*nID*/, const CString& input)
{
    CString sError;

    CString sDroppath = parser.GetVal(_T("droptarget"));
    if (input.IsEmpty())
        sError.LoadString(IDS_ERR_NOVALIDPATH);
    else if (!CTSVNPath(sDroppath+_T("\\")+input).IsValidOnWindows())
        sError.LoadString(IDS_ERR_NOVALIDPATH);

    return sError;
}
