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
#include "resource.h"
#include "TortoiseProc.h"
#include "EditPropUserMultiLine.h"
#include "afxdialogex.h"
#include "AppUtils.h"
#include "UnicodeUtils.h"


// EditPropUserMultiLine dialog

IMPLEMENT_DYNAMIC(EditPropUserMultiLine, CStandAloneDialog)

    EditPropUserMultiLine::EditPropUserMultiLine(CWnd* pParent, const UserProp * p)
    : CStandAloneDialog(EditPropUserMultiLine::IDD, pParent)
    , m_userprop(p)
{

}

EditPropUserMultiLine::~EditPropUserMultiLine()
{
}

void EditPropUserMultiLine::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT, m_sLine);
    DDX_Text(pDX, IDC_LABEL, m_sLabel);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(EditPropUserMultiLine, CStandAloneDialog)
END_MESSAGE_MAP()


// EditPropUserMultiLine message handlers


BOOL EditPropUserMultiLine::OnInitDialog()
{
    __super::OnInitDialog();

    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    AdjustControlSize(IDC_PROPRECURSIVE);

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(!m_bFolder || m_bMultiple || (m_bFolder && !m_userprop->file));
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps || (!m_bFolder && !m_bMultiple) || m_bRemote ? SW_HIDE : SW_SHOW);

    m_sLabel = m_userprop->labelText;
    m_sLine = CUnicodeUtils::GetUnicode(m_PropValue.c_str());
    m_sLine.Replace(L"\n", L"\r\n");

    CString sWindowTitle = m_userprop->propName;
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    if (m_bFolder && m_userprop->file)
    {
        // for folders, the file properties can only be set recursively
        m_bRecursive = TRUE;
    }

    UpdateData(false);

    return TRUE;
}


void EditPropUserMultiLine::OnOK()
{
    UpdateData();

    m_sLine.Replace(L"\r\n", L"\n");

    bool validated = true;
    try
    {
        std::wregex regCheck = std::wregex (m_userprop->validationRegex);
        std::wstring s = m_sLine;
        if (!std::regex_match(s, regCheck))
            validated = false;
    }
    catch (std::exception)
    {
        validated = false;
    }
    if (!validated)
    {
        ShowEditBalloon(IDC_EDIT, IDS_ERR_VALIDATIONFAILED, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }

    m_PropValue = CUnicodeUtils::GetUTF8(m_sLine);

    m_bChanged = true;

    __super::OnOK();
}
