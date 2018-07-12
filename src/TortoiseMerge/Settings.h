// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006, 2009, 2015, 2018 - TortoiseSVN

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
#include "AeroControls.h"
#include "DPIAware.h"

class CSetMainPage;
class CSetColorPage;

/**
 * \ingroup TortoiseMerge
 * This is the container for all settings pages. A setting page is
 * a class derived from CPropertyPage with an additional method called
 * SaveData(). The SaveData() method is used by the dialog to save
 * the settings the user has made - if that method is not called then
 * it means that the changes are discarded! Each settings page has
 * to make sure that no changes are saved outside that method.
 *
 */
class CSettings : public CPropertySheet
{
    DECLARE_DYNAMIC(CSettings)
private:
    /**
     * Adds all pages to this Settings-Dialog.
     */
    void AddPropPages();
    /**
     * Removes the pages and frees up memory.
     */
    void RemovePropPages();

    void BuildPropPageArray() override
    {
        CPropertySheet::BuildPropPageArray();

        LPCPROPSHEETPAGE ppsp = m_psh.ppsp;
        auto nSize = m_pages.GetSize();
        for (decltype(nSize) nPage = 0; nPage < nSize; nPage++)
        {
            const DLGTEMPLATE* pResource = ppsp->pResource;
            CDialogTemplate dlgTemplate(pResource);
            dlgTemplate.SetFont(L"MS Shell Dlg 2", 9);
            memmove((void*)pResource, dlgTemplate.m_hTemplate, dlgTemplate.m_dwTemplateSize);
            (BYTE*&)ppsp += ppsp->dwSize;
        }
    }
private:
    CSetMainPage *      m_pMainPage;
    CSetColorPage *     m_pColorPage;
    AeroControlBase     m_aeroControls;

public:
    CSettings(UINT nIDCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
    CSettings(LPCTSTR pszCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
    virtual ~CSettings();

    /**
     * Calls the SaveData()-methods of each of the settings pages.
     */
    void SaveData();

    BOOL IsReloadNeeded() const;
protected:
    DECLARE_MESSAGE_MAP()
    virtual BOOL OnInitDialog();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


