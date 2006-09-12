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
#include "SwitchDlg.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "Balloon.h"
#include "TSVNPath.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CSwitchDlg, CResizableStandAloneDialog)
CSwitchDlg::CSwitchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSwitchDlg::IDD, pParent)
	, m_URL(_T(""))
	, Revision(_T("HEAD"))
{
}

CSwitchDlg::~CSwitchDlg()
{
}

void CSwitchDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISION_NUM, m_rev);
}


BEGIN_MESSAGE_MAP(CSwitchDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_EN_CHANGE(IDC_REVISION_NUM, &CSwitchDlg::OnEnChangeRevisionNum)
END_MESSAGE_MAP()

void CSwitchDlg::SetDialogTitle(const CString& sTitle)
{
	m_sTitle = sTitle;
}

void CSwitchDlg::SetUrlLabel(const CString& sLabel)
{
	m_sLabel = sLabel;
}

BOOL CSwitchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CTSVNPath svnPath(m_path);
	m_bFolder = svnPath.IsDirectory();
	SVN svn;
	CString url = svn.GetURLFromPath(svnPath);
	CString sUUID = svn.GetUUIDFromPath(svnPath);
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sUUID, _T("url"));
	if (!url.IsEmpty())
	{
		m_path = url;
		m_URLCombo.AddString(CTSVNPath(url).GetUIPathString(), 0);
		m_URLCombo.SelectString(-1, CTSVNPath(url).GetUIPathString());
		m_URL = m_path;
	}

	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);
	SetWindowText(m_sTitle);
	if (m_sLabel.IsEmpty())
		GetDlgItem(IDC_URLLABEL)->GetWindowText(m_sLabel);
	GetDlgItem(IDC_URLLABEL)->SetWindowText(m_sLabel);

	// set head revision as default revision
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);

	AddAnchor(IDC_URLLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_REVGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_REVISION_HEAD, TOP_LEFT);
	AddAnchor(IDC_REVISION_N, TOP_LEFT);
	AddAnchor(IDC_REVISION_NUM, TOP_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;
}

void CSwitchDlg::OnBnClickedBrowse()
{
	CAppUtils::BrowseRepository(m_URLCombo, this, !m_bFolder);
}

void CSwitchDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		m_rev = _T("HEAD");
	}
	Revision = SVNRev(m_rev);
	if (!Revision.IsValid())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_REVISION_NUM), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}

	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	UpdateData(FALSE);
	CResizableStandAloneDialog::OnOK();
}

void CSwitchDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CSwitchDlg::OnEnChangeRevisionNum()
{
	UpdateData();
	if (m_rev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}
