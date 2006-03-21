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
#include "SetMisc.h"
#include "MessageBox.h"


// CSetMisc dialog

IMPLEMENT_DYNAMIC(CSetMisc, CPropertyPage)

CSetMisc::CSetMisc()
	: CPropertyPage(CSetMisc::IDD)
	, m_bUnversionedRecurse(FALSE)
	, m_bAutocompletion(FALSE)
	, m_dwAutocompletionTimeout(0)
	, m_bSpell(TRUE)
	, m_bCheckRepo(FALSE)
	, m_dwMaxHistory(25)
{
	m_regUnversionedRecurse = CRegDWORD(_T("Software\\TortoiseSVN\\UnversionedRecurse"), TRUE);
	m_bUnversionedRecurse = (DWORD)m_regUnversionedRecurse;
	m_regAutocompletion = CRegDWORD(_T("Software\\TortoiseSVN\\Autocompletion"), TRUE);
	m_bAutocompletion = (DWORD)m_regAutocompletion;
	m_regAutocompletionTimeout = CRegDWORD(_T("Software\\TortoiseSVN\\AutocompleteParseTimeout"), 5);
	m_dwAutocompletionTimeout = (DWORD)m_regAutocompletionTimeout;
	m_regSpell = CRegDWORD(_T("Software\\TortoiseSVN\\Spellchecker"), FALSE);
	m_bSpell = (DWORD)m_regSpell;
	m_regCheckRepo = CRegDWORD(_T("Software\\TortoiseSVN\\CheckRepo"), FALSE);
	m_bCheckRepo = (DWORD)m_regCheckRepo;
	m_regMaxHistory = CRegDWORD(_T("Software\\TortoiseSVN\\MaxHistoryItems"), 25);
	m_dwMaxHistory = (DWORD)m_regMaxHistory;
}

CSetMisc::~CSetMisc()
{
}

void CSetMisc::SaveData()
{
	m_regUnversionedRecurse = m_bUnversionedRecurse;
	if (m_regUnversionedRecurse.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regUnversionedRecurse.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regAutocompletion = m_bAutocompletion;
	if (m_regAutocompletion.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regAutocompletion.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regAutocompletionTimeout = m_dwAutocompletionTimeout;
	if (m_regAutocompletionTimeout.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regAutocompletionTimeout.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regSpell = m_bSpell;
	if (m_regSpell.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regSpell.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regCheckRepo = m_bCheckRepo;
	if (m_regCheckRepo.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regCheckRepo.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regMaxHistory = m_dwMaxHistory;
	if (m_regMaxHistory.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regMaxHistory.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
}

void CSetMisc::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_UNVERSIONEDRECURSE, m_bUnversionedRecurse);
	DDX_Check(pDX, IDC_AUTOCOMPLETION, m_bAutocompletion);
	DDX_Text(pDX, IDC_AUTOCOMPLETIONTIMEOUT, m_dwAutocompletionTimeout);
	DDV_MinMaxUInt(pDX, m_dwAutocompletionTimeout, 1, 100);
	DDX_Check(pDX, IDC_SPELL, m_bSpell);
	DDX_Check(pDX, IDC_REPOCHECK, m_bCheckRepo);
	DDX_Text(pDX, IDC_MAXHISTORY, m_dwMaxHistory);
	DDV_MinMaxUInt(pDX, m_dwMaxHistory, 1, 100);
}


BEGIN_MESSAGE_MAP(CSetMisc, CPropertyPage)
	ON_BN_CLICKED(IDC_UNVERSIONEDRECURSE, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_AUTOCOMPLETION, &CSetMisc::OnChanged)
	ON_EN_CHANGE(IDC_AUTOCOMPLETIONTIMEOUT, &CSetMisc::OnChanged)
	ON_EN_CHANGE(IDC_MAXHISTORY, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_SPELL, &CSetMisc::OnChanged)
	ON_BN_CLICKED(IDC_REPOCHECK, &CSetMisc::OnChanged)
END_MESSAGE_MAP()


// CSetMisc message handlers

void CSetMisc::OnChanged()
{
	SetModified();
}

BOOL CSetMisc::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_UNVERSIONEDRECURSE, IDS_SETTINGS_UNVERSIONEDRECURSE_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUT, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_AUTOCOMPLETIONTIMEOUTLABEL, IDS_SETTINGS_AUTOCOMPLETIONTIMEOUT_TT);
	m_tooltips.AddTool(IDC_SPELL, IDS_SETTINGS_SPELLCHECKER_TT);
	m_tooltips.AddTool(IDC_REPOCHECK, IDS_SETTINGS_REPOCHECK_TT);
	m_tooltips.AddTool(IDC_MAXHISTORY, IDS_SETTINGS_MAXHISTORY_TT);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetMisc::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}
