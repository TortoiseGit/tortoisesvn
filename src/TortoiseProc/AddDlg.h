// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2011, 2021 - TortoiseSVN

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
 * a simple dialog to show the user all unversioned
 * files below a specified folder.
 */
class CAddDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CAddDlg)

public:
    CAddDlg(CWnd* pParent = nullptr); // standard constructor
    ~CAddDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_ADD
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
    static UINT     AddThreadEntry(LPVOID pVoid);
    UINT            AddThread();
    afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

public:
    /** holds all the selected files/folders the user wants to add to version
     * control on exit */
    CTSVNPathList m_pathList;
    BOOL          m_useAutoprops;

private:
    CSVNStatusListCtrl m_addListCtrl;
    volatile LONG      m_bThreadRunning;
    CButton            m_selectAll;
    bool               m_bCancelled;
};
