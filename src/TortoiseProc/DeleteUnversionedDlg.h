// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2011, 2014, 2021 - TortoiseSVN

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
* Dialog showing a list of unversioned and ignored files.
*/
class CDeleteUnversionedDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CDeleteUnversionedDlg)

public:
    CDeleteUnversionedDlg(CWnd* pParent = nullptr); // standard constructor
    ~CDeleteUnversionedDlg() override;

    enum
    {
        IDD = IDD_DELUNVERSIONED
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL            OnInitDialog() override;
    void            OnOK() override;
    void            OnCancel() override;
    BOOL            PreTranslateMessage(MSG* pMsg) override;
    afx_msg void    OnBnClickedSelectall();
    afx_msg void    OnBnClickedHideignored();
    afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

private:
    static UINT StatusThreadEntry(LPVOID pVoid);
    UINT        StatusThread();

public:
    CTSVNPathList m_pathList;
    BOOL          m_bUseRecycleBin;

private:
    BOOL               m_bSelectAll;
    BOOL               m_bHideIgnored;
    CString            m_sWindowTitle;
    volatile LONG      m_bThreadRunning;
    CSVNStatusListCtrl m_statusList;
    CButton            m_selectAll;
    bool               m_bCancelled;
};
