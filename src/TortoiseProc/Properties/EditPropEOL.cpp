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
#include "EditPropEOL.h"
#include <cctype>


// CEditPropEOL dialog

IMPLEMENT_DYNAMIC(CEditPropEOL, CStandAloneDialog)

CEditPropEOL::CEditPropEOL(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CEditPropEOL::IDD, pParent)
    , EditPropBase()
{

}

CEditPropEOL::~CEditPropEOL()
{
}

void CEditPropEOL::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropEOL, CStandAloneDialog)
    ON_BN_CLICKED(IDC_PROPRECURSIVE, &CEditPropEOL::OnBnClickedProprecursive)
    ON_BN_CLICKED(IDHELP, &CEditPropEOL::OnBnClickedHelp)
END_MESSAGE_MAP()


BOOL CEditPropEOL::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    std::transform(m_PropValue.begin(), m_PropValue.end(), m_PropValue.begin(), std::tolower);

    CheckRadioButton(IDC_RADIONOEOL, IDC_RADIOCR, IDC_RADIONOEOL);

    if (m_PropValue.compare("native") == 0)
        CheckRadioButton(IDC_RADIONOEOL, IDC_RADIOCR, IDC_RADIONATIVE);
    else if (m_PropValue.compare("crlf") == 0)
        CheckRadioButton(IDC_RADIONOEOL, IDC_RADIOCR, IDC_RADIOCRLF);
    else if (m_PropValue.compare("lf") == 0)
        CheckRadioButton(IDC_RADIONOEOL, IDC_RADIOCR, IDC_RADIOLF);
    else if (m_PropValue.compare("cr") == 0)
        CheckRadioButton(IDC_RADIONOEOL, IDC_RADIOCR, IDC_RADIOCR);

    AdjustControlSize(IDC_RADIONOEOL);
    AdjustControlSize(IDC_RADIONATIVE);
    AdjustControlSize(IDC_RADIOCRLF);
    AdjustControlSize(IDC_RADIOLF);
    AdjustControlSize(IDC_RADIOCR);
    AdjustControlSize(IDC_PROPRECURSIVE);

    if (m_bFolder)
    {
        // for folders, the property can only be set recursively
        m_bRecursive = TRUE;
    }
    UpdateData(false);

    return TRUE;
}

void CEditPropEOL::OnOK()
{
    UpdateData();

    int checked = GetCheckedRadioButton(IDC_RADIONOEOL, IDC_RADIOCR);
    switch (checked)
    {
    case IDC_RADIONOEOL:
        m_PropValue = "";
        break;
    case IDC_RADIONATIVE:
        m_PropValue = "native";
        break;
    case IDC_RADIOCRLF:
        m_PropValue = "CRLF";
        break;
    case IDC_RADIOLF:
        m_PropValue = "LF";
        break;
    case IDC_RADIOCR:
        m_PropValue = "CR";
        break;
    }
    m_bChanged = true;

    CStandAloneDialog::OnOK();
}

void CEditPropEOL::OnBnClickedProprecursive()
{
    UpdateData();
    if (m_bFolder)
        m_bRecursive = TRUE;
    UpdateData(false);
}

void CEditPropEOL::OnBnClickedHelp()
{
    OnHelp();
}
