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
#include "Utils.h"
#include ".\settingsprogsdiff.h"


// CSettingsProgsDiff dialog

IMPLEMENT_DYNAMIC(CSettingsProgsDiff, CPropertyPage)
CSettingsProgsDiff::CSettingsProgsDiff()
	: CPropertyPage(CSettingsProgsDiff::IDD)
	, m_dlgAdvDiff(_T("Diff"))
	, m_iExtDiff(0)
	, m_sDiffPath(_T(""))
	, m_bInitialized(FALSE)
	, m_regConvertBase(_T("Software\\TortoiseSVN\\ConvertBase"), TRUE)
	, m_bConvertBase(false)
{
	m_regDiffPath = CRegString(_T("Software\\TortoiseSVN\\Diff"));
}

CSettingsProgsDiff::~CSettingsProgsDiff()
{
}

void CSettingsProgsDiff::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTDIFF, m_sDiffPath);
	DDX_Radio(pDX, IDC_EXTDIFF_OFF, m_iExtDiff);
	DDX_Check(pDX, IDC_DONTCONVERT, m_bConvertBase);

	GetDlgItem(IDC_EXTDIFF)->EnableWindow(m_iExtDiff == 1);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(m_iExtDiff == 1);
	DDX_Control(pDX, IDC_EXTDIFF, m_cDiffEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsDiff, CPropertyPage)
	ON_BN_CLICKED(IDC_EXTDIFF_OFF, OnBnClickedExtdiffOff)
	ON_BN_CLICKED(IDC_EXTDIFF_ON, OnBnClickedExtdiffOn)
	ON_BN_CLICKED(IDC_EXTDIFFBROWSE, OnBnClickedExtdiffbrowse)
	ON_BN_CLICKED(IDC_EXTDIFFADVANCED, OnBnClickedExtdiffadvanced)
	ON_BN_CLICKED(IDC_DONTCONVERT, OnBnClickedDontconvert)
	ON_EN_CHANGE(IDC_EXTDIFF, OnEnChangeExtdiff)
END_MESSAGE_MAP()


void CSettingsProgsDiff::SaveData()
{
	if (m_bInitialized)
	{
		if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != _T("#"))
			m_sDiffPath = _T("#") + m_sDiffPath;

		m_regDiffPath = m_sDiffPath;
		m_regConvertBase = m_bConvertBase;
		m_dlgAdvDiff.SaveData();
	}
}

BOOL CSettingsProgsDiff::OnInitDialog()
{
	EnableToolTips();

	m_sDiffPath = m_regDiffPath;
	m_iExtDiff = IsExternal(m_sDiffPath);

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTDIFF), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	m_bConvertBase = m_regConvertBase;

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);
	m_tooltips.AddTool(IDC_DONTCONVERT, IDS_SETTINGS_CONVERTBASE_TT);

	m_bInitialized = TRUE;
	UpdateData(FALSE);
	return TRUE;
}

BOOL CSettingsProgsDiff::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CSettingsProgsDiff::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CSettingsProgsDiff::OnBnClickedExtdiffOff()
{
	m_iExtDiff = 0;
	SetModified();
	GetDlgItem(IDC_EXTDIFF)->EnableWindow(false);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(false);
	CheckProgComment();
}

void CSettingsProgsDiff::OnBnClickedExtdiffOn()
{
	m_iExtDiff = 1;
	SetModified();
	GetDlgItem(IDC_EXTDIFF)->EnableWindow(true);
	GetDlgItem(IDC_EXTDIFFBROWSE)->EnableWindow(true);
	GetDlgItem(IDC_EXTDIFF)->SetFocus();
	CheckProgComment();
}

void CSettingsProgsDiff::OnBnClickedExtdiffbrowse()
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

void CSettingsProgsDiff::OnBnClickedExtdiffadvanced()
{
	if (m_dlgAdvDiff.DoModal() == IDOK)
		SetModified();
}

void CSettingsProgsDiff::OnBnClickedDontconvert()
{
	SetModified();
}

void CSettingsProgsDiff::OnEnChangeExtdiff()
{
	SetModified();
}

void CSettingsProgsDiff::CheckProgComment()
{
	UpdateData();
	if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != _T("#"))
		m_sDiffPath = _T("#") + m_sDiffPath;
	else if (m_iExtDiff == 1)
		m_sDiffPath.TrimLeft('#');
	UpdateData(FALSE);
}