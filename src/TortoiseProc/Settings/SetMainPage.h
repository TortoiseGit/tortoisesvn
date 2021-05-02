// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013, 2015, 2021 - TortoiseSVN

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

/**
 * \ingroup TortoiseProc
 * This is the main page of the settings. It contains all the most important
 * settings.
 */
class CSetMainPage : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSetMainPage)

public:
    CSetMainPage();
    ~CSetMainPage() override;

    UINT GetIconID() override { return IDI_GENERAL; }

    enum
    {
        IDD = IDD_SETTINGSMAIN
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    BOOL         OnApply() override;
    afx_msg void OnModified();
    afx_msg void OnBnClickedEditconfig();
    afx_msg void OnBnClickedChecknewerbutton();
    afx_msg void OnBnClickedSounds();
    afx_msg void OnBnClickedCreatelib();

    DECLARE_MESSAGE_MAP()

private:
    CRegString m_regExtensions;
    CString    m_sTempExtensions;
    CComboBox  m_languageCombo;
    CRegDWORD  m_regLanguage;
    DWORD      m_dwLanguage;
    CRegString m_regLastCommitTime;
    BOOL       m_bLastCommitTime;
    CRegDWORD  m_regUseAero;
    BOOL       m_bUseAero;
};
