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
#include "SetOverlayPage.h"
#include "SetOverlayIcons.h"
#include "Globals.h"
#include "ShellUpdater.h"
#include ".\setoverlaypage.h"


// CSetOverlayPage dialog

IMPLEMENT_DYNAMIC(CSetOverlayPage, CPropertyPage)
CSetOverlayPage::CSetOverlayPage()
	: CPropertyPage(CSetOverlayPage::IDD)
	, m_bInitialized(FALSE)
	, m_bRemovable(FALSE)
	, m_bNetwork(FALSE)
	, m_bFixed(FALSE)
	, m_bCDROM(FALSE)
	, m_bRAM(FALSE)
	, m_bUnknown(FALSE)
	, m_bOnlyExplorer(FALSE)
	, m_sExcludePaths(_T(""))
	, m_sIncludePaths(_T(""))
	, m_bRecursive(TRUE)
{
	m_regOnlyExplorer = CRegDWORD(_T("Software\\TortoiseSVN\\OverlaysOnlyInExplorer"), FALSE);
	m_regDriveMaskRemovable = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskRemovable"));
	m_regDriveMaskRemote = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskRemote"));
	m_regDriveMaskFixed = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskFixed"), TRUE);
	m_regDriveMaskCDROM = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskCDROM"));
	m_regDriveMaskRAM = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskRAM"));
	m_regDriveMaskUnknown = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskUnknown"));
	m_regExcludePaths = CRegString(_T("Software\\TortoiseSVN\\OverlayExcludeList"));
	m_regIncludePaths = CRegString(_T("Software\\TortoiseSVN\\OverlayIncludeList"));
	m_regRecursive = CRegDWORD(_T("Software\\TortoiseSVN\\RecursiveOverlay"), TRUE);

	m_bOnlyExplorer = m_regOnlyExplorer;
	m_bRemovable = m_regDriveMaskRemovable;
	m_bNetwork = m_regDriveMaskRemote;
	m_bFixed = m_regDriveMaskFixed;
	m_bCDROM = m_regDriveMaskCDROM;
	m_bRAM = m_regDriveMaskRAM;
	m_bUnknown = m_regDriveMaskUnknown;
	m_sExcludePaths = m_regExcludePaths;
	m_sExcludePaths.Replace(_T("\n"), _T("\r\n"));
	m_sIncludePaths = m_regIncludePaths;
	m_sIncludePaths.Replace(_T("\n"), _T("\r\n"));
	m_bRecursive = m_regRecursive;
}

CSetOverlayPage::~CSetOverlayPage()
{
}

void CSetOverlayPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_REMOVABLE, m_bRemovable);
	DDX_Check(pDX, IDC_NETWORK, m_bNetwork);
	DDX_Check(pDX, IDC_FIXED, m_bFixed);
	DDX_Check(pDX, IDC_CDROM, m_bCDROM);
	DDX_Check(pDX, IDC_RAM, m_bRAM);
	DDX_Check(pDX, IDC_UNKNOWN, m_bUnknown);
	DDX_Check(pDX, IDC_ONLYEXPLORER, m_bOnlyExplorer);
	DDX_Text(pDX, IDC_EXCLUDEPATHS, m_sExcludePaths);
	DDX_Text(pDX, IDC_INCLUDEPATHS, m_sIncludePaths);
	DDX_Check(pDX, IDC_RECURSIVECHECK, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CSetOverlayPage, CPropertyPage)
	ON_BN_CLICKED(IDC_REMOVABLE, OnBnClickedRemovable)
	ON_BN_CLICKED(IDC_NETWORK, OnBnClickedNetwork)
	ON_BN_CLICKED(IDC_FIXED, OnBnClickedFixed)
	ON_BN_CLICKED(IDC_CDROM, OnBnClickedCdrom)
	ON_BN_CLICKED(IDC_UNKNOWN, OnBnClickedUnknown)
	ON_BN_CLICKED(IDC_RAM, OnBnClickedRam)
	ON_BN_CLICKED(IDC_ONLYEXPLORER, OnBnClickedOnlyexplorer)
	ON_EN_CHANGE(IDC_EXCLUDEPATHS, OnEnChangeExcludepaths)
	ON_EN_CHANGE(IDC_INCLUDEPATHS, OnEnChangeIncludepaths)
	ON_BN_CLICKED(IDC_RECURSIVECHECK, OnBnClickedRecursivecheck)
END_MESSAGE_MAP()


void CSetOverlayPage::SaveData()
{
	if (m_bInitialized)
	{
		m_regOnlyExplorer = m_bOnlyExplorer;
		m_regDriveMaskRemovable = m_bRemovable;
		m_regDriveMaskRemote = m_bNetwork;
		m_regDriveMaskFixed = m_bFixed;
		m_regDriveMaskCDROM = m_bCDROM;
		m_regDriveMaskRAM = m_bRAM;
		m_regDriveMaskUnknown = m_bUnknown;
		m_regRecursive = m_bRecursive;
		m_sExcludePaths.Replace(_T("\r"), _T(""));
		if (m_sExcludePaths.Right(1).Compare(_T("\n"))!=0)
			m_sExcludePaths += _T("\n");
		m_regExcludePaths = m_sExcludePaths;
		m_sExcludePaths.Replace(_T("\n"), _T("\r\n"));
		m_sIncludePaths.Replace(_T("\r"), _T(""));
		if (m_sIncludePaths.Right(1).Compare(_T("\n"))!=0)
			m_sIncludePaths += _T("\n");
		m_regIncludePaths = m_sIncludePaths;
		m_sIncludePaths.Replace(_T("\n"), _T("\r\n"));
	}
}

BOOL CSetOverlayPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_ONLYEXPLORER, IDS_SETTINGS_ONLYEXPLORER_TT);
	m_tooltips.AddTool(IDC_EXCLUDEPATHS, IDS_SETTINGS_EXCLUDELIST_TT);	
	m_tooltips.AddTool(IDC_INCLUDEPATHS, IDS_SETTINGS_INCLUDELIST_TT);	
	m_tooltips.AddTool(IDC_RECURSIVECHECK, IDS_SETTINGS_RECURSIVE_TT);
	m_bInitialized = TRUE;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetOverlayPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CSetOverlayPage::OnBnClickedRemovable()
{
	SetModified();
}

void CSetOverlayPage::OnBnClickedNetwork()
{
	SetModified();
}

void CSetOverlayPage::OnBnClickedFixed()
{
	SetModified();
}

void CSetOverlayPage::OnBnClickedCdrom()
{
	SetModified();
}

void CSetOverlayPage::OnBnClickedRam()
{
	SetModified();
}

void CSetOverlayPage::OnBnClickedUnknown()
{
	SetModified();
}

void CSetOverlayPage::OnBnClickedOnlyexplorer()
{
	SetModified();
}

BOOL CSetOverlayPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CSetOverlayPage::OnEnChangeExcludepaths()
{
	SetModified();
}

void CSetOverlayPage::OnEnChangeIncludepaths()
{
	SetModified();
}

void CSetOverlayPage::OnBnClickedRecursivecheck()
{
	SetModified();
}
