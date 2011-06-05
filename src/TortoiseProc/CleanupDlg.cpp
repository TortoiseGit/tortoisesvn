// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011 - TortoiseSVN

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
#include "CleanupDlg.h"
#include "afxdialogex.h"
#include "AppUtils.h"


// CCleanupDlg dialog

IMPLEMENT_DYNAMIC(CCleanupDlg, CStandAloneDialog)

CCleanupDlg::CCleanupDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CCleanupDlg::IDD, pParent)
    , m_bCleanup(FALSE)
    , m_bRevert(FALSE)
    , m_bDelUnversioned(FALSE)
    , m_bRefreshShell(FALSE)
{

}

CCleanupDlg::~CCleanupDlg()
{
}

void CCleanupDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_CLEANUP, m_bCleanup);
    DDX_Check(pDX, IDC_REVERT, m_bRevert);
    DDX_Check(pDX, IDC_DELETEUNVERSIONED, m_bDelUnversioned);
    DDX_Check(pDX, IDC_REFRESHSHELL, m_bRefreshShell);
}


BEGIN_MESSAGE_MAP(CCleanupDlg, CStandAloneDialog)
    ON_BN_CLICKED(IDHELP, &CCleanupDlg::OnBnClickedHelp)
    ON_BN_CLICKED(IDC_CLEANUP, &CCleanupDlg::OnBnClickedCleanup)
    ON_BN_CLICKED(IDC_REVERT, &CCleanupDlg::OnBnClickedRevert)
    ON_BN_CLICKED(IDC_DELETEUNVERSIONED, &CCleanupDlg::OnBnClickedDeleteunversioned)
    ON_BN_CLICKED(IDC_REFRESHSHELL, &CCleanupDlg::OnBnClickedRefreshshell)
END_MESSAGE_MAP()


// CCleanupDlg message handlers


BOOL CCleanupDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(0);
    m_aeroControls.SubclassControl(this, IDC_CLEANUP);
    m_aeroControls.SubclassControl(this, IDC_REVERT);
    m_aeroControls.SubclassControl(this, IDC_DELETEUNVERSIONED);
    m_aeroControls.SubclassControl(this, IDC_REFRESHSHELL);
    m_aeroControls.SubclassOkCancelHelp(this);

    AdjustControlSize(IDC_CLEANUP);
    AdjustControlSize(IDC_REVERT);
    AdjustControlSize(IDC_DELETEUNVERSIONED);
    AdjustControlSize(IDC_REFRESHSHELL);

    m_bCleanup = true;
    UpdateData(FALSE);
    EnableOKButton();

    m_tooltips.Create(this);

    return TRUE;
}

BOOL CCleanupDlg::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return CStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCleanupDlg::EnableOKButton()
{
    UpdateData();
    DialogEnableWindow(IDOK, m_bCleanup || m_bRevert || m_bDelUnversioned || m_bRefreshShell);
}

void CCleanupDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CCleanupDlg::OnBnClickedCleanup()
{
    EnableOKButton();
}

void CCleanupDlg::OnBnClickedRevert()
{
    EnableOKButton();
}

void CCleanupDlg::OnBnClickedDeleteunversioned()
{
    EnableOKButton();
}

void CCleanupDlg::OnBnClickedRefreshshell()
{
    EnableOKButton();
}
