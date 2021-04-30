// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010, 2012-2016, 2018, 2020-2021 - TortoiseSVN
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
#include "MergeWizardBasePage.h"

#include "AppUtils.h"
#include "resource.h"

IMPLEMENT_DYNAMIC(CMergeWizardBasePage, CResizablePageEx)

void CMergeWizardBasePage::SetButtonTexts()
{
    CPropertySheet* psheet = static_cast<CPropertySheet*>(GetParent());
    if (psheet)
    {
        psheet->GetDlgItem(ID_WIZFINISH)->SetWindowText(CString(MAKEINTRESOURCE(IDS_MERGE_MERGE)));
        psheet->GetDlgItem(ID_WIZBACK)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_BACK)));
        psheet->GetDlgItem(ID_WIZNEXT)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_NEXT)));
        psheet->GetDlgItem(IDHELP)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_HELP)));
        CAppUtils::SetAccProperty(psheet->GetDlgItem(IDHELP)->GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"F1");
        psheet->GetDlgItem(IDCANCEL)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_CANCEL)));
        CAppUtils::SetAccProperty(psheet->GetDlgItem(IDCANCEL)->GetSafeHwnd(), PROPID_ACC_KEYBOARDSHORTCUT, L"ESC");
    }
}

void CMergeWizardBasePage::StartWCCheckThread(const CTSVNPath& path)
{
    m_path = path;
    if (InterlockedExchange(&m_bThreadRunning, TRUE))
        return;
    delete m_pThread;
    m_pThread = AfxBeginThread(FindRevThreadEntry, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
    if (!m_pThread)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        // ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
        return;
    }
    m_pThread->m_bAutoDelete = FALSE;
    m_pThread->ResumeThread();
}

void CMergeWizardBasePage::StopWCCheckThread()
{
    InterlockedExchange(&m_bCancelled, TRUE);
    if ((m_pThread) && (m_bThreadRunning))
    {
        WaitForSingleObject(m_pThread->m_hThread, 2000);
        if (m_bThreadRunning)
        {
            // we gave the thread a chance to quit. Since the thread didn't
            // listen to us we have to kill it.
            TerminateThread(m_pThread->m_hThread, static_cast<DWORD>(-1));
            InterlockedExchange(&m_bThreadRunning, FALSE);
        }
    }
}

UINT CMergeWizardBasePage::FindRevThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CMergeWizardBasePage*>(pVoid)->FindRevThread();
}

UINT CMergeWizardBasePage::FindRevThread()
{
    svn_revnum_t minRev    = 0;
    svn_revnum_t maxRev    = 0;
    bool         bSwitched = false;
    bool         bModified = false;
    bool         bSparse   = false;

    if (GetWCRevisionStatus(m_path, true, minRev, maxRev, bSwitched, bModified, bSparse))
    {
        if (!m_bCancelled)
            SendMessage(WM_TSVN_MAXREVFOUND, static_cast<WPARAM>(bModified));
    }
    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

/**
 * Display a balloon with close button, anchored at a given combo box edit control on this dialog.
 */
void CMergeWizardBasePage::ShowComboBalloon(CComboBoxEx* pCombo, UINT nIdText, UINT nIdTitle, int nIcon /* = TTI_WARNING */)
{
    CString        text(MAKEINTRESOURCE(nIdText));
    CString        title(MAKEINTRESOURCE(nIdTitle));
    EDITBALLOONTIP bt;
    bt.cbStruct = sizeof(bt);
    bt.pszText  = text;
    bt.pszTitle = title;
    bt.ttiIcon  = nIcon;
    HWND hEdit  = pCombo->GetEditCtrl()->GetSafeHwnd();
    ::SendMessage(hEdit, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&bt));
}
