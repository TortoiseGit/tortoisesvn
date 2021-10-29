// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2015, 2021 - TortoiseSVN

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
#include "DropExportCommand.h"
#include "SVN.h"
#include "SVNAdminDir.h"
#include "DirFileEnum.h"
#include "ProgressDlg.h"

#define IDCUSTOM1 23
#define IDCUSTOM2 24
#define IDCUSTOM3 25

bool DropExportCommand::Execute()
{
    bool    bRet     = true;
    CString dropPath = parser.GetVal(L"droptarget");
    if (CTSVNPath(dropPath).IsAdminDir())
        return false;
    SVN::SVNExportType exportType = SVN::SVNExportNormal;
    if (parser.HasKey(L"extended"))
    {
        exportType = SVN::SVNExportIncludeUnversioned;
        CString et = parser.GetVal(L"extended");
        if (et == L"localchanges")
            exportType = SVN::SVNExportOnlyLocalChanges;
        if (et == L"unversioned")
            exportType = SVN::SVNExportIncludeUnversioned;
    }
    SVN svn;
    if ((pathList.GetCount() == 1) &&
        (pathList[0].IsEquivalentTo(CTSVNPath(dropPath))))
    {
        // exporting to itself:
        // remove all svn admin dirs, effectively unversion the 'exported' folder.
        CString msg;
        msg.Format(IDS_PROC_EXPORTUNVERSION, static_cast<LPCWSTR>(dropPath));
        CTaskDialog taskdlg(msg,
                            CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskdlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK3)));
        taskdlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(100);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        if (taskdlg.DoModal(GetExplorerHWND()) != 100)
            return false;

        CProgressDlg progress;
        progress.SetTitle(IDS_PROC_UNVERSION);
        progress.FormatNonPathLine(1, IDS_SVNPROGRESS_EXPORTINGWAIT);
        progress.SetTime(true);
        progress.ShowModeless(GetExplorerHWND());
        std::vector<CTSVNPath> removeVector;

        CDirFileEnum lister(dropPath);
        CString      srcFile;
        bool         bFolder = false;
        while (lister.NextFile(srcFile, &bFolder))
        {
            CTSVNPath item(srcFile);
            if ((bFolder) && (g_SVNAdminDir.IsAdminDirName(item.GetFileOrDirectoryName())))
            {
                removeVector.push_back(item);
            }
        }
        DWORD count = 0;
        for (std::vector<CTSVNPath>::iterator it = removeVector.begin(); (it != removeVector.end()) && (!progress.HasUserCancelled()); ++it)
        {
            progress.FormatPathLine(1, IDS_SVNPROGRESS_UNVERSION, static_cast<LPCWSTR>(it->GetWinPath()));
            progress.SetProgress64(count, removeVector.size());
            count++;
            it->Delete(false);
        }
        progress.Stop();
    }
    else
    {
        bool bOverwrite  = !!parser.HasKey(L"overwrite");
        bool bAutorename = !!parser.HasKey(L"autorename");
        UINT retDefault  = bAutorename ? IDCUSTOM1 : 0;
        for (int nPath = 0; nPath < pathList.GetCount(); nPath++)
        {
            CString dropper = dropPath + L"\\" + pathList[nPath].GetFileOrDirectoryName();
            if ((!bOverwrite) && (PathFileExists(dropper)))
            {
                CString renamedDropper;
                renamedDropper.FormatMessage(IDS_PROC_EXPORTFOLDERNAME, static_cast<LPCWSTR>(dropPath), static_cast<LPCWSTR>(pathList[nPath].GetFileOrDirectoryName()));
                int exportCount = 1;
                while (PathFileExists(renamedDropper))
                {
                    renamedDropper.FormatMessage(IDS_PROC_EXPORTFOLDERNAME2, static_cast<LPCWSTR>(dropPath), static_cast<LPCWSTR>(pathList[nPath].GetFileOrDirectoryName()), exportCount++);
                }

                UINT ret = retDefault;
                if (ret == 0)
                {
                    CString sMsg;
                    sMsg.Format(IDS_PROC_OVERWRITEEXPORT, static_cast<LPCWSTR>(dropper));
                    CTaskDialog taskDlg(sMsg,
                                        CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_TASK2)),
                                        L"TortoiseSVN",
                                        0,
                                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                    taskDlg.AddCommandControl(IDCUSTOM1, CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_TASK3)));
                    CString task4;
                    task4.Format(IDS_PROC_OVERWRITEEXPORT_TASK4, static_cast<LPCWSTR>(renamedDropper));
                    taskDlg.AddCommandControl(IDCUSTOM2, task4);
                    taskDlg.AddCommandControl(IDCUSTOM3, CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_TASK5)));
                    taskDlg.SetDefaultCommandControl(IDCUSTOM2);
                    taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                    taskDlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_TASK6)));
                    taskDlg.SetMainIcon(TD_WARNING_ICON);
                    ret = static_cast<UINT>(taskDlg.DoModal(GetExplorerHWND()));
                    if (taskDlg.GetVerificationCheckboxState())
                        retDefault = ret;
                }

                if (ret == IDCUSTOM3)
                    return false;
                if (ret == IDCUSTOM2)
                {
                    dropper = renamedDropper;
                }
            }
            if (!svn.Export(pathList[nPath], CTSVNPath(dropper), SVNRev::REV_WC, SVNRev::REV_WC, !!parser.HasKey(L"overwrite"), false, false, svn_depth_infinity, GetExplorerHWND(), exportType))
            {
                svn.ShowErrorDialog(GetExplorerHWND(), pathList[nPath]);
                bRet = false;
            }
        }
    }
    return bRet;
}
