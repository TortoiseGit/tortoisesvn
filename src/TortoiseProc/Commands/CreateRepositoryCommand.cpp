// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2011, 2014 - TortoiseSVN

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
#include "CreateRepositoryCommand.h"

#include "AppUtils.h"
#include "SVN.h"
#include "TempFile.h"
#include "IconExtractor.h"
#include "RepoCreationFinished.h"
#include "SmartHandle.h"

bool CreateRepositoryCommand::Execute()
{
    if (!SVN::CreateRepository(cmdLinePath))
    {
        TSVNMessageBox(GetExplorerHWND(), IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
        return false;
    }
    else
    {
        // create a desktop.ini file which sets our own icon for the repo folder
        // we extract the icon to use from the resources and write it to disk
        // so even those who don't have TSVN installed can benefit from it.
        CIconExtractor svnIconResource;
        if (svnIconResource.ExtractIcon(NULL, MAKEINTRESOURCE(IDI_SVNFOLDER), cmdLinePath.GetWinPathString() + L"\\svn.ico") == 0)
        {
            DWORD dwWritten = 0;
            CAutoFile hFile = CreateFile(cmdLinePath.GetWinPathString() + L"\\Desktop.ini", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN, NULL);
            if (hFile)
            {
                CString sIni = L"[.ShellClassInfo]\nConfirmFileOp=0\nIconFile=svn.ico\nIconIndex=0\nInfoTip=Subversion Repository\n";
                WriteFile(hFile, (LPCTSTR)sIni,  sIni.GetLength()*sizeof(TCHAR), &dwWritten, NULL);
            }
            PathMakeSystemFolder(cmdLinePath.GetWinPath());
        }

        CWnd parent;
        parent.FromHandle(GetExplorerHWND());
        CRepoCreationFinished finDlg(&parent);
        finDlg.SetRepoPath(cmdLinePath);
        finDlg.DoModal();
    }
    return true;
}


