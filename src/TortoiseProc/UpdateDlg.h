// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2009-2012, 2015, 2021 - TortoiseSVN

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
#include "SVNRev.h"
#include "TSVNPath.h"

/// forward declarations

class CLogDlg;

/**
 * \ingroup TortoiseProc
 * A simple dialog box asking the user for a revision number
 * to update to.
 */
class CUpdateDlg : public CStateStandAloneDialog
{
    DECLARE_DYNAMIC(CUpdateDlg)

public:
    CUpdateDlg(CWnd* pParent = nullptr); // standard constructor
    ~CUpdateDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_UPDATE
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL            OnInitDialog() override;
    void            OnOK() override;
    void            OnCancel() override;
    afx_msg void    OnBnClickedShowLog();
    afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnEnChangeRevnum();
    afx_msg void    OnCbnSelchangeDepth();
    afx_msg void    OnBnClickedSparse();

    DECLARE_MESSAGE_MAP()

    CLogDlg*  m_pLogDlg;
    CString   m_sRevision;
    CComboBox m_depthCombo;

public:
    SVNRev                         m_revision;
    CTSVNPath                      m_wcPath;
    BOOL                           m_bNoExternals;
    BOOL                           m_bStickyDepth;
    std::map<CString, svn_depth_t> m_checkoutDepths;
    svn_depth_t                    m_depth;
};
