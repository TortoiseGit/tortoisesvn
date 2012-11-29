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
#include "stdafx.h"
#include "ResolveCommand.h"

#include "ResolveDlg.h"
#include "SVNProgressDlg.h"

bool ResolveCommand::Execute()
{
    CResolveDlg dlg;
    dlg.m_pathList = pathList;
    INT_PTR ret = IDOK;
    if (!parser.HasKey(_T("noquestion")))
        ret = dlg.DoModal();
    if (ret == IDOK)
    {
        if (dlg.m_pathList.GetCount())
        {
            if (parser.HasKey(L"silent"))
            {
                SVN svn;
                bool bRet = true;
                for (auto i = 0; i < pathList.GetCount(); ++i)
                {
                    bRet = bRet && svn.Resolve(pathList[i], svn_wc_conflict_choose_merged, true);
                }

                HWND   resolveMsgWnd    = parser.HasVal(L"resolvemsghwnd")   ? (HWND)parser.GetLongLongVal(L"resolvemsghwnd")     : 0;
                WPARAM resolveMsgWParam = parser.HasVal(L"resolvemsgwparam") ? (WPARAM)parser.GetLongLongVal(L"resolvemsgwparam") : 0;
                LPARAM resolveMsgLParam = parser.HasVal(L"resolvemsglparam") ? (LPARAM)parser.GetLongLongVal(L"resolvemsglparam") : 0;

                if (resolveMsgWnd)
                {
                    static UINT WM_REVERTMSG = RegisterWindowMessage(_T("TORTOISESVN_RESOLVEDONE_MSG"));
                    ::PostMessage(resolveMsgWnd, WM_REVERTMSG, resolveMsgWParam, resolveMsgLParam);
                }

                return bRet;
            }
            else
            {
                CSVNProgressDlg progDlg(CWnd::FromHandle(GetExplorerHWND()));
                theApp.m_pMainWnd = &progDlg;
                progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Resolve);
                progDlg.SetAutoClose (parser);
                progDlg.SetOptions(parser.HasKey(_T("skipcheck")) ? ProgOptSkipConflictCheck : ProgOptNone);
                progDlg.SetPathList(dlg.m_pathList);
                progDlg.DoModal();
                return !progDlg.DidErrorsOccur();
            }
        }
    }
    return false;
}
