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
#include "SetMainPage.h"
#include "Utils.h"
#include "DirFileEnum.h"
#include "SVNProgressDlg.h"
#include "..\version.h"
#include ".\setmainpage.h"
#include "SVN.h"


// CSetMainPage dialog

IMPLEMENT_DYNAMIC(CSetMainPage, CPropertyPage)
CSetMainPage::CSetMainPage()
	: CPropertyPage(CSetMainPage::IDD)
	, m_sTempExtensions(_T(""))
	//, m_bAutoClose(FALSE)
	, m_sDefaultLogs(_T(""))
	, m_bShortDateFormat(FALSE)
	, m_bLastCommitTime(FALSE)
	, m_bCheckNewer(TRUE)
{
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	m_regExtensions = CRegString(_T("Software\\Tigris.org\\Subversion\\Config\\miscellany\\global-ignores"));
	m_regAutoClose = CRegDWORD(_T("Software\\TortoiseSVN\\AutoClose"));
	m_regDefaultLogs = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
	m_regShortDateFormat = CRegDWORD(_T("Software\\TortoiseSVN\\LogDateFormat"), FALSE);
	m_regFontName = CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New"));
	m_regFontSize = CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8);
	m_regLastCommitTime = CRegString(_T("Software\\Tigris.org\\Subversion\\Config\\miscellany\\use-commit-times"), _T(""));
	m_regCheckNewer = CRegDWORD(_T("Software\\TortoiseSVN\\CheckNewer"), TRUE);
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::SaveData()
{
	m_regLanguage = m_dwLanguage;
	m_regExtensions = m_sTempExtensions;
	m_regAutoClose = m_dwAutoClose;
	m_regShortDateFormat = m_bShortDateFormat;
	m_regCheckNewer = m_bCheckNewer;
	long val = _ttol(m_sDefaultLogs);
	if (val > 5)
		m_regDefaultLogs = val;
	else
		m_regDefaultLogs = 100;

	m_regFontName = m_sFontName;
	m_regFontSize = m_dwFontSize;
	m_regLastCommitTime = (m_bLastCommitTime ? _T("yes") : _T("no"));
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	m_dwLanguage = (DWORD)m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel());
	m_dwFontSize = (DWORD)m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel());
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _ttoi(t);
	}
	DDX_FontPreviewCombo (pDX, IDC_FONTNAMES, m_sFontName);
	DDX_Text(pDX, IDC_TEMPEXTENSIONS, m_sTempExtensions);
	DDX_Text(pDX, IDC_DEFAULTLOG, m_sDefaultLogs);
	DDX_Control(pDX, IDC_MISCGROUP, m_cMiscGroup);
	DDX_Control(pDX, IDC_COMMITGROUP, m_cCommitGroup);
	DDX_Check(pDX, IDC_SHORTDATEFORMAT, m_bShortDateFormat);
	DDX_Check(pDX, IDC_COMMITFILETIMES, m_bLastCommitTime);
	DDX_Check(pDX, IDC_CHECKNEWERVERSION, m_bCheckNewer);
	DDX_Control(pDX, IDC_AUTOCLOSECOMBO, m_cAutoClose);
}


BEGIN_MESSAGE_MAP(CSetMainPage, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnCbnSelchangeLanguagecombo)
	ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnEnChangeTempextensions)
	ON_EN_CHANGE(IDC_DEFAULTLOG, OnEnChangeDefaultlog)
	ON_BN_CLICKED(IDC_SHORTDATEFORMAT, OnBnClickedShortdateformat)
	ON_BN_CLICKED(IDC_COMMITFILETIMES, OnBnClickedCommitfiletimes)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, OnCbnSelchangeFontsizes)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, OnCbnSelchangeFontnames)
	ON_BN_CLICKED(IDC_EDITCONFIG, OnBnClickedEditconfig)
	ON_BN_CLICKED(IDC_CHECKNEWERVERSION, OnBnClickedChecknewerversion)
	ON_BN_CLICKED(IDC_CLEARAUTH, OnBnClickedClearauth)
	ON_CBN_SELCHANGE(IDC_AUTOCLOSECOMBO, OnCbnSelchangeAutoclosecombo)
END_MESSAGE_MAP()


// CSetMainPage message handlers
BOOL CSetMainPage::OnInitDialog()
{
	m_cFontNames.SubclassDlgItem (IDC_FONTNAMES, this);
	m_cFontNames.SetFontHeight(16, false);
	m_cFontNames.SetPreviewStyle(CFontPreviewCombo::NAME_THEN_SAMPLE, false);
	m_cFontNames.Init();

	CPropertyPage::OnInitDialog();

	m_cMiscGroup.SetIcon(IDI_MISC);
	m_cCommitGroup.SetIcon(IDI_COMMIT);

	EnableToolTips();

	int ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_MANUAL)));
	m_cAutoClose.SetItemData(ind, CLOSE_MANUAL);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOMERGES)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOMERGES);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOCONFLICTS)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOCONFLICTS);
	ind = m_cAutoClose.AddString(CString(MAKEINTRESOURCE(IDS_PROGRS_CLOSE_NOERROR)));
	m_cAutoClose.SetItemData(ind, CLOSE_NOERRORS);

	m_sTempExtensions = m_regExtensions;
	m_dwAutoClose = m_regAutoClose;
	m_dwLanguage = m_regLanguage;
	m_bShortDateFormat = m_regShortDateFormat;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_bCheckNewer = m_regCheckNewer;

	for (int i=0; i<m_cAutoClose.GetCount(); ++i)
		if (m_cAutoClose.GetItemData(i)==m_dwAutoClose)
			m_cAutoClose.SetCurSel(i);

	CString temp;
	temp = m_regLastCommitTime;
	m_bLastCommitTime = (temp.CompareNoCase(_T("yes"))==0);

	temp.Format(_T("%ld"), (DWORD)m_regDefaultLogs);
	m_sDefaultLogs = temp;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_TEMPEXTENSIONS, IDS_SETTINGS_TEMPEXTENSIONS_TT);
	m_tooltips.AddTool(IDC_SHORTDATEFORMAT, IDS_SETTINGS_SHORTDATEFORMAT_TT);
	m_tooltips.AddTool(IDC_CHECKNEWERVERSION, IDS_SETTINGS_CHECKNEWER_TT);
	m_tooltips.AddTool(IDC_CLEARAUTH, IDS_SETTINGS_CLEARAUTH_TT);
	m_tooltips.AddTool(IDC_COMMITFILETIMES, IDS_SETTINGS_COMMITFILETIMES_TT);
	m_tooltips.AddTool(IDC_AUTOCLOSECOMBO, IDS_SETTINGS_AUTOCLOSE_TT);

	//set up the language selecting combobox
	m_LanguageCombo.AddString(_T("English"));
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
	} // for (int i=0; i<m_LanguageCombo.GetCount(); i++) 

	int count = 0;
	for (int i=6; i<32; i=i+2)
	{
		temp.Format(_T("%d"), i);
		m_cFontSizes.AddString(temp);
		m_cFontSizes.SetItemData(count++, i);
	}
	BOOL foundfont = FALSE;
	for (int i=0; i<m_cFontSizes.GetCount(); i++)
	{
		if (m_cFontSizes.GetItemData(i) == m_dwFontSize)
		{
			m_cFontSizes.SetCurSel(i);
			foundfont = TRUE;
		}
	}
	if (!foundfont)
	{
		temp.Format(_T("%d"), m_dwFontSize);
		m_cFontSizes.SetWindowText(temp);
	}

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
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

void CSetMainPage::OnBnClickedShortdateformat()
{
	SetModified();
}

void CSetMainPage::OnEnChangeDefaultlog()
{
	SetModified();
}

void CSetMainPage::OnCbnSelchangeFontsizes()
{
	SetModified();
}

void CSetMainPage::OnCbnSelchangeFontnames()
{
	SetModified();
}

void CSetMainPage::OnBnClickedCommitfiletimes()
{
	SetModified();
}

void CSetMainPage::OnBnClickedChecknewerversion()
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

void CSetMainPage::OnBnClickedClearauth()
{
	CRegStdString auth = CRegStdString(_T("Software\\TortoiseSVN\\Auth\\"));
	auth.removeKey();
	TCHAR pathbuf[MAX_PATH] = {0};
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathbuf)==S_OK)
	{
		_tcscat(pathbuf, _T("\\Subversion\\auth"));
		SHFILEOPSTRUCT fileop;
		fileop.hwnd = this->m_hWnd;
		fileop.wFunc = FO_DELETE;
		fileop.pFrom = pathbuf;
		fileop.pTo = NULL;
		fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
		fileop.lpszProgressTitle = _T("deleting file");
		SHFileOperation(&fileop);
	}
}

void CSetMainPage::OnCbnSelchangeAutoclosecombo()
{
	if (m_cAutoClose.GetCurSel() != CB_ERR)
	{
		m_dwAutoClose = m_cAutoClose.GetItemData(m_cAutoClose.GetCurSel());
	}
	SetModified();
}









