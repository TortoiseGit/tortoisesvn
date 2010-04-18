// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

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
#include "AddCommand.h"

#include "AddDlg.h"
#include "SVNProgressDlg.h"
#include "ShellUpdater.h"
#include "SVNStatus.h"
#include "MessageBox.h"

bool AddCommand::Execute()
{
    bool bRet = false;
    if (parser.HasKey(_T("noui")))
    {
        SVN svn;
        ProjectProperties props;
        props.ReadPropsPathList(pathList);
        bRet = !!svn.Add(pathList, &props, svn_depth_empty, false, false, true);
        CShellUpdater::Instance().AddPathsForUpdate(pathList);
    }
    else
    {
        if (pathList.AreAllPathsFiles())
        {
            SVNStatus status;
            CTSVNPath retPath;
            svn_wc_status2_t * s = NULL;

            if ((s = status.GetFirstFileStatus(pathList.GetCommonDirectory(), retPath))!=0)
            {
                do
                {
                    if (s->text_status == svn_wc_status_missing)
                    {
                        for (int i = 0; i < pathList.GetCount(); ++i)
                        {
                            if (pathList[i].IsEquivalentToWithoutCase(retPath))
                            {
                                CString sMessage;
                                sMessage.FormatMessage(IDS_WARN_ADDCASERENAMED, pathList[i].GetWinPath(), retPath.GetWinPath());
                                CString sTitle(MAKEINTRESOURCE(IDS_WARN_WARNING));
                                CString sFixRenaming(MAKEINTRESOURCE(IDS_WARN_ADDCASERENAMED_RENAME));
                                CString sAddAnyway(MAKEINTRESOURCE(IDS_WARN_ADDCASERENAMED_ADD));
                                CString sCancel(MAKEINTRESOURCE(IDS_MSGBOX_CANCEL));

                                UINT ret = CMessageBox::Show(hWndExplorer, sMessage, sTitle, 1, IDI_WARNING, sFixRenaming, sAddAnyway, sCancel);
                                if (ret == 1)
                                {
                                    // fix case of filename
                                    MoveFileEx(pathList[i].GetWinPath(), retPath.GetWinPath(), MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED);
                                    // remove it from the list
                                    pathList.RemovePath(pathList[i]);
                                }
                                else if (ret != 2)
                                    return false;
                                break;
                            }
                        }
                    }
                } while ((s = status.GetNextFileStatus(retPath))!=0);
            }

            SVN svn;
            ProjectProperties props;
            props.ReadPropsPathList(pathList);
            bRet = !!svn.Add(pathList, &props, svn_depth_empty, false, false, true);
            if (!bRet)
            {
                CMessageBox::Show(hWndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
            }
            CShellUpdater::Instance().AddPathsForUpdate(pathList);
        }
        else
        {
            CAddDlg dlg;
            dlg.m_pathList = pathList;
            if (dlg.DoModal() == IDOK)
            {
                if (dlg.m_pathList.GetCount() == 0)
                    return FALSE;
                CSVNProgressDlg progDlg;
                theApp.m_pMainWnd = &progDlg;
                progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Add);
                progDlg.SetAutoClose (parser);
                progDlg.SetPathList(dlg.m_pathList);
                ProjectProperties props;
                props.ReadPropsPathList(dlg.m_pathList);
                progDlg.SetProjectProperties(props);
                progDlg.SetItemCount(dlg.m_pathList.GetCount());
                progDlg.DoModal();
                bRet = !progDlg.DidErrorsOccur();
            }
        }
    }
    return bRet;
}