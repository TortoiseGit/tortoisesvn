// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2014, 2021 - TortoiseSVN

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

// CRecycleBinDlg dialog

class CRecycleBinDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CRecycleBinDlg)

public:
    CRecycleBinDlg(CWnd* pParent = nullptr); // standard constructor
    ~CRecycleBinDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_RECYCLEBIN
    };

    ULONGLONG StartTime();
    void      EndTime(int fileCount);

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnCancel() override;

    afx_msg void OnBnClickedEmptybin();
    afx_msg void OnBnClickedDontusebin();
    DECLARE_MESSAGE_MAP()

private:
    ULONGLONG m_startTicks;
    CRegDWORD m_regDontDoAgain;
    CString   m_sLabel;
    BOOL      m_bDontAskAgain;
};
