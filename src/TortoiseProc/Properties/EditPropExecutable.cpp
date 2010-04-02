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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "EditPropExecutable.h"


// CEditPropExecutable dialog

IMPLEMENT_DYNAMIC(CEditPropExecutable, CStandAloneDialog)

CEditPropExecutable::CEditPropExecutable(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CEditPropExecutable::IDD, pParent)
{

}

CEditPropExecutable::~CEditPropExecutable()
{
}

void CEditPropExecutable::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropExecutable, CStandAloneDialog)
    ON_BN_CLICKED(IDC_PROPRECURSIVE, &CEditPropExecutable::OnBnClickedProprecursive)
END_MESSAGE_MAP()

BOOL CEditPropExecutable::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(0,0,0,0);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancel(this);

    AdjustControlSize(IDC_PROPSET);
    AdjustControlSize(IDC_PROPNOTSET);
    AdjustControlSize(IDC_PROPRECURSIVE);

    CheckRadioButton(IDC_PROPSET, IDC_PROPNOTSET, m_PropValue.size() ? IDC_PROPSET : IDC_PROPNOTSET);

    if (m_bFolder)
    {
        // for folders, the property can only be set recursively
        m_bRecursive = TRUE;
        UpdateData(false);
    }

    return TRUE;
}

void CEditPropExecutable::OnOK()
{
    UpdateData();

    bool bSet = (GetCheckedRadioButton(IDC_PROPSET, IDC_PROPNOTSET) == IDC_PROPSET);

    if (bSet && m_PropValue.size() == 0)
        m_bChanged = true;
    if (!bSet && m_PropValue.size())
        m_bChanged = true;

    if (bSet)
        m_PropValue = "*";
    else
        m_PropValue.empty();

    CStandAloneDialog::OnOK();
}

void CEditPropExecutable::OnBnClickedProprecursive()
{
    UpdateData();
    if (m_bFolder)
        m_bRecursive = TRUE;
    UpdateData(false);
}
