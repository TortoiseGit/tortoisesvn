// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012 - TortoiseSVN

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
#include "MergeWizard.h"
#include "MergeWizardRevRange.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "LogDialog\LogDlg.h"

IMPLEMENT_DYNAMIC(CMergeWizardRevRange, CMergeWizardBasePage)

CMergeWizardRevRange::CMergeWizardRevRange()
    : CMergeWizardBasePage(CMergeWizardRevRange::IDD)
    , m_sRevRange(_T(""))
    , m_pLogDlg(NULL)
    , m_pLogDlg2(NULL)
{
    m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
    m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_REVRANGETITLE);
    m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_REVRANGESUBTITLE);
}

CMergeWizardRevRange::~CMergeWizardRevRange()
{
    if (m_pLogDlg)
    {
        m_pLogDlg->DestroyWindow();
        delete m_pLogDlg;
    }
    if (m_pLogDlg2)
    {
        m_pLogDlg2->DestroyWindow();
        delete m_pLogDlg2;
    }
}

void CMergeWizardRevRange::DoDataExchange(CDataExchange* pDX)
{
    CMergeWizardBasePage::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_REVISION_RANGE, m_sRevRange);
    DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
    DDX_Check(pDX, IDC_REVERSEMERGE, ((CMergeWizard*)GetParent())->bReverseMerge);
    DDX_Control(pDX, IDC_WCEDIT, m_WC);
}


BEGIN_MESSAGE_MAP(CMergeWizardRevRange, CMergeWizardBasePage)
    ON_MESSAGE(WM_TSVN_MAXREVFOUND, &CMergeWizardRevRange::OnWCStatus)
    ON_REGISTERED_MESSAGE(WM_REVLIST, OnRevSelected)
    ON_REGISTERED_MESSAGE(WM_REVLISTONERANGE, OnRevSelectedOneRange)
    ON_BN_CLICKED(IDC_SELLOG, &CMergeWizardRevRange::OnBnClickedShowlog)
    ON_BN_CLICKED(IDC_BROWSE, &CMergeWizardRevRange::OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_SHOWLOGWC, &CMergeWizardRevRange::OnBnClickedShowlogwc)
    ON_BN_CLICKED(IDC_MERGERADIO_ALL, &CMergeWizardRevRange::OnBnClickedMergeradioAll)
    ON_BN_CLICKED(IDC_MERGERADIO_SPECIFIC, &CMergeWizardRevRange::OnBnClickedMergeradioSpecific)
END_MESSAGE_MAP()


LRESULT CMergeWizardRevRange::OnWizardBack()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return -1;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd())&&(m_pLogDlg2->IsWindowVisible()))
    {
        m_pLogDlg2->SendMessage(WM_CLOSE);
        return -1;
    }
    return IDD_MERGEWIZARD_START;
}

LRESULT CMergeWizardRevRange::OnWizardNext()
{
    StopWCCheckThread();

    UpdateData();

    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return -1;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd())&&(m_pLogDlg2->IsWindowVisible()))
    {
        m_pLogDlg2->SendMessage(WM_CLOSE);
        return -1;
    }

    m_URLCombo.SaveHistory();

    CString sRegKey = _T("Software\\TortoiseSVN\\History\\repoURLS\\MergeURLFor") + ((CMergeWizard*)GetParent())->wcPath.GetSVNPathString();
    CRegString regMergeUrlForWC = CRegString(sRegKey);
    regMergeUrlForWC = m_URLCombo.GetString();

    ((CMergeWizard*)GetParent())->URL1 = m_URLCombo.GetString();
    ((CMergeWizard*)GetParent())->URL2 = m_URLCombo.GetString();

    if (GetCheckedRadioButton(IDC_MERGERADIO_ALL, IDC_MERGERADIO_SPECIFIC)==IDC_MERGERADIO_ALL)
        m_sRevRange.Empty();

    // if the revision range has HEAD as a revision specified, we have to
    // ask the server what the HEAD revision is: the SVNRevList can only deal
    // with numerical revisions because we have to sort the list to get the
    // ranges correctly
    if (m_sRevRange.Find(_T("HEAD")) >= 0)
    {
        if (!m_HEAD.IsValid())
        {
            SVN svn;
            m_HEAD = svn.GetHEADRevision(CTSVNPath(((CMergeWizard*)GetParent())->URL1));
        }
        m_sRevRange.Replace(_T("HEAD"), m_HEAD.ToString());
    }
    int atpos = -1;
    if ((atpos = m_sRevRange.ReverseFind('@')) >= 0)
    {
        ((CMergeWizard*)GetParent())->pegRev = SVNRev(m_sRevRange.Mid(atpos+1));
        m_sRevRange = m_sRevRange.Left(atpos);
    }
    if (!((CMergeWizard*)GetParent())->revRangeArray.FromListString(m_sRevRange))
    {
        ShowEditBalloon(IDC_REVISION_RANGE, IDS_ERR_INVALIDREVRANGE, IDS_ERR_ERROR, TTI_ERROR);
        return -1;
    }
    return IDD_MERGEWIZARD_OPTIONS;
}


BOOL CMergeWizardRevRange::OnInitDialog()
{
    CMergeWizardBasePage::OnInitDialog();

    CMergeWizard * pWizard = (CMergeWizard*)GetParent();

    CString sRegKey = _T("Software\\TortoiseSVN\\History\\repoURLS\\MergeURLFor") + ((CMergeWizard*)GetParent())->wcPath.GetSVNPathString();
    CString sMergeUrlForWC = CRegString(sRegKey);

    CString sUUID = pWizard->sUUID;
    m_URLCombo.SetURLHistory(true, false);
    m_URLCombo.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sUUID, _T("url"));
    if (!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\MergeWCURL"), FALSE))
        m_URLCombo.SetCurSel(0);
    else if (!sMergeUrlForWC.IsEmpty())
        m_URLCombo.SetWindowText(CPathUtils::PathUnescape(sMergeUrlForWC));
    else if (!pWizard->url.IsEmpty())
        m_URLCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->url));

    if (m_URLCombo.GetString().IsEmpty())
        m_URLCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->url));
    if (!pWizard->URL1.IsEmpty())
        m_URLCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->URL1));
    if (pWizard->revRangeArray.GetCount())
    {
        m_sRevRange = pWizard->revRangeArray.ToListString();
        if (pWizard->pegRev.IsValid())
            m_sRevRange = m_sRevRange + L"@" + pWizard->pegRev.ToString();
        SetDlgItemText(IDC_REVISION_RANGE, m_sRevRange);
    }

    CheckRadioButton(IDC_MERGERADIO_ALL, IDC_MERGERADIO_SPECIFIC, IDC_MERGERADIO_SPECIFIC);

    CString sLabel;
    sLabel.LoadString(IDS_MERGEWIZARD_REVRANGESTRING);
    SetDlgItemText(IDC_REVRANGELABEL, sLabel);

    SetDlgItemText(IDC_WCEDIT, ((CMergeWizard*)GetParent())->wcPath.GetWinPath());

    AdjustControlSize(IDC_REVERSEMERGE);

    AddAnchor(IDC_MERGEREVRANGEFROMGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_MERGEREVRANGERANGEGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MERGERADIO_ALL, TOP_LEFT);
    AddAnchor(IDC_MERGERADIO_SPECIFIC, TOP_LEFT);
    AddAnchor(IDC_REVISION_RANGE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SELLOG, TOP_RIGHT);
    AddAnchor(IDC_REVERSEMERGE, TOP_LEFT);
    AddAnchor(IDC_REVRANGELABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MERGEREVRANGEWCGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_WCEDIT, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SHOWLOGWC, TOP_RIGHT);

    StartWCCheckThread(((CMergeWizard*)GetParent())->wcPath);

    return TRUE;
}

void CMergeWizardRevRange::OnBnClickedShowlog()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
        return;

    CString url;
    m_URLCombo.GetWindowText(url);

    if (!url.IsEmpty())
    {
        StopWCCheckThread();
        CTSVNPath wcPath = ((CMergeWizard*)GetParent())->wcPath;
        if (m_pLogDlg)
            m_pLogDlg->DestroyWindow();
        delete m_pLogDlg;
        m_pLogDlg = new CLogDlg();
        m_pLogDlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));

        m_pLogDlg->SetSelect(true);
        m_pLogDlg->m_pNotifyWindow = this;
        m_pLogDlg->SetParams(CTSVNPath(url), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, TRUE, FALSE);
        m_pLogDlg->SetProjectPropertiesPath(wcPath);
        m_pLogDlg->SetMergePath(wcPath);

        UpdateData(TRUE);
        if (m_sRevRange.Find(_T("HEAD")) < 0)
        {
            CString sRevRange = m_sRevRange;
            int atpos = -1;
            if ((atpos = sRevRange.ReverseFind('@')) >= 0)
            {
                sRevRange = sRevRange.Left(atpos);
            }
            if (((CMergeWizard*)GetParent())->revRangeArray.FromListString(sRevRange))
            {
                m_pLogDlg->SetSelectedRevRanges(((CMergeWizard*)GetParent())->revRangeArray);
            }
        }

        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
    }
}

LPARAM CMergeWizardRevRange::OnRevSelected(WPARAM wParam, LPARAM lParam)
{
    ((CMergeWizard*)GetParent())->revRangeArray.Clear();

    // lParam is a pointer to an SVNRevList, wParam contains the number of elements in that list.
    if ((lParam)&&(wParam))
    {
        UpdateData(TRUE);
        CMergeWizard* dlg = (CMergeWizard*)GetParent();
        dlg->revRangeArray = *((SVNRevRangeArray*)lParam);
        bool bReverse = !!dlg->bReverseMerge;
        m_sRevRange = dlg->revRangeArray.ToListString(bReverse);
        if (dlg->pegRev.IsValid())
            m_sRevRange = m_sRevRange + L"@" + dlg->pegRev.ToString();
        UpdateData(FALSE);
        SetFocus();
    }
    return 0;
}

LPARAM CMergeWizardRevRange::OnRevSelectedOneRange(WPARAM /*wParam*/, LPARAM lParam)
{
    ((CMergeWizard*)GetParent())->revRangeArray.Clear();

    // lParam is a pointer to an SVNRevList
    if ((lParam))
    {
        UpdateData(TRUE);
        CMergeWizard* dlg = (CMergeWizard*)GetParent();
        dlg->revRangeArray = *((SVNRevRangeArray*)lParam);
        bool bReverse = !!dlg->bReverseMerge;
        m_sRevRange = dlg->revRangeArray.ToListString(bReverse);
        if (dlg->pegRev.IsValid())
            m_sRevRange = m_sRevRange + L"@" + dlg->pegRev.ToString();
        UpdateData(FALSE);
        SetFocus();
    }
    return 0;
}

void CMergeWizardRevRange::OnBnClickedBrowse()
{
    SVNRev rev(SVNRev::REV_HEAD);
    CAppUtils::BrowseRepository(m_URLCombo, this, rev);
}


BOOL CMergeWizardRevRange::OnSetActive()
{
    CPropertySheet* psheet = (CPropertySheet*) GetParent();
    psheet->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
    SetButtonTexts();

    return CMergeWizardBasePage::OnSetActive();
}

void CMergeWizardRevRange::OnBnClickedShowlogwc()
{
    StopWCCheckThread();
    CTSVNPath wcPath = ((CMergeWizard*)GetParent())->wcPath;
    if (m_pLogDlg2)
        m_pLogDlg2->DestroyWindow();
    delete m_pLogDlg2;
    m_pLogDlg2 = new CLogDlg();
    m_pLogDlg2->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));

    m_pLogDlg2->m_pNotifyWindow = NULL;
    m_pLogDlg2->SetParams(wcPath, SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, TRUE, FALSE);
    m_pLogDlg2->SetProjectPropertiesPath(wcPath);
    m_pLogDlg2->SetMergePath(wcPath);
    m_pLogDlg2->Create(IDD_LOGMESSAGE, this);
    m_pLogDlg2->ShowWindow(SW_SHOW);
}

LPARAM CMergeWizardRevRange::OnWCStatus(WPARAM wParam, LPARAM /*lParam*/)
{
    if (wParam)
    {
        ShowEditBalloon(IDC_WCEDIT, IDS_MERGE_WCDIRTY, IDS_WARN_WARNING, TTI_WARNING);
    }
    return 0;
}

void CMergeWizardRevRange::OnBnClickedMergeradioAll()
{
    CWnd * pwndDlgItem = GetDlgItem(IDC_REVISION_RANGE);
    if (pwndDlgItem == NULL)
        return;
    if (GetFocus() == pwndDlgItem)
    {
        SendMessage(WM_NEXTDLGCTL, 0, FALSE);
    }
    pwndDlgItem->EnableWindow(FALSE);
}

void CMergeWizardRevRange::OnBnClickedMergeradioSpecific()
{
    CWnd * pwndDlgItem = GetDlgItem(IDC_REVISION_RANGE);
    if (pwndDlgItem == NULL)
        return;
    pwndDlgItem->EnableWindow(TRUE);
}

bool CMergeWizardRevRange::OkToCancel()
{
    StopWCCheckThread();
    if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return false;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd())&&(m_pLogDlg2->IsWindowVisible()))
    {
        m_pLogDlg2->SendMessage(WM_CLOSE);
        return false;
    }
    return true;
}
