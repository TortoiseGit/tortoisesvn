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

#include "SetMainPage.h"
#include "SetProxyPage.h"
#include "SetOverlayPage.h"
#include "SetOverlayIcons.h"
#include "SetOverlayHandlers.h"
#include "SettingsProgsDiff.h"
#include "SettingsProgsMerge.h"
#include "SetLookAndFeelPage.h"
#include "SetWin11ContextMenu.h"
#include "SetDialogs.h"
#include "SettingsColors.h"
#include "SetMisc.h"
#include "SetLogCache.h"
#include "SettingsLogCaches.h"
#include "SetSavedDataPage.h"
#include "SetHooks.h"
#include "SetBugTraq.h"
#include "SettingsTBlame.h"
#include "SettingsRevisionGraph.h"
#include "SettingsRevGraphColors.h"
#include "SettingsAdvanced.h"
#include "SettingsDialogs3.h"
#include "SettingsSync.h"
#include "SettingsTUDiff.h"

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
    m_pPages.clear();
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

ISettingsPropPage* CSettings::AddPropPage(std::unique_ptr<ISettingsPropPage> page)
{
    auto pagePtr = page.get();
    AddPage(pagePtr);
    SetPageIcon(pagePtr, page->GetIconID());
    m_pPages.push_back(std::move(page));
    return pagePtr;
}

ISettingsPropPage* CSettings::AddPropPage(std::unique_ptr<ISettingsPropPage> page, CPropertyPage* parentPage)
{
    auto pagePtr = AddPropPage(std::move(page));
    SetParentPage(parentPage, pagePtr);
    return pagePtr;
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
    bool isWin11OrLater = versions.size() > 3 && versions[2] >= 22000;

    // don't change the order here, since the
    // page number can be passed on the command line!
    auto pMainPage      = AddPropPage(std::make_unique<CSetMainPage>());
    auto pRevGraphPage  = AddPropPage(std::make_unique<CSettingsRevisionGraph>());
    AddPropPage(std::make_unique<CSettingsRevisionGraphColors>(), pRevGraphPage);
    auto pOverlayPage = AddPropPage(std::make_unique<CSetOverlayPage>());
    AddPropPage(std::make_unique<CSetOverlayIcons>(), pOverlayPage);
    AddPropPage(std::make_unique<CSetOverlayHandlers>(), pOverlayPage);
    AddPropPage(std::make_unique<CSetProxyPage>());
    auto pDiffPage = AddPropPage(std::make_unique<CSettingsProgsDiff>());
    AddPropPage(std::make_unique<CSettingsProgsMerge>(), pDiffPage);
    auto pContextMenuPage = AddPropPage(std::make_unique<CSetLookAndFeelPage>(), pMainPage);
    AddPropPage(std::make_unique<CSetDialogs>(), pMainPage);
    AddPropPage(std::make_unique<CSetMisc>(), pMainPage);
    AddPropPage(std::make_unique<SettingsDialogs3>(), pMainPage);
    AddPropPage(std::make_unique<CSettingsColors>(), pMainPage);
    AddPropPage(std::make_unique<CSetSavedDataPage>());
    auto pLogCachePage = AddPropPage(std::make_unique<CSetLogCache>());
    AddPropPage(std::make_unique<CSettingsLogCaches>(), pLogCachePage);
    auto pHookPage = AddPropPage(std::make_unique<CSetHooks>());
    AddPropPage(std::make_unique<CSetBugTraq>(), pHookPage);
    AddPropPage(std::make_unique<CSettingsTBlame>());
    AddPropPage(std::make_unique<CSettingsUDiff>());
    AddPropPage(std::make_unique<CSettingsSync>());
    AddPropPage(std::make_unique<CSettingsAdvanced>());
    if (isWin11OrLater)
        AddPropPage(std::make_unique<CSetWin11ContextMenu>(), pContextMenuPage);
}

void CSettings::HandleRestart() const
{
    int restart = ISettingsPropPage::Restart_None;
    for (const auto& page : m_pPages)
    {
        restart |= page->GetRestart();
    }
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