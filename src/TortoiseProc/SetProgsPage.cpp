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
#include "SetProgsPage.h"
#include "SetProgsAdvDlg.h"
#include "Utils.h"
#include ".\setprogspage.h"


// CSetProgsPage dialog

IMPLEMENT_DYNAMIC(CSetProgsPage, CPropertyPage)
CSetProgsPage::CSetProgsPage()
	: CPropertyPage(CSetProgsPage::IDD)
	, m_sDiffPath(_T(""))
	, m_sDiffViewerPath(_T(""))
	, m_sMergePath(_T(""))
	, m_iExtDiff(0)
	, m_iDiffViewer(0)
	, m_iExtMerge(0)
	, m_dlgAdvDiff(_T("Diff"))
	, m_dlgAdvMerge(_T("Merge"))
	, m_bInitialized(FALSE)
	, m_regConvertBase(_T("Software\\TortoiseSVN\\ConvertBase"), FALSE)
	, m_bConvertBase(false)
{
	m_regDiffPath = CRegString(_T("Software\\TortoiseSVN\\Diff"));
	m_regDiffViewerPath = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
	m_regMergePath = CRegString(_T("Software\\TortoiseSVN\\Merge"));
}

CSetProgsPage::~CSetProgsPage()
{
}

void CSetProgsPage::SaveData()
{
	if (m_bInitialized)
	{
		if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != _T("#"))
			m_sDiffPath = _T("#") + m_sDiffPath;
		if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != _T("#"))
			m_sDiffViewerPath = _T("#") + m_sDiffViewerPath;
		if (m_iExtMerge == 0 && !m_sMergePath.IsEmpty() && m_sMergePath.Left(1) != _T("#"))
			m_sMergePath = _T("#") + m_sMergePath;

		m_regDiffPath = m_sDiffPath;
		m_regDiffViewerPath = m_sDiffViewerPath;
		m_regMergePath = m_sMergePath;
		m_regConvertBase = m_bConvertBase;

		m_dlgAdvDiff.SaveData();
		m_dlgAdvMerge.SaveData();
	}
}

void CSetProgsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTDIFF, m_sDiffPath);
	DDX_Text(pDX, IDC_DIFFVIEWER, m_sDiffViewerPath);
	DDX_Text(pDX, IDC_EXTMERGE, m_sMergePath);
	DDX_Radio(pDX, IDC_EXTDIFF_OFF, m_iExtDiff);
	DDX_Radio(pDX, IDC_DIFFVIEWER_OFF, m_iDiffViewer);
	DDX_Radio(pDX, IDC_EXTMERGE_OFF, m_iExtMerge);
	DDX_Check(pDX, IDC_DONTCONVERT, m_bConvertBase);

	GetDlgItem(IDC_EXTDIFF)->EnableWindow(m_iExtDiff == 1);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(m_iExtDiff == 1);
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(m_iDiffViewer == 1);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(m_iDiffViewer == 1);
	GetDlgItem(IDC_EXTMERGE)->EnableWindow(m_iExtMerge == 1);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(m_iExtMerge == 1);
}


BEGIN_MESSAGE_MAP(CSetProgsPage, CPropertyPage)
	ON_BN_CLICKED(IDC_EXTDIFFBROWSE, OnBnClickedExtdiffbrowse)
	ON_EN_CHANGE(IDC_EXTDIFF, OnEnChangeExtdiff)
	ON_BN_CLICKED(IDC_EXTMERGEBROWSE, OnBnClickedExtmergebrowse)
	ON_EN_CHANGE(IDC_EXTMERGE, OnEnChangeExtmerge)
	ON_BN_CLICKED(IDC_DIFFVIEWERBROWSE, OnBnClickedDiffviewerbrowse)
	ON_EN_CHANGE(IDC_DIFFVIEWER, OnEnChangeDiffviewer)
	ON_BN_CLICKED(IDC_EXTDIFF_OFF, OnBnClickedExtdiffOff)
	ON_BN_CLICKED(IDC_EXTDIFF_ON, OnBnClickedExtdiffOn)
	ON_BN_CLICKED(IDC_EXTMERGE_OFF, OnBnClickedExtmergeOff)
	ON_BN_CLICKED(IDC_EXTMERGE_ON, OnBnClickedExtmergeOn)
	ON_BN_CLICKED(IDC_DIFFVIEWER_OFF, OnBnClickedDiffviewerOff)
	ON_BN_CLICKED(IDC_DIFFVIEWER_ON, OnBnClickedDiffviewerOn)
	ON_BN_CLICKED(IDC_EXTDIFFADVANCED, OnBnClickedExtdiffadvanced)
	ON_BN_CLICKED(IDC_EXTMERGEADVANCED, OnBnClickedExtmergeadvanced)
	ON_BN_CLICKED(IDC_DONTCONVERT, OnBnClickedDontconvert)
END_MESSAGE_MAP()


void CSetProgsPage::OnBnClickedExtdiffbrowse()
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
	CString sFilter;
	sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy (pszFilters, sFilter);
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
	temp.LoadString(IDS_SETTINGS_SELECTDIFF);
	CUtils::RemoveAccelerators(temp);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_sDiffPath = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	} // if (GetOpenFileName(&ofn)==TRUE)
	delete [] pszFilters;
}
void CSetProgsPage::OnBnClickedExtmergebrowse()
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
	CString sFilter;
	sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy (pszFilters, sFilter);
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
	temp.LoadString(IDS_SETTINGS_SELECTMERGE);
	CUtils::RemoveAccelerators(temp);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_sMergePath = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	} // if (GetOpenFileName(&ofn)==TRUE)
	delete [] pszFilters;
}

void CSetProgsPage::OnBnClickedDiffviewerbrowse()
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
	CString sFilter;
	sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy (pszFilters, sFilter);
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
	temp.LoadString(IDS_SETTINGS_SELECTDIFFVIEWER);
	CUtils::RemoveAccelerators(temp);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_sDiffViewerPath = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	} // if (GetOpenFileName(&ofn)==TRUE)
	delete [] pszFilters;
}


// CSetProgsPage message handlers

BOOL CSetProgsPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	EnableToolTips();

	m_sDiffPath = m_regDiffPath;
	m_iExtDiff = IsExternal(m_sDiffPath);

	m_sDiffViewerPath = m_regDiffViewerPath;
	m_iDiffViewer = IsExternal(m_sDiffViewerPath);

	m_sMergePath = m_regMergePath;
	m_iExtMerge = IsExternal(m_sMergePath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTDIFF), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);
	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_DIFFVIEWER), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);
	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTMERGE), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_bConvertBase = m_regConvertBase;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);
	m_tooltips.AddTool(IDC_DIFFVIEWER, IDS_SETTINGS_DIFFVIEWER_TT);
	m_tooltips.AddTool(IDC_EXTMERGE, IDS_SETTINGS_EXTMERGE_TT);
	m_tooltips.AddTool(IDC_EXTDIFFBROWSE, IDS_SETTINGS_EXTDIFFBROWSE_TT);
	m_tooltips.AddTool(IDC_DONTCONVERT, IDS_SETTINGS_CONVERTBASE_TT);

	m_bInitialized = TRUE;
	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSetProgsPage::OnEnChangeExtdiff()
{
	SetModified();
}

void CSetProgsPage::OnEnChangeExtmerge()
{
	SetModified();
}

void CSetProgsPage::OnEnChangeDiffviewer()
{
	SetModified();
}

BOOL CSetProgsPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

BOOL CSetProgsPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CSetProgsPage::OnBnClickedExtdiffOff()
{
	m_iExtDiff = 0;
	SetModified();
	GetDlgItem(IDC_EXTDIFF)->EnableWindow(false);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(false);
	CheckProgComment();
}

void CSetProgsPage::OnBnClickedExtdiffOn()
{
	m_iExtDiff = 1;
	SetModified();
	GetDlgItem(IDC_EXTDIFF)->EnableWindow(true);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(true);
	GetDlgItem(IDC_EXTDIFF)->SetFocus();
	CheckProgComment();
}

void CSetProgsPage::OnBnClickedExtmergeOff()
{
	m_iExtMerge = 0;
	SetModified();
	GetDlgItem(IDC_EXTMERGE)->EnableWindow(FALSE);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(FALSE);
	CheckProgComment();
}

void CSetProgsPage::OnBnClickedExtmergeOn()
{
	m_iExtMerge = 1;
	SetModified();
	GetDlgItem(IDC_EXTMERGE)->EnableWindow(TRUE);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_EXTMERGE)->SetFocus();
	CheckProgComment();
}

void CSetProgsPage::OnBnClickedDiffviewerOff()
{
	m_iDiffViewer = 0;
	SetModified();
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(FALSE);
	CheckProgComment();
}

void CSetProgsPage::OnBnClickedDiffviewerOn()
{
	m_iDiffViewer = 1;
	SetModified();
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(TRUE);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_DIFFVIEWER)->SetFocus();
	CheckProgComment();
}

void CSetProgsPage::OnBnClickedExtdiffadvanced()
{
	if (m_dlgAdvDiff.DoModal() == IDOK)
		SetModified();
}

void CSetProgsPage::OnBnClickedExtmergeadvanced()
{
	if (m_dlgAdvMerge.DoModal() == IDOK)
		SetModified();
}

void CSetProgsPage::OnBnClickedDontconvert()
{
	SetModified();
}

void CSetProgsPage::CheckProgComment()
{
	UpdateData();
	if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != _T("#"))
		m_sDiffPath = _T("#") + m_sDiffPath;
	else if (m_iExtDiff == 1)
		m_sDiffPath.TrimLeft('#');
	if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != _T("#"))
		m_sDiffViewerPath = _T("#") + m_sDiffViewerPath;
	else if (m_iDiffViewer == 1)
		m_sDiffViewerPath.TrimLeft('#');
	if (m_iExtMerge == 0 && !m_sMergePath.IsEmpty() && m_sMergePath.Left(1) != _T("#"))
		m_sMergePath = _T("#") + m_sMergePath;
	else if (m_iExtMerge == 1)
		m_sMergePath.TrimLeft('#');
	UpdateData(FALSE);
}