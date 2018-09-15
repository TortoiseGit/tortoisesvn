// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2017-2018 - TortoiseSVN

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
#include "ShelveCommand.h"

#include "PathUtils.h"
#include "StringUtils.h"
#include "AppUtils.h"
#include "ShelveDlg.h"
#include "SVN.h"
#include "TempFile.h"
#include "ProgressDlg.h"
#include "SelectFileFilter.h"
#include "SmartHandle.h"
#include "PreserveChdir.h"

bool ShelveCommand::Execute()
{
    bool    bRet = false;
    CShelve dlg;
    dlg.m_pathList = pathList;

    if (parser.HasKey(L"shelfname"))
    {
        dlg.m_sShelveName = parser.GetVal(L"shelfname");
        dlg.m_revert      = !parser.HasKey(L"checkpoint");
        if (parser.HasKey(L"logmsg"))
        {
            dlg.m_sLogMsg = parser.GetVal(L"logmsg");
        }
    }
    if (parser.HasKey(L"noui") || (dlg.DoModal() == IDOK))
    {
        if (cmdLinePath.IsEmpty())
        {
            cmdLinePath = pathList.GetCommonRoot();
        }
        bRet = Shelve(dlg.m_sShelveName, dlg.m_pathList, dlg.m_sLogMsg, dlg.m_revert);
    }
    return bRet;
}

bool ShelveCommand::Shelve(const CString& shelveName, const CTSVNPathList& paths, const CString& logMsg, bool revert)
{
    CProgressDlg progDlg;
    CString      sTitle;
    if (revert)
        sTitle.Format(IDS_SHELF_SHELVING, (LPCWSTR)shelveName);
    else
        sTitle.Format(IDS_SHELF_CHECKPOINTING, (LPCWSTR)shelveName);
    progDlg.SetTitle(sTitle);
    progDlg.SetShowProgressBar(false);
    progDlg.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));

    CTSVNPath sDir = paths.GetCommonRoot();
    SVN       svn;
    svn.SetAndClearProgressInfo(&progDlg);
    if (!svn.Shelve(shelveName, paths, logMsg, svn_depth_infinity, revert))
    {
        progDlg.Stop();
        svn.ShowErrorDialog(GetExplorerHWND(), sDir);
        return FALSE;
    }

    progDlg.Stop();

    return TRUE;
}
