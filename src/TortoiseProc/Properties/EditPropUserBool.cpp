// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2013 - TortoiseSVN

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
#include "EditPropUserBool.h"
#include "afxdialogex.h"
#include "AppUtils.h"
#include "UnicodeUtils.h"

// EditPropUserBool dialog

IMPLEMENT_DYNAMIC(EditPropUserBool, CStandAloneDialog)

EditPropUserBool::EditPropUserBool(CWnd* pParent, const UserProp * p)
    : CStandAloneDialog(EditPropUserBool::IDD, pParent)
    , EditPropBase()
    , m_sLabel(_T(""))
    , m_bChecked(FALSE)
    , m_userprop(p)
{

}

EditPropUserBool::~EditPropUserBool()
{
}

void EditPropUserBool::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_LABEL, m_sLabel);
    DDX_Text(pDX, IDC_CHECK, m_sCheck);
    DDX_Check(pDX, IDC_CHECK, m_bChecked);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(EditPropUserBool, CStandAloneDialog)
END_MESSAGE_MAP()


// EditPropUserBool message handlers


BOOL EditPropUserBool::OnInitDialog()
{
    __super::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    AdjustControlSize(IDC_PROPRECURSIVE);

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(!m_bFolder || m_bMultiple || (m_bFolder && !m_userprop->file));
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps || (!m_bFolder && !m_bMultiple) || m_bRemote ? SW_HIDE : SW_SHOW);

    m_bChecked = m_PropValue.compare(CUnicodeUtils::GetUTF8(m_userprop->boolYes))==0;
    m_sLabel = m_userprop->labelText;
    m_sCheck = m_userprop->boolCheckText;

    CString sWindowTitle = m_userprop->propName;
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    if (m_bFolder && m_userprop->file)
    {
        // for folders, the file properties can only be set recursively
        m_bRecursive = TRUE;
    }

    UpdateData(false);
    AdjustControlSize(IDC_CHECK);

    return TRUE;
}

void EditPropUserBool::OnOK()
{
    UpdateData();

    bool bSet = !!m_bChecked;

    if (bSet)
        m_PropValue = CUnicodeUtils::GetUTF8(m_userprop->boolYes);
    else
        m_PropValue = CUnicodeUtils::GetUTF8(m_userprop->boolNo);
    m_bChanged = true;

    CStandAloneDialog::OnOK();
}
