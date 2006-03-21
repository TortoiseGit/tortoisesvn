// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "SetMainPage.h"
#include "Utils.h"
#include "DirFileEnum.h"
#include "SVNProgressDlg.h"
#include "..\version.h"
#include ".\setmainpage.h"
#include "SVN.h"
#include "MessageBox.h"


// CSetMainPage dialog

IMPLEMENT_DYNAMIC(CSetMainPage, CPropertyPage)
CSetMainPage::CSetMainPage()
	: CPropertyPage(CSetMainPage::IDD)
	, m_sTempExtensions(_T(""))
	, m_bCheckNewer(TRUE)
	, m_bLastCommitTime(FALSE)
{
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	m_regExtensions = CRegString(_T("Software\\Tigris.org\\Subversion\\Config\\miscellany\\global-ignores"));
	m_regCheckNewer = CRegDWORD(_T("Software\\TortoiseSVN\\CheckNewer"), TRUE);
	m_regLastCommitTime = CRegString(_T("Software\\Tigris.org\\Subversion\\Config\\miscellany\\use-commit-times"), _T(""));
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::SaveData()
{
	m_regLanguage = m_dwLanguage;
	if (m_regLanguage.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regLanguage.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regExtensions = m_sTempExtensions;
	if (m_regExtensions.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regExtensions.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regCheckNewer = m_bCheckNewer;
	if (m_regCheckNewer.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regCheckNewer.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regLastCommitTime = (m_bLastCommitTime ? _T("yes") : _T("no"));
	if (m_regLastCommitTime.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regLastCommitTime.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
	m_dwLanguage = (DWORD)m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel());
	DDX_Text(pDX, IDC_TEMPEXTENSIONS, m_sTempExtensions);
	DDX_Check(pDX, IDC_CHECKNEWERVERSION, m_bCheckNewer);
	DDX_Check(pDX, IDC_COMMITFILETIMES, m_bLastCommitTime);
}


BEGIN_MESSAGE_MAP(CSetMainPage, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnCbnSelchangeLanguagecombo)
	ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnEnChangeTempextensions)
	ON_BN_CLICKED(IDC_EDITCONFIG, OnBnClickedEditconfig)
	ON_BN_CLICKED(IDC_CHECKNEWERVERSION, OnBnClickedChecknewerversion)
	ON_BN_CLICKED(IDC_CHECKNEWERBUTTON, OnBnClickedChecknewerbutton)
	ON_BN_CLICKED(IDC_COMMITFILETIMES, OnBnClickedCommitfiletimes)
	ON_BN_CLICKED(IDC_SOUNDS, OnBnClickedSounds)
END_MESSAGE_MAP()


// CSetMainPage message handlers
BOOL CSetMainPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	EnableToolTips();

	m_sTempExtensions = m_regExtensions;
	m_dwLanguage = m_regLanguage;
	m_bCheckNewer = m_regCheckNewer;

	CString temp;
	temp = m_regLastCommitTime;
	m_bLastCommitTime = (temp.CompareNoCase(_T("yes"))==0);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_TEMPEXTENSIONS, IDS_SETTINGS_TEMPEXTENSIONS_TT);
	m_tooltips.AddTool(IDC_CHECKNEWERVERSION, IDS_SETTINGS_CHECKNEWER_TT);
	m_tooltips.AddTool(IDC_CLEARAUTH, IDS_SETTINGS_CLEARAUTH_TT);
	m_tooltips.AddTool(IDC_COMMITFILETIMES, IDS_SETTINGS_COMMITFILETIMES_TT);

	//set up the language selecting combobox
	TCHAR buf[MAX_PATH];
	GetLocaleInfo(1033, LOCALE_SNATIVELANGNAME, buf, sizeof(buf)/sizeof(TCHAR));
	m_LanguageCombo.AddString(buf);
	m_LanguageCombo.SetItemData(0, 1033);
	CString path = CUtils::GetAppParentDirectory();
	path = path + _T("Languages\\");
	CSimpleFileFind finder(path, _T("*.dll"));
	int langcount = 1;
	while (finder.FindNextFileNoDirectories())
	{
		CString file = finder.GetFilePath();
		CString filename = finder.GetFileName();
		if (filename.Left(12).CompareNoCase(_T("TortoiseProc"))==0)
		{
			CString sVer = _T(STRPRODUCTVER);
			sVer = sVer.Left(sVer.ReverseFind(','));
			CString sFileVer = CUtils::GetVersionFromFile(file);
			sFileVer = sFileVer.Left(sFileVer.ReverseFind(','));
			if (sFileVer.Compare(sVer)!=0)
				continue;
			DWORD loc = _tstoi(filename.Mid(12));
			TCHAR buf[MAX_PATH];
			GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, sizeof(buf)/sizeof(TCHAR));
			m_LanguageCombo.AddString(buf);
			m_LanguageCombo.SetItemData(langcount++, loc);
		} // if (filename.Left(12).CompareNoCase(_T("TortoiseProc"))==0) 
	} // while (finder.FindNextFileNoDirectories()) 
	
	for (int i=0; i<m_LanguageCombo.GetCount(); i++)
	{
		if (m_LanguageCombo.GetItemData(i) == m_dwLanguage)
			m_LanguageCombo.SetCurSel(i);
	}

	UpdateData(FALSE);
	return TRUE;
}

BOOL CSetMainPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CSetMainPage::OnCbnSelchangeLanguagecombo()
{
	SetModified();
}

void CSetMainPage::OnEnChangeTempextensions()
{
	SetModified();
}

void CSetMainPage::OnBnClickedChecknewerversion()
{
	SetModified();
}

void CSetMainPage::OnBnClickedCommitfiletimes()
{
	SetModified();
}

BOOL CSetMainPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CSetMainPage::OnBnClickedEditconfig()
{
	TCHAR buf[MAX_PATH];
	SVN::EnsureConfigFile();
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
	CString path = buf;
	path += _T("\\Subversion\\config");
	CUtils::StartTextViewer(path);
}

void CSetMainPage::OnBnClickedChecknewerbutton()
{
	TCHAR com[MAX_PATH+100];
	GetModuleFileName(NULL, com, MAX_PATH);
	_tcscat_s(com, MAX_PATH+100, _T(" /command:updatecheck /visible"));

	CUtils::LaunchApplication(com, 0, false);
}

void CSetMainPage::OnBnClickedSounds()
{
	CUtils::LaunchApplication(_T("RUNDLL32 Shell32,Control_RunDLL mmsys.cpl,,1"), NULL, false);
}







