// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014, 2018, 2021 - TortoiseSVN
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
#include "CreatePatchDlg.h"
#include "SVN.h"
#include "DiffOptionsDlg.h"
#include "AppUtils.h"

#define REFRESHTIMER 100

IMPLEMENT_DYNAMIC(CCreatePatch, CResizableStandAloneDialog)
CCreatePatch::CCreatePatch(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CCreatePatch::IDD, pParent)
    , m_bThreadRunning(FALSE)
    , m_bCancelled(false)
    , m_bShowUnversioned(FALSE)
    , m_bPrettyPrint(true)
{
}

CCreatePatch::~CCreatePatch()
{
}

void CCreatePatch::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PATCHLIST, m_patchList);
    DDX_Control(pDX, IDC_SELECTALL, m_selectAll);
    DDX_Check(pDX, IDC_SHOWUNVERSIONED, m_bShowUnversioned);
}

BEGIN_MESSAGE_MAP(CCreatePatch, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
    ON_BN_CLICKED(IDC_SHOWUNVERSIONED, OnBnClickedShowunversioned)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_DIFFOPTIONS, &CCreatePatch::OnBnClickedDiffoptions)
END_MESSAGE_MAP()

BOOL CCreatePatch::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_PATCHLIST, IDC_PATCHLIST, IDC_PATCHLIST, IDC_PATCHLIST);
    m_aeroControls.SubclassControl(this, IDC_SHOWUNVERSIONED);
    m_aeroControls.SubclassControl(this, IDC_SELECTALL);
    m_aeroControls.SubclassControl(this, IDC_DIFFOPTIONS);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_regAddBeforeCommit = CRegDWORD(L"Software\\TortoiseSVN\\AddBeforeCommit", TRUE);
    m_bShowUnversioned   = m_regAddBeforeCommit;
    UpdateData(FALSE);

    m_patchList.Init(0, L"CreatePatchDlg", SVNSLC_POPALL ^ (SVNSLC_POPIGNORE | SVNSLC_POPCOMMIT | SVNSLC_POPCREATEPATCH | SVNSLC_POPRESTORE));
    m_patchList.SetConfirmButton(static_cast<CButton*>(GetDlgItem(IDOK)));
    m_patchList.SetSelectButton(&m_selectAll);
    m_patchList.SetCancelBool(&m_bCancelled);
    m_patchList.EnableFileDrop();
    m_patchList.SetRevertMode(true);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    AdjustControlSize(IDC_SELECTALL);
    AdjustControlSize(IDC_SHOWUNVERSIONED);

    AddAnchor(IDC_PATCHLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
    AddAnchor(IDC_SHOWUNVERSIONED, BOTTOM_LEFT);
    AddAnchor(IDC_DIFFOPTIONS, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"CreatePatchDlg");

    // first start a thread to obtain the file list with the status without
    // blocking the dialog
    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(PatchThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }

    return TRUE;
}

UINT CCreatePatch::PatchThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CCreatePatch*>(pVoid)->PatchThread();
}

DWORD CCreatePatch::ShowMask() const
{
    return SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWDIRECTFILES |
           (m_bShowUnversioned ? SVNSLC_SHOWUNVERSIONED : 0);
}

UINT CCreatePatch::PatchThread()
{
    // get the status of all selected file/folders recursively
    // and show the ones which can be included in a patch (i.e. the versioned and not-normal ones)
    DialogEnableWindow(IDOK, false);
    DialogEnableWindow(IDC_SHOWUNVERSIONED, false);
    m_bCancelled = false;

    if (!m_patchList.GetStatus(m_pathList))
    {
        m_patchList.SetEmptyString(m_patchList.GetLastErrorMessage());
    }

    m_patchList.Show(
        ShowMask(), CTSVNPathList(), SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS, true, true);

    DialogEnableWindow(IDC_SHOWUNVERSIONED, true);
    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

BOOL CCreatePatch::PreTranslateMessage(MSG* pMsg)
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
                    if (AfxBeginThread(PatchThreadEntry, this) == nullptr)
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

void CCreatePatch::OnBnClickedSelectall()
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
    m_patchList.SelectAll(state == BST_CHECKED);
    theApp.DoWaitCursor(-1);
}

void CCreatePatch::OnBnClickedShowunversioned()
{
    UpdateData();
    m_regAddBeforeCommit = m_bShowUnversioned;
    if (!m_bThreadRunning)
        m_patchList.Show(ShowMask(), CTSVNPathList(), 0, true, true);
}

void CCreatePatch::OnBnClickedHelp()
{
    OnHelp();
}

void CCreatePatch::OnCancel()
{
    m_bCancelled = true;
    if (m_bThreadRunning)
        return;

    CResizableStandAloneDialog::OnCancel();
}

void CCreatePatch::OnOK()
{
    if (m_bThreadRunning)
        return;

    int nListItems = m_patchList.GetItemCount();
    m_filesToRevert.Clear();

    for (int j = 0; j < nListItems; j++)
    {
        const CSVNStatusListCtrl::FileEntry* entry = m_patchList.GetConstListEntry(j);
        if (entry->IsChecked())
        {
            // Unversioned files are not included in the resulting patch file!
            // We add those files to a list which will be used to add those files
            // before creating the patch.
            if ((entry->status == svn_wc_status_none) || (entry->status == svn_wc_status_unversioned))
            {
                m_filesToRevert.AddPath(entry->GetPath());
            }
        }
    }

    if (m_filesToRevert.GetCount())
    {
        // add all unversioned files to version control
        // so they're included in the resulting patch
        // NOTE: these files must be reverted after the patch
        // has been created! Since this dialog doesn't create the patch
        // itself, the calling function is responsible to revert these files!
        SVN svn;
        svn.Add(m_filesToRevert, nullptr, svn_depth_empty, false, false, false, true);
    }

    //save only the files the user has selected into the path list
    m_patchList.WriteCheckedNamesToPathList(m_pathList);

    CResizableStandAloneDialog::OnOK();
}

LRESULT CCreatePatch::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    if (InterlockedExchange(&m_bThreadRunning, TRUE))
        return 0;
    if (AfxBeginThread(PatchThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }
    return 0;
}

LRESULT CCreatePatch::OnFileDropped(WPARAM, LPARAM lParam)
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

    if (!m_patchList.HasPath(path))
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

void CCreatePatch::OnTimer(UINT_PTR nIDEvent)
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

void CCreatePatch::OnBnClickedDiffoptions()
{
    CDiffOptionsDlg dlg(this);
    dlg.SetDiffOptions(m_diffOptions);

    if (dlg.DoModal() == IDOK)
    {
        m_diffOptions  = dlg.GetDiffOptions();
        m_bPrettyPrint = dlg.GetPrettyPrint();
    }
}
