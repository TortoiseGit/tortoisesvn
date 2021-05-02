// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2012, 2020-2021 - TortoiseSVN

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

// CWaitDlg dialog

class CWaitDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CWaitDlg)

public:
    CWaitDlg(CWnd* pParent = nullptr); // standard constructor
    ~CWaitDlg() override;

    void SetTitle(const CString& s) { m_title = s; }
    void SetInfo(const CString& s) { m_info = s; }

    // Dialog Data
    enum
    {
        IDD = IDD_PLEASEWAIT
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnCancel() override;
    void OnOK() override;

    DECLARE_MESSAGE_MAP()

private:
    CString m_title;
    CString m_info;
};
