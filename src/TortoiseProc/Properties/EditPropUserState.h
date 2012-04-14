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
#include "afxwin.h"
#include "EditPropBase.h"
#include "StandAloneDlg.h"
#include "UserProperties.h"


// EditPropUserState dialog

class EditPropUserState : public CStandAloneDialog, public EditPropBase
{
    DECLARE_DYNAMIC(EditPropUserState)

public:
    EditPropUserState(CWnd* pParent, const UserProp * p);   // standard constructor
    virtual ~EditPropUserState();

    virtual bool IsFolderOnlyProperty() override { return !m_userprop->file; }
    void SetUserProp(UserProp* p) {m_userprop = p;}

    // Dialog Data
    enum { IDD = IDD_EDITPROPUSERSTATE };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    DECLARE_MESSAGE_MAP()

    INT_PTR DoModal() override { return CStandAloneDialog::DoModal(); }

private:
    CString             m_sLabel;
    CComboBox           m_combo;
    const UserProp *    m_userprop;
};
