// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012, 2014-2016, 2021 - TortoiseSVN

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
#include "ExportCommand.h"

#include "ExportDlg.h"
#include "SVNProgressDlg.h"
#include "SVNAdminDir.h"
#include "ProgressDlg.h"
#include "BrowseFolder.h"
#include "DirFileEnum.h"
#include "SVNStatus.h"
#include "SVN.h"
#include "TortoiseProc.h"

bool ExportCommand::Execute()
{
    bool bRet = false;
    // When the user clicked on a working copy, we know that the export should
    // be done from that. We then have to ask where the export should go to.
    // If however the user clicked on an unversioned folder, we assume that
    // this is where the export should go to and have to ask from where
    // the export should be done from.
    bool               bURL = !!SVN::PathIsURL(cmdLinePath);
    svn_wc_status_kind s    = SVNStatus::GetAllStatus(cmdLinePath);
    if ((bURL) || (s == svn_wc_status_unversioned) || (s == svn_wc_status_none))
    {
        // ask from where the export has to be done
        CExportDlg dlg;
        if (bURL)
            dlg.m_url = cmdLinePath.GetSVNPathString();
        else
            dlg.m_strExportDirectory = cmdLinePath.GetWinPathString();
        if (parser.HasKey(L"revision"))
        {
            SVNRev rev   = SVNRev(parser.GetVal(L"revision"));
            dlg.m_revision = rev;
        }
        dlg.m_blockPathAdjustments = parser.HasKey(L"blockpathadjustments");
        if (dlg.DoModal() == IDOK)
        {
            CTSVNPath exportPath(dlg.m_strExportDirectory);

            CSVNProgressDlg progDlg;
            theApp.m_pMainWnd = &progDlg;
            progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Export);
            progDlg.SetAutoClose(parser);
            DWORD options = dlg.m_bNoExternals ? ProgOptIgnoreExternals : ProgOptNone;
            options |= dlg.m_bNoKeywords ? ProgOptIgnoreKeywords : ProgOptNone;
            if (dlg.m_eolStyle.CompareNoCase(L"CRLF") == 0)
                options |= ProgOptEolCRLF;
            if (dlg.m_eolStyle.CompareNoCase(L"CR") == 0)
                options |= ProgOptEolCR;
            if (dlg.m_eolStyle.CompareNoCase(L"LF") == 0)
                options |= ProgOptEolLF;
            progDlg.SetOptions(options);
            progDlg.SetPathList(CTSVNPathList(exportPath));
            progDlg.SetUrl(dlg.m_url);
            progDlg.SetRevision(dlg.m_revision);
            progDlg.SetDepth(dlg.m_depth);
            progDlg.DoModal();
            bRet = !progDlg.DidErrorsOccur();
        }
    }
    else
    {
        // ask where the export should go to.
        CBrowseFolder folderBrowser;
        folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_PROC_EXPORT_1)));
        folderBrowser.m_style = BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_VALIDATE | BIF_EDITBOX;
        folderBrowser.SetCheckBoxText(CString(MAKEINTRESOURCE(IDS_PROC_EXPORT_2)));
        folderBrowser.SetCheckBoxText2(CString(MAKEINTRESOURCE(IDS_PROC_OMMITEXTERNALS)));
        folderBrowser.DisableCheckBox2WhenCheckbox1IsEnabled(true);
        CRegDWORD regExtended   = CRegDWORD(L"Software\\TortoiseSVN\\ExportExtended", FALSE);
        CBrowseFolder::m_bCheck = regExtended;
        wchar_t saveTo[MAX_PATH] = {0};
        if (folderBrowser.Show(GetExplorerHWND(), saveTo, _countof(saveTo)) == CBrowseFolder::OK)
        {
            CString saveplace = CString(saveTo);

            if (cmdLinePath.IsEquivalentTo(CTSVNPath(saveplace)))
            {
                // exporting to itself:
                // remove all svn admin dirs, effectively unversion the 'exported' folder.
                CString msg;
                msg.Format(IDS_PROC_EXPORTUNVERSION, static_cast<LPCWSTR>(saveplace));
                CTaskDialog taskDlg(msg,
                                    CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK3)));
                taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_PROC_EXPORTUNVERSION_TASK4)));
                taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskDlg.SetDefaultCommandControl(100);
                taskDlg.SetMainIcon(TD_WARNING_ICON);
                bool bUnversion = (taskDlg.DoModal(GetExplorerHWND()) == 100);
                if (bUnversion)
                {
                    CProgressDlg progress;
                    progress.SetTitle(IDS_PROC_UNVERSION);
                    progress.FormatNonPathLine(1, IDS_SVNPROGRESS_EXPORTINGWAIT);
                    progress.SetTime(true);
                    progress.ShowModeless(GetExplorerHWND());
                    std::vector<CTSVNPath> removeVector;

                    CDirFileEnum lister(saveplace);
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
                    bRet = true;
                }
                else
                    return false;
            }
            else
            {
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": export %s to %s\n", static_cast<LPCWSTR>(cmdLinePath.GetUIPathString()), static_cast<LPCWSTR>(saveTo));
                SVN svn;
                if (!svn.Export(cmdLinePath, CTSVNPath(saveplace), SVNRev::REV_WC,
                                SVNRev::REV_WC, false, !!folderBrowser.m_bCheck2, false, svn_depth_infinity,
                                GetExplorerHWND(), folderBrowser.m_bCheck ? SVN::SVNExportIncludeUnversioned : SVN::SVNExportNormal))
                {
                    svn.ShowErrorDialog(GetExplorerHWND(), cmdLinePath);
                    bRet = false;
                }
                else
                    bRet = true;
                regExtended = CBrowseFolder::m_bCheck;
            }
        }
    }
    return bRet;
}
