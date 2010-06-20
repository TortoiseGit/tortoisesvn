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
#include "StdAfx.h"
#include "CreateRepositoryCommand.h"

#include "MessageBox.h"
#include "SVN.h"
#include "TempFile.h"
#include "IconExtractor.h"

bool CreateRepositoryCommand::Execute()
{
    if (!SVN::CreateRepository(cmdLinePath))
    {
        CMessageBox::Show(hwndExplorer, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
        return false;
    }
    else
    {
        // create a desktop.ini file which sets our own icon for the repo folder
        // we extract the icon to use from the resources and write it to disk
        // so even those who don't have TSVN installed can benefit from it.
        CIconExtractor svnIconResource;
        if (svnIconResource.ExtractIcon(NULL, MAKEINTRESOURCE(IDI_SVNFOLDER), cmdLinePath.GetWinPathString() + _T("\\svn.ico")) == 0)
        {
            DWORD dwWritten = 0;
            HANDLE hFile = CreateFile(cmdLinePath.GetWinPathString() + _T("\\Desktop.ini"), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                CString sIni = _T("[.ShellClassInfo]\nConfirmFileOp=0\nIconFile=svn.ico\nIconIndex=0\nInfoTip=Subversion Repository\n");
                WriteFile(hFile, (LPCTSTR)sIni,  sIni.GetLength()*sizeof(TCHAR), &dwWritten, NULL);
                CloseHandle(hFile);
            }
            PathMakeSystemFolder(cmdLinePath.GetWinPath());
        }

        if (CMessageBox::Show(hwndExplorer, IDS_PROC_REPOCREATEFINISHED, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            // create the default folder structure in a temp folder
            CTSVNPath tempDir = CTempFiles::Instance().GetTempDirPath(true);
            CTSVNPath tempDirSub = tempDir;
            tempDirSub.AppendPathString(_T("trunk"));
            CreateDirectory(tempDirSub.GetWinPath(), NULL);
            tempDirSub = tempDir;
            tempDirSub.AppendPathString(_T("branches"));
            CreateDirectory(tempDirSub.GetWinPath(), NULL);
            tempDirSub = tempDir;
            tempDirSub.AppendPathString(_T("tags"));
            CreateDirectory(tempDirSub.GetWinPath(), NULL);

            CString url;
            if (cmdLinePath.GetWinPathString().GetAt(0) == '\\')    // starts with '\' means an UNC path
            {
                CString p = cmdLinePath.GetWinPathString();
                p.TrimLeft('\\');
                url = _T("file://")+p;
            }
            else
                url = _T("file:///")+cmdLinePath.GetWinPathString();
            
            // import the folder structure into the new repository
            SVN svn;
            if (!svn.Import(tempDir, CTSVNPath(url), CString(MAKEINTRESOURCE(IDS_MSG_IMPORTEDSTRUCTURE)), NULL, svn_depth_infinity, true, false))
            {
                CMessageBox::Show(hwndExplorer, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
                return false;
            }
            // TODO: should we show another 'created structure successfully' dialog here?
        }
    }
    return true;
}


