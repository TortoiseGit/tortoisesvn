// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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


// CSetOverlayPage dialog

IMPLEMENT_DYNAMIC(CSetOverlayPage, CPropertyPage)
CSetOverlayPage::CSetOverlayPage()
	: CPropertyPage(CSetOverlayPage::IDD)
	, m_bRemovable(FALSE)
	, m_bNetwork(FALSE)
	, m_bFixed(FALSE)
	, m_bCDROM(FALSE)
{
	m_regShowChangedDirs = CRegDWORD(_T("Software\\TortoiseSVN\\RecursiveOverlay"));
	m_regDriveMaskRemovable = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskRemovable"));
	m_regDriveMaskRemote = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskRemote"));
	m_regDriveMaskFixed = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskFixed"));
	m_regDriveMaskCDROM = CRegDWORD(_T("Software\\TortoiseSVN\\DriveMaskCDROM"));

	m_bShowChangedDirs = m_regShowChangedDirs;
	m_bRemovable = m_regDriveMaskRemovable;
	m_bNetwork = m_regDriveMaskRemote;
	m_bFixed = m_regDriveMaskFixed;
	m_bCDROM = m_regDriveMaskCDROM;
}

CSetOverlayPage::~CSetOverlayPage()
{
}

void CSetOverlayPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHANGEDDIRS, m_bShowChangedDirs);
	DDX_Check(pDX, IDC_REMOVABLE, m_bRemovable);
	DDX_Check(pDX, IDC_NETWORK, m_bNetwork);
	DDX_Check(pDX, IDC_FIXED, m_bFixed);
	DDX_Check(pDX, IDC_CDROM, m_bCDROM);
	DDX_Control(pDX, IDC_DRIVEGROUP, m_cDriveGroup);
}


BEGIN_MESSAGE_MAP(CSetOverlayPage, CPropertyPage)
	ON_BN_CLICKED(IDC_CHANGEDDIRS, OnBnClickedChangeddirs)
	ON_BN_CLICKED(IDC_REMOVABLE, OnBnClickedRemovable)
	ON_BN_CLICKED(IDC_NETWORK, OnBnClickedNetwork)
	ON_BN_CLICKED(IDC_FIXED, OnBnClickedFixed)
	ON_BN_CLICKED(IDC_CDROM, OnBnClickedCdrom)
END_MESSAGE_MAP()


void CSetOverlayPage::SaveData()
{
	m_regShowChangedDirs = m_bShowChangedDirs;
	m_regDriveMaskRemovable = m_bRemovable;
	m_regDriveMaskRemote = m_bNetwork;
	m_regDriveMaskFixed = m_bFixed;
	m_regDriveMaskCDROM = m_bCDROM;
}

BOOL CSetOverlayPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_cDriveGroup.SetIcon(IDI_DRIVES);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_CHANGEDDIRS, IDS_SETTINGS_CHANGEDDIRS_TT);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetOverlayPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CSetOverlayPage::OnBnClickedChangeddirs()
{
	SetModified();
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

BOOL CSetOverlayPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}
