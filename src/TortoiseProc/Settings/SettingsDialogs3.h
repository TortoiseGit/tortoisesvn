// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2013, 2015, 2021 - TortoiseSVN

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
    ~SettingsDialogs3() override;

    UINT GetIconID() override { return IDI_DIALOGS; }

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSDIALOGS3
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    BOOL         OnApply() override;
    afx_msg void OnBnClicked();

    DECLARE_MESSAGE_MAP()
private:
    CRegDWORD m_regPreFetch;
    BOOL      m_bPreFetch;
    CRegDWORD m_regIncludeExternals;
    BOOL      m_bIncludeExternals;
    CRegDWORD m_regIncludeLocks;
    BOOL      m_bIncludeLocks;
};
