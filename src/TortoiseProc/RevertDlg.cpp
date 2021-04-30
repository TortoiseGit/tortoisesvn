// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2021 - TortoiseSVN
// Copyright (C) 2019 - TortoiseGit

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
#include "SVN.h"
#include "registry.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "RevertDlg.h"

#define REFRESHTIMER 100

IMPLEMENT_DYNAMIC(CRevertDlg, CResizableStandAloneDialog)
CRevertDlg::CRevertDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CRevertDlg::IDD, pParent)
    , m_bRecursive(false)
    , m_bClearChangeLists(TRUE)
    , m_bSelectAll(TRUE)
    , m_bThreadRunning(FALSE)
    , m_bCancelled(false)
{
}

CRevertDlg::~CRevertDlg()
{
}

void CRevertDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_REVERTLIST, m_revertList);
    DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
    DDX_Check(pDX, IDC_CLEARCHANGELISTS, m_bClearChangeLists);
    DDX_Control(pDX, IDC_SELECTALL, m_selectAll);
}

BEGIN_MESSAGE_MAP(CRevertDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_DELUNVERSIONED, &CRevertDlg::OnBnClickedDelunversioned)
    ON_BN_CLICKED(ID_OK, &CRevertDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CRevertDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_REVERTLIST, IDC_REVERTLIST, IDC_REVERTLIST, IDC_REVERTLIST);
    m_aeroControls.SubclassControl(this, IDC_SELECTALL);
    m_aeroControls.SubclassControl(this, IDC_CLEARCHANGELISTS);
    m_aeroControls.SubclassControl(this, IDC_UNVERSIONEDITEMS);
    m_aeroControls.SubclassControl(this, IDC_DELUNVERSIONED);
    m_aeroControls.SubclassControl(this, ID_OK);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_revertList.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS | SVNSLC_COLPROPSTATUS, L"RevertDlg");
    m_revertList.SetConfirmButton(static_cast<CButton*>(GetDlgItem(ID_OK)));
    m_revertList.SetSelectButton(&m_selectAll);
    m_revertList.SetCancelBool(&m_bCancelled);
    m_revertList.SetBackgroundImage(IDI_REVERT_BKG);
    m_revertList.EnableFileDrop();
    m_revertList.SetRevertMode(true);

    GetWindowText(m_sWindowTitle);

    AdjustControlSize(IDC_SELECTALL);
    AdjustControlSize(IDC_CLEARCHANGELISTS);

    AddAnchor(IDC_REVERTLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
    AddAnchor(IDC_CLEARCHANGELISTS, BOTTOM_LEFT);
    AddAnchor(IDC_UNVERSIONEDITEMS, BOTTOM_RIGHT);
    AddAnchor(IDC_DELUNVERSIONED, BOTTOM_LEFT);
    AddAnchor(ID_OK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"RevertDlg");

    // first start a thread to obtain the file list with the status without
    // blocking the dialog
    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(RevertThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }

    return TRUE;
}

UINT CRevertDlg::RevertThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CRevertDlg*>(pVoid)->RevertThread();
}

UINT CRevertDlg::RevertThread()
{
    // get the status of all selected file/folders recursively
    // and show the ones which can be reverted to the user
    // in a list control.
    DialogEnableWindow(ID_OK, false);
    m_bCancelled = false;

    if (!m_revertList.GetStatus(m_pathList))
    {
        m_revertList.SetEmptyString(m_revertList.GetLastErrorMessage());
    }
    m_revertList.Show(SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO | SVNSLC_SHOWNESTED,
                      CTSVNPathList(),
                      // do not select all files, only the ones the user has selected directly
                      SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWADDED | SVNSLC_SHOWADDEDINADDED, true, true);

    CTSVNPath commonDir = m_revertList.GetCommonDirectory(false);
    CAppUtils::SetWindowTitle(m_hWnd, commonDir.GetWinPathString(), m_sWindowTitle);

    if (m_revertList.HasUnversionedItems())
    {
        if (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\UnversionedAsModified", FALSE)))
        {
            GetDlgItem(IDC_UNVERSIONEDITEMS)->ShowWindow(SW_SHOW);
        }
        else
            GetDlgItem(IDC_UNVERSIONEDITEMS)->ShowWindow(SW_HIDE);
    }
    else
        GetDlgItem(IDC_UNVERSIONEDITEMS)->ShowWindow(SW_HIDE);

    InterlockedExchange(&m_bThreadRunning, FALSE);
    RefreshCursor();

    return 0;
}

void CRevertDlg::OnBnClickedOk()
{
    if (m_bThreadRunning)
        return;
    // save only the files the user has selected into the temporary file
    m_bRecursive = TRUE;
    for (int i = 0; i < m_revertList.GetItemCount(); ++i)
    {
        if (!m_revertList.GetCheck(i))
        {
            m_bRecursive = FALSE;
        }
        else
        {
            const CSVNStatusListCtrl::FileEntry* entry = m_revertList.GetConstListEntry(i);
            // add all selected entries to the list, except the ones with 'added'
            // status: we later *delete* all the entries in the list before
            // the actual revert is done (so the user has the reverted files
            // still in the trash bin to recover from), but it's not good to
            // delete added files because they're not restored by the revert.
            if ((entry->status != svn_wc_status_added) || (entry->IsCopied()))
                m_selectedPathList.AddPath(entry->GetPath());
            // if an entry inside an external is selected, we can't revert
            // recursively anymore because the recursive revert stops at the
            // external boundaries.
            if (entry->IsInExternal())
                m_bRecursive = FALSE;
        }
    }
    if (!m_bRecursive)
    {
        m_revertList.WriteCheckedNamesToPathList(m_pathList);
    }
    m_selectedPathList.SortByPathname();
    CResizableStandAloneDialog::OnOK();
}

void CRevertDlg::OnCancel()
{
    m_bCancelled = true;
    if (m_bThreadRunning)
        return;

    CResizableStandAloneDialog::OnCancel();
}

void CRevertDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CRevertDlg::OnBnClickedSelectall()
{
    UINT state = (m_selectAll.GetState() & 0x0003);
    if (state == BST_INDETERMINATE)
    {
        // It is not at all useful to manually place the checkbox into the indeterminate state...
        // We will force this on to the unchecked state
        state = BST_UNCHECKED;
        m_selectAll.SetCheck(state);
    }
    theApp.DoWaitCursor(1);
    m_revertList.SelectAll(state == BST_CHECKED);
    theApp.DoWaitCursor(-1);
}

BOOL CRevertDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
            case VK_RETURN:
                if (OnEnterPressed())
                    return TRUE;
                break;
            case VK_F5:
            {
                if (!InterlockedExchange(&m_bThreadRunning, TRUE))
                {
                    if (AfxBeginThread(RevertThreadEntry, this) == nullptr)
                    {
                        InterlockedExchange(&m_bThreadRunning, FALSE);
                        OnCantStartThread();
                    }
                }
            }
            break;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CRevertDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    if (InterlockedExchange(&m_bThreadRunning, TRUE))
        return 0;
    if (AfxBeginThread(RevertThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }
    return 0;
}

LRESULT CRevertDlg::OnFileDropped(WPARAM, LPARAM lParam)
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
    path.SetFromWin(reinterpret_cast<LPCWSTR>(lParam));

    if (!m_revertList.HasPath(path))
    {
        if (m_pathList.AreAllPathsFiles())
        {
            m_pathList.AddPath(path);
            m_pathList.RemoveDuplicates();
        }
        else
        {
            // if the path list contains folders, we have to check whether
            // our just (maybe) added path is a child of one of those. If it is
            // a child of a folder already in the list, we must not add it. Otherwise
            // that path could show up twice in the list.
            bool bHasParentInList = false;
            for (int i = 0; i < m_pathList.GetCount(); ++i)
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
            }
        }
    }

    // Always start the timer, since the status of an existing item might have changed
    SetTimer(REFRESHTIMER, 200, nullptr);
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
    return 0;
}

void CRevertDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
        case REFRESHTIMER:
            if (m_bThreadRunning)
            {
                SetTimer(REFRESHTIMER, 200, nullptr);
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Wait some more before refreshing\n");
            }
            else
            {
                KillTimer(REFRESHTIMER);
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Refreshing after items dropped\n");
                OnSVNStatusListCtrlNeedsRefresh(0, 0);
            }
            break;
    }
    __super::OnTimer(nIDEvent);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRevertDlg::OnBnClickedDelunversioned()
{
    CString sCmd;

    sCmd.Format(L"/command:delunversioned /path:\"%s\"",
                static_cast<LPCWSTR>(m_pathList.CreateAsteriskSeparatedString()));
    CAppUtils::RunTortoiseProc(sCmd);
}
