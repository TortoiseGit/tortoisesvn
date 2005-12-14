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
#include "Settings.h"
#include ".\settings.h"



IMPLEMENT_DYNAMIC(CSettings, CTreePropSheet)
CSettings::CSettings(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CTreePropSheet(nIDCaption, pParentWnd, iSelectPage)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	AddPropPages();
}

CSettings::~CSettings()
{
	RemovePropPages();
}

void CSettings::AddPropPages()
{
	m_pMainPage = new CSetMainPage();
	m_pOverlayPage = new CSetOverlayPage();
	m_pOverlaysPage = new CSetOverlayIcons();
	m_pProxyPage = new CSetProxyPage();
	m_pProgsDiffPage = new CSettingsProgsDiff();
	m_pProgsMergePage = new CSettingsProgsMerge();
	m_pProgsUniDiffPage = new CSettingsProgsUniDiff();
	m_pLookAndFeelPage = new CSetLookAndFeelPage();
	m_pDialogsPage = new CSetDialogs();
	m_pColorsPage = new CSettingsColors();
	m_pMiscPage = new CSetMisc();

	SetPageIcon(m_pMainPage, m_pMainPage->GetIconID());
	SetPageIcon(m_pOverlayPage, m_pOverlayPage->GetIconID());
	SetPageIcon(m_pOverlaysPage, m_pOverlaysPage->GetIconID());
	SetPageIcon(m_pProxyPage, m_pProxyPage->GetIconID());
	SetPageIcon(m_pProgsDiffPage, m_pProgsDiffPage->GetIconID());
	SetPageIcon(m_pProgsMergePage, m_pProgsMergePage->GetIconID());
	SetPageIcon(m_pProgsUniDiffPage, m_pProgsUniDiffPage->GetIconID());
	SetPageIcon(m_pLookAndFeelPage, m_pLookAndFeelPage->GetIconID());
	SetPageIcon(m_pDialogsPage, m_pDialogsPage->GetIconID());
	SetPageIcon(m_pColorsPage, m_pColorsPage->GetIconID());
	SetPageIcon(m_pMiscPage, m_pMiscPage->GetIconID());

	AddPage(m_pMainPage);
	AddPage(m_pOverlayPage);
	AddPage(m_pOverlaysPage);
	AddPage(m_pProxyPage);
	AddPage(m_pProgsDiffPage);
	AddPage(m_pProgsMergePage);
	AddPage(m_pProgsUniDiffPage);
	AddPage(m_pLookAndFeelPage);
	AddPage(m_pDialogsPage);
	AddPage(m_pColorsPage);
	AddPage(m_pMiscPage);
}

void CSettings::RemovePropPages()
{
	delete m_pMainPage;
	delete m_pOverlayPage;
	delete m_pOverlaysPage;
	delete m_pProxyPage;
	delete m_pProgsDiffPage;
	delete m_pProgsMergePage;
	delete m_pProgsUniDiffPage;
	delete m_pLookAndFeelPage;
	delete m_pDialogsPage;
	delete m_pColorsPage;
	delete m_pMiscPage;
}

void CSettings::SaveData()
{
	m_pMainPage->SaveData();
	m_pOverlayPage->SaveData();
	m_pOverlaysPage->SaveData();
	m_pProxyPage->SaveData();
	m_pProgsDiffPage->SaveData();
	m_pProgsMergePage->SaveData();
	m_pProgsUniDiffPage->SaveData();
	m_pLookAndFeelPage->SaveData();
	m_pDialogsPage->SaveData();
	m_pColorsPage->SaveData();
	m_pMiscPage->SaveData();
}

BEGIN_MESSAGE_MAP(CSettings, CTreePropSheet)
	ON_WM_QUERYDRAGICON()
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CSettings message handlers

BOOL CSettings::OnInitDialog()
{
	BOOL bResult = CTreePropSheet::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return bResult;
}

void CSettings::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CTreePropSheet::OnPaint();
	}
}

HCURSOR CSettings::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
