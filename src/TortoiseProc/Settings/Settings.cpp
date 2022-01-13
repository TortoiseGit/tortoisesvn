// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2020-2022 - TortoiseSVN

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
#include "Theme.h"
#include "DarkModeHelper.h"
#include "../../Utils/PathUtils.h"
#include "../../Utils/StringUtils.h"

#define BOTTOMMARG 32

IMPLEMENT_DYNAMIC(CSettings, CTreePropSheet)
CSettings::CSettings(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    : CTreePropSheet(nIDCaption, pParentWnd, iSelectPage)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    AddPropPages();
    CSettings::SetTheme(CTheme::Instance().IsDarkTheme());
}

CSettings::~CSettings()
{
    RemovePropPages();
}

void CSettings::SetTheme(bool bDark)
{
    __super::SetTheme(bDark);
    for (int i = 0; i < GetPageCount(); ++i)
    {
        auto pPage = GetPage(i);
        if (IsWindow(pPage->GetSafeHwnd()))
            CTheme::Instance().SetThemeForDialog(pPage->GetSafeHwnd(), bDark);
    }
}

void CSettings::AddPropPage(ISettingsPropPage* page)
{
    AddPage(page);
    SetPageIcon(page, page->GetIconID());
}

void CSettings::AddPropPages()
{
    PWSTR   pszPath = nullptr;
    CString sysPath;
    if (SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_CREATE, nullptr, &pszPath) == S_OK)
    {
        sysPath = pszPath;
        CoTaskMemFree(pszPath);
    }
    auto             explorerVersion = CPathUtils::GetVersionFromFile(sysPath + L"\\shell32.dll");
    std::vector<int> versions;
    stringtok(versions, explorerVersion, true, L".");
    bool isWin11OrLater    = versions.size() > 3 && versions[2] >= 22000;
    m_pMainPage            = new CSetMainPage();
    m_pOverlayPage         = new CSetOverlayPage();
    m_pOverlaysPage        = new CSetOverlayIcons();
    m_pOverlayHandlersPage = new CSetOverlayHandlers();
    m_pProxyPage           = new CSetProxyPage();
    m_pProgsDiffPage       = new CSettingsProgsDiff();
    m_pProgsMergePage      = new CSettingsProgsMerge();
    m_pLookAndFeelPage     = new CSetLookAndFeelPage();
    if (isWin11OrLater)
        m_pWin11ContextMenu = new CSetWin11ContextMenu();
    m_pDialogsPage             = new CSetDialogs();
    m_pMiscPage                = new CSetMisc();
    m_pDialogs3Page            = new SettingsDialogs3();
    m_pRevisionGraphPage       = new CSettingsRevisionGraph();
    m_pRevisionGraphColorsPage = new CSettingsRevisionGraphColors();
    m_pLogCachePage            = new CSetLogCache();
    m_pLogCacheListPage        = new CSettingsLogCaches();
    m_pColorsPage              = new CSettingsColors();
    m_pSavedPage               = new CSetSavedDataPage();
    m_pHooksPage               = new CSetHooks();
    m_pBugTraqPage             = new CSetBugTraq();
    m_pTBlamePage              = new CSettingsTBlame();
    m_pAdvanced                = new CSettingsAdvanced();
    m_pSyncPage                = new CSettingsSync();
    m_pUDiffPage               = new CSettingsUDiff();
    
    // don't change the order here, since the
    // page number can be passed on the command line!
    AddPropPage(m_pMainPage);
    AddPropPage(m_pRevisionGraphPage);
    AddPropPage(m_pRevisionGraphColorsPage);
    AddPropPage(m_pOverlayPage);
    AddPropPage(m_pOverlaysPage);
    AddPropPage(m_pOverlayHandlersPage);
    AddPropPage(m_pProxyPage);
    AddPropPage(m_pProgsDiffPage);
    AddPropPage(m_pProgsMergePage);
    AddPropPage(m_pLookAndFeelPage);
    if (IsWindows10OrGreater())
        AddPropPage(m_pWin11ContextMenu);
    AddPropPage(m_pDialogsPage);
    AddPropPage(m_pMiscPage);
    AddPropPage(m_pDialogs3Page);
    AddPropPage(m_pColorsPage);
    AddPropPage(m_pSavedPage);
    AddPropPage(m_pLogCachePage);
    AddPropPage(m_pLogCacheListPage);
    AddPropPage(m_pHooksPage);
    AddPropPage(m_pBugTraqPage);
    AddPropPage(m_pTBlamePage);
    AddPropPage(m_pUDiffPage);
    AddPropPage(m_pSyncPage);
    AddPropPage(m_pAdvanced);
}

void CSettings::RemovePropPages() const
{
    delete m_pMainPage;
    delete m_pOverlayPage;
    delete m_pOverlaysPage;
    delete m_pOverlayHandlersPage;
    delete m_pProxyPage;
    delete m_pProgsDiffPage;
    delete m_pProgsMergePage;
    delete m_pLookAndFeelPage;
    delete m_pWin11ContextMenu;
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
    delete m_pUDiffPage;
    delete m_pSyncPage;
    delete m_pAdvanced;
}

void CSettings::HandleRestart() const
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
    restart |= m_pWin11ContextMenu->GetRestart();
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
    restart |= m_pUDiffPage->GetRestart();
    restart |= m_pSyncPage->GetRestart();
    restart |= m_pAdvanced->GetRestart();
    if (restart & ISettingsPropPage::Restart_System)
    {
        DWORD_PTR res = 0;
        ::SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 20, &res);
        TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_SETTINGS_RESTARTSYSTEM), nullptr, TDCBF_OK_BUTTON, TD_INFORMATION_ICON, nullptr);
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

    SetIcon(m_hIcon, TRUE);  // Set big icon
    SetIcon(m_hIcon, FALSE); // Set small icon

    MARGINS mArgs;
    mArgs.cxLeftWidth    = 0;
    mArgs.cyTopHeight    = 0;
    mArgs.cxRightWidth   = 0;
    mArgs.cyBottomHeight = BOTTOMMARG;

    if (m_aeroControls.AeroDialogsEnabled())
    {
        DwmExtendFrameIntoClientArea(m_hWnd, &mArgs);
        m_aeroControls.SubclassOkCancelHelp(this);
        m_aeroControls.SubclassControl(this, ID_APPLY_NOW);
    }

    DarkModeHelper::Instance().AllowDarkModeForApp(CTheme::Instance().IsDarkTheme());
    SetTheme(CTheme::Instance().IsDarkTheme());
    CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

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
        int   cxIcon = GetSystemMetrics(SM_CXICON);
        int   cyIcon = GetSystemMetrics(SM_CYICON);
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

// ReSharper disable once CppMemberFunctionMayBeConst
HCURSOR CSettings::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

BOOL CSettings::OnEraseBkgnd(CDC* pDC)
{
    CTreePropSheet::OnEraseBkgnd(pDC);

    if (m_aeroControls.AeroDialogsEnabled())
    {
        // draw the frame margins in black
        RECT rc;
        GetClientRect(&rc);
        pDC->FillSolidRect(rc.left, rc.bottom - BOTTOMMARG, rc.right - rc.left, BOTTOMMARG, RGB(0, 0, 0));
    }
    return TRUE;
}