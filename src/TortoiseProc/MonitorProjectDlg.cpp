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
#include "MonitorProjectDlg.h"
#include "afxdialogex.h"


// CMonitorProjectDlg dialog

IMPLEMENT_DYNAMIC(CMonitorProjectDlg, CDialogEx)

CMonitorProjectDlg::CMonitorProjectDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CMonitorProjectDlg::IDD, pParent)
    , m_sName(_T(""))
    , m_sPathOrURL(_T(""))
    , m_sUsername(_T(""))
    , m_sPassword(_T(""))
    , m_monitorInterval(30)
    , m_sIgnoreUsers(_T(""))
    , m_sIgnoreRegex(_T(""))
{

}

CMonitorProjectDlg::~CMonitorProjectDlg()
{
}

void CMonitorProjectDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_NAME, m_sName);
    DDX_Text(pDX, IDC_PATHORURL, m_sPathOrURL);
    DDX_Text(pDX, IDC_USERNAME, m_sUsername);
    DDX_Text(pDX, IDC_PASSWORD, m_sPassword);
    DDX_Text(pDX, IDC_INTERVAL, m_monitorInterval);
    DDV_MinMaxInt(pDX, m_monitorInterval, 1, 1000);
    DDX_Text(pDX, IDC_IGNOREUSERS, m_sIgnoreUsers);
    DDX_Text(pDX, IDC_IGNOREREGEX, m_sIgnoreRegex);
}


BEGIN_MESSAGE_MAP(CMonitorProjectDlg, CDialogEx)
END_MESSAGE_MAP()


// CMonitorProjectDlg message handlers


void CMonitorProjectDlg::OnOK()
{
    UpdateData();

    try
    {
        std::wregex r1 = std::wregex(m_sIgnoreRegex);
        UNREFERENCED_PARAMETER(r1);
    }
    catch (std::exception)
    {
        CString text = CString(MAKEINTRESOURCE(IDS_ERR_INVALIDREGEX));
        CString title = CString(MAKEINTRESOURCE(IDS_ERR_ERROR));
        EDITBALLOONTIP bt;
        bt.cbStruct = sizeof(bt);
        bt.pszText = text;
        bt.pszTitle = title;
        bt.ttiIcon = TTI_WARNING;
        SendDlgItemMessage(IDC_IGNOREREGEX, EM_SHOWBALLOONTIP, 0, (LPARAM)&bt);
        return;
    }

    CDialogEx::OnOK();
}
