// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2015, 2021 - TortoiseSVN

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
#include "SciEdit.h"
#include "ProjectProperties.h"
#include "HistoryCombo.h"
#include "RegHistory.h"
#include "TSVNPath.h"

/**
 * \ingroup TortoiseProc
 * Dialog used to prompt the user for required information to do an import.
 * The required information is the URL to import to.
 */
class CImportDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CImportDlg)

public:
    CImportDlg(CWnd* pParent = nullptr); // standard constructor
    ~CImportDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_IMPORT
    };

protected:
    CFont        m_logFont;
    CButton      m_butBrowse;
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         PreTranslateMessage(MSG* pMsg) override;
    void         OnOK() override;
    void         OnCancel() override;
    BOOL         OnInitDialog() override;
    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedSelectall();
    afx_msg void OnBnClickedHelp();
    afx_msg void OnEnChangeLogmessage();
    afx_msg void OnBnClickedHistory();
    afx_msg void OnCbnEditchangeUrlcombo();
    DECLARE_MESSAGE_MAP()
public:
    CTSVNPath m_path;
    CString   m_url;
    BOOL      m_bIncludeIgnored;
    BOOL      m_useAutoprops;
    CString   m_sMessage;

private:
    CSciEdit          m_cMessage;
    CHistoryCombo     m_urlCombo;
    ProjectProperties m_projectProperties;
    CRegHistory       m_history;
};
