// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2017-2018, 2020-2021 - TortoiseSVN

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
#include "SciEdit.h"

/**
 * \ingroup TortoiseProc
 * Shows the unshelve dialog where the user can select the shelf to be applied
 * to the working copy
 */
class CUnshelve : public CResizableStandAloneDialog //CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CUnshelve)

public:
    CUnshelve(CWnd* pParent = nullptr); // standard constructor
    ~CUnshelve() override;

    enum
    {
        IDD = IDD_UNSHELVE
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    void         OnCancel() override;
    void         OnOK() override;
    afx_msg void OnBnClickedHelp();
    afx_msg void OnBnClickedDelete();
    afx_msg void OnCbnSelchangeShelvename();
    afx_msg void OnCbnSelchangeVersioncombo();
    afx_msg void OnLvnGetdispinfoFilelist(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    CComboBox     m_cShelvesCombo;
    CComboBox     m_cVersionCombo;
    CListCtrl     m_cFileList;
    CSciEdit      m_cLogMessage;
    ShelfInfo     m_currentShelfInfo;
    CTSVNPathList m_currentFiles;
    int           m_nIconFolder;
    wchar_t       m_columnBuf[MAX_PATH];

public:
    CString       m_sShelveName;
    CTSVNPathList m_pathList;
    int           m_version;
};
