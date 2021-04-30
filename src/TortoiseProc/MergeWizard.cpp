// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2015, 2020-2021 - TortoiseSVN

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

#include "AppUtils.h"
#include "SVN.h"
#include "registry.h"
#include "TaskbarUUID.h"
#include "Theme.h"
#include "DarkModeHelper.h"

#define BOTTOMMARG 48

const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

IMPLEMENT_DYNAMIC(CMergeWizard, CResizableSheetEx)

CMergeWizard::CMergeWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    : CResizableSheetEx(nIDCaption, pParentWnd, iSelectPage)
    , m_nRevRangeMerge(MERGEWIZARD_REVRANGE)
    , m_bReverseMerge(FALSE)
    , m_bReintegrate(false)
    , m_bRecordOnly(FALSE)
    , m_bIgnoreAncestry(FALSE)
    , m_depth(svn_depth_unknown)
    , m_bIgnoreEOL(FALSE)
    , m_ignoreSpaces(svn_diff_file_ignore_space_none)
    , m_bForce(FALSE)
    , m_bAllowMixed(FALSE)
    , m_firstPageActivation(true)
{
    SetWizardMode();
    AddPage(&m_page1);
    AddPage(&m_tree);
    AddPage(&m_revRange);
    AddPage(&m_options);

    switch (iSelectPage)
    {
        case 1:
            m_nRevRangeMerge = MERGEWIZARD_TREE;
            break;
        case 2:
            m_nRevRangeMerge = MERGEWIZARD_REVRANGE;
            break;
    }

    m_psh.dwFlags |= PSH_WIZARD97 | PSP_HASHELP | PSH_HEADER;
    m_psh.pszbmHeader = MAKEINTRESOURCE(IDB_MERGEWIZARDTITLE);

    m_psh.hInstance = AfxGetResourceHandle();
    m_hIcon         = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMergeWizard::~CMergeWizard()
{
}

BEGIN_MESSAGE_MAP(CMergeWizard, CResizableSheetEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_ERASEBKGND()
    ON_REGISTERED_MESSAGE(TaskBarButtonCreated, OnTaskbarButtonCreated)
    ON_WM_SYSCOMMAND()
    ON_COMMAND(IDCANCEL, &CMergeWizard::OnCancel)
END_MESSAGE_MAP()

// CMergeWizard message handlers

BOOL CMergeWizard::OnInitDialog()
{
    BOOL bResult = CResizableSheetEx::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    SetIcon(m_hIcon, TRUE);  // Set big icon
    SetIcon(m_hIcon, FALSE); // Set small icon

    SVN svn;
    m_url   = svn.GetURLFromPath(m_wcPath);
    m_sUuid = svn.GetUUIDFromPath(m_wcPath);

    if (m_aeroControls.AeroDialogsEnabled())
    {
        MARGINS mArgs;
        mArgs.cxLeftWidth    = 0;
        mArgs.cyTopHeight    = 0;
        mArgs.cxRightWidth   = 0;
        mArgs.cyBottomHeight = BOTTOMMARG;
        DwmExtendFrameIntoClientArea(m_hWnd, &mArgs);
        ShowGrip(false);
        m_aeroControls.SubclassOkCancelHelp(this);
        m_aeroControls.SubclassControl(this, ID_WIZFINISH);
        m_aeroControls.SubclassControl(this, ID_WIZBACK);
        m_aeroControls.SubclassControl(this, ID_WIZNEXT);
    }

    if ((m_pParentWnd == nullptr) && (GetExplorerHWND()))
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"MergeWizard");

    DarkModeHelper::Instance().AllowDarkModeForApp(CTheme::Instance().IsDarkTheme());
    CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

    return bResult;
}

void CMergeWizard::OnSysCommand(UINT nID, LPARAM lParam)
{
    switch (nID & 0xFFF0)
    {
        case SC_CLOSE:
        {
            CMergeWizardBasePage* page = static_cast<CMergeWizardBasePage*>(GetActivePage());
            if (page && !page->OkToCancel())
                break;
        }
            [[fallthrough]];
        default:
            __super::OnSysCommand(nID, lParam);
            break;
    }
}

void CMergeWizard::OnCancel()
{
    CMergeWizardBasePage* page = static_cast<CMergeWizardBasePage*>(GetActivePage());
    if ((page == nullptr) || page->OkToCancel())
        Default();
}
void CMergeWizard::SaveMode() const
{
    CRegDWORD regMergeWizardMode(L"Software\\TortoiseSVN\\MergeWizardMode", 0);
    if (static_cast<DWORD>(regMergeWizardMode))
    {
        switch (m_nRevRangeMerge)
        {
            case IDD_MERGEWIZARD_REVRANGE:
                regMergeWizardMode = 2;
                break;
            case IDD_MERGEWIZARD_TREE:
                regMergeWizardMode = 1;
                break;
            default:
                regMergeWizardMode = 0;
                break;
        }
    }
}

LRESULT CMergeWizard::GetSecondPage() const
{
    switch (m_nRevRangeMerge)
    {
        case MERGEWIZARD_REVRANGE:
            return IDD_MERGEWIZARD_REVRANGE;
        case MERGEWIZARD_TREE:
            return IDD_MERGEWIZARD_TREE;
    }
    return IDD_MERGEWIZARD_REVRANGE;
}

void CMergeWizard::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int   cxIcon = GetSystemMetrics(SM_CXICON);
        int   cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CResizableSheetEx::OnPaint();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
HCURSOR CMergeWizard::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

BOOL CMergeWizard::OnEraseBkgnd(CDC* pDC)
{
    CResizableSheetEx::OnEraseBkgnd(pDC);

    if (m_aeroControls.AeroDialogsEnabled())
    {
        // draw the frame margins in black
        RECT rc;
        GetClientRect(&rc);
        pDC->FillSolidRect(rc.left, rc.bottom - BOTTOMMARG, rc.right - rc.left, BOTTOMMARG, RGB(0, 0, 0));
    }
    return TRUE;
}

// ReSharper disable once CppMemberFunctionMayBeConst
LRESULT CMergeWizard::OnTaskbarButtonCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    setUuidOverlayIcon(m_hWnd);
    return 0;
}
