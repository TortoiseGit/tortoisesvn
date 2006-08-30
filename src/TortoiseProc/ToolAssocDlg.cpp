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
#include "ToolAssocDlg.h"
#include "AppUtils.h"
#include "StringUtils.h"


// CToolAssocDlg dialog

IMPLEMENT_DYNAMIC(CToolAssocDlg, CDialog)
CToolAssocDlg::CToolAssocDlg(const CString& type, bool add, CWnd* pParent /*=NULL*/)
	: CDialog(CToolAssocDlg::IDD, pParent)
	, m_sType(type)
	, m_bAdd(add)
	, m_sExtension(_T(""))
	, m_sTool(_T(""))
{
}

CToolAssocDlg::~CToolAssocDlg()
{
}

void CToolAssocDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTEDIT, m_sExtension);
	DDV_MaxChars(pDX, m_sExtension, 20);
	DDX_Text(pDX, IDC_TOOLEDIT, m_sTool);

	if (pDX->m_bSaveAndValidate)
	{
		if (m_sExtension.Find('/')<0)
		{
			m_sExtension.TrimLeft(_T("."));
			m_sExtension = _T(".") + m_sExtension;
		}
	}
}


BEGIN_MESSAGE_MAP(CToolAssocDlg, CDialog)
	ON_BN_CLICKED(IDC_TOOLBROWSE, OnBnClickedToolbrowse)
END_MESSAGE_MAP()


// CToolAssocDlg message handlers

BOOL CToolAssocDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	EnableToolTips();
	m_tooltips.Create(this);

	CString title;
	if (m_sType == _T("Diff"))
	{
		title.LoadString(m_bAdd ? IDS_DLGTITLE_ADD_DIFF_TOOL : IDS_DLGTITLE_EDIT_DIFF_TOOL);
		m_tooltips.AddTool(IDC_TOOLEDIT, IDS_SETTINGS_EXTDIFF_TT);
	}
	else
	{
		title.LoadString(m_bAdd ? IDS_DLGTITLE_ADD_MERGE_TOOL : IDS_DLGTITLE_EDIT_MERGE_TOOL);
		m_tooltips.AddTool(IDC_TOOLEDIT, IDS_SETTINGS_EXTMERGE_TT);
	}

	SetWindowText(title);
	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_TOOLEDIT), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	UpdateData(FALSE);
	return TRUE;
}

BOOL CToolAssocDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CToolAssocDlg::OnBnClickedToolbrowse()
{
	UpdateData(TRUE);

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
	CStringUtils::RemoveAccelerators(temp);
	ofn.lpstrTitle = temp;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE)
	{
		m_sTool = CString(ofn.lpstrFile);
		UpdateData(FALSE);
	} // if (GetOpenFileName(&ofn)==TRUE)
	delete [] pszFilters;
}
