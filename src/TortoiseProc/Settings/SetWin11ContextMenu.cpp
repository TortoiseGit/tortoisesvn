// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2021-2022 - TortoiseSVN

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

#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>

#include "../../Utils/PathUtils.h"
#include "../../Utils/MiscUI/ProgressDlg.h"
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;

#pragma comment(lib, "windowsapp.lib")

IMPLEMENT_DYNAMIC(CSetWin11ContextMenu, ISettingsPropPage)
CSetWin11ContextMenu::CSetWin11ContextMenu()
    : ISettingsPropPage(CSetWin11ContextMenu::IDD)
    , m_bModified(false)
    , m_bBlock(false)
{
    m_regTopMenu = CRegQWORD(L"Software\\TortoiseSVN\\ContextMenu11Entries", static_cast<QWORD>(defaultWin11TopMenuEntries));
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
    ON_BN_CLICKED(IDC_REGISTER, &CSetWin11ContextMenu::OnBnClickedRegister)
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

void CSetWin11ContextMenu::OnBnClickedRegister()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    OnOutOfScope(CoUninitialize());
    PackageManager                                                    manager;

    // first unregister if already registered
    Collections::IIterable<winrt::Windows::ApplicationModel::Package> packages;
    try
    {
        packages = manager.FindPackagesForUser(L"");
    }
    catch (winrt::hresult_error const& ex)
    {
        std::wstring error   = L"FindPackagesForUser failed (Errorcode: ";
        error += std::to_wstring(ex.code().value);
        error += L"):\n";
        error += ex.message();
        MessageBox(error.c_str(), nullptr, MB_ICONERROR);
        return;
    }

    for (const auto& package : packages)
    {
        if (package.Id().Name() != L"3A48D7FC-AEE2-4CBC-91D1-0007951B8006")
            continue;

        winrt::hstring fullName            = package.Id().FullName();
        auto           deploymentOperation = manager.RemovePackageAsync(fullName, RemovalOptions::None);
        auto           deployResult        = deploymentOperation.get();
        if (SUCCEEDED(deployResult.ExtendedErrorCode()))
            break;

        // Undeployment failed
        std::wstring error   = L"RemovePackageAsync failed (Errorcode: ";
        error += std::to_wstring(deployResult.ExtendedErrorCode());
        error += L"):\n";
        error += deployResult.ErrorText();
        MessageBox(error.c_str(), nullptr, MB_ICONERROR);
        return;
    }

    // now register the package
    auto              appDir = CPathUtils::GetAppParentDirectory();
    Uri               externalUri(static_cast<LPCWSTR>(appDir));
    auto packagePath = appDir + L"bin\\package.msix";
    Uri packageUri(static_cast<LPCWSTR>(packagePath));
    AddPackageOptions options;
    options.ExternalLocationUri(externalUri);
    auto deploymentOperation = manager.AddPackageByUriAsync(packageUri, options);

    auto deployResult        = deploymentOperation.get();

    if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
    {
        std::wstring error   = L"AddPackageByUriAsync failed (Errorcode: ";
        error += std::to_wstring(deployResult.ExtendedErrorCode());
        error += L"):\n";
        error += deployResult.ErrorText();
        MessageBox(error.c_str(), nullptr, MB_ICONERROR);
        return;
    }
    MessageBox(CString(MAKEINTRESOURCE(IDS_PACKAGE_REGISTERED)));
}
