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
#include "URLDlg.h"
#include ".\urldlg.h"

IMPLEMENT_DYNAMIC(CURLDlg, CResizableStandAloneDialog)
CURLDlg::CURLDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CURLDlg::IDD, pParent)
{
}

CURLDlg::~CURLDlg()
{
}

void CURLDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
}


BEGIN_MESSAGE_MAP(CURLDlg, CResizableStandAloneDialog)
	ON_WM_SIZING()
END_MESSAGE_MAP()


BOOL CURLDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS"), _T("url"));
	m_URLCombo.SetFocus();

	RECT rect;
	GetWindowRect(&rect);
	m_heigth = rect.bottom - rect.top;
	AddAnchor(IDC_LABEL, TOP_LEFT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	return FALSE;
}

void CURLDlg::OnOK()
{
	if (m_URLCombo.IsWindowEnabled())
	{
		m_URLCombo.SaveHistory();
		m_url = m_URLCombo.GetString();
		UpdateData();
	}

	CResizableStandAloneDialog::OnOK();
}


void CURLDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	// don't allow the dialog to be changed in height
	switch (fwSide)
	{
	case WMSZ_BOTTOM:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_BOTTOMRIGHT:
		pRect->bottom = pRect->top + m_heigth;
		break;
	case WMSZ_TOP:
	case WMSZ_TOPLEFT:
	case WMSZ_TOPRIGHT:
		pRect->top = pRect->bottom - m_heigth;
		break;
	}
	CResizableStandAloneDialog::OnSizing(fwSide, pRect);	
}
