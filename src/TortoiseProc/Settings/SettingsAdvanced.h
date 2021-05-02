// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010, 2012, 2015, 2021 - TortoiseSVN

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

class CSettingsAdvanced : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSettingsAdvanced)

public:
    CSettingsAdvanced();
    ~CSettingsAdvanced() override;

    UINT GetIconID() override { return IDI_GENERAL; }

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGS_CONFIG
    };

    enum SettingType
    {
        SettingTypeBoolean,
        SettingTypeNumber,
        SettingTypeString,
        SettingTypeNone,
    };

    union DefaultValue
    {
        bool    b;
        LPCTSTR s;
        DWORD   l;
    };

    struct AdvancedSetting
    {
        CString      sName;
        SettingType  type;
        DefaultValue def;
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnApply() override;
    BOOL         PreTranslateMessage(MSG* pMsg) override;
    BOOL         OnInitDialog() override;
    afx_msg void OnLvnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLvnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMDblclkConfig(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl       m_listCtrl;
    AdvancedSetting settings[50];
};
