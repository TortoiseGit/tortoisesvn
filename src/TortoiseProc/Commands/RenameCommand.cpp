// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011, 2013-2015, 2019, 2021 - TortoiseSVN

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
    bool    bRet     = false;
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
        if (!SVN::PathIsURL(cmdLinePath))
            dlg.SetFileSystemAutoComplete();
        if (dlg.DoModal() != IDOK)
            return FALSE;
        sNewName = dlg.m_name;
    } while (PathIsRelative(sNewName) && !PathIsURL(sNewName) && (sNewName.IsEmpty() || (sNewName.Compare(filename) == 0)));

    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": rename file %s to %s\n", static_cast<LPCWSTR>(cmdLinePath.GetWinPathString()), static_cast<LPCWSTR>(sNewName));
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
        CString      sUuid;
        SVN          svn;
        svn.GetRepositoryRootAndUUID(cmdLinePath, true, sUuid);
        input.SetUUID(sUuid);
        CString sHint;
        sHint.FormatMessage(IDS_INPUT_MOVE, static_cast<LPCWSTR>(cmdLinePath.GetSVNPathString()), static_cast<LPCWSTR>(destinationPath.GetSVNPathString()));
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
    if ((cmdLinePath.IsDirectory()) || (pathList.GetCount() > 1))
    {
        // renaming a directory can take a while: use the
        // progress dialog to show the progress of the renaming
        // operation.
        CSVNProgressDlg progDlg;
        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Rename);
        progDlg.SetAutoClose(parser);
        progDlg.SetPathList(pathList);
        progDlg.SetUrl(destinationPath.GetWinPathString());
        progDlg.SetCommitMessage(sMsg);
        progDlg.DoModal();
        bRet = !progDlg.DidErrorsOccur();
    }
    else
    {
        CString sFilemask = cmdLinePath.GetFilename();
        int     slashPos  = 0;
        // find out up to which char sFilemask and sNewName are identical
        int minLen = min(sFilemask.GetLength(), sNewName.GetLength());
        for (; slashPos < minLen; ++slashPos)
        {
            if (sFilemask[slashPos] != sNewName[slashPos])
                break;
        }

        if (sFilemask.ReverseFind('.') >= slashPos)
        {
            while (sFilemask.ReverseFind('.') >= slashPos)
                sFilemask = sFilemask.Left(sFilemask.ReverseFind('.'));
        }
        else
            sFilemask.Empty();
        CString sNewMask = sNewName;
        if (sNewMask.ReverseFind('.') >= slashPos)
        {
            while (sNewMask.ReverseFind('.') >= slashPos)
                sNewMask = sNewMask.Left(sNewMask.ReverseFind('.'));
        }
        else
            sNewMask.Empty();

        CString sRightPartNew = sNewName.Mid(sNewMask.GetLength());
        CString sRightPartOld = cmdLinePath.GetFilename().Mid(sFilemask.GetLength());

        // if the file extension changed, or the old and new right parts are not the
        // same then we can not correctly guess the new names of similar files, so
        // just do the plain rename of the selected file and don't offer to rename similar ones.
        if (((!sFilemask.IsEmpty()) && (parser.HasKey(L"noquestion"))) ||
            (cmdLinePath.GetFileExtension().Compare(destinationPath.GetFileExtension()) != 0) ||
            (sRightPartOld.CompareNoCase(sRightPartNew)))
        {
            if (RenameWithReplace(GetExplorerHWND(), CTSVNPathList(cmdLinePath), destinationPath, sMsg))
                bRet = true;
        }
        else
        {
            // when refactoring, multiple files have to be renamed
            // at once because those files belong together.
            // e.g. file.aspx, file.aspx.cs, file.aspx.resx
            CTSVNPathList   renList;
            CSimpleFileFind fileFind(cmdLinePath.GetDirectory().GetWinPathString(), sFilemask + L".*");
            while (fileFind.FindNextFileNoDots())
            {
                if (!fileFind.IsDirectory())
                    renList.AddPath(CTSVNPath(fileFind.GetFilePath()));
            }
            if ((renList.GetCount() <= 1) ||
                (renList.GetCount() > 10)) // arbitrary value of ten
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
                std::map<CString, CString> renMap;
                CString                    sTemp;
                CString                    sRenList;
                for (int i = 0; i < renList.GetCount(); ++i)
                {
                    CString sFilename    = renList[i].GetFilename();
                    CString sNewFilename = sNewMask + sFilename.Mid(sFilemask.GetLength());
                    sTemp.Format(L"\n%s -> %s", static_cast<LPCWSTR>(sFilename), static_cast<LPCWSTR>(sNewFilename));
                    if (!renList[i].IsEquivalentTo(cmdLinePath))
                    {
                        if (sFilename != sNewFilename)
                        {
                            sRenList += sTemp;
                            renMap[renList[i].GetWinPathString()] = renList[i].GetContainingDirectory().GetWinPathString() + L"\\" + sNewFilename;
                        }
                    }
                    else
                        renMap[renList[i].GetWinPathString()] = renList[i].GetContainingDirectory().GetWinPathString() + L"\\" + sNewFilename;
                }
                if (!renMap.empty())
                {
                    CString sRenameMultipleQuestion;
                    sRenameMultipleQuestion.Format(IDS_PROC_MULTIRENAME, static_cast<LPCWSTR>(sRenList));
                    UINT idRet = ::MessageBox(GetExplorerHWND(), sRenameMultipleQuestion, L"TortoiseSVN", MB_ICONQUESTION | MB_YESNOCANCEL);
                    if (idRet == IDYES)
                    {
                        CProgressDlg progress;
                        progress.SetTitle(IDS_PROC_MOVING);
                        progress.SetTime(true);
                        progress.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
                        DWORD count = 1;
                        for (std::map<CString, CString>::iterator it = renMap.begin(); it != renMap.end(); ++it)
                        {
                            progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, static_cast<LPCWSTR>(it->first));
                            progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, static_cast<LPCWSTR>(it->second));
                            progress.SetProgress64(count, renMap.size());
                            if (RenameWithReplace(GetExplorerHWND(), CTSVNPathList(CTSVNPath(it->first)), CTSVNPath(it->second), sMsg))
                            {
                                bRet = true;
                                CShellUpdater::Instance().AddPathForUpdate(CTSVNPath(it->second));
                            }
                        }
                        progress.Stop();
                    }
                    else if (idRet == IDNO)
                    {
                        // no, user wants to just rename the file he selected
                        if (RenameWithReplace(GetExplorerHWND(), CTSVNPathList(cmdLinePath), destinationPath, sMsg))
                        {
                            bRet = true;
                            CShellUpdater::Instance().AddPathForUpdate(destinationPath);
                        }
                    }
                    else if (idRet == IDCANCEL)
                    {
                        // nothing
                    }
                }
                else
                {
                    if (RenameWithReplace(GetExplorerHWND(), CTSVNPathList(cmdLinePath), destinationPath, sMsg))
                    {
                        bRet = true;
                        CShellUpdater::Instance().AddPathForUpdate(destinationPath);
                    }
                }
            }
        }
    }
    return bRet;
}

bool RenameCommand::RenameWithReplace(HWND hWnd, const CTSVNPathList &srcPathList,
                                      const CTSVNPath &destPath,
                                      const CString &  message /* = L"" */,
                                      bool             moveAsChild /* = false */,
                                      bool             makeParents /* = false */)
{
    SVN  svn;
    UINT idRet = IDYES;
    bool bRet  = true;
    if (destPath.Exists() && !destPath.IsDirectory() && !destPath.IsEquivalentToWithoutCase(srcPathList[0]))
    {
        CString sReplace;
        sReplace.Format(IDS_PROC_REPLACEEXISTING, destPath.GetWinPath());

        CTaskDialog taskDlg(sReplace,
                            CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK3)));
        taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_PROC_REPLACEEXISTING_TASK4)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetDefaultCommandControl(200);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        INT_PTR ret = taskDlg.DoModal(hWnd);
        if (ret == 100) // replace
            idRet = IDYES;
        else
            idRet = IDNO;

        if (idRet == IDYES)
        {
            if (!svn.Remove(CTSVNPathList(destPath), true, false))
            {
                destPath.Delete(true);
            }
        }
    }
    if ((idRet != IDNO) && (!svn.Move(srcPathList, destPath, message, moveAsChild, makeParents)))
    {
        auto aprErr = svn.GetSVNError()->apr_err;
        if ((aprErr == SVN_ERR_ENTRY_NOT_FOUND) || (aprErr == SVN_ERR_WC_PATH_NOT_FOUND))
        {
            bRet = !!MoveFile(srcPathList[0].GetWinPath(), destPath.GetWinPath());
        }
        else
        {
            svn.ShowErrorDialog(hWnd, srcPathList.GetCommonDirectory());
            bRet = false;
        }
    }
    if (idRet == IDNO)
        bRet = false;
    return bRet;
}