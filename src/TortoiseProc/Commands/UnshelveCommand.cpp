// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2017 - TortoiseSVN

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
#include "UnshelveCommand.h"

#include "PathUtils.h"
#include "StringUtils.h"
#include "AppUtils.h"
#include "UnshelveDlg.h"
#include "SVN.h"
#include "TempFile.h"
#include "ProgressDlg.h"
#include "SelectFileFilter.h"
#include "SmartHandle.h"
#include "PreserveChdir.h"





bool UnshelveCommand::Execute()
{
    bool bRet = false;
    SVN svn;
    CString name;

    // use the passed-in name, if given
    if (parser.HasKey(L"shelvename"))
    {
        name = parser.GetVal(L"shelvename");
    }
    // else show a dialog to select a name
    else if (!parser.HasKey(L"noui"))
    {
        CUnshelve dlg;
        dlg.m_pathList = pathList;
        // get the list of shelved names
        if (!svn.ShelvesList(dlg.m_Names, cmdLinePath))
        {
            svn.ShowErrorDialog(GetExplorerHWND(), cmdLinePath);
            return FALSE;
        }
        if (dlg.m_Names.empty())
        {
            CString temp;
            temp.LoadStringW(IDS_NOTHING_SHELVED);
            ::MessageBox(GetExplorerHWND(), temp, L"TortoiseSVN", MB_ICONERROR);
            return FALSE;
        }
        if (dlg.DoModal() == IDOK)
        {
            name = dlg.m_sShelveName;
        }
    }
    if (cmdLinePath.IsEmpty() || name.IsEmpty())
    {
        return FALSE;
    }
    bRet = Unshelve(name, cmdLinePath);

    return bRet;
}

bool UnshelveCommand::Unshelve(const CString& shelveName, const CTSVNPath &sDir)
{
    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_PROC_PATCHTITLE);
    progDlg.SetShowProgressBar(false);
    progDlg.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));

    SVN svn;
    if (!svn.Unshelve(shelveName, sDir))
    {
        progDlg.Stop();
        svn.ShowErrorDialog(GetExplorerHWND(), sDir);
        return FALSE;
    }

    progDlg.Stop();

    return TRUE;
}
