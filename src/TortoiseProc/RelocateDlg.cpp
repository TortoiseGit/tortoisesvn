// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RelocateDlg.h"
#include "RepositoryBrowser.h"
#include "UnicodeUtils.h"
#include ".\relocatedlg.h"


// CRelocateDlg dialog

IMPLEMENT_DYNAMIC(CRelocateDlg, CDialog)
CRelocateDlg::CRelocateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRelocateDlg::IDD, pParent)
	, m_sToUrl(_T(""))
	, m_sFromUrl(_T(""))
{
}

CRelocateDlg::~CRelocateDlg()
{
}

void CRelocateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOURL, m_URLCombo);
}


BEGIN_MESSAGE_MAP(CRelocateDlg, CDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
END_MESSAGE_MAP()


// CRelocateDlg message handlers

BOOL CRelocateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("repoURLS"), _T("url"));

	CStringA urla = CUnicodeUtils::GetUTF8(m_sFromUrl);
	CUtils::Unescape(urla.GetBuffer());
	urla.ReleaseBuffer();
	CString url = CUnicodeUtils::GetUnicode(urla);
	GetDlgItem(IDC_FROMURL)->SetWindowText(url);
	m_URLCombo.SetWindowText(url);
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
				m_URLCombo.SetWindowText(browser.m_strUrl);
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
				m_URLCombo.SetWindowText(m_sToUrl);
			}
		}
	}
	else if ((m_sToUrl.Left(7) == _T("http://")
		||(m_sToUrl.Left(8) == _T("https://"))
		||(m_sToUrl.Left(6) == _T("svn://"))
		||(m_sToUrl.Left(10) == _T("svn+ssl://"))) && m_sToUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(m_sToUrl, this);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetWindowText(browser.m_strUrl);
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

	CDialog::OnOK();
}
