// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#pragma once
#include "Balloon.h"

/**
 * \ingroup TortoiseProc
 * This is the Proxy page of the settings dialog. It gives the user the
 * possibility to set the proxy settings for subversion clients.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 01-28-2003
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class CSetProxyPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetProxyPage)

public:
	CSetProxyPage();
	virtual ~CSetProxyPage();

	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	enum { IDD = IDD_SETTINGSPROXY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void EnableGroup(BOOL b);

	DECLARE_MESSAGE_MAP()
private:
	CBalloon	m_tooltips;
public:
	CString		m_serveraddress;
	CRegString	m_regServeraddress;
	CRegString	m_regServeraddress_copy;
	UINT		m_serverport;
	CRegString	m_regServerport;
	CRegString	m_regServerport_copy;
	CString		m_username;
	CRegString	m_regUsername;
	CRegString	m_regUsername_copy;
	CString		m_password;
	CRegString	m_regPassword;
	CRegString	m_regPassword_copy;
	UINT		m_timeout;
	CRegString	m_regTimeout;
	CRegString	m_regTimeout_copy;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedEnable();
	BOOL		m_isEnabled;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnEnChangeServeraddress();
	afx_msg void OnEnChangeServerport();
	afx_msg void OnEnChangeUsername();
	afx_msg void OnEnChangePassword();
	afx_msg void OnEnChangeTimeout();
	virtual BOOL OnApply();
};
