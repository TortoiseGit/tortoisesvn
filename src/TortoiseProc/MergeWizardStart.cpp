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


IMPLEMENT_DYNAMIC(CMergeWizardStart, CPropertyPage)

CMergeWizardStart::CMergeWizardStart()
	: CPropertyPage(CMergeWizardStart::IDD)
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
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMergeWizardStart, CPropertyPage)
END_MESSAGE_MAP()


LRESULT CMergeWizardStart::OnWizardNext()
{
	int nButton = GetCheckedRadioButton(IDC_MERGE_REVRANGE, IDC_MERGE_TREE);

	if (nButton == IDC_MERGE_TREE)
	{
		((CMergeWizard*)GetParent())->bRevRangeMerge = false;
		return IDD_MERGEWIZARD_TREE;
	}
	((CMergeWizard*)GetParent())->bRevRangeMerge = true;
	return IDD_MERGEWIZARD_REVRANGE;
}

BOOL CMergeWizardStart::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CString sLabel;
	sLabel.LoadString(IDS_MERGEWIZARD_REVRANGELABEL);
	SetDlgItemText(IDC_MERGERANGELABEL, sLabel);
	sLabel.LoadString(IDS_MERGEWIZARD_TREELABEL);
	SetDlgItemText(IDC_TREELABEL, sLabel);

	CheckRadioButton(IDC_MERGE_REVRANGE, IDC_MERGE_TREE, IDC_MERGE_REVRANGE);

	return TRUE;
}

BOOL CMergeWizardStart::OnSetActive()
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();   
	psheet->SetWizardButtons(PSWIZB_NEXT);
	psheet->GetDlgItem(ID_WIZBACK)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_BACK)));
	psheet->GetDlgItem(ID_WIZNEXT)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_NEXT)));
	psheet->GetDlgItem(IDHELP)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_HELP)));
	psheet->GetDlgItem(IDCANCEL)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_CANCEL)));

	return CPropertyPage::OnSetActive();
}
