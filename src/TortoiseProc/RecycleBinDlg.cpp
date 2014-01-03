// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2014 - TortoiseSVN

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
#include "RecycleBinDlg.h"
#include <afxdialogex.h>


// CRecycleBinDlg dialog

IMPLEMENT_DYNAMIC(CRecycleBinDlg, CStandAloneDialog)

CRecycleBinDlg::CRecycleBinDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CRecycleBinDlg::IDD, pParent)
    , m_regDontDoAgain(L"Software\\TortoiseSVN\\RecycleBinSlowDontAskAgain")
    , m_sLabel(_T(""))
    , m_bDontAskAgain(FALSE)
    , m_startTicks(0)
{

}

CRecycleBinDlg::~CRecycleBinDlg()
{
}

void CRecycleBinDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_INFO, m_sLabel);
    DDX_Check(pDX, IDC_DONTASKAGAIN, m_bDontAskAgain);
}


BEGIN_MESSAGE_MAP(CRecycleBinDlg, CStandAloneDialog)
    ON_BN_CLICKED(IDC_EMPTYBIN, &CRecycleBinDlg::OnBnClickedEmptybin)
    ON_BN_CLICKED(IDC_DONTUSEBIN, &CRecycleBinDlg::OnBnClickedDontusebin)
END_MESSAGE_MAP()


// CRecycleBinDlg message handlers


BOOL CRecycleBinDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    m_bDontAskAgain = m_regDontDoAgain;

    UpdateData(FALSE);

    return TRUE;
}

ULONGLONG CRecycleBinDlg::StartTime()
{
    m_startTicks = GetTickCount64();
    return m_startTicks;
}

void CRecycleBinDlg::EndTime(int filecount)
{
    bool tooSlow = false;
    if (((GetTickCount64() - m_startTicks)/1000) > (5UL+filecount))
        tooSlow = true;

    if ((tooSlow)&&(DWORD(m_regDontDoAgain)==0))
    {
        SHQUERYRBINFO SHQueryRBInfo = {sizeof(SHQUERYRBINFO)};
        if (SHQueryRecycleBin(NULL, &SHQueryRBInfo)==S_OK)
        {
            if (SHQueryRBInfo.i64NumItems > 200)
            {
                m_sLabel.FormatMessage(IDS_RECYCLEBINSLOWDOWN, (unsigned long)filecount, (unsigned long)SHQueryRBInfo.i64NumItems);
                DoModal();
            }
        }
    }
}


void CRecycleBinDlg::OnBnClickedEmptybin()
{
    SHEmptyRecycleBin(m_hWnd, NULL, SHERB_NOCONFIRMATION);
    CStandAloneDialog::OnCancel();
}


void CRecycleBinDlg::OnBnClickedDontusebin()
{
    CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\RevertWithRecycleBin"), TRUE);
    reg = FALSE;
    CStandAloneDialog::OnCancel();
}


void CRecycleBinDlg::OnCancel()
{
    UpdateData();

    if (m_bDontAskAgain)
        m_regDontDoAgain = m_bDontAskAgain;
    CStandAloneDialog::OnCancel();
}
