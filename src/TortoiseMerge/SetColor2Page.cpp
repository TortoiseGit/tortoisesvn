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
#include "SetColor2Page.h"
#include ".\setcolor2page.h"


// CSetColor2Page dialog

IMPLEMENT_DYNAMIC(CSetColor2Page, CPropertyPage)
CSetColor2Page::CSetColor2Page()
	: CPropertyPage(CSetColor2Page::IDD)
{
	m_bInit = FALSE;
}

CSetColor2Page::~CSetColor2Page()
{
}

void CSetColor2Page::SaveData()
{
	if (m_bInit)
	{
		CDiffData diffdata;
		COLORREF cBk;
		COLORREF cFg;

		cBk = m_cBkEmpty.GetColor(TRUE);
		cFg = m_cFgEmpty.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_EMPTY, cBk, cFg);
		cBk = m_cBkConflicted.GetColor(TRUE);
		cFg = m_cFgConflicted.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_CONFLICTED, cBk, cFg);
		cBk = m_cBkConflictedAdded.GetColor(TRUE);
		cFg = m_cFgConflictedAdded.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_CONFLICTADDED, cBk, cFg);
		cBk = m_cBkConflictedEmpty.GetColor(TRUE);
		cFg = m_cFgConflictedEmpty.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_CONFLICTEMPTY, cBk, cFg);
		cBk = m_cBkIdentical.GetColor(TRUE);
		cFg = m_cFgIdentical.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_IDENTICAL, cBk, cFg);
		cBk = m_cBkIdenticalAdded.GetColor(TRUE);
		cFg = m_cFgIdenticalAdded.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_IDENTICALADDED, cBk, cFg);
		cBk = m_cBkIdenticalRemoved.GetColor(TRUE);
		cFg = m_cFgIdenticalRemoved.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_IDENTICALREMOVED, cBk, cFg);
		cBk = m_cBkTheirsAdded.GetColor(TRUE);
		cFg = m_cFgTheirsAdded.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_THEIRSADDED, cBk, cFg);
		cBk = m_cBkTheirsRemoved.GetColor(TRUE);
		cFg = m_cFgTheirsRemoved.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_THEIRSREMOVED, cBk, cFg);
		cBk = m_cBkYoursAdded.GetColor(TRUE);
		cFg = m_cFgYoursAdded.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_YOURSADDED, cBk, cFg);
		cBk = m_cBkYoursRemoved.GetColor(TRUE);
		cFg = m_cFgYoursRemoved.GetColor(TRUE);
		diffdata.SetColors(CDiffData::DIFFSTATE_YOURSREMOVED, cBk, cFg);
	}
}

void CSetColor2Page::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BKEMPTY, m_cBkEmpty);
	DDX_Control(pDX, IDC_BKCONFLICTED, m_cBkConflicted);
	DDX_Control(pDX, IDC_BKCONFLICTEDADDED, m_cBkConflictedAdded);
	DDX_Control(pDX, IDC_BKCONFLICTEDEMPTY, m_cBkConflictedEmpty);
	DDX_Control(pDX, IDC_BKIDENTICAL, m_cBkIdentical);
	DDX_Control(pDX, IDC_BKIDENTICALADDED, m_cBkIdenticalAdded);
	DDX_Control(pDX, IDC_BKIDENTICALREMOVED, m_cBkIdenticalRemoved);
	DDX_Control(pDX, IDC_BKTHEIRSADDED, m_cBkTheirsAdded);
	DDX_Control(pDX, IDC_BKTHEIRSREMOVED, m_cBkTheirsRemoved);
	DDX_Control(pDX, IDC_BKYOURSADDED, m_cBkYoursAdded);
	DDX_Control(pDX, IDC_BKYOURSREMOVED, m_cBkYoursRemoved);
	DDX_Control(pDX, IDC_FGEMPTY, m_cFgEmpty);
	DDX_Control(pDX, IDC_FGCONFLICTED, m_cFgConflicted);
	DDX_Control(pDX, IDC_FGCONFLICTEDADDED, m_cFgConflictedAdded);
	DDX_Control(pDX, IDC_FGCONFLICTEDEMPTY, m_cFgConflictedEmpty);
	DDX_Control(pDX, IDC_FGIDENTICAL, m_cFgIdentical);
	DDX_Control(pDX, IDC_FGIDENTICALADDED, m_cFgIdenticalAdded);
	DDX_Control(pDX, IDC_FGIDENTICALREMOVED, m_cFgIdenticalRemoved);
	DDX_Control(pDX, IDC_FGTHEIRSADDED, m_cFgTheirsAdded);
	DDX_Control(pDX, IDC_FGTHEIRSREMOVED, m_cFgTheirsRemoved);
	DDX_Control(pDX, IDC_FGYOURSADDED, m_cFgYoursAdded);
	DDX_Control(pDX, IDC_FGYOURSREMOVED, m_cFgYoursRemoved);
}


BEGIN_MESSAGE_MAP(CSetColor2Page, CPropertyPage)
	ON_MESSAGE(CPN_SELENDOK, OnSelEndOK)
END_MESSAGE_MAP()


// CSetColor2Page message handlers

LONG CSetColor2Page::OnSelEndOK(UINT lParam, LONG wParam)
{
	SetModified();
	return 0;
}

BOOL CSetColor2Page::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CDiffData diffdata;
	COLORREF cBk;
	COLORREF cFg;

	diffdata.GetColors(CDiffData::DIFFSTATE_EMPTY, cBk, cFg);
	m_cBkEmpty.SetDefaultColor(DIFFSTATE_EMPTY_DEFAULT_BG);
	m_cBkEmpty.SetColor(cBk);
	m_cFgEmpty.SetDefaultColor(DIFFSTATE_EMPTY_DEFAULT_FG);
	m_cFgEmpty.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_CONFLICTED, cBk, cFg);
	m_cBkConflicted.SetDefaultColor(DIFFSTATE_CONFLICTED_DEFAULT_BG);
	m_cBkConflicted.SetColor(cBk);
	m_cFgConflicted.SetDefaultColor(DIFFSTATE_CONFLICTED_DEFAULT_FG);
	m_cFgConflicted.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_CONFLICTADDED, cBk, cFg);
	m_cBkConflictedAdded.SetDefaultColor(DIFFSTATE_CONFLICTADDED_DEFAULT_BG);
	m_cBkConflictedAdded.SetColor(cBk);
	m_cFgConflictedAdded.SetDefaultColor(DIFFSTATE_CONFLICTADDED_DEFAULT_FG);
	m_cFgConflictedAdded.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_CONFLICTEMPTY, cBk, cFg);
	m_cBkConflictedEmpty.SetDefaultColor(DIFFSTATE_CONFLICTEMPTY_DEFAULT_BG);
	m_cBkConflictedEmpty.SetColor(cBk);
	m_cFgConflictedEmpty.SetDefaultColor(DIFFSTATE_CONFLICTEMPTY_DEFAULT_FG);
	m_cFgConflictedEmpty.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_IDENTICAL, cBk, cFg);
	m_cBkIdentical.SetDefaultColor(DIFFSTATE_IDENTICAL_DEFAULT_BG);
	m_cBkIdentical.SetColor(cBk);
	m_cFgIdentical.SetDefaultColor(DIFFSTATE_IDENTICAL_DEFAULT_FG);
	m_cFgIdentical.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_IDENTICALREMOVED, cBk, cFg);
	m_cBkIdenticalRemoved.SetDefaultColor(DIFFSTATE_IDENTICALREMOVED_DEFAULT_BG);
	m_cBkIdenticalRemoved.SetColor(cBk);
	m_cFgIdenticalRemoved.SetDefaultColor(DIFFSTATE_IDENTICALREMOVED_DEFAULT_FG);
	m_cFgIdenticalRemoved.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_IDENTICALADDED, cBk, cFg);
	m_cBkIdenticalAdded.SetDefaultColor(DIFFSTATE_IDENTICALADDED_DEFAULT_BG);
	m_cBkIdenticalAdded.SetColor(cBk);
	m_cFgIdenticalAdded.SetDefaultColor(DIFFSTATE_IDENTICALADDED_DEFAULT_FG);
	m_cFgIdenticalAdded.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_THEIRSREMOVED, cBk, cFg);
	m_cBkTheirsRemoved.SetDefaultColor(DIFFSTATE_THEIRSREMOVED_DEFAULT_BG);
	m_cBkTheirsRemoved.SetColor(cBk);
	m_cFgTheirsRemoved.SetDefaultColor(DIFFSTATE_THEIRSREMOVED_DEFAULT_FG);
	m_cFgTheirsRemoved.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_THEIRSADDED, cBk, cFg);
	m_cBkTheirsAdded.SetDefaultColor(DIFFSTATE_THEIRSADDED_DEFAULT_BG);
	m_cBkTheirsAdded.SetColor(cBk);
	m_cFgTheirsAdded.SetDefaultColor(DIFFSTATE_THEIRSADDED_DEFAULT_FG);
	m_cFgTheirsAdded.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_YOURSREMOVED, cBk, cFg);
	m_cBkYoursRemoved.SetDefaultColor(DIFFSTATE_YOURSREMOVED_DEFAULT_BG);
	m_cBkYoursRemoved.SetColor(cBk);
	m_cFgYoursRemoved.SetDefaultColor(DIFFSTATE_YOURSREMOVED_DEFAULT_FG);
	m_cFgYoursRemoved.SetColor(cFg);

	diffdata.GetColors(CDiffData::DIFFSTATE_YOURSADDED, cBk, cFg);
	m_cBkYoursAdded.SetDefaultColor(DIFFSTATE_YOURSADDED_DEFAULT_BG);
	m_cBkYoursAdded.SetColor(cBk);
	m_cFgYoursAdded.SetDefaultColor(DIFFSTATE_YOURSADDED_DEFAULT_FG);
	m_cFgYoursAdded.SetColor(cFg);

	m_bInit = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetColor2Page::OnApply()
{
	UpdateData();
	SaveData();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

