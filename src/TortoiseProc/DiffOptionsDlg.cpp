// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2013, 2016 - TortoiseSVN

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
#include "DiffOptionsDlg.h"
#include <afxdialogex.h>
#include "SVN.h"

// CDiffOptionsDlg dialog

IMPLEMENT_DYNAMIC(CDiffOptionsDlg, CStandAloneDialog)

CDiffOptionsDlg::CDiffOptionsDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CDiffOptionsDlg::IDD, pParent)
    , m_bIgnoreEOLs(FALSE)
    , m_bIgnoreWhitespaces(FALSE)
    , m_bIgnoreAllWhitespaces(FALSE)
{

}

CDiffOptionsDlg::~CDiffOptionsDlg()
{
}

void CDiffOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_IGNOREEOL, m_bIgnoreEOLs);
    DDX_Check(pDX, IDC_IGNOREWHITESPACE, m_bIgnoreWhitespaces);
    DDX_Check(pDX, IDC_IGNOREALLWHITESPACE, m_bIgnoreAllWhitespaces);
}


BEGIN_MESSAGE_MAP(CDiffOptionsDlg, CStandAloneDialog)
END_MESSAGE_MAP()

void CDiffOptionsDlg::SetDiffOptions(const SVNDiffOptions & opts)
{
    m_bIgnoreEOLs = opts.GetIgnoreEOL();
    switch(opts.GetIgnoreSpace())
    {
    case svn_diff_file_ignore_space_none:
        m_bIgnoreWhitespaces = FALSE;
        m_bIgnoreAllWhitespaces = FALSE;
        break;

    case svn_diff_file_ignore_space_change:
        m_bIgnoreWhitespaces = TRUE;
        m_bIgnoreAllWhitespaces = FALSE;
        break;

    case svn_diff_file_ignore_space_all:
        m_bIgnoreWhitespaces = FALSE;
        m_bIgnoreAllWhitespaces = TRUE;
        break;
    }
}

SVNDiffOptions CDiffOptionsDlg::GetDiffOptions()
{
    SVNDiffOptions result;

    result.SetIgnoreEOL(!!m_bIgnoreEOLs);

    if (m_bIgnoreAllWhitespaces)
        result.SetIgnoreSpace(svn_diff_file_ignore_space_all);
    else if (m_bIgnoreWhitespaces)
        result.SetIgnoreSpace(svn_diff_file_ignore_space_change);
    else
        result.SetIgnoreSpace(svn_diff_file_ignore_space_none);

    return result;
}

BOOL CDiffOptionsDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(0,0,0,0);
    m_aeroControls.SubclassControl(this, IDC_IGNOREEOL);
    m_aeroControls.SubclassControl(this, IDC_IGNOREWHITESPACE);
    m_aeroControls.SubclassControl(this, IDC_IGNOREALLWHITESPACE);
    m_aeroControls.SubclassOkCancel(this);

    AdjustControlSize(IDC_IGNOREEOL);
    AdjustControlSize(IDC_IGNOREWHITESPACE);
    AdjustControlSize(IDC_IGNOREALLWHITESPACE);

    return TRUE;
}
