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
#include "CopyDlg.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "RepositoryBrowser.h"
#include "Balloon.h"
#include "BrowseFolder.h"

// CCopyDlg dialog

IMPLEMENT_DYNAMIC(CCopyDlg, CResizableDialog)
CCopyDlg::CCopyDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CCopyDlg::IDD, pParent)
	, m_URL(_T(""))
	, m_sLogMessage(_T(""))
	, m_bDirectCopy(TRUE)
	, m_sBugID(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CCopyDlg::~CCopyDlg()
{
}

void CCopyDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Text(pDX, IDC_LOGMESSAGE, m_sLogMessage);
	DDX_Check(pDX, IDC_DIRECTCOPY, m_bDirectCopy);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Control(pDX, IDC_OLDLOGS, m_OldLogs);
}


BEGIN_MESSAGE_MAP(CCopyDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_CBN_SELCHANGE(IDC_OLDLOGS, OnCbnSelchangeOldlogs)
	ON_CBN_CLOSEUP(IDC_OLDLOGS, OnCbnCloseupOldlogs)
END_MESSAGE_MAP()


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCopyDlg::OnPaint() 
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
		CResizableDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCopyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CCopyDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	LOGFONT LogFont;
	LogFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
	LogFont.lfWidth          = 0;
	LogFont.lfEscapement     = 0;
	LogFont.lfOrientation    = 0;
	LogFont.lfWeight         = 400;
	LogFont.lfItalic         = 0;
	LogFont.lfUnderline      = 0;
	LogFont.lfStrikeOut      = 0;
	LogFont.lfCharSet        = DEFAULT_CHARSET;
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy(LogFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	m_logFont.CreateFontIndirect(&LogFont);
	GetDlgItem(IDC_LOGMESSAGE)->SetFont(&m_logFont);

	m_bFile = !PathIsDirectory(m_path);
	SVN svn;
	m_wcURL = svn.GetURLFromPath(m_path);
	CString sUUID = svn.GetUUIDFromPath(m_path);
	if (m_wcURL.IsEmpty())
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_NOURLOFFILE, IDS_APPNAME, MB_ICONERROR);
		TRACE(_T("could not retrieve the URL of the file!\n"));
		this->EndDialog(IDCANCEL);		//exit
	} // if ((rev == (-2))||(status.status->entry == NULL))
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS"), _T("url"));
	m_URLCombo.AddString(m_wcURL, 0);
	m_URLCombo.SelectString(-1, m_wcURL);
	GetDlgItem(IDC_FROMURL)->SetWindowText(m_wcURL);

	CString reg;
	reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)sUUID);
	m_OldLogs.LoadHistory(reg, _T("logmsgs"));

	m_ProjectProperties.ReadProps(m_path);
	if (m_ProjectProperties.sMessage.IsEmpty())
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}
	else
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
		if (!m_ProjectProperties.sLabel.IsEmpty())
			GetDlgItem(IDC_BUGIDLABEL)->SetWindowText(m_ProjectProperties.sLabel);
		GetDlgItem(IDC_BUGID)->SetFocus();
	}

	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CopyDlg"));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCopyDlg::OnOK()
{
	CString id;
	GetDlgItem(IDC_BUGID)->GetWindowText(id);
	if (m_ProjectProperties.bNumber)
	{
		TCHAR c = 0;
		BOOL bInvalid = FALSE;
		int len = id.GetLength();
		for (int i=0; i<len; ++i)
		{
			c = id.GetAt(i);
			if ((c < '0')&&(c != ','))
			{
				bInvalid = TRUE;
				break;
			}
			if (c > '9')
				bInvalid = TRUE;
		}
		if (bInvalid)
		{
			CWnd* ctrl = GetDlgItem(IDC_BUGID);
			CRect rt;
			ctrl->GetWindowRect(rt);
			CPoint point = CPoint((rt.left+rt.right)/2, (rt.top+rt.bottom)/2);
			CBalloon::ShowBalloon(ctrl, point, IDS_LOGPROMPT_ONLYNUMBERS, TRUE, IDI_EXCLAMATION);
			return;
		}
	}
	if ((m_ProjectProperties.bWarnIfNoIssue)&&(id.IsEmpty()))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_LOGPROMPT_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
			return;
	}
	UpdateData(TRUE);

	CString combourl;
	m_URLCombo.GetWindowText(combourl);
	if (m_wcURL.CompareNoCase(combourl)==0)
	{
		CString temp;
		temp.Format(IDS_ERR_COPYITSELF, (LPCTSTR)m_wcURL, (LPCTSTR)m_URLCombo.GetString());
		CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}
	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();
	m_OldLogs.AddString(m_sLogMessage, 0);
	m_OldLogs.SaveHistory();

	m_sBugID.Trim();
	if (!m_sBugID.IsEmpty())
	{
		m_sBugID.Replace(_T(", "), _T(","));
		m_sBugID.Replace(_T(" ,"), _T(","));
		CString sBugID = m_ProjectProperties.sMessage;
		sBugID.Replace(_T("%BUGID%"), m_sBugID);
		if (m_ProjectProperties.bAppend)
			m_sLogMessage += _T("\n") + sBugID + _T("\n");
		else
			m_sLogMessage = sBugID + _T("\n") + m_sLogMessage;
		UpdateData(FALSE);		
	}
	CResizableDialog::OnOK();
}

void CCopyDlg::OnBnClickedBrowse()
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
		||(strUrl.Left(10) == _T("svn+ssh://"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, this, m_bFile);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetWindowText(browser.GetPath());
		}
	}
}

void CCopyDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CCopyDlg::OnCbnSelchangeOldlogs()
{
	m_sLogMessage = m_OldLogs.GetString();
	UpdateData(FALSE);
}

void CCopyDlg::OnCbnCloseupOldlogs()
{
	m_sLogMessage = m_OldLogs.GetString();
	UpdateData(FALSE);
}

void CCopyDlg::OnCancel()
{
	UpdateData();
	m_OldLogs.AddString(m_sLogMessage, 0);
	m_OldLogs.SaveHistory();
	CResizableDialog::OnCancel();
}

BOOL CCopyDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					PostMessage(WM_COMMAND, IDOK);
				}
			}
		}
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}
