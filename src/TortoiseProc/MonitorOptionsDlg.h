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
#pragma once
#include "StandAloneDlg.h"

// CMonitorOptionsDlg dialog

class CMonitorOptionsDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CMonitorOptionsDlg)

public:
    CMonitorOptionsDlg(CWnd* pParent = nullptr); // standard constructor
    ~CMonitorOptionsDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_MONITOROPTIONS
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnOK() override;

    DECLARE_MESSAGE_MAP()
public:
    BOOL m_bStartWithWindows;
    BOOL m_bShowNotification;
    BOOL m_bPlaySound;
    int  m_defaultInterval;
};
