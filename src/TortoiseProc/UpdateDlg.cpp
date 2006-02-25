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
#include "UpdateDlg.h"
#include "Balloon.h"
#include "registry.h"


// CUpdateDlg dialog

IMPLEMENT_DYNAMIC(CUpdateDlg, CStandAloneDialog)
CUpdateDlg::CUpdateDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CUpdateDlg::IDD, pParent)
	, Revision(_T("HEAD"))
	, m_bNonRecursive(FALSE)
	, m_bNoExternals(FALSE)
	, m_pLogDlg(NULL)
{
}

CUpdateDlg::~CUpdateDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVNUM, m_sRevision);
	DDX_Check(pDX, IDC_NON_RECURSIVE, m_bNonRecursive);
	DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
}


BEGIN_MESSAGE_MAP(CUpdateDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_LOG, OnBnClickedShowLog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_EN_CHANGE(IDC_REVNUM, &CUpdateDlg::OnEnChangeRevnum)
END_MESSAGE_MAP()

BOOL CUpdateDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	GetDlgItem(IDC_REVNUM)->SetFocus();
	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return FALSE;  // return TRUE unless you set the focus to a control
	               // EXCEPTION: OCX Property Pages should return FALSE
}

void CUpdateDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	Revision = SVNRev(m_sRevision);
	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_NEWEST, IDC_REVISION_N) == IDC_NEWEST)
	{
		Revision = SVNRev(_T("HEAD"));
	}
	if (!Revision.IsValid())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_REVNUM), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}

	UpdateData(FALSE);

	CStandAloneDialog::OnOK();
}

void CUpdateDlg::OnBnClickedShowLog()
{
	UpdateData(TRUE);
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	if (!m_wcPath.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->m_wParam = 0;
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->SetParams(m_wcPath, SVNRev::REV_HEAD, 1, limit, TRUE);
		m_pLogDlg->ContinuousSelection(true);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CUpdateDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	GetDlgItem(IDC_REVNUM)->SetWindowText(temp);
	CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
	return 0;
}

void CUpdateDlg::OnEnChangeRevnum()
{
	UpdateData();
	if (m_sRevision.IsEmpty())
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_NEWEST);
	else
		CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
}
