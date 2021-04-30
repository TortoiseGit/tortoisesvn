// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014-2015, 2020-2021 - TortoiseSVN

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
#include "MonitorOptionsDlg.h"
#include "registry.h"
#include "PathUtils.h"

// CMonitorOptionsDlg dialog

IMPLEMENT_DYNAMIC(CMonitorOptionsDlg, CStandAloneDialog)

CMonitorOptionsDlg::CMonitorOptionsDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CMonitorOptionsDlg::IDD, pParent)
    , m_bStartWithWindows(FALSE)
    , m_bShowNotification(FALSE)
    , m_bPlaySound(FALSE)
    , m_defaultInterval(30)
{
}

CMonitorOptionsDlg::~CMonitorOptionsDlg()
{
}

void CMonitorOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_CHECK1, m_bStartWithWindows);
    DDX_Check(pDX, IDC_SHOWNOTIFICATION, m_bShowNotification);
    DDX_Check(pDX, IDC_PLAYSOUND, m_bPlaySound);
    DDX_Text(pDX, IDC_INTERVAL, m_defaultInterval);
    DDV_MinMaxInt(pDX, m_defaultInterval, 1, INT_MAX);
}

BEGIN_MESSAGE_MAP(CMonitorOptionsDlg, CStandAloneDialog)
END_MESSAGE_MAP()

// CMonitorOptionsDlg message handlers

BOOL CMonitorOptionsDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    CRegString regStart(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run\\TortoiseSVN Monitor");
    m_bStartWithWindows = !CString(regStart).IsEmpty();

    UpdateData(FALSE);

    return TRUE; // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CMonitorOptionsDlg::OnOK()
{
    UpdateData();

    CRegString regStart(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run\\TortoiseSVN Monitor");

    if (m_bStartWithWindows)
    {
        regStart = CPathUtils::GetAppPath() + L" /tray";
    }
    else
    {
        regStart.removeValue();
    }
    CStandAloneDialog::OnOK();
}
