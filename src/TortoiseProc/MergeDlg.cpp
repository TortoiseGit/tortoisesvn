// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "MergeDlg.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "MessageBox.h"
#include "registry.h"
#include "SVNDiff.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"

IMPLEMENT_DYNAMIC(CMergeDlg, CStandAloneDialog)
CMergeDlg::CMergeDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CMergeDlg::IDD, pParent)
	, m_URLFrom(_T(""))
	, m_URLTo(_T(""))
	, StartRev(0)
	, EndRev(_T("HEAD"))
	, m_bUseFromURL(TRUE)
	, m_bDryRun(FALSE)
	, m_bIgnoreAncestry(FALSE)
	, m_pLogDlg(NULL)
	, m_pLogDlg2(NULL)
	, bRepeating(FALSE)
	, m_bRecordOnly(FALSE)
	, m_depth(svn_depth_unknown)
	, m_bIgnoreEOL(FALSE)
{
}

CMergeDlg::~CMergeDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
	if (m_pLogDlg2)
		delete m_pLogDlg2;
}

void CMergeDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Text(pDX, IDC_REVISION_START, m_sStartRev);
	DDX_Text(pDX, IDC_REVISION_END, m_sEndRev);
	DDX_Control(pDX, IDC_URLCOMBO2, m_URLCombo2);
	DDX_Check(pDX, IDC_USEFROMURL, m_bUseFromURL);
	DDX_Check(pDX, IDC_IGNOREANCESTRY, m_bIgnoreAncestry);
	DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
	DDX_Control(pDX, IDOK, m_mergeButton);
	DDX_Check(pDX, IDC_IGNOREEOL, m_bIgnoreEOL);
}

BEGIN_MESSAGE_MAP(CMergeDlg, CStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_BROWSE2, OnBnClickedBrowse2)
	ON_BN_CLICKED(IDC_FINDBRANCHSTART, OnBnClickedFindbranchstart)
	ON_BN_CLICKED(IDC_FINDBRANCHEND, OnBnClickedFindbranchend)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_USEFROMURL, OnBnClickedUsefromurl)
	ON_BN_CLICKED(IDC_WCLOG, OnBnClickedWCLog)
	ON_BN_CLICKED(IDC_DRYRUNBUTTON, OnBnClickedDryrunbutton)
	ON_BN_CLICKED(IDC_DIFFBUTTON, OnBnClickedDiffbutton)
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, OnCbnEditchangeUrlcombo)
	ON_EN_CHANGE(IDC_REVISION_END, &CMergeDlg::OnEnChangeRevisionEnd)
	ON_EN_CHANGE(IDC_REVISION_START, &CMergeDlg::OnEnChangeRevisionStart)
	ON_BN_CLICKED(IDC_UIDIFFBUTTON, &CMergeDlg::OnBnClickedUidiffbutton)
END_MESSAGE_MAP()


// CMergeDlg message handlers

BOOL CMergeDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	m_tooltips.Create(this);

	AdjustControlSize(IDC_REVISION_HEAD1);
	AdjustControlSize(IDC_REVISION_N1);
	AdjustControlSize(IDC_USEFROMURL);
	AdjustControlSize(IDC_REVISION_HEAD);
	AdjustControlSize(IDC_REVISION_N);

	AdjustControlSize(IDC_IGNOREANCESTRY);
	AdjustControlSize(IDC_IGNOREEOL);
	AdjustControlSize(IDC_COMPAREWHITESPACES);
	AdjustControlSize(IDC_IGNOREWHITESPACECHANGES);
	AdjustControlSize(IDC_IGNOREALLWHITESPACES);

	CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_COMPAREWHITESPACES);

	m_bFile = !PathIsDirectory(m_URLFrom);
	SVN svn;
	CString url = svn.GetURLFromPath(CTSVNPath(m_wcPath));
	CString sUUID = svn.GetUUIDFromPath(CTSVNPath(m_wcPath));
	if (url.IsEmpty())
	{
		CString temp;
		temp.Format(IDS_ERR_NOURLOFFILE, m_wcPath.GetWinPath());
		CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
		this->EndDialog(IDCANCEL);
		return TRUE;
	}
	else
	{
		CString urlunescaped = CPathUtils::PathUnescape(url);
		if (!bRepeating)
		{
			// unescape the url, it's shown to the user: unescaped url look better
			// do not overwrite the "from" url if set on the commandline
			if (m_URLFrom.IsEmpty())
				m_URLFrom = urlunescaped;
			m_URLTo = urlunescaped;
		}
		SetDlgItemText(IDC_WCURL, urlunescaped);
		m_tooltips.AddTool(IDC_WCURL, url);
		SetDlgItemText(IDC_WCPATH, m_wcPath.GetWinPath());
		m_tooltips.AddTool(IDC_WCPATH, m_wcPath.GetWinPathString());
	}

	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sUUID, _T("url"));
	if (!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\MergeWCURL"), FALSE))
		m_URLCombo.SetCurSel(0);
	// Only set the "From" Url if there is no url history available, if we're repeating and want
	// to set the previous string, or if the URL was set via commandline (and is different than
	// the wc path URL).
	if ((m_URLCombo.GetString().IsEmpty())||bRepeating||(m_URLFrom.Compare(url)))
		m_URLCombo.SetWindowText(m_URLFrom);
	m_URLCombo2.SetURLHistory(TRUE);
	m_URLCombo2.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sUUID, _T("url"));
	m_URLCombo2.SetCurSel(0);
	if ((m_URLCombo2.GetString().IsEmpty())||bRepeating)
		m_URLCombo2.SetWindowText(m_URLTo);

	// if StartRev and/or EndRev are not HEAD, then they're set from the command
	// line and we have to fill in the edit boxes for them and of course set the
	// correct radio button
	if ((bRepeating)||(!StartRev.IsHead() || !EndRev.IsHead()))
	{
		SetStartRevision(StartRev);
		SetEndRevision(EndRev);
		if (m_bUseFromURL)
			DialogEnableWindow(IDC_URLCOMBO2, FALSE);
		else
		{
			DialogEnableWindow(IDC_URLCOMBO2, TRUE);
			SetDlgItemText(IDC_URLCOMBO2, m_URLTo);
		}
		UpdateData(FALSE);
	}
	else
	{
		DialogEnableWindow(IDC_URLCOMBO2, FALSE);
		// set head revision as default revision
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
	}
	OnBnClickedUsefromurl();

	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_WORKING)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
	m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
	switch (m_depth)
	{
	case svn_depth_unknown:
		m_depthCombo.SetCurSel(0);
		break;
	case svn_depth_infinity:
		m_depthCombo.SetCurSel(1);
		break;
	case svn_depth_immediates:
		m_depthCombo.SetCurSel(2);
		break;
	case svn_depth_files:
		m_depthCombo.SetCurSel(3);
		break;
	case svn_depth_empty:
		m_depthCombo.SetCurSel(4);
		break;
	default:
		m_depthCombo.SetCurSel(0);
		break;
	}

	// set the choices for the "Show All" button
	CString temp;
	temp.LoadString(IDS_MERGE_MERGE);
	m_mergeButton.AddEntry(temp);
	temp.LoadString(IDS_MERGE_RECORDONLY);
	m_mergeButton.AddEntry(temp);
	m_mergeButton.SetCurrentEntry(0);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;
}

BOOL CMergeDlg::CheckData(bool bShowErrors /* = true */)
{
	if (!UpdateData(TRUE))
		return FALSE;

	StartRev = SVNRev(m_sStartRev);
	EndRev = SVNRev(m_sEndRev);
	if (GetCheckedRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1) == IDC_REVISION_HEAD1)
	{
		StartRev = SVNRev(_T("HEAD"));
	}
	if (!StartRev.IsValid())
	{
		if (bShowErrors)
			ShowBalloon(IDC_REVISION_START, IDS_ERR_INVALIDREV);
		return FALSE;
	}

	// if head revision, set revision as -1
	if (GetCheckedRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N) == IDC_REVISION_HEAD)
	{
		EndRev = SVNRev(_T("HEAD"));
	}
	if (!EndRev.IsValid())
	{
		if (bShowErrors)
			ShowBalloon(IDC_REVISION_END, IDS_ERR_INVALIDREV);
		return FALSE;
	}

	m_URLCombo.SaveHistory();
	m_URLFrom = m_URLCombo.GetString();
	if (!m_bUseFromURL)
	{
		m_URLCombo2.SaveHistory();
		m_URLTo = m_URLCombo2.GetString();
	}
	else
		m_URLTo = m_URLFrom;

	if ( (LONG)StartRev == (LONG)EndRev && m_URLFrom==m_URLTo)
	{
		if (bShowErrors)
			ShowBalloon(IDC_REVISION_HEAD1, IDS_ERR_MERGEIDENTICALREVISIONS);
		return FALSE;
	}

	switch (m_depthCombo.GetCurSel())
	{
	case 0:
		m_depth = svn_depth_unknown;
		break;
	case 1:
		m_depth = svn_depth_infinity;
		break;
	case 2:
		m_depth = svn_depth_immediates;
		break;
	case 3:
		m_depth = svn_depth_files;
		break;
	case 4:
		m_depth = svn_depth_empty;
		break;
	default:
		m_depth = svn_depth_empty;
		break;
	}

	int rb = GetCheckedRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES);
	switch (rb)
	{
	case IDC_IGNOREWHITESPACECHANGES:
		m_IgnoreSpaces = svn_diff_file_ignore_space_change;
		break;
	case IDC_IGNOREALLWHITESPACES:
		m_IgnoreSpaces = svn_diff_file_ignore_space_all;
		break;
	case IDC_COMPAREWHITESPACES:
	default:
		m_IgnoreSpaces = svn_diff_file_ignore_space_none;
		break;
	}

	INT_PTR entry = m_mergeButton.GetCurrentEntry();
	if (entry == 1)
		m_bRecordOnly = TRUE;

	UpdateData(FALSE);
	return TRUE;
}

void CMergeDlg::OnOK()
{
	m_bDryRun = FALSE;
	if (CheckData())
		CStandAloneDialog::OnOK();
	else
		return;
}

void CMergeDlg::OnBnClickedDryrunbutton()
{
	m_bDryRun = TRUE;
	if (CheckData())
		CStandAloneDialog::OnOK();
	else
		return;
}

void CMergeDlg::OnBnClickedDiffbutton()
{
	if (!CheckData())
		return;
	AfxGetApp()->DoWaitCursor(1);
	// create a unified diff of the merge
	SVNDiff diff(NULL, this->m_hWnd);
	diff.ShowUnifiedDiff(CTSVNPath(m_URLFrom), StartRev, CTSVNPath(m_URLTo), EndRev, EndRev, !!m_bIgnoreAncestry);
	
	AfxGetApp()->DoWaitCursor(-1);
}

void CMergeDlg::OnBnClickedUidiffbutton()
{
	if (!CheckData())
		return;
	AfxGetApp()->DoWaitCursor(1);
	// create a unified diff of the merge
	SVNDiff diff(NULL, this->m_hWnd);
	diff.ShowCompare(CTSVNPath(m_URLFrom), StartRev, CTSVNPath(m_URLTo), EndRev, SVNRev(), !!m_bIgnoreAncestry);

	AfxGetApp()->DoWaitCursor(-1);
}

void CMergeDlg::OnBnClickedBrowse()
{
	CheckData(false);
	if ((!StartRev.IsValid())||(StartRev == 0))
		StartRev = SVNRev::REV_HEAD;
	if (CAppUtils::BrowseRepository(m_URLCombo, this, StartRev))
	{
		SetStartRevision(StartRev);
		if (m_bUseFromURL)
		{
			CString strUrl;
			m_URLCombo.GetWindowText(strUrl);
			m_URLCombo2.SetCurSel(-1);
			m_URLCombo2.SetWindowText(strUrl);
		}
	}
}

void CMergeDlg::OnBnClickedBrowse2()
{
	CheckData(false);

	if ((!EndRev.IsValid())||(EndRev == 0))
		EndRev = SVNRev::REV_HEAD;

	CAppUtils::BrowseRepository(m_URLCombo2, this, EndRev);
	SetEndRevision(EndRev);
}

void CMergeDlg::OnBnClickedFindbranchstart()
{
	CheckData(false);
	if ((!StartRev.IsValid())||(StartRev == 0))
		StartRev = SVNRev::REV_HEAD;
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	CString url;
	m_URLCombo.GetWindowText(url);
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for the main trunk
	if (!url.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->m_wParam = (m_bUseFromURL ? (MERGE_REVSELECTSTARTEND | MERGE_REVSELECTMINUSONE) : MERGE_REVSELECTSTART);
		if (m_bUseFromURL)
			m_pLogDlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));
		else
			m_pLogDlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTSTARTREVISION)));
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->m_pNotifyWindow = this;
		m_pLogDlg->SetParams(CTSVNPath(url), StartRev, StartRev, 1, limit, TRUE, FALSE);
		m_pLogDlg->SetProjectPropertiesPath(m_wcPath);
		m_pLogDlg->ContinuousSelection(true);
		m_pLogDlg->SetMergePath(m_wcPath);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

void CMergeDlg::OnBnClickedFindbranchend()
{
	CheckData(false);

	if ((!EndRev.IsValid())||(EndRev == 0))
		EndRev = SVNRev::REV_HEAD;
	if (::IsWindow(m_pLogDlg2->GetSafeHwnd())&&(m_pLogDlg2->IsWindowVisible()))
		return;
	CString url;
	if (m_bUseFromURL)
		m_URLCombo.GetWindowText(url);
	else
		m_URLCombo2.GetWindowText(url);
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for the main trunk
	if (!url.IsEmpty())
	{
		delete m_pLogDlg2;
		m_pLogDlg2 = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg2->m_wParam = (m_bUseFromURL ? (MERGE_REVSELECTSTARTEND | MERGE_REVSELECTMINUSONE) : MERGE_REVSELECTEND);
		if (m_bUseFromURL)
			m_pLogDlg2->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));
		else
			m_pLogDlg2->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTENDREVISION)));
		m_pLogDlg2->SetSelect(true);
		m_pLogDlg2->m_pNotifyWindow = this;
		m_pLogDlg2->SetProjectPropertiesPath(m_wcPath);
		m_pLogDlg2->SetParams(CTSVNPath(url), EndRev, EndRev, 1, limit, TRUE, FALSE);
		m_pLogDlg2->ContinuousSelection(true);
		m_pLogDlg2->SetMergePath(m_wcPath);
		m_pLogDlg2->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg2->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

LPARAM CMergeDlg::OnRevSelected(WPARAM wParam, LPARAM lParam)
{
	CString temp;

	if (wParam & MERGE_REVSELECTSTART)
	{
		if (wParam & MERGE_REVSELECTMINUSONE)
			lParam--;
		temp.Format(_T("%ld"), lParam);
		SetDlgItemText(IDC_REVISION_START, temp);
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
		DialogEnableWindow(IDC_REVISION_START, TRUE);
	}
	if (wParam & MERGE_REVSELECTEND)
	{
		temp.Format(_T("%ld"), lParam);
		SetDlgItemText(IDC_REVISION_END, temp);
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		DialogEnableWindow(IDC_REVISION_END, TRUE);
	}
	return 0;
}

void CMergeDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CMergeDlg::OnBnClickedUsefromurl()
{
	UpdateData();
	if (m_bUseFromURL)
	{
		CString str;
		m_URLCombo.GetWindowText(str);
		m_URLCombo2.SetWindowText(str);
		DialogEnableWindow(IDC_URLCOMBO2, FALSE);
		DialogEnableWindow(IDC_BROWSE2, FALSE);
		DialogEnableWindow(IDC_FINDBRANCHEND, FALSE);
	}
	else
	{
		DialogEnableWindow(IDC_URLCOMBO2, TRUE);
		DialogEnableWindow(IDC_BROWSE2, TRUE);
		DialogEnableWindow(IDC_FINDBRANCHEND, TRUE);
	}
	UpdateData(FALSE);
}

void CMergeDlg::OnBnClickedWCLog()
{
	UpdateData(TRUE);
	if ((m_pLogDlg)&&(m_pLogDlg->IsWindowVisible()))
		return;
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog for working copy
	if (!m_wcPath.IsEmpty())
	{
		delete [] m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		m_pLogDlg->SetParams(m_wcPath, SVNRev::REV_WC, SVNRev::REV_HEAD, 1, (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100), TRUE, FALSE);
		m_pLogDlg->SetProjectPropertiesPath(m_wcPath);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
	}
	AfxGetApp()->DoWaitCursor(-1);
}

void CMergeDlg::OnCbnEditchangeUrlcombo()
{
	if (m_bUseFromURL)
	{
		CString str;
		m_URLCombo.GetWindowText(str);
		m_URLCombo2.SetCurSel(-1);
		m_URLCombo2.SetWindowText(str);
	}
}

void CMergeDlg::OnEnChangeRevisionEnd()
{
	UpdateData();
	if (m_sEndRev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CMergeDlg::OnEnChangeRevisionStart()
{
	UpdateData();
	if (m_sStartRev.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
	else
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
}

BOOL CMergeDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CStandAloneDialogTmpl<CDialog>::PreTranslateMessage(pMsg);
}

void CMergeDlg::SetStartRevision(const SVNRev& rev)
{
	if (rev.IsHead())
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_HEAD1);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD1, IDC_REVISION_N1, IDC_REVISION_N1);
		m_sStartRev = rev.ToString();
		UpdateData(FALSE);
	}
}

void CMergeDlg::SetEndRevision(const SVNRev& rev)
{
	if (rev.IsHead())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		m_sEndRev = rev.ToString();
		UpdateData(FALSE);
	}
}