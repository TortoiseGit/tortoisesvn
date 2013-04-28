// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2011, 2013 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "RepoCreationFinished.h"
#include <afxdialogex.h>
#include "TempFile.h"
#include "SVN.h"
#include "AppUtils.h"
#include "PathUtils.h"


IMPLEMENT_DYNAMIC(CRepoCreationFinished, CStandAloneDialog)

    CRepoCreationFinished::CRepoCreationFinished(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CRepoCreationFinished::IDD, pParent)
{

}

CRepoCreationFinished::~CRepoCreationFinished()
{
}

void CRepoCreationFinished::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URL, m_RepoUrl);
}


BEGIN_MESSAGE_MAP(CRepoCreationFinished, CStandAloneDialog)
    ON_BN_CLICKED(IDC_CREATEFOLDERS, &CRepoCreationFinished::OnBnClickedCreatefolders)
    ON_BN_CLICKED(IDC_REPOBROWSER, &CRepoCreationFinished::OnBnClickedRepobrowser)
END_MESSAGE_MAP()



void CRepoCreationFinished::OnBnClickedCreatefolders()
{
    // create the default folder structure in a temp folder
    CTSVNPath tempDir = CTempFiles::Instance().GetTempDirPath(true);
    if (tempDir.IsEmpty())
    {
        ::MessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_ERR_CREATETEMPDIR)), CString(MAKEINTRESOURCE(IDS_APPNAME)), MB_ICONERROR);
        return;
    }
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
    if (m_RepoPath.GetWinPathString().GetAt(0) == '\\')    // starts with '\' means an UNC path
    {
        CString p = m_RepoPath.GetWinPathString();
        p.TrimLeft('\\');
        url = _T("file://")+p;
    }
    else
        url = _T("file:///")+m_RepoPath.GetWinPathString();

    // import the folder structure into the new repository
    SVN svn;
    if (!svn.Import(tempDir, CTSVNPath(url), CString(MAKEINTRESOURCE(IDS_MSG_IMPORTEDSTRUCTURE)), NULL, svn_depth_infinity, true, true, false))
    {
        svn.ShowErrorDialog(m_hWnd);
        return;
    }
    MessageBox(CString(MAKEINTRESOURCE(IDS_MSG_IMPORTEDSTRUCTUREFINISHED)), _T("TortoiseSVN"), MB_ICONINFORMATION);
    DialogEnableWindow(IDC_CREATEFOLDERS, FALSE);
}


void CRepoCreationFinished::OnBnClickedRepobrowser()
{
    CString sCmd;
    sCmd.Format(_T("/command:repobrowser /path:\"%s\""),
        (LPCTSTR)m_RepoPath.GetWinPath());

    CAppUtils::RunTortoiseProc(sCmd);
}


BOOL CRepoCreationFinished::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    CString url;
    if (m_RepoPath.GetWinPathString().GetAt(0) == '\\')    // starts with '\' means an UNC path
    {
        CString p = m_RepoPath.GetWinPathString();
        p.TrimLeft('\\');
        url = _T("file://")+p;
    }
    else
        url = _T("file:///")+m_RepoPath.GetWinPathString();

    m_RepoUrl.SetWindowText(url);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
