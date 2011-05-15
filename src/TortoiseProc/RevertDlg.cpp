// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "messagebox.h"
#include "SVN.h"
#include "Registry.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "Revertdlg.h"

#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CRevertDlg, CResizableStandAloneDialog)
CRevertDlg::CRevertDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CRevertDlg::IDD, pParent)
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
    DDX_Control(pDX, IDC_REVERTLIST, m_RevertList);
    DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
    DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
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

    ExtendFrameIntoClientArea(IDC_REVERTLIST, IDC_REVERTLIST, IDC_REVERTLIST, IDC_REVERTLIST);
    m_aeroControls.SubclassControl(this, IDC_SELECTALL);
    m_aeroControls.SubclassControl(this, IDC_UNVERSIONEDITEMS);
    m_aeroControls.SubclassControl(this, IDC_DELUNVERSIONED);
    m_aeroControls.SubclassControl(this, ID_OK);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_RevertList.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS | SVNSLC_COLPROPSTATUS, _T("RevertDlg"));
    m_RevertList.SetConfirmButton((CButton*)GetDlgItem(ID_OK));
    m_RevertList.SetSelectButton(&m_SelectAll);
    m_RevertList.SetCancelBool(&m_bCancelled);
    m_RevertList.SetBackgroundImage(IDI_REVERT_BKG);
    m_RevertList.EnableFileDrop();
    m_RevertList.SetRevertMode(true);

    GetWindowText(m_sWindowTitle);

    AdjustControlSize(IDC_SELECTALL);

    AddAnchor(IDC_REVERTLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
    AddAnchor(IDC_UNVERSIONEDITEMS, BOTTOM_RIGHT);
    AddAnchor(IDC_DELUNVERSIONED, BOTTOM_LEFT);
    AddAnchor(ID_OK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(_T("RevertDlg"));

    // first start a thread to obtain the file list with the status without
    // blocking the dialog
    if (AfxBeginThread(RevertThreadEntry, this)==0)
    {
        OnCantStartThread();
    }
    InterlockedExchange(&m_bThreadRunning, TRUE);

    return TRUE;
}

UINT CRevertDlg::RevertThreadEntry(LPVOID pVoid)
{
    return ((CRevertDlg*)pVoid)->RevertThread();
}

UINT CRevertDlg::RevertThread()
{
    // get the status of all selected file/folders recursively
    // and show the ones which can be reverted to the user
    // in a list control.
    DialogEnableWindow(ID_OK, false);
    m_bCancelled = false;

    if (!m_RevertList.GetStatus(m_pathList))
    {
        m_RevertList.SetEmptyString(m_RevertList.GetLastErrorMessage());
    }
    m_RevertList.Show(SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO | SVNSLC_SHOWNESTED,
                        CTSVNPathList(),
                        // do not select all files, only the ones the user has selected directly
                        SVNSLC_SHOWDIRECTFILES|SVNSLC_SHOWADDED, true, true);

    CTSVNPath commonDir = m_RevertList.GetCommonDirectory(false);
    CAppUtils::SetWindowTitle(m_hWnd, commonDir.GetWinPathString(), m_sWindowTitle);

    if (m_RevertList.HasUnversionedItems())
    {
        if (DWORD(CRegStdDWORD(_T("Software\\TortoiseSVN\\UnversionedAsModified"), FALSE)))
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
    for (int i=0; i<m_RevertList.GetItemCount(); ++i)
    {
        if (!m_RevertList.GetCheck(i))
        {
            m_bRecursive = FALSE;
        }
        else
        {
            const CSVNStatusListCtrl::FileEntry * entry = m_RevertList.GetConstListEntry(i);
            // add all selected entries to the list, except the ones with 'added'
            // status: we later *delete* all the entries in the list before
            // the actual revert is done (so the user has the reverted files
            // still in the trash bin to recover from), but it's not good to
            // delete added files because they're not restored by the revert.
            if (entry->status != svn_wc_status_added)
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
        m_RevertList.WriteCheckedNamesToPathList(m_pathList);
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
    UINT state = (m_SelectAll.GetState() & 0x0003);
    if (state == BST_INDETERMINATE)
    {
        // It is not at all useful to manually place the checkbox into the indeterminate state...
        // We will force this on to the unchecked state
        state = BST_UNCHECKED;
        m_SelectAll.SetCheck(state);
    }
    theApp.DoWaitCursor(1);
    m_RevertList.SelectAll(state == BST_CHECKED);
    theApp.DoWaitCursor(-1);
}

BOOL CRevertDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_RETURN:
            if(OnEnterPressed())
                return TRUE;
            break;
        case VK_F5:
            {
                if (!m_bThreadRunning)
                {
                    if (AfxBeginThread(RevertThreadEntry, this)==0)
                    {
                        OnCantStartThread();
                    }
                    else
                        InterlockedExchange(&m_bThreadRunning, TRUE);
                }
            }
            break;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CRevertDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    if (AfxBeginThread(RevertThreadEntry, this)==0)
    {
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
    path.SetFromWin((LPCTSTR)lParam);

    if (!m_RevertList.HasPath(path))
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
            }
        }
    }

    // Always start the timer, since the status of an existing item might have changed
    SetTimer(REFRESHTIMER, 200, NULL);
    ATLTRACE(_T("Item %s dropped, timer started\n"), path.GetWinPath());
    return 0;
}

void CRevertDlg::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
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
            OnSVNStatusListCtrlNeedsRefresh(0, 0);
        }
        break;
    }
    __super::OnTimer(nIDEvent);
}

void CRevertDlg::OnBnClickedDelunversioned()
{
    CString sCmd;

    sCmd.Format(_T("/command:delunversioned /path:\"%s\""),
        (LPCTSTR)m_pathList.CreateAsteriskSeparatedString());
    CAppUtils::RunTortoiseProc(sCmd);
}

