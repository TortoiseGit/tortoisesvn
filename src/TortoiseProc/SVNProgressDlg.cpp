// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015 - TortoiseSVN

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
#include "SVNProgressDlg.h"
#include "LogDialog/LogDlg.h"
#include "TSVNPath.h"
#include "registry.h"
#include "SVNStatus.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "SVNDiff.h"
#include "SVNLogHelper.h"
#include "SVNHelpers.h"
#include "RegHistory.h"
#include "ConflictResolveDlg.h"
#include "EditPropConflictDlg.h"
#include "TreeConflictEditorDlg.h"
#include "LogFile.h"
#include "ShellUpdater.h"
#include "IconMenu.h"
#include "BugTraqAssociations.h"
#include "DragDropImpl.h"
#include "SVNDataObject.h"
#include "SVNProperties.h"
#include "COMError.h"
#include "CmdLineParser.h"
#include "BstrSafeVector.h"
#include "../../TSVNCache/CacheInterface.h"
#include "CachedLogInfo.h"
#include "SmartHandle.h"
#include "RecycleBinDlg.h"
#include "BrowseFolder.h"
#include <strsafe.h>

BOOL    CSVNProgressDlg::m_bAscending = FALSE;
int     CSVNProgressDlg::m_nSortedColumn = -1;

static UINT WM_RESOLVEMSG = RegisterWindowMessage(L"TORTOISESVN_RESOLVEDONE_MSG");

#define TRANSFERTIMER   100
#define VISIBLETIMER    101

#define REG_KEY_ALLOW_UNV_OBSTRUCTIONS L"Software\\TortoiseSVN\\AllowUnversionedObstruction"

enum SVNProgressDlgContextMenuCommands
{
    // needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
    ID_COMPARE = 1,
    ID_EDITCONFLICT,
    ID_CONFLICTRESOLVE,
    ID_CONFLICTUSETHEIRS,
    ID_CONFLICTUSEMINE,
    ID_LOG,
    ID_OPEN,
    ID_OPENWITH,
    ID_EXPLORE,
    ID_COPY
};

IMPLEMENT_DYNAMIC(CSVNProgressDlg, CResizableStandAloneDialog)
CSVNProgressDlg::CSVNProgressDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CSVNProgressDlg::IDD, pParent)
    , m_Revision(L"HEAD")
    , m_RevisionEnd(0)
    , m_bLockWarning(false)
    , m_bLockExists(false)
    , m_bCancelled(FALSE)
    , m_bThreadRunning(FALSE)
    , m_nConflicts(0)
    , m_nTotalConflicts(0)
    , m_bConflictWarningShown(false)
    , m_bWarningShown(false)
    , m_bErrorsOccurred(FALSE)
    , m_bMergesAddsDeletesOccurred(FALSE)
    , m_pThread(NULL)
    , m_options(ProgOptNone)
    , m_dwCloseOnEnd((DWORD)-1)
    , m_bCloseLocalOnEnd((DWORD)-1)
    , m_hidden(false)
    , m_bFinishedItemAdded(false)
    , m_bLastVisible(false)
    , m_depth(svn_depth_unknown)
    , m_itemCount(-1)
    , m_itemCountTotal(-1)
    , m_AlwaysConflicted(false)
    , m_BugTraqProvider(NULL)
    , m_bHookError(false)
    , m_bNoHooks(false)
    , m_bRetryDone(false)
    , m_bExtDataAdded(false)
    , m_bHideExternalInfo(true)
    , m_bExternalStartInfoShown(false)
    , m_Command(SVNProgress_none)
    , m_keepchangelist(false)
    , m_bHooksAreOptional(false)
    , iFirstResized(0)
    , bSecondResized(false)
    , nEnsureVisibleCount(0)
    , m_boldFont(NULL)
    , sIgnoredIncluded(MAKEINTRESOURCE(IDS_PROGRS_IGNOREDINCLUDED))
    , sExtExcluded(MAKEINTRESOURCE(IDS_PROGRS_EXTERNALSEXCLUDED))
    , sExtIncluded(MAKEINTRESOURCE(IDS_PROGRS_EXTERNALSINCLUDED))
    , sIgnoreAncestry(MAKEINTRESOURCE(IDS_PROGRS_IGNOREANCESTRY))
    , sRespectAncestry(MAKEINTRESOURCE(IDS_PROGRS_RESPECTANCESTRY))
    , sDryRun(MAKEINTRESOURCE(IDS_PROGRS_DRYRUN))
    , sRecordOnly(MAKEINTRESOURCE(IDS_MERGE_RECORDONLY))
    , sForce(MAKEINTRESOURCE(IDS_MERGE_FORCE))
{
    m_bHideExternalInfo = !!CRegStdDWORD(L"Software\\TortoiseSVN\\HideExternalInfo", TRUE);
    m_columnbuf[0] = 0;

}

CSVNProgressDlg::~CSVNProgressDlg()
{
    for (auto data : m_arData)
    {
        delete data;
    }
    delete m_pThread;
    if (m_boldFont)
        DeleteObject(m_boldFont);
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
    ON_REGISTERED_MESSAGE(TORTOISESVN_CLOSEONEND_MSG, &CSVNProgressDlg::OnCloseOnEnd)
    ON_WM_TIMER()
    ON_NOTIFY(LVN_BEGINDRAG, IDC_SVNPROGRESS, &CSVNProgressDlg::OnLvnBegindragSvnprogress)
    ON_WM_SIZE()
    ON_NOTIFY(LVN_GETDISPINFO, IDC_SVNPROGRESS, &CSVNProgressDlg::OnLvnGetdispinfoSvnprogress)
    ON_BN_CLICKED(IDC_NONINTERACTIVE, &CSVNProgressDlg::OnBnClickedNoninteractive)
    ON_MESSAGE(WM_SHOWCONFLICTRESOLVER, OnShowConflictResolver)
    ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
    ON_BN_CLICKED(IDC_RETRYNOHOOKS, &CSVNProgressDlg::OnBnClickedRetrynohooks)
    ON_REGISTERED_MESSAGE(CLinkControl::LK_LINKITEMCLICKED, &CSVNProgressDlg::OnCheck)
    ON_REGISTERED_MESSAGE(WM_RESOLVEMSG, &CSVNProgressDlg::OnResolveMsg)
END_MESSAGE_MAP()

BOOL CSVNProgressDlg::Cancel()
{
    return m_bCancelled;
}

LRESULT CSVNProgressDlg::OnShowConflictResolver(WPARAM /*wParam*/, LPARAM lParam)
{
    const svn_wc_conflict_description2_t *description = (svn_wc_conflict_description2_t *)lParam;
    if (description)
    {
        svn_wc_conflict_choice_t retVal = svn_wc_conflict_choose_postpone;
        if (m_pTaskbarList)
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
        }
        switch (description->kind)
        {
        case svn_wc_conflict_kind_property:
        case svn_wc_conflict_kind_text:
            {
                CConflictResolveDlg dlg(this);
                dlg.SetConflictDescription(description);
                if (dlg.DoModal() == IDOK)
                {
                    if (dlg.GetResult() == svn_wc_conflict_choose_postpone)
                    {
                        // if the result is conflicted and the dialog returned IDOK,
                        // that means we should not ask again in case of a conflict
                        m_AlwaysConflicted = true;
                    }
                }
                m_mergedfile = dlg.GetMergedFile();
                m_bCancelled = dlg.IsCancelled();
                retVal = dlg.GetResult();
            }
            break;
        case svn_wc_conflict_kind_tree:
            {
                CTSVNPath treeConflictPath;
                treeConflictPath.SetFromSVN(description->merged_file);

                CTreeConflictEditorDlg dlg;
                dlg.SetInteractive();
                dlg.SetPath(treeConflictPath);
                dlg.SetConflictSources(description->src_left_version, description->src_right_version);
                dlg.SetConflictReason(description->reason);
                dlg.SetConflictAction(description->action);
                dlg.SetConflictOperation(description->operation);
                dlg.SetKind(description->node_kind);
                if (dlg.DoModal() == IDOK)
                {
                    if (dlg.GetResult() == svn_wc_conflict_choose_postpone)
                    {
                        // if the result is conflicted and the dialog returned IDOK,
                        // that means we should not ask again in case of a conflict
                        m_AlwaysConflicted = true;
                    }
                }
                retVal = dlg.GetResult();
                m_bCancelled = dlg.IsCancelled();
            }
        default:
            break;
        }
        if (m_pTaskbarList)
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);

        return retVal;
    }
    return svn_wc_conflict_choose_postpone;
}

svn_wc_conflict_choice_t CSVNProgressDlg::ConflictResolveCallback(const svn_wc_conflict_description2_t *description, CString& mergedfile)
{
    // we only bother the user when merging
    if (((m_Command == SVNProgress_Merge)||(m_Command == SVNProgress_MergeAll)||(m_Command == SVNProgress_MergeReintegrateOldStyle)||(m_Command == SVNProgress_MergeReintegrate))&&(!m_AlwaysConflicted)&&(description))
    {
        // we're in a worker thread here. That means we must not show a dialog from the thread
        // but let the UI thread do it.
        // To do that, we send a message to the UI thread and let it show the conflict resolver dialog.
        LRESULT dlgResult = ::SendMessage(GetSafeHwnd(), WM_SHOWCONFLICTRESOLVER, 0, (LPARAM)description);
        mergedfile = m_mergedfile;
        return (svn_wc_conflict_choice_t)dlgResult;
    }

    return svn_wc_conflict_choose_postpone;
}

void CSVNProgressDlg::AddItemToList(NotificationData * data)
{
    data->id = (long)m_arData.size();
    m_arData.push_back(data);
    int totalcount = m_ProgList.GetItemCount();

    m_ProgList.SetItemCountEx(totalcount+1, LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL);
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
    if (totalcount <= (m_ProgList.GetTopIndex() + count + nEnsureVisibleCount + 2))
    {
        nEnsureVisibleCount++;
        m_bLastVisible = true;
    }
    else
    {
        nEnsureVisibleCount = 0;
        if (IsIconic() == 0)
            m_bLastVisible = false;
    }
}

void CSVNProgressDlg::RemoveItemFromList( size_t index )
{
    if (index >= m_arData.size())
        return;

    // adjust the id's of all items following the one to remove
    for (auto it = m_arData.begin() + index; it != m_arData.end(); ++it)
    {
        (*it)->id--;
    }

    m_arData.erase(m_arData.begin() + index);

    int totalcount = m_ProgList.GetItemCount();

    m_ProgList.SetItemCountEx(totalcount-1, LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL);
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
    if (totalcount <= (m_ProgList.GetTopIndex() + count + nEnsureVisibleCount + 2))
    {
        nEnsureVisibleCount++;
        m_bLastVisible = true;
    }
    else
    {
        nEnsureVisibleCount = 0;
        if (IsIconic() == 0)
            m_bLastVisible = false;
    }
}

BOOL CSVNProgressDlg::Notify(const CTSVNPath& path, const CTSVNPath& url, svn_wc_notify_action_t action,
                             svn_node_kind_t kind, const CString& mime_type,
                             svn_wc_notify_state_t content_state,
                             svn_wc_notify_state_t prop_state, LONG rev,
                             const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
                             const CString& changelistname,
                             const CString& propertyName,
                             svn_merge_range_t * range,
                             svn_error_t * err, apr_pool_t * pool)
{
    static bool bInInteractiveResolving = false;
    static bool sent_first_txdelta = false;

    bool bNoNotify = false;
    bool bDoAddData = true;
    NotificationData * data = new NotificationData();
    data->path = path;
    data->url = url;
    data->action = action;
    data->kind = kind;
    data->mime_type = mime_type;
    data->content_state = content_state;
    data->prop_state = prop_state;
    data->rev = rev;
    data->lock_state = lock_state;
    data->changelistname = changelistname;
    data->propertyName = propertyName;
    data->indent = (int)m_ExtStack.GetCount();
    if ((lock)&&(lock->owner))
        data->owner = CUnicodeUtils::GetUnicode(lock->owner);
    data->sPathColumnText = path.GetUIPathString();
    if (!m_basePath.IsEmpty())
        data->basepath = m_basePath;
    if (range)
        data->merge_range = *range;
    switch (data->action)
    {
    case svn_wc_notify_update_shadowed_add:
    case svn_wc_notify_add:
    case svn_wc_notify_update_add:
        if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
        {
            data->color = m_Colors.GetColor(((m_options & ProgOptDryRun)!=0) ? CColors::DryRunConflict : CColors::Conflict);
            data->bConflictedActionItem = true;
            data->sActionColumnText.LoadString(((m_options & ProgOptDryRun)!=0) ? IDS_SVNACTION_DRYRUN_CONFLICTED : IDS_SVNACTION_CONFLICTED);
            m_nConflicts++;
            m_nTotalConflicts++;
            m_bConflictWarningShown = false;
        }
        else
        {
            m_bMergesAddsDeletesOccurred = true;
            data->sActionColumnText.LoadString(IDS_SVNACTION_ADD);
            data->color = m_Colors.GetColor(CColors::Added);
        }
        break;
    case svn_wc_notify_commit_added:
    case svn_wc_notify_commit_copied:
        {
            data->sActionColumnText.LoadString(IDS_SVNACTION_ADDING);
            data->color = m_Colors.GetColor(CColors::Added);
            if ((data->action == svn_wc_notify_commit_copied)&&
                (m_Command == SVNProgress_Commit)&&
                (!m_bWarningShown)&&
                (m_depth < svn_depth_infinity)&&
                (kind == svn_node_dir))
            {
                AddItemToList(data);

                data = new NotificationData();
                data->bAuxItem = true;
                data->sActionColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED_WARNING);
                data->sPathColumnText.Format(IDS_PROGRS_COPYDEPTH_WARNING, SVNStatus::GetDepthString(m_depth));
                data->color = m_Colors.GetColor(((m_options & ProgOptDryRun)!=0) ? CColors::DryRunConflict : CColors::Conflict);
                PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ALIAS_ID | SND_ASYNC);
                m_bWarningShown = true;
            }
        }
        break;
    case svn_wc_notify_copy:
        data->sActionColumnText.LoadString(IDS_SVNACTION_COPY);
        break;
    case svn_wc_notify_commit_modified:
        data->sActionColumnText.LoadString(IDS_SVNACTION_MODIFIED);
        data->color = m_Colors.GetColor(CColors::Modified);
        break;
    case svn_wc_notify_update_shadowed_delete:
    case svn_wc_notify_delete:
    case svn_wc_notify_update_delete:
    case svn_wc_notify_exclude:
        data->sActionColumnText.LoadString(IDS_SVNACTION_DELETE);
        m_bMergesAddsDeletesOccurred = true;
        data->color = m_Colors.GetColor(CColors::Deleted);
        break;
    case svn_wc_notify_commit_deleted:
    case svn_wc_notify_update_external_removed:
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
        if (bInInteractiveResolving)
        {
            // the now resolved item might be listed above already as
            // conflicted (resolved due to interactive conflict resolve dialog).
            // Try to find the corresponding entry and change it so it does
            // not appear as conflicted anymore.
            size_t index = 0;
            size_t removeit = (size_t)-1;
            for (const auto& it : m_arData)
            {
                if ((it->bTreeConflict||it->bConflictedActionItem) && (it->path.IsEquivalentToWithoutCase(data->path)))
                {
                    it->bConflictedActionItem = false;
                    it->bTreeConflict = false;
                    m_nTotalConflicts--;
                    if (m_nConflicts)
                    {
                        m_nConflicts--;
                    }
                    else
                    {
                        // there's already a line shown that says:
                        // Warning!: One or more files are in a conflicted state.
                        // search downwards for this message while counting the conflicts:
                        // if there are no more conflicts, remove that message
                        int nConflicts = 0;
                        size_t rind = index;
                        for (auto down = m_arData.cbegin() + index; down != m_arData.cend(); ++down)
                        {
                            if ((*down)->bConflictedActionItem)
                                ++nConflicts;
                            if ((*down)->bConflictSummary)
                            {
                                if (nConflicts == 0)
                                    removeit = rind;
                                break;
                            }
                            if ((*down)->bAuxItem)
                                break;
                            ++rind;
                        }
                    }
                    it->sActionColumnText = it->sActionColumnText + L" (" + data->sActionColumnText + L")";
                    it->color = m_Colors.GetColor(CColors::Merged);
                    m_ProgList.RedrawItems((int)index-1, (int)index);
                }
                ++index;
            }
            if (removeit != (size_t)-1)
            {
                RemoveItemFromList(removeit);
            }
        }
        break;
    case svn_wc_notify_update_replace:
    case svn_wc_notify_commit_copied_replaced:
        data->sActionColumnText.LoadString(IDS_SVNACTION_REPLACED);
        data->color = m_Colors.GetColor(CColors::Deleted);
        if ((data->action == svn_wc_notify_commit_copied_replaced)&&
            (m_Command == SVNProgress_Commit)&&
            (!m_bWarningShown)&&
            (m_depth < svn_depth_infinity)&&
            (kind == svn_node_dir))
        {
            AddItemToList(data);

            data = new NotificationData();
            data->bAuxItem = true;
            data->sActionColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED_WARNING);
            data->sPathColumnText.Format(IDS_PROGRS_COPYDEPTH_WARNING, SVNStatus::GetDepthString(m_depth));
            data->color = m_Colors.GetColor(((m_options & ProgOptDryRun)!=0) ? CColors::DryRunConflict : CColors::Conflict);
            PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ALIAS_ID | SND_ASYNC);

            m_bWarningShown = true;
        }
        break;
    case svn_wc_notify_commit_replaced:
        data->sActionColumnText.LoadString(IDS_SVNACTION_REPLACED);
        data->color = m_Colors.GetColor(CColors::Deleted);
        break;
    case svn_wc_notify_exists:
        if ((data->content_state == svn_wc_notify_state_conflicted) || (data->prop_state == svn_wc_notify_state_conflicted))
        {
            data->color = m_Colors.GetColor(((m_options & ProgOptDryRun)!=0) ? CColors::DryRunConflict : CColors::Conflict);
            data->bConflictedActionItem = true;
            m_nConflicts++;
            m_nTotalConflicts++;
            m_bConflictWarningShown = false;
            data->sActionColumnText.LoadString(((m_options & ProgOptDryRun)!=0) ? IDS_SVNACTION_DRYRUN_CONFLICTED : IDS_SVNACTION_CONFLICTED);
        }
        else if ((data->content_state == svn_wc_notify_state_merged) || (data->prop_state == svn_wc_notify_state_merged))
        {
            data->color = m_Colors.GetColor(CColors::Merged);
            m_bMergesAddsDeletesOccurred = true;
            data->sActionColumnText.LoadString(IDS_SVNACTION_MERGED);
        }
        else
            data->sActionColumnText.LoadString(IDS_SVNACTION_EXISTS);
        break;
    case svn_wc_notify_update_started:
        if (m_ExtStack.GetCount())
        {
            // since we already wrote the "External - path" notification,
            // showing an "Update - path" for the very same path is just noise.
            bNoNotify = true;
            break;
        }
        data->sActionColumnText.LoadString(IDS_SVNACTION_UPDATING);
        m_basePath = path;
        m_bConflictWarningShown = false;
        m_bExternalStartInfoShown = false;
        break;
    case svn_wc_notify_update_shadowed_update:
    case svn_wc_notify_merge_record_info:
    case svn_wc_notify_update_update:
        // if this is an inoperative dir change, don't show the notification.
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
            data->color = m_Colors.GetColor(((m_options & ProgOptDryRun)!=0) ? CColors::DryRunConflict : CColors::Conflict);
            data->bConflictedActionItem = true;
            m_nConflicts++;
            m_nTotalConflicts++;
            m_bConflictWarningShown = false;
            data->sActionColumnText.LoadString(((m_options & ProgOptDryRun)!=0) ? IDS_SVNACTION_DRYRUN_CONFLICTED : IDS_SVNACTION_CONFLICTED);
        }
        else if ((data->content_state == svn_wc_notify_state_merged) || (data->prop_state == svn_wc_notify_state_merged))
        {
            data->color = m_Colors.GetColor(CColors::Merged);
            m_bMergesAddsDeletesOccurred = true;
            data->sActionColumnText.LoadString(IDS_SVNACTION_MERGED);
        }
        else if ((data->content_state == svn_wc_notify_state_changed) ||
            (data->prop_state == svn_wc_notify_state_changed))
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
            data->sActionColumnText += L", " + temp;
        }
        break;

    case svn_wc_notify_update_external:
        {
            m_ExtStack.AddHead(path.GetUIPathString());
            m_bExtDataAdded = false;
            m_basePath = path;
            SVNStatus status;
            CTSVNPath dummypath;
            svn_client_status_t * s = status.GetFirstFileStatus(m_basePath, dummypath, false, svn_depth_empty);
            if (s)
                m_UpdateStartRevMap[m_basePath.GetSVNApiPath(pool)] = s->changed_rev;
            data->sActionColumnText.LoadString(IDS_SVNACTION_EXTERNAL);
            data->bAuxItem = true;
            if (m_bHideExternalInfo)
            {
                if (!m_bExternalStartInfoShown)
                {
                    //m_Colors.GetColor(CColors::Cmd)
                    data->bAuxItem = true;
                    data->sPathColumnText.LoadString(IDS_PROGRS_STARTING_EXTERNALS);
                    data->color = m_Colors.GetColor(CColors::Cmd);
                    m_bExternalStartInfoShown = true;
                }
                else
                    bNoNotify = true;
            }
        }
        break;

    case svn_wc_notify_merge_completed:
        {
            if ((m_nConflicts>0)&&(!m_bConflictWarningShown))
            {
                data->sActionColumnText.LoadString(IDS_SVNACTION_COMPLETED);
                data->bAuxItem = true;
                // We're going to add another aux item - let's shove this current onto the list first
                // I don't really like this, but it will do for the moment.
                AddItemToList(data);

                data = new NotificationData();
                data->bAuxItem = true;
                data->sActionColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED_WARNING);
                data->sPathColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED);
                data->color = m_Colors.GetColor(((m_options & ProgOptDryRun)!=0) ? CColors::DryRunConflict : CColors::Conflict);
                data->bConflictSummary = true;
                PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ALIAS_ID | SND_ASYNC);
                m_bConflictWarningShown = true;
                m_nConflicts = 0;
                // This item will now be added after the switch statement
                m_bFinishedItemAdded = true;
            }
        }
        break;
    case svn_wc_notify_update_completed:
        {
            data->sActionColumnText.LoadString(IDS_SVNACTION_COMPLETED);
            data->bAuxItem = true;
            data->bBold = true;
            bool bEmpty = !!m_ExtStack.IsEmpty();
            if (!bEmpty)
            {
                if (m_bHideExternalInfo)
                {
                    bNoNotify = true;
                    m_ExtStack.RemoveHead();
                    break;
                }
                CString sExtPath = m_ExtStack.RemoveHead();
                data->sPathColumnText.FormatMessage(IDS_PROGRS_PATHATREV, (LPCTSTR)sExtPath, rev);
                if (!m_arData.empty() && !m_bExtDataAdded)
                {
                    NotificationData * pOldData = m_arData[m_arData.size()-1];
                    if (pOldData && (pOldData->sPathColumnText==sExtPath))
                    {
                        // just update the "External" entry instead of adding another one
                        pOldData->sPathColumnText = data->sPathColumnText;
                        bNoNotify = true;
                        m_ProgList.Update((int)m_arData.size()-1);
                        break;
                    }
                }
                if (!m_bExtDataAdded)
                {
                    bNoNotify = true;
                    break;
                }
            }
            else
                data->sPathColumnText.Format(IDS_PROGRS_ATREV, rev);
            m_bExtDataAdded = false;
            if ((m_nConflicts>0)&&(bEmpty)&&(!m_bConflictWarningShown))
            {
                // We're going to add another aux item - let's shove this current onto the list first
                // I don't really like this, but it will do for the moment.
                AddItemToList(data);

                data = new NotificationData();
                data->bAuxItem = true;
                data->sActionColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED_WARNING);
                data->sPathColumnText.LoadString(IDS_PROGRS_CONFLICTSOCCURED);
                data->color = m_Colors.GetColor(((m_options & ProgOptDryRun)!=0) ? CColors::DryRunConflict : CColors::Conflict);
                data->bConflictSummary = true;
                PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ALIAS_ID | SND_ASYNC);
                m_bConflictWarningShown = true;
                m_nConflicts = 0;
                // This item will now be added after the switch statement
            }
            if (!m_basePath.IsEmpty())
                m_FinishedRevMap[m_basePath.GetSVNApiPath(pool)] = rev;
            m_RevisionEnd = rev;
            m_bFinishedItemAdded = true;
        }
        break;
    case svn_wc_notify_commit_postfix_txdelta:
        if (!sent_first_txdelta)
        {
            sent_first_txdelta = true;
            data->sActionColumnText.LoadString(IDS_SVNACTION_POSTFIX);
        }
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
            data->color = m_Colors.GetColor(CColors::Conflict);
        }
        else
        {
            data->sActionColumnText.LoadString(IDS_SVNACTION_SKIP);
            if ((content_state == svn_wc_notify_state_obstructed)||(content_state == svn_wc_notify_state_conflicted))
                data->color = m_Colors.GetColor(CColors::Conflict);
        }
        break;
    case svn_wc_notify_update_skip_working_only:
        data->sActionColumnText.LoadString(IDS_SVNACTION_SKIPNOPARENT);
        data->color = m_Colors.GetColor(CColors::Conflict);
        data->bConflictedActionItem = true;
        m_nConflicts++;
        m_nTotalConflicts++;
        m_bConflictWarningShown = false;
        break;
    case svn_wc_notify_locked:
        if ((lock)&&(lock->owner))
            data->sActionColumnText.Format(IDS_SVNACTION_LOCKEDBY, (LPCTSTR)data->owner);
        break;
    case svn_wc_notify_unlocked:
        data->sActionColumnText.LoadString(IDS_SVNACTION_UNLOCKED);
        break;
    case svn_wc_notify_failed_lock:
        data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDLOCK);
        AddItemToList(data);
        ReportError(SVN::GetErrorString(err));
        bDoAddData = false;
        if ((err)&&(err->apr_err == SVN_ERR_FS_OUT_OF_DATE))
            m_bLockWarning = true;
        if ((err)&&(err->apr_err == SVN_ERR_FS_PATH_ALREADY_LOCKED))
            m_bLockExists = true;
        break;
    case svn_wc_notify_failed_unlock:
        data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDUNLOCK);
        AddItemToList(data);
        ReportError(SVN::GetErrorString(err));
        bDoAddData = false;
        if ((err)&&(err->apr_err == SVN_ERR_FS_OUT_OF_DATE))
            m_bLockWarning = true;
        break;
    case svn_wc_notify_changelist_set:
        data->sActionColumnText.Format(IDS_SVNACTION_CHANGELISTSET, (LPCTSTR)data->changelistname);
        break;
    case svn_wc_notify_changelist_clear:
        data->sActionColumnText.LoadString(IDS_SVNACTION_CHANGELISTCLEAR);
        break;
    case svn_wc_notify_changelist_moved:
        data->sActionColumnText.Format(IDS_SVNACTION_CHANGELISTMOVED, (LPCTSTR)data->changelistname);
        break;
    case svn_wc_notify_foreign_merge_begin:
    case svn_wc_notify_merge_begin:
        if (range == NULL)
            data->sActionColumnText.LoadString(IDS_SVNACTION_MERGEBEGINNONE);
        else if ((data->merge_range.start == data->merge_range.end) || (data->merge_range.start == data->merge_range.end - 1))
        {
            data->sActionColumnText.Format(IDS_SVNACTION_MERGEBEGINSINGLE, data->merge_range.end);
            m_mergedRevisions.AddRevision(data->merge_range.end, false);
        }
        else if (data->merge_range.start - 1 == data->merge_range.end)
        {
            data->sActionColumnText.Format(IDS_SVNACTION_MERGEBEGINSINGLEREVERSE, data->merge_range.start);
            m_mergedRevisions.AddRevision(data->merge_range.start, false); // we want the revisions in ascending order for the generated log message
        }
        else if (data->merge_range.start < data->merge_range.end)
        {
            data->sActionColumnText.FormatMessage(IDS_SVNACTION_MERGEBEGINMULTIPLE, data->merge_range.start + 1, data->merge_range.end);
            m_mergedRevisions.AddRevRange(data->merge_range.start + 1, data->merge_range.end);
        }
        else
        {
            data->sActionColumnText.FormatMessage(IDS_SVNACTION_MERGEBEGINMULTIPLEREVERSE, data->merge_range.start, data->merge_range.end + 1);
            m_mergedRevisions.AddRevRange(data->merge_range.start, data->merge_range.end + 1);
        }
        data->bAuxItem = true;
        break;
    case svn_wc_notify_property_added:
    case svn_wc_notify_property_modified:
        data->sActionColumnText.Format(IDS_SVNACTION_PROPSET, (LPCTSTR)data->propertyName);
        break;
    case svn_wc_notify_property_deleted:
    case svn_wc_notify_property_deleted_nonexistent:
        data->sActionColumnText.Format(IDS_SVNACTION_PROPDEL, (LPCTSTR)data->propertyName);
        break;
    case svn_wc_notify_revprop_set:
        data->sActionColumnText.Format(IDS_SVNACTION_PROPSET, (LPCTSTR)data->propertyName);
        break;
    case svn_wc_notify_revprop_deleted:
        data->sActionColumnText.Format(IDS_SVNACTION_PROPDEL, (LPCTSTR)data->propertyName);
        break;
    case svn_wc_notify_update_skip_obstruction:
        data->sActionColumnText.LoadString(IDS_SVNACTION_OBSTRUCTED);
        data->color = m_Colors.GetColor(CColors::Conflict);
        data->bConflictedActionItem = true;
        m_nConflicts++;
        m_nTotalConflicts++;
        m_bConflictWarningShown = false;
        break;
    case svn_wc_notify_tree_conflict:
        data->sActionColumnText.LoadString(IDS_SVNACTION_TREECONFLICTED);
        data->color = m_Colors.GetColor(((m_options & ProgOptDryRun)!=0) ? CColors::DryRunConflict : CColors::Conflict);
        data->bConflictedActionItem = true;
        data->bTreeConflict = true;
        m_nConflicts++;
        m_nTotalConflicts++;
        m_bConflictWarningShown = false;
        break;
    case svn_wc_notify_failed_external:
        data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDEXTERNAL);
        data->color = m_Colors.GetColor(CColors::Conflict);
        AddItemToList(data);
        bDoAddData = false;
        ReportError(SVN::GetErrorString(err));
        break;
    case svn_wc_notify_merge_record_info_begin:
        if (range == NULL)
            data->sActionColumnText.LoadString(IDS_SVNACTION_MERGEINFO);
        else if ((data->merge_range.start == data->merge_range.end) || (data->merge_range.start == data->merge_range.end - 1))
        {
            data->sActionColumnText.Format(IDS_SVNACTION_MERGERECORDBEGINSINGLE, data->merge_range.end);
            if (m_options & ProgOptRecordOnly)
                m_mergedRevisions.AddRevision(data->merge_range.end, false);
        }
        else if (data->merge_range.start - 1 == data->merge_range.end)
        {
            data->sActionColumnText.Format(IDS_SVNACTION_MERGERECORDBEGINSINGLEREVERSE, data->merge_range.start);
            if (m_options & ProgOptRecordOnly)
                m_mergedRevisions.AddRevision(data->merge_range.start, false); // we want the revisions in ascending order for the generated log message
        }
        else if (data->merge_range.start < data->merge_range.end)
        {
            data->sActionColumnText.FormatMessage(IDS_SVNACTION_MERGERECORDBEGINMULTIPLE, data->merge_range.start + 1, data->merge_range.end);
            if (m_options & ProgOptRecordOnly)
                m_mergedRevisions.AddRevRange(data->merge_range.start + 1, data->merge_range.end);
        }
        else
        {
            data->sActionColumnText.FormatMessage(IDS_SVNACTION_MERGERECORDBEGINMULTIPLEREVERSE, data->merge_range.start, data->merge_range.end + 1);
            if (m_options & ProgOptRecordOnly)
                m_mergedRevisions.AddRevRange(data->merge_range.start, data->merge_range.end + 1);
        }
        data->bAuxItem = true;
        break;
    case svn_wc_notify_foreign_copy_begin:
        data->sActionColumnText.LoadString(IDS_SVNACTION_COPYREMOTE);
        data->sPathColumnText = url.GetUIPathString();
        break;
    case svn_wc_notify_merge_elide_info:
        data->sActionColumnText.LoadString(IDS_SVNACTION_ELIDEMERGEINFO);
        break;
    case svn_wc_notify_url_redirect:
        data->sActionColumnText.LoadString(IDS_SVNACTION_URLREDIRECT);
        data->sPathColumnText = url.GetUIPathString();
        break;
    case svn_wc_notify_path_nonexistent:
        data->sActionColumnText.LoadString(IDS_SVNACTION_PATHNOTEXIST);
        data->color = m_Colors.GetColor(CColors::Conflict);
        data->bConflictedActionItem = true;
        m_nConflicts++;
        m_nTotalConflicts++;
        m_bConflictWarningShown = false;
        break;
    case svn_wc_notify_update_skip_access_denied:
        data->sActionColumnText.LoadString(IDS_SVNACTION_ACCESSDENIED);
        data->color = m_Colors.GetColor(CColors::Conflict);
        data->bConflictedActionItem = true;
        m_nConflicts++;
        m_nTotalConflicts++;
        m_bConflictWarningShown = false;
        break;
    case svn_wc_notify_skip_conflicted:
        data->sActionColumnText.LoadString(IDS_SVNACTION_REMAINSCONFLICTED);
        data->color = m_Colors.GetColor(CColors::Conflict);
        data->bConflictedActionItem = true;
        m_nConflicts++;
        m_nTotalConflicts++;
        m_bConflictWarningShown = false;
        break;
    case svn_wc_notify_update_broken_lock:
        data->sActionColumnText.LoadString(IDS_SVNACTION_BROKENLOCKED);
        break;
    case svn_wc_notify_left_local_modifications:
        data->sActionColumnText.LoadString(IDS_SVNACTION_LEFTLOCALMODS);
        break;
    case svn_wc_notify_move_broken:
        data->sActionColumnText.LoadString(IDS_SVNACTION_MOVEBROKEN);
        break;
    case svn_wc_notify_failed_requires_target:
        data->sActionColumnText.LoadString(IDS_SVNACTION_FAILEDREQUIRESTARGET);
        data->color = m_Colors.GetColor(CColors::Conflict);
        break;
    case svn_wc_notify_info_external:
        data->sActionColumnText.LoadString(IDS_SVNACTION_INFOEXTERNAL);
        break;
    case svn_wc_notify_conflict_resolver_starting:
        bInInteractiveResolving = true;
        bNoNotify = true;
        break;
    case svn_wc_notify_conflict_resolver_done:
        bInInteractiveResolving = false;
        bNoNotify = true;
        break;
    case svn_wc_notify_commit_finalizing:
        data->sActionColumnText.LoadString(IDS_SVNACTION_COMMITTINGTRANSACTION);
        data->sPathColumnText.Empty();
        break;
    case svn_wc_notify_upgraded_path:
    case svn_wc_notify_failed_conflict:
    case svn_wc_notify_failed_missing:
    case svn_wc_notify_failed_out_of_date:
    case svn_wc_notify_failed_no_parent:
    case svn_wc_notify_failed_locked:
    case svn_wc_notify_failed_forbidden_by_server:
    case svn_wc_notify_failed_obstruction:
    case svn_wc_notify_cleanup_external:
        bNoNotify = true;
    default:
        break;
    } // switch (data->action)

    if (bNoNotify)
        delete data;
    else
    {
        if (bDoAddData)
        {
            AddItemToList(data);
            if (m_ExtStack.GetCount())
            {
                if ((action != svn_wc_notify_update_completed) &&
                    (action != svn_wc_notify_update_external))
                    m_bExtDataAdded = true;
            }
            if ((!data->bAuxItem)&&(m_itemCount > 0))
            {
                m_itemCount--;

                CProgressCtrl * progControl = (CProgressCtrl *)GetDlgItem(IDC_PROGRESSBAR);
                progControl->ShowWindow(SW_SHOW);
                progControl->SetPos((int)(m_itemCountTotal - m_itemCount));
                progControl->SetRange32(0, (int)m_itemCountTotal);
                if (m_pTaskbarList)
                {
                    m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
                    m_pTaskbarList->SetProgressValue(m_hWnd, m_itemCountTotal-m_itemCount, m_itemCountTotal);
                }
            }
        }
        if ((action == svn_wc_notify_commit_postfix_txdelta)&&(bSecondResized == FALSE))
        {
            ResizeColumns();
            bSecondResized = TRUE;
        }
    }

    return TRUE;
}

static void BuildInfoSubstring(CString &str, UINT nID, int count)
{
    if (!count)
        return;

    if (!str.IsEmpty())
        str += L", ";
    CString temp;
    temp.LoadString(nID);
    str += temp;
    str.AppendFormat(L":%d", count);
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

    for (const auto& dat : m_arData)
    {
        switch (dat->action)
        {
        case svn_wc_notify_add:
        case svn_wc_notify_update_add:
        case svn_wc_notify_commit_added:
        case svn_wc_notify_commit_copied:
        case svn_wc_notify_update_shadowed_add:
        case svn_wc_notify_tree_conflict:
        case svn_wc_notify_path_nonexistent:
            if (dat->bConflictedActionItem)
                conflicted++;
            else
                added++;
            break;
        case svn_wc_notify_copy:
            copied++;
            break;
        case svn_wc_notify_delete:
        case svn_wc_notify_update_delete:
        case svn_wc_notify_exclude:
        case svn_wc_notify_commit_deleted:
        case svn_wc_notify_update_shadowed_delete:
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
        case svn_wc_notify_update_shadowed_update:
        case svn_wc_notify_merge_record_info:
        case svn_wc_notify_exists:
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
        case svn_wc_notify_update_skip_working_only:
        case svn_wc_notify_update_skip_access_denied:
            skipped++;
            break;
        case svn_wc_notify_commit_replaced:
        case svn_wc_notify_update_replace:
        case svn_wc_notify_commit_copied_replaced:
            replaced++;
            break;
        }
    }
    BuildInfoSubstring(infotext, IDS_SVNACTION_CONFLICTED, conflicted);
    BuildInfoSubstring(infotext, IDS_SVNACTION_SKIP, skipped);
    BuildInfoSubstring(infotext, IDS_SVNACTION_MERGED, merged);
    BuildInfoSubstring(infotext, IDS_SVNACTION_ADD, added);
    BuildInfoSubstring(infotext, IDS_SVNACTION_DELETE, deleted);
    BuildInfoSubstring(infotext, IDS_SVNACTION_MODIFIED, modified);
    BuildInfoSubstring(infotext, IDS_SVNACTION_COPY, copied);
    BuildInfoSubstring(infotext, IDS_SVNACTION_REPLACED, replaced);
    BuildInfoSubstring(infotext, IDS_SVNACTION_UPDATE, updated);
    BuildInfoSubstring(infotext, IDS_SVNACTION_RESTORE, restored);
    BuildInfoSubstring(infotext, IDS_SVNACTION_REVERT, reverted);
    BuildInfoSubstring(infotext, IDS_SVNACTION_RESOLVE, resolved);
    return infotext;
}

void CSVNProgressDlg::SetAutoClose(const CCmdLineParser& parser)
{
    if (parser.HasVal(L"closeonend"))
        SetAutoClose(parser.GetLongVal(L"closeonend"));
    if (parser.HasKey(L"closeforlocal"))
        SetAutoCloseLocal(TRUE);
    if (parser.HasKey(L"hideprogress"))
        SetHidden (true);
}

void CSVNProgressDlg::SetSelectedList(const CTSVNPathList& selPaths)
{
    m_selectedPaths = selPaths;
}

void CSVNProgressDlg::ResizeColumns()
{
    m_ProgList.SetRedraw(FALSE);

    TCHAR textbuf[MAX_PATH] = { 0 };

    CHeaderCtrl * pHeaderCtrl = (CHeaderCtrl*)(m_ProgList.GetDlgItem(0));
    if (pHeaderCtrl)
    {
        int maxcol = pHeaderCtrl->GetItemCount()-1;
        for (int col = 0; col <= maxcol; col++)
        {
            // find the longest width of all items
            int count = m_ProgList.GetItemCount();
            HDITEM hdi = {0};
            hdi.mask = HDI_TEXT;
            hdi.pszText = textbuf;
            hdi.cchTextMax = _countof(textbuf);
            pHeaderCtrl->GetItem(col, &hdi);
            int cx = m_ProgList.GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin

            for (int index = 0; index<count; ++index)
            {
                auto data = m_arData[index];
                HFONT hFont = NULL;
                if (data->bBold)
                {
                    hFont = (HFONT)m_ProgList.SendMessage(WM_GETFONT);
                    // set the bold font and ask for the string width again
                    m_ProgList.SendMessage(WM_SETFONT, (WPARAM)m_boldFont, NULL);
                }

                // get the width of the string and add 14 pixels for the column separator and margins
#define SEPANDMARG 14
                int linewidth = cx;
                switch (col)
                {
                case 0:
                    linewidth = m_ProgList.GetStringWidth(data->sActionColumnText) + SEPANDMARG;
                    break;
                case 1:
                    linewidth = m_ProgList.GetStringWidth(data->sPathColumnText) + SEPANDMARG;
                    break;
                case 2:
                    linewidth = m_ProgList.GetStringWidth(data->mime_type) + SEPANDMARG;
                    break;
                }
                if (data->bBold)
                {
                    // restore the system font
                    m_ProgList.SendMessage(WM_SETFONT, (WPARAM)hFont, NULL);
                }

                if (cx < linewidth)
                    cx = linewidth;
            }
            m_ProgList.SetColumnWidth(col, cx);
        }
    }

    m_ProgList.SetRedraw(TRUE);
}

BOOL CSVNProgressDlg::OnInitDialog()
{
    __super::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_SVNPROGRESS, IDC_SVNPROGRESS, IDC_SVNPROGRESS, IDC_SVNPROGRESS);
    m_aeroControls.SubclassControl(this, IDC_PROGRESSLABEL);
    m_aeroControls.SubclassControl(this, IDC_JUMPCONFLICT);
    m_aeroControls.SubclassControl(this, IDC_RETRYNOHOOKS);
    m_aeroControls.SubclassControl(this, IDC_PROGRESSBAR);
    m_aeroControls.SubclassControl(this, IDC_INFOTEXT);
    m_aeroControls.SubclassControl(this, IDC_NONINTERACTIVE);
    m_aeroControls.SubclassControl(this, IDC_LOGBUTTON);
    m_aeroControls.SubclassOkCancel(this);

    // Let the TaskbarButtonCreated message through the UIPI filter. If we don't
    // do this, Explorer would be unable to send that message to our window if we
    // were running elevated. It's OK to make the call all the time, since if we're
    // not elevated, this is a no-op.
    CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
    typedef BOOL STDAPICALLTYPE ChangeWindowMessageFilterExDFN(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
    CAutoLibrary hUser = ::LoadLibrary(L"user32.dll");
    if (hUser)
    {
        ChangeWindowMessageFilterExDFN *pfnChangeWindowMessageFilterEx = (ChangeWindowMessageFilterExDFN*)GetProcAddress(hUser, "ChangeWindowMessageFilterEx");
        if (pfnChangeWindowMessageFilterEx)
        {
            pfnChangeWindowMessageFilterEx(m_hWnd, WM_TASKBARBTNCREATED, MSGFLT_ALLOW, &cfs);
        }
    }
    m_pTaskbarList.Release();
    if (FAILED(m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
        m_pTaskbarList = nullptr;

    m_ProgList.SetExtendedStyle (LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    SetWindowTheme(m_ProgList.GetSafeHwnd(), L"Explorer", NULL);

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

    // use the default GUI font, create a copy of it and
    // change the copy to BOLD (leave the rest of the font
    // the same)
    HFONT hFont = (HFONT)m_ProgList.SendMessage(WM_GETFONT);
    LOGFONT lf = {0};
    GetObject(hFont, sizeof(LOGFONT), &lf);
    lf.lfWeight = FW_BOLD;
    m_boldFont = CreateFontIndirect(&lf);

    UpdateData(FALSE);

    // Call this early so that the column headings aren't hidden before any
    // text gets added.
    ResizeColumns();

    SetTimer(VISIBLETIMER, 300, NULL);

    AdjustControlSize(IDC_JUMPCONFLICT);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_JUMPCONFLICT)->GetSafeHwnd(), PROPID_ACC_STATE, STATE_SYSTEM_READONLY|STATE_SYSTEM_UNAVAILABLE);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_JUMPCONFLICT);

    AddAnchor(IDC_SVNPROGRESS, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_PROGRESSLABEL, BOTTOM_LEFT, BOTTOM_CENTER);
    AddAnchor(IDC_JUMPCONFLICT, BOTTOM_CENTER, BOTTOM_RIGHT);
    AddAnchor(IDC_PROGRESSBAR, BOTTOM_CENTER, BOTTOM_RIGHT);
    AddAnchor(IDC_RETRYNOHOOKS, BOTTOM_RIGHT);
    AddAnchor(IDC_INFOTEXT, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_NONINTERACTIVE, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDC_LOGBUTTON, BOTTOM_RIGHT);
    SetPromptParentWindow(this->m_hWnd);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"SVNProgressDlg");
    GetDlgItem(IDOK)->SetFocus();

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


    return FALSE;
}

bool CSVNProgressDlg::SetBackgroundImage(UINT nID)
{
    return CAppUtils::SetListCtrlBackgroundImage(m_ProgList.GetSafeHwnd(), nID);
}

void CSVNProgressDlg::ReportSVNError()
{
    ReportError(GetLastErrorMessage());
}

void CSVNProgressDlg::ReportError(const CString& sError)
{
    PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, NULL, SND_ALIAS_ID | SND_ASYNC);
    ReportString(sError, CString(MAKEINTRESOURCE(IDS_ERR_ERROR)), m_Colors.GetColor(CColors::Conflict));
    m_bErrorsOccurred = true;
}

void CSVNProgressDlg::ReportHookFailed(hooktype t, const CTSVNPathList& pathList, const CString& error)
{
    CString temp;
    temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
    ReportError(temp);
    m_bHookError = true;
    m_bHooksAreOptional = !CHooks::Instance().IsHookExecutionEnforced(t, pathList);
}

void CSVNProgressDlg::ReportWarning(const CString& sWarning)
{
    PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ALIAS_ID | SND_ASYNC);
    ReportString(sWarning, CString(MAKEINTRESOURCE(IDS_WARN_WARNING)), m_Colors.GetColor(CColors::Conflict));
}

void CSVNProgressDlg::ReportNotification(const CString& sNotification)
{
    PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ALIAS_ID | SND_ASYNC);
    ReportString(sNotification, CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
}

void CSVNProgressDlg::ReportCmd(const CString& sCmd)
{
    ReportString(sCmd, CString(MAKEINTRESOURCE(IDS_PROGRS_CMDINFO)), m_Colors.GetColor(CColors::Cmd));
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
        data->sPathColumnText.Trim(L"\n\r");
        data->color = color;
        if (sMessage.Find('\n')>=0)
        {
            sMessage = sMessage.Mid(sMessage.Find('\n')+1);
        }
        else
            sMessage.Empty();
        AddItemToList(data);
    }
}


UINT CSVNProgressDlg::ProgressThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CSVNProgressDlg*)pVoid)->ProgressThread();
}

UINT CSVNProgressDlg::ProgressThread()
{
    if (m_hidden)
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW|
            SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);

    // The SetParams function should have loaded something for us

    CString temp;
    CString sWindowTitle;
    bool localoperation = false;
    bool bSuccess = false;
    m_AlwaysConflicted = false;

    DialogEnableWindow(IDOK, FALSE);
    DialogEnableWindow(IDCANCEL, TRUE);
    SetAndClearProgressInfo(m_hWnd);
    m_itemCount = m_itemCountTotal;

    InterlockedExchange(&m_bThreadRunning, TRUE);
    iFirstResized = 0;
    bSecondResized = FALSE;
    m_bFinishedItemAdded = false;
    CTime startTime = CTime::GetCurrentTime();
    if (m_pTaskbarList)
        m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);

    if (!m_ProjectProperties.PropsRead())
    {
        m_ProjectProperties.ReadPropsPathList(m_targetPathList);
    }

    switch (m_Command)
    {
    case SVNProgress_Add:
        bSuccess = CmdAdd(sWindowTitle, localoperation);
        break;
    case SVNProgress_Checkout:
        bSuccess = CmdCheckout(sWindowTitle, localoperation);
        break;
    case SVNProgress_SparseCheckout:
        bSuccess = CmdSparseCheckout(sWindowTitle, localoperation);
        break;
    case SVNProgress_SingleFileCheckout:
        bSuccess = CmdSingleFileCheckout(sWindowTitle, localoperation);
        break;
    case SVNProgress_Commit:
        bSuccess = CmdCommit(sWindowTitle, localoperation);
        break;
    case SVNProgress_Copy:
        bSuccess = CmdCopy(sWindowTitle, localoperation);
        break;
    case SVNProgress_Export:
        bSuccess = CmdExport(sWindowTitle, localoperation);
        break;
    case SVNProgress_Import:
        bSuccess = CmdImport(sWindowTitle, localoperation);
        break;
    case SVNProgress_Lock:
        bSuccess = CmdLock(sWindowTitle, localoperation);
        break;
    case SVNProgress_Merge:
        bSuccess = CmdMerge(sWindowTitle, localoperation);
        break;
    case SVNProgress_MergeAll:
        bSuccess = CmdMergeAll(sWindowTitle, localoperation);
        break;
    case SVNProgress_MergeReintegrate:
        bSuccess = CmdMergeReintegrate(sWindowTitle, localoperation);
        break;
    case SVNProgress_MergeReintegrateOldStyle:
        bSuccess = CmdMergeReintegrateOldStyle(sWindowTitle, localoperation);
        break;
    case SVNProgress_Rename:
        bSuccess = CmdRename(sWindowTitle, localoperation);
        break;
    case SVNProgress_Resolve:
        bSuccess = CmdResolve(sWindowTitle, localoperation);
        break;
    case SVNProgress_Revert:
        bSuccess = CmdRevert(sWindowTitle, localoperation);
        break;
    case SVNProgress_Switch:
        bSuccess = CmdSwitch(sWindowTitle, localoperation);
        break;
    case SVNProgress_SwitchBackToParent:
        bSuccess = CmdSwitchBackToParent(sWindowTitle, localoperation);
        break;
    case SVNProgress_Unlock:
        bSuccess = CmdUnlock(sWindowTitle, localoperation);
        break;
    case SVNProgress_Update:
        bSuccess = CmdUpdate(sWindowTitle, localoperation);
        break;
    }
    if (!bSuccess)
        temp.LoadString(IDS_PROGRS_TITLEFAILED);
    else
        temp.LoadString(IDS_PROGRS_TITLEFIN);
    sWindowTitle = sWindowTitle + L" " + temp;
    SetWindowText(sWindowTitle);

    KillTimer(TRANSFERTIMER);
    KillTimer(VISIBLETIMER);

    CString sFinalInfo;
    if (!m_sTotalBytesTransferred.IsEmpty())
    {
        CTimeSpan time = CTime::GetCurrentTime() - startTime;
        temp.FormatMessage(IDS_PROGRS_TIME, (LONG)time.GetTotalMinutes(), (LONG)time.GetSeconds());
        sFinalInfo.FormatMessage(IDS_PROGRS_FINALINFO, m_sTotalBytesTransferred, (LPCTSTR)temp);
        SetDlgItemText(IDC_PROGRESSLABEL, sFinalInfo);
    }
    else
        GetDlgItem(IDC_PROGRESSLABEL)->ShowWindow(SW_HIDE);

    GetDlgItem(IDC_PROGRESSBAR)->ShowWindow(SW_HIDE);

    if (!m_bFinishedItemAdded)
    {
        // there's no "finished: xxx" line at the end. We add one here to make
        // sure the user sees that the command is actually finished.
        NotificationData * data = new NotificationData();
        data->bAuxItem = true;
        data->sActionColumnText.LoadString(IDS_PROGRS_FINISHED);
        AddItemToList(data);
    }

    int count = m_ProgList.GetItemCount();
    if ((count > 0)&&(m_bLastVisible))
        m_ProgList.EnsureVisible(count-1, FALSE);

    CLogFile logfile;
    if (logfile.Open())
    {
        logfile.AddTimeLine();
        for (const auto& data : m_arData)
        {
            temp.Format(L"%-20s : %s", (LPCTSTR)data->sActionColumnText, (LPCTSTR)data->sPathColumnText);
            logfile.AddLine(temp);
        }
        if (!sFinalInfo.IsEmpty())
            logfile.AddLine(sFinalInfo);
        logfile.Close();
    }
    GetDlgItem(IDC_JUMPCONFLICT)->ShowWindow(m_nTotalConflicts ? SW_SHOW : SW_HIDE);

    m_bCancelled = TRUE;
    InterlockedExchange(&m_bThreadRunning, FALSE);

    if (m_pTaskbarList)
    {
        if (DidErrorsOccur())
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
            m_pTaskbarList->SetProgressValue(m_hWnd, 100, 100);
        }
        else
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
    }
    if (m_bHookError && m_bHooksAreOptional)
        GetDlgItem(IDC_RETRYNOHOOKS)->ShowWindow(SW_SHOW);

    CString info = BuildInfoString();
    if (!bSuccess)
        info.LoadString(IDS_PROGRS_INFOFAILED);
    SetDlgItemText(IDC_INFOTEXT, info);
    ResizeColumns();
    DialogEnableWindow(IDCANCEL, FALSE);
    DialogEnableWindow(IDOK, TRUE);
    RefreshCursor();
    CWnd * pWndOk = GetDlgItem(IDOK);
    if (pWndOk && ::IsWindow(pWndOk->GetSafeHwnd()))
    {
        SendMessage(DM_SETDEFID, IDOK);
        GetDlgItem(IDOK)->SetFocus();
    }

    DWORD dwAutoClose = CRegStdDWORD(L"Software\\TortoiseSVN\\AutoClose");
    BOOL bAutoCloseLocal = CRegStdDWORD(L"Software\\TortoiseSVN\\AutoCloseLocal", FALSE);
    if ((m_options & ProgOptDryRun)||(!m_bLastVisible))
    {
        // don't autoclose for dry-runs or if the user scrolled the
        // content of the list control
        dwAutoClose = 0;
        bAutoCloseLocal = FALSE;
    }
    if (m_dwCloseOnEnd != (DWORD)-1)
        dwAutoClose = m_dwCloseOnEnd;       // command line value has priority over setting value
    if (m_bCloseLocalOnEnd != (DWORD)-1)
        bAutoCloseLocal = m_bCloseLocalOnEnd;

    bool sendClose = false;
    if ((dwAutoClose == CLOSE_NOERRORS)&&(!m_bErrorsOccurred))
        sendClose = true;
    if ((dwAutoClose == CLOSE_NOCONFLICTS)&&(!m_bErrorsOccurred)&&(m_nTotalConflicts==0))
        sendClose = true;
    if ((dwAutoClose == CLOSE_NOMERGES)&&(!m_bErrorsOccurred)&&(m_nTotalConflicts==0)&&(!m_bMergesAddsDeletesOccurred))
        sendClose = true;
    // kept for compatibility with pre 1.7 clients
    if ((dwAutoClose == CLOSE_LOCAL)&&(!m_bErrorsOccurred)&&(m_nTotalConflicts==0)&&(localoperation))
        sendClose = true;

    if ((bAutoCloseLocal)&&(!m_bErrorsOccurred)&&(m_nTotalConflicts==0)&&(localoperation))
        sendClose = true;

    if (sendClose)
        PostMessage(TORTOISESVN_CLOSEONEND_MSG, 0, 0);
    else if (m_hidden)
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_SHOWWINDOW|
            SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);

    //Don't do anything here which might cause messages to be sent to the window
    //The window thread is probably now blocked in OnOK if we've done an auto close
    return 0;
}

void CSVNProgressDlg::OnBnClickedLogbutton()
{
    if (m_Command == SVNProgress_Commit)
    {
        if (m_origPathList.GetCount() != 1)
            return;
        MergeAfterCommit();
    }
    else
    {
        if (m_targetPathList.GetCount() != 1)
            return;

        StringRevMap::iterator it = m_UpdateStartRevMap.begin();
        svn_revnum_t rev = -1;
        if (it != m_UpdateStartRevMap.end())
        {
            rev = it->second;
        }
        SVNRev endRev(rev);
        CString sCmd;
        sCmd.Format(L"/command:log /path:\"%s\" /endrev:%s",
                    m_targetPathList[0].GetWinPath(), (LPCTSTR)endRev.ToString());
        CAppUtils::RunTortoiseProc(sCmd);
    }
}

void CSVNProgressDlg::OnBnClickedRetrynohooks()
{
    CWnd * pWndButton = GetDlgItem(IDC_RETRYNOHOOKS);
    if ((pWndButton==nullptr) || !pWndButton->IsWindowVisible())
        return;
    ResetVars();

    m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
    if (m_pThread==NULL)
    {
        ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
        return;
    }
    else
    {
        m_pThread->m_bAutoDelete = FALSE;
        m_pThread->ResumeThread();
    }
    if (pWndButton)
        pWndButton->ShowWindow(SW_HIDE);
}

void CSVNProgressDlg::OnClose()
{
    if ((m_bCancelled)&&(m_bThreadRunning))
    {
        TerminateThread(m_pThread->m_hThread, (DWORD)-1);
        InterlockedExchange(&m_bThreadRunning, FALSE);
    }
    else if (!m_bCancelled)
    {
        m_bCancelled = TRUE;
        return;
    }
    DialogEnableWindow(IDCANCEL, TRUE);
    __super::OnClose();
}

void CSVNProgressDlg::OnOK()
{
    if ((GetKeyState(VK_MENU)&0x8000) == 0) // if the ALT key is pressed, we get here because of an accelerator
    {
        if (GetFocus() != GetDlgItem(IDOK))
            return; // if the "OK" button doesn't have the focus, do nothing: this prevents closing the dialog when pressing enter
    }
    if ((m_bCancelled)&&(!m_bThreadRunning))
    {
        // I have made this wait a sensible amount of time (10 seconds) for the thread to finish
        // You must be careful in the thread that after posting the WM_COMMAND/IDOK message, you
        // don't do any more operations on the window which might require message passing
        // If you try to send windows messages once we're waiting here, then the thread can't finished
        // because the Window's message loop is blocked at this wait
        if (WaitForSingleObject(m_pThread->m_hThread, 10000)==WAIT_TIMEOUT)
        {
            // end the process the hard way
            TerminateProcess(GetCurrentProcess(), 0);
        }
        if (CheckUpdateAndRetry())
            return;
        __super::OnOK();
    }
    m_bCancelled = TRUE;
}

LRESULT CSVNProgressDlg::OnCloseOnEnd(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_bCancelled = TRUE;

    // always wait for the thread to truely terminate.
    // (this function can only be triggered from that thread)

    if (   (m_pThread != NULL) && (m_bThreadRunning)
        && (WaitForSingleObject(m_pThread->m_hThread, 1000) != WAIT_OBJECT_0))
    {
        // end the process the hard way
        TerminateProcess(GetCurrentProcess(), 0);
    }

    __super::OnOK();
    return 0;
}


void CSVNProgressDlg::OnCancel()
{
    if ((m_bCancelled)&&(!m_bThreadRunning))
    {
        if (CheckUpdateAndRetry())
            return;
        __super::OnCancel();
    }
    m_bCancelled = TRUE;
}

void CSVNProgressDlg::OnLvnGetdispinfoSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;

    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    if (pDispInfo == 0)
        return;
    if ((pDispInfo->item.mask & LVIF_TEXT) == 0)
        return;
    if (pDispInfo->item.iItem >= (int)m_arData.size())
        return;

    // Set text pointer, will copy text in the code below
    m_columnbuf[0] = 0;
    pDispInfo->item.pszText = m_columnbuf;

    const NotificationData * data = m_arData[pDispInfo->item.iItem];
    if (data == 0)
        return;

    int indent = 0;
    WCHAR * pColumnBuf = m_columnbuf;
    if (pDispInfo->item.iSubItem == 1)
    {
        indent = data->indent;
        pColumnBuf = &m_columnbuf[data->indent];
        for (int i = 0; i < data->indent; ++i)
            m_columnbuf[i] = ' ';
    }

    const int maxLength = min(MAX_PATH-2-indent, pDispInfo->item.cchTextMax);
    switch (pDispInfo->item.iSubItem)
    {
    case 0:
        lstrcpyn(pColumnBuf, data->sActionColumnText, maxLength);
        break;
    case 1:
        lstrcpyn(pColumnBuf, data->sPathColumnText, maxLength);
        if (!data->bAuxItem)
        {
            int cWidth = m_ProgList.GetColumnWidth(1);
            cWidth = max(12, cWidth-12);
            CDC * pDC = m_ProgList.GetDC();
            if (pDC != NULL)
            {
                CFont * pFont = pDC->SelectObject(m_ProgList.GetFont());
                PathCompactPath(pDC->GetSafeHdc(), pColumnBuf, cWidth);
                pDC->SelectObject(pFont);
                ReleaseDC(pDC);
            }
        }
        break;
    case 2:
        lstrcpyn(pColumnBuf, data->mime_type, maxLength);
        break;
    default:
        break;
    }
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
        if (data->bBold)
        {
            SelectObject(pLVCD->nmcd.hdc, m_boldFont);
            // We changed the font, so we're returning CDRF_NEWFONT. This
            // tells the control to recalculate the extent of the text.
            *pResult = CDRF_NEWFONT;
        }
    }
}

void CSVNProgressDlg::OnNMDblclkSvnprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;
    if (pNMLV->iItem < 0)
        return;
    if (m_options & ProgOptDryRun)
        return; //don't do anything in a dry-run.

    const NotificationData * data = m_arData[pNMLV->iItem];
    if (data == NULL)
        return;

    if (data->bConflictedActionItem)
    {
        // We've double-clicked on a conflicted item - do a three-way merge on it
        CString sCmd;
        sCmd.Format(L"/command:conflicteditor /path:\"%s\" /resolvemsghwnd:%I64d /resolvemsgwparam:%I64d",
                    (LPCTSTR)data->path.GetWinPath(), (__int64)GetSafeHwnd(), (__int64)data->id);
        if (!data->path.IsUrl())
        {
            sCmd += L" /propspath:\"";
            sCmd += data->path.GetWinPathString();
            sCmd += L"\"";
        }
        CAppUtils::RunTortoiseProc(sCmd);
    }
    else if ((data->action == svn_wc_notify_update_update) && ((data->content_state == svn_wc_notify_state_merged)||(SVNProgress_Merge == m_Command)) || (data->action == svn_wc_notify_resolved))
    {
        // This is a modified file which has been merged on update. Diff it against base
        SVNDiff diff(NULL, this->m_hWnd, true); // do not pass 'this' as the SVN instance since that would make the diff command invoke this notify handler
        diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
        svn_revnum_t baseRev = 0;
        diff.DiffFileAgainstBase(data->path, baseRev, false);
    }
    else if ((!data->bAuxItem)&&(data->path.Exists())&&(!data->path.IsDirectory()))
    {
        bool bOpenWith = false;
        int ret = (int)ShellExecute(m_hWnd, NULL, data->path.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
        if (ret <= HINSTANCE_ERROR)
            bOpenWith = true;
        if (bOpenWith)
        {
            OPENASINFO oi = { 0 };
            oi.pcszFile = data->path.GetWinPath();
            oi.oaifInFlags = OAIF_EXEC;
            SHOpenWithDialog(GetSafeHwnd(), &oi);
        }
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
    m_ProgList.SetItemCountEx (static_cast<int>(m_arData.size()));

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
    if ((pProgressData->total > 1000LL)&&(!progControl->IsWindowVisible()))
    {
        progControl->ShowWindow(SW_SHOW);
        if (m_pTaskbarList)
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
    }
    if (((pProgressData->total < 0LL)&&(pProgressData->progress > 1000LL)&&(progControl->IsWindowVisible()))&&(m_itemCountTotal<0LL))
    {
        progControl->ShowWindow(SW_HIDE);
        if (m_pTaskbarList)
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
    }
    if (!GetDlgItem(IDC_PROGRESSLABEL)->IsWindowVisible())
        GetDlgItem(IDC_PROGRESSLABEL)->ShowWindow(SW_SHOW);
    SetTimer(TRANSFERTIMER, 2000, NULL);
    if ((pProgressData->total > 0LL)&&(pProgressData->progress > 1000LL))
    {
        progControl->SetPos((int)pProgressData->progress);
        progControl->SetRange32(0, (int)pProgressData->total);
        if (m_pTaskbarList)
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
            m_pTaskbarList->SetProgressValue(m_hWnd, pProgressData->progress, pProgressData->total);
        }
    }
    CString progText;
    if (pProgressData->overall_total < 1024LL)
        m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALBYTESTRANSFERRED, pProgressData->overall_total);
    else if (pProgressData->overall_total < 1200000LL)
        m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALTRANSFERRED, pProgressData->overall_total / 1024);
    else
        m_sTotalBytesTransferred.Format(IDS_SVN_PROGRESS_TOTALMBTRANSFERRED, (double)((double)pProgressData->overall_total / 1024000.0));
    progText.FormatMessage(L"%1!s!, %2!s!", (LPCTSTR)m_sTotalBytesTransferred, (LPCTSTR)pProgressData->SpeedString);
    SetDlgItemText(IDC_PROGRESSLABEL, progText);
    return 0;
}

void CSVNProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TRANSFERTIMER)
    {
        CString progText;
        CString progSpeed;
        progSpeed.Format(IDS_SVN_PROGRESS_BYTES_SEC, 0i64);
        progText.FormatMessage(IDS_SVN_PROGRESS_TOTALANDSPEED, (LPCTSTR)m_sTotalBytesTransferred, (LPCTSTR)progSpeed);
        SetDlgItemText(IDC_PROGRESSLABEL, progText);
        KillTimer(TRANSFERTIMER);
    }
    if (nIDEvent == VISIBLETIMER)
    {
        if (nEnsureVisibleCount)
            m_ProgList.EnsureVisible(m_ProgList.GetItemCount()-1, false);
        nEnsureVisibleCount = 0;
    }
}

LRESULT CSVNProgressDlg::OnResolveMsg( WPARAM wParam, LPARAM)
{
    if ((wParam > 0) && (wParam < m_arData.size()))
    {
        for (auto it = m_arData.begin(); it != m_arData.end(); ++it)
        {
            if ((*it)->id == (LONG)wParam)
            {
                if ((*it)->bConflictedActionItem)
                {
                    (*it)->color = ::GetSysColor(COLOR_WINDOWTEXT);
                    (*it)->action = svn_wc_notify_resolved;
                    (*it)->sActionColumnText.LoadString(IDS_SVNACTION_RESOLVE);
                    (*it)->bConflictedActionItem = false;
                    m_nTotalConflicts--;
                    CString info = BuildInfoString();
                    SetDlgItemText(IDC_INFOTEXT, info);
                    m_ProgList.Invalidate();
                    break;
                }
            }
        }
    }
    return 0;
}

void CSVNProgressDlg::Sort()
{
    if(m_arData.size() < 2)
    {
        return;
    }

    // We need to sort the blocks which lie between the auxiliary entries
    // This is so that any aux data stays where it was
    auto actionBlockEnd = m_arData.begin();   // We start searching from here

    for(;;)
    {
        // Search to the start of the non-aux entry in the next block
        auto actionBlockBegin = std::find_if(actionBlockEnd, m_arData.end(), std::not1(std::ptr_fun(&CSVNProgressDlg::NotificationDataIsAux)));
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
    case 0:     //action column
        result = pData1->sActionColumnText.Compare(pData2->sActionColumnText);
        break;
    case 1:     //path column
        // Compare happens after switch()
        break;
    case 2:     //mime-type column
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
        if (!IsCursorOverWindowBorder() && ((pWnd)&&(pWnd != GetDlgItem(IDCANCEL))))
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
        if ((pMsg->wParam == 'C')||(pMsg->wParam == VK_INSERT))
        {
            int selIndex = m_ProgList.GetSelectionMark();
            if (selIndex >= 0)
            {
                if (GetKeyState(VK_CONTROL)&0x8000)
                {
                    //Ctrl-C -> copy to clipboard
                    POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
                    if (pos != NULL)
                    {
                        CString sClipdata;
                        while (pos)
                        {
                            int nItem = m_ProgList.GetNextSelectedItem(pos);
                            CString sAction = m_ProgList.GetItemText(nItem, 0);
                            CString sPath = m_ProgList.GetItemText(nItem, 1);
                            CString sMime = m_ProgList.GetItemText(nItem, 2);
                            CString sLogCopyText;
                            sLogCopyText.Format(L"%s: %s  %s\r\n",
                                (LPCTSTR)sAction, (LPCTSTR)sPath, (LPCTSTR)sMime);
                            sClipdata +=  sLogCopyText;
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
        return; // don't do anything in a dry-run.

    if (pWnd != &m_ProgList)
        return;

    int selIndex = m_ProgList.GetSelectionMark();
    if ((point.x == -1) && (point.y == -1))
    {
        // Menu was invoked from the keyboard rather than by right-clicking
        CRect rect;
        m_ProgList.GetItemRect(selIndex, &rect, LVIR_LABEL);
        m_ProgList.ClientToScreen(&rect);
        point = rect.CenterPoint();
    }

    if ((selIndex < 0)||(m_bThreadRunning))
        return;

    // entry is selected, thread has finished with updating so show the popup menu
    CIconMenu popup;
    if (!popup.CreatePopupMenu())
        return;

    bool bAdded = false;
    NotificationData * data = m_arData[selIndex];
    if ((data)&&(!data->path.IsDirectory()))
    {
        if ((data->action == svn_wc_notify_update_update || data->action == svn_wc_notify_resolved) && (!m_UpdateStartRevMap.empty()))
        {
            popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
            bAdded = true;
        }
        if (data->bConflictedActionItem)
        {
            if (m_ProgList.GetSelectedCount() == 1)
            {
                popup.AppendMenuIcon(ID_EDITCONFLICT, IDS_MENUCONFLICT,IDI_CONFLICT);
                popup.SetDefaultItem(ID_EDITCONFLICT, FALSE);
                popup.AppendMenuIcon(ID_CONFLICTRESOLVE, IDS_SVNPROGRESS_MENUMARKASRESOLVED,IDI_RESOLVE);
            }
            if (!data->bTreeConflict)
            {
                popup.AppendMenuIcon(ID_CONFLICTUSETHEIRS, IDS_SVNPROGRESS_MENUUSETHEIRS,IDI_RESOLVE);
                popup.AppendMenuIcon(ID_CONFLICTUSEMINE, IDS_SVNPROGRESS_MENUUSEMINE,IDI_RESOLVE);
            }
        }
        else if ((data->content_state == svn_wc_notify_state_merged)||(SVNProgress_Merge == m_Command)||(data->action == svn_wc_notify_resolved))
            popup.SetDefaultItem(ID_COMPARE, FALSE);

        if (m_ProgList.GetSelectedCount() == 1)
        {
            if ((data->action == svn_wc_notify_add)||
                (data->action == svn_wc_notify_update_add)||
                (data->action == svn_wc_notify_commit_added)||
                (data->action == svn_wc_notify_commit_modified)||
                (data->action == svn_wc_notify_restore)||
                (data->action == svn_wc_notify_revert)||
                (data->action == svn_wc_notify_resolved)||
                (data->action == svn_wc_notify_commit_replaced)||
                (data->action == svn_wc_notify_commit_postfix_txdelta)||
                (data->action == svn_wc_notify_update_update))
            {
                popup.AppendMenuIcon(ID_LOG, IDS_MENULOG,IDI_LOG);
                if (data->action == svn_wc_notify_update_update)
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
                popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
                bAdded = true;
            }
        }
    }

    if (m_ProgList.GetSelectedCount() == 1)
    {
        if (data)
        {
            CString sPath = GetPathFromColumnText(data->sPathColumnText);
            if ((!sPath.IsEmpty())&&(!SVN::PathIsURL(CTSVNPath(sPath))))
            {
                CTSVNPath path = CTSVNPath(sPath);
                if (path.GetDirectory().Exists())
                {
                    popup.AppendMenuIcon(ID_EXPLORE, IDS_SVNPROGRESS_MENUOPENPARENT, IDI_EXPLORER);
                    bAdded = true;
                }
            }
        }
    }
    if (m_ProgList.GetSelectedCount() > 0)
    {
        if (bAdded)
            popup.AppendMenu(MF_SEPARATOR, NULL);
        popup.AppendMenuIcon(ID_COPY, IDS_LOG_POPUP_COPYTOCLIPBOARD,IDI_COPYCLIP);
        bAdded = true;
    }
    if (!bAdded)
        return;

    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, 0);
    DialogEnableWindow(IDOK, FALSE);
    this->SetPromptApp(&theApp);
    theApp.DoWaitCursor(1);
    bool bOpenWith = false;
    switch (cmd)
    {
    case ID_COPY:
        {
            CString sLines;
            POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
            while (pos)
            {
                int nItem = m_ProgList.GetNextSelectedItem(pos);
                NotificationData * data2 = m_arData[nItem];
                if (data2)
                {
                    sLines += data2->sPathColumnText;
                    sLines += L"\r\n";
                }
            }
            sLines.TrimRight();
            if (!sLines.IsEmpty())
            {
                CStringUtils::WriteAsciiStringToClipboard(sLines, GetSafeHwnd());
            }
        }
        break;
    case ID_EXPLORE:
        {
            if (data == NULL)
                break;
            CString sPath = GetPathFromColumnText(data->sPathColumnText);
            CTSVNPath path = CTSVNPath(sPath);
            ShellExecute(m_hWnd, L"explore", path.GetDirectory().GetWinPath(), NULL, path.GetDirectory().GetWinPath(), SW_SHOW);
        }
        break;
    case ID_COMPARE:
        {
            if (data == NULL)
                break;
            POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
            CString sResolvedPaths;
            while (pos)
            {
                int nItem = m_ProgList.GetNextSelectedItem(pos);
                NotificationData * data2 = m_arData[nItem];
                if (data2 == 0)
                    continue;
                CompareWithWC(data2);
            }
        }
        break;
    case ID_EDITCONFLICT:
        {
            if (data == NULL)
                break;
            CString sPath = GetPathFromColumnText(data->sPathColumnText);
            CString sCmd;
            sCmd.Format(L"/command:conflicteditor /path:\"%s\" /resolvemsghwnd:%I64d /resolvemsgwparam:%I64d",
                        (LPCTSTR)sPath, (__int64)GetSafeHwnd(), (__int64)data->id);
            CAppUtils::RunTortoiseProc(sCmd);
        }
        break;
    case ID_CONFLICTUSETHEIRS:
    case ID_CONFLICTUSEMINE:
    case ID_CONFLICTRESOLVE:
        {
            svn_wc_conflict_choice_t result = svn_wc_conflict_choose_merged;
            switch (cmd)
            {
                case ID_CONFLICTUSETHEIRS:
                    result = svn_wc_conflict_choose_theirs_full;
                    break;
                case ID_CONFLICTUSEMINE:
                    result = svn_wc_conflict_choose_mine_full;
                    break;
                case ID_CONFLICTRESOLVE:
                    result = svn_wc_conflict_choose_merged;
                    break;
            }
            SVN svn;
            POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
            CString sResolvedPaths;
            while (pos)
            {
                int nItem = m_ProgList.GetNextSelectedItem(pos);
                NotificationData * data2 = m_arData[nItem];
                if (data2 == 0)
                    continue;
                if (!(data2->bConflictedActionItem))
                    continue;
                if (!svn.Resolve(data2->path, result, FALSE, false, svn_wc_conflict_kind_text))
                {
                    svn.ShowErrorDialog(m_hWnd, data2->path);
                    DialogEnableWindow(IDOK, TRUE);
                    break;
                }
                data2->color = ::GetSysColor(COLOR_WINDOWTEXT);
                data2->action = svn_wc_notify_resolved;
                data2->sActionColumnText.LoadString(IDS_SVNACTION_RESOLVE);
                data2->bConflictedActionItem = false;
                m_nTotalConflicts--;

                if (m_nTotalConflicts==0)
                {
                    // When the last conflict is resolved we remove
                    // the warning(s).
                    int index = 0;
                    auto cs = m_arData.begin();
                    while (cs != m_arData.end())
                    {
                        if ((*cs)->bConflictSummary)
                        {
                            delete (*cs);
                            cs = m_arData.erase(cs);
                            m_ProgList.DeleteItem(index);
                        }
                        else
                        {
                            ++cs;
                        }
                        ++index;
                    }
                }
                sResolvedPaths += data2->path.GetWinPathString() + L"\n";
            }
            m_ProgList.Invalidate();
            CString info = BuildInfoString();
            SetDlgItemText(IDC_INFOTEXT, info);

            if (!sResolvedPaths.IsEmpty())
            {
                CString msg;
                msg.Format(IDS_SVNPROGRESS_RESOLVED, (LPCTSTR)sResolvedPaths);
                ::MessageBox(m_hWnd, msg, L"TortoiseSVN", MB_OK | MB_ICONINFORMATION);
            }
        }
        break;
    case ID_LOG:
        {
            if (data == NULL)
                break;
            // fetch the log from HEAD, not the revision we updated to:
            // the path might be inside an external folder which has its own
            // revisions.
            CString sPath = GetPathFromColumnText(data->sPathColumnText);
            CString sCmd;
            sCmd.Format(L"/command:log /path:\"%s\"",
                (LPCTSTR)sPath);

            CAppUtils::RunTortoiseProc(sCmd);
        }
        break;
    case ID_OPENWITH:
        bOpenWith = true;
        // fall through here
    case ID_OPEN:
        {
            if (data == NULL)
                break;
            CString sWinPath = GetPathFromColumnText(data->sPathColumnText);
            if (!bOpenWith)
            {
                const int ret = (int)ShellExecute(this->m_hWnd, NULL, (LPCTSTR)sWinPath, NULL, NULL, SW_SHOWNORMAL);
                if ((ret <= HINSTANCE_ERROR)||bOpenWith)
                {
                    OPENASINFO oi = { 0 };
                    oi.pcszFile = sWinPath;
                    oi.oaifInFlags = OAIF_EXEC;
                    SHOpenWithDialog(GetSafeHwnd(), &oi);
                }
            }
        }
        break;
    }
    DialogEnableWindow(IDOK, TRUE);
    theApp.DoWaitCursor(-1);
}

void CSVNProgressDlg::OnLvnBegindragSvnprogress(NMHDR* pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;

    int selIndex = m_ProgList.GetSelectionMark();
    if (selIndex < 0)
        return;

    CTSVNPathList pathList;
    int index;
    POSITION pos = m_ProgList.GetFirstSelectedItemPosition();
    while ( (index = m_ProgList.GetNextSelectedItem(pos)) >= 0 )
    {
        NotificationData * data = m_arData[index];

        if ( data->kind==svn_node_file || data->kind==svn_node_dir )
        {
            CString sPath = GetPathFromColumnText(data->sPathColumnText);
            pathList.AddPath(CTSVNPath(sPath));
        }
    }

    if (pathList.GetCount() == 0)
    {
        return;
    }

    std::unique_ptr<CIDropSource> pdsrc(new CIDropSource);
    if (pdsrc == NULL)
        return;
    pdsrc->AddRef();


    SVNDataObject* pdobj = new SVNDataObject(pathList, SVNRev::REV_WC, SVNRev::REV_WC);
    if (pdobj == NULL)
    {
        return;
    }
    pdobj->AddRef();

    CDragSourceHelper dragsrchelper;

    dragsrchelper.InitializeFromWindow(m_ProgList.GetSafeHwnd(), pNMLV->ptAction, pdobj);

    pdsrc->m_pIDataObj = pdobj;
    pdsrc->m_pIDataObj->AddRef();

    // Initiate the Drag & Drop
    DWORD dwEffect;
    ::DoDragDrop(pdobj, pdsrc.get(), DROPEFFECT_MOVE|DROPEFFECT_COPY, &dwEffect);
    pdsrc->Release();
    pdsrc.release();
    pdobj->Release();
}

void CSVNProgressDlg::OnSize(UINT nType, int cx, int cy)
{
    CResizableStandAloneDialog::OnSize(nType, cx, cy);
    if ((nType == SIZE_RESTORED)&&(m_bLastVisible))
    {
        int count = m_ProgList.GetItemCount();
        if (count > 0)
            m_ProgList.EnsureVisible(count-1, false);
    }
}

void CSVNProgressDlg::OnCommitFinished()
{
    if (m_BugTraqProvider == 0)
        return;

    CComPtr<IBugTraqProvider2> pProvider;
    HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider);
    if (FAILED(hr))
        return;

    ATL::CComBSTR commonRoot(m_selectedPaths.GetCommonRoot().GetDirectory().GetWinPath());

    CBstrSafeVector pathList(m_selectedPaths.GetCount());
    for (LONG index = 0; index < m_selectedPaths.GetCount(); ++index)
        pathList.PutElement(index, m_selectedPaths[index].GetSVNPathString());

    ATL::CComBSTR logMessage;
    logMessage.Attach(m_sMessage.AllocSysString());

    ATL::CComBSTR temp;
    hr = pProvider->OnCommitFinished(GetSafeHwnd(), commonRoot, pathList,
        logMessage, (LONG)m_RevisionEnd, &temp);
    if (SUCCEEDED(hr))
        return;

    CString sErr = temp;
    if (!sErr.IsEmpty())
    {
        ReportError(sErr);
        return;
    }
    COMError ce(hr);
    sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, ce.GetSource().c_str(), ce.GetMessageAndDescription().c_str());
    ReportError(sErr);
}

//////////////////////////////////////////////////////////////////////////
/// commands
//////////////////////////////////////////////////////////////////////////
bool CSVNProgressDlg::CmdAdd(CString& sWindowTitle, bool& localoperation)
{
    localoperation = true;
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_ADD);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_ADD_BKG);
    ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_ADD)));
    if (!Add(m_targetPathList, &m_ProjectProperties, svn_depth_empty, (m_options & ProgOptForce) != 0, (m_options & ProgOptUseAutoprops) != 0, true, true))
    {
        ReportSVNError();
        return false;
    }
    CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
    return true;
}

bool CSVNProgressDlg::CmdCheckout(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_CHECKOUT);
    SetBackgroundImage(IDI_CHECKOUT_BKG);
    CTSVNPathList urls;
    urls.LoadFromAsteriskSeparatedString(m_url.GetSVNPathString());
    for (int i=0; i<urls.GetCount(); ++i)
    {
        CAppUtils::SetWindowTitle(m_hWnd, urls[i].GetUIFileOrDirectoryName(), sWindowTitle);
        CTSVNPath checkoutdir = m_targetPathList[0];
        if (urls.GetCount() > 1)
        {
            CString fileordir = urls[i].GetFileOrDirectoryName();
            fileordir = CPathUtils::PathUnescape(fileordir);
            checkoutdir.AppendPathString(fileordir);
        }
        CString sCmdInfo;
        sCmdInfo.FormatMessage(IDS_PROGRS_CMD_CHECKOUT,
            (LPCTSTR)urls[i].GetSVNPathString(), (LPCTSTR)m_Revision.ToString(),
            (LPCTSTR)SVNStatus::GetDepthString(m_depth),
            (m_options & ProgOptIgnoreExternals) ? (LPCTSTR)sExtExcluded : (LPCTSTR)sExtIncluded);
        ReportCmd(sCmdInfo);

        CBlockCacheForPath cacheBlock (checkoutdir.GetWinPath());
        if (!Checkout(urls[i], checkoutdir, m_Revision, m_Revision, m_depth, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(REG_KEY_ALLOW_UNV_OBSTRUCTIONS, true))))
        {
            if (m_ProgList.GetItemCount()>1)
            {
                ReportSVNError();
                return false;
            }
            // if the checkout fails with the peg revision set to the checkout revision,
            // try again with HEAD as the peg revision.
            else
            {
                if (!Checkout(urls[i], checkoutdir, SVNRev::REV_HEAD, m_Revision, m_depth, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(REG_KEY_ALLOW_UNV_OBSTRUCTIONS, true))))
                {
                    ReportSVNError();
                    return false;
                }
            }
        }
    }

    DWORD exitcode = 0;
    CString error;
    // read the project properties from the fresh working copy
    m_ProjectProperties.ReadProps(m_targetPathList[0]);
    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    CTSVNPathList updatedList = GetPathsForUpdateHook(m_targetPathList);
    if ((!m_bNoHooks)&&(CHooks::Instance().PostUpdate(m_hWnd, m_targetPathList, m_depth, m_RevisionEnd, updatedList, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(post_update_hook, m_targetPathList, error);
            return false;
        }
    }
    return true;
}

bool CSVNProgressDlg::CmdSparseCheckout(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_CHECKOUT);
    SetBackgroundImage(IDI_CHECKOUT_BKG);

    CString sCmdInfo;
    int index = 0;
    CString rootUrl;
    for (std::map<CString,svn_depth_t>::iterator it = m_pathdepths.begin(); it != m_pathdepths.end(); ++it)
    {
        if (index == 0)
            rootUrl = it->first;
        svn_depth_t depth = it->second;
        CTSVNPath url = CTSVNPath(it->first);
        CTSVNPath checkoutdir = m_targetPathList[0];
        CString fileordir = it->first.Mid(rootUrl.GetLength());
        if ((index >= 1)||(SVNHelper::IsVersioned(checkoutdir, false)))
        {
            fileordir = CPathUtils::PathUnescape(fileordir);
            checkoutdir.AppendPathString(fileordir);
            fileordir.TrimLeft('/');
        }
        if ((depth == svn_depth_unknown)&&(checkoutdir.Exists()))
        {
            ++index;
            continue;
        }
        if (depth == svn_depth_unknown)
            depth = svn_depth_empty;

        CAppUtils::SetWindowTitle(m_hWnd, url.GetUIFileOrDirectoryName(), sWindowTitle);
        if ((index >= 1)||(SVNHelper::IsVersioned(checkoutdir, false)))
        {
            sCmdInfo.FormatMessage(IDS_PROGRS_CMD_SPARSEUPDATE, (LPCTSTR)fileordir, (LPCTSTR)SVNStatus::GetDepthString(it->second));
            ReportCmd(sCmdInfo);
        }
        else
        {
            sCmdInfo.FormatMessage(IDS_PROGRS_CMD_CHECKOUT,
                (LPCTSTR)url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString(),
                (LPCTSTR)SVNStatus::GetDepthString(m_depth),
                (m_options & ProgOptIgnoreExternals) ? (LPCTSTR)sExtExcluded : (LPCTSTR)sExtIncluded);
            ReportCmd(sCmdInfo);
        }

        CBlockCacheForPath cacheBlock (checkoutdir.GetWinPath());
        if ((index == 0)&&(!SVNHelper::IsVersioned(checkoutdir, false)))
        {
            if (!Checkout(url, checkoutdir, m_Revision, m_Revision, it->second, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(REG_KEY_ALLOW_UNV_OBSTRUCTIONS, true))))
            {
                if (m_ProgList.GetItemCount()>1)
                {
                    ReportSVNError();
                    return false;
                }
                // if the checkout fails with the peg revision set to the checkout revision,
                // try again with HEAD as the peg revision.
                else
                {
                    if (!Checkout(url, checkoutdir, SVNRev::REV_HEAD, m_Revision, m_depth, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(REG_KEY_ALLOW_UNV_OBSTRUCTIONS, true))))
                    {
                        ReportSVNError();
                        return false;
                    }
                }
            }
        }
        else
        {
            if ((depth != svn_depth_exclude)||(checkoutdir.Exists()))
            {
                if (!Update(CTSVNPathList(checkoutdir), m_Revision, depth, true, (m_options & ProgOptIgnoreExternals) != 0, true, false))
                {
                    ReportSVNError();
                    return false;
                }
            }
        }
        ++index;
    }

    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    CTSVNPathList updatedList = GetPathsForUpdateHook(m_targetPathList);
    if ((!m_bNoHooks)&&(CHooks::Instance().PostUpdate(m_hWnd, m_targetPathList, m_depth, m_RevisionEnd, updatedList, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(post_update_hook, m_targetPathList, error);
            return false;
        }
    }
    return true;
}

bool CSVNProgressDlg::CmdSingleFileCheckout(CString& sWindowTitle, bool& localoperation)
{
    // get urls to c/o

    CTSVNPathList urls;
    urls.LoadFromAsteriskSeparatedString(m_url.GetSVNPathString());
    if (urls.GetCount() == 0)
        return true;

    // preparation

    CTSVNPath parentURL = urls[0].GetContainingDirectory();
    CTSVNPath checkoutdir = m_targetPathList[0];

    sWindowTitle.LoadString(IDS_PROGRS_TITLE_CHECKOUT);
    SetBackgroundImage(IDI_CHECKOUT_BKG);

    CAppUtils::SetWindowTitle(m_hWnd, checkoutdir.GetUIFileOrDirectoryName(), sWindowTitle);

    // c/o base folder

    SVNInfo info;
    const SVNInfoData* infoData
        = info.GetFirstFileInfo (checkoutdir, SVNRev(), SVNRev());

    bool allow_obstructions
        = !!DWORD (CRegDWORD (REG_KEY_ALLOW_UNV_OBSTRUCTIONS , true));

    bool revisionIsHead = m_Revision.IsHead() || !m_Revision.IsValid();
    if ((infoData == NULL) || (infoData->url != parentURL.GetSVNPathString()))
    {
        CPathUtils::MakeSureDirectoryPathExists (checkoutdir.GetWinPath());

        if (   !Checkout ( parentURL
                         , checkoutdir
                         , m_Revision
                         , m_Revision
                         , svn_depth_empty
                         , (m_options & ProgOptIgnoreExternals) != 0
                         , allow_obstructions)

             // retry with HEAD as pegRev
            && (   revisionIsHead
                || !Checkout ( parentURL
                             , checkoutdir
                             , SVNRev::REV_HEAD
                             , m_Revision
                             , svn_depth_empty
                             , (m_options & ProgOptIgnoreExternals) != 0
                             , allow_obstructions)))
        {
            ReportSVNError();
            return false;
        }
    }

    // add the sub-items

    m_url = CTSVNPath();
    for (int i=0; i<urls.GetCount(); ++i)
    {
        CTSVNPath filePath = checkoutdir;
        filePath.AppendPathString (urls[i].GetFilename());
        m_targetPathList = CTSVNPathList (filePath);
        m_options = ProgOptNone;

        if (!CmdUpdate (sWindowTitle, localoperation))
            return false;
    }

    return true;
}

bool CSVNProgressDlg::CmdCommit(CString& sWindowTitle, bool& /*localoperation*/)
{
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_COMMIT);
    SetWindowText(sWindowTitle);
    SetBackgroundImage(IDI_COMMIT_BKG);
    if (m_targetPathList.GetCount()==0)
    {
        DialogEnableWindow(IDCANCEL, FALSE);
        DialogEnableWindow(IDOK, TRUE);

        InterlockedExchange(&m_bThreadRunning, FALSE);
        return true;
    }
    if (m_targetPathList.GetCount()==1)
    {
        CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    }
    CString commitUrl;
    if (IsCommittingToTag(commitUrl))
    {
        CString sTask1;
        WCHAR outUrl[MAX_PATH] = { 0 };
        PathCompactPathEx(outUrl, commitUrl.GetBufferSetLength(MAX_PATH), 50, 0);
        sTask1.Format(IDS_PROGRS_COMMITT_TRUNK_TASK1, outUrl);
        CTaskDialog taskdlg(sTask1,
                            CString(MAKEINTRESOURCE(IDS_PROGRS_COMMITT_TRUNK_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_PROGRS_COMMITT_TRUNK_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_PROGRS_COMMITT_TRUNK_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        if (taskdlg.DoModal(m_hWnd) != 1)
            return false;
    }
    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    if ((!m_bNoHooks)&&(CHooks::Instance().PreCommit(m_hWnd, m_selectedPaths, m_depth, m_sMessage, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(pre_commit_hook, m_selectedPaths, error);
            return false;
        }
    }

    ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_COMMIT)));
    CStringArray changelists;
    if (!m_changelist.IsEmpty())
        changelists.Add(m_changelist);
    bool commitSuccessful = true;
    CBlockCacheForPath cacheBlock (m_targetPathList.GetCommonRoot().GetWinPath());
    if (!Commit(m_targetPathList, m_sMessage, changelists, m_keepchangelist,
        m_depth, (m_options & ProgOptKeeplocks) != 0, m_revProps))
    {
        ReportSVNError();
        error = GetLastErrorMessage();
        // if a non-recursive commit failed with SVN_ERR_UNSUPPORTED_FEATURE,
        // that means a folder deletion couldn't be committed.
        if ((m_Revision != 0)&&(Err)&&(Err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE))
        {
            ReportError(CString(MAKEINTRESOURCE(IDS_PROGRS_NONRECURSIVEHINT)));
        }
        commitSuccessful = false;
        return false;
    }
    if (!PostCommitErr.IsEmpty())
    {
        // post commit errors, while still errors don't make
        // the commit itself fail. So we restore the old error
        // state after reporting the post-commit error.
        bool bErrState = m_bErrorsOccurred;
        ReportError(PostCommitErr);
        m_bErrorsOccurred = bErrState;
    }
    if (commitSuccessful)
    {
        OnCommitFinished();
    }
    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    if ((!m_bNoHooks)&&(CHooks::Instance().PostCommit(m_hWnd, m_selectedPaths, m_depth, m_RevisionEnd, m_sMessage, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(post_commit_hook, m_selectedPaths, error);
            return false;
        }
    }
    if (!m_restorepaths.empty())
    {
        // use a separate SVN object to avoid getting the notifications when
        // we re-set the changelists
        SVN svn;
        for (auto it = m_restorepaths.cbegin(); it != m_restorepaths.cend(); ++it)
        {
            CopyFile(it->first, std::get<0>(it->second), FALSE);
            svn.AddToChangeList(CTSVNPathList(CTSVNPath(std::get<0>(it->second))), std::get<1>(it->second), svn_depth_empty);
        }
        m_restorepaths.clear();
    }

    // after a commit, show the user the merge button, but only if only one single item was committed
    // (either a file or a directory)
    if ((m_origPathList.GetCount() == 1)&&(m_RevisionEnd.IsValid()))
    {
        SetDlgItemText(IDC_LOGBUTTON, CString(MAKEINTRESOURCE(IDS_MERGE_MERGE)));
        GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);

        // find out whether this committed item was an external
        CTSVNPath parentPath = m_origPathList[0].GetContainingDirectory();
        if (!parentPath.IsEquivalentTo(m_origPathList[0]))
        {
            SVNProperties props(parentPath, SVNRev::REV_WC, false, false);
            if (props.HasProperty("svn:externals"))
            {
                std::string extval = props.GetItemValue(props.IndexOf("svn:externals"));
                SVNExternals exts;
                exts.Add(parentPath, extval, false);
                bool bFound = false;
                for (auto it = exts.begin(); it != exts.end(); ++it)
                {
                    if (it->targetDir.CompareNoCase(m_origPathList[0].GetFileOrDirectoryName()) == 0)
                    {
                        if ((it->pegrevision.kind != svn_opt_revision_head) || (it->origrevision.kind != svn_opt_revision_head))
                        {
                            it->pegrevision.kind = ((const svn_opt_revision_t*)m_RevisionEnd)->kind;
                            it->pegrevision.value = ((const svn_opt_revision_t*)m_RevisionEnd)->value;
                            it->revision.kind = svn_opt_revision_head;
                            it->revision.value.number = (svn_revnum_t)-1;
                            it->origrevision.kind = svn_opt_revision_head;
                            it->origrevision.value.number = (svn_revnum_t)-1;
                            it->adjust = true;
                            bFound = true;
                            break;
                        }
                    }
                }
                if (bFound)
                {
                    // now ask the user whether to tag the external to the new HEAD revision after this commit
                    bool bTag = false;
                    // tagging external
                    // You've committed changes to external item that is tagged to a specific revision.
                    // Do you want to change the tagged revision now to the just committed revision?
                    //
                    // Change the tagged revision
                    //  The svn:external property of the parent folder is changed accordingly
                    // Cancel
                    //  The tagged revision of the external is left as is
                    CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_ADJUST_EXTERNAL_AFTER_COMMIT_TASK1)),
                                        CString(MAKEINTRESOURCE(IDS_ADJUST_EXTERNAL_AFTER_COMMIT_TASK2)),
                                        L"TortoiseSVN",
                                        0,
                                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
                    taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_ADJUST_EXTERNAL_AFTER_COMMIT_TASK3)));
                    taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_ADJUST_EXTERNAL_AFTER_COMMIT_TASK4)));
                    taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                    taskdlg.SetDefaultCommandControl(2);
                    taskdlg.SetMainIcon(TD_INFORMATION_ICON);
                    if (taskdlg.DoModal(GetExplorerHWND()) == 1)
                        bTag = true;
                    if (bTag)
                    {
                        exts.TagExternals(false);
                        // now start the commit dialog with the folder where we changed the external tag
                        CString sCmd;
                        sCmd.Format(L"/command:commit /path:\"%s\"", parentPath.GetWinPath());
                        CAppUtils::RunTortoiseProc(sCmd);
                    }
                }
            }
        }
    }

    return true;
}

bool CSVNProgressDlg::CmdCopy(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_COPY);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_COPY_BKG);

    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    if ((!m_bNoHooks)&&(CHooks::Instance().PreCommit(m_hWnd, m_selectedPaths, m_depth, m_sMessage, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(pre_commit_hook, m_selectedPaths, error);
            return false;
        }
    }

    CString sCmdInfo;
    CString sUrl = m_targetPathList[0].IsUrl() ? (LPCTSTR)m_targetPathList[0].GetSVNPathString() : m_targetPathList[0].GetWinPath();
    if (m_targetPathList[0].IsUrl() && m_pegRev.IsValid())
        sUrl += L"@" + m_pegRev.ToString();
    sCmdInfo.FormatMessage(IDS_PROGRS_CMD_COPY,
        (LPCTSTR)sUrl,
        (LPCTSTR)m_url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString());
    ReportCmd(sCmdInfo);

    bool makeparents = (m_options & ProgOptMakeParents)!=0;
    // when creating intermediate folders with "make parents", always use "copy_as_child"
    if (!Copy(m_targetPathList, m_url, m_Revision, m_pegRev, m_sMessage, makeparents, makeparents, (m_options & ProgOptIgnoreExternals) != 0, m_externals.NeedsTagging(), m_externals, m_revProps))
    {
        ReportSVNError();
        return false;
    }
    if (!PostCommitErr.IsEmpty())
    {
        ReportError(PostCommitErr);
    }

    if (m_options & ProgOptSwitchAfterCopy)
    {
        sCmdInfo.FormatMessage(IDS_PROGRS_CMD_SWITCH,
            m_targetPathList[0].GetWinPath(),
            (LPCTSTR)m_url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString());
        ReportCmd(sCmdInfo);
        if (!Switch(m_targetPathList[0], m_url, SVNRev::REV_HEAD, SVNRev::REV_HEAD, m_depth, (m_options & ProgOptStickyDepth) != 0, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(L"Software\\TortoiseSVN\\AllowUnversionedObstruction", true)), (m_options & ProgOptIgnoreAncestry) != 0))
        {
            if (m_ProgList.GetItemCount()>1)
            {
                ReportSVNError();
                return false;
            }
            else if (!Switch(m_targetPathList[0], m_url, SVNRev::REV_HEAD, m_Revision, m_depth, (m_options & ProgOptStickyDepth) != 0, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(L"Software\\TortoiseSVN\\AllowUnversionedObstruction", true)), (m_options & ProgOptIgnoreAncestry) != 0))
            {
                ReportSVNError();
                return false;
            }
        }
    }
    else
    {
        if (SVN::PathIsURL(m_url))
        {
            CString sMsg(MAKEINTRESOURCE(IDS_PROGRS_COPY_WARNING));
            ReportNotification(sMsg);
        }
    }

    OnCommitFinished();

    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    if ((!m_bNoHooks)&&(CHooks::Instance().PostCommit(m_hWnd, m_selectedPaths, m_depth, m_RevisionEnd, m_sMessage, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(post_commit_hook, m_selectedPaths, error);
            return false;
        }
    }
    return true;
}

bool CSVNProgressDlg::CmdExport(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_EXPORT);
    CAppUtils::SetWindowTitle(m_hWnd, m_url.GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_EXPORT_BKG);
    CString eol;
    if (m_options & ProgOptEolCRLF)
        eol = L"CRLF";
    if (m_options & ProgOptEolLF)
        eol = L"LF";
    if (m_options & ProgOptEolCR)
        eol = L"CR";
    ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_EXPORT)));

    CTSVNPath targetPath = m_targetPathList[0];
    if (SVNInfo::IsFile (m_url, m_Revision))
        targetPath.AppendPathString (m_url.GetFilename());

    CBlockCacheForPath cacheBlock (targetPath.GetWinPath());
    if (!Export(m_url, targetPath, m_Revision, m_Revision, true, (m_options & ProgOptIgnoreExternals) != 0, (m_options & ProgOptIgnoreKeywords) != 0, m_depth, NULL, SVNExportNormal, eol))
    {
        ReportSVNError();
        return false;
    }
    return true;
}

bool CSVNProgressDlg::CmdImport(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_IMPORT);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_IMPORT_BKG);

    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    if ((!m_bNoHooks)&&(CHooks::Instance().PreCommit(m_hWnd, m_selectedPaths, m_depth, m_sMessage, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(pre_commit_hook, m_selectedPaths, error);
            return false;
        }
    }

    CString sCmdInfo;
    sCmdInfo.FormatMessage(IDS_PROGRS_CMD_IMPORT,
        m_targetPathList[0].GetWinPath(), (LPCTSTR)m_url.GetSVNPathString(),
        (m_options & ProgOptIncludeIgnored) ? (LPCTSTR)(L", " + sIgnoredIncluded) : L"");
    ReportCmd(sCmdInfo);
    if (!Import(m_targetPathList[0], m_url, m_sMessage, &m_ProjectProperties, svn_depth_infinity, (m_options & ProgOptUseAutoprops) != 0, (m_options & ProgOptIncludeIgnored) ? true : false, false, m_revProps))
    {
        ReportSVNError();
        return false;
    }

    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    if ((!m_bNoHooks)&&(CHooks::Instance().PostCommit(m_hWnd, m_selectedPaths, m_depth, m_RevisionEnd, m_sMessage, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(post_commit_hook, m_selectedPaths, error);
            return false;
        }
    }

    return true;
}

bool CSVNProgressDlg::CmdLock(CString& sWindowTitle, bool& /*localoperation*/)
{
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_LOCK);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_LOCK_BKG);
    ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_LOCK)));
    if (!Lock(m_targetPathList, (m_options & ProgOptForce) != 0, m_sMessage))
    {
        ReportSVNError();
        return false;
    }
    CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
    if (m_bLockWarning)
    {
        // the lock failed, because the file was outdated.
        // ask the user whether to update the file and try again
        CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_WARN_LOCKOUTDATED)),
                            CString(MAKEINTRESOURCE(IDS_WARN_LOCKOUTDATED_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_WARN_LOCKOUTDATED_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_WARN_LOCKOUTDATED_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        bool bDoIt = (taskdlg.DoModal(m_hWnd) == 1);

        if (bDoIt)
        {
            ReportString(CString(MAKEINTRESOURCE(IDS_SVNPROGRESS_UPDATEANDRETRY)), CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
            if (!Update(m_targetPathList, SVNRev::REV_HEAD, svn_depth_files, false, true, !!DWORD(CRegDWORD(L"Software\\TortoiseSVN\\AllowUnversionedObstruction", true)), true))
            {
                ReportSVNError();
                return false;
            }
            if (!Lock(m_targetPathList, (m_options & ProgOptForce) != 0, m_sMessage))
            {
                ReportSVNError();
                return false;
            }
        }
    }
    if (m_bLockExists)
    {
        // the locking failed because there already is a lock.
        // if the locking-dialog is skipped in the settings, tell the
        // user how to steal the lock anyway (i.e., how to get the lock
        // dialog back without changing the settings)
        if (!DWORD(CRegDWORD(L"Software\\TortoiseSVN\\ShowLockDlg", TRUE)))
        {
            ReportString(CString(MAKEINTRESOURCE(IDS_SVNPROGRESS_LOCKHINT)), CString(MAKEINTRESOURCE(IDS_WARN_NOTE)));
        }
        return false;
    }
    return true;
}

bool CSVNProgressDlg::CmdMerge(CString& sWindowTitle, bool& /*localoperation*/)
{
    bool bFailed = false;
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGE);
    SetBackgroundImage(IDI_MERGE_BKG);
    if (m_options & ProgOptDryRun)
    {
        sWindowTitle += L" " + sDryRun;
    }
    if (m_options & ProgOptRecordOnly)
    {
        sWindowTitle += L" " + sRecordOnly;
    }
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    GetDlgItem(IDC_INFOTEXT)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_SHOW);
    CRegDWORD nonint = CRegDWORD(L"Software\\TortoiseSVN\\MergeNonInteractive", FALSE);
    if (DWORD(nonint))
    {
        ::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
        m_AlwaysConflicted = true;
    }
    // we only accept a revision list to merge for peg merges
    ATLASSERT((m_revisionArray.GetCount()==0) || (m_revisionArray.GetCount() && (m_url.IsEquivalentTo(m_url2))));

    CBlockCacheForPath cacheBlock (m_targetPathList[0].GetWinPath());
    if (m_url.IsEquivalentTo(m_url2))
    {
        // Merging revisions %s of %s to %s into %s, %s%s
        CString sReportUrl = m_url.GetSVNPathString();
        if (m_pegRev.IsValid())
            sReportUrl = sReportUrl + L"@" + m_pegRev.ToString();
        CString sCmdInfo;
        sCmdInfo.FormatMessage(IDS_PROGRS_CMD_MERGEPEG,
            (LPCTSTR)m_revisionArray.ToListString(),
            (LPCTSTR)sReportUrl,
            m_targetPathList[0].GetWinPath(),
            (m_options & ProgOptIgnoreAncestry) ? (LPCTSTR)sIgnoreAncestry : (LPCTSTR)sRespectAncestry,
            (m_options & ProgOptDryRun) ? ((LPCTSTR)L", " + sDryRun) : L"",
            (m_options & ProgOptForce) ? ((LPCTSTR)L", " + sForce) : L"");
        ReportCmd(sCmdInfo);

        SVNRev firstRevOfRange;
        if (m_revisionArray.GetCount())
        {
            firstRevOfRange = m_revisionArray[0].GetStartRevision();
        }

        if (!PegMerge(m_url, m_revisionArray,
            m_pegRev.IsValid() ? m_pegRev : (m_url.IsUrl() ? firstRevOfRange : SVNRev(SVNRev::REV_WC)),
            m_targetPathList[0], !!(m_options & ProgOptForce), m_depth, m_diffoptions, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun), !!(m_options & ProgOptRecordOnly)))
        {
            if (m_ProgList.GetItemCount()>1)
            {
                ReportSVNError();
                bFailed = true;
            }
            // if the merge fails with the peg revision set,
            // try again with HEAD as the peg revision.
            else if (!PegMerge(m_url, m_revisionArray, SVNRev::REV_HEAD,
                m_targetPathList[0], !!(m_options & ProgOptForce), m_depth, m_diffoptions, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun), !!(m_options & ProgOptRecordOnly)))
            {
                ReportSVNError();
                bFailed = true;
            }
        }
        if (!bFailed)
        {
            GenerateMergeLogMessage();
        }
    }
    else
    {
        CString sCmdInfo;
        sCmdInfo.FormatMessage(IDS_PROGRS_CMD_MERGEURL,
            (LPCTSTR)m_url.GetSVNPathString(), (LPCTSTR)m_Revision.ToString(),
            (LPCTSTR)m_url2.GetSVNPathString(), (LPCTSTR)m_RevisionEnd.ToString(),
            m_targetPathList[0].GetWinPath(),
            (m_options & ProgOptIgnoreAncestry) ? (LPCTSTR)sIgnoreAncestry : (LPCTSTR)sRespectAncestry,
            (m_options & ProgOptDryRun) ? ((LPCTSTR)L", " + sDryRun) : L"",
            (m_options & ProgOptForce) ? ((LPCTSTR)L", " + sForce) : L"");
        ReportCmd(sCmdInfo);

        if (!Merge(m_url, m_Revision, m_url2, m_RevisionEnd, m_targetPathList[0],
            !!(m_options & ProgOptForce), m_depth, m_diffoptions, !!(m_options & ProgOptIgnoreAncestry), !!(m_options & ProgOptDryRun), !!(m_options & ProgOptRecordOnly)))
        {
            ReportSVNError();
            bFailed = true;
        }
    }
    GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_INFOTEXT)->ShowWindow(SW_SHOW);
    return !bFailed;
}

bool CSVNProgressDlg::CmdMergeAll(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGE);
    SetBackgroundImage(IDI_MERGE_BKG);
    if (m_options & ProgOptDryRun)
    {
        sWindowTitle += L" " + sDryRun;
    }
    if (m_options & ProgOptRecordOnly)
    {
        sWindowTitle += L" " + sRecordOnly;
    }
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    ATLASSERT(m_targetPathList.GetCount() == 1);

    CString sCmdInfo;
    sCmdInfo.LoadString(IDS_PROGRS_INFOGETTINGINFO);
    ReportCmd(sCmdInfo);
    CTSVNPathList suggestedSources;
    if (!SuggestMergeSources(m_targetPathList[0], m_Revision, suggestedSources))
    {
        ReportSVNError();
        return false;
    }

    if (suggestedSources.GetCount() == 0)
    {
        CString sErr;
        sErr.Format(IDS_PROGRS_MERGEALLNOSOURCES, m_targetPathList[0].GetWinPath());
        ReportError(sErr);
        return false;
    }
    sCmdInfo.FormatMessage(IDS_PROGRS_CMD_MERGEALL,
        (LPCTSTR)suggestedSources[0].GetSVNPathString(),
        m_targetPathList[0].GetWinPath(),
        (m_options & ProgOptIgnoreAncestry) ? (LPCTSTR)sIgnoreAncestry : (LPCTSTR)sRespectAncestry,
        (m_options & ProgOptForce) ? ((LPCTSTR)L", " + sForce) : L"");
    ReportCmd(sCmdInfo);

    GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_SHOW);
    CRegDWORD nonint = CRegDWORD(L"Software\\TortoiseSVN\\MergeNonInteractive", FALSE);
    if (DWORD(nonint))
    {
        ::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
        m_AlwaysConflicted = true;
    }

    SVNRevRangeArray revarray;
    CBlockCacheForPath cacheBlock (m_targetPathList[0].GetWinPath());
    if (!PegMerge(suggestedSources[0], revarray,
        SVNRev::REV_HEAD,
        m_targetPathList[0], !!(m_options & ProgOptForce), m_depth, m_diffoptions, !!(m_options & ProgOptIgnoreAncestry), FALSE, !!(m_options & ProgOptRecordOnly)))
    {
        GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
        ReportSVNError();
        return false;
    }

    GenerateMergeLogMessage();

    GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
    return true;
}

bool CSVNProgressDlg::CmdMergeReintegrate(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGEAUTOMATIC);
    SetBackgroundImage(IDI_MERGE_BKG);
    if (m_options & ProgOptDryRun)
    {
        sWindowTitle += L" " + sDryRun;
    }
    if (m_options & ProgOptRecordOnly)
    {
        sWindowTitle += L" " + sRecordOnly;
    }
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    CString sCmdInfo;
    sCmdInfo.FormatMessage(IDS_PROGRS_CMD_MERGEAUTOMATIC,
        (LPCTSTR)m_url.GetSVNPathString(),
        m_targetPathList[0].GetWinPath());
    ReportCmd(sCmdInfo);

    GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_SHOW);
    CRegDWORD nonint = CRegDWORD(L"Software\\TortoiseSVN\\MergeNonInteractive", FALSE);
    if (DWORD(nonint))
    {
        ::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
        m_AlwaysConflicted = true;
    }

    ASSERT(m_revisionArray.GetCount()==0);

    CBlockCacheForPath cacheBlock (m_targetPathList[0].GetWinPath());
    if (!PegMerge(m_url, m_revisionArray,
                  m_pegRev.IsValid() ? m_pegRev : (m_url.IsUrl() ? SVNRev() : SVNRev(SVNRev::REV_WC)),
                  m_targetPathList[0],
                  !!(m_options & ProgOptForce),
                  m_depth,
                  m_diffoptions,
                  !!(m_options & ProgOptIgnoreAncestry),
                  !!(m_options & ProgOptDryRun),
                  !!(m_options & ProgOptRecordOnly)))
    {
        ReportSVNError();
        GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
        return false;
    }

    GenerateMergeLogMessage();

    GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
    return true;
}

bool CSVNProgressDlg::CmdMergeReintegrateOldStyle(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_MERGEREINTEGRATE);
    SetBackgroundImage(IDI_MERGE_BKG);
    if (m_options & ProgOptDryRun)
    {
        sWindowTitle += L" " + sDryRun;
    }
    if (m_options & ProgOptRecordOnly)
    {
        sWindowTitle += L" " + sRecordOnly;
    }
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    CString sCmdInfo;
    sCmdInfo.FormatMessage(IDS_PROGRS_CMD_MERGEREINTEGRATE,
        (LPCTSTR)m_url.GetSVNPathString(),
        m_targetPathList[0].GetWinPath());
    ReportCmd(sCmdInfo);

    GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_SHOW);
    CRegDWORD nonint = CRegDWORD(L"Software\\TortoiseSVN\\MergeNonInteractive", FALSE);
    if (DWORD(nonint))
    {
        ::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
        m_AlwaysConflicted = true;
    }

    CBlockCacheForPath cacheBlock (m_targetPathList[0].GetWinPath());
    if (!MergeReintegrate(m_url, SVNRev::REV_HEAD, m_targetPathList[0], !!(m_options & ProgOptDryRun), m_diffoptions))
    {
        ReportSVNError();
        GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
        return false;
    }

    GenerateMergeLogMessage();

    GetDlgItem(IDC_NONINTERACTIVE)->ShowWindow(SW_HIDE);
    return true;
}

bool CSVNProgressDlg::CmdRename(CString& sWindowTitle, bool& localoperation)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    if ((!m_targetPathList[0].IsUrl())&&(!m_url.IsUrl()))
        localoperation = true;
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_RENAME);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_RENAME_BKG);
    ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_RENAME)));
    if (!Move(m_targetPathList, m_url, m_sMessage, false, false, !!(m_options & ProgOptForce), false, m_revProps))
    {
        ReportSVNError();
        return false;
    }
    CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
    return true;
}

bool CSVNProgressDlg::CmdResolve(CString& sWindowTitle, bool& localoperation)
{
    localoperation = true;
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_RESOLVE);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_RESOLVE_BKG);
    // check if the file may still have conflict markers in it.
    BOOL bMarkers = FALSE;
    if ((m_options & ProgOptSkipConflictCheck) == 0)
    {
        try
        {
            for (INT_PTR fileindex=0; (fileindex<m_targetPathList.GetCount()) && (bMarkers==FALSE); ++fileindex)
            {
                CTSVNPath targetPath = m_targetPathList[fileindex];
                if (targetPath.Exists() && !targetPath.IsDirectory())   // only check existing files
                {
                    if (targetPath.GetFileSize() < 100*1024)            // only check files smaller than 100kBytes
                    {
                        bool doCheck = true;
                        SVNProperties props(targetPath, SVNRev::REV_WC, false, false);
                        for (int i=0; i<props.GetCount(); i++)
                        {
                            if (props.GetItemName(i).compare(SVN_PROP_MIME_TYPE)==0)
                            {
                                CString mimetype = CUnicodeUtils::GetUnicode((char *)props.GetItemValue(i).c_str());
                                if ((!mimetype.IsEmpty())&&(mimetype.Left(4).CompareNoCase(L"text")))
                                {
                                    doCheck = false;    // do not check files with a non-text mimetype
                                    break;
                                }
                            }
                        }
                        if (doCheck)
                        {
                            CStdioFile file(targetPath.GetWinPath(), CFile::typeBinary | CFile::modeRead);
                            CString strLine = L"";
                            while (file.ReadString(strLine))
                            {
                                if (strLine.Find(L"<<<<<<<")==0)
                                {
                                    bMarkers = TRUE;
                                    break;
                                }
                            }
                            file.Close();
                        }
                    }
                }
            }
        }
        catch (CFileException* pE)
        {
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException in Resolve!\n");
            pE->Delete();
        }
    }

    UINT showRet = IDYES;   // default to yes
    if (bMarkers)
    {
        CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_PROGRS_REVERTMARKERS)),
                            CString(MAKEINTRESOURCE(IDS_PROGRS_REVERTMARKERS_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(IDYES, CString(MAKEINTRESOURCE(IDS_PROGRS_REVERTMARKERS_TASK3)));
        taskdlg.AddCommandControl(IDNO, CString(MAKEINTRESOURCE(IDS_PROGRS_REVERTMARKERS_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(IDNO);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        showRet = (UINT)taskdlg.DoModal(m_hWnd);
    }
    if (showRet == IDYES)
    {
        ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_RESOLVE)));
        for (INT_PTR fileindex=0; fileindex<m_targetPathList.GetCount(); ++fileindex)
        {
            if (!Resolve(m_targetPathList[fileindex], svn_wc_conflict_choose_merged, true, false, svn_wc_conflict_kind_text))
            {
                ReportSVNError();
            }
        }
    }
    CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
    return true;
}

bool CSVNProgressDlg::CmdRevert(CString& sWindowTitle, bool& localoperation)
{
    localoperation = true;
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_REVERT);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_REVERT_BKG);

    CTSVNPathList delList = m_selectedPaths;
    if (DWORD(CRegDWORD(L"Software\\TortoiseSVN\\RevertWithRecycleBin", TRUE)))
    {
        CRecycleBinDlg rec;
        rec.StartTime();
        int count = delList.GetCount();
        delList.DeleteAllPaths(true, true, NULL);
        rec.EndTime(count);
    }

    ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_REVERT)));
    CBlockCacheForPath cacheBlock (m_targetPathList.GetCommonRoot().GetWinPath());
    if (!Revert(m_targetPathList, CStringArray(), (m_options & ProgOptRecursive) != 0, (m_options & ProgOptClearChangeLists) != 0, false))
    {
        ReportSVNError();
        return false;
    }
    CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
    return true;
}

bool CSVNProgressDlg::CmdSwitch(CString& sWindowTitle, bool& /*localoperation*/)
{
    ASSERT(m_targetPathList.GetCount() == 1);
    SVNStatus st;
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_SWITCH);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_SWITCH_BKG);
    LONG rev = 0;
    if (st.GetStatus(m_targetPathList[0]) != (-2))
    {
        rev = st.status->changed_rev;
    }

    CString sCmdInfo;
    sCmdInfo.FormatMessage(IDS_PROGRS_CMD_SWITCH,
        m_targetPathList[0].GetWinPath(), (LPCTSTR)m_url.GetSVNPathString(),
        (LPCTSTR)m_Revision.ToString());
    ReportCmd(sCmdInfo);

    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    if ((!m_bNoHooks)&&(CHooks::Instance().PreUpdate(m_hWnd, m_targetPathList, m_depth, m_Revision, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(pre_update_hook, m_targetPathList, error);
            return false;
        }
    }

    CBlockCacheForPath cacheBlock (m_targetPathList[0].GetWinPath());
    if (!Switch(m_targetPathList[0], m_url, m_Revision, m_Revision, m_depth, (m_options & ProgOptStickyDepth) != 0, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(L"Software\\TortoiseSVN\\AllowUnversionedObstruction", true)), (m_options & ProgOptIgnoreAncestry) != 0))
    {
        ReportSVNError();
        return false;
    }

    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    CTSVNPathList updatedList = GetPathsForUpdateHook(m_targetPathList);
    if ((!m_bNoHooks)&&(CHooks::Instance().PostUpdate(m_hWnd, m_targetPathList, m_depth, m_RevisionEnd, updatedList, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(post_update_hook, m_targetPathList, error);
            return false;
        }
    }

    m_UpdateStartRevMap[m_targetPathList[0].GetSVNApiPath(pool)] = rev;
    if ((m_RevisionEnd >= 0)&&(rev >= 0)
        &&((LONG)m_RevisionEnd > (LONG)rev))
    {
        GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);
    }
    return true;
}


bool CSVNProgressDlg::CmdSwitchBackToParent( CString& sWindowTitle, bool& /*localoperation*/ )
{
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_SWITCH);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_SWITCH_BKG);

    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    if ((!m_bNoHooks)&&(CHooks::Instance().PreUpdate(m_hWnd, m_targetPathList, m_depth, m_Revision, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(pre_update_hook, m_targetPathList, error);
            return false;
        }
    }

    CBlockCacheForPath cacheBlock (m_targetPathList.GetCommonRoot().GetWinPath());
    for (int i = 0; i < m_targetPathList.GetCount(); ++i)
    {
        SVNStatus st;
        CTSVNPath retPath;
        svn_client_status_t * s = st.GetFirstFileStatus(m_targetPathList[i].GetContainingDirectory(), retPath, false, svn_depth_empty);
        if (s != NULL)
        {
            CTSVNPath switchUrl;
            switchUrl.SetFromSVN(s->repos_root_url);
            switchUrl.AppendPathString(CUnicodeUtils::GetUnicode(s->repos_relpath));
            switchUrl.AppendPathString(m_targetPathList[i].GetFileOrDirectoryName());
            CString sCmdInfo;
            sCmdInfo.FormatMessage(IDS_PROGRS_CMD_SWITCH,
                m_targetPathList[i].GetWinPath(), (LPCTSTR)switchUrl.GetSVNPathString(),
                (LPCTSTR)m_Revision.ToString());
            ReportCmd(sCmdInfo);

            if (!Switch(m_targetPathList[i], switchUrl, s->revision, s->revision, svn_depth_unknown, false, true, true, true))
            {
                ReportSVNError();
                return false;
            }
        }
        else
        {
            CString temp;
            temp.Format(IDS_ERR_NOPARENTFOUND, m_targetPathList[i].GetWinPath());
            ReportError(temp);
            return false;
        }
    }

    CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
    CTSVNPathList updatedList = GetPathsForUpdateHook(m_targetPathList);
    if ((!m_bNoHooks)&&(CHooks::Instance().PostUpdate(m_hWnd, m_targetPathList, m_depth, m_RevisionEnd, updatedList, exitcode, error)))
    {
        if (exitcode)
        {
            ReportHookFailed(post_update_hook, m_targetPathList, error);
            return false;
        }
    }
    return true;
}

bool CSVNProgressDlg::CmdUnlock(CString& sWindowTitle, bool& /*localoperation*/)
{
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_UNLOCK);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_UNLOCK_BKG);
    ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_UNLOCK)));
    if (!Unlock(m_targetPathList, (m_options & ProgOptForce) != 0))
    {
        ReportSVNError();
        return false;
    }
    CShellUpdater::Instance().AddPathsForUpdate(m_targetPathList);
    return true;
}

bool CSVNProgressDlg::CmdUpdate(CString& sWindowTitle, bool& /*localoperation*/)
{
    sWindowTitle.LoadString(IDS_PROGRS_TITLE_UPDATE);
    CAppUtils::SetWindowTitle(m_hWnd, m_targetPathList.GetCommonRoot().GetUIPathString(), sWindowTitle);
    SetBackgroundImage(IDI_UPDATE_BKG);

    // when updating multiple paths, we try to update all of them to the
    // same revision. If updating to HEAD there's a race condition:
    // if between updating the multiple items a commit happens,
    // then subsequent updates of the remaining items would update
    // to that new HEAD revision.
    // To avoid that, we first fetch the HEAD revision of the first target,
    // then go through the other targets to find out whether they're
    // from the same repository as the first one. If all targets
    // are from the same repository, then we use that HEAD revision
    // we fetched and update to that specific revision (the number)
    // instead of the (moving) "HEAD".
    // If the targets are not all from the same repository, then
    // we abort the scan and just update to the user specified
    // revision.
    //
    // We also need the 'changed_rev' of every target before the update
    // so we can do diffs and show logs after the update has finished.
    int targetcount = m_targetPathList.GetCount();
    std::set<CTSVNPath> wcroots;
    if ((m_options & ProgOptSkipPreChecks) == 0)
    {
        bool multipleUUIDs = false;
        CString lastUUID;
        CTSVNPath lastDir;
        for(int nItem = 0; (nItem < targetcount); nItem++)
        {
            const CTSVNPath& targetPath = m_targetPathList[nItem];
            SVNStatus st;
            if (st.GetStatus(targetPath, false) != (-2))
            {
                m_UpdateStartRevMap[targetPath.GetSVNApiPath(pool)] = st.status->changed_rev;
                // find out if this target is from the same repository as
                // the ones before
                CString uuid = CString(st.status->repos_uuid ? st.status->repos_uuid : "");
                if (!uuid.IsEmpty())
                {
                    if (lastUUID.IsEmpty())
                        lastUUID = uuid;
                    if (lastUUID.Compare(uuid) != 0)
                        multipleUUIDs = true;
                }
                if (!lastDir.IsEquivalentToWithoutCase(targetPath))
                {
                    CTSVNPath sRoot = GetWCRootFromPath(targetPath);
                    wcroots.insert(sRoot);
                }
                lastDir = targetPath.GetDirectory();
            }
        }
        // if all targets are from the same repository and we're updating to HEAD,
        // find the HEAD revision number and update specifically to that.
        if (m_Revision.IsHead() && !multipleUUIDs)
        {
            m_Revision = GetHEADRevision(m_targetPathList[0]);
        }
    }
    if (wcroots.size() <= 1)
    {
        DWORD exitcode = 0;
        CString error;
        CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
        if ((!m_bNoHooks)&&(CHooks::Instance().PreUpdate(m_hWnd, m_targetPathList, m_depth, m_Revision, exitcode, error)))
        {
            if (exitcode)
            {
                ReportHookFailed(pre_update_hook, m_targetPathList, error);
                return false;
            }
        }
    }
    else
    {
        for (auto it:wcroots)
        {
            DWORD exitcode = 0;
            CString error;
            ProjectProperties pp;
            pp.ReadProps(it);
            CHooks::Instance().SetProjectProperties(it, pp);
            CTSVNPathList pl(it);
            if ((!m_bNoHooks)&&(CHooks::Instance().PreUpdate(m_hWnd, pl, m_depth, m_Revision, exitcode, error)))
            {
                if (exitcode)
                {
                    ReportHookFailed(pre_update_hook, pl, error);
                    return false;
                }
            }
        }
    }
    ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_UPDATE)));
    CBlockCacheForPath cacheBlock (m_targetPathList[0].GetWinPath());
    if (!Update(m_targetPathList, m_Revision, m_depth, (m_options & ProgOptStickyDepth) != 0, (m_options & ProgOptIgnoreExternals) != 0, !!DWORD(CRegDWORD(L"Software\\TortoiseSVN\\AllowUnversionedObstruction", true)), true))
    {
        ReportSVNError();
        return false;
    }
    if (wcroots.size() <= 1)
    {
        DWORD exitcode = 0;
        CString error;
        CHooks::Instance().SetProjectProperties(m_targetPathList.GetCommonRoot(), m_ProjectProperties);
        if (!m_bNoHooks)
        {
            CTSVNPathList updatedList = GetPathsForUpdateHook(m_targetPathList);
            if (CHooks::Instance().PostUpdate(m_hWnd, m_targetPathList, m_depth, m_RevisionEnd, updatedList, exitcode, error))
            {
                if (exitcode)
                {
                    ReportHookFailed(post_update_hook, m_targetPathList, error);
                    return false;
                }
            }
        }
    }
    else
    {
        if (!m_bNoHooks)
        {
            for (auto it:wcroots)
            {
                DWORD exitcode = 0;
                CString error;
                ProjectProperties pp;
                pp.ReadProps(it);
                CHooks::Instance().SetProjectProperties(it, pp);
                CTSVNPathList pl(it);
                CTSVNPathList updatedList = GetPathsForUpdateHook(pl);
                if (CHooks::Instance().PostUpdate(m_hWnd, pl, m_depth, m_RevisionEnd, updatedList, exitcode, error))
                {
                    if (exitcode)
                    {
                        ReportHookFailed(post_update_hook, pl, error);
                        return false;
                    }
                }
            }
        }
    }

    // after an update, show the user the log button, but only if only one single item was updated
    // (either a file or a directory)
    if ((m_targetPathList.GetCount() == 1) && (!m_UpdateStartRevMap.empty()))
        GetDlgItem(IDC_LOGBUTTON)->ShowWindow(SW_SHOW);

    return true;
}

void CSVNProgressDlg::OnBnClickedNoninteractive()
{
    LRESULT res = ::SendMessage(GetDlgItem(IDC_NONINTERACTIVE)->GetSafeHwnd(), BM_GETCHECK, 0, 0);
    m_AlwaysConflicted = (res == BST_CHECKED);
    CRegDWORD nonint(L"Software\\TortoiseSVN\\MergeNonInteractive", FALSE);
    nonint = m_AlwaysConflicted;
}

CString CSVNProgressDlg::GetPathFromColumnText(const CString& sColumnText)
{
    // First check if the text in the column actually *is* already
    // a valid path
    if (PathFileExists(sColumnText))
    {
        return sColumnText;
    }
    CString sPath = CPathUtils::ParsePathInString(sColumnText);
    if (sPath.Find(':')<0)
    {
        // the path is not absolute: add the common root of all paths to it
        sPath = m_targetPathList.GetCommonRoot().GetDirectory().GetWinPathString() + L"\\" + CPathUtils::ParsePathInString(sColumnText);
    }
    return sPath;
}

LRESULT CSVNProgressDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_pTaskbarList.Release();
    if (FAILED(m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
        m_pTaskbarList = nullptr;
    SetUUIDOverlayIcon(m_hWnd);
    return 0;
}

bool CSVNProgressDlg::IsCommittingToTag(CString& url)
{
    bool isTag = false;
    CRegString regTagsPattern (L"Software\\TortoiseSVN\\RevisionGraph\\TagsPattern", L"tags");
    for (int i=0; (i<m_targetPathList.GetCount()) && (!isTag); ++i)
    {
        url = GetURLFromPath(m_targetPathList[i]);
        if (url.IsEmpty())
            continue;
        CString urllower = url;
        urllower.MakeLower();
        // test if the commit goes to a tag.
        // now since Subversion doesn't force users to
        // create tags in the recommended /tags/ folder
        // only a warning is shown. This won't work if the tags
        // are stored in a non-recommended place, but the check
        // still helps those who do.
        CString sTags = regTagsPattern;
        int pos = 0;
        CString temp;
        while (!isTag)
        {
            temp = sTags.Tokenize(L";", pos);
            if (temp.IsEmpty())
                break;

            int urlpos = 0;
            CString temp2;
            for(;;)
            {
                temp2 = urllower.Tokenize(L"/", urlpos);
                if (temp2.IsEmpty())
                    break;

                if (CStringUtils::WildCardMatch(temp, temp2))
                {
                    isTag = true;
                    break;
                }
            }
        }
    }
    return isTag;
}

bool CSVNProgressDlg::CheckUpdateAndRetry()
{
    if (m_bRetryDone)
        return false;

    if (GetSVNError() && GetSVNError()->apr_err == SVN_ERR_CLIENT_MERGE_UPDATE_REQUIRED)
    {
        if (CAppUtils::AskToUpdate(m_hWnd, GetLastErrorMessage(40)))
        {
            // run an update
            CSVNProgressDlg updateProgDlg;
            updateProgDlg.SetPathList(m_targetPathList);
            updateProgDlg.SetDepth(svn_depth_unknown);
            updateProgDlg.SetCommand (CSVNProgressDlg::SVNProgress_Update);
            updateProgDlg.DoModal();
            if (!updateProgDlg.DidErrorsOccur() && !updateProgDlg.DidConflictsOccur())
            {
                // now retry the failed operation
                ResetVars();

                m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
                if (m_pThread==NULL)
                {
                    ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
                    return false;
                }
                else
                {
                    m_pThread->m_bAutoDelete = FALSE;
                    m_pThread->ResumeThread();
                    m_bRetryDone = true;
                    return true;
                }
            }
        }
    }
    return false;
}

void CSVNProgressDlg::ResetVars()
{
    m_Revision = SVNRev(L"HEAD");
    m_RevisionEnd = 0;
    m_bLockWarning = false;
    m_bLockExists = false;
    m_bCancelled = FALSE;
    m_nConflicts = 0;
    m_nTotalConflicts = 0;
    m_bConflictWarningShown = false;
    m_bWarningShown = false;
    m_bErrorsOccurred = FALSE;
    m_bMergesAddsDeletesOccurred = FALSE;
    m_bFinishedItemAdded = false;
    m_bLastVisible = false;
    m_itemCount = -1;
    m_itemCountTotal = -1;
    m_AlwaysConflicted = false;
    m_BugTraqProvider = NULL;
    m_bHookError = false;
    m_bHooksAreOptional = true;
    m_bExternalStartInfoShown = false;

    m_ProgList.SetRedraw(FALSE);
    m_ProgList.DeleteAllItems();
    m_ProgList.SetItemCountEx (0);

    for (auto data : m_arData)
    {
        delete data;
    }
    m_arData.clear();

    m_ProgList.SetRedraw(TRUE);

    m_bNoHooks = true;

    SetTimer(VISIBLETIMER, 300, NULL);
    SetDlgItemText(IDC_INFOTEXT, L"");
}

void CSVNProgressDlg::MergeAfterCommit()
{
    CString url = GetURLFromPath(m_origPathList[0]);
    if (url.IsEmpty())
        return;

    CString path = m_origPathList[0].GetWinPathString();
    bool bGotSavePath = false;
    if (!m_origPathList[0].IsDirectory())
    {
        bGotSavePath = CAppUtils::FileOpenSave(path, NULL, IDS_LOG_MERGETO, IDS_COMMONFILEFILTER, true, CString(), GetSafeHwnd());
    }
    else
    {
        CBrowseFolder folderBrowser;
        folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_MERGETO)));
        bGotSavePath = (folderBrowser.Show(GetSafeHwnd(), path, path) == CBrowseFolder::OK);
    }
    if (bGotSavePath)
    {
        svn_revnum_t    minrev;
        svn_revnum_t    maxrev;
        bool            bswitched;
        bool            bmodified;
        bool            bSparse;

        if (GetWCRevisionStatus(CTSVNPath(path), true, minrev, maxrev, bswitched, bmodified, bSparse))
        {
            if (bmodified)
            {
                CString sTask1;
                sTask1.Format(IDS_MERGE_WCDIRTYASK_TASK1, (LPCTSTR)path);
                CTaskDialog taskdlg(sTask1,
                                    CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
                taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK3)));
                taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK4)));
                taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskdlg.SetDefaultCommandControl(2);
                taskdlg.SetMainIcon(TD_WARNING_ICON);
                if (taskdlg.DoModal(m_hWnd) != 1)
                    return;
            }
        }
        CSVNProgressDlg dlg(this);
        dlg.SetOptions(ProgOptForce|ProgOptAllowMixedRev);
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetPathList(CTSVNPathList(CTSVNPath(path)));
        dlg.SetUrl(url);
        dlg.SetSecondUrl(url);
        dlg.SetRevision(m_RevisionEnd);
        SVNRevRangeArray tempRevArray;
        tempRevArray.AddRevRange((svn_revnum_t)m_RevisionEnd-1, m_RevisionEnd);
        dlg.SetRevisionRanges(tempRevArray);
        dlg.SetPegRevision(m_RevisionEnd);
        dlg.SetDiffOptions(SVN::GetOptionsString(true, svn_diff_file_ignore_space_all));
        dlg.DoModal();
    }
}

void CSVNProgressDlg::GenerateMergeLogMessage()
{
    CString sUUID = GetUUIDFromPath(m_targetPathList[0]);
    CString sRepositoryRoot = GetRepositoryRoot(m_targetPathList[0]);
    CString escapedUrl = CUnicodeUtils::GetUnicode(m_url.GetSVNApiPath(pool));
    CString relUrl = escapedUrl.Mid(sRepositoryRoot.GetLength() + 1);
    relUrl = CPathUtils::PathUnescape(relUrl);
    CString sSeparator = CRegString(L"Software\\TortoiseSVN\\MergeLogSeparator", L"........");
    CString sRevListRange;
    CString sRevList;
    CString sRevListR;
    CString sLogMessages;

    CString sFormatTitle = CString(MAKEINTRESOURCE(IDS_SVNPROGRESS_MERGELOGRANGE));
    if (!m_ProjectProperties.sMergeLogTemplateTitle.IsEmpty())
        sFormatTitle = m_ProjectProperties.sMergeLogTemplateTitle;
    CString sFormatMsg = L"{msg}\n" + sSeparator + L"\n";
    if (!m_ProjectProperties.sMergeLogTemplateMsg.IsEmpty())
        sFormatMsg = m_ProjectProperties.sMergeLogTemplateMsg;

    try
    {
        CCachedLogInfo* cache = GetLogCachePool()->GetCache(sUUID, sRepositoryRoot);
        const CPathDictionary* paths = &cache->GetLogInfo().GetPaths();
        CDictionaryBasedTempPath logPath(paths, (const char*)CUnicodeUtils::GetUTF8(relUrl));
        CLogCacheUtility logUtil(cache, &m_ProjectProperties);
        for (int ranges = 0; ranges < m_mergedRevisions.GetCount(); ++ranges)
        {
            SVNRevRange range = m_mergedRevisions[ranges];
            svn_revnum_t startRev = range.GetStartRevision();
            svn_revnum_t endRev = range.GetEndRevision();
            bool bReverse = startRev > endRev;
            if (bReverse)
            {
                sFormatTitle = CString(MAKEINTRESOURCE(IDS_SVNPROGRESS_MERGELOGRANGEREVERSE));
                if (!m_ProjectProperties.sMergeLogTemplateReverseTitle.IsEmpty())
                    sFormatTitle = m_ProjectProperties.sMergeLogTemplateReverseTitle;
            }
            if (!sRevList.IsEmpty())
                sRevList += L", ";
            if (!sRevListR.IsEmpty())
                sRevListR += L", ";
            if (!sRevListRange.IsEmpty())
                sRevListRange += L", ";
            if (startRev == endRev)
            {
                sRevList += SVNRev(startRev).ToString();
                sRevListR += L"r" + SVNRev(startRev).ToString();
                sRevListRange += SVNRev(startRev).ToString();
            }
            else
            {
                sRevListRange += SVNRev(startRev).ToString() + L"-" + SVNRev(endRev).ToString();
                for (svn_revnum_t r = startRev; r <= endRev; startRev < endRev ? ++r : --r)
                {
                    if (!sRevList.IsEmpty())
                        sRevList += L", ";
                    if (!sRevListR.IsEmpty())
                        sRevListR += L", ";
                    sRevListR += L"r" + SVNRev(r).ToString();
                }
            }
            if (bReverse)
            {
                svn_revnum_t r = startRev;
                startRev = endRev;
                endRev = r;
            }
            for (svn_revnum_t rev = startRev; rev <= endRev; ++rev)
            {
                if (logUtil.IsCached(rev))
                {
                    auto pLogItem = logUtil.GetRevisionData(rev);
                    if (pLogItem)
                    {
                        pLogItem->Finalize(cache, logPath);
                        if (IsRevisionRelatedToMerge(logPath, pLogItem.get()))
                        {
                            CString sFormattedMsg = sFormatMsg;
                            CString sMsg = CUnicodeUtils::StdGetUnicode(pLogItem->GetMessage()).c_str();
                            sFormattedMsg.Replace(L"{msg}", sMsg);
                            sMsg.Replace(L"\r\n", L" ");
                            sMsg.Replace('\n', ' ');
                            sFormattedMsg.Replace(L"{msgoneline}", sMsg);
                            sFormattedMsg.Replace(L"{author}", CUnicodeUtils::StdGetUnicode(pLogItem->GetAuthor()).c_str());
                            sFormattedMsg.Replace(L"{rev}", SVNRev(pLogItem->GetRevision()).ToString());
                            sFormattedMsg.Replace(L"{bugids}", CUnicodeUtils::StdGetUnicode(pLogItem->GetBugIDs()).c_str());
                            sLogMessages += sFormattedMsg;
                        }
                    }
                }
            }
        }
    }
    catch (CException* e)
    {
        e->Delete();
    }

    CString sSuggestedMessage = sFormatTitle;
    sSuggestedMessage.Replace(L"{revisions}", sRevList);
    sSuggestedMessage.Replace(L"{revisionsr}", sRevListR);
    sSuggestedMessage.Replace(L"{revrange}", sRevListRange);
    sSuggestedMessage.Replace(L"{mergeurl}", relUrl);
    if (m_ProjectProperties.bMergeLogTemplateMsgTitleBottom)
        sSuggestedMessage = sLogMessages + sSuggestedMessage;
    else
        sSuggestedMessage += sLogMessages;

    CRegHistory history;
    history.SetMaxHistoryItems((LONG)CRegDWORD(L"Software\\TortoiseSVN\\MaxHistoryItems", 25));
    CString reg;
    reg.Format(L"Software\\TortoiseSVN\\History\\commit%s", (LPCWSTR)GetUUIDFromPath(m_targetPathList[0]));
    history.Load(reg, L"logmsgs");
    history.AddEntry(sSuggestedMessage);
    history.Save();
}

void CSVNProgressDlg::CompareWithWC( NotificationData * data )
{
    if (data == nullptr)
        return;

    svn_revnum_t rev = -1;
    StringRevMap::iterator it;

    if (data->basepath.IsEmpty())
        it = m_UpdateStartRevMap.begin();
    else
        it = m_UpdateStartRevMap.find(data->basepath.GetSVNApiPath(pool));
    if (it != m_UpdateStartRevMap.end())
        rev = it->second;
    else
    {
        it = m_FinishedRevMap.find(data->basepath.GetSVNApiPath(pool));
        if (it != m_FinishedRevMap.end())
            rev = it->second;
    }
    // if the file was merged during update, do a three way diff between OLD, MINE, THEIRS
    if (data->content_state == svn_wc_notify_state_merged)
    {
        // BASE  : the file before the update
        // THEIRS: the file in HEAD
        // MINE  : the local file, probably with local modifications
        CTSVNPath theirfile = CTempFiles::Instance().GetTempFilePath(false, data->path, SVNRev::REV_HEAD);
        SVN svn;
        if (!svn.Export(data->path, theirfile, SVNRev(SVNRev::REV_WC), SVNRev(SVNRev::REV_HEAD)))
        {
            svn.ShowErrorDialog(m_hWnd, data->path);
            DialogEnableWindow(IDOK, TRUE);
            return;
        }
        SetFileAttributes(theirfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
        CTSVNPath basefile = CTempFiles::Instance().GetTempFilePath(false, data->path, rev);
        if (!svn.Export(data->path, basefile, SVNRev(SVNRev::REV_BASE), rev))
        {
            svn.ShowErrorDialog(m_hWnd, data->path);
            DialogEnableWindow(IDOK, TRUE);
            return;
        }
        SetFileAttributes(basefile.GetWinPath(), FILE_ATTRIBUTE_READONLY);

        CString revname, wcname, basename;
        revname.Format(L"%s Revision %ld", (LPCTSTR)data->path.GetUIFileOrDirectoryName(), rev);
        wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
        basename.Format(IDS_DIFF_BASENAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
        CAppUtils::MergeFlags flags;
        flags.bAlternativeTool = (GetKeyState(VK_SHIFT)&0x8000) != 0;
        flags.bReadOnly = true;
        CAppUtils::StartExtMerge(flags, basefile, theirfile, data->path, data->path, false, basename, revname, wcname, CString(), data->path.GetFileOrDirectoryName());
    }
    else
    {
        CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, data->path, rev);
        SVN svn;
        if (!svn.Export(data->path, tempfile, SVNRev(SVNRev::REV_WC), rev))
        {
            svn.ShowErrorDialog(m_hWnd, data->path);
            DialogEnableWindow(IDOK, TRUE);
            return;
        }
        SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
        CString revname, wcname;
        revname.Format(L"%s Revision %ld", (LPCTSTR)data->path.GetUIFileOrDirectoryName(), rev);
        wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)data->path.GetUIFileOrDirectoryName());
        CAppUtils::StartExtDiff(
            tempfile, data->path, revname, wcname, data->url, data->url, rev, SVNRev::REV_WC, SVNRev::REV_WC,
            CAppUtils::DiffFlags().AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000)), 0,
            data->path.GetUIFileOrDirectoryName(), L"");
    }
}

LRESULT CSVNProgressDlg::OnCheck( WPARAM wnd, LPARAM )
{
    HWND hwnd = (HWND)wnd;
    if (hwnd == GetDlgItem(IDC_JUMPCONFLICT)->GetSafeHwnd())
    {
        int selIndex = m_ProgList.GetSelectionMark();
        if (selIndex < 0)
            selIndex = 0;
        ++selIndex;
        bool bNextFound = false;
        for (int i = selIndex; i < (int)m_arData.size(); ++i)
        {
            NotificationData * data = m_arData[i];
            if (data->bConflictedActionItem)
            {
                m_ProgList.SetItemState(-1, 0, LVIS_SELECTED);
                m_ProgList.SetItemState(i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
                m_ProgList.EnsureVisible(i, FALSE);
                m_ProgList.SetFocus();
                m_ProgList.SetSelectionMark(i);
                bNextFound = true;
                break;
            }
        }
        if (!bNextFound)
        {
            // start over at the beginning
            for (int i = 0; i < selIndex; ++i)
            {
                NotificationData * data = m_arData[i];
                if (data->bConflictedActionItem)
                {
                    m_ProgList.SetItemState(-1, 0, LVIS_SELECTED);
                    m_ProgList.SetItemState(i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
                    m_ProgList.EnsureVisible(i, FALSE);
                    m_ProgList.SetFocus();
                    m_ProgList.SetSelectionMark(i);
                    break;
                }
            }
        }
    }
    return 0;
}

CTSVNPathList CSVNProgressDlg::GetPathsForUpdateHook( const CTSVNPathList& pathList )
{
    CTSVNPathList updatedList;
    if (!m_bNoHooks)
    {
        if (CHooks::Instance().IsHookPresent(post_update_hook, pathList))
        {
            for (const auto& n : m_arData)
            {
                if (!n->path.IsEmpty())
                    updatedList.AddPath(n->path);
            }
            updatedList.RemoveDuplicates();
        }
    }
    return updatedList;
}

bool CSVNProgressDlg::IsRevisionRelatedToMerge(const CDictionaryBasedTempPath& basePath, PLOGENTRYDATA pLogItem)
{
  const auto& changedPathes = pLogItem->GetChangedPaths();
  for (size_t i = 0; i < changedPathes.GetCount(); ++i)
  {
    if (basePath.IsSameOrParentOf(changedPathes[i].GetCachedPath()))
      return true;
  }
  return false;
}

