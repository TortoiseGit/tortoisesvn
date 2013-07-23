// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2012-2013 - TortoiseSVN

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


// CEditPropMergeLogTemplate dialog

class CEditPropMergeLogTemplate : public CResizableStandAloneDialog, public EditPropBase
{
    DECLARE_DYNAMIC(CEditPropMergeLogTemplate)

public:
    CEditPropMergeLogTemplate(CWnd* pParent = NULL);   // standard constructor
    virtual ~CEditPropMergeLogTemplate();

    virtual bool            HasMultipleProperties() override { return true; }
    virtual bool            IsFolderOnlyProperty() override { return true; }


    enum { IDD = IDD_EDITPROPMERGELOGTEMPLATE };


protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnBnClickedHelp();

    DECLARE_MESSAGE_MAP()

    INT_PTR DoModal() override { return CResizableStandAloneDialog::DoModal(); }
};
