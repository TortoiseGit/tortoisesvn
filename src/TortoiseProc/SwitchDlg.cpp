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
#include "SwitchDlg.h"
#include "RepositoryBrowser.h"


// CSwitchDlg dialog

IMPLEMENT_DYNAMIC(CSwitchDlg, CDialog)
CSwitchDlg::CSwitchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSwitchDlg::IDD, pParent)
	, m_URL(_T(""))
	, m_rev(_T("-1"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CSwitchDlg::~CSwitchDlg()
{
}

void CSwitchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISION_NUM, m_rev);
	DDV_MaxChars(pDX, m_rev, 10);
	DDX_Control(pDX, IDC_REVISION_NUM, m_revctrl);
}


BEGIN_MESSAGE_MAP(CSwitchDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_UPDATE(IDC_REV, OnEnUpdateRev)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_REVISION_HEAD, OnBnClickedNewest)
	ON_BN_CLICKED(IDC_REVISION_N, OnBnClickedRevisionN)
END_MESSAGE_MAP()


// CSwitchDlg message handlers

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSwitchDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSwitchDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CSwitchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SVNStatus status;
	status.GetStatus(m_path);
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("repoURLS"), _T("url"));
	if (status.status->entry != NULL)
	{
		m_path = status.status->entry->url;
		m_URLCombo.AddString(m_path, 0);
		m_URLCombo.SelectString(-1, m_path);
		m_URL = m_path;
	}

	// set head revision as default revision
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);

	m_revctrl.SetWindowText(_T(""));
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSwitchDlg::OnEnUpdateRev()
{
	UpdateData(TRUE);
	if (m_rev.Compare(_T("-"))==0)
		return;
	if (m_rev.IsEmpty())
		return;
	long revnum = _tstol((LPCTSTR(m_rev)));
	if (revnum < -1)
		revnum = -1;
	m_rev.Format(_T("%d"),revnum);
	UpdateData(FALSE);
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
			CRepositoryBrowser browser(strUrl, this);
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
		||(strUrl.Left(10) == _T("svn+ssl://"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, this);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetWindowText(browser.m_strUrl);
		}
	}
}

void CSwitchDlg::OnBnClickedNewest()
{
	m_revctrl.EnableWindow(FALSE);
}

void CSwitchDlg::OnBnClickedRevisionN()
{
	m_revctrl.EnableWindow();
}

void CSwitchDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		m_rev = _T("-1");
	}

	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	UpdateData(FALSE);
	CDialog::OnOK();
}
