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
#include "EditPropMimeType.h"
#include "UnicodeUtils.h"
#include "AppUtils.h"


// CEditPropMimeType dialog

IMPLEMENT_DYNAMIC(CEditPropMimeType, CStandAloneDialog)

CEditPropMimeType::CEditPropMimeType(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CEditPropMimeType::IDD, pParent)
    , EditPropBase()
    , m_sCustomMimeType(_T(""))
{

}

CEditPropMimeType::~CEditPropMimeType()
{
}

void CEditPropMimeType::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
    DDX_Text(pDX, IDC_CUSTOMMIMETYPE, m_sCustomMimeType);
}


BEGIN_MESSAGE_MAP(CEditPropMimeType, CStandAloneDialog)
    ON_BN_CLICKED(IDC_MIMEBIN, &CEditPropMimeType::OnBnClickedType)
    ON_BN_CLICKED(IDC_MIMECUSTOM, &CEditPropMimeType::OnBnClickedType)
    ON_BN_CLICKED(IDC_MIMETEXT, &CEditPropMimeType::OnBnClickedType)
    ON_BN_CLICKED(IDC_PROPRECURSIVE, &CEditPropMimeType::OnBnClickedProprecursive)
    ON_BN_CLICKED(IDHELP, &CEditPropMimeType::OnBnClickedHelp)
END_MESSAGE_MAP()


BOOL CEditPropMimeType::OnInitDialog()
{
    BOOL bRet = TRUE;

    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    AdjustControlSize(IDC_MIMETEXT);
    AdjustControlSize(IDC_MIMEBIN);
    AdjustControlSize(IDC_MIMECUSTOM);
    AdjustControlSize(IDC_PROPRECURSIVE);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    DialogEnableWindow(IDC_CUSTOMMIMETYPE, false);
    if (m_PropValue.compare("application/octet-stream") == 0)
        CheckRadioButton(IDC_MIMETEXT, IDC_MIMECUSTOM, IDC_MIMEBIN);
    else if (m_PropValue.empty())
        CheckRadioButton(IDC_MIMETEXT, IDC_MIMECUSTOM, IDC_MIMETEXT);
    else
    {
        m_sCustomMimeType = CUnicodeUtils::StdGetUnicode(m_PropValue).c_str();
        CheckRadioButton(IDC_MIMETEXT, IDC_MIMECUSTOM, IDC_MIMECUSTOM);
        DialogEnableWindow(IDC_CUSTOMMIMETYPE, true);
        GetDlgItem(IDC_CUSTOMMIMETYPE)->SetFocus();
        bRet = FALSE;
    }

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(!m_bFolder || m_bMultiple);
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps || (!m_bFolder && !m_bMultiple) || m_bRemote ? SW_HIDE : SW_SHOW);

    if (m_bFolder)
    {
        // for folders, the property can only be set recursively
        m_bRecursive = TRUE;
    }
    UpdateData(false);

    return bRet;
}

void CEditPropMimeType::OnOK()
{
    UpdateData();

    int checked = GetCheckedRadioButton(IDC_MIMETEXT, IDC_MIMECUSTOM);
    switch (checked)
    {
    case IDC_MIMECUSTOM:
        m_PropValue = CUnicodeUtils::StdGetUTF8((LPCTSTR)m_sCustomMimeType);
        break;
    case IDC_MIMETEXT:
        m_PropValue = "";   // empty mime type means plain text
        break;
    case IDC_MIMEBIN:
        m_PropValue = "application/octet-stream";
        break;
    }
    m_bChanged = true;

    CStandAloneDialog::OnOK();
}

void CEditPropMimeType::OnBnClickedType()
{
    int checked = GetCheckedRadioButton(IDC_MIMETEXT, IDC_MIMECUSTOM);
    DialogEnableWindow(IDC_CUSTOMMIMETYPE, checked == IDC_MIMECUSTOM);
    if (checked == IDC_MIMECUSTOM)
        GetDlgItem(IDC_CUSTOMMIMETYPE)->SetFocus();
}


void CEditPropMimeType::OnBnClickedProprecursive()
{
    UpdateData();
    if (m_bFolder)
        m_bRecursive = TRUE;
    UpdateData(false);
}


void CEditPropMimeType::OnBnClickedHelp()
{
    OnHelp();
}
