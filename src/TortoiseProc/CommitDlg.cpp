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
#include "CommitDlg.h"
#include "DirFileEnum.h"
#include "SVNConfig.h"
#include "SVNProperties.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "SVN.h"
#include "Registry.h"
#include "SVNStatus.h"
#include "HistoryDlg.h"
#include "Hooks.h"
#include "auto_buffer.h"
#include "COMError.h"
#include "..\version.h"
#include "BstrSafeVector.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT CCommitDlg::WM_AUTOLISTREADY = RegisterWindowMessage(_T("TORTOISESVN_AUTOLISTREADY_MSG"));

IMPLEMENT_DYNAMIC(CCommitDlg, CResizableStandAloneDialog)
CCommitDlg::CCommitDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCommitDlg::IDD, pParent)
	, m_bRecursive(FALSE)
	, m_bShowUnversioned(FALSE)
	, m_bShowExternals(FALSE)
	, m_bBlock(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bRunThread(FALSE)
	, m_pThread(NULL)
	, m_bKeepLocks(FALSE)
	, m_bKeepChangeList(TRUE)
	, m_itemsCount(0)
	, m_bSelectFilesForCommit(TRUE)
{
}

CCommitDlg::~CCommitDlg()
{
	delete m_pThread;
}

void CCommitDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_ListCtrl);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
	DDX_Check(pDX, IDC_SHOWEXTERNALS, m_bShowExternals);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Check(pDX, IDC_KEEPLOCK, m_bKeepLocks);
	DDX_Control(pDX, IDC_SPLITTER, m_wndSplitter);
	DDX_Check(pDX, IDC_KEEPLISTS, m_bKeepChangeList);
	DDX_Control(pDX, IDC_COMMIT_TO, m_CommitTo);
	DDX_Control(pDX, IDC_NEWVERSIONLINK, m_cUpdateLink);
}

BEGIN_MESSAGE_MAP(CCommitDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
	ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
	ON_BN_CLICKED(IDC_BUGTRAQBUTTON, OnBnClickedBugtraqbutton)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ITEMCOUNTCHANGED, OnSVNStatusListCtrlItemCountChanged)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_CHECKCHANGED, &CCommitDlg::OnSVNStatusListCtrlCheckChanged)
	ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_CHANGELISTCHANGED, &CCommitDlg::OnSVNStatusListCtrlChangelistChanged)
	ON_REGISTERED_MESSAGE(CLinkControl::LK_LINKITEMCLICKED, &CCommitDlg::OnCheck)
	ON_REGISTERED_MESSAGE(WM_AUTOLISTREADY, OnAutoListReady) 
	ON_WM_TIMER()
    ON_WM_SIZE()
	ON_STN_CLICKED(IDC_EXTERNALWARNING, &CCommitDlg::OnStnClickedExternalwarning)
	ON_BN_CLICKED(IDC_SHOWEXTERNALS, &CCommitDlg::OnBnClickedShowexternals)
END_MESSAGE_MAP()

BOOL CCommitDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	m_regAddBeforeCommit = CRegDWORD(_T("Software\\TortoiseSVN\\AddBeforeCommit"), TRUE);
	m_bShowUnversioned = m_regAddBeforeCommit;

	m_regShowExternals = CRegDWORD(_T("Software\\TortoiseSVN\\ShowExternals"), TRUE);
	m_bShowExternals = m_regShowExternals;

	m_History.SetMaxHistoryItems((LONG)CRegDWORD(_T("Software\\TortoiseSVN\\MaxHistoryItems"), 25));

	m_regKeepChangelists = CRegDWORD(_T("Software\\TortoiseSVN\\KeepChangeLists"), FALSE);
	m_bKeepChangeList = m_regKeepChangelists;

	SVNConfig config;
	m_bKeepLocks = config.KeepLocks();

	ExtendFrameIntoClientArea(0, 0, 0, IDC_LISTGROUP);
	m_aeroControls.SubclassControl(GetDlgItem(IDC_KEEPLOCK)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDC_KEEPLISTS)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDOK)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDCANCEL)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDHELP)->GetSafeHwnd());

	UpdateData(FALSE);
	
	m_ListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLTEXTSTATUS | SVNSLC_COLPROPSTATUS | SVNSLC_COLLOCK, _T("CommitDlg"));
	m_ListCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
	m_ListCtrl.SetCancelBool(&m_bCancelled);
	m_ListCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMIT);
	m_ListCtrl.EnableFileDrop();
	m_ListCtrl.SetBackgroundImage(IDI_COMMIT_BKG);
	
	m_ProjectProperties.ReadPropsPathList(m_pathList);
	if (CRegDWORD(_T("Software\\TortoiseSVN\\AlwaysWarnIfNoIssue"), FALSE)) 
		m_ProjectProperties.bWarnIfNoIssue = TRUE;
	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8));
	m_cLogMessage.RegisterContextMenuHandler(this);

	CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
	CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));
	CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, _T("Alt+")+CString(CAppUtils::FindAcceleratorKey(this, IDC_INVISIBLE)));

	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKALL)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKNONE)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKUNVERSIONED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKVERSIONED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKADDED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKDELETED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKMODIFIED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKFILES)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKDIRECTORIES)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);

	OnEnChangeLogmessage();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_EXTERNALWARNING, IDS_COMMITDLG_EXTERNALS);
	m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);
	
	
	GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);

	CBugTraqAssociations bugtraq_associations;
	bugtraq_associations.Load(m_ProjectProperties.sProviderUuid, m_ProjectProperties.sProviderParams);

	if (bugtraq_associations.FindProvider(m_pathList, &m_bugtraq_association))
	{
		CComPtr<IBugTraqProvider> pProvider;
		HRESULT hr = pProvider.CoCreateInstance(m_bugtraq_association.GetProviderClass());
		if (SUCCEEDED(hr))
		{
			m_BugTraqProvider = pProvider;
			ATL::CComBSTR temp;
			ATL::CComBSTR parameters;
			parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
			hr = pProvider->GetLinkText(GetSafeHwnd(), parameters, &temp);
			if (SUCCEEDED(hr))
			{
				SetDlgItemText(IDC_BUGTRAQBUTTON, temp == 0 ? _T("") : temp);
				GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(TRUE);
				GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
			}
		}

		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}
	if (!m_ProjectProperties.sMessage.IsEmpty())
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
		if (!m_ProjectProperties.sLabel.IsEmpty())
			SetDlgItemText(IDC_BUGIDLABEL, m_ProjectProperties.sLabel);
		GetDlgItem(IDC_BUGID)->SetFocus();
		CString sBugID = m_ProjectProperties.GetBugIDFromLog(m_sLogMessage);
		if (!sBugID.IsEmpty())
		{
			SetDlgItemText(IDC_BUGID, sBugID);
		}
	}
	else
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}

	VersionCheck();

	if (!m_sLogMessage.IsEmpty())
		m_cLogMessage.SetText(m_sLogMessage);
		
	GetWindowText(m_sWindowTitle);
	
	AdjustControlSize(IDC_SHOWUNVERSIONED);
	AdjustControlSize(IDC_KEEPLOCK);

	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKALL);
	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKNONE);
	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKUNVERSIONED);
	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKVERSIONED);
	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKADDED);
	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKDELETED);
	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKMODIFIED);
	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKFILES);
	m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKDIRECTORIES);

	// line up all controls and adjust their sizes.
#define LINKSPACING 9
	RECT rc = AdjustControlSize(IDC_SELECTLABEL);
	rc.right -= 15;	// AdjustControlSize() adds 20 pixels for the checkbox/radio button bitmap, but this is a label...
	rc = AdjustStaticSize(IDC_CHECKALL, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKNONE, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKUNVERSIONED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKVERSIONED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKADDED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKDELETED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKMODIFIED, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKFILES, rc, LINKSPACING);
	rc = AdjustStaticSize(IDC_CHECKDIRECTORIES, rc, LINKSPACING);

	GetClientRect(m_DlgOrigRect);
	m_cLogMessage.GetClientRect(m_LogMsgOrigRect);

	AddAnchor(IDC_COMMITLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_BUGTRAQBUTTON, TOP_RIGHT);
	AddAnchor(IDC_COMMIT_TO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_NEWVERSIONLINK, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_SELECTLABEL, TOP_LEFT);
	AddAnchor(IDC_CHECKALL, TOP_LEFT);
	AddAnchor(IDC_CHECKNONE, TOP_LEFT);
	AddAnchor(IDC_CHECKUNVERSIONED, TOP_LEFT);
	AddAnchor(IDC_CHECKVERSIONED, TOP_LEFT);
	AddAnchor(IDC_CHECKADDED, TOP_LEFT);
	AddAnchor(IDC_CHECKDELETED, TOP_LEFT);
	AddAnchor(IDC_CHECKMODIFIED, TOP_LEFT);
	AddAnchor(IDC_CHECKFILES, TOP_LEFT);
	AddAnchor(IDC_CHECKDIRECTORIES, TOP_LEFT);

	AddAnchor(IDC_INVISIBLE, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	
	AddAnchor(IDC_INVISIBLE2, TOP_RIGHT);
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
	AddAnchor(IDC_SHOWEXTERNALS, BOTTOM_LEFT);
	AddAnchor(IDC_EXTERNALWARNING, BOTTOM_RIGHT);
	AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_KEEPLOCK, BOTTOM_LEFT);
	AddAnchor(IDC_KEEPLISTS, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CommitDlg"));
	DWORD yPos = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\CommitDlgSizer"));
	RECT rcDlg, rcLogMsg, rcFileList;
	GetClientRect(&rcDlg);
	m_cLogMessage.GetWindowRect(&rcLogMsg);
	ScreenToClient(&rcLogMsg);
	m_ListCtrl.GetWindowRect(&rcFileList);
	ScreenToClient(&rcFileList);
	if (yPos)
	{
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos - rectSplitter.top;
		if ((rcLogMsg.bottom + delta > rcLogMsg.top)&&(rcLogMsg.bottom + delta < rcFileList.bottom - 30))
		{
			m_wndSplitter.SetWindowPos(NULL, rectSplitter.left, yPos, 0, 0, SWP_NOSIZE);
			DoSize(delta);
		}
	}

	// add all directories to the watcher
	for (int i=0; i<m_pathList.GetCount(); ++i)
	{
		if (m_pathList[i].IsDirectory())
			m_pathwatcher.AddPath(m_pathList[i]);
	}

	m_updatedPathList = m_pathList;

	//first start a thread to obtain the file list with the status without
	//blocking the dialog
	InterlockedExchange(&m_bBlock, TRUE);
	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
	CRegDWORD err = CRegDWORD(_T("Software\\TortoiseSVN\\ErrorOccurred"), FALSE);
	CRegDWORD historyhint = CRegDWORD(_T("Software\\TortoiseSVN\\HistoryHintShown"), FALSE);
	if ((((DWORD)err)!=FALSE)&&((((DWORD)historyhint)==FALSE)))
	{
		historyhint = TRUE;
		ShowBalloon(IDC_HISTORY, IDS_COMMITDLG_HISTORYHINT_TT, IDI_INFORMATION);
	}
	err = FALSE;


	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCommitDlg::OnOK()
{
	if (m_bBlock)
		return;
	if (m_bThreadRunning)
	{
		m_bCancelled = true;
		InterlockedExchange(&m_bRunThread, FALSE);
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
	id.Trim(_T("\n\r"));
	if (!m_ProjectProperties.CheckBugID(id))
	{
		ShowBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, IDI_EXCLAMATION);
		return;
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
			return;
	}

	CRegDWORD regUnversionedRecurse (_T("Software\\TortoiseSVN\\UnversionedRecurse"), TRUE);
	if (!(DWORD)regUnversionedRecurse)
	{
		// Find unversioned directories which are marked for commit. The user might expect them
		// to be added recursively since he cannot see the files. Let's ask the user if he knows
		// what he is doing.
		int nListItems = m_ListCtrl.GetItemCount();
		for (int j=0; j<nListItems; j++)
		{
			const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
			if (entry->IsChecked() && (entry->status == svn_wc_status_unversioned) && entry->IsFolder() )
			{
				if (CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_UNVERSIONEDFOLDERWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
					return;
			}
		}
	}

	m_pathwatcher.Stop();
	InterlockedExchange(&m_bBlock, TRUE);
	CDWordArray arDeleted;
	//first add all the unversioned files the user selected
	//and check if all versioned files are selected
	int nUnchecked = 0;
	m_bRecursive = true;
	int nListItems = m_ListCtrl.GetItemCount();

	CTSVNPathList itemsToAdd;
	CTSVNPathList itemsToRemove;
	bool bCheckedInExternal = false;
	bool bHasConflicted = false;
	std::set<CString> checkedLists;
	std::set<CString> uncheckedLists;
	for (int j=0; j<nListItems; j++)
	{
		const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(j);
		if (entry->IsChecked())
		{
			if (entry->status == svn_wc_status_unversioned)
			{
				itemsToAdd.AddPath(entry->GetPath());
			}
			if (entry->status == svn_wc_status_conflicted)
			{
				bHasConflicted = true;
			}
			if (entry->textstatus == svn_wc_status_missing)
			{
				itemsToRemove.AddPath(entry->GetPath());
			}
			if (entry->status == svn_wc_status_deleted)
			{
				arDeleted.Add(j);
			}
			if (entry->IsInExternal())
			{
				bCheckedInExternal = true;
			}
			checkedLists.insert(entry->GetChangeList());
		}
		else
		{
			if ((entry->status != svn_wc_status_unversioned)	&&
				(entry->status != svn_wc_status_ignored))
			{
				nUnchecked++;
				uncheckedLists.insert(entry->GetChangeList());
				if (m_bRecursive)
				{
					// This algorithm is for the sake of simplicity of the complexity O(N²)
					for (int k=0; k<nListItems; k++)
					{
						const CSVNStatusListCtrl::FileEntry * entryK = m_ListCtrl.GetListEntry(k);
						if (entryK->IsChecked() && entryK->GetPath().IsAncestorOf(entry->GetPath())  )
						{
							// Fall back to a non-recursive commit to prevent items being
							// committed which aren't checked although its parent is checked
							// (property change, directory deletion, ... )
							m_bRecursive = false;
							break;
						}
					}
				}
			}
		}
	}
	if (m_pathwatcher.GetNumberOfChangedPaths() && m_bRecursive)
	{
		// There are paths which got changed (touched at least).
		// We have to find out if this affects the selection in the commit dialog
		// If it could affect the selection, revert back to a non-recursive commit
		CTSVNPathList changedList = m_pathwatcher.GetChangedPaths();
		changedList.RemoveDuplicates();
		for (int i=0; i<changedList.GetCount(); ++i)
		{
			if (changedList[i].IsAdminDir())
			{
				// TODO: refactor this into an SVN class since 'entries' and
				// 'tmp' are referring to internal files/folders of the .svn dir
				// and will have to change for the SQLite-based wc format

				// something inside an admin dir was changed.
				// if it's the entries file, then we have to fully refresh because
				// files may have been added/removed from version control
				if ((changedList[i].GetWinPathString().Right(7).CompareNoCase(_T("entries")) == 0) &&
					(changedList[i].GetWinPathString().Find(_T("\\tmp\\"))<0))
				{
					m_bRecursive = false;
					break;
				}
			}
			else if (!m_ListCtrl.IsPathShown(changedList[i]))
			{
				// a path which is not shown in the list has changed
				CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(changedList[i]);
				if (entry)
				{
					// check if the changed path would get committed by a recursive commit
					if ((!entry->IsFromDifferentRepository()) &&
						(!entry->IsInExternal()) &&
						(!entry->IsNested()) && 
						(!entry->IsChecked()) &&
						(entry->IsVersioned()))
					{
						m_bRecursive = false;
						break;
					}
				}
			}
		}
	}


	// Now, do all the adds - make sure that the list is sorted so that parents 
	// are added before their children
	itemsToAdd.SortByPathname();
	SVN svn;
	if (!svn.Add(itemsToAdd, &m_ProjectProperties, svn_depth_empty, false, false, true))
	{
		CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
		Refresh();
		return;
	}

	// Remove any missing items
	// Not sure that this sort is really necessary - indeed, it might be better to do a reverse sort at this point
	itemsToRemove.SortByPathname();
	svn.Remove(itemsToRemove, true);

	//the next step: find all deleted files and check if they're 
	//inside a deleted folder. If that's the case, then remove those
	//files from the list since they'll get deleted by the parent
	//folder automatically.
	m_ListCtrl.Block(TRUE, FALSE);
	INT_PTR nDeleted = arDeleted.GetCount();
	for (INT_PTR i=0; i<arDeleted.GetCount(); i++)
	{
		if (!m_ListCtrl.GetCheck(arDeleted.GetAt(i)))
			continue;
		const CTSVNPath& path = m_ListCtrl.GetListEntry(arDeleted.GetAt(i))->GetPath();
		if (!path.IsDirectory())
			continue;
		//now find all children of this directory
		for (int j=0; j<arDeleted.GetCount(); j++)
		{
			if (i==j)
				continue;
			CSVNStatusListCtrl::FileEntry* childEntry = m_ListCtrl.GetListEntry(arDeleted.GetAt(j));
			if (childEntry->IsChecked())
			{
				if (path.IsAncestorOf(childEntry->GetPath()))
				{
					m_ListCtrl.SetEntryCheck(childEntry, arDeleted.GetAt(j), false);
					nDeleted--;
				}
			}
		}
	} 
	m_ListCtrl.Block(FALSE, FALSE);

	if ((nUnchecked != 0)||(bCheckedInExternal)||(bHasConflicted)||(!m_bRecursive))
	{
		//save only the files the user has checked into the temporary file
		m_ListCtrl.WriteCheckedNamesToPathList(m_pathList);
	}
	m_ListCtrl.WriteCheckedNamesToPathList(m_selectedPathList);
	// the item count is used in the progress dialog to show the overall commit
	// progress.
	// deleted items only send one notification event, all others send two
	m_itemsCount = ((m_selectedPathList.GetCount() - nDeleted - itemsToRemove.GetCount()) * 2) + nDeleted + itemsToRemove.GetCount();

	if ((m_bRecursive)&&(checkedLists.size() == 1))
	{
		// all checked items belong to the same changelist
		// find out if there are any unchecked items which belong to that changelist
		if (uncheckedLists.find(*checkedLists.begin()) == uncheckedLists.end())
			m_sChangeList = *checkedLists.begin();
	}
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	m_regShowExternals = m_bShowExternals;
	if (!GetDlgItem(IDC_KEEPLOCK)->IsWindowEnabled())
		m_bKeepLocks = FALSE;
	m_regKeepChangelists = m_bKeepChangeList;
	if (!GetDlgItem(IDC_KEEPLISTS)->IsWindowEnabled())
		m_bKeepChangeList = FALSE;
	InterlockedExchange(&m_bBlock, FALSE);
	m_sBugID.Trim();
	CString sExistingBugID = m_ProjectProperties.FindBugID(m_sLogMessage);
	sExistingBugID.Trim();
	if (!m_sBugID.IsEmpty() && m_sBugID.Compare(sExistingBugID))
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

	// now let the bugtraq plugin check the commit message
	CComPtr<IBugTraqProvider2> pProvider2 = NULL;
	if (m_BugTraqProvider)
	{
		HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
		if (SUCCEEDED(hr))
		{
			ATL::CComBSTR temp;
			CString common = m_ListCtrl.GetCommonURL(true).GetSVNPathString();
			ATL::CComBSTR repositoryRoot;
			repositoryRoot.Attach(common.AllocSysString());
			ATL::CComBSTR parameters;
			parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
			ATL::CComBSTR commonRoot(m_pathList.GetCommonRoot().GetDirectory().GetWinPath());
			ATL::CComBSTR commitMessage;
			commitMessage.Attach(m_sLogMessage.AllocSysString());
			CBstrSafeVector pathList(m_selectedPathList.GetCount());

			for (LONG index = 0; index < m_selectedPathList.GetCount(); ++index)
				pathList.PutElement(index, m_selectedPathList[index].GetSVNPathString());
			
			hr = pProvider2->CheckCommit(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, commitMessage, &temp);
			if (FAILED(hr))
			{
				OnComError(hr);
			}
			else
			{
				CString sError = temp == 0 ? _T("") : temp;
				if (!sError.IsEmpty())
				{
					CMessageBox::Show(m_hWnd, sError, _T("TortoiseSVN"), MB_ICONERROR);
					return;
				}
			}
		}
	}

	m_History.AddEntry(m_sLogMessage);
	m_History.Save();

	SaveSplitterPos();

	CResizableStandAloneDialog::OnOK();
}

void CCommitDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\CommitDlgSizer"));
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos = rectSplitter.top;
	}
}

UINT CCommitDlg::StatusThreadEntry(LPVOID pVoid)
{
	return ((CCommitDlg*)pVoid)->StatusThread();
}

UINT CCommitDlg::StatusThread()
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a list control. 
	InterlockedExchange(&m_bBlock, TRUE);
	InterlockedExchange(&m_bThreadRunning, TRUE);// so the main thread knows that this thread is still running
	InterlockedExchange(&m_bRunThread, TRUE);	// if this is set to FALSE, the thread should stop
	m_bCancelled = false;
	CoInitialize(NULL);

	DialogEnableWindow(IDOK, false);
	DialogEnableWindow(IDC_SHOWUNVERSIONED, false);
	GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_HIDE);
	DialogEnableWindow(IDC_EXTERNALWARNING, false);

	DialogEnableWindow(IDC_CHECKALL, false);
	DialogEnableWindow(IDC_CHECKNONE, false);
	DialogEnableWindow(IDC_CHECKUNVERSIONED, false);
	DialogEnableWindow(IDC_CHECKVERSIONED, false);
	DialogEnableWindow(IDC_CHECKADDED, false);
	DialogEnableWindow(IDC_CHECKDELETED, false);
	DialogEnableWindow(IDC_CHECKMODIFIED, false);
	DialogEnableWindow(IDC_CHECKFILES, false);
	DialogEnableWindow(IDC_CHECKDIRECTORIES, false);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKALL)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKNONE)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKUNVERSIONED)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKVERSIONED)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKADDED)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKDELETED)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKMODIFIED)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKFILES)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKDIRECTORIES)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);

    // read the list of recent log entries before querying the WC for status
    // -> the user may select one and modify / update it while we are crawling the WC
	if (m_History.GetCount()==0)
	{
		CString reg;
		if (m_ListCtrl.m_sUUID.IsEmpty() && m_pathList.GetCount()>0)
		{
			SVN svn;
			reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)svn.GetUUIDFromPath(m_pathList[0]));
		}
		else
			reg.Format(_T("Software\\TortoiseSVN\\History\\commit%s"), (LPCTSTR)m_ListCtrl.m_sUUID);
		m_History.Load(reg, _T("logmsgs"));
	}

    // Initialise the list control with the status of the files/folders below us
	BOOL success = m_ListCtrl.GetStatus(m_pathList);
	m_ListCtrl.CheckIfChangelistsArePresent(false);

	DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWLOCKS | SVNSLC_SHOWINCHANGELIST | SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINEXTERNALS;
	dwShow |= DWORD(m_regAddBeforeCommit) ? SVNSLC_SHOWUNVERSIONED : 0;
	dwShow |= DWORD(m_regShowExternals) ? SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO | SVNSLC_SHOWEXTDISABLED : 0;
	if (success)
	{
		m_cLogMessage.SetRepositoryRoot(m_ListCtrl.m_sRepositoryRoot);

		if (m_checkedPathList.GetCount())
			m_ListCtrl.Show(dwShow, m_checkedPathList, 0, true, true);
		else
		{
			DWORD dwCheck = m_bSelectFilesForCommit ? SVNSLC_SHOWDIRECTS|SVNSLC_SHOWMODIFIED|SVNSLC_SHOWADDED|SVNSLC_SHOWADDEDHISTORY|SVNSLC_SHOWREMOVED|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|SVNSLC_SHOWLOCKS : 0;
			m_ListCtrl.Show(dwShow, CTSVNPathList(), dwCheck, true, true);
			m_bSelectFilesForCommit = true;
		}

		if (m_ListCtrl.HasExternalsFromDifferentRepos())
		{
			GetDlgItem(IDC_EXTERNALWARNING)->ShowWindow(SW_SHOW);
			DialogEnableWindow(IDC_EXTERNALWARNING, TRUE);
		}
		m_CommitTo.SetWindowText(m_ListCtrl.m_sURL);
		m_tooltips.AddTool(GetDlgItem(IDC_STATISTICS), m_ListCtrl.GetStatisticsString());

		{
			// compare all url parts against the branches pattern and if it matches,
			// change the background image of the list control to indicate 
			// a commit to a branch
			CRegString branchesPattern (_T("Software\\TortoiseSVN\\RevisionGraph\\BranchPattern"), _T("branches"));
			CString sBranches = branchesPattern;
			int pos = 0;
			CString temp;
			bool bFound = false;
			while (!bFound)
			{
				temp = sBranches.Tokenize(_T(";"), pos);
				if (temp.IsEmpty())
					break;

				int urlpos = 0;
				CString temp2;
				for(;;)
				{
					temp2 = m_ListCtrl.m_sURL.Tokenize(_T("/"), urlpos);
					if (temp2.IsEmpty())
						break;

					if (CStringUtils::WildCardMatch(temp, temp2))
					{
						m_ListCtrl.SetBackgroundImage(IDI_COMMITBRANCHES_BKG);
						bFound = true;
						break;
					}
				}
			} 
		}
	}
	CString logmsg;
	GetDlgItemText(IDC_LOGMESSAGE, logmsg);
	DialogEnableWindow(IDOK, logmsg.GetLength() >= m_ProjectProperties.nMinLogSize);
	if (!success)
	{
		if (!m_ListCtrl.GetLastErrorMessage().IsEmpty())
			m_ListCtrl.SetEmptyString(m_ListCtrl.GetLastErrorMessage());
		m_ListCtrl.Show(dwShow, CTSVNPathList(), 0, true, true);
	}

	CTSVNPath commonDir = m_ListCtrl.GetCommonDirectory(false);
	SetWindowText(m_sWindowTitle + _T(" - ") + commonDir.GetWinPathString());

	m_autolist.clear();
	// we don't have to block the commit dialog while we fetch the
	// auto completion list.
	m_pathwatcher.ClearChangedPaths();
	InterlockedExchange(&m_bBlock, FALSE);
	if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\Autocompletion"), TRUE)==TRUE)
	{
		m_ListCtrl.Block(TRUE, TRUE);
		GetAutocompletionList();
		m_ListCtrl.Block(FALSE, FALSE);
	}
	if (m_bRunThread)
	{
		DialogEnableWindow(IDC_SHOWUNVERSIONED, true);
		if (m_ListCtrl.HasChangeLists())
			DialogEnableWindow(IDC_KEEPLISTS, true);
		if (m_ListCtrl.HasExternalsFromDifferentRepos())
			DialogEnableWindow(IDC_SHOWEXTERNALS, true);
		if (m_ListCtrl.HasLocks())
			DialogEnableWindow(IDC_KEEPLOCK, true);

		UpdateCheckLinks();
		// we have the list, now signal the main thread about it
		SendMessage(WM_AUTOLISTREADY);	// only send the message if the thread wasn't told to quit!
	}
	InterlockedExchange(&m_bRunThread, FALSE);
	InterlockedExchange(&m_bThreadRunning, FALSE);
	// force the cursor to normal
	RefreshCursor();
	CoUninitialize();
	return 0;
}

void CCommitDlg::OnCancel()
{
	m_bCancelled = true;
	if (m_bBlock)
		return;
	m_pathwatcher.Stop();
	if (m_bThreadRunning)
	{
		InterlockedExchange(&m_bRunThread, FALSE);
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	}
	UpdateData();
	m_sBugID.Trim();
	m_sLogMessage = m_cLogMessage.GetText();
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
	if (m_ProjectProperties.sLogTemplate.Compare(m_sLogMessage) != 0)
		m_History.AddEntry(m_sLogMessage);
	m_History.Save();
	SaveSplitterPos();
	CResizableStandAloneDialog::OnCancel();
}

BOOL CCommitDlg::PreTranslateMessage(MSG* pMsg)
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
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						if (DWORD(CRegStdDWORD(_T("Software\\TortoiseSVN\\CtrlEnter"), TRUE)))
							PostMessage(WM_COMMAND, IDOK);
					}
					return TRUE;
				}
				if ( GetFocus()==GetDlgItem(IDC_BUGID) )
				{
					// Pressing RETURN in the bug id control
					// moves the focus to the message
					GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
					return TRUE;
				}
			}
			break;
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCommitDlg::Refresh()
{
	if (m_bThreadRunning)
		return;

	InterlockedExchange(&m_bBlock, TRUE);
	m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		InterlockedExchange(&m_bBlock, FALSE);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
}

void CCommitDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CCommitDlg::OnBnClickedShowunversioned()
{
	m_tooltips.Pop();	// hide the tooltips
	UpdateData();
	m_regAddBeforeCommit = m_bShowUnversioned;
	if (!m_bBlock)
	{
		DWORD dwShow = m_ListCtrl.GetShowFlags();
		if (DWORD(m_regAddBeforeCommit))
			dwShow |= SVNSLC_SHOWUNVERSIONED;
		else
			dwShow &= ~SVNSLC_SHOWUNVERSIONED;
		m_ListCtrl.Show(dwShow, CTSVNPathList(), 0, true, true);
		UpdateCheckLinks();
	}
}

void CCommitDlg::OnStnClickedExternalwarning()
{
	m_tooltips.Popup();
}

void CCommitDlg::OnEnChangeLogmessage()
{
	UpdateOKButton();
}

LRESULT CCommitDlg::OnSVNStatusListCtrlItemCountChanged(WPARAM, LPARAM)
{
	if ((m_ListCtrl.GetItemCount() == 0)&&(m_ListCtrl.HasUnversionedItems())&&(!m_bShowUnversioned))
	{
		if (CMessageBox::Show(*this, IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
		{
			m_bShowUnversioned = TRUE;
			DWORD dwShow = SVNSLC_SHOWVERSIONEDBUTNORMAL | SVNSLC_SHOWLOCKS | SVNSLC_SHOWINCHANGELIST | SVNSLC_SHOWEXTERNAL | SVNSLC_SHOWINEXTERNALS | SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO | SVNSLC_SHOWEXTDISABLED | SVNSLC_SHOWUNVERSIONED;
			m_ListCtrl.Show(dwShow, CTSVNPathList(), 0, true, true);
			UpdateCheckLinks();
			UpdateData(FALSE);
		}
	}
	return 0;
}

LRESULT CCommitDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
	Refresh();
	return 0;
}

LRESULT CCommitDlg::OnFileDropped(WPARAM, LPARAM lParam)
{
	BringWindowToTop();
	SetForegroundWindow();
	SetActiveWindow();
	// if multiple files/folders are dropped
	// this handler is called for every single item
	// separately.
	// To avoid creating multiple refresh threads and
	// causing crashes, we only add the items to the
	// list control and start a timer.
	// When the timer expires, we start the refresh thread,
	// but only if it isn't already running - otherwise we
	// restart the timer.
	CTSVNPath path;
	path.SetFromWin((LPCTSTR)lParam);

	// just add all the items we get here.
	// if the item is versioned, the add will fail but nothing
	// more will happen.
	SVN svn;
	svn.Add(CTSVNPathList(path), &m_ProjectProperties, svn_depth_empty, false, true, true);

	if (!m_ListCtrl.HasPath(path))
	{
		if (m_pathList.AreAllPathsFiles())
		{
			m_pathList.AddPath(path);
			m_pathList.RemoveDuplicates();
			m_updatedPathList.AddPath(path);
			m_updatedPathList.RemoveDuplicates();
		}
		else
		{
			// if the path list contains folders, we have to check whether
			// our just (maybe) added path is a child of one of those. If it is
			// a child of a folder already in the list, we must not add it. Otherwise
			// that path could show up twice in the list.
			bool bHasParentInList = false;
			for (int i=0; i<m_pathList.GetCount(); ++i)
			{
				if (m_pathList[i].IsAncestorOf(path))
				{
					bHasParentInList = true;
					break;
				}
			}
			if (!bHasParentInList)
			{
				m_pathList.AddPath(path);
				m_pathList.RemoveDuplicates();
				m_updatedPathList.AddPath(path);
				m_updatedPathList.RemoveDuplicates();
			}
		}
	}
	
	// Always start the timer, since the status of an existing item might have changed
	SetTimer(REFRESHTIMER, 200, NULL);
	ATLTRACE(_T("Item %s dropped, timer started\n"), path.GetWinPath());
	return 0;
}

LRESULT CCommitDlg::OnAutoListReady(WPARAM, LPARAM)
{
	m_cLogMessage.SetAutoCompletionList(m_autolist, '*');
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// functions which run in the status thread
//////////////////////////////////////////////////////////////////////////

void CCommitDlg::ParseRegexFile(const CString& sFile, std::map<CString, CString>& mapRegex)
{
	CString strLine;
	try
	{
		CStdioFile file(sFile, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
		while (m_bRunThread && file.ReadString(strLine))
		{
			int eqpos = strLine.Find('=');
			CString rgx;
			rgx = strLine.Mid(eqpos+1).Trim();

			int pos = -1;
			while (((pos = strLine.Find(','))>=0)&&(pos < eqpos))
			{
				mapRegex[strLine.Left(pos)] = rgx;
				strLine = strLine.Mid(pos+1).Trim();
			}
			mapRegex[strLine.Left(strLine.Find('=')).Trim()] = rgx;
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading auto list regex file\n");
		pE->Delete();
	}
}

void CCommitDlg::GetAutocompletionList()
{
	// the auto completion list is made of strings from each selected files.
	// the strings used are extracted from the files with regexes found
	// in the file "autolist.txt".
	// the format of that file is:
	// file extensions separated with commas '=' regular expression to use
	// example:
	// .h, .hpp = (?<=class[\s])\b\w+\b|(\b\w+(?=[\s ]?\(\);))
	// .cpp = (?<=[^\s]::)\b\w+\b
	
	m_autolist.clear();
	std::map<CString, CString> mapRegex;
	CString sRegexFile = CPathUtils::GetAppDirectory();
	CRegDWORD regtimeout = CRegDWORD(_T("Software\\TortoiseSVN\\AutocompleteParseTimeout"), 5);
	DWORD timeoutvalue = regtimeout*1000;
	if (timeoutvalue == 0)
		return;
	sRegexFile += _T("autolist.txt");
	if (!m_bRunThread)
		return;
	if (PathFileExists(sRegexFile))
		ParseRegexFile(sRegexFile, mapRegex);

	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, sRegexFile.GetBuffer(MAX_PATH+1));
	sRegexFile.ReleaseBuffer();
	sRegexFile += _T("\\TortoiseSVN\\autolist.txt");
	if (PathFileExists(sRegexFile))
		ParseRegexFile(sRegexFile, mapRegex);

	DWORD starttime = GetTickCount();

	// now we have two arrays of strings, where the first array contains all
	// file extensions we can use and the second the corresponding regex strings
	// to apply to those files.

	// the next step is to go over all files shown in the commit dialog
	// and scan them for strings we can use
	int nListItems = m_ListCtrl.GetItemCount();
	CRegDWORD removedExtension(_T("Software\\TortoiseSVN\\AutocompleteRemovesExtensions"), FALSE);
	for (int i=0; i<nListItems && m_bRunThread; ++i)
	{
		// stop parsing after timeout
		if ((!m_bRunThread) || (GetTickCount() - starttime > timeoutvalue))
			return;
		const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetListEntry(i);
		if (!entry)
			continue;
		
		// add the path parts to the auto completion list too
		CString sPartPath = entry->GetRelativeSVNPath();
		m_autolist.insert(sPartPath);

		int pos = 0;
		int lastPos = 0;
		while ((pos = sPartPath.Find('/', pos)) >= 0)
		{
			pos++;
			lastPos = pos;
			m_autolist.insert(sPartPath.Mid(pos));
		}

		// Last inserted entry is a file name.
		// Some users prefer to also list file name without extension.
		if ((DWORD)removedExtension)
		{
			int dotPos = sPartPath.ReverseFind('.');
			if ((dotPos >= 0) && (dotPos > lastPos))
				m_autolist.insert(sPartPath.Mid(lastPos, dotPos - lastPos));
		}

		if ((entry->status <= svn_wc_status_normal)||(entry->status == svn_wc_status_ignored))
			continue;

		CString sExt = entry->GetPath().GetFileExtension();
		sExt.MakeLower();
		// find the regex string which corresponds to the file extension
		CString rdata = mapRegex[sExt];
		if (rdata.IsEmpty())
			continue;

		ScanFile(entry->GetPath().GetWinPathString(), rdata, sExt);
		if ((entry->textstatus != svn_wc_status_unversioned) &&
			(entry->textstatus != svn_wc_status_none) &&
			(entry->textstatus != svn_wc_status_ignored) &&
			(entry->textstatus != svn_wc_status_added) &&
			(entry->textstatus != svn_wc_status_normal))
		{
			CTSVNPath basePath = SVN::GetPristinePath(entry->GetPath());
			if (!basePath.IsEmpty())
				ScanFile(basePath.GetWinPathString(), rdata, sExt);
		}
	}
	ATLTRACE(_T("Auto completion list loaded in %d msec\n"), GetTickCount() - starttime);
}

void CCommitDlg::ScanFile(const CString& sFilePath, const CString& sRegex, const CString& sExt)
{
	static std::map<CString, tr1::wregex> regexmap;

	wstring sFileContent;
	HANDLE hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(hFile, NULL);
		if (size > 300000L)
		{
			// no files bigger than 300k
			CloseHandle(hFile);
			return;
		}
		// allocate memory to hold file contents
		auto_buffer<char> buffer(size);
		DWORD readbytes;
		ReadFile(hFile, buffer, size, &readbytes, NULL);
		CloseHandle(hFile);
		int opts = 0;
		IsTextUnicode(buffer, readbytes, &opts);
		if (opts & IS_TEXT_UNICODE_NULL_BYTES)
		{
			return;
		}
		if (opts & IS_TEXT_UNICODE_UNICODE_MASK)
		{
			sFileContent = wstring((wchar_t*)buffer.get(), readbytes/sizeof(WCHAR));
		}
		if ((opts & IS_TEXT_UNICODE_NOT_UNICODE_MASK)||(opts == 0))
		{
			const int ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)buffer, readbytes, NULL, 0);
			auto_buffer<wchar_t> pWideBuf(ret);
			const int ret2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)buffer, readbytes, pWideBuf, ret);
			if (ret2 == ret)
				sFileContent = wstring(pWideBuf, ret);
		}
	}
	if (sFileContent.empty()|| !m_bRunThread)
	{
		return;
	}

	try
	{

		tr1::wregex regCheck;
		std::map<CString, tr1::wregex>::const_iterator regIt;
		if ((regIt = regexmap.find(sExt)) != regexmap.end())
			regCheck = regIt->second;
		else
		{
			regCheck = tr1::wregex(sRegex, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
			regexmap[sExt] = regCheck;
		}
		const tr1::wsregex_iterator end;
		for (tr1::wsregex_iterator it(sFileContent.begin(), sFileContent.end(), regCheck); it != end; ++it)
		{
			const tr1::wsmatch match = *it;
			for (size_t i=1; i<match.size(); ++i)
			{
				if (match[i].second-match[i].first)
				{
					m_autolist.insert(wstring(match[i]).c_str());
				}
			}
		}
	}
	catch (exception) {}
}

// CSciEditContextMenuInterface
void CCommitDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
	CString sMenuItemText(MAKEINTRESOURCE(IDS_COMMITDLG_POPUP_PASTEFILELIST));
	m_nPopupPasteListCmd = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupPasteListCmd, sMenuItemText);
}

bool CCommitDlg::HandleMenuItemClick(int cmd, CSciEdit * pSciEdit)
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
				WORD langID = (WORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
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

void CCommitDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case ENDDIALOGTIMER:
		KillTimer(ENDDIALOGTIMER);
		EndDialog(0);
		break;
	case REFRESHTIMER:
		if (m_bThreadRunning)
		{
			SetTimer(REFRESHTIMER, 200, NULL);
			ATLTRACE("Wait some more before refreshing\n");
		}
		else
		{
			KillTimer(REFRESHTIMER);
			ATLTRACE("Refreshing after items dropped\n");
			Refresh();
		}
		break;
	}
	__super::OnTimer(nIDEvent);
}

void CCommitDlg::OnBnClickedHistory()
{
	m_tooltips.Pop();	// hide the tooltips
	if (m_pathList.GetCount() == 0)
		return;
	CHistoryDlg historyDlg(this);
	historyDlg.SetHistory(m_History);
	if (historyDlg.DoModal() != IDOK)
		return;

	CString sMsg = historyDlg.GetSelectedText();
	if (sMsg != m_cLogMessage.GetText().Left(sMsg.GetLength()))
	{
		CString sBugID = m_ProjectProperties.FindBugID(sMsg);
		if ((!sBugID.IsEmpty())&&((GetDlgItem(IDC_BUGID)->IsWindowVisible())))
		{
			SetDlgItemText(IDC_BUGID, sBugID);
		}
		if (m_ProjectProperties.sLogTemplate.Compare(m_cLogMessage.GetText())!=0)
			m_cLogMessage.InsertText(sMsg, !m_cLogMessage.GetText().IsEmpty());
		else
			m_cLogMessage.SetText(sMsg);
	}

	UpdateOKButton();
	GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
}

void CCommitDlg::OnBnClickedBugtraqbutton()
{
	m_tooltips.Pop();	// hide the tooltips
	CString sMsg = m_cLogMessage.GetText();

	if (m_BugTraqProvider == NULL)
		return;

	ATL::CComBSTR parameters;
	parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
	ATL::CComBSTR commonRoot(m_pathList.GetCommonRoot().GetDirectory().GetWinPath());
	CBstrSafeVector pathList(m_pathList.GetCount());

	for (LONG index = 0; index < m_pathList.GetCount(); ++index)
		pathList.PutElement(index, m_pathList[index].GetSVNPathString());

	ATL::CComBSTR originalMessage;
	originalMessage.Attach(sMsg.AllocSysString());
	ATL::CComBSTR temp;
	m_revProps.clear();

	// first try the IBugTraqProvider2 interface
	CComPtr<IBugTraqProvider2> pProvider2 = NULL;
	HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
	if (SUCCEEDED(hr))
	{
		CString common = m_ListCtrl.GetCommonURL(false).GetSVNPathString();
		ATL::CComBSTR repositoryRoot;
		repositoryRoot.Attach(common.AllocSysString());
		ATL::CComBSTR bugIDOut;
		GetDlgItemText(IDC_BUGID, m_sBugID);
		ATL::CComBSTR bugID;
		bugID.Attach(m_sBugID.AllocSysString());
		CBstrSafeVector revPropNames;
		CBstrSafeVector revPropValues;
		if (FAILED(hr = pProvider2->GetCommitMessage2(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, originalMessage, bugID, &bugIDOut, &revPropNames, &revPropValues, &temp)))
		{
			OnComError(hr);
		}
		else
		{
			if (bugIDOut)
			{
				m_sBugID = bugIDOut;
				SetDlgItemText(IDC_BUGID, m_sBugID);
			}
			m_cLogMessage.SetText((LPCTSTR)temp);
			BSTR HUGEP *pbRevNames;
			BSTR HUGEP *pbRevValues;

			HRESULT hr1 = SafeArrayAccessData(revPropNames, (void HUGEP**)&pbRevNames);
			if (SUCCEEDED(hr1))
			{
				HRESULT hr2 = SafeArrayAccessData(revPropValues, (void HUGEP**)&pbRevValues);
				if (SUCCEEDED(hr2))
				{
					if (revPropNames->rgsabound->cElements == revPropValues->rgsabound->cElements)
					{
						for (ULONG i = 0; i < revPropNames->rgsabound->cElements; i++)
						{
							m_revProps[pbRevNames[i]] = pbRevValues[i];
						}
					}
					SafeArrayUnaccessData(revPropValues);
				}
				SafeArrayUnaccessData(revPropNames);
			}
		}
	}
	else
	{
		// if IBugTraqProvider2 failed, try IBugTraqProvider
		CComPtr<IBugTraqProvider> pProvider = NULL;
		hr = m_BugTraqProvider.QueryInterface(&pProvider);
		if (FAILED(hr))
		{
			OnComError(hr);
			return;
		}

		if (FAILED(hr = pProvider->GetCommitMessage(GetSafeHwnd(), parameters, commonRoot, pathList, originalMessage, &temp)))
		{
			OnComError(hr);
		}
		else
			m_cLogMessage.SetText((LPCTSTR)temp);
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if (!m_ProjectProperties.sMessage.IsEmpty())
	{
		CString sBugID = m_ProjectProperties.FindBugID(m_sLogMessage);
		if (!sBugID.IsEmpty())
		{
			SetDlgItemText(IDC_BUGID, sBugID);
		}
	}

	m_cLogMessage.SetFocus();
}

LRESULT CCommitDlg::OnSVNStatusListCtrlCheckChanged(WPARAM, LPARAM)
{
	UpdateOKButton();
	return 0;
}

LRESULT CCommitDlg::OnSVNStatusListCtrlChangelistChanged(WPARAM count, LPARAM)
{
	DialogEnableWindow(IDC_KEEPLISTS, count > 0);
	return 0;
}

LRESULT CCommitDlg::OnCheck(WPARAM wnd, LPARAM)
{
	HWND hwnd = (HWND)wnd;
	if (hwnd == GetDlgItem(IDC_CHECKALL)->GetSafeHwnd())
		m_ListCtrl.Check(SVNSLC_SHOWEVERYTHING, true);
	else if (hwnd == GetDlgItem(IDC_CHECKNONE)->GetSafeHwnd())
		m_ListCtrl.Check(0, true);
	else if (hwnd == GetDlgItem(IDC_CHECKUNVERSIONED)->GetSafeHwnd())
		m_ListCtrl.Check(SVNSLC_SHOWUNVERSIONED, false);
	else if (hwnd == GetDlgItem(IDC_CHECKVERSIONED)->GetSafeHwnd())
		m_ListCtrl.Check(SVNSLC_SHOWVERSIONED, false);
	else if (hwnd == GetDlgItem(IDC_CHECKADDED)->GetSafeHwnd())
		m_ListCtrl.Check(SVNSLC_SHOWADDED|SVNSLC_SHOWADDEDHISTORY, false);
	else if (hwnd == GetDlgItem(IDC_CHECKDELETED)->GetSafeHwnd())
		m_ListCtrl.Check(SVNSLC_SHOWREMOVED|SVNSLC_SHOWMISSING, false);
	else if (hwnd == GetDlgItem(IDC_CHECKMODIFIED)->GetSafeHwnd())
		m_ListCtrl.Check(SVNSLC_SHOWMODIFIED|SVNSLC_SHOWREPLACED, false);
	else if (hwnd == GetDlgItem(IDC_CHECKFILES)->GetSafeHwnd())
		m_ListCtrl.Check(SVNSLC_SHOWFILES, false);
	else if (hwnd == GetDlgItem(IDC_CHECKDIRECTORIES)->GetSafeHwnd())
		m_ListCtrl.Check(SVNSLC_SHOWFOLDERS, false);

	return 0;
}

void CCommitDlg::UpdateOKButton()
{
	BOOL bValidLogSize = FALSE;

    if (m_cLogMessage.GetText().GetLength() >= m_ProjectProperties.nMinLogSize)
		bValidLogSize = !m_bBlock;

	LONG nSelectedItems = m_ListCtrl.GetSelected();
	DialogEnableWindow(IDOK, bValidLogSize && nSelectedItems>0);
}

void CCommitDlg::OnComError( HRESULT hr )
{
	COMError ce(hr);
	CString sErr;
	sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), ce.GetMessageAndDescription().c_str());
	CMessageBox::Show(m_hWnd, sErr, _T("TortoiseSVN"), MB_ICONERROR);
}

LRESULT CCommitDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_SPLITTER)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
			DoSize(pHdr->delta);
		}
		break;
	}

	return __super::DefWindowProc(message, wParam, lParam);
}

void CCommitDlg::SetSplitterRange()
{
	if ((m_ListCtrl)&&(m_cLogMessage))
	{
		CRect rcTop;
		m_cLogMessage.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		m_ListCtrl.GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		if (rcMiddle.Height() && rcMiddle.Width())
			m_wndSplitter.SetRange(rcTop.top+60, rcMiddle.bottom-80);
	}
}

void CCommitDlg::DoSize(int delta)
{
	RemoveAnchor(IDC_MESSAGEGROUP);
	RemoveAnchor(IDC_LOGMESSAGE);
	RemoveAnchor(IDC_SPLITTER);
	RemoveAnchor(IDC_LISTGROUP);
	RemoveAnchor(IDC_FILELIST);
	RemoveAnchor(IDC_SELECTLABEL);
	RemoveAnchor(IDC_CHECKALL);
	RemoveAnchor(IDC_CHECKNONE);
	RemoveAnchor(IDC_CHECKUNVERSIONED);
	RemoveAnchor(IDC_CHECKVERSIONED);
	RemoveAnchor(IDC_CHECKADDED);
	RemoveAnchor(IDC_CHECKDELETED);
	RemoveAnchor(IDC_CHECKMODIFIED);
	RemoveAnchor(IDC_CHECKFILES);
	RemoveAnchor(IDC_CHECKDIRECTORIES);
	CSplitterControl::ChangeHeight(&m_cLogMessage, delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_MESSAGEGROUP), delta, CW_TOPALIGN);
	CSplitterControl::ChangeHeight(&m_ListCtrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(GetDlgItem(IDC_LISTGROUP), -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SELECTLABEL), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKALL), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKNONE), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKUNVERSIONED), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKVERSIONED), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKADDED), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKDELETED), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKMODIFIED), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKFILES), 0, delta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CHECKDIRECTORIES), 0, delta);
	AddAnchor(IDC_MESSAGEGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPLITTER, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LISTGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SELECTLABEL, TOP_LEFT);
	AddAnchor(IDC_CHECKALL, TOP_LEFT);
	AddAnchor(IDC_CHECKNONE, TOP_LEFT);
	AddAnchor(IDC_CHECKUNVERSIONED, TOP_LEFT);
	AddAnchor(IDC_CHECKVERSIONED, TOP_LEFT);
	AddAnchor(IDC_CHECKADDED, TOP_LEFT);
	AddAnchor(IDC_CHECKDELETED, TOP_LEFT);
	AddAnchor(IDC_CHECKMODIFIED, TOP_LEFT);
	AddAnchor(IDC_CHECKFILES, TOP_LEFT);
	AddAnchor(IDC_CHECKDIRECTORIES, TOP_LEFT);
	ArrangeLayout();
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcLogMsg;
	m_cLogMessage.GetClientRect(rcLogMsg);
	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), m_DlgOrigRect.Height()-m_LogMsgOrigRect.Height()+rcLogMsg.Height()));

	SetSplitterRange();
	m_cLogMessage.Invalidate();
	GetDlgItem(IDC_LOGMESSAGE)->Invalidate();
}

void CCommitDlg::OnSize(UINT nType, int cx, int cy)
{
    // first, let the resizing take place
    __super::OnSize(nType, cx, cy);

    //set range
    SetSplitterRange();
}

void CCommitDlg::OnBnClickedShowexternals()
{
	m_tooltips.Pop();	// hide the tooltips
	UpdateData();
	m_regShowExternals = m_bShowExternals;
	if (!m_bBlock)
	{
		DWORD dwShow = m_ListCtrl.GetShowFlags();
		if (DWORD(m_regShowExternals))
			dwShow |= SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO|SVNSLC_SHOWEXTDISABLED;
		else
			dwShow &= ~(SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO|SVNSLC_SHOWEXTDISABLED);
		m_ListCtrl.Show(dwShow, CTSVNPathList(), 0, true, true);
		UpdateCheckLinks();
	}
}

void CCommitDlg::UpdateCheckLinks()
{
	DialogEnableWindow(IDC_CHECKALL, true);
	DialogEnableWindow(IDC_CHECKNONE, true);
	DialogEnableWindow(IDC_CHECKUNVERSIONED, m_ListCtrl.GetUnversionedCount() > 0);
	DialogEnableWindow(IDC_CHECKVERSIONED, m_ListCtrl.GetItemCount() > m_ListCtrl.GetUnversionedCount());
	DialogEnableWindow(IDC_CHECKADDED, m_ListCtrl.GetAddedCount() > 0);
	DialogEnableWindow(IDC_CHECKDELETED, m_ListCtrl.GetDeletedCount() > 0);
	DialogEnableWindow(IDC_CHECKMODIFIED, m_ListCtrl.GetModifiedCount() > 0);
	DialogEnableWindow(IDC_CHECKFILES, m_ListCtrl.GetFileCount() > 0);
	DialogEnableWindow(IDC_CHECKDIRECTORIES, m_ListCtrl.GetFolderCount() > 0);
	long disabledstate = STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE;

	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKALL)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKNONE)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKUNVERSIONED)->GetSafeHwnd(), PROPID_ACC_STATE, m_ListCtrl.GetUnversionedCount() > 0 ? STATE_SYSTEM_READONLY : disabledstate);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKVERSIONED)->GetSafeHwnd(), PROPID_ACC_STATE, m_ListCtrl.GetItemCount() > m_ListCtrl.GetUnversionedCount() ? STATE_SYSTEM_READONLY : disabledstate);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKADDED)->GetSafeHwnd(), PROPID_ACC_STATE, m_ListCtrl.GetAddedCount() > 0 ? STATE_SYSTEM_READONLY : disabledstate);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKDELETED)->GetSafeHwnd(), PROPID_ACC_STATE, m_ListCtrl.GetDeletedCount() > 0 ? STATE_SYSTEM_READONLY : disabledstate);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKMODIFIED)->GetSafeHwnd(), PROPID_ACC_STATE, m_ListCtrl.GetModifiedCount() > 0 ? STATE_SYSTEM_READONLY : disabledstate);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKFILES)->GetSafeHwnd(), PROPID_ACC_STATE, m_ListCtrl.GetFileCount() > 0 ? STATE_SYSTEM_READONLY : disabledstate);
	CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKDIRECTORIES)->GetSafeHwnd(), PROPID_ACC_STATE, m_ListCtrl.GetFolderCount() > 0 ? STATE_SYSTEM_READONLY : disabledstate);
}

void CCommitDlg::VersionCheck()
{
	CRegString regVer(_T("Software\\TortoiseSVN\\NewVersion"));
	CString vertemp = regVer;
	int major = _ttoi(vertemp);
	vertemp = vertemp.Mid(vertemp.Find('.')+1);
	int minor = _ttoi(vertemp);
	vertemp = vertemp.Mid(vertemp.Find('.')+1);
	int micro = _ttoi(vertemp);
	vertemp = vertemp.Mid(vertemp.Find('.')+1);
	int build = _ttoi(vertemp);
	BOOL bNewer = FALSE;
	if (major > TSVN_VERMAJOR)
		bNewer = TRUE;
	else if ((minor > TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
		bNewer = TRUE;
	else if ((micro > TSVN_VERMICRO)&&(minor == TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
		bNewer = TRUE;
	else if ((build > TSVN_VERBUILD)&&(micro == TSVN_VERMICRO)&&(minor == TSVN_VERMINOR)&&(major == TSVN_VERMAJOR))
		bNewer = TRUE;

	if (bNewer)
	{
		CRegString regDownText(_T("Software\\TortoiseSVN\\NewVersionText"));
		CRegString regDownLink(_T("Software\\TortoiseSVN\\NewVersionLink"), _T("http://tortoisesvn.net"));

		if (CString(regDownText).IsEmpty())
		{
			CString temp;
			temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
			regDownText = temp;
		}
		m_cUpdateLink.SetURL(CString(regDownLink));
		m_cUpdateLink.SetWindowText(CString(regDownText));
		m_cUpdateLink.SetColors(RGB(255,0,0), RGB(150,0,0));

		m_cUpdateLink.ShowWindow(SW_SHOW);
	}
}