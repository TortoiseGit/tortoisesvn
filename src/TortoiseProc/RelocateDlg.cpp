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
#include "RelocateDlg.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"


// CRelocateDlg dialog

IMPLEMENT_DYNAMIC(CRelocateDlg, CStandAloneDialog)
CRelocateDlg::CRelocateDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CRelocateDlg::IDD, pParent)
	, m_sToUrl(_T(""))
	, m_sFromUrl(_T(""))
{
}

CRelocateDlg::~CRelocateDlg()
{
}

void CRelocateDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOURL, m_URLCombo);
}


BEGIN_MESSAGE_MAP(CRelocateDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()


// CRelocateDlg message handlers

BOOL CRelocateDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS"), _T("url"));

	GetDlgItem(IDC_FROMURL)->SetWindowText(m_sFromUrl);
	m_URLCombo.SetWindowText(m_sFromUrl);
	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRelocateDlg::OnBnClickedBrowse()
{
	m_URLCombo.GetWindowText(m_sToUrl);
	if (m_sToUrl.Left(7) == _T("file://"))
	{
		CString strFile(m_sToUrl);
		SVN::UrlToPath(strFile);

		SVN svn;
		if (svn.IsRepository(strFile))
		{
			// browse repository - show repository browser
			CRepositoryBrowser browser(m_sToUrl, this);
			if (browser.DoModal() == IDOK)
			{
				m_URLCombo.SetCurSel(-1);
				m_URLCombo.SetWindowText(browser.GetPath());
			}
		}
		else
		{
			// browse local directories
			CBrowseFolder folderBrowser;
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (folderBrowser.Show(GetSafeHwnd(), m_sToUrl) == CBrowseFolder::OK)
			{
				SVN::PathToUrl(m_sToUrl);
				m_URLCombo.SetCurSel(-1);
				m_URLCombo.SetWindowText(m_sToUrl);
			}
		}
	}
	else if ((m_sToUrl.Left(7) == _T("http://")
		||(m_sToUrl.Left(8) == _T("https://"))
		||(m_sToUrl.Left(6) == _T("svn://"))
		||(m_sToUrl.Left(4) == _T("svn+"))) && m_sToUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(m_sToUrl, this);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetCurSel(-1);
			m_URLCombo.SetWindowText(browser.GetPath());
		}
	}
	else
	{
		// browse local directories
		CBrowseFolder folderBrowser;
		folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		if (folderBrowser.Show(GetSafeHwnd(), m_sToUrl) == CBrowseFolder::OK)
		{
			SVN::PathToUrl(m_sToUrl);
			m_URLCombo.SetCurSel(-1);
			m_URLCombo.SetWindowText(m_sToUrl);
		}
	}
}

void CRelocateDlg::OnOK()
{
	UpdateData(TRUE);
	m_URLCombo.SaveHistory();
	m_sToUrl = m_URLCombo.GetString();
	UpdateData(FALSE);

	CStandAloneDialog::OnOK();
}

void CRelocateDlg::OnBnClickedHelp()
{
	OnHelp();
}
