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
#include "Utils.h"
#include "..\version.h"
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
	, m_bAutoClose(FALSE)
	, m_sDefaultLogs(_T(""))
{
	this->m_pPSP->dwFlags &= ~PSP_HASHELP;
	m_regDiffPath = CRegString(_T("Software\\TortoiseSVN\\Diff"));
	m_regDiffViewerPath = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
	m_regMergePath = CRegString(_T("Software\\TortoiseSVN\\Merge"));
	m_regLanguage = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	m_regExtensions = CRegString(_T("Software\\TortoiseSVN\\TempFileExtensions"));
	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"));
	m_regNoRemoveLogMsg = CRegDWORD(_T("Software\\TortoiseSVN\\NoDeleteLogMsg"));
	m_regAutoClose = CRegDWORD(_T("Software\\TortoiseSVN\\AutoClose"));
	m_regDefaultLogs = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
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
	m_regAutoClose = m_bAutoClose;
	long val = _ttol(m_sDefaultLogs);
	if (val > 5)
		m_regDefaultLogs = val;
	else
		m_regDefaultLogs = 100;
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
	DDX_Check(pDX, IDC_AUTOCLOSE, m_bAutoClose);
	DDX_Text(pDX, IDC_DEFAULTLOG, m_sDefaultLogs);
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
	ON_BN_CLICKED(IDC_AUTOCLOSE, OnBnClickedAutoclose)
	ON_EN_CHANGE(IDC_DEFAULTLOG, OnEnChangeDefaultlog)
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
	m_bAutoClose = m_regAutoClose;
	m_dwLanguage = m_regLanguage;
	CString temp;
	temp.Format(_T("%ld"), (DWORD)m_regDefaultLogs);
	m_sDefaultLogs = temp;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);
	m_tooltips.AddTool(IDC_DIFFVIEWER, IDS_SETTINGS_DIFFVIEWER_TT);
	m_tooltips.AddTool(IDC_EXTMERGE, IDS_SETTINGS_EXTMERGE_TT);
	m_tooltips.AddTool(IDC_EXTDIFFBROWSE, IDS_SETTINGS_EXTDIFFBROWSE_TT);
	m_tooltips.AddTool(IDC_TEMPEXTENSIONS, IDS_SETTINGS_TEMPEXTENSIONS_TT);
	m_tooltips.AddTool(IDC_ADDBEFORECOMMIT, IDS_SETTINGS_ADDBEFORECOMMIT_TT);
	m_tooltips.AddTool(IDC_AUTOCLOSE, IDS_SETTINGS_AUTOCLOSE_TT);
	m_tooltips.AddTool(IDC_NOREMOVELOGMSG, IDS_SETTINGS_NOREMOVELOGMSG_TT);
	//m_tooltips.SetEffectBk(CBalloon::BALLOON_EFFECT_HGRADIENT);
	//m_tooltips.SetGradientColors(0x80ffff, 0x000000, 0xffff80);

	//set up the language selecting combobox
	m_LanguageCombo.AddString(_T("English"));
	m_LanguageCombo.SetItemData(0, 1033);
	CRegString str(_T("Software\\TortoiseSVN\\Directory"),_T(""), FALSE, HKEY_LOCAL_MACHINE);
	CString path = str;
	CDirFileList list;
	list.BuildList(path, FALSE, FALSE);
	int langcount = 1;
	for (int i=0; i<list.GetCount(); i++)
	{
		CString file = list.GetAt(i);
		if (file.Right(3).CompareNoCase(_T("dll"))==0)
		{
			CString filename = file.Mid(file.ReverseFind('\\')+1);
			if (filename.Left(12).CompareNoCase(_T("TortoiseProc"))==0)
			{
				if (CUtils::GetVersionFromFile(file).Compare(_T(STRPRODUCTVER_INCVERSION))!=0)
					continue;
				DWORD loc = _tstoi(filename.Mid(12));
				TCHAR buf[MAX_PATH];
				GetLocaleInfo(loc, LOCALE_SNATIVELANGNAME, buf, sizeof(buf)/sizeof(TCHAR));
				m_LanguageCombo.AddString(buf);
				m_LanguageCombo.SetItemData(langcount++, loc);
			} // if (filename.Left(12).CompareNoCase(_T("TortoiseProc"))==0) 
		} // if (file.Right(3).CompareNoCase(_T("dll"))==0) 
	} // for (int i=0; i<list.GetCount(); i++) 
	
	for (int i=0; i<m_LanguageCombo.GetCount(); i++)
	{
		if (m_LanguageCombo.GetItemData(i) == m_dwLanguage)
			m_LanguageCombo.SetCurSel(i);
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

void CSetMainPage::OnBnClickedAutoclose()
{
	SetModified();
}

void CSetMainPage::OnEnChangeDefaultlog()
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








