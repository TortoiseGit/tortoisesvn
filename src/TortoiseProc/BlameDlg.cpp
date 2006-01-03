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
#include "BlameDlg.h"
#include "Balloon.h"
#include "Registry.h"

// CBlameDlg dialog

IMPLEMENT_DYNAMIC(CBlameDlg, CStandAloneDialog)
CBlameDlg::CBlameDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CBlameDlg::IDD, pParent)
	, StartRev(1)
	, EndRev(0)
	, m_sStartRev(_T("1"))
	, m_bTextView(FALSE)
{
	m_regTextView = CRegDWORD(_T("Software\\TortoiseSVN\\TextBlame"), FALSE);
	m_bTextView = m_regTextView;
}

CBlameDlg::~CBlameDlg()
{
}

void CBlameDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_REVISON_START, m_sStartRev);
	DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
	DDX_Check(pDX, IDC_CHECK1, m_bTextView);
}


BEGIN_MESSAGE_MAP(CBlameDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_EN_CHANGE(IDC_REVISION_END, &CBlameDlg::OnEnChangeRevisionEnd)
END_MESSAGE_MAP()



BOOL CBlameDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	m_bTextView = m_regTextView;
	// set head revision as default revision
	if (EndRev.IsHead())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
	{
		m_sEndRev = EndRev.ToString();
		UpdateData(FALSE);
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	}
	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CBlameDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	m_regTextView = m_bTextView;
	StartRev = SVNRev(m_sStartRev);
	EndRev = SVNRev(m_sEndRev);
	if (!StartRev.IsValid())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_REVISON_START), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}
	EndRev = SVNRev(m_sEndRev);
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		EndRev = SVNRev(_T("HEAD"));
	}
	if (!EndRev.IsValid())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_REVISION_END), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}

	UpdateData(FALSE);

	CStandAloneDialog::OnOK();
}

void CBlameDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CBlameDlg::OnEnChangeRevisionEnd()
{
	UpdateData();
	if (m_sEndRev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}
