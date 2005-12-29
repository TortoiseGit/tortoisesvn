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
#include "TempFile.h"
#include "InsertControl.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include ".\logdlg.h"

// CLogDlg dialog

IMPLEMENT_DYNAMIC(CLogDlg, CResizableStandAloneDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CLogDlg::IDD, pParent),
	m_startrev(0),
	m_LogRevision(0),
	m_endrev(0),
	m_logcounter(0),
	m_nSearchIndex(0),
	m_wParam(0),
	m_nSelectedFilter(LOGFILTER_ALL),
	m_bNoDispUpdates(false),
	m_currentChangedArray(NULL),
	m_nSortColumn(0),
	m_bShowedAll(false),
	m_bSelect(false),
	m_regLastStrict(_T("Software\\TortoiseSVN\\LastLogStrict"), FALSE)
{
	m_pFindDialog = NULL;
	m_bCancelled = FALSE;
	m_pNotifyWindow = NULL;
	m_bThreadRunning = false;
	m_bAscending = FALSE;
}

CLogDlg::~CLogDlg()
{
	m_bNoDispUpdates = true;
	m_logEntries.ClearAll();
	DestroyIcon(m_hModifiedIcon);
	DestroyIcon(m_hReplacedIcon);
	DestroyIcon(m_hAddedIcon);
	DestroyIcon(m_hDeletedIcon);
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
	DDX_Control(pDX, IDC_FILTERCANCEL, m_cFilterCancelButton);
	DDX_Control(pDX, IDC_FILTERICON, m_cFilterIcon);
	DDX_Control(pDX, IDC_HIDEPATHS, m_cHidePaths);
}

const UINT CLogDlg::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CLogDlg, CResizableStandAloneDialog)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, OnLvnKeydownLoglist)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage) 
	ON_BN_CLICKED(IDC_GETALL, OnBnClickedGetall)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGMSG, OnNMDblclkLogmsg)
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
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOGMSG, OnNMCustomdrawLogmsg)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LOGMSG, OnLvnGetdispinfoLogmsg)
	ON_BN_CLICKED(IDC_FILTERCANCEL, OnBnClickedFiltercancel)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LOGLIST, OnLvnColumnclick)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LOGMSG, OnLvnColumnclickLogmsg)
	ON_BN_CLICKED(IDC_HIDEPATHS, OnBnClickedHidepaths)
	ON_NOTIFY(LVN_ODFINDITEM, IDC_LOGLIST, OnLvnOdfinditemLoglist)
END_MESSAGE_MAP()



void CLogDlg::SetParams(const CTSVNPath& path, svn_revnum_t startrev, svn_revnum_t endrev, int limit, BOOL bStrict /* = FALSE */, BOOL bSaveStrict /* = TRUE */)
{
	m_path = path;
	m_startrev = startrev;
	m_LogRevision = startrev;
	m_endrev = endrev;
	m_hasWC = !path.IsUrl();
	m_bStrict = bStrict;
	m_bSaveStrict = bSaveStrict;
	m_limit = limit;
}

BOOL CLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_bStrict = m_regLastStrict;
	
	CUtils::CreateFontForLogs(m_logFont);
	GetDlgItem(IDC_MSGVIEW)->SetFont(&m_logFont);
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_AUTOURLDETECT, TRUE, NULL);
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK);
	m_LogList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES);

	m_cHidePaths.SetCheck(BST_INDETERMINATE);

	m_hModifiedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hAddedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	
	m_LogList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_LogList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_LOG_REVISION);
	m_LogList.InsertColumn(0, temp);
	
	// make the revision column right aligned
	LVCOLUMN Column;
	Column.mask = LVCF_FMT;
	Column.fmt = LVCFMT_RIGHT;
	m_LogList.SetColumn(0, &Column); 
	
	temp.LoadString(IDS_LOG_ACTIONS);
	m_LogList.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_AUTHOR);
	m_LogList.InsertColumn(2, temp);
	temp.LoadString(IDS_LOG_DATE);
	m_LogList.InsertColumn(3, temp);
	temp.LoadString(IDS_LOG_MESSAGE);
	m_LogList.InsertColumn(4, temp);
	m_LogList.SetRedraw(false);
	CUtils::ResizeAllListCtrlCols(&m_LogList);
	m_LogList.SetRedraw(true);

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
	CUtils::ResizeAllListCtrlCols(&m_LogMsgCtrl);
	m_LogMsgCtrl.SetRedraw(true);

	if (m_hasWC)
	{
		m_ProjectProperties.ReadProps(m_path);
	}

	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	m_logcounter = 0;
	m_sMessageBuf.Preallocate(100000);

	if (m_sTitle.IsEmpty())
		GetWindowText(m_sTitle);
	if(m_path.IsDirectory())
	{
		SetWindowText(m_sTitle + _T(" - ") + m_path.GetWinPathString());
	}
	else
	{
		SetWindowText(m_sTitle + _T(" - ") + m_path.GetFilename());
	}

	SetSplitterRange();
	
	m_cFilterCancelButton.LoadBitmaps(IDB_CANCELNORMAL, IDB_CANCELPRESSED, 0, 0);
	m_cFilterCancelButton.SizeToContent();
	m_cFilterIcon.LoadBitmaps(IDB_LOGFILTER);
	m_cFilterIcon.SizeToContent();
	
	AddAnchor(IDC_FROMLABEL, TOP_LEFT);
	AddAnchor(IDC_DATEFROM, TOP_LEFT);
	AddAnchor(IDC_TOLABEL, TOP_LEFT);
	AddAnchor(IDC_DATETO, TOP_LEFT);

	SetFilterCueText();
	AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);
	InsertControl(GetDlgItem(IDC_SEARCHEDIT)->GetSafeHwnd(), GetDlgItem(IDC_FILTERICON)->GetSafeHwnd(), INSERTCONTROL_LEFT);
	InsertControl(GetDlgItem(IDC_SEARCHEDIT)->GetSafeHwnd(), GetDlgItem(IDC_FILTERCANCEL)->GetSafeHwnd(), INSERTCONTROL_RIGHT);
	
	AddAnchor(IDC_LOGLIST, TOP_LEFT, ANCHOR(100, 40));
	AddAnchor(IDC_SPLITTERTOP, ANCHOR(0, 40), ANCHOR(100, 40));
	AddAnchor(IDC_MSGVIEW, ANCHOR(0, 40), ANCHOR(100, 90));
	AddAnchor(IDC_SPLITTERBOTTOM, ANCHOR(0, 90), ANCHOR(100, 90));
	AddAnchor(IDC_LOGMSG, ANCHOR(0, 90), BOTTOM_RIGHT);

	AddAnchor(IDC_HIDEPATHS, BOTTOM_LEFT, BOTTOM_RIGHT);	
	AddAnchor(IDC_CHECK_STOPONCOPY, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GETALL, BOTTOM_LEFT);
	AddAnchor(IDC_NEXTHUNDRED, BOTTOM_LEFT);
	AddAnchor(IDC_STATBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	SetPromptParentWindow(m_hWnd);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("LogDlg"));
	
	if (m_bSelect)
	{
		// the dialog is used to select revisions
		GetDlgItem(IDOK)->EnableWindow(m_LogList.GetSelectedCount()!=0);
	}
	else
	{
		// the dialog is used to just view log messages
		GetDlgItem(IDOK)->GetWindowText(temp);
		GetDlgItem(IDCANCEL)->SetWindowText(temp);
		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	}
	
	GetDlgItem(IDC_LOGLIST)->SetFocus();
	//first start a thread to obtain the log messages without
	//blocking the dialog
	m_tTo = 0;
	m_tFrom = (DWORD)-1;
	m_bThreadRunning = true;
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		m_bThreadRunning = false;
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return FALSE;
}

void CLogDlg::FillLogMessageCtrl(bool bShow /* = true*/)
{
	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	pMsgView->SetWindowText(_T(" "));
	m_LogMsgCtrl.SetRedraw(FALSE);
	m_bNoDispUpdates = true;
	m_currentChangedArray = NULL;
	m_LogMsgCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_LogMsgCtrl.DeleteAllItems();
	m_LogMsgCtrl.SetItemCountEx(0);

	if (!bShow)
	{
		// empty the log message control
		m_LogMsgCtrl.Invalidate();
		m_bNoDispUpdates = false;
		m_LogMsgCtrl.SetRedraw(TRUE);
		return;
	}

	int selCount = m_LogList.GetSelectedCount();
	if (selCount == 0)
	{
		m_bNoDispUpdates = false;
		m_LogMsgCtrl.SetRedraw(TRUE);
		return;
	}
	else if (selCount == 1)
	{
		POSITION pos = m_LogList.GetFirstSelectedItemPosition();
		int selIndex = m_LogList.GetNextSelectedItem(pos);
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(selIndex));

		pMsgView->SetWindowText(pLogEntry->sMessage);
		m_ProjectProperties.FindBugID(pLogEntry->sMessage, pMsgView);
		m_currentChangedArray = pLogEntry->pArChangedPaths;
		if (m_currentChangedArray == NULL)
		{
			m_bNoDispUpdates = false;
			m_LogMsgCtrl.SetRedraw(TRUE);
			return;
		}
		if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
		{
			m_CurrentFilteredChangedArray.RemoveAll();
			for (INT_PTR c = 0; c < m_currentChangedArray->GetCount(); ++c)
			{
				LogChangedPath * cpath = m_currentChangedArray->GetAt(c);
				if (cpath == NULL)
					continue;
				if (m_currentChangedArray->GetAt(c)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
				{
					m_CurrentFilteredChangedArray.Add(cpath);
				}
			}
			m_currentChangedArray = &m_CurrentFilteredChangedArray;
		}
	}
	else
	{
		m_currentChangedPathList = GetChangedPathsFromSelectedRevisions(true);
	}
	
	m_bNoDispUpdates = false;
	if (m_currentChangedArray)
	{
		m_LogMsgCtrl.SetItemCountEx(m_currentChangedArray->GetCount());
		m_LogMsgCtrl.RedrawItems(0, m_currentChangedArray->GetCount());
	}
	else if (m_currentChangedPathList.GetCount())
	{
		m_LogMsgCtrl.SetItemCountEx(m_currentChangedPathList.GetCount());
		m_LogMsgCtrl.RedrawItems(0, m_currentChangedPathList.GetCount());
	}
	else
	{
		m_LogMsgCtrl.SetItemCountEx(0);
		m_LogMsgCtrl.Invalidate();
	}
	CUtils::ResizeAllListCtrlCols(&m_LogMsgCtrl);
	SetSortArrow(&m_LogMsgCtrl, -1, false);
	m_LogMsgCtrl.SetRedraw(TRUE);
}

void CLogDlg::OnBnClickedGetall()
{
	UpdateData();

	m_LogMsgCtrl.SetItemCountEx(0);
	m_LogMsgCtrl.Invalidate();
	m_LogList.SetItemCountEx(0);
	m_LogList.Invalidate();
	m_bNoDispUpdates = true;
    CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	pMsgView->SetWindowText(_T(""));
    
    SetSortArrow(&m_LogList, -1, true);
    
	m_LogList.DeleteAllItems();
	m_arShownList.RemoveAll();
	m_logEntries.ClearAll();
	
	m_logcounter = 0;
	m_endrev = 1;
	m_startrev = m_LogRevision;
	m_bCancelled = FALSE;
	m_limit = 0;
	m_tTo = 0;
	m_tFrom = (DWORD)-1;
	m_bThreadRunning = true;
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		m_bThreadRunning = false;
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
	m_bNoDispUpdates = false;
	
	// TODO: once svn_client_log takes a peg revision, we must only set
	// m_bShowedAll to true if m_bStrict is false!
	m_bShowedAll = true;
}

void CLogDlg::Refresh()
{
	UpdateData();

	m_limit = 0;
	if (m_logEntries.size() != 0)
	{
		m_endrev = m_logEntries[m_logEntries.size()-1]->dwRev;
	}
	m_startrev = -1;
	m_bCancelled = FALSE;

	m_LogMsgCtrl.SetItemCountEx(0);
	m_LogMsgCtrl.Invalidate();
	m_LogList.SetItemCountEx(0);
	m_LogList.Invalidate();
	m_bNoDispUpdates = true;
	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	pMsgView->SetWindowText(_T(""));

	SetSortArrow(&m_LogList, -1, true);

	m_LogList.DeleteAllItems();
	m_arShownList.RemoveAll();
	m_logEntries.ClearAll();

	m_bThreadRunning = false;
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		m_bThreadRunning = false;
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
	m_bNoDispUpdates = false;
}

void CLogDlg::OnBnClickedNexthundred()
{
	UpdateData();
	// we have to fetch the next hundred log messages.
	if (m_logEntries.size() < 1)
	{
		// since there weren't any log messages fetched before, just
		// fetch all since we don't have an 'anchor' to fetch the 'next'
		// messages from.
		return OnBnClickedGetall();
	}
	LONG rev = m_logEntries[m_logEntries.size()-1]->dwRev - 1;
	if (rev < 1)
		return;		// do nothing! No more revisions to get
	m_startrev = rev;
	m_endrev = 1;
	m_bCancelled = FALSE;
	m_limit = 100;
	SetSortArrow(&m_LogList, -1, true);
	m_bThreadRunning = true;
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
	{
		m_bThreadRunning = false;
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
	if ((temp.Compare(temp2)==0)||(m_bThreadRunning))
	{
		m_bCancelled = true;
		return;
	}
	__super::OnCancel();
}

BOOL CLogDlg::Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions)
{
	int found = 0;
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
	__time64_t ttime = time/1000000L;
	if (m_tTo < (DWORD)ttime)
		m_tTo = (DWORD)ttime;
	if (m_tFrom > (DWORD)ttime)
		m_tFrom = (DWORD)ttime;
	// Add as many characters from the log message to the list control
	// so it has a fixed width. If the log message is longer than
	// this predefined fixed with, add "..." as an indication.
	CString sShortMessage = message;
	// Remove newlines 'cause those are not shown nicely in the listcontrol
	sShortMessage.Replace(_T("\r"), _T(""));
	
	found = sShortMessage.Find(_T("\n\n"));
	if (found >=0)
	{
		sShortMessage = sShortMessage.Left(found);
	}
	sShortMessage.Replace('\n', ' ');
	
	PLOGENTRYDATA pLogItem = new LOGENTRYDATA;
	pLogItem->bCopies = !!copies;
	pLogItem->tmDate = ttime;
	pLogItem->sAuthor = author;
	pLogItem->sDate = date;
	pLogItem->sShortMessage = sShortMessage;
	pLogItem->dwFileChanges = filechanges;
	pLogItem->actions = actions;
	
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
        pLogItem->sMessage = m_sMessageBuf;
        pLogItem->pArChangedPaths = cpaths;
        pLogItem->dwRev = rev;
	}
	catch (CException * e)
	{
		::MessageBox(NULL, _T("not enough memory!"), _T("TortoiseSVN"), MB_ICONERROR);
		e->Delete();
		m_bCancelled = TRUE;
	}
	m_logEntries.push_back(pLogItem);
	m_arShownList.Add(pLogItem);
	
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
	m_bThreadRunning = true;

	//disable the "Get All" button while we're receiving
	//log messages.
	GetDlgItem(IDC_GETALL)->EnableWindow(FALSE);
	GetDlgItem(IDC_NEXTHUNDRED)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_STOPONCOPY)->EnableWindow(FALSE);
	
	CString temp;
	temp.LoadString(IDS_MSGBOX_CANCEL);
	GetDlgItem(IDOK)->SetWindowText(temp);
	m_LogProgress.SetRange32(0, 100);
	m_LogProgress.SetPos(0);
	GetDlgItem(IDC_PROGRESS)->ShowWindow(TRUE);
	long r = -1;
	
	CTSVNPath rootpath;
	GetRootAndHead(m_path, rootpath, r);	
	m_sRepositoryRoot = rootpath.GetSVNPathString();
	CString sUrl = m_path.GetSVNPathString();
	if (!m_path.IsUrl())
	{
		sUrl = GetURLFromPath(m_path);
	}
	m_sRelativeRoot = sUrl.Mid(m_sRepositoryRoot.GetLength());
	
	m_LogProgress.SetPos(1);
	if (m_startrev == SVNRev::REV_HEAD)
	{
		m_startrev = r;
	}
	
	if (m_limit != 0)
	{
		m_limitcounter = m_limit;
		m_LogProgress.SetRange32(0, m_limit);
	}
	else
		m_LogProgress.SetRange32(m_endrev, m_startrev);
	
	if (!ReceiveLog(CTSVNPathList(m_path), m_startrev, m_endrev, m_limit, true, m_bStrict))
	{
		CMessageBox::Show(m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
	}
	m_LogList.SetItemCountEx(m_arShownList.GetCount());

	m_timFrom = (__time64_t(m_tFrom));
	m_timTo = (__time64_t(m_tTo));
	m_DateFrom.SetTime(&m_timFrom);
	m_DateTo.SetTime(&m_timTo);
	m_DateFrom.SetRange(&m_timFrom, &m_timTo);
	m_DateTo.SetRange(&m_timFrom, &m_timTo);

	GetDlgItem(IDC_GETALL)->EnableWindow(TRUE);
	
	if (!m_bShowedAll)
		GetDlgItem(IDC_NEXTHUNDRED)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_STOPONCOPY)->EnableWindow(TRUE);

	GetDlgItem(IDC_PROGRESS)->ShowWindow(FALSE);
	m_bCancelled = true;
	m_bThreadRunning = false;
	m_LogList.RedrawItems(0, m_arShownList.GetCount());
	m_LogList.SetRedraw(false);
	CUtils::ResizeAllListCtrlCols(&m_LogList);
	m_LogList.SetRedraw(true);
	temp.LoadString(IDS_MSGBOX_OK);
	GetDlgItem(IDOK)->SetWindowText(temp);
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	// make sure the filter is applied (if any) now, after we refreshed/fetched
	// the log messages
	PostMessage(WM_TIMER, LOGFILTER_TIMER);
	return 0;
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	if (pLVKeyDow->wVKey == 'C')
	{
		if (GetKeyState(VK_CONTROL)&0x8000)
		{
			//Ctrl-C -> copy to clipboard
			CopySelectionToClipBoard();
		}
	}
	*pResult = 0;
}

void CLogDlg::CopySelectionToClipBoard()
{
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
			CString sLogCopyText;
			CString sPaths;
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
			for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
			{
				LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
				sPaths += cpath->sAction + _T(" : ") + cpath->sPath;
				if (cpath->sCopyFromPath.IsEmpty())
					sPaths += _T("\r\n");
				else
				{
					CString sCopyFrom;
					sCopyFrom.Format(_T("(%s: %s, %s, %ld\r\n"), CString(MAKEINTRESOURCE(IDS_LOG_COPYFROM)), 
						cpath->sCopyFromPath, 
						CString(MAKEINTRESOURCE(IDS_LOG_REVISION)), 
						cpath->lCopyFromRev);
					sPaths += sCopyFrom;
				}
			}
			sLogCopyText.Format(_T("%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n"),
				(LPCTSTR)sRev, /*m_arRevs.GetAt(nItem)*/pLogEntry->dwRev,
				(LPCTSTR)sAuthor, (LPCTSTR)/*m_LogList.GetItemText(nItem, 1)*/pLogEntry->sAuthor,
				(LPCTSTR)sDate, (LPCTSTR)/*m_LogList.GetItemText(nItem, 2)*/pLogEntry->sDate,
				(LPCTSTR)sMessage, (LPCTSTR)/*m_arLogMessages.GetAt(nItem)*/pLogEntry->sMessage,
				(LPCTSTR)sPaths);
			sClipdata +=  CStringA(sLogCopyText);
		}
		CUtils::WriteAsciiStringToClipboard(sClipdata);
	}
}

BOOL CLogDlg::DiffPossible(LogChangedPath * changedpath, svn_revnum_t rev)
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
		if (selIndex < 0)
			return;
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			m_LogList.GetItemRect(selIndex, &rect, LVIR_LABEL);
			m_LogList.ClientToScreen(&rect);
			point = rect.CenterPoint();
		}
		m_nSearchIndex = selIndex;
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
						}
						temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
						popup.AppendMenu(MF_SEPARATOR, NULL);
						temp.LoadString(IDS_LOG_POPUP_SAVE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
						temp.LoadString(IDS_LOG_POPUP_OPEN);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPEN, temp);
						temp.LoadString(IDS_LOG_POPUP_OPENWITH);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPENWITH, temp);
					}
					else
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARE);
						if (m_hasWC)
						{
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
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
					temp.LoadString(IDS_MENUCHECKOUT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_CHECKOUT, temp);
					popup.AppendMenu(MF_SEPARATOR, NULL);
				}
				else if (m_LogList.GetSelectedCount() >= 2)
				{
					bool bAddSeparator = false;
					if (m_LogList.GetSelectedCount() == 2)
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARETWO, temp);
						temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF2, temp);
						bAddSeparator = true;
					}
					// reverting revisions only works (in one merge!) when the selected
					// revisions are continuous. So check first if that's the case before
					// we show the context menu.
					POSITION pos = m_LogList.GetFirstSelectedItemPosition();
					bool bContinuous = (m_arShownList.GetCount() == (INT_PTR)m_logEntries.size());
					if (bContinuous)
					{
						int itemindex = m_LogList.GetNextSelectedItem(pos);
						while (pos)
						{
							int nextindex = m_LogList.GetNextSelectedItem(pos);
							if (nextindex - itemindex > 1)
							{
								bContinuous = false;
								break;
							}
							itemindex = nextindex;
						}
					}
					temp.LoadString(IDS_LOG_POPUP_REVERTREVS);
					if ((m_hasWC)&&(bContinuous))
					{
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
						bAddSeparator = true;
					}
					if (bAddSeparator)
						popup.AppendMenu(MF_SEPARATOR, NULL);
				}
				
				if (m_LogList.GetSelectedCount() == 1)
				{
					temp.LoadString(IDS_LOG_POPUP_EDITAUTHOR);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITAUTHOR, temp);
					temp.LoadString(IDS_LOG_POPUP_EDITLOG);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITLOG, temp);
					popup.AppendMenu(MF_SEPARATOR, NULL);
				}
				if (m_LogList.GetSelectedCount() != 0)
				{
					temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPYCLIPBOARD, temp);
				}
				temp.LoadString(IDS_LOG_POPUP_FIND);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_FINDENTRY, temp);

				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				GetDlgItem(IDOK)->EnableWindow(FALSE);
				SetPromptApp(&theApp);
				theApp.DoWaitCursor(1);
				bool bOpenWith = false;
				switch (cmd)
				{
				case ID_GNUDIFF1:
					{
						PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
						long rev = pLogEntry->dwRev;
						this->m_bCancelled = FALSE;
						SVNDiff diff(this, this->m_hWnd, true);
						diff.SetHEADPeg(m_LogRevision);
						diff.ShowUnifiedDiff(m_path, rev-1, m_path, rev);
					}
					break;
				case ID_GNUDIFF2:
					{
						POSITION pos = m_LogList.GetFirstSelectedItemPosition();
						PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						long rev1 = pLogEntry->dwRev;
						pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						long rev2 = pLogEntry->dwRev;
						this->m_bCancelled = FALSE;

						SVNDiff diff(this, this->m_hWnd, true);
						diff.SetHEADPeg(m_LogRevision);
						diff.ShowUnifiedDiff(m_path, rev2, m_path, rev1);
					}
					break;
				case ID_REVERTREV:
					{
						POSITION pos = m_LogList.GetFirstSelectedItemPosition();
						PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						long rev = pLogEntry->dwRev;
						long revend = rev-1;
						while (pos)
						{
						    pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						    revend = pLogEntry->dwRev;
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
								dlg.SetPegRevision(m_LogRevision);
								dlg.DoModal();
							}
						}
					}
					break;
				case ID_COPY:
					{
						long rev = m_logEntries[selIndex]->dwRev;
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
								// should we show here a progress dialog? Copies are done really fast
								// and without much network traffic.
								SVN svn;
								if (!svn.Copy(CTSVNPath(url), CTSVNPath(dlg.m_URL), dlg.m_CopyRev, dlg.m_sLogMessage))
								{
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								}
								else
								{
									CMessageBox::Show(this->m_hWnd, IDS_LOG_COPY_SUCCESS, IDS_APPNAME, MB_ICONINFORMATION);
								}
							}
						}
					} 
					break;
				case ID_COMPARE:
					{
						//user clicked on the menu item "compare with working copy"
						//now first get the revision which is selected
						PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
						long rev = pLogEntry->dwRev;
						this->m_bCancelled = FALSE;
						SVNDiff diff(this, this->m_hWnd, true);
						diff.SetHEADPeg(m_LogRevision);
						diff.ShowCompare(m_path, SVNRev::REV_WC, m_path, rev);
					}
					break;
				case ID_COMPARETWO:
					{
						//user clicked on the menu item "compare revisions"
						POSITION pos = m_LogList.GetFirstSelectedItemPosition();
						PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						long rev1 = pLogEntry->dwRev;
						pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
						long rev2 = pLogEntry->dwRev;
						m_bCancelled = FALSE;

						SVNDiff diff(this, this->m_hWnd, true);
						CTSVNPath url = m_path;
						if (!PathIsURL(m_path.GetSVNPathString()))
						{
							url.SetFromSVN(GetURLFromPath(m_path));
						}
						diff.SetHEADPeg(m_LogRevision);
						diff.ShowCompare(url, rev2, url, rev1);
					}
					break;
				case ID_SAVEAS:
					{
						//now first get the revision which is selected
                        PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(selIndex));
                        long rev = pLogEntry->dwRev;
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
							_tcscpy_s(szFile, MAX_PATH, revFilename);
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
						_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
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
							CProgressDlg progDlg;
							progDlg.SetTitle(IDS_APPNAME);
							CString sInfoLine;
							sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LONG)rev);
							progDlg.SetLine(1, sInfoLine);
							svn.SetAndClearProgressInfo(&progDlg);
							progDlg.ShowModeless(m_hWnd);
							if (!svn.Cat(m_path, SVNRev(SVNRev::REV_HEAD), rev, tempfile))
							{
								progDlg.Stop();
								svn.SetAndClearProgressInfo((HWND)NULL);
								delete [] pszFilters;
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							}
							progDlg.Stop();
							svn.SetAndClearProgressInfo((HWND)NULL);
						}
						delete [] pszFilters;
					}
					break;
				case ID_OPENWITH:
					bOpenWith = true;
				case ID_OPEN:
					{
						//now first get the revision which is selected
						PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
                        long rev = pLogEntry->dwRev;
                        
						CProgressDlg progDlg;
						progDlg.SetTitle(IDS_APPNAME);
						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LONG)rev);
						progDlg.SetLine(1, sInfoLine);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);
						CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, m_path, rev);
						if (!Cat(m_path, SVNRev(SVNRev::REV_HEAD), rev, tempfile))
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							break;
						}
						else
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
							int ret = 0;
							if (!bOpenWith)
								ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
							if ((ret <= HINSTANCE_ERROR)||bOpenWith)
							{
								CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
								cmd += tempfile.GetWinPathString();
								CUtils::LaunchApplication(cmd, NULL, false);
							}
						}
					}
					break;
				case ID_UPDATE:
					{
						//now first get the revision which is selected
						PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
                        long rev = pLogEntry->dwRev;
						SVN svn;
						CProgressDlg progDlg;
						progDlg.SetTitle(IDS_APPNAME);
						progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
						progDlg.SetTime(false);
						svn.SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);
						if (!svn.Update(CTSVNPathList(m_path), rev, TRUE, FALSE))
						{
							progDlg.Stop();
							svn.SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							break;
						}
						progDlg.Stop();
						svn.SetAndClearProgressInfo((HWND)NULL);
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
					    PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
						long rev = pLogEntry->dwRev;
						CString url = m_path.GetSVNPathString();
						CString sCmd;
						sCmd.Format(_T("%s /command:repobrowser /path:\"%s\" /rev:%ld /notempfile"),
									CUtils::GetAppDirectory()+_T("TortoiseProc.exe"),
									url, rev);
						
						CUtils::LaunchApplication(sCmd, NULL, false);
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
				case ID_COPYCLIPBOARD:
				{
					CopySelectionToClipBoard();
				}
				break;
				case ID_CHECKOUT:
				{
					PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
					long rev = pLogEntry->dwRev;
					CString url = m_path.GetSVNPathString();
					CString sCmd;
					if (!SVN::PathIsURL(m_path.GetSVNPathString()))
					{
						url = GetURLFromPath(m_path);
					}
					url = _T("tsvn:")+url;
					sCmd.Format(_T("%s /command:checkout /url:\"%s\" /revision:%ld /notempfile"),
						CUtils::GetAppDirectory()+_T("TortoiseProc.exe"),
						url, rev);
					CUtils::LaunchApplication(sCmd, NULL, false);
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
		if (m_currentChangedArray == NULL)
			return;		// no commands for multiple revisions selected!
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(s));
		long rev = pLogEntry->dwRev;
		LogChangedPath * changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
		
		if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
		{
			// some items are hidden! So find out which item the user really clicked on
			INT_PTR selRealIndex = -1;
			for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
			{
				if (pLogEntry->pArChangedPaths->GetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
					selRealIndex++;
				if (selRealIndex == selIndex)
				{
					selIndex = hiddenindex;
					changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
					break;
				}
			}
		}
		
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
							sUrl = GetURLFromPath(m_path);
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
							CString sAction;
							sAction.LoadString(IDS_SVNACTION_DELETE);
							if (changedpath->sAction.Compare(sAction)==0)
							{
								// a deleted path! Since the path isn't there anymore, merge
								// won't work. So just do a copy url->wc
								CSVNProgressDlg dlg;
								dlg.SetParams(CSVNProgressDlg::Copy, 0, CTSVNPathList(CTSVNPath(fileURL)), wcPath, _T(""), rev-1);
								dlg.DoModal();
							}
							else
							{
								CSVNProgressDlg dlg;
								dlg.SetParams(CSVNProgressDlg::Enum_Merge, 0, CTSVNPathList(CTSVNPath(wcPath)), fileURL, fileURL, rev);		//use the message as the second url
								dlg.m_RevisionEnd = rev-1;
								dlg.DoModal();
							}
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
							filepath = GetURLFromPath(m_path);
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
							filepath = GetURLFromPath(m_path);
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
						_tcscpy_s(szFile, MAX_PATH, revFilename);
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
						_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
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
							CString sAction(MAKEINTRESOURCE(IDS_SVNACTION_DELETE));
							SVNRev getrev = (sAction.Compare(changedpath->sAction)==0) ? rev-1 : rev;

							CProgressDlg progDlg;
							progDlg.SetTitle(IDS_APPNAME);
							CString sInfoLine;
							sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, filepath, (LONG)getrev);
							progDlg.SetLine(1, sInfoLine);
							SetAndClearProgressInfo(&progDlg);
							progDlg.ShowModeless(m_hWnd);

							if (!Cat(CTSVNPath(filepath), getrev, getrev, tempfile))
							{
								progDlg.Stop();
								SetAndClearProgressInfo((HWND)NULL);
								delete [] pszFilters;
								CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								theApp.DoWaitCursor(-1);
								break;
							}
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
						}
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
							filepath = GetURLFromPath(m_path);
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

						CProgressDlg progDlg;
						progDlg.SetTitle(IDS_APPNAME);
						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, filepath, (LONG)rev);
						progDlg.SetLine(1, sInfoLine);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);

						CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, CTSVNPath(filepath), rev);
						if (!Cat(CTSVNPath(filepath), SVNRev(rev), rev, tempfile))
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							theApp.DoWaitCursor(-1);
							break;
						}
						progDlg.Stop();
						SetAndClearProgressInfo((HWND)NULL);
						SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
						if (!bOpenWith)
						{
							int ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
							if (ret <= HINSTANCE_ERROR)
								bOpenWith = true;
						}
						if (bOpenWith)
						{
							CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
							cmd += tempfile.GetWinPathString();
							CUtils::LaunchApplication(cmd, NULL, false);
						}
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
							filepath = GetURLFromPath(m_path);
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
						sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /revstart:%ld"), CUtils::GetAppDirectory()+_T("TortoiseProc.exe"), filepath, rev);
						
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
				PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));

				br = pat.match( restring((LPCTSTR)pLogEntry->sMessage), results );
				if (br.matched)
				{
					bFound = true;
					break;
				}
				LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					br = pat.match( restring ((LPCTSTR)cpath->sCopyFromPath), results);
					if (br.matched)
					{
						bFound = true;
						--i;
						break;
					}
					br = pat.match( restring ((LPCTSTR)cpath->sPath), results);
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
					if (m_logEntries[i]->sMessage.Find(FindText) >= 0)
					{
						bFound = true;
						break;
					}
					PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
					LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
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
				    PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
					CString msg = pLogEntry->sMessage;
					msg = msg.MakeLower();
					CString find = FindText.MakeLower();
					if (msg.Find(find) >= 0)
					{
						bFound = TRUE;
						break;
					}
					LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
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
			FillLogMessageCtrl();
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
		    PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetSelectionMark()));
			LONG lowerRev = pLogEntry->dwRev;
			LONG higherRev = lowerRev;
			POSITION pos = m_LogList.GetFirstSelectedItemPosition();
			//int index = 0;
			while (pos)
			{
			    pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
				LONG rev = pLogEntry->dwRev;
				if (lowerRev > rev)
					lowerRev = rev;
				if (higherRev < rev)
					higherRev = rev;
			}
			m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART | MERGE_REVSELECTMINUSONE), lowerRev);
			m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND | MERGE_REVSELECTMINUSONE), higherRev);
		}
	}
	UpdateData();
	if (m_bSaveStrict)
		m_regLastStrict = m_bStrict;
}

void CLogDlg::OnNMDblclkLogmsg(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;
	if (m_bThreadRunning)
		return;
	int selIndex = m_LogMsgCtrl.GetSelectionMark();
	if (selIndex < 0)
		return;
	int s = m_LogList.GetSelectionMark();
	if (s < 0)
		return;
	if (m_currentChangedArray == NULL)
		return;
	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(s));
	long rev = pLogEntry->dwRev;
	LogChangedPath * changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);

	if ((m_cHidePaths.GetState() & 0x0003)==BST_CHECKED)
	{
		// some items are hidden! So find out which item the user really clicked on
		int selRealIndex = -1;
		for (INT_PTR hiddenindex=0; hiddenindex<pLogEntry->pArChangedPaths->GetCount(); ++hiddenindex)
		{
			if (pLogEntry->pArChangedPaths->GetAt(hiddenindex)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
				selRealIndex++;
			if (selRealIndex == selIndex)
			{
				selIndex = hiddenindex;
				changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);
				break;
			}
		}
	}


	if (DiffPossible(changedpath, rev))
	{
		DoDiffFromLog(selIndex, rev);
	}
}

void CLogDlg::DoDiffFromLog(int selIndex, svn_revnum_t rev)
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
		filepath = GetURLFromPath(m_path);
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
	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(s));
	LogChangedPath * changedpath = pLogEntry->pArChangedPaths->GetAt(selIndex);

	CString firstfile = changedpath->sPath;
	CString secondfile = firstfile;
	svn_revnum_t fromrev = rev - 1;

	if (changedpath->lCopyFromRev > 0) // is it an added file with history?
	{
		secondfile = changedpath->sCopyFromPath;
		fromrev = changedpath->lCopyFromRev;
	}

	firstfile = filepath + firstfile.Trim();
	secondfile = filepath + secondfile.Trim();

	SVNDiff diff(this, this->m_hWnd, true);
	diff.SetHEADPeg(m_LogRevision);
	diff.ShowCompare(CTSVNPath(secondfile), fromrev, CTSVNPath(firstfile), rev);

	theApp.DoWaitCursor(-1);
	GetDlgItem(IDOK)->EnableWindow(TRUE);
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
		url = GetURLFromPath(m_path);
	}
	name = SVN_PROP_REVISION_AUTHOR;

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(index));
	CString value = RevPropertyGet(name, url, pLogEntry->dwRev);
	value.Replace(_T("\n"), _T("\r\n"));
	CInputDlg dlg;
	dlg.m_sHintText.LoadString(IDS_LOG_AUTHOR);
	dlg.m_sInputText = value;
	dlg.m_sTitle.LoadString(IDS_LOG_AUTHOREDITTITLE);
	if (dlg.DoModal() == IDOK)
	{
		dlg.m_sInputText.Replace(_T("\r"), _T(""));
		if (!RevPropertySet(name, dlg.m_sInputText, url, pLogEntry->dwRev))
		{
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		else
		{
			pLogEntry->sAuthor = dlg.m_sInputText;
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
		url = GetURLFromPath(m_path);
	}
	name = SVN_PROP_REVISION_LOG;

	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(index));
	CString value = RevPropertyGet(name, url, pLogEntry->dwRev);
	value.Replace(_T("\n"), _T("\r\n"));
	CInputDlg dlg;
	dlg.m_sHintText.LoadString(IDS_LOG_MESSAGE);
	dlg.m_sInputText = value;
	dlg.m_sTitle.LoadString(IDS_LOG_MESSAGEEDITTITLE);
	dlg.m_pProjectProperties = &m_ProjectProperties;
	if (dlg.DoModal() == IDOK)
	{
		dlg.m_sInputText.Replace(_T("\r"), _T(""));
		if (!RevPropertySet(name, dlg.m_sInputText, url, pLogEntry->dwRev))
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
			pLogEntry->sShortMessage = sShortMessage;
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
			pLogEntry->sMessage = dlg.m_sInputText;
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
				Refresh();
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
	if (m_bThreadRunning)
		return;
	if (m_bSelect)
	{
		GetDlgItem(IDOK)->EnableWindow(m_LogList.GetSelectedCount()!=0);
	}
	if (pNMLV->iItem >= 0)
	{
		m_nSearchIndex = pNMLV->iItem;
		if (pNMLV->iSubItem != 0)
			return;
		if (pNMLV->uChanged & LVIF_STATE)
		{
			FillLogMessageCtrl();
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
	if (m_arShownList.IsEmpty())
		return;		// nothing is shown, so no statistics.
	// create arrays which are aware of the current filter
	CStringArray m_arAuthorsFiltered;
	CDWordArray m_arDatesFiltered;
	CDWordArray m_arFileChangesFiltered;
	for (INT_PTR i=0; i<m_arShownList.GetCount(); ++i)
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
		m_arAuthorsFiltered.Add(pLogEntry->sAuthor);
		m_arDatesFiltered.Add(static_cast<DWORD>(pLogEntry->tmDate));
		m_arFileChangesFiltered.Add(pLogEntry->dwFileChanges);
	}
	CStatGraphDlg dlg;
	dlg.m_parAuthors = &m_arAuthorsFiltered;
	dlg.m_parDates = &m_arDatesFiltered;
	dlg.m_parFileChanges = &m_arFileChangesFiltered;
	dlg.DoModal();
		
}

void CLogDlg::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT:
		{
			// This is the prepaint stage for an item. Here's where we set the
			// item's text color. 
			
			// Tell Windows to send draw notifications for each subitem.
			*pResult = CDRF_NOTIFYSUBITEMDRAW;

			COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

			if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
			{
				if (((PLOGENTRYDATA)m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec))->bCopies)
					crText = m_Colors.GetColor(CColors::Modified);
			}
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT|CDDS_ITEM|CDDS_SUBITEM:
		{
			if (pLVCD->iSubItem == 1)
			{
				*pResult = CDRF_DODEFAULT;

				int		nIcons = 0;
				int		iconwidth = ::GetSystemMetrics(SM_CXSMICON);
				int		iconheigth = ::GetSystemMetrics(SM_CYSMICON);

				PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec));

				// Get the selected state of the
				// item being drawn.
				// Even though the state should be in pLVCD->nmcd.state, that
				// variable always contained 0x201 when I tested it.
				LVITEM   rItem;
				ZeroMemory(&rItem, sizeof(LVITEM));
				rItem.mask  = LVIF_STATE;
				rItem.iItem = pLVCD->nmcd.dwItemSpec;
				rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
				m_LogList.GetItem(&rItem);

				// Fill the background
				HBRUSH brush;
				if (rItem.state & LVIS_SELECTED)
				{
					if (::GetFocus() == m_LogList.m_hWnd)
						brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
					else
						brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
				}
				else
					brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
				if (brush == NULL)
				{
					return;
				}
				CRect rect;
				m_LogList.GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);
				::FillRect(pLVCD->nmcd.hdc, &rect, brush);
				::DeleteObject(brush);
#				define ICONITEMBORDER 5
				// Draw the icon(s) into the compatible DC
				if (pLogEntry->actions & LOGACTIONS_MODIFIED)
				{
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheigth, 0, NULL, DI_NORMAL);
					nIcons++;
				}
				if (pLogEntry->actions & LOGACTIONS_REPLACED)
				{
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hReplacedIcon, iconwidth, iconheigth, 0, NULL, DI_NORMAL);
					nIcons++;
				}
				if (pLogEntry->actions & LOGACTIONS_ADDED)
				{
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hAddedIcon, iconwidth, iconheigth, 0, NULL, DI_NORMAL);
					nIcons++;
				}
				if (pLogEntry->actions & LOGACTIONS_DELETED)
				{
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hDeletedIcon, iconwidth, iconheigth, 0, NULL, DI_NORMAL);
					nIcons++;
				}
				*pResult = CDRF_SKIPDEFAULT;
				return;
			}
		}
		break;
	}
	*pResult = CDRF_DODEFAULT;
}
void CLogDlg::OnNMCustomdrawLogmsg(NMHDR *pNMHDR, LRESULT *pResult)
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

		if ((m_cHidePaths.GetState() & 0x0003)==BST_INDETERMINATE)
		{
			if ((m_currentChangedArray)&&((m_currentChangedArray->GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)))
			{
				if (m_currentChangedArray->GetAt(pLVCD->nmcd.dwItemSpec)->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
					crText = GetSysColor(COLOR_GRAYTEXT);
			}
			else if (m_currentChangedPathList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
			{
				if (m_currentChangedPathList[pLVCD->nmcd.dwItemSpec].GetSVNPathString().Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
					crText = GetSysColor(COLOR_GRAYTEXT);
			}
		}
		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = crText;
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
	point = CPoint(rect.left, rect.bottom);
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
		temp.LoadString(IDS_LOG_FILTER_REVS);
		popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REVS), LOGFILTER_REVS, temp);
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
	DWORD itemid = pItem->iItem;
	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(pItem->iItem));
    
	//Do the list need text information?
	if (pItem->mask & LVIF_TEXT)
	{
		//Which column?
		switch (pItem->iSubItem)
		{
		case 0:	//revision
			if (itemid < m_logEntries.size())
				_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), pLogEntry->dwRev);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 1: //action
			lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 2: //author
			if (itemid < m_logEntries.size())
				pItem->pszText = const_cast<LPWSTR>((LPCTSTR)pLogEntry->sAuthor);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 3: //date
			if (itemid < m_logEntries.size())
				pItem->pszText = const_cast<LPWSTR>((LPCTSTR)pLogEntry->sDate);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 4: //message
			if (itemid < m_logEntries.size())
			{
				pItem->pszText = const_cast<LPWSTR>((LPCTSTR)pLogEntry->sShortMessage);
				pItem->cchTextMax = pLogEntry->sShortMessage.GetLength();
			}
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		}
	}
	*pResult = 0;
}

void CLogDlg::OnLvnGetdispinfoLogmsg(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	//Create a pointer to the item
	LV_ITEM* pItem= &(pDispInfo)->item;

	*pResult = 0;
	if ((m_bNoDispUpdates)||(m_bThreadRunning))
	{
		lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}
	if ((m_currentChangedArray!=NULL)&&(pItem->iItem >= m_currentChangedArray->GetCount()))
	{
		lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}
	if ((m_currentChangedArray==NULL)&&(pItem->iItem >= m_currentChangedPathList.GetCount()))
	{
		lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
		return;
	}
	LogChangedPath * lcpath = NULL;
	if (m_currentChangedArray)
		lcpath = m_currentChangedArray->GetAt(pItem->iItem);
	//Does the list need text information?
	if (pItem->mask & LVIF_TEXT)
	{
		//Which column?
		switch (pItem->iSubItem)
		{
		case 0:	//Action
			if (lcpath)
				pItem->pszText = const_cast<LPWSTR>((LPCTSTR)lcpath->sAction);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);				
			break;
		case 1: //path
			if (lcpath)
				pItem->pszText = const_cast<LPWSTR>((LPCTSTR)lcpath->sPath);
			else
				pItem->pszText = const_cast<LPWSTR>((LPCTSTR)m_currentChangedPathList[pItem->iItem].GetSVNPathString());
			break;
		case 2: //copyfrom path
			if (lcpath)
				pItem->pszText = const_cast<LPWSTR>((LPCTSTR)lcpath->sCopyFromPath);
			else
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			break;
		case 3: //revision
			if ((lcpath==NULL)||(lcpath->sCopyFromPath.IsEmpty()))
				lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
			else
				_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), lcpath->lCopyFromRev);
			break;
		}
	}

	*pResult = 0;
}

void CLogDlg::OnBnClickedFiltercancel()
{
	KillTimer(LOGFILTER_TIMER);
	int selIndex = m_LogList.GetSelectionMark();
	
	m_sFilterText.Empty();
	UpdateData(FALSE);
	theApp.DoWaitCursor(1);
	FillLogMessageCtrl(false);
	m_bNoDispUpdates = true;
	m_arShownList.RemoveAll();

	// reset the time filter too
	m_timFrom = (__time64_t(m_tFrom));
	m_timTo = (__time64_t(m_tTo));
	m_DateFrom.SetTime(&m_timFrom);
	m_DateTo.SetTime(&m_timTo);
	m_DateFrom.SetRange(&m_timFrom, &m_timTo);
	m_DateTo.SetRange(&m_timFrom, &m_timTo);

	for (DWORD i=0; i<m_logEntries.size(); ++i)
	{
		m_arShownList.Add(m_logEntries[i]);
	}
	m_bNoDispUpdates = false;
	m_LogList.SetItemCountEx(m_arShownList.GetCount());
	m_LogList.RedrawItems(0, m_arShownList.GetCount());
	m_LogList.SetRedraw(false);
	CUtils::ResizeAllListCtrlCols(&m_LogList);
	
	if (selIndex >= 0)
	{
		// restore the previous selected log message
		m_LogList.SetSelectionMark(selIndex);
		m_LogList.SetItemState(selIndex, LVIS_SELECTED, LVIS_SELECTED);
		m_LogList.EnsureVisible(selIndex, FALSE);
	}
	
	m_LogList.SetRedraw(true);
	theApp.DoWaitCursor(-1);
	m_cFilterCancelButton.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
	return;	
}

void CLogDlg::OnEnChangeSearchedit()
{
	UpdateData();
	if (m_sFilterText.IsEmpty())
	{
		// clear the filter, i.e. make all entries appear
		theApp.DoWaitCursor(1);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(false);
		m_bNoDispUpdates = true;
		m_arShownList.RemoveAll();
		for (DWORD i=0; i<m_logEntries.size(); ++i)
		{
			if (IsEntryInDateRange(i))
				m_arShownList.Add(m_logEntries[i]);
		}
		m_bNoDispUpdates = false;
		m_LogList.SetItemCountEx(m_arShownList.GetCount());
		m_LogList.RedrawItems(0, m_arShownList.GetCount());
		m_LogList.SetRedraw(false);
		CUtils::ResizeAllListCtrlCols(&m_LogList);
		m_LogList.SetRedraw(true);
		theApp.DoWaitCursor(-1);
		m_cFilterCancelButton.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
		GetDlgItem(IDC_STATBUTTON)->EnableWindow(!(((m_bThreadRunning)||(m_arShownList.IsEmpty()))));
		return;
	}
	SetTimer(LOGFILTER_TIMER, 1000, NULL);
}

void CLogDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == LOGFILTER_TIMER)
	{
		if (m_bThreadRunning)
		{
			// thread still running! So just restart the timer.
			SetTimer(LOGFILTER_TIMER, 1000, NULL);
			return;
		}
		if (m_sFilterText.IsEmpty())
		{
			GetDlgItem(IDC_STATBUTTON)->EnableWindow(!(((m_bThreadRunning)||(m_arShownList.IsEmpty()))));
			return;
		}
		theApp.DoWaitCursor(1);
		KillTimer(LOGFILTER_TIMER);
		FillLogMessageCtrl(false);
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
		
		CString sRev;
		for (DWORD i=0; i<m_logEntries.size(); ++i)
		{
			if (bRegex)
			{
				match_results results;
				match_results::backref_type br;
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
				{
					br = pat.match( restring ((LPCTSTR)m_logEntries[i]->sMessage), results );
					if ((br.matched)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(m_logEntries[i]);
						continue;
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_PATHS))
				{
					LogChangedPathArray * cpatharray = m_logEntries[i]->pArChangedPaths;
					
					bool bGoing = true;
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount() && bGoing; ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						br = pat.match( restring ((LPCTSTR)cpath->sCopyFromPath), results);
						if ((br.matched)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(m_logEntries[i]);
							bGoing = false;
							continue;
						}
						br = pat.match( restring ((LPCTSTR)cpath->sPath), results);
						if ((br.matched)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(m_logEntries[i]);
							bGoing = false;
							continue;
						}
						br = pat.match( restring ((LPCTSTR)cpath->sAction), results);
						if ((br.matched)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(m_logEntries[i]);
							bGoing = false;
							continue;
						}
					}
					if (!bGoing)
						continue;
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_AUTHORS))
				{
					br = pat.match( restring ((LPCTSTR)m_logEntries[i]->sAuthor), results );
					if ((br.matched)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(m_logEntries[i]);
						continue;
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_REVS))
				{
					sRev.Format(_T("%ld"), m_logEntries[i]->dwRev);
					br = pat.match( restring ((LPCTSTR)sRev), results );
					if ((br.matched)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(m_logEntries[i]);
						continue;
					}
				}
			} // if (bRegex)
			else
			{
				CString find = m_sFilterText.MakeLower();
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
				{
					CString msg = m_logEntries[i]->sMessage;
					
					msg = msg.MakeLower();
					if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(m_logEntries[i]);
						continue;
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_PATHS))
				{
					LogChangedPathArray * cpatharray = m_logEntries[i]->pArChangedPaths;
					
					bool bGoing = true;
					for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount() && bGoing; ++cpPathIndex)
					{
						LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
						CString path = cpath->sCopyFromPath;
						path.MakeLower();
						if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(m_logEntries[i]);
							bGoing = false;
							continue;
						}
						path = cpath->sPath;
						path.MakeLower();
						if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(m_logEntries[i]);
							bGoing = false;
							continue;
						}
						path = cpath->sAction;
						path.MakeLower();
						if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
						{
							m_arShownList.Add(m_logEntries[i]);
							bGoing = false;
							continue;
						}
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_AUTHORS))
				{
					CString msg = m_logEntries[i]->sAuthor;
					msg = msg.MakeLower();
					if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(m_logEntries[i]);
						continue;
					}
				}
				if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_REVS))
				{
					sRev.Format(_T("%ld"), m_logEntries[i]->dwRev);
					if ((sRev.Find(find) >= 0)&&(IsEntryInDateRange(i)))
					{
						m_arShownList.Add(m_logEntries[i]);
						continue;
					}
				}
			} // else (from if (bRegex))	
		} // for (INT_PTR i=0; i<m_logEntries.size(); ++i)
		
		m_bNoDispUpdates = false;
		m_LogList.SetItemCountEx(m_arShownList.GetCount());
		m_LogList.RedrawItems(0, m_arShownList.GetCount());
		m_LogList.SetRedraw(false);
		CUtils::ResizeAllListCtrlCols(&m_LogList);
		m_LogList.SetRedraw(true);
		m_LogList.Invalidate();
		theApp.DoWaitCursor(-1);
		m_cFilterCancelButton.ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
	} // if (nIDEvent == LOGFILTER_TIMER)
	GetDlgItem(IDC_STATBUTTON)->EnableWindow(!(((m_bThreadRunning)||(m_arShownList.IsEmpty()))));
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
	__time64_t time = m_logEntries[i]->tmDate;
	if ((time >= m_tFrom)&&(time <= m_tTo))
		return TRUE;
	return FALSE;
}

CTSVNPathList CLogDlg::GetChangedPathsFromSelectedRevisions(bool bRelativePaths /* = false */, bool bUseFilter /* = true */)
{
	CTSVNPathList pathList;
	if (m_sRepositoryRoot.IsEmpty() && (bRelativePaths == false))
	{
		m_sRepositoryRoot = GetRepositoryRoot(m_path);
	}
	if (m_sRepositoryRoot.IsEmpty() && (bRelativePaths == false))
		return pathList;
	
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		while (pos)
		{
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
			LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
			for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount(); ++cpPathIndex)
			{
				LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
				if (cpath == NULL)
					continue;
				CTSVNPath path;
				if (!bRelativePaths)
					path.SetFromSVN(m_sRepositoryRoot);
				path.AppendPathString(cpath->sPath);
				if ((!bUseFilter)||
					((m_cHidePaths.GetState() & 0x0003)!=BST_CHECKED)||
					(cpath->sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0))
					pathList.AddPath(path);
				
			}
		}
	}
	pathList.RemoveDuplicates();
	return pathList;
}

void CLogDlg::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (m_bThreadRunning)
		return;		//no sorting while the arrays are filled
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;
	m_bAscending = nColumn == m_nSortColumn ? !m_bAscending : TRUE;
	m_nSortColumn = nColumn;	
	switch(m_nSortColumn)
	{
	    case 0: // Revision
	        {
	            if(m_bAscending)
	                std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscRevSort());
	            else
	                std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescRevSort());
	        }
	        break;
		case 1: // action
			{
				if(m_bAscending)
					std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscActionSort());
				else
					std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescActionSort());
			}
			break;
	    case 2: // Author
	        {
	            if(m_bAscending)
	                std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscAuthorSort());
	            else
	                std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescAuthorSort());
	        }
	        break;
	    case 3: // Date
	        {
	            if(m_bAscending)
	                std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscDateSort());
	            else
	                std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescDateSort());
	        }
	        break;
	    case 4: // Message
	        {
	            if(m_bAscending)
	                std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::AscMessageSort());
	            else
	                std::sort(m_logEntries.begin(), m_logEntries.end(), CLogDataVector::DescMessageSort());
	        }
	        break;
	    default:
		    ATLASSERT(0);
		    break;
	}

	SetSortArrow(&m_LogList, m_nSortColumn, !!m_bAscending);
	SortShownListArray();
	m_LogList.Invalidate();
	*pResult = 0;
}

void CLogDlg::SortShownListArray()
{
	// make sure the shown list still matches the filter after sorting.
    OnTimer(LOGFILTER_TIMER);
    // clear the selection states
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	while (pos)
	{
		m_LogList.SetItemState(m_LogList.GetNextSelectedItem(pos), 0, LVIS_SELECTED);
	}    
	m_LogList.SetSelectionMark(-1);
}

void CLogDlg::SetSortArrow(CListCtrl * control, int nColumn, bool bAscending)
{
	if (control == NULL)
		return;
	// set the sort arrow
	CHeaderCtrl * pHeader = control->GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (nColumn >= 0)
	{
		pHeader->GetItem(nColumn, &HeaderItem);
		HeaderItem.fmt |= (bAscending ? HDF_SORTDOWN : HDF_SORTUP);
		pHeader->SetItem(nColumn, &HeaderItem);
	}
}
void CLogDlg::OnLvnColumnclickLogmsg(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (m_bThreadRunning)
		return;		//no sorting while the arrays are filled
	if (m_currentChangedArray == NULL)
		return;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	const int nColumn = pNMLV->iSubItem;
	m_bAscendingPathList = nColumn == m_nSortColumnPathList ? !m_bAscendingPathList : TRUE;
	m_nSortColumnPathList = nColumn;
	qsort(m_currentChangedArray->GetData(), m_currentChangedArray->GetSize(), sizeof(LogChangedPath*), (GENERICCOMPAREFN)SortCompare);

	SetSortArrow(&m_LogMsgCtrl, m_nSortColumnPathList, m_bAscendingPathList);
	m_LogMsgCtrl.Invalidate();
	*pResult = 0;
}

int CLogDlg::m_nSortColumnPathList = 0;
bool CLogDlg::m_bAscendingPathList = false;

int CLogDlg::SortCompare(const void * pElem1, const void * pElem2)
{
	LogChangedPath * cpath1 = *((LogChangedPath**)pElem1);
	LogChangedPath * cpath2 = *((LogChangedPath**)pElem2);
	switch (m_nSortColumnPathList)
	{
	case 0:		// action
		if (m_bAscendingPathList)
			return cpath1->sAction.Compare(cpath2->sAction);
		else
			return cpath2->sAction.Compare(cpath1->sAction);
	case 1:		// path
		if (m_bAscendingPathList)
			return cpath1->sPath.Compare(cpath2->sPath);
		else
			return cpath2->sPath.Compare(cpath1->sPath);
	case 2:		// copyfrom path
		if (m_bAscendingPathList)
			return cpath1->sCopyFromPath.Compare(cpath2->sCopyFromPath);
		else
			return cpath2->sCopyFromPath.Compare(cpath1->sCopyFromPath);
	case 3:		// copyfrom revision
		if (m_bAscendingPathList)
			return cpath1->lCopyFromRev > cpath2->lCopyFromRev;
		else
			return cpath2->lCopyFromRev > cpath1->lCopyFromRev;
	}
	return 0;
}

void CLogDlg::OnBnClickedHidepaths()
{
	FillLogMessageCtrl();
	m_LogMsgCtrl.Invalidate();
}


void CLogDlg::OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);
	*pResult = -1;
	
	if (pFindInfo->lvfi.flags & LVFI_PARAM)
		return;	
	if ((pFindInfo->iStart < 0)||(pFindInfo->iStart >= m_arShownList.GetCount()))
		return;
	if (pFindInfo->lvfi.psz == 0)
		return;
		
	CString sCmp = pFindInfo->lvfi.psz;
	CString sRev;	
	for (int i=pFindInfo->iStart; i<m_arShownList.GetCount(); ++i)
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
		sRev.Format(_T("%ld"), pLogEntry->dwRev);
		if (pFindInfo->lvfi.flags & LVFI_PARTIAL)
		{
			if (sCmp.Compare(sRev.Left(sCmp.GetLength()))==0)
			{
				*pResult = i;
				return;
			}
		}
		else
		{
			if (sCmp.Compare(sRev)==0)
			{
				*pResult = i;
				return;
			}
		}
	}
	if (pFindInfo->lvfi.flags & LVFI_WRAP)
	{
		for (int i=0; i<pFindInfo->iStart; ++i)
		{
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
			sRev.Format(_T("%ld"), pLogEntry->dwRev);
			if (pFindInfo->lvfi.flags & LVFI_PARTIAL)
			{
				if (sCmp.Compare(sRev.Left(sCmp.GetLength()))==0)
				{
					*pResult = i;
					return;
				}
			}
			else
			{
				if (sCmp.Compare(sRev)==0)
				{
					*pResult = i;
					return;
				}
			}
		}
	}

	*pResult = -1;
}
