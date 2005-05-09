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
#include "cursor.h"
#include "MergeDlg.h"
#include "InputDlg.h"
#include "PropDlg.h"
#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "RepositoryBrowser.h"
#include "CopyDlg.h"
#include "StatGraphDlg.h"
#include "Logdlg.h"
#include "MessageBox.h"
#include "Registry.h"
#include "Utils.h"
#include "InsertControl.h"
#include ".\logdlg.h"

// CLogDlg dialog

IMPLEMENT_DYNAMIC(CLogDlg, CResizableStandAloneDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CLogDlg::IDD, pParent),
	m_startrev(0),
	m_endrev(0),
	m_logcounter(0),
	m_bStrict(FALSE),
	m_nSearchIndex(0),
	m_wParam(0),
	m_nSelectedFilter(LOGFILTER_ALL),
	m_bNoDispUpdates(false)
{
	m_pFindDialog = NULL;
	m_bCancelled = FALSE;
	m_pNotifyWindow = NULL;
	m_bThreadRunning = FALSE;
}

CLogDlg::~CLogDlg()
{
	m_tempFileList.DeleteAllFiles();
	for (INT_PTR i=0; i<m_arLogPaths.GetCount(); ++i)
	{
		LogChangedPathArray * patharray = m_arLogPaths.GetAt(i);
		for (INT_PTR j=0; j<patharray->GetCount(); ++j)
		{
			delete patharray->GetAt(j);
		}
		patharray->RemoveAll();
		delete patharray;
	}
	m_arLogPaths.RemoveAll();
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGLIST, m_LogList);
	DDX_Control(pDX, IDC_LOGMSG, m_LogMsgCtrl);
	DDX_Control(pDX, IDC_PROGRESS, m_LogProgress);
	DDX_Control(pDX, IDC_SPLITTERTOP, m_wndSplitter1);
	DDX_Control(pDX, IDC_SPLITTERBOTTOM, m_wndSplitter2);
	DDX_Check(pDX, IDC_CHECK_STOPONCOPY, m_bStrict);
	DDX_Text(pDX, IDC_SEARCHEDIT, m_sFilterText);
	DDX_Control(pDX, IDC_DATEFROM, m_DateFrom);
	DDX_Control(pDX, IDC_DATETO, m_DateTo);
}

const UINT CLogDlg::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CLogDlg, CResizableStandAloneDialog)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, OnLvnKeydownLoglist)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage) 
	ON_BN_CLICKED(IDC_GETALL, OnBnClickedGetall)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGMSG, OnNMDblclkLogmsg)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGLIST, OnNMDblclkLoglist)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOGLIST, OnLvnItemchangedLoglist)
	ON_NOTIFY(EN_LINK, IDC_MSGVIEW, OnEnLinkMsgview)
	ON_BN_CLICKED(IDC_STATBUTTON, OnBnClickedStatbutton)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOGLIST, OnNMCustomdrawLoglist)
	ON_STN_CLICKED(IDC_FILTERICON, OnStnClickedFiltericon)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LOGLIST, OnLvnGetdispinfoLoglist)
	ON_EN_CHANGE(IDC_SEARCHEDIT, OnEnChangeSearchedit)
	ON_WM_TIMER()
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETO, OnDtnDatetimechangeDateto)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATEFROM, OnDtnDatetimechangeDatefrom)
	ON_BN_CLICKED(IDC_NEXTHUNDRED, OnBnClickedNexthundred)
END_MESSAGE_MAP()



void CLogDlg::SetParams(const CTSVNPath& path, long startrev, long endrev, int limit, BOOL bStrict /* = FALSE */)
{
	m_path = path;
	m_startrev = startrev;
	m_endrev = endrev;
	m_hasWC = !path.IsUrl();
	m_bStrict = bStrict;
	m_limit = limit;
}

BOOL CLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	CUtils::CreateFontForLogs(m_logFont);
	GetDlgItem(IDC_MSGVIEW)->SetFont(&m_logFont);
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_AUTOURLDETECT, TRUE, NULL);
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK);
	m_LogList.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );

	m_LogList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_LogList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_LOG_REVISION);
	m_LogList.InsertColumn(0, temp);
	temp.LoadString(IDS_LOG_AUTHOR);
	m_LogList.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_DATE);
	m_LogList.InsertColumn(2, temp);
	temp.LoadString(IDS_LOG_MESSAGE);
	m_LogList.InsertColumn(3, temp);
	m_arLogMessages.SetSize(0,100);
	m_arRevs.SetSize(0,100);

	m_LogList.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	m_LogMsgCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_LogMsgCtrl.DeleteAllItems();
	c = ((CHeaderCtrl*)(m_LogMsgCtrl.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_LogMsgCtrl.DeleteColumn(c--);
	temp.LoadString(IDS_PROGRS_ACTION);
	m_LogMsgCtrl.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_PATH);
	m_LogMsgCtrl.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_COPYFROM);
	m_LogMsgCtrl.InsertColumn(2, temp);
	temp.LoadString(IDS_LOG_REVISION);
	m_LogMsgCtrl.InsertColumn(3, temp);
	m_LogMsgCtrl.SetRedraw(false);
	mincol = 0;
	maxcol = ((CHeaderCtrl*)(m_LogMsgCtrl.GetDlgItem(0)))->GetItemCount()-1;
	col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_LogMsgCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	if (m_hasWC)
	{
		m_ProjectProperties.ReadProps(m_path);
	}

	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	m_logcounter = 0;
	m_sMessageBuf.Preallocate(100000);

	CString sTitle;
	GetWindowText(sTitle);
	if(m_path.IsDirectory())
	{
		SetWindowText(sTitle + _T(" - ") + m_path.GetWinPathString());
	}
	else
	{
		SetWindowText(sTitle + _T(" - ") + m_path.GetFilename());
	}

	SetSplitterRange();

	AddAnchor(IDC_FROMLABEL, TOP_LEFT);
	AddAnchor(IDC_DATEFROM, TOP_LEFT);
	AddAnchor(IDC_TOLABEL, TOP_LEFT);
	AddAnchor(IDC_DATETO, TOP_LEFT);

	SetFilterCueText();
	AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);
	InsertControl(GetDlgItem(IDC_SEARCHEDIT)->GetSafeHwnd(), GetDlgItem(IDC_FILTERICON)->GetSafeHwnd(), INSERTCONTROL_LEFT);
	
	AddAnchor(IDC_LOGLIST, TOP_LEFT, ANCHOR(100, 40));
	AddAnchor(IDC_SPLITTERTOP, ANCHOR(0, 40), ANCHOR(100, 40));
	AddAnchor(IDC_MSGVIEW, ANCHOR(0, 40), ANCHOR(100, 90));
	AddAnchor(IDC_SPLITTERBOTTOM, ANCHOR(0, 90), ANCHOR(100, 90));
	AddAnchor(IDC_LOGMSG, ANCHOR(0, 90), BOTTOM_RIGHT);
	
	AddAnchor(IDC_CHECK_STOPONCOPY, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GETALL, BOTTOM_LEFT);
	AddAnchor(IDC_NEXTHUNDRED, BOTTOM_LEFT);
	AddAnchor(IDC_STATBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	SetPromptParentWindow(m_hWnd);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("LogDlg"));
	GetDlgItem(IDC_LOGLIST)->SetFocus();
	//first start a thread to obtain the log messages without
	//blocking the dialog
	m_tTo = 0;
	m_tFrom = (DWORD)-1;
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return FALSE;
}

void CLogDlg::FillLogMessageCtrl(const CString& msg, LogChangedPathArray * paths)
{
	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	pMsgView->SetWindowText(_T(" "));
	pMsgView->SetWindowText(msg);
	m_ProjectProperties.FindBugID(msg, pMsgView);

	m_LogMsgCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_LogMsgCtrl.DeleteAllItems();
	m_LogMsgCtrl.SetRedraw(FALSE);
	int line = 0;
	CString temp;
	if (paths)
	{
		for (INT_PTR i=0; i<paths->GetCount(); ++i)
		{
			LogChangedPath * changedpath = paths->GetAt(i);
			m_LogMsgCtrl.InsertItem(line, changedpath->sAction);
			m_LogMsgCtrl.SetItemText(line, 1, changedpath->sPath);
			m_LogMsgCtrl.SetItemText(line, 2, changedpath->sCopyFromPath);
			if (!changedpath->sCopyFromPath.IsEmpty())
			{
				temp.Format(_T("%ld"), changedpath->lCopyFromRev);
				m_LogMsgCtrl.SetItemText(line, 3, temp);
			}
			line++;
		}
	}
	else
	{
		m_LogMsgCtrl.DeleteAllItems();
	}

	int maxcol = ((CHeaderCtrl*)(m_LogMsgCtrl.GetDlgItem(0)))->GetItemCount()-1;
	for (int col = 0; col <= maxcol; col++)
	{
		m_LogMsgCtrl.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_LogMsgCtrl.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
	m_LogMsgCtrl.SetRedraw();
}

void CLogDlg::OnBnClickedGetall()
{
	UpdateData();

	for (INT_PTR i=0; i<m_arLogPaths.GetCount(); ++i)
	{
		LogChangedPathArray * patharray = m_arLogPaths.GetAt(i);
		for (INT_PTR j=0; j<patharray->GetCount(); ++j)
		{
			delete patharray->GetAt(j);
		}
		patharray->RemoveAll();
		delete patharray;
	}
	m_arLogPaths.RemoveAll();

	m_LogList.DeleteAllItems();
	m_arLogMessages.RemoveAll();
	m_arShortLogMessages.RemoveAll();
	m_arLogPaths.RemoveAll();
	m_arRevs.RemoveAll();
	m_arAuthors.RemoveAll();
	m_arDates.RemoveAll();
	m_arDateStrings.RemoveAll();
	m_arCopies.RemoveAll();
	m_arShownList.RemoveAll();
	m_logcounter = 0;
	m_endrev = 1;
	m_startrev = -1;
	m_bCancelled = FALSE;
	m_limit = 0;
	m_tTo = 0;
	m_tFrom = (DWORD)-1;
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::OnBnClickedNexthundred()
{
	UpdateData();
	// we have to fetch the next hundred log messages.
	LONG rev = m_arRevs.GetAt(m_arRevs.GetCount()-1) - 1;
	if (rev < 1)
		return;		// do nothing! No more revisions to get
	m_startrev = rev;
	m_endrev = 1;
	m_bCancelled = FALSE;
	m_limit = 100;
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

BOOL CLogDlg::Cancel()
{
	return m_bCancelled;
}

void CLogDlg::OnCancel()
{
	CString temp, temp2;
	GetDlgItem(IDOK)->GetWindowText(temp);
	temp2.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(temp2)==0)
	{
		m_bCancelled = TRUE;
		return;
	}
	__super::OnCancel();
}

BOOL CLogDlg::Log(LONG rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies)
{
	int found = 0;
	m_LogList.SetRedraw(FALSE);
	m_logcounter += 1;
	if (m_startrev == -1)
		m_startrev = rev;
	if (m_limit != 0)
	{
		m_limitcounter--;
		m_LogProgress.SetPos(m_limit - m_limitcounter);
	}
	else
		m_LogProgress.SetPos(m_startrev-rev+m_endrev);
	int count = m_LogList.GetItemCount();
	int lastvisible = m_LogList.GetCountPerPage();
	__time64_t ttime = time/1000000L;
	m_arDates.Add((DWORD)ttime);
	if (m_tTo < (DWORD)ttime)
		m_tTo = (DWORD)ttime;
	if (m_tFrom > (DWORD)ttime)
		m_tFrom = (DWORD)ttime;
	m_arDateStrings.Add(date);
	m_arAuthors.Add(author);
	m_arFileChanges.Add(filechanges);
	m_arCopies.Add(copies);
	// Add as many characters from the log message to the list control
	// so it has a fixed width. If the log message is longer than
	// this predefined fixed with, add "..." as an indication.
	CString sShortMessage = message;
	// Remove newlines 'cause those are not shown nicely in the listcontrol
	sShortMessage.Replace(_T("\r"), _T(""));
	sShortMessage.Replace('\n', ' ');
	
	found = sShortMessage.Find(_T("\n\n"));
	if (found >=0)
	{
		if (found <=80)
			sShortMessage = sShortMessage.Left(found);
		else
		{
			found = sShortMessage.Find(_T("\n"));
			if ((found >= 0)&&(found <=80))
				sShortMessage = sShortMessage.Left(found);
		}
	}
	else if (sShortMessage.GetLength() > 80)
		sShortMessage = sShortMessage.Left(77) + _T("...");
		
	m_arShortLogMessages.Add(sShortMessage);
	
	m_arShownList.Add(m_arShownList.GetCount());
	
	//split multiline logentries and concatenate them
	//again but this time with \r\n as line separators
	//so that the edit control recognizes them
	try
	{
		if (message.GetLength()>0)
		{
			m_sMessageBuf = message;
			m_sMessageBuf.Replace(_T("\n\r"), _T("\n"));
			m_sMessageBuf.Replace(_T("\r\n"), _T("\n"));
			if (m_sMessageBuf.Right(1).Compare(_T("\n"))==0)
				m_sMessageBuf = m_sMessageBuf.Left(m_sMessageBuf.GetLength()-1);
		} // if (message.GetLength()>0)
		else
			m_sMessageBuf.Empty();
		m_arLogMessages.Add(m_sMessageBuf);
		m_arLogPaths.Add(cpaths);
		m_arRevs.Add(rev);
	}
	catch (CException * e)
	{
		::MessageBox(NULL, _T("not enough memory!"), _T("TortoiseSVN"), MB_ICONERROR);
		e->Delete();
		m_bCancelled = TRUE;
	}
	m_LogList.SetItemCountEx(m_arShownList.GetCount());
	if (count < lastvisible)
	{
		m_LogList.SetRedraw();
	}
	return TRUE;
}

//this is the thread function which calls the subversion function
UINT CLogDlg::LogThreadEntry(LPVOID pVoid)
{
	return ((CLogDlg*)pVoid)->LogThread();
}


//this is the thread function which calls the subversion function
UINT CLogDlg::LogThread()
{
	m_bThreadRunning = TRUE;
	// to make gettext happy
	SetThreadLocale(CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033));
	
	CString temp;
	temp.LoadString(IDS_MSGBOX_CANCEL);
	GetDlgItem(IDOK)->SetWindowText(temp);
	m_LogProgress.SetRange32(0, 100);
	m_LogProgress.SetPos(0);
	GetDlgItem(IDC_PROGRESS)->ShowWindow(TRUE);
	long r = -1;
	if (m_startrev == -1)
	{
		r = GetHEADRevision(m_path);
		m_startrev = r;
	}
	//disable the "Get All" button while we're receiving
	//log messages.
	GetDlgItem(IDC_GETALL)->EnableWindow(FALSE);
	GetDlgItem(IDC_NEXTHUNDRED)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_STOPONCOPY)->EnableWindow(FALSE);
	
	BOOL bOldAPI = (BOOL)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\OldLogAPI"), FALSE);
	if ((bOldAPI)&&(m_limit != 0))
	{
		m_endrev = m_startrev - m_limit;
		if (m_endrev < 1)
			m_endrev = 1;
		m_limit = 0;
	}
	if (m_limit != 0)
	{
		m_limitcounter = m_limit;
		m_LogProgress.SetRange32(0, m_limit);
	}
	else
		m_LogProgress.SetRange32(m_endrev, m_startrev);
	m_LogProgress.SetPos(0);
	
	if (!ReceiveLog(CTSVNPathList(m_path), m_startrev, m_endrev, m_limit, true, m_bStrict))
	{
		CMessageBox::Show(m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}

	__time64_t rt = m_tFrom;
	CTime tim(rt);
	m_DateFrom.SetTime(&tim);
	rt = m_tTo;
	tim = rt;
	m_DateTo.SetTime(&tim);

	temp.LoadString(IDS_MSGBOX_OK);
	GetDlgItem(IDOK)->SetWindowText(temp);

	GetDlgItem(IDC_GETALL)->EnableWindow(TRUE);
	GetDlgItem(IDC_NEXTHUNDRED)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_STOPONCOPY)->EnableWindow(TRUE);

	GetDlgItem(IDC_PROGRESS)->ShowWindow(FALSE);
	m_bCancelled = TRUE;
	m_bThreadRunning = FALSE;
	m_LogList.RedrawItems(0, m_arShownList.GetCount());
	m_LogList.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_LogList.SetRedraw(true);
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	return 0;
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex >= 0)
	{
		//check which key was pressed
		//this is necessary because we get here BEFORE
		//the selection actually changes, so we have to
		//adjust the selected index
		if ((pLVKeyDow->wVKey == VK_UP)||(pLVKeyDow->wVKey == VK_DOWN))
		{
			if (pLVKeyDow->wVKey == VK_UP)
			{
				if (selIndex > 0)
					selIndex--;
			}
			if (pLVKeyDow->wVKey == VK_DOWN)
			{
				selIndex++;		
				if (selIndex >= m_LogList.GetItemCount())
					selIndex = m_LogList.GetItemCount()-1;
			}
			this->m_nSearchIndex = selIndex;
			FillLogMessageCtrl(m_arLogMessages.GetAt(m_arShownList.GetAt(selIndex)), m_arLogPaths.GetAt(m_arShownList.GetAt(selIndex)));
			UpdateData(FALSE);
		}
		if (pLVKeyDow->wVKey == 'C')
		{
			if (GetKeyState(VK_CONTROL)&0x8000)
			{
				//Ctrl-C -> copy to clipboard
				CStringA sClipdata;
				POSITION pos = m_LogList.GetFirstSelectedItemPosition();
				if (pos != NULL)
				{
					CString sRev;
					sRev.LoadString(IDS_LOG_REVISION);
					CString sAuthor;
					sAuthor.LoadString(IDS_LOG_AUTHOR);
					CString sDate;
					sDate.LoadString(IDS_LOG_DATE);
					CString sMessage;
					sMessage.LoadString(IDS_LOG_MESSAGE);
					while (pos)
					{
						int nItem = m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos));
						CString sLogCopyText;
						CString sPaths;
						LogChangedPathArray * cpatharray = m_arLogPaths.GetAt(nItem);
						for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
						{
							LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
							sPaths += cpath->sPath + _T("\r\n");
						}
						sLogCopyText.Format(_T("%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n"),
							(LPCTSTR)sRev, m_arRevs.GetAt(nItem),
							(LPCTSTR)sAuthor, (LPCTSTR)m_LogList.GetItemText(nItem, 1),
							(LPCTSTR)sDate, (LPCTSTR)m_LogList.GetItemText(nItem, 2),
							(LPCTSTR)sMessage, (LPCTSTR)m_arLogMessages.GetAt(nItem),
							(LPCTSTR)sPaths);
						sClipdata +=  CStringA(sLogCopyText);
					}
					CUtils::WriteAsciiStringToClipboard(sClipdata);
				}
			}
		}
	}
	else
	{
		if (m_arLogMessages.GetCount()>0)
		{
			m_LogList.SetSelectionMark(0);
			m_LogList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED|LVIS_FOCUSED);
			FillLogMessageCtrl(m_arLogMessages.GetAt(0), m_arLogPaths.GetAt(0));
		}
		UpdateData(FALSE);
	}
	*pResult = 0;
}

BOOL CLogDlg::DiffPossible(LogChangedPath * changedpath, long rev)
{
	CString added, deleted;
	added.LoadString(IDS_SVNACTION_ADD);
	deleted.LoadString(IDS_SVNACTION_DELETE);

	if ((rev > 1)&&(changedpath->sAction.Compare(deleted)!=0))
	{
		if (changedpath->sAction.Compare(added)==0) // file is added
		{
			if (changedpath->lCopyFromRev == 0)
				return FALSE; // but file was not added with history
		}
		return TRUE;
	}
	return FALSE;
}

void CLogDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
//#region m_LogList
	if (pWnd == &m_LogList)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			m_LogList.GetItemRect(selIndex, &rect, LVIR_LABEL);
			m_LogList.ClientToScreen(&rect);
			point = rect.CenterPoint();
		}
		m_nSearchIndex = selIndex;
		selIndex = m_arShownList.GetAt(selIndex);
		if (selIndex >= 0)
		{
			//entry is selected, now show the popup menu
			CMenu popup;
			if (popup.CreatePopupMenu())
			{
				CString temp;
				if (m_LogList.GetSelectedCount() == 1)
				{
					if (!m_path.IsDirectory())
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARE);
						if (m_hasWC)
						{
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
							popup.SetDefaultItem(ID_COMPARE, FALSE);
						}
						temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
						popup.AppendMenu(MF_SEPARATOR, NULL);
						temp.LoadString(IDS_LOG_POPUP_SAVE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
						temp.LoadString(IDS_LOG_POPUP_OPEN);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPEN, temp);
					}
					else
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARE);
						if (m_hasWC)
						{
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
							popup.SetDefaultItem(ID_COMPARE, FALSE);
						}
						temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
						popup.AppendMenu(MF_SEPARATOR, NULL);
					}
					temp.LoadString(IDS_LOG_BROWSEREPO);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REPOBROWSE, temp);
					temp.LoadString(IDS_LOG_POPUP_COPY);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPY, temp);
					temp.LoadString(IDS_LOG_POPUP_UPDATE);
					if (m_hasWC)
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UPDATE, temp);
					temp.LoadString(IDS_LOG_POPUP_REVERTREV);
					if (m_hasWC)
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
				}
				else if (m_LogList.GetSelectedCount() >= 2)
				{
					if (m_LogList.GetSelectedCount() == 2)
					{
						if (!m_path.IsDirectory())
						{
							temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARETWO, temp);
						}
						temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF2, temp);
					}
					// reverting revisions only works (in one merge!) when the selected
					// revisions are continuous. So check first if that's the case before
					// we show the context menu.
					POSITION pos = m_LogList.GetFirstSelectedItemPosition();
					bool bContinuous = true;
					int itemindex = m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos));
					while (pos)
					{
						int nextindex = m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos));
						if (nextindex - itemindex > 1)
						{
							bContinuous = false;
							break;
						}
						itemindex = nextindex;
					}					
					temp.LoadString(IDS_LOG_POPUP_REVERTREVS);
					if ((m_hasWC)&&(bContinuous))
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
				}
				popup.AppendMenu(MF_SEPARATOR, NULL);
				
				temp.LoadString(IDS_LOG_POPUP_EDITAUTHOR);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITAUTHOR, temp);
				temp.LoadString(IDS_LOG_POPUP_EDITLOG);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITLOG, temp);
				
				popup.AppendMenu(MF_SEPARATOR, NULL);
				temp.LoadString(IDS_LOG_POPUP_FIND);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_FINDENTRY, temp);

				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				GetDlgItem(IDOK)->EnableWindow(FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				switch (cmd)
				{
				case ID_GNUDIFF1:
					{
						int selIndex = m_arShownList.GetAt(m_LogList.GetSelectionMark());
						long rev = m_arRevs.GetAt(selIndex);
						this->m_bCancelled = FALSE;
						CTSVNPath tempfile = CUtils::GetTempFilePath(CTSVNPath(_T("Test.diff")));
						m_tempFileList.AddPath(tempfile);
						if (!PegDiff(m_path, (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), rev-1, rev, TRUE, FALSE, TRUE, TRUE, _T(""), tempfile))
						{
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;		//exit
						}
						else
						{
							if (CUtils::CheckForEmptyDiff(tempfile))
							{
								CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
								break;
							}
							CUtils::StartUnifiedDiffViewer(tempfile);
						}
					}
					break;
				case ID_GNUDIFF2:
					{
						POSITION pos = m_LogList.GetFirstSelectedItemPosition();
						long rev1 = m_arRevs.GetAt(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						long rev2 = m_arRevs.GetAt(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						this->m_bCancelled = FALSE;
						CTSVNPath tempfile = CUtils::GetTempFilePath(CTSVNPath(_T("Test.diff")));
						m_tempFileList.AddPath(tempfile);
						if (!PegDiff(m_path, (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), rev2, rev1, TRUE, FALSE, TRUE, TRUE, _T(""), tempfile))
						{
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;		//exit
						}
						else
						{
							if (CUtils::CheckForEmptyDiff(tempfile))
							{
								CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
								break;
							}
							CUtils::StartUnifiedDiffViewer(tempfile);
						}
					}
					break;
				case ID_REVERTREV:
					{
						// we show the context menu.
						POSITION pos = m_LogList.GetFirstSelectedItemPosition();
						long rev = m_arRevs.GetAt(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						long revend = rev-1;
						while (pos)
						{
							revend = m_arRevs.GetAt(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						}					
						CString msg;
						msg.Format(IDS_LOG_REVERT_CONFIRM, m_path.GetWinPathString());
						if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
						{
							CString url = this->GetURLFromPath(m_path);
							if (url.IsEmpty())
							{
								CString strMessage;
								strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
								CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseSVN"), MB_ICONERROR);
								TRACE(_T("could not retrieve the URL of the folder!\n"));
								break;		//exit
							}
							else
							{
								CSVNProgressDlg dlg;
								dlg.SetParams(CSVNProgressDlg::Enum_Merge, 0, CTSVNPathList(m_path), url, url, rev);		//use the message as the second url
								dlg.m_RevisionEnd = revend;
								dlg.DoModal();
							}
						}
					}
					break;
				case ID_COPY:
					{
						long rev = m_arRevs.GetAt(selIndex);
						CCopyDlg dlg;
						CString url = GetURLFromPath(m_path);
						if (url.IsEmpty())
						{
							CString temp;
							temp.Format(IDS_ERR_NOURLOFFILE, m_path);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
							TRACE(_T("could not retrieve the URL of the folder!\n"));
							break;		//exit
						}
						else
						{
							dlg.m_URL = url;
							dlg.m_path = m_path;
							dlg.m_CopyRev = SVNRev(rev);
							if (dlg.DoModal() == IDOK)
							{
								SVN svn;
								if (!svn.Copy(CTSVNPath(url), CTSVNPath(dlg.m_URL), rev, dlg.m_sLogMessage))
								{
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								}
								else
								{
									CMessageBox::Show(this->m_hWnd, IDS_LOG_COPY_SUCCESS, IDS_APPNAME, MB_ICONINFORMATION);
								}
							} // if (dlg.DoModal() == IDOK) 
						}
					} 
					break;
				case ID_COMPARE:
					{
						//user clicked on the menu item "compare with working copy"
						//now first get the revision which is selected
						int selIndex = m_arShownList.GetAt(m_LogList.GetSelectionMark());
						long rev = m_arRevs.GetAt(selIndex);
						if ((m_path.IsDirectory())||(!m_hasWC))
						{
							this->m_bCancelled = FALSE;
							CTSVNPath tempfile = CUtils::GetTempFilePath();
							m_tempFileList.AddPath(tempfile);
							tempfile.AppendRawString(_T(".diff"));
							m_tempFileList.AddPath(tempfile);
							if (!PegDiff(m_path, (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), SVNRev::REV_WC, rev, TRUE, FALSE, TRUE, TRUE, _T(""), tempfile))
							{
								CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								break;		//exit
							} 
							else
							{
								if (CUtils::CheckForEmptyDiff(tempfile))
								{
									CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
									break;
								}
								CString sWC, sRev;
								sWC.LoadString(IDS_DIFF_WORKINGCOPY);
								sRev.Format(IDS_DIFF_REVISIONPATCHED, rev);
								CUtils::StartExtPatch(tempfile, m_path.GetDirectory(), sWC, sRev, TRUE);
							}
						}
						else
						{
							CTSVNPath tempfile = CUtils::GetTempFilePath(m_path);
							m_tempFileList.AddPath(tempfile);
							SVN svn;
							if (!svn.Cat(m_path, rev, tempfile))
							{
								CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							} 
							else
							{
								CString revname, wcname;
								revname.Format(_T("%s Revision %ld"), (LPCTSTR)m_path.GetFilename(), rev);
								wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)m_path.GetFilename());
								CUtils::StartExtDiff(tempfile, m_path, revname, wcname);
							}
						}
					}
					break;
				case ID_COMPARETWO:
					{
						//user clicked on the menu item "compare revisions"
						//now first get the revisions which are selected
						POSITION pos = m_LogList.GetFirstSelectedItemPosition();
						long rev1 = m_arRevs.GetAt(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						long rev2 = m_arRevs.GetAt(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));

						StartDiff(m_path, rev1, m_path, rev2);
					}
					break;
				case ID_SAVEAS:
					{
						//now first get the revision which is selected
						long rev = m_arRevs.GetAt(selIndex);

						OPENFILENAME ofn;		// common dialog box structure
						TCHAR szFile[MAX_PATH];  // buffer for file name
						ZeroMemory(szFile, sizeof(szFile));
						if (m_hasWC)
						{
							CString revFilename;
							CString strWinPath = m_path.GetWinPathString();
							int rfind = strWinPath.ReverseFind('.');
							if (rfind > 0)
								revFilename.Format(_T("%s-%ld%s"), (LPCTSTR)strWinPath.Left(rfind), rev, (LPCTSTR)strWinPath.Mid(rfind));
							else
								revFilename.Format(_T("%s-%ld"), (LPCTSTR)strWinPath, rev);
							_tcscpy(szFile, revFilename);
						}
						// Initialize OPENFILENAME
						ZeroMemory(&ofn, sizeof(OPENFILENAME));
						ofn.lStructSize = sizeof(OPENFILENAME);
						//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
						ofn.hwndOwner = this->m_hWnd;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
						CString temp;
						temp.LoadString(IDS_LOG_POPUP_SAVE);
						//ofn.lpstrTitle = "Save revision to...\0";
						CUtils::RemoveAccelerators(temp);
						if (temp.IsEmpty())
							ofn.lpstrTitle = NULL;
						else
							ofn.lpstrTitle = temp;
						ofn.Flags = OFN_OVERWRITEPROMPT;

						CString sFilter;
						sFilter.LoadString(IDS_COMMONFILEFILTER);
						TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
						_tcscpy (pszFilters, sFilter);
						// Replace '|' delimeters with '\0's
						TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
						while (ptr != pszFilters)
						{
							if (*ptr == '|')
								*ptr = '\0';
							ptr--;
						} // while (ptr != pszFilters) 
						ofn.lpstrFilter = pszFilters;
						ofn.nFilterIndex = 1;
						// Display the Open dialog box. 
						CTSVNPath tempfile;
						if (GetSaveFileName(&ofn)==TRUE)
						{
							tempfile.SetFromWin(ofn.lpstrFile);
							SVN svn;
							if (!svn.Cat(m_path, rev, tempfile))
							{
								delete [] pszFilters;
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							} // if (!svn.Cat(m_path, rev, tempfile)) 
						} // if (GetSaveFileName(&ofn)==TRUE)
						delete [] pszFilters;
					}
					break;
				case ID_OPEN:
					{
						//now first get the revision which is selected
						int selIndex = m_arShownList.GetAt(m_LogList.GetSelectionMark());
						long rev = m_arRevs.GetAt(selIndex);

						CTSVNPath tempfile = CUtils::GetTempFilePath(m_path);
						m_tempFileList.AddPath(tempfile);
						SVN svn;
						if (!svn.Cat(m_path, rev, tempfile))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							break;
						} // if (!svn.Cat(m_path, rev, tempfile))
						else
						{
							ShellExecute(this->m_hWnd, _T("open"), tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
						}
					}
					break;
				case ID_UPDATE:
					{
						//now first get the revision which is selected
						int selIndex = m_arShownList.GetAt(m_LogList.GetSelectionMark());
						long rev = m_arRevs.GetAt(selIndex);

						SVN svn;
						if (!svn.Update(CTSVNPathList(m_path), rev, TRUE, FALSE))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							break;
						}
					}
					break;
				case ID_FINDENTRY:
					{
						m_nSearchIndex = m_LogList.GetSelectionMark();
						if (m_nSearchIndex < 0)
							m_nSearchIndex = 0;
						if (m_pFindDialog)
						{
							break;
						}
						else
						{
							m_pFindDialog = new CFindReplaceDialog();
							m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
						}
					}
					break;
				case ID_REPOBROWSE:
					{
						int selIndex = m_arShownList.GetAt(m_LogList.GetSelectionMark());
						long rev = m_arRevs.GetAt(selIndex);
						CString url = m_path.GetSVNPathString();
						if (m_hasWC)
						{
							url = GetURLFromPath(m_path);
							if (url.IsEmpty())
							{
								CString temp;
								temp.Format(IDS_ERR_NOURLOFFILE, m_path);
								CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							}
						}

						CRepositoryBrowser dlg(SVNUrl(url, SVNRev(rev)));
						dlg.DoModal();
					}
					break;
				case ID_EDITLOG:
				{
					EditLogMessage(selIndex);
				}
				break;
				case ID_EDITAUTHOR:
				{
					EditAuthor(selIndex);
				}
				break;
				default:
					break;
				} // switch (cmd)
				theApp.DoWaitCursor(-1);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
			} // if (popup.CreatePopupMenu())
		} // if (selIndex >= 0)
	} // if (pWnd == &m_LogList)
//#endregion
//#region m_LogMsgCtrl
	if (pWnd == &m_LogMsgCtrl)
	{
		int selIndex = m_LogMsgCtrl.GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			m_LogMsgCtrl.GetItemRect(selIndex, &rect, LVIR_LABEL);
			m_LogMsgCtrl.ClientToScreen(&rect);
			point = rect.CenterPoint();
		}
		if (selIndex < 0)
			return;
		int s = m_LogList.GetSelectionMark();
		if (s < 0)
			return;
		s = m_arShownList.GetAt(s);
		long rev = m_arRevs.GetAt(s);
		LogChangedPath * changedpath = m_arLogPaths.GetAt(s)->GetAt(selIndex);
		//entry is selected, now show the popup menu
		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			CString temp;
			if (m_LogMsgCtrl.GetSelectedCount() == 1)
			{
				if (DiffPossible(changedpath, rev))
				{
					temp.LoadString(IDS_LOG_POPUP_DIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_DIFF, temp);
					popup.SetDefaultItem(ID_DIFF, FALSE);
					popup.AppendMenu(MF_SEPARATOR, NULL);
				}
				temp.LoadString(IDS_LOG_POPUP_OPEN);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPEN, temp);
				temp.LoadString(IDS_LOG_POPUP_OPENWITH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPENWITH, temp);
				popup.AppendMenu(MF_SEPARATOR, NULL);
				temp.LoadString(IDS_LOG_POPUP_REVERTREV);
				if (m_hasWC)
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
				temp.LoadString(IDS_REPOBROWSE_SHOWPROP);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPPROPS, temp);			// "Show Properties"
				temp.LoadString(IDS_MENULOG);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_LOG, temp);					// "Show Log"				
				temp.LoadString(IDS_LOG_POPUP_SAVE);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				bool bOpenWith = false;
				switch (cmd)
				{
				case ID_DIFF:
					{
						DoDiffFromLog(selIndex, rev);
					}
					break;
				case ID_REVERTREV:
					{
						SetPromptApp(&theApp);
						theApp.DoWaitCursor(1);
						CString sUrl;
						if (SVN::PathIsURL(m_path.GetSVNPathString()))
						{
							sUrl = m_path.GetSVNPathString();
						}
						else
						{
							SVN svn;
							sUrl = svn.GetURLFromPath(m_path);
							if (sUrl.IsEmpty())
							{
								theApp.DoWaitCursor(-1);
								CString temp;
								temp.Format(IDS_ERR_NOURLOFFILE, m_path);
								CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								theApp.DoWaitCursor(-11);
								break;		//exit
							}
						}
						CString sUrlRoot = GetRepositoryRoot(CTSVNPath(sUrl));

						CString fileURL = changedpath->sPath;
						fileURL = sUrlRoot + fileURL.Trim();
						// firstfile = (e.g.) http://mydomain.com/repos/trunk/folder/file1
						// sUrl = http://mydomain.com/repos/trunk/folder
						CString wcPath = m_path.GetWinPathString() + fileURL.Mid(sUrl.GetLength());
						wcPath.Replace('/', '\\');
						CString msg;
						msg.Format(IDS_LOG_REVERT_CONFIRM, wcPath);
						if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
						{
							CSVNProgressDlg dlg;
							dlg.SetParams(CSVNProgressDlg::Enum_Merge, 0, CTSVNPathList(CTSVNPath(wcPath)), fileURL, fileURL, rev);		//use the message as the second url
							dlg.m_RevisionEnd = rev-1;
							dlg.DoModal();
						}
						theApp.DoWaitCursor(-1);
					}
					break;
				case ID_POPPROPS:
					{
						GetDlgItem(IDOK)->EnableWindow(FALSE);
						SetPromptApp(&theApp);
						theApp.DoWaitCursor(1);
						CString filepath;
						if (SVN::PathIsURL(m_path.GetSVNPathString()))
						{
							filepath = m_path.GetSVNPathString();
						}
						else
						{
							SVN svn;
							filepath = svn.GetURLFromPath(m_path);
							if (filepath.IsEmpty())
							{
								theApp.DoWaitCursor(-1);
								CString temp;
								temp.Format(IDS_ERR_NOURLOFFILE, filepath);
								CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
								TRACE(_T("could not retrieve the URL of the file!\n"));
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							}
						}
						filepath = GetRepositoryRoot(CTSVNPath(filepath));
						filepath += changedpath->sPath;
						CPropDlg dlg;
						dlg.m_rev = rev;
						dlg.m_Path = CTSVNPath(filepath);
						dlg.DoModal();
						GetDlgItem(IDOK)->EnableWindow(TRUE);
						theApp.DoWaitCursor(-1);
					}
					break;
				case ID_SAVEAS:
					{
						GetDlgItem(IDOK)->EnableWindow(FALSE);
						SetPromptApp(&theApp);
						theApp.DoWaitCursor(1);
						CString filepath;
						if (SVN::PathIsURL(m_path.GetSVNPathString()))
						{
							filepath = m_path.GetSVNPathString();
						}
						else
						{
							SVN svn;
							filepath = svn.GetURLFromPath(m_path);
							if (filepath.IsEmpty())
							{
								theApp.DoWaitCursor(-1);
								CString temp;
								temp.Format(IDS_ERR_NOURLOFFILE, filepath);
								CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
								TRACE(_T("could not retrieve the URL of the file!\n"));
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							}
						}
						filepath = GetRepositoryRoot(CTSVNPath(filepath));
						filepath += changedpath->sPath;

						OPENFILENAME ofn;		// common dialog box structure
						TCHAR szFile[MAX_PATH];  // buffer for file name
						ZeroMemory(szFile, sizeof(szFile));
						CString revFilename;
						temp = CUtils::GetFileNameFromPath(filepath);
						int rfind = filepath.ReverseFind('.');
						if (rfind > 0)
							revFilename.Format(_T("%s-%ld%s"), temp.Left(rfind), rev, temp.Mid(rfind));
						else
							revFilename.Format(_T("%s-%ld"), temp, rev);
						_tcscpy(szFile, revFilename);
						// Initialize OPENFILENAME
						ZeroMemory(&ofn, sizeof(OPENFILENAME));
						ofn.lStructSize = sizeof(OPENFILENAME);
						//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
						ofn.hwndOwner = this->m_hWnd;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
						temp.LoadString(IDS_LOG_POPUP_SAVE);
						CUtils::RemoveAccelerators(temp);
						if (temp.IsEmpty())
							ofn.lpstrTitle = NULL;
						else
							ofn.lpstrTitle = temp;
						ofn.Flags = OFN_OVERWRITEPROMPT;

						CString sFilter;
						sFilter.LoadString(IDS_COMMONFILEFILTER);
						TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
						_tcscpy (pszFilters, sFilter);
						// Replace '|' delimeters with '\0's
						TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
						while (ptr != pszFilters)
						{
							if (*ptr == '|')
								*ptr = '\0';
							ptr--;
						} // while (ptr != pszFilters) 
						ofn.lpstrFilter = pszFilters;
						ofn.nFilterIndex = 1;
						// Display the Open dialog box. 
						CTSVNPath tempfile;
						if (GetSaveFileName(&ofn)==TRUE)
						{
							tempfile.SetFromWin(ofn.lpstrFile);
							SVN svn;
							if (!svn.Cat(CTSVNPath(filepath), rev, tempfile))
							{
								delete [] pszFilters;
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								theApp.DoWaitCursor(-1);
								break;
							} // if (!svn.Cat(m_path, rev, tempfile)) 
						} // if (GetSaveFileName(&ofn)==TRUE)
						delete [] pszFilters;
						GetDlgItem(IDOK)->EnableWindow(TRUE);
						theApp.DoWaitCursor(-1);
					}
					break;
				case ID_OPENWITH:
					bOpenWith = true;
				case ID_OPEN:
					{
						GetDlgItem(IDOK)->EnableWindow(FALSE);
						SetPromptApp(&theApp);
						theApp.DoWaitCursor(1);
						CString filepath;
						if (SVN::PathIsURL(m_path.GetSVNPathString()))
						{
							filepath = m_path.GetSVNPathString();
						}
						else
						{
							SVN svn;
							filepath = svn.GetURLFromPath(m_path);
							if (filepath.IsEmpty())
							{
								theApp.DoWaitCursor(-1);
								CString temp;
								temp.Format(IDS_ERR_NOURLOFFILE, filepath);
								CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
								TRACE(_T("could not retrieve the URL of the file!\n"));
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							}
						}
						filepath = GetRepositoryRoot(CTSVNPath(filepath));
						filepath += changedpath->sPath;

						CTSVNPath tempfile = CUtils::GetTempFilePath(CTSVNPath(filepath));
						m_tempFileList.AddPath(tempfile);
						SVN svn;
						if (!svn.Cat(CTSVNPath(filepath), rev, tempfile))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							theApp.DoWaitCursor(-1);
							break;
						}
						if (bOpenWith)
						{
							CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
							cmd += tempfile.GetWinPathString();
							CUtils::LaunchApplication(cmd, NULL, false);
						}
						else
							ShellExecute(this->m_hWnd, _T("open"), tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
						GetDlgItem(IDOK)->EnableWindow(TRUE);
						theApp.DoWaitCursor(-1);
					}
					break;
				case ID_LOG:
					{
						GetDlgItem(IDOK)->EnableWindow(FALSE);
						SetPromptApp(&theApp);
						theApp.DoWaitCursor(1);
						CString filepath;
						if (SVN::PathIsURL(m_path.GetSVNPathString()))
						{
							filepath = m_path.GetSVNPathString();
						}
						else
						{
							SVN svn;
							filepath = svn.GetURLFromPath(m_path);
							if (filepath.IsEmpty())
							{
								theApp.DoWaitCursor(-1);
								CString temp;
								temp.Format(IDS_ERR_NOURLOFFILE, filepath);
								CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
								TRACE(_T("could not retrieve the URL of the file!\n"));
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							}
						}
						filepath = GetRepositoryRoot(CTSVNPath(filepath));
						filepath += changedpath->sPath;
						
						CString sCmd;
						sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /rev:%ld"), CUtils::GetAppDirectory()+_T("TortoiseProc.exe"), filepath, rev);
						
						CUtils::LaunchApplication(sCmd, NULL, false);
						GetDlgItem(IDOK)->EnableWindow(TRUE);
						theApp.DoWaitCursor(-1);
					}
					break;
				default:
					break;
				} // switch (cmd)
			} // if (m_LogMsgCtrl.GetSelectedCount() == 1)
		} // if (popup.CreatePopupMenu())
	} // if (pWnd == &m_LogMsgCtrl) 
//#endregion

}

void CLogDlg::OnNMDblclkLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	int selIndex = pNMLV->iItem;
	if (selIndex >= 0)
	{
		selIndex = m_arShownList.GetAt(selIndex);
		CString temp;
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		SetPromptApp(&theApp);
		theApp.DoWaitCursor(1);
		if ((!m_path.IsDirectory())&&(m_hasWC))
		{
			long rev = m_arRevs.GetAt(selIndex);
			CTSVNPath tempfile = CUtils::GetTempFilePath(m_path);
			m_tempFileList.AddPath(tempfile);

			SVN svn;
			if (!svn.Cat(m_path, rev, tempfile))
			{
				CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
			}
			else
			{
				CString revname, wcname;
				revname.Format(_T("%s Revision %ld"), (LPCTSTR)m_path.GetFilename(), rev);
				wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)m_path.GetFilename());
				CUtils::StartExtDiff(tempfile, m_path, revname, wcname);
			}
		} 
		else
		{
			long rev = m_arRevs.GetAt(selIndex);
			this->m_bCancelled = FALSE;
			CTSVNPath tempfile = CUtils::GetTempFilePath(CTSVNPath(_T("Test.diff")));
			m_tempFileList.AddPath(tempfile);
			if (!PegDiff(m_path, (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), rev, TRUE, FALSE, TRUE, TRUE, _T(""), tempfile))
			{
				CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				if (CUtils::CheckForEmptyDiff(tempfile))
				{
					CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
				}
				else
				{
					if (m_hasWC)
					{
						CString sWC, sRev;
						sWC.LoadString(IDS_DIFF_WORKINGCOPY);
						sRev.Format(IDS_DIFF_REVISIONPATCHED, rev);
						CUtils::StartExtPatch(tempfile, m_path.GetDirectory(), sWC, sRev, TRUE);
					}
					else
					{
						CUtils::StartUnifiedDiffViewer(tempfile);
					}
				}
			}
		}
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
}

LRESULT CLogDlg::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_pFindDialog != NULL);

    if (m_pFindDialog->IsTerminating())
    {
	    // invalidate the handle identifying the dialog box.
        m_pFindDialog = NULL;
        return 0;
    }

    if(m_pFindDialog->FindNext())
    {
        //read data from dialog
        CString FindText = m_pFindDialog->GetFindString();
        bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
		bool bFound = false;
		bool bRegex = false;
		rpattern pat;
		try
		{
			pat.init( (LPCTSTR)FindText, MULTILINE | (bMatchCase ? NOFLAGS : NOCASE) );
			bRegex = true;
		}
		catch (bad_alloc) {}
		catch (bad_regexpr) {}

		int i;
		for (i = this->m_nSearchIndex; i<m_arShownList.GetCount()&&!bFound; i++)
		{
			if (bRegex)
			{
				match_results results;
				match_results::backref_type br;
				br = pat.match( (LPCTSTR)m_arLogMessages.GetAt(m_arShownList.GetAt(i)), results );
				if (br.matched)
				{
					bFound = true;
					break;
				}
				LogChangedPathArray * cpatharray = m_arLogPaths.GetAt(m_arShownList.GetAt(i));
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					br = pat.match( (LPCTSTR)cpath->sCopyFromPath, results);
					if (br.matched)
					{
						bFound = true;
						--i;
						break;
					}
					br = pat.match( (LPCTSTR)cpath->sPath, results);
					if (br.matched)
					{
						bFound = true;
						--i;
						break;
					}
				}
			}
			else
			{
				if (bMatchCase)
				{
					if (m_arLogMessages.GetAt(i).Find(FindText) >= 0)
					{
						bFound = true;
						break;
					}
					LogChangedPathArray * cpatharray = m_arLogPaths.GetAt(m_arShownList.GetAt(i));
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						if (cpath->sCopyFromPath.Find(FindText)>=0)
						{
							bFound = true;
							--i;
							break;
						}
						if (cpath->sPath.Find(FindText)>=0)
						{
							bFound = true;
							--i;
							break;
						}
					}
				}
				else
				{
					CString msg = m_arLogMessages.GetAt(m_arShownList.GetAt(i));
					msg = msg.MakeLower();
					CString find = FindText.MakeLower();
					if (msg.Find(find) >= 0)
					{
						bFound = TRUE;
						break;
					}
					LogChangedPathArray * cpatharray = m_arLogPaths.GetAt(m_arShownList.GetAt(i));
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						CString lowerpath = cpath->sCopyFromPath;
						lowerpath.MakeLower();
						if (lowerpath.Find(find)>=0)
						{
							bFound = TRUE;
							--i;
							break;
						}
						lowerpath = cpath->sPath;
						lowerpath.MakeLower();
						if (lowerpath.Find(find)>=0)
						{
							bFound = TRUE;
							--i;
							break;
						}
					}
				} 
			}
		} // for (i = this->m_nSearchIndex; i<m_arShownList.GetItemCount()&&!bFound; i++)
		if (bFound)
		{
			this->m_nSearchIndex = (i+1);
			m_LogList.EnsureVisible(i, FALSE);
			m_LogList.SetItemState(m_LogList.GetSelectionMark(), 0, LVIS_SELECTED);
			m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_LogList.SetSelectionMark(i);
			FillLogMessageCtrl(m_arLogMessages.GetAt(m_arShownList.GetAt(i)), m_arLogPaths.GetAt(m_arShownList.GetAt(i)));
			UpdateData(FALSE);
			m_nSearchIndex++;
			if (m_nSearchIndex >= m_arShownList.GetCount())
				m_nSearchIndex = (int)m_arShownList.GetCount()-1;
		}
    } // if(m_pFindDialog->FindNext()) 

    return 0;
}

void CLogDlg::OnOK()
{
	if (GetFocus() != GetDlgItem(IDOK))
		return;
	CString temp;
	CString buttontext;
	GetDlgItem(IDOK)->GetWindowText(buttontext);
	temp.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(buttontext) != 0)
		__super::OnOK();
	m_bCancelled = TRUE;
	if (m_pNotifyWindow)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if (selIndex >= 0)
		{	
			selIndex = m_arShownList.GetAt(selIndex);
			LONG lowerRev = m_arRevs.GetAt(selIndex);
			LONG higherRev = lowerRev;
			POSITION pos = m_LogList.GetFirstSelectedItemPosition();
			int index = 0;
			while (pos)
			{
				index = m_LogList.GetNextSelectedItem(pos);
				LONG rev = m_arRevs.GetAt(m_arShownList.GetAt(index));
				if (lowerRev > rev)
					lowerRev = rev;
				if (higherRev < rev)
					higherRev = rev;
			}
			m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART | MERGE_REVSELECTMINUSONE), lowerRev);
			m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND | MERGE_REVSELECTMINUSONE), higherRev);
		}
	}
}

void CLogDlg::OnNMDblclkLogmsg(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;
	int selIndex = m_LogMsgCtrl.GetSelectionMark();
	if (selIndex < 0)
		return;
	int s = m_LogList.GetSelectionMark();
	if (s < 0)
		return;
	s = m_arShownList.GetAt(s);
	long rev = m_arRevs.GetAt(s);
	LogChangedPath * changedpath = m_arLogPaths.GetAt(s)->GetAt(selIndex);

	if (DiffPossible(changedpath, rev))
	{
		DoDiffFromLog(selIndex, rev);
	}
}

void CLogDlg::DoDiffFromLog(int selIndex, long rev)
{
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	//get the filename
	CString filepath;
	if (SVN::PathIsURL(m_path.GetSVNPathString()))
	{
		filepath = m_path.GetSVNPathString();
	}
	else
	{
		SVN svn;
		filepath = svn.GetURLFromPath(m_path);
		if (filepath.IsEmpty())
		{
			theApp.DoWaitCursor(-1);
			CString temp;
			temp.Format(IDS_ERR_NOURLOFFILE, filepath);
			CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
			TRACE(_T("could not retrieve the URL of the file!\n"));
			GetDlgItem(IDOK)->EnableWindow(TRUE);
			theApp.DoWaitCursor(-11);
			return;		//exit
		}
	}
	m_bCancelled = FALSE;
	filepath = GetRepositoryRoot(CTSVNPath(filepath));

	int s = m_LogList.GetSelectionMark();
	s = m_arShownList.GetAt(s);
	LogChangedPath * changedpath = m_arLogPaths.GetAt(s)->GetAt(selIndex);

	CString firstfile = changedpath->sPath;
	CString secondfile = firstfile;
	long fromrev = rev - 1;

	if (changedpath->lCopyFromRev > 0) // is it an added file with history?
	{
		secondfile = changedpath->sCopyFromPath;
		fromrev = changedpath->lCopyFromRev;
	}

	firstfile = filepath + firstfile.Trim();
	secondfile = filepath + secondfile.Trim();

	StartDiff(CTSVNPath(firstfile), rev, CTSVNPath(secondfile), fromrev);
	theApp.DoWaitCursor(-1);
	GetDlgItem(IDOK)->EnableWindow(TRUE);
}

BOOL CLogDlg::StartDiff(const CTSVNPath& path1, LONG rev1, const CTSVNPath& path2, LONG rev2)
{
	CTSVNPath tempfile1 = CUtils::GetTempFilePath(path1);
	// The mere process of asking for a temp file creates it as a zero length file
	// We need to clean these up after us
	m_tempFileList.AddPath(tempfile1); 
	CTSVNPath tempfile2 = CUtils::GetTempFilePath(path2);
	m_tempFileList.AddPath(tempfile2);

	CProgressDlg progDlg;
	if (progDlg.IsValid())
	{
		progDlg.SetTitle(IDS_PROGRESSWAIT);
		progDlg.ShowModeless(this);
		progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)path1.GetUIPathString());
		progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, rev1);
	}
	m_bCancelled = FALSE;
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	if (!Cat(path1, rev1, tempfile1))
	{
		theApp.DoWaitCursor(-1);
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	}
	if (progDlg.IsValid())
	{
		progDlg.SetProgress(1, 2);
		progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)path2.GetUIPathString());
		progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, rev2);
	}

	if (!Cat(path2, rev2, tempfile2))
	{
		theApp.DoWaitCursor(-1);
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	}
	if (progDlg.IsValid())
	{
		progDlg.SetProgress(2,2);
		progDlg.Stop();
	}
	theApp.DoWaitCursor(-1);
	CString revname1, revname2;
	revname1.Format(_T("%s Revision %ld"), (LPCTSTR)path1.GetFileOrDirectoryName(), rev1);
	revname2.Format(_T("%s Revision %ld"), (LPCTSTR)path2.GetFileOrDirectoryName(), rev2);
	return CUtils::StartExtDiff(tempfile2, tempfile1, revname2, revname1);
}

void CLogDlg::EditAuthor(int index)
{
	CString url;
	CString name;
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	if (SVN::PathIsURL(m_path.GetSVNPathString()))
		url = m_path.GetSVNPathString();
	else
	{
		SVN svn;
		url = svn.GetURLFromPath(m_path);
	}
	name = SVN_PROP_REVISION_AUTHOR;

	CString value = RevPropertyGet(name, url, m_arRevs.GetAt(index));
	value.Replace(_T("\n"), _T("\r\n"));
	CInputDlg dlg;
	dlg.m_sHintText.LoadString(IDS_LOG_AUTHOR);
	dlg.m_sInputText = value;
	dlg.m_sTitle.LoadString(IDS_LOG_AUTHOREDITTITLE);
	if (dlg.DoModal() == IDOK)
	{
		dlg.m_sInputText.Replace(_T("\r"), _T(""));
		if (!RevPropertySet(name, dlg.m_sInputText, url, m_arRevs.GetAt(index)))
		{
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else
		{
			m_arAuthors.SetAt(index, dlg.m_sInputText);
			m_LogList.Invalidate();
		}
	}
	theApp.DoWaitCursor(-1);
	GetDlgItem(IDOK)->EnableWindow(TRUE);
}

void CLogDlg::EditLogMessage(int index)
{
	CString url;
	CString name;
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);
	if (SVN::PathIsURL(m_path.GetSVNPathString()))
		url = m_path.GetSVNPathString();
	else
	{
		SVN svn;
		url = svn.GetURLFromPath(m_path);
	}
	name = SVN_PROP_REVISION_LOG;

	CString value = RevPropertyGet(name, url, m_arRevs.GetAt(index));
	value.Replace(_T("\n"), _T("\r\n"));
	CInputDlg dlg;
	dlg.m_sHintText.LoadString(IDS_LOG_MESSAGE);
	dlg.m_sInputText = value;
	dlg.m_sTitle.LoadString(IDS_LOG_MESSAGEEDITTITLE);
	dlg.m_pProjectProperties = &m_ProjectProperties;
	if (dlg.DoModal() == IDOK)
	{
		dlg.m_sInputText.Replace(_T("\r"), _T(""));
		if (!RevPropertySet(name, dlg.m_sInputText, url, m_arRevs.GetAt(index)))
		{
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else
		{
			// Add as many characters from the log message to the list control
			// so it has a fixed width. If the log message is longer than
			// this predefined fixed with, add "..." as an indication.
			CString sShortMessage = dlg.m_sInputText;
			// Remove newlines 'cause those are not shown nicely in the listcontrol
			sShortMessage.Replace(_T("\r"), _T(""));
			sShortMessage.Replace('\n', ' ');

			int found = sShortMessage.Find(_T("\n\n"));
			if (found >=0)
			{
				if (found <=80)
					sShortMessage = sShortMessage.Left(found);
				else
				{
					found = sShortMessage.Find(_T("\n"));
					if ((found >= 0)&&(found <=80))
						sShortMessage = sShortMessage.Left(found);
				}
			}
			else if (sShortMessage.GetLength() > 80)
				sShortMessage = sShortMessage.Left(77) + _T("...");
			m_arShortLogMessages.SetAt(index, sShortMessage);
			//split multiline logentries and concatenate them
			//again but this time with \r\n as line separators
			//so that the edit control recognizes them
			if (dlg.m_sInputText.GetLength()>0)
			{
				m_sMessageBuf = dlg.m_sInputText;
				dlg.m_sInputText.Replace(_T("\n\r"), _T("\n"));
				dlg.m_sInputText.Replace(_T("\r\n"), _T("\n"));
				if (dlg.m_sInputText.Right(1).Compare(_T("\n"))==0)
					dlg.m_sInputText = dlg.m_sInputText.Left(dlg.m_sInputText.GetLength()-1);
			} 
			else
				dlg.m_sInputText.Empty();
			m_arLogMessages.SetAt(index, dlg.m_sInputText);
			CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
			pMsgView->SetWindowText(_T(" "));
			pMsgView->SetWindowText(dlg.m_sInputText);
			m_ProjectProperties.FindBugID(dlg.m_sInputText, pMsgView);
			m_LogList.Invalidate();
		}
	}
	theApp.DoWaitCursor(-1);
	GetDlgItem(IDOK)->EnableWindow(TRUE);
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (!GetDlgItem(IDC_GETALL)->IsWindowEnabled())
					return __super::PreTranslateMessage(pMsg);
				OnBnClickedGetall();
			}
			break;
		case 'F':
			{
				if (GetKeyState(VK_CONTROL)&0x8000)
				{
					if (m_pFindDialog)
					{
						break;
					}
					else
					{
						m_pFindDialog = new CFindReplaceDialog();
						m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
					}
				}
			}	
			break;
		case VK_F3:
			{
				if (m_pFindDialog)
				{
					break;
				}
				else
				{
					m_pFindDialog = new CFindReplaceDialog();
					m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
				}
			}
			break;
		default:
			break;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

BOOL CLogDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bThreadRunning)
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&
			((pWnd == GetDlgItem(IDC_LOGLIST))||
			(pWnd == GetDlgItem(IDC_MSGVIEW))||
			(pWnd == GetDlgItem(IDC_LOGMSG))))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CLogDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CLogDlg::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMLV->iItem >= 0)
	{
		int selIndex = pNMLV->iItem;
		m_nSearchIndex = selIndex;
		if (pNMLV->iSubItem != 0)
			return;
		if (pNMLV->uNewState & LVIS_SELECTED)
		{
			if (m_LogList.GetSelectedCount() > 1)
				FillLogMessageCtrl(_T(" "), NULL);
			else
				FillLogMessageCtrl(m_arLogMessages.GetAt(m_arShownList.GetAt(selIndex)), m_arLogPaths.GetAt(m_arShownList.GetAt(selIndex)));
			UpdateData(FALSE);
		}
	}
}

void CLogDlg::OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult)
{
	ENLINK *pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink->msg == WM_LBUTTONUP)
	{
		CString url, msg;
		GetDlgItem(IDC_MSGVIEW)->GetWindowText(msg);
		msg.Replace(_T("\r\n"), _T("\n"));
		url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax-pEnLink->chrg.cpMin);
		if (!::PathIsURL(url))
		{
			url = m_ProjectProperties.GetBugIDUrl(url);
		}
		if (!url.IsEmpty())
			ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
	}
	*pResult = 0;
}

void CLogDlg::OnBnClickedStatbutton()
{
	if (m_bThreadRunning)
		return;
	CStatGraphDlg dlg;
	dlg.m_parAuthors = &m_arAuthors;
	dlg.m_parDates = &m_arDates;
	dlg.m_parFileChanges = &m_arFileChanges;
	dlg.DoModal();
		
}

void CLogDlg::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

		if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			if (m_arCopies.GetAt(m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec)))
				crText = GetSysColor(COLOR_HIGHLIGHT);

			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
		}
	}
}

void CLogDlg::DoSizeV1(int delta)
{
	RemoveAnchor(IDC_LOGLIST);
	RemoveAnchor(IDC_SPLITTERTOP);
	RemoveAnchor(IDC_MSGVIEW);
	RemoveAnchor(IDC_SPLITTERBOTTOM);
	RemoveAnchor(IDC_LOGMSG);
	CSplitterControl::ChangeHeight(&m_LogList, delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), -delta, CW_BOTTOMALIGN);
	//CSplitterControl::ChangePos(&m_wndSplitter1, 0, delta);
	AddAnchor(IDC_LOGLIST, TOP_LEFT, ANCHOR(100, 40));
	AddAnchor(IDC_SPLITTERTOP, ANCHOR(0, 40), ANCHOR(100, 40));
	AddAnchor(IDC_MSGVIEW, ANCHOR(0, 40), ANCHOR(100, 90));
	AddAnchor(IDC_SPLITTERBOTTOM, ANCHOR(0, 90), ANCHOR(100, 90));
	AddAnchor(IDC_LOGMSG, ANCHOR(0, 90), BOTTOM_RIGHT);
	ArrangeLayout();
	SetSplitterRange();
	m_LogList.Invalidate();
	GetDlgItem(IDC_MSGVIEW)->Invalidate();
}

void CLogDlg::DoSizeV2(int delta)
{
	RemoveAnchor(IDC_LOGLIST);
	RemoveAnchor(IDC_SPLITTERTOP);
	RemoveAnchor(IDC_MSGVIEW);
	RemoveAnchor(IDC_SPLITTERBOTTOM);
	RemoveAnchor(IDC_LOGMSG);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(&m_LogMsgCtrl, -delta, CW_BOTTOMALIGN);
	//CSplitterControl::ChangePos(&m_wndSplitter2, 0, delta);
	AddAnchor(IDC_LOGLIST, TOP_LEFT, ANCHOR(100, 40));
	AddAnchor(IDC_SPLITTERTOP, ANCHOR(0, 40), ANCHOR(100, 40));
	AddAnchor(IDC_MSGVIEW, ANCHOR(0, 40), ANCHOR(100, 90));
	AddAnchor(IDC_SPLITTERBOTTOM, ANCHOR(0, 90), ANCHOR(100, 90));
	AddAnchor(IDC_LOGMSG, ANCHOR(0, 90), BOTTOM_RIGHT);
	ArrangeLayout();
	SetSplitterRange();
	GetDlgItem(IDC_MSGVIEW)->Invalidate();
	m_LogMsgCtrl.Invalidate();
}

LRESULT CLogDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_SPLITTERTOP)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSizeV1(pHdr->delta);
		}
		else if (wParam == IDC_SPLITTERBOTTOM)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSizeV2(pHdr->delta);
		}
		break;
	case WM_WINDOWPOSCHANGED : 
		{
			CRect rcW;
			GetWindowRect(rcW);
			ScreenToClient(rcW);

			SetSplitterRange();
			
			if (m_wndSplitter1 && rcW.Height()>0) Invalidate();
			if (m_wndSplitter2 && rcW.Height()>0) Invalidate();
			break;
		}
	case WM_SIZE:
		{
			// first, let the resizing take place
			LRESULT res = CResizableDialog::DefWindowProc(message, wParam, lParam);
			//set range
			SetSplitterRange();
			return res;
		}
	}

	return CResizableDialog::DefWindowProc(message, wParam, lParam);
}

void CLogDlg::SetSplitterRange()
{
	if ((m_LogList)&&(m_LogMsgCtrl))
	{
		CRect rcTop;
		m_LogList.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		GetDlgItem(IDC_MSGVIEW)->GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		m_wndSplitter1.SetRange(rcTop.top+20, rcMiddle.bottom-20);
		CRect rcBottom;
		m_LogMsgCtrl.GetWindowRect(rcBottom);
		ScreenToClient(rcBottom);
		m_wndSplitter2.SetRange(rcMiddle.top+20, rcBottom.bottom-20);
	}
}

void CLogDlg::OnStnClickedFiltericon()
{
	CRect rect;
	CPoint point;
	CString temp;
	GetDlgItem(IDC_FILTERICON)->GetWindowRect(rect);
	point = rect.CenterPoint();
#define LOGMENUFLAGS(x) (MF_STRING | MF_ENABLED | (m_nSelectedFilter == x ? MF_CHECKED : MF_UNCHECKED))
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		temp.LoadString(IDS_LOG_FILTER_ALL);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_ALL), LOGFILTER_ALL, temp);

		popup.AppendMenu(MF_SEPARATOR, NULL);
		
		temp.LoadString(IDS_LOG_FILTER_MESSAGES);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_MESSAGES), LOGFILTER_MESSAGES, temp);
		temp.LoadString(IDS_LOG_FILTER_PATHS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_PATHS), LOGFILTER_PATHS, temp);
		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_AUTHORS), LOGFILTER_AUTHORS, temp);
		int oldfilter = m_nSelectedFilter;
		m_nSelectedFilter = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		if (m_nSelectedFilter == 0)
			m_nSelectedFilter = oldfilter;
		SetFilterCueText();
		SetTimer(LOGFILTER_TIMER, 1000, NULL);
	}
}

void CLogDlg::SetFilterCueText()
{
	CString temp;
	switch (m_nSelectedFilter)
	{
	case LOGFILTER_ALL:
		temp.LoadString(IDS_LOG_FILTER_ALL);
		break;
	case LOGFILTER_MESSAGES:
		temp.LoadString(IDS_LOG_FILTER_MESSAGES);
		break;
	case LOGFILTER_PATHS:
		temp.LoadString(IDS_LOG_FILTER_PATHS);
		break;
	case LOGFILTER_AUTHORS:
		temp.LoadString(IDS_LOG_FILTER_AUTHORS);
		break;
	}
	// to make the cue banner text appear more to the right of the edit control
	temp = _T("   ")+temp;	
	::SendMessage(GetDlgItem(IDC_SEARCHEDIT)->GetSafeHwnd(), EM_SETCUEBANNER, 0, (LPARAM)(LPCTSTR)temp);
}

void CLogDlg::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	//Create a pointer to the item
	LV_ITEM* pItem= &(pDispInfo)->item;

	if ((m_bNoDispUpdates)||(m_bThreadRunning)||(pItem->iItem >= m_arShownList.GetCount()))
	{
		lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		*pResult = 0;
		return;
	}

	//Which item number?
	int itemid = m_arShownList.GetAt(pItem->iItem);

	//Do the list need text information?
	if (pItem->mask & LVIF_TEXT)
	{
		//Which column?
		switch (pItem->iSubItem)
		{
		case 0:	//revision
			_stprintf(pItem->pszText, _T("%ld"), m_arRevs.GetAt(itemid));
			break;
		case 1: //author
			lstrcpyn(pItem->pszText, m_arAuthors.GetAt(itemid), pItem->cchTextMax);
			break;
		case 2: //date
			lstrcpyn(pItem->pszText, m_arDateStrings.GetAt(itemid), pItem->cchTextMax);
			break;
		case 3: //message
			lstrcpyn(pItem->pszText, m_arShortLogMessages.GetAt(itemid), pItem->cchTextMax);
			break;
		}
	}

	*pResult = 0;
}

void CLogDlg::OnEnChangeSearchedit()
{
	UpdateData();
	if (m_sFilterText.IsEmpty())
	{
		// clear the filter, i.e. make all entries appear
		theApp.DoWaitCursor(1);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(_T(""), NULL);
		m_bNoDispUpdates = true;
		m_arShownList.RemoveAll();
		for (INT_PTR i=0; i<m_arLogMessages.GetCount(); ++i)
		{
			if (IsEntryInDateRange(i))
				m_arShownList.Add(i);
		}
		m_bNoDispUpdates = false;
		m_LogList.SetItemCountEx(m_arShownList.GetCount());
		m_LogList.RedrawItems(0, m_arShownList.GetCount());
		m_LogList.SetRedraw(false);
		int mincol = 0;
		int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
		int col;
		for (col = mincol; col <= maxcol; col++)
		{
			m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
		}
		m_LogList.SetRedraw(true);
		theApp.DoWaitCursor(-1);
		return;
	}
	SetTimer(LOGFILTER_TIMER, 1000, NULL);
}

void CLogDlg::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == LOGFILTER_TIMER)
	{
		if (m_bThreadRunning)
		{
			// thread still running! So just restart the timer.
			SetTimer(LOGFILTER_TIMER, 1000, NULL);
			return;
		}
		theApp.DoWaitCursor(1);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(_T(""), NULL);
		// now start filter the log list
		m_bNoDispUpdates = true;
		
		m_arShownList.RemoveAll();
		bool bRegex = false;
		rpattern pat;
		try
		{
			pat.init( (LPCTSTR)m_sFilterText, MULTILINE | NOCASE );
			bRegex = true;
		}
		catch (bad_alloc) {}
		catch (bad_regexpr) {}
		
		for (INT_PTR i=0; i<m_arLogMessages.GetCount(); ++i)
		{
			if (bRegex)
			{
				match_results results;
				match_results::backref_type br;
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
				{
					br = pat.match( (LPCTSTR)m_arLogMessages.GetAt(i), results );
					if ((br.matched)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(i);
						continue;
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_PATHS))
				{
					LogChangedPathArray * cpatharray = m_arLogPaths.GetAt(i);
					bool bGoing = true;
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount() && bGoing; ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						br = pat.match( (LPCTSTR)cpath->sCopyFromPath, results);
						if ((br.matched)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(i);
							bGoing = false;
							continue;
						}
						br = pat.match( (LPCTSTR)cpath->sPath, results);
						if ((br.matched)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(i);
							bGoing = false;
							continue;
						}
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_AUTHORS))
				{
					br = pat.match( (LPCTSTR)m_arAuthors.GetAt(i), results );
					if ((br.matched)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(i);
						continue;
					}
				}
			} // if (bRegex)
			else
			{
				CString find = m_sFilterText.MakeLower();
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
				{
					CString msg = m_arLogMessages.GetAt(i);
					msg = msg.MakeLower();
					if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(i);
						continue;
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_PATHS))
				{
					LogChangedPathArray * cpatharray = m_arLogPaths.GetAt(i);
					bool bGoing = true;
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount() && bGoing; ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						if ((cpath->sCopyFromPath.MakeLower().Find(find)>=0)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(i);
							bGoing = false;
							continue;
						}
						if ((cpath->sPath.MakeLower().Find(find)>=0)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(i);
							bGoing = false;
							continue;
						}
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_AUTHORS))
				{
					CString msg = m_arAuthors.GetAt(i);
					msg = msg.MakeLower();
					if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(i);
						continue;
					}
				}
			} // else (from if (bRegex))	
		} // for (INT_PTR i=0; i<m_arLogMessages.GetCount(); ++i)
		
		m_bNoDispUpdates = false;
		m_LogList.SetItemCountEx(m_arShownList.GetCount());
		m_LogList.RedrawItems(0, m_arShownList.GetCount());
		m_LogList.SetRedraw(false);
		int mincol = 0;
		int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
		int col;
		for (col = mincol; col <= maxcol; col++)
		{
			m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
		}
		m_LogList.SetRedraw(true);
		theApp.DoWaitCursor(-1);
	} // if (nIDEvent == LOGFILTER_TIMER)
	__super::OnTimer(nIDEvent);
}

void CLogDlg::OnDtnDatetimechangeDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	CTime time;
	m_DateTo.GetTime(time);
	if (time.GetTime() != m_tTo)
	{
		m_tTo = (DWORD)time.GetTime();
		SetTimer(LOGFILTER_TIMER, 10, NULL);
	}
	
	*pResult = 0;
}

void CLogDlg::OnDtnDatetimechangeDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	CTime time;
	m_DateFrom.GetTime(time);
	if (time.GetTime() != m_tFrom)
	{
		m_tFrom = (DWORD)time.GetTime();
		SetTimer(LOGFILTER_TIMER, 10, NULL);
	}
	
	*pResult = 0;
}

BOOL CLogDlg::IsEntryInDateRange(int i)
{
	DWORD time = m_arDates.GetAt(i);
	if ((time >= m_tFrom)&&(time <= m_tTo))
		return TRUE;
	return FALSE;
}












