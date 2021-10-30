// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2021 - TortoiseSVN

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
#include "Globals.h"
#include "StringUtils.h"
#include "LoadIconEx.h"
#include "SetLookAndFeelPage.h"

IMPLEMENT_DYNAMIC(CSetLookAndFeelPage, ISettingsPropPage)
CSetLookAndFeelPage::CSetLookAndFeelPage()
    : ISettingsPropPage(CSetLookAndFeelPage::IDD)
    , m_bModified(false)
    , m_bBlock(false)
    , m_bGetLockTop(FALSE)
    , m_bHideMenus(false)
{
    m_regTopMenu     = CRegDWORD(L"Software\\TortoiseSVN\\ContextMenuEntries", static_cast<DWORD>(TSVNContextMenuEntries::Checkout | TSVNContextMenuEntries::Update | TSVNContextMenuEntries::Commit));
    m_regTopMenuHigh = CRegDWORD(L"Software\\TortoiseSVN\\ContextMenuEntrieshigh", 0);

    m_topMenu = static_cast<TSVNContextMenuEntries>(static_cast<unsigned long long>(static_cast<DWORD>(m_regTopMenuHigh)) << 32);
    m_topMenu |= static_cast<TSVNContextMenuEntries>(static_cast<unsigned long long>(static_cast<DWORD>(m_regTopMenu)));

    m_regGetLockTop = CRegDWORD(L"Software\\TortoiseSVN\\GetLockTop", TRUE);
    m_bGetLockTop   = m_regGetLockTop;
    m_regHideMenus  = CRegDWORD(L"Software\\TortoiseSVN\\HideMenusForUnversionedItems", FALSE);
    m_bHideMenus    = m_regHideMenus;

    m_regNoContextPaths = CRegString(L"Software\\TortoiseSVN\\NoContextPaths", L"");
    m_sNoContextPaths   = m_regNoContextPaths;
    m_sNoContextPaths.Replace(L"\n", L"\r\n");

    m_regEnableDragContextMenu = CRegDWORD(L"Software\\TortoiseSVN\\EnableDragContextMenu", TRUE);
    m_bEnableDragContextMenu   = m_regEnableDragContextMenu;
}

CSetLookAndFeelPage::~CSetLookAndFeelPage()
{
}

void CSetLookAndFeelPage::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MENULIST, m_cMenuList);
    DDX_Check(pDX, IDC_GETLOCKTOP, m_bGetLockTop);
    DDX_Check(pDX, IDC_HIDEMENUS, m_bHideMenus);
    DDX_Text(pDX, IDC_NOCONTEXTPATHS, m_sNoContextPaths);
    DDX_Check(pDX, IDC_ENABLEDRAGCONTEXTMENU, m_bEnableDragContextMenu);
}

BEGIN_MESSAGE_MAP(CSetLookAndFeelPage, ISettingsPropPage)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
    ON_BN_CLICKED(IDC_GETLOCKTOP, OnChange)
    ON_BN_CLICKED(IDC_HIDEMENUS, OnChange)
    ON_EN_CHANGE(IDC_NOCONTEXTPATHS, &CSetLookAndFeelPage::OnEnChangeNocontextpaths)
    ON_BN_CLICKED(IDC_ENABLEDRAGCONTEXTMENU, OnChange)
END_MESSAGE_MAP()

BOOL CSetLookAndFeelPage::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_tooltips.AddTool(IDC_MENULIST, IDS_SETTINGS_MENULAYOUT_TT);
    m_tooltips.AddTool(IDC_GETLOCKTOP, IDS_SETTINGS_GETLOCKTOP_TT);
    m_tooltips.AddTool(IDC_HIDEMENUS, IDS_SETTINGS_HIDEMENUS_TT);
    m_tooltips.AddTool(IDC_NOCONTEXTPATHS, IDS_SETTINGS_EXCLUDECONTEXTLIST_TT);
    m_tooltips.AddTool(IDC_ENABLEDRAGCONTEXTMENU, IDS_SETTINGS_ENABLEDRAGCONTEXTMENU_TT);

    m_cMenuList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_cMenuList.DeleteAllItems();
    int c = m_cMenuList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_cMenuList.DeleteColumn(c--);
    m_cMenuList.InsertColumn(0, L"");

    SetWindowTheme(m_cMenuList.GetSafeHwnd(), L"Explorer", nullptr);

    m_cMenuList.SetRedraw(false);

    int iconWidth  = GetSystemMetrics(SM_CXSMICON);
    int iconHeight = GetSystemMetrics(SM_CYSMICON);
    m_imgList.Create(iconWidth, iconHeight, ILC_COLOR16 | ILC_MASK, 4, 1);

    m_bBlock = true;

    for (const auto& [menu, name, icon] : TSVNContextMenuEntriesVec)
    {
        InsertItem(name, icon, menu, iconWidth, iconHeight);
    }

    m_bBlock = false;

    m_cMenuList.SetImageList(&m_imgList, LVSIL_SMALL);
    int minCol = 0;
    int maxCol = m_cMenuList.GetHeaderCtrl()->GetItemCount() - 1;
    for (int col = minCol; col <= maxCol; col++)
    {
        m_cMenuList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
    }
    m_cMenuList.SetRedraw(true);

    UpdateData(FALSE);

    return TRUE;
}

BOOL CSetLookAndFeelPage::OnApply()
{
    UpdateData();
    Store(static_cast<DWORD>(static_cast<QWORD>(m_topMenu) & 0xFFFFFFFF), m_regTopMenu);
    Store(static_cast<DWORD>(static_cast<QWORD>(m_topMenu) >> 32), m_regTopMenuHigh);
    Store(m_bGetLockTop, m_regGetLockTop);
    Store(m_bHideMenus, m_regHideMenus);
    Store(m_bEnableDragContextMenu, m_regEnableDragContextMenu);

    m_sNoContextPaths.Remove('\r');
    if (m_sNoContextPaths.Right(1).Compare(L"\n") != 0)
        m_sNoContextPaths += L"\n";

    Store(m_sNoContextPaths, m_regNoContextPaths);

    m_sNoContextPaths.Replace(L"\n", L"\r\n");
    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

void CSetLookAndFeelPage::InsertItem(UINT nTextID, UINT nIconID, TSVNContextMenuEntries dwFlags, int iconWidth, int iconHeight)
{
    auto    hIcon  = LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(nIconID), iconWidth, iconHeight);
    int     nImage = m_imgList.Add(hIcon);
    CString temp;
    temp.LoadString(nTextID);
    CStringUtils::RemoveAccelerators(temp);
    int nIndex = m_cMenuList.GetItemCount();
    m_cMenuList.InsertItem(nIndex, temp, nImage);
    m_cMenuList.SetCheck(nIndex, (m_topMenu & dwFlags) != TSVNContextMenuEntries::None);
}

void CSetLookAndFeelPage::OnLvnItemchangedMenulist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    if (m_bBlock)
        return;
    SetModified(TRUE);
    if (m_cMenuList.GetItemCount() > 0)
    {
        int i     = 0;
        m_topMenu = TSVNContextMenuEntries::None;
        for (const auto& [menu, name, icon] : TSVNContextMenuEntriesVec)
        {
            m_topMenu |= m_cMenuList.GetCheck(i++) ? menu : TSVNContextMenuEntries::None;
        }
    }
    *pResult = 0;
}

void CSetLookAndFeelPage::OnChange()
{
    SetModified();
}

void CSetLookAndFeelPage::OnEnChangeNocontextpaths()
{
    SetModified();
}
