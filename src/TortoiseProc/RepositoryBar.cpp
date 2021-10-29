// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2021 - TortoiseSVN

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
#include "RepositoryBar.h"
#include "RevisionDlg.h"
#include "SVNInfo.h"
#include "SVN.h"
#include "WaitCursorEx.h"
#include "CommonAppUtils.h"
#include "DPIAware.h"

#define IDC_URL_COMBO      10000
#define IDC_REVISION_BTN   10001
#define IDC_UP_BTN         10002
#define IDC_BACK_BTN       10003
#define IDC_FORWARD_BTN    10004
#define IDC_REVISION_LABEL 10005

IMPLEMENT_DYNAMIC(CRepositoryBar, CReBarCtrl)

CRepositoryBar::CRepositoryBar()
    : m_pRepo(nullptr)
    , m_cbxUrl(this)
    , m_themeCallbackId(0)
{
    m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
        [this]() {
            SetTheme(CTheme::Instance().IsDarkTheme());
        });
}

CRepositoryBar::~CRepositoryBar()
{
}

BEGIN_MESSAGE_MAP(CRepositoryBar, CReBarCtrl)
    ON_CBN_SELENDOK(IDC_URL_COMBO, OnCbnSelEndOK)
    ON_BN_CLICKED(IDC_REVISION_BTN, OnBnClicked)
    ON_BN_CLICKED(IDC_UP_BTN, OnGoUp)
    ON_BN_CLICKED(IDC_BACK_BTN, OnHistoryBack)
    ON_BN_CLICKED(IDC_FORWARD_BTN, OnHistoryForward)
    ON_WM_DESTROY()
    ON_NOTIFY(CBEN_DRAGBEGIN, IDC_URL_COMBO, OnCbenDragbeginUrlcombo)
END_MESSAGE_MAP()

bool CRepositoryBar::Create(CWnd* parent, UINT id, bool inDialog)
{
    CRect rect;
    ASSERT(parent != nullptr);
    parent->GetWindowRect(&rect);

    DWORD style = WS_CHILD | WS_VISIBLE | CCS_TOP | RBS_AUTOSIZE | RBS_VARHEIGHT;

    DWORD styleEx = WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;

    if (inDialog)
    {
        style |= CCS_NODIVIDER;
    }
    else
    {
        style |= RBS_BANDBORDERS;
    }

    if (CReBarCtrl::CreateEx(styleEx, style, CRect(0, 0, 200, 100), parent, id))
    {
        CFont*  font = parent->GetFont();
        CString temp;

        REBARINFO rbi = {0};
        rbi.cbSize    = sizeof rbi;
        rbi.fMask     = 0;
        rbi.himl      = static_cast<HIMAGELIST>(nullptr);

        if (!this->SetBarInfo(&rbi))
            return false;

        REBARBANDINFO rbBi = {0};
        rbBi.cbSize        = REBARBANDINFO_V6_SIZE;
        rbBi.fMask         = RBBIM_TEXT | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
        rbBi.fStyle        = RBBS_NOGRIPPER | RBBS_FIXEDBMP;

        if (inDialog)
            rbBi.fMask |= RBBIM_COLORS;
        else
            rbBi.fMask |= RBBS_CHILDEDGE;
        int bandPos = 0;
        // Create the "Back" button control to be added
        auto size     = CDPIAware::Instance().Scale(GetSafeHwnd(), 24);
        auto iconSize = GetSystemMetrics(SM_CXSMICON);
        rect          = CRect(0, 0, size, size);
        m_btnBack.Create(L"BACK", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_ICON, rect, this, IDC_BACK_BTN);
        m_btnBack.SetImage(CCommonAppUtils::LoadIconEx(IDI_BACKWARD, iconSize, iconSize));
        m_btnBack.SetWindowText(L"");
        m_btnBack.Invalidate();
        rbBi.lpText     = const_cast<wchar_t*>(L"");
        rbBi.hwndChild  = m_btnBack.m_hWnd;
        rbBi.clrFore    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_WINDOWTEXT));
        rbBi.clrBack    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_BTNFACE));
        rbBi.cx         = rect.Width();
        rbBi.cxMinChild = rect.Width();
        rbBi.cyMinChild = rect.Height();
        if (!InsertBand(bandPos++, &rbBi))
            return false;
        // Create the "Forward" button control to be added
        rect = CRect(0, 0, size, size);
        m_btnForward.Create(L"FORWARD", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_ICON, rect, this, IDC_FORWARD_BTN);
        m_btnForward.SetImage(CCommonAppUtils::LoadIconEx(IDI_FORWARD, iconSize, iconSize));
        m_btnForward.SetWindowText(L"");
        m_btnForward.Invalidate();
        rbBi.lpText     = const_cast<wchar_t*>(L"");
        rbBi.hwndChild  = m_btnForward.m_hWnd;
        rbBi.clrFore    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_WINDOWTEXT));
        rbBi.clrBack    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_BTNFACE));
        rbBi.cx         = rect.Width();
        rbBi.cxMinChild = rect.Width();
        rbBi.cyMinChild = rect.Height();
        if (!InsertBand(bandPos++, &rbBi))
            return false;

        // Create the "URL" combo box control to be added
        rect = CRect(0, 0, CDPIAware::Instance().Scale(GetSafeHwnd(), 100), CDPIAware::Instance().Scale(GetSafeHwnd(), 400));
        m_cbxUrl.Create(WS_CHILD | WS_TABSTOP | CBS_DROPDOWN, rect, this, IDC_URL_COMBO);
        m_cbxUrl.SetURLHistory(true, false);
        m_cbxUrl.SetFont(font);
        m_cbxUrl.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS", L"url");
        temp.LoadString(IDS_REPO_BROWSEURL);
        rbBi.lpText     = const_cast<LPWSTR>(static_cast<LPCWSTR>(temp));
        rbBi.hwndChild  = m_cbxUrl.m_hWnd;
        rbBi.clrFore    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_WINDOWTEXT));
        rbBi.clrBack    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_BTNFACE));
        rbBi.cx         = rect.Width();
        rbBi.cxMinChild = rect.Width();
        rbBi.cyMinChild = m_cbxUrl.GetItemHeight(-1) + CDPIAware::Instance().Scale(GetSafeHwnd(), 10);
        if (!InsertBand(bandPos++, &rbBi))
            return false;

        // Reposition the combobox for correct redrawing
        m_cbxUrl.GetWindowRect(rect);
        m_cbxUrl.MoveWindow(rect.left, rect.top, rect.Width(), CDPIAware::Instance().Scale(GetSafeHwnd(), 300));

        // Create the "Up" button control to be added
        rect = CRect(0, 0, size, m_cbxUrl.GetItemHeight(-1) + CDPIAware::Instance().Scale(GetSafeHwnd(), 8));
        m_btnUp.Create(L"UP", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_ICON, rect, this, IDC_UP_BTN);
        m_btnUp.SetImage(CCommonAppUtils::LoadIconEx(IDI_UP, iconSize, iconSize));
        m_btnUp.SetWindowText(L"");
        m_btnUp.Invalidate();
        rbBi.lpText = const_cast<wchar_t*>(L"");
        rbBi.hwndChild  = m_btnUp.m_hWnd;
        rbBi.clrFore    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_WINDOWTEXT));
        rbBi.clrBack    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_BTNFACE));
        rbBi.cx         = rect.Width();
        rbBi.cxMinChild = rect.Width();
        rbBi.cyMinChild = rect.Height();
        if (!InsertBand(bandPos++, &rbBi))
            return false;

        if (CTheme::Instance().IsDarkTheme())
        {
            rect = CRect(0, 0, CDPIAware::Instance().Scale(GetSafeHwnd(), 100), m_cbxUrl.GetItemHeight(-1) + CDPIAware::Instance().Scale(GetSafeHwnd(), 10));
            temp.LoadString(IDS_REPO_BROWSEREV);
            m_revText.Create(temp, WS_CHILD | SS_RIGHT | SS_CENTERIMAGE, rect, this, IDC_REVISION_LABEL);
            m_revText.SetFont(font);
            rbBi.lpText     = const_cast<wchar_t*>(L"");
            rbBi.hwndChild  = m_revText.m_hWnd;
            rbBi.clrFore    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_WINDOWTEXT));
            rbBi.clrBack    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_BTNFACE));
            rbBi.cx         = rect.Width();
            rbBi.cxMinChild = rect.Width();
            rbBi.cyMinChild = rect.Height();
            if (!InsertBand(bandPos++, &rbBi))
                return false;
        }

        // Create the "Revision" button control to be added
        rect = CRect(0, 0, CDPIAware::Instance().Scale(GetSafeHwnd(), 60), m_cbxUrl.GetItemHeight(-1) + CDPIAware::Instance().Scale(GetSafeHwnd(), 10));
        m_btnRevision.Create(L"HEAD", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, rect, this, IDC_REVISION_BTN);
        m_btnRevision.SetFont(font);
        temp.LoadString(IDS_REPO_BROWSEREV);
        if (CTheme::Instance().IsDarkTheme())
            rbBi.lpText = const_cast<wchar_t*>(L"");
        else
            rbBi.lpText = temp.GetBuffer();
        rbBi.hwndChild  = m_btnRevision.m_hWnd;
        rbBi.clrFore    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_WINDOWTEXT));
        rbBi.clrBack    = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_BTNFACE));
        rbBi.cx         = rect.Width();
        rbBi.cxMinChild = rect.Width();
        rbBi.cyMinChild = rect.Height();
        if (!InsertBand(bandPos++, &rbBi))
            return false;

        MaximizeBand(2);

        m_tooltips.Create(this);
        m_tooltips.AddTool(&m_btnUp, IDS_REPOBROWSE_TT_UPFOLDER);
        m_tooltips.AddTool(&m_btnBack, IDS_REPOBROWSE_TT_BACKWARD);
        m_tooltips.AddTool(&m_btnForward, IDS_REPOBROWSE_TT_FORWARD);

        SetTheme(CTheme::Instance().IsDarkTheme());

        return true;
    }

    SetTheme(CTheme::Instance().IsDarkTheme());

    return false;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRepositoryBar::OnCbenDragbeginUrlcombo(NMHDR* pNMHDR, LRESULT* pResult)
{
    if (m_pRepo)
        m_pRepo->OnCbenDragbeginUrlcombo(pNMHDR, pResult);
}

void CRepositoryBar::ShowUrl(const CString& url, const SVNRev& rev)
{
    if (url.Find('?') >= 0)
    {
        m_url = url.Left(url.Find('?'));
        m_rev = SVNRev(url.Mid(url.Find('?') + 1));
    }
    else
    {
        m_url = url;
        m_rev = rev;
    }
    m_cbxUrl.SetWindowText(m_url);
    m_cbxUrl.AddString(m_url, 0);
    m_btnUp.EnableWindow(m_pRepo ? m_url.CompareNoCase(m_pRepo->GetRepoRoot()) : FALSE);
    m_btnRevision.SetWindowText(m_rev.ToString());
    if (m_headRev.IsValid())
    {
        CString sTTText;
        sTTText.Format(IDS_REPOBROWSE_TT_HEADREV, static_cast<LPCWSTR>(m_headRev.ToString()));
        m_tooltips.AddTool(&m_btnRevision, sTTText);
    }
    else
        m_tooltips.DelTool(&m_btnRevision);

    if (m_pRepo)
    {
        m_btnBack.EnableWindow(m_pRepo->GetHistoryBackwardCount() != 0);
        m_btnForward.EnableWindow(m_pRepo->GetHistoryForwardCount() != 0);
    }
}

void CRepositoryBar::GotoUrl(const CString& url, const SVNRev& rev, bool bAlreadyChecked /* = false */)
{
    if (m_pRepo && m_pRepo->IsThreadRunning())
    {
        return;
    }
    CString       newURL = url;
    SVNRev        newRev = rev;
    CWaitCursorEx wait;

    newURL.TrimRight('/');
    if (newURL.IsEmpty())
    {
        newURL = GetCurrentUrl();
        newRev = GetCurrentRev();
        newURL.TrimRight('/');
    }
    if (newURL.Find('?') >= 0)
    {
        newRev = SVNRev(newURL.Mid(newURL.Find('?') + 1));
        if (!newRev.IsValid())
            newRev = SVNRev::REV_HEAD;
        newURL = newURL.Left(newURL.Find('?'));
    }

    if (m_pRepo)
    {
        if (!CTSVNPath(newURL).IsCanonical())
        {
            CString sErr;
            sErr.Format(IDS_ERR_INVALIDURLORPATH, static_cast<LPCWSTR>(newURL));
            ::MessageBox(GetSafeHwnd(), sErr, L"TortoiseSVN", MB_ICONERROR);
            return;
        }
        SVNRev r  = newRev;
        m_headRev = SVNRev();
        m_pRepo->ChangeToUrl(newURL, r, bAlreadyChecked);
        if (newRev.IsHead() && !r.IsHead())
            m_headRev = r;
        if (!m_headRev.IsValid())
        {
            SVN svn;
            m_headRev = svn.GetHEADRevision(CTSVNPath(newURL));
        }
    }
    ShowUrl(newURL, newRev);
}

void CRepositoryBar::SetRevision(const SVNRev& rev)
{
    m_btnRevision.SetWindowText(rev.ToString());
    if (m_headRev.IsValid())
    {
        CString sTTText;
        sTTText.Format(IDS_REPOBROWSE_TT_HEADREV, static_cast<LPCWSTR>(m_headRev.ToString()));
        m_tooltips.AddTool(&m_btnRevision, sTTText);
    }
    else
        m_tooltips.DelTool(&m_btnRevision);
}

void CRepositoryBar::SetHeadRevision(svn_revnum_t rev)
{
    if (rev < 0)
        return;

    m_headRev = rev;
    CString sTTText;
    sTTText.Format(IDS_REPOBROWSE_TT_HEADREV, static_cast<LPCWSTR>(m_headRev.ToString()));
    m_tooltips.AddTool(&m_btnRevision, sTTText);
}

CString CRepositoryBar::GetCurrentUrl() const
{
    if (m_cbxUrl.m_hWnd != nullptr)
    {
        CString path;
        m_cbxUrl.GetWindowText(path);
        path.Replace('\\', '/');
        return path;
    }
    else
    {
        return m_url;
    }
}

SVNRev CRepositoryBar::GetCurrentRev() const
{
    if (m_cbxUrl.m_hWnd != nullptr)
    {
        CString revision;
        m_btnRevision.GetWindowText(revision);
        return SVNRev(revision);
    }
    else
    {
        return m_rev;
    }
}

void CRepositoryBar::SaveHistory()
{
    m_cbxUrl.SaveHistory();
}

bool CRepositoryBar::CRepositoryCombo::OnReturnKeyPressed()
{
    if (m_bar != nullptr)
        m_bar->GotoUrl();
    if (GetDroppedState())
        ShowDropDown(FALSE);
    return true;
}

void CRepositoryBar::OnCbnSelEndOK()
{
    if (!IsWindowVisible())
        return;
    int idx = m_cbxUrl.GetCurSel();
    if (idx >= 0)
    {
        CString path, revision;
        m_cbxUrl.GetLBText(idx, path);
        m_btnRevision.GetWindowText(revision);
        m_url = path;
        m_rev = revision;
        GotoUrl(m_url, m_rev);
    }
}

void CRepositoryBar::OnBnClicked()
{
    CString revision;

    m_tooltips.Pop();
    m_btnRevision.GetWindowText(revision);

    CRevisionDlg dlg(this);
    dlg.AllowWCRevs(false);
    dlg.SetLogPath(CTSVNPath(GetCurrentUrl()), GetCurrentRev());
    *static_cast<SVNRev*>(&dlg) = SVNRev(revision);

    if (dlg.DoModal() == IDOK)
    {
        revision = dlg.GetEnteredRevisionString();
        m_rev    = SVNRev(revision);
        m_btnRevision.SetWindowText(SVNRev(revision).ToString());
        GotoUrl();
    }
}

void CRepositoryBar::OnGoUp()
{
    CString sCurrentUrl = GetCurrentUrl();
    CString sNewUrl     = sCurrentUrl.Left(sCurrentUrl.ReverseFind('/'));
    if (sNewUrl.GetLength() >= m_pRepo->GetRepoRoot().GetLength())
        GotoUrl(sNewUrl, GetCurrentRev(), true);
}

void CRepositoryBar::SetFocusToURL() const
{
    m_cbxUrl.GetEditCtrl()->SetFocus();
}

void CRepositoryBar::OnDestroy()
{
    int idx = m_cbxUrl.GetCurSel();
    if (idx >= 0)
    {
        CString path, revision;
        m_cbxUrl.GetLBText(idx, path);
        m_btnRevision.GetWindowText(revision);
        m_url = path;
        m_rev = revision;
    }
    CReBarCtrl::OnDestroy();
}

BOOL CRepositoryBar::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return CReBarCtrl::PreTranslateMessage(pMsg);
}

ULONG CRepositoryBar::GetGestureStatus(CPoint /*ptTouch*/)
{
    return 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRepositoryBar::OnHistoryBack()
{
    if (m_pRepo)
        ::SendMessage(m_pRepo->GetHWND(), WM_COMMAND, MAKEWPARAM(ID_URL_HISTORY_BACK, 1), 0);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRepositoryBar::OnHistoryForward()
{
    if (m_pRepo)
        ::SendMessage(m_pRepo->GetHWND(), WM_COMMAND, MAKEWPARAM(ID_URL_HISTORY_FORWARD, 1), 0);
}

void CRepositoryBar::SetTheme(bool bDark) const
{
    CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), bDark);
}

////////////////////////////////////////////////////////////////////////////////

CRepositoryBarCnr::CRepositoryBarCnr(CRepositoryBar* repositoryBar)
    : m_pbarRepository(repositoryBar)
{
}

CRepositoryBarCnr::~CRepositoryBarCnr()
{
}

BEGIN_MESSAGE_MAP(CRepositoryBarCnr, CStatic)
    ON_WM_SIZE()
    ON_WM_GETDLGCODE()
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CRepositoryBarCnr, CStatic)

// ReSharper disable once CppMemberFunctionMayBeConst
void CRepositoryBarCnr::OnSize(UINT /* nType */, int cx, int cy)
{
    m_pbarRepository->MoveWindow(48, 0, cx, cy);
}

void CRepositoryBarCnr::DrawItem(LPDRAWITEMSTRUCT)
{
}

UINT CRepositoryBarCnr::OnGetDlgCode()
{
    return CStatic::OnGetDlgCode() | DLGC_WANTTAB;
}

void CRepositoryBarCnr::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_TAB)
    {
        CWnd* child = m_pbarRepository->GetWindow(GW_CHILD);
        if (child != nullptr)
        {
            child = child->GetWindow(GW_HWNDLAST);
            if (child != nullptr)
                child->SetFocus();
        }
    }

    CStatic::OnKeyDown(nChar, nRepCnt, nFlags);
}
