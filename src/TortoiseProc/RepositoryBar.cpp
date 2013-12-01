// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013 - TortoiseSVN

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
#include "RepositoryBar.h"
#include "RevisionDlg.h"
#include "SVNInfo.h"
#include "SVN.h"
#include "WaitCursorEx.h"

#define IDC_URL_COMBO     10000
#define IDC_REVISION_BTN  10001
#define IDC_UP_BTN        10002
#define IDC_BACK_BTN      10003
#define IDC_FORWARD_BTN   10004

IMPLEMENT_DYNAMIC(CRepositoryBar, CReBarCtrl)

CRepositoryBar::CRepositoryBar()
    : m_cbxUrl(this)
    , m_pRepo(NULL)
{
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

bool CRepositoryBar::Create(CWnd* parent, UINT id, bool in_dialog)
{
    CRect rect;
    ASSERT(parent != 0);
    parent->GetWindowRect(&rect);

    DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
                | CCS_TOP | RBS_AUTOSIZE | RBS_VARHEIGHT;

    DWORD style_ex = WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;

    if (in_dialog)
    {
        style |= CCS_NODIVIDER;
    }
    else
    {
        style |= RBS_BANDBORDERS;
    }

    if (CReBarCtrl::CreateEx(style_ex, style, CRect(0,0,200,100), parent, id))
    {
        CFont *font = parent->GetFont();
        CString temp;

        REBARINFO rbi;
        SecureZeroMemory(&rbi, sizeof rbi);
        rbi.cbSize = sizeof rbi;
        rbi.fMask  = 0;
        rbi.himl   = (HIMAGELIST)0;

        if (!this->SetBarInfo(&rbi))
            return false;

        REBARBANDINFO rbbi;
        SecureZeroMemory(&rbbi, sizeof rbbi);
        rbbi.cbSize = REBARBANDINFO_V6_SIZE;
        rbbi.fMask  = RBBIM_TEXT | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
        rbbi.fStyle = RBBS_NOGRIPPER | RBBS_FIXEDBMP;

        if (in_dialog)
            rbbi.fMask |= RBBIM_COLORS;
        else
            rbbi.fMask |= RBBS_CHILDEDGE;
        int bandpos = 0;
        // Create the "Back" button control to be added
        rect = CRect(0, 0, 24, 24);
        m_btnBack.Create(_T("BACK"), WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_ICON, rect, this, IDC_BACK_BTN);
        m_btnBack.SetImage((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_BACKWARD), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
        m_btnBack.SetWindowText(_T(""));
        m_btnBack.Invalidate();
        rbbi.lpText     = _T("");
        rbbi.hwndChild  = m_btnBack.m_hWnd;
        rbbi.clrFore    = ::GetSysColor(COLOR_WINDOWTEXT);
        rbbi.clrBack    = ::GetSysColor(COLOR_BTNFACE);
        rbbi.cx         = rect.right - rect.left;
        rbbi.cxMinChild = rect.right - rect.left;
        rbbi.cyMinChild = rect.bottom - rect.top;
        if (!InsertBand(bandpos++, &rbbi))
            return false;
        // Create the "Forward" button control to be added
        rect = CRect(0, 0, 24, 24);
        m_btnForward.Create(_T("FORWARD"), WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_ICON, rect, this, IDC_FORWARD_BTN);
        m_btnForward.SetImage((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_FORWARD), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
        m_btnForward.SetWindowText(_T(""));
        m_btnForward.Invalidate();
        rbbi.lpText     = _T("");
        rbbi.hwndChild  = m_btnForward.m_hWnd;
        rbbi.clrFore    = ::GetSysColor(COLOR_WINDOWTEXT);
        rbbi.clrBack    = ::GetSysColor(COLOR_BTNFACE);
        rbbi.cx         = rect.right - rect.left;
        rbbi.cxMinChild = rect.right - rect.left;
        rbbi.cyMinChild = rect.bottom - rect.top;
        if (!InsertBand(bandpos++, &rbbi))
            return false;

        // Create the "URL" combo box control to be added
        rect = CRect(0, 0, 100, 400);
        m_cbxUrl.Create(WS_CHILD | WS_TABSTOP | CBS_DROPDOWN, rect, this, IDC_URL_COMBO);
        m_cbxUrl.SetURLHistory(true, false);
        m_cbxUrl.SetFont(font);
        m_cbxUrl.LoadHistory(_T("Software\\TortoiseSVN\\History\\repoURLS"), _T("url"));
        temp.LoadString(IDS_REPO_BROWSEURL);
        rbbi.lpText     = const_cast<LPTSTR>((LPCTSTR)temp);
        rbbi.hwndChild  = m_cbxUrl.m_hWnd;
        rbbi.clrFore    = ::GetSysColor(COLOR_WINDOWTEXT);
        rbbi.clrBack    = ::GetSysColor(COLOR_BTNFACE);
        rbbi.cx         = rect.right - rect.left;
        rbbi.cxMinChild = rect.right - rect.left;
        rbbi.cyMinChild = m_cbxUrl.GetItemHeight(-1) + 10;
        if (!InsertBand(bandpos++, &rbbi))
            return false;

        // Reposition the combobox for correct redrawing
        m_cbxUrl.GetWindowRect(rect);
        m_cbxUrl.MoveWindow(rect.left, rect.top, rect.Width(), 300);

        // Create the "Up" button control to be added
        rect = CRect(0, 0, 24, m_cbxUrl.GetItemHeight(-1) + 8);
        m_btnUp.Create(_T("UP"), WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_ICON, rect, this, IDC_UP_BTN);
        m_btnUp.SetImage((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_UP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
        m_btnUp.SetWindowText(_T(""));
        m_btnUp.Invalidate();
        rbbi.lpText     = _T("");
        rbbi.hwndChild  = m_btnUp.m_hWnd;
        rbbi.clrFore    = ::GetSysColor(COLOR_WINDOWTEXT);
        rbbi.clrBack    = ::GetSysColor(COLOR_BTNFACE);
        rbbi.cx         = rect.right - rect.left;
        rbbi.cxMinChild = rect.right - rect.left;
        rbbi.cyMinChild = rect.bottom - rect.top;
        if (!InsertBand(bandpos++, &rbbi))
            return false;

        // Create the "Revision" button control to be added
        rect = CRect(0, 0, 60, m_cbxUrl.GetItemHeight(-1) + 10);
        m_btnRevision.Create(_T("HEAD"), WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, rect, this, IDC_REVISION_BTN);
        m_btnRevision.SetFont(font);
        temp.LoadString(IDS_REPO_BROWSEREV);
        rbbi.lpText     = const_cast<LPTSTR>((LPCTSTR)temp);
        rbbi.hwndChild  = m_btnRevision.m_hWnd;
        rbbi.clrFore    = ::GetSysColor(COLOR_WINDOWTEXT);
        rbbi.clrBack    = ::GetSysColor(COLOR_BTNFACE);
        rbbi.cx         = rect.right - rect.left;
        rbbi.cxMinChild = rect.right - rect.left;
        rbbi.cyMinChild = rect.bottom - rect.top;
        if (!InsertBand(bandpos++, &rbbi))
            return false;

        MaximizeBand(2);

        m_tooltips.Create(this);
        m_tooltips.AddTool(&m_btnUp, IDS_REPOBROWSE_TT_UPFOLDER);
        m_tooltips.AddTool(&m_btnBack, IDS_REPOBROWSE_TT_BACKWARD);
        m_tooltips.AddTool(&m_btnForward, IDS_REPOBROWSE_TT_FORWARD);

        return true;
    }

    return false;
}

void CRepositoryBar::OnCbenDragbeginUrlcombo(NMHDR *pNMHDR, LRESULT *pResult)
{
    if (m_pRepo)
        m_pRepo->OnCbenDragbeginUrlcombo(pNMHDR, pResult);
}

void CRepositoryBar::ShowUrl(const CString& url, SVNRev rev)
{
    if (url.Find('?')>=0)
    {
        m_url = url.Left(url.Find('?'));
        m_rev = SVNRev(url.Mid(url.Find('?')+1));
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
        sTTText.Format(IDS_REPOBROWSE_TT_HEADREV, (LPCTSTR)m_headRev.ToString());
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

void CRepositoryBar::GotoUrl(const CString& url, SVNRev rev, bool bAlreadyChecked /* = false */)
{
    if (m_pRepo && m_pRepo->IsThreadRunning())
    {
        return;
    }
    CString new_url = url;
    SVNRev new_rev = rev;
    CWaitCursorEx wait;

    new_url.TrimRight('/');
    if (new_url.IsEmpty())
    {
        new_url = GetCurrentUrl();
        new_rev = GetCurrentRev();
        new_url.TrimRight('/');
    }
    if (new_url.Find('?')>=0)
    {
        new_rev = SVNRev(new_url.Mid(new_url.Find('?')+1));
        if (!new_rev.IsValid())
            new_rev = SVNRev::REV_HEAD;
        new_url = new_url.Left(new_url.Find('?'));
    }

    if (m_pRepo)
    {
        SVNRev r = new_rev;
        m_headRev = SVNRev();
        m_pRepo->ChangeToUrl(new_url, r, bAlreadyChecked);
        if (new_rev.IsHead() && !r.IsHead())
            m_headRev = r;
        if (!m_headRev.IsValid())
        {
            SVN svn;
            m_headRev = svn.GetHEADRevision(CTSVNPath(new_url));
        }
    }
    ShowUrl(new_url, new_rev);
}

void CRepositoryBar::SetRevision(SVNRev rev)
{
    m_btnRevision.SetWindowText(rev.ToString());
    if (m_headRev.IsValid())
    {
        CString sTTText;
        sTTText.Format(IDS_REPOBROWSE_TT_HEADREV, (LPCTSTR)m_headRev.ToString());
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
    sTTText.Format(IDS_REPOBROWSE_TT_HEADREV, (LPCTSTR)m_headRev.ToString());
    m_tooltips.AddTool(&m_btnRevision, sTTText);
}

CString CRepositoryBar::GetCurrentUrl() const
{
    if (m_cbxUrl.m_hWnd != 0)
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
    if (m_cbxUrl.m_hWnd != 0)
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
    if (m_bar != 0)
        m_bar->GotoUrl();
    if (GetDroppedState())
        ShowDropDown(FALSE);
    return true;
}


void CRepositoryBar::OnCbnSelEndOK()
{
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
    *((SVNRev*)&dlg) = SVNRev(revision);

    if (dlg.DoModal() == IDOK)
    {
        revision = dlg.GetEnteredRevisionString();
        m_rev = SVNRev(revision);
        m_btnRevision.SetWindowText(SVNRev(revision).ToString());
        GotoUrl();
    }
}

void CRepositoryBar::OnGoUp()
{
    CString sCurrentUrl = GetCurrentUrl();
    CString sNewUrl = sCurrentUrl.Left(sCurrentUrl.ReverseFind('/'));
    if (sNewUrl.GetLength() >= m_pRepo->GetRepoRoot().GetLength())
        GotoUrl(sNewUrl, GetCurrentRev(), true);
}

void CRepositoryBar::SetFocusToURL()
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

void CRepositoryBar::OnHistoryBack()
{
    if (m_pRepo)
        ::SendMessage(m_pRepo->GetHWND(), WM_COMMAND, MAKEWPARAM(ID_URL_HISTORY_BACK, 1), 0);
}

void CRepositoryBar::OnHistoryForward()
{
    if (m_pRepo)
        ::SendMessage(m_pRepo->GetHWND(), WM_COMMAND, MAKEWPARAM(ID_URL_HISTORY_FORWARD, 1), 0);
}

////////////////////////////////////////////////////////////////////////////////

CRepositoryBarCnr::CRepositoryBarCnr(CRepositoryBar *repository_bar) :
    m_pbarRepository(repository_bar)
{
}

CRepositoryBarCnr::~CRepositoryBarCnr()
{
}

BEGIN_MESSAGE_MAP(CRepositoryBarCnr, CStatic)
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_GETDLGCODE()
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CRepositoryBarCnr, CStatic)

BOOL CRepositoryBarCnr::OnEraseBkgnd(CDC* /* pDC */)
{
    return TRUE;
}

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
        CWnd *child = m_pbarRepository->GetWindow(GW_CHILD);
        if (child != 0)
        {
            child = child->GetWindow(GW_HWNDLAST);
            if (child != 0)
                child->SetFocus();
        }
    }

    CStatic::OnKeyDown(nChar, nRepCnt, nFlags);
}

