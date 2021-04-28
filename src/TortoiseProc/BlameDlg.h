// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2015, 2019, 2021 - TortoiseSVN

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
#include "registry.h"
#include "StandAloneDlg.h"
#include "TSVNPath.h"

/**
 * \ingroup TortoiseProc
 * Show the blame dialog where the user can select the revision to blame
 * and whether to use TortoiseBlame or the default text editor to view the blame.
 */
class CBlameDlg : public CStateStandAloneDialog
{
    DECLARE_DYNAMIC(CBlameDlg)

public:
    CBlameDlg(CWnd* pParent = nullptr); // standard constructor
    ~CBlameDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_BLAME
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    afx_msg void OnBnClickedHelp();
    afx_msg void OnEnChangeRevisionEnd();
    BOOL         OnInitDialog() override;
    void         OnOK() override;

    DECLARE_MESSAGE_MAP()

protected:
    CString   m_sStartRev;
    CString   m_sEndRev;
    CRegDWORD m_regTextView;
    CRegDWORD m_regIncludeMerge;
    CRegDWORD m_regIgnoreWhitespace;

public:
    SVNRev                       m_startRev;
    SVNRev                       m_endRev;
    SVNRev                       m_pegRev;
    BOOL                         m_bTextView;
    BOOL                         m_bIgnoreEOL;
    BOOL                         m_bIncludeMerge;
    svn_diff_file_ignore_space_t m_ignoreSpaces;
    CTSVNPath                    m_path;
};
