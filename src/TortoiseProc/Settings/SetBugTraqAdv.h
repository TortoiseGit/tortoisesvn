// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2010, 2013, 2015, 2021 - TortoiseSVN

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
#include "resource.h"
#include "StandAloneDlg.h"

class CBugTraqAssociation;
class CBugTraqAssociations;

/**
 * \ingroup TortoiseProc
 * helper dialog to configure client side hook scripts.
 */
class CSetBugTraqAdv : public CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CSetBugTraqAdv)

public:
    CSetBugTraqAdv(CWnd* pParent = nullptr);
    CSetBugTraqAdv(const CBugTraqAssociation& assoc, CWnd* pParent = nullptr);
    ~CSetBugTraqAdv() override;

    CBugTraqAssociation GetAssociation() const;
    void                SetAssociations(CBugTraqAssociations* as) { m_pAssociations = as; }

    // Dialog Data
    enum
    {
        IDD = IDD_SETTINGSBUGTRAQADV
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnInitDialog() override;
    void         OnOK() override;
    afx_msg void OnDestroy();
    afx_msg void OnBnClickedBugTraqbrowse();
    afx_msg void OnBnClickedHelp();
    afx_msg void OnCbnSelchangeBugtraqprovidercombo();
    afx_msg void OnBnClickedOptions();

    DECLARE_MESSAGE_MAP()

    void CheckHasOptions();

protected:
    CString               m_sPath;
    CLSID                 m_providerClsid;
    CString               m_sParameters;
    CComboBox             m_cProviderCombo;
    CBugTraqAssociations* m_pAssociations;
};