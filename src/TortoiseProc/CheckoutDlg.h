// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2015, 2021-2022 - TortoiseSVN

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
#include "SVNRev.h"
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "FileDropEdit.h"
#include "TSVNPath.h"

/// forward declarations

class CLogDlg;

/**
 * \ingroup TortoiseProc
 * Prompts the user for required information for a checkout command. The information
 * is the module name and the repository url.
 */
class CCheckoutDlg : public CResizableStandAloneDialog // CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CCheckoutDlg)

public:
    CCheckoutDlg(CWnd* pParent = nullptr); ///< standard constructor
    ~CCheckoutDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_CHECKOUT
    };

protected:
    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL            OnInitDialog() override;
    void            OnOK() override;
    void            OnCancel() override;
    afx_msg void    OnBnClickedBrowse();
    afx_msg void    OnBnClickedCheckoutdirectoryBrowse();
    afx_msg void    OnEnChangeCheckoutdirectory();
    afx_msg void    OnBnClickedHelp();
    afx_msg void    OnBnClickedShowlog();
    afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnEnChangeRevisionNum();
    afx_msg void    OnCbnEditchangeUrlcombo();
    afx_msg void    OnCbnSelchangeDepth();
    afx_msg void    OnBnClickedSparse();

    DECLARE_MESSAGE_MAP()

    void   UpdateURLsFromCombo();
    bool   IsStandardCheckout();
    void   SetRevision(const SVNRev& rev);
    SVNRev GetSelectedRevision() const;
    SVNRev GetSelectedRevisionOrHead() const;

protected:
    CString   m_sRevision;
    CString   m_sCheckoutDirOrig;
    bool      m_bAutoCreateTargetName;
    CComboBox m_depthCombo;
    bool      m_bBlockMessages;

public:
    CHistoryCombo                  m_urlCombo;
    CTSVNPathList                  m_urLs;
    SVNRev                         m_revision;
    BOOL                           m_bNoExternals;
    BOOL                           m_bIndependentWCs;
    CButton                        m_butBrowse;
    CEdit                          m_editRevision;
    CString                        m_strCheckoutDirectory;
    CFileDropEdit                  m_cCheckoutEdit;
    CLogDlg*                       m_pLogDlg;
    svn_depth_t                    m_depth;
    BOOL                           m_bStorePristines;
    BOOL                           m_blockPathAdjustments;

    std::map<CString, svn_depth_t> m_checkoutDepths;
    bool                           m_standardCheckout; ///< true if only one path got selected and that URL path is a folder
    bool                           m_parentExists;     ///< W/C for parent folder already exists. Only valid if \ref m_standardCheckout is false.
};
