// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004-2010 - TortoiseSVN

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
#include "TortoiseMerge.h"
#include "OpenDlg.h"
#include "Patch.h"
#include "ProgressDlg.h"
#include "Settings.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "MainFrm.h"
#include "LeftView.h"
#include "RightView.h"
#include "BottomView.h"
#include "DiffColors.h"
#include ".\mainfrm.h"
#include "SelectFileFilter.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame
const UINT CMainFrame::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnApplicationLook)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnUpdateApplicationLook)
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, CFrameWndEx::OnHelpFinder)
    ON_COMMAND(ID_HELP, CFrameWndEx::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CFrameWndEx::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWndEx::OnHelpFinder)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_VIEW_WHITESPACES, OnViewWhitespaces)
    ON_WM_SIZE()
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
    ON_COMMAND(ID_VIEW_ONEWAYDIFF, OnViewOnewaydiff)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ONEWAYDIFF, OnUpdateViewOnewaydiff)
    ON_UPDATE_COMMAND_UI(ID_VIEW_WHITESPACES, OnUpdateViewWhitespaces)
    ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
    ON_WM_CLOSE()
    ON_COMMAND(ID_EDIT_FIND, OnEditFind)
    ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
    ON_COMMAND(ID_EDIT_FINDNEXT, OnEditFindnext)
    ON_COMMAND(ID_EDIT_FINDPREV, OnEditFindprev)
    ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
    ON_COMMAND(ID_VIEW_LINEDOWN, OnViewLinedown)
    ON_COMMAND(ID_VIEW_LINEUP, OnViewLineup)
    ON_COMMAND(ID_VIEW_MOVEDBLOCKS, OnViewMovedBlocks)
    ON_UPDATE_COMMAND_UI(ID_VIEW_MOVEDBLOCKS, OnUpdateViewMovedBlocks)
    ON_UPDATE_COMMAND_UI(ID_EDIT_MARKASRESOLVED, OnUpdateMergeMarkasresolved)
    ON_COMMAND(ID_EDIT_MARKASRESOLVED, OnMergeMarkasresolved)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTCONFLICT, OnUpdateMergeNextconflict)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVIOUSCONFLICT, OnUpdateMergePreviousconflict)
    ON_WM_MOVE()
    ON_WM_MOVING()
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_VIEW_SWITCHLEFT, OnViewSwitchleft)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SWITCHLEFT, OnUpdateViewSwitchleft)
    ON_COMMAND(ID_VIEW_LINELEFT, &CMainFrame::OnViewLineleft)
    ON_COMMAND(ID_VIEW_LINERIGHT, &CMainFrame::OnViewLineright)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWFILELIST, &CMainFrame::OnUpdateViewShowfilelist)
    ON_COMMAND(ID_VIEW_SHOWFILELIST, &CMainFrame::OnViewShowfilelist)
    ON_COMMAND(ID_EDIT_USETHEIRBLOCK, &CMainFrame::OnEditUseTheirs)
    ON_COMMAND(ID_EDIT_USEMYBLOCK, &CMainFrame::OnEditUseMine)
    ON_COMMAND(ID_EDIT_USETHEIRTHENMYBLOCK, &CMainFrame::OnEditUseTheirsThenMine)
    ON_COMMAND(ID_EDIT_USEMINETHENTHEIRBLOCK, &CMainFrame::OnEditUseMineThenTheirs)
    ON_COMMAND(ID_EDIT_UNDO, &CMainFrame::OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CMainFrame::OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USEMINETHENTHEIRBLOCK, &CMainFrame::OnUpdateEditUseminethentheirblock)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USEMYBLOCK, &CMainFrame::OnUpdateEditUsemyblock)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USETHEIRBLOCK, &CMainFrame::OnUpdateEditUsetheirblock)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USETHEIRTHENMYBLOCK, &CMainFrame::OnUpdateEditUsetheirthenmyblock)
    ON_COMMAND(ID_VIEW_INLINEDIFFWORD, &CMainFrame::OnViewInlinediffword)
    ON_UPDATE_COMMAND_UI(ID_VIEW_INLINEDIFFWORD, &CMainFrame::OnUpdateViewInlinediffword)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CREATEUNIFIEDDIFFFILE, &CMainFrame::OnUpdateEditCreateunifieddifffile)
    ON_COMMAND(ID_EDIT_CREATEUNIFIEDDIFFFILE, &CMainFrame::OnEditCreateunifieddifffile)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LINEDIFFBAR, &CMainFrame::OnUpdateViewLinediffbar)
    ON_COMMAND(ID_VIEW_LINEDIFFBAR, &CMainFrame::OnViewLinediffbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LOCATORBAR, &CMainFrame::OnUpdateViewLocatorbar)
    ON_COMMAND(ID_VIEW_LOCATORBAR, &CMainFrame::OnViewLocatorbar)
    ON_COMMAND(ID_EDIT_USELEFTBLOCK, &CMainFrame::OnEditUseleftblock)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USELEFTBLOCK, &CMainFrame::OnUpdateEditUseleftblock)
    ON_COMMAND(ID_EDIT_USELEFTFILE, &CMainFrame::OnEditUseleftfile)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USELEFTFILE, &CMainFrame::OnUpdateEditUseleftfile)
    ON_COMMAND(ID_EDIT_USEBLOCKFROMLEFTBEFORERIGHT, &CMainFrame::OnEditUseblockfromleftbeforeright)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USEBLOCKFROMLEFTBEFORERIGHT, &CMainFrame::OnUpdateEditUseblockfromleftbeforeright)
    ON_COMMAND(ID_EDIT_USEBLOCKFROMRIGHTBEFORELEFT, &CMainFrame::OnEditUseblockfromrightbeforeleft)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USEBLOCKFROMRIGHTBEFORELEFT, &CMainFrame::OnUpdateEditUseblockfromrightbeforeleft)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTDIFFERENCE, &CMainFrame::OnUpdateNavigateNextdifference)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVIOUSDIFFERENCE, &CMainFrame::OnUpdateNavigatePreviousdifference)
    ON_COMMAND(ID_VIEW_COLLAPSED, &CMainFrame::OnViewCollapsed)
    ON_UPDATE_COMMAND_UI(ID_VIEW_COLLAPSED, &CMainFrame::OnUpdateViewCollapsed)
    ON_COMMAND(ID_VIEW_COMPAREWHITESPACES, &CMainFrame::OnViewComparewhitespaces)
    ON_UPDATE_COMMAND_UI(ID_VIEW_COMPAREWHITESPACES, &CMainFrame::OnUpdateViewComparewhitespaces)
    ON_COMMAND(ID_VIEW_IGNOREWHITESPACECHANGES, &CMainFrame::OnViewIgnorewhitespacechanges)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IGNOREWHITESPACECHANGES, &CMainFrame::OnUpdateViewIgnorewhitespacechanges)
    ON_COMMAND(ID_VIEW_IGNOREALLWHITESPACECHANGES, &CMainFrame::OnViewIgnoreallwhitespacechanges)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IGNOREALLWHITESPACECHANGES, &CMainFrame::OnUpdateViewIgnoreallwhitespacechanges)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTINLINEDIFF, &CMainFrame::OnUpdateNavigateNextinlinediff)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVINLINEDIFF, &CMainFrame::OnUpdateNavigatePrevinlinediff)
    ON_COMMAND(ID_VIEW_WRAPLONGLINES, &CMainFrame::OnViewWraplonglines)
    ON_UPDATE_COMMAND_UI(ID_VIEW_WRAPLONGLINES, &CMainFrame::OnUpdateViewWraplonglines)
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_LEFTVIEW,
    ID_INDICATOR_RIGHTVIEW,
    ID_INDICATOR_BOTTOMVIEW,
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    m_pFindDialog = NULL;
    m_nSearchIndex = 0;
    m_bInitSplitter = FALSE;
    m_bOneWay = (0 != ((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\OnePane"))));
    m_bReversedPatch = FALSE;
    m_bHasConflicts = false;
    m_bInlineWordDiff = true;
    m_bLineDiff = true;
    m_bLocatorBar = true;
    m_nMoveMovesToIgnore = 0;
    theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2005);
    m_bCollapsed = !!(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\Collapsed"), 0);
    m_bViewMovedBlocks = !!(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\ViewMovedBlocks"), 0);
    m_bWrapLines = !!(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\WrapLines"), 0);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    OnApplicationLook(theApp.m_nAppLook);

    if (!m_wndMenuBar.Create(this))
    {
        TRACE0("Failed to create menubar\n");
        return -1;      // fail to create
    }

    m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

    // prevent the menu bar from taking the focus on activation
    CMFCPopupMenu::SetForceMenuFocus(FALSE);

    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
    m_wndToolBar.SetWindowText(_T("Main"));

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    if (!m_wndLocatorBar.Create(this, IDD_DIFFLOCATOR,
        CBRS_ALIGN_LEFT | CBRS_SIZE_FIXED, ID_VIEW_LOCATORBAR))
    {
        TRACE0("Failed to create dialogbar\n");
        return -1;      // fail to create
    }
    if (!m_wndLineDiffBar.Create(this, IDD_LINEDIFF,
        CBRS_ALIGN_BOTTOM | CBRS_SIZE_FIXED, ID_VIEW_LINEDIFFBAR))
    {
        TRACE0("Failed to create dialogbar\n");
        return -1;      // fail to create
    }
    m_wndLocatorBar.m_pMainFrm = this;
    m_wndLineDiffBar.m_pMainFrm = this;

    EnableDocking(CBRS_ALIGN_ANY);
    m_wndMenuBar.EnableDocking(CBRS_ALIGN_TOP);
    m_wndToolBar.EnableDocking(CBRS_ALIGN_TOP);
    DockPane(&m_wndMenuBar);
    DockPane(&m_wndToolBar);
    DockPane(&m_wndLocatorBar);
    DockPane(&m_wndLineDiffBar);
    ShowPane(&m_wndLocatorBar, true, false, true);
    ShowPane(&m_wndLineDiffBar, true, false, true);

    m_wndLocatorBar.EnableGripper(FALSE);
    m_wndLineDiffBar.EnableGripper(FALSE);

    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWndEx::PreCreateWindow(cs) )
        return FALSE;
    return TRUE;
}

void CMainFrame::OnApplicationLook(UINT id)
{
    CWaitCursor wait;

    theApp.m_nAppLook = id;

    switch (theApp.m_nAppLook)
    {
    case ID_VIEW_APPLOOK_WIN_2000:
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
        break;

    case ID_VIEW_APPLOOK_OFF_XP:
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
        break;

    case ID_VIEW_APPLOOK_WIN_XP:
        CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
        break;

    case ID_VIEW_APPLOOK_OFF_2003:
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
        CDockingManager::SetDockingMode(DT_SMART);
        break;

    case ID_VIEW_APPLOOK_VS_2005:
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
        CDockingManager::SetDockingMode(DT_SMART);
        break;

    default:
        switch (theApp.m_nAppLook)
        {
        case ID_VIEW_APPLOOK_OFF_2007_BLUE:
            CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
            break;

        case ID_VIEW_APPLOOK_OFF_2007_BLACK:
            CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
            break;

        case ID_VIEW_APPLOOK_OFF_2007_SILVER:
            CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
            break;

        case ID_VIEW_APPLOOK_OFF_2007_AQUA:
            CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
            break;
        }

        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
        CDockingManager::SetDockingMode(DT_SMART);
    }

    RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

    theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWndEx::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers


BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
    CRect cr;
    GetClientRect( &cr);


    // split into three panes:
    //        -------------
    //        |     |     |
    //        |     |     |
    //        |------------
    //        |           |
    //        |           |
    //        |------------

    // create a splitter with 2 rows, 1 column
    if (!m_wndSplitter.CreateStatic(this, 2, 1))
    {
        TRACE0("Failed to CreateStaticSplitter\n");
        return FALSE;
    }

    // add the second splitter pane - which is a nested splitter with 2 columns
    if (!m_wndSplitter2.CreateStatic(
        &m_wndSplitter, // our parent window is the first splitter
        1, 2, // the new splitter is 1 row, 2 columns
        WS_CHILD | WS_VISIBLE | WS_BORDER, // style, WS_BORDER is needed
        m_wndSplitter.IdFromRowCol(0, 0)
        // new splitter is in the first row, 1st column of first splitter
        ))
    {
        TRACE0("Failed to create nested splitter\n");
        return FALSE;
    }
    // add the first splitter pane - the default view in row 0
    if (!m_wndSplitter.CreateView(1, 0,
        RUNTIME_CLASS(CBottomView), CSize(cr.Width(), cr.Height()), pContext))
    {
        TRACE0("Failed to create first pane\n");
        return FALSE;
    }
    m_pwndBottomView = (CBottomView *)m_wndSplitter.GetPane(1,0);
    m_pwndBottomView->m_pwndLocator = &m_wndLocatorBar;
    m_pwndBottomView->m_pwndLineDiffBar = &m_wndLineDiffBar;
    m_pwndBottomView->m_pwndStatusBar = &m_wndStatusBar;
    m_pwndBottomView->m_pMainFrame = this;

    // now create the two views inside the nested splitter

    if (!m_wndSplitter2.CreateView(0, 0,
        RUNTIME_CLASS(CLeftView), CSize(cr.Width()/2, cr.Height()/2), pContext))
    {
        TRACE0("Failed to create second pane\n");
        return FALSE;
    }
    m_pwndLeftView = (CLeftView *)m_wndSplitter2.GetPane(0,0);
    m_pwndLeftView->m_pwndLocator = &m_wndLocatorBar;
    m_pwndLeftView->m_pwndLineDiffBar = &m_wndLineDiffBar;
    m_pwndLeftView->m_pwndStatusBar = &m_wndStatusBar;
    m_pwndLeftView->m_pMainFrame = this;

    if (!m_wndSplitter2.CreateView(0, 1,
        RUNTIME_CLASS(CRightView), CSize(cr.Width()/2, cr.Height()/2), pContext))
    {
        TRACE0("Failed to create third pane\n");
        return FALSE;
    }
    m_pwndRightView = (CRightView *)m_wndSplitter2.GetPane(0,1);
    m_pwndRightView->m_pwndLocator = &m_wndLocatorBar;
    m_pwndRightView->m_pwndLineDiffBar = &m_wndLineDiffBar;
    m_pwndRightView->m_pwndStatusBar = &m_wndStatusBar;
    m_pwndRightView->m_pMainFrame = this;
    m_bInitSplitter = TRUE;

    m_dlgFilePatches.Create(IDD_FILEPATCHES, this);
    UpdateLayout();
    return TRUE;
}

// Callback function
BOOL CMainFrame::PatchFile(CString sFilePath, CString sVersion, BOOL bAutoPatch)
{
    CString sDummy;
    //"dry run" was successful, so save the patched file somewhere...
    CString sTempFile = m_TempFiles.GetTempFilePath();
    CString sRejectedFile;
    if (m_Patch.GetPatchResult(sFilePath, sTempFile, sRejectedFile) < 0)
    {
        MessageBox(m_Patch.GetErrorMessage(), NULL, MB_ICONERROR);
        return FALSE;
    }
    sFilePath = m_Patch.GetTargetPath() + _T("\\") + sFilePath;
    sFilePath.Replace('/', '\\');
    if (m_bReversedPatch)
    {
        m_Data.m_baseFile.SetFileName(sTempFile);
        CString temp;
        temp.Format(_T("%s %s"), (LPCTSTR)CPathUtils::GetFileNameFromPath(sFilePath), (LPCTSTR)m_Data.m_sPatchPatched);
        m_Data.m_baseFile.SetDescriptiveName(temp);
        m_Data.m_yourFile.SetFileName(sFilePath);
        temp.Format(_T("%s %s"), (LPCTSTR)CPathUtils::GetFileNameFromPath(sFilePath), (LPCTSTR)m_Data.m_sPatchOriginal);
        m_Data.m_yourFile.SetDescriptiveName(temp);
        m_Data.m_theirFile.SetOutOfUse();
        m_Data.m_mergedFile.SetOutOfUse();
    }
    else
    {
        if (!PathFileExists(sFilePath))
        {
            m_Data.m_baseFile.SetFileName(m_TempFiles.GetTempFilePath());
            m_Data.m_baseFile.CreateEmptyFile();
        }
        else
        {
            m_Data.m_baseFile.SetFileName(sFilePath);
        }
        CString sDescription;
        sDescription.Format(_T("%s %s"), (LPCTSTR)CPathUtils::GetFileNameFromPath(sFilePath), (LPCTSTR)m_Data.m_sPatchOriginal);
        m_Data.m_baseFile.SetDescriptiveName(sDescription);
        m_Data.m_yourFile.SetFileName(sTempFile);
        CString temp;
        temp.Format(_T("%s %s"), (LPCTSTR)CPathUtils::GetFileNameFromPath(sFilePath), (LPCTSTR)m_Data.m_sPatchPatched);
        m_Data.m_yourFile.SetDescriptiveName(temp);
        m_Data.m_theirFile.SetOutOfUse();
        m_Data.m_mergedFile.SetFileName(sFilePath);
    }
    TRACE(_T("comparing %s\nwith the patched result %s\n"), (LPCTSTR)sFilePath, (LPCTSTR)sTempFile);

    LoadViews();
    if (!sRejectedFile.IsEmpty())
    {
        // start TortoiseUDiff with the rejected hunks
        CString sTitle;
        sTitle.Format(IDS_TITLE_REJECTEDHUNKS, (LPCTSTR)CPathUtils::GetFileNameFromPath(sFilePath));
        CAppUtils::StartUnifiedDiffViewer(sRejectedFile, sTitle);
    }
    if (bAutoPatch)
    {
        OnFileSave();
    }
    return TRUE;
}

// Callback function
BOOL CMainFrame::DiffFiles(CString sURL1, CString sRev1, CString sURL2, CString sRev2)
{
    CString tempfile1 = m_TempFiles.GetTempFilePath();
    CString tempfile2 = m_TempFiles.GetTempFilePath();

    ASSERT(tempfile1.Compare(tempfile2));

    CString sTemp;
    CProgressDlg progDlg;
    sTemp.Format(IDS_GETVERSIONOFFILE, (LPCTSTR)sRev1);
    progDlg.SetLine(1, sTemp, true);
    progDlg.SetLine(2, sURL1, true);
    sTemp.LoadString(IDS_GETVERSIONOFFILETITLE);
    progDlg.SetTitle(sTemp);
    progDlg.SetShowProgressBar(true);
    progDlg.SetAnimation(IDR_DOWNLOAD);
    progDlg.SetTime(FALSE);
    progDlg.SetProgress(1,100);
    progDlg.ShowModeless(this);
    if (!CAppUtils::GetVersionedFile(sURL1, sRev1, tempfile1, &progDlg, m_hWnd))
    {
        progDlg.Stop();
        CString sErrMsg;
        sErrMsg.FormatMessage(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, (LPCTSTR)sRev1, (LPCTSTR)sURL1);
        MessageBox(sErrMsg, NULL, MB_ICONERROR);
        return FALSE;
    }
    sTemp.Format(IDS_GETVERSIONOFFILE, (LPCTSTR)sRev2);
    progDlg.SetLine(1, sTemp, true);
    progDlg.SetLine(2, sURL2, true);
    progDlg.SetProgress(50, 100);
    if (!CAppUtils::GetVersionedFile(sURL2, sRev2, tempfile2, &progDlg, m_hWnd))
    {
        progDlg.Stop();
        CString sErrMsg;
        sErrMsg.FormatMessage(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, (LPCTSTR)sRev2, (LPCTSTR)sURL2);
        MessageBox(sErrMsg, NULL, MB_ICONERROR);
        return FALSE;
    }
    progDlg.SetProgress(100,100);
    progDlg.Stop();
    CString temp;
    temp.Format(_T("%s Revision %s"), (LPCTSTR)CPathUtils::GetFileNameFromPath(sURL1), (LPCTSTR)sRev1);
    m_Data.m_baseFile.SetFileName(tempfile1);
    m_Data.m_baseFile.SetDescriptiveName(temp);
    temp.Format(_T("%s Revision %s"), (LPCTSTR)CPathUtils::GetFileNameFromPath(sURL2), (LPCTSTR)sRev2);
    m_Data.m_yourFile.SetFileName(tempfile2);
    m_Data.m_yourFile.SetDescriptiveName(temp);

    LoadViews();

    return TRUE;
}

void CMainFrame::OnFileOpen()
{
    if (CheckForSave()==IDCANCEL)
        return;
    COpenDlg dlg;
    if (dlg.DoModal()!=IDOK)
    {
        return;
    }
    m_dlgFilePatches.ShowWindow(SW_HIDE);
    m_dlgFilePatches.Init(NULL, NULL, CString(), NULL);
    TRACE(_T("got the files:\n   %s\n   %s\n   %s\n   %s\n   %s\n"), (LPCTSTR)dlg.m_sBaseFile, (LPCTSTR)dlg.m_sTheirFile, (LPCTSTR)dlg.m_sYourFile,
        (LPCTSTR)dlg.m_sUnifiedDiffFile, (LPCTSTR)dlg.m_sPatchDirectory);
    m_Data.m_baseFile.SetFileName(dlg.m_sBaseFile);
    m_Data.m_theirFile.SetFileName(dlg.m_sTheirFile);
    m_Data.m_yourFile.SetFileName(dlg.m_sYourFile);
    m_Data.m_sDiffFile = dlg.m_sUnifiedDiffFile;
    m_Data.m_sPatchPath = dlg.m_sPatchDirectory;
    m_Data.m_mergedFile.SetOutOfUse();
    g_crasher.AddFile((LPCSTR)(LPCTSTR)dlg.m_sBaseFile, (LPCSTR)(LPCTSTR)_T("Basefile"));
    g_crasher.AddFile((LPCSTR)(LPCTSTR)dlg.m_sTheirFile, (LPCSTR)(LPCTSTR)_T("Theirfile"));
    g_crasher.AddFile((LPCSTR)(LPCTSTR)dlg.m_sYourFile, (LPCSTR)(LPCTSTR)_T("Yourfile"));
    g_crasher.AddFile((LPCSTR)(LPCTSTR)dlg.m_sUnifiedDiffFile, (LPCSTR)(LPCTSTR)_T("Difffile"));

    if (!m_Data.IsBaseFileInUse() && m_Data.IsTheirFileInUse() && m_Data.IsYourFileInUse())
    {
        // a diff between two files means "Yours" against "Base", not "Theirs" against "Yours"
        m_Data.m_baseFile.TransferDetailsFrom(m_Data.m_theirFile);
    }
    if (m_Data.IsBaseFileInUse() && m_Data.IsTheirFileInUse() && !m_Data.IsYourFileInUse())
    {
        // a diff between two files means "Yours" against "Base", not "Theirs" against "Base"
        m_Data.m_yourFile.TransferDetailsFrom(m_Data.m_theirFile);
    }

    LoadViews();
}

void CMainFrame::ClearViewNamesAndPaths()
{
    m_pwndLeftView->m_sWindowName.Empty();
    m_pwndLeftView->m_sFullFilePath.Empty();
    m_pwndRightView->m_sWindowName.Empty();
    m_pwndRightView->m_sFullFilePath.Empty();
    m_pwndBottomView->m_sWindowName.Empty();
    m_pwndBottomView->m_sFullFilePath.Empty();
}

bool CMainFrame::LoadViews(int line)
{
    m_Data.SetBlame(m_bBlame);
    m_Data.SetMovedBlocks(m_bViewMovedBlocks);
    m_bHasConflicts = false;
    CBaseView* pwndActiveView = m_pwndLeftView;
    int nOldLine = m_pwndLeftView ? m_pwndLeftView->m_nTopLine : -1;
    int nOldLineNumber =
        m_pwndLeftView && m_pwndLeftView->m_pViewData ?
        m_pwndLeftView->m_pViewData->GetLineNumber(m_pwndLeftView->m_nTopLine) : -1;
    int nOldCaretPos = -1;
    if (m_pwndRightView && m_pwndRightView->HasCaret())
        nOldCaretPos = m_pwndRightView->GetCaretPosition().y;
    if (m_pwndBottomView && m_pwndBottomView->HasCaret())
        nOldCaretPos = m_pwndBottomView->GetCaretPosition().y;
    if (!m_Data.Load())
    {
        ::MessageBox(m_hWnd, m_Data.GetError(), _T("TortoiseMerge"), MB_ICONERROR);
        m_Data.m_mergedFile.SetOutOfUse();
        return false;
    }
    m_pwndLeftView->DocumentUpdated();
    m_pwndRightView->DocumentUpdated();
    m_pwndBottomView->DocumentUpdated();
    m_wndLocatorBar.DocumentUpdated();
    m_wndLineDiffBar.DocumentUpdated();

    m_pwndRightView->UseCaret(false);
    m_pwndBottomView->UseCaret(false);

    if (!m_Data.IsBaseFileInUse())
    {
        CProgressDlg progDlg;
        if (m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
        {
            m_Data.m_baseFile.TransferDetailsFrom(m_Data.m_theirFile);
        }
        else if ((!m_Data.m_sDiffFile.IsEmpty())&&(!m_Patch.Init(m_Data.m_sDiffFile, m_Data.m_sPatchPath, &progDlg)))
        {
            ClearViewNamesAndPaths();
            MessageBox(m_Patch.GetErrorMessage(), NULL, MB_ICONERROR);
            return false;
        }
        if (m_Patch.GetNumberOfFiles() > 0)
        {
            CString betterpatchpath = m_Patch.CheckPatchPath(m_Data.m_sPatchPath);
            if (betterpatchpath.CompareNoCase(m_Data.m_sPatchPath)!=0)
            {
                CString msg;
                msg.FormatMessage(IDS_WARNBETTERPATCHPATHFOUND, (LPCTSTR)m_Data.m_sPatchPath, (LPCTSTR)betterpatchpath);
                if (::MessageBox(m_hWnd, msg, _T("TortoiseMerge"), MB_ICONQUESTION | MB_YESNO)==IDYES)
                {
                    m_Data.m_sPatchPath = betterpatchpath;
                    m_Patch.Init(m_Data.m_sDiffFile, m_Data.m_sPatchPath, &progDlg);
                }
            }
            m_dlgFilePatches.Init(&m_Patch, this, m_Data.m_sPatchPath, this);
            m_dlgFilePatches.ShowWindow(SW_SHOW);
            ClearViewNamesAndPaths();
            if (!m_wndSplitter.IsRowHidden(1))
                m_wndSplitter.HideRow(1);
            m_pwndLeftView->SetHidden(FALSE);
            m_pwndRightView->SetHidden(FALSE);
            m_pwndBottomView->SetHidden(TRUE);
        }
    }
    if (m_Data.IsBaseFileInUse() && !m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
    {
        m_Data.m_yourFile.TransferDetailsFrom(m_Data.m_theirFile);
    }
    if (m_Data.IsBaseFileInUse() && m_Data.IsYourFileInUse() && !m_Data.IsTheirFileInUse())
    {
        //diff between YOUR and BASE
        m_pwndRightView->UseCaret();
        if (m_bOneWay)
        {
            if (!m_wndSplitter2.IsColumnHidden(1))
                m_wndSplitter2.HideColumn(1);

            m_pwndLeftView->m_pViewData = &m_Data.m_YourBaseBoth;
            m_pwndLeftView->texttype = m_Data.m_arYourFile.GetUnicodeType();
            m_pwndLeftView->lineendings = m_Data.m_arYourFile.GetLineEndings();
            m_pwndLeftView->m_sWindowName = m_Data.m_baseFile.GetWindowName() + _T(" - ") + m_Data.m_yourFile.GetWindowName();
            m_pwndLeftView->m_sFullFilePath = m_Data.m_baseFile.GetFilename() + _T(" - ") + m_Data.m_yourFile.GetFilename();

            m_pwndRightView->m_pViewData = NULL;
            m_pwndBottomView->m_pViewData = NULL;

            if (!m_wndSplitter.IsRowHidden(1))
                m_wndSplitter.HideRow(1);
            m_pwndLeftView->SetHidden(FALSE);
            m_pwndRightView->SetHidden(TRUE);
            m_pwndBottomView->SetHidden(TRUE);
            ::SetWindowPos(m_pwndLeftView->m_hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        }
        else
        {
            pwndActiveView = m_pwndRightView;
            if (m_wndSplitter2.IsColumnHidden(1))
                m_wndSplitter2.ShowColumn();

            m_pwndLeftView->m_pViewData = &m_Data.m_YourBaseLeft;
            m_pwndLeftView->texttype = m_Data.m_arBaseFile.GetUnicodeType();
            m_pwndLeftView->lineendings = m_Data.m_arBaseFile.GetLineEndings();
            m_pwndLeftView->m_sWindowName = m_Data.m_baseFile.GetWindowName();
            m_pwndLeftView->m_sFullFilePath = m_Data.m_baseFile.GetFilename();

            m_pwndRightView->m_pViewData = &m_Data.m_YourBaseRight;
            m_pwndRightView->texttype = m_Data.m_arYourFile.GetUnicodeType();
            m_pwndRightView->lineendings = m_Data.m_arYourFile.GetLineEndings();
            m_pwndRightView->m_sWindowName = m_Data.m_yourFile.GetWindowName();
            m_pwndRightView->m_sFullFilePath = m_Data.m_yourFile.GetFilename();

            m_pwndBottomView->m_pViewData = NULL;

            if (!m_wndSplitter.IsRowHidden(1))
                m_wndSplitter.HideRow(1);
            m_pwndLeftView->SetHidden(FALSE);
            m_pwndRightView->SetHidden(FALSE);
            m_pwndBottomView->SetHidden(TRUE);
        }
    }
    else if (m_Data.IsBaseFileInUse() && m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
    {
        //diff between THEIR, YOUR and BASE
        m_pwndBottomView->UseCaret();
        pwndActiveView = m_pwndBottomView;

        m_pwndLeftView->m_pViewData = &m_Data.m_TheirBaseBoth;
        m_pwndLeftView->texttype = m_Data.m_arTheirFile.GetUnicodeType();
        m_pwndLeftView->lineendings = m_Data.m_arTheirFile.GetLineEndings();
        m_pwndLeftView->m_sWindowName.LoadString(IDS_VIEWTITLE_THEIRS);
        m_pwndLeftView->m_sWindowName += _T(" - ") + m_Data.m_theirFile.GetWindowName();
        m_pwndLeftView->m_sFullFilePath = m_Data.m_theirFile.GetFilename();

        m_pwndRightView->m_pViewData = &m_Data.m_YourBaseBoth;
        m_pwndRightView->texttype = m_Data.m_arYourFile.GetUnicodeType();
        m_pwndRightView->lineendings = m_Data.m_arYourFile.GetLineEndings();
        m_pwndRightView->m_sWindowName.LoadString(IDS_VIEWTITLE_MINE);
        m_pwndRightView->m_sWindowName += _T(" - ") + m_Data.m_yourFile.GetWindowName();
        m_pwndRightView->m_sFullFilePath = m_Data.m_yourFile.GetFilename();

        m_pwndBottomView->m_pViewData = &m_Data.m_Diff3;
        m_pwndBottomView->texttype = m_Data.m_arTheirFile.GetUnicodeType();
        m_pwndBottomView->lineendings = m_Data.m_arTheirFile.GetLineEndings();
        m_pwndBottomView->m_sWindowName.LoadString(IDS_VIEWTITLE_MERGED);
        m_pwndBottomView->m_sWindowName += _T(" - ") + m_Data.m_mergedFile.GetWindowName();
        m_pwndBottomView->m_sFullFilePath = m_Data.m_mergedFile.GetFilename();

        if (m_wndSplitter2.IsColumnHidden(1))
            m_wndSplitter2.ShowColumn();
        if (m_wndSplitter.IsRowHidden(1))
            m_wndSplitter.ShowRow();
        m_pwndLeftView->SetHidden(FALSE);
        m_pwndRightView->SetHidden(FALSE);
        m_pwndBottomView->SetHidden(FALSE);
        // in three pane view, hide the line diff bar
        m_wndLineDiffBar.ShowPane(false, false, true);
        m_wndLineDiffBar.DocumentUpdated();
    }
    if (!m_Data.m_mergedFile.InUse())
    {
        m_Data.m_mergedFile.SetFileName(m_Data.m_yourFile.GetFilename());
    }
    m_pwndLeftView->DocumentUpdated();
    m_pwndRightView->DocumentUpdated();
    m_pwndBottomView->DocumentUpdated();
    m_wndLocatorBar.DocumentUpdated();
    m_wndLineDiffBar.DocumentUpdated();
    UpdateLayout();
    SetActiveView(pwndActiveView);

    if ((line >= -1) && m_pwndRightView->m_pViewData)
    {
        int n = line == -1 ? min( nOldLineNumber, nOldLine ) : line;
        if (n >= 0)
            n = m_pwndRightView->m_pViewData->FindLineNumber(n);
        if (n < 0)
            n = nOldLine;

        m_pwndRightView->ScrollAllToLine(n);
        POINT p;
        p.x = 0;
        p.y = n;
        if (nOldCaretPos >= 0)
            p.y = nOldCaretPos;
        m_pwndLeftView->SetCaretPosition(p);
        m_pwndRightView->SetCaretPosition(p);
        m_pwndBottomView->SetCaretPosition(p);
        m_pwndBottomView->ScrollToChar(0);
        m_pwndLeftView->ScrollToChar(0);
        m_pwndRightView->ScrollToChar(0);
    }
    else
    {
        bool bGoFirstDiff = (0 != (DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\FirstDiffOnLoad"), TRUE));
        bool bGoFirstConflict = (0 != (DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\FirstConflictOnLoad"), TRUE));
        if (bGoFirstConflict && (CheckResolved()>=0))
        {
            pwndActiveView->GoToFirstConflict();
            // Ignore the first few Mouse Move messages, so that the line diff stays on
            // the first diff line until the user actually moves the mouse
            m_nMoveMovesToIgnore = MOVESTOIGNORE;
        }
        else if (bGoFirstDiff)
        {
            pwndActiveView->GoToFirstDifference();
            // Ignore the first few Mouse Move messages, so that the line diff stays on
            // the first diff line until the user actually moves the mouse
            m_nMoveMovesToIgnore = MOVESTOIGNORE;
        }
        else
        {
            // Avoid incorrect rendering of active pane.
            m_pwndBottomView->ScrollToChar(0);
            m_pwndLeftView->ScrollToChar(0);
            m_pwndRightView->ScrollToChar(0);
        }
    }
    CheckResolved();
    CUndo::GetInstance().Clear();
    return true;
}

void CMainFrame::UpdateLayout()
{
    if (m_bInitSplitter)
    {
        CRect cr, rclocbar;
        GetClientRect(&cr);
        int width = cr.Width();
        if (::IsWindow(m_wndLocatorBar) && m_wndLocatorBar.IsWindowVisible())
        {
            m_wndLocatorBar.GetWindowRect(&rclocbar);
            width -= rclocbar.Width();
        }
        m_wndSplitter.SetRowInfo(0, cr.Height()/2, 0);
        m_wndSplitter.SetRowInfo(1, cr.Height()/2, 0);
        m_wndSplitter.SetColumnInfo(0, width / 2, 50);
        m_wndSplitter2.SetRowInfo(0, cr.Height()/2, 0);
        m_wndSplitter2.SetColumnInfo(0, width / 2, 50);
        m_wndSplitter2.SetColumnInfo(1, width / 2, 50);

        m_wndSplitter.RecalcLayout();
    }
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    if (m_bInitSplitter && nType != SIZE_MINIMIZED)
    {
        UpdateLayout();
    }
    CFrameWndEx::OnSize(nType, cx, cy);
}

void CMainFrame::OnViewWhitespaces()
{
    CRegDWORD regViewWhitespaces = CRegDWORD(_T("Software\\TortoiseMerge\\ViewWhitespaces"), 1);
    BOOL bViewWhitespaces = regViewWhitespaces;
    if (m_pwndLeftView)
        bViewWhitespaces = m_pwndLeftView->m_bViewWhitespace;

    bViewWhitespaces = !bViewWhitespaces;
    regViewWhitespaces = bViewWhitespaces;
    if (m_pwndLeftView)
    {
        m_pwndLeftView->m_bViewWhitespace = bViewWhitespaces;
        m_pwndLeftView->Invalidate();
    }
    if (m_pwndRightView)
    {
        m_pwndRightView->m_bViewWhitespace = bViewWhitespaces;
        m_pwndRightView->Invalidate();
    }
    if (m_pwndBottomView)
    {
        m_pwndBottomView->m_bViewWhitespace = bViewWhitespaces;
        m_pwndBottomView->Invalidate();
    }
}

void CMainFrame::OnUpdateViewWhitespaces(CCmdUI *pCmdUI)
{
    if (m_pwndLeftView)
        pCmdUI->SetCheck(m_pwndLeftView->m_bViewWhitespace);
}

void CMainFrame::OnViewCollapsed()
{
    CRegDWORD regViewCollapsed = CRegDWORD(_T("Software\\TortoiseMerge\\Collapsed"), 0);
    regViewCollapsed = !(DWORD)regViewCollapsed;
    m_bCollapsed = !!(DWORD)regViewCollapsed;

    if (m_pwndLeftView)
    {
        m_pwndLeftView->BuildScreen2ViewVector();
        m_pwndLeftView->UpdateCaret();
        m_pwndLeftView->Invalidate();
        if (m_pwndLeftView->HasCaret())
            m_pwndLeftView->EnsureCaretVisible();
    }
    if (m_pwndRightView)
    {
        m_pwndRightView->BuildScreen2ViewVector();
        m_pwndRightView->UpdateCaret();
        m_pwndRightView->Invalidate();
        if (m_pwndRightView->HasCaret())
            m_pwndRightView->EnsureCaretVisible();
    }
    if (m_pwndBottomView)
    {
        m_pwndBottomView->BuildScreen2ViewVector();
        m_pwndBottomView->UpdateCaret();
        m_pwndBottomView->Invalidate();
        if (m_pwndBottomView->HasCaret())
            m_pwndBottomView->EnsureCaretVisible();
    }
    m_wndLocatorBar.Invalidate();
}

void CMainFrame::OnUpdateViewCollapsed(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bCollapsed);
}

void CMainFrame::OnViewWraplonglines()
{
    CRegDWORD regViewWrapLines = CRegDWORD(_T("Software\\TortoiseMerge\\WrapLines"), 0);
    regViewWrapLines = !(DWORD)regViewWrapLines;
    m_bWrapLines = !!(DWORD)regViewWrapLines;

    if (m_pwndLeftView)
    {
        m_pwndLeftView->BuildScreen2ViewVector();
        m_pwndLeftView->UpdateCaret();
        m_pwndLeftView->Invalidate();
        if (m_pwndLeftView->HasCaret())
            m_pwndLeftView->EnsureCaretVisible();
    }
    if (m_pwndRightView)
    {
        m_pwndRightView->BuildScreen2ViewVector();
        m_pwndRightView->UpdateCaret();
        m_pwndRightView->Invalidate();
        if (m_pwndRightView->HasCaret())
            m_pwndRightView->EnsureCaretVisible();
    }
    if (m_pwndBottomView)
    {
        m_pwndBottomView->BuildScreen2ViewVector();
        m_pwndBottomView->UpdateCaret();
        m_pwndBottomView->Invalidate();
        if (m_pwndBottomView->HasCaret())
            m_pwndBottomView->EnsureCaretVisible();
    }
    m_wndLocatorBar.DocumentUpdated();
}


void CMainFrame::OnUpdateViewWraplonglines(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bWrapLines);
}

void CMainFrame::OnViewOnewaydiff()
{
    if (CheckForSave()==IDCANCEL)
        return;
    m_bOneWay = !m_bOneWay;
    if (m_bOneWay)
    {
        // in one way view, hide the line diff bar
        m_wndLineDiffBar.ShowPane(false, false, true);
        m_wndLineDiffBar.DocumentUpdated();
    }
    else
    {
        // restore the line diff bar
        m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
        m_wndLineDiffBar.DocumentUpdated();
        m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
        m_wndLocatorBar.DocumentUpdated();
    }
    LoadViews(-1);
}

void CMainFrame::ShowDiffBar(bool bShow)
{
    if (bShow)
    {
        // restore the line diff bar
        m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
        m_wndLineDiffBar.DocumentUpdated();
        m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
        m_wndLocatorBar.DocumentUpdated();
    }
    else
    {
        // in one way view, hide the line diff bar
        m_wndLineDiffBar.ShowPane(false, false, true);
        m_wndLineDiffBar.DocumentUpdated();
    }
}

int CMainFrame::CheckResolved()
{
    //only in three way diffs can be conflicts!
    m_bHasConflicts = true;
    if (m_pwndBottomView->IsWindowVisible())
    {
        CViewData* viewdata = m_pwndBottomView->m_pViewData;
        if (viewdata)
        {
            for (int i=0; i<viewdata->GetCount(); i++)
            {
                const DiffStates state = viewdata->GetState(i);
                if ((DIFFSTATE_CONFLICTED == state)||(DIFFSTATE_CONFLICTED_IGNORED == state))
                    return i;
            }
        }
    }
    m_bHasConflicts = false;
    return -1;
}

int CMainFrame::SaveFile(const CString& sFilePath)
{
    CViewData * pViewData = NULL;
    CFileTextLines * pOriginFile = &m_Data.m_arBaseFile;
    if ((m_pwndBottomView)&&(m_pwndBottomView->IsWindowVisible()))
    {
        pViewData = m_pwndBottomView->m_pViewData;
        Invalidate();
    }
    else if ((m_pwndRightView)&&(m_pwndRightView->IsWindowVisible()))
    {
        pViewData = m_pwndRightView->m_pViewData;
        if (m_Data.IsYourFileInUse())
            pOriginFile = &m_Data.m_arYourFile;
        else if (m_Data.IsTheirFileInUse())
            pOriginFile = &m_Data.m_arTheirFile;
        Invalidate();
    }
    else
    {
        // nothing to save!
        return -1;
    }
    if ((pViewData)&&(pOriginFile))
    {
        CFileTextLines file;
        pOriginFile->CopySettings(&file);
        for (int i=0; i<pViewData->GetCount(); i++)
        {
            //only copy non-removed lines
            DiffStates state = pViewData->GetState(i);
            switch (state)
            {
            case DIFFSTATE_CONFLICTED:
            case DIFFSTATE_CONFLICTED_IGNORED:
                {
                    int first = i;
                    int last = i;
                    do
                    {
                        last++;
                    } while((last<pViewData->GetCount()) && ((pViewData->GetState(last)==DIFFSTATE_CONFLICTED)||(pViewData->GetState(last)==DIFFSTATE_CONFLICTED_IGNORED)));
                    file.Add(_T("<<<<<<< .mine"), EOL_NOENDING);
                    for (int j=first; j<last; j++)
                    {
                        file.Add(m_pwndRightView->m_pViewData->GetLine(j), m_pwndRightView->m_pViewData->GetLineEnding(j));
                    }
                    file.Add(_T("======="), EOL_NOENDING);
                    for (int j=first; j<last; j++)
                    {
                        file.Add(m_pwndLeftView->m_pViewData->GetLine(j), m_pwndLeftView->m_pViewData->GetLineEnding(j));
                    }
                    file.Add(_T(">>>>>>> .theirs"), EOL_NOENDING);
                    i = last-1;
                }
                break;
            case DIFFSTATE_EMPTY:
            case DIFFSTATE_CONFLICTEMPTY:
            case DIFFSTATE_IDENTICALREMOVED:
            case DIFFSTATE_REMOVED:
            case DIFFSTATE_THEIRSREMOVED:
            case DIFFSTATE_YOURSREMOVED:
            case DIFFSTATE_CONFLICTRESOLVEDEMPTY:
                // do not save removed lines
                break;
            default:
                file.Add(pViewData->GetLine(i), pViewData->GetLineEnding(i));
                break;
            }
        }
        if (!file.Save(sFilePath, false))
        {
            ::MessageBox(m_hWnd, file.GetErrorString(), _T("TortoiseMerge"), MB_ICONERROR);
            return -1;
        }
        m_dlgFilePatches.SetFileStatusAsPatched(sFilePath);
        if (m_pwndBottomView)
            m_pwndBottomView->SetModified(FALSE);
        if (m_pwndRightView)
            m_pwndRightView->SetModified(FALSE);
        CUndo::GetInstance().MarkAsOriginalState();
        return file.GetCount();
    }
    return -1;
}

void CMainFrame::OnFileSave()
{
    FileSave();
}

bool CMainFrame::FileSave(bool bCheckResolved /*=true*/)
{
    if (!m_Data.m_mergedFile.InUse())
        return FileSaveAs(bCheckResolved);
    // check if the file has the readonly attribute set
    bool bDoesNotExist = false;
    DWORD fAttribs = GetFileAttributes(m_Data.m_mergedFile.GetFilename());
    if ((fAttribs != INVALID_FILE_ATTRIBUTES)&&(fAttribs & FILE_ATTRIBUTE_READONLY))
        return FileSaveAs(bCheckResolved);
    if (fAttribs == INVALID_FILE_ATTRIBUTES)
    {
        bDoesNotExist = (GetLastError() == ERROR_FILE_NOT_FOUND);
    }
    if (bCheckResolved && HasConflictsWontKeep())
        return false;

    if (((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\Backup"))) != 0)
    {
        MoveFileEx(m_Data.m_mergedFile.GetFilename(), m_Data.m_mergedFile.GetFilename() + _T(".bak"), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    if (SaveFile(m_Data.m_mergedFile.GetFilename())==0)
    {
        // file was saved with 0 lines!
        // ask the user if the file should be deleted
        CString sTemp;
        sTemp.Format(IDS_DELETEWHENEMPTY, (LPCTSTR)m_Data.m_mergedFile.GetFilename());
        if (CMessageBox::ShowCheck(m_hWnd, sTemp, _T("TortoiseMerge"), MB_YESNO, _T("DeleteFileWhenEmpty")) == IDYES)
        {
            DeleteFile(m_Data.m_mergedFile.GetFilename());
        }
    }

    if ((bDoesNotExist)&&(DWORD(CRegDWORD(_T("Software\\TortoiseMerge\\AutoAdd"), TRUE))))
    {
        // call TortoiseProc to add the new file to version control
        CString cmd = _T("\"") + CPathUtils::GetAppDirectory();
        cmd += _T("TortoiseProc.exe\" /command:add /noui /path:\"");
        cmd += m_Data.m_mergedFile.GetFilename() + _T("\"");
        auto_buffer<TCHAR> buf(cmd.GetLength()+1);
        _tcscpy_s(buf, cmd.GetLength()+1, cmd);
        if(!RunCommand(buf))
            return FALSE;
    }
    return true;
}

void CMainFrame::OnFileSaveAs()
{
    FileSaveAs();
}

bool CMainFrame::FileSaveAs(bool bCheckResolved /*=true*/)
{
    if (bCheckResolved && HasConflictsWontKeep())
        return false;

    CString fileName;
    if(!TryGetFileName(fileName))
        return false;

    SaveFile(fileName);
    return true;
}

void CMainFrame::OnUpdateFileSave(CCmdUI *pCmdUI)
{
    BOOL bEnable = FALSE;
    if (m_Data.m_mergedFile.InUse())
    {
        if (m_pwndBottomView)
        {
            if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_pViewData))
            {
                bEnable = TRUE;
            }
        }
        if (m_pwndRightView)
        {
            if ((m_pwndRightView->IsWindowVisible())&&(m_pwndRightView->m_pViewData))
            {
                if (m_pwndRightView->IsModified() || (m_Data.m_yourFile.GetWindowName().Right(9).Compare(_T(": patched"))==0))
                    bEnable = TRUE;
            }
        }
    }
    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateFileSaveAs(CCmdUI *pCmdUI)
{
    BOOL bEnable = FALSE;
    if (m_pwndBottomView)
    {
        if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_pViewData))
        {
            bEnable = TRUE;
        }
    }
    if (m_pwndRightView)
    {
        if ((m_pwndRightView->IsWindowVisible())&&(m_pwndRightView->m_pViewData))
        {
            bEnable = TRUE;
        }
    }
    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateViewOnewaydiff(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(!m_bOneWay);
    BOOL bEnable = TRUE;
    if (m_pwndBottomView)
    {
        if (m_pwndBottomView->IsWindowVisible())
            bEnable = FALSE;
    }
    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnViewOptions()
{
    CString sTemp;
    sTemp.LoadString(IDS_SETTINGSTITLE);
    CSettings dlg(sTemp);
    dlg.DoModal();
    if (dlg.IsReloadNeeded())
    {
        if (CheckForSave()==IDCANCEL)
            return;
        CDiffColors::GetInstance().LoadRegistry();
        LoadViews();
        return;
    }
    CDiffColors::GetInstance().LoadRegistry();
    if (m_pwndBottomView)
        m_pwndBottomView->Invalidate();
    if (m_pwndLeftView)
        m_pwndLeftView->Invalidate();
    if (m_pwndRightView)
        m_pwndRightView->Invalidate();
}

void CMainFrame::OnClose()
{
    if ((m_pFindDialog)&&(!m_pFindDialog->IsTerminating()))
    {
        m_pFindDialog->SendMessage(WM_CLOSE);
        return;
    }
    int ret = IDNO;
    if (((m_pwndBottomView)&&(m_pwndBottomView->IsModified())) ||
        ((m_pwndRightView)&&(m_pwndRightView->IsModified())))
    {
        CString sTemp;
        sTemp.LoadString(IDS_ASKFORSAVE);
        ret = MessageBox(sTemp, 0, MB_YESNOCANCEL | MB_ICONQUESTION);
        if (ret == IDYES)
        {
            if (!FileSave())
                return;
        }
    }
    if ((ret == IDNO)||(ret == IDYES))
    {
        WINDOWPLACEMENT    wp;

        // before it is destroyed, save the position of the window
        wp.length = sizeof wp;

        if (GetWindowPlacement(&wp))
        {

            if (IsIconic())
                // never restore to Iconic state
                wp.showCmd = SW_SHOW ;

            if ((wp.flags & WPF_RESTORETOMAXIMIZED) != 0)
                // if maximized and maybe iconic restore maximized state
                wp.showCmd = SW_SHOWMAXIMIZED ;

            // and write it to the .INI file
            WriteWindowPlacement(&wp);
        }
        __super::OnClose();
    }
}

void CMainFrame::OnEditFind()
{
    if (m_pFindDialog)
        return;

    // start searching from the start again
    // if no line is selected, otherwise start from
    // the selected line
    m_nSearchIndex = FindSearchStart(0);
    m_pFindDialog = new CFindDlg();
    m_pFindDialog->Create(this);
    CString markedWord;
    if (m_pwndLeftView && m_pwndLeftView->IsWindowVisible())
        markedWord = m_pwndLeftView->GetMarkedWord();
    if (markedWord.IsEmpty() && m_pwndRightView && m_pwndRightView->IsWindowVisible())
        markedWord = m_pwndRightView->GetMarkedWord();
    if (markedWord.IsEmpty() && m_pwndBottomView && m_pwndBottomView->IsWindowVisible())
        markedWord = m_pwndBottomView->GetMarkedWord();

    m_pFindDialog->SetFindString(markedWord);
}

LRESULT CMainFrame::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_pFindDialog != NULL);

    if (m_pFindDialog->IsTerminating())
    {
        // invalidate the handle identifying the dialog box.
        m_pFindDialog = NULL;
        return 0;
    }

    if(m_pFindDialog->FindNext())
    {
        //read data from dialog
        m_sFindText = m_pFindDialog->GetFindString();
        m_bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
        m_bLimitToDiff = m_pFindDialog->LimitToDiffs();
        m_bWholeWord = m_pFindDialog->WholeWord();

        OnEditFindnext();
    }

    return 0;
}

bool CharIsDelimiter(const CString& ch)
{
    CString delimiters(_T(" .,:;=+-*/\\\n\t()[]<>@"));
    return delimiters.Find(ch) >= 0;
}

bool CMainFrame::StringFound(const CString& str)const
{
    int nSubStringStartIdx = str.Find(m_sFindText);
    bool bStringFound = (nSubStringStartIdx >= 0);
    if (bStringFound && m_bWholeWord)
    {
        if (nSubStringStartIdx)
            bStringFound = CharIsDelimiter(str.Mid(nSubStringStartIdx-1,1));

        if (bStringFound)
        {
            int nEndIndex = nSubStringStartIdx + m_sFindText.GetLength();
            if (str.GetLength() > nEndIndex)
                bStringFound = CharIsDelimiter(str.Mid(nEndIndex, 1));
        }
    }
    return bStringFound;
}

void CMainFrame::OnEditFindprev()
{
    Search(SearchPrevious);
}

void CMainFrame::OnEditFindnext()
{
    Search(SearchNext);
}

void CMainFrame::Search(SearchDirection srchDir)
{
    if (m_sFindText.IsEmpty())
        return;
    if (!m_pwndLeftView)
        return;
    if(!m_pwndLeftView->m_pViewData)
        return;

    bool bFound = FALSE;

    CString left;
    CString right;
    CString bottom;
    DiffStates leftstate = DIFFSTATE_NORMAL;
    DiffStates rightstate = DIFFSTATE_NORMAL;
    DiffStates bottomstate = DIFFSTATE_NORMAL;
    int i = 0;

    m_nSearchIndex = FindSearchStart(m_nSearchIndex);
    m_nSearchIndex++;
    if (m_nSearchIndex >= m_pwndLeftView->m_pViewData->GetCount())
        m_nSearchIndex = 0;
    if (srchDir == SearchPrevious)
    {
        // SearchIndex points 1 past where we found the last match,
        // so if we are searching backwards we need to adjust accordingly
        m_nSearchIndex -= 2;
        // if at the top, start again from the end
        if (m_nSearchIndex < 0)
            m_nSearchIndex += m_pwndLeftView->m_pViewData->GetCount();
    }
    const int idxLimits[2][2][2]={{{m_nSearchIndex, m_pwndLeftView->m_pViewData->GetCount()},
                                       {0, m_nSearchIndex}},
                                  {{m_nSearchIndex, -1},
                                       {m_pwndLeftView->m_pViewData->GetCount()-1, m_nSearchIndex}}};
    const int offsets[2]={+1, -1};

    for (int j=0; j != 2 && !bFound; ++j)
    {
        for (i=idxLimits[srchDir][j][0]; i != idxLimits[srchDir][j][1]; i += offsets[srchDir])
        {
            left = m_pwndLeftView->m_pViewData->GetLine(i);
            leftstate = m_pwndLeftView->m_pViewData->GetState(i);
            if ((!m_bOneWay)&&(m_pwndRightView->m_pViewData))
            {
                right = m_pwndRightView->m_pViewData->GetLine(i);
                rightstate = m_pwndRightView->m_pViewData->GetState(i);
            }
            if ((m_pwndBottomView)&&(m_pwndBottomView->m_pViewData))
            {
                bottom = m_pwndBottomView->m_pViewData->GetLine(i);
                bottomstate = m_pwndBottomView->m_pViewData->GetState(i);
            }

            if (!m_bMatchCase)
            {
                left = left.MakeLower();
                right = right.MakeLower();
                bottom = bottom.MakeLower();
                m_sFindText = m_sFindText.MakeLower();
            }
            if (StringFound(left))
            {
                if ((!m_bLimitToDiff)||(leftstate != DIFFSTATE_NORMAL))
                {
                    bFound = TRUE;
                    break;
                }
            }
            else if (StringFound(right))
            {
                if ((!m_bLimitToDiff)||(rightstate != DIFFSTATE_NORMAL))
                {
                    bFound = TRUE;
                    break;
                }
            }
            else if (StringFound(bottom))
            {
                if ((!m_bLimitToDiff)||(bottomstate != DIFFSTATE_NORMAL))
                {
                    bFound = TRUE;
                    break;
                }
            }
        }
    }
    if (!bFound)
    {
        m_nSearchIndex = 0;
        return;
    }

    m_nSearchIndex = i;
    m_pwndLeftView->GoToLine(m_nSearchIndex);
    if (StringFound(left))
    {
        m_pwndLeftView->SetFocus();
        m_pwndLeftView->HiglightLines(m_nSearchIndex);
    }
    else if (StringFound(right))
    {
        m_pwndRightView->SetFocus();
        m_pwndRightView->HiglightLines(m_nSearchIndex);
    }
    else if (StringFound(bottom))
    {
        m_pwndBottomView->SetFocus();
        m_pwndBottomView->HiglightLines(m_nSearchIndex);
    }
    m_nMoveMovesToIgnore = MOVESTOIGNORE;
}

int CMainFrame::FindSearchStart(int nDefault)
{
    // TortoiseMerge doesn't have a cursor which we could use to
    // anchor the search on.
    // Instead we use a line that is selected.
    // If however no line is selected, use the default line (which could
    // be the top of the document for a new search, or the line where the
    // search was successful on)
    int nLine = nDefault;
    int nSelStart = 0;
    int nSelEnd = 0;
    if (m_pwndLeftView)
    {
        if (m_pwndLeftView->GetSelection(nSelStart, nSelEnd))
        {
            if (nSelStart == nSelEnd)
                nLine = nSelStart;
        }
    }
    else if ((nLine == nDefault)&&(m_pwndRightView))
    {
        if (m_pwndRightView->GetSelection(nSelStart, nSelEnd))
        {
            if (nSelStart == nSelEnd)
                nLine = nSelStart;
        }
    }
    else if ((nLine == nDefault)&&(m_pwndBottomView))
    {
        if (m_pwndBottomView->GetSelection(nSelStart, nSelEnd))
        {
            if (nSelStart == nSelEnd)
                nLine = nSelStart;
        }
    }
    return nLine;
}

void CMainFrame::OnViewLinedown()
{
    OnViewLineUpDown(1);
}

void CMainFrame::OnViewLineup()
{
    OnViewLineUpDown(-1);
}

void CMainFrame::OnViewLineUpDown(int direction)
{
    if (m_pwndLeftView)
        m_pwndLeftView->ScrollToLine(m_pwndLeftView->m_nTopLine+direction);
    if (m_pwndRightView)
        m_pwndRightView->ScrollToLine(m_pwndRightView->m_nTopLine+direction);
    if (m_pwndBottomView)
        m_pwndBottomView->ScrollToLine(m_pwndBottomView->m_nTopLine+direction);
    m_wndLocatorBar.Invalidate();
    m_nMoveMovesToIgnore = MOVESTOIGNORE;
}

void CMainFrame::OnViewLineleft()
{
    OnViewLineLeftRight(-1);
}

void CMainFrame::OnViewLineright()
{
    OnViewLineLeftRight(1);
}

void CMainFrame::OnViewLineLeftRight(int direction)
{
    if (m_pwndLeftView)
        m_pwndLeftView->ScrollSide(direction);
    if (m_pwndRightView)
        m_pwndRightView->ScrollSide(direction);
    if (m_pwndBottomView)
        m_pwndBottomView->ScrollSide(direction);
}

void CMainFrame::OnEditUseTheirs()
{
    if (m_pwndBottomView)
        m_pwndBottomView->UseTheirTextBlock();
}
void CMainFrame::OnUpdateEditUsetheirblock(CCmdUI *pCmdUI)
{
    int nSelBlockStart = -1;
    int nSelBlockEnd = -1;
    if (m_pwndBottomView)
        m_pwndBottomView->GetSelection(nSelBlockStart, nSelBlockEnd);
    pCmdUI->Enable((nSelBlockStart >= 0)&&(nSelBlockEnd >= 0));
}

void CMainFrame::OnEditUseMine()
{
    if (m_pwndBottomView)
        m_pwndBottomView->UseMyTextBlock();
}
void CMainFrame::OnUpdateEditUsemyblock(CCmdUI *pCmdUI)
{
    OnUpdateEditUsetheirblock(pCmdUI);
}

void CMainFrame::OnEditUseTheirsThenMine()
{
    if (m_pwndBottomView)
        m_pwndBottomView->UseTheirThenMyTextBlock();
}

void CMainFrame::OnUpdateEditUsetheirthenmyblock(CCmdUI *pCmdUI)
{
    OnUpdateEditUsetheirblock(pCmdUI);
}

void CMainFrame::OnEditUseMineThenTheirs()
{
    if (m_pwndBottomView)
        m_pwndBottomView->UseMyThenTheirTextBlock();
}

void CMainFrame::OnUpdateEditUseminethentheirblock(CCmdUI *pCmdUI)
{
    OnUpdateEditUsetheirblock(pCmdUI);
}

void CMainFrame::OnEditUseleftblock()
{
    if (m_pwndRightView)
        m_pwndRightView->UseBlock();
}

void CMainFrame::OnUpdateEditUseleftblock(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_pwndRightView && m_pwndRightView->IsWindowVisible() && m_pwndRightView->HasCaret() && m_pwndRightView->HasSelection());
}

void CMainFrame::OnEditUseleftfile()
{
    if (m_pwndRightView)
        m_pwndRightView->UseFile();
}

void CMainFrame::OnUpdateEditUseleftfile(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_pwndRightView && m_pwndRightView->IsWindowVisible() && m_pwndRightView->HasCaret());
}

void CMainFrame::OnEditUseblockfromleftbeforeright()
{
    if (m_pwndRightView)
        m_pwndRightView->UseLeftBeforeRight();
}

void CMainFrame::OnUpdateEditUseblockfromleftbeforeright(CCmdUI *pCmdUI)
{
    OnUpdateEditUseleftblock(pCmdUI);
}

void CMainFrame::OnEditUseblockfromrightbeforeleft()
{
    if (m_pwndRightView)
        m_pwndRightView->UseRightBeforeLeft();
}

void CMainFrame::OnUpdateEditUseblockfromrightbeforeleft(CCmdUI *pCmdUI)
{
    OnUpdateEditUseleftblock(pCmdUI);
}

void CMainFrame::OnFileReload()
{
    if (CheckForSave()==IDCANCEL)
        return;
    CDiffColors::GetInstance().LoadRegistry();
    LoadViews(-1);
}

void CMainFrame::ActivateFrame(int nCmdShow)
{
    // nCmdShow is the normal show mode this frame should be in
    // translate default nCmdShow (-1)
    if (nCmdShow == -1)
    {
        if (!IsWindowVisible())
            nCmdShow = SW_SHOWNORMAL;
        else if (IsIconic())
            nCmdShow = SW_RESTORE;
    }

    // bring to top before showing
    BringToTop(nCmdShow);

    if (nCmdShow != -1)
    {
        // show the window as specified
        WINDOWPLACEMENT wp;

        if ( !ReadWindowPlacement(&wp) )
        {
            ShowWindow(nCmdShow);
        }
        else
        {
            if ( nCmdShow != SW_SHOWNORMAL )
                wp.showCmd = nCmdShow;

            SetWindowPlacement(&wp);
        }

        // and finally, bring to top after showing
        BringToTop(nCmdShow);
    }
}

BOOL CMainFrame::ReadWindowPlacement(WINDOWPLACEMENT * pwp)
{
    CRegString placement = CRegString(_T("Software\\TortoiseMerge\\WindowPos"));
    CString sPlacement = placement;
    if (sPlacement.IsEmpty())
        return FALSE;
    int nRead = _stscanf_s(sPlacement, _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d"),
                &pwp->flags, &pwp->showCmd,
                &pwp->ptMinPosition.x, &pwp->ptMinPosition.y,
                &pwp->ptMaxPosition.x, &pwp->ptMaxPosition.y,
                &pwp->rcNormalPosition.left,  &pwp->rcNormalPosition.top,
                &pwp->rcNormalPosition.right, &pwp->rcNormalPosition.bottom);
    if ( nRead != 10 )
        return FALSE;
    pwp->length = sizeof(WINDOWPLACEMENT);

    return TRUE;
}

void CMainFrame::WriteWindowPlacement(WINDOWPLACEMENT * pwp)
{
    CRegString placement = CRegString(_T("Software\\TortoiseMerge\\WindowPos"));
    TCHAR szBuffer[_countof("-32767")*8 + sizeof("65535")*2];
    CString s;

    _stprintf_s(szBuffer, _countof("-32767")*8 + sizeof("65535")*2, _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d"),
            pwp->flags, pwp->showCmd,
            pwp->ptMinPosition.x, pwp->ptMinPosition.y,
            pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
            pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
            pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
    placement = szBuffer;
}

void CMainFrame::OnUpdateMergeMarkasresolved(CCmdUI *pCmdUI)
{
    if (pCmdUI == NULL)
        return;
    BOOL bEnable = FALSE;
    if ((!m_bReadOnly)&&(m_Data.m_mergedFile.InUse()))
    {
        if (m_pwndBottomView)
        {
            if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_pViewData))
            {
                bEnable = TRUE;
            }
        }
    }
    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnMergeMarkasresolved()
{
    if(HasConflictsWontKeep())
        return;

    // now check if the file has already been saved and if not, save it.
    if (m_Data.m_mergedFile.InUse())
    {
        if (m_pwndBottomView)
        {
            if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_pViewData))
            {
                FileSave(false);
            }
        }
    }
    MarkAsResolved();
}

BOOL CMainFrame::MarkAsResolved()
{
    if (m_bReadOnly)
        return FALSE;
    if ((!m_pwndBottomView)||(!m_pwndBottomView->IsWindowVisible()))
        return FALSE;

    TCHAR buf[MAX_PATH*3];
    GetModuleFileName(NULL, buf, MAX_PATH);
    TCHAR * end = _tcsrchr(buf, '\\');
    end++;
    (*end) = 0;
    _tcscat_s(buf, MAX_PATH*3, _T("TortoiseProc.exe /command:resolve /path:\""));
    _tcscat_s(buf, MAX_PATH*3, m_Data.m_mergedFile.GetFilename());
    _tcscat_s(buf, MAX_PATH*3, _T("\" /closeonend:1 /noquestion /skipcheck"));
    if(!RunCommand(buf))
        return FALSE;
    return TRUE;
}

void CMainFrame::OnUpdateMergeNextconflict(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if ((m_pwndBottomView)&&(m_pwndBottomView->HasCaret())&&(m_pwndBottomView->HasNextConflict()))
        bShow = TRUE;
    if ((m_pwndRightView)&&(m_pwndRightView->HasCaret())&&(m_pwndRightView->HasNextConflict()))
        bShow = TRUE;
    if ((m_pwndLeftView)&&(m_pwndLeftView->HasCaret())&&(m_pwndLeftView->HasNextConflict()))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

void CMainFrame::OnUpdateMergePreviousconflict(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if ((m_pwndBottomView)&&(m_pwndBottomView->HasCaret())&&(m_pwndBottomView->HasPrevConflict()))
        bShow = TRUE;
    if ((m_pwndRightView)&&(m_pwndRightView->HasCaret())&&(m_pwndRightView->HasPrevConflict()))
        bShow = TRUE;
    if ((m_pwndLeftView)&&(m_pwndLeftView->HasCaret())&&(m_pwndLeftView->HasPrevConflict()))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

void CMainFrame::OnUpdateNavigateNextdifference(CCmdUI *pCmdUI)
{
    CBaseView* baseView = GetActiveBaseView();
    BOOL bShow = FALSE;
    if (baseView != 0)
        bShow = baseView->HasNextDiff();
    pCmdUI->Enable(bShow);
}

void CMainFrame::OnUpdateNavigatePreviousdifference(CCmdUI *pCmdUI)
{
    CBaseView* baseView = GetActiveBaseView();
    BOOL bShow = FALSE;
    if (baseView != 0)
        bShow = baseView->HasPrevDiff();
    pCmdUI->Enable(bShow);
}

void CMainFrame::OnUpdateNavigateNextinlinediff(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if ((m_pwndBottomView)&&(m_pwndBottomView->HasCaret())&&(m_pwndBottomView->HasNextInlineDiff()))
        bShow = TRUE;
    if ((m_pwndRightView)&&(m_pwndRightView->HasCaret())&&(m_pwndRightView->HasNextInlineDiff()))
        bShow = TRUE;
    if ((m_pwndLeftView)&&(m_pwndLeftView->HasCaret())&&(m_pwndLeftView->HasNextInlineDiff()))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

void CMainFrame::OnUpdateNavigatePrevinlinediff(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if ((m_pwndBottomView)&&(m_pwndBottomView->HasCaret())&&(m_pwndBottomView->HasPrevInlineDiff()))
        bShow = TRUE;
    if ((m_pwndRightView)&&(m_pwndRightView->HasCaret())&&(m_pwndRightView->HasPrevInlineDiff()))
        bShow = TRUE;
    if ((m_pwndLeftView)&&(m_pwndLeftView->HasCaret())&&(m_pwndLeftView->HasPrevInlineDiff()))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

void CMainFrame::OnMoving(UINT fwSide, LPRECT pRect)
{
    // if the pathfilelist dialog is attached to the mainframe,
    // move it along with the mainframe
    if (::IsWindow(m_dlgFilePatches.m_hWnd))
    {
        RECT patchrect;
        m_dlgFilePatches.GetWindowRect(&patchrect);
        if (::IsWindow(m_hWnd))
        {
            RECT thisrect;
            GetWindowRect(&thisrect);
            if (patchrect.right == thisrect.left)
            {
                m_dlgFilePatches.SetWindowPos(NULL, patchrect.left - (thisrect.left - pRect->left), patchrect.top - (thisrect.top - pRect->top),
                    0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            }
        }
    }
    __super::OnMoving(fwSide, pRect);
}

void CMainFrame::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if ((m_pwndBottomView)&&(m_pwndBottomView->HasSelection()))
        bShow = TRUE;
    if ((m_pwndRightView)&&(m_pwndRightView->HasSelection()))
        bShow = TRUE;
    if ((m_pwndLeftView)&&(m_pwndLeftView->HasSelection()))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

void CMainFrame::OnViewSwitchleft()
{
    int ret = IDNO;
    if (((m_pwndBottomView)&&(m_pwndBottomView->IsModified())) ||
        ((m_pwndRightView)&&(m_pwndRightView->IsModified())))
    {
        CString sTemp;
        sTemp.LoadString(IDS_ASKFORSAVE);
        ret = MessageBox(sTemp, 0, MB_YESNOCANCEL | MB_ICONQUESTION);
        if (ret == IDYES)
        {
            if (!FileSave())
                return;
        }
    }
    if ((ret == IDNO)||(ret == IDYES))
    {
        CWorkingFile file = m_Data.m_baseFile;
        m_Data.m_baseFile = m_Data.m_yourFile;
        m_Data.m_yourFile = file;
        if (m_Data.m_mergedFile.GetFilename().CompareNoCase(m_Data.m_yourFile.GetFilename())==0)
        {
            m_Data.m_mergedFile = m_Data.m_baseFile;
        }
        else if (m_Data.m_mergedFile.GetFilename().CompareNoCase(m_Data.m_baseFile.GetFilename())==0)
        {
            m_Data.m_mergedFile = m_Data.m_yourFile;
        }
        LoadViews();
    }
}

void CMainFrame::OnUpdateViewSwitchleft(CCmdUI *pCmdUI)
{
    BOOL bEnable = TRUE;
    if (m_pwndBottomView)
    {
        if (m_pwndBottomView->IsWindowVisible())
            bEnable = FALSE;
    }
    pCmdUI->Enable(bEnable);
}


void CMainFrame::OnUpdateViewShowfilelist(CCmdUI *pCmdUI)
{
    if (m_dlgFilePatches.HasFiles())
    {
        pCmdUI->Enable(true);
    }
    else
        pCmdUI->Enable(false);
    pCmdUI->SetCheck(m_dlgFilePatches.IsWindowVisible());
}

void CMainFrame::OnViewShowfilelist()
{
    m_dlgFilePatches.ShowWindow(m_dlgFilePatches.IsWindowVisible() ? SW_HIDE : SW_SHOW);
}

void CMainFrame::OnEditUndo()
{
    if (CUndo::GetInstance().CanUndo())
    {
        CUndo::GetInstance().Undo(m_pwndLeftView, m_pwndRightView, m_pwndBottomView);
    }
}

void CMainFrame::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(CUndo::GetInstance().CanUndo());
}

int CMainFrame::CheckForSave()
{
    int ret = IDNO;
    if (((m_pwndBottomView)&&(m_pwndBottomView->IsModified())) ||
        ((m_pwndRightView)&&(m_pwndRightView->IsModified())))
    {
        CString sTemp;
        sTemp.LoadString(IDS_WARNMODIFIEDLOOSECHANGES);
        ret = MessageBox(sTemp, 0, MB_YESNOCANCEL | MB_ICONQUESTION);

        if (ret == IDYES)
        {
            FileSave();
        }
    }
    return ret;
}

void CMainFrame::OnViewInlinediffword()
{
    m_bInlineWordDiff = !m_bInlineWordDiff;
    if (m_pwndLeftView)
    {
        m_pwndLeftView->SetInlineWordDiff(m_bInlineWordDiff);
        m_pwndLeftView->Invalidate();
    }
    if (m_pwndRightView)
    {
        m_pwndRightView->SetInlineWordDiff(m_bInlineWordDiff);
        m_pwndRightView->Invalidate();
    }
    if (m_pwndBottomView)
    {
        m_pwndBottomView->SetInlineWordDiff(m_bInlineWordDiff);
        m_pwndBottomView->Invalidate();
    }
    m_wndLineDiffBar.Invalidate();
}

void CMainFrame::OnUpdateViewInlinediffword(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_pwndLeftView && m_pwndLeftView->IsWindowVisible() &&
        m_pwndRightView && m_pwndRightView->IsWindowVisible());
    pCmdUI->SetCheck(m_bInlineWordDiff);
}

void CMainFrame::OnUpdateEditCreateunifieddifffile(CCmdUI *pCmdUI)
{
    // "create unified diff file" is only available if two files
    // are diffed, not three.
    bool bEnabled = true;
    if ((m_pwndLeftView == NULL)||(!m_pwndLeftView->IsWindowVisible()))
        bEnabled = false;
    if ((m_pwndRightView == NULL)||(!m_pwndRightView->IsWindowVisible()))
        bEnabled = false;
    if ((m_pwndBottomView)&&(m_pwndBottomView->IsWindowVisible()))
        bEnabled = false;
    pCmdUI->Enable(bEnabled);
}

void CMainFrame::OnEditCreateunifieddifffile()
{
    CString origFile, modifiedFile;
    // the original file is the one on the left
    if (m_pwndLeftView)
        origFile = m_pwndLeftView->m_sFullFilePath;
    if (m_pwndRightView)
        modifiedFile = m_pwndRightView->m_sFullFilePath;
    if (origFile.IsEmpty() || modifiedFile.IsEmpty())
        return;

    CString outputFile;
    if(!TryGetFileName(outputFile))
        return;

    CAppUtils::CreateUnifiedDiff(origFile, modifiedFile, outputFile, true);
}

void CMainFrame::OnUpdateViewLinediffbar(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bLineDiff);
    pCmdUI->Enable();
}

void CMainFrame::OnViewLinediffbar()
{
    m_bLineDiff = !m_bLineDiff;
    m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
    m_wndLineDiffBar.DocumentUpdated();
    m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
    m_wndLocatorBar.DocumentUpdated();
}

void CMainFrame::OnUpdateViewLocatorbar(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bLocatorBar);
    pCmdUI->Enable();
}

void CMainFrame::OnViewLocatorbar()
{
    m_bLocatorBar = !m_bLocatorBar;
    m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
    m_wndLocatorBar.DocumentUpdated();
    m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
    m_wndLineDiffBar.DocumentUpdated();
}

bool CMainFrame::RunCommand(TCHAR* command)
{
    if(CCreateProcessHelper::CreateProcessDetached (NULL, command))
        return true;

    CFormatMessageWrapper errorDetails;
    MessageBox(errorDetails, _T("TortoiseMerge"), MB_OK | MB_ICONINFORMATION);
    return false;
}

void CMainFrame::OnViewComparewhitespaces()
{
    if (CheckForSave()==IDCANCEL)
        return;
    CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
    regIgnoreWS = 0;
    LoadViews(-1);
}

void CMainFrame::OnUpdateViewComparewhitespaces(CCmdUI *pCmdUI)
{
    CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
    DWORD dwIgnoreWS = regIgnoreWS;
    pCmdUI->SetCheck(dwIgnoreWS == 0);
}

void CMainFrame::OnViewIgnorewhitespacechanges()
{
    if (CheckForSave()==IDCANCEL)
        return;
    CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
    regIgnoreWS = 2;
    LoadViews(-1);
}

void CMainFrame::OnUpdateViewIgnorewhitespacechanges(CCmdUI *pCmdUI)
{
    CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
    DWORD dwIgnoreWS = regIgnoreWS;
    pCmdUI->SetCheck(dwIgnoreWS == 2);
}

void CMainFrame::OnViewIgnoreallwhitespacechanges()
{
    if (CheckForSave()==IDCANCEL)
        return;
    CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
    regIgnoreWS = 1;
    LoadViews(-1);
}

void CMainFrame::OnUpdateViewIgnoreallwhitespacechanges(CCmdUI *pCmdUI)
{
    CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
    DWORD dwIgnoreWS = regIgnoreWS;
    pCmdUI->SetCheck(dwIgnoreWS == 1);
}

void CMainFrame::OnViewMovedBlocks()
{
    CRegDWORD regShowMovedBlocks = CRegDWORD(_T("Software\\TortoiseMerge\\ViewMovedBlocks"), 0);
    m_bViewMovedBlocks = !(DWORD)regShowMovedBlocks;
    regShowMovedBlocks = m_bViewMovedBlocks;
    LoadViews(-1);
}

void CMainFrame::OnUpdateViewMovedBlocks(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bViewMovedBlocks);
    BOOL bEnable = TRUE;
    if (m_pwndBottomView)
    {
        if (m_pwndBottomView->IsWindowVisible())
            bEnable = FALSE;
    }
    pCmdUI->Enable(bEnable);
}

bool CMainFrame::HasConflictsWontKeep()
{
    const int nConflictLine = CheckResolved();
    if (nConflictLine < 0)
        return false;

    CString sTemp;
    sTemp.Format(IDS_ERR_MAINFRAME_FILEHASCONFLICTS, m_pwndBottomView->m_pViewData->GetLineNumber(nConflictLine)+1);
    if (MessageBox(sTemp, 0, MB_ICONERROR | MB_YESNO)==IDYES)
        return false;

    if (m_pwndBottomView)
        m_pwndBottomView->GoToLine(nConflictLine);
    return true;
}

bool CMainFrame::TryGetFileName(CString& result)
{
    return CCommonAppUtils::FileOpenSave(result, NULL, IDS_SAVEASTITLE, IDS_COMMONFILEFILTER, false, m_hWnd);
}

CBaseView* CMainFrame::GetActiveBaseView() const
{
    CView* activeView = GetActiveView();
    CBaseView* activeBase = dynamic_cast<CBaseView*>( activeView );
    return activeBase;
}

