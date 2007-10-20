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
#include "MergeWizardOptions.h"
#include "SVNProgressDlg.h"


IMPLEMENT_DYNAMIC(CMergeWizardOptions, CPropertyPage)

CMergeWizardOptions::CMergeWizardOptions()
	: CPropertyPage(CMergeWizardOptions::IDD)
{
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_OPTIONSTITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_OPTIONSSUBTITLE);
}

CMergeWizardOptions::~CMergeWizardOptions()
{
}

void CMergeWizardOptions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
	DDX_Check(pDX, IDC_IGNOREANCESTRY, ((CMergeWizard*)GetParent())->m_bIgnoreAncestry);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
	DDX_Check(pDX, IDC_IGNOREEOL, ((CMergeWizard*)GetParent())->m_bIgnoreEOL);
	DDX_Check(pDX, IDC_RECORDONLY, ((CMergeWizard*)GetParent())->bRecordOnly);
}


BEGIN_MESSAGE_MAP(CMergeWizardOptions, CPropertyPage)
	ON_BN_CLICKED(IDC_DRYRUN, &CMergeWizardOptions::OnBnClickedDryrun)
END_MESSAGE_MAP()


BOOL CMergeWizardOptions::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_WORKING)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
	switch (((CMergeWizard*)GetParent())->m_depth)
	{
	case svn_depth_unknown:
		m_depthCombo.SetCurSel(0);
		break;
	case svn_depth_infinity:
		m_depthCombo.SetCurSel(1);
		break;
	case svn_depth_immediates:
		m_depthCombo.SetCurSel(2);
		break;
	case svn_depth_files:
		m_depthCombo.SetCurSel(3);
		break;
	case svn_depth_empty:
		m_depthCombo.SetCurSel(4);
		break;
	default:
		m_depthCombo.SetCurSel(0);
		break;
	}

	CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_COMPAREWHITESPACES);

	return TRUE;
}

LRESULT CMergeWizardOptions::OnWizardBack()
{
	if (((CMergeWizard*)GetParent())->bRevRangeMerge)
		return IDD_MERGEWIZARD_REVRANGE;
	return IDD_MERGEWIZARD_TREE;
}

BOOL CMergeWizardOptions::OnWizardFinish()
{
	switch (m_depthCombo.GetCurSel())
	{
	case 0:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_unknown;
		break;
	case 1:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_infinity;
		break;
	case 2:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_immediates;
		break;
	case 3:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_files;
		break;
	case 4:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_empty;
		break;
	default:
		((CMergeWizard*)GetParent())->m_depth = svn_depth_empty;
		break;
	}

	return CPropertyPage::OnWizardFinish();
}

BOOL CMergeWizardOptions::OnSetActive()
{
	CPropertySheet* psheet = (CPropertySheet*) GetParent();   
	psheet->SetWizardButtons(PSWIZB_BACK|PSWIZB_FINISH);
	psheet->GetDlgItem(ID_WIZFINISH)->SetWindowText(CString(MAKEINTRESOURCE(IDS_MERGE_MERGE)));
	return CPropertyPage::OnSetActive();
}

void CMergeWizardOptions::OnBnClickedDryrun()
{
	CMergeWizard * pWizard = ((CMergeWizard*)GetParent());
	CSVNProgressDlg progDlg;
	progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
	int options = ProgOptDryRun;
	options |= pWizard->m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
	progDlg.SetOptions(options);
	progDlg.SetPathList(CTSVNPathList(pWizard->wcPath));
	progDlg.SetUrl(pWizard->URL1);
	progDlg.SetSecondUrl(pWizard->URL2);
	if (pWizard->bRevRangeMerge)
	{
		if (pWizard->revList.GetCount())
		{
			pWizard->revList.Sort(!pWizard->bReverseMerge);
			progDlg.SetRevisionList(pWizard->revList);
		}
		else
		{
			progDlg.SetRevision(SVNRev());
			progDlg.SetRevisionEnd(SVNRev());
		}
	}
	else
	{
		progDlg.SetRevision(pWizard->startRev);
		progDlg.SetRevisionEnd(pWizard->endRev);
	}
	progDlg.SetDepth(pWizard->m_depth);
	progDlg.SetDiffOptions(SVN::GetOptionsString(pWizard->m_bIgnoreEOL, pWizard->m_IgnoreSpaces));
	progDlg.DoModal();
}
