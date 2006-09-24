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
#include "messagebox.h"
#include "SVNProgressDlg.h"
#include "LogDlg.h"
#include "TSVNPath.h"
#include "Registry.h"
#include "SVNStatus.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "SoundUtils.h"
#include "SVNDiff.h"
#include "Hooks.h"

BOOL	CSVNProgressDlg::m_bAscending = FALSE;
int		CSVNProgressDlg::m_nSortedColumn = -1;

#define TRANSFERTIMER 100

IMPLEMENT_DYNAMIC(CSVNProgressDlg, CResizableStandAloneDialog)
CSVNProgressDlg::CSVNProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSVNProgressDlg::IDD, pParent)
	, m_Revision(_T("HEAD"))
	, m_RevisionEnd(0)
	, m_bLockWarning(false)
	, m_bCancelled(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bConflictsOccurred(FALSE)
	, m_bErrorsOccurred(FALSE)
	, m_bMergesAddsDeletesOccurred(FALSE)
	, m_pThread(NULL)
	, m_options(ProgOptNone)
	, m_dwCloseOnEnd(0)
{

	m_pSvn = this;
}

CSVNProgressDlg::~CSVNProgressDlg()
{
	for (size_t i=0; i<m_arData.size(); i++)
	{
		delete m_arData[i];
	} 
	if(m_pThread != NULL)
	{
		delete m_pThread;
	}
}

void CSVNProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SVNPROGRESS, m_ProgList);
}

BEGIN_MESSAGE_MAP(CSVNProgressDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_LOGBUTTON, OnBnClickedLogbutton)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SVNPROGRESS, OnNMCustomdrawSvnprogress)
	ON_WM_CLOSE()
	ON_NOTIFY(NM_DBLCLK, IDC_SVNPROGRESS, OnNMDblclkSvnprogress)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickSvnprogress)
	ON_WM_SETCURSOR()
	ON_WM_CONTEXTMENU()
	ON_REGISTERED_MESSAGE(WM_SVNPROGRESS, OnSVNProgress)
	ON_WM_TIMER()
	ON_EN_SETFOCUS(IDC_INFOTEXT, &CSVNProgressDlg::OnEnSetfocusInfotext)
END_MESSAGE_MAP()

BOOL CSVNProgressDlg::Cancel()
{
	return m_bCancelled;
}

void CSVNProgressDlg::AddItemToList(const NotificationData* pData)
{
	int count = m_ProgList.GetItemCount();

	int iInsertedAt = m_ProgList.InsertItem(count, pData->sActionColumnText);
	if (iInsertedAt != -1)
	{
		m_ProgList.SetItemText(iInsertedAt, 1, pData->sPathColumnText);
		m_ProgList.SetItemText(iInsertedAt, 2, pData->mime_type);

		// make columns width fit
		if (iFirstResized < 30)
		{
			// only resize the columns for the first 30 or so entries.
			// after that, don't resize them anymore because that's an
			// expensive function call and the columns will be sized
			// close enough already.
			ResizeColumns();
			iFirstResized++;
		}

		// Make sure the item is *entirely* visible even if the horizontal
		// scroll bar is visible.
		int count = m_ProgList.GetCountPerPage();
		if (iInsertedAt <= (m_ProgList.GetTopIndex() + count + 2))
			m_ProgList.EnsureVisible(iInsertedAt, false);
	}
}
BOOL CSVNProgressDlg::Notify(const CTSVNPath& path, svn_wc_notify_action_t action, 
							 svn_node_kind_t kind, const CString& mime_type, 
							 svn_wc_notify_state_t content_state, 
							 svn_wc_notify_state_t prop_state, LONG rev,
							 const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
							 svn_error_t * err, apr_pool_t * /*pool*/)
{
	bool bNoNotify = false;
	bool bDoAddData = true;
	NotificationData * data = new NotificationData();
	data->path = path;
	data->action = action;
	data->kind = kind;
	data->mime_type = mime_type;
	data->content_state = content_state;
	data->prop_state = prop_state;
	data->rev = rev;
	data->lock_state = lock_state;
	if ((lock)&&(lock->owner))
		data->owner = CUnicodeUtils::GetUnicode(lock->owner);
	data->sPathColumnText = path.GetUIPathString();
	if (!m_basePath.IsEmpty())
		data->basepath = m_basePath;
	switch (data->action)
	{
	case svn_wc_notify_add:
	case svn_wc_notify_update_add:
		m_bMergesAddsDeletesOccurred = true;
		data->sActionColumnText.LoadString(IDS_SVNACTION_ADD);
		data->color = m_Colors.GetColor(CColors::Added);
		break;
	case svn_wc_notify_commit_added:
		data->sActionColumnText.LoadString(IDS_SVNACTION_ADDING);
		data->color = m_Colors.GetColor(CColors::Added);
		break;
	case svn_wc_notify_copy:
		data->sActionColumnText.LoadString(IDS_SVNACTION_COPY);
		break;
	case svn_wc_notify_commit_modified:
		data->sActionColumnText.LoadString(IDS_SVNACTION_MODIFIED);
		data->color = m_Colors.GetColor(CColors::Modified);
		break;
	case svn_wc_notify_delete:
	case svn_wc_notify_update_delete:
		data->sActionColumnText.LoadString(IDS_SVNACTION_DELETE);
		m_bMergesAddsDeletesOccurred = true;
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_commit_deleted:
		data->sActionColumnText.LoadString(IDS_SVNACTION_DELETING);
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_restore:
		data->sActionColumnText.LoadString(IDS_SVNACTION_RESTORE);
		break;
	case svn_wc_notify_revert:
		data->sActionColumnText.LoadString(IDS_SVNACTION_REVERT);
		break;
	case svn_wc_notify_resolved:
		data->sActionColumnText.LoadString(IDS_SVNACTION_RESOLVE);
		break;
	case svn_wc_notify_commit_replaced:
		data->sActionColumnText.LoadString(IDS_SVNACTION_REPLACED);
		data->color = m_Colors.GetColor(CColors::Deleted);
		break;
	case svn_wc_notify_update_update:
		// if this is an inoperative dir change, don't show the nofification.
		// an inoperative dir change is when a directory gets updated without
		// any real change in either text or properties.
		if ((kind == svn_node_dir)
			&& ((prop_state == svn_wc_notify_state_inapplicable)
			|| (prop_state == svn_wc_notify_state_unknown)
			|| (prop_state == svn_wc_notify_state_unchanged)))
		{
			bNoNotify = true;
			break;
		}
		if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
		{
			data->color = m_Colors.GetColor(CColors::Conflict);
			data->bConflictedActionItem = true;
			m_bConflictsOccurred = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_CONFLICTED);
		}
		else if ((data->content_state == svn_wc_notify_state_merged) || (data->prop_state == svn_wc_notify_state_merged))
		{
			data->color = m_Colors.GetColor(CColors::Merged);
			m_bMergesAddsDeletesOccurred = true;
			data->sActionColumnText.LoadString(IDS_SVNACTION_MERGED);
		}
		else if (((data->content_state != svn_wc_notify_state_unchanged)&&(data->content_state != svn_wc_notify_state_unknown)) || 
			((data->prop_state != svn_wc_notify_state_unchanged)&&(data->prop_state != svn_wc_notify_state_unknown)))
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_UPDATE);
		}
		else
		{
			bNoNotify = true;
			break;
		}
		if (lock_state == svn_wc_notify_lock_state_unlocked)
		{
			CString temp(MAKEINTRESOURCE(IDS_SVNACTION_UNLOCKED));
			data->sActionColumnText += _T(", ") + temp;
		}
		break;

	case svn_wc_notify_update_external:
		// For some reason we build a list of externals...
		m_ExtStack.AddHead(path.GetUIPathString());
		data->sActionColumnText.LoadString(IDS_SVNACTION_EXTERNAL);
		data->bAuxItem = true;
		break;

	case svn_wc_notify_update_completed:
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_COMPLETED);
			data->bAuxItem = true;
			bool bEmpty = !!m_ExtStack.IsEmpty();
			if (!bEmpty)
				data->sPathColumnText.Format(IDS_PROGRS_PATHATREV, (LPCTSTR)m_ExtStack.RemoveHead(), rev);
			else
				data->sPathColumnText.Format(IDS_PROGRS_ATREV, rev);

			if ((m_bConflictsOccurred)&&(bEmpty))
			{
				// We're going to add another aux item - let's shove this current onto the list first
				// I don't really like this, but it will do for the moment.
				m_arData.push_back(data);
				AddItemToList(data);

				data = new NotificationData();
				data->bAuxItem = true;
				data->sActionColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED_WARNING);
				data->sPathColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED);
				data->color = m_Colors.GetColor(CColors::Conflict);
				CSoundUtils::PlayTSVNWarning();
				// This item will now be added after the switch statement
			}
			if (!m_basePath.IsEmpty())
				m_FinishedRevMap[m_basePath.GetSVNApiPath()] = rev;
			m_RevisionEnd = rev;

		}
		break;
	case svn_wc_notify_commit_postfix_txdelta:
		data->sActionColumnText.LoadString(IDS_SVNACTION_POSTFIX);
		break;
	case svn_wc_notify_failed_revert:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDREVERT);
		break;
	case svn_wc_notify_status_completed:
	case svn_wc_notify_status_external:
		data->sActionColumnText.LoadString(IDS_SVNACTION_STATUS);
		break;
	case svn_wc_notify_skip:
		if (content_state == svn_wc_notify_state_missing)
		{
			data->sActionColumnText.LoadString(IDS_SVNACTION_SKIPMISSING);

			// The color settings dialog describes the red color with
			// "possible or real conflict / obstructed" which also applies to
			// skipped targets during a merge. So we just use the same color.
			data->color = m_Colors.GetColor(CColors::Conflict);
		}
		else
			data->sActionColumnText.LoadString(IDS_SVNACTION_SKIP);
		break;
	case svn_wc_notify_locked:
		if ((lock)&&(lock->owner))
			data->sActionColumnText.Format(IDS_SVNACTION_LOCKEDBY, CUnicodeUtils::GetUnicode(lock->owner));
		break;
	case svn_wc_notify_unlocked:
		data->sActionColumnText.LoadString(IDS_SVNACTION_UNLOCKED);
		break;
	case svn_wc_notify_failed_lock:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDLOCK);
		m_arData.push_back(data);
		AddItemToList(data);
		ReportError(SVN::GetErrorString(err));
		bDoAddData = false;
		if (err->apr_err == SVN_ERR_FS_OUT_OF_DATE)
			m_bLockWarning = true;
		break;
	case svn_wc_notify_failed_unlock:
		data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDUNLOCK);
		m_arData.push_back(data);
		AddItemToList(data);
		ReportError(SVN::GetErrorString(err));
		bDoAddData = false;
		if (err->apr_err == SVN_ERR_FS_OUT_OF_DATE)
			m_bLockWarning = true;
		break;
	default:
		break;
	} // switch (action)

	if (bNoNotify)
		delete data;
	else
	{
		if (bDoAddData)
		{
			m_arData.push_back(data);
			AddItemToList(data);
		}
		if ((action == svn_wc_notify_commit_postfix_txdelta)&&(bSecondResized == FALSE))
		{
			ResizeColumns();
			bSecondResized = TRUE;
		}
	}

	return TRUE;
}

CString CSVNProgressDlg::BuildInfoString()
{
	CString infotext;
	CString temp;
	int added = 0;
	int copied = 0;
	int deleted = 0;
	int restored = 0;
	int reverted = 0;
	int resolved = 0;
	int conflicted = 0;
	int updated = 0;
	int merged = 0;
	int modified = 0;
	int skipped = 0;
	int replaced = 0;

	for (size_t i=0; i<m_arData.size(); ++i)
	{
		const NotificationData * dat = m_arData[i];
		switch (dat->action)
		{
		case svn_wc_notify_add:
		case svn_wc_notify_update_add:
		case svn_wc_notify_commit_added:
			added++;
			break;
		case svn_wc_notify_copy:
			copied++;
			break;
		case svn_wc_notify_delete:
		case svn_wc_notify_update_delete:
		case svn_wc_notify_commit_deleted:
			deleted++;
			break;
		case svn_wc_notify_restore:
			restored++;
			break;
		case svn_wc_notify_revert:
			reverted++;
			break;
		case svn_wc_notify_resolved:
			resolved++;
			break;
		case svn_wc_notify_update_update:
			if (dat->bConflictedActionItem)
				conflicted++;
			else if ((dat->content_state == svn_wc_notify_state_merged) || (dat->prop_state == svn_wc_notify_state_merged))
				merged++;
			else
				updated++;
			break;
		case svn_wc_notify_commit_modified:
			modified++;
			break;
		case svn_wc_notify_skip:
			skipped++;
			break;
		case svn_wc_notify_commit_replaced:
			replaced++;
			break;
		}
	}
	if (conflicted)
	{
		temp.LoadString(IDS_SVNACTION_CONFLICTED);
		infotext += temp;
		temp.Format(_T(":%d "), conflicted);
		infotext += temp;
	}
	if (skipped)
	{
		temp.LoadString(IDS_SVNACTION_SKIP);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), skipped);
	}
	if (merged)
	{
		temp.LoadString(IDS_SVNACTION_MERGED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), merged);
	}
	if (added)
	{
		temp.LoadString(IDS_SVNACTION_ADD);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), added);
	}
	if (deleted)
	{
		temp.LoadString(IDS_SVNACTION_DELETE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), deleted);
	}
	if (modified)
	{
		temp.LoadString(IDS_SVNACTION_MODIFIED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), modified);
	}
	if (copied)
	{
		temp.LoadString(IDS_SVNACTION_COPY);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), copied);
	}
	if (replaced)
	{
		temp.LoadString(IDS_SVNACTION_REPLACED);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), replaced);
	}
	if (updated)
	{
		temp.LoadString(IDS_SVNACTION_UPDATE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), updated);
	}
	if (restored)
	{
		temp.LoadString(IDS_SVNACTION_RESTORE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), restored);
	}
	if (reverted)
	{
		temp.LoadString(IDS_SVNACTION_REVERT);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), reverted);
	}
	if (resolved)
	{
		temp.LoadString(IDS_SVNACTION_RESOLVE);
		infotext += temp;
		infotext.AppendFormat(_T(":%d "), resolved);
	}
	return infotext;
}

void CSVNProgressDlg::SetParams(Command cmd, int options, const CTSVNPathList& pathList, const CString& url /* = "" */, const CString& message /* = "" */, SVNRev revision /* = -1 */)
{
	m_Command = cmd;
	m_options = options;

	m_targetPathList = pathList;

	m_url.SetFromUnknown(url);
	m_sMessage = message;
	m_Revision = revision;
}

void CSVNProgressDlg::ResizeColumns()
{
	m_ProgList.SetRedraw(FALSE);

	CAppUtils::ResizeAllListCtrlCols(&m_ProgList);

	m_ProgList.SetRedraw(TRUE);	
}

BOOL CSVNProgressDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_ProgList.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_DOUBLEBUFFER);

	m_ProgList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_ProgList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_ProgList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_PROGRS_ACTION);
	m_ProgList.InsertColumn(0, temp);
	temp.LoadString(IDS_PROGRS_PATH);
	m_ProgList.InsertColumn(1, temp);
	temp.LoadString(IDS_PROGRS_MIMETYPE);
	m_ProgList.InsertColumn(2, temp);

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}

	UpdateData(FALSE);

	// Call this early so that the column headings aren't hidden before any
	// text gets added.
	ResizeColumns();

	AddAnchor(IDC_SVNPROGRESS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESSLABEL, BOTTOM_LEFT, BOTTOM_CENTER);
	AddAnchor(IDC_PROGRESSBAR, BOTTOM_CENTER, BOTTOM_RIGHT);
	AddAnchor(IDC_INFOTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_LOGBUTTON, BOTTOM_RIGHT);
	this->SetPromptParentWindow(this->m_hWnd);
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("SVNProgressDlg"));
	return TRUE;
}

void CSVNProgressDlg::ReportSVNError()
{
	ReportError(m_pSvn->GetLastErrorMessage(MAX_PATH));
}

void CSVNProgressDlg::ReportError(const CString& sError)
{
	CSoundUtils::PlayTSVNError();
	ReportString(sError, CString(MAKEINTRESOURCE(IDS_ERR_ERROR)), m_Colors.GetColor(CColors::Conflict));
	m_bErrorsOccurred = true;
}

void CSVNProgressDlg::ReportWarning(const CString& sWarning)
{
	CSoundUtils::PlayTSVNWarning();
	ReportString(sWarning, CString(MAKEINTRESOURCE(IDS_WARN_WARNING)), m_Colors.GetColor(CColors::Conflict));
}

void CSVNProgressDlg::ReportNotification(const CString& sNotification)
{
	CSoundUtils::PlayTSVNNotification();
	ReportString(sNotification, CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
}

void CSVNProgressDlg::ReportString(CString sMessage, const CString& sMsgKind, COLORREF color)
{
	// instead of showing a dialog box with the error message or notification,
	// just insert the error text into the list control.
	// that way the user isn't 'interrupted' by a dialog box popping up!

	// the message may be split up into different lines
	// so add a new entry for each line of the message
	while (!sMessage.IsEmpty())
	{
		NotificationData * data = new NotificationData();
		data->bAuxItem = true;
		data->sActionColumnText = sMsgKind;
		if (sMessage.Find('\n')>=0)
			data->sPathColumnText = sMessage.Left(sMessage.Find('\n'));
		else
			data->sPathColumnText = sMessage;		
		data->sPathColumnText.Trim(_T("\n\r"));
		data->color = color;
		if (sMessage.Find('\n')>=0)
		{
			sMessage = sMessage.Mid(sMessage.Find('\n'));
			sMessage.Trim(_T("\n\r"));
		}
		else
			sMessage.Empty();
		m_arData.push_back(data);
		AddItemToList(data);
	}
}

UINT CSVNProgressDlg::ProgressThreadEntry(LPVOID pVoid)
{
	return ((CSVNProgressDlg*)pVoid)->ProgressThread();
}

UINT CSVNProgressDlg::ProgressThread()
{
	// The SetParams function should have loaded something for us

	CString temp;
	CString sWindowTitle;
	bool localoperation = false;

	DialogEnableWindow(IDOK, FALSE);
	DialogEnableWindow(IDCANCEL, TRUE);
	SetAndClearProgressInfo(m_hWnd);
	InterlockedExchange(&m_bThreadRunning, TRUE);
	iFirstResized = 0;
	bSecondResized = FALSE;
	switch (m_Command)
	{
	case Checkout:			//no tempfile!
		{
			ASSERT(m_targetPathList.GetCount() == 1);
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_CHECKOUT);
			CTSVNPathList urls;
			urls.LoadFromAsteriskSeparatedString(m_url.GetSVNPathString());
			CTSVNPath checkoutdir = m_targetPathList[0];
			for (int i=0; i<urls.GetCount(); ++i)
			{
				sWindowTitle = urls[i].GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
				SetWindowText(sWindowTitle);
				checkoutdir = m_targetPathList[0];
				if (urls.GetCount() > 1)
					checkoutdir.AppendPathString(urls[i].GetFileOrDirectoryName());
				if (!m_pSvn->Checkout(urls[i], checkoutdir, m_Revision, m_Revision, m_options & ProgOptRecursive, m_options & ProgOptIgnoreExternals))
				{
					if (m_ProgList.GetItemCount()!=0)
					{
						ReportSVNError();
					}
					// if the checkout fails with the peg revision set to the checkout revision,
					// try again with HEAD as the peg revision.
					else if (!m_pSvn->Checkout(urls[i], checkoutdir, SVNRev::REV_HEAD, m_Revision, m_options & ProgOptRecursive, m_options & ProgOptIgnoreExternals))
					{
						ReportSVNError();
					}
				}
			}
		}
		break;
	case Import:			//no tempfile!
		ASSERT(m_targetPathList.GetCount() == 1);
		sWindowTitle.LoadString(IDS_PROGRS_TITLE_IMPORT);
		sWindowTitle = m_targetPathList[0].GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
		SetWindowText(sWindowTitle);
		if (!m_pSvn->Import(m_targetPathList[0], m_url, m_sMessage, true, m_options & ProgOptIncludeIgnored ? true : false))
		{
			ReportSVNError();
		}
		break;
	case Update:
		sWindowTitle.LoadString(IDS_PROGRS_TITLE_UPDATE);
		SetWindowText(sWindowTitle);
		{
			int targetcount = m_targetPathList.GetCount();
			CString sfile;
			CStringA uuid;
			StringRevMap uuidmap;
			bool bRecursive = !!(m_options & ProgOptRecursive);
			SVNRev revstore = m_Revision;
			int nUUIDs = 0;
			for(int nItem = 0; nItem < targetcount; nItem++)
			{
				const CTSVNPath& targetPath = m_targetPathList[nItem];
				SVNStatus st;
				LONG headrev = -1;
				m_Revision = revstore;
				if (m_Revision.IsHead())
				{
					if ((targetcount > 1)&&((headrev = st.GetStatus(targetPath, true)) != (-2)))
					{
						if (st.status->entry != NULL)
						{

							m_UpdateStartRevMap[targetPath.GetSVNApiPath()] = st.status->entry->cmt_rev;
							if (st.status->entry->uuid)
							{
								uuid = st.status->entry->uuid;
								StringRevMap::iterator iter = uuidmap.lower_bound(uuid);
								if (iter == uuidmap.end() || iter->first != uuid)
								{
									uuidmap.insert(iter, std::make_pair(uuid, headrev));
									nUUIDs++;
								}
								else
									headrev = iter->second;
								m_Revision = headrev;
							}
							else
								m_Revision = headrev;
						}
					}
					else
					{
						if ((headrev = st.GetStatus(targetPath, FALSE)) != (-2))
						{
							if (st.status->entry != NULL)
								m_UpdateStartRevMap[targetPath.GetSVNApiPath()] = st.status->entry->cmt_rev;
						}
					}
				} // if (m_Revision.IsHead()) 
			} // for(int nItem = 0; nItem < m_targetPathList.GetCount(); nItem++)
			if (m_targetPathList.GetCount() > 1)
			{
				sWindowTitle = m_targetPathList.GetCommonRoot().GetWinPathString()+_T(" - ")+sWindowTitle;
				SetWindowText(sWindowTitle);
			}
			else if (m_targetPathList.GetCount() == 1)
			{
				sWindowTitle = m_targetPathList[0].GetWinPathString()+_T(" - ")+sWindowTitle;
				SetWindowText(sWindowTitle);
			}

			DWORD exitcode = 0;
			CString error;
			if (CHooks::Instance().PreUpdate(m_targetPathList, bRecursive, nUUIDs > 1 ? revstore : m_Revision, exitcode, error))
			{
				if (exitcode)
				{
					CString temp;
					temp.Format(IDS_ERR_HOOKFAILED, error);
					ReportError(error);
					break;
				}
			}
			if (nUUIDs > 1)
			{
				// the selected items are from different repositories,
				// so we have to update them separately
				for(int nItem = 0; nItem < targetcount; nItem++)
				{
					const CTSVNPath& targetPath = m_targetPathList[nItem];
					m_basePath = targetPath;
					CString sNotify;
					sNotify.Format(IDS_PROGRS_UPDATEPATH, m_basePath.GetWinPath());
					ReportNotification(sNotify);
					if (!m_pSvn->Update(CTSVNPathList(targetPath), revstore, bRecursive, m_options & ProgOptIgnoreExternals))
					{
						ReportSVNError();
						break;
					}
				}
			}
			else if (!m_pSvn->Update(m_targetPathList, m_Revision, bRecursive, m_options & ProgOptIgnoreExternals))
			{
				ReportSVNError();
				break;
			}
			if (CHooks::Instance().PostUpdate(m_targetPathList, bRecursive, m_RevisionEnd, exitcode, error))
			{
				if (exitcode)
				{
					CString temp;
					temp.Format(IDS_ERR_HOOKFAILED, error);
					ReportError(error);
					break;
				}
			}

			// after an update, show the user the log button, but only if only one single item was updated
			// (either a file or a directory)
			if ((m_targetPathList.GetCount() == 1)&&(m_UpdateStartRevMap.size()>0))
				GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
		} 
		break;
	case Commit:
		{
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_COMMIT);
			SetWindowText(sWindowTitle);
			if (m_targetPathList.GetCount()==0)
			{
				SetWindowText(sWindowTitle);
				temp.LoadString(IDS_MSGBOX_OK);

				DialogEnableWindow(IDCANCEL, FALSE);
				DialogEnableWindow(IDOK, TRUE);

				InterlockedExchange(&m_bThreadRunning, FALSE);
				break;
			}
			if (m_targetPathList.GetCount()==1)
			{
				sWindowTitle = m_targetPathList[0].GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
				SetWindowText(sWindowTitle);
			}
			BOOL isTag = FALSE;
			BOOL bURLFetched = FALSE;
			CString url;
			for (int i=0; i<m_targetPathList.GetCount(); ++i)
			{
				if (bURLFetched == FALSE)
				{
					url = m_pSvn->GetURLFromPath(m_targetPathList[i]);
					if (!url.IsEmpty())
						bURLFetched = TRUE;
					CString urllower = url;
					urllower.MakeLower();
					// test if the commit goes to a tag.
					// now since Subversion doesn't force users to
					// create tags in the recommended /tags/ folder
					// only a warning is shown. This won't work if the tags
					// are stored in a non-recommended place, but the check
					// still helps those who do.
					if (urllower.Find(_T("/tags/"))>=0)
						isTag = TRUE;
					break;
				}
			}
			if (isTag)
			{
				if (CMessageBox::Show(m_hWnd, IDS_PROGRS_COMMITT_TRUNK, IDS_APPNAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION)==IDNO)
					break;
			}
			DWORD exitcode = 0;
			CString error;
			if (CHooks::Instance().PreCommit(m_targetPathList, (m_Revision == 0), exitcode, error))
			{
				if (exitcode)
				{
					CString temp;
					temp.Format(IDS_ERR_HOOKFAILED, error);
					ReportError(error);
					break;
				}
			}

			if (!m_pSvn->Commit(m_targetPathList, m_sMessage, (m_Revision == 0), m_options & ProgOptKeeplocks))
			{
				ReportSVNError();
				// if a non-recursive commit failed with SVN_ERR_UNSUPPORTED_FEATURE,
				// that means a folder deletion couldn't be committed.
				if ((m_Revision != 0)&&(m_pSvn->Err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE))
				{
					ReportError(CString(MAKEINTRESOURCE(IDS_PROGRS_NONRECURSIVEHINT)));
				}
			}
			if (CHooks::Instance().PostCommit(m_targetPathList, (m_Revision == 0), m_RevisionEnd, exitcode, error))
			{
				if (exitcode)
				{
					CString temp;
					temp.Format(IDS_ERR_HOOKFAILED, error);
					ReportError(error);
					break;
				}
			}
		}
		break;
	case Add:
		localoperation = true;
		sWindowTitle.LoadString(IDS_PROGRS_TITLE_ADD);
		SetWindowText(sWindowTitle);
		if (!m_pSvn->Add(m_targetPathList, false, FALSE, TRUE))
		{
			ReportSVNError();
		}
		break;
	case Revert:
		localoperation = true;
		sWindowTitle.LoadString(IDS_PROGRS_TITLE_REVERT);
		SetWindowText(sWindowTitle);
		if (!m_pSvn->Revert(m_targetPathList, !!(m_options & ProgOptRecursive)))
		{
			ReportSVNError();
			break;
		}
		break;
	case Resolve:
		{
			localoperation = true;
			ASSERT(m_targetPathList.GetCount() == 1);
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_RESOLVE);
			SetWindowText(sWindowTitle);
			// check if the file may still have conflict markers in it.
			BOOL bMarkers = FALSE;
			try
			{
				for (INT_PTR fileindex=0; (fileindex<m_targetPathList.GetCount()) && (bMarkers==FALSE); ++fileindex)
				{
					if (!m_targetPathList[fileindex].IsDirectory())
					{
						CStdioFile file(m_targetPathList[fileindex].GetWinPath(), CFile::typeBinary | CFile::modeRead);
						CString strLine = _T("");
						while (file.ReadString(strLine))
						{
							if (strLine.Find(_T("<<<<<<<"))==0)
							{
								bMarkers = TRUE;
								break;
							}
						}
						file.Close();
					}
				}
			} 
			catch (CFileException* pE)
			{
				TRACE(_T("CFileException in Resolve!\n"));
				TCHAR error[10000] = {0};
				pE->GetErrorMessage(error, 10000);
				ReportError(error);
				pE->Delete();
			}
			if (bMarkers)
			{
				if (CMessageBox::Show(m_hWnd, IDS_PROGRS_REVERTMARKERS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)==IDYES)
				{
					for (INT_PTR fileindex=0; fileindex<m_targetPathList.GetCount(); ++fileindex)
						m_pSvn->Resolve(m_targetPathList[fileindex], true);
				}
			}
			else
			{
				for (INT_PTR fileindex=0; fileindex<m_targetPathList.GetCount(); ++fileindex)
					m_pSvn->Resolve(m_targetPathList[fileindex], true);
			}
		}
		break;
	case Switch:
		{
			ASSERT(m_targetPathList.GetCount() == 1);
			SVNStatus st;
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_SWITCH);
			SetWindowText(sWindowTitle);
			LONG rev = 0;
			if (st.GetStatus(m_targetPathList[0]) != (-2))
			{
				if (st.status->entry != NULL)
				{
					rev = st.status->entry->revision;
				}
			}
			if (!m_pSvn->Switch(m_targetPathList[0], m_url, m_Revision, true))
			{
				ReportSVNError();
				break;
			}
			m_UpdateStartRevMap[m_targetPathList[0].GetSVNApiPath()] = rev;
			if ((m_RevisionEnd >= 0)&&(rev >= 0)
				&&((LONG)m_RevisionEnd > (LONG)rev))
				GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
		}
		break;
	case Export:
		ASSERT(m_targetPathList.GetCount() == 1);
		sWindowTitle.LoadString(IDS_PROGRS_TITLE_EXPORT);
		sWindowTitle = m_url.GetUIFileOrDirectoryName()+_T(" - ")+sWindowTitle;
		SetWindowText(sWindowTitle);
		if (!m_pSvn->Export(m_url, m_targetPathList[0], m_Revision, m_Revision, TRUE, m_options & ProgOptIgnoreExternals))
		{
			ReportSVNError();
		}
		break;
	case Merge:
		{
			ASSERT(m_targetPathList.GetCount() == 1);
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGE);
			if (m_options & ProgOptDryRun)
			{
				CString sDryRun(MAKEINTRESOURCE(IDS_PROGRS_DRYRUN));
				sWindowTitle += _T(" ") + sDryRun;
			}
			SetWindowText(sWindowTitle);
			// Eeek!  m_sMessage is actually a path for this command...
			CTSVNPath urlTo(m_sMessage);
			if (m_url.IsEquivalentTo(urlTo))
			{
				if (!m_pSvn->PegMerge(m_url, m_Revision, m_RevisionEnd, 
					m_pegRev.IsValid() ? m_pegRev : (m_url.IsUrl() ? m_RevisionEnd : SVNRev(SVNRev::REV_WC)),
					m_targetPathList[0], true, true, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun)))
				{
					// if the merge fails with the peg revision set to the end revision of the merge,
					// try again with HEAD as the peg revision.
					if (!m_pSvn->PegMerge(m_url, m_Revision, m_RevisionEnd, SVNRev::REV_HEAD,
						m_targetPathList[0], true, true, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun)))
					{
						ReportSVNError();
					}
				}
			}
			else
			{
				if (!m_pSvn->Merge(m_url, m_Revision, urlTo, m_RevisionEnd, m_targetPathList[0], 
					true, true, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun)))
				{
					ReportSVNError();
				}
			}
		}
		break;
	case Copy:
		{
			ASSERT(m_targetPathList.GetCount() == 1);
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_COPY);
			SetWindowText(sWindowTitle);
			if (!m_pSvn->Copy(m_targetPathList[0], m_url, m_Revision, m_sMessage))
			{
				ReportSVNError();
				break;
			}
			if (m_options & ProgOptSwitchAfterCopy)
			{
				if (!m_pSvn->Switch(m_targetPathList[0], m_url, SVNRev::REV_HEAD, true))
				{
					ReportSVNError();
					break;
				}
			}
			else
			{
				if (SVN::PathIsURL(m_url.GetSVNPathString()))
				{
					CString sMsg(MAKEINTRESOURCE(IDS_PROGRS_COPY_WARNING));
					ReportNotification(sMsg);
				}
			}
		}
		break;
	case Rename:
		{
			ASSERT(m_targetPathList.GetCount() == 1);
			if ((!m_targetPathList[0].IsUrl())&&(!m_url.IsUrl()))
				localoperation = true;
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_RENAME);
			SetWindowText(sWindowTitle);
			if (!m_pSvn->Move(m_targetPathList[0], m_url, m_Revision, m_sMessage))
			{
				ReportSVNError();
				break;
			}
		}
		break;
	case Lock:
		{
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_LOCK);
			SetWindowText(sWindowTitle);
			if (!m_pSvn->Lock(m_targetPathList, m_options & ProgOptLockForce, m_sMessage))
			{
				ReportSVNError();
				break;
			}
			if (m_bLockWarning)
			{
				// the lock failed, because the file was outdated.
				// ask the user wheter to update the file and try again
				if (CMessageBox::Show(m_hWnd, IDS_WARN_LOCKOUTDATED, IDS_APPNAME, MB_ICONQUESTION|MB_YESNO)==IDYES)
				{
					ReportString(CString(MAKEINTRESOURCE(IDS_SVNPROGRESS_UPDATEANDRETRY)), CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
					if (!m_pSvn->Update(m_targetPathList, SVNRev::REV_HEAD, false, true))
					{
						ReportSVNError();
					}
					if (!m_pSvn->Lock(m_targetPathList, m_options & ProgOptLockForce, m_sMessage))
					{
						ReportSVNError();
						break;
					}
				}
			}
		}
		break;
	case Unlock:
		{
			sWindowTitle.LoadString(IDS_PROGRS_TITLE_UNLOCK);
			SetWindowText(sWindowTitle);
			if (!m_pSvn->Unlock(m_targetPathList, m_options & ProgOptLockForce))
			{
				ReportSVNError();
				break;
			}
		}
		break;
	}
	temp.LoadString(IDS_PROGRS_TITLEFIN);
	sWindowTitle = sWindowTitle + _T(" ") + temp;
	SetWindowText(sWindowTitle);

	DialogEnableWindow(IDCANCEL, FALSE);
	DialogEnableWindow(IDOK, TRUE);

	CString info = BuildInfoString();
	GetDlgItem(IDC_INFOTEXT)->SetWindowText(info);
	ResizeColumns();
	int count = m_ProgList.GetItemCount();
	if (count > 0)
		m_ProgList.EnsureVisible(count-1, FALSE);
	SendMessage(DM_SETDEFID, IDOK);
	GetDlgItem(IDOK)->SetFocus();	

	KillTimer(TRANSFERTIMER);
	GetDlgItem(IDC_PROGRESSLABEL)->SetWindowText(m_sTotalBytesTransferred);
	GetDlgItem(IDC_PROGRESSBAR)->ShowWindow(SW_HIDE);

	m_bCancelled = TRUE;
	InterlockedExchange(&m_bThreadRunning, FALSE);
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);

	DWORD dwAutoClose = CRegStdWORD(_T("Software\\TortoiseSVN\\AutoClose"));
	if (m_options & ProgOptDryRun)
		dwAutoClose = 0;		// dry run means progress dialog doesn't autoclose at all
	if (m_dwCloseOnEnd != 0)
		dwAutoClose = m_dwCloseOnEnd;		// command line value has priority over setting value
	if ((dwAutoClose == CLOSE_NOERRORS)&&(!m_bErrorsOccurred))
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
	if ((dwAutoClose == CLOSE_NOCONFLICTS)&&(!m_bErrorsOccurred)&&(!m_bConflictsOccurred))
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
	if ((dwAutoClose == CLOSE_NOMERGES)&&(!m_bErrorsOccurred)&&(!m_bConflictsOccurred)&&(!m_bMergesAddsDeletesOccurred))
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
	if ((dwAutoClose == CLOSE_LOCAL)&&(!m_bErrorsOccurred)&&(!m_bConflictsOccurred)&&(localoperation))
		PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);

	//Don't do anything here which might cause messages to be sent to the window
	//The window thread is probably now blocked in OnOK if we've done an autoclose
	return 0;
}

void CSVNProgressDlg::OnBnClickedLogbutton()
{
	if (m_targetPathList.GetCount() != 1)
		return;
	StringRevMap::iterator it = m_UpdateStartRevMap.begin();
	svn_revnum_t rev = -1;
	if (it != m_UpdateStartRevMap.end())
	{
		rev = it->second;
	}
	CLogDlg dlg;
	dlg.SetParams(m_targetPathList[0], m_RevisionEnd, m_RevisionEnd, rev, 0, TRUE);
	dlg.DoModal();
}


void CSVNProgressDlg::OnClose()
{
	if (m_bCancelled)
	{
		TerminateThread(m_pThread->m_hThread, (DWORD)-1);
		InterlockedExchange(&m_bThreadRunning, FALSE);
	}
	else
	{
		m_bCancelled = TRUE;
		return;
	}
	DialogEnableWindow(IDCANCEL, TRUE);
	__super::OnClose();
}

void CSVNProgressDlg::OnOK()
{
	if ((m_bCancelled)&&(!m_bThreadRunning))
	{
		// I have made this wait a sensible amount of time (10 seconds) for the thread to finish
		// You must be careful in the thread that after posting the WM_COMMAND/IDOK message, you 
		// don't do any more operations on the window which might require message passing
		// If you try to send windows messages once we're waiting here, then the thread can't finished
		// because the Window's message loop is blocked at this wait
		WaitForSingleObject(m_pThread->m_hThread, 10000);
		__super::OnOK();
	}
	m_bCancelled = TRUE;
}

void CSVNProgressDlg::OnCancel()
{
	if ((m_bCancelled)&&(!m_bThreadRunning))
		__super::OnCancel();
	m_bCancelled = TRUE;
}

void CSVNProgressDlg::OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
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

		ASSERT(pLVCD->nmcd.dwItemSpec <  m_arData.size());
		if(pLVCD->nmcd.dwItemSpec >= m_arData.size())
		{
			return;
		}
		const NotificationData * data = m_arData[pLVCD->nmcd.dwItemSpec];
		ASSERT(data != NULL);
		if (data == NULL)
			return;

		// Store the color back in the NMLVCUSTOMDRAW struct.
		pLVCD->clrText = data->color;
	}
}

void CSVNProgressDlg::OnNMDblclkSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMLV->iItem < 0)
		return;
	if (m_options & ProgOptDryRun)
		return;	//don't do anything in a dry-run.

	const NotificationData * data = m_arData[pNMLV->iItem];
	ASSERT(data != NULL);
	if (data->bConflictedActionItem)
	{
		// We've double-clicked on a conflicted item - do a three-way merge on it
		SVNDiff::StartConflictEditor(data->path);
	}
	else if ((data->action == svn_wc_notify_update_update) && ((data->content_state == svn_wc_notify_state_merged)||(Enum_Merge == m_Command)))
	{
		// This is a modified file which has been merged on update
		// Diff it against base
		CTSVNPath temporaryFile;
		SVNDiff diff(this, this->m_hWnd, true);
		diff.DiffFileAgainstBase(data->path);
	}
}

void CSVNProgressDlg::OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	if (m_bThreadRunning)
		return;
	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = phdr->iItem;
	Sort();

	CString temp;
	m_ProgList.SetRedraw(FALSE);
	m_ProgList.DeleteAllItems();
	for (size_t i=0; i<m_arData.size(); i++)
	{
		AddItemToList(m_arData[i]);
	} 

	m_ProgList.SetRedraw(TRUE);

	*pResult = 0;
}

bool CSVNProgressDlg::NotificationDataIsAux(const NotificationData* pData)
{
	return pData->bAuxItem;
}

LRESULT CSVNProgressDlg::OnSVNProgress(WPARAM /*wParam*/, LPARAM lParam)
{
	SVNProgress * pProgressData = (SVNProgress *)lParam;
	CProgressCtrl * progControl = (CProgressCtrl *)GetDlgItem(IDC_PROGRESSBAR);
	if ((pProgressData->total > 1000)&&(!progControl->IsWindowVisible()))
	{
		progControl->ShowWindow(SW_SHOW);
	}
	if ((pProgressData->total < 0)&&(pProgressData->progress > 1000)&&(progControl->IsWindowVisible()))
	{
		progControl->ShowWindow(SW_HIDE);
	}
	if (!GetDlgItem(IDC_PROGRESSLABEL)->IsWindowVisible())
		GetDlgItem(IDC_PROGRESSLABEL)->ShowWindow(SW_SHOW);
	SetTimer(TRANSFERTIMER, 2000, NULL);
	if ((pProgressData->total > 0)&&(pProgressData->progress > 1000))
	{
		progControl->SetPos((int)pProgressData->progress);
		progControl->SetRange32(0, (int)pProgressData->total);
	}
	CString progText;
	if (pProgressData->overall_total < 1024)
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALBYTESTRANSFERRED, pProgressData->overall_total);	
	else if (pProgressData->overall_total < 1200000)
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALTRANSFERRED, pProgressData->overall_total / 1024);
	else
		m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALMBTRANSFERRED, (double)((double)pProgressData->overall_total / 1024000.0));
	progText.Format(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)m_sTotalBytesTransferred, pProgressData->SpeedString);
	GetDlgItem(IDC_PROGRESSLABEL)->SetWindowText(progText);
	return 0;
}

void CSVNProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TRANSFERTIMER)
	{
		CString progText;
		progText.Format(IDS_SVN_PROGRESS_TOTALANDSPEED, m_sTotalBytesTransferred, _T("0 Bytes/s"));
		GetDlgItem(IDC_PROGRESSLABEL)->SetWindowText(progText);
		KillTimer(TRANSFERTIMER);
	}
}

void CSVNProgressDlg::Sort()
{
	if(m_arData.size() < 2)
	{
		return;
	}

	// We need to sort the blocks which lie between the auxiliary entries
	// This is so that any aux data stays where it was
	NotificationDataVect::iterator actionBlockBegin;
	NotificationDataVect::iterator actionBlockEnd = m_arData.begin();	// We start searching from here

	for(;;)
	{
		// Search to the start of the non-aux entry in the next block
		actionBlockBegin = std::find_if(actionBlockEnd, m_arData.end(), std::not1(std::ptr_fun(&CSVNProgressDlg::NotificationDataIsAux)));
		if(actionBlockBegin == m_arData.end())
		{
			// There are no more actions
			break;
		}
		// Now search to find the end of the block
		actionBlockEnd = std::find_if(actionBlockBegin+1, m_arData.end(), std::ptr_fun(&CSVNProgressDlg::NotificationDataIsAux));
		// Now sort the block
		std::sort(actionBlockBegin, actionBlockEnd, &CSVNProgressDlg::SortCompare);
	}
}

bool CSVNProgressDlg::SortCompare(const NotificationData * pData1, const NotificationData * pData2)
{
	int result = 0;
	switch (m_nSortedColumn)
	{
	case 0:		//action column
		result = pData1->sActionColumnText.Compare(pData2->sActionColumnText);
		break;
	case 1:		//path column
		// Compare happens after switch()
		break;
	case 2:		//mime-type column
		result = pData1->mime_type.Compare(pData2->mime_type);
		break;
	default:
		break;
	}

	// Sort by path if everything else is equal
	if (result == 0)
	{
		result = CTSVNPath::Compare(pData1->path, pData2->path);
	}

	if (!m_bAscending)
		result = -result;
	return result < 0;
}

BOOL CSVNProgressDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (!GetDlgItem(IDOK)->IsWindowEnabled())
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&(pWnd == GetDlgItem(IDC_SVNPROGRESS)))
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

BOOL CSVNProgressDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			// pressing the ESC key should close the dialog. But since we disabled the escape
			// key (so the user doesn't get the idea that he could simply undo an e.g. update)
			// this won't work.
			// So if the user presses the ESC key, change it to VK_RETURN so the dialog gets
			// the impression that the OK button was pressed.
			if ((!m_bThreadRunning)&&(!GetDlgItem(IDCANCEL)->IsWindowEnabled())
				&&(GetDlgItem(IDOK)->IsWindowEnabled())&&(GetDlgItem(IDOK)->IsWindowVisible()))
			{
				// since we convert ESC to RETURN, make sure the OK button has the focus.
				GetDlgItem(IDOK)->SetFocus();
				pMsg->wParam = VK_RETURN;
			}
		}
		if (pMsg->wParam == 'A')
		{
			if (GetKeyState(VK_CONTROL)&0x8000)
			{
				// Ctrl-A -> select all
				m_ProgList.SetSelectionMark(0);
				for (int i=0; i<m_ProgList.GetItemCount(); ++i)
				{
					m_ProgList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
		}
		if (pMsg->wParam == 'C')
		{
			int selIndex = m_ProgList.GetSelectionMark();
			if (selIndex >= 0)
			{
				if (GetKeyState(VK_CONTROL)&0x8000)
				{
					//Ctrl-C -> copy to clipboard
					CStringA sClipdata;
					POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
					if (pos != NULL)
					{
						while (pos)
						{
							int nItem = m_ProgList.GetNextSelectedItem(pos);
							CString sAction = m_ProgList.GetItemText(nItem, 0);
							CString sPath = m_ProgList.GetItemText(nItem, 1);
							CString sMime = m_ProgList.GetItemText(nItem, 2);
							CString sLogCopyText;
							sLogCopyText.Format(_T("%s: %s  %s\r\n"),
								(LPCTSTR)sAction, (LPCTSTR)sPath, (LPCTSTR)sMime);
							sClipdata +=  CStringA(sLogCopyText);
						}
						CStringUtils::WriteAsciiStringToClipboard(sClipdata);
					}
				}
			}
		} 
	} // if (pMsg->message == WM_KEYDOWN)
	return __super::PreTranslateMessage(pMsg);
}

void CSVNProgressDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (m_options & ProgOptDryRun)
		return;	// don't do anything in a dry-run.

	if (pWnd == &m_ProgList)
	{
		int selIndex = m_ProgList.GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			// Menu was invoked from the keyboard rather than by right-clicking
			CRect rect;
			m_ProgList.GetItemRect(selIndex, &rect, LVIR_LABEL);
			m_ProgList.ClientToScreen(&rect);
			point = rect.CenterPoint();
		}

		if ((selIndex >= 0)&&(!m_bThreadRunning))
		{
			// entry is selected, thread has finished with updating so show the popup menu
			CMenu popup;
			if (popup.CreatePopupMenu())
			{
				CString temp;
				bool bAdded = false;
				if (m_ProgList.GetSelectedCount() == 1)
				{
					NotificationData * data = m_arData[selIndex];
					if ((data)&&(!data->path.IsDirectory()))
					{
						if (data->action == svn_wc_notify_update_update)
						{
							temp.LoadString(IDS_LOG_POPUP_COMPARE);
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
							bAdded = true;
							if (data->bConflictedActionItem)
							{
								temp.LoadString(IDS_MENUCONFLICT);
								popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITCONFLICT, temp);
								popup.SetDefaultItem(ID_EDITCONFLICT, FALSE);
								temp.LoadString(IDS_SVNPROGRESS_MENUMARKASRESOLVED);
								popup.AppendMenu(MF_STRING | MF_ENABLED, ID_CONFLICTRESOLVE, temp);
								temp.LoadString(IDS_SVNPROGRESS_MENUUSETHEIRS);
								popup.AppendMenu(MF_STRING | MF_ENABLED, ID_CONFLICTUSETHEIRS, temp);
								temp.LoadString(IDS_SVNPROGRESS_MENUUSEMINE);
								popup.AppendMenu(MF_STRING | MF_ENABLED, ID_CONFLICTUSEMINE, temp);
							}
							else if ((data->content_state == svn_wc_notify_state_merged)||(Enum_Merge == m_Command))
								popup.SetDefaultItem(ID_COMPARE, FALSE);
						}
						if ((data->action == svn_wc_notify_add)||
							(data->action == svn_wc_notify_update_add)||
							(data->action == svn_wc_notify_commit_added)||
							(data->action == svn_wc_notify_commit_modified)||
							(data->action == svn_wc_notify_restore)||
							(data->action == svn_wc_notify_revert)||
							(data->action == svn_wc_notify_resolved)||
							(data->action == svn_wc_notify_commit_replaced)||
							(data->action == svn_wc_notify_update_update))
						{
							temp.LoadString(IDS_MENULOG);
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_LOG, temp);
							if (data->action == svn_wc_notify_update_update)
								popup.AppendMenu(MF_SEPARATOR, NULL);
							temp.LoadString(IDS_LOG_POPUP_OPEN);
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPEN, temp);
							temp.LoadString(IDS_LOG_POPUP_OPENWITH);
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPENWITH, temp);
							bAdded = true;
						}
					} // if ((data)&&(!data->path.IsDirectory()))
					if (data)
					{
						CString sPath = CPathUtils::ParsePathInString(data->sPathColumnText);
						if ((!sPath.IsEmpty())&&(!SVN::PathIsURL(sPath)))
						{
							CTSVNPath path = CTSVNPath(sPath);
							if (path.GetDirectory().Exists())
							{
								temp.LoadString(IDS_SVNPROGRESS_MENUOPENPARENT);
								popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EXPLORE, temp);
								bAdded = true;
							}
						}
					}
					if (bAdded)
					{
						int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
						DialogEnableWindow(IDOK, FALSE);
						this->SetPromptApp(&theApp);
						theApp.DoWaitCursor(1);
						bool bOpenWith = false;
						switch (cmd)
						{
						case ID_EXPLORE:
							{
								CString sPath = CPathUtils::ParsePathInString(data->sPathColumnText);
								if (sPath.Find(':')<0)
								{
									// the path is not absolute: add the current directory in front of it
									DWORD len = GetCurrentDirectory(0, NULL);
									TCHAR * buf = new TCHAR[len+1];
									GetCurrentDirectory(len, buf);
									sPath = buf;
									sPath += _T("\\") + CPathUtils::ParsePathInString(data->sPathColumnText);
									delete [] buf;
								}

								CTSVNPath path = CTSVNPath(sPath);
								ShellExecute(m_hWnd, _T("explore"), path.GetDirectory().GetWinPath(), NULL, path.GetDirectory().GetWinPath(), SW_SHOW);
							}
							break;
						case ID_COMPARE:
							{
								svn_revnum_t rev = -1;
								StringRevMap::iterator it = m_UpdateStartRevMap.end();
								if (data->basepath.IsEmpty())
									it = m_UpdateStartRevMap.begin();
								else
									it = m_UpdateStartRevMap.find(data->basepath.GetSVNApiPath());
								if (it != m_UpdateStartRevMap.end())
									rev = it->second;
								// if the file was merged during update, do a three way diff between OLD, MINE, THEIRS
								if (data->content_state == svn_wc_notify_state_merged)
								{
									CTSVNPath basefile = CTempFiles::Instance().GetTempFilePath(true, data->path, rev);
									CTSVNPath newfile = CTempFiles::Instance().GetTempFilePath(true, data->path, SVNRev::REV_HEAD);
									SVN svn;
									if (!svn.Cat(data->path, SVNRev(SVNRev::REV_WC), rev, basefile))
									{
										ReportSVNError();
										DialogEnableWindow(IDOK, TRUE);
										break;
									}
									// If necessary, convert the line-endings on the file before diffing
									if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\ConvertBase"), TRUE))
									{
										CTSVNPath temporaryFile = CTempFiles::Instance().GetTempFilePath(false, data->path, SVNRev::REV_BASE);
										if (!svn.Cat(data->path, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE), temporaryFile))
										{
											temporaryFile.Reset();
											break;
										}
										else
										{
											newfile = temporaryFile;
										}
									}

									SetFileAttributes(newfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
									SetFileAttributes(basefile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
									CString revname, wcname, basename;
									revname.Format(_T("%s Revision %ld"), (LPCTSTR)data->path.GetUIFileOrDirectoryName(), rev);
									wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
									basename.Format(IDS_DIFF_BASENAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
									CAppUtils::StartExtMerge(basefile, newfile, data->path, data->path, basename, revname, wcname, CString(), true);
								}
								else
								{
									CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, data->path, rev);
									SVN svn;
									if (!svn.Cat(data->path, SVNRev(SVNRev::REV_WC), rev, tempfile))
									{
										ReportSVNError();
										DialogEnableWindow(IDOK, TRUE);
										break;
									}
									else
									{
										SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
										CString revname, wcname;
										revname.Format(_T("%s Revision %ld"), (LPCTSTR)data->path.GetUIFileOrDirectoryName(), rev);
										wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
										CAppUtils::StartExtDiff(tempfile, data->path, revname, wcname);
									}
								}
							}
							break;
						case ID_EDITCONFLICT:
							{
								CString sWinPath = data->path.GetWinPathString();
								if (sWinPath.Find(':')<0)
								{
									// the path is not absolute: add the current directory in front of it
									DWORD len = GetCurrentDirectory(0, NULL);
									TCHAR * buf = new TCHAR[len+1];
									GetCurrentDirectory(len, buf);
									sWinPath = buf;
									sWinPath += _T("\\") + data->path.GetWinPathString();
									delete [] buf;
								}
								SVNDiff::StartConflictEditor(CTSVNPath(sWinPath));
							}
							break;
						case ID_CONFLICTUSETHEIRS:
							{
								CTSVNPath directory = data->path.GetDirectory();
								CTSVNPath theirs(directory);
								SVNStatus stat;
								stat.GetStatus(data->path);
								if (stat.status->text_status != svn_wc_status_conflicted)
									break;
								if (stat.status && stat.status->entry)
								{
									if (stat.status->entry->conflict_new)
									{
										theirs.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
									}
								}
								CopyFile(theirs.GetWinPath(), data->path.GetWinPath(), FALSE);
								SVN svn;
								if (!svn.Resolve(data->path, FALSE))
								{
									ReportSVNError();
									DialogEnableWindow(IDOK, TRUE);
									break;
								}
								else
								{
									data->color = ::GetSysColor(COLOR_WINDOWTEXT);
									m_ProgList.Invalidate();
									CString msg;
									msg.Format(IDS_SVNPROGRESS_RESOLVED, data->path.GetWinPath());
									CMessageBox::Show(m_hWnd, msg, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
								}
							}
							break;
						case ID_CONFLICTUSEMINE:
							{
								CTSVNPath directory = data->path.GetDirectory();
								CTSVNPath mine(directory);
								SVNStatus stat;
								stat.GetStatus(data->path);
								if (stat.status->text_status != svn_wc_status_conflicted)
									break;
								if (stat.status && stat.status->entry)
								{
									if (stat.status->entry->conflict_wrk)
									{
										mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
									}
								}
								CopyFile(mine.GetWinPath(), data->path.GetWinPath(), FALSE);
								SVN svn;
								if (!svn.Resolve(data->path, FALSE))
								{
									ReportSVNError();
									DialogEnableWindow(IDOK, TRUE);
									break;
								}
								else
								{
									data->color = ::GetSysColor(COLOR_WINDOWTEXT);
									m_ProgList.Invalidate();
									CString msg;
									msg.Format(IDS_SVNPROGRESS_RESOLVED, data->path.GetWinPath());
									CMessageBox::Show(m_hWnd, msg, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
								}
							}
							break;
						case ID_CONFLICTRESOLVE:
							{
								SVN svn;
								if (!svn.Resolve(data->path, FALSE))
								{
									ReportSVNError();
									DialogEnableWindow(IDOK, TRUE);
									break;
								}
								else
								{
									data->color = ::GetSysColor(COLOR_WINDOWTEXT);
									m_ProgList.Invalidate();
									CString msg;
									msg.Format(IDS_SVNPROGRESS_RESOLVED, data->path.GetWinPath());
									CMessageBox::Show(m_hWnd, msg, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
								}
							}
							break;
						case ID_LOG:
							{
								CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
								int limit = (int)(DWORD)reg;
								svn_revnum_t rev = m_RevisionEnd;
								if (!data->basepath.IsEmpty())
								{
									StringRevMap::iterator it = m_FinishedRevMap.find(data->basepath.GetSVNApiPath());
									if (it != m_FinishedRevMap.end())
										rev = it->second;
								}
								CLogDlg dlg;
								// fetch the log from HEAD, not the revision we updated to:
								// the path might be inside an external folder which has its own
								// revisions.
								CString sWinPath = data->path.GetWinPathString();
								if (sWinPath.Find(':')<0)
								{
									// the path is not absolute: add the current directory in front of it
									DWORD len = GetCurrentDirectory(0, NULL);
									TCHAR * buf = new TCHAR[len+1];
									GetCurrentDirectory(len, buf);
									sWinPath = buf;
									sWinPath += _T("\\") + data->path.GetWinPathString();
									delete [] buf;
								}
								dlg.SetParams(CTSVNPath(sWinPath), SVNRev(), SVNRev::REV_HEAD, 1, limit, TRUE);
								dlg.DoModal();
							}
							break;
						case ID_OPENWITH:
							bOpenWith = true;
						case ID_OPEN:
							{
								int ret = 0;
								CString sWinPath = data->path.GetWinPathString();
								if (sWinPath.Find(':')<0)
								{
									// the path is not absolute: add the current directory in front of it
									DWORD len = GetCurrentDirectory(0, NULL);
									TCHAR * buf = new TCHAR[len+1];
									GetCurrentDirectory(len, buf);
									sWinPath = buf;
									sWinPath += _T("\\") + data->path.GetWinPathString();
									delete [] buf;
								}
								if (!bOpenWith)
									ret = (int)ShellExecute(this->m_hWnd, NULL, (LPCTSTR)sWinPath, NULL, NULL, SW_SHOWNORMAL);
								if ((ret <= HINSTANCE_ERROR)||bOpenWith)
								{
									CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
									cmd += sWinPath;
									CAppUtils::LaunchApplication(cmd, NULL, false);
								}
							}
						}
						DialogEnableWindow(IDOK, TRUE);
						theApp.DoWaitCursor(-1);
					} // if (bAdded)
				} // if (m_ProgList.GetSelectedCount() == 1)
			}
		}
	}
}

void CSVNProgressDlg::OnEnSetfocusInfotext()
{
	CString sTemp;
	GetDlgItem(IDC_INFOTEXT)->GetWindowText(sTemp);
	if (sTemp.IsEmpty())
		GetDlgItem(IDC_INFOTEXT)->HideCaret();
}
