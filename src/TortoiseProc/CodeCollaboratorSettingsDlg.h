// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

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
#include "SciEdit.h"
#include "Tooltip.h"
#include <afxcmn.h>

// CodeCollaboratorSettingsDlg dialog

class CodeCollaboratorSettingsDlg : public CDialog
{
    DECLARE_DYNAMIC(CodeCollaboratorSettingsDlg)

public:
    CodeCollaboratorSettingsDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CodeCollaboratorSettingsDlg();
    
// Dialog Data
    enum { IDD = IDD_COLLABORATORSETTINGS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    
    DECLARE_MESSAGE_MAP()

private:
    CToolTips       m_tooltips;
    CString         m_svnUser;
    CString         m_svnPassword;
    CString         m_collabUser;
    CString         m_collabPassword;

    CRegString      m_regSvnUser;
    CRegString      m_regSvnPassword;
    CRegString      m_regCollabUser;
    CRegString      m_regCollabPassword;
public:
    afx_msg void OnBnClickedOk();
};
