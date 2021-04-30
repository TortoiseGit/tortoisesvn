// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2012, 2015, 2021 - TortoiseSVN

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
#include "EditPropBase.h"
#include "StandAloneDlg.h"
#include "ProjectProperties.h"

// CEditPropsLocalHooks dialog

class CEditPropsLocalHooks : public CResizableStandAloneDialog
    , public EditPropBase
{
    DECLARE_DYNAMIC(CEditPropsLocalHooks)

public:
    CEditPropsLocalHooks(CWnd* pParent = nullptr); // standard constructor
    ~CEditPropsLocalHooks() override;

    // Dialog Data
    enum
    {
        IDD = IDD_EDITPROPLOCALHOOKS
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    void         OnOK() override;
    void         OnCancel() override;
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

    INT_PTR DoModal() override { return CResizableStandAloneDialog::DoModal(); }

protected:
    ProjectProperties m_projectProperties;
    CString           m_sCommandLine;
    BOOL              m_bWait;
    BOOL              m_bHide;
    BOOL              m_bEnforce;
    CComboBox         m_cHookTypeCombo;

public:
};
