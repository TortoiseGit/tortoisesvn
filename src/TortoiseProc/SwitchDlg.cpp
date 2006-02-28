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


// CSwitchDlg dialog

IMPLEMENT_DYNAMIC(CSwitchDlg, CStandAloneDialog)
CSwitchDlg::CSwitchDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CSwitchDlg::IDD, pParent)
	, m_URL(_T(""))
	, Revision(_T("HEAD"))
{
}

CSwitchDlg::~CSwitchDlg()
{
}

void CSwitchDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISION_NUM, m_rev);
}


BEGIN_MESSAGE_MAP(CSwitchDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_EN_CHANGE(IDC_REVISION_NUM, &CSwitchDlg::OnEnChangeRevisionNum)
END_MESSAGE_MAP()


// CSwitchDlg message handlers


BOOL CSwitchDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

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
		m_URLCombo.AddString(m_path, 0);
		m_URLCombo.SelectString(-1, m_path);
		m_URL = m_path;
	}

	// set head revision as default revision
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSwitchDlg::OnBnClickedBrowse()
{
	CString strUrl;
	m_URLCombo.GetWindowText(strUrl);
	if (strUrl.Left(7) == _T("file://"))
	{
		CString strFile(strUrl);
		SVN::UrlToPath(strFile);

		SVN svn;
		if (svn.IsRepository(strFile))
		{
			// browse repository - show repository browser
			CRepositoryBrowser browser(strUrl, this, !m_bFolder);
			if (browser.DoModal() == IDOK)
			{
				m_URLCombo.SetWindowText(browser.GetPath());
			}
		}
		else
		{
			// browse local directories
			CBrowseFolder folderBrowser;
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (folderBrowser.Show(GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
			{
				SVN::PathToUrl(strUrl);

				m_URLCombo.SetWindowText(strUrl);
			}
		}
	}
	else if ((strUrl.Left(7) == _T("http://")
		||(strUrl.Left(8) == _T("https://"))
		||(strUrl.Left(6) == _T("svn://"))
		||(strUrl.Left(4) == _T("svn+"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, this, !m_bFolder);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetWindowText(browser.GetPath());
		}
	}
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
	CStandAloneDialog::OnOK();
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
