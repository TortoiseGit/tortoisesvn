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
#include "SetProgsPage.h"
#include ".\setprogspage.h"


// CSetProgsPage dialog

IMPLEMENT_DYNAMIC(CSetProgsPage, CPropertyPage)
CSetProgsPage::CSetProgsPage()
	: CPropertyPage(CSetProgsPage::IDD)
	, m_sDiffPath(_T(""))
	, m_sDiffViewerPath(_T(""))
	, m_sMergePath(_T(""))
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
	m_regDiffPath = m_sDiffPath;
	m_regDiffViewerPath = m_sDiffViewerPath;
	m_regMergePath = m_sMergePath;
}

void CSetProgsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTDIFF, m_sDiffPath);
	DDX_Text(pDX, IDC_DIFFVIEWER, m_sDiffViewerPath);
	DDX_Text(pDX, IDC_EXTMERGE, m_sMergePath);
}


BEGIN_MESSAGE_MAP(CSetProgsPage, CPropertyPage)
	ON_BN_CLICKED(IDC_EXTDIFFBROWSE, OnBnClickedExtdiffbrowse)
	ON_EN_CHANGE(IDC_EXTDIFF, OnEnChangeExtdiff)
	ON_BN_CLICKED(IDC_EXTMERGEBROWSE, OnBnClickedExtmergebrowse)
	ON_EN_CHANGE(IDC_EXTMERGE, OnEnChangeExtmerge)
	ON_BN_CLICKED(IDC_DIFFVIEWERROWSE, OnBnClickedDiffviewerrowse)
	ON_EN_CHANGE(IDC_DIFFVIEWER, OnEnChangeDiffviewer)
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

void CSetProgsPage::OnBnClickedDiffviewerrowse()
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


// CSetProgsPage message handlers

BOOL CSetProgsPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	EnableToolTips();

	m_sDiffPath = m_regDiffPath;
	m_sDiffViewerPath = m_regDiffViewerPath;
	m_sMergePath = m_regMergePath;
	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);
	m_tooltips.AddTool(IDC_DIFFVIEWER, IDS_SETTINGS_DIFFVIEWER_TT);
	m_tooltips.AddTool(IDC_EXTMERGE, IDS_SETTINGS_EXTMERGE_TT);
	m_tooltips.AddTool(IDC_EXTDIFFBROWSE, IDS_SETTINGS_EXTDIFFBROWSE_TT);

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

