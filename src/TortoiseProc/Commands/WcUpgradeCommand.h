// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2016, 2021-2022 - TortoiseSVN

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
#pragma once
#include "Command.h"
#include "SVN.h"
#include "ProgressDlg.h"
#include "../../TSVNCache/CacheInterface.h"

/**
 * \ingroup TortoiseProc
 * Upgrades the working copy to the new format
 */
class WcUpgradeCommand : public Command
    , SVN
{
public:
    /**
     * Executes the command.
     */
    bool Execute() override
    {
        bool        bUpgrade = false;
        CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_PROC_UPGRADECONFIRMATION_TASK1)),
                            CString(MAKEINTRESOURCE(IDS_PROC_UPGRADECONFIRMATION_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_PROC_UPGRADECONFIRMATION_TASK3)));
        taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_PROC_UPGRADECONFIRMATION_TASK4)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_PROC_UPGRADECONFIRMATION_TASK5)));
        taskDlg.SetDefaultCommandControl(200);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        if (taskDlg.DoModal(GetExplorerHWND()) == 100)
            bUpgrade = true;
        if (bUpgrade)
        {
            progress.SetTitle(IDS_PROC_UPGRADE);
            progress.SetShowProgressBar(false);
            for (int i = 0; i < pathList.GetCount(); ++i)
            {
                progress.FormatPathLine(2, IDS_PROC_UPGRADE_INFO, pathList[i].GetWinPath());
                progress.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO2)));
                progress.ShowModeless(GetExplorerHWND());
                CBlockCacheForPath cacheBlock(pathList[i].GetWinPath());
                if (!Upgrade(pathList[i], nullptr))
                {
                    progress.Stop();
                    ShowErrorDialog(GetExplorerHWND());
                    return false;
                }
            }
            progress.Stop();
            return true;
        }
        return false;
    }

    BOOL Notify(const CTSVNPath& path, const CTSVNPath& /*url*/, svn_wc_notify_action_t /*action*/,
                svn_node_kind_t /*kind*/, const CString& /*mime_type*/,
                svn_wc_notify_state_t /*content_state*/,
                svn_wc_notify_state_t /*prop_state*/, svn_revnum_t /*rev*/,
                const svn_lock_t* /*lock*/, svn_wc_notify_lock_state_t /*lock_state*/,
                const CString& /*changelistname*/,
                const CString& /*propertyName*/,
                svn_merge_range_t* /*range*/,
                svn_error_t* /*err*/, apr_pool_t* /*pool*/) override
    {
        progress.FormatPathLine(2, IDS_PROC_UPGRADE_INFO, path.GetWinPath());
        return TRUE;
    }

private:
    CProgressDlg progress;
};
