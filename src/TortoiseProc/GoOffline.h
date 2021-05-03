// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2021 - TortoiseSVN

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

#include "RepositoryInfo.h"
#include "resource.h"

// CGoOffline dialog

class CGoOffline : public CDialog
{
    DECLARE_DYNAMIC(CGoOffline)

public:
    CGoOffline(CWnd* pParent = nullptr); // standard constructor
    ~CGoOffline() override;

    void SetErrorMessage(const CString& errMsg);

    LogCache::ConnectionState selection;
    BOOL                      asDefault;
    BOOL                      doRetry;

    // Dialog Data
    enum
    {
        IDD = IDD_GOOFFLINE
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;

    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedPermanentlyOffline();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedRetry();

    DECLARE_MESSAGE_MAP()
private:
    CString m_errMsg;
};
