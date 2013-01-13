// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2013 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "SettingsDialogs3.h"
#include "afxdialogex.h"


// SettingsDialogs3 dialog

IMPLEMENT_DYNAMIC(SettingsDialogs3, ISettingsPropPage)

SettingsDialogs3::SettingsDialogs3()
    : ISettingsPropPage(SettingsDialogs3::IDD)
    , m_bPreFetch(FALSE)
    , m_bIncludeExternals(FALSE)
    , m_bIncludeLocks(FALSE)
{
    m_regPreFetch = CRegDWORD(_T("Software\\TortoiseSVN\\RepoBrowserPrefetch"), TRUE);
    m_regIncludeExternals = CRegDWORD(_T("Software\\TortoiseSVN\\RepoBrowserShowExternals"), TRUE);
    m_regIncludeLocks = CRegDWORD(_T("Software\\TortoiseSVN\\RepoBrowserShowLocks"), TRUE);
}

SettingsDialogs3::~SettingsDialogs3()
{
}

void SettingsDialogs3::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_ALLOWPREFETCH, m_bPreFetch);
    DDX_Check(pDX, IDC_SHOWEXTERNALS, m_bIncludeExternals);
    DDX_Check(pDX, IDC_SHOWLOCKS,     m_bIncludeLocks);
}


BEGIN_MESSAGE_MAP(SettingsDialogs3, ISettingsPropPage)
    ON_BN_CLICKED(IDC_ALLOWPREFETCH, &SettingsDialogs3::OnBnClicked)
    ON_BN_CLICKED(IDC_SHOWEXTERNALS, &SettingsDialogs3::OnBnClicked)
    ON_BN_CLICKED(IDC_SHOWLOCKS,     &SettingsDialogs3::OnBnClicked)
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

    m_bPreFetch = m_regPreFetch;
    m_bIncludeExternals = m_regIncludeExternals;
    m_bIncludeLocks = m_regIncludeLocks;

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_ALLOWPREFETCH, IDS_SETTINGS_REPOBROWSER_PREFETCH_TT);
    m_tooltips.AddTool(IDC_SHOWEXTERNALS, IDS_SETTINGS_REPOBROWSER_EXTERNALS_TT);
    m_tooltips.AddTool(IDC_SHOWEXTERNALS, IDS_SETTINGS_REPOBROWSER_LOCKS_TT);

    UpdateData(FALSE);
    return TRUE;
}

BOOL SettingsDialogs3::OnApply()
{
    UpdateData();
    Store (m_bPreFetch, m_regPreFetch);
    Store (m_bIncludeExternals, m_regIncludeExternals);
    Store (m_bIncludeLocks, m_regIncludeLocks);

    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

BOOL SettingsDialogs3::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);

    return ISettingsPropPage::PreTranslateMessage(pMsg);
}

