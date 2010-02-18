// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "CopyDlg.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "Registry.h"
#include "TSVNPath.h"
#include "AppUtils.h"
#include "HistoryDlg.h"
#include "SVNStatus.h"
#include "SVNProperties.h"

IMPLEMENT_DYNAMIC(CCopyDlg, CResizableStandAloneDialog)
CCopyDlg::CCopyDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCopyDlg::IDD, pParent)
	, m_URL(_T(""))
	, m_sLogMessage(_T(""))
	, m_sBugID(_T(""))
	, m_CopyRev(SVNRev::REV_HEAD)
	, m_bDoSwitch(false)
	, m_bSettingChanged(false)
	, m_bCancelled(false)
	, m_pThread(NULL)
	, m_pLogDlg(NULL)
	, m_bThreadRunning(0)
{
}

CCopyDlg::~CCopyDlg()
{
	delete m_pLogDlg;
}

void CCopyDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_DOSWITCH, m_bDoSwitch);
	DDX_Control(pDX, IDC_FROMURL, m_FromUrl);
	DDX_Control(pDX, IDC_EXTERNALSLIST, m_ExtList);
}


BEGIN_MESSAGE_MAP(CCopyDlg, CResizableStandAloneDialog)
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
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CCopyDlg::OnCbnEditchangeUrlcombo)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_EXTERNALSLIST, &CCopyDlg::OnLvnGetdispinfoExternalslist)
	ON_NOTIFY(LVN_KEYDOWN, IDC_EXTERNALSLIST, &CCopyDlg::OnLvnKeydownExternalslist)
	ON_NOTIFY(NM_CLICK, IDC_EXTERNALSLIST, &CCopyDlg::OnNMClickExternalslist)
END_MESSAGE_MAP()


BOOL CCopyDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	ExtendFrameIntoClientArea(0, 0, 0, IDC_MSGGROUP);
	m_aeroControls.SubclassControl(GetDlgItem(IDC_DOSWITCH)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDCANCEL)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDOK)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDHELP)->GetSafeHwnd());

	DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES;
	m_ExtList.SetExtendedStyle(exStyle);
	CXPTheme theme;
	theme.SetWindowTheme(m_ExtList.GetSafeHwnd(), L"Explorer", NULL);
	m_ExtList.ShowText(CString(MAKEINTRESOURCE(IDS_COPY_WAITFOREXTERNALS)));

	AdjustControlSize(IDC_COPYHEAD);
	AdjustControlSize(IDC_COPYREV);
	AdjustControlSize(IDC_COPYWC);
	AdjustControlSize(IDC_DOSWITCH);

	CTSVNPath path(m_path);

	m_History.SetMaxHistoryItems((LONG)CRegDWORD(_T("Software\\TortoiseSVN\\MaxHistoryItems"), 25));

	SetRevision(m_CopyRev);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);
	
	if (SVN::PathIsURL(path))
	{
		DialogEnableWindow(IDC_COPYWC, FALSE);
		DialogEnableWindow(IDC_DOSWITCH, FALSE);
		SetDlgItemText(IDC_COPYSTARTLABEL, CString(MAKEINTRESOURCE(IDS_COPYDLG_FROMURL)));
	}
	
	m_bFile = !path.IsDirectory();
	SVN svn;
	m_wcURL = svn.GetURLFromPath(path);
	CString sUUID = svn.GetUUIDFromPath(path);
	if (m_wcURL.IsEmpty())
	{
		CString Wrong_URL=path.GetSVNPathString();
		CString temp;
		temp.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)Wrong_URL);
		CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONINFORMATION);
		TRACE(_T("could not retrieve the URL of the file!"));
		this->EndDialog(IDCANCEL);		//exit
	}
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sUUID, _T("url"));
	m_URLCombo.AddString(CTSVNPath(m_wcURL).GetUIPathString(), 0);
	m_URLCombo.SelectString(-1, CTSVNPath(m_wcURL).GetUIPathString());
	SetDlgItemText(IDC_FROMURL, m_wcURL);
	if (!m_URL.IsEmpty())
		m_URLCombo.SetWindowText(m_URL);
	GetDlgItem(IDC_BROWSE)->EnableWindow(!m_URLCombo.GetString().IsEmpty());

	CString reg;
	reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)sUUID);
	m_History.Load(reg, _T("logmsgs"));

	m_ProjectProperties.ReadProps(m_path);
	if (CRegDWORD(_T("Software\\TortoiseSVN\\AlwaysWarnIfNoIssue"), FALSE)) 
		m_ProjectProperties.bWarnIfNoIssue = TRUE;

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
			SetDlgItemText(IDC_BUGIDLABEL, m_ProjectProperties.sLabel);
		GetDlgItem(IDC_BUGID)->SetFocus();
	}
	if (!m_sLogMessage.IsEmpty())
		m_cLogMessage.SetText(m_sLogMessage);

	CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
	CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));
	CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, _T("Alt+")+CString(CAppUtils::FindAcceleratorKey(this, IDC_INVISIBLE)));

	AddAnchor(IDC_REPOGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COPYSTARTLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FROMURL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOURLLABEL, TOP_LEFT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_FROMGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COPYHEAD, TOP_LEFT);
	AddAnchor(IDC_COPYREV, TOP_LEFT);
	AddAnchor(IDC_COPYREVTEXT, TOP_LEFT);
	AddAnchor(IDC_BROWSEFROM, TOP_LEFT);
	AddAnchor(IDC_COPYWC, TOP_LEFT);
	AddAnchor(IDC_EXTGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EXTERNALSLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_INVISIBLE, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_DOSWITCH, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CopyDlg"));

	m_bSettingChanged = false;
	// start a thread to obtain the highest revision number of the working copy
	// without blocking the dialog
	if ((m_pThread = AfxBeginThread(FindRevThreadEntry, this))==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;
}

UINT CCopyDlg::FindRevThreadEntry(LPVOID pVoid)
{
	return ((CCopyDlg*)pVoid)->FindRevThread();
}

UINT CCopyDlg::FindRevThread()
{
	InterlockedExchange(&m_bThreadRunning, TRUE);
	m_bmodified = false;
	if (!m_bCancelled)
	{
		// find the external properties
		SVNStatus stats(&m_bCancelled);
		CTSVNPath retPath;
		svn_wc_status2_t * s = NULL;
		s = stats.GetFirstFileStatus(m_path, retPath, false, svn_depth_unknown, true, false);
		while ((s) && (!m_bCancelled))
		{
			if (s->entry)
			{
				if (s->entry->kind == svn_node_dir)
				{
					// read the props of this dir and find out if it has svn:external props
					SVNProperties props(retPath, SVNRev::REV_WC, false);
					for (int i = 0; i < props.GetCount(); ++i)
					{
						if (props.GetItemName(i).compare(SVN_PROP_EXTERNALS) == 0)
						{
							m_externals.Add(retPath, props.GetItemValue(i), true);
						}
					}
				}
				if (s->entry->has_prop_mods)
					m_bmodified = true;
				if (s->entry->cmt_rev < m_maxrev)
					m_maxrev = s->entry->cmt_rev;
			}
			if ( (s->text_status != svn_wc_status_none) &&
				(s->text_status != svn_wc_status_normal) &&
				(s->text_status != svn_wc_status_external) &&
				(s->text_status != svn_wc_status_unversioned) &&
				(s->text_status != svn_wc_status_ignored))
				m_bmodified = true;

			s = stats.GetNextFileStatus(retPath);
		}
		if (!m_bCancelled)
			SendMessage(WM_TSVN_MAXREVFOUND);
	}
	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

void CCopyDlg::OnOK()
{
	m_bCancelled = true;
	// check if the status thread has already finished
	if (m_pThread)
	{
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	}

	CString id;
	GetDlgItemText(IDC_BUGID, id);
	CString sRevText;
	GetDlgItemText(IDC_COPYREVTEXT, sRevText);
	if (!m_ProjectProperties.CheckBugID(id))
	{
		ShowBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS);
		return;
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
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
		ShowBalloon(IDC_COPYREVTEXT, IDS_ERR_INVALIDREV);
		return;
	}
		
	CString combourl;
	m_URLCombo.GetWindowText(combourl);
	if ((m_wcURL.CompareNoCase(combourl)==0)&&(m_CopyRev.IsHead()))
	{
		CString temp;
		temp.FormatMessage(IDS_ERR_COPYITSELF, (LPCTSTR)m_wcURL, (LPCTSTR)m_URLCombo.GetString());
		CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}

	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();
	if (!CTSVNPath(m_URL).IsValidOnWindows())
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONINFORMATION|MB_YESNO) != IDYES)
			return;
	}

	m_History.AddEntry(m_sLogMessage);
	m_History.Save();

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
	CResizableStandAloneDialog::OnOK();
}

void CCopyDlg::OnBnClickedBrowse()
{
	m_tooltips.Pop();	// hide the tooltips
	SVNRev rev = SVNRev::REV_HEAD;

	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
}

void CCopyDlg::OnBnClickedHelp()
{
	m_tooltips.Pop();	// hide the tooltips
	OnHelp();
}

void CCopyDlg::OnCancel()
{
	m_bCancelled = true;
	// check if the status thread has already finished
	if (m_pThread)
	{
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	}
	if (m_ProjectProperties.sLogTemplate.Compare(m_cLogMessage.GetText()) != 0)
		m_History.AddEntry(m_cLogMessage.GetText());
	m_History.Save();
	CResizableStandAloneDialog::OnCancel();
}

BOOL CCopyDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);

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
						if (DWORD(CRegStdDWORD(_T("Software\\TortoiseSVN\\CtrlEnter"), TRUE)))
							PostMessage(WM_COMMAND, IDOK);
					}
				}
			}
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCopyDlg::OnBnClickedBrowsefrom()
{
	m_tooltips.Pop();	// hide the tooltips
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
    CString sTemp = m_cLogMessage.GetText();
	DialogEnableWindow(IDOK, sTemp.GetLength() >= m_ProjectProperties.nMinLogSize);
}

LPARAM CCopyDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	SetDlgItemText(IDC_COPYREVTEXT, temp);
	CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
	DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
	return 0;
}

void CCopyDlg::OnBnClickedCopyhead()
{
	m_tooltips.Pop();	// hide the tooltips
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
}

void CCopyDlg::OnBnClickedCopyrev()
{
	m_tooltips.Pop();	// hide the tooltips
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
}

void CCopyDlg::OnBnClickedCopywc()
{
	m_tooltips.Pop();	// hide the tooltips
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
}

void CCopyDlg::OnBnClickedHistory()
{
	m_tooltips.Pop();	// hide the tooltips
	SVN svn;
	CHistoryDlg historyDlg;
	historyDlg.SetHistory(m_History);
	if (historyDlg.DoModal()==IDOK)
	{
		if (historyDlg.GetSelectedText().Compare(m_cLogMessage.GetText().Left(historyDlg.GetSelectedText().GetLength()))!=0)
		{
			if (m_ProjectProperties.sLogTemplate.Compare(m_cLogMessage.GetText())!=0)
				m_cLogMessage.InsertText(historyDlg.GetSelectedText(), !m_cLogMessage.GetText().IsEmpty());
			else
				m_cLogMessage.SetText(historyDlg.GetSelectedText());
		}
		DialogEnableWindow(IDOK, m_ProjectProperties.nMinLogSize <= m_cLogMessage.GetText().GetLength());
	}
}

LPARAM CCopyDlg::OnRevFound(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	// we have found the highest last-committed revision
	// in the working copy
	if ((!m_bSettingChanged)&&(m_maxrev != 0)&&(!m_bCancelled))
	{
		// we only change the setting automatically if the user hasn't done so
		// already him/herself, if the highest revision is valid. And of course, 
		// if the thread hasn't been stopped forcefully.
		if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
		{
			if (m_bmodified)
			{
				// the working copy has local modifications.
				// show a warning balloon if the user has selected HEAD as the
				// source revision
				ShowBalloon(IDC_COPYHEAD, IDS_WARN_COPYHEADWITHLOCALMODS);
			}
			else
			{
				// and of course, we only change it if the radio button for a REPO-to-REPO copy
				// is enabled for HEAD and if there are no local modifications
				CString temp;
				temp.Format(_T("%ld"), m_maxrev);
				SetDlgItemText(IDC_COPYREVTEXT, temp);
				CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
				DialogEnableWindow(IDC_COPYREVTEXT, TRUE);			
			}
		}
	}


	if (m_externals.size())
		m_ExtList.ClearText();
	else
		m_ExtList.ShowText(CString(MAKEINTRESOURCE(IDS_COPY_NOEXTERNALSFOUND)));

	m_ExtList.SetRedraw(false);
	m_ExtList.DeleteAllItems();

	int c = ((CHeaderCtrl*)(m_ExtList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ExtList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_STATUSLIST_COLFILE);
	m_ExtList.InsertColumn(0, temp);
	temp.LoadString(IDS_STATUSLIST_COLURL);
	m_ExtList.InsertColumn(1, temp);
	temp.LoadString(IDS_STATUSLIST_COLREVISION);
	m_ExtList.InsertColumn(2, temp);
	m_ExtList.SetItemCountEx(m_externals.size());

	CRect rect;
	m_ExtList.GetClientRect(&rect);
	int cx = (rect.Width()-100)/2;
	m_ExtList.SetColumnWidth(0, cx);
	m_ExtList.SetColumnWidth(1, cx);
	m_ExtList.SetColumnWidth(2, 80);

	m_ExtList.SetRedraw(true);
	DialogEnableWindow(IDC_EXTERNALSLIST, true);

	return 0;
}

void CCopyDlg::SetRevision(const SVNRev& rev)
{
	if (rev.IsHead())
	{
		CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYHEAD);
		DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
	}
	else if (rev.IsWorking())
	{
		CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYWC);
		DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
	}
	else
	{
		CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
		DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
		CString temp;
		temp.Format(_T("%ld"), (LONG)rev);
		SetDlgItemText(IDC_COPYREVTEXT, temp);
	}
}

void CCopyDlg::OnCbnEditchangeUrlcombo()
{
	m_bSettingChanged = true;
	GetDlgItem(IDC_BROWSE)->EnableWindow(!m_URLCombo.GetString().IsEmpty());
}

void CCopyDlg::OnLvnGetdispinfoExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = 0;

	if (m_ExtList.HasText())
		return;

	if (pDispInfo)
	{
		if (pDispInfo->item.iItem < (int)m_externals.size())
		{
			SVNExternal ext = m_externals[pDispInfo->item.iItem];
			if (pDispInfo->item.mask & LVIF_INDENT)
			{
				pDispInfo->item.iIndent = 0;
			}
			if (pDispInfo->item.mask & LVIF_IMAGE)
			{
				pDispInfo->item.mask |= LVIF_STATE;
				pDispInfo->item.stateMask = LVIS_STATEIMAGEMASK;

				if (ext.adjust)
				{
					//Turn check box on
					pDispInfo->item.state = INDEXTOSTATEIMAGEMASK(2);
				}
				else
				{
					//Turn check box off
					pDispInfo->item.state = INDEXTOSTATEIMAGEMASK(1);
				}
			}
			if (pDispInfo->item.mask & LVIF_TEXT)
			{
				switch (pDispInfo->item.iSubItem)
				{
				case 0:	// folder
					{
						CTSVNPath p = ext.path;
						p.AppendPathString(ext.targetDir);
						lstrcpyn(m_columnbuf, p.GetWinPath(), pDispInfo->item.cchTextMax);
						int cWidth = m_ExtList.GetColumnWidth(0);
						cWidth = max(28, cWidth-28);
						CDC * pDC = m_ExtList.GetDC();
						if (pDC != NULL)
						{
							CFont * pFont = pDC->SelectObject(m_ExtList.GetFont());
							PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
							pDC->SelectObject(pFont);
							ReleaseDC(pDC);
						}
					}
					break;
				case 1:	// url
					{
						lstrcpyn(m_columnbuf, ext.url, pDispInfo->item.cchTextMax);
						int cWidth = m_ExtList.GetColumnWidth(0);
						cWidth = max(28, cWidth-28);
						CDC * pDC = m_ExtList.GetDC();
						if (pDC != NULL)
						{
							CFont * pFont = pDC->SelectObject(m_ExtList.GetFont());
							PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
							pDC->SelectObject(pFont);
							ReleaseDC(pDC);
						}
					}
					break;
				case 2: // revision
					if ((ext.revision.kind == svn_opt_revision_number) && (ext.revision.value.number >= 0))
						_stprintf_s(m_columnbuf, MAX_PATH, _T("%ld"), ext.revision.value.number);
					else
						m_columnbuf[0] = 0;
					break;

				default:
					m_columnbuf[0] = 0;
				}
				pDispInfo->item.pszText = m_columnbuf;
			}
		}
	}
}

void CCopyDlg::OnLvnKeydownExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	*pResult = 0;

	if (m_ExtList.HasText())
		return;

	if (pLVKeyDow->wVKey == VK_SPACE)
	{
		int nFocusedItem = m_ExtList.GetNextItem(-1, LVNI_FOCUSED);
		if (nFocusedItem >= 0)
			ToggleCheckbox(nFocusedItem);
	}
}

void CCopyDlg::OnNMClickExternalslist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	if (m_ExtList.HasText())
		return;

	UINT flags = 0;
	CPoint point(pNMItemActivate->ptAction);
	// Make the hit test...
	int item = m_ExtList.HitTest(point, &flags); 

	if (item != -1)
	{
		// We hit one item... did we hit state image (check box)?
		if( (flags & LVHT_ONITEMSTATEICON) != 0)
		{
			ToggleCheckbox(item);
		}
	}
}

void CCopyDlg::ToggleCheckbox(int index)
{
	SVNExternal ext = m_externals[index];
	ext.adjust = !ext.adjust;
	m_externals[index] = ext;
	m_ExtList.RedrawItems(index, index);
}