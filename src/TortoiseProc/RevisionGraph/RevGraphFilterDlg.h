// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2013, 2020-2021 - TortoiseSVN

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
#include <afxcmn.h>

/**
 * \ingroup TortoiseProc
 * helper dialog to enter filter data for the revision graph.
 */
class CRevGraphFilterDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CRevGraphFilterDlg)

public:
    CRevGraphFilterDlg(CWnd* pParent = nullptr); // standard constructor
    ~CRevGraphFilterDlg() override;

    void    SetMaxRevision(svn_revnum_t rev) { m_headRev = rev; }
    void    GetRevisionRange(svn_revnum_t& minrev, svn_revnum_t& maxrev) const;
    void    SetRevisionRange(svn_revnum_t minrev, svn_revnum_t maxrev);
    CString GetFilterString() const { return m_sFilterPaths; }
    void    SetFilterString(const CString& str) { m_sFilterPaths = str; }
    bool    GetRemoveSubTrees() const { return m_removeSubTree != FALSE; }
    void    SetRemoveSubTrees(bool value) { m_removeSubTree = value ? TRUE : FALSE; }

    // Dialog Data
    enum
    {
        IDD = IDD_REVGRAPHFILTER
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;
    void OnOK() override;

    afx_msg void OnDeltaposFromspin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeltaposTospin(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()
protected:
    CString         m_sFilterPaths;
    BOOL            m_removeSubTree;
    CSpinButtonCtrl m_cFromSpin;
    CSpinButtonCtrl m_cToSpin;
    CString         m_sFromRev;
    CString         m_sToRev;
    svn_revnum_t    m_headRev;
    svn_revnum_t    m_minRev;
    svn_revnum_t    m_maxRev;
};
