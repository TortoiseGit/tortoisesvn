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
#include "SettingsColors.h"
#include ".\settingscolors.h"


// CSettingsColors dialog

IMPLEMENT_DYNAMIC(CSettingsColors, CPropertyPage)
CSettingsColors::CSettingsColors()
	: CPropertyPage(CSettingsColors::IDD)
{
}

CSettingsColors::~CSettingsColors()
{
}

void CSettingsColors::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CONFLICTCOLOR, m_cConflict);
	DDX_Control(pDX, IDC_ADDEDCOLOR, m_cAdded);
	DDX_Control(pDX, IDC_DELETEDCOLOR, m_cDeleted);
	DDX_Control(pDX, IDC_MERGEDCOLOR, m_cMerged);
	DDX_Control(pDX, IDC_MODIFIEDCOLOR, m_cModified);
}


BEGIN_MESSAGE_MAP(CSettingsColors, CPropertyPage)
	ON_MESSAGE(CPN_SELCHANGE, OnColorChanged)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
END_MESSAGE_MAP()

void CSettingsColors::SaveData()
{
	m_Colors.SetColor(CColors::Added, m_cAdded.GetColor(TRUE));
	m_Colors.SetColor(CColors::Deleted, m_cDeleted.GetColor(TRUE));
	m_Colors.SetColor(CColors::Merged, m_cMerged.GetColor(TRUE));
	m_Colors.SetColor(CColors::Modified, m_cModified.GetColor(TRUE));
	m_Colors.SetColor(CColors::Conflict, m_cConflict.GetColor(TRUE));
}

BOOL CSettingsColors::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_cAdded.SetColor(m_Colors.GetColor(CColors::Added));
	m_cDeleted.SetColor(m_Colors.GetColor(CColors::Deleted));
	m_cMerged.SetColor(m_Colors.GetColor(CColors::Merged));
	m_cModified.SetColor(m_Colors.GetColor(CColors::Modified));
	m_cConflict.SetColor(m_Colors.GetColor(CColors::Conflict));

	return TRUE;
}

LRESULT CSettingsColors::OnColorChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	SetModified(TRUE);
	return 0;
}

void CSettingsColors::OnBnClickedRestore()
{
	m_cAdded.SetColor(m_Colors.GetColor(CColors::Added, true));
	m_cDeleted.SetColor(m_Colors.GetColor(CColors::Deleted, true));
	m_cMerged.SetColor(m_Colors.GetColor(CColors::Merged, true));
	m_cModified.SetColor(m_Colors.GetColor(CColors::Modified, true));
	m_cConflict.SetColor(m_Colors.GetColor(CColors::Conflict, true));
	SetModified(TRUE);
}
