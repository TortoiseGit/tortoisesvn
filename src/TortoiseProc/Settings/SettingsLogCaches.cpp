// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011 - TortoiseSVN

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
#include "SettingsLogCaches.h"
#include "MessageBox.h"
#include "SVN.h"
#include "SVNError.h"
#include "LogCachePool.h"
#include "LogCacheStatistics.h"
#include "LogCacheStatisticsDlg.h"
#include "ProgressDlg.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"
#include "Access/CSVWriter.h"

using namespace LogCache;

IMPLEMENT_DYNAMIC(CSettingsLogCaches, ISettingsPropPage)

#define WM_REFRESH_REPOSITORYLIST (WM_APP + 110)

CSettingsLogCaches::CSettingsLogCaches()
    : ISettingsPropPage(CSettingsLogCaches::IDD)
    , progress(NULL)
    , m_bThreadRunning(0)
{
}

CSettingsLogCaches::~CSettingsLogCaches()
{
}

// update cache list

BOOL CSettingsLogCaches::OnSetActive()
{
    FillRepositoryList();

    return ISettingsPropPage::OnSetActive();
}

void CSettingsLogCaches::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_REPOSITORYLIST, m_cRepositoryList);
}


BEGIN_MESSAGE_MAP(CSettingsLogCaches, ISettingsPropPage)
    ON_BN_CLICKED(IDC_CACHEDETAILS, OnBnClickedDetails)
    ON_BN_CLICKED(IDC_CACHEUPDATE, OnBnClickedUpdate)
    ON_BN_CLICKED(IDC_CACHEEXPORT, OnBnClickedExport)
    ON_BN_CLICKED(IDC_CACHEDELETE, OnBnClickedDelete)

    ON_MESSAGE(WM_REFRESH_REPOSITORYLIST, OnRefeshRepositoryList)
    ON_NOTIFY(NM_DBLCLK, IDC_REPOSITORYLIST, &CSettingsLogCaches::OnNMDblclkRepositorylist)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_REPOSITORYLIST, &CSettingsLogCaches::OnLvnItemchangedRepositorylist)
END_MESSAGE_MAP()

BOOL CSettingsLogCaches::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    // repository list (including header text update when language changed)

    m_cRepositoryList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

    m_cRepositoryList.DeleteAllItems();
    int c = m_cRepositoryList.GetHeaderCtrl()->GetItemCount()-1;
    while (c>=0)
        m_cRepositoryList.DeleteColumn(c--);

    CString temp;
    temp.LoadString(IDS_SETTINGS_REPOSITORY_URL);
    m_cRepositoryList.InsertColumn (0, temp, LVCFMT_LEFT, 289);
    temp.LoadString(IDS_SETTINGS_REPOSITORY_SIZE);
    m_cRepositoryList.InsertColumn (1, temp, LVCFMT_RIGHT, 95);

    SetWindowTheme(m_cRepositoryList.GetSafeHwnd(), L"Explorer", NULL);

    FillRepositoryList();

    // tooltips

    m_tooltips.Create(this);

    m_tooltips.AddTool(IDC_REPOSITORYLIST, IDS_SETTINGS_LOGCACHE_CACHELIST);

    m_tooltips.AddTool(IDC_CACHEDETAILS, IDS_SETTINGS_LOGCACHE_DETAILS);
    m_tooltips.AddTool(IDC_CACHEUPDATE, IDS_SETTINGS_LOGCACHE_UPDATE);
    m_tooltips.AddTool(IDC_CACHEEXPORT, IDS_SETTINGS_LOGCACHE_EXPORT);
    m_tooltips.AddTool(IDC_CACHEDELETE, IDS_SETTINGS_LOGCACHE_DELETE);

    return TRUE;
}

BOOL CSettingsLogCaches::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSettingsLogCaches::OnBnClickedDetails()
{
    TRepo repo = GetSelectedRepo();
    if (!repo.first.IsEmpty())
    {
        CLogCacheStatistics staticstics
            (*SVN().GetLogCachePool(), repo.second, repo.first);

        CLogCacheStatisticsDlg dialog (staticstics.GetData(), this);
        dialog.DoModal();
    }
}

void CSettingsLogCaches::OnBnClickedUpdate()
{
    AfxBeginThread (WorkerThread, this);
}

void CSettingsLogCaches::OnBnClickedExport()
{
    TRepo repo = GetSelectedRepo();
    if (!repo.first.IsEmpty())
    {
        CFileDialog dialog (FALSE);
        if (dialog.DoModal() == IDOK)
        {
            SVN svn;
            CCachedLogInfo* cache
                = svn.GetLogCachePool()->GetCache (repo.second, repo.first);
            CCSVWriter writer;
            writer.Write (*cache, (LPCTSTR)dialog.GetFileName());
        }
    }
}

void CSettingsLogCaches::OnBnClickedDelete()
{
    int nSelCount = m_cRepositoryList.GetSelectedCount();
    CString sQuestion;
    sQuestion.Format(IDS_SETTINGS_CACHEDELETEQUESTION, nSelCount);
    bool bDelete = false;
    if (CTaskDialog::IsSupported())
    {
        CTaskDialog taskdlg(sQuestion,
                            CString(MAKEINTRESOURCE(IDS_SETTINGS_CACHEDELETEQUESTION_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_ENABLE_HYPERLINKS|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_SETTINGS_CACHEDELETEQUESTION_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_SETTINGS_CACHEDELETEQUESTION_TASK4)));
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        bDelete = (taskdlg.DoModal(m_hWnd)==1);
    }
    else
    {
        bDelete = (TSVNMessageBox(m_hWnd, sQuestion, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES);
    }

    if (bDelete)
    {
        POSITION pos = m_cRepositoryList.GetFirstSelectedItemPosition();
        while (pos)
        {
            int index = m_cRepositoryList.GetNextSelectedItem(pos);
            IT iter = repos.begin();
            std::advance (iter, index);
            SVN().GetLogCachePool()->DropCache (iter->second, iter->first);
        }
        FillRepositoryList();
    }
}

LRESULT CSettingsLogCaches::OnRefeshRepositoryList (WPARAM, LPARAM)
{
    FillRepositoryList();
    return 0L;
}

CSettingsLogCaches::TRepo CSettingsLogCaches::GetSelectedRepo()
{
    int index = m_cRepositoryList.GetSelectionMark();
    if (index == -1)
        return CSettingsLogCaches::TRepo();

    IT iter = repos.begin();
    std::advance (iter, index);

    return *iter;
}

void CSettingsLogCaches::FillRepositoryList()
{
    int count = m_cRepositoryList.GetItemCount();
    while (count > 0)
        m_cRepositoryList.DeleteItem (--count);

    SVN svn;
    CLogCachePool* caches = svn.GetLogCachePool();
    repos = caches->GetRepositoryURLs();

    for (IT iter = repos.begin(), end = repos.end(); iter != end; ++iter, ++count)
    {
        CString url = iter->first;

        m_cRepositoryList.InsertItem (count, url);
        size_t fileSize = caches->FileSize (iter->second, url) / 1024;

        CString sizeText;
        sizeText.Format(TEXT("%d"), fileSize);
        m_cRepositoryList.SetItemText (count, 1, sizeText);
    }
}

// implement ILogReceiver

void CSettingsLogCaches::ReceiveLog ( TChangedPaths*
                                    , svn_revnum_t rev
                                    , const StandardRevProps*
                                    , UserRevPropArray*
                                    , const MergeInfo*)
{
    // update internal data

    if ((headRevision < (svn_revnum_t)rev) || (headRevision == NO_REVISION))
        headRevision = rev;

    // update progress bar and check for user pressing "Cancel"

    static DWORD lastProgressCall = 0;
    if (lastProgressCall < GetTickCount() - 500)
    {
        lastProgressCall = GetTickCount();

        CString temp;
        temp.LoadString(IDS_REVGRAPH_PROGGETREVS);
        progress->SetLine(1, temp);
        temp.Format(IDS_REVGRAPH_PROGCURRENTREV, rev);
        progress->SetLine(2, temp);

        progress->SetProgress (headRevision - rev, headRevision);
        if (progress->HasUserCancelled())
            throw SVNError (SVN_ERR_CANCELLED, "");
    }
}

UINT CSettingsLogCaches::WorkerThread(LPVOID pVoid)
{
    CSettingsLogCaches* dialog = (CSettingsLogCaches*)pVoid;
    InterlockedExchange(&dialog->m_bThreadRunning, TRUE);

    CoInitialize (NULL);

    dialog->DialogEnableWindow(IDC_CACHEUPDATE, false);

    dialog->progress = new CProgressDlg();
    dialog->progress->SetTitle(IDS_SETTINGS_LOGCACHE_UPDATETITLE);
    dialog->progress->SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
    dialog->progress->SetTime();
    dialog->progress->ShowModeless(dialog->m_hWnd);

    // we have to get the log from the repository root

    SVN svn;
    svn.SetPromptParentWindow(dialog->GetSafeHwnd());
    CLogCachePool* caches = svn.GetLogCachePool();
    CRepositoryInfo& info = caches->GetRepositoryInfo();

    TRepo repo = dialog->GetSelectedRepo();
    CTSVNPath urlpath;
    urlpath.SetFromSVN (repo.first);

    dialog->headRevision = info.GetHeadRevision (repo.second, urlpath);
    dialog->progress->SetProgress (0, dialog->headRevision);

    apr_pool_t *pool = svn_pool_create(NULL);

    try
    {
        CSVNLogQuery svnQuery (svn.GetSVNClientContext(), pool);
        CCacheLogQuery query (caches, &svnQuery);

        query.Log ( CTSVNPathList (urlpath)
                  , dialog->headRevision
                  , dialog->headRevision
                  , SVNRev(0)
                  , 0
                  , false       // strictNodeHistory
                  , dialog
                  , true        // includeChanges
                  , false       // includeMerges
                  , true        // includeStandardRevProps
                  , true        // includeUserRevProps
                  , TRevPropNames());
    }
    catch (SVNError&)
    {
    }

    caches->Flush();
    svn_pool_destroy (pool);

    if (dialog->progress)
    {
        dialog->progress->Stop();
        delete dialog->progress;
        dialog->progress = NULL;
    }

    CoUninitialize();

    dialog->PostMessage (WM_REFRESH_REPOSITORYLIST);

    dialog->DialogEnableWindow(IDC_CACHEUPDATE, true);

    InterlockedExchange(&dialog->m_bThreadRunning, FALSE);
    return 0;
}

void CSettingsLogCaches::OnNMDblclkRepositorylist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    OnBnClickedDetails();
    *pResult = 0;
}

void CSettingsLogCaches::OnLvnItemchangedRepositorylist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    UINT count = m_cRepositoryList.GetSelectedCount();
    DialogEnableWindow(IDC_CACHEDETAILS, count == 1);
    DialogEnableWindow(IDC_CACHEUPDATE, !m_bThreadRunning && (count == 1));
    DialogEnableWindow(IDC_CACHEEXPORT, count == 1);
    DialogEnableWindow(IDC_CACHEDELETE, count >= 1);
    *pResult = 0;
}

BOOL CSettingsLogCaches::OnKillActive()
{
    if (m_bThreadRunning)
    {
        // don't allow closing this page if
        // the update thread is still running.
        return 0;
    }
    return __super::OnKillActive();
}

BOOL CSettingsLogCaches::OnQueryCancel()
{
    if (m_bThreadRunning)
    {
        // don't allow closing this page if
        // the update thread is still running.
        return FALSE;
    }
    return __super::OnQueryCancel();
}
