// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * Upgrades the working copy to the new format
 */
class WcUpgradeCommand : public Command
{
public:
    /**
     * Executes the command.
     */
    virtual bool            Execute()
    {
        SVN svn;
        CProgressDlg progress;
        CString tmp;
        progress.SetTitle(IDS_PROC_UPGRADE);
        progress.SetShowProgressBar(false);
        for (int i = 0; i < pathList.GetCount(); ++i)
        {
            tmp.FormatMessage(IDS_PROC_UPGRADE_INFO, pathList[i].GetWinPath());
            progress.SetLine(1, tmp);
            progress.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROC_CLEANUP_INFO2)));
            progress.ShowModeless(hwndExplorer);
            if (!svn.Upgrade(pathList[i]))
            {
                progress.Stop();
                ::MessageBox(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
                return false;
            }
        }
        progress.Stop();
        return true;
    }
};


