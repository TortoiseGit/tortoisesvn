// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseSVN

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
#include "SettingsClearAuth.h"
#include "SVNAuthData.h"
#include "AppUtils.h"
#include <afxdialogex.h>


// CSettingsClearAuth dialog

IMPLEMENT_DYNAMIC(CSettingsClearAuth, CResizableStandAloneDialog)

CSettingsClearAuth::CSettingsClearAuth(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CSettingsClearAuth::IDD, pParent)
{

}

CSettingsClearAuth::~CSettingsClearAuth()
{
}

void CSettingsClearAuth::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_AUTHDATALIST, m_cAuthList);
}


BEGIN_MESSAGE_MAP(CSettingsClearAuth, CResizableStandAloneDialog)
END_MESSAGE_MAP()


// CSettingsClearAuth message handlers


BOOL CSettingsClearAuth::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();

    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassOkCancel(this);


    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES;
    m_cAuthList.SetExtendedStyle(exStyle);
    SetWindowTheme(m_cAuthList.GetSafeHwnd(), L"Explorer", NULL);

    FillAuthListControl();

    AddAnchor(IDC_INFO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_AUTHDATALIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_DWM, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);

    EnableSaveRestore(L"SettingsClearAuth");

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


void CSettingsClearAuth::OnOK()
{
    std::vector<std::tuple<CString, CString, SVNAuthDataInfo>> delList;
    for (int i = 0; i < m_cAuthList.GetItemCount(); ++i)
    {
        if (m_cAuthList.GetCheck(i))
        {
            if (m_cAuthList.GetItemText(i, 0).Compare(L"certificate")==0)
            {
                SHDeleteValue(HKEY_CURRENT_USER, L"Software\\TortoiseSVN\\CAPIAuthz", m_cAuthList.GetItemText(i, 1));
            }
            else
            {
                auto data = std::make_tuple(m_cAuthList.GetItemText(i, 0), m_cAuthList.GetItemText(i, 1), SVNAuthDataInfo());
                delList.push_back(data);
            }
        }
    }

    if (!delList.empty())
    {
        SVNAuthData authData;
        authData.DeleteAuthList(delList);
    }

    CResizableStandAloneDialog::OnOK();
}

void CSettingsClearAuth::FillAuthListControl()
{
    m_cAuthList.SetRedraw(false);
    m_cAuthList.DeleteAllItems();

    int c = ((CHeaderCtrl*)(m_cAuthList.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_cAuthList.DeleteColumn(c--);
    CString temp;
    temp.LoadString(IDS_SETTINGSCLEAR_COL1);
    m_cAuthList.InsertColumn(0, temp);
    temp.LoadString(IDS_SETTINGSCLEAR_COL2);
    m_cAuthList.InsertColumn(1, temp);
    temp.LoadString(IDS_SETTINGSCLEAR_COL3);
    m_cAuthList.InsertColumn(2, temp);

    SVNAuthData authData;
    auto authList = authData.GetAuthList();
    int iItem = 0;
    for (auto it: authList)
    {
        m_cAuthList.InsertItem (iItem,    std::get<0>(it));
        m_cAuthList.SetItemText(iItem, 1, std::get<1>(it));
        m_cAuthList.SetItemText(iItem, 2, std::get<2>(it).username);
        ++iItem;
    }

    CRegistryKey regCerts(L"Software\\TortoiseSVN\\CAPIAuthz");
    CStringList certList;
    regCerts.getValues(certList);
    for (POSITION pos = certList.GetHeadPosition(); pos != NULL; )
    {
        CString certHash = certList.GetNext(pos);
        CRegDWORD regCert(L"Software\\TortoiseSVN\\CAPIAuthz\\"+certHash);
        m_cAuthList.InsertItem (iItem,    L"certificate");
        m_cAuthList.SetItemText(iItem, 1, certHash);
        temp.Format(L"%d", (int)(DWORD)regCert);
        m_cAuthList.SetItemText(iItem, 2, temp);
        m_cAuthList.SetItemText(iItem, 3, L"");
        ++iItem;
    }


    int maxcol = ((CHeaderCtrl*)(m_cAuthList.GetDlgItem(0)))->GetItemCount()-1;
    for (int col = 0; col <= maxcol; col++)
    {
        m_cAuthList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
    }

    m_cAuthList.SetRedraw(true);
}
