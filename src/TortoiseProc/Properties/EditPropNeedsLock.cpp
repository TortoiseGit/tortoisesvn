// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2011 - TortoiseSVN

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
#include "EditPropNeedsLock.h"


// CEditPropNeedsLock dialog

IMPLEMENT_DYNAMIC(CEditPropNeedsLock, CStandAloneDialog)

CEditPropNeedsLock::CEditPropNeedsLock(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CEditPropNeedsLock::IDD, pParent)
    , EditPropBase()
{

}

CEditPropNeedsLock::~CEditPropNeedsLock()
{
}

void CEditPropNeedsLock::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropNeedsLock, CStandAloneDialog)
    ON_BN_CLICKED(IDC_PROPRECURSIVE, &CEditPropNeedsLock::OnBnClickedProprecursive)
    ON_BN_CLICKED(IDHELP, &CEditPropNeedsLock::OnBnClickedHelp)
END_MESSAGE_MAP()

BOOL CEditPropNeedsLock::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(0,0,0,0);
    m_aeroControls.SubclassControl(this, IDC_PROPSET);
    m_aeroControls.SubclassControl(this, IDC_PROPNOTSET);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    AdjustControlSize(IDC_PROPSET);
    AdjustControlSize(IDC_PROPNOTSET);
    AdjustControlSize(IDC_PROPRECURSIVE);

    CheckRadioButton(IDC_PROPSET, IDC_PROPNOTSET, m_PropValue.size() ? IDC_PROPSET : IDC_PROPNOTSET);

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(m_bFolder || m_bMultiple);
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps ? SW_HIDE : SW_SHOW);
    if (m_bFolder)
    {
        // for folders, the property can only be set recursively
        m_bRecursive = TRUE;
        UpdateData(false);
    }

    return TRUE;
}

void CEditPropNeedsLock::OnOK()
{
    UpdateData();

    bool bSet = (GetCheckedRadioButton(IDC_PROPSET, IDC_PROPNOTSET) == IDC_PROPSET);

    if (bSet)
        m_PropValue = "*";
    else
        m_PropValue.clear();
    m_bChanged = true;

    CStandAloneDialog::OnOK();
}

void CEditPropNeedsLock::OnBnClickedProprecursive()
{
    UpdateData();
    if (m_bFolder)
        m_bRecursive = TRUE;
    UpdateData(false);
}


void CEditPropNeedsLock::OnBnClickedHelp()
{
    OnHelp();
}
