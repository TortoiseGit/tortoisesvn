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
#include "CheckoutDlg.h"
#include "RepositoryBrowser.h"
#include "Messagebox.h"
#include "Dbghelp.h"
#include "PathUtils.h"


// CCheckoutDlg dialog

IMPLEMENT_DYNAMIC(CCheckoutDlg, CDialog)
CCheckoutDlg::CCheckoutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCheckoutDlg::IDD, pParent)
	, m_lRevision(-1)
	, m_strCheckoutDirectory(_T(""))
	, IsExport(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CCheckoutDlg::~CCheckoutDlg()
{
}

void CCheckoutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_REVISION_NUM, m_editRevision);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	if (m_editRevision.IsWindowEnabled())
	{
		DDX_Text(pDX, IDC_REVISION_NUM, m_lRevision);
	}
	DDX_Text(pDX, IDC_CHECKOUTDIRECTORY, m_strCheckoutDirectory);
}


BEGIN_MESSAGE_MAP(CCheckoutDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_REVISION_N, OnBnClickedRevisionN)
	ON_BN_CLICKED(IDC_REVISION_HEAD, OnBnClickedRevisionHead)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_CHECKOUTDIRECTORY_BROWSE, OnBnClickedCheckoutdirectoryBrowse)
	ON_EN_CHANGE(IDC_CHECKOUTDIRECTORY, OnEnChangeCheckoutdirectory)
END_MESSAGE_MAP()

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCheckoutDlg::OnPaint()
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
HCURSOR CCheckoutDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CCheckoutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_URLCombo.LoadHistory(_T("repoURLS"), _T("url"));

	// set head revision as default revision
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);

	m_editRevision.SetWindowText(_T(""));

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_CHECKOUTDIRECTORY, IDS_CHECKOUT_TT_DIR);
	//m_tooltips.SetEffectBk(CBalloon::BALLOON_EFFECT_HGRADIENT);
	//m_tooltips.SetGradientColors(0x80ffff, 0x000000, 0xffff80);

	if (IsExport)
	{
		CString temp;
		temp.LoadString(IDS_PROGRS_TITLE_EXPORT);
		this->SetWindowText(temp);
		temp.LoadString(IDS_CHECKOUT_EXPORTDIR);
		GetDlgItem(IDC_EXPORT_CHECKOUTDIR)->SetWindowText(temp);
	} // if (IsExport)

	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCheckoutDlg::OnOK()
{
	UpdateData(TRUE);
	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();

	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		m_lRevision = -1;
	}
	if (m_strCheckoutDirectory.IsEmpty())
	{
		return;			//don't dismiss the dialog
	}
	if (!PathFileExists(m_strCheckoutDirectory))
	{
		CString temp;
		temp.Format(IDS_WARN_FOLDERNOTEXIST, m_strCheckoutDirectory);
		if (CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			CPathUtils::MakeSureDirectoryPathExists(m_strCheckoutDirectory);
		}
		else
			return;		//don't dismiss the dialog
	} // if (!PathFileExists(m_strCheckoutDirectory))
	UpdateData(FALSE);
	CDialog::OnOK();
}

void CCheckoutDlg::OnBnClickedRevisionN()
{
	m_editRevision.EnableWindow();
}

void CCheckoutDlg::OnBnClickedRevisionHead()
{
	m_editRevision.EnableWindow(FALSE);
}

void CCheckoutDlg::OnBnClickedBrowse()
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
	} // if (strUrl.Left(7) == _T("file://")) 
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

void CCheckoutDlg::OnBnClickedCheckoutdirectoryBrowse()
{
	//
	// Create a folder browser dialog. If the user selects OK, we should update
	// the local data members with values from the controls, copy the checkout
	// directory from the browse folder, then restore the local values into the
	// dialog controls.
	//
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCheckoutDirectory;
	if (browseFolder.Show(GetSafeHwnd(), strCheckoutDirectory) == CBrowseFolder::OK) 
	{
		UpdateData(true);
		m_strCheckoutDirectory = strCheckoutDirectory;
		UpdateData(false);
	}
}

BOOL CCheckoutDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

void CCheckoutDlg::OnEnChangeCheckoutdirectory()
{
	UpdateData(TRUE);
	if (m_strCheckoutDirectory.IsEmpty())
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
}
