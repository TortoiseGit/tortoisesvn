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
#include "StateDlg.h"

CStateDialog::CStateDialog()
{
	m_bEnableSaveRestore = FALSE;
}

CStateDialog::CStateDialog(UINT nIDTemplate, CWnd* pParentWnd)
: CDialog(nIDTemplate, pParentWnd)
{
	m_bEnableSaveRestore = FALSE;
}

CStateDialog::CStateDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
: CDialog(lpszTemplateName, pParentWnd)
{
	m_bEnableSaveRestore = FALSE;
}

CStateDialog::~CStateDialog()
{
}

BEGIN_MESSAGE_MAP(CStateDialog, CDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CStateDialog::EnableSaveRestore(LPCTSTR pszSection, BOOL bRectOnly)
{
	m_sSection = pszSection;

	m_bEnableSaveRestore = TRUE;
	m_bRectOnly = bRectOnly;

	// restore immediately
	LoadWindowRect(pszSection, bRectOnly);
}

void CStateDialog::OnDestroy() 
{
	if (m_bEnableSaveRestore)
		SaveWindowRect(m_sSection, m_bRectOnly);
	CDialog::OnDestroy();
}
