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
#include "UpdateDlg.h"
#include "Balloon.h"


// CUpdateDlg dialog

IMPLEMENT_DYNAMIC(CUpdateDlg, CDialog)
CUpdateDlg::CUpdateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUpdateDlg::IDD, pParent)
	, Revision(_T("HEAD"))
	, m_bNonRecursive(FALSE)
	, m_bNoExternals(FALSE)
{
}

CUpdateDlg::~CUpdateDlg()
{
}

void CUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVNUM, m_sRevision);
	DDX_Check(pDX, IDC_NON_RECURSIVE, m_bNonRecursive);
	DDX_Check(pDX, IDC_NOEXTERNALS, m_bNoExternals);
}


BEGIN_MESSAGE_MAP(CUpdateDlg, CDialog)
	ON_BN_CLICKED(IDC_NEWEST, OnBnClickedNewest)
	ON_BN_CLICKED(IDC_REVISION_N, OnBnClickedRevisionN)
END_MESSAGE_MAP()

BOOL CUpdateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Since this dialog is called to update to a specific revision, we should
	// enable and set focus to the edit control so that the user can enter the
	// revision number without clicking or tabbing around first.
	CheckRadioButton(IDC_NEWEST, IDC_REVISION_N, IDC_REVISION_N);
	GetDlgItem(IDC_REVNUM)->EnableWindow(TRUE);
	GetDlgItem(IDC_REVNUM)->SetFocus();
	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return FALSE;  // return TRUE unless you set the focus to a control
	               // EXCEPTION: OCX Property Pages should return FALSE
}

void CUpdateDlg::OnBnClickedNewest()
{
	GetDlgItem(IDC_REVNUM)->EnableWindow(FALSE);
}

void CUpdateDlg::OnBnClickedRevisionN()
{
	GetDlgItem(IDC_REVNUM)->EnableWindow();
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

	CDialog::OnOK();
}
