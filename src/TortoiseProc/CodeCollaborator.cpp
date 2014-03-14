// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseSVN

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
#include "StringUtils.h"

CodeCollaboratorInfo::CodeCollaboratorInfo(CString revisions, CString repoUrl)
{
    CollabUser = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\CollabUser", L"");
    CString encrypted = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\CollabPassword", L"");
    if (encrypted.IsEmpty())
        CollabPassword = L"";
    else
        CollabPassword = CStringUtils::Decrypt((LPCWSTR)encrypted);
    SvnUser = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\SvnUser", L"");
    encrypted = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\SvnPassword", L"");
    if (encrypted.IsEmpty())
        SvnPassword = L"";
    else
        SvnPassword = CStringUtils::Decrypt((LPCWSTR)encrypted);
    RepoUrl = repoUrl;
    m_Revisions = revisions;
}

CString CodeCollaboratorInfo::GetPathToCollabGuiExe()
{
    // this will work for X86 and X64 if the matching 'bitness' client has been installed
    PWSTR pszPath = NULL;
    if (SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_CREATE, NULL, &pszPath) != S_OK)
        return CString();

    CString path = pszPath;
    CoTaskMemFree(pszPath);

    path += L"Collaborator Client\\ccollabgui.exe";
    if (PathFileExists(path))
        return path;

#ifdef _WIN64
    // if running on x64 OS, but installed X86 client - get try getting directory from there
    // on X86 this just returns %ProgramFiles%
    if (SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, KF_FLAG_CREATE, NULL, &pszPath) != S_OK)
        return CString();

    path += L"Collaborator Client\\ccollabgui.exe";
    if (PathFileExists(path))
        return path;
#endif
    return CString(L"");
}

bool CodeCollaboratorInfo::IsInstalled()
{
    return !(GetPathToCollabGuiExe().IsEmpty());
}

CString CodeCollaboratorInfo::GetCommandLine()
{
    CString arguments;
    if (!((CString)SvnUser).IsEmpty())
    {
        arguments.Format(L"%s --user %s --password %s --scm svn --svn-repo-url %s --svn-user %s --svn-passwd %s addchangelist new %s",
            (LPCWSTR)GetPathToCollabGuiExe(),
            (LPCWSTR)(CString)CollabUser, (LPCWSTR)(CString)CollabPassword, (LPCWSTR)RepoUrl,
            (LPCWSTR)(CString)SvnUser,(LPCWSTR)(CString)SvnPassword, (LPCWSTR)m_Revisions);
    }
    else
    {
        // allow for anonymous svn access
        arguments.Format(L"%s --user %s --password %s --scm svn --svn-repo-url %s addchangelist new %s",
            (LPCWSTR)GetPathToCollabGuiExe(),
            (LPCWSTR)(CString)CollabUser, (LPCWSTR)(CString)CollabPassword, (LPCWSTR)RepoUrl,
            (LPCWSTR)m_Revisions);
    }
    return arguments;
}

bool CodeCollaboratorInfo::IsUserInfoSet()
{
    return !((CString)CollabUser).IsEmpty();
}

