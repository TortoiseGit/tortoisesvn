// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2012-2013 - TortoiseSVN

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
#include "EditPropMergeLogTemplate.h"
#include "UnicodeUtils.h"
#include "AppUtils.h"


// CEditPropMergeLogTemplate dialog

IMPLEMENT_DYNAMIC(CEditPropMergeLogTemplate, CResizableStandAloneDialog)

CEditPropMergeLogTemplate::CEditPropMergeLogTemplate(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropMergeLogTemplate::IDD, pParent)
{

}

CEditPropMergeLogTemplate::~CEditPropMergeLogTemplate()
{
}

void CEditPropMergeLogTemplate::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropMergeLogTemplate, CResizableStandAloneDialog)
END_MESSAGE_MAP()


// CEditPropMergeLogTemplate message handlers


BOOL CEditPropMergeLogTemplate::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    SetDlgItemText(IDC_TITLEHINT, CString(MAKEINTRESOURCE(IDS_EDITPROPS_MERGETITLEHINT)));
    SetDlgItemText(IDC_TITLEHINTREVERSE, CString(MAKEINTRESOURCE(IDS_EDITPROPS_MERGETITLEHINTREVERSE)));
    SetDlgItemText(IDC_MSGHINT, CString(MAKEINTRESOURCE(IDS_EDITPROPS_MERGEMSGHINT)));


    for (auto it = m_properties.begin(); it != m_properties.end(); ++it)
    {
        if (it->first.compare(PROJECTPROPNAME_MERGELOGTEMPLATETITLE) == 0)
        {
            CString sTitle = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
            sTitle.Replace(L"\n", L"\r\n");
            SetDlgItemText(IDC_TITLE, sTitle);
        }
        else if (it->first.compare(PROJECTPROPNAME_MERGELOGTEMPLATEREVERSETITLE) == 0)
        {
            CString sRevTitle = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
            sRevTitle.Replace(L"\n", L"\r\n");
            SetDlgItemText(IDC_TITLEREVERSE, sRevTitle);
        }
        else if (it->first.compare(PROJECTPROPNAME_MERGELOGTEMPLATEMSG) == 0)
        {
            CString sMsg = CUnicodeUtils::StdGetUnicode(it->second.value).c_str();
            sMsg.Replace(L"\n", L"\r\n");
            SetDlgItemText(IDC_MSG, sMsg);
        }
    }

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(m_bFolder || m_bMultiple);
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps ? SW_HIDE : SW_SHOW);

    AdjustControlSize(IDC_PROPRECURSIVE);

    AddAnchor(IDC_TITLEHINT, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_TITLE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_TITLEHINTREVERSE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_TITLEREVERSE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MSGHINT, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MSG, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_DWM, BOTTOM_LEFT);
    AddAnchor(IDC_PROPRECURSIVE, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    EnableSaveRestore(_T("EditPropMergeLogTemplate"));
    GetDlgItem(IDC_TITLE)->SetFocus();

    return FALSE;
}


void CEditPropMergeLogTemplate::OnOK()
{
    TProperties newProps;
    PropValue pVal;

    CString sText;
    GetDlgItemText(IDC_TITLE, sText);
    CStringA propVal = CUnicodeUtils::GetUTF8(sText);
    propVal.Replace("\r\n", "\n");
    pVal.value = propVal;
    pVal.remove = (pVal.value.empty());
    newProps[PROJECTPROPNAME_MERGELOGTEMPLATETITLE] = pVal;

    GetDlgItemText(IDC_TITLEREVERSE, sText);
    propVal = CUnicodeUtils::GetUTF8(sText);
    propVal.Replace("\r\n", "\n");
    pVal.value = propVal;
    pVal.remove = (pVal.value.empty());
    newProps[PROJECTPROPNAME_MERGELOGTEMPLATEREVERSETITLE] = pVal;

    GetDlgItemText(IDC_MSG, sText);
    propVal = CUnicodeUtils::GetUTF8(sText);
    propVal.Replace("\r\n", "\n");
    pVal.remove = (pVal.value.empty());
    pVal.value = propVal;
    newProps[PROJECTPROPNAME_MERGELOGTEMPLATEMSG] = pVal;

    m_bChanged = true;
    m_properties = newProps;

    __super::OnOK();
}
