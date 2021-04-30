// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2009-2011, 2021 - TortoiseSVN

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
#include "TSVNPath.h"

/**
 * Reduced merge dialog, used when merge tracking is available to merge all
 * not-yet merged revisions to the target path.
 */
class CMergeAllDlg : public CStandAloneDialog
{
    DECLARE_DYNAMIC(CMergeAllDlg)

public:
    CMergeAllDlg(CWnd* pParent = nullptr); // standard constructor
    ~CMergeAllDlg() override;

    enum
    {
        IDD = IDD_MERGEALL
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    void         OnOK() override;
    BOOL         OnInitDialog() override;
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

    CComboBox m_depthCombo;

public:
    BOOL                         m_bForce;
    BOOL                         m_bIgnoreAncestry;
    svn_depth_t                  m_depth;
    BOOL                         m_bIgnoreEOL;
    svn_diff_file_ignore_space_t m_ignoreSpaces;
    CTSVNPathList                m_pathList;
};
