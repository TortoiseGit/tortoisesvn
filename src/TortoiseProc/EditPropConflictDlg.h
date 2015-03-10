// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2010, 2013-2015 - TortoiseSVN

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

// CEditPropConflictDlg dialog

class CEditPropConflictDlg : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CEditPropConflictDlg)

public:
    CEditPropConflictDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CEditPropConflictDlg();

    bool    SetPrejFile(const CTSVNPath& prejFile);
    void    SetConflictedItem(const CTSVNPath& conflictItem) {m_conflictItem = conflictItem;}
    void    SetPropertyName(const std::string& name) { m_propname = name; }
    void    SetPropValues(const std::string& propvalue_base, const std::string& propvalue_working, const std::string& propvalue_incoming_old, const std::string& propvalue_incoming_new);


// Dialog Data
    enum { IDD = IDD_EDITPROPCONFLICT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

    afx_msg void OnBnClickedEditprops();
    afx_msg void OnBnClickedStartmergeeditor();
    afx_msg void OnBnClickedResolved();
private:
    CTSVNPath       m_prejFile;
    CTSVNPath       m_conflictItem;
    CTSVNPath       m_ResultPath;
    CString         m_sPrejText;

    std::string     m_propname;
    std::string     m_value_base;
    std::string     m_value_working;
    std::string     m_value_incomingold;
    std::string     m_value_incomingnew;
};
