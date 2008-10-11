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

void CSetLogCache::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_ENABLELOGCACHING, m_bEnableLogCaching);
	DDX_Check(pDX, IDC_SUPPORTAMBIGUOUSURL, m_bSupportAmbiguousURL);
	DDX_Text(pDX, IDC_MAXIMINHEADAGE, m_dwMaxHeadAge);

    DDX_Control(pDX, IDC_GOOFFLINESETTING, m_cDefaultConnectionState);
}


BEGIN_MESSAGE_MAP(CSetLogCache, ISettingsPropPage)
	ON_BN_CLICKED(IDC_ENABLELOGCACHING, OnChanged)
	ON_BN_CLICKED(IDC_SUPPORTAMBIGUOUSURL, OnChanged)
	ON_CBN_SELCHANGE(IDC_GOOFFLINESETTING, OnChanged)
	ON_EN_CHANGE(IDC_MAXIMINHEADAGE, OnChanged)
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

    // tooltips

	m_tooltips.Create(this);

	m_tooltips.AddTool(IDC_ENABLELOGCACHING, IDS_SETTINGS_LOGCACHE_ENABLE);
	m_tooltips.AddTool(IDC_SUPPORTAMBIGUOUSURL, IDS_SETTINGS_LOGCACHE_AMBIGUOUSURL);
	m_tooltips.AddTool(IDC_GOOFFLINESETTING, IDS_SETTINGS_LOGCACHE_GOOFFLINE);

    m_tooltips.AddTool(IDC_MAXIMINHEADAGE, IDS_SETTINGS_LOGCACHE_HEADAGE);

	return TRUE;
}

BOOL CSetLogCache::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}
