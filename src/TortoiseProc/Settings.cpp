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


#include "stdafx.h"
#include "TortoiseProc.h"
#include "Settings.h"



IMPLEMENT_DYNAMIC(CSettings, CPropertySheet)
CSettings::CSettings(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CSettings::CSettings(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
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
	m_pProxyPage = new CSetProxyPage();
	m_pMenuPage = new CSetMenuPage();
	m_pProgsPage = new CSetProgsPage();

	AddPage(m_pMainPage);
	AddPage(m_pOverlayPage);
	AddPage(m_pMenuPage);
	AddPage(m_pProxyPage);
	AddPage(m_pProgsPage);
}

void CSettings::RemovePropPages()
{
	delete m_pMainPage;
	delete m_pOverlayPage;
	delete m_pProxyPage;
	delete m_pMenuPage;
	delete m_pProgsPage;
}

void CSettings::SaveData()
{
	m_pMainPage->SaveData();
	m_pOverlayPage->SaveData();
	m_pProxyPage->SaveData();
	m_pMenuPage->SaveData();
	m_pProgsPage->SaveData();
}

BEGIN_MESSAGE_MAP(CSettings, CPropertySheet)
END_MESSAGE_MAP()


// CSettings message handlers

BOOL CSettings::OnInitDialog()
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return bResult;
}
