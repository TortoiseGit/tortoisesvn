// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "Settings.h"
#include "AppUtils.h"
#include "../../TSVNCache/CacheInterface.h"

#define BOTTOMMARG 32

IMPLEMENT_DYNAMIC(CSettings, CTreePropSheet)
CSettings::CSettings(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CTreePropSheet(nIDCaption, pParentWnd, iSelectPage)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    AddPropPages();
}

CSettings::~CSettings()
{
    RemovePropPages();
}

void CSettings::AddPropPages()
{
    m_pMainPage = new CSetMainPage();
    m_pOverlayPage = new CSetOverlayPage();
    m_pOverlaysPage = new CSetOverlayIcons();
    m_pOverlayHandlersPage = new CSetOverlayHandlers();
    m_pProxyPage = new CSetProxyPage();
    m_pProgsDiffPage = new CSettingsProgsDiff();
    m_pProgsMergePage = new CSettingsProgsMerge();
    m_pLookAndFeelPage = new CSetLookAndFeelPage();
    m_pDialogsPage = new CSetDialogs();
    m_pMiscPage = new CSetMisc();
    m_pDialogs3Page = new SettingsDialogs3();
    m_pRevisionGraphPage = new CSettingsRevisionGraph();
    m_pRevisionGraphColorsPage = new CSettingsRevisionGraphColors();
    m_pLogCachePage = new CSetLogCache();
    m_pLogCacheListPage = new CSettingsLogCaches();
    m_pColorsPage = new CSettingsColors();
    m_pSavedPage = new CSetSavedDataPage();
    m_pHooksPage = new CSetHooks();
    m_pBugTraqPage = new CSetBugTraq();
    m_pTBlamePage = new CSettingsTBlame();
    m_pAdvanced = new CSettingsAdvanced();

    SetPageIcon(m_pMainPage, m_pMainPage->GetIconID());
    SetPageIcon(m_pOverlayPage, m_pOverlayPage->GetIconID());
    SetPageIcon(m_pOverlaysPage, m_pOverlaysPage->GetIconID());
    SetPageIcon(m_pOverlayHandlersPage, m_pOverlayHandlersPage->GetIconID());
    SetPageIcon(m_pProxyPage, m_pProxyPage->GetIconID());
    SetPageIcon(m_pProgsDiffPage, m_pProgsDiffPage->GetIconID());
    SetPageIcon(m_pProgsMergePage, m_pProgsMergePage->GetIconID());
    SetPageIcon(m_pLookAndFeelPage, m_pLookAndFeelPage->GetIconID());
    SetPageIcon(m_pDialogsPage, m_pDialogsPage->GetIconID());
    SetPageIcon(m_pRevisionGraphPage, m_pRevisionGraphPage->GetIconID());
    SetPageIcon(m_pRevisionGraphColorsPage, m_pRevisionGraphColorsPage->GetIconID());
    SetPageIcon(m_pMiscPage, m_pMiscPage->GetIconID());
    SetPageIcon(m_pDialogs3Page, m_pDialogs3Page->GetIconID());
    SetPageIcon(m_pLogCachePage, m_pLogCachePage->GetIconID());
    SetPageIcon(m_pLogCacheListPage, m_pLogCacheListPage->GetIconID());
    SetPageIcon(m_pColorsPage, m_pColorsPage->GetIconID());
    SetPageIcon(m_pSavedPage, m_pSavedPage->GetIconID());
    SetPageIcon(m_pHooksPage, m_pHooksPage->GetIconID());
    SetPageIcon(m_pBugTraqPage, m_pBugTraqPage->GetIconID());
    SetPageIcon(m_pTBlamePage, m_pTBlamePage->GetIconID());
    SetPageIcon(m_pAdvanced, m_pAdvanced->GetIconID());

    // don't change the order here, since the
    // page number can be passed on the command line!
    AddPage(m_pMainPage);
    AddPage(m_pRevisionGraphPage);
    AddPage(m_pRevisionGraphColorsPage);
    AddPage(m_pOverlayPage);
    AddPage(m_pOverlaysPage);
    AddPage(m_pOverlayHandlersPage);
    AddPage(m_pProxyPage);
    AddPage(m_pProgsDiffPage);
    AddPage(m_pProgsMergePage);
    AddPage(m_pLookAndFeelPage);
    AddPage(m_pDialogsPage);
    AddPage(m_pMiscPage);
    AddPage(m_pDialogs3Page);
    AddPage(m_pColorsPage);
    AddPage(m_pSavedPage);
    AddPage(m_pLogCachePage);
    AddPage(m_pLogCacheListPage);
    AddPage(m_pHooksPage);
    AddPage(m_pBugTraqPage);
    AddPage(m_pTBlamePage);
    AddPage(m_pAdvanced);
}

void CSettings::RemovePropPages()
{
    delete m_pMainPage;
    delete m_pOverlayPage;
    delete m_pOverlaysPage;
    delete m_pOverlayHandlersPage;
    delete m_pProxyPage;
    delete m_pProgsDiffPage;
    delete m_pProgsMergePage;
    delete m_pLookAndFeelPage;
    delete m_pDialogsPage;
    delete m_pRevisionGraphColorsPage;
    delete m_pRevisionGraphPage;
    delete m_pMiscPage;
    delete m_pDialogs3Page;
    delete m_pLogCachePage;
    delete m_pLogCacheListPage;
    delete m_pColorsPage;
    delete m_pSavedPage;
    delete m_pHooksPage;
    delete m_pBugTraqPage;
    delete m_pTBlamePage;
    delete m_pAdvanced;
}

void CSettings::HandleRestart()
{
    int restart = ISettingsPropPage::Restart_None;
    restart |= m_pMainPage->GetRestart();
    restart |= m_pOverlayPage->GetRestart();
    restart |= m_pOverlaysPage->GetRestart();
    restart |= m_pOverlayHandlersPage->GetRestart();
    restart |= m_pProxyPage->GetRestart();
    restart |= m_pProgsDiffPage->GetRestart();
    restart |= m_pProgsMergePage->GetRestart();
    restart |= m_pLookAndFeelPage->GetRestart();
    restart |= m_pDialogsPage->GetRestart();
    restart |= m_pRevisionGraphPage->GetRestart();
    restart |= m_pMiscPage->GetRestart();
    restart |= m_pDialogs3Page->GetRestart();
    restart |= m_pLogCachePage->GetRestart();
    restart |= m_pLogCacheListPage->GetRestart();
    restart |= m_pColorsPage->GetRestart();
    restart |= m_pSavedPage->GetRestart();
    restart |= m_pHooksPage->GetRestart();
    restart |= m_pBugTraqPage->GetRestart();
    restart |= m_pTBlamePage->GetRestart();
    restart |= m_pAdvanced->GetRestart();
    if (restart & ISettingsPropPage::Restart_System)
    {
        DWORD_PTR res = 0;
        ::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 20, &res);
        TSVNMessageBox(NULL, IDS_SETTINGS_RESTARTSYSTEM, IDS_APPNAME, MB_ICONINFORMATION);
    }
    if (restart & ISettingsPropPage::Restart_Cache)
    {
        DWORD_PTR res = 0;
        ::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 20, &res);
        // tell the cache to refresh everything
        SendCacheCommand(TSVNCACHECOMMAND_REFRESHALL);
    }
}

BEGIN_MESSAGE_MAP(CSettings, CTreePropSheet)
    ON_WM_QUERYDRAGICON()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CSettings::OnInitDialog()
{
    BOOL bResult = CTreePropSheet::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    MARGINS margs;
    margs.cxLeftWidth = 0;
    margs.cyTopHeight = 0;
    margs.cxRightWidth = 0;
    margs.cyBottomHeight = BOTTOMMARG;

    if ((DWORD)CRegDWORD(L"Software\\TortoiseSVN\\EnableDWMFrame", TRUE))
    {
        m_Dwm.Initialize();
        m_Dwm.DwmExtendFrameIntoClientArea(m_hWnd, &margs);
        m_aeroControls.SubclassOkCancelHelp(this);
        m_aeroControls.SubclassControl(this, ID_APPLY_NOW);
    }

    CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    return bResult;
}

void CSettings::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CTreePropSheet::OnPaint();
    }
}

HCURSOR CSettings::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

BOOL CSettings::OnEraseBkgnd(CDC* pDC)
{
    CTreePropSheet::OnEraseBkgnd(pDC);

    if (m_Dwm.IsDwmCompositionEnabled())
    {
        // draw the frame margins in black
        RECT rc;
        GetClientRect(&rc);
        pDC->FillSolidRect(rc.left, rc.bottom-BOTTOMMARG, rc.right-rc.left, BOTTOMMARG, RGB(0,0,0));
    }
    return TRUE;
}