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
#include "SetMainPage.h"
#include ".\setmainpage.h"


// CSetMainPage dialog

IMPLEMENT_DYNAMIC(CSetMainPage, CPropertyPage)
CSetMainPage::CSetMainPage()
	: CPropertyPage(CSetMainPage::IDD)
	, m_sDiffPath(_T(""))
	, m_sDiffViewerPath(_T(""))
	, m_sTempExtensions(_T(""))
	, m_sMergePath(_T(""))
	, m_bNoRemoveLogMsg(FALSE)
{
	this->m_pPSP->dwFlags &= ~PSP_HASHELP;
	m_regDiffPath = CRegString(_T("Software\\TortoiseSVN\\Diff"));
	m_regDiffViewerPath = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
	m_regMergePath = CRegString(_T("Software\\TortoiseSVN\\Merge"));
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
	m_regExtensions = CRegString(_T("Software\\TortoiseSVN\\TempFileExtensions"));
	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"));
	m_regNoRemoveLogMsg = CRegDWORD(_T("Software\\TortoiseSVN\\NoDeleteLogMsg"));
}

CSetMainPage::~CSetMainPage()
{
}

void CSetMainPage::SaveData()
{
	m_regDiffPath = m_sDiffPath;
	m_regDiffViewerPath = m_sDiffViewerPath;
	m_regMergePath = m_sMergePath;
	m_regLanguage = m_dwLanguage;
	m_regExtensions = m_sTempExtensions;
	m_regAddBeforeCommit = m_bAddBeforeCommit;
	m_regNoRemoveLogMsg = m_bNoRemoveLogMsg;
}

void CSetMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTDIFF, m_sDiffPath);
	DDX_Text(pDX, IDC_DIFFVIEWER, m_sDiffViewerPath);
	DDX_Control(pDX, IDC_LANGUAGECOMBO, m_LanguageCombo);
	m_dwLanguage = (DWORD)m_LanguageCombo.GetItemData(m_LanguageCombo.GetCurSel());
	DDX_Text(pDX, IDC_TEMPEXTENSIONS, m_sTempExtensions);
	DDX_Check(pDX, IDC_ADDBEFORECOMMIT, m_bAddBeforeCommit);
	DDX_Text(pDX, IDC_EXTMERGE, m_sMergePath);
	DDX_Check(pDX, IDC_NOREMOVELOGMSG, m_bNoRemoveLogMsg);
}


BEGIN_MESSAGE_MAP(CSetMainPage, CPropertyPage)
	ON_BN_CLICKED(IDC_EXTDIFFBROWSE, OnBnClickedExtdiffbrowse)
	ON_EN_CHANGE(IDC_EXTDIFF, OnEnChangeExtdiff)
	ON_CBN_SELCHANGE(IDC_LANGUAGECOMBO, OnCbnSelchangeLanguagecombo)
	ON_EN_CHANGE(IDC_TEMPEXTENSIONS, OnEnChangeTempextensions)
	ON_BN_CLICKED(IDC_ADDBEFORECOMMIT, OnBnClickedAddbeforecommit)
	ON_BN_CLICKED(IDC_EXTMERGEBROWSE, OnBnClickedExtmergebrowse)
	ON_EN_CHANGE(IDC_EXTMERGE, OnEnChangeExtmerge)
	ON_BN_CLICKED(IDC_DIFFVIEWERROWSE, OnBnClickedDiffviewerrowse)
	ON_EN_CHANGE(IDC_DIFFVIEWER, OnEnChangeDiffviewer)
	ON_BN_CLICKED(IDC_NOREMOVELOGMSG, OnBnClickedNoremovelogmsg)
END_MESSAGE_MAP()


// CSetMainPage message handlers

void CSetMainPage::OnBnClickedExtdiffbrowse()
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
	temp.LoadString(IDS_SETTINGS_SELECTDIFF);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_sDiffPath = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	}
}
void CSetMainPage::OnBnClickedExtmergebrowse()
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
	temp.LoadString(IDS_SETTINGS_SELECTMERGE);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_sMergePath = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	}
}

void CSetMainPage::OnBnClickedDiffviewerrowse()
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
	temp.LoadString(IDS_SETTINGS_SELECTDIFFVIEWER);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_sDiffViewerPath = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	}
}

BOOL CSetMainPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	EnableToolTips();

	m_sDiffPath = m_regDiffPath;
	m_sDiffViewerPath = m_regDiffViewerPath;
	m_sMergePath = m_regMergePath;
	m_sTempExtensions = m_regExtensions;
	m_bAddBeforeCommit = m_regAddBeforeCommit;
	m_bNoRemoveLogMsg = m_regNoRemoveLogMsg;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);
	m_tooltips.AddTool(IDC_DIFFVIEWER, IDS_SETTINGS_DIFFVIEWER_TT);
	m_tooltips.AddTool(IDC_EXTMERGE, IDS_SETTINGS_EXTMERGE_TT);
	m_tooltips.AddTool(IDC_EXTDIFFBROWSE, IDS_SETTINGS_EXTDIFFBROWSE_TT);
	m_tooltips.AddTool(IDC_TEMPEXTENSIONS, IDS_SETTINGS_TEMPEXTENSIONS_TT);
	m_tooltips.AddTool(IDC_ADDBEFORECOMMIT, IDS_SETTINGS_ADDBEFORECOMMIT_TT);
	m_tooltips.SetEffectBk(CBalloon::BALLOON_EFFECT_HGRADIENT);
	m_tooltips.SetGradientColors(0x80ffff, 0x000000, 0xffff80);

	//set up the language selecting combobox
	m_LanguageCombo.AddString(_T("English"));
	m_LanguageCombo.SetItemData(0, 1033);
	m_LanguageCombo.AddString(_T("Deutsch"));
	m_LanguageCombo.SetItemData(1, 1031);
	m_LanguageCombo.AddString(_T("Italiano"));
	m_LanguageCombo.SetItemData(2, 1040);
	m_LanguageCombo.AddString(_T("Russian"));
	m_LanguageCombo.SetItemData(3, 1049);
	m_LanguageCombo.AddString(_T("Norwegian"));
	m_LanguageCombo.SetItemData(4, 1044);

	int index = 0;
	switch ((int)m_regLanguage)
	{
	case 1033:	//english
		index = 0;
		break;
	case 1031:	//german
		index = 1;
		break;
	case 1040:	//italian
		index = 2;
		break;
	case 1049:	//russian
		index = 3;
		break;
	case 1044:	//norwegian
		index = 4;
		break;
	default:
		break;
	}
	m_LanguageCombo.SetCurSel(index);
	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetMainPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CSetMainPage::OnEnChangeExtdiff()
{
	SetModified();
}

void CSetMainPage::OnEnChangeExtmerge()
{
	SetModified();
}

void CSetMainPage::OnEnChangeDiffviewer()
{
	SetModified();
}

void CSetMainPage::OnCbnSelchangeLanguagecombo()
{
	SetModified();
}

void CSetMainPage::OnEnChangeTempextensions()
{
	SetModified();
}

void CSetMainPage::OnBnClickedAddbeforecommit()
{
	SetModified();
}

void CSetMainPage::OnBnClickedNoremovelogmsg()
{
	SetModified();
}

BOOL CSetMainPage::OnApply()
{
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}






