// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004 - Stefan Kueng

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
#include "TortoiseMerge.h"
#include "SetColorPage.h"
#include ".\setcolorpage.h"


// CSetColorPage dialog

IMPLEMENT_DYNAMIC(CSetColorPage, CPropertyPage)
CSetColorPage::CSetColorPage()
	: CPropertyPage(CSetColorPage::IDD)
{
}

CSetColorPage::~CSetColorPage()
{
	m_bInit = FALSE;
}

void CSetColorPage::SaveData()
{
	if (m_bInit)
	{
		CDiffData diffdata;
		COLORREF cBk;
		COLORREF cFg;

		cBk = m_cBkUnknown.GetColor(TRUE);
		cFg = m_cFgUnknown.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_UNKNOWN, cBk, cFg);
		cBk = m_cBkNormal.GetColor(TRUE);
		cFg = m_cFgNormal.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_NORMAL, cBk, cFg);
		cBk = m_cBkRemoved.GetColor(TRUE);
		cFg = m_cFgRemoved.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_REMOVED, cBk, cFg);
		cBk = m_cBkWhitespaceRemoved.GetColor(TRUE);
		cFg = m_cFgWhitespaceRemoved.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_REMOVEDWHITESPACE, cBk, cFg);
		cBk = m_cBkAdded.GetColor(TRUE);
		cFg = m_cFgAdded.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_ADDED, cBk, cFg);
		cBk = m_cBkWhitespaceAdded.GetColor(TRUE);
		cFg = m_cFgWhitespaceAdded.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_ADDEDWHITESPACE, cBk, cFg);
	}
}

void CSetColorPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BKUNKNOWN, m_cBkUnknown);
	DDX_Control(pDX, IDC_FGUNKNOWN, m_cFgUnknown);
	DDX_Control(pDX, IDC_BKNORMAL, m_cBkNormal);
	DDX_Control(pDX, IDC_FGNORMAL, m_cFgNormal);
	DDX_Control(pDX, IDC_BKREMOVED, m_cBkRemoved);
	DDX_Control(pDX, IDC_BKWHITESPACEREMOVED, m_cBkWhitespaceRemoved);
	DDX_Control(pDX, IDC_BKADDED, m_cBkAdded);
	DDX_Control(pDX, IDC_BKWHITESPACEADDED, m_cBkWhitespaceAdded);
	DDX_Control(pDX, IDC_FGREMOVED, m_cFgRemoved);
	DDX_Control(pDX, IDC_FGWHITESPACEREMOVED, m_cFgWhitespaceRemoved);
	DDX_Control(pDX, IDC_FGADDED, m_cFgAdded);
	DDX_Control(pDX, IDC_FGWHITESPACEADDED, m_cFgWhitespaceAdded);
}


BEGIN_MESSAGE_MAP(CSetColorPage, CPropertyPage)
	ON_MESSAGE(CPN_SELENDOK, OnSelEndOK)
END_MESSAGE_MAP()


// CSetColorPage message handlers

LONG CSetColorPage::OnSelEndOK(UINT lParam, LONG wParam)
{
	SetModified();
	return 0;
}


BOOL CSetColorPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CDiffData diffdata;
	COLORREF cBk;
	COLORREF cFg;

	diffdata.GetColors(CDiffData::DIFFSTATE_UNKNOWN, cBk, cFg);
	m_cBkUnknown.SetDefaultColor(DIFFSTATE_UNKNOWN_DEFAULT_BG);
	m_cBkUnknown.SetColor(cBk);
	m_cFgUnknown.SetDefaultColor(DIFFSTATE_UNKNOWN_DEFAULT_FG);
	m_cFgUnknown.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_NORMAL, cBk, cFg);
	m_cBkNormal.SetDefaultColor(DIFFSTATE_NORMAL_DEFAULT_BG);
	m_cBkNormal.SetColor(cBk);
	m_cFgNormal.SetDefaultColor(DIFFSTATE_NORMAL_DEFAULT_FG);
	m_cFgNormal.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_REMOVED, cBk, cFg);
	m_cBkRemoved.SetDefaultColor(DIFFSTATE_REMOVED_DEFAULT_BG);
	m_cBkRemoved.SetColor(cBk);
	m_cFgRemoved.SetDefaultColor(DIFFSTATE_REMOVED_DEFAULT_FG);
	m_cFgRemoved.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_REMOVEDWHITESPACE, cBk, cFg);
	m_cBkWhitespaceRemoved.SetDefaultColor(DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_BG);
	m_cBkWhitespaceRemoved.SetColor(cBk);
	m_cFgWhitespaceRemoved.SetDefaultColor(DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_FG);
	m_cFgWhitespaceRemoved.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_ADDED, cBk, cFg);
	m_cBkAdded.SetDefaultColor(DIFFSTATE_ADDED_DEFAULT_BG);
	m_cBkAdded.SetColor(cBk);
	m_cFgAdded.SetDefaultColor(DIFFSTATE_ADDED_DEFAULT_FG);
	m_cFgAdded.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_ADDEDWHITESPACE, cBk, cFg);
	m_cBkWhitespaceAdded.SetDefaultColor(DIFFSTATE_ADDEDWHITESPACE_DEFAULT_BG);
	m_cBkWhitespaceAdded.SetColor(cBk);
	m_cFgWhitespaceAdded.SetDefaultColor(DIFFSTATE_ADDEDWHITESPACE_DEFAULT_FG);
	m_cFgWhitespaceAdded.SetColor(cFg);


	m_bInit = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetColorPage::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

