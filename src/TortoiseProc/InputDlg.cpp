// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include "InputDlg.h"
#include ".\inputdlg.h"


// CInputDlg dialog

IMPLEMENT_DYNAMIC(CInputDlg, CResizableDialog)
CInputDlg::CInputDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CInputDlg::IDD, pParent)
	, m_sInputText(_T(""))
{
}

CInputDlg::~CInputDlg()
{
}

void CInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_INPUTTEXT, m_sInputText);
}


BEGIN_MESSAGE_MAP(CInputDlg, CResizableDialog)
END_MESSAGE_MAP()


// CInputDlg message handlers

BOOL CInputDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	if (!m_sHintText.IsEmpty())
	{
		GetDlgItem(IDC_HINTTEXT)->SetWindowText(m_sHintText);
	}
	if (!m_sTitle.IsEmpty())
	{
		this->SetWindowText(m_sTitle);
	}
	if (!m_sInputText.IsEmpty())
	{
		GetDlgItem(IDC_INPUTTEXT)->SetWindowText(m_sInputText);
	}

	AddAnchor(IDC_HINTTEXT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_INPUTTEXT, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_CENTER);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
