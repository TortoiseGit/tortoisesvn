// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2012, 2015, 2021 - TortoiseSVN

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

class CProgressDlg;

/**
 * \ingroup TortoiseProc
 * Settings page to configure miscellaneous stuff.
 */
class CSetLogCache
    : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSetLogCache)

public:
    CSetLogCache();
    ~CSetLogCache() override;

    UINT GetIconID() override { return IDI_CACHE; }

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSLOGCACHE
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    BOOL         OnApply() override;
    afx_msg void OnChanged();
    afx_msg void OnStandardDefaults();
    afx_msg void OnPowerDefaults();

    DECLARE_MESSAGE_MAP()
private:
    BOOL m_bEnableLogCaching;
    BOOL m_bSupportAmbiguousURL;
    BOOL m_bSupportAmbiguousUuid;

    CComboBox m_cDefaultConnectionState;

    DWORD m_dwMaxHeadAge;
    DWORD m_dwCacheDropAge;
    DWORD m_dwCacheDropMaxSize;

    DWORD m_dwMaxFailuresUntilDrop;
};
