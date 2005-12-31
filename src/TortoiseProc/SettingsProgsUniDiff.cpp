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
#include "Utils.h"
#include ".\settingsprogsunidiff.h"


// CSettingsProgsUniDiff dialog

IMPLEMENT_DYNAMIC(CSettingsProgsUniDiff, CPropertyPage)
CSettingsProgsUniDiff::CSettingsProgsUniDiff()
	: CPropertyPage(CSettingsProgsUniDiff::IDD)
	, m_sDiffViewerPath(_T(""))
	, m_iDiffViewer(0)
	, m_bInitialized(FALSE)
{
	m_regDiffViewerPath = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
}

CSettingsProgsUniDiff::~CSettingsProgsUniDiff()
{
}

void CSettingsProgsUniDiff::SaveData()
{
	if (m_bInitialized)
	{
		if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != _T("#"))
			m_sDiffViewerPath = _T("#") + m_sDiffViewerPath;

		m_regDiffViewerPath = m_sDiffViewerPath;
	}
}

void CSettingsProgsUniDiff::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DIFFVIEWER, m_sDiffViewerPath);
	DDX_Radio(pDX, IDC_DIFFVIEWER_OFF, m_iDiffViewer);

	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(m_iDiffViewer == 1);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(m_iDiffViewer == 1);
	DDX_Control(pDX, IDC_DIFFVIEWER, m_cUnifiedDiffEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsUniDiff, CPropertyPage)
	ON_BN_CLICKED(IDC_DIFFVIEWER_OFF, OnBnClickedDiffviewerOff)
	ON_BN_CLICKED(IDC_DIFFVIEWER_ON, OnBnClickedDiffviewerOn)
	ON_BN_CLICKED(IDC_DIFFVIEWERBROWSE, OnBnClickedDiffviewerbrowse)
	ON_EN_CHANGE(IDC_DIFFVIEWER, OnEnChangeDiffviewer)
END_MESSAGE_MAP()


// CSettingsProgsUniDiff message handlers

void CSettingsProgsUniDiff::OnBnClickedDiffviewerOff()
{
	m_iDiffViewer = 0;
	SetModified();
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(FALSE);
	CheckProgComment();
}

void CSettingsProgsUniDiff::OnBnClickedDiffviewerOn()
{
	m_iDiffViewer = 1;
	SetModified();
	GetDlgItem(IDC_DIFFVIEWER)->EnableWindow(TRUE);
	GetDlgItem(IDC_DIFFVIEWERBROWSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_DIFFVIEWER)->SetFocus();
	CheckProgComment();
}

void CSettingsProgsUniDiff::OnEnChangeDiffviewer()
{
	SetModified();
}

void CSettingsProgsUniDiff::OnBnClickedDiffviewerbrowse()
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

BOOL CSettingsProgsUniDiff::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	EnableToolTips();

	m_sDiffViewerPath = m_regDiffViewerPath;
	m_iDiffViewer = IsExternal(m_sDiffViewerPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_DIFFVIEWER), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_DIFFVIEWER, IDS_SETTINGS_DIFFVIEWER_TT);

	m_bInitialized = TRUE;
	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsUniDiff::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CSettingsProgsUniDiff::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CSettingsProgsUniDiff::CheckProgComment()
{
	UpdateData();
	if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != _T("#"))
		m_sDiffViewerPath = _T("#") + m_sDiffViewerPath;
	else if (m_iDiffViewer == 1)
		m_sDiffViewerPath.TrimLeft('#');
	UpdateData(FALSE);
}