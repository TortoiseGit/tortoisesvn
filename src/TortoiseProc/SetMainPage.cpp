// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "SetMainPage.h"
#include "Utils.h"
#include "DirFileEnum.h"
#include "..\version.h"
#include ".\setmainpage.h"


// CSetMainPage dialog

IMPLEMENT_DYNAMIC(CSetMainPage, CPropertyPage)
CSetMainPage::CSetMainPage()
	: CPropertyPage(CSetMainPage::IDD)
	, m_sTempExtensions(_T(""))
	, m_bNoRemoveLogMsg(FALSE)
	, m_bAutoClose(FALSE)
	, m_sDefaultLogs(_T(""))
	, m_bShortDateFormat(FALSE)
	, m_bLastCommitTime(FALSE)
	, m_bCheckNewer(TRUE)
	, m_nMinLogSize(10)
{
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	m_regExtensions = CRegString(_T("Software\\Tigris.org\\Subversion\\Config\\miscellany\\global-ignores"));
	m_regNoRemoveLogMsg = CRegDWORD(_T("Software\\TortoiseSVN\\NoDeleteLogMsg"));
	m_regAutoClose = CRegDWORD(_T("Software\\TortoiseSVN\\AutoClose"));
	m_regDefaultLogs = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
	m_regShortDateFormat = CRegDWORD(_T("Software\\TortoiseSVN\\LogDateFormat"), FALSE);
	m_regFontName = CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New"));
	m_regFontSize = CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8);
	m_regLastCommitTime = CRegString(_T("Software\\Tigris.org\\Subversion\\Config\\miscellany\\use-commit-times"), _T(""));
	m_regCheckNewer = CRegDWORD(_T("Software\\TortoiseSVN\\CheckNewer"), TRUE);
	m_regMinLogSize = CRegDWORD(_T("Software\\TortoiseSVN\\MinLogSize"), 10);
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::SaveData()
{
	m_regLanguage = m_dwLanguage;
	m_regExtensions = m_sTempExtensions;
	m_regNoRemoveLogMsg = m_bNoRemoveLogMsg;
	m_regAutoClose = m_bAutoClose;
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
	m_regMinLogSize = m_nMinLogSize;
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
	DDX_Check(pDX, IDC_NOREMOVELOGMSG, m_bNoRemoveLogMsg);
	DDX_Check(pDX, IDC_AUTOCLOSE, m_bAutoClose);
	DDX_Text(pDX, IDC_DEFAULTLOG, m_sDefaultLogs);
	DDX_Control(pDX, IDC_MISCGROUP, m_cMiscGroup);
	DDX_Control(pDX, IDC_COMMITGROUP, m_cCommitGroup);
	DDX_Check(pDX, IDC_SHORTDATEFORMAT, m_bShortDateFormat);
	DDX_Check(pDX, IDC_COMMITFILETIMES, m_bLastCommitTime);
	DDX_Check(pDX, IDC_CHECKNEWERVERSION, m_bCheckNewer);
	DDX_Text(pDX, IDC_MINLOGSIZE, m_nMinLogSize);
	DDV_MinMaxUInt(pDX, m_nMinLogSize, 0, 100);
}


BEGIN_MESSAGE_MAP(CSetMainPage, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnCbnSelchangeLanguagecombo)
	ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnEnChangeTempextensions)
	ON_BN_CLICKED(IDC_NOREMOVELOGMSG, OnBnClickedNoremovelogmsg)
	ON_BN_CLICKED(IDC_AUTOCLOSE, OnBnClickedAutoclose)
	ON_EN_CHANGE(IDC_DEFAULTLOG, OnEnChangeDefaultlog)
	ON_BN_CLICKED(IDC_SHORTDATEFORMAT, OnBnClickedShortdateformat)
	ON_BN_CLICKED(IDC_COMMITFILETIMES, OnBnClickedCommitfiletimes)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, OnCbnSelchangeFontsizes)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, OnCbnSelchangeFontnames)
	ON_BN_CLICKED(IDC_EDITCONFIG, OnBnClickedEditconfig)
	ON_BN_CLICKED(IDC_CHECKNEWERVERSION, OnBnClickedChecknewerversion)
	ON_EN_CHANGE(IDC_MINLOGSIZE, OnEnChangeMinlogsize)
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

	m_sTempExtensions = m_regExtensions;
	m_bNoRemoveLogMsg = m_regNoRemoveLogMsg;
	m_bAutoClose = m_regAutoClose;
	m_dwLanguage = m_regLanguage;
	m_bShortDateFormat = m_regShortDateFormat;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_bCheckNewer = m_regCheckNewer;
	m_nMinLogSize = m_regMinLogSize;

	CString temp;
	temp = m_regLastCommitTime;
	m_bLastCommitTime = (temp.CompareNoCase(_T("yes"))==0);

	temp.Format(_T("%ld"), (DWORD)m_regDefaultLogs);
	m_sDefaultLogs = temp;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_TEMPEXTENSIONS, IDS_SETTINGS_TEMPEXTENSIONS_TT);
	m_tooltips.AddTool(IDC_AUTOCLOSE, IDS_SETTINGS_AUTOCLOSE_TT);
	m_tooltips.AddTool(IDC_NOREMOVELOGMSG, IDS_SETTINGS_NOREMOVELOGMSG_TT);
	m_tooltips.AddTool(IDC_SHORTDATEFORMAT, IDS_SETTINGS_SHORTDATEFORMAT_TT);
	m_tooltips.AddTool(IDC_CHECKNEWERVERSION, IDS_SETTINGS_CHECKNEWER_TT);
	//m_tooltips.SetEffectBk(CBalloon::BALLOON_EFFECT_HGRADIENT);
	//m_tooltips.SetGradientColors(0x80ffff, 0x000000, 0xffff80);

	//set up the language selecting combobox
	m_LanguageCombo.AddString(_T("English"));
	m_LanguageCombo.SetItemData(0, 1033);
	TCHAR procpathbuf[MAX_PATH];
	GetModuleFileName(NULL, procpathbuf, MAX_PATH);
	CString path = procpathbuf;
	path = path.Left(path.ReverseFind('\\')+1);
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
	} // for (int i=6; i<20; i=i+2) 
	for (int i=0; i<m_cFontSizes.GetCount(); i++)
	{
		if (m_cFontSizes.GetItemData(i) == m_dwFontSize)
			m_cFontSizes.SetCurSel(i);
	} // for (int i=0; i<m_LanguageCombo.GetCount(); i++) 


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

void CSetMainPage::OnBnClickedNoremovelogmsg()
{
	SetModified();
}

void CSetMainPage::OnBnClickedAutoclose()
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

void CSetMainPage::OnEnChangeMinlogsize()
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
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
	CString path = buf;
	path += _T("\\Subversion\\config");
	CUtils::StartTextViewer(path);
}









