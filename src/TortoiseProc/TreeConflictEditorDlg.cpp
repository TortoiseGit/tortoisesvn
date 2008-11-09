// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "TreeConflictEditorDlg.h"

#include "SVN.h"
#include "MessageBox.h"

// CTreeConflictEditorDlg dialog

IMPLEMENT_DYNAMIC(CTreeConflictEditorDlg, CDialog)

CTreeConflictEditorDlg::CTreeConflictEditorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTreeConflictEditorDlg::IDD, pParent)
{

}

CTreeConflictEditorDlg::~CTreeConflictEditorDlg()
{
}

void CTreeConflictEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTreeConflictEditorDlg, CDialog)
	ON_BN_CLICKED(IDC_RESOLVEUSINGTHEIRS, &CTreeConflictEditorDlg::OnBnClickedResolveusingtheirs)
	ON_BN_CLICKED(IDC_RESOLVEUSINGMINE, &CTreeConflictEditorDlg::OnBnClickedResolveusingmine)
END_MESSAGE_MAP()


// CTreeConflictEditorDlg message handlers

BOOL CTreeConflictEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetDlgItemText(IDC_CONFLICTINFO, m_sConflictInfo);
	SetDlgItemText(IDC_RESOLVEUSINGTHEIRS, m_sUseTheirs);
	SetDlgItemText(IDC_RESOLVEUSINGMINE, m_sUseMine);


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CTreeConflictEditorDlg::OnBnClickedResolveusingtheirs()
{
	SVN svn;
	if (!svn.Resolve(m_path, svn_wc_conflict_choose_theirs_full, false))
	{
		CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}
	EndDialog(IDC_RESOLVEUSINGTHEIRS);
}

void CTreeConflictEditorDlg::OnBnClickedResolveusingmine()
{
	SVN svn;
	if (!svn.Resolve(m_path, svn_wc_conflict_choose_mine_full, false))
	{
		CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}
	EndDialog(IDC_RESOLVEUSINGMINE);
}
