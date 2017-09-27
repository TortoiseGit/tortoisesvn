// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2016 - TortoiseSVN

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
    bool bRet = false;
    CString shelveName = CPathUtils::GetLongPathname(parser.GetVal(L"shelvename"));
    CShelve dlg;
    dlg.m_pathList = pathList;
    if (parser.HasKey(L"noui")||(dlg.DoModal()==IDOK))
    {
        if (cmdLinePath.IsEmpty())
        {
            cmdLinePath = pathList.GetCommonRoot();
        }
        bRet = Shelve(CString(shelveName), dlg.m_pathList);
    }
    return bRet;
}

bool ShelveCommand::Shelve(const CString& cmdLineShelveName, const CTSVNPathList& paths)
{
	CString shelveName;

    if (cmdLineShelveName.IsEmpty())
    {
		shelveName = CString("test");  // ###
	}
    else
    {
        shelveName = cmdLineShelveName;
    }

    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_PROC_PATCHTITLE);
    progDlg.SetShowProgressBar(false);
    progDlg.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));

	CTSVNPath sDir = paths.GetCommonRoot();
	SVN svn;
    if (!svn.Shelve(shelveName, paths, svn_depth_infinity /*, changelists*/))
    {
        progDlg.Stop();
        svn.ShowErrorDialog(GetExplorerHWND(), sDir);
        return FALSE;
    }

    progDlg.Stop();

    return TRUE;
}
