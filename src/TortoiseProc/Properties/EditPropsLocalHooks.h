// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2012 - TortoiseSVN

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
#include "Tooltip.h"
#include "Hooks.h"


// CEditPropsLocalHooks dialog

class CEditPropsLocalHooks : public CResizableStandAloneDialog, public EditPropBase
{
    DECLARE_DYNAMIC(CEditPropsLocalHooks)

public:
    CEditPropsLocalHooks(CWnd* pParent = NULL);   // standard constructor
    virtual ~CEditPropsLocalHooks();

    // Dialog Data
    enum { IDD = IDD_EDITPROPLOCALHOOKS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

    void CheckRecursive();
    INT_PTR DoModal() override { return CResizableStandAloneDialog::DoModal(); }
protected:
    CToolTips           m_tooltips;
    ProjectProperties   m_ProjectProperties;
    CString             m_sCommandLine;
    BOOL                m_bWait;
    BOOL                m_bHide;
    BOOL                m_bEnforce;
    CComboBox           m_cHookTypeCombo;
public:
};
