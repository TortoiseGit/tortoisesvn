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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetProxyPage.h"
#include ".\setproxypage.h"


// CSetProxyPage dialog

IMPLEMENT_DYNAMIC(CSetProxyPage, CPropertyPage)
CSetProxyPage::CSetProxyPage()
	: CPropertyPage(CSetProxyPage::IDD)
	, m_serveraddress(_T(""))
	, m_serverport(0)
	, m_username(_T(""))
	, m_password(_T(""))
	, m_timeout(0)
	, m_isEnabled(FALSE)
	, m_SSHClient(_T(""))
{
	this->m_pPSP->dwFlags &= ~PSP_HASHELP;
	m_regServeraddress = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\DEFAULT\\http-proxy-host"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regServerport = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\DEFAULT\\http-proxy-port"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regUsername = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\DEFAULT\\http-proxy-username"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regPassword = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\DEFAULT\\http-proxy-password"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regTimeout = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\DEFAULT\\http-proxy-timeout"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regSSHClient = CRegString(_T("Software\\TortoiseSVN\\SSH"));

	m_regServeraddress_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\DEFAULT\\http-proxy-host"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regServerport_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\DEFAULT\\http-proxy-port"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regUsername_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\DEFAULT\\http-proxy-username"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regPassword_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\DEFAULT\\http-proxy-password"), _T(""), 0, HKEY_LOCAL_MACHINE);
	m_regTimeout_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\DEFAULT\\http-proxy-timeout"), _T(""), 0, HKEY_LOCAL_MACHINE);
}

CSetProxyPage::~CSetProxyPage()
{
}

void CSetProxyPage::SaveData()
{
	if (m_isEnabled)
	{
		CString temp;
		m_regServeraddress = m_serveraddress;
		m_regServeraddress_copy = m_serveraddress;
		temp.Format(_T("%d"), m_serverport);
		m_regServerport = temp;
		m_regServerport_copy = temp;
		m_regUsername = m_username;
		m_regUsername_copy = m_username;
		m_regPassword = m_password;
		m_regPassword_copy = m_password;
		temp.Format(_T("%d"), m_timeout);
		m_regTimeout = temp;
		m_regTimeout_copy = temp;
	} // if (m_isEnabled)
	else
	{
		m_regServeraddress.removeValue();
		m_regServerport.removeValue();
		m_regUsername.removeValue();
		m_regPassword.removeValue();
		m_regTimeout.removeValue();

		CString temp;
		m_regServeraddress_copy = m_serveraddress;
		temp.Format(_T("%d"), m_serverport);
		m_regServerport_copy = temp;
		m_regUsername_copy = m_username;
		m_regPassword_copy = m_password;
		temp.Format(_T("%d"), m_timeout);
		m_regTimeout_copy = temp;
	}
	m_regSSHClient = m_SSHClient;
}

void CSetProxyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SERVERADDRESS, m_serveraddress);
	DDX_Text(pDX, IDC_SERVERPORT, m_serverport);
	DDX_Text(pDX, IDC_USERNAME, m_username);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	DDX_Text(pDX, IDC_TIMEOUT, m_timeout);
	DDX_Check(pDX, IDC_ENABLE, m_isEnabled);
	DDX_Text(pDX, IDC_SSHCLIENT, m_SSHClient);
}


BEGIN_MESSAGE_MAP(CSetProxyPage, CPropertyPage)
	ON_BN_CLICKED(IDC_ENABLE, OnBnClickedEnable)
	ON_EN_CHANGE(IDC_SERVERADDRESS, OnEnChangeServeraddress)
	ON_EN_CHANGE(IDC_SERVERPORT, OnEnChangeServerport)
	ON_EN_CHANGE(IDC_USERNAME, OnEnChangeUsername)
	ON_EN_CHANGE(IDC_PASSWORD, OnEnChangePassword)
	ON_EN_CHANGE(IDC_TIMEOUT, OnEnChangeTimeout)
	ON_EN_CHANGE(IDC_SSHCLIENT, OnEnChangeSshclient)
	ON_BN_CLICKED(IDC_SSHBROWSE, OnBnClickedSshbrowse)
END_MESSAGE_MAP()



BOOL CSetProxyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_SERVERADDRESS, IDS_SETTINGS_PROXYSERVER_TT);
	//m_tooltips.SetEffectBk(CBalloon::BALLOON_EFFECT_HGRADIENT);
	//m_tooltips.SetGradientColors(0x80ffff, 0x000000, 0xffff80);

	m_SSHClient = m_regSSHClient;
	m_serveraddress = m_regServeraddress;
	m_serverport = _ttoi((LPCTSTR)(CString)m_regServerport);
	m_username = m_regUsername;
	m_password = m_regPassword;
	m_timeout = _ttoi((LPCTSTR)(CString)m_regTimeout);

	if (m_serveraddress.IsEmpty())
	{
		m_isEnabled = FALSE;
		EnableGroup(FALSE);
		//now since we already created our registry entries
		//we delete them here again...
		m_regServeraddress.removeValue();
		m_regServerport.removeValue();
		m_regUsername.removeValue();
		m_regPassword.removeValue();
		m_regTimeout.removeValue();
	}
	else
	{
		m_isEnabled = TRUE;
		EnableGroup(TRUE);
	}
	if (m_serveraddress.IsEmpty())
		m_serveraddress = m_regServeraddress_copy;
	if (m_serverport==0)
		m_serverport = _ttoi((LPCTSTR)(CString)m_regServerport_copy);
	if (m_username.IsEmpty())
		m_username = m_regUsername_copy;
	if (m_password.IsEmpty())
		m_password = m_regPassword_copy;
	if (m_timeout == 0)
		m_timeout = _ttoi((LPCTSTR)(CString)m_regTimeout_copy);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSetProxyPage::OnBnClickedEnable()
{
	UpdateData();
	if (m_isEnabled)
	{
		EnableGroup(TRUE);
	}
	else
	{
		EnableGroup(FALSE);
	}
}

void CSetProxyPage::EnableGroup(BOOL b)
{
	GetDlgItem(IDC_SERVERADDRESS)->EnableWindow(b);
	GetDlgItem(IDC_SERVERPORT)->EnableWindow(b);
	GetDlgItem(IDC_USERNAME)->EnableWindow(b);
	GetDlgItem(IDC_PASSWORD)->EnableWindow(b);
	GetDlgItem(IDC_TIMEOUT)->EnableWindow(b);
}



BOOL CSetProxyPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CSetProxyPage::OnEnChangeServeraddress()
{
	SetModified();
}

void CSetProxyPage::OnEnChangeServerport()
{
	SetModified();
}

void CSetProxyPage::OnEnChangeUsername()
{
	SetModified();
}

void CSetProxyPage::OnEnChangePassword()
{
	SetModified();
}

void CSetProxyPage::OnEnChangeTimeout()
{
	SetModified();
}

void CSetProxyPage::OnEnChangeSshclient()
{
	SetModified();
}

BOOL CSetProxyPage::OnApply()
{
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}


void CSetProxyPage::OnBnClickedSshbrowse()
{
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	//ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	ofn.lpstrFilter = _T("Programs\0*.exe\0All\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	temp.LoadString(IDS_SETTINGS_SELECTSSH);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_SSHClient = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	}
}
