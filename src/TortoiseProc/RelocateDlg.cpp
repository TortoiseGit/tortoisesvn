// RelocateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RelocateDlg.h"
#include "RepositoryBrowser.h"
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

	m_URLCombo.LoadHistory(_T("repoURLS"), _T("url"));

	CString url = m_sFromUrl;
	CUtils::Unescape(url.GetBuffer());
	url.ReleaseBuffer();
	GetDlgItem(IDC_FROMURL)->SetWindowText(url);

	CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRelocateDlg::OnBnClickedBrowse()
{
	UpdateData();
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
				m_sToUrl = browser.m_strUrl;
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
			m_sToUrl = browser.m_strUrl;
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
		}
	}
	UpdateData(FALSE);
}

void CRelocateDlg::OnOK()
{
	UpdateData(TRUE);
	m_URLCombo.SaveHistory();
	m_sToUrl = m_URLCombo.GetString();
	UpdateData(FALSE);

	CDialog::OnOK();
}
