// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include "BrowseFolder.h"
#include "OpenDlg.h"
#include ".\opendlg.h"


// COpenDlg dialog

IMPLEMENT_DYNAMIC(COpenDlg, CDialog)
COpenDlg::COpenDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COpenDlg::IDD, pParent)
	, m_sBaseFile(_T(""))
	, m_sTheirFile(_T(""))
	, m_sYourFile(_T(""))
	, m_sUnifiedDiffFile(_T(""))
	, m_sPatchDirectory(_T(""))
{
}

COpenDlg::~COpenDlg()
{
}

void COpenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BASEFILEEDIT, m_sBaseFile);
	DDX_Text(pDX, IDC_THEIRFILEEDIT, m_sTheirFile);
	DDX_Text(pDX, IDC_YOURFILEEDIT, m_sYourFile);
	DDX_Text(pDX, IDC_DIFFFILEEDIT, m_sUnifiedDiffFile);
	DDX_Text(pDX, IDC_DIRECTORYEDIT, m_sPatchDirectory);
	DDX_Control(pDX, IDC_BASEFILEEDIT, m_cBaseFileEdit);
	DDX_Control(pDX, IDC_THEIRFILEEDIT, m_cTheirFileEdit);
	DDX_Control(pDX, IDC_YOURFILEEDIT, m_cYourFileEdit);
	DDX_Control(pDX, IDC_DIFFFILEEDIT, m_cDiffFileEdit);
	DDX_Control(pDX, IDC_DIRECTORYEDIT, m_cDirEdit);
}

BEGIN_MESSAGE_MAP(COpenDlg, CDialog)
	ON_BN_CLICKED(IDC_BASEFILEBROWSE, OnBnClickedBasefilebrowse)
	ON_BN_CLICKED(IDC_THEIRFILEBROWSE, OnBnClickedTheirfilebrowse)
	ON_BN_CLICKED(IDC_YOURFILEBROWSE, OnBnClickedYourfilebrowse)
	ON_BN_CLICKED(IDC_HELPBUTTON, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_DIFFFILEBROWSE, OnBnClickedDifffilebrowse)
	ON_BN_CLICKED(IDC_DIRECTORYBROWSE, OnBnClickedDirectorybrowse)
	ON_BN_CLICKED(IDC_MERGERADIO, OnBnClickedMergeradio)
	ON_BN_CLICKED(IDC_APPLYRADIO, OnBnClickedApplyradio)
END_MESSAGE_MAP()

BOOL COpenDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	GroupRadio(IDC_MERGERADIO);

	CheckRadioButton(IDC_MERGERADIO, IDC_APPLYRADIO, IDC_MERGERADIO);

	// turn on autocompletion for the edit controls
	HWND hwndEdit;
	GetDlgItem(IDC_BASEFILEEDIT, &hwndEdit);
	if (hwndEdit)
		SHAutoComplete(hwndEdit, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_AUTOAPPEND_FORCE_ON | SHACF_FILESYSTEM);
	GetDlgItem(IDC_THEIRFILEEDIT, &hwndEdit);
	if (hwndEdit)
		SHAutoComplete(hwndEdit, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_AUTOAPPEND_FORCE_ON | SHACF_FILESYSTEM);
	GetDlgItem(IDC_YOURFILEEDIT, &hwndEdit);
	if (hwndEdit)
		SHAutoComplete(hwndEdit, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_AUTOAPPEND_FORCE_ON | SHACF_FILESYSTEM);
	GetDlgItem(IDC_DIFFFILEEDIT, &hwndEdit);
	if (hwndEdit)
		SHAutoComplete(hwndEdit, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_AUTOAPPEND_FORCE_ON | SHACF_FILESYSTEM);
	GetDlgItem(IDC_DIRECTORYEDIT, &hwndEdit);
	if (hwndEdit)
		SHAutoComplete(hwndEdit, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_AUTOAPPEND_FORCE_ON | SHACF_FILESYSTEM);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

// COpenDlg message handlers

void COpenDlg::OnBnClickedBasefilebrowse()
{
	CString temp;
	UpdateData();
	temp.LoadString(IDS_SELECTFILE);
	BrowseForFile(m_sBaseFile, temp);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedTheirfilebrowse()
{
	CString temp;
	UpdateData();
	temp.LoadString(IDS_SELECTFILE);
	BrowseForFile(m_sTheirFile, temp);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedYourfilebrowse()
{
	CString temp;
	UpdateData();
	temp.LoadString(IDS_SELECTFILE);
	BrowseForFile(m_sYourFile, temp);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedHelp()
{
	this->OnHelp();
}

BOOL COpenDlg::BrowseForFile(CString& filepath, CString title, UINT nFileFilter)
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
	sFilter.LoadString(nFileFilter);
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
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		filepath = CString(ofn.lpstrFile);
		delete [] pszFilters;
		return TRUE;
	} // if (GetOpenFileName(&ofn)==TRUE)
	delete [] pszFilters;
	return FALSE;			//user cancelled the dialog
}

void COpenDlg::OnBnClickedDifffilebrowse()
{
	CString temp;
	UpdateData();
	temp.LoadString(IDS_SELECTFILE);
	BrowseForFile(m_sUnifiedDiffFile, temp, IDS_PATCHFILEFILTER);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedDirectorybrowse()
{
	CBrowseFolder folderBrowser;
	UpdateData();
	folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	folderBrowser.Show(GetSafeHwnd(), m_sPatchDirectory);
	UpdateData(FALSE);
}

void COpenDlg::OnBnClickedMergeradio()
{
	GroupRadio(IDC_MERGERADIO);
}

void COpenDlg::OnBnClickedApplyradio()
{
	GroupRadio(IDC_APPLYRADIO);
}

void COpenDlg::GroupRadio(UINT nID)
{
	BOOL bMerge = FALSE;
	BOOL bUnified = FALSE;
	if (nID == IDC_MERGERADIO)
		bMerge = TRUE;
	if (nID == IDC_APPLYRADIO)
		bUnified = TRUE;

	GetDlgItem(IDC_BASEFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_BASEFILEBROWSE)->EnableWindow(bMerge);
	GetDlgItem(IDC_THEIRFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_THEIRFILEBROWSE)->EnableWindow(bMerge);
	GetDlgItem(IDC_YOURFILEEDIT)->EnableWindow(bMerge);
	GetDlgItem(IDC_YOURFILEBROWSE)->EnableWindow(bMerge);

	GetDlgItem(IDC_DIFFFILEEDIT)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIFFFILEBROWSE)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIRECTORYEDIT)->EnableWindow(bUnified);
	GetDlgItem(IDC_DIRECTORYBROWSE)->EnableWindow(bUnified);
}

void COpenDlg::OnOK()
{
	UpdateData(TRUE);
	if (GetDlgItem(IDC_BASEFILEEDIT)->IsWindowEnabled())
	{
		m_sUnifiedDiffFile.Empty();
		m_sPatchDirectory.Empty();
	} // if (GetDlgItem(IDC_BASEFILEEDIT)->IsWindowEnabled())
	else
	{
		m_sBaseFile.Empty();
		m_sYourFile.Empty();
		m_sTheirFile.Empty();
	}
	UpdateData(FALSE);
	CString sFile;
	if (!m_sUnifiedDiffFile.IsEmpty())
		if (!PathFileExists(m_sUnifiedDiffFile))
			sFile = m_sUnifiedDiffFile;
	if (!m_sPatchDirectory.IsEmpty())
		if (!PathFileExists(m_sPatchDirectory))
			sFile = m_sPatchDirectory;
	if (!m_sBaseFile.IsEmpty())
		if (!PathFileExists(m_sBaseFile))
			sFile = m_sBaseFile;
	if (!m_sYourFile.IsEmpty())
		if (!PathFileExists(m_sYourFile))
			sFile = m_sYourFile;
	if (!m_sTheirFile.IsEmpty())
		if (!PathFileExists(m_sTheirFile))
			sFile = m_sTheirFile;

	if (!sFile.IsEmpty())
	{
		CString sErr;
		sErr.Format(IDS_ERR_PATCH_INVALIDPATCHFILE, sFile);
		MessageBox(sErr, NULL, MB_ICONERROR);
		return;
	}
	CDialog::OnOK();
}
