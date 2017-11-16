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
#include "TortoiseProc.h"
#include "UnshelveDlg.h"
#include "SVN.h"
#include "AppUtils.h"

#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CUnshelve, CResizableStandAloneDialog)
CUnshelve::CUnshelve(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CUnshelve::IDD, pParent)
{
}

CUnshelve::~CUnshelve()
{
}

void CUnshelve::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SHELVENAME1, m_ShelveNameCombo);
}


BEGIN_MESSAGE_MAP(CUnshelve, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()

BOOL CUnshelve::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    DialogEnableWindow(IDHELP, false);

    // populate the list of shelved changes and select the first one
    for (auto i = m_Names.begin(); i != m_Names.end(); ++i)
    {
        m_ShelveNameCombo.AddString(*i);
    }
    m_ShelveNameCombo.SetCurSel(0);

    UpdateData(FALSE);

    AddAnchor(IDC_NAMELABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SHELVENAME1, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    BlockResize(DIALOG_BLOCKVERTICAL);
    EnableSaveRestore(L"UnshelveDlg");

    return TRUE;
}

void CUnshelve::OnBnClickedHelp()
{
    OnHelp();
}

void CUnshelve::OnCancel()
{
    CResizableStandAloneDialog::OnCancel();
}

void CUnshelve::OnOK()
{
    CResizableStandAloneDialog::OnOK();

    int selectedName = m_ShelveNameCombo.GetCurSel();
    if (selectedName >= 0)
    {
        m_ShelveNameCombo.GetLBText(selectedName, m_sShelveName);
    }
}
