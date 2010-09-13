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
    progress.ShowModeless(CWnd::FromHandle(hwndExplorer));
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
            progress.ShowModeless(CWnd::FromHandle(hwndExplorer));
            // Rebuild the destination path, with the new name
            fullDropPath.SetFromUnknown(sDroppath);
            fullDropPath.AppendPathString(dlg.m_name);
        }
        if (!svn.Copy(CTSVNPathList(sourcePath), fullDropPath, SVNRev::REV_WC, SVNRev()))
        {
            if ((svn.Err && svn.Err->apr_err == SVN_ERR_ENTRY_EXISTS) && (fullDropPath.Exists()))
            {
                // target file already exists. Ask user if he wants to replace the file
                CString sReplace;
                sReplace.Format(IDS_PROC_REPLACEEXISTING, fullDropPath.GetWinPath());
                if (MessageBox(hwndExplorer, sReplace, _T("TortoiseSVN"), MB_ICONQUESTION|MB_YESNO) == IDYES)
                {
                    if (!svn.Remove(CTSVNPathList(fullDropPath), true, false))
                    {
                        fullDropPath.Delete(true);
                    }
                    if (!svn.Copy(CTSVNPathList(pathList[nPath]), fullDropPath, SVNRev::REV_WC, SVNRev()))
                    {
                        MessageBox(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
                        return FALSE;       //get out of here
                    }
                    return TRUE;
                }
            }
            TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
            MessageBox(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
            return FALSE;       //get out of here
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
            MessageBox(hwndExplorer, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
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
    else if (PathFileExists(sDroppath+_T("\\")+input))
        sError.LoadString(IDS_ERR_FILEEXISTS);

    return sError;
}
