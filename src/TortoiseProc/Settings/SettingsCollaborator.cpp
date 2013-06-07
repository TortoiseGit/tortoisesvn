// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

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
#include "SettingsCollaborator.h"
#include "AppUtils.h"
#include "TSVNAuth.h"
#include <afxdialogex.h>
#include <atlbase.h>
#include <atlconv.h>

// SettingsCollaborator dialog

IMPLEMENT_DYNAMIC(SettingsCollaborator, ISettingsPropPage)

SettingsCollaborator::SettingsCollaborator()
    : ISettingsPropPage(SettingsCollaborator::IDD),
    m_collabGuiPath(L""),
    m_svnUser(L""),
    m_svnPassword(L""),
    m_collabUser(L""),
    m_collabPassword(L"")
{
    m_regCollabGuiPath  = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\PathToCollabGui", L"");
    m_regCollabUser     = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\CollabUser", L"");
    m_regCollabPassword = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\CollabPassword", L"");
    m_regSvnUser        = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\SvnUser", L"");
    m_regSvnPassword    = CRegString(L"Software\\TortoiseSVN\\CodeCollaborator\\SvnPassword", L"");
}

SettingsCollaborator::~SettingsCollaborator()
{
}

void SettingsCollaborator::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_COLLABORATOR_PATH, m_collabGuiPath);
    DDX_Text(pDX, IDC_COLLABORATOR_USEREDIT, m_collabUser);
    DDX_Text(pDX, IDC_COLLABORATOR_PASSEDIT, m_collabPassword);
    DDX_Text(pDX, IDC_SVN_USEREDIT, m_svnUser);
    DDX_Text(pDX, IDC_SVN_PASSEDIT, m_svnPassword);
}

BEGIN_MESSAGE_MAP(SettingsCollaborator, ISettingsPropPage)
    ON_BN_CLICKED(IDC_EXTDIFFBROWSE, &SettingsCollaborator::OnBnClickedBrowse)
    ON_EN_CHANGE(IDC_COLLABORATOR_USEREDIT, OnChange)
    ON_EN_CHANGE(IDC_COLLABORATOR_PASSEDIT, OnChange)
    ON_EN_CHANGE(IDC_SVN_USEREDIT, OnChange)
    ON_EN_CHANGE(IDC_SVN_PASSEDIT, OnChange)
    ON_EN_CHANGE(IDC_COLLABORATOR_PATH, OnChange)
END_MESSAGE_MAP()


// SettingsCollaborator message handlers

void SettingsCollaborator::OnBnClicked()
{
    SetModified();
}

BOOL SettingsCollaborator::OnInitDialog()
{
    USES_CONVERSION;
    ISettingsPropPage::OnInitDialog();

    EnableToolTips();

    m_collabGuiPath = (CString)m_regCollabGuiPath;
    m_svnUser = (CString)m_regSvnUser;
    
    m_collabUser = (CString)m_regCollabUser;
    CString encSvnPassword = (CString)m_regSvnPassword;
    CString encCollabPassword = (CString)m_regCollabPassword;

    Creds creds;
    m_svnPassword = A2T(creds.Decrypt(T2A(encSvnPassword)));
    m_collabPassword = A2T(creds.Decrypt(T2A(encCollabPassword)));
    m_tooltips.Create(this);

    m_tooltips.AddTool(IDC_COLLABORATOR_PATH, IDS_SETTINGS_COLLABORATOR_PATH_TT);
    m_tooltips.AddTool(IDC_COLLABORATOR_USEREDIT, IDS_SETTINGS_COLLABORATOR_USEREDIT_TT);
    m_tooltips.AddTool(IDC_COLLABORATOR_PASSEDIT, IDS_SETTINGS_COLLABORATOR_PASSEDIT_TT);
    m_tooltips.AddTool(IDC_SVN_USEREDIT, IDS_SETTINGS_SVN_USEREDIT_TT);
    m_tooltips.AddTool(IDC_SVN_PASSEDIT, IDS_SETTINGS_SVN_PASSEDIT_TT);

    UpdateData(FALSE);
    return TRUE;
}

BOOL SettingsCollaborator::OnApply()
{
    USES_CONVERSION;
    UpdateData();
    Creds creds;
    CString encSvnPassword = A2T(creds.Encrypt(T2A(m_svnPassword)));
    CString encCollabPassword = A2T(creds.Encrypt(T2A(m_collabPassword)));
    Store (m_collabGuiPath, m_regCollabGuiPath);
    Store (m_svnUser, m_regSvnUser);
    Store (encSvnPassword, m_regSvnPassword);
    Store (m_collabUser, m_regCollabUser);
    Store (encCollabPassword, m_regCollabPassword);
    
    SetModified(FALSE);

    return ISettingsPropPage::OnApply();
}

BOOL SettingsCollaborator::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);

    return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void SettingsCollaborator::OnBnClickedBrowse()
{
    if (CAppUtils::FileOpenSave(m_collabGuiPath, NULL, IDS_SETTINGS_SELECTCOLLAB, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
    {
        UpdateData(FALSE);
        SetModified();
    }
}

void SettingsCollaborator::OnChange()
{
    SetModified();
}

