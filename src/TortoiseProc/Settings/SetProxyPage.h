// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2012, 2015, 2021 - TortoiseSVN

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

#include "SettingsPropPage.h"
#include "registry.h"
#include "FileDropEdit.h"

/**
 * \ingroup TortoiseProc
 * This is the Proxy page of the settings dialog. It gives the user the
 * possibility to set the proxy settings for subversion clients.
 */
class CSetProxyPage : public ISettingsPropPage
{
    DECLARE_DYNAMIC(CSetProxyPage)

public:
    CSetProxyPage();
    ~CSetProxyPage() override;

    UINT GetIconID() override { return IDI_PROXY; }

    enum
    {
        IDD = IDD_SETTINGSPROXY
    };

protected:
    void         DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL         OnApply() override;
    BOOL         OnInitDialog() override;
    afx_msg void OnChange();
    afx_msg void OnBnClickedEnable();
    afx_msg void OnBnClickedSshbrowse();
    afx_msg void OnBnClickedEditservers();

    void EnableGroup(BOOL b);

    DECLARE_MESSAGE_MAP()
private:
    CString       m_serverAddress;
    CRegString    m_regServerAddress;
    CRegString    m_regServerAddressCopy;
    UINT          m_serverport;
    CRegString    m_regServerport;
    CRegString    m_regServerportCopy;
    CString       m_username;
    CRegString    m_regUsername;
    CRegString    m_regUsernameCopy;
    CString       m_password;
    CRegString    m_regPassword;
    CRegString    m_regPasswordCopy;
    UINT          m_timeout;
    CRegString    m_regTimeout;
    CRegString    m_regTimeoutCopy;
    BOOL          m_isEnabled;
    CRegString    m_regSSHClient;
    CString       m_sshClient;
    CRegString    m_regExceptions;
    CRegString    m_regExceptionsCopy;
    CString       m_exceptions;
    CFileDropEdit m_cSSHClientEdit;
};
