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
#include "CommitDlg.h"
#include "DirFileEnum.h"
#include "SVNConfig.h"
#include "SVNProperties.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "SVN.h"
#include "registry.h"
#include "SVNStatus.h"
#include "HistoryDlg.h"
#include "Hooks.h"
#include "COMError.h"
#include "../version.h"
#include "BstrSafeVector.h"
#include "SmartHandle.h"
#include "Hooks.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT CCommitDlg::WM_AUTOLISTREADY = RegisterWindowMessage(L"TORTOISESVN_AUTOLISTREADY_MSG");

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
    , m_nPopupPasteListCmd(0)
    , m_bCancelled(FALSE)
    , m_bUnchecked(false)
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
    ON_BN_CLICKED(IDC_LOG, &CCommitDlg::OnBnClickedLog)
    ON_WM_QUERYENDSESSION()
    ON_BN_CLICKED(IDC_RUNHOOK, &CCommitDlg::OnBnClickedRunhook)
END_MESSAGE_MAP()


BOOL CCommitDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    ReadPersistedDialogSettings();
    ExtendFrameIntoClientArea(IDC_DWM);
    SubclassControls();
    UpdateData(FALSE);
    InitializeListControl();
    InitializeLogMessageControl();
    SetControlAccessibilityProperties();
    SetupToolTips();
    HideAndDisableBugTraqButton();
    SetupBugTraqControlsIfConfigured();
    ShowOrHideBugIdAndLabelControls();
    VersionCheck();
    SetupLogMessageDefaultText();
    SetCommitWindowTitleAndEnableStatus();
    AdjustControlSizes();
    ConvertStaticToLinkControl();
    LineupControlsAndAdjustSizes();
    SaveDialogAndLogMessageControlRectangles();
    AddAnchorsToFacilitateResizing();
    CenterWindowWhenLaunchedFromExplorer();
    AdjustDialogSizeAndPanes();
    AddDirectoriesToPathWatcher();
    GetAsyncFileListStatus();
    ShowBalloonInCaseOfError();
    GetDlgItem(IDC_RUNHOOK)->ShowWindow(CHooks::Instance().IsHookPresent(manual_precommit, m_pathList) ? SW_SHOW : SW_HIDE);
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
    id.Trim(L"\n\r");
    if (!m_ProjectProperties.CheckBugID(id))
    {
        ShowEditBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, TTI_ERROR);
        return;
    }
    m_sLogMessage = m_cLogMessage.GetText();
    if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
    {
        CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK1)),
                            CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        if (taskdlg.DoModal(m_hWnd) != 1)
            return;
    }

    CRegDWORD regUnversionedRecurse (L"Software\\TortoiseSVN\\UnversionedRecurse", TRUE);
    if (!(DWORD)regUnversionedRecurse)
    {
        // Find unversioned directories which are marked for commit. The user might expect them
        // to be added recursively since he cannot see the files. Let's ask the user if he knows
        // what he is doing.
        int nListItems = m_ListCtrl.GetItemCount();
        bool bCheckUnversioned = true;
        for (int j=0; j<nListItems; j++)
        {
            const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetConstListEntry(j);
            if (bCheckUnversioned && entry->IsChecked() && (entry->status == svn_wc_status_unversioned) && entry->IsFolder() )
            {
                CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK1)),
                                    CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK3)));
                taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK4)));
                taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskdlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNUNVERSIONEDFOLDER_TASK5)));
                taskdlg.SetVerificationCheckbox(false);
                taskdlg.SetDefaultCommandControl(2);
                taskdlg.SetMainIcon(TD_WARNING_ICON);
                if (taskdlg.DoModal(m_hWnd) != 1)
                    return;
                if (taskdlg.GetVerificationCheckboxState())
                    bCheckUnversioned = false;
            }
        }
    }

    m_pathwatcher.Stop();
    InterlockedExchange(&m_bBlock, TRUE);
    CDWordArray arDeleted;
    //first add all the unversioned files the user selected
    //and check if all versioned files are selected
    int nUnchecked = 0;
    m_bUnchecked = false;
    m_bRecursive = true;
    int nListItems = m_ListCtrl.GetItemCount();

    CTSVNPathList itemsToAdd;
    CTSVNPathList itemsToRemove;
    CTSVNPathList itemsToUnlock;

    bool bCheckedInExternal = false;
    bool bHasConflicted = false;
    bool bHasDirCopyPlus = false;
    std::set<CString> checkedLists;
    std::set<CString> uncheckedLists;
    m_restorepaths.clear();
    m_restorepaths = m_ListCtrl.GetRestorePaths();
    bool bAllowPeggedExternals = !DWORD(CRegDWORD(L"Software\\TortoiseSVN\\BlockPeggedExternals", TRUE));
    for (int j=0; j<nListItems; j++)
    {
        const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetConstListEntry(j);
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
            if (entry->status == svn_wc_status_missing)
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
            if (entry->IsPeggedExternal())
            {
                bCheckedInExternal = true;
            }
            if (entry->IsLocked() && entry->status == svn_wc_status_normal)
            {
                itemsToUnlock.AddPath (entry->GetPath());
            }
            if (entry->IsCopied() && entry->IsFolder())
            {
                bHasDirCopyPlus = true;
            }
            checkedLists.insert(entry->GetChangeList());
        }
        else
        {
            if ((entry->status != svn_wc_status_unversioned)    &&
                (entry->status != svn_wc_status_ignored) &&
                (!entry->IsFromDifferentRepository()))
            {
                nUnchecked++;
                uncheckedLists.insert(entry->GetChangeList());
                if (m_bRecursive)
                {
                    // items which are not modified, i.e. won't get committed can
                    // still be shown in a changelist, e.g. the 'don't commit' changelist.
                    if ((entry->status > svn_wc_status_normal) ||
                        (entry->propstatus > svn_wc_status_normal) ||
                        (entry->IsCopied()))
                    {
                        // items that can't be checked don't count as unchecked.
                        if (!(DWORD(m_regShowExternals)&&(entry->IsFromDifferentRepository() || entry->IsNested() || (!bAllowPeggedExternals && entry->IsPeggedExternal()))))
                            m_bUnchecked = true;
                        // This algorithm is for the sake of simplicity of the complexity O(N²)
                        for (int k=0; k<nListItems; k++)
                        {
                            const CSVNStatusListCtrl::FileEntry * entryK = m_ListCtrl.GetConstListEntry(k);
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
    }
    if (m_pathwatcher.LimitReached())
        m_bRecursive = false;
    else if (m_pathwatcher.GetNumberOfChangedPaths() && m_bRecursive)
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
                // something inside an admin dir was changed.
                // if it's the entries file, then we have to fully refresh because
                // files may have been added/removed from version control
                m_bRecursive = false;
                break;
            }
            else if (!m_ListCtrl.IsPathShown(changedList[i]))
            {
                // a path which is not shown in the list has changed
                const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetConstListEntry(changedList[i]);
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
    if (!svn.Add(itemsToAdd, &m_ProjectProperties, svn_depth_empty, true, true, false, true))
    {
        svn.ShowErrorDialog(m_hWnd);
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
    m_ListCtrl.BusyCursor(true);
    INT_PTR nDeleted = arDeleted.GetCount();
    for (INT_PTR i=0; i<arDeleted.GetCount(); i++)
    {
        if (!m_ListCtrl.GetCheck(arDeleted.GetAt(i)))
            continue;
        const CTSVNPath& path = m_ListCtrl.GetConstListEntry(arDeleted.GetAt(i))->GetPath();
        if (!path.IsDirectory())
            continue;
        //now find all children of this directory
        for (int j=0; j<arDeleted.GetCount(); j++)
        {
            if (i==j)
                continue;
            const CSVNStatusListCtrl::FileEntry* childEntry = m_ListCtrl.GetConstListEntry(arDeleted.GetAt(j));
            if (childEntry->IsChecked())
            {
                if (path.IsAncestorOf(childEntry->GetPath()))
                {
                    m_ListCtrl.SetEntryCheck(arDeleted.GetAt(j), false);
                    nDeleted--;
                }
            }
        }
    }
    m_ListCtrl.BusyCursor(false);

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

    if ((!m_bRecursive)&&(bHasDirCopyPlus))
    {
        CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_COPYDEPTH_TASK1)),
                            CString(MAKEINTRESOURCE(IDS_COMMITDLG_COPYDEPTH_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_COMMITDLG_COPYDEPTH_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_COMMITDLG_COPYDEPTH_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(1);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        if (taskdlg.DoModal(m_hWnd) != 1)
        {
            InterlockedExchange(&m_bBlock, FALSE);
            return;
        }
    }

    UpdateData();
    // Release all locks to unchanged items - they won't be released by commit.
    if (!m_bKeepLocks && !svn.Unlock (itemsToUnlock, false))
    {
        svn.ShowErrorDialog(m_hWnd);
        InterlockedExchange(&m_bBlock, FALSE);
        Refresh();
        return;
    }

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
        m_sBugID.Replace(L", ", L",");
        m_sBugID.Replace(L" ,", L",");
        CString sBugID = m_ProjectProperties.sMessage;
        sBugID.Replace(L"%BUGID%", m_sBugID);
        if (m_ProjectProperties.bAppend)
            m_sLogMessage += L"\n" + sBugID + L"\n";
        else
            m_sLogMessage = sBugID + L"\n" + m_sLogMessage;
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
            ATL::CComBSTR commonRoot(m_selectedPathList.GetCommonRoot().GetDirectory().GetWinPath());
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
                CString sError = temp == 0 ? L"" : temp;
                if (!sError.IsEmpty())
                {
                    CAppUtils::ReportFailedHook(m_hWnd, sError);
                    return;
                }
            }
        }
    }

    // run the check-commit hook script
    CTSVNPathList checkedItems;
    m_ListCtrl.WriteCheckedNamesToPathList(checkedItems);
    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(checkedItems.GetCommonRoot(), m_ProjectProperties);
    if (CHooks::Instance().CheckCommit(m_hWnd, checkedItems, m_sLogMessage, exitcode, error))
    {
        if (exitcode)
        {
            CString sErrorMsg;
            sErrorMsg.Format(IDS_HOOK_ERRORMSG, (LPCWSTR)error);

            CTaskDialog taskdlg(sErrorMsg,
                                CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
            taskdlg.SetDefaultCommandControl(1);
            taskdlg.SetMainIcon(TD_ERROR_ICON);
            bool doIt = (taskdlg.DoModal(GetSafeHwnd()) == 1);

            if (doIt)
            {
                // parse the error message and select all mentioned paths if there are any
                // the paths must be separated by newlines!
                CTSVNPathList errorpaths;
                error.Replace(L"\r\n", L"*");
                error.Replace(L"\n", L"*");
                errorpaths.LoadFromAsteriskSeparatedString(error);
                m_ListCtrl.SetSelectionMark(-1);
                for (int i = 0; i < errorpaths.GetCount(); ++i)
                {
                    CTSVNPath errorpath = errorpaths[i];
                    int nListItems = m_ListCtrl.GetItemCount();
                    for (int j = 0; j < nListItems; j++)
                    {
                        const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetConstListEntry(j);
                        if (entry->GetPath().IsEquivalentTo(errorpath))
                        {
                            m_ListCtrl.SetItemState(j, TVIS_SELECTED, TVIS_SELECTED);
                            break;
                        }
                    }
                }
                return;
            }
        }
    }

    if (!m_sLogMessage.IsEmpty())
    {
        m_History.AddEntry(m_sLogMessage);
        m_History.Save();
    }

    SaveSplitterPos();

    CResizableStandAloneDialog::OnOK();
}

void CCommitDlg::SaveSplitterPos()
{
    if (!IsIconic())
    {
        CRegDWORD regPos(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\CommitDlgSizer");
        RECT rectSplitter;
        m_wndSplitter.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        regPos = rectSplitter.top;
    }
}

UINT CCommitDlg::StatusThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CCommitDlg*)pVoid)->StatusThread();
}

UINT CCommitDlg::StatusThread()
{
    //get the status of all selected file/folders recursively
    //and show the ones which have to be committed to the user
    //in a list control.
    InterlockedExchange(&m_bBlock, TRUE);
    InterlockedExchange(&m_bThreadRunning, TRUE);// so the main thread knows that this thread is still running
    InterlockedExchange(&m_bRunThread, TRUE);   // if this is set to FALSE, the thread should stop
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
        SVN svn;
        reg.Format(L"Software\\TortoiseSVN\\History\\commit%s", (LPCTSTR)svn.GetUUIDFromPath(m_pathList[0]));
        m_History.Load(reg, L"logmsgs");
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
        m_CommitTo.SetBold();
        m_CommitTo.SetWindowText(m_ListCtrl.m_sURL);
        m_tooltips.AddTool(GetDlgItem(IDC_STATISTICS), m_ListCtrl.GetStatisticsString());

        {
            // compare all url parts against the branches pattern and if it matches,
            // change the background image of the list control to indicate
            // a commit to a branch
            CRegString branchesPattern (L"Software\\TortoiseSVN\\RevisionGraph\\BranchPattern", L"branches");
            CString sBranches = branchesPattern;
            int pos = 0;
            CString temp;
            bool bFound = false;
            while (!bFound)
            {
                temp = sBranches.Tokenize(L";", pos);
                if (temp.IsEmpty())
                    break;

                int urlpos = 0;
                CString temp2;
                for(;;)
                {
                    temp2 = m_ListCtrl.m_sURL.Tokenize(L"/", urlpos);
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
    if (!success)
    {
        if (!m_ListCtrl.GetLastErrorMessage().IsEmpty())
            m_ListCtrl.SetEmptyString(m_ListCtrl.GetLastErrorMessage());
        m_ListCtrl.Show(dwShow, CTSVNPathList(), 0, true, true);
    }

    CTSVNPath commonDir = m_ListCtrl.GetCommonDirectory(false);
    CAppUtils::SetWindowTitle(m_hWnd, commonDir.GetWinPathString(), m_sWindowTitle);

    // we don't have to block the commit dialog while we fetch the
    // auto completion list.
    m_pathwatcher.ClearChangedPaths();
    InterlockedExchange(&m_bBlock, FALSE);
    UpdateOKButton();
    std::map<CString, int> autolist;
    if ((DWORD)CRegDWORD(L"Software\\TortoiseSVN\\Autocompletion", TRUE)==TRUE)
    {
        m_ListCtrl.BusyCursor(true);
        GetAutocompletionList(autolist);
        m_ListCtrl.BusyCursor(false);
    }
    if (m_bRunThread)
    {
        DialogEnableWindow(IDC_BUGTRAQBUTTON, TRUE);
        DialogEnableWindow(IDC_SHOWUNVERSIONED, true);
        if (m_ListCtrl.HasChangeLists())
            DialogEnableWindow(IDC_KEEPLISTS, true);
        if (m_ListCtrl.HasExternalsFromDifferentRepos())
            DialogEnableWindow(IDC_SHOWEXTERNALS, true);
        if (m_ListCtrl.HasLocks())
            DialogEnableWindow(IDC_KEEPLOCK, true);

        UpdateCheckLinks();
        // we have the list, now signal the main thread about it
        SendMessage(WM_AUTOLISTREADY, 0, (LPARAM)&autolist);  // only send the message if the thread wasn't told to quit!
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
        m_sBugID.Replace(L", ", L",");
        m_sBugID.Replace(L" ,", L",");
        CString sBugID = m_ProjectProperties.sMessage;
        sBugID.Replace(L"%BUGID%", m_sBugID);
        if (m_ProjectProperties.bAppend)
            m_sLogMessage += L"\n" + sBugID + L"\n";
        else
            m_sLogMessage = sBugID + L"\n" + m_sLogMessage;
    }
    if ((m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATECOMMIT).Compare(m_sLogMessage) != 0) &&
        !m_sLogMessage.IsEmpty())
    {
        m_History.AddEntry(m_sLogMessage);
        m_History.Save();
    }
    m_restorepaths = m_ListCtrl.GetRestorePaths();
    if (!m_restorepaths.empty())
    {
        SVN svn;
        for (auto it = m_restorepaths.cbegin(); it != m_restorepaths.cend(); ++it)
        {
            CopyFile(it->first, std::get<0>(it->second), FALSE);
            svn.AddToChangeList(CTSVNPathList(CTSVNPath(std::get<0>(it->second))), std::get<1>(it->second), svn_depth_empty);
        }
    }

    SaveSplitterPos();
    CResizableStandAloneDialog::OnCancel();
}

BOOL CCommitDlg::PreTranslateMessage(MSG* pMsg)
{
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
                if(OnEnterPressed())
                    return TRUE;
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
        OnCantStartThread();
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
    m_tooltips.Pop();   // hide the tooltips
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
        m_ListCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMITUNVERSIONED);
        m_ListCtrl.Invalidate();
    }
    else
    {
        m_ListCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMIT);
        m_ListCtrl.Invalidate();
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
    svn.Add(CTSVNPathList(path), &m_ProjectProperties, svn_depth_empty, false, true, true, true);

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
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
    return 0;
}

LRESULT CCommitDlg::OnAutoListReady(WPARAM, LPARAM lParam)
{
    std::map<CString, int> * autolist = (std::map<CString, int>*)lParam;
    m_cLogMessage.SetAutoCompletionList(std::move(*autolist), '*');
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
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading auto list regex file\n");
        pE->Delete();
    }
}

void CCommitDlg::ParseSnippetFile(const CString& sFile, std::map<CString, CString>& mapSnippet)
{
    CString strLine;
    try
    {
        CStdioFile file(sFile, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
        while (m_bRunThread && file.ReadString(strLine))
        {
            if (strLine.IsEmpty())
                continue;
            if (strLine.Left(1) == _T('#')) // comment char
                continue;
            int eqpos = strLine.Find('=');
            CString key = strLine.Left(eqpos);
            CString value = strLine.Mid(eqpos + 1);
            value.Replace(_T("\\\t"), _T("\t"));
            value.Replace(_T("\\\r"), _T("\r"));
            value.Replace(_T("\\\n"), _T("\n"));
            value.Replace(_T("\\\\"), _T("\\"));
            mapSnippet[key] = value;
        }
        file.Close();
    }
    catch (CFileException* pE)
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading snippet file\n");
        pE->Delete();
    }
}

void CCommitDlg::GetAutocompletionList(std::map<CString, int>& autolist)
{
    // the auto completion list is made of strings from each selected files.
    // the strings used are extracted from the files with regexes found
    // in the file "autolist.txt".
    // the format of that file is:
    // file extensions separated with commas '=' regular expression to use
    // example:
    // .h, .hpp = (?<=class[\s])\b\w+\b|(\b\w+(?=[\s ]?\(\);))
    // .cpp = (?<=[^\s]::)\b\w+\b

    autolist.clear();
    std::map<CString, CString> mapRegex;
    CString sRegexFile = CPathUtils::GetAppDirectory();
    CRegDWORD regtimeout = CRegDWORD(L"Software\\TortoiseSVN\\AutocompleteParseTimeout", 5);
    ULONGLONG timeoutvalue = ULONGLONG(DWORD(regtimeout)) * 1000UL;
    if (timeoutvalue == 0)
        return;
    sRegexFile += L"autolist.txt";
    if (!m_bRunThread)
        return;
    if (PathFileExists(sRegexFile))
        ParseRegexFile(sRegexFile, mapRegex);
    sRegexFile = CPathUtils::GetAppDataDirectory();
    sRegexFile += L"\\autolist.txt";
    if (PathFileExists(sRegexFile))
        ParseRegexFile(sRegexFile, mapRegex);

    m_snippet.clear();
    CString sSnippetFile = CPathUtils::GetAppDirectory();
    sSnippetFile += _T("snippet.txt");
    ParseSnippetFile(sSnippetFile, m_snippet);
    sSnippetFile = CPathUtils::GetAppDataDirectory();
    sSnippetFile += _T("snippet.txt");
    if (PathFileExists(sSnippetFile))
        ParseSnippetFile(sSnippetFile, m_snippet);
    for (auto snip : m_snippet)
        autolist.insert(std::make_pair(snip.first, AUTOCOMPLETE_SNIPPET));

    ULONGLONG starttime = GetTickCount64();

    // now we have two arrays of strings, where the first array contains all
    // file extensions we can use and the second the corresponding regex strings
    // to apply to those files.

    // the next step is to go over all files shown in the commit dialog
    // and scan them for strings we can use
    int nListItems = m_ListCtrl.GetItemCount();
    CRegDWORD removedExtension(L"Software\\TortoiseSVN\\AutocompleteRemovesExtensions", FALSE);
    for (int i = 0; i < nListItems && m_bRunThread; ++i)
    {
        // stop parsing after timeout
        if ((!m_bRunThread) || (GetTickCount64() - starttime > timeoutvalue))
            return;
        const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetConstListEntry(i);
        if (!entry)
            continue;
        if (!entry->IsChecked() && (entry->GetChangeList().Compare(SVNSLC_IGNORECHANGELIST) == 0))
            continue;

        // add the path parts to the auto completion list too
        CString sPartPath = entry->GetRelativeSVNPath(false);
        autolist.insert(std::make_pair(sPartPath, AUTOCOMPLETE_FILENAME));

        int pos = 0;
        int lastPos = 0;
        while ((pos = sPartPath.Find('/', pos)) >= 0)
        {
            pos++;
            lastPos = pos;
            autolist.insert(std::make_pair(sPartPath.Mid(pos), AUTOCOMPLETE_FILENAME));
        }

        // Last inserted entry is a file name.
        // Some users prefer to also list file name without extension.
        if ((DWORD)removedExtension)
        {
            int dotPos = sPartPath.ReverseFind('.');
            if ((dotPos >= 0) && (dotPos > lastPos))
                autolist.insert(std::make_pair(sPartPath.Mid(lastPos, dotPos - lastPos), AUTOCOMPLETE_FILENAME));
        }
    }
    for (int i = 0; i < nListItems && m_bRunThread; ++i)
    {
        // stop parsing after timeout
        if ((!m_bRunThread) || (GetTickCount64() - starttime > timeoutvalue))
            return;
        const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetConstListEntry(i);
        if (!entry)
            continue;
        if (!entry->IsChecked() && (entry->GetChangeList().Compare(SVNSLC_IGNORECHANGELIST) == 0))
            continue;
        if ((entry->status <= svn_wc_status_normal) || (entry->status == svn_wc_status_ignored))
            continue;

        CString sExt = entry->GetPath().GetFileExtension();
        sExt.MakeLower();
        // find the regex string which corresponds to the file extension
        CString rdata = mapRegex[sExt];
        if (rdata.IsEmpty())
            continue;

        ScanFile(autolist, entry->GetPath().GetWinPathString(), rdata, sExt);
        if ((entry->status != svn_wc_status_unversioned) &&
            (entry->status != svn_wc_status_none) &&
            (entry->status != svn_wc_status_ignored) &&
            (entry->status != svn_wc_status_added) &&
            (entry->textstatus != svn_wc_status_normal))
        {
            CTSVNPath basePath = SVN::GetPristinePath(entry->GetPath());
            if (!basePath.IsEmpty())
                ScanFile(autolist, basePath.GetWinPathString(), rdata, sExt);
        }
    }
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Auto completion list loaded in %d msec\n", GetTickCount64() - starttime);
}

void CCommitDlg::ScanFile(std::map<CString, int>& autolist, const CString& sFilePath, const CString& sRegex, const CString& sExt)
{
    static std::map<CString, std::tr1::wregex> regexmap;

    std::wstring sFileContent;
    CAutoFile hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if (hFile)
    {
        DWORD size = GetFileSize(hFile, NULL);
        if (size > 300000L)
        {
            // no files bigger than 300k
            return;
        }
        // allocate memory to hold file contents
        std::unique_ptr<char[]> buffer(new char[size]);
        DWORD readbytes;
        if (!ReadFile(hFile, buffer.get(), size, &readbytes, NULL))
            return;
        int opts = 0;
        IsTextUnicode(buffer.get(), readbytes, &opts);
        if (opts & IS_TEXT_UNICODE_NULL_BYTES)
        {
            return;
        }
        if (opts & IS_TEXT_UNICODE_UNICODE_MASK)
        {
            sFileContent = std::wstring((wchar_t*)buffer.get(), readbytes/sizeof(WCHAR));
        }
        if ((opts & IS_TEXT_UNICODE_NOT_UNICODE_MASK)||(opts == 0))
        {
            const int ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)buffer.get(), readbytes, NULL, 0);
            std::unique_ptr<wchar_t[]> pWideBuf(new wchar_t[ret]);
            const int ret2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)buffer.get(), readbytes, pWideBuf.get(), ret);
            if (ret2 == ret)
                sFileContent = std::wstring(pWideBuf.get(), ret);
        }
    }
    if (sFileContent.empty()|| !m_bRunThread)
    {
        return;
    }

    try
    {

        std::tr1::wregex regCheck;
        std::map<CString, std::tr1::wregex>::const_iterator regIt;
        if ((regIt = regexmap.find(sExt)) != regexmap.end())
            regCheck = regIt->second;
        else
        {
            regCheck = std::tr1::wregex(sRegex, std::tr1::regex_constants::icase | std::tr1::regex_constants::ECMAScript);
            regexmap[sExt] = regCheck;
        }
        const std::tr1::wsregex_iterator end;
        for (std::tr1::wsregex_iterator it(sFileContent.begin(), sFileContent.end(), regCheck); it != end; ++it)
        {
            const std::tr1::wsmatch match = *it;
            for (size_t i=1; i<match.size(); ++i)
            {
                if (match[i].second-match[i].first)
                {
                    autolist.insert(std::make_pair(std::wstring(match[i]).c_str(), AUTOCOMPLETE_PROGRAMCODE));
                }
            }
        }
    }
    catch (std::exception) {}
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
        TCHAR buf[MAX_STATUS_STRING_LENGTH] = { 0 };
        int nListItems = m_ListCtrl.GetItemCount();
        for (int i=0; i<nListItems; ++i)
        {
            const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetConstListEntry(i);
            if (entry->IsChecked())
            {
                CString line;
                svn_wc_status_kind status = entry->status;
                if (status == svn_wc_status_unversioned)
                    status = svn_wc_status_added;
                if (status == svn_wc_status_missing)
                    status = svn_wc_status_deleted;
                WORD langID = (WORD)CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", GetUserDefaultLangID());
                if (m_ProjectProperties.bFileListInEnglish)
                    langID = 1033;
                SVNStatus::GetStatusString(AfxGetResourceHandle(), status, buf, _countof(buf), langID);
                line.Format(L"%-10s %s\r\n", buf, (LPCTSTR)m_ListCtrl.GetItemText(i,0));
                logmsg += line;
            }
        }
        pSciEdit->InsertText(logmsg);
        return true;
    }
    return false;
}

void CCommitDlg::HandleSnippet(int type, const CString &text, CSciEdit *pSciEdit)
{
    if (type == AUTOCOMPLETE_SNIPPET)
    {
        CString target = m_snippet[text];
        pSciEdit->GetWordUnderCursor(true);
        pSciEdit->InsertText(target, false);
    }
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
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Wait some more before refreshing\n");
        }
        else
        {
            KillTimer(REFRESHTIMER);
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Refreshing after items dropped\n");
            Refresh();
        }
        break;
    }
    __super::OnTimer(nIDEvent);
}

void CCommitDlg::OnBnClickedHistory()
{
    m_tooltips.Pop();   // hide the tooltips
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
        if (m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATECOMMIT).Compare(m_cLogMessage.GetText())!=0)
            m_cLogMessage.InsertText(sMsg, !m_cLogMessage.GetText().IsEmpty());
        else
            m_cLogMessage.SetText(sMsg);
    }

    UpdateOKButton();
    GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
}

void CCommitDlg::OnBnClickedBugtraqbutton()
{
    m_tooltips.Pop();   // hide the tooltips
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
    bool bugIdOutSet = false;
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
                bugIdOutSet = true;
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
        if (!sBugID.IsEmpty() && !bugIdOutSet)
        {
            SetDlgItemText(IDC_BUGID, sBugID);
        }
    }

    m_cLogMessage.SetFocus();
}

void CCommitDlg::OnBnClickedLog()
{
    CString sCmd;
    CTSVNPath root = m_pathList.GetCommonRoot();
    if (root.IsEmpty())
    {
        SVN svn;
        root = svn.GetWCRootFromPath(m_pathList[0]);
    }
    sCmd.Format(L"/command:log /path:\"%s\"", root.GetWinPath());
    CAppUtils::RunTortoiseProc(sCmd);
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
        bValidLogSize = TRUE;

    LONG nSelectedItems = m_ListCtrl.GetSelected();
    if (!bValidLogSize)
        m_tooltips.AddTool(IDOK, IDS_COMMITDLG_MSGLENGTH);
    else if (nSelectedItems == 0)
        m_tooltips.AddTool(IDOK, IDS_COMMITDLG_NOTHINGSELECTED);
    else
        m_tooltips.DelTool(IDOK);

    DialogEnableWindow(IDOK, !m_bBlock && bValidLogSize && nSelectedItems>0);
}

void CCommitDlg::OnComError( HRESULT hr )
{
    COMError ce(hr);
    CString sErr;
    sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), ce.GetMessageAndDescription().c_str());
    ::MessageBox(m_hWnd, sErr, L"TortoiseSVN", MB_ICONERROR);
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
    m_tooltips.Pop();   // hide the tooltips
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
    if (CRegDWORD(L"Software\\TortoiseSVN\\VersionCheck", TRUE) == FALSE)
        return;
    CRegString regVer(L"Software\\TortoiseSVN\\NewVersion");
    CString vertemp = regVer;
    int major = _wtoi(vertemp);
    vertemp = vertemp.Mid(vertemp.Find('.')+1);
    int minor = _wtoi(vertemp);
    vertemp = vertemp.Mid(vertemp.Find('.')+1);
    int micro = _wtoi(vertemp);
    vertemp = vertemp.Mid(vertemp.Find('.')+1);
    int build = _wtoi(vertemp);
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
        CRegString regDownText(L"Software\\TortoiseSVN\\NewVersionText");
        CRegString regDownLink(L"Software\\TortoiseSVN\\NewVersionLink", L"http://tortoisesvn.net");

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

BOOL CCommitDlg::OnQueryEndSession()
{
    if (!__super::OnQueryEndSession())
        return FALSE;

    OnCancel();

    return TRUE;
}

void CCommitDlg::ReadPersistedDialogSettings()
{
    m_regAddBeforeCommit = CRegDWORD(L"Software\\TortoiseSVN\\AddBeforeCommit", TRUE);
    m_bShowUnversioned = m_regAddBeforeCommit;

    m_regShowExternals = CRegDWORD(L"Software\\TortoiseSVN\\ShowExternals", TRUE);
    m_bShowExternals = m_regShowExternals;

    m_History.SetMaxHistoryItems((LONG)CRegDWORD(L"Software\\TortoiseSVN\\MaxHistoryItems", 25));

    m_regKeepChangelists = CRegDWORD(L"Software\\TortoiseSVN\\KeepChangeLists", FALSE);
    m_bKeepChangeList = m_regKeepChangelists;

    if (CRegDWORD(L"Software\\TortoiseSVN\\AlwaysWarnIfNoIssue", FALSE))
        m_ProjectProperties.bWarnIfNoIssue = TRUE;

    m_bKeepLocks = SVNConfig::Instance().KeepLocks();
}

void CCommitDlg::SubclassControls()
{
    m_aeroControls.SubclassControl(this, IDC_KEEPLOCK);
    m_aeroControls.SubclassControl(this, IDC_KEEPLISTS);
    m_aeroControls.SubclassControl(this, IDC_LOG);
    m_aeroControls.SubclassControl(this, IDC_RUNHOOK);
    m_aeroControls.SubclassOkCancelHelp(this);
}

void CCommitDlg::InitializeListControl()
{
    m_ListCtrl.SetRestorePaths(m_restorepaths);
    m_ListCtrl.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS | SVNSLC_COLPROPSTATUS | SVNSLC_COLLOCK,
        L"CommitDlg", SVNSLC_POPALL ^ SVNSLC_POPCOMMIT);
    m_ListCtrl.SetStatLabel(GetDlgItem(IDC_STATISTICS));
    m_ListCtrl.SetCancelBool(&m_bCancelled);
    m_ListCtrl.SetEmptyString(IDS_COMMITDLG_NOTHINGTOCOMMIT);
    m_ListCtrl.EnableFileDrop();
    m_ListCtrl.SetBackgroundImage(IDI_COMMIT_BKG);
}

void CCommitDlg::InitializeLogMessageControl()
{
    m_cLogMessage.Init(m_ProjectProperties);
    m_cLogMessage.SetFont((CString)CRegString(L"Software\\TortoiseSVN\\LogFontName",
        L"Courier New"), (DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LogFontSize", 8));
    m_cLogMessage.RegisterContextMenuHandler(this);
    std::map<int, UINT> icons;
    icons[AUTOCOMPLETE_SPELLING] = IDI_SPELL;
    icons[AUTOCOMPLETE_FILENAME] = IDI_FILE;
    icons[AUTOCOMPLETE_PROGRAMCODE] = IDI_CODE;
    icons[AUTOCOMPLETE_SNIPPET] = IDI_SNIPPET;
    m_cLogMessage.SetIcon(icons);
}

void CCommitDlg::SetControlAccessibilityProperties()
{
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));
    CAppUtils::SetAccProperty(m_cLogMessage.GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"Alt+"+CString(CAppUtils::FindAcceleratorKey(this, IDC_INVISIBLE)));
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKALL)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKNONE)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKUNVERSIONED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKVERSIONED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKADDED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKDELETED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKMODIFIED)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKFILES)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
    CAppUtils::SetAccProperty(GetDlgItem(IDC_CHECKDIRECTORIES)->GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_LINK);
}

void CCommitDlg::SetupToolTips()
{
    m_tooltips.AddTool(IDC_EXTERNALWARNING, IDS_COMMITDLG_EXTERNALS);
    m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);
}

void CCommitDlg::HideAndDisableBugTraqButton()
{
    GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BUGTRAQBUTTON)->EnableWindow(FALSE);
}

void CCommitDlg::SetupBugTraqControlsIfConfigured()
{
    CBugTraqAssociations bugtraq_associations;
    bugtraq_associations.Load(m_ProjectProperties.GetProviderUUID(), m_ProjectProperties.sProviderParams);

    bool bExtendUrlControl = true;
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
                SetDlgItemText(IDC_BUGTRAQBUTTON, temp == 0 ? L"" : temp);
                GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
                bExtendUrlControl = false;
            }
        }

        GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
    }

    if (bExtendUrlControl)
        CAppUtils::ExtendControlOverHiddenControl(this, IDC_COMMIT_TO, IDC_BUGTRAQBUTTON);
}

void CCommitDlg::RestoreBugIdAndLabelFromProjectProperties()
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

void CCommitDlg::HideBugIdAndLabel()
{
    GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
}

void CCommitDlg::ShowOrHideBugIdAndLabelControls()
{
    if (!m_ProjectProperties.sMessage.IsEmpty())
        RestoreBugIdAndLabelFromProjectProperties();
    else
        HideBugIdAndLabel();
}

void CCommitDlg::SetupLogMessageDefaultText()
{
    if (!m_sLogMessage.IsEmpty())
        m_cLogMessage.SetText(m_sLogMessage);
    else
        m_cLogMessage.SetText(m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATECOMMIT));
    OnEnChangeLogmessage();
}

void CCommitDlg::SetCommitWindowTitleAndEnableStatus()
{
    GetWindowText(m_sWindowTitle);
    DialogEnableWindow(IDC_LOG, m_pathList.GetCount() > 0);
}

void CCommitDlg::AdjustControlSizes()
{
    AdjustControlSize(IDC_SHOWUNVERSIONED);
    AdjustControlSize(IDC_KEEPLOCK);
}

void CCommitDlg::ConvertStaticToLinkControl()
{
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKALL);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKNONE);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKUNVERSIONED);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKVERSIONED);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKADDED);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKDELETED);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKMODIFIED);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKFILES);
    m_linkControl.ConvertStaticToLink(m_hWnd, IDC_CHECKDIRECTORIES);
}

void CCommitDlg::LineupControlsAndAdjustSizes()
{
    // line up all controls and adjust their sizes.
#define LINKSPACING 9
    RECT rc = AdjustControlSize(IDC_SELECTLABEL);
    rc.right -= 15; // AdjustControlSize() adds 20 pixels for the checkbox/radio button bitmap, but this is a label...
    rc = AdjustStaticSize(IDC_CHECKALL, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKNONE, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKUNVERSIONED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKVERSIONED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKADDED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKDELETED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKMODIFIED, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKFILES, rc, LINKSPACING);
    rc = AdjustStaticSize(IDC_CHECKDIRECTORIES, rc, LINKSPACING);
}

void CCommitDlg::AddAnchorsToFacilitateResizing()
{
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
    AddAnchor(IDC_DWM, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWEXTERNALS, BOTTOM_LEFT);
    AddAnchor(IDC_EXTERNALWARNING, BOTTOM_RIGHT);
    AddAnchor(IDC_STATISTICS, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_KEEPLOCK, BOTTOM_LEFT);
    AddAnchor(IDC_KEEPLISTS, BOTTOM_LEFT);
    AddAnchor(IDC_RUNHOOK, BOTTOM_RIGHT);
    AddAnchor(IDC_LOG, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
}

void CCommitDlg::SaveDialogAndLogMessageControlRectangles()
{
    GetClientRect(m_DlgOrigRect);
    m_cLogMessage.GetClientRect(m_LogMsgOrigRect);
}

void CCommitDlg::CenterWindowWhenLaunchedFromExplorer()
{
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
}

void CCommitDlg::AdjustDialogSizeAndPanes()
{
    EnableSaveRestore(L"CommitDlg");
    DWORD yPos = CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\CommitDlgSizer");
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
}

void CCommitDlg::AddDirectoriesToPathWatcher()
{
    // add all directories to the watcher
    for (int i=0; i<m_pathList.GetCount(); ++i)
    {
        if (m_pathList[i].IsDirectory())
            m_pathwatcher.AddPath(m_pathList[i]);
    }
    m_updatedPathList = m_pathList;
}

void CCommitDlg::GetAsyncFileListStatus()
{
    //first start a thread to obtain the file list with the status without
    //blocking the dialog
    InterlockedExchange(&m_bBlock, TRUE);
    m_pThread = AfxBeginThread(StatusThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
    if (m_pThread==NULL)
    {
        OnCantStartThread();
        InterlockedExchange(&m_bBlock, FALSE);
    }
    else
    {
        m_pThread->m_bAutoDelete = FALSE;
        m_pThread->ResumeThread();
    }
}

void CCommitDlg::ShowBalloonInCaseOfError()
{
    CRegDWORD err = CRegDWORD(L"Software\\TortoiseSVN\\ErrorOccurred", FALSE);
    CRegDWORD historyhint = CRegDWORD(L"Software\\TortoiseSVN\\HistoryHintShown", FALSE);
    if ((((DWORD)err)!=FALSE)&&((((DWORD)historyhint)==FALSE)))
    {
        historyhint = TRUE;
        m_tooltips.ShowBalloon(IDC_HISTORY, IDS_COMMITDLG_HISTORYHINT_TT, IDS_WARN_NOTE, TTI_INFO);
    }
    err = FALSE;
}


void CCommitDlg::OnBnClickedRunhook()
{
    UpdateData();
    // create a list of all checked items
    CTSVNPathList checkedItems;
    m_ListCtrl.WriteCheckedNamesToPathList(checkedItems);
    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(checkedItems.GetCommonRoot(), m_ProjectProperties);
    if (CHooks::Instance().ManualPreCommit(m_hWnd, checkedItems, m_sLogMessage, exitcode, error))
    {
        if (exitcode)
        {
            ::MessageBox(m_hWnd, error, L"TortoiseSVN hook", MB_ICONERROR);
            // parse the error message and select all mentioned paths if there are any
            // the paths must be separated by newlines!
            CTSVNPathList errorpaths;
            error.Replace(L"\r\n", L"*");
            error.Replace(L"\n", L"*");
            errorpaths.LoadFromAsteriskSeparatedString(error);
            m_ListCtrl.SetSelectionMark(-1);
            for (int i = 0; i < errorpaths.GetCount(); ++i)
            {
                CTSVNPath errorpath = errorpaths[i];
                int nListItems = m_ListCtrl.GetItemCount();
                for (int j = 0; j < nListItems; j++)
                {
                    const CSVNStatusListCtrl::FileEntry * entry = m_ListCtrl.GetConstListEntry(j);
                    if (entry->GetPath().IsEquivalentTo(errorpath))
                    {
                        m_ListCtrl.SetItemState(j, TVIS_SELECTED, TVIS_SELECTED);
                        break;
                    }
                }
            }

            return;
        }
        m_tooltips.ShowBalloon(IDC_RUNHOOK, IDS_COMMITDLG_HOOKSUCCESSFUL_TT, IDS_WARN_NOTE, TTI_INFO);
    }
}

