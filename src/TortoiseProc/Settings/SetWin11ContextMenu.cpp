// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2021 - TortoiseSVN

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
#include "SetWin11ContextMenu.h"

IMPLEMENT_DYNAMIC(CSetWin11ContextMenu, ISettingsPropPage)
CSetWin11ContextMenu::CSetWin11ContextMenu()
    : ISettingsPropPage(CSetWin11ContextMenu::IDD)
    , m_bModified(false)
    , m_bBlock(false)
{
    m_regTopMenu = CRegQWORD(L"Software\\TortoiseSVN\\ContextMenu11Entries",
                             static_cast<QWORD>(TSVNContextMenuEntries::Checkout |
                                                TSVNContextMenuEntries::Update |
                                                TSVNContextMenuEntries::Commit));
    m_topMenu    = static_cast<TSVNContextMenuEntries>(static_cast<QWORD>(m_regTopMenu));
}

CSetWin11ContextMenu::~CSetWin11ContextMenu()
{
}

void CSetWin11ContextMenu::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MENULIST, m_cMenuList);
}

BEGIN_MESSAGE_MAP(CSetWin11ContextMenu, ISettingsPropPage)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_MENULIST, OnLvnItemchangedMenulist)
END_MESSAGE_MAP()

BOOL CSetWin11ContextMenu::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_tooltips.AddTool(IDC_MENULIST, IDS_SETTINGS_MENULAYOUT_TT);

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

BOOL CSetWin11ContextMenu::OnApply()
{
    UpdateData();
    Store(static_cast<QWORD>(m_topMenu), m_regTopMenu);

    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

void CSetWin11ContextMenu::InsertItem(UINT nTextID, UINT nIconID, TSVNContextMenuEntries menu, int iconWidth, int iconHeight)
{
    auto    hIcon  = LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(nIconID), iconWidth, iconHeight);
    int     nImage = m_imgList.Add(hIcon);
    CString temp;
    temp.LoadString(nTextID);
    CStringUtils::RemoveAccelerators(temp);
    int nIndex = m_cMenuList.GetItemCount();
    m_cMenuList.InsertItem(nIndex, temp, nImage);
    m_cMenuList.SetCheck(nIndex, (m_topMenu & menu) != TSVNContextMenuEntries::None);
}

void CSetWin11ContextMenu::OnLvnItemchangedMenulist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
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
