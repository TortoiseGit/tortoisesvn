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
#include "MonitorOptionsDlg.h"
#include "afxdialogex.h"
#include "registry.h"
#include "PathUtils.h"


// CMonitorOptionsDlg dialog

IMPLEMENT_DYNAMIC(CMonitorOptionsDlg, CDialogEx)

CMonitorOptionsDlg::CMonitorOptionsDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CMonitorOptionsDlg::IDD, pParent)
    , m_bStartWithWindows(FALSE)
{

}

CMonitorOptionsDlg::~CMonitorOptionsDlg()
{
}

void CMonitorOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_CHECK1, m_bStartWithWindows);
}


BEGIN_MESSAGE_MAP(CMonitorOptionsDlg, CDialogEx)
END_MESSAGE_MAP()


// CMonitorOptionsDlg message handlers


BOOL CMonitorOptionsDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CRegString regStart(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run\\TortoiseSVN Monitor");
    m_bStartWithWindows = !CString(regStart).IsEmpty();

    UpdateData(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
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
    CDialogEx::OnOK();
}
