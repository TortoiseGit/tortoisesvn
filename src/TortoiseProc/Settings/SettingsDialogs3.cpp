// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2013-2016, 2020-2021 - TortoiseSVN

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
#include "stdafx.h"
#include "SettingsDialogs3.h"

// SettingsDialogs3 dialog

IMPLEMENT_DYNAMIC(SettingsDialogs3, ISettingsPropPage)

SettingsDialogs3::SettingsDialogs3()
    : ISettingsPropPage(SettingsDialogs3::IDD)
    , m_bPreFetch(FALSE)
    , m_bIncludeExternals(FALSE)
    , m_bIncludeLocks(FALSE)
{
    m_regPreFetch         = CRegDWORD(L"Software\\TortoiseSVN\\RepoBrowserPrefetch", TRUE);
    m_regIncludeExternals = CRegDWORD(L"Software\\TortoiseSVN\\RepoBrowserShowExternals", TRUE);
    m_regIncludeLocks     = CRegDWORD(L"Software\\TortoiseSVN\\RepoBrowserShowLocks", TRUE);
}

SettingsDialogs3::~SettingsDialogs3()
{
}

void SettingsDialogs3::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_ALLOWPREFETCH, m_bPreFetch);
    DDX_Check(pDX, IDC_SHOWEXTERNALS, m_bIncludeExternals);
    DDX_Check(pDX, IDC_SHOWLOCKS, m_bIncludeLocks);
}

BEGIN_MESSAGE_MAP(SettingsDialogs3, ISettingsPropPage)
    ON_BN_CLICKED(IDC_ALLOWPREFETCH, &SettingsDialogs3::OnBnClicked)
    ON_BN_CLICKED(IDC_SHOWEXTERNALS, &SettingsDialogs3::OnBnClicked)
    ON_BN_CLICKED(IDC_SHOWLOCKS, &SettingsDialogs3::OnBnClicked)
    ON_BN_CLICKED(IDC_SHELF_V2, &SettingsDialogs3::OnBnClicked)
    ON_BN_CLICKED(IDC_SHELF_V3, &SettingsDialogs3::OnBnClicked)
END_MESSAGE_MAP()

// SettingsDialogs3 message handlers

void SettingsDialogs3::OnBnClicked()
{
    SetModified();
}

BOOL SettingsDialogs3::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    EnableToolTips();

    m_bPreFetch         = m_regPreFetch;
    m_bIncludeExternals = m_regIncludeExternals;
    m_bIncludeLocks     = m_regIncludeLocks;

    char*   pValue = nullptr;
    size_t  len    = 0;
    errno_t err    = _dupenv_s(&pValue, &len, "SVN_EXPERIMENTAL_COMMANDS");
    auto    isV3   = ((err == 0) && pValue && strstr(pValue, "shelf3"));
    free(pValue);
    CheckRadioButton(IDC_SHELF_V2, IDC_SHELF_V3, isV3 ? IDC_SHELF_V3 : IDC_SHELF_V2);

    m_tooltips.AddTool(IDC_ALLOWPREFETCH, IDS_SETTINGS_REPOBROWSER_PREFETCH_TT);
    m_tooltips.AddTool(IDC_SHOWEXTERNALS, IDS_SETTINGS_REPOBROWSER_EXTERNALS_TT);
    m_tooltips.AddTool(IDC_SHOWLOCKS, IDS_SETTINGS_REPOBROWSER_LOCKS_TT);

    UpdateData(FALSE);
    return TRUE;
}

BOOL SettingsDialogs3::OnApply()
{
    UpdateData();
    Store(m_bPreFetch, m_regPreFetch);
    Store(m_bIncludeExternals, m_regIncludeExternals);
    Store(m_bIncludeLocks, m_regIncludeLocks);

    char*   pValue = nullptr;
    size_t  len    = 0;
    errno_t err    = _dupenv_s(&pValue, &len, "SVN_EXPERIMENTAL_COMMANDS");
    auto    wasV3  = ((err == 0) && pValue && strstr(pValue, "shelf3"));
    free(pValue);

    auto       isV3     = IsDlgButtonChecked(IDC_SHELF_V3) != FALSE;
    CRegString regShelf = CRegString(L"Environment\\SVN_EXPERIMENTAL_COMMANDS");
    regShelf            = isV3 ? L"shelf3" : L"shelf2";
    if (isV3 != wasV3)
    {
        SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"Environment"), SMTO_ABORTIFHUNG, 5000, nullptr);
        m_restart = Restart_System;
    }
    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}
