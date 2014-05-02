// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "DeleteUnversionedDlg.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CDeleteUnversionedDlg, CResizableStandAloneDialog)
CDeleteUnversionedDlg::CDeleteUnversionedDlg(CWnd* pParent /*=NULL*/)
: CResizableStandAloneDialog(CDeleteUnversionedDlg::IDD, pParent)
    , m_bSelectAll(TRUE)
    , m_bUseRecycleBin(TRUE)
    , m_bHideIgnored(FALSE)
    , m_bThreadRunning(FALSE)
    , m_bCancelled(false)
{
}

CDeleteUnversionedDlg::~CDeleteUnversionedDlg()
{
}

void CDeleteUnversionedDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ITEMLIST, m_StatusList);
    DDX_Check(pDX, IDC_SELECTALL, m_bSelectAll);
    DDX_Control(pDX, IDC_SELECTALL, m_SelectAll);
    DDX_Check(pDX, IDC_HIDEIGNORED, m_bHideIgnored);
    DDX_Check(pDX, IDC_USERECYCLEBIN, m_bUseRecycleBin);
}


BEGIN_MESSAGE_MAP(CDeleteUnversionedDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_BN_CLICKED(IDC_HIDEIGNORED, &CDeleteUnversionedDlg::OnBnClickedHideignored)
END_MESSAGE_MAP()



BOOL CDeleteUnversionedDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_ITEMLIST, IDC_ITEMLIST, IDC_ITEMLIST, IDC_ITEMLIST);
    m_aeroControls.SubclassControl(this, IDC_SELECTALL);
    m_aeroControls.SubclassControl(this, IDC_USERECYCLEBIN);
    m_aeroControls.SubclassControl(this, IDC_HIDEIGNORED);
    m_aeroControls.SubclassOkCancel(this);

    m_StatusList.Init(SVNSLC_COLEXT | SVNSLC_COLSTATUS, L"DeleteUnversionedDlg", 0, true);
    m_StatusList.SetUnversionedRecurse(true);
    m_StatusList.PutUnversionedLast(false);
    m_StatusList.CheckChildrenWithParent(true);
    m_StatusList.SetConfirmButton((CButton*)GetDlgItem(IDOK));
    m_StatusList.SetSelectButton(&m_SelectAll);
    m_StatusList.SetCancelBool(&m_bCancelled);
    m_StatusList.SetBackgroundImage(IDI_DELUNVERSIONED_BKG);

    GetWindowText(m_sWindowTitle);

    AdjustControlSize(IDC_SELECTALL);

    AddAnchor(IDC_ITEMLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SELECTALL, BOTTOM_LEFT);
    AddAnchor(IDC_HIDEIGNORED, BOTTOM_LEFT);
    AddAnchor(IDC_USERECYCLEBIN, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"DeleteUnversionedDlg");

    // first start a thread to obtain the file list with the status without
    // blocking the dialog
    if (AfxBeginThread(StatusThreadEntry, this)==0)
    {
        OnCantStartThread();
    }

    return TRUE;
}

UINT CDeleteUnversionedDlg::StatusThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CDeleteUnversionedDlg*)pVoid)->StatusThread();
}

UINT CDeleteUnversionedDlg::StatusThread()
{
    InterlockedExchange(&m_bThreadRunning, TRUE);
    // get the status of all selected file/folders recursively
    // and show the ones which are unversioned/ignored to the user
    // in a list control.
    DialogEnableWindow(IDOK, false);
    DialogEnableWindow(IDC_HIDEIGNORED, false);
    m_bCancelled = false;

    if (!m_StatusList.GetStatus(m_pathList, false, !m_bHideIgnored))
    {
        m_StatusList.SetEmptyString(m_StatusList.GetLastErrorMessage());
    }
    DWORD dwShow = SVNSLC_SHOWUNVERSIONED;
    if (!m_bHideIgnored)
        dwShow |= SVNSLC_SHOWIGNORED;
    m_StatusList.Show(dwShow, CTSVNPathList(), dwShow, true, true);

    CTSVNPath commonDir = m_StatusList.GetCommonDirectory(false);
    CAppUtils::SetWindowTitle(m_hWnd, commonDir.GetWinPathString(), m_sWindowTitle);

    InterlockedExchange(&m_bThreadRunning, FALSE);
    RefreshCursor();
    DialogEnableWindow(IDC_HIDEIGNORED, true);

    return 0;
}

void CDeleteUnversionedDlg::OnOK()
{
    if (m_bThreadRunning)
        return;
    // save only the files the user has selected into the temporary file
    m_StatusList.WriteCheckedNamesToPathList(m_pathList);

    CResizableStandAloneDialog::OnOK();
}

void CDeleteUnversionedDlg::OnCancel()
{
    m_bCancelled = true;
    if (m_bThreadRunning)
        return;

    CResizableStandAloneDialog::OnCancel();
}

void CDeleteUnversionedDlg::OnBnClickedSelectall()
{
    if (m_bThreadRunning)
        return;

    UINT state = (m_SelectAll.GetState() & 0x0003);
    if (state == BST_INDETERMINATE)
    {
        // It is not at all useful to manually place the checkbox into the indeterminate state...
        // We will force this on to the unchecked state
        state = BST_UNCHECKED;
        m_SelectAll.SetCheck(state);
    }
    theApp.DoWaitCursor(1);
    m_StatusList.SelectAll(state == BST_CHECKED);
    theApp.DoWaitCursor(-1);
}

BOOL CDeleteUnversionedDlg::PreTranslateMessage(MSG* pMsg)
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
                    if (AfxBeginThread(StatusThreadEntry, this)==0)
                    {
                        OnCantStartThread();
                    }
                }
            }
            break;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

LRESULT CDeleteUnversionedDlg::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    if (AfxBeginThread(StatusThreadEntry, this)==0)
    {
        OnCantStartThread();
    }
    return 0;
}

void CDeleteUnversionedDlg::OnBnClickedHideignored()
{
    UpdateData();
    if (m_bThreadRunning)
        return;
    if (AfxBeginThread(StatusThreadEntry, this) == 0)
    {
        OnCantStartThread();
    }
}
