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
#include "registry.h"

class CProgressDlg;

/**
 * \ingroup TortoiseProc
 * Settings page to configure miscellaneous stuff.
 */
class CSettingsRevisionGraph
    : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSettingsRevisionGraph)

public:
    CSettingsRevisionGraph();
    ~CSettingsRevisionGraph() override;

    UINT GetIconID() override { return IDI_SETTINGSREVGRAPH; }

    BOOL OnApply() override;

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSREVGRAPH
    };

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;

    afx_msg void OnChanged();

    DECLARE_MESSAGE_MAP()

private:
    CRegString regTrunkPattern;
    CRegString regBranchesPattern;
    CRegString regTagsPattern;
    CRegDWORD  regTweakTrunkColors;
    CRegDWORD  regTweakTagsColors;

    CString trunkPattern;
    CString branchesPattern;
    CString tagsPattern;
    BOOL    tweakTrunkColors;
    BOOL    tweakTagsColors;
};
