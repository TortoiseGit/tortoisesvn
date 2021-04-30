// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2010, 2021 - TortoiseSVN

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
#include "SVNStatusListCtrl.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog, showing a list of conflicted files of the working copy.
 */
class CResolveDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CResolveDlg)

public:
    CResolveDlg(CWnd* pParent = nullptr); // standard constructor
    ~CResolveDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_RESOLVE
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    afx_msg void    OnBnClickedSelectall();
    afx_msg void    OnBnClickedHelp();
    afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    BOOL            PreTranslateMessage(MSG* pMsg) override;
    BOOL            OnInitDialog() override;
    void            OnOK() override;
    void            OnCancel() override;

private:
    static UINT     ResolveThreadEntry(LPVOID pVoid);
    UINT            ResolveThread();
    afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

public:
    CTSVNPathList m_pathList;

private:
    CSVNStatusListCtrl m_resolveListCtrl;
    volatile LONG      m_bThreadRunning;
    CButton            m_selectAll;
    bool               m_bCancelled;
};
