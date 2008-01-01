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
#include "MergeWizardStart.h"


IMPLEMENT_DYNAMIC(CMergeWizardStart, CMergeWizardBasePage)

CMergeWizardStart::CMergeWizardStart()
	: CMergeWizardBasePage(CMergeWizardStart::IDD)
{
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_STARTTITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_STARTSUBTITLE);
}

CMergeWizardStart::~CMergeWizardStart()
{
}

void CMergeWizardStart::DoDataExchange(CDataExchange* pDX)
{
	CMergeWizardBasePage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMergeWizardStart, CMergeWizardBasePage)
END_MESSAGE_MAP()


LRESULT CMergeWizardStart::OnWizardNext()
{
	int nButton = GetCheckedRadioButton(IDC_MERGE_REVRANGE, IDC_MERGE_TREE);

	CMergeWizard* wiz = (CMergeWizard*)GetParent();
	wiz->bRevRangeMerge = nButton == IDC_MERGE_REVRANGE;
	wiz->SaveMode();

	return wiz->GetSecondPage();
}

BOOL CMergeWizardStart::OnInitDialog()
{
	CMergeWizardBasePage::OnInitDialog();

	CString sLabel;
	sLabel.LoadString(IDS_MERGEWIZARD_REVRANGELABEL);
	SetDlgItemText(IDC_MERGERANGELABEL, sLabel);
	sLabel.LoadString(IDS_MERGEWIZARD_TREELABEL);
	SetDlgItemText(IDC_TREELABEL, sLabel);

	AdjustControlSize(IDC_MERGE_REVRANGE);
	AdjustControlSize(IDC_MERGE_TREE);

	return TRUE;
}

BOOL CMergeWizardStart::OnSetActive()
{
	CMergeWizard* wiz = (CMergeWizard*)GetParent();

	if (wiz->AutoSetMode())
		return FALSE;

	wiz->SetWizardButtons(PSWIZB_NEXT);
	SetButtonTexts();

	CheckRadioButton(
		IDC_MERGE_REVRANGE, IDC_MERGE_TREE,
		wiz->bRevRangeMerge ? IDC_MERGE_REVRANGE : IDC_MERGE_TREE);

	return CMergeWizardBasePage::OnSetActive();
}
