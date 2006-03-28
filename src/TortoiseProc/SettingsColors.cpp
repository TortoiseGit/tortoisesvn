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
#include "SettingsColors.h"
#include ".\settingscolors.h"


// CSettingsColors dialog

IMPLEMENT_DYNAMIC(CSettingsColors, CPropertyPage)
CSettingsColors::CSettingsColors()
	: CPropertyPage(CSettingsColors::IDD)
	, bInit(false)
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
	DDX_Control(pDX, IDC_DELETEDNODECOLOR, m_cDeletedNode);
	DDX_Control(pDX, IDC_ADDEDNODECOLOR, m_cAddedNode);
	DDX_Control(pDX, IDC_REPLACEDNODECOLOR, m_cReplacedNode);
	DDX_Control(pDX, IDC_RENAMEDNODECOLOR, m_cRenamedNode);
}


BEGIN_MESSAGE_MAP(CSettingsColors, CPropertyPage)
	ON_MESSAGE(CPN_SELCHANGE, OnColorChanged)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedRestore)
END_MESSAGE_MAP()

int CSettingsColors::SaveData()
{
	if (!bInit)
		return 0;
	m_Colors.SetColor(CColors::Added, m_cAdded.GetColor(TRUE));
	m_Colors.SetColor(CColors::Deleted, m_cDeleted.GetColor(TRUE));
	m_Colors.SetColor(CColors::Merged, m_cMerged.GetColor(TRUE));
	m_Colors.SetColor(CColors::Modified, m_cModified.GetColor(TRUE));
	m_Colors.SetColor(CColors::Conflict, m_cConflict.GetColor(TRUE));
	m_Colors.SetColor(CColors::AddedNode, m_cAddedNode.GetColor(TRUE));
	m_Colors.SetColor(CColors::DeletedNode, m_cDeletedNode.GetColor(TRUE));
	m_Colors.SetColor(CColors::RenamedNode, m_cRenamedNode.GetColor(TRUE));
	m_Colors.SetColor(CColors::ReplacedNode, m_cReplacedNode.GetColor(TRUE));
	return 0;
}

BOOL CSettingsColors::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_cAdded.SetColor(m_Colors.GetColor(CColors::Added));
	m_cDeleted.SetColor(m_Colors.GetColor(CColors::Deleted));
	m_cMerged.SetColor(m_Colors.GetColor(CColors::Merged));
	m_cModified.SetColor(m_Colors.GetColor(CColors::Modified));
	m_cConflict.SetColor(m_Colors.GetColor(CColors::Conflict));
	m_cAddedNode.SetColor(m_Colors.GetColor(CColors::AddedNode));
	m_cDeletedNode.SetColor(m_Colors.GetColor(CColors::DeletedNode));
	m_cRenamedNode.SetColor(m_Colors.GetColor(CColors::RenamedNode));
	m_cReplacedNode.SetColor(m_Colors.GetColor(CColors::ReplacedNode));

	CString sDefaultText, sCustomText;
	sDefaultText.LoadString(IDS_COLOURPICKER_DEFAULTTEXT);
	sCustomText.LoadString(IDS_COLOURPICKER_CUSTOMTEXT);
	m_cAdded.SetDefaultText(sDefaultText);
	m_cAdded.SetCustomText(sCustomText);
	m_cDeleted.SetDefaultText(sDefaultText);
	m_cDeleted.SetCustomText(sCustomText);
	m_cMerged.SetDefaultText(sDefaultText);
	m_cMerged.SetCustomText(sCustomText);
	m_cModified.SetDefaultText(sDefaultText);
	m_cModified.SetCustomText(sCustomText);
	m_cConflict.SetDefaultText(sDefaultText);
	m_cConflict.SetCustomText(sCustomText);
	m_cAddedNode.SetDefaultText(sDefaultText);
	m_cAddedNode.SetCustomText(sCustomText);
	m_cDeletedNode.SetDefaultText(sDefaultText);
	m_cDeletedNode.SetCustomText(sCustomText);
	m_cRenamedNode.SetDefaultText(sDefaultText);
	m_cRenamedNode.SetCustomText(sCustomText);
	m_cReplacedNode.SetDefaultText(sDefaultText);
	m_cReplacedNode.SetCustomText(sCustomText);
	
	bInit = true;
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
	m_cAddedNode.SetColor(m_Colors.GetColor(CColors::AddedNode, true));
	m_cDeletedNode.SetColor(m_Colors.GetColor(CColors::DeletedNode, true));
	m_cRenamedNode.SetColor(m_Colors.GetColor(CColors::RenamedNode, true));
	m_cReplacedNode.SetColor(m_Colors.GetColor(CColors::ReplacedNode, true));
	SetModified(TRUE);
}
