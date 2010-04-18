// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006, 2009-2010 - TortoiseSVN

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
#include "Settings.h"
#include "SetMainPage.h"
#include "SetColorPage.h"

#define BOTTOMMARG 32

IMPLEMENT_DYNAMIC(CSettings, CPropertySheet)
CSettings::CSettings(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CSettings::CSettings(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    AddPropPages();
}

CSettings::~CSettings()
{
    RemovePropPages();
}

void CSettings::AddPropPages()
{
    m_pMainPage = new CSetMainPage();
    m_pColorPage = new CSetColorPage();

    AddPage(m_pMainPage);
    AddPage(m_pColorPage);
}

void CSettings::RemovePropPages()
{
    delete m_pMainPage;
    m_pMainPage = NULL;
    delete m_pColorPage;
    m_pColorPage = NULL;
}

void CSettings::SaveData()
{
    m_pMainPage->SaveData();
    m_pColorPage->SaveData();
}

BOOL CSettings::IsReloadNeeded() const
{
    BOOL bReload = FALSE;
    bReload = (m_pMainPage->m_bReloadNeeded || bReload);
    bReload = (m_pColorPage->m_bReloadNeeded || bReload);
    return bReload;
}

BEGIN_MESSAGE_MAP(CSettings, CPropertySheet)
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CSettings message handlers

BOOL CSettings::OnInitDialog()
{
    BOOL bResult = CPropertySheet::OnInitDialog();
    MARGINS margs;
    margs.cxLeftWidth = 0;
    margs.cyTopHeight = 0;
    margs.cxRightWidth = 0;
    margs.cyBottomHeight = BOTTOMMARG;

    m_Dwm.Initialize();
    m_Dwm.DwmExtendFrameIntoClientArea(m_hWnd, &margs);
    m_aeroControls.SubclassOkCancelHelp(this);
    m_aeroControls.SubclassControl(this, ID_APPLY_NOW);
    return bResult;
}

BOOL CSettings::OnEraseBkgnd(CDC* pDC)
{
    CPropertySheet::OnEraseBkgnd(pDC);

    if (m_Dwm.IsDwmCompositionEnabled())
    {
        // draw the frame margins in black
        RECT rc;
        GetClientRect(&rc);
        pDC->FillSolidRect(rc.left, rc.bottom-BOTTOMMARG, rc.right-rc.left, BOTTOMMARG, RGB(0,0,0));
    }
    return TRUE;
}