// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020-2021 - TortoiseSVN

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
#include "RevisionGraphDlg.h"
#include "SVN.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "ProgressDlg.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "RevGraphFilterDlg.h"
#include "RepositoryInfo.h"
#include "RevisionInRange.h"
#include "RemovePathsBySubString.h"
#include "DPIAware.h"
#include <strsafe.h>

#ifdef _DEBUG
// ReSharper disable once CppInconsistentNaming
#    define new DEBUG_NEW
#    undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

struct CToolBarData
{
    WORD wVersion;
    WORD wWidth;
    WORD wHeight;
    WORD wItemCount;
    //WORD aItems[wItemCount]

    WORD* items()
    {
        return reinterpret_cast<WORD*>(this + 1);
    }
};

IMPLEMENT_DYNAMIC(CRevisionGraphDlg, CResizableStandAloneDialog)
CRevisionGraphDlg::CRevisionGraphDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CRevisionGraphDlg::IDD, pParent)
    , m_bFetchLogs(true)
    , m_hAccel(nullptr)
    , m_fZoomFactor(DEFAULT_ZOOM)
    , m_bVisible(true)
    , m_themeCallbackId(0)
{
    // GDI+ initialization

    GdiplusStartupInput input;
    GdiplusStartup(&m_gdiPlusToken, &input, nullptr);

    // restore option state

    DWORD dwOpts = CRegStdDWORD(L"Software\\TortoiseSVN\\RevisionGraphOptions", 0x1ff199);
    m_graph.m_state.GetOptions()->SetRegistryFlags(dwOpts, 0x407fbf);

    m_szTip[0]  = 0;
    m_wszTip[0] = 0;
}

CRevisionGraphDlg::~CRevisionGraphDlg()
{
    CTheme::Instance().RemoveRegisteredCallback(m_themeCallbackId);

    // save option state

    CRegStdDWORD regOpts(L"Software\\TortoiseSVN\\RevisionGraphOptions", 1);
    regOpts = m_graph.m_state.GetOptions()->GetRegistryFlags();

    // GDI+ cleanup

    GdiplusShutdown(m_gdiPlusToken);
}

void CRevisionGraphDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRevisionGraphDlg, CResizableStandAloneDialog)
    ON_WM_SIZE()
    ON_WM_LBUTTONDOWN()
    ON_COMMAND(ID_VIEW_ZOOMIN, OnViewZoomin)
    ON_COMMAND(ID_VIEW_ZOOMOUT, OnViewZoomout)
    ON_COMMAND(ID_VIEW_ZOOM100, OnViewZoom100)
    ON_COMMAND(ID_VIEW_ZOOMHEIGHT, OnViewZoomHeight)
    ON_COMMAND(ID_VIEW_ZOOMWIDTH, OnViewZoomWidth)
    ON_COMMAND(ID_VIEW_ZOOMALL, OnViewZoomAll)
    ON_COMMAND(ID_MENUEXIT, OnMenuexit)
    ON_COMMAND(ID_MENUHELP, OnMenuhelp)
    ON_COMMAND(ID_VIEW_COMPAREHEADREVISIONS, OnViewCompareheadrevisions)
    ON_COMMAND(ID_VIEW_COMPAREREVISIONS, OnViewComparerevisions)
    ON_COMMAND(ID_VIEW_UNIFIEDDIFF, OnViewUnifieddiff)
    ON_COMMAND(ID_VIEW_UNIFIEDDIFFOFHEADREVISIONS, OnViewUnifieddiffofheadrevisions)
    ON_COMMAND(ID_FILE_SAVEGRAPHAS, OnFileSavegraphas)
    ON_COMMAND_EX(ID_VIEW_SHOWALLREVISIONS, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_GROUPBRANCHES, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_TOPDOWN, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_TOPALIGNTREES, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_SHOWHEAD, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_EXACTCOPYSOURCE, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_FOLDTAGS, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_REDUCECROSSLINES, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_REMOVEDELETEDONES, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_SHOWWCREV, OnToggleReloadOption)
    ON_COMMAND_EX(ID_VIEW_REMOVEUNCHANGEDBRANCHES, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_REMOVETAGS, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_SHOWWCMODIFICATION, OnToggleReloadOption)
    ON_COMMAND_EX(ID_VIEW_SHOWDIFFPATHS, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_SHOWTREESTRIPES, OnToggleRedrawOption)
    ON_CBN_SELCHANGE(ID_REVGRAPH_ZOOMCOMBO, OnChangeZoom)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
    ON_COMMAND(ID_VIEW_FILTER, OnViewFilter)
    ON_COMMAND(ID_VIEW_SHOWOVERVIEW, OnViewShowoverview)
    ON_WM_WINDOWPOSCHANGING()
    ON_MESSAGE(WM_DPICHANGED, OnDPIChanged)
END_MESSAGE_MAP()

BOOL CRevisionGraphDlg::InitializeToolbar()
{
    // set up the toolbar
    // add the tool bar to the dialog
    m_toolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_WRAPABLE | TBSTYLE_TRANSPARENT | CBRS_SIZE_DYNAMIC);

    // LoadToolBar() asserts in debug mode because the bitmap
    // fails to load. That's not a problem because we load the bitmap
    // further down manually.
    // but the assertion is ugly, so we load the button resource here
    // manually as well and call SetButtons().
    HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(IDR_REVGRAPHBAR), RT_TOOLBAR);
    HRSRC     hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(IDR_REVGRAPHBAR), RT_TOOLBAR);
    if (hRsrc == nullptr)
        return FALSE;

    HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
    if (hGlobal == nullptr)
        return FALSE;

    auto pData = static_cast<CToolBarData*>(LockResource(hGlobal));
    if (pData == nullptr)
        return FALSE;
    ASSERT(pData->wVersion == 1);

    auto pItems = std::make_unique<UINT[]>(pData->wItemCount);
    for (int i = 0; i < pData->wItemCount; i++)
        pItems[i] = pData->items()[i];
    m_toolBar.SetButtons(pItems.get(), pData->wItemCount);

    UnlockResource(hGlobal);
    FreeResource(hGlobal);

    m_toolBar.ShowWindow(SW_SHOW);
    m_toolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);

    // toolbars aren't true-color without some tweaking:
    {
        CImageList cImageList;
        CBitmap    cBitmap;
        BITMAP     bmBitmap;

        // load the toolbar with the dimensions of the bitmap itself
        cBitmap.Attach(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_REVGRAPHBAR),
                                 IMAGE_BITMAP, 0, 0,
                                 LR_DEFAULTSIZE | LR_CREATEDIBSECTION));
        cBitmap.GetBitmap(&bmBitmap);
        cBitmap.DeleteObject();
        // now load the toolbar again, but this time with the dpi-scaled dimensions
        // note: we could just load it once and then resize the bitmap, but
        // that's not faster. So loading it again is what we do.
        cBitmap.Attach(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_REVGRAPHBAR),
                                 IMAGE_BITMAP,
                                 CDPIAware::Instance().Scale(GetSafeHwnd(), bmBitmap.bmWidth),
                                 CDPIAware::Instance().Scale(GetSafeHwnd(), bmBitmap.bmHeight),
                                 LR_CREATEDIBSECTION));
        cBitmap.GetBitmap(&bmBitmap);

        CSize      cSize(bmBitmap.bmWidth, bmBitmap.bmHeight);
        int        nNbBtn  = cSize.cx / CDPIAware::Instance().Scale(GetSafeHwnd(), 20);
        RGBTRIPLE* rgb     = static_cast<RGBTRIPLE*>(bmBitmap.bmBits);
        COLORREF   rgbMask = RGB(rgb[0].rgbtRed, rgb[0].rgbtGreen, rgb[0].rgbtBlue);

        cImageList.Create(CDPIAware::Instance().Scale(GetSafeHwnd(), 20), cSize.cy, ILC_COLOR32 | ILC_MASK | ILC_HIGHQUALITYSCALE, nNbBtn, 0);
        cImageList.Add(&cBitmap, rgbMask);
        // set the sizes of the button and images:
        // note: buttonX must be 7 pixels more than imageX, and buttonY must be 6 pixels more than imageY.
        // See the source of SetSizes().
        m_toolBar.SetSizes(CSize(CDPIAware::Instance().Scale(GetSafeHwnd(), 27), CDPIAware::Instance().Scale(GetSafeHwnd(), 26)),
                           CSize(CDPIAware::Instance().Scale(GetSafeHwnd(), 20), CDPIAware::Instance().Scale(GetSafeHwnd(), 20)));
        m_toolBar.SendMessage(TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(cImageList.m_hImageList));
        cImageList.Detach();
        cBitmap.Detach();
    }

    RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

#define SNAP_WIDTH CDPIAware::Instance().Scale(GetSafeHwnd(), 60) //the width of the combo box
    // set up the ComboBox control as a snap mode select box
    // First get the index of the placeholders position in the toolbar
    int zoomComboIndex = 0;
    while (m_toolBar.GetItemID(zoomComboIndex) != ID_REVGRAPH_ZOOMCOMBO)
        zoomComboIndex++;

    // next convert that button to a separator and get its position
    m_toolBar.SetButtonInfo(zoomComboIndex, ID_REVGRAPH_ZOOMCOMBO, TBBS_SEPARATOR,
                            SNAP_WIDTH);
    RECT rect;
    m_toolBar.GetItemRect(zoomComboIndex, &rect);

    // expand the rectangle to allow the combo box room to drop down
    rect.top += CDPIAware::Instance().Scale(GetSafeHwnd(), 3);
    rect.bottom += CDPIAware::Instance().Scale(GetSafeHwnd(), 200);

    // then create the combo box and show it
    if (!m_toolBar.m_zoomCombo.CreateEx(WS_EX_RIGHT, WS_CHILD | WS_VISIBLE | CBS_AUTOHSCROLL | CBS_DROPDOWN,
                                        rect, &m_toolBar, ID_REVGRAPH_ZOOMCOMBO))
    {
        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Failed to create combo-box\n");
        return FALSE;
    }
    m_toolBar.m_zoomCombo.ShowWindow(SW_SHOW);

    // fill the combo box

    const wchar_t* texts[] = {L"5%", L"10%", L"20%", L"40%", L"50%", L"75%", L"100%", L"200%",
                              nullptr};

    COMBOBOXEXITEM cbei = {0};
    cbei.mask           = CBEIF_TEXT;

    for (const wchar_t** text = texts; *text != nullptr; ++text)
    {
        cbei.pszText = const_cast<wchar_t*>(*text);
        m_toolBar.m_zoomCombo.InsertItem(&cbei);
    }

    m_toolBar.m_zoomCombo.SetCurSel(1);

    UpdateOptionAvailability();

    return TRUE;
}

BOOL CRevisionGraphDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
        [this]() {
            SetTheme(CTheme::Instance().IsDarkTheme());
        });

    EnableToolTips();

    // begin background operation

    StartWorkerThread();

    // set up the status bar
    m_statusBar.Create(WS_CHILD | WS_VISIBLE | SBT_OWNERDRAW,
                       CRect(0, 0, 0, 0), this, 1);
    int strPartDim[2] = {CDPIAware::Instance().Scale(GetSafeHwnd(), 120), -1};
    m_statusBar.SetParts(2, strPartDim);

    if (InitializeToolbar() != TRUE)
        return FALSE;

    m_pTaskbarList.Release();
    m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);

    CSyncPointer<CAllRevisionGraphOptions>
        options(m_graph.m_state.GetOptions());

    for (size_t i = 0; i < options->count(); ++i)
        if ((*options)[i]->CommandID() != 0)
            SetOption((*options)[i]->CommandID());

    CMenu* pMenu = GetMenu();
    if (pMenu)
    {
        CRegDWORD reg = CRegDWORD(L"Software\\TortoiseSVN\\ShowRevGraphOverview", FALSE);
        m_graph.SetShowOverview(static_cast<DWORD>(reg) != FALSE);
        pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | (static_cast<DWORD>(reg) ? MF_CHECKED : 0));
        int tbstate = m_toolBar.GetToolBarCtrl().GetState(ID_VIEW_SHOWOVERVIEW);
        m_toolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbstate | (static_cast<DWORD>(reg) ? TBSTATE_CHECKED : 0));
    }

    m_hAccel = LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_ACC_REVISIONGRAPH));

    CRect graphRect = GetGraphRect();
    m_graph.Init(this, &graphRect);
    m_graph.SetOwner(this);
    m_graph.UpdateWindow();
    DoZoom(DEFAULT_ZOOM);

    EnableSaveRestore(L"RevisionGraphDlg");
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));

    __super::SetTheme(CTheme::Instance().IsDarkTheme());
    SetTheme(CTheme::Instance().IsDarkTheme());

    return TRUE; // return TRUE unless you set the focus to a control
}

bool CRevisionGraphDlg::UpdateData()
{
    CoInitialize(nullptr);

    if (m_bFetchLogs)
    {
        CProgressDlg progress;
        progress.SetTitle(IDS_REVGRAPH_PROGTITLE);
        progress.SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
        progress.SetTime();
        progress.SetProgress(0, 100);
        if (m_pTaskbarList)
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
            m_pTaskbarList->SetProgressValue(m_hWnd, 0, 100);
        }

        svn_revnum_t pegRev = m_graph.m_pegRev.IsNumber()
                                  ? static_cast<svn_revnum_t>(m_graph.m_pegRev)
                                  : static_cast<svn_revnum_t>(-1);

        if (!m_graph.FetchRevisionData(m_graph.m_sPath, pegRev, &progress, m_pTaskbarList, m_hWnd))
        {
            // only show the error dialog if we're not in hidden mode
            if (m_bVisible)
            {
                TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), m_graph.m_state.GetLastErrorMessage(), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
            }
        }

        progress.Stop();
        if (m_pTaskbarList)
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
        }

        m_bFetchLogs = false; // we've got the logs, no need to fetch them a second time
    }

    // standard plus user settings

    if (m_graph.AnalyzeRevisionData())
    {
        UpdateStatusBar();
        UpdateOptionAvailability();
    }

    CoUninitialize();
    if (::IsWindow(m_graph))
        m_graph.PostMessage(CRevisionGraphWnd::WM_WORKERTHREADDONE, 0, 0);

    return true;
}

void CRevisionGraphDlg::SetTheme(bool bDark) const
{
    DarkModeHelper::Instance().AllowDarkModeForWindow(m_graph.GetSafeHwnd(), bDark);
    DarkModeHelper::Instance().AllowDarkModeForWindow(m_statusBar.GetSafeHwnd(), bDark);
    DarkModeHelper::Instance().AllowDarkModeForWindow(m_toolBar.GetSafeHwnd(), bDark);

    SetWindowTheme(m_graph.GetSafeHwnd(), L"Explorer", nullptr);
    SetWindowTheme(m_statusBar.GetSafeHwnd(), L"Explorer", nullptr);
    SetWindowTheme(m_toolBar.GetSafeHwnd(), L"Explorer", nullptr);
}

void CRevisionGraphDlg::OnSize(UINT nType, int cx, int cy)
{
    __super::OnSize(nType, cx, cy);
    CRect rect;
    GetClientRect(&rect);
    if (IsWindow(m_toolBar))
    {
        RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
    }
    if (IsWindow(m_statusBar))
    {
        CRect statusBarRect;
        m_statusBar.GetClientRect(&statusBarRect);
        statusBarRect.top = rect.bottom - statusBarRect.top + statusBarRect.bottom;
        m_statusBar.MoveWindow(&statusBarRect);
    }
    if (IsWindow(m_graph))
    {
        m_graph.MoveWindow(GetGraphRect());
    }
}

BOOL CRevisionGraphDlg::PreTranslateMessage(MSG* pMsg)
{
#define SCROLL_STEP 20
    if (pMsg->message == WM_KEYDOWN)
    {
        int pos = 0;
        switch (pMsg->wParam)
        {
            case VK_UP:
                pos = m_graph.GetScrollPos(SB_VERT);
                m_graph.SetScrollPos(SB_VERT, pos - SCROLL_STEP);
                m_graph.Invalidate();
                break;
            case VK_DOWN:
                pos = m_graph.GetScrollPos(SB_VERT);
                m_graph.SetScrollPos(SB_VERT, pos + SCROLL_STEP);
                m_graph.Invalidate();
                break;
            case VK_LEFT:
                pos = m_graph.GetScrollPos(SB_HORZ);
                m_graph.SetScrollPos(SB_HORZ, pos - SCROLL_STEP);
                m_graph.Invalidate();
                break;
            case VK_RIGHT:
                pos = m_graph.GetScrollPos(SB_HORZ);
                m_graph.SetScrollPos(SB_HORZ, pos + SCROLL_STEP);
                m_graph.Invalidate();
                break;
            case VK_PRIOR:
                pos = m_graph.GetScrollPos(SB_VERT);
                m_graph.SetScrollPos(SB_VERT, pos - GetGraphRect().Height() / 2);
                m_graph.Invalidate();
                break;
            case VK_NEXT:
                pos = m_graph.GetScrollPos(SB_VERT);
                m_graph.SetScrollPos(SB_VERT, pos + GetGraphRect().Height() / 2);
                m_graph.Invalidate();
                break;
            case VK_F5:
                UpdateFullHistory();
                break;
        }
    }
    if ((m_hAccel) && (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST))
    {
        if (pMsg->wParam == VK_ESCAPE)
            if (m_graph.CancelMouseZoom())
                return TRUE;
        return TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
    }
    return __super::PreTranslateMessage(pMsg);
}

void CRevisionGraphDlg::DoZoom(float zoom)
{
    m_fZoomFactor = zoom;
    m_graph.DoZoom(zoom);
    UpdateZoomBox();
}

void CRevisionGraphDlg::OnViewZoomin()
{
    DoZoom(min(MAX_ZOOM, m_fZoomFactor / ZOOM_STEP));
}

void CRevisionGraphDlg::OnViewZoomout()
{
    DoZoom(max(MIN_ZOOM, m_fZoomFactor * ZOOM_STEP));
}

void CRevisionGraphDlg::OnViewZoom100()
{
    DoZoom(DEFAULT_ZOOM);
}

void CRevisionGraphDlg::OnViewZoomHeight()
{
    CRect graphRect  = m_graph.GetGraphRect();
    CRect windowRect = m_graph.GetWindowRect();

    float horzFact = (windowRect.Width() - 4.0f) / (4.0f + graphRect.Width());
    float vertFact = (windowRect.Height() - 4.0f) / (4.0f + graphRect.Height());
    if ((horzFact < vertFact) && (horzFact < MAX_ZOOM))
        vertFact = (windowRect.Height() - CDPIAware::Instance().Scale(GetSafeHwnd(), 20)) / (4.0f + graphRect.Height());

    DoZoom(min(MAX_ZOOM, vertFact));
}

void CRevisionGraphDlg::OnViewZoomWidth()
{
    // zoom the graph so that it is completely visible in the window
    CRect graphRect  = m_graph.GetGraphRect();
    CRect windowRect = m_graph.GetWindowRect();

    float horzFact = (windowRect.Width() - 4.0f) / (4.0f + graphRect.Width());
    float vertFact = (windowRect.Height() - 4.0f) / (4.0f + graphRect.Height());
    if ((vertFact < horzFact) && (vertFact < MAX_ZOOM))
        horzFact = (windowRect.Width() - CDPIAware::Instance().Scale(GetSafeHwnd(), 20)) / (4.0f + graphRect.Width());

    DoZoom(min(MAX_ZOOM, horzFact));
}

void CRevisionGraphDlg::OnViewZoomAll()
{
    // zoom the graph so that it is completely visible in the window
    CRect graphRect  = m_graph.GetGraphRect();
    CRect windowRect = m_graph.GetWindowRect();

    float horzFact = (windowRect.Width() - 4.0f) / (4.0f + graphRect.Width());
    float vertFact = (windowRect.Height() - 4.0f) / (4.0f + graphRect.Height());

    DoZoom(min(MAX_ZOOM, min(horzFact, vertFact)));
}

void CRevisionGraphDlg::OnMenuexit()
{
    if (!m_graph.IsUpdateJobRunning())
        EndDialog(IDOK);
}

void CRevisionGraphDlg::OnMenuhelp()
{
    OnHelp();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRevisionGraphDlg::OnViewCompareheadrevisions()
{
    m_graph.CompareRevs(true);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRevisionGraphDlg::OnViewComparerevisions()
{
    m_graph.CompareRevs(false);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRevisionGraphDlg::OnViewUnifieddiff()
{
    m_graph.UnifiedDiffRevs(false);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRevisionGraphDlg::OnViewUnifieddiffofheadrevisions()
{
    m_graph.UnifiedDiffRevs(true);
}

void CRevisionGraphDlg::UpdateFullHistory()
{
    m_graph.SetDlgTitle(false);

    SVN                        svn;
    LogCache::CRepositoryInfo& cachedProperties = svn.GetLogCachePool()->GetRepositoryInfo();
    CString                    root             = m_graph.m_state.GetRepositoryRoot();
    CString                    uuid             = m_graph.m_state.GetRepositoryUUID();

    cachedProperties.ResetHeadRevision(uuid, root);

    m_bFetchLogs = true;
    StartWorkerThread();
}

void CRevisionGraphDlg::SetOption(UINT controlID)
{
    CMenu* pMenu = GetMenu();
    if (pMenu == nullptr)
        return;

    int tbState = m_toolBar.GetToolBarCtrl().GetState(controlID);
    if (tbState != -1)
    {
        if (m_graph.m_state.GetOptions()->IsSelected(controlID))
        {
            pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_CHECKED);
            m_toolBar.GetToolBarCtrl().SetState(controlID, tbState | TBSTATE_CHECKED);
        }
        else
        {
            pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_UNCHECKED);
            m_toolBar.GetToolBarCtrl().SetState(controlID, tbState & (~TBSTATE_CHECKED));
        }
    }
}

BOOL CRevisionGraphDlg::ToggleOption(UINT controlID)
{
    // check request for validity

    if (m_graph.IsUpdateJobRunning())
    {
        // restore previous state

        int state = m_toolBar.GetToolBarCtrl().GetState(controlID);
        if (state & TBSTATE_CHECKED)
            state &= ~TBSTATE_CHECKED;
        else
            state |= TBSTATE_CHECKED;
        m_toolBar.GetToolBarCtrl().SetState(controlID, state);

        return FALSE;
    }

    CMenu* pMenu = GetMenu();
    if (pMenu == nullptr)
        return FALSE;

    // actually toggle the option

    int  tbstate = m_toolBar.GetToolBarCtrl().GetState(controlID);
    UINT state   = pMenu->GetMenuState(controlID, MF_BYCOMMAND);
    if (state & MF_CHECKED)
    {
        pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_UNCHECKED);
        m_toolBar.GetToolBarCtrl().SetState(controlID, tbstate & (~TBSTATE_CHECKED));
    }
    else
    {
        pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_CHECKED);
        m_toolBar.GetToolBarCtrl().SetState(controlID, tbstate | TBSTATE_CHECKED);
    }

    CSyncPointer<CAllRevisionGraphOptions>
        options(m_graph.m_state.GetOptions());
    if (((state & MF_CHECKED) != 0) == options->IsSelected(controlID))
        options->ToggleSelection(controlID);

    return TRUE;
}

BOOL CRevisionGraphDlg::OnToggleOption(UINT controlID)
{
    if (!ToggleOption(controlID))
        return FALSE;

    // re-process the data

    StartWorkerThread();

    return TRUE;
}

BOOL CRevisionGraphDlg::OnToggleReloadOption(UINT controlID)
{
    if (!m_graph.m_state.GetFetchedWCState())
        m_bFetchLogs = true;

    return OnToggleOption(controlID);
}

BOOL CRevisionGraphDlg::OnToggleRedrawOption(UINT controlID)
{
    if (!ToggleOption(controlID))
        return FALSE;

    m_graph.BuildPreview();
    Invalidate();

    return TRUE;
}

void CRevisionGraphDlg::StartWorkerThread()
{
    if (!m_graph.IsUpdateJobRunning())
        m_graph.updateJob = std::make_unique<CFuture<bool>>(this, &CRevisionGraphDlg::UpdateData);
}

void CRevisionGraphDlg::OnCancel()
{
    if (!m_graph.IsUpdateJobRunning())
        __super::OnCancel();
}

void CRevisionGraphDlg::OnOK()
{
    OnChangeZoom();
}

void CRevisionGraphDlg::OnFileSavegraphas()
{
    CString tempFile;
    int     filterIndex = 0;
    if (CAppUtils::FileOpenSave(tempFile, &filterIndex, IDS_REVGRAPH_SAVEPIC, IDS_PICTUREFILEFILTER, false, ::PathIsURL(m_graph.m_sPath) ? CString() : m_graph.m_sPath, m_hWnd))
    {
        // if the user doesn't specify a file extension, default to
        // wmf and add that extension to the filename. But only if the
        // user chose the 'pictures' filter. The filename isn't changed
        // if the 'All files' filter was chosen.
        CString extension;
        int     dotPos   = tempFile.ReverseFind('.');
        int     slashPos = tempFile.ReverseFind('\\');
        if (dotPos > slashPos)
            extension = tempFile.Mid(dotPos);
        if ((filterIndex == 1) && (extension.IsEmpty()))
        {
            extension = L".wmf";
            tempFile += extension;
        }
        m_graph.SaveGraphAs(tempFile);
    }
}

CRect CRevisionGraphDlg::GetGraphRect() const
{
    CRect rect;
    GetClientRect(&rect);

    CRect statusBarRect;
    m_statusBar.GetClientRect(&statusBarRect);
    rect.bottom -= statusBarRect.Height();

    CRect toolBarRect;
    m_toolBar.GetClientRect(&toolBarRect);
    rect.top += toolBarRect.Height();

    return rect;
}

void CRevisionGraphDlg::UpdateStatusBar()
{
    CString sFormat;
    sFormat.Format(IDS_REVGRAPH_STATUSBARURL, static_cast<LPCWSTR>(m_graph.m_sPath));
    m_statusBar.SetText(sFormat, 1, 0);
    sFormat.Format(IDS_REVGRAPH_STATUSBARNUMNODES, m_graph.m_state.GetNodeCount());
    m_statusBar.SetText(sFormat, 0, 0);
}

void CRevisionGraphDlg::OnChangeZoom()
{
    if (!IsWindow(m_graph.GetSafeHwnd()))
        return;
    CString      strItem;
    CComboBoxEx* pCBox = static_cast<CComboBoxEx*>(m_toolBar.GetDlgItem(ID_REVGRAPH_ZOOMCOMBO));
    pCBox->GetWindowText(strItem);
    if (strItem.IsEmpty())
        return;

    DoZoom(static_cast<float>((_wtof(strItem) / 100.0)));
}

void CRevisionGraphDlg::UpdateZoomBox() const
{
    CString      strText;
    CString      strItem;
    CComboBoxEx* pCBox = static_cast<CComboBoxEx*>(m_toolBar.GetDlgItem(ID_REVGRAPH_ZOOMCOMBO));
    pCBox->GetWindowText(strItem);
    strText.Format(L"%.0f%%", (m_fZoomFactor * 100.0));
    if (strText.Compare(strItem) != 0)
        pCBox->SetWindowText(strText);
}

BOOL CRevisionGraphDlg::OnToolTipNotify(UINT /*id*/, NMHDR* pNMHDR, LRESULT* pResult)
{
    // need to handle both ANSI and UNICODE versions of the message
    TOOLTIPTEXTA* pTTTA = reinterpret_cast<NMTTDISPINFOA*>(pNMHDR);
    TOOLTIPTEXTW* pTTTW = reinterpret_cast<NMTTDISPINFOW*>(pNMHDR);
    CString       strTipText;

    UINT_PTR nID = pNMHDR->idFrom;

    if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
    {
        // idFrom is actually the HWND of the tool
        nID = ::GetDlgCtrlID(reinterpret_cast<HWND>(nID));
    }

    if (nID != 0) // will be zero on a separator
    {
        strTipText.LoadString(static_cast<UINT>(nID));
    }

    *pResult = 0;
    if (strTipText.IsEmpty())
        return TRUE;

    if (strTipText.GetLength() >= MAX_TT_LENGTH)
        strTipText = strTipText.Left(MAX_TT_LENGTH);

    if (pNMHDR->code == TTN_NEEDTEXTA)
    {
        ::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);
        pTTTA->lpszText = m_szTip;
        WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength() + 1, nullptr, nullptr);
    }
    else
    {
        ::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);
        StringCchCopy(m_wszTip, _countof(m_wszTip), strTipText);
        pTTTW->lpszText = m_wszTip;
    }
    // bring the tooltip window above other pop up windows
    ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);
    return TRUE; // message was handled
}

void CRevisionGraphDlg::OnViewFilter()
{
    CSyncPointer<CAllRevisionGraphOptions>
        options(m_graph.m_state.GetOptions());

    CRevisionInRange* revisionRange = options->GetOption<CRevisionInRange>();
    svn_revnum_t      head          = m_graph.m_state.GetHeadRevision();
    svn_revnum_t      lowerLimit    = revisionRange->GetLowerLimit();
    svn_revnum_t      upperLimit    = revisionRange->GetUpperLimit();

    CRemovePathsBySubString* pathFilter = options->GetOption<CRemovePathsBySubString>();

    CRevGraphFilterDlg dlg;
    dlg.SetMaxRevision(head);
    dlg.SetFilterString(m_sFilter);
    dlg.SetRemoveSubTrees(pathFilter->GetRemoveSubTrees());
    dlg.SetRevisionRange(min(head, lowerLimit == -1 ? 1 : lowerLimit),
                         min(head, upperLimit == -1 ? head : upperLimit));

    if (dlg.DoModal() == IDOK)
    {
        // user pressed OK to dismiss the dialog, which means
        // we have to accept the new filter settings and apply them
        svn_revnum_t minRev, maxRev;
        dlg.GetRevisionRange(minRev, maxRev);
        m_sFilter = dlg.GetFilterString();

        revisionRange->SetLowerLimit(minRev);
        revisionRange->SetUpperLimit(maxRev == head ? static_cast<revision_t>(NO_REVISION) : maxRev);

        pathFilter->SetRemoveSubTrees(dlg.GetRemoveSubTrees());
        std::set<std::string>& filterPaths = pathFilter->GetFilterPaths();
        int                    index       = 0;
        filterPaths.clear();

        CString path = m_sFilter.Tokenize(L"*", index);
        while (!path.IsEmpty())
        {
            filterPaths.insert(CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(path)));
            path = m_sFilter.Tokenize(L"*", index);
        }

        // update menu & toolbar

        CMenu* pMenu   = GetMenu();
        int    tbState = m_toolBar.GetToolBarCtrl().GetState(ID_VIEW_FILTER);
        if (revisionRange->IsActive() || pathFilter->IsActive())
        {
            if (pMenu != nullptr)
                pMenu->CheckMenuItem(ID_VIEW_FILTER, MF_BYCOMMAND | MF_CHECKED);
            m_toolBar.GetToolBarCtrl().SetState(ID_VIEW_FILTER, tbState | TBSTATE_CHECKED);
        }
        else
        {
            if (pMenu != nullptr)
                pMenu->CheckMenuItem(ID_VIEW_FILTER, MF_BYCOMMAND | MF_UNCHECKED);
            m_toolBar.GetToolBarCtrl().SetState(ID_VIEW_FILTER, tbState & (~TBSTATE_CHECKED));
        }

        // re-run query

        StartWorkerThread();
    }
}

void CRevisionGraphDlg::OnViewShowoverview()
{
    CMenu* pMenu = GetMenu();
    if (pMenu == nullptr)
        return;
    int  tbState = m_toolBar.GetToolBarCtrl().GetState(ID_VIEW_SHOWOVERVIEW);
    UINT state   = pMenu->GetMenuState(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND);
    if (state & MF_CHECKED)
    {
        pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | MF_UNCHECKED);
        m_toolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbState & (~TBSTATE_CHECKED));
        m_graph.SetShowOverview(false);
    }
    else
    {
        pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | MF_CHECKED);
        m_toolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbState | TBSTATE_CHECKED);
        m_graph.SetShowOverview(true);
    }

    CRegDWORD reg(L"Software\\TortoiseSVN\\ShowRevGraphOverview", FALSE);
    reg = m_graph.GetShowOverview();
    m_graph.Invalidate(FALSE);
}

void CRevisionGraphDlg::UpdateOptionAvailability(UINT id, bool available) const
{
    CMenu* pMenu = GetMenu();
    if (pMenu == nullptr)
        return;

    pMenu->EnableMenuItem(id, available ? MF_ENABLED : MF_GRAYED);

    int tbState    = m_toolBar.GetToolBarCtrl().GetState(id);
    int newTbstate = available
                         ? tbState | TBSTATE_ENABLED
                         : tbState & ~TBSTATE_ENABLED;

    if (tbState != newTbstate)
        m_toolBar.GetToolBarCtrl().SetState(id, newTbstate);
}

void CRevisionGraphDlg::UpdateOptionAvailability() const
{
    bool multipleTrees = m_graph.m_state.GetTreeCount() > 1;
    bool isWCPath      = !CTSVNPath(m_graph.m_sPath).IsUrl();

    UpdateOptionAvailability(ID_VIEW_TOPALIGNTREES, multipleTrees);
    UpdateOptionAvailability(ID_VIEW_SHOWTREESTRIPES, multipleTrees);
    UpdateOptionAvailability(ID_VIEW_SHOWWCREV, isWCPath);
    UpdateOptionAvailability(ID_VIEW_SHOWWCMODIFICATION, isWCPath);
}

void CRevisionGraphDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    if (!m_bVisible)
    {
        lpwndpos->flags &= ~SWP_SHOWWINDOW;
    }
    CResizableStandAloneDialog::OnWindowPosChanging(lpwndpos);
}

LRESULT CRevisionGraphDlg::OnDPIChanged(WPARAM wParam, LPARAM lParam)
{
    CDPIAware::Instance().Invalidate();
    m_toolBar.CloseWindow();
    m_toolBar.DestroyWindow();
    m_graph.DeleteFonts();
    InitializeToolbar();
    return __super::OnDPIChanged(wParam, lParam);
}
