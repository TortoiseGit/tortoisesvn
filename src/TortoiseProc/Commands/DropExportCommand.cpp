// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011 - TortoiseSVN

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
#include "DropExportCommand.h"
#include "MessageBox.h"
#include "SVN.h"
#include "SVNAdminDir.h"
#include "DirFileEnum.h"
#include "ProgressDlg.h"

bool DropExportCommand::Execute()
{
    bool bRet = true;
    CString droppath = parser.GetVal(_T("droptarget"));
    if (CTSVNPath(droppath).IsAdminDir())
        return false;
    SVN svn;
    if ((pathList.GetCount() == 1)&&
        (pathList[0].IsEquivalentTo(CTSVNPath(droppath))))
    {
        // exporting to itself:
        // remove all svn admin dirs, effectively unversion the 'exported' folder.
        CString msg;
        msg.Format(IDS_PROC_EXPORTUNVERSION, (LPCTSTR)droppath);
        if (CTaskDialog::IsSupported())
        {
            CTaskDialog taskdlg(msg, 
                                CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK2)), 
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK4)));
            taskdlg.SetDefaultCommandControl(1);
            taskdlg.SetMainIcon(TD_WARNING_ICON);
            if (taskdlg.DoModal(GetExplorerHWND()) != 1)
                return false;
        }
        else
        {
            if (MessageBox(GetExplorerHWND(), msg, _T("TortoiseSVN"), MB_ICONQUESTION|MB_YESNO) != IDYES)
                return false;
        }

        CProgressDlg progress;
        progress.SetTitle(IDS_PROC_UNVERSION);
        progress.SetAnimation(IDR_MOVEANI);
        progress.FormatNonPathLine(1, IDS_SVNPROGRESS_EXPORTINGWAIT);
        progress.SetTime(true);
        progress.ShowModeless(GetExplorerHWND());
        std::vector<CTSVNPath> removeVector;

        CDirFileEnum lister(droppath);
        CString srcFile;
        bool bFolder = false;
        while (lister.NextFile(srcFile, &bFolder))
        {
            CTSVNPath item(srcFile);
            if ((bFolder)&&(g_SVNAdminDir.IsAdminDirName(item.GetFileOrDirectoryName())))
            {
                removeVector.push_back(item);
            }
        }
        DWORD count = 0;
        for (std::vector<CTSVNPath>::iterator it = removeVector.begin(); (it != removeVector.end()) && (!progress.HasUserCancelled()); ++it)
        {
            progress.FormatPathLine(1, IDS_SVNPROGRESS_UNVERSION, (LPCTSTR)it->GetWinPath());
            progress.SetProgress64(count, removeVector.size());
            count++;
            it->Delete(false);
        }
        progress.Stop();
    }
    else
    {
        bool bOverwrite = !!parser.HasKey(L"overwrite");
        bool bAutorename = !!parser.HasKey(L"autorename");
        UINT retDefault = bAutorename ? IDCUSTOM1 : 0;
        for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
        {
            CString dropper = droppath + _T("\\") + pathList[nPath].GetFileOrDirectoryName();
            if ((!bOverwrite)&&(PathFileExists(dropper)))
            {
                CString renameddropper;
                renameddropper.FormatMessage(IDS_PROC_EXPORTFOLDERNAME, (LPCTSTR)droppath, (LPCTSTR)pathList[nPath].GetFileOrDirectoryName());
                int exportcount = 1;
                while (PathFileExists(renameddropper))
                {
                    renameddropper.FormatMessage(IDS_PROC_EXPORTFOLDERNAME2, (LPCTSTR)droppath, exportcount++, (LPCTSTR)pathList[nPath].GetFileOrDirectoryName());
                }

                UINT ret = retDefault;
                if (ret == 0)
                {
                    CString sMsg;
                    sMsg.Format(IDS_PROC_OVERWRITEEXPORT, (LPCTSTR)dropper);
                    if (CTaskDialog::IsSupported())
                    {
                        CTaskDialog taskdlg(sMsg, 
                                            CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_TASK2)), 
                                            L"TortoiseSVN",
                                            0,
                                            TDF_ENABLE_HYPERLINKS|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
                        taskdlg.AddCommandControl(IDCUSTOM1, CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_TASK3)));
                        CString task4;
                        task4.Format(IDS_PROC_OVERWRITEEXPORT_TASK4, (LPCTSTR)renameddropper);
                        taskdlg.AddCommandControl(IDCUSTOM2, task4);
                        taskdlg.AddCommandControl(IDCUSTOM3, CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_TASK5)));
                        taskdlg.SetDefaultCommandControl(IDCUSTOM2);
                        taskdlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_TASK6)));
                        taskdlg.SetMainIcon(TD_WARNING_ICON);
                        ret = (UINT)taskdlg.DoModal(GetExplorerHWND());
                        if (taskdlg.GetVerificationCheckboxState())
                            retDefault = ret;
                    }
                    else
                    {
                        CString sBtn1(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_OVERWRITE));
                        CString sBtn2(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_RENAME));
                        CString sBtn3(MAKEINTRESOURCE(IDS_PROC_OVERWRITEEXPORT_CANCEL));
                        ret = TSVNMessageBox(GetExplorerHWND(), sMsg, _T("TortoiseSVN"), MB_DEFBUTTON1|MB_ICONQUESTION, sBtn1, sBtn2, sBtn3);
                    }
                }

                if (ret == IDCUSTOM3)
                    return false;
                if (ret==IDCUSTOM2)
                {
                    dropper = renameddropper;
                }
            }
            if (!svn.Export(pathList[nPath], CTSVNPath(dropper), SVNRev::REV_WC ,SVNRev::REV_WC, false, false, svn_depth_infinity, GetExplorerHWND(), !!parser.HasKey(_T("extended"))))
            {
                svn.ShowErrorDialog(GetExplorerHWND(), pathList[nPath]);
                bRet = false;
            }
        }
    }
    return bRet;
}
