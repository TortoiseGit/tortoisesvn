// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014, 2020-2021 - TortoiseSVN

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
#pragma once
#include "StandAloneDlg.h"

// CMonitorProjectDlg dialog

class CMonitorProjectDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CMonitorProjectDlg)

public:
    CMonitorProjectDlg(CWnd* pParent = nullptr); // standard constructor
    ~CMonitorProjectDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_MONITORPROJECT
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    void OnOK() override;

    DECLARE_MESSAGE_MAP()
public:
    CString m_sName;
    CString m_sPathOrURL;
    CString m_sUsername;
    CString m_sPassword;
    CString m_sIgnoreUsers;
    CString m_sIgnoreRegex;
    BOOL    m_isParentPath;
    int     m_monitorInterval;
};
