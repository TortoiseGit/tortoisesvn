// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "SetLogCache.h"
#include "MessageBox.h"
#include "SVN.h"
#include "SVNError.h"
#include "LogCachePool.h"
#include "LogCacheStatistics.h"
#include "LogCacheStatisticsDlg.h"
#include "ProgressDlg.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"
#include "CSVWriter.h"
#include "XPTheme.h"

using namespace LogCache;

IMPLEMENT_DYNAMIC(CSetLogCache, ISettingsPropPage)

#define WM_REFRESH_REPOSITORYLIST (WM_APP + 110)

CSetLogCache::CSetLogCache()
	: ISettingsPropPage(CSetLogCache::IDD)
	, m_bEnableLogCaching(FALSE)
	, m_bSupportAmbiguousURL(FALSE)
	, m_dwMaxHeadAge(0)
    , progress(NULL)
{
	m_regEnableLogCaching = CRegDWORD(_T("Software\\TortoiseSVN\\UseLogCache"), TRUE);
	m_bEnableLogCaching = (DWORD)m_regEnableLogCaching;
	m_regSupportAmbiguousURL = CRegDWORD(_T("Software\\TortoiseSVN\\SupportAmbiguousURL"), FALSE);
	m_bSupportAmbiguousURL = (DWORD)m_regSupportAmbiguousURL;
	m_regDefaultConnectionState = CRegDWORD(_T("Software\\TortoiseSVN\\DefaultConnectionState"), 0);
	m_regMaxHeadAge = CRegDWORD(_T("Software\\TortoiseSVN\\HeadCacheAgeLimit"), 0);
	m_dwMaxHeadAge = (DWORD)m_regMaxHeadAge;
}

CSetLogCache::~CSetLogCache()
{
}

// update cache list

BOOL CSetLogCache::OnSetActive()
{
    FillRepositoryList();

    return ISettingsPropPage::OnSetActive();
}

void CSetLogCache::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_ENABLELOGCACHING, m_bEnableLogCaching);
	DDX_Check(pDX, IDC_SUPPORTAMBIGUOUSURL, m_bSupportAmbiguousURL);
	DDX_Text(pDX, IDC_MAXIMINHEADAGE, m_dwMaxHeadAge);

    DDX_Control(pDX, IDC_GOOFFLINESETTING, m_cDefaultConnectionState);

	DDX_Control(pDX, IDC_REPOSITORYLIST, m_cRepositoryList);
}


BEGIN_MESSAGE_MAP(CSetLogCache, ISettingsPropPage)
	ON_BN_CLICKED(IDC_ENABLELOGCACHING, OnChanged)
	ON_BN_CLICKED(IDC_SUPPORTAMBIGUOUSURL, OnChanged)
	ON_CBN_SELCHANGE(IDC_GOOFFLINESETTING, OnChanged)
	ON_EN_CHANGE(IDC_MAXIMINHEADAGE, OnChanged)

	ON_BN_CLICKED(IDC_CACHEDETAILS, OnBnClickedDetails)
	ON_BN_CLICKED(IDC_CACHEUPDATE, OnBnClickedUpdate)
	ON_BN_CLICKED(IDC_CACHEEXPORT, OnBnClickedExport)
	ON_BN_CLICKED(IDC_CACHEDELETE, OnBnClickedDelete)

	ON_MESSAGE(WM_REFRESH_REPOSITORYLIST, OnRefeshRepositoryList)
	ON_NOTIFY(NM_DBLCLK, IDC_REPOSITORYLIST, &CSetLogCache::OnNMDblclkRepositorylist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_REPOSITORYLIST, &CSetLogCache::OnLvnItemchangedRepositorylist)
END_MESSAGE_MAP()

void CSetLogCache::OnChanged()
{
	SetModified();
}

BOOL CSetLogCache::OnApply()
{
	UpdateData();

	m_regEnableLogCaching = m_bEnableLogCaching;
	if (m_regEnableLogCaching.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regEnableLogCaching.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
	m_regSupportAmbiguousURL = m_bSupportAmbiguousURL;
	if (m_regSupportAmbiguousURL.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regSupportAmbiguousURL.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);
    m_regDefaultConnectionState = m_cDefaultConnectionState.GetCurSel();
	if (m_regDefaultConnectionState.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regDefaultConnectionState.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);

	m_regMaxHeadAge = m_dwMaxHeadAge;
	if (m_regMaxHeadAge.LastError != ERROR_SUCCESS)
		CMessageBox::Show(m_hWnd, m_regMaxHeadAge.getErrorString(), _T("TortoiseSVN"), MB_ICONERROR);

    SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

BOOL CSetLogCache::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

    // connectivity combobox

    while (m_cDefaultConnectionState.GetCount() > 0)
        m_cDefaultConnectionState.DeleteItem(0);

	CString temp;
	temp.LoadString(IDS_SETTINGS_CONNECTIVITY_ASKUSER);
    m_cDefaultConnectionState.AddString (temp);
	temp.LoadString(IDS_SETTINGS_CONNECTIVITY_OFFLINENOW);
    m_cDefaultConnectionState.AddString (temp);
	temp.LoadString(IDS_SETTINGS_CONNECTIVITY_OFFLINEFOREVER);
    m_cDefaultConnectionState.AddString (temp);

    m_cDefaultConnectionState.SetCurSel ((int)m_regDefaultConnectionState);

    // repository list (including header text update when language changed)

	m_cRepositoryList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	m_cRepositoryList.DeleteAllItems();
    int c = m_cRepositoryList.GetHeaderCtrl()->GetItemCount()-1;
	while (c>=0)
		m_cRepositoryList.DeleteColumn(c--);

	temp.LoadString(IDS_SETTINGS_REPOSITORY_URL);
	m_cRepositoryList.InsertColumn (0, temp, LVCFMT_LEFT, 289);
	temp.LoadString(IDS_SETTINGS_REPOSITORY_SIZE);
	m_cRepositoryList.InsertColumn (1, temp, LVCFMT_RIGHT, 95);

	CXPTheme theme;
	theme.SetWindowTheme(m_cRepositoryList.GetSafeHwnd(), L"Explorer", NULL);

    FillRepositoryList();

    // tooltips

	m_tooltips.Create(this);

	m_tooltips.AddTool(IDC_ENABLELOGCACHING, IDS_SETTINGS_LOGCACHE_ENABLE);
	m_tooltips.AddTool(IDC_SUPPORTAMBIGUOUSURL, IDS_SETTINGS_LOGCACHE_AMBIGUOUSURL);
	m_tooltips.AddTool(IDC_GOOFFLINESETTING, IDS_SETTINGS_LOGCACHE_GOOFFLINE);

    m_tooltips.AddTool(IDC_MAXIMINHEADAGE, IDS_SETTINGS_LOGCACHE_HEADAGE);

	m_tooltips.AddTool(IDC_REPOSITORYLIST, IDS_SETTINGS_LOGCACHE_CACHELIST);

	m_tooltips.AddTool(IDC_CACHEDETAILS, IDS_SETTINGS_LOGCACHE_DETAILS);
	m_tooltips.AddTool(IDC_CACHEUPDATE, IDS_SETTINGS_LOGCACHE_UPDATE);
	m_tooltips.AddTool(IDC_CACHEEXPORT, IDS_SETTINGS_LOGCACHE_EXPORT);
	m_tooltips.AddTool(IDC_CACHEDELETE, IDS_SETTINGS_LOGCACHE_DELETE);

	return TRUE;
}

BOOL CSetLogCache::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSetLogCache::OnBnClickedDetails()
{
    CString uuid = GetSelectedUUID();
    if (!uuid.IsEmpty())
    {
        CLogCacheStatistics staticstics 
            (*SVN().GetLogCachePool(), uuid);

        CLogCacheStatisticsDlg dialog (staticstics.GetData(), this);
        dialog.DoModal();
    }
}

void CSetLogCache::OnBnClickedUpdate()
{
	AfxBeginThread (WorkerThread, this);
}

void CSetLogCache::OnBnClickedExport()
{
    CString uuid = GetSelectedUUID();

    CFileDialog dialog (FALSE);
    if (dialog.DoModal() == IDOK)
    {
        SVN svn;
        CCachedLogInfo* cache = svn.GetLogCachePool()->GetCache (uuid);
        CCSVWriter writer;
        writer.Write (*cache, (LPCTSTR)dialog.GetFileName());
    }
}

void CSetLogCache::OnBnClickedDelete()
{
	int nSelCount = m_cRepositoryList.GetSelectedCount();
	CString sQuestion;
	sQuestion.Format(IDS_SETTINGS_CACHEDELETEQUESTION, nSelCount);
	if (CMessageBox::Show(m_hWnd, sQuestion, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		POSITION pos = m_cRepositoryList.GetFirstSelectedItemPosition();
		while (pos)
		{
			int index = m_cRepositoryList.GetNextSelectedItem(pos);
			IT iter = urls.begin();
			std::advance (iter, index);
			SVN().GetLogCachePool()->DropCache (iter->second);
		}
		FillRepositoryList();
	}
}

LRESULT CSetLogCache::OnRefeshRepositoryList (WPARAM, LPARAM)
{
    FillRepositoryList();
	return 0L;
}

CString CSetLogCache::GetSelectedUUID()
{
    int index = m_cRepositoryList.GetSelectionMark();
    if (index == -1)
        return CString();

    IT iter = urls.begin();
    std::advance (iter, index);

    return iter->second;
}

void CSetLogCache::FillRepositoryList()
{
    int count = m_cRepositoryList.GetItemCount();
    while (count > 0)
        m_cRepositoryList.DeleteItem (--count);

    SVN svn;
    CLogCachePool* caches = svn.GetLogCachePool();
    urls = caches->GetRepositoryURLs();

    for (IT iter = urls.begin(), end = urls.end(); iter != end; ++iter, ++count)
    {
        CString url = iter->first;
        if (url == iter->second)
        {
            // we either don't know a repository URL for this one
            // or we know multiple URLs

            CString uuid = iter->second;
            if (caches->GetRepositoryInfo().HasMultipleURLs (uuid))
            {
                url.Format ( IDS_SETTINGS_MULTIPLEURLSFORUUID
                           , caches->GetRepositoryInfo().GetFirstURL (uuid));
            }
            else
            {
                url.Format (IDS_SETTINGS_DELETEDCACHE, uuid);
            }
        }

        m_cRepositoryList.InsertItem (count, url);
        size_t fileSize = caches->FileSize (iter->second) / 1024;

        CString sizeText;
        sizeText.Format(TEXT("%d"), fileSize);
        m_cRepositoryList.SetItemText (count, 1, sizeText);
    }
}

// implement ILogReceiver

void CSetLogCache::ReceiveLog ( LogChangedPathArray* 
					          , svn_revnum_t rev
                              , const StandardRevProps* 
                              , UserRevPropArray* 
                              , bool )
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

UINT CSetLogCache::WorkerThread(LPVOID pVoid)
{
    CoInitialize (NULL);

	CSetLogCache* dialog = (CSetLogCache*)pVoid;

    dialog->progress = new CProgressDlg();
	dialog->progress->SetTitle(IDS_SETTINGS_LOGCACHE_UPDATETITLE);
	dialog->progress->SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
	dialog->progress->SetTime();
	dialog->progress->ShowModal (dialog->m_hWnd);

	// we have to get the log from the repository root

    SVN svn;
	CLogCachePool* caches = svn.GetLogCachePool();
    CRepositoryInfo& info = caches->GetRepositoryInfo();

	CTSVNPath urlpath;
    urlpath.SetFromSVN (info.GetRootFromUUID (dialog->GetSelectedUUID()));

    dialog->headRevision = info.GetHeadRevision (urlpath);
	dialog->progress->SetProgress (0, dialog->headRevision);

	apr_pool_t *pool = svn_pool_create(NULL);

    try
	{
        CSVNLogQuery svnQuery (svn.m_pctx, pool);
		CCacheLogQuery query (caches, &svnQuery);

		query.Log ( CTSVNPathList (urlpath)
				  , dialog->headRevision
				  , dialog->headRevision
				  , SVNRev(0)
				  , 0
				  , false		// strictNodeHistory
				  , dialog
                  , true		// includeChanges
                  , false		// includeMerges
                  , true		// includeStandardRevProps
                  , true		// includeUserRevProps
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

	return 0;
}

void CSetLogCache::OnNMDblclkRepositorylist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	OnBnClickedDetails();
	*pResult = 0;
}

void CSetLogCache::OnLvnItemchangedRepositorylist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	UINT count = m_cRepositoryList.GetSelectedCount();
	GetDlgItem(IDC_CACHEDETAILS)->EnableWindow(count == 1);
	GetDlgItem(IDC_CACHEUPDATE)->EnableWindow(count == 1);
	GetDlgItem(IDC_CACHEEXPORT)->EnableWindow(count == 1);
	GetDlgItem(IDC_CACHEDELETE)->EnableWindow(count >= 1);
	*pResult = 0;
}
