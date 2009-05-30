// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2009 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "EditPropConflictDlg.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "SVN.h"

// CEditPropConflictDlg dialog

IMPLEMENT_DYNAMIC(CEditPropConflictDlg, CResizableStandAloneDialog)

CEditPropConflictDlg::CEditPropConflictDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CEditPropConflictDlg::IDD, pParent)
{

}

CEditPropConflictDlg::~CEditPropConflictDlg()
{
}

void CEditPropConflictDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
}

bool CEditPropConflictDlg::SetPrejFile(const CTSVNPath& prejFile)
{
	m_prejFile = prejFile;
	m_sPrejText.Empty();
	// open the prej file to get the info text
	char contentbuf[10000+1];
	FILE * File;
	_tfopen_s(&File, m_prejFile.GetWinPath(), _T("rb, ccs=UTF-8"));
	if (File == NULL)
	{
		return false;
	}
	size_t len = 0;
	for (;;)
	{
		len = fread(contentbuf, sizeof(char), 10000, File);
		m_sPrejText += CUnicodeUtils::GetUnicode(CStringA(contentbuf, (int)len));
		if (len < 10000)
		{
			fclose(File);
			// we've read the whole file
			m_sPrejText.Replace(_T("\n"), _T("\r\n"));
			return true;
		}
	}
	return false;
}

BEGIN_MESSAGE_MAP(CEditPropConflictDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_RESOLVE, &CEditPropConflictDlg::OnBnClickedResolve)
	ON_BN_CLICKED(IDC_EDITPROPS, &CEditPropConflictDlg::OnBnClickedEditprops)
END_MESSAGE_MAP()


// CEditPropConflictDlg message handlers

BOOL CEditPropConflictDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CString sInfo;
	sInfo.Format(IDS_PROPCONFLICT_INFO, (LPCTSTR)m_conflictItem.GetFileOrDirectoryName());
	SetDlgItemText(IDC_PROPINFO, sInfo);
	SetDlgItemText(IDC_PROPCONFLICTINFO, m_sPrejText);


	AddAnchor(IDC_PROPINFO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_PROPCONFLICTINFO, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_RESOLVE, BOTTOM_LEFT);
	AddAnchor(IDC_EDITPROPS, BOTTOM_LEFT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CEditPropConflictDlg::OnBnClickedResolve()
{
	// remove the prej file to 'resolve' the conflict
	m_prejFile.Delete(false);
	
	EndDialog(IDC_RESOLVE);
}

void CEditPropConflictDlg::OnBnClickedEditprops()
{
	// start the properties dialog
	CString sCmd;

	sCmd.Format(_T("%s /command:properties /path:\"%s\""),
		(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
		(LPCTSTR)m_conflictItem.GetWinPath());
	CAppUtils::LaunchApplication(sCmd, NULL, false);

	EndDialog(IDC_EDITPROPS);
}
