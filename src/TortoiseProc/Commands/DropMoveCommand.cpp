// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010 - TortoiseSVN

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
#include "DropMoveCommand.h"

#include "ProgressDlg.h"
#include "AppUtils.h"
#include "SVN.h"
#include "RenameDlg.h"
#include "ShellUpdater.h"

bool DropMoveCommand::Execute()
{
    CString droppath = parser.GetVal(_T("droptarget"));
    if (CTSVNPath(droppath).IsAdminDir())
        return FALSE;
    SVN svn;
    unsigned long count = 0;
    pathList.RemoveAdminPaths();
    CString sNewName;
    if ((parser.HasKey(_T("rename")))&&(pathList.GetCount()==1))
    {
        // ask for a new name of the source item
        CRenameDlg renDlg;
        renDlg.SetInputValidator(this);
        renDlg.m_windowtitle.LoadString(IDS_PROC_MOVERENAME);
        renDlg.m_name = pathList[0].GetFileOrDirectoryName();
        if (renDlg.DoModal() != IDOK)
        {
            return FALSE;
        }
        sNewName = renDlg.m_name;
    }
    CProgressDlg progress;
    if (progress.IsValid())
    {
        progress.SetTitle(IDS_PROC_MOVING);
        progress.SetAnimation(IDR_MOVEANI);
        progress.SetTime(true);
        progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
    }
    for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
    {
        CTSVNPath destPath;
        if (sNewName.IsEmpty())
            destPath = CTSVNPath(droppath+_T("\\")+pathList[nPath].GetFileOrDirectoryName());
        else
            destPath = CTSVNPath(droppath+_T("\\")+sNewName);
        if (destPath.Exists())
        {
            CString name = pathList[nPath].GetFileOrDirectoryName();
            if (!sNewName.IsEmpty())
                name = sNewName;
            progress.Stop();
            CRenameDlg dlg;
            dlg.m_name = name;
            dlg.m_windowtitle.Format(IDS_PROC_NEWNAMEMOVE, (LPCTSTR)name);
            if (dlg.DoModal() != IDOK)
            {
                return FALSE;
            }
            destPath.SetFromWin(droppath+_T("\\")+dlg.m_name);
        }
        if (!svn.Move(CTSVNPathList(pathList[nPath]), destPath))
        {
            if ((svn.GetSVNError() && svn.GetSVNError()->apr_err == SVN_ERR_ENTRY_EXISTS) && (destPath.Exists()))
            {
                // target file already exists. Ask user if he wants to replace the file
                CString sReplace;
                sReplace.Format(IDS_PROC_REPLACEEXISTING, destPath.GetWinPath());
                if (MessageBox(GetExplorerHWND(), sReplace, _T("TortoiseSVN"), MB_ICONQUESTION|MB_YESNO) == IDYES)
                {
                    if (!svn.Remove(CTSVNPathList(destPath), true, false))
                    {
                        destPath.Delete(true);
                    }
                    if (!svn.Move(CTSVNPathList(pathList[nPath]), destPath))
                    {
                        svn.ShowErrorDialog(GetExplorerHWND(), pathList[nPath]);
                        return FALSE;       //get out of here
                    }
                    CShellUpdater::Instance().AddPathForUpdate(destPath);
                }
            }
            else
            {
                svn.ShowErrorDialog(GetExplorerHWND(), pathList[nPath]);
                return FALSE;       //get out of here
            }
        }
        else
            CShellUpdater::Instance().AddPathForUpdate(destPath);
        count++;
        if (progress.IsValid())
        {
            progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, pathList[nPath].GetWinPath());
            progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, destPath.GetWinPath());
            progress.SetProgress(count, pathList.GetCount());
        }
        if ((progress.IsValid())&&(progress.HasUserCancelled()))
        {
            MessageBox(GetExplorerHWND(), IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
            return FALSE;
        }
    }
    return true;
}

CString DropMoveCommand::Validate(const int /*nID*/, const CString& input)
{
    CString sError;

    CString sDroppath = parser.GetVal(_T("droptarget"));
    if (input.IsEmpty())
        sError.LoadString(IDS_ERR_NOVALIDPATH);
    else if (PathFileExists(sDroppath+_T("\\")+input))
        sError.LoadString(IDS_ERR_FILEEXISTS);

    return sError;
}
