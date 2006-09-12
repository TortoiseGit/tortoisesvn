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
#include "AppUtils.h"
#include "StringUtils.h"
#include ".\settingsprogsmerge.h"


IMPLEMENT_DYNAMIC(CSettingsProgsMerge, CPropertyPage)
CSettingsProgsMerge::CSettingsProgsMerge()
	: CPropertyPage(CSettingsProgsMerge::IDD)
	, m_sMergePath(_T(""))
	, m_iExtMerge(0)
	, m_dlgAdvMerge(_T("Merge"))
	, m_bInitialized(FALSE)
{
	m_regMergePath = CRegString(_T("Software\\TortoiseSVN\\Merge"));
}

CSettingsProgsMerge::~CSettingsProgsMerge()
{
}

void CSettingsProgsMerge::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTMERGE, m_sMergePath);
	DDX_Radio(pDX, IDC_EXTMERGE_OFF, m_iExtMerge);

	GetDlgItem(IDC_EXTMERGE)->EnableWindow(m_iExtMerge == 1);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(m_iExtMerge == 1);
	DDX_Control(pDX, IDC_EXTMERGE, m_cMergeEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsMerge, CPropertyPage)
	ON_BN_CLICKED(IDC_EXTMERGE_OFF, OnBnClickedExtmergeOff)
	ON_BN_CLICKED(IDC_EXTMERGE_ON, OnBnClickedExtmergeOn)
	ON_BN_CLICKED(IDC_EXTMERGEBROWSE, OnBnClickedExtmergebrowse)
	ON_BN_CLICKED(IDC_EXTMERGEADVANCED, OnBnClickedExtmergeadvanced)
	ON_EN_CHANGE(IDC_EXTMERGE, OnEnChangeExtmerge)
END_MESSAGE_MAP()


int CSettingsProgsMerge::SaveData()
{
	if (m_bInitialized)
	{
		if (m_iExtMerge == 0 && !m_sMergePath.IsEmpty() && m_sMergePath.Left(1) != _T("#"))
			m_sMergePath = _T("#") + m_sMergePath;

		m_regMergePath = m_sMergePath;

		m_dlgAdvMerge.SaveData();
	}
	return 0;
}

BOOL CSettingsProgsMerge::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	EnableToolTips();

	m_sMergePath = m_regMergePath;
	m_iExtMerge = IsExternal(m_sMergePath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTMERGE), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTMERGE, IDS_SETTINGS_EXTMERGE_TT);

	m_bInitialized = TRUE;
	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsMerge::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CSettingsProgsMerge::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CSettingsProgsMerge::OnBnClickedExtmergeOff()
{
	m_iExtMerge = 0;
	SetModified();
	GetDlgItem(IDC_EXTMERGE)->EnableWindow(FALSE);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(FALSE);
	CheckProgComment();
}

void CSettingsProgsMerge::OnBnClickedExtmergeOn()
{
	m_iExtMerge = 1;
	SetModified();
	GetDlgItem(IDC_EXTMERGE)->EnableWindow(TRUE);
	GetDlgItem(IDC_EXTMERGEBROWSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_EXTMERGE)->SetFocus();
	CheckProgComment();
}

void CSettingsProgsMerge::OnEnChangeExtmerge()
{
	SetModified();
}

void CSettingsProgsMerge::OnBnClickedExtmergebrowse()
{
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	CString sFilter;
	sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
	// Replace '|' delimiters with '\0's
	TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
	while (ptr != pszFilters)
	{
		if (*ptr == '|')
			*ptr = '\0';
		ptr--;
	}
	ofn.lpstrFilter = pszFilters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	temp.LoadString(IDS_SETTINGS_SELECTMERGE);
	CStringUtils::RemoveAccelerators(temp);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_sMergePath = CString(ofn.lpstrFile);
		UpdateData(FALSE);
		SetModified();
	}
	delete [] pszFilters;
}

void CSettingsProgsMerge::OnBnClickedExtmergeadvanced()
{
	if (m_dlgAdvMerge.DoModal() == IDOK)
		SetModified();
}

void CSettingsProgsMerge::CheckProgComment()
{
	UpdateData();
	if (m_iExtMerge == 0 && !m_sMergePath.IsEmpty() && m_sMergePath.Left(1) != _T("#"))
		m_sMergePath = _T("#") + m_sMergePath;
	else if (m_iExtMerge == 1)
		m_sMergePath.TrimLeft('#');
	UpdateData(FALSE);
}