// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "SetProxyPage.h"
#include "Utils.h"
#include "SVN.h"
#include ".\setproxypage.h"

// CSetProxyPage dialog

IMPLEMENT_DYNAMIC(CSetProxyPage, CPropertyPage)
CSetProxyPage::CSetProxyPage()
	: CPropertyPage(CSetProxyPage::IDD)
	, m_bInit(FALSE)
	, m_serveraddress(_T(""))
	, m_serverport(0)
	, m_username(_T(""))
	, m_password(_T(""))
	, m_timeout(0)
	, m_isEnabled(FALSE)
	, m_SSHClient(_T(""))
	, m_Exceptions(_T(""))
{
	m_regServeraddress = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-host"), _T(""));
	m_regServerport = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-port"), _T(""));
	m_regUsername = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-username"), _T(""));
	m_regPassword = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-password"), _T(""));
	m_regTimeout = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-timeout"), _T(""));
	m_regSSHClient = CRegString(_T("Software\\TortoiseSVN\\SSH"));
	m_SSHClient = m_regSSHClient;
	m_regExceptions = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-exceptions"), _T(""));

	m_regServeraddress_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\global\\http-proxy-host"), _T(""));
	m_regServerport_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\global\\http-proxy-port"), _T(""));
	m_regUsername_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\global\\http-proxy-username"), _T(""));
	m_regPassword_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\global\\http-proxy-password"), _T(""));
	m_regTimeout_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\global\\http-proxy-timeout"), _T(""));
	m_regExceptions_copy = CRegString(_T("Software\\TortoiseSVN\\Servers\\global\\http-proxy-exceptions"), _T(""));
}

CSetProxyPage::~CSetProxyPage()
{
}

void CSetProxyPage::SaveData()
{
	if (m_bInit)
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
			m_regExceptions = m_Exceptions;
			m_regExceptions_copy = m_Exceptions;
		} // if (m_isEnabled)
		else
		{
			m_regServeraddress.removeValue();
			m_regServerport.removeValue();
			m_regUsername.removeValue();
			m_regPassword.removeValue();
			m_regTimeout.removeValue();
			m_regExceptions.removeValue();

			CString temp;
			m_regServeraddress_copy = m_serveraddress;
			temp.Format(_T("%d"), m_serverport);
			m_regServerport_copy = temp;
			m_regUsername_copy = m_username;
			m_regPassword_copy = m_password;
			temp.Format(_T("%d"), m_timeout);
			m_regTimeout_copy = temp;
			m_regExceptions_copy = m_Exceptions;
		}
		m_regSSHClient = m_SSHClient;
	}
}

void CSetProxyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SERVERADDRESS, m_serveraddress);
	DDX_Text(pDX, IDC_SERVERPORT, m_serverport);
	DDX_Text(pDX, IDC_USERNAME, m_username);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	DDX_Text(pDX, IDC_TIMEOUT, m_timeout);
	DDX_Text(pDX, IDC_EXCEPTIONS, m_Exceptions);
	DDX_Check(pDX, IDC_ENABLE, m_isEnabled);
	DDX_Text(pDX, IDC_SSHCLIENT, m_SSHClient);
	DDX_Control(pDX, IDC_SSHCLIENT, m_cSSHClientEdit);
}


BEGIN_MESSAGE_MAP(CSetProxyPage, CPropertyPage)
	ON_BN_CLICKED(IDC_ENABLE, OnBnClickedEnable)
	ON_EN_CHANGE(IDC_SERVERADDRESS, OnEnChangeServeraddress)
	ON_EN_CHANGE(IDC_SERVERPORT, OnEnChangeServerport)
	ON_EN_CHANGE(IDC_USERNAME, OnEnChangeUsername)
	ON_EN_CHANGE(IDC_PASSWORD, OnEnChangePassword)
	ON_EN_CHANGE(IDC_TIMEOUT, OnEnChangeTimeout)
	ON_EN_CHANGE(IDC_SSHCLIENT, OnEnChangeSshclient)
	ON_EN_CHANGE(IDC_EXCEPTIONS, OnEnChangeExceptions)
	ON_BN_CLICKED(IDC_SSHBROWSE, OnBnClickedSshbrowse)
	ON_BN_CLICKED(IDC_EDITSERVERS, OnBnClickedEditservers)
END_MESSAGE_MAP()



BOOL CSetProxyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_SERVERADDRESS, IDS_SETTINGS_PROXYSERVER_TT);
	m_tooltips.AddTool(IDC_EXCEPTIONS, IDS_SETTINGS_PROXYEXCEPTIONS_TT);
	//m_tooltips.SetEffectBk(CBalloon::BALLOON_EFFECT_HGRADIENT);
	//m_tooltips.SetGradientColors(0x80ffff, 0x000000, 0xffff80);

	m_SSHClient = m_regSSHClient;
	m_serveraddress = m_regServeraddress;
	m_serverport = _ttoi((LPCTSTR)(CString)m_regServerport);
	m_username = m_regUsername;
	m_password = m_regPassword;
	m_Exceptions = m_regExceptions;
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
		m_regExceptions.removeValue();
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
	if (m_Exceptions.IsEmpty())
		m_Exceptions = m_regExceptions_copy;
	if (m_timeout == 0)
		m_timeout = _ttoi((LPCTSTR)(CString)m_regTimeout_copy);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_SSHCLIENT), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	UpdateData(FALSE);

	m_bInit = TRUE;
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
	SetModified();
}

void CSetProxyPage::EnableGroup(BOOL b)
{
	GetDlgItem(IDC_SERVERADDRESS)->EnableWindow(b);
	GetDlgItem(IDC_SERVERPORT)->EnableWindow(b);
	GetDlgItem(IDC_USERNAME)->EnableWindow(b);
	GetDlgItem(IDC_PASSWORD)->EnableWindow(b);
	GetDlgItem(IDC_TIMEOUT)->EnableWindow(b);
	GetDlgItem(IDC_EXCEPTIONS)->EnableWindow(b);
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

void CSetProxyPage::OnEnChangeExceptions()
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
	ofn.lStructSize = sizeof(OPENFILENAME);
	//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	CString sFilter;
	sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
	// Replace '|' delimeters with '\0's
	TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
	while (ptr != pszFilters)
	{
		if (*ptr == '|')
			*ptr = '\0';
		ptr--;
	} // while (ptr != pszFilters) 
	ofn.lpstrFilter = pszFilters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	temp.LoadString(IDS_SETTINGS_SELECTSSH);
	CUtils::RemoveAccelerators(temp);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		UpdateData();
		m_SSHClient = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	}
	delete [] pszFilters;
}

void CSetProxyPage::OnBnClickedEditservers()
{
	TCHAR buf[MAX_PATH];
	SVN::EnsureConfigFile();
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
	CString path = buf;
	path += _T("\\Subversion\\servers");
	CUtils::StartTextViewer(path);
}
