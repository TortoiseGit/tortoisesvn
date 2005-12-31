// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "RevisionDlg.h"
#include "Balloon.h"
#include ".\revisiondlg.h"


// CRevisionDlg dialog

IMPLEMENT_DYNAMIC(CRevisionDlg, CDialog)
CRevisionDlg::CRevisionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRevisionDlg::IDD, pParent)
	, SVNRev(_T("HEAD"))
{
}

CRevisionDlg::~CRevisionDlg()
{
}

void CRevisionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVNUM, m_sRevision);
}


BEGIN_MESSAGE_MAP(CRevisionDlg, CDialog)
	ON_EN_CHANGE(IDC_REVNUM, OnEnChangeRevnum)
END_MESSAGE_MAP()

BOOL CRevisionDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (IsHead())
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
		CString sRev;
		if (IsDate())
			sRev = GetDateString();
		else
			sRev.Format(_T("%ld"), (LONG)(*this));
		GetDlgItem(IDC_REVNUM)->SetWindowText(sRev);
	}
	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	GetDlgItem(IDC_REVNUM)->SetFocus();
	return FALSE;
}

void CRevisionDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	SVNRev::Create(m_sRevision);
	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_NEWEST, IDC_REVISION_N) == IDC_NEWEST)
	{
		SVNRev::Create(_T("HEAD"));
		m_sRevision = _T("HEAD");
	}
	if (!IsValid())
	{
		CWnd* ctrl = GetDlgItem(IDC_REVNUM);
		CRect rt;
		ctrl->GetWindowRect(rt);
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_REVNUM), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}

	UpdateData(FALSE);

	CDialog::OnOK();
}

void CRevisionDlg::OnEnChangeRevnum()
{
	CString sText;
	GetDlgItem(IDC_REVNUM)->GetWindowText(sText);
	if (sText.IsEmpty())
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	}
	else
	{
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
	}
}
