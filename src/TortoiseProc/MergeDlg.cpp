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
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "MergeDlg.h"
#include "RepositoryBrowser.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include ".\mergedlg.h"



IMPLEMENT_DYNAMIC(CMergeDlg, CDialog)
CMergeDlg::CMergeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMergeDlg::IDD, pParent)
	, m_URL(_T(""))
	, StartRev(0)
	, EndRev(_T("HEAD"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pLogDlg = NULL;
}

CMergeDlg::~CMergeDlg()
{
	if (m_pLogDlg)
		delete [] m_pLogDlg;
}

void CMergeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISON_START, m_sStartRev);
	DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
}


BEGIN_MESSAGE_MAP(CMergeDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_REVISION_HEAD, OnBnClickedRevisionHead)
	ON_BN_CLICKED(IDC_REVISION_N, OnBnClickedRevisionN)
	ON_BN_CLICKED(IDC_FINDBRANCHSTART, OnBnClickedFindbranchstart)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
END_MESSAGE_MAP()


// CMergeDlg message handlers

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMergeDlg::OnPaint() 
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
HCURSOR CMergeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CMergeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_bFile = !PathIsDirectory(m_URL);
	SVN svn;
	CString url = svn.GetURLFromPath(m_URL);
	if (url.IsEmpty())
	{
		CString temp;
		temp.Format(IDS_ERR_NOURLOFFILE, m_URL);
		CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
		this->EndDialog(IDCANCEL);
		return TRUE;
	} // if ((status.status == NULL) || (status.status->entry == NULL))
	else
		m_URL = url;
	m_BranchURL = m_URL;

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("repoURLS"), _T("url"));
	m_URLCombo.SetWindowText(m_URL);

	// set head revision as default revision
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CMergeDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	StartRev = SVNRev(m_sStartRev);
	EndRev = SVNRev(m_sEndRev);
	if (!StartRev.IsValid())
	{
		CWnd* ctrl = GetDlgItem(IDC_REVISON_START);
		CRect rt;
		ctrl->GetWindowRect(rt);
		CPoint point = CPoint((rt.left+rt.right)/2, (rt.top+rt.bottom)/2);
		CBalloon::ShowBalloon(this, point, IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}

	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		EndRev = SVNRev(_T("HEAD"));
	}
	if (!EndRev.IsValid())
	{
		CWnd* ctrl = GetDlgItem(IDC_REVISION_END);
		CRect rt;
		ctrl->GetWindowRect(rt);
		CPoint point = CPoint((rt.left+rt.right)/2, (rt.top+rt.bottom)/2);
		CBalloon::ShowBalloon(this, point, IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}

	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();


	UpdateData(FALSE);

	CDialog::OnOK();
}

void CMergeDlg::OnBnClickedBrowse()
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
			CRepositoryBrowser browser(strUrl, this, m_bFile);
			if (browser.DoModal() == IDOK)
			{
				m_URLCombo.SetWindowText(browser.GetPath(true));
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
		CRepositoryBrowser browser(strUrl, this, m_bFile);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetWindowText(browser.GetPath(true));
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

void CMergeDlg::OnBnClickedRevisionHead()
{
	GetDlgItem(IDC_REVISION_END)->EnableWindow(FALSE);
}

void CMergeDlg::OnBnClickedRevisionN()
{
	GetDlgItem(IDC_REVISION_END)->EnableWindow(TRUE);
}

void CMergeDlg::OnBnClickedFindbranchstart()
{
	UpdateData(TRUE);
	if ((m_pLogDlg)&&(m_pLogDlg->IsWindowVisible()))
		return;
	CString url;
	m_URLCombo.GetWindowText(url);
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for the main trunk
	if (!url.IsEmpty())
	{
		delete [] m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		m_pLogDlg->SetParams(url, SVNRev::REV_HEAD, 1);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
		m_pLogDlg->m_pNotifyWindow = this;
	} // if (!url.IsEmpty()) 
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CMergeDlg::OnRevSelected(WPARAM wParam, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	GetDlgItem(IDC_REVISON_START)->SetWindowText(temp);
	return 0;
}
void CMergeDlg::OnBnClickedHelp()
{
	OnHelp();
}
