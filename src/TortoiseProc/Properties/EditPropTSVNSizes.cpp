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
#include "EditPropTSVNSizes.h"
#include "AppUtils.h"


// CEditPropTSVNSizes dialog

IMPLEMENT_DYNAMIC(CEditPropTSVNSizes, CStandAloneDialog)

CEditPropTSVNSizes::CEditPropTSVNSizes(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CEditPropTSVNSizes::IDD, pParent)
    , EditPropBase()
    , m_LogMinSize(0)
    , m_LockMinSize(0)
    , m_Border(0)
{

}

CEditPropTSVNSizes::~CEditPropTSVNSizes()
{
}

void CEditPropTSVNSizes::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_LOGMINSIZE, m_LogMinSize);
    DDV_MinMaxInt(pDX, m_LogMinSize, 0, 1000);
    DDX_Text(pDX, IDC_LOCKMINSIZE, m_LockMinSize);
    DDV_MinMaxInt(pDX, m_LockMinSize, 0, 1000);
    DDX_Text(pDX, IDC_BORDER, m_Border);
    DDV_MinMaxInt(pDX, m_Border, 0, 1000);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropTSVNSizes, CStandAloneDialog)
    ON_BN_CLICKED(IDHELP, &CEditPropTSVNSizes::OnBnClickedHelp)
END_MESSAGE_MAP()


// CEditPropTSVNSizes message handlers

BOOL CEditPropTSVNSizes::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    for (IT it = m_properties.begin(); it != m_properties.end(); ++it)
    {
        if (it->first.compare(PROJECTPROPNAME_LOGMINSIZE) == 0)
        {
            m_LogMinSize = atoi(it->second.value.c_str());
        }
        else if (it->first.compare(PROJECTPROPNAME_LOCKMSGMINSIZE) == 0)
        {
            m_LockMinSize = atoi(it->second.value.c_str());
        }
        else if (it->first.compare(PROJECTPROPNAME_LOGWIDTHLINE) == 0)
        {
            m_Border = atoi(it->second.value.c_str());
        }
    }

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(m_bFolder || m_bMultiple);
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps || (!m_bFolder && !m_bMultiple) || m_bRemote ? SW_HIDE : SW_SHOW);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    UpdateData(false);

    return TRUE;
}

void CEditPropTSVNSizes::OnOK()
{
    UpdateData();

    TProperties newProps;
    PropValue pVal;

    char numBuf[20];
    sprintf_s(numBuf, "%ld", m_LogMinSize);
    pVal.value = numBuf;
    pVal.remove = (m_LogMinSize == 0);
    newProps[PROJECTPROPNAME_LOGMINSIZE] = pVal;

    sprintf_s(numBuf, "%ld", m_LockMinSize);
    pVal.value = numBuf;
    pVal.remove = (m_LockMinSize == 0);
    newProps[PROJECTPROPNAME_LOCKMSGMINSIZE] = pVal;

    sprintf_s(numBuf, "%ld", m_Border);
    pVal.value = numBuf;
    pVal.remove = (m_Border == 0);
    newProps[PROJECTPROPNAME_LOGWIDTHLINE] = pVal;

    m_bChanged = true;
    m_properties = newProps;

    CStandAloneDialog::OnOK();
}

void CEditPropTSVNSizes::OnBnClickedHelp()
{
    OnHelp();
}
