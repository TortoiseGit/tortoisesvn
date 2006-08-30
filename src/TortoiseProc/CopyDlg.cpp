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
#include "CopyDlg.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "RepositoryBrowser.h"
#include "Balloon.h"
#include "BrowseFolder.h"
#include "Registry.h"
#include "TSVNPath.h"
#include ".\copydlg.h"

// CCopyDlg dialog

IMPLEMENT_DYNAMIC(CCopyDlg, CStandAloneDialog)
CCopyDlg::CCopyDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CCopyDlg::IDD, pParent)
	, m_URL(_T(""))
	, m_sLogMessage(_T(""))
	, m_sBugID(_T(""))
	, m_CopyRev(SVNRev::REV_HEAD)
	, m_bDoSwitch(false)
	, m_bSettingChanged(false)
	, m_bCancelled(false)
	, m_pThread(NULL)
{
	m_pLogDlg = NULL;
}

CCopyDlg::~CCopyDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CCopyDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_DOSWITCH, m_bDoSwitch);
}


BEGIN_MESSAGE_MAP(CCopyDlg, CStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_MESSAGE(WM_TSVN_MAXREVFOUND, OnRevFound)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_BROWSEFROM, OnBnClickedBrowsefrom)
	ON_BN_CLICKED(IDC_COPYHEAD, OnBnClickedCopyhead)
	ON_BN_CLICKED(IDC_COPYREV, OnBnClickedCopyrev)
	ON_BN_CLICKED(IDC_COPYWC, OnBnClickedCopywc)
	ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
END_MESSAGE_MAP()


BOOL CCopyDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	CTSVNPath path(m_path);

	m_HistoryDlg.SetMaxHistoryItems((LONG)CRegDWORD(_T("Software\\TortoiseSVN\\MaxHistoryItems"), 25));

	if (m_CopyRev.IsHead())
	{
		CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYHEAD);
		DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
	}
	else
	{
		CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
		DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
		CString temp;
		temp.Format(_T("%ld"), (LONG)m_CopyRev);
		GetDlgItem(IDC_COPYREVTEXT)->SetWindowText(temp);
	}
	
	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_HISTORY, IDS_LOGPROMPT_HISTORY_TT);
	
	if (SVN::PathIsURL(path.GetSVNPathString()))
	{
		DialogEnableWindow(IDC_COPYWC, FALSE);
		DialogEnableWindow(IDC_DOSWITCH, FALSE);
		GetDlgItem(IDC_COPYSTARTLABEL)->SetWindowText(CString(MAKEINTRESOURCE(IDS_COPYDLG_FROMURL)));
	}
	
	m_bFile = !path.IsDirectory();
	SVN svn;
	m_wcURL = svn.GetURLFromPath(path);
	CString sUUID = svn.GetUUIDFromPath(path);
	if (m_wcURL.IsEmpty())
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_NOURLOFFILE, IDS_APPNAME, MB_ICONERROR);
		TRACE(_T("could not retrieve the URL of the file!\n"));
		this->EndDialog(IDCANCEL);		//exit
	}
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sUUID, _T("url"));
	m_URLCombo.AddString(CTSVNPath(m_wcURL).GetUIPathString(), 0);
	m_URLCombo.SelectString(-1, CTSVNPath(m_wcURL).GetUIPathString());
	GetDlgItem(IDC_FROMURL)->SetWindowText(m_wcURL);

	CString reg;
	reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)sUUID);
	m_HistoryDlg.LoadHistory(reg, _T("logmsgs"));

	m_ProjectProperties.ReadProps(m_path);
	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8));
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

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));

	m_bSettingChanged = false;
	// start a thread to obtain the highest revision number of the working copy
	// without blocking the dialog
	if ((m_pThread = AfxBeginThread(FindRevThreadEntry, this))==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

UINT CCopyDlg::FindRevThreadEntry(LPVOID pVoid)
{
	return ((CCopyDlg*)pVoid)->FindRevThread();
}

UINT CCopyDlg::FindRevThread()
{
	if (GetWCRevisionStatus(m_path, true, m_minrev, m_maxrev, m_bswitched, m_bmodified))
	{
		SendMessage(WM_TSVN_MAXREVFOUND);
	}
	return 0;
}

void CCopyDlg::OnOK()
{
	m_bCancelled = true;
	// check if the status thread has already finished
	if (m_pThread)
	{
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
	}

	CString id;
	GetDlgItem(IDC_BUGID)->GetWindowText(id);
	CString sRevText;
	GetDlgItem(IDC_COPYREVTEXT)->GetWindowText(sRevText);
	if (!m_ProjectProperties.CheckBugID(id))
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_BUGID), IDS_LOGPROMPT_ONLYNUMBERS, TRUE, IDI_EXCLAMATION);
		return;
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_LOGPROMPT_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
			return;
	}
	UpdateData(TRUE);

	if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
		m_CopyRev = SVNRev(SVNRev::REV_HEAD);
	else if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYWC)
		m_CopyRev = SVNRev(SVNRev::REV_WC);
	else
		m_CopyRev = SVNRev(sRevText);
	
	if (!m_CopyRev.IsValid())
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_COPYREVTEXT), IDS_ERR_INVALIDREV, TRUE, IDI_EXCLAMATION);
		return;
	}
		
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
	m_HistoryDlg.AddString(m_sLogMessage);
	m_HistoryDlg.SaveHistory();

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
	CStandAloneDialog::OnOK();
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
				m_URLCombo.SetCurSel(-1);
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

				m_URLCombo.SetCurSel(-1);
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
		CRepositoryBrowser browser(strUrl, this, m_bFile);
		if (browser.DoModal() == IDOK)
		{
			m_URLCombo.SetCurSel(-1);
			m_URLCombo.SetWindowText(browser.GetPath());
		}
	}
}

void CCopyDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CCopyDlg::OnCancel()
{
	m_bCancelled = true;
	// check if the status thread has already finished
	if (m_pThread)
	{
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
	}
	m_HistoryDlg.AddString(m_cLogMessage.GetText());
	m_HistoryDlg.SaveHistory();
	CStandAloneDialog::OnCancel();
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
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
				}
			}
		}
	}

	return CStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCopyDlg::OnBnClickedBrowsefrom()
{
	UpdateData(TRUE);
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog
	if (!m_wcURL.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->SetParams(CTSVNPath(m_wcURL), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, limit, TRUE);
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
		m_pLogDlg->m_wParam = 0;
		m_pLogDlg->m_pNotifyWindow = this;
	}
	AfxGetApp()->DoWaitCursor(-1);
}

void CCopyDlg::OnEnChangeLogmessage()
{
	CString sTemp;
	GetDlgItem(IDC_LOGMESSAGE)->GetWindowText(sTemp);
	DialogEnableWindow(IDOK, sTemp.GetLength() >= m_ProjectProperties.nMinLogSize);
}

LPARAM CCopyDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	GetDlgItem(IDC_COPYREVTEXT)->SetWindowText(temp);
	CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
	DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
	return 0;
}

void CCopyDlg::OnBnClickedCopyhead()
{
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
}

void CCopyDlg::OnBnClickedCopyrev()
{
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
}

void CCopyDlg::OnBnClickedCopywc()
{
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
}

void CCopyDlg::OnBnClickedHistory()
{
	SVN svn;
	CString reg;
	reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), svn.GetUUIDFromPath(m_path));
	m_HistoryDlg.LoadHistory(reg, _T("logmsgs"));
	if (m_HistoryDlg.DoModal()==IDOK)
	{
		if (m_HistoryDlg.GetSelectedText().Compare(m_cLogMessage.GetText().Left(m_HistoryDlg.GetSelectedText().GetLength()))!=0)
		{
			if (m_ProjectProperties.sLogTemplate.Compare(m_cLogMessage.GetText())!=0)
				m_cLogMessage.InsertText(m_HistoryDlg.GetSelectedText(), !m_cLogMessage.GetText().IsEmpty());
			else
				m_cLogMessage.SetText(m_HistoryDlg.GetSelectedText());
		}
		DialogEnableWindow(IDOK, m_ProjectProperties.nMinLogSize <= m_cLogMessage.GetText().GetLength());
	}
}

LPARAM CCopyDlg::OnRevFound(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	// we have found the highest last-committed revision
	// in the working copy
	if ((!m_bSettingChanged)&&(m_maxrev != 0)&&(!m_bmodified)&&(!m_bCancelled))
	{
		// we only change the setting automatically if the user hasn't done so
		// already him/herself, if the highest revision is valid and if the
		// working copy has no modifications. And of course, if the thread hasn't
		// been stopped forcefully.
		if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
		{
			// and of course, we only change it if the radio button for a REPO-to-REPO copy
			// is enabled for HEAD
			CString temp;
			temp.Format(_T("%ld"), m_maxrev);
			GetDlgItem(IDC_COPYREVTEXT)->SetWindowText(temp);
			CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
			DialogEnableWindow(IDC_COPYREVTEXT, TRUE);			
		}
	}
	return 0;
}
