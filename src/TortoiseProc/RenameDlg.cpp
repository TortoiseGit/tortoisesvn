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
#include "MessageBox.h"
#include "TSVNPath.h"
#include "RenameDlg.h"
#include ".\renamedlg.h"


// CRenameDlg dialog

IMPLEMENT_DYNAMIC(CRenameDlg, CResizableStandAloneDialog)
CRenameDlg::CRenameDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRenameDlg::IDD, pParent)
	, m_name(_T(""))
{
}

CRenameDlg::~CRenameDlg()
{
}

void CRenameDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NAME, m_name);
}


BEGIN_MESSAGE_MAP(CRenameDlg, CResizableStandAloneDialog)
END_MESSAGE_MAP()


// CRenameDlg message handlers

BOOL CRenameDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	SHAutoComplete(GetDlgItem(IDC_NAME)->m_hWnd, SHACF_DEFAULT);

	if (!m_windowtitle.IsEmpty())
		this->SetWindowText(m_windowtitle);
	if (!m_label.IsEmpty())
		GetDlgItem(IDC_LABEL)->SetWindowText(m_label);
	AddAnchor(IDC_LABEL, TOP_LEFT);
	AddAnchor(IDC_NAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("RenameDlg"));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRenameDlg::OnOK()
{
	UpdateData();
	CTSVNPath path(m_name);
	if (!path.IsValidOnWindows())
	{
		if (CMessageBox::Show(GetSafeHwnd(), IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONWARNING | MB_OKCANCEL)==IDCANCEL)
			return;
	}
	CResizableDialog::OnOK();
}
