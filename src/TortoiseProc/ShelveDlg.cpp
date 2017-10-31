// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2017 - TortoiseSVN

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
#include "ShelveDlg.h"
#include "SVN.h"
#include "AppUtils.h"

#define REFRESHTIMER   100

IMPLEMENT_DYNAMIC(CShelve, CResizableStandAloneDialog)
CShelve::CShelve(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CShelve::IDD, pParent)
    , m_bThreadRunning(FALSE)
    , m_bCancelled(false)
{
}

CShelve::~CShelve()
{
}

void CShelve::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PATCHLIST, m_PatchList);
    DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
    DDX_Control(pDX, IDC_EDITCONFIG, m_ShelveName);
}


BEGIN_MESSAGE_MAP(CShelve, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
    ON_EN_CHANGE(IDC_EDITCONFIG, OnShelveNameChanged)
    ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CShelve::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    UpdateData(FALSE);

    m_PatchList.Init(0, L"ShelveDlg", SVNSLC_POPALL ^ (SVNSLC_POPIGNORE|SVNSLC_POPCOMMIT|SVNSLC_POPCREATEPATCH|SVNSLC_POPRESTORE));
    m_PatchList.SetConfirmButton((CButton*)GetDlgItem(IDOK));
    m_PatchList.SetSelectButton(&m_SelectAll);
    m_PatchList.SetCancelBool(&m_bCancelled);
    m_PatchList.EnableFileDrop();
    m_PatchList.SetRevertMode(true);

    // We hide help here:
    DialogEnableWindow(IDHELP, false);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    AdjustControlSize(IDC_SELECTALL);

    AddAnchor(IDC_PATCHLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_NAMELABEL, BOTTOM_LEFT);
    AddAnchor(IDC_EDITCONFIG, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"ShelveDlg");

    // first start a thread to obtain the file list with the status without
    // blocking the dialog
    if(AfxBeginThread(PatchThreadEntry, this) == NULL)
    {
        OnCantStartThread();
    }
    InterlockedExchange(&m_bThreadRunning, TRUE);

    return TRUE;
}

UINT CShelve::PatchThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CShelve*)pVoid)->PatchThread();
}

DWORD CShelve::ShowMask()
{
    return
        SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWDIRECTFILES;
}

void CShelve::LockOrUnlockOKBtn()
{
    if (m_ShelveName.GetWindowTextLength() > 0 && m_PatchList.GetSelected() > 0)
    {
        DialogEnableWindow(IDOK, true);
    }
    else
    {
        DialogEnableWindow(IDOK, false);
    }
}

UINT CShelve::PatchThread()
{
    // get the status of all selected file/folders recursively
    // and show the ones which can be included in a patch (i.e. the versioned and not-normal ones)
    DialogEnableWindow(IDOK, false);
    m_bCancelled = false;

    if (!m_PatchList.GetStatus(m_pathList))
    {
        m_PatchList.SetEmptyString(m_PatchList.GetLastErrorMessage());
    }

    m_PatchList.Show(
        ShowMask(), CTSVNPathList(), SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS, true, true);

    LockOrUnlockOKBtn();
    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

BOOL CShelve::PreTranslateMessage(MSG* pMsg)
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
                    if(AfxBeginThread(PatchThreadEntry, this) == NULL)
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

void CShelve::OnBnClickedSelectall()
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
    m_PatchList.SelectAll(state == BST_CHECKED);
    LockOrUnlockOKBtn();
    theApp.DoWaitCursor(-1);
}

void CShelve::OnBnClickedHelp()
{
    OnHelp();
}

void CShelve::OnCancel()
{
    m_bCancelled = true;
    if (m_bThreadRunning)
        return;

    CResizableStandAloneDialog::OnCancel();
}

void CShelve::OnOK()
{
    if (m_bThreadRunning)
        return;

    CString val;
    m_ShelveName.GetWindowText(val);

    m_sShelveName = val;

    //save only the files the user has selected into the path list
    m_PatchList.WriteCheckedNamesToPathList(m_pathList);

    CResizableStandAloneDialog::OnOK();
}

LRESULT CShelve::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    if(AfxBeginThread(PatchThreadEntry, this) == NULL)
    {
        OnCantStartThread();
    }
    return 0;
}

void CShelve::OnShelveNameChanged() {
    if (m_bThreadRunning)
    {
        return;
    }
    LockOrUnlockOKBtn();
}

LRESULT CShelve::OnFileDropped(WPARAM, LPARAM lParam)
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

    if (!m_PatchList.HasPath(path))
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
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
    return 0;
}

void CShelve::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
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
            OnSVNStatusListCtrlNeedsRefresh(0, 0);
        }
        break;
    }
    __super::OnTimer(nIDEvent);
}
