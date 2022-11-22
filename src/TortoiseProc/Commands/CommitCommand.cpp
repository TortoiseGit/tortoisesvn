// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2014, 2018, 2021-2022 - TortoiseSVN

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
#include "CommitCommand.h"

#include "CommitDlg.h"
#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "AppUtils.h"

CString CommitCommand::LoadLogMessage() const
{
    CString msg;
    if (parser.HasKey(L"logmsg"))
    {
        msg = parser.GetVal(L"logmsg");
    }
    if (parser.HasKey(L"logmsgfile"))
    {
        CString logMsgFile = parser.GetVal(L"logmsgfile");
        CStringUtils::ReadStringFromTextFile(logMsgFile, msg);
    }
    return msg;
}

void CommitCommand::InitProgressDialog(CCommitDlg& commitDlg, CSVNProgressDlg& progDlg) const
{
    progDlg.SetChangeList(commitDlg.m_sChangeList, !!commitDlg.m_bKeepChangeList);
    progDlg.SetAutoClose(parser);
    progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Commit);
    progDlg.SetOptions(commitDlg.m_bKeepLocks ? ProgOptKeeplocks : ProgOptNone);
    progDlg.SetPathList(commitDlg.m_pathList);
    progDlg.SetCommitMessage(commitDlg.m_sLogMessage);
    progDlg.SetDepth(commitDlg.m_bRecursive ? svn_depth_infinity : svn_depth_empty);
    progDlg.SetSelectedList(commitDlg.m_selectedPathList);
    progDlg.SetItemCount(commitDlg.m_itemsCount);
    progDlg.SetBugTraqProvider(commitDlg.m_bugTraqProvider);
    progDlg.SetRevisionProperties(commitDlg.m_revProps);
}

bool CommitCommand::IsOutOfDate(const svn_error_t* pErr)
{
    while (pErr)
    {
        const apr_status_t errorStatus = pErr->apr_err;
        if ((errorStatus == SVN_ERR_FS_TXN_OUT_OF_DATE) ||
            (errorStatus == SVN_ERR_RA_OUT_OF_DATE) ||
            (errorStatus == SVN_ERR_WC_NOT_UP_TO_DATE) ||
            (errorStatus == SVN_ERR_FS_CONFLICT))
        {
            return true;
        }
        pErr = pErr->child;
    }
    return false;
}

bool CommitCommand::Execute()
{
    bool          bRet    = false;
    bool          bRepeat = true;
    CTSVNPathList selectedList;
    if (parser.HasKey(L"logmsg") && (parser.HasKey(L"logmsgfile")))
    {
        TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_INVALIDPARAMS), MAKEINTRESOURCE(IDS_ERR_TWOLOGPARAMS), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
        return false;
    }
    CString           sLogMsg               = LoadLogMessage();
    bool              bSelectFilesForCommit = !!static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\SelectFilesForCommit", TRUE));
    DWORD             exitCode              = 0;
    CString           error;
    ProjectProperties props;
    props.ReadPropsPathList(pathList);
    CHooks::Instance().SetProjectProperties(pathList.GetCommonRoot(), props);
    if (CHooks::Instance().StartCommit(GetExplorerHWND(), pathList, sLogMsg, exitCode, error))
    {
        if (exitCode)
        {
            CString temp;
            temp.Format(IDS_ERR_HOOKFAILED, static_cast<LPCWSTR>(error));
            MessageBox(GetExplorerHWND(), temp, L"TortoiseSVN", MB_ICONERROR);
            return false;
        }
    }
    CTSVNPathList                                   updateList   = pathList;
    CTSVNPathList                                   origPathList = pathList;
    std::map<CString, std::tuple<CString, CString>> restorePaths;
    CString                                         sBugID;
    if (parser.HasKey(L"bugid"))
    {
        sBugID = parser.GetVal(L"bugid");
    }
    while (bRepeat)
    {
        bRepeat = false;
        CCommitDlg dlg;
        dlg.m_sBugID                = sBugID;
        dlg.m_projectProperties     = props;
        dlg.m_sLogMessage           = sLogMsg;
        dlg.m_pathList              = pathList;
        dlg.m_checkedPathList       = selectedList;
        dlg.m_bSelectFilesForCommit = bSelectFilesForCommit;
        dlg.m_restorePaths          = restorePaths;
        if (dlg.DoModal() == IDOK)
        {
            if (dlg.m_pathList.GetCount() == 0)
                return false;
            // if the user hasn't changed the list of selected items
            // we don't use that list. Because if we would use the list
            // of pre-checked items, the dialog would show different
            // checked items on the next startup: it would only try
            // to check the parent folder (which might not even show)
            // instead, we simply use an empty list and let the
            // default checking do its job.
            if (!dlg.m_pathList.IsEqual(pathList))
                selectedList = dlg.m_pathList;
            pathList              = dlg.m_updatedPathList;
            updateList            = dlg.m_updatedPathList;
            sLogMsg               = dlg.m_sLogMessage;
            sBugID                = dlg.m_sBugID;
            bSelectFilesForCommit = true;
            CSVNProgressDlg progDlg;
            InitProgressDialog(dlg, progDlg);
            progDlg.SetRestorePaths(dlg.m_restorePaths);
            restorePaths = dlg.m_restorePaths;
            progDlg.SetProjectProperties(props);
            progDlg.SetOrigPathList(origPathList);
            progDlg.DoModal();

            // FailRepeat can have these values:
            // 0 : like 1, but do not reopen the commit dialog
            // 1 : default. Ask to update and update the items selected for commit, and then reopen the commit dialog
            // 2 : Like 1, but update from the WC root
            // 3 : don't ask to update, just let the commit fail and have the user deal with the error.
            auto failRepeat = static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\OutOfDateRetry", 1));

            if (IsOutOfDate(progDlg.GetSVNError()) && (failRepeat != 3))
            {
                // the commit failed at least one of the items was outdated.
                // -> suggest to update them
                if (CAppUtils::AskToUpdate(nullptr, progDlg.GetLastErrorMessage(40)))
                {
                    // auto-update
                    CSVNProgressDlg updateProgDlg;
                    InitProgressDialog(dlg, updateProgDlg);
                    auto updatePaths = updateList;
                    if (failRepeat == 2)
                    {
                        SVN  svn;
                        auto root   = svn.GetWCRootFromPath(updateList[0]);
                        updatePaths = CTSVNPathList(root);
                    }
                    updateProgDlg.SetPathList(updatePaths);
                    updateProgDlg.SetCommand(CSVNProgressDlg::SVNProgress_Update);
                    // always update with svn_depth_unknown (the depth of the wc).
                    // because otherwise we would change the depth here which is
                    // not what we want!
                    updateProgDlg.SetDepth(svn_depth_unknown);
                    updateProgDlg.SetProjectProperties(props);
                    CRegDWORD updateExternals(L"Software\\TortoiseSVN\\IncludeExternals", true);
                    int       options = static_cast<DWORD>(updateExternals) ? 0 : ProgOptIgnoreExternals;
                    updateProgDlg.SetOptions(options);
                    updateProgDlg.DoModal();

                    // re-open commit dialog only if update *SUCCEEDED*
                    // (don't pass update errors to caller and *never*
                    // auto-reopen commit dialog upon error)

                    bRepeat                                                   = !updateProgDlg.DidErrorsOccur() && !updateProgDlg.DidConflictsOccur();
                    bRet                                                      = bRepeat;
                    CRegDWORD(L"Software\\TortoiseSVN\\ErrorOccurred", FALSE) = bRet ? TRUE : FALSE;

                    continue;
                }
            }

            // If there was an error and the user set the
            // "automatically re-open commit dialog" option, do so.

            CRegDWORD err(L"Software\\TortoiseSVN\\ErrorOccurred", FALSE);
            err     = static_cast<DWORD>(progDlg.DidErrorsOccur());
            bRepeat = progDlg.DidErrorsOccur();
            bRet    = !progDlg.DidErrorsOccur();
            if ((failRepeat == 0) || (failRepeat == 3))
                bRepeat = false; // do not repeat if the user chose not to in the settings.

            if (bRet)
            {
                // commit succeeded
                CRegDWORD bIncompleteReopen = CRegDWORD(L"Software\\TortoiseSVN\\IncompleteReopen", FALSE);
                if (static_cast<DWORD>(bIncompleteReopen))
                {
                    // user wants to reopen the commit dialog after a commit
                    // but we do this only if the commit wasn't complete, i.e. if not all files have
                    // been committed. So check for that first.
                    if (!dlg.m_bRecursive || !restorePaths.empty() || !dlg.m_sChangeList.IsEmpty() || dlg.m_bUnchecked)
                    {
                        // reopen the dialog
                        bRepeat = true;
                    }
                }
            }
            restorePaths = progDlg.GetRestorePaths();
        }
    }
    if (!restorePaths.empty())
    {
        SVN svn;
        for (auto it = restorePaths.cbegin(); it != restorePaths.cend(); ++it)
        {
            CopyFile(it->first, std::get<0>(it->second), FALSE);
            svn.AddToChangeList(CTSVNPathList(CTSVNPath(std::get<0>(it->second))), std::get<1>(it->second), svn_depth_empty);
        }
    }
    return bRet;
}