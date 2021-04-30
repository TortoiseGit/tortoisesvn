// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009, 2012, 2021 - TortoiseSVN

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
#include "MergeWizardBasePage.h"
#include "HistoryCombo.h"
#include "PathEdit.h"
#include "SVNRev.h"

/// forward declarations

class CLogDlg;

/**
 * Page in the merge wizard for selecting two urls and revisions for
 * a tree merge.
 */
class CMergeWizardTree : public CMergeWizardBasePage
{
    DECLARE_DYNAMIC(CMergeWizardTree)

public:
    CMergeWizardTree();
    ~CMergeWizardTree() override;

    enum
    {
        IDD = IDD_MERGEWIZARD_TREE
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL            OnInitDialog() override;
    LRESULT         OnWizardNext() override;
    LRESULT         OnWizardBack() override;
    BOOL            OnSetActive() override;
    bool            OkToCancel() override;
    afx_msg void    OnBnClickedBrowse();
    afx_msg void    OnBnClickedFindbranchstart();
    afx_msg void    OnBnClickedFindbranchend();
    afx_msg void    OnBnClickedShowlogwc();
    afx_msg void    OnBnClickedBrowse2();
    afx_msg void    OnEnChangeRevisionEnd();
    afx_msg void    OnEnChangeRevisionStart();
    afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnCbnEditchangeUrlcombo();
    afx_msg void    OnCbnEditchangeUrlcombo2();
    afx_msg LRESULT OnWCStatus(WPARAM wParam, LPARAM lParam);

    BOOL CheckData(bool bShowErrors = true);
    void SetStartRevision(const SVNRev& rev);
    void SetEndRevision(const SVNRev& rev);

    DECLARE_MESSAGE_MAP()

    CLogDlg*      m_pLogDlg;
    CLogDlg*      m_pLogDlg2;
    CLogDlg*      m_pLogDlg3;
    CHistoryCombo m_urlCombo;
    CHistoryCombo m_urlCombo2;
    CString       m_sStartRev;
    CString       m_sEndRev;
    CPathEdit     m_wc;

public:
    CString m_urlFrom;
    CString m_urlTo;
    SVNRev  m_startRev;
    SVNRev  m_endRev;
};
