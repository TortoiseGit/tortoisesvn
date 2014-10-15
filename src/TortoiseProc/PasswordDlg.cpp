// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014 - TortoiseSVN

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
#include "PasswordDlg.h"
#include "afxdialogex.h"


// CPasswordDlg dialog

IMPLEMENT_DYNAMIC(CPasswordDlg, CStandAloneDialog)

CPasswordDlg::CPasswordDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CPasswordDlg::IDD, pParent)
    , m_sPW1(_T(""))
    , m_sPW2(_T(""))
    , m_bForSave(false)
{

}

CPasswordDlg::~CPasswordDlg()
{
}

void CPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_PW1, m_sPW1);
    DDX_Text(pDX, IDC_PW2, m_sPW2);
}


BEGIN_MESSAGE_MAP(CPasswordDlg, CStandAloneDialog)
END_MESSAGE_MAP()


// CPasswordDlg message handlers


BOOL CPasswordDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    if (m_bForSave)
        GetDlgItem(IDC_PW2)->ShowWindow(SW_SHOW);

    GetDlgItem(IDC_PW1)->SetFocus();
    return FALSE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


void CPasswordDlg::OnOK()
{
    UpdateData(TRUE);
    if (m_bForSave && (m_sPW1 != m_sPW2))
    {
        // passwords don't match
        ShowEditBalloon(IDC_PW1, IDS_ERR_SYNCPW_NOMATCH, IDS_ERR_ERROR, TTI_ERROR);
        return;
    }
    CStandAloneDialog::OnOK();
}
