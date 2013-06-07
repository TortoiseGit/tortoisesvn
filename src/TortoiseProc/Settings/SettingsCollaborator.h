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
#include "SettingsPropPage.h"
#include "Tooltip.h"
#include "registry.h"
#include "SettingsCollaborator.h"


// SettingsCollaborator dialog

class SettingsCollaborator : public ISettingsPropPage
{
    DECLARE_DYNAMIC(SettingsCollaborator)

public:
    SettingsCollaborator();
    virtual ~SettingsCollaborator();

    UINT GetIconID() override {return IDI_CODE_COLLABORATOR;}

    // Dialog Data
    enum { IDD = IDD_SETTINGS_COLLABORATOR };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    afx_msg void OnBnClicked();
    afx_msg void OnChange();

    DECLARE_MESSAGE_MAP()
private:
    CToolTips       m_tooltips;
    CString         m_collabGuiPath;
    CString         m_svnUser;
    CString         m_svnPassword;
    CString         m_collabUser;
    CString         m_collabPassword;

    CRegString      m_regCollabGuiPath;
    CRegString      m_regSvnUser;
    CRegString      m_regSvnPassword;
    CRegString      m_regCollabUser;
    CRegString      m_regCollabPassword;
    
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    
    afx_msg void OnBnClickedBrowse();
};
