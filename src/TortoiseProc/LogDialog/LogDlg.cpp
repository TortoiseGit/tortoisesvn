// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012 - TortoiseSVN

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
#include "InputDlg.h"
#include "PropDlg.h"
#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
#include "RepositoryBrowser.h"
#include "CopyDlg.h"
#include "StatGraphDlg.h"
#include "Logdlg.h"
#include "MessageBox.h"
#include "Registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "TempFile.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include "IconMenu.h"
#include "RevisionRangeDlg.h"
#include "BrowseFolder.h"
#include "BlameDlg.h"
#include "Blame.h"
#include "SVNHelpers.h"
#include "SVNStatus.h"
#include "LogDlgHelper.h"
#include "CachedLogInfo.h"
#include "RepositoryInfo.h"
#include "Properties/EditPropertiesDlg.h"
#include "LogCacheSettings.h"
#include "SysInfo.h"
#include "SysImageList.h"
#include "svn_props.h"
#include "AsyncCall.h"
#include "Future.h"
#include "svntrace.h"
#include "LogDlgFilter.h"
#include "SVNLogHelper.h"
#include "DiffOptionsDlg.h"
#include "../LogCache/Streams/StreamException.h"

#if (NTDDI_VERSION < NTDDI_LONGHORN)

enum LISTITEMSTATES_MINE {
    LISS_NORMAL = 1,
    LISS_HOT = 2,
    LISS_SELECTED = 3,
    LISS_DISABLED = 4,
    LISS_SELECTEDNOTFOCUS = 5,
    LISS_HOTSELECTED = 6,
};

#define MCS_NOTRAILINGDATES  0x0040
#define MCS_SHORTDAYSOFWEEK  0x0080
#define MCS_NOSELCHANGEONNAV 0x0100

#define DTM_SETMCSTYLE    (DTM_FIRST + 11)

#endif

#define ICONITEMBORDER 5

const UINT CLogDlg::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);


enum LogDlgContextMenuCommands
{
    // needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
    ID_COMPARE = 1,
    ID_SAVEAS,
    ID_COMPARETWO,
    ID_UPDATE,
    ID_COPY,
    ID_REVERTREV,
    ID_MERGEREV,
    ID_GNUDIFF1,
    ID_GNUDIFF2,
    ID_FINDENTRY,
    ID_OPEN,
    ID_BLAME,
    ID_REPOBROWSE,
    ID_LOG,
    ID_POPPROPS,
    ID_EDITAUTHOR,
    ID_EDITLOG,
    ID_DIFF,
    ID_OPENWITH,
    ID_COPYCLIPBOARD,
    ID_CHECKOUT,
    ID_REVERTTOREV,
    ID_BLAMECOMPARE,
    ID_BLAMETWO,
    ID_BLAMEDIFF,
    ID_VIEWREV,
    ID_VIEWPATHREV,
    ID_EXPORT,
    ID_EXPORTTREE,
    ID_COMPAREWITHPREVIOUS,
    ID_BLAMEWITHPREVIOUS,
    ID_GETMERGELOGS,
    ID_REVPROPS
};

enum LogDlgShowBtnCommands
{
    ID_CMD_DEFAULT = 0,
    ID_CMD_SHOWALL,
    ID_CMD_SHOWRANGE,
};


IMPLEMENT_DYNAMIC(CLogDlg, CResizableStandAloneDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CLogDlg::IDD, pParent)
    , m_nSearchIndex(0)
    , m_wParam(0)
    , m_SelectedFilters(LOGFILTER_ALL)
    , m_nSortColumn(0)
    , m_bShowedAll(false)
    , m_bSelect(false)
    , m_regLastStrict(_T("Software\\TortoiseSVN\\LastLogStrict"), FALSE)
    , m_regMaxBugIDColWidth(_T("Software\\TortoiseSVN\\MaxBugIDColWidth"), 200)
    , m_bSelectionMustBeContinuous(false)
    , m_bShowBugtraqColumn(false)
    , m_bStrictStopped(false)
    , m_bSingleRevision(true)
    , m_sLogInfo(_T(""))
    , m_pFindDialog(NULL)
    , m_bCancelled(FALSE)
    , m_pNotifyWindow(NULL)
    , m_bLogThreadRunning(FALSE)
    , m_bAscending(FALSE)
    , m_pStoreSelection(NULL)
    , m_limit(0)
    , m_bIncludeMerges(FALSE)
    , m_hAccel(NULL)
    , m_nRefresh(None)
    , netScheduler(1, 0, true)
    , diskScheduler(1, 0, true)
    , m_pLogListAccServer(NULL)
    , m_pChangedListAccServer(NULL)
    , m_head(-1)
    , m_nSortColumnPathList(0)
    , m_bAscendingPathList(false)
    , m_bHideNonMergeables(FALSE)
    , m_copyfromrev(0)
    , m_bStartRevIsHead(true)
    , m_boldFont(NULL)
    , m_bStrict(false)
    , m_bSaveStrict(false)
    , m_hasWC(false)
    , m_hModifiedIcon(NULL)
    , m_hReplacedIcon(NULL)
    , m_hAddedIcon(NULL)
    , m_hDeletedIcon(NULL)
    , m_hMergedIcon(NULL)
    , m_hReverseMergedIcon(NULL)
    , m_nIconFolder(0)
    , m_prevLogEntriesSize(0)
{
    m_bFilterWithRegex =
        !!CRegDWORD(_T("Software\\TortoiseSVN\\UseRegexFilter"), FALSE);
    m_bFilterCaseSensitively =
        !!CRegDWORD(_T("Software\\TortoiseSVN\\FilterCaseSensitively"), FALSE);
}

CLogDlg::~CLogDlg()
{
    // prevent further event processing

    InterlockedExchange(&m_bLogThreadRunning, TRUE);
    m_logEntries.ClearAll();
    DestroyIcon(m_hModifiedIcon);
    DestroyIcon(m_hReplacedIcon);
    DestroyIcon(m_hAddedIcon);
    DestroyIcon(m_hDeletedIcon);
    DestroyIcon(m_hMergedIcon);
    DestroyIcon(m_hReverseMergedIcon);
    if ( m_pStoreSelection )
    {
        m_pStoreSelection->ClearSelection();
        delete m_pStoreSelection;
        m_pStoreSelection = NULL;
    }
    if (m_boldFont)
        DeleteObject(m_boldFont);
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LOGLIST, m_LogList);
    DDX_Control(pDX, IDC_LOGMSG, m_ChangedFileListCtrl);
    DDX_Control(pDX, IDC_PROGRESS, m_LogProgress);
    DDX_Control(pDX, IDC_SPLITTERTOP, m_wndSplitter1);
    DDX_Control(pDX, IDC_SPLITTERBOTTOM, m_wndSplitter2);
    DDX_Check(pDX, IDC_CHECK_STOPONCOPY, m_bStrict);
    DDX_Text(pDX, IDC_SEARCHEDIT, m_sFilterText);
    DDX_Control(pDX, IDC_DATEFROM, m_DateFrom);
    DDX_Control(pDX, IDC_DATETO, m_DateTo);
    DDX_Control(pDX, IDC_SHOWPATHS, m_cShowPaths);
    DDX_Control(pDX, IDC_GETALL, m_btnShow);
    DDX_Text(pDX, IDC_LOGINFO, m_sLogInfo);
    DDX_Check(pDX, IDC_INCLUDEMERGE, m_bIncludeMerges);
    DDX_Control(pDX, IDC_SEARCHEDIT, m_cFilter);
    DDX_Check(pDX, IDC_HIDENONMERGEABLE, m_bHideNonMergeables);
}

BEGIN_MESSAGE_MAP(CLogDlg, CResizableStandAloneDialog)
    ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
    ON_BN_CLICKED(IDC_GETALL, OnBnClickedGetall)
    ON_NOTIFY(NM_DBLCLK, IDC_LOGMSG, OnNMDblclkChangedFileList)
    ON_NOTIFY(NM_DBLCLK, IDC_LOGLIST, OnNMDblclkLoglist)
    ON_WM_CONTEXTMENU()
    ON_WM_SETCURSOR()
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOGLIST, OnLvnItemchangedLoglist)
    ON_NOTIFY(EN_LINK, IDC_MSGVIEW, OnEnLinkMsgview)
    ON_BN_CLICKED(IDC_STATBUTTON, OnBnClickedStatbutton)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOGLIST, OnNMCustomdrawLoglist)
    ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_INFOCLICKED, OnClickedInfoIcon)
    ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_CANCELCLICKED, OnClickedCancelFilter)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_LOGLIST, OnLvnGetdispinfoLoglist)
    ON_EN_CHANGE(IDC_SEARCHEDIT, OnEnChangeSearchedit)
    ON_WM_TIMER()
    ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETO, OnDtnDatetimechangeDateto)
    ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATEFROM, OnDtnDatetimechangeDatefrom)
    ON_BN_CLICKED(IDC_NEXTHUNDRED, OnBnClickedNexthundred)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOGMSG, OnNMCustomdrawChangedFileList)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_LOGMSG, OnLvnGetdispinfoChangedFileList)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_LOGLIST, OnLvnColumnclick)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_LOGMSG, OnLvnColumnclickChangedFileList)
    ON_BN_CLICKED(IDC_SHOWPATHS, OnBnClickedHidepaths)
    ON_NOTIFY(LVN_ODFINDITEM, IDC_LOGLIST, OnLvnOdfinditemLoglist)
    ON_BN_CLICKED(IDC_CHECK_STOPONCOPY, &CLogDlg::OnBnClickedCheckStoponcopy)
    ON_NOTIFY(DTN_DROPDOWN, IDC_DATEFROM, &CLogDlg::OnDtnDropdownDatefrom)
    ON_NOTIFY(DTN_DROPDOWN, IDC_DATETO, &CLogDlg::OnDtnDropdownDateto)
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_INCLUDEMERGE, &CLogDlg::OnBnClickedIncludemerge)
    ON_BN_CLICKED(IDC_REFRESH, &CLogDlg::OnBnClickedRefresh)
    ON_BN_CLICKED(IDC_HIDENONMERGEABLE, &CLogDlg::OnEnChangeSearchedit)
    ON_BN_CLICKED(IDC_LOGCANCEL,&CLogDlg::OnLogCancel)
    ON_COMMAND(ID_LOGDLG_REFRESH,&CLogDlg::OnRefresh)
    ON_COMMAND(ID_LOGDLG_FIND,&CLogDlg::OnFind)
    ON_COMMAND(ID_LOGDLG_FOCUSFILTER,&CLogDlg::OnFocusFilter)
    ON_COMMAND(ID_EDIT_COPY, &CLogDlg::OnEditCopy)
    ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, &CLogDlg::OnLvnKeydownLoglist)
    ON_NOTIFY(LVN_KEYDOWN, IDC_LOGMSG, &CLogDlg::OnLvnKeydownFilelist)
    ON_NOTIFY(NM_CLICK, IDC_LOGLIST, &CLogDlg::OnNMClickLoglist)
    ON_EN_VSCROLL(IDC_MSGVIEW, &CLogDlg::OnEnscrollMsgview)
    ON_EN_HSCROLL(IDC_MSGVIEW, &CLogDlg::OnEnscrollMsgview)
    ON_WM_CLOSE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

void CLogDlg::SetParams(const CTSVNPath& path, SVNRev pegrev, SVNRev startrev, SVNRev endrev, BOOL bStrict /* = FALSE */, BOOL bSaveStrict /* = TRUE */, int limit)
{
    m_path = path;
    m_pegrev = pegrev;
    m_startrev = startrev;
    m_bStartRevIsHead = !!m_startrev.IsHead();
    m_LogRevision = startrev;
    m_endrev = endrev;
    m_hasWC = !path.IsUrl();
    m_bStrict = bStrict;
    m_bSaveStrict = bSaveStrict;
    m_limit = limit;
    if (::IsWindow(m_hWnd))
        UpdateData(FALSE);
}

void CLogDlg::SetFilter(const CString& findstr, LONG findtype, bool findregex)
{
    m_sFilterText = findstr;
    if (findtype)
        m_SelectedFilters = findtype;
    m_bFilterWithRegex = findregex;
}


void CLogDlg::SetSelectedRevRanges( const SVNRevRangeArray& revArray )
{
    delete m_pStoreSelection;
    m_pStoreSelection = NULL;

    m_pStoreSelection = new CStoreSelection(this, revArray);
}

BOOL CLogDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_LOGMSG, IDC_SEARCHEDIT, IDC_LOGMSG, IDC_LOGMSG);
    m_aeroControls.SubclassControl(this, IDC_LOGINFO);
    m_aeroControls.SubclassControl(this, IDC_HIDENONMERGEABLE);
    m_aeroControls.SubclassControl(this, IDC_SHOWPATHS);
    m_aeroControls.SubclassControl(this, IDC_STATBUTTON);
    m_aeroControls.SubclassControl(this, IDC_CHECK_STOPONCOPY);
    m_aeroControls.SubclassControl(this, IDC_INCLUDEMERGE);
    m_aeroControls.SubclassControl(this, IDC_PROGRESS);
    m_aeroControls.SubclassControl(this, IDC_GETALL);
    m_aeroControls.SubclassControl(this, IDC_NEXTHUNDRED);
    m_aeroControls.SubclassControl(this, IDC_REFRESH);
    m_aeroControls.SubclassControl(this, IDC_LOGCANCEL);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_pTaskbarList.Release();
    if (FAILED(m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
        m_pTaskbarList = nullptr;

    // use the default GUI font, create a copy of it and
    // change the copy to BOLD (leave the rest of the font
    // the same)
    HFONT hFont = (HFONT)m_LogList.SendMessage(WM_GETFONT);
    LOGFONT lf = {0};
    GetObject(hFont, sizeof(LOGFONT), &lf);
    lf.lfWeight = FW_BOLD;
    m_boldFont = CreateFontIndirect(&lf);

    EnableToolTips();
    m_LogList.SetTooltipProvider(this);

    m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_LOGDLG));

    // use the state of the "stop on copy/rename" option from the last time
    if (!m_bStrict)
        m_bStrict = m_regLastStrict;
    UpdateData(FALSE);
    CString temp;
    if (m_limit)
        temp.Format(IDS_LOG_SHOWNEXT, m_limit);
    else
        temp.Format(IDS_LOG_SHOWNEXT, (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100));

    SetDlgItemText(IDC_NEXTHUNDRED, temp);

    // set the font to use in the log message view, configured in the settings dialog
    CAppUtils::CreateFontForLogs(m_logFont);
    GetDlgItem(IDC_MSGVIEW)->SetFont(&m_logFont);
    // automatically detect URLs in the log message and turn them into links
    GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_AUTOURLDETECT, TRUE, NULL);
    // make the log message rich edit control send a message when the mouse pointer is over a link
    GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK|ENM_SCROLL);
    DWORD dwStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;

    // we *could* enable checkboxes on pre Vista OS too, but those don't have
    // the LVS_EX_AUTOCHECKSELECT style. Without that style, users could get
    // very confused because selected items are not checked.
    // Also, while handling checkboxes is implemented, most code paths in this
    // file still only work on the selected items, not the checked ones.
    if (m_bSelect && SysInfo::Instance().IsVistaOrLater())
        dwStyle |= LVS_EX_CHECKBOXES | 0x08000000 /*LVS_EX_AUTOCHECKSELECT*/;
    m_LogList.SetExtendedStyle(dwStyle);

    int checkState = (int)DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\LogShowPaths"), BST_UNCHECKED));
    m_cShowPaths.SetCheck(checkState);

    // load the icons for the action columns
    m_hModifiedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hAddedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hMergedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMERGED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hReverseMergedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREVERSEMERGED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    // if there is a working copy, load the project properties
    // to get information about the bugtraq: integration
    if (m_hasWC)
        m_ProjectProperties.ReadProps(m_path);

    // the bugtraq issue id column is only shown if the bugtraq:url or bugtraq:regex is set
    if ((!m_ProjectProperties.sUrl.IsEmpty())||(!m_ProjectProperties.GetCheckRe().IsEmpty()))
        m_bShowBugtraqColumn = true;

    SetWindowTheme(m_LogList.GetSafeHwnd(), L"Explorer", NULL);
    SetWindowTheme(m_ChangedFileListCtrl.GetSafeHwnd(), L"Explorer", NULL);

    // set up the columns
    int c = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_LogList.DeleteColumn(c--);
    temp.LoadString(IDS_LOG_REVISION);
    m_LogList.InsertColumn(0, temp);

    // make the revision column right aligned
    LVCOLUMN Column;
    Column.mask = LVCF_FMT;
    Column.fmt = LVCFMT_RIGHT;
    m_LogList.SetColumn(0, &Column);

    temp.LoadString(IDS_LOG_ACTIONS);
    m_LogList.InsertColumn(1, temp);
    temp.LoadString(IDS_LOG_AUTHOR);
    m_LogList.InsertColumn(2, temp);
    temp.LoadString(IDS_LOG_DATE);
    m_LogList.InsertColumn(3, temp);
    if (m_bShowBugtraqColumn)
    {
        temp = m_ProjectProperties.sLabel;
        if (temp.IsEmpty())
            temp.LoadString(IDS_LOG_BUGIDS);
        m_LogList.InsertColumn(4, temp);
    }
    temp.LoadString(IDS_LOG_MESSAGE);
    m_LogList.InsertColumn(m_bShowBugtraqColumn ? 5 : 4, temp);
    m_LogList.SetRedraw(false);
    ResizeAllListCtrlCols(true);
    m_LogList.SetRedraw(true);

    m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
    m_ChangedFileListCtrl.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
    m_ChangedFileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
    c = ((CHeaderCtrl*)(m_ChangedFileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_ChangedFileListCtrl.DeleteColumn(c--);
    temp.LoadString(IDS_PROGRS_PATH);
    m_ChangedFileListCtrl.InsertColumn(0, temp);
    temp.LoadString(IDS_PROGRS_ACTION);
    m_ChangedFileListCtrl.InsertColumn(1, temp);
    temp.LoadString(IDS_LOG_COPYFROM);
    m_ChangedFileListCtrl.InsertColumn(2, temp);
    temp.LoadString(IDS_LOG_REVISION);
    m_ChangedFileListCtrl.InsertColumn(3, temp);
    m_ChangedFileListCtrl.SetRedraw(false);
    CAppUtils::ResizeAllListCtrlCols(&m_ChangedFileListCtrl);
    m_ChangedFileListCtrl.SetRedraw(true);


    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
    m_sMessageBuf.Preallocate(100000);

    // set the dialog title to "Log - path/to/whatever/we/show/the/log/for"
    SetDlgTitle(false);

    m_tooltips.Create(this);
    CheckRegexpTooltip();

    // the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
    m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
    m_cFilter.SetInfoIcon(IDI_LOGFILTER);
    m_cFilter.SetValidator(this);
    m_cFilter.SetWindowText(m_sFilterText);

    AdjustControlSize(IDC_SHOWPATHS);
    AdjustControlSize(IDC_CHECK_STOPONCOPY);
    AdjustControlSize(IDC_INCLUDEMERGE);
    AdjustControlSize(IDC_HIDENONMERGEABLE);

    GetClientRect(m_DlgOrigRect);
    m_LogList.GetClientRect(m_LogListOrigRect);
    GetDlgItem(IDC_MSGVIEW)->GetClientRect(m_MsgViewOrigRect);
    m_ChangedFileListCtrl.GetClientRect(m_ChgOrigRect);

    m_DateFrom.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);
    m_DateTo.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);

    // resizable stuff
    AddAnchor(IDC_FROMLABEL, TOP_RIGHT);
    AddAnchor(IDC_DATEFROM, TOP_RIGHT);
    AddAnchor(IDC_TOLABEL, TOP_RIGHT);
    AddAnchor(IDC_DATETO, TOP_RIGHT);

    SetFilterCueText();
    AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);

    AddMainAnchors();

    AddAnchor(IDC_LOGINFO, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SHOWPATHS, BOTTOM_LEFT);
    AddAnchor(IDC_CHECK_STOPONCOPY, BOTTOM_LEFT);
    AddAnchor(IDC_INCLUDEMERGE, BOTTOM_LEFT);
    AddAnchor(IDC_GETALL, BOTTOM_LEFT);
    AddAnchor(IDC_NEXTHUNDRED, BOTTOM_LEFT);
    AddAnchor(IDC_REFRESH, BOTTOM_LEFT);
    AddAnchor(IDC_STATBUTTON, BOTTOM_RIGHT);
    AddAnchor(IDC_HIDENONMERGEABLE, BOTTOM_LEFT);
    AddAnchor(IDC_PROGRESS, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDC_LOGCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    SetPromptParentWindow(m_hWnd);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(_T("LogDlg"));

    SetSplitterRange();

    DWORD yPos1 = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1"));
    DWORD yPos2 = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2"));
    RECT rcDlg, rcLogList, rcChgMsg;
    GetClientRect(&rcDlg);
    m_LogList.GetWindowRect(&rcLogList);
    ScreenToClient(&rcLogList);
    m_ChangedFileListCtrl.GetWindowRect(&rcChgMsg);
    ScreenToClient(&rcChgMsg);

    if (yPos1 && ((LONG)yPos1 < rcDlg.bottom - 185))
    {
        RECT rectSplitter;
        m_wndSplitter1.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        int delta = yPos1 - rectSplitter.top;

        if ((rcLogList.bottom + delta > rcLogList.top)&&(rcLogList.bottom + delta < rcChgMsg.bottom - 30))
        {
            m_wndSplitter1.SetWindowPos(NULL, rectSplitter.left, yPos1, 0, 0, SWP_NOSIZE);
            DoSizeV1(delta);
        }
    }
    if (yPos2 && ((LONG)yPos2 < rcDlg.bottom - 153))
    {
        RECT rectSplitter;
        m_wndSplitter2.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        int delta = yPos2 - rectSplitter.top;

        if ((rcChgMsg.top + delta < rcChgMsg.bottom)&&(rcChgMsg.top + delta > rcLogList.top + 30))
        {
            m_wndSplitter2.SetWindowPos(NULL, rectSplitter.left, yPos2, 0, 0, SWP_NOSIZE);
            DoSizeV2(delta);
        }
    }

    SetSplitterRange();
    if (m_bSelect)
    {
        // the dialog is used to select revisions
        if (m_bSelectionMustBeContinuous)
            DialogEnableWindow(IDOK, (m_LogList.GetSelectedCount()!=0)&&(IsSelectionContinuous()));
        else
            DialogEnableWindow(IDOK, m_LogList.GetSelectedCount()!=0);
    }
    else
    {
        // the dialog is used to just view log messages
        GetDlgItemText(IDOK, temp);
        SetDlgItemText(IDC_LOGCANCEL, temp);
        GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
    }
    m_btnMenu.CreatePopupMenu();
    m_btnMenu.AppendMenu(MF_STRING|MF_BYCOMMAND, ID_CMD_SHOWALL, CString(MAKEINTRESOURCE(IDS_LOG_SHOWALL)));
    m_btnMenu.AppendMenu(MF_STRING|MF_BYCOMMAND, ID_CMD_SHOWRANGE, CString(MAKEINTRESOURCE(IDS_LOG_SHOWRANGE)));
    m_btnShow.m_hMenu = m_btnMenu.GetSafeHmenu();
    m_btnShow.m_bOSMenu = TRUE;
    m_btnShow.m_bRightArrow = TRUE;
    m_btnShow.m_bDefaultClick = TRUE;
    m_btnShow.m_bTransparent = TRUE;
    switch ((LONG)CRegDWORD(_T("Software\\TortoiseSVN\\ShowAllEntry")))
    {
    default:
    case 0:
        m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWALL)));
        break;
    case  1:
        m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWRANGE)));
        break;
    }

    m_mergedRevs.clear();

    // set up the accessibility callback
    m_pLogListAccServer = ListViewAccServer::CreateProvider(m_LogList.GetSafeHwnd(), this);
    m_pChangedListAccServer = ListViewAccServer::CreateProvider(m_ChangedFileListCtrl.GetSafeHwnd(), this);

    // show/hide the date filter controls according to the filter setting
    AdjustDateFilterVisibility();

    // first start a thread to obtain the log messages without
    // blocking the dialog
    InterlockedExchange(&m_bLogThreadRunning, TRUE);
    new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
    GetDlgItem(IDC_LOGLIST)->SetFocus();
    return FALSE;
}

void CLogDlg::SetDlgTitle(bool bOffline)
{
    if (m_sTitle.IsEmpty())
        GetWindowText(m_sTitle);

    if (bOffline)
    {
        CString sTemp;
        sTemp.FormatMessage(IDS_LOG_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle);
        CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sTemp);
    }
    else
    {
        CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), m_sTitle);
    }
}

void CLogDlg::CheckRegexpTooltip()
{
    CWnd *pWnd = GetDlgItem(IDC_SEARCHEDIT);

    m_tooltips.DelTool(pWnd);
    m_tooltips.AddTool(pWnd, m_bFilterWithRegex ? IDS_LOG_FILTER_REGEX_TT
                                                : IDS_LOG_FILTER_SUBSTRING_TT);
}

void CLogDlg::EnableOKButton()
{
    if (m_bSelect)
    {
        // the dialog is used to select revisions
        if (m_bSelectionMustBeContinuous)
            DialogEnableWindow(IDOK, (m_LogList.GetSelectedCount()!=0)&&(IsSelectionContinuous()));
        else
            DialogEnableWindow(IDOK, m_LogList.GetSelectedCount()!=0);
    }
    else
        DialogEnableWindow(IDOK, TRUE);
}

namespace
{
    bool IsAllWhitespace (const wstring& text, long first, long last)
    {
        for (; first < last; ++first)
        {
            wchar_t c = text[first];
            if (c > L' ')
                return false;

            if ((c != L' ') && (c != L'\t') && (c != L'\r') && (c != L'\n'))
                return false;
        }

        return true;
    }

    void ReduceRanges(std::vector<CHARRANGE>& ranges, const wstring& text)
    {
        if (ranges.size() < 2)
            return;

        auto begin = ranges.begin();
        auto end = ranges.end();

        auto target = begin;
        for (auto source = begin + 1; source != end; ++source)
            if (IsAllWhitespace (text, target->cpMax, source->cpMin))
                target->cpMax = source->cpMax;
            else
                *(++target) = *source;

        ranges.erase (++target, end);
    }
}

namespace
{
    struct SMarkerInfo
    {
        CString sText;
        wstring text;

        std::vector<CHARRANGE> ranges;
        std::vector<CHARRANGE> idRanges;
        std::vector<CHARRANGE> revRanges;

        BOOL RunRegex (ProjectProperties *project)
        {
            // turn bug ID's into links if the bugtraq: properties have been set
            // and we can find a match of those in the log message
            idRanges = project->FindBugIDPositions (sText);

            // underline all revisions mentioned in the message
            revRanges = CAppUtils::FindRegexMatches ( text
                                                    , project->sLogRevRegex
                                                    , _T("\\d+"));

            return TRUE;
        }
    };

}

void CLogDlg::FillLogMessageCtrl(bool bShow /* = true*/)
{
    // we fill here the log message rich edit control,
    // and also populate the changed files list control
    // according to the selected revision(s).

    CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
    if (pMsgView == NULL)
        return; // can happen if the dialog is already closed, but the threads are still running
    // empty the log message view
    pMsgView->SetWindowText(_T(" "));
    // empty the changed files list
    m_ChangedFileListCtrl.SetRedraw(FALSE);
    m_currentChangedArray.RemoveAll();
    m_ChangedFileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
    m_ChangedFileListCtrl.SetItemCountEx(0);

    // if we're not here to really show a selected revision, just
    // get out of here after clearing the views, which is what is intended
    // if that flag is not set.
    if (!bShow)
    {
        // force a redraw
        m_ChangedFileListCtrl.Invalidate();
        m_ChangedFileListCtrl.SetRedraw(TRUE);
        return;
    }

    // depending on how many revisions are selected, we have to do different
    // tasks.
    int selCount = m_LogList.GetSelectedCount();
    if (selCount == 0)
    {
        // if nothing is selected, we have nothing more to do
        m_ChangedFileListCtrl.SetRedraw(TRUE);
        return;
    }
    else
    {
        m_currentChangedPathList.Clear();
        m_bSingleRevision = selCount == 1;

        // if one revision is selected, we have to fill the log message view
        // with the corresponding log message, and also fill the changed files
        // list fully.
        POSITION pos = m_LogList.GetFirstSelectedItemPosition();
        size_t selIndex = m_LogList.GetNextSelectedItem(pos);
        if (selIndex >= m_logEntries.GetVisibleCount())
        {
            m_ChangedFileListCtrl.SetRedraw(TRUE);
            return;
        }
        m_nSearchIndex = (int)selIndex;
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (selIndex);

        pMsgView->SetRedraw(FALSE);

        // the rich edit control doesn't count the CR char!
        // to be exact: CRLF is treated as one char.

        SMarkerInfo info;
        if (m_bSingleRevision)
        {
            info.sText = CUnicodeUtils::GetUnicode
                            (pLogEntry->GetMessage().c_str());
            info.sText.Remove(_T('\r'));
        }
        else
        {
            m_currentChangedArray.RemoveAll();
            m_currentChangedPathList = GetChangedPathsAndMessageSketchFromSelectedRevisions(info.sText, m_currentChangedArray);
        }
        info.text = info.sText;

        async::CFuture<BOOL> regexRunner ( &info
                                         , &SMarkerInfo::RunRegex
                                         , &m_ProjectProperties);

        // set the log message text
        pMsgView->SetWindowText(info.sText);

        // mark filter matches
        if (m_SelectedFilters & LOGFILTER_MESSAGES)
        {
            CLogDlgFilter filter ( m_sFilterText
                , m_bFilterWithRegex
                , m_SelectedFilters
                , m_bFilterCaseSensitively
                , m_tFrom
                , m_tTo
                , false
                , &m_mergedRevs
                , !!m_bHideNonMergeables
                , m_copyfromrev
                , 0);

            info.ranges = filter.GetMatchRanges (info.text);

            // combine ranges only separated by whitespace
            ReduceRanges (info.ranges, info.text);

            CAppUtils::SetCharFormat ( pMsgView
                                     , CFM_COLOR
                                     , m_Colors.GetColor(CColors::FilterMatch)
                                     , info.ranges);
        }

        if (((DWORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\StyleCommitMessages"), TRUE))==TRUE)
            CAppUtils::FormatTextInRichEditControl(pMsgView);

        // fill in the changed files list control
        if (m_bSingleRevision)
        {
            m_currentChangedArray = pLogEntry->GetChangedPaths();
            if ((m_cShowPaths.GetState() & 0x0003)==BST_CHECKED)
                m_currentChangedArray.RemoveIrrelevantPaths();
        }

        regexRunner.GetResult();
        CAppUtils::SetCharFormat (pMsgView, CFM_LINK, CFE_LINK, info.idRanges);
        CAppUtils::SetCharFormat (pMsgView, CFM_LINK, CFE_LINK, info.revRanges);

        pMsgView->SetRedraw(TRUE);
        pMsgView->Invalidate();
    }

    // sort according to the settings
    if (m_nSortColumnPathList > 0)
    {
        m_currentChangedArray.Sort (m_nSortColumnPathList, m_bAscendingPathList);
        SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
    }
    else
        SetSortArrow(&m_ChangedFileListCtrl, -1, false);

    // redraw the views
    if (m_bSingleRevision)
    {
        m_ChangedFileListCtrl.SetItemCountEx((int)m_currentChangedArray.GetCount());
        m_ChangedFileListCtrl.RedrawItems(0, (int)m_currentChangedArray.GetCount());
    }
    else if (m_currentChangedPathList.GetCount())
    {
        m_ChangedFileListCtrl.SetItemCountEx((int)m_currentChangedPathList.GetCount());
        m_ChangedFileListCtrl.RedrawItems(0, (int)m_currentChangedPathList.GetCount());
    }
    else
    {
        m_ChangedFileListCtrl.SetItemCountEx(0);
        m_ChangedFileListCtrl.Invalidate();
    }

    CAppUtils::ResizeAllListCtrlCols(&m_ChangedFileListCtrl);
    m_ChangedFileListCtrl.SetRedraw(TRUE);
}

void CLogDlg::OnBnClickedGetall()
{
    GetAll();
}

void CLogDlg::GetAll(bool bForceAll /* = false */)
{
    // fetch all requested log messages, either the specified range or
    // really *all* available log messages.
    UpdateData();

    INT_PTR entry = m_btnShow.m_nMenuResult;
    if (bForceAll)
        entry = ID_CMD_SHOWALL;

    CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowAllEntry"));

    if (entry == ID_CMD_DEFAULT)
        entry = (LONG)reg;

    reg = (DWORD)entry;

    switch (entry)
    {
    default:
    case ID_CMD_SHOWALL:
        m_endrev = 0;
        m_startrev = m_LogRevision;
        if (m_bStrict)
            m_bShowedAll = true;
        m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWALL)));
        break;
    case ID_CMD_SHOWRANGE:
        {
            // ask for a revision range
            CRevisionRangeDlg dlg;
            dlg.SetStartRevision(m_startrev);
            dlg.SetEndRevision( (m_endrev>=0) ? m_endrev : 0);
            if (dlg.DoModal()!=IDOK)
                return;
            m_endrev = dlg.GetEndRevision();
            m_startrev = dlg.GetStartRevision();
            if (((m_endrev.IsNumber())&&(m_startrev.IsNumber()))||
                (m_endrev.IsHead()||m_startrev.IsHead()))
            {
                if (((LONG)m_startrev < (LONG)m_endrev)||
                    (m_endrev.IsHead()))
                {
                    svn_revnum_t temp = m_startrev;
                    m_startrev = m_endrev;
                    m_endrev = temp;
                }
            }
            m_bShowedAll = false;
            m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWRANGE)));
        }
        break;
    }
    m_ChangedFileListCtrl.SetItemCountEx(0);
    m_ChangedFileListCtrl.Invalidate();
    // We need to create CStoreSelection on the heap or else
    // the variable will run out of the scope before the
    // thread ends. Therefore we let the thread delete
    // the instance.
    AutoStoreSelection();

    m_LogList.SetItemCountEx(0);
    m_LogList.Invalidate();
    CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
    if ((m_startrev > m_head)&&(m_head > 0))
    {
        CString sTemp;
        sTemp.FormatMessage(IDS_ERR_NOSUCHREVISION, m_startrev.ToString());
        m_LogList.ShowText(sTemp, true);
        return;
    }
    pMsgView->SetWindowText(_T(""));

    SetSortArrow(&m_LogList, -1, true);

    m_logEntries.ClearAll();

    m_bCancelled = FALSE;
    m_limit = 0;

    InterlockedExchange(&m_bLogThreadRunning, TRUE);
    new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::OnBnClickedRefresh()
{
    m_limit = 0;
    Refresh (true);
}

void CLogDlg::Refresh (bool autoGoOnline)
{
    //does the user force the cache to refresh (shift or control key down)?
    m_nRefresh = ((GetKeyState (VK_CONTROL) < 0) || (GetKeyState (VK_SHIFT) < 0)) ? Cache : Simple;

    // refreshing means re-downloading the already shown log messages
    UpdateData();
    if (m_logEntries.size())
        m_startrev = m_logEntries.GetMaxRevision();
    if ((m_startrev < m_head)&&(m_nRefresh==Cache))
    {
        m_startrev = -1;
        m_nRefresh = Simple;
    }
    if (m_startrev >= m_head)
    {
        m_startrev = -1;
    }
    if ((m_limit == 0)||(m_bStrict)||(int(m_logEntries.size()-1) > m_limit))
    {
        if (m_logEntries.size() != 0)
        {
            m_endrev = m_logEntries.GetMinRevision();
        }
    }
    m_bCancelled = FALSE;
    m_wcRev = SVNRev();

    // We need to create CStoreSelection on the heap or else
    // the variable will run out of the scope before the
    // thread ends. Therefore we let the thread delete
    // the instance.
    AutoStoreSelection();

    m_ChangedFileListCtrl.SetItemCountEx(0);
    m_ChangedFileListCtrl.Invalidate();
    m_LogList.SetItemCountEx(0);
    m_LogList.Invalidate();
    CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
    pMsgView->SetWindowText(_T(""));

    SetSortArrow(&m_LogList, -1, true);
    m_logEntries.ClearAll();

    // reset the cached HEAD property & go on-line

    if (autoGoOnline)
    {
        SetDlgTitle (false);
        GetLogCachePool()->GetRepositoryInfo().ResetHeadRevision (m_sUUID, m_sRepositoryRoot);
    }

    InterlockedExchange(&m_bLogThreadRunning, TRUE);
    new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::OnBnClickedNexthundred()
{
    UpdateData();
    // we have to fetch the next X log messages.
    if (m_logEntries.size() < 1)
    {
        // since there weren't any log messages fetched before, just
        // fetch all since we don't have an 'anchor' to fetch the 'next'
        // messages from.
        return GetAll(true);
    }
    svn_revnum_t rev = m_logEntries[m_logEntries.size()-1]->GetRevision();

    if (rev < 1)
        return;     // do nothing! No more revisions to get
    m_startrev = rev;
    m_endrev = 0;
    m_bCancelled = FALSE;
    m_nRefresh = None;

    // rev is is revision we already have and we will receive it again
    // -> fetch one extra revision to get NumberOfLogs *new* revisions

    m_limit = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100) +1;
    InterlockedExchange(&m_bLogThreadRunning, TRUE);
    SetSortArrow(&m_LogList, -1, true);

    // clear the list controls: after the thread is finished
    // the controls will be populated again.
    m_ChangedFileListCtrl.SetItemCountEx(0);
    m_ChangedFileListCtrl.Invalidate();
    // We need to create CStoreSelection on the heap or else
    // the variable will run out of the scope before the
    // thread ends. Therefore we let the thread delete
    // the instance.
    AutoStoreSelection();

    m_LogList.SetItemCountEx(0);
    m_LogList.Invalidate();

    // since we fetch the log from the last revision we already have,
    // we have to remove that revision entry to avoid getting it twice
    m_logEntries.RemoveLast();
    new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

BOOL CLogDlg::Cancel()
{
    return m_bCancelled;
}

void CLogDlg::SaveSplitterPos()
{
    if (!IsIconic())
    {
        CRegDWORD regPos1 = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1"));
        CRegDWORD regPos2 = CRegDWORD(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2"));
        RECT rectSplitter;
        m_wndSplitter1.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        regPos1 = rectSplitter.top;
        m_wndSplitter2.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        regPos2 = rectSplitter.top;
    }
}

void CLogDlg::OnLogCancel()
{
    // canceling means stopping the working thread if it's still running.
    // we do this by using the Subversion cancel callback.

    m_bCancelled = true;

    // Canceling can mean just to stop fetching data, depending on the text
    // shown on the cancel button (it will read "Cancel" in that case).

    CString temp, temp2;
    GetDlgItemText(IDC_LOGCANCEL, temp);
    temp2.LoadString(IDS_MSGBOX_CANCEL);
    if ((temp.Compare(temp2)==0) && !GetDlgItem(IDOK)->IsWindowVisible())
        return;

    // we actually want to close the dialog.

    OnCancel();
}

void CLogDlg::OnCancel()
{
    // canceling means stopping the working thread if it's still running.
    // we do this by using the Subversion cancel callback.

    m_bCancelled = true;

    // We want to close the dialog -> give the background threads some time
    // to actually finish. Otherwise, we might not save the latest data.

    bool threadsStillRunning
        =    !netScheduler.WaitForEmptyQueueOrTimeout(5000)
          || !diskScheduler.WaitForEmptyQueueOrTimeout(5000);

    // can we close the app cleanly?

    if (threadsStillRunning)
    {
        // end the process the hard way
        TerminateProcess(GetCurrentProcess(), 0);
    }
    else
    {
        // Store window positions etc. only if we actually hit an cancel
        // button dressed as an "OK" button.

        CString temp, temp2;
        GetDlgItemText(IDC_LOGCANCEL, temp);
        temp2.LoadString(IDS_MSGBOX_CANCEL);
        if ((temp.Compare(temp2)!=0))
        {
            // store settings

            UpdateData();
            if (m_bSaveStrict)
                m_regLastStrict = m_bStrict;
            CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowAllEntry"));
            reg = (DWORD)m_btnShow.m_nMenuResult;

            CRegDWORD reg2 = CRegDWORD(_T("Software\\TortoiseSVN\\LogShowPaths"));
            reg2 = (DWORD)m_cShowPaths.GetCheck();
            SaveSplitterPos();
        }

        // close app

        __super::OnCancel();
    }
}

void CLogDlg::OnClose()
{
    if (m_bLogThreadRunning)
    {
        m_bCancelled = true;

        bool threadsStillRunning
            =    !netScheduler.WaitForEmptyQueueOrTimeout(5000)
            || !diskScheduler.WaitForEmptyQueueOrTimeout(5000);

        if (threadsStillRunning)
            return;
    }
    __super::OnClose();
}

void CLogDlg::OnDestroy()
{
    if (m_pLogListAccServer)
    {
        ListViewAccServer::ClearProvider(m_LogList.GetSafeHwnd());
        m_pLogListAccServer->Release();
        delete m_pLogListAccServer;
        m_pLogListAccServer = nullptr;
    }
    if (m_pChangedListAccServer)
    {
        ListViewAccServer::ClearProvider(m_ChangedFileListCtrl.GetSafeHwnd());
        m_pChangedListAccServer->Release();
        delete m_pChangedListAccServer;
        m_pChangedListAccServer = nullptr;
    }
    __super::OnDestroy();
}

BOOL CLogDlg::Log(svn_revnum_t rev, const std::string& author, const std::string& message, apr_time_t time, const MergeInfo* mergeInfo)
{
    // this is the callback function which receives the data for every revision we ask the log for
    // we store this information here one by one.

    __time64_t ttime = time / 1000000L;
    m_logEntries.Add ( rev
                     , ttime
                     , author
                     , message
                     , &m_ProjectProperties
                     , mergeInfo);

    // end of child list

    if (rev == SVN_INVALID_REVNUM)
        return TRUE;

    // update progress

    if (m_startrev == -1)
        m_startrev = rev;

    if (m_limit != 0)
    {
        m_LogProgress.SetPos ((int)(m_logEntries.size() - m_prevLogEntriesSize));
        if (m_pTaskbarList)
        {
            int l,u;
            m_LogProgress.GetRange(l, u);
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
            m_pTaskbarList->SetProgressValue(m_hWnd, m_logEntries.size() - m_prevLogEntriesSize - l, u-l);
        }
    }
    else if (m_startrev.IsNumber() && m_endrev.IsNumber())
    {
        svn_revnum_t range = (svn_revnum_t)m_startrev - (svn_revnum_t)m_endrev;
        if ((rev > m_temprev) || (m_temprev - rev) > (range / 100))
        {
            m_temprev = rev;
            m_LogProgress.SetPos((svn_revnum_t)m_startrev-rev+(svn_revnum_t)m_endrev);
            if (m_pTaskbarList)
            {
                int l,u;
                m_LogProgress.GetRange(l, u);
                m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
                m_pTaskbarList->SetProgressValue(m_hWnd, (svn_revnum_t)m_startrev-rev+(svn_revnum_t)m_endrev-l, u-l);
            }
        }
    }

    // clean-up

    return TRUE;
}

//this is the thread function which calls the subversion function
void CLogDlg::LogThread()
{
    InterlockedExchange(&m_bLogThreadRunning, TRUE);

    new async::CAsyncCall(this, &CLogDlg::StatusThread, &diskScheduler);

    //disable the "Get All" button while we're receiving
    //log messages.
    DialogEnableWindow(IDC_GETALL, FALSE);
    DialogEnableWindow(IDC_NEXTHUNDRED, FALSE);
    DialogEnableWindow(IDC_HIDENONMERGEABLE, FALSE);
    DialogEnableWindow(IDC_SHOWPATHS, FALSE);
    DialogEnableWindow(IDC_CHECK_STOPONCOPY, FALSE);
    DialogEnableWindow(IDC_INCLUDEMERGE, FALSE);
    DialogEnableWindow(IDC_STATBUTTON, FALSE);
    DialogEnableWindow(IDC_REFRESH, FALSE);

    CString temp;
    temp.LoadString(IDS_PROGRESSWAIT);
    m_LogList.ShowText(temp, true);
    // change the text of the close button to "Cancel" since now the thread
    // is running, and simply closing the dialog doesn't work.
    if (!GetDlgItem(IDOK)->IsWindowVisible())
    {
        temp.LoadString(IDS_MSGBOX_CANCEL);
        SetDlgItemText(IDC_LOGCANCEL, temp);
    }
    // We use a progress bar while getting the logs
    m_LogProgress.SetRange32(0, 100);
    m_LogProgress.SetPos(0);
    GetDlgItem(IDC_HIDENONMERGEABLE)->ShowWindow(FALSE);
    GetDlgItem(IDC_PROGRESS)->ShowWindow(TRUE);

    // we need the UUID to unambiguously identify the log cache
    BOOL succeeded = true;
    if (LogCache::CSettings::GetEnabled())
    {
        m_sUUID = GetLogCachePool()->GetRepositoryInfo().GetRepositoryUUID (m_path);
        if (m_sUUID.IsEmpty())
            succeeded = false;
    }

    // get the repository root url, because the changed-files-list has the
    // paths shown there relative to the repository root.
    CTSVNPath rootpath;
    if (succeeded)
    {
        // e.g. when we show the "next 100", we already have proper
        // start and end revs.
        // -> we don't need to look for the head revision in these cases

        if (m_bStartRevIsHead || (m_startrev == SVNRev::REV_HEAD) || (m_endrev == SVNRev::REV_HEAD) || (m_head < 0))
        {
            // expensive repository lookup
            int maxheadage = LogCache::CSettings::GetMaxHeadAge();
            LogCache::CSettings::SetMaxHeadAge(0);
            svn_revnum_t head = -1;
            succeeded = GetRootAndHead(m_path, rootpath, head);
            m_head = head;
            if ((m_startrev == SVNRev::REV_HEAD) ||
                (m_bStartRevIsHead && ((m_nRefresh==Simple) || (m_nRefresh==Cache)) ))
            {
                m_startrev = head;
            }
            if (m_endrev == SVNRev::REV_HEAD)
                m_endrev = head;
            LogCache::CSettings::SetMaxHeadAge(maxheadage);
        }
        else
        {
            // far less expensive root lookup

            rootpath.SetFromSVN (GetRepositoryRoot (m_path));
            succeeded = !rootpath.IsEmpty();
        }
    }

    m_sRepositoryRoot = rootpath.GetSVNPathString();
    m_sURL = m_path.GetSVNPathString();

    // if the log dialog is started from a working copy, we need to turn that
    // local path into an url here
    if (succeeded)
    {
        if (!m_path.IsUrl())
        {
            m_sURL = GetURLFromPath(m_path);
        }
        m_sURL = CPathUtils::PathUnescape(m_sURL);
        m_sRelativeRoot = m_sURL.Mid(CPathUtils::PathUnescape(m_sRepositoryRoot).GetLength());
    }

    if (succeeded && !m_mergePath.IsEmpty())
    {
        // in case we got a merge path set, retrieve the merge info
        // of that path and check whether one of the merge URLs
        // match the URL we show the log for.
        SVNPool localpool(pool);
        svn_error_clear(Err);
        apr_hash_t * mergeinfo = NULL;

        const char* svnPath = m_mergePath.GetSVNApiPath(localpool);
        SVNTRACE (
            Err = svn_client_mergeinfo_get_merged (&mergeinfo, svnPath, SVNRev(SVNRev::REV_WC), m_pctx, localpool),
            svnPath
        )
        if (Err == NULL)
        {
            // now check the relative paths
            apr_hash_index_t *hi;
            const void *key;
            void *val;

            if (mergeinfo)
            {
                // unfortunately it's not defined whether the urls have a trailing
                // slash or not, so we have to trim such a slash before comparing
                // the sUrl with the mergeinfo url
                CStringA sUrl = CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(m_sURL));
                sUrl.TrimRight('/');

                for (hi = apr_hash_first(localpool, mergeinfo); hi; hi = apr_hash_next(hi))
                {
                    apr_hash_this(hi, &key, NULL, &val);
                    CStringA sKey = (char*)key;
                    sKey.TrimRight('/');
                    if (sUrl.Compare(sKey) == 0)
                    {
                        apr_array_header_t * arr = (apr_array_header_t*)val;
                        if (val)
                        {
                            for (long i=0; i<arr->nelts; ++i)
                            {
                                svn_merge_range_t * pRange = APR_ARRAY_IDX(arr, i, svn_merge_range_t*);
                                if (pRange)
                                {
                                    for (svn_revnum_t re = pRange->start+1; re <= pRange->end; ++re)
                                    {
                                        m_mergedRevs.insert(re);
                                    }
                                }
                            }
                        }
                        break;
                    }
                }

                bool bFindCopyFrom = !!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFindCopyFrom"), FALSE);
                if (bFindCopyFrom)
                {
                    SVNLogHelper helper;
                    CString sCopyFrom;
                    CTSVNPath mergeUrl = CTSVNPath(GetURLFromPath(m_mergePath)+L"/");
                    SVNRev rev = helper.GetCopyFromRev(mergeUrl, SVNRev::REV_HEAD, sCopyFrom);
                    if (sCopyFrom.Compare(m_sURL) == 0)
                        m_copyfromrev = rev;
                }
            }
        }
    }

    m_LogProgress.SetPos(1);
    m_prevLogEntriesSize = m_logEntries.size();
    if (m_pTaskbarList)
    {
        m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
    }
    if (m_limit != 0)
        m_LogProgress.SetRange32(0, m_limit);
    else
        m_LogProgress.SetRange32(m_endrev, m_startrev);

    if (!m_pegrev.IsValid())
        m_pegrev = m_startrev;
    size_t startcount = m_logEntries.size();
    m_bStrictStopped = false;

    std::unique_ptr<const CCacheLogQuery> cachedData;
    if (succeeded)
    {
        cachedData = ReceiveLog (CTSVNPathList(m_path), m_pegrev, m_startrev, m_endrev, m_limit, !!m_bStrict, !!m_bIncludeMerges, m_nRefresh==Cache);
        if ((cachedData.get() == NULL)&&(!m_path.IsUrl()))
        {
            // try again with REV_WC as the start revision, just in case the path doesn't
            // exist anymore in HEAD.
            // Also, make sure we use these parameters for furter requests (like "next 100").

            m_pegrev = SVNRev::REV_WC;
            m_startrev = SVNRev::REV_WC;

            cachedData = ReceiveLog(CTSVNPathList(m_path), m_pegrev, m_startrev, m_endrev, m_limit, !!m_bStrict, !!m_bIncludeMerges, m_nRefresh==Cache);
        }

        // Err will also be set if the user cancelled.

        if (Err && (Err->apr_err == SVN_ERR_CANCELLED))
        {
            svn_error_clear(Err);
            Err = NULL;
        }

        succeeded = Err == NULL;

        // make sure the m_logEntries is consistent

        if (cachedData.get() != NULL)
            m_logEntries.Finalize (std::move(cachedData), m_sRelativeRoot, !LogCache::CSettings::GetEnabled());
        else
            m_logEntries.ClearAll();
    }
    m_LogList.ClearText();
    if (!succeeded)
    {
        temp.LoadString(IDS_LOG_CLEARERROR);
        m_LogList.ShowText(GetLastErrorMessage() + _T("\n\n") + temp, true);
        FillLogMessageCtrl(false);
        if (m_pTaskbarList)
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
        }
    }

    if (   m_bStrict
        && (m_logEntries.GetMinRevision() > 1)
        && (m_limit > 0
               ? (startcount + m_limit > m_logEntries.size())
               : (m_endrev < m_logEntries.GetMinRevision())))
    {
        m_bStrictStopped = true;
    }
    m_LogList.SetItemCountEx(ShownCountWithStopped());

    m_tFrom = m_logEntries.GetMinDate();
    m_tTo = m_logEntries.GetMaxDate();
    m_timFrom = m_tFrom;
    m_timTo = m_tTo;
    m_DateFrom.SetRange(&m_timFrom, &m_timTo);
    m_DateTo.SetRange(&m_timFrom, &m_timTo);
    m_DateFrom.SetTime(&m_timFrom);
    m_DateTo.SetTime(&m_timTo);

    DialogEnableWindow(IDC_GETALL, TRUE);

    if (!m_bShowedAll)
        DialogEnableWindow(IDC_NEXTHUNDRED, TRUE);
    DialogEnableWindow(IDC_HIDENONMERGEABLE, TRUE);
    DialogEnableWindow(IDC_SHOWPATHS, TRUE);
    DialogEnableWindow(IDC_CHECK_STOPONCOPY, TRUE);
    DialogEnableWindow(IDC_INCLUDEMERGE, TRUE);
    DialogEnableWindow(IDC_STATBUTTON, TRUE);
    DialogEnableWindow(IDC_REFRESH, TRUE);

    LogCache::CRepositoryInfo& cachedProperties = GetLogCachePool()->GetRepositoryInfo();
    SetDlgTitle(cachedProperties.IsOffline (m_sUUID, m_sRepositoryRoot, false));

    GetDlgItem(IDC_PROGRESS)->ShowWindow(FALSE);
    GetDlgItem(IDC_HIDENONMERGEABLE)->ShowWindow(!m_mergedRevs.empty());
    if (m_pTaskbarList)
    {
        m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
    }
    m_bCancelled = true;
    InterlockedExchange(&m_bLogThreadRunning, FALSE);
    if ( m_pStoreSelection == NULL )
    {
        // If no selection has been set then this must be the first time
        // the revisions are shown. Let's preselect the topmost revision.
        if ( m_LogList.GetItemCount()>0 )
        {
            m_LogList.SetSelectionMark(0);
            m_LogList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
    if (!GetDlgItem(IDOK)->IsWindowVisible())
    {
        temp.LoadString(IDS_MSGBOX_OK);
        SetDlgItemText(IDC_LOGCANCEL, temp);
    }
    RefreshCursor();
    // make sure the filter is applied (if any) now, after we refreshed/fetched
    // the log messages
    PostMessage(WM_TIMER, LOGFILTER_TIMER);
}

//this is the thread function which calls the subversion function
void CLogDlg::StatusThread()
{
    bool bAllowStatusCheck = !!(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogStatusCheck"), TRUE);
    if ((bAllowStatusCheck)&&(!m_wcRev.IsValid()))
    {
        // fetch the revision the wc path is on so we can mark it
        CTSVNPath revWCPath = m_ProjectProperties.GetPropsPath();
        if (!m_path.IsUrl())
            revWCPath = m_path;

        if (revWCPath.IsUrl() || revWCPath.IsEmpty())
            return;

        svn_revnum_t minrev, maxrev;
        SVN svn;
        svn.SetCancelBool(&m_bCancelled);
        if (svn.GetWCMinMaxRevs(revWCPath, true, minrev, maxrev)&&(maxrev))
        {
            m_wcRev = maxrev;
            // force a redraw of the log list control to make sure the wc rev is
            // redrawn in bold
            m_LogList.Invalidate(FALSE);
        }
    }
}

void CLogDlg::CopySelectionToClipBoard()
{
    CopySelectionToClipBoard(!(GetKeyState(VK_SHIFT) & 0x8000));
}

void CLogDlg::CopySelectionToClipBoard(bool bIncludeChangedList)
{
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    if (pos != NULL)
    {
        CString sClipdata;
        CString sRev;
        sRev.LoadString(IDS_LOG_REVISION);
        CString sAuthor;
        sAuthor.LoadString(IDS_LOG_AUTHOR);
        CString sDate;
        sDate.LoadString(IDS_LOG_DATE);
        CString sMessage;
        sMessage.LoadString(IDS_LOG_MESSAGE);
        while (pos)
        {
            CString sLogCopyText;
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index >= (int)m_logEntries.GetVisibleCount())
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (index);
            if (bIncludeChangedList)
            {
                CString sPaths;
                const CLogChangedPathArray& cpatharray = pLogEntry->GetChangedPaths();
                for (size_t cpPathIndex = 0; cpPathIndex < cpatharray.GetCount(); ++cpPathIndex)
                {
                    const CLogChangedPath& cpath = cpatharray[cpPathIndex];
                    sPaths += CUnicodeUtils::GetUnicode (cpath.GetActionString().c_str())
                        + _T(" : ") + cpath.GetPath();
                    if (cpath.GetCopyFromPath().IsEmpty())
                        sPaths += _T("\r\n");
                    else
                    {
                        CString sCopyFrom;
                        sCopyFrom.Format(_T(" (%s: %s, %s, %ld)\r\n"), (LPCTSTR)CString(MAKEINTRESOURCE(IDS_LOG_COPYFROM)),
                            (LPCTSTR)cpath.GetCopyFromPath(),
                            (LPCTSTR)CString(MAKEINTRESOURCE(IDS_LOG_REVISION)),
                            cpath.GetCopyFromRev());
                        sPaths += sCopyFrom;
                    }
                }
                sPaths.Trim();
                CString nlMessage = CUnicodeUtils::GetUnicode (pLogEntry->GetMessage().c_str());
                nlMessage.Remove('\r');
                nlMessage.Replace(L"\n", L"\r\n");
                sLogCopyText.Format(_T("%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n"),
                    (LPCTSTR)sRev, pLogEntry->GetRevision(),
                    (LPCTSTR)sAuthor,  (LPCTSTR)CUnicodeUtils::GetUnicode (pLogEntry->GetAuthor().c_str()),
                    (LPCTSTR)sDate, (LPCTSTR)CUnicodeUtils::GetUnicode (pLogEntry->GetDateString().c_str()),
                    (LPCTSTR)sMessage, (LPCTSTR)nlMessage,
                    (LPCTSTR)sPaths);
            }
            else
            {
                CString nlMessage = CUnicodeUtils::GetUnicode (pLogEntry->GetMessage().c_str());
                nlMessage.Remove('\r');
                nlMessage.Replace(L"\n", L"\r\n");
                sLogCopyText.Format(_T("%s\r\n----\r\n"),
                    (LPCTSTR)nlMessage);
            }
            sClipdata +=  sLogCopyText;
        }
        CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
    }
}

void CLogDlg::CopyChangedSelectionToClipBoard()
{
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    if (pos == NULL)
        return; // nothing is selected, get out of here

    CString sPaths;

    POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
    while (pos2)
    {
        int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
        sPaths += m_currentChangedArray[nItem].GetPath();
        sPaths += _T("\r\n");
    }
    sPaths.Trim();
    CStringUtils::WriteAsciiStringToClipboard(sPaths, GetSafeHwnd());
}

BOOL CLogDlg::IsDiffPossible(const CLogChangedPath& changedpath, svn_revnum_t rev)
{
    if ((rev > 1)&&(changedpath.GetAction() != LOGACTIONS_DELETED))
    {
        if (changedpath.GetAction() == LOGACTIONS_ADDED) // file is added
        {
            if (changedpath.GetCopyFromRev() == 0)
                return FALSE; // but file was not added with history
        }
        return TRUE;
    }
    return FALSE;
}

void CLogDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    // we have two separate context menus:
    // one shown on the log message list control,
    // the other shown in the changed-files list control
    int selCount = m_LogList.GetSelectedCount();
    if (pWnd == &m_LogList)
    {
        ShowContextMenuForRevisions(pWnd, point);
    }
    else if (pWnd == &m_ChangedFileListCtrl)
    {
        ShowContextMenuForChangedpaths(pWnd, point);
    }
    else if ((selCount > 0)&&(pWnd == GetDlgItem(IDC_MSGVIEW)))
    {
        POSITION pos = m_LogList.GetFirstSelectedItemPosition();
        int selIndex = -1;
        if (pos)
            selIndex = m_LogList.GetNextSelectedItem(pos);
        if ((point.x == -1) && (point.y == -1))
        {
            CRect rect;
            GetDlgItem(IDC_MSGVIEW)->GetClientRect(&rect);
            ClientToScreen(&rect);
            point = rect.CenterPoint();
        }
        CString sMenuItemText;
        CMenu popup;
        if (popup.CreatePopupMenu())
        {
            // add the 'default' entries
            sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
            popup.AppendMenu(MF_STRING | MF_ENABLED, WM_COPY, sMenuItemText);
            sMenuItemText.LoadString(IDS_SCIEDIT_SELECTALL);
            popup.AppendMenu(MF_STRING | MF_ENABLED, EM_SETSEL, sMenuItemText);

            if ((selIndex >= 0)&&(selCount == 1))
            {
                popup.AppendMenu(MF_SEPARATOR);
                sMenuItemText.LoadString(IDS_LOG_POPUP_EDITLOG);
                popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITLOG, sMenuItemText);
            }

            int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, 0);
            switch (cmd)
            {
            case 0:
                break;  // no command selected
            case EM_SETSEL:
            case WM_COPY:
                ::SendMessage(GetDlgItem(IDC_MSGVIEW)->GetSafeHwnd(), cmd, 0, -1);
                break;
            case ID_EDITLOG:
                if (selIndex >= 0)
                    EditLogMessage(selIndex);
                break;
            }
        }
    }
}

bool CLogDlg::IsSelectionContinuous()
{
    if ( m_LogList.GetSelectedCount()==1 )
    {
        // if only one revision is selected, the selection is of course
        // continuous
        return true;
    }

    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    bool bContinuous = m_logEntries.GetVisibleCount() == m_logEntries.size();
    if (bContinuous)
    {
        int itemindex = m_LogList.GetNextSelectedItem(pos);
        while (pos)
        {
            int nextindex = m_LogList.GetNextSelectedItem(pos);
            if (nextindex - itemindex > 1)
            {
                bContinuous = false;
                break;
            }
            itemindex = nextindex;
        }
    }
    return bContinuous;
}

LRESULT CLogDlg::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_pFindDialog != NULL);
    if (m_pFindDialog == nullptr)
        return 0;

    if (m_pFindDialog->IsTerminating())
    {
        // invalidate the handle identifying the dialog box.
        m_pFindDialog = NULL;
        return 0;
    }

    if(m_pFindDialog->FindNext())
    {
        //read data from dialog
        CString findText = m_pFindDialog->GetFindString();
        bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
        tr1::wregex pat;
        bool bRegex = ValidateRegexp(findText, pat, bMatchCase);

        bool scanRelevantPathsOnly = (m_cShowPaths.GetState() & 0x0003)==BST_CHECKED;
        CLogDlgFilter filter ( findText
                             , bRegex
                             , LOGFILTER_ALL
                             , bMatchCase
                             , m_tFrom
                             , m_tTo
                             , scanRelevantPathsOnly
                             , &m_mergedRevs
                             , !!m_bHideNonMergeables
                             , m_copyfromrev
                             , -1);

        for (size_t i = m_nSearchIndex; i < m_logEntries.GetVisibleCount(); i++)
        {
            if (filter (*m_logEntries.GetVisible (i)))
            {
                m_LogList.EnsureVisible((int)i, FALSE);
                m_LogList.SetItemState(m_LogList.GetSelectionMark(), 0, LVIS_SELECTED);
                m_LogList.SetItemState((int)i, LVIS_SELECTED, LVIS_SELECTED);
                m_LogList.SetSelectionMark((int)i);

                FillLogMessageCtrl();
                UpdateData(FALSE);

                m_nSearchIndex = (int)(i+1);
                break;
            }
        }
    } // if(m_pFindDialog->FindNext())
    UpdateLogInfoLabel();
    return 0;
}

void CLogDlg::UpdateSelectedRevs()
{
    m_selectedRevs.Clear();
    m_selectedRevsOneRange.Clear();

    std::vector<svn_revnum_t> revisions;
    revisions.reserve (m_logEntries.GetVisibleCount());

    PLOGENTRYDATA pLogEntry = NULL;
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    if (pos)
    {
        while (pos)
        {
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index < (int)m_logEntries.GetVisibleCount())
            {
                pLogEntry = m_logEntries.GetVisible (index);
                if (pLogEntry)
                    revisions.push_back (pLogEntry->GetRevision());
            }
        }

        m_selectedRevs.AddRevisions (revisions);
        svn_revnum_t lowerRev = m_selectedRevs.GetLowestRevision();
        svn_revnum_t higherRev = m_selectedRevs.GetHighestRevision();

        if (m_sFilterText.IsEmpty() && m_nSortColumn == 0 && IsSelectionContinuous())
        {
            m_selectedRevsOneRange.AddRevRange(lowerRev, higherRev);
        }
    }
}

void CLogDlg::OnOK()
{
    // since the log dialog is also used to select revisions for other
    // dialogs, we have to do some work before closing this dialog
    if ((GetKeyState(VK_MENU)&0x8000) == 0) // if the ALT key is pressed, we get here because of an accelerator
    {
        if ((GetDlgItem(IDOK)->IsWindowVisible()) && (GetFocus() != GetDlgItem(IDOK)))
            return; // if the "OK" button doesn't have the focus, do nothing: this prevents closing the dialog when pressing enter
        if (!GetDlgItem(IDOK)->IsWindowVisible() && GetFocus() != GetDlgItem(IDC_LOGCANCEL))
            return; // the Cancel button works as the OK button. But if the cancel button has not the focus, do nothing.
    }

    m_bCancelled = true;
    if (   !netScheduler.WaitForEmptyQueueOrTimeout(0)
        || !diskScheduler.WaitForEmptyQueueOrTimeout(1000))
    {
        return;
    }

    if (m_pNotifyWindow || m_bSelect)
        UpdateSelectedRevs();

    NotifyTargetOnOk();

    UpdateData();
    if (m_bSaveStrict)
        m_regLastStrict = m_bStrict;
    CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowAllEntry"));
    reg = (DWORD)m_btnShow.m_nMenuResult;
    CRegDWORD reg2 = CRegDWORD(_T("Software\\TortoiseSVN\\LogHidePaths"));
    reg2 = (DWORD)m_cShowPaths.GetCheck();
    SaveSplitterPos();

    CString buttontext;
    GetDlgItemText(IDOK, buttontext);
    CString temp;
    temp.LoadString(IDS_MSGBOX_CANCEL);
    if (temp.Compare(buttontext) != 0)
        __super::OnOK();    // only exit if the button text matches, and that will match only if the thread isn't running anymore
}

void CLogDlg::NotifyTargetOnOk()
{
    if (m_pNotifyWindow == 0)
        return;

    bool bSentMessage = false;
    svn_revnum_t lowerRev = m_selectedRevsOneRange.GetHighestRevision();
    svn_revnum_t higherRev = m_selectedRevsOneRange.GetLowestRevision();
    if (m_selectedRevsOneRange.GetCount() == 0)
    {
        lowerRev = m_selectedRevs.GetLowestRevision();
        higherRev = m_selectedRevs.GetHighestRevision();
    }
    if (m_LogList.GetSelectedCount() == 1)
    {
        // if only one revision is selected, check if the path/url with which the dialog was started
        // was directly affected in that revision. If it was, then check if our path was copied from somewhere.
        // if it was copied, use the copy from revision as lowerRev
        POSITION pos = m_LogList.GetFirstSelectedItemPosition();
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));

        if ((pLogEntry)&&(lowerRev == higherRev))
        {
            CString sUrl = m_path.GetSVNPathString();
            if (!m_path.IsUrl())
            {
                sUrl = GetURLFromPath(m_path);
            }
            sUrl = sUrl.Mid(m_sRepositoryRoot.GetLength());

            const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();
            for (size_t cp = 0; cp < paths.GetCount(); ++cp)
            {
                const CLogChangedPath& pData = paths[cp];
                if (sUrl.Compare(pData.GetPath()) != 0)
                    continue;
                if (pData.GetCopyFromPath().IsEmpty())
                    continue;

                lowerRev = pData.GetCopyFromRev();
                m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART), lowerRev);
                m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND), higherRev);
                m_pNotifyWindow->SendMessage(WM_REVLIST, m_selectedRevs.GetCount(), (LPARAM)&m_selectedRevs);
                bSentMessage = true;
                break;
            }
        }
    }
    if ( !bSentMessage )
    {
        m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTSTART | MERGE_REVSELECTMINUSONE), lowerRev);
        m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam & (MERGE_REVSELECTEND | MERGE_REVSELECTMINUSONE), higherRev);
        m_pNotifyWindow->SendMessage(WM_REVLIST, m_selectedRevs.GetCount(), (LPARAM)&m_selectedRevs);
        if (m_selectedRevsOneRange.GetCount())
            m_pNotifyWindow->SendMessage(WM_REVLISTONERANGE, 0, (LPARAM)&m_selectedRevsOneRange);
    }
}

void CLogDlg::CreateFindDialog()
{
    if (m_pFindDialog == 0)
    {
        m_pFindDialog = new CFindReplaceDialog();
        m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);
    }
}

void CLogDlg::DoOpenFileWith(bool bOpenWith, const CTSVNPath& tempfile)
{
    SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    int ret = 0;
    if (!bOpenWith)
        ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
    if ((ret <= HINSTANCE_ERROR)||bOpenWith)
    {
        CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
        cmd += tempfile.GetWinPathString() + _T(" ");
        CAppUtils::LaunchApplication(cmd, NULL, false);
    }
}

void CLogDlg::OnNMDblclkChangedFileList(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    // a double click on an entry in the changed-files list has happened
    *pResult = 0;

    DiffSelectedFile();
}

void CLogDlg::DiffSelectedFile()
{
    if ((m_bLogThreadRunning)||(m_LogList.HasText()))
        return;
    UpdateLogInfoLabel();
    INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();
    if (selIndex < 0)
        return;
    if (m_ChangedFileListCtrl.GetSelectedCount() == 0)
        return;
    // find out if there's an entry selected in the log list
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos));
    svn_revnum_t rev1 = pLogEntry->GetRevision();
    svn_revnum_t rev2 = rev1;
    if (pos)
    {
        while (pos)
        {
            // there's at least a second entry selected in the log list: several revisions selected!
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index < (int)m_logEntries.GetVisibleCount())
            {
                pLogEntry = m_logEntries.GetVisible(index);
                if (pLogEntry)
                {
                    const long entryRevision = (long)pLogEntry->GetRevision();
                    rev1 = max(rev1, entryRevision);
                    rev2 = min(rev2, entryRevision);
                }
            }
        }
        rev2--;
        // now we have both revisions selected in the log list, so we can do a diff of the selected
        // entry in the changed files list with these two revisions.
        auto f = [=](){CoInitialize(NULL); this->EnableWindow(FALSE); DoDiffFromLog(selIndex, rev1, rev2, false, false); this->EnableWindow(TRUE);this->SetFocus();};
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();
        rev2 = rev1-1;
        // nothing or only one revision selected in the log list

        const CLogChangedPath& changedpath = m_currentChangedArray[selIndex];

        if (IsDiffPossible(changedpath, rev1))
        {
            // diffs with renamed files are possible
            if (!changedpath.GetCopyFromPath().IsEmpty())
                rev2 = changedpath.GetCopyFromRev();
            else
            {
                // if the path was modified but the parent path was 'added with history'
                // then we have to use the copy from revision of the parent path
                CTSVNPath cpath = CTSVNPath(changedpath.GetPath());
                for (size_t flist = 0; flist < paths.GetCount(); ++flist)
                {
                    const CLogChangedPath& path = paths[flist];
                    CTSVNPath p = CTSVNPath(path.GetPath());
                    if (p.IsAncestorOf(cpath))
                    {
                        if (!path.GetCopyFromPath().IsEmpty())
                            rev2 = path.GetCopyFromRev();
                    }
                }
            }
            auto f = [=](){CoInitialize(NULL); this->EnableWindow(FALSE); DoDiffFromLog(selIndex, rev1, rev2, false, false); this->EnableWindow(TRUE);this->SetFocus();};
            new async::CAsyncCall(f, &netScheduler);
        }
        else
        {
            CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(changedpath.GetPath()));
            CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(changedpath.GetPath()));
            SVNRev r = rev1;
            // deleted files must be opened from the revision before the deletion
            if (changedpath.GetAction() == LOGACTIONS_DELETED)
                r = rev1-1;
            m_bCancelled = false;

            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            progDlg.SetAnimation(IDR_DOWNLOAD);
            CString sInfoLine;
            sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)(m_sRepositoryRoot + changedpath.GetPath()), (LPCTSTR)r.ToString());
            progDlg.SetLine(1, sInfoLine, true);
            SetAndClearProgressInfo(&progDlg);
            progDlg.ShowModeless(m_hWnd);

            CTSVNPath url = CTSVNPath(m_sRepositoryRoot + changedpath.GetPath());
            if (!Export(url, tempfile, r, r))
            {
                m_bCancelled = false;
                if (!Export(url, tempfile, SVNRev::REV_HEAD, r))
                {
                    progDlg.Stop();
                    SetAndClearProgressInfo((HWND)NULL);
                    ShowErrorDialog(m_hWnd);
                    return;
                }
            }
            progDlg.Stop();
            SetAndClearProgressInfo((HWND)NULL);

            CString sName1, sName2;
            sName1.Format(_T("%s - Revision %ld"), (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath.GetPath()), (svn_revnum_t)rev1);
            sName2.Format(_T("%s - Revision %ld"), (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath.GetPath()), (svn_revnum_t)rev1-1);
            CAppUtils::DiffFlags flags;
            flags.AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
            if (changedpath.GetAction() == LOGACTIONS_DELETED)
                CAppUtils::StartExtDiff(tempfile, tempfile2, sName2, sName1, url, url, r, SVNRev(), r, flags, 0);
            else
                CAppUtils::StartExtDiff(tempfile2, tempfile, sName2, sName1, url, url, r, SVNRev(), r, flags, 0);
        }
    }
}


void CLogDlg::OnNMDblclkLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    // a double click on an entry in the revision list has happened
    *pResult = 0;

    if (m_LogList.HasText())
    {
        m_LogList.ClearText();
        FillLogMessageCtrl();
        if (m_pTaskbarList)
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
        }
        return;
    }
    if (CRegDWORD(_T("Software\\TortoiseSVN\\DiffByDoubleClickInLog"), FALSE))
        DiffSelectedRevWithPrevious();
}

void CLogDlg::DiffSelectedRevWithPrevious()
{
    if ((m_bLogThreadRunning)||(m_LogList.HasText()))
        return;
    UpdateLogInfoLabel();
    int selIndex = m_LogList.GetSelectionMark();
    if (selIndex < 0)
        return;
    int selCount = m_LogList.GetSelectedCount();
    if (selCount != 1)
        return;

    // Find selected entry in the log list
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
    long rev1 = pLogEntry->GetRevision();
    long rev2 = rev1-1;
    CTSVNPath path = m_path;

    // See how many files under the relative root were changed in selected revision
    int nChanged = 0;
    size_t lastChangedIndex = (size_t)(-1);

    const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();
    for (size_t c = 0; c < paths.GetCount(); ++c)
        if (paths[c].IsRelevantForStartPath())
        {
            ++nChanged;
            lastChangedIndex = c;
        }

    svn_node_kind_t nodekind = svn_node_unknown;
    if (!m_path.IsUrl())
        nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;

    if (nChanged == 1)
    {
        // We're looking at the log for a directory and only one file under dir was changed in the revision
        // Do diff on that file instead of whole directory

        const CLogChangedPath& cpath = pLogEntry->GetChangedPaths()[lastChangedIndex];
        path.SetFromWin (m_sRepositoryRoot + cpath.GetPath());
        nodekind = cpath.GetNodeKind() < 0 ? svn_node_unknown : cpath.GetNodeKind();
    }

    m_bCancelled = FALSE;
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    theApp.DoWaitCursor(1);

    if (PromptShown())
    {
        SVNDiff diff(this, m_hWnd, true);
        diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
        diff.SetHEADPeg(m_LogRevision);
        diff.ShowCompare(path, rev2, path, rev1, SVNRev(), L"", false, false, nodekind);
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, path, rev2, path, rev1, SVNRev(), m_LogRevision, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), false, false, nodekind);
    }

    theApp.DoWaitCursor(-1);
    EnableOKButton();
}

void CLogDlg::DoDiffFromLog(INT_PTR selIndex, svn_revnum_t rev1, svn_revnum_t rev2, bool blame, bool unified)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    theApp.DoWaitCursor(1);
    //get the filename
    CString filepath;
    if (SVN::PathIsURL(m_path))
    {
        filepath = m_path.GetSVNPathString();
    }
    else
    {
        filepath = GetURLFromPath(m_path);
        if (filepath.IsEmpty())
        {
            theApp.DoWaitCursor(-1);
            ReportNoUrlOfFile(filepath);
            EnableOKButton();
            return;     //exit
        }
    }
    m_bCancelled = FALSE;
    filepath = GetRepositoryRoot(CTSVNPath(filepath));

    svn_node_kind_t nodekind = svn_node_unknown;
    CString firstfile, secondfile;

    if (m_currentChangedArray.GetCount() <= (size_t)selIndex)
        return;
    const CLogChangedPath& changedpath = m_currentChangedArray[selIndex];
    nodekind = changedpath.GetNodeKind();
    firstfile = changedpath.GetPath();
    secondfile = firstfile;
    if ((rev2 == rev1-1)&&(changedpath.GetCopyFromRev() > 0)) // is it an added file with history?
    {
        secondfile = changedpath.GetCopyFromPath();
        rev2 = changedpath.GetCopyFromRev();
    }

    firstfile = filepath + firstfile.Trim();
    secondfile = filepath + secondfile.Trim();

    SVNDiff diff(this, this->m_hWnd, true);
    diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
    // since we have the direct urls here and revisions, we set the head peg
    // revision to rev1.
    // see this thread for why: http://tortoisesvn.tigris.org/ds/viewMessage.do?dsForumId=4061&dsMessageId=2096879
    diff.SetHEADPeg(rev1);
    if (unified)
    {
        CString options;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        {
            CDiffOptionsDlg dlg(this);
            if (dlg.DoModal() == IDOK)
                options = dlg.GetDiffOptionsString();
            else
            {
                theApp.DoWaitCursor(-1);
                EnableOKButton();
                return;
            }
        }
        if (PromptShown())
            diff.ShowUnifiedDiff(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(), options);
        else
            CAppUtils::StartShowUnifiedDiff(m_hWnd, CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(), m_LogRevision, options);
    }
    else
    {
        diff.ShowCompare(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(), L"", false, blame, nodekind);
    }
    theApp.DoWaitCursor(-1);
    EnableOKButton();
}

BOOL CLogDlg::Open(bool bOpenWith,CString changedpath, svn_revnum_t rev)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    theApp.DoWaitCursor(1);
    CString filepath;
    if (SVN::PathIsURL(m_path))
    {
        filepath = m_path.GetSVNPathString();
    }
    else
    {
        filepath = GetURLFromPath(m_path);
        if (filepath.IsEmpty())
        {
            theApp.DoWaitCursor(-1);
            ReportNoUrlOfFile(filepath);
            EnableOKButton();
            return FALSE;
        }
    }
    m_bCancelled = false;
    filepath = GetRepositoryRoot(CTSVNPath(filepath));
    filepath += changedpath;

    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_APPNAME);
    progDlg.SetAnimation(IDR_DOWNLOAD);
    CString sInfoLine;
    sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)filepath, (LPCTSTR)SVNRev(rev).ToString());
    progDlg.SetLine(1, sInfoLine, true);
    SetAndClearProgressInfo(&progDlg);
    progDlg.ShowModeless(m_hWnd);

    CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(filepath), rev);
    m_bCancelled = false;
    if (!Export(CTSVNPath(filepath), tempfile, SVNRev(rev), rev))
    {
        progDlg.Stop();
        SetAndClearProgressInfo((HWND)NULL);
        ShowErrorDialog(m_hWnd);
        EnableOKButton();
        theApp.DoWaitCursor(-1);
        return FALSE;
    }
    progDlg.Stop();
    SetAndClearProgressInfo((HWND)NULL);
    DoOpenFileWith(bOpenWith, tempfile);
    EnableOKButton();
    theApp.DoWaitCursor(-1);
    return TRUE;
}

void CLogDlg::EditAuthor(const std::vector<PLOGENTRYDATA>& logs)
{
    if (logs.empty())
        return;

    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    theApp.DoWaitCursor(1);

    CString url;
    if (SVN::PathIsURL(m_path))
        url = m_path.GetSVNPathString();
    else
        url = GetURLFromPath(m_path);

    CString name = CString(SVN_PROP_REVISION_AUTHOR);
    CString value = RevPropertyGet(name, CTSVNPath(url), logs[0]->GetRevision());
    CString sOldValue = value;
    value.Replace(_T("\n"), _T("\r\n"));

    CInputDlg dlg(this);
    dlg.m_sHintText.LoadString(IDS_LOG_AUTHOR);
    dlg.m_sInputText = value;
    dlg.m_sTitle.LoadString(IDS_LOG_AUTHOREDITTITLE);
    if (dlg.DoModal() == IDOK)
    {
        if(sOldValue.Compare(dlg.m_sInputText))
        {
            dlg.m_sInputText.Remove(_T('\r'));

            LogCache::CCachedLogInfo* toUpdate
                = GetLogCache (CTSVNPath (m_sRepositoryRoot));

            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
            progDlg.SetTime(true);
            progDlg.SetShowProgressBar(true);
            progDlg.ShowModeless(m_hWnd);
            for (DWORD i=0; (i<logs.size()) && (!progDlg.HasUserCancelled()); ++i)
            {
                if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url), logs[i]->GetRevision()))
                {
                    progDlg.Stop();
                    ShowErrorDialog(m_hWnd);
                    break;
                }
                logs[i]->SetAuthor (CUnicodeUtils::StdGetUTF8 ((LPCTSTR)dlg.m_sInputText));
                m_LogList.Invalidate();

                // update the log cache
                if (toUpdate == NULL)
                    continue;
                // log caching is active
                LogCache::CCachedLogInfo newInfo;
                newInfo.Insert ( logs[i]->GetRevision()
                    , logs[i]->GetAuthor().c_str()
                    , ""
                    , 0
                    , LogCache::CRevisionInfoContainer::HAS_AUTHOR);
                toUpdate->Update (newInfo);
                try
                {
                    toUpdate->Save();
                }
                catch (CStreamException&)
                {
                    // can't save the file right now
                }

                progDlg.SetProgress64(i, logs.size());

                MSG msg;
                if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
            progDlg.Stop();
        }
    }
    theApp.DoWaitCursor(-1);
    EnableOKButton();
}

void CLogDlg::EditLogMessage( size_t index )
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    theApp.DoWaitCursor(1);

    CString url;
    if (SVN::PathIsURL(m_path))
        url = m_path.GetSVNPathString();
    else
        url = GetURLFromPath(m_path);

    CString name = CString(SVN_PROP_REVISION_LOG);

    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
    m_bCancelled = FALSE;
    CString value = RevPropertyGet(name, CTSVNPath(url), pLogEntry->GetRevision());
    CString sOldValue = value;
    value.Replace(_T("\n"), _T("\r\n"));
    CInputDlg dlg(this);
    dlg.m_sHintText.LoadString(IDS_LOG_MESSAGE);
    dlg.m_sInputText = value;
    dlg.m_sTitle.LoadString(IDS_LOG_MESSAGEEDITTITLE);
    if (dlg.DoModal() == IDOK)
    {
        if(sOldValue.Compare(dlg.m_sInputText))
        {
            dlg.m_sInputText.Remove(_T('\r'));
            if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url), pLogEntry->GetRevision()))
            {
                ShowErrorDialog(m_hWnd);
            }
            else
            {
                pLogEntry->SetMessage (CUnicodeUtils::StdGetUTF8
                    ( (LPCTSTR)dlg.m_sInputText));

                CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
                pMsgView->SetWindowText(_T(" "));
                pMsgView->SetWindowText(dlg.m_sInputText);
                m_ProjectProperties.FindBugID(dlg.m_sInputText, pMsgView);
                m_LogList.Invalidate();

                // update the log cache

                LogCache::CCachedLogInfo* toUpdate
                    = GetLogCache (CTSVNPath (m_sRepositoryRoot));
                if (toUpdate != NULL)
                {
                    // log caching is active

                    LogCache::CCachedLogInfo newInfo;
                    newInfo.Insert ( pLogEntry->GetRevision()
                        , ""
                        , pLogEntry->GetMessage().c_str()
                        , 0
                        , LogCache::CRevisionInfoContainer::HAS_COMMENT);

                    toUpdate->Update (newInfo);
                    try
                    {
                        toUpdate->Save();
                    }
                    catch (CStreamException&)
                    {
                        // can't save the file right now
                    }
                }
            }
        }
    }
    theApp.DoWaitCursor(-1);
    EnableOKButton();
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
    // Skip Ctrl-C when copying text out of the log message or search filter
    BOOL bSkipAccelerator = ( pMsg->message == WM_KEYDOWN && (pMsg->wParam=='C' || pMsg->wParam== VK_INSERT) && (GetFocus()==GetDlgItem(IDC_MSGVIEW) || GetFocus()==GetDlgItem(IDC_SEARCHEDIT) ) && GetKeyState(VK_CONTROL)&0x8000 );
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam==VK_RETURN)
    {
        if (GetAsyncKeyState(VK_CONTROL)&0x8000)
        {
            if (DWORD(CRegStdDWORD(_T("Software\\TortoiseSVN\\CtrlEnter"), TRUE)))
            {
                if ( GetDlgItem(IDOK)->IsWindowVisible() )
                {
                    GetDlgItem(IDOK)->SetFocus();
                    PostMessage(WM_COMMAND, IDOK);
                }
                else
                {
                    GetDlgItem(IDC_LOGCANCEL)->SetFocus();
                    PostMessage(WM_COMMAND, IDOK);
                }
            }
            return TRUE;
        }
        if (GetFocus()==GetDlgItem(IDC_LOGLIST))
        {
            if (CRegDWORD(_T("Software\\TortoiseSVN\\DiffByDoubleClickInLog"), FALSE))
            {
                DiffSelectedRevWithPrevious();
                return TRUE;
            }
        }
        if (GetFocus()==GetDlgItem(IDC_LOGMSG))
        {
            DiffSelectedFile();
            return TRUE;
        }
    }
    if (m_hAccel && !bSkipAccelerator)
    {
        int ret = TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
        if (ret)
            return TRUE;
    }

    m_tooltips.RelayEvent(pMsg);
    return __super::PreTranslateMessage(pMsg);
}

BOOL CLogDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if ((m_bLogThreadRunning)||(netScheduler.GetRunningThreadCount()))
    {
        if (!IsCursorOverWindowBorder() && ((pWnd)&&(pWnd != GetDlgItem(IDC_LOGCANCEL))))
        {
            HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
            SetCursor(hCur);
            return TRUE;
        }
    }
    if ((pWnd) && (pWnd == GetDlgItem(IDC_MSGVIEW)))
        return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);

    HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    SetCursor(hCur);
    return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CLogDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CLogDlg::ToggleCheckbox(size_t item)
{
    if (!SysInfo::Instance().IsVistaOrLater())
    {
        if (item < m_logEntries.GetVisibleCount())
        {
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(item);
            if (pLogEntry)
            {
                pLogEntry->SetChecked (!pLogEntry->GetChecked());
                m_LogList.RedrawItems ((int)item, (int)item);
            }
        }
    }
}

void CLogDlg::SelectAllVisibleRevisions()
{
    if (!SysInfo::Instance().IsVistaOrLater())
    {
        for ( size_t i = 0, count = m_logEntries.GetVisibleCount()
            ; i < count
            ; ++i)
        {
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(i);
            if (pLogEntry)
                pLogEntry->SetChecked (true);
        }
    }

    m_LogList.SetItemState (-1, LVIS_SELECTED, LVIS_SELECTED);
    if (m_bStrict && m_bStrictStopped)
        m_LogList.SetItemState(m_LogList.GetItemCount()-1, 0, LVIS_SELECTED);
    m_LogList.RedrawWindow();
}

void CLogDlg::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult = 0;
    if ((m_bLogThreadRunning)||(m_LogList.HasText()))
        return;
    if (pNMLV->iItem >= 0)
    {
        if (pNMLV->iSubItem != 0)
            return;

        size_t item = pNMLV->iItem;
        if ((item == m_logEntries.GetVisibleCount())&&(m_bStrict)&&(m_bStrictStopped))
        {
            // remove the selected state
            if (pNMLV->uChanged & LVIF_STATE)
            {
                m_LogList.SetItemState(pNMLV->iItem, 0, LVIS_SELECTED);
                FillLogMessageCtrl();
                UpdateData(FALSE);
                UpdateLogInfoLabel();
            }
            return;
        }
        if (pNMLV->uChanged & LVIF_STATE)
        {
            FillLogMessageCtrl();
            UpdateData(FALSE);
            if (SysInfo::Instance().IsVistaOrLater())
            {
                if (item < m_logEntries.GetVisibleCount())
                {
                    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(item);
                    if (pLogEntry)
                    {
                        pLogEntry->SetChecked ((pNMLV->uNewState & LVIS_SELECTED) != 0);
                    }
                }
            }
        }
    }
    else
    {
        FillLogMessageCtrl();
        UpdateData(FALSE);
    }
    EnableOKButton();
    UpdateLogInfoLabel();
}

void CLogDlg::OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    ENLINK *pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
    if ((pEnLink->msg != WM_LBUTTONUP)&&(pEnLink->msg != WM_SETCURSOR))
        return;

    CString url, msg;
    GetDlgItemText(IDC_MSGVIEW, msg);
    msg.Replace(_T("\r\n"), _T("\n"));
    url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax-pEnLink->chrg.cpMin);
    if (!::PathIsURL(url))
    {
        // not a full url: either a bug ID or a revision.
        std::set<CString> bugIDs = m_ProjectProperties.FindBugIDs(msg);
        bool bBugIDFound = false;
        for (std::set<CString>::iterator it = bugIDs.begin(); it != bugIDs.end(); ++it)
        {
            if (it->Compare(url) == 0)
            {
                url = m_ProjectProperties.GetBugIDUrl(url);
                url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
                bBugIDFound = true;
                break;
            }
        }
        if ((!bBugIDFound)&&(pEnLink->msg != WM_SETCURSOR))
        {
            // now check whether it matches a revision
            const tr1::wregex regMatch(m_ProjectProperties.sLogRevRegex, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
            const tr1::wsregex_iterator end;
            wstring s = msg;
            for (tr1::wsregex_iterator it(s.begin(), s.end(), regMatch); it != end; ++it)
            {
                wstring matchedString = (*it)[0];
                const tr1::wregex regRevMatch(_T("\\d+"));
                wstring ss = matchedString;
                for (tr1::wsregex_iterator it2(ss.begin(), ss.end(), regRevMatch); it2 != end; ++it2)
                {
                    wstring matchedRevString = (*it2)[0];
                    if (url.Compare(matchedRevString.c_str()) == 0)
                    {
                        svn_revnum_t rev = _ttol(matchedRevString.c_str());
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": found revision %ld\n", rev);
                        // do we already show this revision? If yes, just select that revision and 'scroll' to it
                        for (size_t i=0; i<m_logEntries.GetVisibleCount(); ++i)
                        {
                            PLOGENTRYDATA data = m_logEntries.GetVisible(i);
                            if (!data)
                                continue;
                            if (data->GetRevision() != rev)
                                continue;
                            int selMark = m_LogList.GetSelectionMark();
                            if (selMark>=0)
                            {
                                m_LogList.SetItemState(selMark, 0, LVIS_SELECTED);
                            }
                            m_LogList.EnsureVisible((int)i, FALSE);
                            m_LogList.SetSelectionMark((int)i);
                            m_LogList.SetItemState((int)i, LVIS_SELECTED, LVIS_SELECTED);
                            return;
                        }
                        try
                        {
                            CLogCacheUtility logUtil(GetLogCachePool()->GetCache(m_sUUID, m_sRepositoryRoot), &m_ProjectProperties);
                            if (logUtil.IsCached(rev))
                            {
                                PLOGENTRYDATA pLogItem = logUtil.GetRevisionData(rev);
                                if (pLogItem)
                                {
                                    // insert the data
                                    m_logEntries.Sort(CLogDataVector::RevisionCol, false);
                                    m_logEntries.AddSorted (pLogItem, &m_ProjectProperties);

                                    int selMark = m_LogList.GetSelectionMark();
                                    // now start filter the log list
                                    SortAndFilter (rev);
                                    m_LogList.SetItemCountEx(ShownCountWithStopped());
                                    m_LogList.RedrawItems(0, ShownCountWithStopped());
                                    if (selMark >= 0)
                                        m_LogList.SetSelectionMark(selMark);
                                    m_LogList.Invalidate();

                                    for (size_t i=0; i<m_logEntries.GetVisibleCount(); ++i)
                                    {
                                        PLOGENTRYDATA data = m_logEntries.GetVisible(i);
                                        if (!data)
                                            continue;
                                        if (data->GetRevision() != rev)
                                            continue;
                                        if (selMark>=0)
                                        {
                                            m_LogList.SetItemState(selMark, 0, LVIS_SELECTED);
                                        }
                                        m_LogList.EnsureVisible((int)i, FALSE);
                                        m_LogList.SetSelectionMark((int)i);
                                        m_LogList.SetItemState((int)i, LVIS_SELECTED, LVIS_SELECTED);
                                        return;
                                    }
                                }
                            }
                        }
                        catch (CException* e)
                        {
                            e->Delete();
                        }

                        // if we get here, then the linked revision is not shown in this dialog:
                        // start a new log dialog for the repository root and this revision
                        CString sCmd;
                        sCmd.Format(_T("/command:log /path:\"%s\" /startrev:%ld /propspath:\"%s\""),
                            (LPCTSTR)m_sRepositoryRoot, rev, (LPCTSTR)m_path.GetWinPath());
                        CAppUtils::RunTortoiseProc(sCmd);
                        return;
                    }
                }
            }
        }
    }

    if ((!url.IsEmpty())&&(pEnLink->msg == WM_SETCURSOR))
    {
        static RECT prevRect = {0};
        CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
        if (pMsgView)
        {
            RECT rc;
            LRESULT lRes = pMsgView->SendMessage(EM_POSFROMCHAR, pEnLink->chrg.cpMin, 0);
            rc.left = LOWORD(lRes);
            rc.top = HIWORD(lRes);
            lRes = pMsgView->SendMessage(EM_POSFROMCHAR, pEnLink->chrg.cpMax, 0);
            rc.right = LOWORD(lRes);
            rc.bottom = HIWORD(lRes)+12;
            if ((prevRect.left != rc.left)||(prevRect.top != rc.top))
            {
                m_tooltips.DelTool(pMsgView, 1);
                m_tooltips.AddTool(pMsgView, url, &rc, 1);
                prevRect = rc;
            }
        }
        return;
    }
    if (!url.IsEmpty())
        ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
}

void CLogDlg::OnBnClickedStatbutton()
{
    if ((m_bLogThreadRunning)||(m_LogList.HasText()))
        return;
    if (m_logEntries.GetVisibleCount() == 0)
        return;     // nothing is shown, so no statistics.

    // the statistics dialog expects the log entries to be sorted by date
    // and we must remove duplicate entries created by merge info etc.

    typedef std::map<__time64_t, PLOGENTRYDATA> TMap;
    TMap revsByDate;

    std::set<svn_revnum_t> revisionsCovered;
    for (size_t i=0; i<m_logEntries.GetVisibleCount(); ++i)
    {
        PLOGENTRYDATA entry = m_logEntries.GetVisible(i);
        if (revisionsCovered.insert (entry->GetRevision()).second)
            revsByDate.insert (std::make_pair (entry->GetDate(), entry));
    }

    // create arrays which are aware of the current filter
    CStringArray m_arAuthorsFiltered;
    CDWordArray m_arDatesFiltered;
    CDWordArray m_arFileChangesFiltered;
    for ( TMap::const_reverse_iterator iter = revsByDate.rbegin()
        , end = revsByDate.rend()
        ; iter != end
        ; ++iter)
    {
        PLOGENTRYDATA pLogEntry = iter->second;
        CString strAuthor = CUnicodeUtils::GetUnicode (pLogEntry->GetAuthor().c_str());
        if ( strAuthor.IsEmpty() )
        {
            strAuthor.LoadString(IDS_STATGRAPH_EMPTYAUTHOR);
        }
        m_arAuthorsFiltered.Add(strAuthor);
        m_arDatesFiltered.Add(static_cast<DWORD>(pLogEntry->GetDate()));
        m_arFileChangesFiltered.Add (static_cast<DWORD>(pLogEntry->GetChangedPaths().GetCount()));
    }
    CStatGraphDlg dlg;
    dlg.m_parAuthors = &m_arAuthorsFiltered;
    dlg.m_parDates = &m_arDatesFiltered;
    dlg.m_parFileChanges = &m_arFileChangesFiltered;
    dlg.m_path = m_path;
    dlg.DoModal();
    OnTimer(LOGFILTER_TIMER);
}

void CLogDlg::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_bLogThreadRunning)
        return;

    static COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

    switch (pLVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            *pResult = CDRF_NOTIFYITEMDRAW;
            return;
        }
        break;
    case CDDS_ITEMPREPAINT:
        {
            // This is the prepaint stage for an item. Here's where we set the
            // item's text color.

            // Tell Windows to send draw notifications for each subitem.
            *pResult = CDRF_NOTIFYSUBITEMDRAW;
            crText = GetSysColor(COLOR_WINDOWTEXT);
            if (m_logEntries.GetVisibleCount() > pLVCD->nmcd.dwItemSpec)
            {
                PLOGENTRYDATA data = m_logEntries.GetVisible (pLVCD->nmcd.dwItemSpec);
                if (data)
                {
                    if (data->GetChangedPaths().ContainsSelfCopy())
                    {
                        // only change the background color if the item is not 'hot' (on vista with themes enabled)
                        if (!IsAppThemed() || !SysInfo::Instance().IsVistaOrLater() || ((pLVCD->nmcd.uItemState & CDIS_HOT)==0))
                            pLVCD->clrTextBk = GetSysColor(COLOR_MENU);
                    }
                    if (data->GetChangedPaths().ContainsCopies())
                        crText = m_Colors.GetColor(CColors::Modified);
                    if ((data->GetDepth())||(m_mergedRevs.find(data->GetRevision()) != m_mergedRevs.end()))
                        crText = GetSysColor(COLOR_GRAYTEXT);
                    if (data->GetRevision() == m_wcRev)
                    {
                        SelectObject(pLVCD->nmcd.hdc, m_boldFont);
                        // We changed the font, so we're returning CDRF_NEWFONT. This
                        // tells the control to recalculate the extent of the text.
                        *pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
                    }
                }
            }
            if (m_logEntries.GetVisibleCount() == pLVCD->nmcd.dwItemSpec)
            {
                if (m_bStrictStopped)
                    crText = GetSysColor(COLOR_GRAYTEXT);
            }
            // Store the color back in the NMLVCUSTOMDRAW struct.
            pLVCD->clrText = crText;
            return;
        }
        break;
    case CDDS_ITEMPREPAINT|CDDS_ITEM|CDDS_SUBITEM:
        {
            *pResult = CDRF_DODEFAULT;
            pLVCD->clrText = crText;
            PLOGENTRYDATA pLogEntry = NULL;
            if (m_logEntries.GetVisibleCount() > pLVCD->nmcd.dwItemSpec)
                pLogEntry = m_logEntries.GetVisible (pLVCD->nmcd.dwItemSpec);
            if ((m_bStrictStopped)&&(m_logEntries.GetVisibleCount() == pLVCD->nmcd.dwItemSpec))
            {
                pLVCD->nmcd.uItemState &= ~(CDIS_SELECTED|CDIS_FOCUS);
            }
            switch (pLVCD->iSubItem)
            {
            case 0: // revision
                if (pLogEntry == NULL)
                    return;
                if ((m_SelectedFilters & LOGFILTER_REVS)&&(m_filter.IsFilterActive()))
                {
                    *pResult = DrawListItemWithMatches(m_LogList, pLVCD, pLogEntry);
                    return;
                }
                break;
            case 1: // actions
                {
                    if (pLogEntry == NULL)
                        return;

                    int     nIcons = 0;
                    int     iconwidth = ::GetSystemMetrics(SM_CXSMICON);
                    int     iconheight = ::GetSystemMetrics(SM_CYSMICON);

                    CRect rect = DrawListColumnBackground(m_LogList, pLVCD, pLogEntry);

                    // Draw the icon(s) into the compatible DC

                    DWORD actions = pLogEntry->GetChangedPaths().GetActions();
                    if (actions & LOGACTIONS_MODIFIED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_ADDED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hAddedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_DELETED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_REPLACED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hReplacedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if ((pLogEntry->GetDepth())||(m_mergedRevs.find(pLogEntry->GetRevision()) != m_mergedRevs.end()))
                    {
                        if (pLogEntry->IsSubtractiveMerge())
                            ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hReverseMergedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                        else
                            ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hMergedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    }
                    nIcons++;

                    *pResult = CDRF_SKIPDEFAULT;
                    return;
                }
                break;
            case 2: // author
                if (pLogEntry == NULL)
                    return;
                if ((m_SelectedFilters & LOGFILTER_AUTHORS)&&(m_filter.IsFilterActive()))
                {
                    *pResult = DrawListItemWithMatches(m_LogList, pLVCD, pLogEntry);
                    return;
                }
                break;
            case 3: // date
                if (pLogEntry == NULL)
                    return;
                if ((m_SelectedFilters & LOGFILTER_DATE)&&(m_filter.IsFilterActive()))
                {
                    *pResult = DrawListItemWithMatches(m_LogList, pLVCD, pLogEntry);
                    return;
                }
                break;
            case 4: //message or bug id
                if (pLogEntry == NULL)
                    return;
                if (m_bShowBugtraqColumn)
                {
                    if ((m_SelectedFilters & LOGFILTER_BUGID)&&(m_filter.IsFilterActive()))
                    {
                        *pResult = DrawListItemWithMatches(m_LogList, pLVCD, pLogEntry);
                        return;
                    }
                    break;
                }
                // fall through here!
            case 5: // log msg
                if (pLogEntry == NULL)
                    return;
                if ((m_SelectedFilters & LOGFILTER_MESSAGES)&&(m_filter.IsFilterActive()))
                {
                    *pResult = DrawListItemWithMatches(m_LogList, pLVCD, pLogEntry);
                    return;
                }
                break;
            }
        }
        break;
    }
    *pResult = CDRF_DODEFAULT;
}

void CLogDlg::OnNMCustomdrawChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_bLogThreadRunning)
        return;

    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
    // First thing - check the draw stage. If it's the control's prepaint
    // stage, then tell Windows we want messages for every item.

    if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        // Tell Windows to send draw notifications for each subitem.
        *pResult = CDRF_NOTIFYSUBITEMDRAW;

        COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
        bool bGrayed = false;
        if ((m_cShowPaths.GetState() & 0x0003)==BST_UNCHECKED)
        {
            if (m_currentChangedArray.GetCount() > pLVCD->nmcd.dwItemSpec)
            {
                if (!m_currentChangedArray[pLVCD->nmcd.dwItemSpec].IsRelevantForStartPath())
                {
                    crText = GetSysColor(COLOR_GRAYTEXT);
                    bGrayed = true;
                }
            }
            else if ((DWORD_PTR)m_currentChangedPathList.GetCount() > pLVCD->nmcd.dwItemSpec)
            {
                if (m_currentChangedPathList[pLVCD->nmcd.dwItemSpec].GetSVNPathString().Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
                {
                    crText = GetSysColor(COLOR_GRAYTEXT);
                    bGrayed = true;
                }
            }
        }

        if ((!bGrayed)&&(m_currentChangedArray.GetCount() > pLVCD->nmcd.dwItemSpec))
        {
            DWORD action = m_currentChangedArray[pLVCD->nmcd.dwItemSpec].GetAction();
            if (action == LOGACTIONS_MODIFIED)
                crText = m_Colors.GetColor(CColors::Modified);
            if (action == LOGACTIONS_REPLACED)
                crText = m_Colors.GetColor(CColors::Deleted);
            if (action == LOGACTIONS_ADDED)
                crText = m_Colors.GetColor(CColors::Added);
            if (action == LOGACTIONS_DELETED)
                crText = m_Colors.GetColor(CColors::Deleted);
        }
        if (m_currentChangedArray.GetCount() > pLVCD->nmcd.dwItemSpec)
        {
            svn_tristate_t textModifies = m_currentChangedArray[pLVCD->nmcd.dwItemSpec].GetTextModifies();
            svn_tristate_t propsModifies = m_currentChangedArray[pLVCD->nmcd.dwItemSpec].GetPropsModifies();
            if ((propsModifies == svn_tristate_true)&&(textModifies != svn_tristate_true))
            {
                // property only modification, content of entry hasn't changed: show in gray
                crText = GetSysColor(COLOR_GRAYTEXT);
            }
        }

        // Store the color back in the NMLVCUSTOMDRAW struct.
        pLVCD->clrText = crText;
    }
    else if ( pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT|CDDS_ITEM|CDDS_SUBITEM))
    {
        if ((m_SelectedFilters & LOGFILTER_PATHS)&&(m_filter.IsFilterActive()))
        {
            *pResult = DrawListItemWithMatches(m_ChangedFileListCtrl, pLVCD, NULL);
            return;
        }
    }
}

CRect CLogDlg::DrawListColumnBackground(CListCtrl& listCtrl, NMLVCUSTOMDRAW * pLVCD, PLOGENTRYDATA pLogEntry)
{
    // Get the selected state of the
    // item being drawn.
    LVITEM   rItem;
    SecureZeroMemory(&rItem, sizeof(LVITEM));
    rItem.mask  = LVIF_STATE;
    rItem.iItem = (int)pLVCD->nmcd.dwItemSpec;
    rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    listCtrl.GetItem(&rItem);


    CRect rect;
    listCtrl.GetSubItemRect((int)pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);

    // the rect we get for column 0 always extends over the whole row instead of just
    // the column itself. Since we must not redraw the background for the whole row (other columns
    // might not be asked to redraw), we have to find the right border of the column
    // another way.
    if (pLVCD->iSubItem == 0)
        rect.right = listCtrl.GetColumnWidth(0);

    // Fill the background
    if (IsAppThemed() && SysInfo::Instance().IsVistaOrLater())
    {
        HTHEME hTheme = OpenThemeData(m_hWnd, L"Explorer");
        int state = LISS_NORMAL;
        if (rItem.state & LVIS_SELECTED)
        {
            if (::GetFocus() == listCtrl.m_hWnd)
                state = LISS_SELECTED;
            else
                state = LISS_SELECTEDNOTFOCUS;
        }
        else
        {
            if (pLogEntry && (pLogEntry->GetChangedPaths().ContainsSelfCopy()))
            {
                // unfortunately, the pLVCD->nmcd.uItemState does not contain valid
                // information at this drawing stage. But we can check the whether the
                // previous stage changed the background color of the item
                if (pLVCD->clrTextBk == GetSysColor(COLOR_MENU))
                {
                    HBRUSH brush;
                    brush = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
                    if (brush)
                    {
                        ::FillRect(pLVCD->nmcd.hdc, &rect, brush);
                        ::DeleteObject(brush);
                    }
                }
            }
        }

        if (IsThemeBackgroundPartiallyTransparent(hTheme, LVP_LISTDETAIL, state))
            DrawThemeParentBackground(m_hWnd, pLVCD->nmcd.hdc, &rect);

        DrawThemeBackground(hTheme, pLVCD->nmcd.hdc, LVP_LISTDETAIL, state, &rect, NULL);
        CloseThemeData(hTheme);
    }
    else
    {
        HBRUSH brush;
        if (rItem.state & LVIS_SELECTED)
        {
            if (::GetFocus() == listCtrl.m_hWnd)
                brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
            else
                brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
        }
        else
        {
            if (pLogEntry && pLogEntry->GetChangedPaths().ContainsSelfCopy())
                brush = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
            else
                brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
        }
        if (brush == NULL)
            return rect;

        ::FillRect(pLVCD->nmcd.hdc, &rect, brush);
        ::DeleteObject(brush);
    }

    return rect;
}

LRESULT CLogDlg::DrawListItemWithMatches(CListCtrl& listCtrl, NMLVCUSTOMDRAW * pLVCD, PLOGENTRYDATA pLogEntry)
{
    wstring text;
    text = (LPCTSTR)listCtrl.GetItemText((int)pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem);
    if (text.empty())
        return CDRF_DODEFAULT;

    wstring matchtext = text;
    std::vector<CHARRANGE> ranges = m_filter.GetMatchRanges(matchtext);
    if (!ranges.empty())
    {
        int drawPos = 0;

        // even though we initialize the 'rect' here with nmcd.rc,
        // we must not use it but use the rects from GetItemRect()
        // and GetSubItemRect(). Because on XP, the nmcd.rc has
        // bogus data in it.
        CRect rect = pLVCD->nmcd.rc;

        // find the margin where the text label starts
        CRect labelRC, boundsRC, iconRC;

        listCtrl.GetItemRect((int)pLVCD->nmcd.dwItemSpec, &labelRC, LVIR_LABEL);
        listCtrl.GetItemRect((int)pLVCD->nmcd.dwItemSpec, &iconRC, LVIR_ICON);
        listCtrl.GetItemRect((int)pLVCD->nmcd.dwItemSpec, &boundsRC, LVIR_BOUNDS);

        DrawListColumnBackground(listCtrl, pLVCD, pLogEntry);
        int leftmargin = labelRC.left - boundsRC.left;
        if (pLVCD->iSubItem)
        {
            leftmargin -= iconRC.Width();
        }
        if (pLVCD->iSubItem != 0)
            listCtrl.GetSubItemRect((int)pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);

        int borderWidth = 0;
        if (IsAppThemed() && SysInfo::Instance().IsVistaOrLater())
        {
            HTHEME hTheme = OpenThemeData(m_hWnd, L"LISTVIEW");
            GetThemeMetric(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, LISS_NORMAL, TMT_BORDERSIZE, &borderWidth);
            CloseThemeData(hTheme);
        }
        else
        {
            borderWidth = GetSystemMetrics(SM_CXBORDER);
        }

        if (listCtrl.GetExtendedStyle() & LVS_EX_CHECKBOXES)
        {
            // I'm not very happy about this fixed margin here
            // but I haven't found a way to ask the system what
            // the margin really is.
            // At least it works on XP/Vista/win7, and even with
            // increased font sizes
            leftmargin = 4;
        }

        LVITEM item = {0};
        item.iItem = (int)pLVCD->nmcd.dwItemSpec;
        item.iSubItem = 0;
        item.mask = LVIF_IMAGE | LVIF_STATE;
        item.stateMask = (UINT)-1;
        listCtrl.GetItem(&item);

        // draw the icon for the first column
        if (pLVCD->iSubItem == 0)
        {
            rect = boundsRC;
            rect.right = rect.left + listCtrl.GetColumnWidth(0) - 2*borderWidth;
            rect.left = iconRC.left;

            if (item.iImage >= 0)
            {
                POINT pt;
                pt.x = rect.left;
                pt.y = rect.top;
                CDC dc;
                dc.Attach(pLVCD->nmcd.hdc);
                listCtrl.GetImageList(LVSIL_SMALL)->Draw(&dc, item.iImage, pt, ILD_TRANSPARENT);
                dc.Detach();
                leftmargin -= iconRC.left;
            }
            else
            {
                RECT irc = boundsRC;
                irc.left += borderWidth;
                irc.right = iconRC.left;

                int state = 0;
                if (item.state & LVIS_SELECTED)
                {
                    if (listCtrl.GetHotItem() == item.iItem)
                        state = CBS_CHECKEDHOT;
                    else
                        state = CBS_CHECKEDNORMAL;
                }
                else
                {
                    if (listCtrl.GetHotItem() == item.iItem)
                        state = CBS_UNCHECKEDHOT;
                }
                if ((state)&&(listCtrl.GetExtendedStyle() & LVS_EX_CHECKBOXES))
                {
                    HTHEME hTheme = OpenThemeData(m_hWnd, L"BUTTON");
                    DrawThemeBackground(hTheme, pLVCD->nmcd.hdc, BP_CHECKBOX, state, &irc, NULL);
                    CloseThemeData(hTheme);
                }
            }
        }
        InflateRect(&rect, -(2*borderWidth), 0);

        rect.left += leftmargin;
        RECT rc = rect;

        HTHEME hTheme = OpenThemeData(listCtrl.GetSafeHwnd(), L"Explorer");
        // is the column left- or right-aligned? (we don't handle centered (yet))
        LVCOLUMN Column;
        Column.mask = LVCF_FMT;
        listCtrl.GetColumn(pLVCD->iSubItem, &Column);
        if (Column.fmt & LVCFMT_RIGHT)
        {
            DrawText(pLVCD->nmcd.hdc, text.c_str(), -1, &rc, DT_CALCRECT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
            rect.left = rect.right-(rc.right-rc.left);
            if (!IsAppThemed() || SysInfo::Instance().IsXP())
            {
                rect.left += 2*borderWidth;
                rect.right += 2*borderWidth;
            }
        }
        COLORREF textColor = pLVCD->clrText;
        if ((item.state & LVIS_SELECTED)&&(::GetFocus() == listCtrl.m_hWnd))
        {
            // the theme API really is ridiculous. Instead of returning
            // what was asked for, in most cases we get an "unsupported" error
            // and have to fall back ourselves to the plain windows API to get
            // whatever we want (colors, metrics, ...)
            // What the API should do is to do the fallback automatically and
            // return that value - Windows knows best to what it falls back
            // if something isn't defined in the .msstyles file!
            if (SysInfo::Instance().IsXP())
            {
                // we only do that on XP, because on Vista/Win7, the COLOR_HIGHLIGHTTEXT
                // is *not* used but some other color I don't know where to get from
                if (FAILED(GetThemeColor(hTheme, LVP_LISTITEM, 0, TMT_HIGHLIGHTTEXT, &textColor)))
                    textColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
            }
        }

        SetTextColor(pLVCD->nmcd.hdc, textColor);
        SetBkMode(pLVCD->nmcd.hdc, TRANSPARENT);
        for (std::vector<CHARRANGE>::iterator it = ranges.begin(); it != ranges.end(); ++it)
        {
            rc = rect;
            if (it->cpMin-drawPos)
            {
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMin-drawPos, &rc, DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMin-drawPos, &rc, DT_CALCRECT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
                rect.left = rc.right;
            }
            rc = rect;
            drawPos = it->cpMin;
            if (it->cpMax-drawPos)
            {
                SetTextColor(pLVCD->nmcd.hdc, m_Colors.GetColor(CColors::FilterMatch));
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMax-drawPos, &rc, DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMax-drawPos, &rc, DT_CALCRECT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
                rect.left = rc.right;
                SetTextColor(pLVCD->nmcd.hdc, textColor);
            }
            rc = rect;
            drawPos = it->cpMax;
        }
        DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), -1, &rc, DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
        CloseThemeData(hTheme);
        return CDRF_SKIPDEFAULT;
    }
    return CDRF_DODEFAULT;
}

void CLogDlg::DoSizeV1(int delta)
{
    RemoveMainAnchors();

    // first, reduce the middle section to a minimum.
    // if that is not sufficient, minimize the lower section

    CRect changeListViewRect;
    m_ChangedFileListCtrl.GetClientRect (changeListViewRect);
    CRect messageViewRect;
    GetDlgItem(IDC_MSGVIEW)->GetClientRect (messageViewRect);

    int messageViewDelta = max (-delta, 20 - messageViewRect.Height());
    int changeFileListDelta = -delta - messageViewDelta;

    // set new sizes & positions

    CSplitterControl::ChangeHeight(&m_LogList, delta, CW_TOPALIGN);
    CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), messageViewDelta, CW_TOPALIGN);
    CSplitterControl::ChangePos(GetDlgItem(IDC_MSGVIEW), 0, delta);
    CSplitterControl::ChangePos(GetDlgItem(IDC_SPLITTERBOTTOM), 0, -changeFileListDelta);
    CSplitterControl::ChangeHeight(&m_ChangedFileListCtrl, changeFileListDelta, CW_BOTTOMALIGN);

    AddMainAnchors();
    ArrangeLayout();
    AdjustMinSize();
    SetSplitterRange();
    m_LogList.Invalidate();
    m_ChangedFileListCtrl.Invalidate();
    GetDlgItem(IDC_MSGVIEW)->Invalidate();
}

void CLogDlg::DoSizeV2(int delta)
{
    RemoveMainAnchors();

    // first, reduce the middle section to a minimum.
    // if that is not sufficient, minimize the top section

    CRect logViewRect;
    m_LogList.GetClientRect (logViewRect);
    CRect messageViewRect;
    GetDlgItem(IDC_MSGVIEW)->GetClientRect (messageViewRect);

    int messageViewDelta = max (delta, 20 - messageViewRect.Height());
    int logListDelta = delta - messageViewDelta;

    // set new sizes & positions

    CSplitterControl::ChangeHeight(&m_LogList, logListDelta, CW_TOPALIGN);
    CSplitterControl::ChangePos(GetDlgItem(IDC_SPLITTERTOP), 0, logListDelta);

    CSplitterControl::ChangeHeight(GetDlgItem(IDC_MSGVIEW), messageViewDelta, CW_TOPALIGN);
    CSplitterControl::ChangePos(GetDlgItem(IDC_MSGVIEW), 0, logListDelta);

    CSplitterControl::ChangeHeight(&m_ChangedFileListCtrl, -delta, CW_BOTTOMALIGN);

    AddMainAnchors();
    ArrangeLayout();
    AdjustMinSize();
    SetSplitterRange();
    GetDlgItem(IDC_MSGVIEW)->Invalidate();
    m_ChangedFileListCtrl.Invalidate();
}

void CLogDlg::AdjustMinSize()
{
    // adjust the minimum size of the dialog to prevent the resizing from
    // moving the list control too far down.
    CRect rcChgListView;
    m_ChangedFileListCtrl.GetClientRect(rcChgListView);
    CRect rcLogList;
    m_LogList.GetClientRect(rcLogList);
    CRect rcLogMsg;
    GetDlgItem(IDC_MSGVIEW)->GetClientRect(rcLogMsg);

    SetMinTrackSize(CSize(m_DlgOrigRect.Width(),
        m_DlgOrigRect.Height()-m_ChgOrigRect.Height()-m_LogListOrigRect.Height()-m_MsgViewOrigRect.Height()
        +rcLogMsg.Height()+abs(rcChgListView.Height()-rcLogList.Height())+60));
}

LRESULT CLogDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_NOTIFY:
        if (wParam == IDC_SPLITTERTOP)
        {
            SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
            DoSizeV1(pHdr->delta);
        }
        else if (wParam == IDC_SPLITTERBOTTOM)
        {
            SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
            DoSizeV2(pHdr->delta);
        }
        break;
    }

    return CResizableDialog::DefWindowProc(message, wParam, lParam);
}

void CLogDlg::SetSplitterRange()
{
    if ((m_LogList)&&(m_ChangedFileListCtrl))
    {
        CRect rcTop;
        m_LogList.GetWindowRect(rcTop);
        ScreenToClient(rcTop);

        CRect rcMiddle;
        GetDlgItem(IDC_MSGVIEW)->GetWindowRect(rcMiddle);
        ScreenToClient(rcMiddle);

        CRect rcBottom;
        m_ChangedFileListCtrl.GetWindowRect(rcBottom);
        ScreenToClient(rcBottom);

        m_wndSplitter1.SetRange(rcTop.top+20, rcBottom.bottom-50);
        m_wndSplitter2.SetRange(rcTop.top+50, rcBottom.bottom-20);
    }
}

LRESULT CLogDlg::OnClickedInfoIcon(WPARAM /*wParam*/, LPARAM lParam)
{
    RECT * rect = (LPRECT)lParam;
    CPoint point;
    CString temp;
    point = CPoint(rect->left, rect->bottom);
#define LOGMENUFLAGS(x) (MF_STRING | MF_ENABLED | (m_SelectedFilters & x ? MF_CHECKED : MF_UNCHECKED))
    CMenu popup;
    if (popup.CreatePopupMenu())
    {
        temp.LoadString(IDS_LOG_FILTER_MESSAGES);
        popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_MESSAGES), LOGFILTER_MESSAGES, temp);
        temp.LoadString(IDS_LOG_FILTER_PATHS);
        popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_PATHS), LOGFILTER_PATHS, temp);
        temp.LoadString(IDS_LOG_FILTER_AUTHORS);
        popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_AUTHORS), LOGFILTER_AUTHORS, temp);
        temp.LoadString(IDS_LOG_FILTER_REVS);
        popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_REVS), LOGFILTER_REVS, temp);
        temp.LoadString(IDS_LOG_FILTER_BUGIDS);
        popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_BUGID), LOGFILTER_BUGID, temp);
        temp.LoadString(IDS_LOG_DATE);
        popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_DATE), LOGFILTER_DATE, temp);
        temp.LoadString(IDS_LOG_FILTER_DATERANGE);
        popup.AppendMenu(LOGMENUFLAGS(LOGFILTER_DATERANGE), LOGFILTER_DATERANGE, temp);

        popup.AppendMenu(MF_SEPARATOR, NULL);

        temp.LoadString(IDS_LOG_FILTER_REGEX);
        popup.AppendMenu(MF_STRING | MF_ENABLED | (m_bFilterWithRegex ? MF_CHECKED : MF_UNCHECKED), LOGFILTER_REGEX, temp);
        temp.LoadString(IDS_LOG_FILTER_CASESENSITIVE);
        popup.AppendMenu(MF_STRING | MF_ENABLED | (m_bFilterCaseSensitively ? MF_CHECKED : MF_UNCHECKED), LOGFILTER_CASE, temp);

        m_tooltips.Pop();
        int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, 0);
        switch (selection)
        {
        case 0 :
            break;

        case LOGFILTER_REGEX :
            m_bFilterWithRegex = !m_bFilterWithRegex;
            CRegDWORD(_T("Software\\TortoiseSVN\\UseRegexFilter")) = m_bFilterWithRegex;
            CheckRegexpTooltip();
            break;

        case LOGFILTER_CASE:
            m_bFilterCaseSensitively = !m_bFilterCaseSensitively;
            CRegDWORD(_T("Software\\TortoiseSVN\\FilterCaseSensitively")) = m_bFilterCaseSensitively;
            break;

        default:

            m_SelectedFilters ^= selection;
            SetFilterCueText();
            AdjustDateFilterVisibility();
        }

        if (selection != 0)
            SetTimer(LOGFILTER_TIMER, 1000, NULL);
    }
    return 0L;
}

LRESULT CLogDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    KillTimer(LOGFILTER_TIMER);

    m_sFilterText.Empty();
    UpdateData(FALSE);
    theApp.DoWaitCursor(1);
    AutoStoreSelection();
    FillLogMessageCtrl(false);

    // reset the time filter too
    m_logEntries.ClearFilter(!!m_bHideNonMergeables, &m_mergedRevs, m_copyfromrev);
    m_timFrom = m_logEntries.GetMinDate();
    m_timTo = m_logEntries.GetMaxDate();
    m_DateFrom.SetTime(&m_timFrom);
    m_DateTo.SetTime(&m_timTo);
    m_DateFrom.SetRange(&m_timFrom, &m_timTo);
    m_DateTo.SetRange(&m_timFrom, &m_timTo);

    m_filter = CLogDlgFilter();

    m_LogList.SetItemCountEx(0);
    m_LogList.SetItemCountEx(ShownCountWithStopped());
    m_LogList.RedrawItems(0, ShownCountWithStopped());
    m_LogList.SetRedraw(false);
    ResizeAllListCtrlCols(true);
    m_LogList.SetRedraw(true);
    m_LogList.Invalidate();
    m_ChangedFileListCtrl.Invalidate();

    theApp.DoWaitCursor(-1);
    GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_SEARCHEDIT)->SetFocus();

    AutoRestoreSelection();

    return 0L;
}


void CLogDlg::SetFilterCueText()
{
    CString temp(MAKEINTRESOURCE(IDS_LOG_FILTER_BY));

    if (m_SelectedFilters & LOGFILTER_MESSAGES)
    {
        temp += _T(" ");
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_MESSAGES));
    }
    if (m_SelectedFilters & LOGFILTER_PATHS)
    {
        if (!temp.IsEmpty())
            temp += _T(", ");
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_PATHS));
    }
    if (m_SelectedFilters & LOGFILTER_AUTHORS)
    {
        if (!temp.IsEmpty())
            temp += _T(", ");
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_AUTHORS));
    }
    if (m_SelectedFilters & LOGFILTER_REVS)
    {
        if (!temp.IsEmpty())
            temp += _T(", ");
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_REVS));
    }
    if (m_SelectedFilters & LOGFILTER_BUGID)
    {
        if (!temp.IsEmpty())
            temp += _T(", ");
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_BUGIDS));
    }
    if (m_SelectedFilters & LOGFILTER_DATE)
    {
        if (!temp.IsEmpty())
            temp += _T(", ");
        temp += CString(MAKEINTRESOURCE(IDS_LOG_DATE));
    }
    if (m_SelectedFilters & LOGFILTER_DATERANGE)
    {
        if (!temp.IsEmpty())
            temp += _T(", ");
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_DATERANGE));
    }

    // to make the cue banner text appear more to the right of the edit control
    temp = _T("   ")+temp;
    m_cFilter.SetCueBanner(temp);
}

void CLogDlg::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

    // Create a pointer to the item
    LV_ITEM* pItem = &(pDispInfo)->item;

    // Which item number?
    size_t itemid = pItem->iItem;
    PLOGENTRYDATA pLogEntry = NULL;
    if (itemid < m_logEntries.GetVisibleCount())
        pLogEntry = m_logEntries.GetVisible (pItem->iItem);

    if (pItem->mask & LVIF_INDENT)
    {
        pItem->iIndent = 0;
    }
    if (pItem->mask & LVIF_IMAGE)
    {
        pItem->mask |= LVIF_STATE;
        pItem->stateMask = LVIS_STATEIMAGEMASK;

        if (pLogEntry)
        {
            if (SysInfo::Instance().IsVistaOrLater())
            {
                UINT state = m_LogList.GetItemState(pItem->iItem, LVIS_SELECTED);
                if (state & LVIS_SELECTED)
                {
                    //Turn check box on
                    pItem->state = INDEXTOSTATEIMAGEMASK(2);
                }
                else
                {
                    //Turn check box off
                    pItem->state = INDEXTOSTATEIMAGEMASK(1);
                }
            }
            else
            {
                if (pLogEntry->GetChecked())
                {
                    //Turn check box on
                    pItem->state = INDEXTOSTATEIMAGEMASK(2);
                }
                else
                {
                    //Turn check box off
                    pItem->state = INDEXTOSTATEIMAGEMASK(1);
                }
            }
        }
    }
    if (pItem->mask & LVIF_TEXT)
    {
        // By default, clear text buffer.
        lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);

        bool bOutOfRange = pItem->iItem >= ShownCountWithStopped();

        if (m_bLogThreadRunning || bOutOfRange)
            return;


        // Which column?
        switch (pItem->iSubItem)
        {
        case 0: //revision
            if (pLogEntry)
            {
                _stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), pLogEntry->GetRevision());
                // to make the child entries indented, add spaces
                size_t len = _tcslen(pItem->pszText);
                TCHAR * pBuf = pItem->pszText + len;
                DWORD nSpaces = m_logEntries.GetMaxDepth() - pLogEntry->GetDepth();
                while ((pItem->cchTextMax >= (int)len)&&(nSpaces))
                {
                    *pBuf = ' ';
                    pBuf++;
                    nSpaces--;
                }
                *pBuf = 0;
            }
            break;
        case 1: //action -- dummy text, not drawn. Used to trick the auto-column resizing to not go below the icons
            if (pLogEntry)
                lstrcpyn(pItem->pszText, L"XXXXXXXXXX", pItem->cchTextMax);
            break;
        case 2: //author
            if (pLogEntry)
            {
                lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetAuthor()).c_str(), pItem->cchTextMax);
            }
            break;
        case 3: //date
            if (pLogEntry)
            {
                lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetDateString()).c_str(), pItem->cchTextMax);
            }
            break;
        case 4: //message or bug id
            if (m_bShowBugtraqColumn)
            {
                if (pLogEntry)
                {
                    lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetBugIDs()).c_str(), pItem->cchTextMax);
                }
                break;
            }
            // fall through here!
        case 5:
            if (pLogEntry)
            {
                // Add as many characters as possible from the short log message
                // to the list control. If the message is longer than
                // allowed width, add "..." as an indication.
                const int dots_len = 3;
                CString shortMessage = pLogEntry->GetShortMessageUTF16();
                if (shortMessage.GetLength() >= pItem->cchTextMax && pItem->cchTextMax > dots_len)
                {
                    lstrcpyn(pItem->pszText, (LPCTSTR)shortMessage, pItem->cchTextMax - dots_len);
                    lstrcpyn(pItem->pszText + pItem->cchTextMax - dots_len - 1, _T("..."), dots_len + 1);
                }
                else
                    lstrcpyn(pItem->pszText, (LPCTSTR)shortMessage, pItem->cchTextMax);
            }
            else if ((itemid == m_logEntries.GetVisibleCount()) && m_bStrict && m_bStrictStopped)
            {
                CString sTemp;
                sTemp.LoadString(IDS_LOG_STOPONCOPY_HINT);
                lstrcpyn(pItem->pszText, sTemp, pItem->cchTextMax);
            }
            break;
        default:
            ASSERT(false);
        }
    }
}

void CLogDlg::OnLvnGetdispinfoChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;

    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    //Create a pointer to the item
    LV_ITEM* pItem= &(pDispInfo)->item;

    if (m_bLogThreadRunning)
    {
        if (pItem->mask & LVIF_TEXT)
            lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
        return;
    }
    if (m_bSingleRevision && ((size_t)pItem->iItem >= m_currentChangedArray.GetCount()))
    {
        if (pItem->mask & LVIF_TEXT)
            lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
        return;
    }
    if (!m_bSingleRevision && (pItem->iItem >= m_currentChangedPathList.GetCount()))
    {
        if (pItem->mask & LVIF_TEXT)
            lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
        return;
    }

    //Does the list need text information?
    if (pItem->mask & LVIF_TEXT)
    {
        //Which column?
        switch (pItem->iSubItem)
        {
        case 0: //path
            lstrcpyn ( pItem->pszText
                     , (LPCTSTR) (m_currentChangedArray.GetCount() > 0
                           ? m_currentChangedArray[pItem->iItem].GetPath()
                           : m_currentChangedPathList[pItem->iItem].GetSVNPathString())
                     , pItem->cchTextMax);
            break;

        case 1: //Action
            lstrcpyn ( pItem->pszText
                     , m_bSingleRevision && m_currentChangedArray.GetCount() > (size_t)pItem->iItem
                           ? (LPCTSTR)CUnicodeUtils::GetUnicode
                                (m_currentChangedArray[pItem->iItem].GetActionString().c_str())
                           : _T("")
                     , pItem->cchTextMax);
            break;

        case 2: //copyfrom path
            lstrcpyn ( pItem->pszText
                     , m_bSingleRevision && m_currentChangedArray.GetCount() > (size_t)pItem->iItem
                           ? (LPCTSTR)m_currentChangedArray[pItem->iItem].GetCopyFromPath()
                           : _T("")
                     , pItem->cchTextMax);
            break;

        case 3: //revision
            svn_revnum_t revision = 0;
            if (m_bSingleRevision && m_currentChangedArray.GetCount() > (size_t)pItem->iItem)
                revision = m_currentChangedArray[pItem->iItem].GetCopyFromRev();

            if (revision == 0)
                lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);
            else
                _stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%ld"), revision);
            break;
        }
    }
    if (pItem->mask & LVIF_IMAGE)
    {
        int icon_idx = 0;
        if (m_currentChangedArray.GetCount() > (size_t)pItem->iItem)
        {
            const CLogChangedPath& cpath = m_currentChangedArray[pItem->iItem];
            if (cpath.GetNodeKind() == svn_node_dir)
                icon_idx = m_nIconFolder;
            else
                icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(CTSVNPath(cpath.GetPath()));
        }
        else
        {
            icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(m_currentChangedPathList[pItem->iItem]);
        }
        pDispInfo->item.iImage = icon_idx;
    }

    *pResult = 0;
}

void CLogDlg::OnEnChangeSearchedit()
{
    UpdateData();
    if (m_sFilterText.IsEmpty())
    {
        AutoStoreSelection();

        // clear the filter, i.e. make all entries appear
        theApp.DoWaitCursor(1);
        KillTimer(LOGFILTER_TIMER);
        FillLogMessageCtrl(false);
        m_filter = CLogDlgFilter();
        m_logEntries.Filter (m_tFrom, m_tTo, !!m_bHideNonMergeables, &m_mergedRevs, m_copyfromrev);
        m_LogList.SetItemCountEx(0);
        m_LogList.SetItemCountEx(ShownCountWithStopped());
        m_LogList.RedrawItems(0, ShownCountWithStopped());
        m_LogList.SetRedraw(false);
        ResizeAllListCtrlCols(true);
        m_LogList.SetRedraw(true);
        m_LogList.Invalidate();
        m_ChangedFileListCtrl.Invalidate();
        theApp.DoWaitCursor(-1);
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
        DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning)||(m_logEntries.GetVisibleCount() == 0))));

        AutoRestoreSelection();
        return;
    }
    if (Validate(m_sFilterText) && FilterConditionChanged())
        SetTimer(LOGFILTER_TIMER, 1000, NULL);
    else
        KillTimer(LOGFILTER_TIMER);
}

bool CLogDlg::ValidateRegexp(LPCTSTR regexp_str, tr1::wregex& pat, bool bMatchCase /* = false */)
{
    try
    {
        tr1::regex_constants::syntax_option_type type = tr1::regex_constants::ECMAScript;
        if (!bMatchCase)
            type |= tr1::regex_constants::icase;
        pat = tr1::wregex(regexp_str, type);
        return true;
    }
    catch (exception) {}
    return false;
}

bool CLogDlg::Validate(LPCTSTR string)
{
    if (!m_bFilterWithRegex)
        return true;
    tr1::wregex pat;
    return ValidateRegexp(string, pat, false);
}

bool CLogDlg::FilterConditionChanged()
{
    // actually filter the data

    bool scanRelevantPathsOnly = (m_cShowPaths.GetState() & 0x0003)==BST_CHECKED;
    CLogDlgFilter filter ( m_sFilterText
                         , m_bFilterWithRegex
                         , m_SelectedFilters
                         , m_bFilterCaseSensitively
                         , m_tFrom
                         , m_tTo
                         , scanRelevantPathsOnly
                         , &m_mergedRevs
                         , !!m_bHideNonMergeables
                         , m_copyfromrev
                         , NO_REVISION);

    return m_filter != filter;
}

void CLogDlg::RecalculateShownList(svn_revnum_t revToKeep)
{
    // actually filter the data

    bool scanRelevantPathsOnly = (m_cShowPaths.GetState() & 0x0003)==BST_CHECKED;
    CLogDlgFilter filter ( m_sFilterText
                         , m_bFilterWithRegex
                         , m_SelectedFilters
                         , m_bFilterCaseSensitively
                         , m_tFrom
                         , m_tTo
                         , scanRelevantPathsOnly
                         , &m_mergedRevs
                         , !!m_bHideNonMergeables
                         , m_copyfromrev
                         , revToKeep);
    m_filter = filter;
    m_logEntries.Filter (filter);
}

void CLogDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == LOGFILTER_TIMER)
    {
        if (m_bLogThreadRunning || IsIconic())
        {
            // thread still running! So just restart the timer.
            SetTimer(LOGFILTER_TIMER, 1000, NULL);
            return;
        }
        CWnd * focusWnd = GetFocus();
        bool bSetFocusToFilterControl = ((focusWnd != GetDlgItem(IDC_DATEFROM))&&(focusWnd != GetDlgItem(IDC_DATETO))
            && (focusWnd != GetDlgItem(IDC_LOGLIST)));
        if (m_sFilterText.IsEmpty())
        {
            DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning)||(m_logEntries.GetVisibleCount() == 0))));
            // do not return here!
            // we also need to run the filter if the filter text is empty:
            // 1. to clear an existing filter
            // 2. to rebuild the filtered list after sorting
        }
        theApp.DoWaitCursor(1);
        AutoStoreSelection();
        KillTimer(LOGFILTER_TIMER);

        // now start filter the log list
        SortAndFilter();

        m_LogList.SetItemCountEx(0);
        m_LogList.SetItemCountEx(ShownCountWithStopped());
        m_LogList.RedrawItems(0, ShownCountWithStopped());
        m_LogList.SetRedraw(false);
        ResizeAllListCtrlCols(true);
        m_LogList.SetRedraw(true);
        m_LogList.Invalidate();
        if ( m_LogList.GetItemCount()==1 )
        {
            m_LogList.SetSelectionMark(0);
            m_LogList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
        }
        theApp.DoWaitCursor(-1);
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
        if (bSetFocusToFilterControl)
            GetDlgItem(IDC_SEARCHEDIT)->SetFocus();

        AutoRestoreSelection();
    } // if (nIDEvent == LOGFILTER_TIMER)
    DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning)||(m_logEntries.GetVisibleCount() == 0))));
    __super::OnTimer(nIDEvent);
}

void CLogDlg::OnDtnDatetimechangeDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    CTime _time;
    m_DateTo.GetTime(_time);
    try
    {
        CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 23, 59, 59);
        if (time.GetTime() != m_tTo)
        {
            m_tTo = (DWORD)time.GetTime();
            SetTimer(LOGFILTER_TIMER, 10, NULL);
        }
    }
    catch (CAtlException)
    {
    }

    *pResult = 0;
}

void CLogDlg::OnDtnDatetimechangeDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    CTime _time;
    m_DateFrom.GetTime(_time);
    try
    {
        CTime time(_time.GetYear(), _time.GetMonth(), _time.GetDay(), 0, 0, 0);
        if (time.GetTime() != m_tFrom)
        {
            m_tFrom = (DWORD)time.GetTime();
            SetTimer(LOGFILTER_TIMER, 10, NULL);
        }
    }
    catch (CAtlException)
    {
    }

    *pResult = 0;
}

CTSVNPathList CLogDlg::GetChangedPathsAndMessageSketchFromSelectedRevisions(CString& sMessageSketch, CLogChangedPathArray& currentChangedArray)
{
    CTSVNPathList pathList;
    sMessageSketch.Empty();

    quick_hash_set<LogCache::index_t> pathIDsAdded;
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    if (pos != NULL)
    {
        while (pos)
        {
            size_t nextpos = m_LogList.GetNextSelectedItem(pos);
            if (nextpos >= m_logEntries.GetVisibleCount())
                continue;

            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (nextpos);
            CString sRevMsg;
            sRevMsg.Format(L"r%ld\n%s\n---------------------\n", pLogEntry->GetRevision(), pLogEntry->GetShortMessageUTF16());
            sMessageSketch +=  sRevMsg;
            const CLogChangedPathArray& cpatharray = pLogEntry->GetChangedPaths();
            for (size_t cpPathIndex = 0; cpPathIndex<cpatharray.GetCount(); ++cpPathIndex)
            {
                const CLogChangedPath& cpath = cpatharray[cpPathIndex];

                LogCache::index_t pathID = cpath.GetCachedPath().GetIndex();
                if (pathIDsAdded.contains (pathID))
                    continue;

                pathIDsAdded.insert (pathID);

                if (((m_cShowPaths.GetState() & 0x0003)!=BST_CHECKED)
                    || cpath.IsRelevantForStartPath())
                {
                    CTSVNPath path;
                    path.SetFromSVN(cpath.GetPath());

                    pathList.AddPath(path);
                    currentChangedArray.Add(cpath);
                }
            }
        }
    }
    return pathList;
}

void CLogDlg::SortByColumn(int nSortColumn, bool bAscending)
{
    if (nSortColumn > 5)
    {
        ATLASSERT(0);
        return;
    }

    // sort by message instead of bug id, if bug ids are not visible
    if ((nSortColumn == 4) && !m_bShowBugtraqColumn)
        ++nSortColumn;

    m_logEntries.Sort (CLogDataVector::SortColumn (nSortColumn), bAscending);
}

void CLogDlg::SortAndFilter (svn_revnum_t revToKeep)
{
    SortByColumn (m_nSortColumn, m_bAscending);
    RecalculateShownList (revToKeep);
}

void CLogDlg::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
    if ((m_bLogThreadRunning)||(m_LogList.HasText()))
        return;     //no sorting while the arrays are filled
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    const int nColumn = pNMLV->iSubItem;
    m_bAscending = nColumn == m_nSortColumn ? !m_bAscending : TRUE;
    m_nSortColumn = nColumn;
    SortAndFilter();
    SetSortArrow(&m_LogList, m_nSortColumn, !!m_bAscending);

    // clear the selection states
    for (POSITION pos = m_LogList.GetFirstSelectedItemPosition(); pos != NULL; )
        m_LogList.SetItemState(m_LogList.GetNextSelectedItem(pos), 0, LVIS_SELECTED);

    m_LogList.SetSelectionMark(-1);

    m_LogList.Invalidate();
    UpdateLogInfoLabel();
    // the "next 100" button only makes sense if the log messages
    // are sorted by revision in descending order
    if ((m_nSortColumn)||(m_bAscending))
    {
        DialogEnableWindow(IDC_NEXTHUNDRED, false);
    }
    else
    {
        DialogEnableWindow(IDC_NEXTHUNDRED, true);
    }
    *pResult = 0;
}

void CLogDlg::SetSortArrow(CListCtrl * control, int nColumn, bool bAscending)
{
    if (control == NULL)
        return;
    // set the sort arrow
    CHeaderCtrl * pHeader = control->GetHeaderCtrl();
    HDITEM HeaderItem = {0};
    HeaderItem.mask = HDI_FORMAT;
    for (int i=0; i<pHeader->GetItemCount(); ++i)
    {
        pHeader->GetItem(i, &HeaderItem);
        HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        pHeader->SetItem(i, &HeaderItem);
    }
    if (nColumn >= 0)
    {
        pHeader->GetItem(nColumn, &HeaderItem);
        HeaderItem.fmt |= (bAscending ? HDF_SORTUP : HDF_SORTDOWN);
        pHeader->SetItem(nColumn, &HeaderItem);
    }
}
void CLogDlg::OnLvnColumnclickChangedFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
    if ((m_bLogThreadRunning)||(m_LogList.HasText()))
        return;     //no sorting while the arrays are filled

    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    const int nColumn = pNMLV->iSubItem;

    // multi-rev selection shows paths only -> can only sort by column
    if ((m_currentChangedPathList.GetCount() > 0) && (nColumn > 0))
        return;

    m_bAscendingPathList = nColumn == m_nSortColumnPathList ? !m_bAscendingPathList : TRUE;
    m_nSortColumnPathList = nColumn;
    if (m_currentChangedArray.GetCount() > 0)
        m_currentChangedArray.Sort (m_nSortColumnPathList, m_bAscendingPathList);
    else
        m_currentChangedPathList.SortByPathname (!m_bAscendingPathList);

    SetSortArrow(&m_ChangedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
    m_ChangedFileListCtrl.Invalidate();
    *pResult = 0;
}

void CLogDlg::ResizeAllListCtrlCols(bool bOnlyVisible)
{
    CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(m_LogList.GetDlgItem(0));
    if (pHdrCtrl == 0)
        return;

    const int maxcol = pHdrCtrl->GetItemCount()-1;
    size_t startRow = 0;
    size_t endRow = m_LogList.GetItemCount();
    if (bOnlyVisible)
    {
        if (endRow > 0)
            startRow = m_LogList.GetTopIndex();
        endRow = startRow + m_LogList.GetCountPerPage();
    }
    for (int col = 0; col <= maxcol; col++)
    {
        TCHAR textbuf[MAX_PATH + 1] = {};
        HDITEM hdi = {0};
        hdi.mask = HDI_TEXT;
        hdi.pszText = textbuf;
        hdi.cchTextMax = _countof(textbuf) - 1;
        pHdrCtrl->GetItem(col, &hdi);
        int cx = m_LogList.GetStringWidth(textbuf)+20; // 20 pixels for col separator and margin
        for (size_t index = startRow; index<endRow; ++index)
        {
            // get the width of the string and add 14 pixels for the column separator and margins
            int linewidth = m_LogList.GetStringWidth(m_LogList.GetItemText((int)index, col)) + 14;
            if (index < m_logEntries.GetVisibleCount())
            {
                PLOGENTRYDATA pCurLogEntry = m_logEntries.GetVisible(index);
                if ((pCurLogEntry)&&(pCurLogEntry->GetRevision() == m_wcRev))
                {
                    HFONT hFont = (HFONT)m_LogList.SendMessage(WM_GETFONT);
                    // set the bold font and ask for the string width again
                    m_LogList.SendMessage(WM_SETFONT, (WPARAM)m_boldFont, NULL);
                    linewidth = m_LogList.GetStringWidth(m_LogList.GetItemText((int)index, col)) + 14;
                    // restore the system font
                    m_LogList.SendMessage(WM_SETFONT, (WPARAM)hFont, NULL);
                }
            }
            if (index == 0)
            {
                // add the image size
                CImageList * pImgList = m_LogList.GetImageList(LVSIL_SMALL);
                if ((pImgList)&&(pImgList->GetImageCount()))
                {
                    IMAGEINFO imginfo;
                    pImgList->GetImageInfo(0, &imginfo);
                    linewidth += (imginfo.rcImage.right - imginfo.rcImage.left);
                    linewidth += 3; // 3 pixels between icon and text
                }
            }
            if (cx < linewidth)
                cx = linewidth;
        }
        // Adjust columns "Actions" containing icons
        if (col == 1)
        {
            const int nMinimumWidth = ICONITEMBORDER+16*5;
            if (cx < nMinimumWidth)
            {
                cx = nMinimumWidth;
            }
        }
        if ((col == 0)&&(m_bSelect)&&(SysInfo::Instance().IsVistaOrLater()))
        {
            cx += 16;   // add space for the checkbox
        }
        // keep the bug id column small
        if ((col == 4)&&(m_bShowBugtraqColumn))
        {
            if (cx > (int)(DWORD)m_regMaxBugIDColWidth)
            {
                cx = (int)(DWORD)m_regMaxBugIDColWidth;
            }
        }

        m_LogList.SetColumnWidth(col, cx);
    }
}

void CLogDlg::OnBnClickedHidepaths()
{
    FillLogMessageCtrl();
    m_ChangedFileListCtrl.Invalidate();
}


void CLogDlg::OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = -1;
    LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);

    if (pFindInfo->lvfi.flags & LVFI_PARAM)
        return;
    if ((pFindInfo->iStart < 0)||(pFindInfo->iStart >= (int)m_logEntries.GetVisibleCount()))
        return;
    if (pFindInfo->lvfi.psz == 0)
        return;

    CString sCmp = pFindInfo->lvfi.psz;
    if(DoFindItemLogList(pFindInfo, pFindInfo->iStart, m_logEntries.GetVisibleCount(),
        sCmp, pResult ))
    {
        return;
    }
    if (pFindInfo->lvfi.flags & LVFI_WRAP)
    {
        if(DoFindItemLogList(pFindInfo, 0, pFindInfo->iStart, sCmp, pResult ))
            return;
    }
    *pResult = -1;
}

bool CLogDlg::DoFindItemLogList(LPNMLVFINDITEM pFindInfo, size_t startIndex,
    size_t endIndex, const CString& whatToFind, LRESULT* pResult)
{
    CString sRev;
    for (size_t i=startIndex; i<endIndex; ++i)
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(i);
        sRev.Format(_T("%ld"), pLogEntry->GetRevision());
        if (pFindInfo->lvfi.flags & LVFI_PARTIAL)
        {
            if (whatToFind.Compare(sRev.Left(whatToFind.GetLength()))==0)
            {
                *pResult = i;
                return true;
            }
        }
        else
        {
            if (whatToFind.Compare(sRev)==0)
            {
                *pResult = i;
                return true;
            }
        }
    }
    return false;
}

void CLogDlg::OnBnClickedCheckStoponcopy()
{
    if (!GetDlgItem(IDC_GETALL)->IsWindowEnabled())
        return;

    // ignore old fetch limits when switching
    // between copy-following and stop-on-copy
    // (otherwise stop-on-copy will limit what
    // we see immediately after switching to
    // copy-following)

    m_endrev = 0;

    // now, restart the query

    Refresh();
}

void CLogDlg::OnBnClickedIncludemerge()
{
    m_endrev = 0;

    m_limit = 0;
    Refresh();
}

void CLogDlg::UpdateLogInfoLabel()
{
    svn_revnum_t rev1   = 0;
    svn_revnum_t rev2   = 0;
    long selectedrevs   = 0;
    size_t changedPaths = 0;
    if (m_logEntries.GetVisibleCount())
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(0);
        rev1 = pLogEntry->GetRevision();
        pLogEntry = m_logEntries.GetVisible (m_logEntries.GetVisibleCount()-1);
        rev2 = pLogEntry->GetRevision();
        selectedrevs = m_LogList.GetSelectedCount();

        if (m_bSingleRevision)
        {
            changedPaths = m_currentChangedArray.GetCount();
        }
        else if (m_currentChangedPathList.GetCount())
        {
            changedPaths = m_currentChangedPathList.GetCount();
        }
    }
    CString sTemp;
    sTemp.FormatMessage(IDS_LOG_LOGINFOSTRING, m_logEntries.GetVisibleCount(), rev2, rev1, selectedrevs, changedPaths);
    m_sLogInfo = sTemp;
    UpdateData(FALSE);
    GetDlgItem(IDC_LOGINFO)->Invalidate();
}

void CLogDlg::ShowContextMenuForRevisions(CWnd* /*pWnd*/, CPoint point)
{
    int selIndex = m_LogList.GetSelectionMark();
    if (selIndex < 0)
        return; // nothing selected, nothing to do with a context menu

    // if the user selected the info text telling about not all revisions shown due to
    // the "stop on copy/rename" option, we also don't show the context menu
    if ((m_bStrictStopped)&&(selIndex == (int)m_logEntries.GetVisibleCount()))
        return;

    // if the context menu is invoked through the keyboard, we have to use
    // a calculated position on where to anchor the menu on
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        m_LogList.GetItemRect(selIndex, &rect, LVIR_LABEL);
        m_LogList.ClientToScreen(&rect);
        point = rect.CenterPoint();
    }
    m_nSearchIndex = selIndex;
    m_bCancelled = FALSE;

    // calculate some information the context menu commands can use
    CString pathURL = GetURLFromPath(m_path);
    CString relPathURL = pathURL.Mid(m_sRepositoryRoot.GetLength());
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    int indexNext = m_LogList.GetNextSelectedItem(pos);
    if ((indexNext < 0)||(indexNext >= (int)m_logEntries.GetVisibleCount()))
        return;
    PLOGENTRYDATA pSelLogEntry = m_logEntries.GetVisible(indexNext);
    SVNRev revSelected = pSelLogEntry->GetRevision();
    SVNRev revPrevious = svn_revnum_t(revSelected)-1;

    const CLogChangedPathArray& paths = pSelLogEntry->GetChangedPaths();
    if (paths.GetCount() <= 2)
    {
        for (size_t i=0; i<paths.GetCount(); ++i)
        {
            const CLogChangedPath& changedpath = paths[i];
            if (changedpath.GetCopyFromRev() && (changedpath.GetPath().Compare(relPathURL)==0))
                revPrevious = changedpath.GetCopyFromRev();
        }
    }
    SVNRev revSelected2;
    if (pos)
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
        revSelected2 = pLogEntry->GetRevision();
    }
    bool bAllFromTheSameAuthor = true;
    std::vector<PLOGENTRYDATA> selEntries;
    SVNRev revLowest, revHighest;
    SVNRevRangeArray revisionRanges;
    {
        std::vector<svn_revnum_t> revisions;
        revisions.reserve (m_logEntries.GetVisibleCount());

        POSITION pos2 = m_LogList.GetFirstSelectedItemPosition();
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos2));
        revisions.push_back (pLogEntry->GetRevision());
        selEntries.push_back(pLogEntry);

        const std::string& firstAuthor = pLogEntry->GetAuthor();
        while (pos2)
        {
            int index2 = m_LogList.GetNextSelectedItem(pos2);
            if (index2 < (int)m_logEntries.GetVisibleCount())
            {
                pLogEntry = m_logEntries.GetVisible(index2);
                revisions.push_back (pLogEntry->GetRevision());
                selEntries.push_back(pLogEntry);
                if (firstAuthor != pLogEntry->GetAuthor())
                    bAllFromTheSameAuthor = false;
            }
        }

        revisionRanges.AddRevisions (revisions);
        revLowest = revisionRanges.GetLowestRevision();
        revHighest = revisionRanges.GetHighestRevision();
    }



    //entry is selected, now show the popup menu
    CIconMenu popup;
    if (popup.CreatePopupMenu())
    {
        if ((m_LogList.GetSelectedCount() == 1) && (pSelLogEntry->GetDepth()==0))
        {
            if (!m_path.IsDirectory())
            {
                if (m_hasWC)
                {
                    popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
                    popup.AppendMenuIcon(ID_BLAMECOMPARE, IDS_LOG_POPUP_BLAMECOMPARE, IDI_BLAME);
                }
                popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
                popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
                popup.AppendMenu(MF_SEPARATOR, NULL);
                popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE, IDI_SAVEAS);
                popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
                popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
                popup.AppendMenuIcon(ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);
                popup.AppendMenu(MF_SEPARATOR, NULL);
            }
            else
            {
                if (m_hasWC)
                {
                    popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
                    // TODO:
                    // TortoiseMerge could be improved to take a /blame switch
                    // and then not 'cat' the files from a unified diff but
                    // blame then.
                    // But until that's implemented, the context menu entry for
                    // this feature is commented out.
                    //popup.AppendMenu(ID_BLAMECOMPARE, IDS_LOG_POPUP_BLAMECOMPARE, IDI_BLAME);
                }
                popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
                popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
                popup.AppendMenuIcon(ID_BLAMEWITHPREVIOUS, IDS_LOG_POPUP_BLAMEWITHPREVIOUS, IDI_BLAME);
                popup.AppendMenu(MF_SEPARATOR, NULL);
            }
            if (!m_ProjectProperties.sWebViewerRev.IsEmpty())
            {
                popup.AppendMenuIcon(ID_VIEWREV, IDS_LOG_POPUP_VIEWREV);
            }
            if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
            {
                popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);
            }
            if ((!m_ProjectProperties.sWebViewerPathRev.IsEmpty())||
                (!m_ProjectProperties.sWebViewerRev.IsEmpty()))
            {
                popup.AppendMenu(MF_SEPARATOR, NULL);
            }

            popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
            popup.AppendMenuIcon(ID_COPY, IDS_LOG_POPUP_COPY, IDI_COPY);
            if (m_hasWC)
            {
                popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATE, IDI_UPDATE);
                popup.AppendMenuIcon(ID_REVERTTOREV, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);
                popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
                popup.AppendMenuIcon(ID_MERGEREV, IDS_LOG_POPUP_MERGEREV, IDI_MERGE);
            }
            if (m_path.IsDirectory())
            {
                popup.AppendMenuIcon(ID_CHECKOUT, IDS_MENUCHECKOUT, IDI_CHECKOUT);
                popup.AppendMenuIcon(ID_EXPORT, IDS_MENUEXPORT, IDI_EXPORT);
            }
            popup.AppendMenu(MF_SEPARATOR, NULL);
        }
        else if (m_LogList.GetSelectedCount() >= 2)
        {
            bool bAddSeparator = false;
            if (IsSelectionContinuous() || (m_LogList.GetSelectedCount() == 2))
            {
                popup.AppendMenuIcon(ID_COMPARETWO, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
                popup.AppendMenuIcon(ID_GNUDIFF2, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
            }
            if (m_LogList.GetSelectedCount() == 2)
            {
                popup.AppendMenuIcon(ID_BLAMETWO, IDS_LOG_POPUP_BLAMEREVS, IDI_BLAME);
                bAddSeparator = true;
            }
            if (m_hasWC)
            {
                popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREVS, IDI_REVERT);
                if (m_hasWC)
                    popup.AppendMenuIcon(ID_MERGEREV, IDS_LOG_POPUP_MERGEREVS, IDI_MERGE);
                bAddSeparator = true;
            }
            if (bAddSeparator)
                popup.AppendMenu(MF_SEPARATOR, NULL);
        }

        if ((!selEntries.empty())&&(bAllFromTheSameAuthor))
        {
            popup.AppendMenuIcon(ID_EDITAUTHOR, IDS_LOG_POPUP_EDITAUTHOR);
        }
        if (m_LogList.GetSelectedCount() == 1)
        {
            popup.AppendMenuIcon(ID_EDITLOG, IDS_LOG_POPUP_EDITLOG);
            popup.AppendMenuIcon(ID_REVPROPS, IDS_REPOBROWSE_SHOWREVPROP, IDI_PROPERTIES); // "Show Revision Properties"
            popup.AppendMenu(MF_SEPARATOR, NULL);
        }
        if (m_LogList.GetSelectedCount() != 0)
        {
            popup.AppendMenuIcon(ID_COPYCLIPBOARD, IDS_LOG_POPUP_COPYTOCLIPBOARD, IDI_COPYCLIP);
        }
        popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND, IDI_FILTEREDIT);

        int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, 0);
        DialogEnableWindow(IDOK, FALSE);
        SetPromptApp(&theApp);
        theApp.DoWaitCursor(1);
        bool bOpenWith = false;
        switch (cmd)
        {
        case ID_GNUDIFF1:
            {
                CString options;
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    CDiffOptionsDlg dlg(this);
                    if (dlg.DoModal() == IDOK)
                        options = dlg.GetDiffOptionsString();
                    else
                        break;
                }
                if (PromptShown())
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        SVNDiff diff(this, this->m_hWnd, true);
                        diff.SetHEADPeg(m_LogRevision);
                        diff.ShowUnifiedDiff(m_path, revPrevious, m_path, revSelected, SVNRev(), options);

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                    CAppUtils::StartShowUnifiedDiff(m_hWnd, m_path, revPrevious, m_path, revSelected, SVNRev(), m_LogRevision, options);
            }
            break;
        case ID_GNUDIFF2:
            {
                SVNRev r1 = revSelected;
                SVNRev r2 = revSelected2;
                if (m_LogList.GetSelectedCount() > 2)
                {
                    r1 = revHighest;
                    r2 = revLowest;
                }
                CString options;
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    CDiffOptionsDlg dlg(this);
                    if (dlg.DoModal() == IDOK)
                        options = dlg.GetDiffOptionsString();
                    else
                        break;
                }
                if (PromptShown())
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        SVNDiff diff(this, this->m_hWnd, true);
                        diff.SetHEADPeg(m_LogRevision);
                        diff.ShowUnifiedDiff(m_path, r2, m_path, r1, SVNRev(), options);

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                    CAppUtils::StartShowUnifiedDiff(m_hWnd, m_path, r2, m_path, r1, SVNRev(), m_LogRevision, options);
            }
            break;
        case ID_REVERTREV:
            {
                // we need an URL to complete this command, so error out if we can't get an URL
                if (pathURL.IsEmpty())
                {
                    ReportNoUrlOfFile(m_path.GetUIPathString());
                    break;      //exit
                }

                if (ConfirmRevert(m_path.GetUIPathString()))
                {
                    CSVNProgressDlg dlg;
                    dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
                    dlg.SetOptions(ProgOptIgnoreAncestry);
                    dlg.SetPathList(CTSVNPathList(m_path));
                    dlg.SetUrl(pathURL);
                    dlg.SetSecondUrl(pathURL);
                    revisionRanges.AdjustForMerge(true);
                    dlg.SetRevisionRanges(revisionRanges);
                    dlg.SetPegRevision(m_LogRevision);
                    dlg.DoModal();
                }
            }
            break;
        case ID_MERGEREV:
            {
                // we need an URL to complete this command, so error out if we can't get an URL
                if (pathURL.IsEmpty())
                {
                    ReportNoUrlOfFile(m_path.GetUIPathString());
                    break;      //exit
                }

                CString path = m_path.GetWinPathString();
                bool bGotSavePath = false;
                if ((m_LogList.GetSelectedCount() == 1)&&(!m_path.IsDirectory()))
                {
                    bGotSavePath = CAppUtils::FileOpenSave(path, NULL, IDS_LOG_MERGETO, IDS_COMMONFILEFILTER, true, GetSafeHwnd());
                }
                else
                {
                    CBrowseFolder folderBrowser;
                    folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_MERGETO)));
                    bGotSavePath = (folderBrowser.Show(GetSafeHwnd(), path, path) == CBrowseFolder::OK);
                }
                if (bGotSavePath)
                {
                    svn_revnum_t    minrev;
                    svn_revnum_t    maxrev;
                    bool            bswitched;
                    bool            bmodified;
                    bool            bSparse;

                    if (GetWCRevisionStatus(CTSVNPath(path), true, minrev, maxrev, bswitched, bmodified, bSparse))
                    {
                        if (bmodified)
                        {
                            if (CTaskDialog::IsSupported())
                            {
                                CString sTask1;
                                sTask1.Format(IDS_MERGE_WCDIRTYASK_TASK1, (LPCTSTR)path);
                                CTaskDialog taskdlg(sTask1,
                                                    CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK2)),
                                                    L"TortoiseSVN",
                                                    0,
                                                    TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
                                taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK3)));
                                taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK4)));
                                taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                                taskdlg.SetDefaultCommandControl(2);
                                taskdlg.SetMainIcon(TD_WARNING_ICON);
                                if (taskdlg.DoModal(m_hWnd) != 1)
                                    return;
                            }
                            else
                            {
                                if (TSVNMessageBox(this->m_hWnd, IDS_MERGE_WCDIRTYASK, IDS_APPNAME, MB_YESNO | MB_ICONWARNING) != IDYES)
                                    break;
                            }
                        }
                    }
                    CSVNProgressDlg dlg;
                    dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
                    dlg.SetPathList(CTSVNPathList(CTSVNPath(path)));
                    dlg.SetUrl(pathURL);
                    dlg.SetSecondUrl(pathURL);
                    revisionRanges.AdjustForMerge(false);
                    dlg.SetRevisionRanges(revisionRanges);
                    dlg.SetPegRevision(m_LogRevision);
                    dlg.DoModal();
                }
            }
            break;
        case ID_REVERTTOREV:
            {
                // we need an URL to complete this command, so error out if we can't get an URL
                if (pathURL.IsEmpty())
                {
                    ReportNoUrlOfFile(m_path.GetUIPathString());
                    break;      //exit
                }

                if (ConfirmRevert(m_path.GetWinPath(), true))
                {
                    CSVNProgressDlg dlg;
                    dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
                    dlg.SetOptions(ProgOptIgnoreAncestry);
                    dlg.SetPathList(CTSVNPathList(m_path));
                    dlg.SetUrl(pathURL);
                    dlg.SetSecondUrl(pathURL);
                    SVNRevRangeArray revarray;
                    revarray.AddRevRange(SVNRev::REV_HEAD, revSelected);
                    dlg.SetRevisionRanges(revarray);
                    dlg.SetPegRevision(m_LogRevision);
                    dlg.DoModal();
                }
            }
            break;
        case ID_COPY:
            {
                // we need an URL to complete this command, so error out if we can't get an URL
                if (pathURL.IsEmpty())
                {
                    ReportNoUrlOfFile(m_path.GetUIPathString());
                    break;      //exit
                }

                CCopyDlg dlg;
                dlg.m_URL = pathURL;
                dlg.m_path = m_path;
                dlg.m_CopyRev = revSelected;
                if (dlg.DoModal() == IDOK)
                {
                    CTSVNPath url = CTSVNPath(dlg.m_URL);
                    SVNRev copyrev = dlg.m_CopyRev;
                    CString logmsg = dlg.m_sLogMessage;
                    SVNExternals exts = dlg.GetExternalsToTag();
                    auto f = [=]() mutable
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        // should we show a progress dialog here? Copies are done really fast
                        // and without much network traffic.
                        if (!Copy(CTSVNPathList(CTSVNPath(pathURL)), url, copyrev, copyrev, logmsg))
                            ShowErrorDialog(m_hWnd);
                        else
                        {
                            if (!exts.TagExternals(true, CString(MAKEINTRESOURCE(IDS_COPY_COMMITMSG)), m_commitRev, CTSVNPath(pathURL), url))
                            {
                                ShowErrorDialog(m_hWnd, CTSVNPath(), exts.GetLastErrorString());
                            }
                            else
                                TSVNMessageBox(this->m_hWnd, IDS_LOG_COPY_SUCCESS, IDS_APPNAME, MB_ICONINFORMATION);
                        }

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
            }
            break;
        case ID_COMPARE:
            {
                //user clicked on the menu item "compare with working copy"
                if (PromptShown())
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        SVNDiff diff(this, m_hWnd, true);
                        diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
                        diff.SetHEADPeg(m_LogRevision);
                        diff.ShowCompare(m_path, SVNRev::REV_WC, m_path, revSelected, SVNRev(), L"");

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                    CAppUtils::StartShowCompare(m_hWnd, m_path, SVNRev::REV_WC, m_path, revSelected, SVNRev(), m_LogRevision, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
            }
            break;
        case ID_COMPARETWO:
            {
                SVNRev r1 = revSelected;
                SVNRev r2 = revSelected2;
                if (m_LogList.GetSelectedCount() > 2)
                {
                    r1 = revHighest;
                    r2 = revLowest;
                }
                svn_node_kind_t nodekind = svn_node_unknown;
                if (!m_path.IsUrl())
                {
                    nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
                }
                //user clicked on the menu item "compare revisions"
                if (PromptShown())
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        SVNDiff diff(this, m_hWnd, true);
                        diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
                        diff.SetHEADPeg(m_LogRevision);
                        diff.ShowCompare(CTSVNPath(pathURL), r2, CTSVNPath(pathURL), r1, SVNRev(), L"", false, false, nodekind);

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                    CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), r2, CTSVNPath(pathURL), r1,
                                                SVNRev(), m_LogRevision, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000),
                                                false, false, nodekind);
            }
            break;
        case ID_COMPAREWITHPREVIOUS:
            {
                svn_node_kind_t nodekind = svn_node_unknown;
                if (!m_path.IsUrl())
                {
                    nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
                }
                if (PromptShown())
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        SVNDiff diff(this, m_hWnd, true);
                        diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
                        diff.SetHEADPeg(m_LogRevision);
                        diff.ShowCompare(CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, SVNRev(), L"", false, false, nodekind);

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                    CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected,
                                                SVNRev(), m_LogRevision, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000),
                                                false, false, nodekind);
            }
            break;
        case ID_BLAMECOMPARE:
            {
                //user clicked on the menu item "compare with working copy"
                //now first get the revision which is selected
                if (PromptShown())
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        SVNDiff diff(this, this->m_hWnd, true);
                        diff.SetHEADPeg(m_LogRevision);
                        diff.ShowCompare(m_path, SVNRev::REV_BASE, m_path, revSelected, SVNRev(), false, true);

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                    CAppUtils::StartShowCompare(m_hWnd, m_path, SVNRev::REV_BASE, m_path, revSelected, SVNRev(), m_LogRevision, false, false, true);
            }
            break;
        case ID_BLAMETWO:
            {
                //user clicked on the menu item "compare and blame revisions"
                svn_node_kind_t nodekind = svn_node_unknown;
                if (!m_path.IsUrl())
                {
                    nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
                }
                if (PromptShown())
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        SVNDiff diff(this, this->m_hWnd, true);
                        diff.SetHEADPeg(m_LogRevision);
                        diff.ShowCompare(CTSVNPath(pathURL), revSelected2, CTSVNPath(pathURL), revSelected, SVNRev(), L"", false, true, nodekind);

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                    CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revSelected2, CTSVNPath(pathURL), revSelected,
                                                SVNRev(), m_LogRevision, L"", false, false, true, nodekind);
            }
            break;
        case ID_BLAMEWITHPREVIOUS:
            {
                //user clicked on the menu item "Compare and Blame with previous revision"
                svn_node_kind_t nodekind = svn_node_unknown;
                if (!m_path.IsUrl())
                {
                    nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
                }
                if (PromptShown())
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);

                        SVNDiff diff(this, this->m_hWnd, true);
                        diff.SetHEADPeg(m_LogRevision);
                        diff.ShowCompare(CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected, SVNRev(), L"", false, true, nodekind);
                    };
                    new async::CAsyncCall(f, &netScheduler);
                    netScheduler.WaitForEmptyQueue();
                }
                else
                    CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pathURL), revPrevious, CTSVNPath(pathURL), revSelected,
                                                SVNRev(), m_LogRevision, L"", false, false, true, nodekind);
            }
            break;
        case ID_SAVEAS:
            {
                //now first get the revision which is selected
                CString revFilename;
                if (m_hasWC)
                {
                    CString strWinPath = m_path.GetWinPathString();
                    int rfind = strWinPath.ReverseFind('.');
                    if (rfind > 0)
                        revFilename.Format(_T("%s-%s%s"), (LPCTSTR)strWinPath.Left(rfind), (LPCTSTR)revSelected.ToString(), (LPCTSTR)strWinPath.Mid(rfind));
                    else
                        revFilename.Format(_T("%s-%s"), (LPCTSTR)strWinPath, (LPCTSTR)revSelected.ToString());
                }
                if (CAppUtils::FileOpenSave(revFilename, NULL, IDS_LOG_POPUP_SAVE, IDS_COMMONFILEFILTER, false, m_hWnd))
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        CTSVNPath tempfile;
                        tempfile.SetFromWin(revFilename);
                        CProgressDlg progDlg;
                        progDlg.SetTitle(IDS_APPNAME);
                        progDlg.SetAnimation(IDR_DOWNLOAD);
                        CString sInfoLine;
                        sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LPCTSTR)revSelected.ToString());
                        progDlg.SetLine(1, sInfoLine, true);
                        SetAndClearProgressInfo(&progDlg);
                        progDlg.ShowModeless(m_hWnd);
                        if (!Export(m_path, tempfile, SVNRev(SVNRev::REV_HEAD), revSelected))
                        {
                            // try again with another peg revision
                            if (!Export(m_path, tempfile, revSelected, revSelected))
                            {
                                progDlg.Stop();
                                SetAndClearProgressInfo((HWND)NULL);
                                ShowErrorDialog(m_hWnd);
                                EnableOKButton();
                            }
                        }
                        progDlg.Stop();
                        SetAndClearProgressInfo((HWND)NULL);

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
            }
            break;
        case ID_OPENWITH:
            bOpenWith = true;
            // fallthrough
        case ID_OPEN:
            {
                auto f = [=]()
                {
                    CoInitialize(NULL);
                    this->EnableWindow(FALSE);

                    CProgressDlg progDlg;
                    progDlg.SetTitle(IDS_APPNAME);
                    progDlg.SetAnimation(IDR_DOWNLOAD);
                    CString sInfoLine;
                    sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LPCTSTR)revSelected.ToString());
                    progDlg.SetLine(1, sInfoLine, true);
                    SetAndClearProgressInfo(&progDlg);
                    progDlg.ShowModeless(m_hWnd);
                    CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, m_path, revSelected);
                    bool bSuccess = true;
                    if (!Export(m_path, tempfile, SVNRev(SVNRev::REV_HEAD), revSelected))
                    {
                        bSuccess = false;
                        // try again, but with the selected revision as the peg revision
                        if (!Export(m_path, tempfile, revSelected, revSelected))
                        {
                            progDlg.Stop();
                            SetAndClearProgressInfo((HWND)NULL);
                            ShowErrorDialog(m_hWnd);
                            EnableOKButton();
                        }
                        else
                            bSuccess = true;
                    }
                    if (bSuccess)
                    {
                        progDlg.Stop();
                        SetAndClearProgressInfo((HWND)NULL);
                        DoOpenFileWith(bOpenWith, tempfile);
                    }

                    this->EnableWindow(TRUE);
                    this->SetFocus();
                };
                new async::CAsyncCall(f, &netScheduler);
            }
            break;
        case ID_BLAME:
            {
                CBlameDlg dlg;
                dlg.EndRev = revSelected;
                dlg.PegRev = m_pegrev;
                if (dlg.DoModal() == IDOK)
                {
                    SVNRev startrev = dlg.StartRev;
                    SVNRev endrev = dlg.EndRev;
                    bool includeMerge = !!dlg.m_bIncludeMerge;
                    bool textViewer = !!dlg.m_bTextView;
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);

                        CBlame blame;
                        CString tempfile;
                        tempfile = blame.BlameToTempFile(m_path, startrev, endrev, m_pegrev, _T(""), includeMerge, TRUE, TRUE);
                        if (!tempfile.IsEmpty())
                        {
                            if (textViewer)
                            {
                                //open the default text editor for the result file
                                CAppUtils::StartTextViewer(tempfile);
                            }
                            else
                            {
                                CString sParams = _T("/path:\"") + m_path.GetSVNPathString() + _T("\" ");
                                CAppUtils::LaunchTortoiseBlame(tempfile,
                                                               CPathUtils::GetFileNameFromPath(m_path.GetFileOrDirectoryName()),
                                                               sParams,
                                                               startrev,
                                                               endrev,
                                                               m_pegrev);
                            }
                        }
                        else
                        {
                            blame.ShowErrorDialog(m_hWnd);
                        }

                        this->EnableWindow(TRUE);
                        this->SetFocus();
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
            }
            break;
        case ID_UPDATE:
            {
                CString sCmd;
                sCmd.Format(_T("/command:update /path:\"%s\" /rev:%ld"),
                    (LPCTSTR)m_path.GetWinPath(), (LONG)revSelected);
                CAppUtils::RunTortoiseProc(sCmd);
            }
            break;
        case ID_FINDENTRY:
            {
                m_nSearchIndex = m_LogList.GetSelectionMark();
                if (m_nSearchIndex < 0)
                    m_nSearchIndex = 0;
                CreateFindDialog();
            }
            break;
        case ID_REPOBROWSE:
            {
                CString sCmd;
                sCmd.Format(_T("/command:repobrowser /path:\"%s\" /rev:%s"),
                    (LPCTSTR)pathURL, (LPCTSTR)revSelected.ToString());

                CAppUtils::RunTortoiseProc(sCmd);
            }
            break;
        case ID_EDITLOG:
            {
                EditLogMessage(selIndex);
            }
            break;
        case ID_EDITAUTHOR:
            {
                EditAuthor(selEntries);
            }
            break;
        case ID_REVPROPS:
            {
                CEditPropertiesDlg dlg;
                dlg.SetProjectProperties(&m_ProjectProperties);
                dlg.SetPathList(CTSVNPathList(CTSVNPath(pathURL)));
                dlg.SetRevision(revSelected);
                dlg.RevProps(true);
                dlg.DoModal();
            }
            break;
        case ID_COPYCLIPBOARD:
            CopySelectionToClipBoard();
            break;
        case ID_EXPORT:
            {
                CString sCmd;
                sCmd.Format(_T("/command:export /path:\"%s\" /revision:%ld"),
                    (LPCTSTR)pathURL, (LONG)revSelected);
                CAppUtils::RunTortoiseProc(sCmd);
            }
            break;
        case ID_CHECKOUT:
            {
                CString sCmd;
                CString url = _T("tsvn:")+pathURL;
                sCmd.Format(_T("/command:checkout /url:\"%s\" /revision:%ld"),
                    (LPCTSTR)url, (LONG)revSelected);
                CAppUtils::RunTortoiseProc(sCmd);
            }
            break;
        case ID_VIEWREV:
            {
                CString url = m_ProjectProperties.sWebViewerRev;
                url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
                url.Replace(_T("%REVISION%"), revSelected.ToString());
                if (!url.IsEmpty())
                    ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
            }
            break;
        case ID_VIEWPATHREV:
            {
                CString relurl = pathURL;
                CString sRoot = GetRepositoryRoot(CTSVNPath(relurl));
                relurl = relurl.Mid(sRoot.GetLength());
                CString url = m_ProjectProperties.sWebViewerPathRev;
                url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
                url.Replace(_T("%REVISION%"), revSelected.ToString());
                url.Replace(_T("%PATH%"), relurl);
                if (!url.IsEmpty())
                    ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
            }
            break;
        default:
            break;
        } // switch (cmd)
        theApp.DoWaitCursor(-1);
        EnableOKButton();
    } // if (popup.CreatePopupMenu())
}

void CLogDlg::ShowContextMenuForChangedpaths(CWnd* /*pWnd*/, CPoint point)
{
    INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        m_ChangedFileListCtrl.GetItemRect((int)selIndex, &rect, LVIR_LABEL);
        m_ChangedFileListCtrl.ClientToScreen(&rect);
        point = rect.CenterPoint();
    }
    if (selIndex < 0)
        return;
    int s = m_LogList.GetSelectionMark();
    if (s < 0)
        return;
    std::vector<CString> changedpaths;
    std::vector<size_t> changedlogpathindices;
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    if (pos == NULL)
        return; // nothing is selected, get out of here

    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
    svn_revnum_t rev1 = pLogEntry->GetRevision();
    svn_revnum_t rev2 = rev1;
    bool bOneRev = true;
    if (pos)
    {
        while (pos)
        {
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index < (int)m_logEntries.GetVisibleCount())
            {
                pLogEntry = m_logEntries.GetVisible (index);
                if (pLogEntry)
                {
                    rev1 = max(rev1,(svn_revnum_t)pLogEntry->GetRevision());
                    rev2 = min(rev2,(svn_revnum_t)pLogEntry->GetRevision());
                    bOneRev = false;
                }
            }
        }
        if (!bOneRev)
            rev2--;
        POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
        while (pos2)
        {
            int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
            changedpaths.push_back(m_currentChangedPathList[nItem].GetSVNPathString());
            changedlogpathindices.push_back (static_cast<size_t>(nItem));
        }
    }
    else
    {
        // only one revision is selected in the log dialog top pane
        // but multiple items could be selected  in the changed items list
        rev2 = rev1-1;

        POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
        while (pos2)
        {
            const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();

            int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
            changedlogpathindices.push_back (static_cast<size_t>(nItem));
            if ((m_cShowPaths.GetState() & 0x0003)==BST_CHECKED)
            {
                // some items are hidden! So find out which item the user really clicked on
                INT_PTR selRealIndex = -1;
                for (INT_PTR hiddenindex=0; hiddenindex<(INT_PTR)paths.GetCount(); ++hiddenindex)
                {
                    if (paths[hiddenindex].IsRelevantForStartPath())
                        selRealIndex++;
                    if (selRealIndex == nItem)
                    {
                        nItem = static_cast<int>(hiddenindex);
                        break;
                    }
                }
            }

            const CLogChangedPath& changedlogpath = paths[nItem];
            if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
            {
                if (!changedlogpath.GetCopyFromPath().IsEmpty())
                    rev2 = changedlogpath.GetCopyFromRev();
                else
                {
                    // if the path was modified but the parent path was 'added with history'
                    // then we have to use the copy from revision of the parent path
                    CTSVNPath cpath = CTSVNPath(changedlogpath.GetPath());
                    for (size_t flist = 0; flist < paths.GetCount(); ++flist)
                    {
                        CTSVNPath p = CTSVNPath(paths[flist].GetPath());
                        if (p.IsAncestorOf(cpath))
                        {
                            if (!paths[flist].GetCopyFromPath().IsEmpty())
                                rev2 = paths[flist].GetCopyFromRev();
                        }
                    }
                }
            }

            changedpaths.push_back(changedlogpath.GetPath());
        }
    }

    //entry is selected, now show the popup menu
    CIconMenu popup;
    if (popup.CreatePopupMenu())
    {
        bool bEntryAdded = false;
        if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
        {
            if ((!bOneRev)||(IsDiffPossible (m_currentChangedArray[selIndex], rev1)))
            {
                popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
                popup.AppendMenuIcon(ID_BLAMEDIFF, IDS_LOG_POPUP_BLAMEDIFF, IDI_BLAME);
                popup.SetDefaultItem(ID_DIFF, FALSE);
                popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
                bEntryAdded = true;
            }
            else if (bOneRev)
            {
                popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
                popup.SetDefaultItem(ID_DIFF, FALSE);
                bEntryAdded = true;
            }
            if ((rev2 == rev1-1)||(changedpaths.size() == 1))
            {
                if (bEntryAdded)
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
                popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
                popup.AppendMenuIcon(ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);
                popup.AppendMenu(MF_SEPARATOR, NULL);
                if ((m_hasWC)&&(bOneRev))
                    popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
                popup.AppendMenuIcon(ID_POPPROPS, IDS_REPOBROWSE_SHOWPROP, IDI_PROPERTIES);         // "Show Properties"
                popup.AppendMenuIcon(ID_LOG, IDS_MENULOG, IDI_LOG);                     // "Show Log"
                popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
                popup.AppendMenuIcon(ID_GETMERGELOGS, IDS_LOG_POPUP_GETMERGELOGS, IDI_LOG);     // "Show merge log"
                popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE, IDI_SAVEAS);
                bEntryAdded = true;
                if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
                {
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                    popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);
                }
                if (popup.GetDefaultItem(0,FALSE)==-1)
                    popup.SetDefaultItem(ID_OPEN, FALSE);
            }
        }
        else if (!changedlogpathindices.empty())
        {
            // more than one entry is selected
            popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE);
            bEntryAdded = true;
        }
        if (!changedpaths.empty())
        {
            popup.AppendMenuIcon(ID_EXPORTTREE, IDS_MENUEXPORT, IDI_EXPORT);
            bEntryAdded = true;
        }

        if (!bEntryAdded)
            return;
        int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, 0);
        bool bOpenWith = false;
        bool bMergeLog = false;
        m_bCancelled = false;
        switch (cmd)
        {
        case ID_DIFF:
            {
                if ((!bOneRev)|| IsDiffPossible (m_currentChangedArray[selIndex], rev1))
                {
                    auto f = [=](){CoInitialize(NULL); this->EnableWindow(FALSE); DoDiffFromLog(selIndex, rev1, rev2, false, false); this->EnableWindow(TRUE);this->SetFocus();};
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                {
                    auto f = [=](){CoInitialize(NULL); this->EnableWindow(FALSE); DiffSelectedFile(); this->EnableWindow(TRUE);this->SetFocus();};
                    new async::CAsyncCall(f, &netScheduler);
                }
            }
            break;
        case ID_BLAMEDIFF:
            {
                auto f = [=](){CoInitialize(NULL); this->EnableWindow(FALSE); DoDiffFromLog(selIndex, rev1, rev2, true, false); this->EnableWindow(TRUE);this->SetFocus();};
                new async::CAsyncCall(f, &netScheduler);
            }
            break;
        case ID_GNUDIFF1:
            {
                auto f = [=](){CoInitialize(NULL); this->EnableWindow(FALSE); DoDiffFromLog(selIndex, rev1, rev2, false, true); this->EnableWindow(TRUE);this->SetFocus();};
                new async::CAsyncCall(f, &netScheduler);
            }
            break;
        case ID_REVERTREV:
            {
                const CLogChangedPath& changedlogpath
                    = m_currentChangedArray[selIndex];

                SetPromptApp(&theApp);
                theApp.DoWaitCursor(1);
                CString sUrl;
                if (SVN::PathIsURL(m_path))
                {
                    sUrl = m_path.GetSVNPathString();
                }
                else
                {
                    sUrl = GetURLFromPath(m_path);
                    if (sUrl.IsEmpty())
                    {
                        theApp.DoWaitCursor(-1);
                        ReportNoUrlOfFile(m_path.GetWinPath());
                        EnableOKButton();
                        theApp.DoWaitCursor(-1);
                        break;      //exit
                    }
                }
                // find the working copy path of the selected item from the URL
                m_bCancelled = false;
                CString sUrlRoot = GetRepositoryRoot(CTSVNPath(sUrl));

                CString fileURL = changedlogpath.GetPath();
                fileURL = sUrlRoot + fileURL.Trim();
                // firstfile = (e.g.) http://mydomain.com/repos/trunk/folder/file1
                // sUrl = http://mydomain.com/repos/trunk/folder
                CString sUnescapedUrl = CPathUtils::PathUnescape(sUrl);
                // find out until which char the urls are identical
                int i=0;
                while ((i<fileURL.GetLength())&&(i<sUnescapedUrl.GetLength())&&(fileURL[i]==sUnescapedUrl[i]))
                    i++;
                int leftcount = m_path.GetWinPathString().GetLength()-(sUnescapedUrl.GetLength()-i);
                CString wcPath = m_path.GetWinPathString().Left(leftcount);
                wcPath += fileURL.Mid(i);
                wcPath.Replace('/', '\\');
                CSVNProgressDlg dlg;
                if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
                {
                    // a deleted path! Since the path isn't there anymore, merge
                    // won't work. So just do a copy url->wc
                    dlg.SetCommand(CSVNProgressDlg::SVNProgress_Copy);
                    dlg.SetPathList(CTSVNPathList(CTSVNPath(fileURL)));
                    dlg.SetUrl(wcPath);
                    dlg.SetRevision(rev2);
                }
                else
                {
                    if (!PathFileExists(wcPath))
                    {
                        // seems the path got renamed
                        // tell the user how to work around this.
                        TSVNMessageBox(this->m_hWnd, IDS_LOG_REVERTREV_ERROR, IDS_APPNAME, MB_ICONERROR);
                        EnableOKButton();
                        theApp.DoWaitCursor(-1);
                        break;      //exit
                    }
                    dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
                    dlg.SetOptions(ProgOptIgnoreAncestry);
                    dlg.SetPathList(CTSVNPathList(CTSVNPath(wcPath)));
                    dlg.SetUrl(fileURL);
                    dlg.SetSecondUrl(fileURL);
                    SVNRevRangeArray revarray;
                    revarray.AddRevRange(rev1, rev2);
                    dlg.SetRevisionRanges(revarray);
                }
                if (ConfirmRevert(wcPath))
                {
                    dlg.DoModal();
                }
                theApp.DoWaitCursor(-1);
            }
            break;
        case ID_POPPROPS:
            {
                DialogEnableWindow(IDOK, FALSE);
                SetPromptApp(&theApp);
                theApp.DoWaitCursor(1);
                CString filepath;
                if (SVN::PathIsURL(m_path))
                {
                    filepath = m_path.GetSVNPathString();
                }
                else
                {
                    filepath = GetURLFromPath(m_path);
                    if (filepath.IsEmpty())
                    {
                        theApp.DoWaitCursor(-1);
                        ReportNoUrlOfFile(filepath);
                        EnableOKButton();
                        break;
                    }
                }
                filepath = GetRepositoryRoot(CTSVNPath(filepath));
                filepath += m_currentChangedArray[selIndex].GetPath();
                CPropDlg dlg;
                dlg.m_rev = rev1;
                dlg.m_Path = CTSVNPath(filepath);
                dlg.DoModal();
                EnableOKButton();
                theApp.DoWaitCursor(-1);
            }
            break;
        case ID_SAVEAS:
            {
                DialogEnableWindow(IDOK, FALSE);
                SetPromptApp(&theApp);
                theApp.DoWaitCursor(1);
                CString filepath;
                if (SVN::PathIsURL(m_path))
                {
                    filepath = m_path.GetSVNPathString();
                }
                else
                {
                    filepath = GetURLFromPath(m_path);
                    if (filepath.IsEmpty())
                    {
                        theApp.DoWaitCursor(-1);
                        ReportNoUrlOfFile(filepath);
                        EnableOKButton();
                        break;
                    }
                }
                m_bCancelled = false;
                CString sRoot = GetRepositoryRoot(CTSVNPath(filepath));
                // if more than one entry is selected, we save them
                // one by one into a folder the user has selected
                bool bTargetSelected = false;
                CTSVNPath TargetPath;
                if (m_ChangedFileListCtrl.GetSelectedCount() > 1)
                {
                    CBrowseFolder browseFolder;
                    browseFolder.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_SAVEFOLDERTOHINT)));
                    browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
                    CString strSaveAsDirectory;
                    if (browseFolder.Show(GetSafeHwnd(), strSaveAsDirectory) == CBrowseFolder::OK)
                    {
                        TargetPath = CTSVNPath(strSaveAsDirectory);
                        bTargetSelected = true;
                    }
                }
                else
                {
                    // Display the Open dialog box.
                    CString revFilename;
                    CString temp;
                    temp = CPathUtils::GetFileNameFromPath(m_currentChangedArray[selIndex].GetPath());
                    int rfind = temp.ReverseFind('.');
                    if (rfind > 0)
                        revFilename.Format(_T("%s-%ld%s"), (LPCTSTR)temp.Left(rfind), rev1, (LPCTSTR)temp.Mid(rfind));
                    else
                        revFilename.Format(_T("%s-%ld"), (LPCTSTR)temp, rev1);
                    bTargetSelected = CAppUtils::FileOpenSave(revFilename, NULL, IDS_LOG_POPUP_SAVE, IDS_COMMONFILEFILTER, false, m_hWnd);
                    TargetPath.SetFromWin(revFilename);
                }
                if (bTargetSelected)
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);
                        CProgressDlg progDlg;
                        progDlg.SetTitle(IDS_APPNAME);
                        progDlg.SetAnimation(IDR_DOWNLOAD);
                        for ( size_t i = 0; i < changedlogpathindices.size(); ++i)
                        {
                            const CLogChangedPath& changedlogpath
                                = m_currentChangedArray[changedlogpathindices[i]];

                            SVNRev getrev = (changedlogpath.GetAction() == LOGACTIONS_DELETED) ? rev2 : rev1;

                            CString sInfoLine;
                            sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)CPathUtils::GetFileNameFromPath(changedlogpath.GetPath()), (LPCTSTR)getrev.ToString());
                            progDlg.SetLine(1, sInfoLine, true);
                            SetAndClearProgressInfo(&progDlg);
                            progDlg.ShowModeless(m_hWnd);

                            CTSVNPath tempfile = TargetPath;
                            if (changedpaths.size() > 1)
                            {
                                // if multiple items are selected, then the TargetPath
                                // points to a folder and we have to append the filename
                                // to save to that folder.
                                CString sName = changedlogpath.GetPath();
                                int slashpos = sName.ReverseFind('/');
                                if (slashpos >= 0)
                                    sName = sName.Mid(slashpos);
                                tempfile.AppendPathString(sName);
                            }
                            CString filepath = sRoot + changedlogpath.GetPath();
                            progDlg.SetLine(2, filepath, true);
                            if (!Export(CTSVNPath(filepath), tempfile, getrev, getrev))
                            {
                                progDlg.Stop();
                                SetAndClearProgressInfo((HWND)NULL);
                                ShowErrorDialog(m_hWnd);
                                tempfile.Delete(false);
                                EnableOKButton();
                                theApp.DoWaitCursor(-1);
                                break;
                            }
                            progDlg.SetProgress((DWORD)i+1, (DWORD)changedlogpathindices.size());
                        }
                        progDlg.Stop();
                        SetAndClearProgressInfo((HWND)NULL);
                        this->EnableWindow(TRUE);
                        this->SetFocus();
                        EnableOKButton();
                        theApp.DoWaitCursor(-1);
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                {
                    EnableOKButton();
                    theApp.DoWaitCursor(-1);
                }
            }
            break;
        case ID_EXPORTTREE:
            {
                DialogEnableWindow(IDOK, FALSE);
                SetPromptApp(&theApp);
                theApp.DoWaitCursor(1);
                m_bCancelled = false;

                bool bTargetSelected = false;
                CTSVNPath TargetPath;
                if (m_ChangedFileListCtrl.GetSelectedCount() > 0)
                {
                    CBrowseFolder browseFolder;
                    browseFolder.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_SAVEFOLDERTOHINT)));
                    browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
                    CString strSaveAsDirectory;
                    if (browseFolder.Show(GetSafeHwnd(), strSaveAsDirectory) == CBrowseFolder::OK)
                    {
                        TargetPath = CTSVNPath(strSaveAsDirectory);
                        bTargetSelected = true;
                    }
                }
                if (bTargetSelected)
                {
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);
                        CProgressDlg progDlg;
                        progDlg.SetTitle(IDS_APPNAME);
                        progDlg.SetAnimation(IDR_DOWNLOAD);
                        progDlg.SetTime(true);
                        for ( size_t i = 0; i < changedlogpathindices.size(); ++i)
                        {
                            const CString& changedlogpath
                                = m_currentChangedArray[changedlogpathindices[i]].GetPath();

                            SVNRev getrev = rev1;

                            CTSVNPath tempfile = TargetPath;
                            tempfile.AppendPathString(changedlogpath);

                            CString sInfoLine;
                            sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)changedlogpath, (LPCTSTR)getrev.ToString());
                            progDlg.SetLine(1, sInfoLine, true);
                            progDlg.SetLine(2, tempfile.GetWinPath(), true);
                            progDlg.SetProgress64(i, changedlogpathindices.size());
                            progDlg.ShowModeless(m_hWnd);

                            SHCreateDirectoryEx(m_hWnd, tempfile.GetContainingDirectory().GetWinPath(), NULL);
                            CString filepath = m_sRepositoryRoot + changedlogpath;
                            if (!Export(CTSVNPath(filepath), tempfile, getrev, getrev, true, true, svn_depth_empty))
                            {
                                progDlg.Stop();
                                SetAndClearProgressInfo((HWND)NULL);
                                ShowErrorDialog(m_hWnd);
                                tempfile.Delete(false);
                                EnableOKButton();
                                theApp.DoWaitCursor(-1);
                                break;
                            }
                        }
                        progDlg.Stop();
                        SetAndClearProgressInfo((HWND)NULL);
                        this->EnableWindow(TRUE);
                        this->SetFocus();
                        EnableOKButton();
                        theApp.DoWaitCursor(-1);
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
                else
                {
                    EnableOKButton();
                    theApp.DoWaitCursor(-1);
                }
            }
            break;
        case ID_OPENWITH:
            bOpenWith = true;
            // fallthrough
        case ID_OPEN:
            {
                SVNRev getrev = m_currentChangedArray[selIndex].GetAction() == LOGACTIONS_DELETED ? rev2 : rev1;
                auto f = [=](){CoInitialize(NULL); this->EnableWindow(FALSE); Open(bOpenWith,m_currentChangedArray[selIndex].GetPath(),getrev); this->EnableWindow(TRUE);this->SetFocus();};
                new async::CAsyncCall(f, &netScheduler);
            }
            break;
        case ID_BLAME:
            {
                CString filepath;
                if (SVN::PathIsURL(m_path))
                {
                    filepath = m_path.GetSVNPathString();
                }
                else
                {
                    filepath = GetURLFromPath(m_path);
                    if (filepath.IsEmpty())
                    {
                        theApp.DoWaitCursor(-1);
                        ReportNoUrlOfFile(filepath);
                        EnableOKButton();
                        break;
                    }
                }
                filepath = GetRepositoryRoot(CTSVNPath(filepath));
                filepath += m_currentChangedArray[selIndex].GetPath();
                const CLogChangedPath& changedlogpath
                    = m_currentChangedArray[selIndex];
                CBlameDlg dlg;
                if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
                    rev1--;
                dlg.EndRev = rev1;
                if (dlg.DoModal() == IDOK)
                {
                    SVNRev startrev = dlg.StartRev;
                    SVNRev endrev = dlg.EndRev;
                    SVNRev pegrev = rev1;
                    bool includeMerge = !!dlg.m_bIncludeMerge;
                    bool textView = !!dlg.m_bTextView;
                    auto f = [=]()
                    {
                        CoInitialize(NULL);
                        this->EnableWindow(FALSE);
                        CBlame blame;
                        CString tempfile;
                        tempfile = blame.BlameToTempFile(CTSVNPath(filepath), startrev, endrev, pegrev, _T(""), includeMerge, TRUE, TRUE);
                        if (!tempfile.IsEmpty())
                        {
                            if (textView)
                            {
                                //open the default text editor for the result file
                                CAppUtils::StartTextViewer(tempfile);
                            }
                            else
                            {
                                CString sParams = _T("/path:\"") + filepath + _T("\" ");
                                CAppUtils::LaunchTortoiseBlame(tempfile,
                                                               CPathUtils::GetFileNameFromPath(filepath),
                                                               sParams,
                                                               startrev,
                                                               endrev,
                                                               pegrev);
                            }
                        }
                        else
                        {
                            blame.ShowErrorDialog(m_hWnd);
                        }
                        this->EnableWindow(TRUE);
                        this->SetFocus();
                        EnableOKButton();
                        theApp.DoWaitCursor(-1);
                    };
                    new async::CAsyncCall(f, &netScheduler);
                }
            }
            break;
        case ID_GETMERGELOGS:
            bMergeLog = true;
            // fall through
        case ID_LOG:
            {
                const CLogChangedPath& changedlogpath
                    = m_currentChangedArray[selIndex];

                DialogEnableWindow(IDOK, FALSE);
                SetPromptApp(&theApp);
                theApp.DoWaitCursor(1);
                CString filepath;
                if (SVN::PathIsURL(m_path))
                {
                    filepath = m_path.GetSVNPathString();
                }
                else
                {
                    filepath = GetURLFromPath(m_path);
                    if (filepath.IsEmpty())
                    {
                        theApp.DoWaitCursor(-1);
                        ReportNoUrlOfFile(filepath);
                        EnableOKButton();
                        break;
                    }
                }
                m_bCancelled = false;
                filepath = GetRepositoryRoot(CTSVNPath(filepath));
                filepath += m_currentChangedArray[selIndex].GetPath();
                svn_revnum_t logrev = rev1;
                CString sCmd;
                if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
                {
                    // if the item got deleted in this revision,
                    // fetch the log from the previous revision where it
                    // still existed.
                    sCmd.Format(_T("/command:log /path:\"%s\" /startrev:%ld /pegrev:%ld"),
                        (LPCTSTR)filepath, logrev-1, logrev-1);
                }
                else
                {
                    sCmd.Format(_T("/command:log /path:\"%s\" /pegrev:%ld"),
                        (LPCTSTR)filepath, logrev);
                }

                if (bMergeLog)
                    sCmd += _T(" /merge");
                CAppUtils::RunTortoiseProc(sCmd);
                EnableOKButton();
                theApp.DoWaitCursor(-1);
            }
            break;
        case ID_REPOBROWSE:
            {
                const CLogChangedPath& changedlogpath
                    = m_currentChangedArray[selIndex];

                DialogEnableWindow(IDOK, FALSE);
                SetPromptApp(&theApp);
                theApp.DoWaitCursor(1);
                CString filepath;
                if (SVN::PathIsURL(m_path))
                {
                    filepath = m_path.GetSVNPathString();
                }
                else
                {
                    filepath = GetURLFromPath(m_path);
                    if (filepath.IsEmpty())
                    {
                        theApp.DoWaitCursor(-1);
                        ReportNoUrlOfFile(filepath);
                        EnableOKButton();
                        break;
                    }
                }
                m_bCancelled = false;
                filepath = GetRepositoryRoot(CTSVNPath(filepath));
                filepath += m_currentChangedArray[selIndex].GetPath();
                svn_revnum_t logrev = rev1;
                CString sCmd;
                if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
                {
                    sCmd.Format(_T("/command:repobrowser /path:\"%s\" /rev:%ld"),
                                (LPCTSTR)filepath, logrev-1);
                }
                else
                {
                    sCmd.Format(_T("/command:repobrowser /path:\"%s\" /rev:%ld"),
                                (LPCTSTR)filepath, logrev);
                }

                CAppUtils::RunTortoiseProc(sCmd);
                EnableOKButton();
                theApp.DoWaitCursor(-1);
            }
            break;
        case ID_VIEWPATHREV:
            {
                PLOGENTRYDATA pLogEntry2 = m_logEntries.GetVisible (m_LogList.GetSelectionMark());
                SVNRev rev = pLogEntry2->GetRevision();
                CString relurl = m_currentChangedArray[selIndex].GetPath();
                CString url = m_ProjectProperties.sWebViewerPathRev;
                url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
                url.Replace(_T("%REVISION%"), rev.ToString());
                url.Replace(_T("%PATH%"), relurl);
                relurl = relurl.Mid(relurl.Find('/'));
                url.Replace(_T("%PATH1%"), relurl);
                if (!url.IsEmpty())
                    ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
            }
            break;
        default:
            break;
        } // switch (cmd)
    } // if (popup.CreatePopupMenu())
}

void CLogDlg::OnDtnDropdownDatefrom(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    // the date control should not show the "today" button
    CMonthCalCtrl * pCtrl = m_DateFrom.GetMonthCalCtrl();
    if (pCtrl)
        SetWindowLongPtr(pCtrl->GetSafeHwnd(), GWL_STYLE, LONG_PTR(pCtrl->GetStyle() | MCS_NOTODAY));
    *pResult = 0;
}

void CLogDlg::OnDtnDropdownDateto(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    // the date control should not show the "today" button
    CMonthCalCtrl * pCtrl = m_DateTo.GetMonthCalCtrl();
    if (pCtrl)
        SetWindowLongPtr(pCtrl->GetSafeHwnd(), GWL_STYLE, LONG_PTR(pCtrl->GetStyle() | MCS_NOTODAY));
    *pResult = 0;
}

void CLogDlg::OnSize(UINT nType, int cx, int cy)
{
    __super::OnSize(nType, cx, cy);
    //set range
    SetSplitterRange();
}

void CLogDlg::OnRefresh()
{
    if (GetDlgItem(IDC_GETALL)->IsWindowEnabled())
    {
        m_limit = 0;
        Refresh (true);
    }
}

void CLogDlg::OnFind()
{
    CreateFindDialog();
}

void CLogDlg::OnFocusFilter()
{
    GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
}

void CLogDlg::OnEditCopy()
{
    if (GetFocus() == &m_ChangedFileListCtrl)
        CopyChangedSelectionToClipBoard();
    else
        CopySelectionToClipBoard();
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
    // If user press space, toggle flag on selected item
    if (pLVKeyDown->wVKey == VK_SPACE)
    {
        // Toggle checked for the focused item.
        int nFocusedItem = m_LogList.GetNextItem(-1, LVNI_FOCUSED);
        if (nFocusedItem >= 0)
            ToggleCheckbox(nFocusedItem);
    }
    else if ((pLVKeyDown->wVKey == 'A') && (GetKeyState (VK_CONTROL) < 0))
    {
        // Ctrl-A: select all visible revision
        SelectAllVisibleRevisions();
    }

    *pResult = 0;
}

void CLogDlg::OnLvnKeydownFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

    // Ctrl-A: select all files
    if ((pLVKeyDown->wVKey == 'A') && (GetKeyState (VK_CONTROL) < 0))
        m_ChangedFileListCtrl.SetItemState (-1, LVIS_SELECTED, LVIS_SELECTED);

    *pResult = 0;
}

void CLogDlg::OnNMClickLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

    UINT flags = 0;
    CPoint point(pNMItemActivate->ptAction);

    //Make the hit test...
    int item = m_LogList.HitTest(point, &flags);

    if (item != -1)
    {
        //We hit one item... did we hit state image (check box)?
        //This test only works if we are in list or report mode.
        if( (flags & LVHT_ONITEMSTATEICON) != 0)
        {
            ToggleCheckbox(item);
        }
    }

    *pResult = 0;
}

CString CLogDlg::GetToolTipText(int nItem, int nSubItem)
{
    if ((nSubItem == 1) && (m_logEntries.GetVisibleCount() > (size_t)nItem))
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (nItem);

        CString sToolTipText;

        // Draw the icon(s) into the compatible DC

        DWORD actions = pLogEntry->GetChangedPaths().GetActions();

        std::string actionText;
        if (actions & LOGACTIONS_MODIFIED)
        {
            actionText += CLogChangedPath::GetActionString (LOGACTIONS_MODIFIED);
        }

        if (actions & LOGACTIONS_ADDED)
        {
            if (!actionText.empty())
                actionText += "\r\n";
            actionText += CLogChangedPath::GetActionString (LOGACTIONS_ADDED);
        }

        if (actions & LOGACTIONS_DELETED)
        {
            if (!actionText.empty())
                actionText += "\r\n";
            actionText += CLogChangedPath::GetActionString (LOGACTIONS_DELETED);
        }

        if (actions & LOGACTIONS_REPLACED)
        {
            if (!actionText.empty())
                actionText += "\r\n";
            actionText += CLogChangedPath::GetActionString (LOGACTIONS_REPLACED);
        }

        sToolTipText = CUnicodeUtils::GetUnicode (actionText.c_str());
        if ((pLogEntry->GetDepth())||(m_mergedRevs.find(pLogEntry->GetRevision()) != m_mergedRevs.end()))
        {
            if (!sToolTipText.IsEmpty())
                sToolTipText += _T("\r\n");
            if (pLogEntry->IsSubtractiveMerge())
                sToolTipText += CString(MAKEINTRESOURCE(IDS_LOG_ALREADYMERGEDREVERSED));
            else
                sToolTipText += CString(MAKEINTRESOURCE(IDS_LOG_ALREADYMERGED));
        }

        if (!sToolTipText.IsEmpty())
        {
            CString sTitle(MAKEINTRESOURCE(IDS_LOG_ACTIONS));
            sToolTipText = sTitle + _T(":\r\n") + sToolTipText;
        }
        return sToolTipText;
    }
    return CString();
}

// selection management

void CLogDlg::AutoStoreSelection()
{
    if (m_pStoreSelection == NULL)
        m_pStoreSelection = new CStoreSelection(this);
}

void CLogDlg::AutoRestoreSelection()
{
    if (m_pStoreSelection != NULL)
    {
        delete m_pStoreSelection;
        m_pStoreSelection = NULL;

        FillLogMessageCtrl();
        UpdateLogInfoLabel();
    }
}

CString CLogDlg::GetListviewHelpString(HWND hControl, int index)
{
    CString sHelpText;
    if (hControl == m_LogList.GetSafeHwnd())
    {
        if (m_logEntries.GetVisibleCount() > (size_t)index)
        {
            PLOGENTRYDATA data = m_logEntries.GetVisible(index);
            if (data)
            {
                if ((data->GetDepth())||(m_mergedRevs.find(data->GetRevision()) == m_mergedRevs.end()))
                {
                    // this revision was already merged or is not mergeable
                    sHelpText = CString(MAKEINTRESOURCE(IDS_ACC_LOGENTRYALREADYMERGED));
                }
                if (data->GetRevision() == m_wcRev)
                {
                    // the working copy is at this revision
                    if (!sHelpText.IsEmpty())
                        sHelpText += _T(", ");
                    sHelpText += CString(MAKEINTRESOURCE(IDS_ACC_WCISATTHISREVISION));
                }
            }
        }
    }
    else if (hControl == m_ChangedFileListCtrl.GetSafeHwnd())
    {
        if ((int)m_currentChangedArray.GetCount() > index)
        {
            svn_tristate_t textModifies = m_currentChangedArray[index].GetTextModifies();
            svn_tristate_t propsModifies = m_currentChangedArray[index].GetPropsModifies();
            if ((propsModifies == svn_tristate_true)&&(textModifies != svn_tristate_true))
            {
                // property only modification, content of entry hasn't changed
                sHelpText = CString(MAKEINTRESOURCE(IDS_ACC_PROPONLYCHANGE));
            }
        }
    }

    return sHelpText;
}

void CLogDlg::AddMainAnchors()
{
    AddAnchor(IDC_LOGLIST, TOP_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_SPLITTERTOP, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_MSGVIEW, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_SPLITTERBOTTOM, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_LOGMSG, MIDDLE_LEFT, BOTTOM_RIGHT);
}

void CLogDlg::RemoveMainAnchors()
{
    RemoveAnchor(IDC_LOGLIST);
    RemoveAnchor(IDC_SPLITTERTOP);
    RemoveAnchor(IDC_MSGVIEW);
    RemoveAnchor(IDC_SPLITTERBOTTOM);
    RemoveAnchor(IDC_LOGMSG);
}

void CLogDlg::OnEnscrollMsgview()
{
    m_tooltips.DelTool(GetDlgItem(IDC_MSGVIEW), 1);
}

void CLogDlg::AdjustDateFilterVisibility()
{
    int nCmdShow = (m_SelectedFilters & LOGFILTER_DATERANGE) ? SW_SHOW : SW_HIDE;
    GetDlgItem(IDC_FROMLABEL)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_DATEFROM)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_TOLABEL)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_DATETO)->ShowWindow(nCmdShow);

    RemoveAnchor(IDC_SEARCHEDIT);
    RemoveAnchor(IDC_FROMLABEL);
    RemoveAnchor(IDC_DATEFROM);
    RemoveAnchor(IDC_TOLABEL);
    RemoveAnchor(IDC_DATETO);
    if (m_SelectedFilters & LOGFILTER_DATERANGE)
    {
        RECT rcLabel = {0};
        GetDlgItem(IDC_FROMLABEL)->GetWindowRect(&rcLabel);
        ScreenToClient(&rcLabel);
        RECT rcEdit = {0};
        GetDlgItem(IDC_SEARCHEDIT)->GetWindowRect(&rcEdit);
        ScreenToClient(&rcEdit);
        rcEdit.right = rcLabel.left - 20;
        GetDlgItem(IDC_SEARCHEDIT)->MoveWindow(&rcEdit);
    }
    else
    {
        RECT rcLogMsg = {0};
        GetDlgItem(IDC_LOGLIST)->GetWindowRect(&rcLogMsg);
        ScreenToClient(&rcLogMsg);
        RECT rcEdit = {0};
        GetDlgItem(IDC_SEARCHEDIT)->GetWindowRect(&rcEdit);
        ScreenToClient(&rcEdit);
        rcEdit.right = rcLogMsg.right;
        GetDlgItem(IDC_SEARCHEDIT)->MoveWindow(&rcEdit);
    }
    AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_FROMLABEL, TOP_RIGHT);
    AddAnchor(IDC_DATEFROM, TOP_RIGHT);
    AddAnchor(IDC_TOLABEL, TOP_RIGHT);
    AddAnchor(IDC_DATETO, TOP_RIGHT);
}

void CLogDlg::ReportNoUrlOfFile(const CString& filepath) const
{
    ReportNoUrlOfFile((LPCTSTR)filepath);
}

void CLogDlg::ReportNoUrlOfFile(LPCTSTR filepath) const
{
    CString messageString;
    messageString.Format(IDS_ERR_NOURLOFFILE, filepath);
    ::MessageBox(this->m_hWnd, messageString, _T("TortoiseSVN"), MB_ICONERROR);
}

bool CLogDlg::ConfirmRevert( const CString& path, bool bToRev /*= false*/ )
{
    CString msg;
    if (CTaskDialog::IsSupported())
    {
        if (bToRev)
            msg.Format(IDS_LOG_REVERT_CONFIRM_TASK6, (LPCTSTR)path);
        else
            msg.Format(IDS_LOG_REVERT_CONFIRM_TASK1, (LPCTSTR)path);
        CTaskDialog taskdlg(msg,
                            CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK5)));
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_INFORMATION_ICON);
        return (taskdlg.DoModal(m_hWnd) == 1);
    }
    if (bToRev)
        msg.Format(IDS_LOG_REVERTTOREV_CONFIRM, m_path.GetWinPath());
    else
        msg.Format(IDS_LOG_REVERT_CONFIRM, m_path.GetWinPath());
    return (::MessageBox(this->m_hWnd, msg, _T("TortoiseSVN"), MB_YESNO | MB_ICONQUESTION) == IDYES);
}

