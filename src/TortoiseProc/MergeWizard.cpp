// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "MergeWizard.h"


// CMergeWizard

IMPLEMENT_DYNAMIC(CMergeWizard, CPropertySheet)

CMergeWizard::CMergeWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
	, bReverseMerge(FALSE)
	, bRevRangeMerge(TRUE)
	, m_bIgnoreAncestry(FALSE)
	, m_bIgnoreEOL(FALSE)
	, m_depth(svn_depth_unknown)
	, bRecordOnly(FALSE)
	, m_FirstPageActivation(true)
{
	SetWizardMode();
	AddPage(&page1);
	AddPage(&tree);
	AddPage(&revrange);
	AddPage(&options);

	m_psh.dwFlags |= PSH_WIZARD97|PSP_HASHELP|PSH_HEADER;
	m_psh.pszbmHeader = MAKEINTRESOURCE(IDB_MERGEWIZARDTITLE);

	m_psh.hInstance = AfxGetResourceHandle();
}


CMergeWizard::~CMergeWizard()
{
}


BEGIN_MESSAGE_MAP(CMergeWizard, CPropertySheet)
END_MESSAGE_MAP()


// CMergeWizard message handlers

BOOL CMergeWizard::OnInitDialog()
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	SVN svn;
	url = svn.GetURLFromPath(wcPath);
	sUUID = svn.GetUUIDFromPath(wcPath);

	return bResult;
}

// Mode numbers coincide with wizard page indexes.

#define MODE_TREE 1
#define MODE_REVRANGE 2

bool CMergeWizard::AutoSetMode()
{
	if (!m_FirstPageActivation)
		return false;
	m_FirstPageActivation = false;
	DWORD nMergeWizardMode =
		(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\MergeWizardMode"), 0);
	if ((nMergeWizardMode != MODE_TREE) && (nMergeWizardMode != MODE_REVRANGE))
		return false;
	bRevRangeMerge = nMergeWizardMode == MODE_REVRANGE;
	SendMessage(PSM_SETCURSEL, nMergeWizardMode);
	return true;
}

void CMergeWizard::SaveMode()
{
	CRegDWORD regMergeWizardMode(_T("Software\\TortoiseSVN\\MergeWizardMode"), 0);
	if (DWORD(regMergeWizardMode))
	{
		regMergeWizardMode = bRevRangeMerge ? MODE_REVRANGE : MODE_TREE;
	}
}
