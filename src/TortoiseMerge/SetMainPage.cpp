// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2005 - Stefan Kueng

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
#include "TortoiseMerge.h"
#include "DirFileEnum.h"
#include "version.h"
#include "Utils.h"
#include "SetMainPage.h"
#include ".\setmainpage.h"


// CSetMainPage dialog

IMPLEMENT_DYNAMIC(CSetMainPage, CPropertyPage)
CSetMainPage::CSetMainPage()
	: CPropertyPage(CSetMainPage::IDD)
	, m_bBackup(FALSE)
	, m_bFirstDiffOnLoad(FALSE)
	, m_nTabSize(0)
	, m_bIgnoreEOL(FALSE)
	, m_bOnePane(FALSE)
	, m_bViewLinenumbers(FALSE)
	, m_bMagnifier(FALSE)
	, m_bDiffBar(FALSE)
	, m_bStrikeout(FALSE)
	, m_bReloadNeeded(FALSE)
	, m_bDisplayBinDiff(TRUE)
	, m_bCaseInsensitive(FALSE)
{
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseMerge\\LanguageID"), 1033);
	m_regBackup = CRegDWORD(_T("Software\\TortoiseMerge\\Backup"));
	m_regFirstDiffOnLoad = CRegDWORD(_T("Software\\TortoiseMerge\\FirstDiffOnLoad"));
	m_regTabSize = CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	m_regIgnoreEOL = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreEOL"), TRUE);	
	m_regOnePane = CRegDWORD(_T("Software\\TortoiseMerge\\OnePane"));
	m_regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
	m_regViewLinenumbers = CRegDWORD(_T("Software\\TortoiseMerge\\ViewLinenumbers"), 1);
	m_regMagnifier = CRegDWORD(_T("Software\\TortoiseMerge\\Magnifier"), TRUE);
	m_regDiffBar = CRegDWORD(_T("Software\\TortoiseMerge\\DiffBar"), TRUE);
	m_regStrikeout = CRegDWORD(_T("Software\\TortoiseMerge\\StrikeOut"), TRUE);
	m_regFontName = CRegString(_T("Software\\TortoiseMerge\\LogFontName"), _T("Courier New"));
	m_regFontSize = CRegDWORD(_T("Software\\TortoiseMerge\\LogFontSize"), 10);
	m_regDisplayBinDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
	m_regCaseInsensitive = CRegDWORD(_T("Software\\TortoiseMerge\\CaseInsensitive"), FALSE);

	m_dwLanguage = m_regLanguage;
	m_bBackup = m_regBackup;
	m_bFirstDiffOnLoad = m_regFirstDiffOnLoad;
	m_nTabSize = m_regTabSize;
	m_bIgnoreEOL = m_regIgnoreEOL;
	m_bOnePane = m_regOnePane;
	m_nIgnoreWS = m_regIgnoreWS;
	m_bViewLinenumbers = m_regViewLinenumbers;
	m_bMagnifier = m_regMagnifier;
	m_bDiffBar = m_regDiffBar;
	m_bStrikeout = m_regStrikeout;
	m_bDisplayBinDiff = m_regDisplayBinDiff;
	m_bCaseInsensitive = m_regCaseInsensitive;
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_BACKUP, m_bBackup);
	DDX_Check(pDX, IDC_FIRSTDIFFONLOAD, m_bFirstDiffOnLoad);
	DDX_Text(pDX, IDC_TABSIZE, m_nTabSize);
	DDV_MinMaxInt(pDX, m_nTabSize, 1, 1000);
	DDX_Check(pDX, IDC_IGNORELF, m_bIgnoreEOL);
	DDX_Check(pDX, IDC_ONEPANE, m_bOnePane);
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
	DDX_Control(pDX, IDC_FONTSIZES, m_cFontSizes);
	DDX_FontPreviewCombo (pDX, IDC_FONTNAMES, m_sFontName);
	m_dwLanguage = (DWORD)m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel());
	m_dwFontSize = (DWORD)m_cFontSizes.GetItemData(m_cFontSizes.GetCurSel());
	if ((m_dwFontSize==0)||(m_dwFontSize == -1))
	{
		CString t;
		m_cFontSizes.GetWindowText(t);
		m_dwFontSize = _ttoi(t);
	}
	DDX_Check(pDX, IDC_LINENUMBERS, m_bViewLinenumbers);
	DDX_Check(pDX, IDC_MAGNIFIER, m_bMagnifier);
	DDX_Check(pDX, IDC_DIFFBAR, m_bDiffBar);
	DDX_Check(pDX, IDC_STRIKEOUT, m_bStrikeout);
	DDX_Check(pDX, IDC_USEBDIFF, m_bDisplayBinDiff);
	DDX_Check(pDX, IDC_CASEINSENSITIVE, m_bCaseInsensitive);
}

void CSetMainPage::SaveData()
{
	m_regLanguage = m_dwLanguage;
	m_regBackup = m_bBackup;
	m_regFirstDiffOnLoad = m_bFirstDiffOnLoad;
	m_regTabSize = m_nTabSize;
	m_regIgnoreEOL = m_bIgnoreEOL;
	m_regOnePane = m_bOnePane;
	m_regIgnoreWS = m_nIgnoreWS;
	m_regFontName = m_sFontName;
	m_regFontSize = m_dwFontSize;
	m_regViewLinenumbers = m_bViewLinenumbers;
	m_regMagnifier = m_bMagnifier;
	m_regDiffBar = m_bDiffBar;
	m_regStrikeout = m_bStrikeout;
	m_regDisplayBinDiff = m_bDisplayBinDiff;
	m_regCaseInsensitive = m_bCaseInsensitive;
}

BOOL CSetMainPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

BOOL CSetMainPage::OnInitDialog()
{
	m_cFontNames.SubclassDlgItem (IDC_FONTNAMES, this);
	m_cFontNames.SetFontHeight(16, false);
	m_cFontNames.SetPreviewStyle(CFontPreviewCombo::NAME_THEN_SAMPLE, false);
	m_cFontNames.Init();

	CPropertyPage::OnInitDialog();

	m_dwLanguage = m_regLanguage;
	m_bBackup = m_regBackup;
	m_bFirstDiffOnLoad = m_regFirstDiffOnLoad;
	m_nTabSize = m_regTabSize;
	m_bIgnoreEOL = m_regIgnoreEOL;
	m_bOnePane = m_regOnePane;
	m_nIgnoreWS = m_regIgnoreWS;
	m_bDisplayBinDiff = m_regDisplayBinDiff;
	m_sFontName = m_regFontName;
	m_dwFontSize = m_regFontSize;
	m_bViewLinenumbers = m_regViewLinenumbers;
	m_bMagnifier = m_regMagnifier;
	m_bDiffBar = m_regDiffBar;
	m_bStrikeout = m_regStrikeout;
	m_bCaseInsensitive = m_bCaseInsensitive;

	UINT uRadio = IDC_WSIGNORELEADING;
	switch (m_nIgnoreWS)
	{
	case 0:
		uRadio = IDC_WSCOMPARE;
		break;
	case 1:
		uRadio = IDC_WSIGNOREALL;
		break;
	case 2:
		uRadio = IDC_WSIGNORELEADING;
		break;	
	default:
		break;	
	}

	CheckRadioButton(IDC_WSCOMPARE, IDC_WSIGNOREALL, uRadio);

	//set up the language selecting combobox
	m_LanguageCombo.AddString(_T("English"));
	m_LanguageCombo.SetItemData(0, 1033);
	TCHAR procpathbuf[MAX_PATH];
	GetModuleFileName(NULL, procpathbuf, MAX_PATH);
	CString path = procpathbuf;
	path = path.Left(path.ReverseFind('\\'));
	path = path.Left(path.ReverseFind('\\')+1);
	path = path + _T("Languages\\");
	CSimpleFileFind finder(path, _T("*.dll"));
	int langcount = 1;
	while (finder.FindNextFileNoDirectories())
	{
		CString file = finder.GetFilePath();
		CString filename = finder.GetFileName();
		if (filename.Left(13).CompareNoCase(_T("TortoiseMerge"))==0)
		{
			CString sVer = _T(STRPRODUCTVER);
			sVer = sVer.Left(sVer.ReverseFind(','));
			CString sFileVer = CUtils::GetVersionFromFile(file);
			sFileVer = sFileVer.Left(sFileVer.ReverseFind(','));
			if (sFileVer.Compare(sVer)!=0)
				continue;
			DWORD loc = _tstoi(filename.Mid(13));
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

	CString temp;
	int count = 0;
	for (int i=6; i<32; i=i+2)
	{
		temp.Format(_T("%d"), i);
		m_cFontSizes.AddString(temp);
		m_cFontSizes.SetItemData(count++, i);
	} // for (int i=6; i<20; i=i+2) 
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
	m_cFontNames.AdjustHeight(&m_cFontSizes);

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CSetMainPage, CPropertyPage)
	ON_BN_CLICKED(IDC_BACKUP, OnBnClickedBackup)
	ON_BN_CLICKED(IDC_IGNORELF, OnBnClickedIgnorelf)
	ON_BN_CLICKED(IDC_ONEPANE, OnBnClickedOnepane)
	ON_BN_CLICKED(IDC_FIRSTDIFFONLOAD, OnBnClickedFirstdiffonload)
	ON_BN_CLICKED(IDC_WSCOMPARE, OnBnClickedWscompare)
	ON_BN_CLICKED(IDC_WSIGNORELEADING, OnBnClickedWsignoreleading)
	ON_BN_CLICKED(IDC_WSIGNOREALL, OnBnClickedWsignoreall)
	ON_BN_CLICKED(IDC_LINENUMBERS, OnBnClickedLinenumbers)
	ON_BN_CLICKED(IDC_MAGNIFIER, OnBnClickedMagnifier)
	ON_BN_CLICKED(IDC_DIFFBAR, OnBnClickedDiffbar)
	ON_BN_CLICKED(IDC_STRIKEOUT, OnBnClickedStrikeout)
	ON_EN_CHANGE(IDC_TABSIZE, OnEnChangeTabsize)
	ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnCbnSelchangeLanguagecombo)
	ON_CBN_SELCHANGE(IDC_FONTSIZES, OnCbnSelchangeFontsizes)
	ON_CBN_SELCHANGE(IDC_FONTNAMES, OnCbnSelchangeFontnames)
	ON_BN_CLICKED(IDC_USEBDIFF, OnBnClickedUsebdiff)
	ON_BN_CLICKED(IDC_CASEINSENSITIVE, OnBnClickedCaseinsensitive)
END_MESSAGE_MAP()


// CSetMainPage message handlers

void CSetMainPage::OnCbnSelchangeLanguagecombo()
{
	SetModified();
}

void CSetMainPage::OnBnClickedBackup()
{
	SetModified();
}

void CSetMainPage::OnBnClickedIgnorelf()
{
	m_bReloadNeeded = TRUE;
	SetModified();
}

void CSetMainPage::OnBnClickedOnepane()
{
	SetModified();
}

void CSetMainPage::OnBnClickedFirstdiffonload()
{
	SetModified();
}

void CSetMainPage::OnBnClickedLinenumbers()
{
	SetModified();
}

void CSetMainPage::OnBnClickedMagnifier()
{
	SetModified();
}

void CSetMainPage::OnBnClickedDiffbar()
{
	SetModified();
}

void CSetMainPage::OnBnClickedStrikeout()
{
	SetModified();
}

void CSetMainPage::OnBnClickedWscompare()
{
	m_bReloadNeeded = TRUE;
	SetModified();
	UINT uRadio = GetCheckedRadioButton(IDC_WSCOMPARE, IDC_WSIGNOREALL);
	switch (uRadio)
	{
	case IDC_WSCOMPARE:
		m_nIgnoreWS = 0;
		break;
	case IDC_WSIGNOREALL:
		m_nIgnoreWS = 1;
		break;
	case IDC_WSIGNORELEADING:
		m_nIgnoreWS = 2;
		break;	
	default:
		break;	
	}
}

void CSetMainPage::OnBnClickedWsignoreleading()
{
	m_bReloadNeeded = TRUE;
	SetModified();
	UINT uRadio = GetCheckedRadioButton(IDC_WSCOMPARE, IDC_WSIGNOREALL);
	switch (uRadio)
	{
	case IDC_WSCOMPARE:
		m_nIgnoreWS = 0;
		break;
	case IDC_WSIGNOREALL:
		m_nIgnoreWS = 1;
		break;
	case IDC_WSIGNORELEADING:
		m_nIgnoreWS = 2;
		break;	
	default:
		break;	
	}
}

void CSetMainPage::OnBnClickedWsignoreall()
{
	m_bReloadNeeded = TRUE;
	SetModified();
	UINT uRadio = GetCheckedRadioButton(IDC_WSCOMPARE, IDC_WSIGNOREALL);
	switch (uRadio)
	{
	case IDC_WSCOMPARE:
		m_nIgnoreWS = 0;
		break;
	case IDC_WSIGNOREALL:
		m_nIgnoreWS = 1;
		break;
	case IDC_WSIGNORELEADING:
		m_nIgnoreWS = 2;
		break;	
	default:
		break;	
	}
}

void CSetMainPage::OnEnChangeTabsize()
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

void CSetMainPage::OnBnClickedUsebdiff()
{
	SetModified();
}

void CSetMainPage::OnBnClickedCaseinsensitive()
{
	SetModified();
}
