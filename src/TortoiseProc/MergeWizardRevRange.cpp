// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2014, 2020-2022 - TortoiseSVN

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
#include "MergeWizard.h"
#include "MergeWizardRevRange.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "LogDialog/LogDlg.h"
#include "Theme.h"

IMPLEMENT_DYNAMIC(CMergeWizardRevRange, CMergeWizardBasePage)

CMergeWizardRevRange::CMergeWizardRevRange()
    : CMergeWizardBasePage(CMergeWizardRevRange::IDD)
    , m_pLogDlg(nullptr)
    , m_pLogDlg2(nullptr)
{
    m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    m_psp.pszHeaderTitle    = MAKEINTRESOURCE(IDS_MERGEWIZARD_REVRANGETITLE);
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
    DDX_Control(pDX, IDC_URLCOMBO, m_urlCombo);
    DDX_Check(pDX, IDC_REVERSEMERGE, static_cast<CMergeWizard*>(GetParent())->m_bReverseMerge);
    DDX_Control(pDX, IDC_WCEDIT, m_wc);
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
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return -1;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd()) && (m_pLogDlg2->IsWindowVisible()))
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

    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return -1;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd()) && (m_pLogDlg2->IsWindowVisible()))
    {
        m_pLogDlg2->SendMessage(WM_CLOSE);
        return -1;
    }

    CString sUrl;
    m_urlCombo.GetWindowText(sUrl);
    auto newlinePos = sUrl.FindOneOf(L"\r\n");
    if (newlinePos >= 0)
        sUrl = sUrl.Left(newlinePos);
    // check if the url has a revision appended to it and remove it if there is one
    auto atposurl = sUrl.ReverseFind('@');
    if (atposurl >= 0)
    {
        CString sRev = sUrl.Mid(atposurl + 1);
        SVNRev  rev(sRev);
        if (rev.IsValid())
        {
            static_cast<CMergeWizard*>(GetParent())->m_pegRev = rev;
            sUrl                                              = sUrl.Left(atposurl);
        }
    }
    CTSVNPath url(sUrl);
    if (!url.IsUrl())
    {
        ShowComboBalloon(&m_urlCombo, IDS_ERR_MUSTBEURL, IDS_ERR_ERROR, TTI_ERROR);
        return -1;
    }

    m_urlCombo.SaveHistory();

    CString    sRegKey = L"Software\\TortoiseSVN\\History\\repoURLS\\MergeURLFor" + static_cast<CMergeWizard*>(GetParent())->m_wcPath.GetSVNPathString();
    CRegString regMergeUrlForWC(sRegKey);
    regMergeUrlForWC                                = sUrl;

    static_cast<CMergeWizard*>(GetParent())->m_url1 = sUrl;
    static_cast<CMergeWizard*>(GetParent())->m_url2 = sUrl;

    if (GetCheckedRadioButton(IDC_MERGERADIO_ALL, IDC_MERGERADIO_SPECIFIC) == IDC_MERGERADIO_ALL)
        m_sRevRange.Empty();

    // if the revision range has HEAD as a revision specified, we have to
    // ask the server what the HEAD revision is: the SVNRevList can only deal
    // with numerical revisions because we have to sort the list to get the
    // ranges correctly
    if (m_sRevRange.Find(L"HEAD") >= 0)
    {
        if (!m_head.IsValid())
        {
            SVN svn;
            m_head = svn.GetHEADRevision(CTSVNPath(static_cast<CMergeWizard*>(GetParent())->m_url1));
        }
        m_sRevRange.Replace(L"HEAD", m_head.ToString());
    }
    int atpos = -1;
    if ((atpos = m_sRevRange.ReverseFind('@')) >= 0)
    {
        static_cast<CMergeWizard*>(GetParent())->m_pegRev = SVNRev(m_sRevRange.Mid(atpos + 1));
        m_sRevRange                                       = m_sRevRange.Left(atpos);
    }
    if (!static_cast<CMergeWizard*>(GetParent())->m_revRangeArray.FromListString(m_sRevRange))
    {
        ShowEditBalloon(IDC_REVISION_RANGE, IDS_ERR_INVALIDREVRANGE, IDS_ERR_ERROR, TTI_ERROR);
        return -1;
    }
    return IDD_MERGEWIZARD_OPTIONS;
}

BOOL CMergeWizardRevRange::OnInitDialog()
{
    CMergeWizardBasePage::OnInitDialog();

    CMergeWizard* pWizard        = static_cast<CMergeWizard*>(GetParent());

    CString       sRegKey        = L"Software\\TortoiseSVN\\History\\repoURLS\\MergeURLFor" + static_cast<CMergeWizard*>(GetParent())->m_wcPath.GetSVNPathString();
    CString       sMergeUrlForWC = CRegString(sRegKey);

    CString       sUuid          = pWizard->m_sUuid;
    m_urlCombo.SetURLHistory(true, false);
    m_urlCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS\\" + sUuid, L"url");
    if (!static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\MergeWCURL", FALSE)))
        m_urlCombo.SetCurSel(0);
    else if (!sMergeUrlForWC.IsEmpty())
        m_urlCombo.SetWindowText(CPathUtils::PathUnescape(sMergeUrlForWC));
    else if (!pWizard->m_url.IsEmpty())
        m_urlCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->m_url));

    if (m_urlCombo.GetString().IsEmpty())
        m_urlCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->m_url));
    if (!pWizard->m_url1.IsEmpty())
        m_urlCombo.SetWindowText(CPathUtils::PathUnescape(pWizard->m_url1));
    if (pWizard->m_revRangeArray.GetCount())
    {
        m_sRevRange = pWizard->m_revRangeArray.ToListString();
        if (pWizard->m_pegRev.IsValid())
            m_sRevRange = m_sRevRange + L"@" + pWizard->m_pegRev.ToString();
        SetDlgItemText(IDC_REVISION_RANGE, m_sRevRange);
    }

    CheckRadioButton(IDC_MERGERADIO_ALL, IDC_MERGERADIO_SPECIFIC, IDC_MERGERADIO_SPECIFIC);

    CString sLabel;
    sLabel.LoadString(IDS_MERGEWIZARD_REVRANGESTRING);
    SetDlgItemText(IDC_REVRANGELABEL, sLabel);

    SetDlgItemText(IDC_WCEDIT, static_cast<CMergeWizard*>(GetParent())->m_wcPath.GetWinPath());

    AdjustControlSize(IDC_REVERSEMERGE);

    AddAnchor(IDC_MERGEREVRANGEFROMGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_MERGEREVRANGERANGEGROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_MERGERADIO_ALL, TOP_LEFT);
    AddAnchor(IDC_MERGERADIO_SPECIFIC, TOP_LEFT);
    AddAnchor(IDC_REVISION_RANGE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SELLOG, TOP_RIGHT);
    AddAnchor(IDC_REVERSEMERGE, TOP_LEFT);
    AddAnchor(IDC_REVRANGELABEL, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_MERGEREVRANGEWCGROUP, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_WCEDIT, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SHOWLOGWC, BOTTOM_RIGHT);

    CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

    StartWCCheckThread(static_cast<CMergeWizard*>(GetParent())->m_wcPath);

    return TRUE;
}

void CMergeWizardRevRange::OnBnClickedShowlog()
{
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
        return;

    CString sUrl;
    m_urlCombo.GetWindowText(sUrl);
    auto newlinePos = sUrl.FindOneOf(L"\r\n");
    if (newlinePos >= 0)
        sUrl = sUrl.Left(newlinePos);

    SVNRev rev(SVNRev::REV_HEAD);

    // check if the url has a revision appended to it
    auto   atPosUrl = sUrl.ReverseFind('@');
    if (atPosUrl >= 0)
    {
        CString sRev = sUrl.Mid(atPosUrl + 1);
        rev          = SVNRev(sRev);
        if (rev.IsValid())
        {
            sUrl = sUrl.Left(atPosUrl);
        }
        else
            rev = SVNRev::REV_HEAD;
    }

    CTSVNPath url(sUrl);

    if (!url.IsEmpty() && url.IsUrl())
    {
        StopWCCheckThread();
        CTSVNPath wcPath = static_cast<CMergeWizard*>(GetParent())->m_wcPath;
        if (m_pLogDlg)
            m_pLogDlg->DestroyWindow();
        delete m_pLogDlg;
        m_pLogDlg = new CLogDlg();
        m_pLogDlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));

        m_pLogDlg->SetSelect(true);
        m_pLogDlg->m_pNotifyWindow = this;
        m_pLogDlg->SetParams(url, rev, rev, 1, TRUE, FALSE);
        m_pLogDlg->SetProjectPropertiesPath(wcPath);
        m_pLogDlg->SetMergePath(wcPath);

        UpdateData(TRUE);
        if (!m_sRevRange.IsEmpty() && (m_sRevRange.Find(L"HEAD") < 0))
        {
            CString sRevRange = m_sRevRange;
            int     atpos     = -1;
            if ((atpos = sRevRange.ReverseFind('@')) >= 0)
            {
                sRevRange = sRevRange.Left(atpos);
            }
            if (static_cast<CMergeWizard*>(GetParent())->m_revRangeArray.FromListString(sRevRange))
            {
                m_pLogDlg->SetSelectedRevRanges(static_cast<CMergeWizard*>(GetParent())->m_revRangeArray);
            }
        }

        m_pLogDlg->Create(IDD_LOGMESSAGE, this);
        m_pLogDlg->ShowWindow(SW_SHOW);
    }
}

LPARAM CMergeWizardRevRange::OnRevSelected(WPARAM wParam, LPARAM lParam)
{
    static_cast<CMergeWizard*>(GetParent())->m_revRangeArray.Clear();

    // lParam is a pointer to an SVNRevList, wParam contains the number of elements in that list.
    if ((lParam) && (wParam))
    {
        UpdateData(TRUE);
        CMergeWizard* dlg    = static_cast<CMergeWizard*>(GetParent());
        dlg->m_revRangeArray = *reinterpret_cast<SVNRevRangeArray*>(lParam);
        bool bReverse        = !!dlg->m_bReverseMerge;
        m_sRevRange          = dlg->m_revRangeArray.ToListString(bReverse);
        if (dlg->m_pegRev.IsValid())
            m_sRevRange = m_sRevRange + L"@" + dlg->m_pegRev.ToString();
        UpdateData(FALSE);
        SetFocus();
    }
    return 0;
}

LPARAM CMergeWizardRevRange::OnRevSelectedOneRange(WPARAM /*wParam*/, LPARAM lParam)
{
    static_cast<CMergeWizard*>(GetParent())->m_revRangeArray.Clear();

    // lParam is a pointer to an SVNRevList
    if ((lParam))
    {
        UpdateData(TRUE);
        CMergeWizard* dlg    = static_cast<CMergeWizard*>(GetParent());
        dlg->m_revRangeArray = *reinterpret_cast<SVNRevRangeArray*>(lParam);
        bool bReverse        = !!dlg->m_bReverseMerge;
        m_sRevRange          = dlg->m_revRangeArray.ToListString(bReverse);
        if (dlg->m_pegRev.IsValid())
            m_sRevRange = m_sRevRange + L"@" + dlg->m_pegRev.ToString();
        UpdateData(FALSE);
        SetFocus();
    }
    return 0;
}

void CMergeWizardRevRange::OnBnClickedBrowse()
{
    SVNRev rev(SVNRev::REV_HEAD);
    CAppUtils::BrowseRepository(m_urlCombo, this, rev);
    if (!rev.IsHead() && rev.IsNumber())
    {
        CString sUrl;
        m_urlCombo.GetWindowText(sUrl);
        auto newlinePos = sUrl.FindOneOf(L"\r\n");
        if (newlinePos >= 0)
            sUrl = sUrl.Left(newlinePos);

        m_urlCombo.SetWindowText(CPathUtils::PathUnescape(sUrl + L"@" + rev.ToString()));
    }
}

BOOL CMergeWizardRevRange::OnSetActive()
{
    CPropertySheet* psheet = static_cast<CPropertySheet*>(GetParent());
    psheet->SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
    SetButtonTexts();

    return CMergeWizardBasePage::OnSetActive();
}

void CMergeWizardRevRange::OnBnClickedShowlogwc()
{
    StopWCCheckThread();
    CTSVNPath wcPath = static_cast<CMergeWizard*>(GetParent())->m_wcPath;
    if (m_pLogDlg2)
        m_pLogDlg2->DestroyWindow();
    delete m_pLogDlg2;
    m_pLogDlg2 = new CLogDlg();
    m_pLogDlg2->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_MERGE_SELECTRANGE)));

    m_pLogDlg2->m_pNotifyWindow = nullptr;
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

// ReSharper disable once CppMemberFunctionMayBeConst
void CMergeWizardRevRange::OnBnClickedMergeradioAll()
{
    CWnd* pwndDlgItem = GetDlgItem(IDC_REVISION_RANGE);
    if (pwndDlgItem == nullptr)
        return;
    if (GetFocus() == pwndDlgItem)
    {
        SendMessage(WM_NEXTDLGCTL, 0, FALSE);
    }
    pwndDlgItem->EnableWindow(FALSE);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMergeWizardRevRange::OnBnClickedMergeradioSpecific()
{
    CWnd* pwndDlgItem = GetDlgItem(IDC_REVISION_RANGE);
    if (pwndDlgItem == nullptr)
        return;
    pwndDlgItem->EnableWindow(TRUE);
}

bool CMergeWizardRevRange::OkToCancel()
{
    StopWCCheckThread();
    if (::IsWindow(m_pLogDlg->GetSafeHwnd()) && (m_pLogDlg->IsWindowVisible()))
    {
        m_pLogDlg->SendMessage(WM_CLOSE);
        return false;
    }
    if (::IsWindow(m_pLogDlg2->GetSafeHwnd()) && (m_pLogDlg2->IsWindowVisible()))
    {
        m_pLogDlg2->SendMessage(WM_CLOSE);
        return false;
    }
    return true;
}
