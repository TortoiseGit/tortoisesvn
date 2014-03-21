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
#include "PrevDiffCommand.h"
#include "ChangedDlg.h"
#include "SVNDiff.h"
#include "SVNStatus.h"
#include "AppUtils.h"

bool PrevDiffCommand::Execute()
{
    bool bRet = false;
    bool bAlternativeTool = !!parser.HasKey(L"alternative");
    if (cmdLinePath.IsDirectory())
    {
        CChangedDlg dlg;
        dlg.m_pathList = CTSVNPathList(cmdLinePath);
        dlg.DoModal();
        bRet = true;
    }
    else
    {
        SVNDiff diff(NULL, GetExplorerHWND());
        diff.SetAlternativeTool(bAlternativeTool);
        diff.SetJumpLine(parser.GetLongVal(L"line"));
        SVNStatus st;
        st.GetStatus(cmdLinePath);
        if (st.status && st.status->changed_rev)
        {
            bool ignoreprops = !!parser.HasKey(L"ignoreprops");
            bRet = diff.ShowCompare(cmdLinePath, SVNRev::REV_WC, cmdLinePath, st.status->changed_rev - 1, st.status->changed_rev, ignoreprops, L"", false, false, st.status->kind);
        }
        else
        {
            if (st.GetLastErrorMessage().IsEmpty())
            {
                TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_NOPREVREVISION), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
            }
            else
            {
                TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_NOSTATUS), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
            }
        }
    }
    return bRet;
}
