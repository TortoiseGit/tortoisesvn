// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "Colors.h"
#include "ToolTip.h"

/**
 * \ingroup TortoiseProc
 * Settings property page to set custom colors used in TortoiseSVN
 */
class CSettingsRevisionGraphColors : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSettingsRevisionGraphColors)

public:
    CSettingsRevisionGraphColors();
    virtual ~CSettingsRevisionGraphColors();

    UINT GetIconID() {return IDI_LOOKANDFEEL;}

    enum { IDD = IDD_SETTINGSREVGRAPHCOLORS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedColor();
    afx_msg void OnBnClickedRestore();
    virtual BOOL OnApply();
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    DECLARE_MESSAGE_MAP()
private:

    // utility methods

    void InitColorPicker (CMFCColorButton& button, CColors::GDIPlusColor color);
    void ResetColor (CMFCColorButton& button, CColors::GDIPlusColor color);
    void ApplyColor (CMFCColorButton& button, CColors::GDIPlusColor color, DWORD alpha = 255);

    void InitColorPicker (CMFCColorButton& button, CColors::GDIPlusColorTable table, int index);
    void ResetColor (CMFCColorButton& button, CColors::GDIPlusColorTable table, int index);
    void ApplyColor (CMFCColorButton& button, CColors::GDIPlusColorTable table, int index);

    // controls

    CMFCColorButton m_cAddedNode;
    CMFCColorButton m_cDeletedNode;
    CMFCColorButton m_cRenamedNode;
    CMFCColorButton m_cModifiedNode;
    CMFCColorButton m_cUnchangedNode;
    CMFCColorButton m_cLastCommitNode;
    CMFCColorButton m_cWCNode;
    CMFCColorButton m_cWCNodeBorder;

    CMFCColorButton m_cTagOverlay;
    CMFCColorButton m_cTrunkOverlay;

    CMFCColorButton m_cTagMarker;
    CMFCColorButton m_cTrunkMarker;

    CMFCColorButton m_cStripeColor1;
    CMFCColorButton m_cStripeColor2;

    BYTE m_sStripeAlpha1;
    BYTE m_sStripeAlpha2;

    CColors         m_Colors;

    CToolTips       m_tooltips;
public:
};
