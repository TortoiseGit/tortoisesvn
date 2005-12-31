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
#include "LogPromptDlg.h"
#include "DirFileEnum.h"
#include "SVNConfig.h"
#include "SVNProperties.h"
#include "MessageBox.h"
#include "Utils.h"
#include "SVN.h"
#include "Registry.h"
#include "SVNStatus.h"
#include ".\logpromptdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CLogPromptDlg dialog

UINT CLogPromptDlg::WM_AUTOLISTREADY = RegisterWindowMessage(_T("TORTOISESVN_AUTOLISTREADY_MSG"));


IMPLEMENT_DYNAMIC(CLogPromptDlg, CResizableStandAloneDialog)
CLogPromptDlg::CLogPromptDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CLogPromptDlg::IDD, pParent)
	, m_bRecursive(FALSE)
	, m_bShowUnversioned(FALSE)
	, m_bBlock(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bRunThread(FALSE)
	, m_pThread(NULL)
	, m_bKeepLocks(FALSE)
{
}

CLogPromptDlg::~CLogPromptDlg()
{
	if(m_pThread != NULL)
	{
		delete m_pThread;
	}
}

void CLogPromptDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_ListCtrl);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
	DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Check(pDX, IDC_KEEPLOCK, m_bKeepLocks);
}


BEGIN_MESSAGE_MAP(CLogPromptDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED, OnSVNStatusListCtrlItemCountChanged)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(WM_AUTOLISTREADY, OnAutoListReady) 
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CLogPromptDlg message handlers

BOOL CLogPromptDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	UpdateData(FALSE);
	
	OnEnChangeLogmessage();

	m_ListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS | SVNSLC_COLLOCK);
	m_ListCtrl.SetSelectButton(&m_SelectAll);
	m_ListCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
	m_ListCtrl.SetCancelBool(&m_bCancelled);
	m_ListCtrl.SetEmptyString(IDS_LOGPROMPT_NOTHINGTOCOMMIT);
	
	m_ProjectProperties.ReadPropsPathList(m_pathList);
	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8));
	m_cLogMessage.RegisterContextMenuHandler(this);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTERNALWARNING, IDS_LOGPROMPT_EXTERNALS);
	m_tooltips.AddTool(IDC_HISTORY, IDS_LOGPROMPT_HISTORY_TT);
	
	m_SelectAll.SetCheck(BST_INDETERMINATE);
	
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
		
	if (!m_sLogMessage.IsEmpty())
		m_cLogMessage.SetText(m_sLogMessage);
		
	GetWindowText(m_sWindowTitle);
	
	AddAnchor(IDC_COMMITLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_EXTERNALWARNING, BOTTOM_RIGHT, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_KEEPLOCK, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("LogPromptDlg"));

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
	m_bBlock = TRUE;
	CRegDWORD err = CRegDWORD(_T("Software\\TortoiseSVN\\ErrorOccurred"), FALSE);
	CRegDWORD historyhint = CRegDWORD(_T("Software\\TortoiseSVN\\HistoryHintShown"), FALSE);
	if ((((DWORD)err)!=FALSE)&&((((DWORD)historyhint)==FALSE)))
	{
		historyhint = TRUE;
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this, IDC_HISTORY), IDS_LOGPROMPT_HISTORYHINT_TT, TRUE, IDI_INFORMATION);
	}
	err = FALSE;


	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CLogPromptDlg::OnOK()
{
	if (m_bBlock)
		return;
	if (m_bThreadRunning)
	{
		m_bRunThread = FALSE;
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			m_bThreadRunning = FALSE;
		}
	}
	CString id;
	GetDlgItem(IDC_BUGID)->GetWindowText(id);
	if (!m_ProjectProperties.CheckBugID(id))
	{
		CBalloon::ShowBalloon(this, CBalloon::GetCtrlCentre(this,IDC_BUGID), IDS_LOGPROMPT_ONLYNUMBERS, TRUE, IDI_EXCLAMATION);
		return;
	}
	if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_LOGPROMPT_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
			return;
	}
	m_bBlock = TRUE;
	CDWordArray arDeleted;
	//first add all the unversioned files the user selected
	//and check if all versioned files are selected
	int nUnchecked = 0;
	int nListItems = m_ListCtrl.GetItemCount();

	CTSVNPathList itemsToAdd;
	CTSVNPathList itemsToRemove;

	for (int j=0; j<nListItems; j++)
	{
		const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
		if (entry->IsChecked())
		{
			if (entry->status == svn_wc_status_unversioned)
			{
				itemsToAdd.AddPath(entry->GetPath());
			} 
			if (entry->status == svn_wc_status_missing)
			{
				itemsToRemove.AddPath(entry->GetPath());
			}
			if (entry->status == svn_wc_status_deleted)
			{
				arDeleted.Add(j);
			}
		}
		else
		{
			if ((entry->status != svn_wc_status_unversioned)	&&
				(entry->status != svn_wc_status_ignored))
				nUnchecked++;
		}
	} // for (int j=0; j<m_ListCtrl.GetItemCount(); j++)

	// Now, do all the adds - make sure that the list is sorted into order, so that parents 
	// are added before any children
	itemsToAdd.SortByPathname();
	SVN svn;
	svn.Add(itemsToAdd, FALSE);

	// Remove any missing items
	// Not sure that this sort is really necessary - indeed, it might be better to do a reverse sort at this point
	itemsToRemove.SortByPathname();
	svn.Remove(itemsToRemove, TRUE);

	if ((nUnchecked == 0)&&(m_ListCtrl.m_nTargetCount == 1))
	{
		m_bRecursive = TRUE;
	}
	else
	{
		m_bRecursive = FALSE;

		//the next step: find all deleted files and check if they're 
		//inside a deleted folder. If that's the case, then remove those
		//files from the list since they'll get deleted by the parent
		//folder automatically.
		m_ListCtrl.Block(TRUE);
		for (int i=0; i<arDeleted.GetCount(); i++)
		{
			if (m_ListCtrl.GetCheck(arDeleted.GetAt(i)))
			{
				const CTSVNPath& path = m_ListCtrl.GetListEntry(arDeleted.GetAt(i))->GetPath();
				if (path.IsDirectory())
				{
					//now find all children of this directory
					for (int j=0; j<arDeleted.GetCount(); j++)
					{
						if (i!=j)
						{
							CSVNStatusListCtrl::FileEntry* childEntry = m_ListCtrl.GetListEntry(arDeleted.GetAt(j));
							if (childEntry->IsChecked())
							{
								if (path.IsAncestorOf(childEntry->GetPath()))
								{
									m_ListCtrl.SetEntryCheck(childEntry, arDeleted.GetAt(j), false);
								}
							}
						}
					}
				}
			}
		} // for (int i=0; i<arDeleted.GetCount(); i++) 
		m_ListCtrl.Block(FALSE);

	}
	//save only the files the user has checked into the temporary file
	m_ListCtrl.WriteCheckedNamesToPathList(m_pathList);
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	m_bBlock = FALSE;
	m_sBugID.Trim();
	m_sLogMessage = m_cLogMessage.GetText();
	m_HistoryDlg.AddString(m_sLogMessage);
	m_HistoryDlg.SaveHistory();
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
	}
	CResizableStandAloneDialog::OnOK();
}

UINT CLogPromptDlg::StatusThreadEntry(LPVOID pVoid)
{
	return ((CLogPromptDlg*)pVoid)->StatusThread();
}

UINT CLogPromptDlg::StatusThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	m_bBlock = TRUE;
	m_bThreadRunning = TRUE;	// so the main thread knows that this thread is still running
	m_bRunThread = TRUE;		// if this is set to FALSE, the thread should stop
	m_bCancelled = false;

	GetDlgItem(IDOK)->EnableWindow(false);
	GetDlgItem(IDC_SHOWUNVERSIONED)->EnableWindow(false);
	GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EXTERNALWARNING)->EnableWindow(false);

	// Initialise the list control with the status of the files/folders below us
	BOOL success = m_ListCtrl.GetStatus(m_pathList);

	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWLOCKS;
	dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
	m_ListCtrl.Show(dwShow, SVNSLC_SHOWDIRECTS|SVNSLC_SHOWMODIFIED|SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|SVNSLC_SHOWLOCKS);

	if (m_ListCtrl.HasExternalsFromDifferentRepos())
	{
		GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EXTERNALWARNING)->EnableWindow();
	}
	GetDlgItem(IDC_COMMIT_TO)->SetWindowText(m_ListCtrl.m_sURL);
	m_tooltips.AddTool(GetDlgItem(IDC_STATISTICS), m_ListCtrl.GetStatisticsString());
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	CString logmsg;
	GetDlgItem(IDC_LOGMESSAGE)->GetWindowText(logmsg);
	if (m_ProjectProperties.nMinLogSize > logmsg.GetLength())
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	if (!success)
	{
		CMessageBox::Show(m_hWnd, m_ListCtrl.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
		m_bBlock = FALSE;
		SetTimer(ENDDIALOGTIMER, 100, NULL);
		return (DWORD)-1;
	}
	if ((m_ListCtrl.GetItemCount()==0)&&(m_ListCtrl.HasUnversionedItems()))
	{
		if (CMessageBox::Show(m_hWnd, IDS_LOGPROMPT_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
		{
			m_bShowUnversioned = TRUE;
			GetDlgItem(IDC_SHOWUNVERSIONED)->SendMessage(BM_SETCHECK, BST_CHECKED);
			DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWLOCKS;
			m_ListCtrl.Show(dwShow);
		}
	}
	if (m_HistoryDlg.GetCount()==0)
	{
		CString reg;
		reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)m_ListCtrl.m_sUUID);
		m_HistoryDlg.LoadHistory(reg, _T("logmsgs"));
	}

	CTSVNPath commonDir = m_ListCtrl.GetCommonDirectory(false);
	SetWindowText(m_sWindowTitle + _T(" - ") + commonDir.GetWinPathString());

	m_autolist.RemoveAll();
	// we don't have to block the commit dialog while we fetch the
	// auto completion list.
	m_bBlock = FALSE;
	if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\Autocompletion"), TRUE)==TRUE)
	{
		m_ListCtrl.Block(TRUE);
		GetAutocompletionList();
		m_ListCtrl.Block(FALSE);
	}
	GetDlgItem(IDC_SHOWUNVERSIONED)->EnableWindow(true);
	// we have the list, now signal the main thread about it
	if (m_bRunThread)
		SendMessage(WM_AUTOLISTREADY);	// only send the message if the thread wasn't told to quit!
	m_bRunThread = FALSE;
	m_bThreadRunning = FALSE;
	return 0;
}

void CLogPromptDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bBlock)
		return;
	if (m_bThreadRunning)
	{
		m_bRunThread = FALSE;
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			m_bThreadRunning = FALSE;
		}
	}
	m_HistoryDlg.AddString(m_cLogMessage.GetText());
	m_HistoryDlg.SaveHistory();
	CResizableStandAloneDialog::OnCancel();
}

void CLogPromptDlg::OnBnClickedSelectall()
{
	UINT state = (m_SelectAll.GetState() & 0x0003);
	if (state == BST_INDETERMINATE)
	{
		// It is not at all useful to manually place the checkbox into the indeterminate state...
		// We will force this on to the unchecked state
		state = BST_UNCHECKED;
		m_SelectAll.SetCheck(state);
	}
	m_ListCtrl.SelectAll(state == BST_CHECKED);
}

BOOL CLogPromptDlg::PreTranslateMessage(MSG* pMsg)
{
	if (!m_bBlock)
		m_tooltips.RelayEvent(pMsg);
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (m_bBlock)
					return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
				Refresh();
			}
			break;
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					PostMessage(WM_COMMAND, IDOK);
					return TRUE;
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CLogPromptDlg::Refresh()
{
	m_bBlock = TRUE;
	if (AfxBeginThread(StatusThreadEntry, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CLogPromptDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CLogPromptDlg::OnBnClickedShowunversioned()
{
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	if (!m_bBlock)
	{
		DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWLOCKS;
		dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
		m_ListCtrl.Show(dwShow);
	}
}

void CLogPromptDlg::OnEnChangeLogmessage()
{
	CString sTemp;
	GetDlgItem(IDC_LOGMESSAGE)->GetWindowText(sTemp);
	if (sTemp.GetLength() >= m_ProjectProperties.nMinLogSize)
	{
		if (!m_bBlock)
			GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
}

LRESULT CLogPromptDlg::OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
	if ((m_ListCtrl.GetItemCount()==0)&&(!m_ListCtrl.HasUnversionedItems()))
	{
		CMessageBox::Show(*this, IDS_LOGPROMPT_NOTHINGTOCOMMIT, IDS_APPNAME, MB_ICONINFORMATION);
		GetDlgItem(IDCANCEL)->EnableWindow(true);
		SetTimer(ENDDIALOGTIMER, 100, NULL);
	}
	else
	{
		if (m_ListCtrl.GetItemCount()==0)
		{
			if (CMessageBox::Show(*this, IDS_LOGPROMPT_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
			{
				m_bShowUnversioned = TRUE;
				DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWUNVERSIONED | SVNSLC_SHOWLOCKS;
				m_ListCtrl.Show(dwShow);
				UpdateData(FALSE);
			}
			else
			{
				GetDlgItem(IDCANCEL)->EnableWindow(true);
				SetTimer(ENDDIALOGTIMER, 100, NULL);
			}
		}
	}
	return 0;
}

LRESULT CLogPromptDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	Refresh();
	return 0;
}

LRESULT CLogPromptDlg::OnAutoListReady(WPARAM, LPARAM)
{
	m_cLogMessage.SetAutoCompletionList(m_autolist, '*');
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// functions which run in the status thread
//////////////////////////////////////////////////////////////////////////

void CLogPromptDlg::GetAutocompletionList()
{
	// the autocompletion list is made of strings from each selected files.
	// the strings used are extracted from the files with regexes found
	// in the file "autolist.txt".
	// the format of that file is:
	// file extensions separated with commas '=' regular expression to use
	// example:
	// (MULTILINE|NOCASE) .h, .hpp = (?<=class[\s])\b\w+\b|(\b\w+(?=[\s ]?\(\);))
	// .cpp = (?<=[^\s]::)\b\w+\b
	
	CMapStringToString mapRegex;
	CString sRegexFile = CUtils::GetAppDirectory();
	CRegDWORD regtimeout = CRegDWORD(_T("Software\\TortoiseSVN\\AutocompleteParseTimeout"), 5);
	DWORD timeoutvalue = regtimeout*1000;
	sRegexFile += _T("autolist.txt");
	if (!m_bRunThread)
		return;
	REGEX_FLAGS rflags = NOFLAGS;
	try
	{
		ATLTRACE("start parsing regex file for autocompletion\n");
		CString strLine;
		CStdioFile file(sRegexFile, CFile::typeText | CFile::modeRead);
		while (m_bRunThread && file.ReadString(strLine))
		{
			int eqpos = strLine.Find('=');
			CString sRegex = strLine.Mid(eqpos+1).Trim();
			CString sFlags = (strLine[0] == '(' ? strLine.Left(strLine.Find(')')+1).Trim(_T(" ()")) : _T(""));
			rflags |= sFlags.Find(_T("GLOBAL"))>=0 ? GLOBAL : NOFLAGS;
			rflags |= sFlags.Find(_T("MULTILINE"))>=0 ? MULTILINE : NOFLAGS;
			rflags |= sFlags.Find(_T("SINGLELINE"))>=0 ? SINGLELINE : NOFLAGS;
			rflags |= sFlags.Find(_T("RIGHTMOST"))>=0 ? RIGHTMOST : NOFLAGS;
			rflags |= sFlags.Find(_T("NORMALIZE"))>=0 ? NORMALIZE : NOFLAGS;
			rflags |= sFlags.Find(_T("NOCASE"))>=0 ? NOCASE : NOFLAGS;
				
			if (!sFlags.IsEmpty())
				strLine = strLine.Mid(strLine.Find(')')+1).Trim();
			int pos = -1;
			while (((pos = strLine.Find(','))>=0)&&(pos < eqpos))
			{
				mapRegex[strLine.Left(pos)] = sRegex;
				strLine = strLine.Mid(pos+1).Trim();
			}
			mapRegex[strLine.Left(strLine.Find('=')).Trim()] = sRegex;
		}
		file.Close();
		DWORD timeout = GetTickCount()+timeoutvalue;		// stop parsing after timeout
		
		// now we have two arrays of strings, where the first array contains all
		// file extensions we can use and the second the corresponding regex strings
		// to apply to those files.
		
		// the next step is to go over all files shown in the commit dialog
		// and scan them for strings we can use
		int nListItems = m_ListCtrl.GetItemCount();

		for (int i=0; i<nListItems; ++i)
		{
			if ((!m_bRunThread)||(GetTickCount()>timeout))
				return;
			const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(i);
			if (entry)
			{
				// add the path parts to the autocompletion list too
				CString sPartPath = entry->GetRelativeSVNPath();
				ATLTRACE("parse file %ws for autocompletion\n", (LPCTSTR)sPartPath);
				m_autolist.AddSorted(sPartPath);
				int pos = 0;
				while ((pos = sPartPath.Find('/', pos)) >= 0)
				{
					pos++;
					m_autolist.AddSorted(sPartPath.Mid(pos));
				}
				if (entry->IsChecked())
				{
					CString sExt = entry->GetPath().GetFileExtension();
					sExt.MakeLower();
					CString sRegex;
					// find the regex string which corresponds to the file extension
					sRegex = mapRegex[sExt];
					if (!sRegex.IsEmpty())
					{
						ScanFile(entry->GetPath().GetWinPathString(), sRegex, rflags);
						CTSVNPath basePath = SVN::GetPristinePath(entry->GetPath());
						if (!basePath.IsEmpty())
							ScanFile(basePath.GetWinPathString(), sRegex, rflags);
					}
				}
			}
		}
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading autolist regex file\n");
		pE->Delete();
		return;
	}
}

void CLogPromptDlg::ScanFile(const CString& sFilePath, const CString& sRegex, REGEX_FLAGS rflags)
{
	CString sFileContent;
	HANDLE hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(hFile, NULL);
		if (size > 1000000L)
		{
			// no files bigger than 1 Meg
			CloseHandle(hFile);
			return;
		}
		// allocate memory to hold file contents
		char * buffer = new char[size];
		DWORD readbytes;
		ReadFile(hFile, buffer, size, &readbytes, NULL);
		CloseHandle(hFile);
		int opts = 0;
		IsTextUnicode(buffer, readbytes, &opts);
		if (opts & IS_TEXT_UNICODE_NULL_BYTES)
		{
			delete buffer;
			ATLTRACE("file %ws is either binary or unicode\n", (LPCTSTR)sFilePath);
			return;
		}
		if (opts & IS_TEXT_UNICODE_UNICODE_MASK)
		{
			CString sText = CString(buffer, readbytes);
			sFileContent = sText;
		}
		if ((opts & IS_TEXT_UNICODE_NOT_UNICODE_MASK)||(opts == 0))
		{
			CStringA sText = CStringA(buffer, readbytes);
			sFileContent = CString(sText);
		}
		delete buffer;
	}
	if (sFileContent.IsEmpty()|| !m_bRunThread)
	{
		ATLTRACE("file %ws is empty\n", (LPCTSTR)sFilePath);
		return;
	}
	match_results results;
	size_t offset = 0;
	try
	{
		rpattern pat( (LPCTSTR)sRegex, rflags ); 
		match_results::backref_type br;
		restring reFileContent = (LPCTSTR)sFileContent;
		do 
		{
			br = pat.match( reFileContent, results, offset );
			if( br.matched ) 
			{
				for (size_t i=1; i<results.cbackrefs(); ++i)
				{
					m_autolist.AddSorted((LPCTSTR)results.backref(i).str().c_str());
					ATLTRACE("group %d is \"%ws\"\n", i, results.backref(i).str().c_str());
				}
				offset += results.rstart(0);
				offset += results.rlength(0);
			}
		} while((br.matched)&&(m_bRunThread));
		sFileContent.ReleaseBuffer();
	}
	catch (bad_alloc) {ATLTRACE("bad alloc exception when parsing file %ws\n", (LPCTSTR)sFilePath);}
	catch (bad_regexpr) {ATLTRACE("bad regexp exception when parsing file %ws\n", (LPCTSTR)sFilePath);}
}

// CSciEditContextMenuInterface
void CLogPromptDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
	CString sMenuItemText(MAKEINTRESOURCE(IDS_LOGPROMPT_POPUP_PASTEFILELIST));
	m_nPopupPasteListCmd = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPasteListCmd, sMenuItemText);
}

bool CLogPromptDlg::HandleMenuItemClick(int cmd, CSciEdit * pSciEdit)
{
	if (m_bBlock)
		return false;
	if (cmd == m_nPopupPasteListCmd)
	{
		CString logmsg;
		TCHAR buf[MAX_STATUS_STRING_LENGTH];
		int nListItems = m_ListCtrl.GetItemCount();
		for (int i=0; i<nListItems; ++i)
		{
			CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(i);
			if (entry->IsChecked())
			{
				CString line;
				svn_wc_status_kind status = entry->status;
				if (status == svn_wc_status_unversioned)
					status = svn_wc_status_added;
				if (status == svn_wc_status_missing)
					status = svn_wc_status_deleted;
				WORD langID = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
				if (m_ProjectProperties.bFileListInEnglish)
					langID = 1033;
				SVNStatus::GetStatusString(AfxGetResourceHandle(), status, buf, sizeof(buf)/sizeof(TCHAR), langID);
				line.Format(_T("%-10s %s\r\n"), buf, (LPCTSTR)m_ListCtrl.GetItemText(i,0));
				logmsg += line;
			}
		}
		pSciEdit->InsertText(logmsg);
		return true;
	}
	return false;
}

void CLogPromptDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == ENDDIALOGTIMER)
		EndDialog(0);
	__super::OnTimer(nIDEvent);
}

void CLogPromptDlg::OnBnClickedHistory()
{
	SVN svn;
	CString reg;
	reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), svn.GetUUIDFromPath(m_pathList[0]));
	m_HistoryDlg.LoadHistory(reg, _T("logmsgs"));
	if (m_HistoryDlg.DoModal()==IDOK)
	{
		if (m_HistoryDlg.GetSelectedText().Compare(m_cLogMessage.GetText().Left(m_HistoryDlg.GetSelectedText().GetLength()))!=0)
			m_cLogMessage.InsertText(m_HistoryDlg.GetSelectedText(), !m_cLogMessage.GetText().IsEmpty());
		if (m_ProjectProperties.nMinLogSize > m_cLogMessage.GetText().GetLength())
		{
			GetDlgItem(IDOK)->EnableWindow(FALSE);
		}
		else
		{
			if (!m_bBlock)
				GetDlgItem(IDOK)->EnableWindow(TRUE);
		}
	}
	
}