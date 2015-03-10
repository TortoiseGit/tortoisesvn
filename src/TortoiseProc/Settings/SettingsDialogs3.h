// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2013, 2015 - TortoiseSVN

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
#include "registry.h"


// SettingsDialogs3 dialog

class SettingsDialogs3 : public ISettingsPropPage
{
    DECLARE_DYNAMIC(SettingsDialogs3)

public:
    SettingsDialogs3();
    virtual ~SettingsDialogs3();

    UINT GetIconID() override {return IDI_DIALOGS;}

    // Dialog Data
    enum { IDD = IDD_SETTINGSDIALOGS3 };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    afx_msg void OnBnClicked();

    DECLARE_MESSAGE_MAP()
private:
    CRegDWORD       m_regPreFetch;
    BOOL            m_bPreFetch;
    CRegDWORD       m_regIncludeExternals;
    BOOL            m_bIncludeExternals;
    CRegDWORD       m_regIncludeLocks;
    BOOL            m_bIncludeLocks;
};
