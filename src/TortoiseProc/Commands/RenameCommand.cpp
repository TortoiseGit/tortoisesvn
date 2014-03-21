// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011, 2013-2014 - TortoiseSVN

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
#include "RenameCommand.h"

#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "RenameDlg.h"
#include "InputLogDlg.h"
#include "SVN.h"
#include "DirFileEnum.h"
#include "ShellUpdater.h"

bool RenameCommand::Execute()
{
    bool bRet = false;
    CString filename = cmdLinePath.GetFileOrDirectoryName();
    CString basePath = cmdLinePath.GetContainingDirectory().GetWinPathString();
    ::SetCurrentDirectory(basePath);

    // show the rename dialog until the user either cancels or enters a new
    // name (one that's different to the original name
    CString sNewName;
    do
    {
        CRenameDlg dlg;
        dlg.m_name = filename;
        if (dlg.DoModal() != IDOK)
            return FALSE;
        sNewName = dlg.m_name;
    } while(PathIsRelative(sNewName) && !PathIsURL(sNewName) && (sNewName.IsEmpty() || (sNewName.Compare(filename)==0)));

    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": rename file %s to %s\n", (LPCTSTR)cmdLinePath.GetWinPathString(), (LPCTSTR)sNewName);
    CTSVNPath destinationPath(basePath);
    if (PathIsRelative(sNewName) && !PathIsURL(sNewName))
        destinationPath.AppendPathString(sNewName);
    else
        destinationPath.SetFromWin(sNewName);
    CString sMsg;
    if (SVN::PathIsURL(cmdLinePath))
    {
        // rename an URL.
        // Ask for a commit message, then rename directly in
        // the repository
        CInputLogDlg input;
        CString sUUID;
        SVN svn;
        svn.GetRepositoryRootAndUUID(cmdLinePath, true, sUUID);
        input.SetUUID(sUUID);
        CString sHint;
        sHint.FormatMessage(IDS_INPUT_MOVE, (LPCTSTR)cmdLinePath.GetSVNPathString(), (LPCTSTR)destinationPath.GetSVNPathString());
        input.SetActionText(sHint);
        if (input.DoModal() == IDOK)
        {
            sMsg = input.GetLogMessage();
        }
        else
        {
            return FALSE;
        }
    }
    if ((cmdLinePath.IsDirectory())||(pathList.GetCount() > 1))
    {
        // renaming a directory can take a while: use the
        // progress dialog to show the progress of the renaming
        // operation.
        CSVNProgressDlg progDlg;
        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Rename);
        progDlg.SetAutoClose (parser);
        progDlg.SetPathList(pathList);
        progDlg.SetUrl(destinationPath.GetWinPathString());
        progDlg.SetCommitMessage(sMsg);
        progDlg.DoModal();
        bRet = !progDlg.DidErrorsOccur();
    }
    else
    {
        CString sFilemask = cmdLinePath.GetFilename();
        int slashpos = sFilemask.ReverseFind('\\');
        if (slashpos < 0)
            slashpos = 0;
        if (sFilemask.ReverseFind('.')>=slashpos)
        {
            while (sFilemask.ReverseFind('.')>=slashpos)
                sFilemask = sFilemask.Left(sFilemask.ReverseFind('.'));
        }
        else
            sFilemask.Empty();
        CString sNewMask = sNewName;
        if (sNewMask.ReverseFind('.')>=0)
        {
            while (sNewMask.ReverseFind('.')>=0)
                sNewMask = sNewMask.Left(sNewMask.ReverseFind('.'));
        }
        else
            sNewMask.Empty();

        if (((!sFilemask.IsEmpty()) && (parser.HasKey(L"noquestion"))) ||
            (cmdLinePath.GetFileExtension().Compare(destinationPath.GetFileExtension())!=0))
        {
            if (RenameWithReplace(GetExplorerHWND(), CTSVNPathList(cmdLinePath), destinationPath, sMsg))
                bRet = true;
        }
        else
        {
            // when refactoring, multiple files have to be renamed
            // at once because those files belong together.
            // e.g. file.aspx, file.aspx.cs, file.aspx.resx
            CTSVNPathList renlist;
            CSimpleFileFind filefind(cmdLinePath.GetDirectory().GetWinPathString(), sFilemask+L".*");
            while (filefind.FindNextFileNoDots())
            {
                if (!filefind.IsDirectory())
                    renlist.AddPath(CTSVNPath(filefind.GetFilePath()));
            }
            if ((renlist.GetCount() <= 1) ||
                (renlist.GetCount() > 10))  // arbitrary value of ten
            {
                // Either no matching files to rename, or way too many of them:
                // just do the default...
                if (RenameWithReplace(GetExplorerHWND(), CTSVNPathList(cmdLinePath), destinationPath, sMsg))
                {
                    bRet = true;
                    CShellUpdater::Instance().AddPathForUpdate(destinationPath);
                }
            }
            else
            {
                std::map<CString, CString> renmap;
                CString sTemp;
                CString sRenList;
                for (int i=0; i<renlist.GetCount(); ++i)
                {
                    CString sFilename = renlist[i].GetFilename();
                    CString sNewFilename = sNewMask + sFilename.Mid(sFilemask.GetLength());
                    sTemp.Format(L"\n%s -> %s", (LPCTSTR)sFilename, (LPCTSTR)sNewFilename);
                    if (!renlist[i].IsEquivalentTo(cmdLinePath))
                        sRenList += sTemp;
                    renmap[renlist[i].GetWinPathString()] = renlist[i].GetContainingDirectory().GetWinPathString()+L"\\"+sNewFilename;
                }
                CString sRenameMultipleQuestion;
                sRenameMultipleQuestion.Format(IDS_PROC_MULTIRENAME, (LPCTSTR)sRenList);
                UINT idret = ::MessageBox(GetExplorerHWND(), sRenameMultipleQuestion, L"TortoiseSVN", MB_ICONQUESTION|MB_YESNOCANCEL);
                if (idret == IDYES)
                {
                    CProgressDlg progress;
                    progress.SetTitle(IDS_PROC_MOVING);
                    progress.SetAnimation(IDR_MOVEANI);
                    progress.SetTime(true);
                    progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
                    DWORD count = 1;
                    for (std::map<CString, CString>::iterator it=renmap.begin(); it != renmap.end(); ++it)
                    {
                        progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, (LPCTSTR)it->first);
                        progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, (LPCTSTR)it->second);
                        progress.SetProgress64(count, renmap.size());
                        if (RenameWithReplace(GetExplorerHWND(), CTSVNPathList(CTSVNPath(it->first)), CTSVNPath(it->second), sMsg))
                        {
                            bRet = true;
                            CShellUpdater::Instance().AddPathForUpdate(CTSVNPath(it->second));
                        }
                    }
                    progress.Stop();
                }
                else if (idret == IDNO)
                {
                    // no, user wants to just rename the file he selected
                    if (RenameWithReplace(GetExplorerHWND(), CTSVNPathList(cmdLinePath), destinationPath, sMsg))
                    {
                        bRet = true;
                        CShellUpdater::Instance().AddPathForUpdate(destinationPath);
                    }
                }
                else if (idret == IDCANCEL)
                {
                    // nothing
                }
            }
        }
    }
    return bRet;
}

bool RenameCommand::RenameWithReplace(HWND hWnd, const CTSVNPathList &srcPathList,
                                      const CTSVNPath &destPath,
                                      const CString &message /* = L"" */,
                                      bool move_as_child /* = false */,
                                      bool make_parents /* = false */) const
{
    SVN svn;
    UINT idret = IDYES;
    bool bRet = true;
    if (destPath.Exists() && !destPath.IsDirectory() && !destPath.IsEquivalentToWithoutCase(srcPathList[0]))
    {
        CString sReplace;
        sReplace.Format(IDS_PROC_REPLACEEXISTING, destPath.GetWinPath());

        CTaskDialog taskdlg(sReplace,
                            CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        INT_PTR ret = taskdlg.DoModal(hWnd);
        if (ret == 1) // replace
            idret = IDYES;
        else
            idret = IDNO;

        if (idret == IDYES)
        {
            if (!svn.Remove(CTSVNPathList(destPath), true, false))
            {
                destPath.Delete(true);
            }
        }
    }
    if ((idret != IDNO)&&(!svn.Move(srcPathList, destPath, message, move_as_child, make_parents)))
    {
        if (svn.GetSVNError()->apr_err == SVN_ERR_ENTRY_NOT_FOUND)
        {
            bRet = !!MoveFile(srcPathList[0].GetWinPath(), destPath.GetWinPath());
        }
        else
        {
            svn.ShowErrorDialog(hWnd, srcPathList.GetCommonDirectory());
            bRet = false;
        }
    }
    if (idret == IDNO)
        bRet = false;
    return bRet;
}