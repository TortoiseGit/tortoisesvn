// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012-2015, 2020-2022 - TortoiseSVN

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

#include "TreePropSheet.h"
#include "SettingsPropPage.h"
#include "AeroControls.h"

using namespace TreePropSheet;

/**
 * \ingroup TortoiseProc
 * This is the container for all settings pages. A setting page is
 * a class derived from CPropertyPage with an additional method called
 * SaveData(). The SaveData() method is used by the dialog to save
 * the settings the user has made - if that method is not called then
 * it means that the changes are discarded! Each settings page has
 * to make sure that no changes are saved outside that method.
 */
class CSettings : public CTreePropSheet
{
    DECLARE_DYNAMIC(CSettings)
private:
    /**
     * Adds all pages to this Settings-Dialog.
     */
    void AddPropPages();

    ISettingsPropPage* AddPropPage(std::unique_ptr<ISettingsPropPage> page);
    ISettingsPropPage* AddPropPage(std::unique_ptr<ISettingsPropPage> page, CPropertyPage* parentPage);

private:

    HICON                         m_hIcon;
    AeroControlBase               m_aeroControls;

public:
    CSettings(UINT nIDCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
    ~CSettings() override;

    void SetTheme(bool bDark) override;

    /**
     * Calls the SaveData()-methods of each of the settings pages.
     */
    void HandleRestart() const;

protected:
    DECLARE_MESSAGE_MAP()
    BOOL            OnInitDialog() override;
    afx_msg void    OnPaint();
    afx_msg BOOL    OnEraseBkgnd(CDC* pDC);
    afx_msg HCURSOR OnQueryDragIcon();

private:
    std::vector<std::unique_ptr<ISettingsPropPage>> m_pPages;
};
