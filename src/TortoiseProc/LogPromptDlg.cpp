// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
	DDX_Control(pDX, IDC_OLDLOGS, m_OldLogs);
}


BEGIN_MESSAGE_MAP(CLogPromptDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
	ON_BN_CLICKED(IDC_FILLLOG, OnBnClickedFilllog)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED, OnSVNStatusListCtrlItemCountChanged)
	ON_REGISTERED_MESSAGE(WM_AUTOLISTREADY, OnAutoListReady) 
	ON_CBN_CLOSEUP(IDC_OLDLOGS, OnCbnCloseupOldlogs)
END_MESSAGE_MAP()

// CLogPromptDlg message handlers

BOOL CLogPromptDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	UpdateData(FALSE);
	
	OnEnChangeLogmessage();

//	CString temp = m_sPath;

	m_ListCtrl.Init(SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS);
	m_ListCtrl.SetSelectButton(&m_SelectAll);
	m_ListCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
	m_ProjectProperties.ReadPropsPathList(m_pathList);
	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8));


	m_tooltips.Create(this);
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
	
	SVN svn;
	CString reg;
	reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), svn.GetUUIDFromPath(m_pathList[0]));
	m_OldLogs.LoadHistory(reg, _T("logmsgs"));
	
	AddAnchor(IDC_COMMITLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CHIST, TOP_LEFT);
	AddAnchor(IDC_OLDLOGS, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SELECTALL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HINTLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FILLLOG, BOTTOM_RIGHT);
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

		//save only the files the user has checked into the temporary file
		m_ListCtrl.WriteCheckedNamesToPathList(m_pathList);
	}
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	m_bBlock = FALSE;
	m_sBugID.Trim();
	m_sLogMessage = m_cLogMessage.GetText();
	m_OldLogs.AddString(m_sLogMessage, 0);
	m_OldLogs.SaveHistory();
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
	GetDlgItem(IDCANCEL)->EnableWindow(false);
	GetDlgItem(IDOK)->EnableWindow(false);
	GetDlgItem(IDC_FILLLOG)->EnableWindow(false);

	// Initialise the list control with the status of the files/folders below us
	BOOL success = m_ListCtrl.GetStatus(m_pathList);

	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS;
	dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
	m_ListCtrl.Show(dwShow, SVNSLC_SHOWMODIFIED|SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWMISSING|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED);

	if (m_ListCtrl.HasExternalsFromDifferentRepos())
	{
		CMessageBox::Show(m_hWnd, IDS_LOGPROMPT_EXTERNALS, IDS_APPNAME, MB_ICONINFORMATION);
	} // if (bHasExternalsFromDifferentRepos) 
	GetDlgItem(IDC_COMMIT_TO)->SetWindowText(m_ListCtrl.m_sURL);
	m_tooltips.AddTool(GetDlgItem(IDC_STATISTICS), m_ListCtrl.GetStatisticsString());
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	GetDlgItem(IDCANCEL)->EnableWindow(true);
	GetDlgItem(IDC_FILLLOG)->EnableWindow(true);
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
		GetDlgItem(IDCANCEL)->EnableWindow(true);
		m_bBlock = FALSE;
		EndDialog(0);
		return (DWORD)-1;
	}
	if ((m_ListCtrl.GetItemCount()==0)&&(!m_ListCtrl.HasUnversionedItems()))
	{
		CMessageBox::Show(m_hWnd, IDS_LOGPROMPT_NOTHINGTOCOMMIT, IDS_APPNAME, MB_ICONINFORMATION);
		GetDlgItem(IDCANCEL)->EnableWindow(true);
		EndDialog(0);
		return (DWORD)-1;
	}
	else
	{
		if (m_ListCtrl.GetItemCount()==0)
		{
			if (CMessageBox::Show(m_hWnd, IDS_LOGPROMPT_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
			{
				m_bShowUnversioned = TRUE;
				GetDlgItem(IDC_SHOWUNVERSIONED)->SendMessage(BM_SETCHECK, BST_CHECKED);
				DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWUNVERSIONED;
				m_ListCtrl.Show(dwShow);
			}
			else
			{
				GetDlgItem(IDCANCEL)->EnableWindow(true);
				EndDialog(0);
				return (DWORD)-1;
			}
		}
	}
	if (m_OldLogs.GetCount()==0)
	{
		CString reg;
		reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)m_ListCtrl.m_sUUID);
		m_OldLogs.LoadHistory(reg, _T("logmsgs"));
	}
	m_autolist.RemoveAll();
	// we don't have to block the commit dialog while we fetch the
	// auto completion list.
	m_bThreadRunning = TRUE;	// so the main thread knows that this thread is still running
	m_bRunThread = TRUE;		// if this is set to FALSE, the thread should stop
	m_bBlock = FALSE;
	GetAutocompletionList();
	// we have the list, now signal the main thread about it
	if (m_bRunThread)
		SendMessage(WM_AUTOLISTREADY);	// only send the message if the thread wasn't told to quit!
	m_bRunThread = FALSE;
	m_bThreadRunning = FALSE;
	return 0;
}

void CLogPromptDlg::OnCancel()
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
	m_OldLogs.AddString(m_cLogMessage.GetText(), 0);
	m_OldLogs.SaveHistory();
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
		DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS;
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

void CLogPromptDlg::OnBnClickedFilllog()
{
	if (m_bBlock)
		return;
	CString logmsg;
	TCHAR buf[MAX_PATH];
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
			if ((DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\EnglishTemplate"), FALSE)==TRUE)
				langID = 1033;
			SVNStatus::GetStatusString(AfxGetResourceHandle(), status, buf, sizeof(buf)/sizeof(TCHAR), langID);
			line.Format(_T("%-10s %s\r\n"), buf, (LPCTSTR)m_ListCtrl.GetItemText(i,0));
			logmsg += line;
		}
	}
	m_cLogMessage.InsertText(logmsg);
}

void CLogPromptDlg::OnCbnCloseupOldlogs()
{
	if (m_OldLogs.GetString().Compare(m_cLogMessage.GetText().Left(m_OldLogs.GetString().GetLength()))!=0)
		m_cLogMessage.InsertText(m_OldLogs.GetString(), !m_cLogMessage.GetText().IsEmpty());
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

LRESULT CLogPromptDlg::OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
	if ((m_ListCtrl.GetItemCount()==0)&&(!m_ListCtrl.HasUnversionedItems()))
	{
		CMessageBox::Show(*this, IDS_LOGPROMPT_NOTHINGTOCOMMIT, IDS_APPNAME, MB_ICONINFORMATION);
		GetDlgItem(IDCANCEL)->EnableWindow(true);
		EndDialog(0);
	}
	else
	{
		if (m_ListCtrl.GetItemCount()==0)
		{
			if (CMessageBox::Show(*this, IDS_LOGPROMPT_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
			{
				m_bShowUnversioned = TRUE;
				DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS | SVNSLC_SHOWUNVERSIONED;
				m_ListCtrl.Show(dwShow);
				UpdateData(FALSE);
			}
			else
			{
				GetDlgItem(IDCANCEL)->EnableWindow(true);
				EndDialog(0);
			}
		}
	}
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
	// .h, .hpp = (?<=class[\s])\b\w+\b|(\b\w+(?=[\s ]?\(\);))
	// .cpp = (?<=[^\s]::)\b\w+\b
	
	CStringArray arExtensions;
	CStringArray arRegexes;
	CString sRegexFile = CUtils::GetAppDirectory();
	sRegexFile += _T("autolist.txt");
	if (!m_bRunThread)
		return;
	DWORD timeout = GetTickCount()+5000;		// stop parsing after 5 seconds.
	try
	{
		CString strLine;
		CStdioFile file(sRegexFile, CFile::typeText | CFile::modeRead);
		while (m_bRunThread && (GetTickCount()<timeout) && file.ReadString(strLine))
		{
			CString sRegex = strLine.Mid(strLine.Find('=')+1).Trim();
			int pos = -1;
			while ((pos = strLine.Find(','))>=0)
			{
				arExtensions.Add(strLine.Left(pos));
				arRegexes.Add(sRegex);
				strLine = strLine.Mid(pos+1).Trim();
			}
			arExtensions.Add(strLine.Left(strLine.Find('=')).Trim());
			arRegexes.Add(sRegex);						
		}
		file.Close();
		
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
			if (entry->IsChecked())
			{
				// add the path parts to the autocompletion list too
				CString sPartPath = entry->GetRelativeSVNPath();
				m_autolist.AddSorted(sPartPath);
				int pos = 0;
				while ((pos = sPartPath.Find('/', pos)) >= 0)
				{
					pos++;
					m_autolist.AddSorted(sPartPath.Mid(pos));
				}
				CString sExt = entry->GetPath().GetFileExtension();
				CString sRegex;
				// find the regex string which corresponds to the file extension
				for (int j=0; (m_bRunThread && (j<arExtensions.GetCount())); ++j)
				{
					if (sExt.CompareNoCase(arExtensions.GetAt(j))==0)
					{
						sRegex = arRegexes.GetAt(j);
						break;
					}
				}
				if (!sRegex.IsEmpty())
					ScanFile(entry->GetPath().GetWinPathString(), sRegex);
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

void CLogPromptDlg::ScanFile(const CString& sFilePath, const CString& sRegex)
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
		return;
	match_results results;
	int offset1 = 0;
	int offset2 = 0;
	try
	{
		rpattern pat( (LPCTSTR)sRegex, MULTILINE ); 
		match_results::backref_type br;
		do 
		{
			br = pat.match( (LPCTSTR)sFileContent.Mid(offset1), results );
			if( br.matched ) 
			{
				offset1 += results.rstart(0);
				offset2 = offset1 + results.rlength(0);
				m_autolist.AddSorted(sFileContent.Mid(offset1, offset2-offset1));
				offset1 = offset2;
			}
		} while((br.matched)&&(m_bRunThread));
	}
	catch (bad_alloc) {}
	catch (bad_regexpr) {}
}


