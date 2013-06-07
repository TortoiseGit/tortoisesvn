// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

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
#include "CodeCollaborator.h"

/*
Example registry file contents required for Code Collaborator menu item to show...
Windows Registry Editor Version 5.00

    [HKEY_CURRENT_USER\Software\TortoiseSVN\CodeCollaborator]
    "PathToCollabGui"="C:\\Program Files\\Collaborator Client\\ccollabgui.exe"
    "CollabUser"="collab_user"
    "CollabPassword"="secret1"
    "RepoUrl"="http://ntsvn1.sciex.com/cliquid/Helium/Trunk"
    "SvnUser"="svn_user"
    "SvnPassword"="Secret2"
*/

CodeCollaboratorInfo::CodeCollaboratorInfo(CString revisions)
{
    PathToCollabGui =
        CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\PathToCollabGui", L"");
    CollabUser      =
        CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\CollabUser", L"");
    CollabPassword  =
        CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\CollabPassword", L"");
    RepoUrl         =
        CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\RepoUrl", L"");
    SvnUser         =
        CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\SvnUser", L"");
    SvnPassword     =
        CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\SvnPassword", L"");

    m_Revisions = revisions;
}

bool CodeCollaboratorInfo::IsInstalled()
{
    // Special Registry item must be set and CollabGui.exe File must exist
    if (((CString)PathToCollabGui).IsEmpty() ||
            !PathFileExists((LPCWSTR)(CString)PathToCollabGui))
        return false;
    return true;
}

CString CodeCollaboratorInfo::GetCommandLineArguments()
{
    CString arguments;
    arguments.Format(L"--user %s --password %s --scm svn --svn-repo-url %s --svn-user %s --svn-passwd %s addchangelist new %s",
        (LPCWSTR)(CString)CollabUser, (LPCWSTR)(CString)CollabPassword, (LPCWSTR)(CString)RepoUrl,
        (LPCWSTR)(CString)SvnUser,(LPCWSTR)(CString)SvnPassword, (LPCWSTR)m_Revisions);
    return arguments;
}
