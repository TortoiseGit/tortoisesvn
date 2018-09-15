// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2017-2018 - TortoiseSVN

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
#include "SVN.h"
#include "SVNStatusListCtrl.h"
#include "SciEdit.h"

/**
 * \ingroup TortoiseProc
 * Shows the shelve dialog where the user can select the files to
 * be shelved.
 */
class CShelve : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CShelve)

public:
    CShelve(CWnd* pParent = NULL); // standard constructor
    virtual ~CShelve();

    enum
    {
        IDD = IDD_SHELVE
    };

protected:
    virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnCancel();
    virtual void OnOK();
    void         FillData();
    afx_msg void OnBnClickedSelectall();
    afx_msg void OnBnClickedHelp();
    afx_msg void OnShelveNameChanged();
    afx_msg LRESULT OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
    afx_msg LRESULT OnFileDropped(WPARAM, LPARAM lParam);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnBnClickedCheckpoint();
    afx_msg void    OnCbnSelchangeShelves();
    afx_msg void    OnCbnEditchangeShelves();

    DECLARE_MESSAGE_MAP()

private:
    static UINT PatchThreadEntry(LPVOID pVoid);
    UINT        PatchThread();
    DWORD       ShowMask();
    void        LockOrUnlockBtns();

private:
    SVN                m_svn;
    CSVNStatusListCtrl m_PatchList;
    LONG               m_bThreadRunning;
    CButton            m_SelectAll;
    bool               m_bCancelled;
    CRegDWORD          m_regAddBeforeCommit;
    CSciEdit           m_cLogMessage;
    CComboBox          m_cShelvesCombo;
    ProjectProperties  m_ProjectProperties;

public:
    /// the list of files to include in the patch
    CTSVNPathList m_pathList;
    /// the name of the shelf
    CString m_sShelveName;
    /// the log message to store with the shelf
    CString m_sLogMsg;
    /// if true, do a shelve. Otherwise only create a checkpoint
    bool m_revert;
};
