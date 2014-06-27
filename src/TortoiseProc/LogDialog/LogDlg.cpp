// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "LogDlg.h"
#include "registry.h"
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
#include "SVNTrace.h"
#include "LogDlgFilter.h"
#include "SVNLogHelper.h"
#include "DiffOptionsDlg.h"
#include "SmartHandle.h"
#include "CodeCollaborator.h"
#include "CodeCollaboratorSettingsDlg.h"
#include "MonitorProjectDlg.h"
#include "MonitorOptionsDlg.h"
#include <tlhelp32.h>
#include <shlwapi.h>

#include "../LogCache/Streams/StreamException.h"

#define ICONITEMBORDER 5

const UINT CLogDlg::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);
const UINT CLogDlg::WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");

#define WM_TSVN_REFRESH_SELECTION       (WM_APP + 1)
#define WM_TSVN_MONITOR_TASKBARCALLBACK (WM_APP + 2)
#define WM_TSVN_MONITOR_NOTIFY_CLICK    (WM_APP + 3)
#define WM_TSVN_MONITOR_TREEDROP        (WM_APP + 4)

#define OVERLAY_MODIFIED        1

class MonitorAlertWnd : public CMFCDesktopAlertWnd
{
public:
    MonitorAlertWnd(HWND hParent) : CMFCDesktopAlertWnd(), m_hParent(hParent) {}
    ~MonitorAlertWnd() {}

    virtual BOOL OnClickLinkButton(UINT uiCmdID)
    {
        ::SendMessage(m_hParent, WM_TSVN_MONITOR_NOTIFY_CLICK, uiCmdID, 0);
        return TRUE;
    }
private:
    HWND m_hParent;
};



enum LogDlgContextMenuCommands
{
    // needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
    ID_COMPARE = 1,
    ID_COMPARETWO,
    ID_COMPAREWITHPREVIOUS,
    ID_BLAMEWITHPREVIOUS,
    ID_UPDATE,
    ID_COPY,
    ID_REVERTREV,
    ID_MERGEREV,
    ID_GNUDIFF1,
    ID_GNUDIFF2,
    ID_FINDENTRY,
    ID_OPEN,
    ID_OPENWITH,
    ID_OPENLOCAL,
    ID_OPENWITHLOCAL,
    ID_REPOBROWSE,
    ID_LOG,
    ID_POPPROPS,
    ID_EDITAUTHOR,
    ID_EDITLOG,
    ID_DIFF,
    ID_DIFF_CONTENTONLY,
    ID_COPYCLIPBOARDFULL,
    ID_COPYCLIPBOARDFULLNOPATHS,
    ID_COPYCLIPBOARDREVS,
    ID_COPYCLIPBOARDAUTHORS,
    ID_COPYCLIPBOARDMESSAGES,
    ID_COPYCLIPBOARDURL,
    ID_COPYCLIPBOARDRELPATH,
    ID_COPYCLIPBOARDFILENAMES,
    ID_CHECKOUT,
    ID_REVERTTOREV,
    ID_BLAME,
    ID_BLAMECOMPARE,
    ID_BLAMETWO,
    ID_BLAMEDIFF,
    ID_VIEWREV,
    ID_VIEWPATHREV,
    ID_SAVEAS,
    ID_EXPORT,
    ID_EXPORTTREE,
    ID_GETMERGELOGS,
    ID_REVPROPS,
    ID_DIFF_MULTIPLE,
    ID_DIFF_MULTIPLE_CONTENTONLY,
    ID_OPENLOCAL_MULTIPLE,
    ID_CODE_COLLABORATOR
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
    , m_regLastStrict(L"Software\\TortoiseSVN\\LastLogStrict", FALSE)
    , m_regMaxBugIDColWidth(L"Software\\TortoiseSVN\\MaxBugIDColWidth", 200)
    , m_bSelectionMustBeContinuous(false)
    , m_bShowBugtraqColumn(false)
    , m_bStrictStopped(false)
    , m_bSingleRevision(true)
    , m_sLogInfo(L"")
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
    , vsRunningScheduler(1, 0, true)
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
    , m_hMovedIcon(NULL)
    , m_hMoveReplacedIcon(NULL)
    , m_nIconFolder(0)
    , m_nOpenIconFolder(0)
    , m_prevLogEntriesSize(0)
    , m_temprev(0)
    , m_tFrom(0)
    , m_tTo(0)
    , m_bVisualStudioRunningAtStart(false)
    , m_bEnsureSelection(false)
    , m_bMonitoringMode(false)
    , m_bKeepHidden(false)
    , m_hwndToolbar(NULL)
    , m_hToolbarImages(NULL)
    , m_bMonitorThreadRunning(FALSE)
    , m_nMonitorUrlIcon(0)
    , m_nMonitorWCIcon(0)
    , m_nErrorOvl(0)
    , m_hMonitorIconNormal(NULL)
    , m_hMonitorIconNewCommits(NULL)
{
    m_bFilterWithRegex =
        !!CRegDWORD(L"Software\\TortoiseSVN\\UseRegexFilter", FALSE);
    m_bFilterCaseSensitively =
        !!CRegDWORD(L"Software\\TortoiseSVN\\FilterCaseSensitively", FALSE);
    m_sMultiLogFormat = CRegString(L"Software\\TortoiseSVN\\LogMultiRevFormat", L"r%1!ld!\n%2!s!\n---------------------\n");
    m_sMultiLogFormat.Replace(L"\\r", L"\r");
    m_sMultiLogFormat.Replace(L"\\n", L"\n");
    // just in case the user sets an impossible/illegal format string: try to use that format
    // string and handle possible exceptions. In case of an exception, fall back to the default.
    try
    {
        CString sRevMsg;
        sRevMsg.FormatMessage(m_sMultiLogFormat, 0, L"test");
    }
    catch (...)
    {
        // fall back to the default
        m_sMultiLogFormat = L"r%1!ld!\n%2!s!\n---------------------\n";
    }
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
    DestroyIcon(m_hMovedIcon);
    DestroyIcon(m_hMoveReplacedIcon);
    if (m_bMonitoringMode)
    {
        DestroyIcon(m_hMonitorIconNormal);
        DestroyIcon(m_hMonitorIconNewCommits);
        Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
    }
    if (m_pStoreSelection)
    {
        m_pStoreSelection->ClearSelection();
        delete m_pStoreSelection;
        m_pStoreSelection = NULL;
    }
    if (m_boldFont)
        DeleteObject(m_boldFont);
    if (m_hToolbarImages)
        ImageList_Destroy(m_hToolbarImages);
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LOGLIST, m_LogList);
    DDX_Control(pDX, IDC_LOGMSG, m_ChangedFileListCtrl);
    DDX_Control(pDX, IDC_PROGRESS, m_LogProgress);
    DDX_Control(pDX, IDC_SPLITTERTOP, m_wndSplitter1);
    DDX_Control(pDX, IDC_SPLITTERBOTTOM, m_wndSplitter2);
    DDX_Control(pDX, IDC_SPLITTERLEFT, m_wndSplitterLeft);
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
    DDX_Control(pDX, IDC_PROJTREE, m_projTree);
}

BEGIN_MESSAGE_MAP(CLogDlg, CResizableStandAloneDialog)
    ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
    ON_REGISTERED_MESSAGE(WM_TASKBARCREATED, OnTaskbarCreated)
    ON_MESSAGE(WM_TSVN_REFRESH_SELECTION, OnRefreshSelection)
    ON_MESSAGE(WM_TSVN_MONITOR_TASKBARCALLBACK, OnTaskbarCallBack)
    ON_MESSAGE(WM_TSVN_MONITOR_NOTIFY_CLICK, OnMonitorNotifyClick)
    ON_MESSAGE(WM_TSVN_MONITOR_TREEDROP, OnTreeDrop)
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
    ON_EN_VSCROLL(IDC_MSGVIEW, &CLogDlg::OnEnscrollMsgview)
    ON_EN_HSCROLL(IDC_MSGVIEW, &CLogDlg::OnEnscrollMsgview)
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_COMMAND(ID_LOGDLG_MONITOR_CHECKREPOSITORIESNOW, &CLogDlg::OnMonitorCheckNow)
    ON_COMMAND(ID_LOGDLG_MONITOR_ADDPROJECT, &CLogDlg::OnMonitorAddProject)
    ON_COMMAND(ID_LOGDLG_MONITOR_EDIT, &CLogDlg::OnMonitorEditProject)
    ON_COMMAND(ID_LOGDLG_MONITOR_REMOVE, &CLogDlg::OnMonitorRemoveProject)
    ON_COMMAND(ID_MISC_OPTIONS, &CLogDlg::OnMonitorOptions)
    ON_COMMAND(ID_LOGDLG_MONITOR_THREADFINISHED, &CLogDlg::OnMonitorThreadFinished)
    ON_NOTIFY(TVN_SELCHANGED, IDC_PROJTREE, &CLogDlg::OnTvnSelchangedProjtree)
    ON_NOTIFY(TVN_GETDISPINFO, IDC_PROJTREE, &CLogDlg::OnTvnGetdispinfoProjtree)
    ON_NOTIFY(NM_CLICK, IDC_PROJTREE, &CLogDlg::OnNMClickProjtree)
    ON_WM_WINDOWPOSCHANGING()
    ON_NOTIFY(TVN_ENDLABELEDIT, IDC_PROJTREE, &CLogDlg::OnTvnEndlabeleditProjtree)
    ON_COMMAND(ID_INLINEEDIT, &CLogDlg::OnInlineedit)
END_MESSAGE_MAP()

void CLogDlg::SetParams(const CTSVNPath& path, SVNRev pegrev, SVNRev startrev, SVNRev endrev,
                        BOOL bStrict /* = FALSE */, BOOL bSaveStrict /* = TRUE */, int limit)
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
    if (revArray.GetCount() && revArray.GetLowestRevision().IsNumber() && svn_revnum_t(revArray.GetLowestRevision()))
    {
        m_bEnsureSelection = true;
        m_endrev = revArray.GetLowestRevision();
    }
}

void CLogDlg::SubclassControls()
{
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
}

void CLogDlg::SetupDialogFonts()
{
    // use the default GUI font, create a copy of it and
    // change the copy to BOLD (leave the rest of the font
    // the same)
    HFONT hFont = (HFONT)m_LogList.SendMessage(WM_GETFONT);
    LOGFONT lf = {0};
    GetObject(hFont, sizeof(LOGFONT), &lf);
    lf.lfWeight = FW_BOLD;
    m_boldFont = CreateFontIndirect(&lf);
    CAppUtils::CreateFontForLogs(m_logFont);
}

void CLogDlg::RestoreSavedDialogSettings()
{
    // use the state of the "stop on copy/rename" option from the last time
    if (!m_bStrict)
        m_bStrict = m_regLastStrict;
    UpdateData(FALSE);
    CString temp;
    if (m_limit)
        temp.Format(IDS_LOG_SHOWNEXT, m_limit);
    else
        temp.Format(IDS_LOG_SHOWNEXT, (int)(DWORD)CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs", 100));

    SetDlgItemText(IDC_NEXTHUNDRED, temp);

    // Show paths checkbox
    int checkState = (int)DWORD(CRegDWORD(L"Software\\TortoiseSVN\\LogShowPaths", BST_UNCHECKED));
    m_cShowPaths.SetCheck(checkState);

    switch ((LONG)CRegDWORD(L"Software\\TortoiseSVN\\ShowAllEntry"))
    {
        default:
        case 0:
            m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWALL)));
            break;
        case  1:
            m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWRANGE)));
            break;
    }
}

void CLogDlg::SetupLogMessageViewControl()
{
     // set the font to use in the log message view, configured in the settings dialog
    GetDlgItem(IDC_MSGVIEW)->SetFont(&m_logFont);
    // make the log message rich edit control send a message when the mouse pointer is over a link
    GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK|ENM_SCROLL);
}

void CLogDlg::SetupLogListControl()
{
    DWORD dwStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
    // we *could* enable checkboxes on pre Vista OS too, but those don't have
    // the LVS_EX_AUTOCHECKSELECT style. Without that style, users could get
    // very confused because selected items are not checked.
    // Also, while handling checkboxes is implemented, most code paths in this
    // file still only work on the selected items, not the checked ones.
    if (m_bSelect)
        dwStyle |= LVS_EX_CHECKBOXES | 0x08000000 /*LVS_EX_AUTOCHECKSELECT*/;
    m_LogList.SetExtendedStyle(dwStyle);
    m_LogList.SetTooltipProvider(this);
}

void CLogDlg::LoadIconsForActionColumns()
{
    // load the icons for the action columns
    m_hModifiedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED),
                                                                        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED),
                                                                        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hAddedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED),
                                                                        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED),
                                                                        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hMergedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMERGED),
                                                                        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hReverseMergedIcon = (HICON)LoadImage(AfxGetResourceHandle(),
                                MAKEINTRESOURCE(IDI_ACTIONREVERSEMERGED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hMovedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED),
                                       IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    m_hMoveReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED),
                                       IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
}

void CLogDlg::ConfigureColumnsForLogListControl()
{
    CString temp;
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

    SetWindowTheme(m_LogList.GetSafeHwnd(), L"Explorer", NULL);
    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::ConfigureColumnsForChangedFileListControl()
{
    CString temp;
    m_ChangedFileListCtrl.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
    m_ChangedFileListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
    int c = ((CHeaderCtrl*)(m_ChangedFileListCtrl.GetDlgItem(0)))->GetItemCount()-1;
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

    SetWindowTheme(m_ChangedFileListCtrl.GetSafeHwnd(), L"Explorer", NULL);
}

void CLogDlg::SetupFilterControlBitmaps()
{
    // the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
    m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED);
    m_cFilter.SetInfoIcon(IDI_LOGFILTER);
    m_cFilter.SetValidator(this);
    m_cFilter.SetWindowText(m_sFilterText);
}

void CLogDlg::ConfigureResizableControlAnchors()
{
    // resizable stuff
    AddMainAnchors();
}

void CLogDlg::RestoreLogDlgWindowAndSplitters()
{
    DWORD yPos1 = CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1");
    DWORD yPos2 = CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2");
    DWORD xPos = CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer3");
    RECT rcDlg, rcLogList, rcChgMsg, rcProjTree;
    GetClientRect(&rcDlg);
    m_LogList.GetWindowRect(&rcLogList);
    ScreenToClient(&rcLogList);
    m_ChangedFileListCtrl.GetWindowRect(&rcChgMsg);
    ScreenToClient(&rcChgMsg);
    m_projTree.GetWindowRect(&rcProjTree);
    ScreenToClient(&rcProjTree);

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
    if (xPos || m_bMonitoringMode)
    {
        if (xPos == 0)
            xPos = 80;
        RECT rectSplitter;
        m_wndSplitterLeft.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        int delta = xPos - rectSplitter.left;
        if ((rcProjTree.right + delta > rcProjTree.left) && (rcProjTree.right + delta < m_LogListOrigRect.Width()))
        {
            m_wndSplitterLeft.SetWindowPos(NULL, xPos, rectSplitter.top, 0, 0, SWP_NOSIZE);
            DoSizeV3(delta);
        }
    }
    SetSplitterRange();
}

void CLogDlg::AdjustControlSizesForLocalization()
{
    AdjustControlSize(IDC_SHOWPATHS);
    AdjustControlSize(IDC_CHECK_STOPONCOPY);
    AdjustControlSize(IDC_INCLUDEMERGE);
    AdjustControlSize(IDC_HIDENONMERGEABLE);
}

void CLogDlg::GetOriginalControlRectangles()
{
    GetClientRect(m_DlgOrigRect);
    m_LogList.GetClientRect(m_LogListOrigRect);
    GetDlgItem(IDC_MSGVIEW)->GetClientRect(m_MsgViewOrigRect);
    m_ChangedFileListCtrl.GetClientRect(m_ChgOrigRect);
    m_projTree.GetClientRect(m_ProjTreeOrigRect);
}

void CLogDlg::SetupDatePickerControls()
{
    m_DateFrom.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|
                                                MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);
    m_DateTo.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS|MCS_NOTODAY|
                                                MCS_NOTRAILINGDATES|MCS_NOSELCHANGEONNAV);

    // show/hide the date filter controls according to the filter setting
    AdjustDateFilterVisibility();
}

void CLogDlg::ConfigureDialogForPickingRevisionsOrShowingLog()
{
    CString temp;
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
}

void CLogDlg::SetupButtonMenu()
{
    m_btnMenu.CreatePopupMenu();
    m_btnMenu.AppendMenu(MF_STRING|MF_BYCOMMAND, ID_CMD_SHOWALL,
                                             CString(MAKEINTRESOURCE(IDS_LOG_SHOWALL)));
    m_btnMenu.AppendMenu(MF_STRING|MF_BYCOMMAND, ID_CMD_SHOWRANGE,
                                             CString(MAKEINTRESOURCE(IDS_LOG_SHOWRANGE)));
    m_btnShow.m_hMenu = m_btnMenu.GetSafeHmenu();
    m_btnShow.m_bOSMenu = TRUE;
    m_btnShow.m_bRightArrow = TRUE;
    m_btnShow.m_bDefaultClick = TRUE;
    m_btnShow.m_bTransparent = TRUE;
}

void CLogDlg::ReadProjectPropertiesAndBugTraqInfo()
{
    // if there is a working copy, load the project properties
    // to get information about the bugtraq: integration
    if (m_hasWC)
        m_ProjectProperties.ReadProps(m_path);

    // the bugtraq issue id column is only shown if the bugtraq:url or bugtraq:regex is set
    if ((!m_ProjectProperties.sUrl.IsEmpty())||(!m_ProjectProperties.GetCheckRe().IsEmpty()))
        m_bShowBugtraqColumn = true;

}

void CLogDlg::SetupToolTips()
{
    EnableToolTips();
    m_tooltips.Create(this);
    CheckRegexpTooltip();
}

void CLogDlg::InitializeTaskBarListPtr()
{
    m_pTaskbarList.Release();
    if (m_bMonitoringMode)
    {
        m_pTaskbarList = nullptr;
        return;
    }
    if (FAILED(m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
        m_pTaskbarList = nullptr;
}

void CLogDlg::CenterThisWindow()
{
     if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
}

void CLogDlg::SetupAccessibility()
{
    // set up the accessibility callback
    m_pLogListAccServer = ListViewAccServer::CreateProvider(m_LogList.GetSafeHwnd(), this);
    m_pChangedListAccServer = ListViewAccServer::CreateProvider(m_ChangedFileListCtrl.GetSafeHwnd(), this);
}

void CLogDlg::ExtraInitialization()
{
    m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_LOGDLG));
    m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
    m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();
    m_sMessageBuf.Preallocate(100000);
    m_mergedRevs.clear();
}

BOOL CLogDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    ExtraInitialization();
    if (!m_bMonitoringMode)
    {
        ExtendFrameIntoClientArea(IDC_LOGMSG, IDC_SEARCHEDIT, IDC_LOGMSG, IDC_LOGMSG);

        SubclassControls();
    }
    else
        InitMonitoringMode();

    InitializeTaskBarListPtr();
    SetupDialogFonts();
    RestoreSavedDialogSettings();
    SetupLogMessageViewControl();
    SetupLogListControl();
    LoadIconsForActionColumns();
    ReadProjectPropertiesAndBugTraqInfo();
    ConfigureColumnsForLogListControl();
    ConfigureColumnsForChangedFileListControl();

    // set the dialog title to "Log - path/to/whatever/we/show/the/log/for"
    SetDlgTitle(false);
    SetupFilterControlBitmaps();
    AdjustControlSizesForLocalization();
    GetOriginalControlRectangles();
    SetupDatePickerControls();
    SetFilterCueText();
    ConfigureResizableControlAnchors();
    SetPromptParentWindow(m_hWnd);
    CenterThisWindow();
    EnableSaveRestore(m_bMonitoringMode ? L"MonitorLogDlg" : L"LogDlg");
    SetSplitterRange();
    RestoreLogDlgWindowAndSplitters();
    ConfigureDialogForPickingRevisionsOrShowingLog();
    SetupButtonMenu();
    SetupAccessibility();
    SetupToolTips();

    if (!m_bMonitoringMode)
    {
        // first start a thread to obtain the log messages without
        // blocking the dialog
        InterlockedExchange(&m_bLogThreadRunning, TRUE);
        new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
        // detect Visual Studio Running with thread
    }
    new async::CAsyncCall(this, &CLogDlg::DetectVisualStudioRunningThread, &vsRunningScheduler);
    GetDlgItem(IDC_LOGLIST)->SetFocus();
    return FALSE;
}

void CLogDlg::SetDlgTitle(bool bOffline)
{
    if (m_bMonitoringMode)
    {
        SetWindowText(CString(MAKEINTRESOURCE(IDS_MONITOR_DLGTITLE)));
    }
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
    bool IsAllWhitespace (const std::wstring& text, long first, long last)
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

    void ReduceRanges(std::vector<CHARRANGE>& ranges, const std::wstring& text)
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
        std::wstring text;

        std::vector<CHARRANGE> ranges;
        std::vector<CHARRANGE> idRanges;
        std::vector<CHARRANGE> revRanges;
        std::vector<CHARRANGE> urlRanges;

        BOOL RunRegex (ProjectProperties *project)
        {
            // turn bug ID's into links if the bugtraq: properties have been set
            // and we can find a match of those in the log message
            idRanges = project->FindBugIDPositions (sText);

            // underline all revisions mentioned in the message
            revRanges = CAppUtils::FindRegexMatches ( text
                                                    , project->GetLogRevRegex()
                                                    , L"\\d+");

            urlRanges = CAppUtils::FindURLMatches(sText);

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
    pMsgView->SetWindowText(L" ");
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
        if (pLogEntry == NULL)
            return;

        pLogEntry->SetUnread(false);

        pMsgView->SetRedraw(FALSE);

        // the rich edit control doesn't count the CR char!
        // to be exact: CRLF is treated as one char.

        SMarkerInfo info;
        if (m_bSingleRevision)
        {
            info.sText = CUnicodeUtils::GetUnicode
                            (pLogEntry->GetMessage().c_str());
            info.sText.Remove(L'\r');
        }
        else
        {
            m_currentChangedArray.RemoveAll();
            m_currentChangedPathList = GetChangedPathsAndMessageSketchFromSelectedRevisions(info.sText,
                                                                                    m_currentChangedArray);
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

        if (((DWORD)CRegStdDWORD(L"Software\\TortoiseSVN\\StyleCommitMessages", TRUE))==TRUE)
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
        CAppUtils::SetCharFormat (pMsgView, CFM_LINK, CFE_LINK, info.urlRanges);
        CHARRANGE range;
        range.cpMin = 0;
        range.cpMax = 0;
        pMsgView->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);

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

    CRegDWORD reg = CRegDWORD(L"Software\\TortoiseSVN\\ShowAllEntry");

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
    pMsgView->SetWindowText(L"");

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
    pMsgView->SetWindowText(L"");

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

    m_limit = (int)(DWORD)CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs", 100) +1;
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
        CRegDWORD regPos1 =
            CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1");
        CRegDWORD regPos2 =
            CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2");
        CRegDWORD regPos3 =
            CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer3");
        RECT rectSplitter;
        m_wndSplitter1.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        regPos1 = rectSplitter.top;
        m_wndSplitter2.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        regPos2 = rectSplitter.top;
        if (m_bMonitoringMode)
        {
            m_wndSplitterLeft.GetWindowRect(&rectSplitter);
            ScreenToClient(&rectSplitter);
            regPos3 = rectSplitter.left;
        }
    }
}

void CLogDlg::OnLogCancel()
{
    // Canceling can mean just to stop fetching data, depending on the text
    // shown on the cancel button (it will read "Cancel" in that case).

    CString temp, temp2;
    GetDlgItemText(IDC_LOGCANCEL, temp);
    temp2.LoadString(IDS_MSGBOX_CANCEL);
    if ((temp.Compare(temp2)==0) && !GetDlgItem(IDOK)->IsWindowVisible())
    {
        m_bCancelled = true;
        return;
    }

    // we actually want to close the dialog.

    OnCancel();
}

void CLogDlg::OnCancel()
{
    if (m_bMonitoringMode)
    {
        ShowWindow(SW_HIDE);
        return;
    }
    bool bWasCancelled = m_bCancelled;
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
        if (bWasCancelled)
        {
            // end the process the hard way
            TerminateProcess(GetCurrentProcess(), 0);
        }
        else
        {
            // to force the shutdown, another click on cancel is required
            return;
        }
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
            CRegDWORD reg = CRegDWORD(L"Software\\TortoiseSVN\\ShowAllEntry");
            reg = (DWORD)m_btnShow.m_nMenuResult;

            CRegDWORD reg2 = CRegDWORD(L"Software\\TortoiseSVN\\LogShowPaths");
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
    if (m_bMonitoringMode)
        ShowWindow(SW_HIDE);
    else
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

    if (m_bMonitoringMode)
        ShutDownMonitoring();

    __super::OnDestroy();
}

BOOL CLogDlg::Log(svn_revnum_t rev, const std::string& author, const std::string& message,
                  apr_time_t time, const MergeInfo* mergeInfo)
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
                m_pTaskbarList->SetProgressValue(m_hWnd, (svn_revnum_t)m_startrev-rev +
                                                            (svn_revnum_t)m_endrev-l, u-l);
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

    {
        CAutoReadLock pathlock(m_monitorpathguard);
        if (m_path.IsEquivalentToWithoutCase(CTSVNPath(m_pathCurrentlyChecked)))
            return;
    }

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
    m_LogProgress.ShowWindow(TRUE);

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

        if (m_bStartRevIsHead || (m_startrev == SVNRev::REV_HEAD) || (m_endrev == SVNRev::REV_HEAD)
            || (m_head < 0))
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
            Err = svn_client_mergeinfo_get_merged (&mergeinfo, svnPath, SVNRev(SVNRev::REV_WC),
                                                    m_pctx, localpool),
            svnPath
        )
        if (Err == NULL)
        {
            // now check the relative paths
            const void *key;
            void *val;

            if (mergeinfo)
            {
                // unfortunately it's not defined whether the urls have a trailing
                // slash or not, so we have to trim such a slash before comparing
                // the sUrl with the mergeinfo url
                CStringA sUrl = CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(m_sURL));
                sUrl.TrimRight('/');

                for (apr_hash_index_t *hi = apr_hash_first(localpool, mergeinfo); hi; hi = apr_hash_next(hi))
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

                bool bFindCopyFrom = !!(DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LogFindCopyFrom",
                                                        FALSE);
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
        if (m_bEnsureSelection)
        {
            // ensure that the end revision is fetched, so adjust the limit
            if (m_limit && m_startrev.IsNumber() && (svn_revnum_t(m_startrev) > 0))
            {
                m_limit = max(m_limit, svn_revnum_t(m_startrev) - svn_revnum_t(m_endrev));
                m_endrev = 0;
            }
            m_bEnsureSelection = false;
        }
        cachedData = ReceiveLog (CTSVNPathList(m_path), m_pegrev, m_startrev, m_endrev, m_limit,
                                        !!m_bStrict, !!m_bIncludeMerges, m_nRefresh==Cache);
        if ((cachedData.get() == NULL)&&(!m_path.IsUrl()))
        {
            // try again with REV_WC as the start revision, just in case the path doesn't
            // exist anymore in HEAD.
            // Also, make sure we use these parameters for further requests (like "next 100").

            m_pegrev = SVNRev::REV_WC;
            m_startrev = SVNRev::REV_WC;

            cachedData = ReceiveLog(CTSVNPathList(m_path), m_pegrev, m_startrev, m_endrev, m_limit,
                                                !!m_bStrict, !!m_bIncludeMerges, m_nRefresh==Cache);
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
            m_logEntries.Finalize (std::move(cachedData), m_sRelativeRoot,
                                            !LogCache::CSettings::GetEnabled());
        else
            m_logEntries.ClearAll();

        if (m_bMonitoringMode && (m_revUnread > 0))
        {
            // mark all entries that are new as unread, so they're shown
            // in bold in the list control
            for (size_t i = 0; i < m_logEntries.GetVisibleCount(); ++i)
            {
                PLOGENTRYDATA pEntry = m_logEntries.GetVisible(i);
                if (pEntry && (pEntry->GetRevision() > m_revUnread))
                    pEntry->SetUnread(true);
            }
            m_revUnread = 0;
        }
    }
    m_LogList.ClearText();
    if (!succeeded)
    {
        temp.LoadString(IDS_LOG_CLEARERROR);
        m_LogList.ShowText(GetLastErrorMessage() + L"\n\n" + temp, true);
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

    m_LogProgress.ShowWindow(FALSE);
    if (!m_bMonitoringMode)
        GetDlgItem(IDC_HIDENONMERGEABLE)->ShowWindow(!m_mergedRevs.empty());
    else
        DialogEnableWindow(IDC_PROJTREE, TRUE);

    if (m_pTaskbarList)
    {
        m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
    }
    m_bCancelled = false;
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
    bool bAllowStatusCheck = m_bMonitoringMode || !!(DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LogStatusCheck", TRUE);
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
    if ((GetKeyState(VK_CONTROL) & 0x8000) && ((GetKeyState(L'C') & 0x8000)==0) &&
                                                    ((GetKeyState(VK_INSERT) & 0x8000)==0))
    {
        CopyCommaSeparatedRevisionsToClipboard();
    }
    else
    {
        CopySelectionToClipBoard(!(GetKeyState(VK_SHIFT) & 0x8000));
    }
}


// generate a comma delimited string of revision numbers
// we can paste this list into a code review tool
void CLogDlg::CopyCommaSeparatedRevisionsToClipboard()
{
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    CString sRevisions;
    CString sRevision;

    if (pos != NULL)
    {
        while(pos)
        {
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index >= (int)m_logEntries.GetVisibleCount())
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (index);
            if (pLogEntry)
            {
                sRevision.Format(L"%ld, ",pLogEntry->GetRevision());
                sRevisions += sRevision;
            }
        }

        // trim trailing comma and space
        int revisionsLength = sRevisions.GetLength() - 2;
        if (revisionsLength > 0)
        {
            sRevisions = sRevisions.Left(revisionsLength);
            CStringUtils::WriteAsciiStringToClipboard(sRevisions, GetSafeHwnd());
        }
    }
}

void CLogDlg::CopyCommaSeparatedAuthorsToClipboard()
{
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    CString sAuthors;
    CString sAuthor;

    if (pos != NULL)
    {
        while(pos)
        {
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index >= (int)m_logEntries.GetVisibleCount())
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (index);
            if (pLogEntry)
            {
                sAuthor.Format(L"%s, ", CUnicodeUtils::StdGetUnicode(pLogEntry->GetAuthor()).c_str());
                sAuthors += sAuthor;
            }
        }

        // trim trailing comma and space
        int authorsLength = sAuthors.GetLength() - 2;
        if (authorsLength > 0)
        {
            sAuthors = sAuthors.Left(authorsLength);
            CStringUtils::WriteAsciiStringToClipboard(sAuthors, GetSafeHwnd());
        }
    }
}

void CLogDlg::CopyMessagesToClipboard()
{
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    CString sMessages;
    CString sMessage;

    if (pos != NULL)
    {
        while(pos)
        {
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index >= (int)m_logEntries.GetVisibleCount())
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (index);
            if (pLogEntry)
            {
                sMessage.Format(L"%s\r\n----\r\n", CUnicodeUtils::StdGetUnicode(pLogEntry->GetMessageW()).c_str());
                sMessages += sMessage;
            }
        }

        CStringUtils::WriteAsciiStringToClipboard(sMessages, GetSafeHwnd());
    }
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
            if (pLogEntry == NULL)
                continue;
            if (bIncludeChangedList)
            {
                CString sPaths;
                const CLogChangedPathArray& cpatharray = pLogEntry->GetChangedPaths();
                for (size_t cpPathIndex = 0; cpPathIndex < cpatharray.GetCount(); ++cpPathIndex)
                {
                    const CLogChangedPath& cpath = cpatharray[cpPathIndex];
                    sPaths += CUnicodeUtils::GetUnicode (cpath.GetActionString().c_str())
                        + L" : " + cpath.GetPath();
                    if (cpath.GetCopyFromPath().IsEmpty())
                        sPaths += L"\r\n";
                    else
                    {
                        CString sCopyFrom;
                        sCopyFrom.Format(L" (%s: %s, %s, %ld)\r\n",
                            (LPCTSTR)CString(MAKEINTRESOURCE(IDS_LOG_COPYFROM)),
                            (LPCTSTR)cpath.GetCopyFromPath(),
                            (LPCTSTR)CString(MAKEINTRESOURCE(IDS_LOG_REVISION)),
                            cpath.GetCopyFromRev());
                        sPaths += sCopyFrom;
                    }
                }
                sPaths.Trim();
                CString nlMessage = CUnicodeUtils::GetUnicode (pLogEntry->GetMessage().c_str());
                nlMessage.Remove(L'\r');
                nlMessage.Replace(L"\n", L"\r\n");
                sLogCopyText.Format(L"%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n",
                    (LPCTSTR)sRev, pLogEntry->GetRevision(),
                    (LPCTSTR)sAuthor,  (LPCTSTR)CUnicodeUtils::GetUnicode (pLogEntry->GetAuthor().c_str()),
                    (LPCTSTR)sDate,
                    (LPCTSTR)CUnicodeUtils::GetUnicode (pLogEntry->GetDateString().c_str()),
                    (LPCTSTR)sMessage, (LPCTSTR)nlMessage,
                    (LPCTSTR)sPaths);
            }
            else
            {
                CString nlMessage = CUnicodeUtils::GetUnicode (pLogEntry->GetMessage().c_str());
                nlMessage.Remove(L'\r');
                nlMessage.Replace(L"\n", L"\r\n");
                sLogCopyText.Format(L"%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n",
                    (LPCTSTR)sRev, pLogEntry->GetRevision(),
                    (LPCTSTR)sAuthor,  (LPCTSTR)CUnicodeUtils::GetUnicode (pLogEntry->GetAuthor().c_str()),
                    (LPCTSTR)sDate,
                    (LPCTSTR)CUnicodeUtils::GetUnicode (pLogEntry->GetDateString().c_str()),
                    (LPCTSTR)sMessage, (LPCTSTR)nlMessage);
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
        sPaths += L"\r\n";
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
        ShowContextMenuForChangedPaths(pWnd, point);
    }
    else if (pWnd == &m_projTree)
    {
        ShowContextMenuForMonitorTree(pWnd, point);
    }
    else if ((selCount > 0) && (pWnd == GetDlgItem(IDC_MSGVIEW)))
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

            int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY |
                                                    TPM_RIGHTBUTTON, point.x, point.y, this, 0);
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
        std::tr1::wregex pat;
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

    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    if (pos)
    {
        while (pos)
        {
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index < (int)m_logEntries.GetVisibleCount())
            {
                PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (index);
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
    if (m_bMonitoringMode)
    {
        ShowWindow(SW_HIDE);
        return;
    }
    // since the log dialog is also used to select revisions for other
    // dialogs, we have to do some work before closing this dialog
    if ((GetKeyState(VK_MENU)&0x8000) == 0)
    {
        // if the ALT key is pressed, we get here because of an accelerator

        // if the "OK" button doesn't have the focus, do nothing: this prevents
        // closing the dialog when pressing enter
        if ((GetDlgItem(IDOK)->IsWindowVisible()) && (GetFocus() != GetDlgItem(IDOK)))
            return;
        // the Cancel button works as the OK button. But if the cancel button
        // does not have the focus, do nothing.
        if (!GetDlgItem(IDOK)->IsWindowVisible() && GetFocus() != GetDlgItem(IDC_LOGCANCEL))
            return;
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
    CRegDWORD reg = CRegDWORD(L"Software\\TortoiseSVN\\ShowAllEntry");
    reg = (DWORD)m_btnShow.m_nMenuResult;
    CRegDWORD reg2 = CRegDWORD(L"Software\\TortoiseSVN\\LogHidePaths");
    reg2 = (DWORD)m_cShowPaths.GetCheck();
    SaveSplitterPos();

    CString buttontext;
    GetDlgItemText(IDOK, buttontext);
    CString temp;
    temp.LoadString(IDS_MSGBOX_CANCEL);
    // only exit if the button text matches, and that will match only if the thread isn't running anymore
    if (temp.Compare(buttontext) != 0)
        __super::OnOK();
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
        // was directly affected in that revision. If it was, then check if our path was copied
        // from somewhere. If it was copied, use the copy from revision as lowerRev
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
                m_pNotifyWindow->SendMessage(WM_REVLIST, m_selectedRevs.GetCount(),
                                                                    (LPARAM)&m_selectedRevs);
                bSentMessage = true;
                break;
            }
        }
    }
    if ( !bSentMessage )
    {
        m_pNotifyWindow->SendMessage(WM_REVSELECTED,
            m_wParam & (MERGE_REVSELECTSTART | MERGE_REVSELECTMINUSONE), lowerRev);
        m_pNotifyWindow->SendMessage(WM_REVSELECTED,
            m_wParam & (MERGE_REVSELECTEND | MERGE_REVSELECTMINUSONE), higherRev);
        m_pNotifyWindow->SendMessage(WM_REVLIST,
            m_selectedRevs.GetCount(), (LPARAM)&m_selectedRevs);
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

int CLogDlg::OpenWorkingCopyFileWithRegisteredProgram(CString& fullPath)
{
    if (!PathFileExists((LPCWSTR)fullPath))
        return -1;

    return (int)ShellExecute(this->m_hWnd, NULL, fullPath, NULL, NULL, SW_SHOWNORMAL);
}

void CLogDlg::DoOpenFileWith(bool bReadOnly, bool bOpenWith, const CTSVNPath& tempfile)
{
    if (bReadOnly)
        SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    int ret = 0;
    if (!bOpenWith)
        ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
    if ((ret <= HINSTANCE_ERROR)||bOpenWith)
    {
        OPENASINFO oi = { 0 };
        oi.pcszFile = tempfile.GetWinPath();
        oi.oaifInFlags = OAIF_EXEC;
        SHOpenWithDialog(GetSafeHwnd(), &oi);
    }
}

void CLogDlg::OnNMDblclkChangedFileList(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    // a double click on an entry in the changed-files list has happened
    *pResult = 0;

    DiffSelectedFile(true);
}

void CLogDlg::DiffSelectedFile( bool ignoreprops )
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
    if (pLogEntry == NULL)
        return;
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
        auto f = [=]()
        {
            CoInitialize(NULL);
            this->EnableWindow(FALSE);
            DoDiffFromLog(selIndex, rev1, rev2, false, false, ignoreprops);
            this->EnableWindow(TRUE);
            this->SetFocus();
        };
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
            auto f = [=]()
            {
                CoInitialize(NULL);
                this->EnableWindow(FALSE);
                DoDiffFromLog(selIndex, rev1, rev2, false, false, ignoreprops);
                this->EnableWindow(TRUE);
                this->SetFocus();
            };
            new async::CAsyncCall(f, &netScheduler);
        }
        else
        {
            CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false,
                                                                        CTSVNPath(changedpath.GetPath()));
            CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(false,
                                                                        CTSVNPath(changedpath.GetPath()));
            SVNRev r = rev1;
            // deleted files must be opened from the revision before the deletion
            if (changedpath.GetAction() == LOGACTIONS_DELETED)
                r = rev1-1;
            m_bCancelled = false;

            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            CString sInfoLine;
            sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION,
                (LPCTSTR)(m_sRepositoryRoot + changedpath.GetPath()), (LPCTSTR)r.ToString());
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
            sName1.Format(L"%s - Revision %ld",
                (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath.GetPath()), (svn_revnum_t)rev1);
            sName2.Format(L"%s - Revision %ld",
                (LPCTSTR)CPathUtils::GetFileNameFromPath(changedpath.GetPath()), (svn_revnum_t)rev1-1);
            CAppUtils::DiffFlags flags;
            flags.AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
            if (changedpath.GetAction() == LOGACTIONS_DELETED)
                CAppUtils::StartExtDiff(tempfile, tempfile2, sName2, sName1,
                                        url, url, r, SVNRev(), r, flags, 0, url.GetFileOrDirectoryName());
            else
                CAppUtils::StartExtDiff(tempfile2, tempfile, sName2, sName1,
                                        url, url, r, SVNRev(), r, flags, 0, url.GetFileOrDirectoryName());
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
    if (CRegDWORD(L"Software\\TortoiseSVN\\DiffByDoubleClickInLog", FALSE))
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
    if (pLogEntry == NULL)
        return;
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
        // We're looking at the log for a directory and only one file under
        // dir was changed in the revision. Do diff on that file instead of whole directory

        const CLogChangedPath& cpath = pLogEntry->GetChangedPaths()[lastChangedIndex];
        path.SetFromWin (m_sRepositoryRoot + cpath.GetPath());
        nodekind = cpath.GetNodeKind() < 0 ? svn_node_unknown : cpath.GetNodeKind();
    }

    m_bCancelled = FALSE;
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    CLogWndHourglass wait;


    if (PromptShown())
    {
        SVNDiff diff(this, m_hWnd, true);
        diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
        diff.SetHEADPeg(m_LogRevision);
        diff.ShowCompare(path, rev2, path, rev1, SVNRev(), false, L"", false, false, nodekind);
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, path, rev2, path, rev1, SVNRev(),
            m_LogRevision, false, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), false, false, nodekind);
    }

    EnableOKButton();
}

void CLogDlg::DoDiffFromLog( INT_PTR selIndex, svn_revnum_t rev1, svn_revnum_t rev2, bool blame, bool unified, bool ignoreprops )
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
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
    // see this thread for why:
    // http://tortoisesvn.tigris.org/ds/viewMessage.do?dsForumId=4061&dsMessageId=2096879
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
                EnableOKButton();
                return;
            }
        }
        if (PromptShown())
            diff.ShowUnifiedDiff(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1,
                                    SVNRev(), options);
        else
            CAppUtils::StartShowUnifiedDiff(m_hWnd, CTSVNPath(secondfile), rev2, CTSVNPath(firstfile),
                                                rev1, SVNRev(), m_LogRevision, options);
    }
    else
    {
        diff.ShowCompare(CTSVNPath(secondfile), rev2, CTSVNPath(firstfile), rev1, SVNRev(),
                                        ignoreprops, L"", false, blame, nodekind);
    }
    EnableOKButton();
}

BOOL CLogDlg::Open(bool bOpenWith,CString changedpath, svn_revnum_t rev)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
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
    CString sInfoLine;
    sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)filepath,
                                                    (LPCTSTR)SVNRev(rev).ToString());
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
        return FALSE;
    }
    progDlg.Stop();
    SetAndClearProgressInfo((HWND)NULL);
    DoOpenFileWith(true, bOpenWith, tempfile);
    EnableOKButton();
    return TRUE;
}

void CLogDlg::EditAuthor(const std::vector<PLOGENTRYDATA>& logs)
{
    if (logs.empty())
        return;

    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);

    CString url;
    if (SVN::PathIsURL(m_path))
        url = m_path.GetSVNPathString();
    else
        url = GetURLFromPath(m_path);

    CString name = CString(SVN_PROP_REVISION_AUTHOR);
    CString value = RevPropertyGet(name, CTSVNPath(url), logs[0]->GetRevision());
    CString sOldValue = value;
    value.Replace(L"\n", L"\r\n");

    CInputDlg dlg(this);
    dlg.m_sHintText.LoadString(IDS_LOG_AUTHOR);
    dlg.m_sInputText = value;
    dlg.m_sTitle.LoadString(IDS_LOG_AUTHOREDITTITLE);
    if (dlg.DoModal() == IDOK)
    {
        if(sOldValue.Compare(dlg.m_sInputText))
        {
            dlg.m_sInputText.Remove(L'\r');

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
                if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url),
                                                logs[i]->GetRevision()))
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
    EnableOKButton();
}

void CLogDlg::EditLogMessage( size_t index )
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    CLogWndHourglass wait;

    CString url;
    if (SVN::PathIsURL(m_path))
        url = m_path.GetSVNPathString();
    else
        url = GetURLFromPath(m_path);

    CString name = CString(SVN_PROP_REVISION_LOG);

    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
    if (pLogEntry == NULL)
        return;

    m_bCancelled = FALSE;
    CString value = RevPropertyGet(name, CTSVNPath(url), pLogEntry->GetRevision());
    CString sOldValue = value;
    value.Replace(L"\n", L"\r\n");
    CInputDlg dlg(this);
    dlg.m_sHintText.LoadString(IDS_LOG_MESSAGE);
    dlg.m_sInputText = value;
    dlg.m_sTitle.LoadString(IDS_LOG_MESSAGEEDITTITLE);
    if (dlg.DoModal() == IDOK)
    {
        if(sOldValue.Compare(dlg.m_sInputText))
        {
            dlg.m_sInputText.Remove(L'\r');
            if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url),
                                        pLogEntry->GetRevision()))
            {
                ShowErrorDialog(m_hWnd);
            }
            else
            {
                pLogEntry->SetMessage (CUnicodeUtils::StdGetUTF8
                    ( (LPCTSTR)dlg.m_sInputText));

                CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
                pMsgView->SetWindowText(L" ");
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
    EnableOKButton();
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
    CWnd * wndFocus = GetFocus();

    // Skip Ctrl-C when copying text out of the log message or search filter
    BOOL bSkipAccelerator = (pMsg->message == WM_KEYDOWN &&
                             (pMsg->wParam == L'C' || pMsg->wParam == VK_INSERT) &&
                             (wndFocus == GetDlgItem(IDC_MSGVIEW) || wndFocus == GetDlgItem(IDC_SEARCHEDIT)) &&
                             GetKeyState(VK_CONTROL) & 0x8000);
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            if (DWORD(CRegStdDWORD(L"Software\\TortoiseSVN\\CtrlEnter", TRUE)))
            {
                if (GetDlgItem(IDOK)->IsWindowVisible())
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
        if (wndFocus == GetDlgItem(IDC_LOGLIST))
        {
            if (CRegDWORD(L"Software\\TortoiseSVN\\DiffByDoubleClickInLog", FALSE))
            {
                DiffSelectedRevWithPrevious();
                return TRUE;
            }
        }
        if (wndFocus == GetDlgItem(IDC_LOGMSG))
        {
            DiffSelectedFile(false);
            return TRUE;
        }
        if (wndFocus == GetDlgItem(IDC_PROJTREE))
        {
            HTREEITEM hItem = m_projTree.GetSelectedItem();
            if (hItem)
                MonitorShowProject(hItem);
            return TRUE;
        }
        if (wndFocus == m_projTree.GetEditControl())
        {
            m_projTree.EndEditLabelNow(FALSE);
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
    if (m_bLogThreadRunning || (!m_bMonitorThreadRunning && (netScheduler.GetRunningThreadCount())))
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

void CLogDlg::SelectAllVisibleRevisions()
{
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
    msg.Replace(L"\r\n", L"\n");
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
            const std::tr1::wregex regMatch(m_ProjectProperties.GetLogRevRegex(),
                                        std::tr1::regex_constants::icase | std::tr1::regex_constants::ECMAScript);
            const std::tr1::wsregex_iterator end;
            std::wstring s = msg;
            for (std::tr1::wsregex_iterator it(s.begin(), s.end(), regMatch); it != end; ++it)
            {
                std::wstring matchedString = (*it)[0];
                const std::tr1::wregex regRevMatch(L"\\d+");
                std::wstring ss = matchedString;
                for (std::tr1::wsregex_iterator it2(ss.begin(), ss.end(), regRevMatch); it2 != end; ++it2)
                {
                    std::wstring matchedRevString = (*it2)[0];
                    if (url.Compare(matchedRevString.c_str()) == 0)
                    {
                        svn_revnum_t rev = _wtol(matchedRevString.c_str());
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": found revision %ld\n", rev);
                        // do we already show this revision? If yes, just select that
                        // revision and 'scroll' to it
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
                            PostMessage(WM_TSVN_REFRESH_SELECTION, 0, 0);
                            return;
                        }
                        try
                        {
                            CLogCacheUtility logUtil(GetLogCachePool()->GetCache(m_sUUID,
                                                    m_sRepositoryRoot), &m_ProjectProperties);
                            if (logUtil.IsCached(rev))
                            {
                                auto pLogItem = logUtil.GetRevisionData(rev);
                                if (pLogItem)
                                {
                                    // insert the data
                                    m_logEntries.Sort(CLogDataVector::RevisionCol, false);
                                    m_logEntries.AddSorted (pLogItem.release(), &m_ProjectProperties);

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
                                        PostMessage(WM_TSVN_REFRESH_SELECTION, 0, 0);
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
                        sCmd.Format(L"/command:log /path:\"%s\" /startrev:%ld /propspath:\"%s\"",
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
            POINTL pt;
            pMsgView->SendMessage(EM_POSFROMCHAR, (WPARAM)&pt, pEnLink->chrg.cpMin);
            rc.left = pt.x;
            rc.top = pt.y;
            pMsgView->SendMessage(EM_POSFROMCHAR, (WPARAM)&pt, pEnLink->chrg.cpMax);
            rc.right = pt.x;
            rc.bottom = pt.y+12;
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
        ShellExecute(this->m_hWnd, L"open", url, NULL, NULL, SW_SHOWDEFAULT);
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
        if (entry)
        {
            if (revisionsCovered.insert (entry->GetRevision()).second)
                revsByDate.insert (std::make_pair (entry->GetDate(), entry));
        }
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
                        // only change the background color if the item is not 'hot' (on vista
                        // with themes enabled)
                        if (!IsAppThemed() ||
                            ((pLVCD->nmcd.uItemState & CDIS_HOT)==0))
                            pLVCD->clrTextBk = GetSysColor(COLOR_MENU);
                    }
                    if (data->GetChangedPaths().ContainsCopies())
                        crText = m_Colors.GetColor(CColors::Modified);
                    if ((data->GetDepth())||(m_mergedRevs.find(data->GetRevision()) != m_mergedRevs.end()))
                        crText = GetSysColor(COLOR_GRAYTEXT);
                    if ((data->GetRevision() == m_wcRev) || data->GetUnread())
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
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top,
                                        m_hModifiedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_ADDED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER,
                                        rect.top, m_hAddedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_DELETED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER,
                                        rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_REPLACED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER,
                                    rect.top, m_hReplacedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_MOVED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left + nIcons*iconwidth + ICONITEMBORDER,
                        rect.top, m_hMovedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_MOVEREPLACED)
                        ::DrawIconEx(pLVCD->nmcd.hdc, rect.left + nIcons*iconwidth + ICONITEMBORDER,
                        rect.top, m_hMoveReplacedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                    nIcons++;

                    if ((pLogEntry->GetDepth()) ||
                        (m_mergedRevs.find(pLogEntry->GetRevision()) != m_mergedRevs.end()))
                    {
                        if (pLogEntry->IsSubtractiveMerge())
                            ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER,
                                rect.top, m_hReverseMergedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
                        else
                            ::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER,
                                    rect.top, m_hMergedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
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
                if (m_currentChangedPathList[pLVCD->nmcd.dwItemSpec].GetSVNPathString().Left
                                    (m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)!=0)
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
            if (action == LOGACTIONS_MOVED)
                crText = m_Colors.GetColor(CColors::Added);
            if (action == LOGACTIONS_MOVEREPLACED)
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

CRect CLogDlg::DrawListColumnBackground(CListCtrl& listCtrl, NMLVCUSTOMDRAW * pLVCD,
                                        PLOGENTRYDATA pLogEntry)
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
    if (IsAppThemed())
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

LRESULT CLogDlg::DrawListItemWithMatches(CListCtrl& listCtrl, NMLVCUSTOMDRAW * pLVCD,
                                         PLOGENTRYDATA pLogEntry)
{
    std::wstring text;
    text = (LPCTSTR)listCtrl.GetItemText((int)pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem);
    if (text.empty())
        return CDRF_DODEFAULT;

    std::wstring matchtext = text;
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
        if (IsAppThemed())
        {
            HTHEME hTheme = OpenThemeData(m_hWnd, L"LISTVIEW");
            GetThemeMetric(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, LISS_NORMAL, TMT_BORDERSIZE,
                                    &borderWidth);
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
            // At least it works on XP/Vista/win7/win8, and even with
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
            DrawText(pLVCD->nmcd.hdc, text.c_str(), -1, &rc, DT_CALCRECT|DT_SINGLELINE|
                                                            DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
            rect.left = rect.right-(rc.right-rc.left);
            if (!IsAppThemed())
            {
                rect.left += 2*borderWidth;
                rect.right += 2*borderWidth;
            }
        }
        COLORREF textColor = pLVCD->clrText;
        SetTextColor(pLVCD->nmcd.hdc, textColor);
        SetBkMode(pLVCD->nmcd.hdc, TRANSPARENT);
        for (std::vector<CHARRANGE>::iterator it = ranges.begin(); it != ranges.end(); ++it)
        {
            rc = rect;
            if (it->cpMin-drawPos)
            {
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMin-drawPos, &rc,
                            DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMin-drawPos, &rc,
                            DT_CALCRECT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
                rect.left = rc.right;
            }
            rc = rect;
            drawPos = it->cpMin;
            if (it->cpMax-drawPos)
            {
                SetTextColor(pLVCD->nmcd.hdc, m_Colors.GetColor(CColors::FilterMatch));
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMax-drawPos, &rc,
                            DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMax-drawPos, &rc,
                            DT_CALCRECT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
                rect.left = rc.right;
                SetTextColor(pLVCD->nmcd.hdc, textColor);
            }
            rc = rect;
            drawPos = it->cpMax;
        }
        DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), -1, &rc,
                            DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
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

void CLogDlg::DoSizeV3(int delta)
{
    RemoveMainAnchors();

    CSplitterControl::ChangeWidth(&m_projTree, delta, CW_LEFTALIGN);
    CSplitterControl::ChangeWidth(&m_cFilter, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_LogList, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_wndSplitter1, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_MSGVIEW), -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_wndSplitter2, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_ChangedFileListCtrl, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_LogProgress, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_LOGINFO), -delta, CW_RIGHTALIGN);

    AddMainAnchors();
    ArrangeLayout();
    AdjustMinSize();
    SetSplitterRange();
    GetDlgItem(IDC_MSGVIEW)->Invalidate();
    m_LogList.Invalidate();
    m_ChangedFileListCtrl.Invalidate();
    m_cFilter.Redraw();
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
        else if (wParam == IDC_SPLITTERLEFT)
        {
            SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
            DoSizeV3(pHdr->delta);
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
        m_wndSplitterLeft.SetRange(80, rcTop.right - m_LogListOrigRect.Width());
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
        popup.AppendMenu(MF_STRING | MF_ENABLED | (m_bFilterWithRegex ? MF_CHECKED : MF_UNCHECKED),
                            LOGFILTER_REGEX, temp);
        temp.LoadString(IDS_LOG_FILTER_CASESENSITIVE);
        popup.AppendMenu(MF_STRING | MF_ENABLED | (m_bFilterCaseSensitively ? MF_CHECKED : MF_UNCHECKED),
                            LOGFILTER_CASE, temp);

        m_tooltips.Pop();
        int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY |
                                                TPM_RIGHTBUTTON, point.x, point.y, this, 0);
        switch (selection)
        {
        case 0 :
            break;

        case LOGFILTER_REGEX :
            m_bFilterWithRegex = !m_bFilterWithRegex;
            CRegDWORD(L"Software\\TortoiseSVN\\UseRegexFilter") = (DWORD)m_bFilterWithRegex;
            CheckRegexpTooltip();
            break;

        case LOGFILTER_CASE:
            m_bFilterCaseSensitively = !m_bFilterCaseSensitively;
            CRegDWORD(L"Software\\TortoiseSVN\\FilterCaseSensitively") = (DWORD)m_bFilterCaseSensitively;
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
    CLogWndHourglass wait;
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

    GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_SEARCHEDIT)->SetFocus();

    AutoRestoreSelection();

    DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning)||(m_logEntries.GetVisibleCount() == 0))));

    return 0L;
}


void CLogDlg::SetFilterCueText()
{
    CString temp(MAKEINTRESOURCE(IDS_LOG_FILTER_BY));

    if (m_SelectedFilters & LOGFILTER_MESSAGES)
    {
        temp += L" ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_MESSAGES));
    }
    if (m_SelectedFilters & LOGFILTER_PATHS)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_PATHS));
    }
    if (m_SelectedFilters & LOGFILTER_AUTHORS)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_AUTHORS));
    }
    if (m_SelectedFilters & LOGFILTER_REVS)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_REVS));
    }
    if (m_SelectedFilters & LOGFILTER_BUGID)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_BUGIDS));
    }
    if (m_SelectedFilters & LOGFILTER_DATE)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_DATE));
    }
    if (m_SelectedFilters & LOGFILTER_DATERANGE)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_DATERANGE));
    }

    // to make the cue banner text appear more to the right of the edit control
    temp = L"   "+temp;
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
    }
    if (pItem->mask & LVIF_TEXT)
    {
        // By default, clear text buffer.
        lstrcpyn(pItem->pszText, L"", pItem->cchTextMax);

        bool bOutOfRange = pItem->iItem >= ShownCountWithStopped();

        if (m_bLogThreadRunning || bOutOfRange)
            return;
        if (pItem->cchTextMax == 0)
            return;

        // Which column?
        switch (pItem->iSubItem)
        {
        case 0: //revision
            if (pLogEntry)
            {
                swprintf_s(pItem->pszText, pItem->cchTextMax, L"%ld", pLogEntry->GetRevision());
                // to make the child entries indented, add spaces
                size_t len = wcslen(pItem->pszText);
                TCHAR * pBuf = pItem->pszText + len;
                DWORD nSpaces = m_logEntries.GetMaxDepth() - pLogEntry->GetDepth();
                while ((pItem->cchTextMax >= (int)len)&&(nSpaces))
                {
                    *pBuf = L' ';
                    pBuf++;
                    nSpaces--;
                }
                *pBuf = 0;
            }
            break;
        case 1: //action -- dummy text, not drawn. Used to trick the auto-column resizing to not
            // go below the icons
            if (pLogEntry)
                lstrcpyn(pItem->pszText, L"XXXXXXXXXXXXXXXX", pItem->cchTextMax);
            break;
        case 2: //author
            if (pLogEntry)
            {
                lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetAuthor()).c_str(),
                                pItem->cchTextMax);
            }
            break;
        case 3: //date
            if (pLogEntry)
            {
                lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetDateString()).c_str(),
                                pItem->cchTextMax);
            }
            break;
        case 4: //message or bug id
            if (m_bShowBugtraqColumn)
            {
                if (pLogEntry)
                {
                    lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetBugIDs()).c_str(),
                                pItem->cchTextMax);
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
                    lstrcpyn(pItem->pszText + pItem->cchTextMax - dots_len - 1, L"...", dots_len + 1);
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
            lstrcpyn(pItem->pszText, L"", pItem->cchTextMax);
        return;
    }
    if (m_bSingleRevision && ((size_t)pItem->iItem >= m_currentChangedArray.GetCount()))
    {
        if (pItem->mask & LVIF_TEXT)
            lstrcpyn(pItem->pszText, L"", pItem->cchTextMax);
        return;
    }
    if (!m_bSingleRevision && (pItem->iItem >= m_currentChangedPathList.GetCount()))
    {
        if (pItem->mask & LVIF_TEXT)
            lstrcpyn(pItem->pszText, L"", pItem->cchTextMax);
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
                           : L""
                     , pItem->cchTextMax);
            break;

        case 2: //copyfrom path
            lstrcpyn ( pItem->pszText
                     , m_bSingleRevision && m_currentChangedArray.GetCount() > (size_t)pItem->iItem
                           ? (LPCTSTR)m_currentChangedArray[pItem->iItem].GetCopyFromPath()
                           : L""
                     , pItem->cchTextMax);
            break;

        case 3: //revision
            svn_revnum_t revision = 0;
            if (m_bSingleRevision && m_currentChangedArray.GetCount() > (size_t)pItem->iItem)
                revision = m_currentChangedArray[pItem->iItem].GetCopyFromRev();

            if (revision == 0)
                lstrcpyn(pItem->pszText, L"", pItem->cchTextMax);
            else
                swprintf_s(pItem->pszText, pItem->cchTextMax, L"%ld", revision);
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
        CLogWndHourglass wait;

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

bool CLogDlg::ValidateRegexp(LPCTSTR regexp_str, std::tr1::wregex& pat, bool bMatchCase /* = false */)
{
    try
    {
        std::tr1::regex_constants::syntax_option_type type = std::tr1::regex_constants::ECMAScript;
        if (!bMatchCase)
            type |= std::tr1::regex_constants::icase;
        pat = std::tr1::wregex(regexp_str, type);
        return true;
    }
    catch (std::exception) {}
    return false;
}

bool CLogDlg::Validate(LPCTSTR string)
{
    if (!m_bFilterWithRegex)
        return true;
    std::tr1::wregex pat;
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
        bool bSetFocusToFilterControl = ((focusWnd != GetDlgItem(IDC_DATEFROM))&&
                                            (focusWnd != GetDlgItem(IDC_DATETO))
            && (focusWnd != GetDlgItem(IDC_LOGLIST)));
        if (m_sFilterText.IsEmpty())
        {
            DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning)||
                                                (m_logEntries.GetVisibleCount() == 0))));
            // do not return here!
            // we also need to run the filter if the filter text is empty:
            // 1. to clear an existing filter
            // 2. to rebuild the filtered list after sorting
        }
        CLogWndHourglass wait;
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
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
        if (bSetFocusToFilterControl)
            GetDlgItem(IDC_SEARCHEDIT)->SetFocus();

        AutoRestoreSelection();
    } // if (nIDEvent == LOGFILTER_TIMER)
    if (nIDEvent == MONITOR_TIMER)
        MonitorTimer();
    if (nIDEvent == MONITOR_POPUP_TIMER)
        MonitorPopupTimer();
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

CTSVNPathList CLogDlg::GetChangedPathsAndMessageSketchFromSelectedRevisions(CString& sMessageSketch,
                                                             CLogChangedPathArray& currentChangedArray)
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
            if (pLogEntry)
            {
                CString sRevMsg;
                sRevMsg.FormatMessage(m_sMultiLogFormat, //L"r%1!ld!\n%2!s!\n---------------------\n",
                                      pLogEntry->GetRevision(),
                                      (LPCWSTR)pLogEntry->GetShortMessageUTF16());
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

    CString selPath;
    POSITION pos = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
    if (pos)
    {
        int posindex = m_ChangedFileListCtrl.GetNextSelectedItem(pos);
        if (m_currentChangedArray.GetCount() > 0)
            selPath = m_currentChangedArray[posindex].GetPath();
        else
            selPath = m_currentChangedPathList[posindex].GetSVNPathString();
    }

    // clear the selection
    int iItem = -1;
    while ((iItem = m_ChangedFileListCtrl.GetNextItem(-1, LVNI_SELECTED)) >= 0)
        m_ChangedFileListCtrl.SetItemState(iItem, 0, LVIS_SELECTED);

    m_bAscendingPathList = nColumn == m_nSortColumnPathList ? !m_bAscendingPathList : TRUE;
    m_nSortColumnPathList = nColumn;
    if (m_currentChangedArray.GetCount() > 0)
        m_currentChangedArray.Sort (m_nSortColumnPathList, m_bAscendingPathList);
    else
        m_currentChangedPathList.SortByPathname (!m_bAscendingPathList);

    if (!selPath.IsEmpty())
    {
        if (m_currentChangedArray.GetCount() > 0)
        {
            for (int i = 0; i < (int)m_currentChangedArray.GetCount(); ++i)
            {
                if (selPath.Compare(m_currentChangedArray[i].GetPath())==0)
                {
                    m_ChangedFileListCtrl.SetSelectionMark(i);
                    m_ChangedFileListCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
                    m_ChangedFileListCtrl.EnsureVisible(i, FALSE);
                    break;
                }
            }
        }
        else
        {
            for (int i = 0; i < (int)m_currentChangedPathList.GetCount(); ++i)
            {
                if (selPath.Compare(m_currentChangedPathList[i].GetSVNPathString())==0)
                {
                    m_ChangedFileListCtrl.SetSelectionMark(i);
                    m_ChangedFileListCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
                    m_ChangedFileListCtrl.EnsureVisible(i, FALSE);
                    break;
                }
            }
        }

    }

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
        TCHAR textbuf[MAX_PATH + 1] = { 0 };
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
            const int nMinimumWidth = ICONITEMBORDER+16*7;
            if (cx < nMinimumWidth)
            {
                cx = nMinimumWidth;
            }
        }
        if ((col == 0) && m_bSelect)
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
        if (pLogEntry)
        {
            sRev.Format(L"%ld", pLogEntry->GetRevision());
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
        if (pLogEntry)
        {
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
    }
    CString sTemp;
    sTemp.FormatMessage(IDS_LOG_LOGINFOSTRING, m_logEntries.GetVisibleCount(), rev2, rev1, selectedrevs,
                            changedPaths);
    m_sLogInfo = sTemp;
    UpdateData(FALSE);
    GetDlgItem(IDC_LOGINFO)->Invalidate();
}


bool CLogDlg::VerifyContextMenuForRevisionsAllowed(int selIndex)
{
    if (selIndex < 0)
        return false; // nothing selected, nothing to do with a context menu

    // if the user selected the info text telling about not all revisions shown due to
    // the "stop on copy/rename" option, we also don't show the context menu
    if ((m_bStrictStopped)&&(selIndex == (int)m_logEntries.GetVisibleCount()))
        return false;

    return true;
}

void CLogDlg::AdjustContextMenuAnchorPointIfKeyboardInvoked(CPoint &point, int selIndex,
                                                            CListCtrl& listControl)
{
    // if the context menu is invoked through the keyboard, we have to use
    // a calculated position on where to anchor the menu on
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        listControl.GetItemRect(selIndex, &rect, LVIR_LABEL);
        listControl.ClientToScreen(&rect);
        point = rect.CenterPoint();
    }
}


bool CLogDlg::GetContextMenuInfoForRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    // calculate some information the context menu commands can use
    pCmi->PathURL = GetURLFromPath(m_path);
    CString relPathURL = pCmi->PathURL.Mid(m_sRepositoryRoot.GetLength());
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    int indexNext = m_LogList.GetNextSelectedItem(pos);
    if ((indexNext < 0)||(indexNext >= (int)m_logEntries.GetVisibleCount()))
        return false;
    pCmi->SelLogEntry = m_logEntries.GetVisible(indexNext);
    if (pCmi->SelLogEntry == NULL)
        return false;
    pCmi->RevSelected = pCmi->SelLogEntry->GetRevision();
    pCmi->RevPrevious = svn_revnum_t(pCmi->RevSelected)-1;

    const CLogChangedPathArray& paths = pCmi->SelLogEntry->GetChangedPaths();
    if (paths.GetCount() <= 2)
    {
        for (size_t i=0; i<paths.GetCount(); ++i)
        {
            const CLogChangedPath& changedpath = paths[i];
            if (changedpath.GetCopyFromRev() && (changedpath.GetPath().Compare(relPathURL)==0))
                pCmi->RevPrevious = changedpath.GetCopyFromRev();
        }
    }

    if (pos)
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (m_LogList.GetNextSelectedItem(pos));
        if (pLogEntry)
            pCmi->RevSelected2 = pLogEntry->GetRevision();
    }
    pCmi->AllFromTheSameAuthor = true;

    std::vector<svn_revnum_t> revisions;
    revisions.reserve (m_logEntries.GetVisibleCount());

    POSITION pos2 = m_LogList.GetFirstSelectedItemPosition();
    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos2));
    if (pLogEntry == NULL)
        return false;
    revisions.push_back (pLogEntry->GetRevision());
    pCmi->SelEntries.push_back(pLogEntry);

    const std::string& firstAuthor = pLogEntry->GetAuthor();
    while (pos2)
    {
        int index2 = m_LogList.GetNextSelectedItem(pos2);
        if (index2 < (int)m_logEntries.GetVisibleCount())
        {
            pLogEntry = m_logEntries.GetVisible(index2);
            if (pLogEntry)
            {
                revisions.push_back (pLogEntry->GetRevision());
                pCmi->SelEntries.push_back(pLogEntry);
                if (firstAuthor != pLogEntry->GetAuthor())
                    pCmi->AllFromTheSameAuthor = false;
            }
        }
    }

    pCmi->RevisionRanges.AddRevisions (revisions);
    pCmi->RevLowest = pCmi->RevisionRanges.GetLowestRevision();
    pCmi->RevHighest = pCmi->RevisionRanges.GetHighestRevision();

    return true;
}

void CLogDlg::PopulateContextMenuForRevisions(ContextMenuInfoForRevisionsPtr& pCmi, CIconMenu& popup, CIconMenu& clipSubMenu)
{
    if ((m_LogList.GetSelectedCount() == 1) && (pCmi->SelLogEntry->GetDepth()==0))
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
            if (m_hasWC)
            {
                popup.AppendMenuIcon(ID_OPENLOCAL, IDS_LOG_POPUP_OPENLOCAL, IDI_OPEN);
                popup.AppendMenuIcon(ID_OPENWITHLOCAL, IDS_LOG_POPUP_OPENWITHLOCAL, IDI_OPEN);
            }
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
            popup.AppendMenuIcon(ID_MERGEREV, IDS_LOG_POPUP_MERGEREVS, IDI_MERGE);
            bAddSeparator = true;
        }
        if (bAddSeparator)
            popup.AppendMenu(MF_SEPARATOR, NULL);
    }

    if ((!pCmi->SelEntries.empty())&&(pCmi->AllFromTheSameAuthor))
    {
        popup.AppendMenuIcon(ID_EDITAUTHOR, IDS_LOG_POPUP_EDITAUTHOR);
    }
    if (m_LogList.GetSelectedCount() == 1)
    {
        popup.AppendMenuIcon(ID_EDITLOG, IDS_LOG_POPUP_EDITLOG);
        // "Show Revision Properties"
        popup.AppendMenuIcon(ID_REVPROPS, IDS_REPOBROWSE_SHOWREVPROP, IDI_PROPERTIES);
        popup.AppendMenu(MF_SEPARATOR, NULL);

    }
    if (m_LogList.GetSelectedCount() != 0)
    {
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDFULL, IDS_LOG_POPUP_CLIPBOARD_FULL, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDFULLNOPATHS, IDS_LOG_POPUP_CLIPBOARD_FULLNOPATHS, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDREVS, IDS_LOG_POPUP_CLIPBOARD_REVS, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDAUTHORS, IDS_LOG_POPUP_CLIPBOARD_AUTHORS, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDMESSAGES, IDS_LOG_POPUP_CLIPBOARD_MSGS, IDI_COPYCLIP);

        CString temp;
        temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
        popup.InsertMenu((UINT)-1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)clipSubMenu.m_hMenu, temp);
    }
    popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND, IDI_FILTEREDIT);
    // this menu shows only if Code Collaborator Installed & Registry configured
    if (CodeCollaboratorInfo::IsInstalled())
        popup.AppendMenuIcon(ID_CODE_COLLABORATOR, IDS_LOG_CODE_COLLABORATOR,
                                                        IDI_CODE_COLLABORATOR);
}

void CLogDlg::ShowContextMenuForRevisions(CWnd* /*pWnd*/, CPoint point)
{
    m_bCancelled = FALSE;
    int selIndex = m_LogList.GetSelectionMark();
    if (!VerifyContextMenuForRevisionsAllowed(selIndex))
        return;
    AdjustContextMenuAnchorPointIfKeyboardInvoked(point, selIndex, m_LogList);

    m_nSearchIndex = selIndex;

    // grab extra revision info that the Execute methods will use
    ContextMenuInfoForRevisionsPtr pCmi(new CContextMenuInfoForRevisions());
    if (!GetContextMenuInfoForRevisions(pCmi))
        return;

    CIconMenu popup;
    if (!popup.CreatePopupMenu())
       return;
    CIconMenu clipSubMenu;
    if (!clipSubMenu.CreatePopupMenu())
        return;
    // get the menu items
    PopulateContextMenuForRevisions(pCmi, popup, clipSubMenu);

    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY |
                                        TPM_RIGHTBUTTON, point.x, point.y, this, 0);
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    CLogWndHourglass wait;

    switch (cmd)
    {
        case ID_GNUDIFF1:
            ExecuteGnuDiff1MenuRevisions(pCmi);
            break;
        case ID_GNUDIFF2:
            ExecuteGnuDiff2MenuRevisions(pCmi);
            break;
        case ID_REVERTREV:
            ExecuteRevertRevisionMenuRevisions(pCmi);
            break;
        case ID_MERGEREV:
            ExecuteMergeRevisionMenuRevisions(pCmi);
            break;
        case ID_REVERTTOREV:
            ExecuteRevertToRevisionMenuRevisions(pCmi);
            break;
        case ID_COPY:
            ExecuteCopyMenuRevisions(pCmi);
            break;
        case ID_COMPARE:
            ExecuteCompareWithWorkingCopyMenuRevisions(pCmi);
            break;
        case ID_COMPARETWO:
            ExecuteCompareTwoMenuRevisions(pCmi);
            break;
        case ID_COMPAREWITHPREVIOUS:
            ExecuteCompareWithPreviousMenuRevisions(pCmi);
            break;
        case ID_BLAMECOMPARE:
            ExecuteBlameCompareMenuRevisions(pCmi);
            break;
        case ID_BLAMETWO:
            ExecuteBlameTwoMenuRevisions(pCmi);
            break;
        case ID_BLAMEWITHPREVIOUS:
            ExecuteWithPreviousMenuRevisions(pCmi);
            break;
        case ID_SAVEAS:
            ExecuteSaveAsMenuRevisions(pCmi);
            break;
        case ID_OPENWITH:
            ExecuteOpenMenuRevisions(pCmi, true);
            break;
        case ID_OPEN:
            ExecuteOpenMenuRevisions(pCmi, false);
            break;
        case ID_OPENWITHLOCAL:
            DoOpenFileWith(false, true, m_path);
            break;
        case ID_OPENLOCAL:
            DoOpenFileWith(false, false, m_path);
            break;
        case ID_BLAME:
            ExecuteBlameMenuRevisions(pCmi);
            break;
        case ID_UPDATE:
            ExecuteUpdateMenuRevisions(pCmi);
            break;
        case ID_FINDENTRY:
            ExecuteFindEntryMenuRevisions();
            break;
        case ID_REPOBROWSE:
            ExecuteRepoBrowseMenuRevisions(pCmi);
            break;
        case ID_EDITLOG:
            EditLogMessage(selIndex);
            break;
        case ID_EDITAUTHOR:
            EditAuthor(pCmi->SelEntries);
            break;
        case ID_REVPROPS:
            ExecuteRevisionPropsMenuRevisions(pCmi);
            break;
        case ID_COPYCLIPBOARDFULL:
            CopySelectionToClipBoard(true);
            break;
        case ID_COPYCLIPBOARDFULLNOPATHS:
            CopySelectionToClipBoard(false);
            break;
        case ID_COPYCLIPBOARDREVS:
            CopyCommaSeparatedRevisionsToClipboard();
            break;
        case ID_COPYCLIPBOARDAUTHORS:
            CopyCommaSeparatedAuthorsToClipboard();
            break;
        case ID_COPYCLIPBOARDMESSAGES:
            CopyMessagesToClipboard();
            break;
        case ID_EXPORT:
            ExecuteExportMenuRevisions(pCmi);
            break;
        case ID_CHECKOUT:
            ExecuteCheckoutMenuRevisions(pCmi);
            break;
        case ID_VIEWREV:
            ExecuteViewRevMenuRevisions(pCmi);
            break;
        case ID_VIEWPATHREV:
            ExecuteViewPathRevMenuRevisions(pCmi);
            break;
        case ID_CODE_COLLABORATOR:
            ExecuteAddCodeCollaboratorReview();
            break;
        default:
            break;
    } // switch (cmd)


    EnableOKButton();
}

CString CLogDlg::GetUrlOfTrunk()
{
    // this may not work for SVNSERVE based repos...
    CString returnedString = L"";
    CString repositoryRootUrl = GetRepositoryRoot(m_path);
    CString selectedUrl = GetSUrl();
    int slashPos = selectedUrl.Find(L"/", repositoryRootUrl.GetLength() + 1);
    if (slashPos == -1)
        return selectedUrl;
    returnedString = selectedUrl.Left(slashPos);
    return returnedString;
}

void CLogDlg::ExecuteAddCodeCollaboratorReview()
{
    CString revisions;
    CString commandLine;

    revisions = GetSpaceSeparatedSelectedRevisions();
    if (revisions.IsEmpty())
        return;
    CodeCollaboratorInfo codeCollaborator (revisions, GetUrlOfTrunk());
    if (!codeCollaborator.IsUserInfoSet() || (GetKeyState(VK_CONTROL) & 0x8000))
    {
        CodeCollaboratorSettingsDlg dlg(this);
        dlg.DoModal();
        return;
    }

    CAppUtils::LaunchApplication(codeCollaborator.GetCommandLine(), NULL, false);
}

CString CLogDlg::GetSpaceSeparatedSelectedRevisions()
{
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    CString sRevisions = L"";
    CString sRevision = L"";

    if (pos != NULL)
    {
        while(pos)
        {
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index >= (int)m_logEntries.GetVisibleCount())
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (index);
            if (pLogEntry)
            {
                sRevision.Format(L"%ld ", pLogEntry->GetRevision());
                sRevisions += sRevision;
            }
        }
    }
    return sRevisions;
}

void CLogDlg::ExecuteGnuDiff1MenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CString options;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        CDiffOptionsDlg dlg(this);
        if (dlg.DoModal() == IDOK)
            options = dlg.GetDiffOptionsString();
        else
            return;
    }
    if (PromptShown())
    {
        auto f = [=]()
        {
            CoInitialize(NULL);
            this->EnableWindow(FALSE);

            SVNDiff diff(this, this->m_hWnd, true);
            diff.SetHEADPeg(m_LogRevision);
            diff.ShowUnifiedDiff(m_path, pCmi->RevPrevious, m_path, pCmi->RevSelected, SVNRev(), options);

            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowUnifiedDiff(m_hWnd, m_path, pCmi->RevPrevious, m_path,
        pCmi->RevSelected, SVNRev(), m_LogRevision, options);
}

void CLogDlg::ExecuteGnuDiff2MenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    SVNRev r1 = pCmi->RevSelected;
    SVNRev r2 = pCmi->RevSelected2;
    if (m_LogList.GetSelectedCount() > 2)
    {
        r1 = pCmi->RevHighest;
        r2 = pCmi->RevLowest;
    }
    CString options;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        CDiffOptionsDlg dlg(this);
        if (dlg.DoModal() == IDOK)
            options = dlg.GetDiffOptionsString();
        else
            return;
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

void CLogDlg::ExecuteRevertRevisionMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    // we need an URL to complete this command, so error out if we can't get an URL
    if (pCmi->PathURL.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetUIPathString());
        return;      //exit
    }

    if (ConfirmRevert(m_path.GetUIPathString()))
    {
        CSVNProgressDlg dlg;
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetOptions(ProgOptIgnoreAncestry);
        dlg.SetPathList(CTSVNPathList(m_path));
        dlg.SetUrl(pCmi->PathURL);
        dlg.SetSecondUrl(pCmi->PathURL);
        pCmi->RevisionRanges.AdjustForMerge(true);
        dlg.SetRevisionRanges(pCmi->RevisionRanges);
        dlg.SetPegRevision(m_LogRevision);
        dlg.DoModal();
    }
}

void CLogDlg::ExecuteMergeRevisionMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    // we need an URL to complete this command, so error out if we can't get an URL
    if (pCmi->PathURL.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetUIPathString());
        return;      //exit
    }

    CString path = m_path.GetWinPathString();
    bool bGotSavePath = false;
    if ((m_LogList.GetSelectedCount() == 1)&&(!m_path.IsDirectory()))
    {
        bGotSavePath = CAppUtils::FileOpenSave(path, NULL, IDS_LOG_MERGETO, IDS_COMMONFILEFILTER,
            true, m_path.GetDirectory().GetWinPathString(), GetSafeHwnd());
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
                CString sTask1;
                sTask1.Format(IDS_MERGE_WCDIRTYASK_TASK1, (LPCTSTR)path);
                CTaskDialog taskdlg(sTask1,
                                    CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION |
                                    TDF_POSITION_RELATIVE_TO_WINDOW);
                taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK3)));
                taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK4)));
                taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskdlg.SetDefaultCommandControl(2);
                taskdlg.SetMainIcon(TD_WARNING_ICON);
                if (taskdlg.DoModal(m_hWnd) != 1)
                    return;
            }
        }
        CSVNProgressDlg dlg;
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetPathList(CTSVNPathList(CTSVNPath(path)));
        dlg.SetUrl(pCmi->PathURL);
        dlg.SetSecondUrl(pCmi->PathURL);
        pCmi->RevisionRanges.AdjustForMerge(false);
        dlg.SetRevisionRanges(pCmi->RevisionRanges);
        dlg.SetPegRevision(m_LogRevision);
        dlg.DoModal();
    }
}

void CLogDlg::ExecuteRevertToRevisionMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    // we need an URL to complete this command, so error out if we can't get an URL
    if (pCmi->PathURL.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetUIPathString());
        return;      //exit
    }

    if (ConfirmRevert(m_path.GetWinPath(), true))
    {
        CSVNProgressDlg dlg;
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetOptions(ProgOptIgnoreAncestry);
        dlg.SetPathList(CTSVNPathList(m_path));
        dlg.SetUrl(pCmi->PathURL);
        dlg.SetSecondUrl(pCmi->PathURL);
        SVNRevRangeArray revarray;
        revarray.AddRevRange(SVNRev::REV_HEAD, pCmi->RevSelected);
        dlg.SetRevisionRanges(revarray);
        dlg.SetPegRevision(m_LogRevision);
        dlg.DoModal();
    }
}

void CLogDlg::ExecuteCopyMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    // we need an URL to complete this command, so error out if we can't get an URL
    if (pCmi->PathURL.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetUIPathString());
        return;      //exit
    }

    CCopyDlg dlg;
    dlg.m_URL = pCmi->PathURL;
    dlg.m_path = m_path;
    dlg.m_CopyRev = pCmi->RevSelected;
    if (dlg.DoModal() == IDOK)
    {
        CTSVNPath url = CTSVNPath(dlg.m_URL);
        SVNRev copyrev = dlg.m_CopyRev;
        CString logmsg = dlg.m_sLogMessage;
        SVNExternals exts = dlg.GetExternalsToTag();
        bool bMakeParents = !!dlg.m_bMakeParents;
        auto f = [=]() mutable
        {
            CoInitialize(NULL);
            this->EnableWindow(FALSE);

            // should we show a progress dialog here? Copies are done really fast
            // and without much network traffic.
            if (!Copy(CTSVNPathList(CTSVNPath(pCmi->PathURL)), url, copyrev, copyrev, logmsg, bMakeParents, bMakeParents))
                ShowErrorDialog(m_hWnd);
            else
            {
                if (!exts.TagExternals(true, CString(MAKEINTRESOURCE(IDS_COPY_COMMITMSG)),
                    m_commitRev, CTSVNPath(pCmi->PathURL), url))
                {
                    ShowErrorDialog(m_hWnd, CTSVNPath(), exts.GetLastErrorString());
                }
                else
                    TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_SUCCESS), MAKEINTRESOURCE(IDS_LOG_COPY_SUCCESS), TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
            }

            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteCompareWithWorkingCopyMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
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
            diff.ShowCompare(m_path, SVNRev::REV_WC, m_path, pCmi->RevSelected, SVNRev(), false, L"");

            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowCompare(m_hWnd, m_path, SVNRev::REV_WC, m_path,
        pCmi->RevSelected, SVNRev(), m_LogRevision,
        false, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
}

void CLogDlg::ExecuteCompareTwoMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    SVNRev r1 = pCmi->RevSelected;
    SVNRev r2 = pCmi->RevSelected2;
    if (m_LogList.GetSelectedCount() > 2)
    {
        r1 = pCmi->RevHighest;
        r2 = pCmi->RevLowest;
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
            diff.ShowCompare(CTSVNPath(pCmi->PathURL), r2, CTSVNPath(pCmi->PathURL),
                r1, SVNRev(), false, L"", false, false, nodekind);

            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pCmi->PathURL), r2, CTSVNPath(pCmi->PathURL), r1,
        SVNRev(), m_LogRevision, false, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000),
        false, false, nodekind);
}

void CLogDlg::ExecuteCompareWithPreviousMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
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
            diff.ShowCompare(CTSVNPath(pCmi->PathURL), pCmi->RevPrevious,
                CTSVNPath(pCmi->PathURL), pCmi->RevSelected, SVNRev(),
                false, L"", false, false, nodekind);

            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pCmi->PathURL),
        pCmi->RevPrevious, CTSVNPath(pCmi->PathURL), pCmi->RevSelected,
        SVNRev(), m_LogRevision, false, L"",
        !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), false, false, nodekind);
}

void CLogDlg::ExecuteBlameCompareMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
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
            diff.ShowCompare(m_path, SVNRev::REV_BASE, m_path, pCmi->RevSelected, SVNRev(), false, L"", false, true);

            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowCompare(m_hWnd, m_path, SVNRev::REV_BASE, m_path,
        pCmi->RevSelected, SVNRev(), m_LogRevision, false, L"", false, false, true);
}

void CLogDlg::ExecuteBlameTwoMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
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
            diff.ShowCompare(CTSVNPath(pCmi->PathURL), pCmi->RevSelected2,
                CTSVNPath(pCmi->PathURL), pCmi->RevSelected, SVNRev(),
                false, L"", false, true, nodekind);

            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pCmi->PathURL),
        pCmi->RevSelected, CTSVNPath(pCmi->PathURL), pCmi->RevSelected,
        SVNRev(), m_LogRevision, false, L"", false, false, true, nodekind);
}


void CLogDlg::ExecuteWithPreviousMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
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
            diff.ShowCompare(CTSVNPath(pCmi->PathURL), pCmi->RevPrevious,
                CTSVNPath(pCmi->PathURL), pCmi->RevSelected, SVNRev(),
                false, L"", false, true, nodekind);
        };
        new async::CAsyncCall(f, &netScheduler);
        netScheduler.WaitForEmptyQueue();
    }
    else
        CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pCmi->PathURL),
        pCmi->RevPrevious, CTSVNPath(pCmi->PathURL), pCmi->RevSelected,
        SVNRev(), m_LogRevision, false, L"", false, false, true, nodekind);
}

void CLogDlg::ExecuteSaveAsMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    //now first get the revision which is selected
    CString revFilename;
    if (m_hasWC)
    {
        CString strWinPath = m_path.GetWinPathString();
        int rfind = strWinPath.ReverseFind('.');
        if (rfind > 0)
            revFilename.Format(L"%s-%s%s", (LPCTSTR)strWinPath.Left(rfind),
            (LPCTSTR)pCmi->RevSelected.ToString(), (LPCTSTR)strWinPath.Mid(rfind));
        else
            revFilename.Format(L"%s-%s", (LPCTSTR)strWinPath, (LPCTSTR)pCmi->RevSelected.ToString());
    }
    if (CAppUtils::FileOpenSave(revFilename, NULL, IDS_LOG_POPUP_SAVE, IDS_COMMONFILEFILTER, false, m_path.GetDirectory().GetWinPathString(), m_hWnd))
    {
        auto f = [=]()
        {
            CoInitialize(NULL);
            this->EnableWindow(FALSE);

            CTSVNPath tempfile;
            tempfile.SetFromWin(revFilename);
            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            CString sInfoLine;
            sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(),
                (LPCTSTR)pCmi->RevSelected.ToString());
            progDlg.SetLine(1, sInfoLine, true);
            SetAndClearProgressInfo(&progDlg);
            progDlg.ShowModeless(m_hWnd);
            if (!Export(m_path, tempfile, SVNRev(SVNRev::REV_HEAD), pCmi->RevSelected))
            {
                // try again with another peg revision
                if (!Export(m_path, tempfile, pCmi->RevSelected, pCmi->RevSelected))
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

void CLogDlg::ExecuteOpenMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi, bool bOpenWith)
{
    auto f = [=]()
    {
        CoInitialize(NULL);
        this->EnableWindow(FALSE);

        CProgressDlg progDlg;
        progDlg.SetTitle(IDS_APPNAME);
        CString sInfoLine;
        sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(),
            (LPCTSTR)pCmi->RevSelected.ToString());
        progDlg.SetLine(1, sInfoLine, true);
        SetAndClearProgressInfo(&progDlg);
        progDlg.ShowModeless(m_hWnd);
        CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, m_path, pCmi->RevSelected);
        bool bSuccess = true;
        if (!Export(m_path, tempfile, SVNRev(SVNRev::REV_HEAD), pCmi->RevSelected))
        {
            bSuccess = false;
            // try again, but with the selected revision as the peg revision
            if (!Export(m_path, tempfile, pCmi->RevSelected, pCmi->RevSelected))
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
            DoOpenFileWith(true, bOpenWith, tempfile);
        }

        this->EnableWindow(TRUE);
        this->SetFocus();
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteBlameMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CBlameDlg dlg;
    dlg.EndRev = pCmi->RevSelected;
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
            tempfile = blame.BlameToTempFile(m_path, startrev, endrev, m_pegrev,
                L"", includeMerge, TRUE, TRUE);
            if (!tempfile.IsEmpty())
            {
                if (textViewer)
                {
                    //open the default text editor for the result file
                    CAppUtils::StartTextViewer(tempfile);
                }
                else
                {
                    CString sParams = L"/path:\"" + m_path.GetSVNPathString() + L"\" ";
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

void CLogDlg::ExecuteUpdateMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CString sCmd;
    sCmd.Format(L"/command:update /path:\"%s\" /rev:%ld",
        (LPCTSTR)m_path.GetWinPath(), (LONG)pCmi->RevSelected);
    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteFindEntryMenuRevisions()
{
    m_nSearchIndex = m_LogList.GetSelectionMark();
    if (m_nSearchIndex < 0)
        m_nSearchIndex = 0;
    CreateFindDialog();
}

void CLogDlg::ExecuteRepoBrowseMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CString sCmd;
    sCmd.Format(L"/command:repobrowser /path:\"%s\" /rev:%s",
        (LPCTSTR)pCmi->PathURL, (LPCTSTR)pCmi->RevSelected.ToString());

    CAppUtils::RunTortoiseProc(sCmd);
}


void CLogDlg::ExecuteRevisionPropsMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CEditPropertiesDlg dlg;
    dlg.SetProjectProperties(&m_ProjectProperties);
    dlg.SetPathList(CTSVNPathList(CTSVNPath(pCmi->PathURL)));
    dlg.SetRevision(pCmi->RevSelected);
    dlg.RevProps(true);
    dlg.DoModal();
}

void CLogDlg::ExecuteExportMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CString sCmd;
    sCmd.Format(L"/command:export /path:\"%s\" /revision:%ld",
        (LPCTSTR)pCmi->PathURL, (LONG)pCmi->RevSelected);
    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteCheckoutMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CString sCmd;
    CString url = L"tsvn:"+pCmi->PathURL;
    sCmd.Format(L"/command:checkout /url:\"%s\" /revision:%ld",
        (LPCTSTR)url, (LONG)pCmi->RevSelected);
    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteViewRevMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CString url = m_ProjectProperties.sWebViewerRev;
    url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
    url.Replace(L"%REVISION%", pCmi->RevSelected.ToString());
    if (!url.IsEmpty())
        ShellExecute(this->m_hWnd, L"open", url, NULL, NULL, SW_SHOWDEFAULT);
}

void CLogDlg::ExecuteViewPathRevMenuRevisions(ContextMenuInfoForRevisionsPtr& pCmi)
{
    CString relurl = pCmi->PathURL;
    CString sRoot = GetRepositoryRoot(CTSVNPath(relurl));
    relurl = relurl.Mid(sRoot.GetLength());
    CString url = m_ProjectProperties.sWebViewerPathRev;
    url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
    url.Replace(L"%REVISION%", pCmi->RevSelected.ToString());
    url.Replace(L"%PATH%", relurl);
    if (!url.IsEmpty())
        ShellExecute(this->m_hWnd, L"open", url, NULL, NULL, SW_SHOWDEFAULT);
}

void CLogDlg::ShowContextMenuForChangedPaths(CWnd* /*pWnd*/, CPoint point)
{
    m_bCancelled = false;
    INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();
    AdjustContextMenuAnchorPointIfKeyboardInvoked(point, (int)selIndex, m_ChangedFileListCtrl);

    if (!VerifyContextMenuForChangedPathsAllowed(selIndex))
        return;

    // grab extra revision info that the Execute methods will use
    ContextMenuInfoForChangedPathsPtr pCmi(new CContextMenuInfoForChangedPaths());
    if (!GetContextMenuInfoForChangedPaths(pCmi))
        return;

    // need this shortcut that can't easily go into the helper class
    const CLogChangedPath& changedlogpath = m_currentChangedArray[selIndex];

    //entry is selected, now show the popup menu
    CIconMenu popup;
    CIconMenu clipSubMenu;
    if (!clipSubMenu.CreatePopupMenu())
        return;

    PopulateContextMenuForChangedPaths(pCmi, popup, clipSubMenu);
    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY |
                                    TPM_RIGHTBUTTON, point.x, point.y, this, 0);
    CLogWndHourglass wait;

    switch (cmd)
    {
    case ID_DIFF:
        ExecuteDiffChangedPaths(pCmi, selIndex, false);
        break;
    case ID_DIFF_CONTENTONLY:
        ExecuteDiffChangedPaths(pCmi, selIndex, true);
        break;
    case ID_BLAMEDIFF:
        ExecuteBlameDiffChangedPaths(selIndex, pCmi);
        break;
    case ID_GNUDIFF1:
        ExecuteGnuDiff1ChangedPaths(selIndex, pCmi);
        break;
    case ID_REVERTREV:
        ExecuteRevertChangedPaths(pCmi, changedlogpath);
        break;
    case ID_POPPROPS:
        ExecuteShowPropertiesChangedPaths(pCmi);
        break;
    case ID_SAVEAS:
        ExecuteSaveAsChangedPaths(pCmi, selIndex);
        break;
    case ID_EXPORTTREE:
        ExecuteExportTreeChangedPaths(pCmi);
        break;
    case ID_OPENWITH:
        ExecuteOpenChangedPaths(selIndex, pCmi, true);
        break;
    case ID_OPEN:
        ExecuteOpenChangedPaths(selIndex, pCmi, false);
        break;
    case ID_OPENWITHLOCAL:
        DoOpenFileWith(false, true, CTSVNPath(pCmi->wcPath));
        break;
    case ID_OPENLOCAL:
        DoOpenFileWith(false, false, CTSVNPath(pCmi->wcPath));
        break;
    case ID_DIFF_MULTIPLE:
        ExecuteMultipleDiffChangedPaths(pCmi, false);
        break;
    case ID_DIFF_MULTIPLE_CONTENTONLY:
        ExecuteMultipleDiffChangedPaths(pCmi, true);
        break;
    case ID_OPENLOCAL_MULTIPLE:
        if (((GetKeyState(VK_CONTROL) & 0x8000) && m_bVisualStudioRunningAtStart == true))
            OpenSelectedWcFilesWithVisualStudio(pCmi->ChangedLogPathIndices);
        else
            OpenSelectedWcFilesWithRegistedProgram(pCmi->ChangedLogPathIndices);
        break;
    case ID_BLAME:
        ExecuteBlameChangedPaths(pCmi, changedlogpath);
        break;
    case ID_GETMERGELOGS:
        ExecuteShowLogChangedPaths(pCmi, changedlogpath, true);
        break;
    case ID_LOG:
        ExecuteShowLogChangedPaths(pCmi, changedlogpath, false);
        break;
    case ID_REPOBROWSE:
        ExecuteBrowseRepositoryChangedPaths(pCmi, changedlogpath);
        break;
    case ID_VIEWPATHREV:
        ExecuteViewPathRevisionChangedPaths(selIndex);
        break;
    case ID_COPYCLIPBOARDURL:
    case ID_COPYCLIPBOARDRELPATH:
    case ID_COPYCLIPBOARDFILENAMES:
        CopyChangedPathInfoToClipboard(pCmi, cmd);
        break;

    default:
        break;
    } // switch (cmd)
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
    if ((pLVKeyDown->wVKey == 'A') && (GetKeyState (VK_CONTROL) < 0))
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

CString CLogDlg::GetToolTipText(int nItem, int nSubItem)
{
    if ((nSubItem == 1) && (m_logEntries.GetVisibleCount() > (size_t)nItem))
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible (nItem);
        if (pLogEntry == NULL)
            return CString();

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

        if (actions & LOGACTIONS_MOVED)
        {
            if (!actionText.empty())
                actionText += "\r\n";
            actionText += CLogChangedPath::GetActionString(LOGACTIONS_MOVED);
        }

        if (actions & LOGACTIONS_MOVEREPLACED)
        {
            if (!actionText.empty())
                actionText += "\r\n";
            actionText += CLogChangedPath::GetActionString(LOGACTIONS_MOVEREPLACED);
        }

        sToolTipText = CUnicodeUtils::GetUnicode(actionText.c_str());
        if ((pLogEntry->GetDepth())||(m_mergedRevs.find(pLogEntry->GetRevision()) != m_mergedRevs.end()))
        {
            if (!sToolTipText.IsEmpty())
                sToolTipText += L"\r\n";
            if (pLogEntry->IsSubtractiveMerge())
                sToolTipText += CString(MAKEINTRESOURCE(IDS_LOG_ALREADYMERGEDREVERSED));
            else
                sToolTipText += CString(MAKEINTRESOURCE(IDS_LOG_ALREADYMERGED));
        }

        if (!sToolTipText.IsEmpty())
        {
            CString sTitle(MAKEINTRESOURCE(IDS_LOG_ACTIONS));
            sToolTipText = sTitle + L":\r\n" + sToolTipText;
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
        if (m_bSelect)
            EnableOKButton();
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
                        sHelpText += L", ";
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
    AddAnchor(IDC_PROJTREE, TOP_LEFT, BOTTOM_LEFT);
    AddAnchor(IDC_SPLITTERLEFT, TOP_LEFT, BOTTOM_LEFT);
    AddAnchor(IDC_LOGLIST, TOP_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_SPLITTERTOP, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_MSGVIEW, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_SPLITTERBOTTOM, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_LOGMSG, MIDDLE_LEFT, BOTTOM_RIGHT);
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
    if (m_bMonitoringMode)
        AdjustDateFilterVisibility();
}

void CLogDlg::RemoveMainAnchors()
{
    RemoveAnchor(IDC_PROJTREE);
    RemoveAnchor(IDC_SPLITTERLEFT);
    RemoveAnchor(IDC_SEARCHEDIT);
    RemoveAnchor(IDC_LOGLIST);
    RemoveAnchor(IDC_SPLITTERTOP);
    RemoveAnchor(IDC_MSGVIEW);
    RemoveAnchor(IDC_SPLITTERBOTTOM);
    RemoveAnchor(IDC_LOGMSG);
    RemoveAnchor(IDC_LOGINFO);
    RemoveAnchor(IDC_SHOWPATHS);
    RemoveAnchor(IDC_CHECK_STOPONCOPY);
    RemoveAnchor(IDC_INCLUDEMERGE);
    RemoveAnchor(IDC_GETALL);
    RemoveAnchor(IDC_NEXTHUNDRED);
    RemoveAnchor(IDC_REFRESH);
    RemoveAnchor(IDC_STATBUTTON);
    RemoveAnchor(IDC_HIDENONMERGEABLE);
    RemoveAnchor(IDC_PROGRESS);
    RemoveAnchor(IDOK);
    RemoveAnchor(IDC_LOGCANCEL);
    RemoveAnchor(IDHELP);
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
    ::MessageBox(this->m_hWnd, messageString, L"TortoiseSVN", MB_ICONERROR);
}

bool CLogDlg::ConfirmRevert( const CString& path, bool bToRev /*= false*/ )
{
    CString msg;
    if (bToRev)
        msg.Format(IDS_LOG_REVERT_CONFIRM_TASK6, (LPCTSTR)path);
    else
        msg.Format(IDS_LOG_REVERT_CONFIRM_TASK1, (LPCTSTR)path);
    CTaskDialog taskdlg(msg,
                        CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK2)),
                        L"TortoiseSVN",
                        0,
                        TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION |
                        TDF_POSITION_RELATIVE_TO_WINDOW);
    taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK3)));
    taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK4)));
    taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskdlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK5)));
    taskdlg.SetDefaultCommandControl(2);
    taskdlg.SetMainIcon(TD_INFORMATION_ICON);
    return (taskdlg.DoModal(m_hWnd) == 1);
}

// this to be called on a thread so we don't delay the startup of the dialog
// and we can take advantage of multiple cores...
void CLogDlg::DetectVisualStudioRunningThread()
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    m_bVisualStudioRunningAtStart = false;

    CAutoGeneralHandle snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry))
    {
        while (Process32Next(snapshot, &entry))
        {
            if (!_wcsicmp(entry.szExeFile, L"devenv.exe"))
            {
                if (RunningInSameUserContextWithSameProcessIntegrity(entry.th32ProcessID))
                     m_bVisualStudioRunningAtStart = true;
                return;
            }
        }
    }
}

// This code was borrowed from:
// http://stackoverflow.com/questions/350323/open-a-file-in-visual-studio-at-a-specific-line-number
bool CLogDlg::OpenSelectedWcFilesWithVisualStudio(std::vector<size_t>& changedlogpathindices)
{
    HRESULT result;
    CLSID   clsid;
    result = ::CLSIDFromProgID(L"VisualStudio.DTE", &clsid);
    if (FAILED(result))
        return false;

    // grab active pointer to VS
    CComPtr<IUnknown> pUnk;
    result = ::GetActiveObject(clsid, NULL, &pUnk);
    if (FAILED(result))
        return false;

    // cast to our smart pointer type for EnvDTE
    CComPtr<EnvDTE::_DTE> pDTE;
    pDTE = pUnk;

    // get the item operations pointer we need
    CComPtr<EnvDTE::ItemOperations> pItemOperations;
    result = pDTE->get_ItemOperations(&pItemOperations);
    if (FAILED(result))
        return false;

    // make sure we got a selection
    if (m_ChangedFileListCtrl.GetSelectedCount() <= 0)
        return false;

    // preparation
    DialogEnableWindow(IDOK, FALSE);

    // do the deed...
    OpenSelectedFilesInVisualStudio(changedlogpathindices, pItemOperations);

    // re-enable and end wait
    EnableOKButton();

    ActivateVisualStudioWindow(pDTE);
    return true;
 }

void CLogDlg::OpenSelectedWcFilesWithRegistedProgram(std::vector<size_t>& changedlogpathindices)
{
    CString wcPath;
    int openCount = 0;
    const int MaxFilesToOpen = 20;

    // loop over all the selections
    for ( size_t i = 0; i < changedlogpathindices.size(); ++i)
    {
        wcPath = GetWcPathFromUrl(m_currentChangedArray[changedlogpathindices[i]].GetPath());
        if (!PathFileExists((LPCWSTR)wcPath))
            continue;
        OpenWorkingCopyFileWithRegisteredProgram(wcPath);
        if (++openCount >= MaxFilesToOpen)
            break;
    }
}

void CLogDlg::OpenSelectedFilesInVisualStudio(std::vector<size_t>& changedlogpathindices,
                                              CComPtr<EnvDTE::ItemOperations>& pItemOperations)
{
    CString wcPath;
    int openCount = 0;
    const int MaxFilesToOpen = 100;

    // loop over all the selections
    for ( size_t i = 0; i < changedlogpathindices.size(); ++i)
    {
        wcPath = GetWcPathFromUrl(m_currentChangedArray[changedlogpathindices[i]].GetPath());
        if (!PathFileExists((LPCWSTR)wcPath))
            continue;
        CString extension = PathFindExtension((LPCWSTR)wcPath);
        extension = extension.MakeLower();
        // following extensions might make sense to review in VisualStudio
        if (extension == L".cpp"  || extension == L".h"    || extension == L".cs"   ||
            extension == L".rc"   || extension == L".resx" || extension == L".xaml" ||
            extension == L".js"   || extension == L".html" || extension == L".htm"  ||
            extension == L".asp"  || extension == L".aspx" || extension == L".php"  ||
            extension == L".css"  || extension == L".xml")
        {
            // we arbitrarily limit the number of files per code review to 100
            if (++openCount >= MaxFilesToOpen)
                break;
            OpenOneFileInVisualStudio(wcPath, pItemOperations);
        }
    }
}

// todo: remove duplicated code line ~5752
CString CLogDlg::GetWcPathFromUrl(CString url)
{
    CString wcPath;
    CString fileUrl = GetRepositoryRoot(m_path) + url.Trim();
    // firstfile = (e.g.) http://mydomain.com/repos/trunk/folder/file1
    // sUrl = http://mydomain.com/repos/trunk/folder
    CString sUnescapedUrl = CPathUtils::PathUnescape(GetSUrl());
    // find out until which char the urls are identical
    int j = 0;
    while ((j<fileUrl.GetLength()) && (j<sUnescapedUrl.GetLength())
        && (fileUrl[j] == sUnescapedUrl[j]))
    {
        j++;
    }
    int leftcount = m_path.GetWinPathString().GetLength()-(sUnescapedUrl.GetLength()-j);
    wcPath = m_path.GetWinPathString().Left(leftcount);
    wcPath += fileUrl.Mid(j);
    wcPath.Replace('/', '\\');
    return wcPath;
}

CString CLogDlg::GetSUrl()
{
    CString sUrl;
    if (SVN::PathIsURL(m_path))
    {
        sUrl = m_path.GetSVNPathString();
    }
    else
    {
        sUrl = GetURLFromPath(m_path);
    }
    return sUrl;
}

bool CLogDlg::OpenOneFileInVisualStudio(CString& filename,
                                        CComPtr<EnvDTE::ItemOperations>& pItemOperations)
{
    HRESULT result;
    _bstr_t bstrKind(EnvDTE::vsViewKindTextView);
    CComPtr<EnvDTE::Window> pWindow;
    _bstr_t bstrFileName(filename);

    // ok, open one file in VS
    result = pItemOperations->OpenFile(bstrFileName, bstrKind, &pWindow);
    if (FAILED(result))
        return false;
    return true;
}


// The run in VS won't work if the process owner for VS is different than the current user
// Also, if the same user runs VS as administrator, they run in "High Integrity" mode
// and we don't want to show the Open in Visual Studio menu item in either case.
bool CLogDlg::RunningInSameUserContextWithSameProcessIntegrity(DWORD pidVisualStudio)
{
    DWORD tortoisePid = GetCurrentProcessId();
    PTOKEN_USER pUserTokenTortoise = GetUserTokenFromProcessId(tortoisePid);
    PTOKEN_USER pUserTokenVisualStudio = GetUserTokenFromProcessId(pidVisualStudio);
    BOOL isSameOwner = FALSE;
    if (pUserTokenTortoise != NULL && pUserTokenVisualStudio != NULL)
        isSameOwner = EqualSid((pUserTokenTortoise->User).Sid,
                                    (pUserTokenVisualStudio->User).Sid);
    if(pUserTokenTortoise)
        LocalFree(pUserTokenTortoise);
    if(pUserTokenVisualStudio)
        LocalFree(pUserTokenVisualStudio);

    // check if the process integrity matches, problem if dissimilar
    bool vsHighIntegrity = IsProcessRunningInHighIntegrity(pidVisualStudio);
    bool tortoiseHighIntegrity = IsProcessRunningInHighIntegrity(tortoisePid);
    bool integrityMatches = !(vsHighIntegrity ^ tortoiseHighIntegrity);
    return (isSameOwner && integrityMatches) ? true : false;
}

// Got a pid, want a User Token* ?  Don't forget to free any valid pointers returned
PTOKEN_USER CLogDlg::GetUserTokenFromProcessId(DWORD pid)
{
    CAutoGeneralHandle hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
    CAutoGeneralHandle hToken;

    // yuck!
    if (OpenProcessToken(hProcess, TOKEN_QUERY, hToken.GetPointer()))
    {
        DWORD dwLengthNeeded;
        if (!GetTokenInformation(hToken, TokenUser,  NULL, 0, &dwLengthNeeded))
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                PTOKEN_USER pUserToken = (PTOKEN_USER)LocalAlloc(0, dwLengthNeeded);
                if (pUserToken != NULL)
                {
                    if (GetTokenInformation(hToken, TokenUser,
                        pUserToken, dwLengthNeeded, &dwLengthNeeded))
                    {
                        return pUserToken;
                    }
                    LocalFree(pUserToken);
                    return NULL; // no token info
                }
                return NULL;  // LocalAlloc() failed
            }
            return NULL; // GetLastError() returned a weird error
        }
        return NULL; // no token info :-(
    }
    return NULL; // can't get a process token
}

// adapted from http://msdn.microsoft.com/en-us/library/bb625966.aspx
bool CLogDlg::IsProcessRunningInHighIntegrity(DWORD pid)
{
    bool runningHighIntegrity = false;
    DWORD dwLengthNeeded = 0;
    DWORD dwError = ERROR_SUCCESS;
    PTOKEN_MANDATORY_LABEL pTIL = NULL;
    CAutoGeneralHandle hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
    CAutoGeneralHandle hToken;

    // yuck2
    if (OpenProcessToken(hProcess, TOKEN_QUERY, hToken.GetPointer()))
    {
        // Get the Integrity level.
        if (!GetTokenInformation(hToken, TokenIntegrityLevel,
            NULL, 0, &dwLengthNeeded))
        {
            dwError = GetLastError();
            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0,
                    dwLengthNeeded);
                if (pTIL != NULL)
                {
                    if (GetTokenInformation(hToken, TokenIntegrityLevel,
                        pTIL, dwLengthNeeded, &dwLengthNeeded))
                    {
                        DWORD dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
                            (DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid)-1));

                        if (dwIntegrityLevel >= SECURITY_MANDATORY_HIGH_RID)
                            runningHighIntegrity = true;
                    }
                    LocalFree(pTIL);
                }
            }
        }
    }
    return runningHighIntegrity;
}

void CLogDlg::ActivateVisualStudioWindow(CComPtr<EnvDTE::_DTE>& pDTE)
{
    CComPtr<EnvDTE::Window> pMainWindow;
    HRESULT result = pDTE->get_MainWindow(&pMainWindow);
    if (FAILED(result))
        return;
    long hwnd = 0;
    pMainWindow->get_HWnd(&hwnd);
    if (IsWindow((HWND)hwnd))
        ::SetForegroundWindow((HWND)hwnd);
}

bool CLogDlg::VerifyContextMenuForChangedPathsAllowed(INT_PTR selIndex)
{
    if (selIndex < 0)
        return false;
    int s = m_LogList.GetSelectionMark();
    if (s < 0)
        return false;

    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    if (pos == NULL)
        return false; // nothing is selected, get out of here
    else
        return true;
}

bool CLogDlg::GetContextMenuInfoForChangedPaths(ContextMenuInfoForChangedPathsPtr& pCmi)
{
    POSITION pos = m_LogList.GetFirstSelectedItemPosition();
    INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();

    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_LogList.GetNextSelectedItem(pos));
    if (pLogEntry == NULL)
        return false;
    pCmi->Rev1 = pLogEntry->GetRevision();
    pCmi->Rev2 = pCmi->Rev1;
    pCmi->OneRev = true;
    if (pos)
    {
        while (pos)
        {
            int index = m_LogList.GetNextSelectedItem(pos);
            if (index < (int)m_logEntries.GetVisibleCount())
            {
                pLogEntry = m_logEntries.GetVisible(index);
                if (pLogEntry)
                {
                    pCmi->Rev1 = max(pCmi->Rev1,(svn_revnum_t)pLogEntry->GetRevision());
                    pCmi->Rev2 = min(pCmi->Rev2,(svn_revnum_t)pLogEntry->GetRevision());
                    pCmi->OneRev = false;
                }
            }
        }
        if (!pCmi->OneRev)
            pCmi->Rev2--;

        POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
        while (pos2)
        {
            int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
            pCmi->ChangedPaths.push_back(m_currentChangedPathList[nItem].GetSVNPathString());
            pCmi->ChangedLogPathIndices.push_back (static_cast<size_t>(nItem));
        }
    }
    else
    {
        // only one revision is selected in the log dialog top pane
        // but multiple items could be selected  in the changed items list
        pCmi->Rev2 = pCmi->Rev1-1;

        POSITION pos2 = m_ChangedFileListCtrl.GetFirstSelectedItemPosition();
        while (pos2)
        {
            const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();

            int nItem = m_ChangedFileListCtrl.GetNextSelectedItem(pos2);
            pCmi->ChangedLogPathIndices.push_back (static_cast<size_t>(nItem));
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
                    pCmi->Rev2 = changedlogpath.GetCopyFromRev();
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
                                pCmi->Rev2 = paths[flist].GetCopyFromRev();
                        }
                    }
                }
            }

            pCmi->ChangedPaths.push_back(changedlogpath.GetPath());
        }
    }

    pCmi->sUrl = GetSUrl();

    // find the working copy path of the selected item from the URL
    CString sUrlRoot = GetRepositoryRoot(m_path);
    if (!sUrlRoot.IsEmpty())
    {
        const CLogChangedPath& changedlogpath = m_currentChangedArray[selIndex];
        pCmi->fileUrl = changedlogpath.GetPath();
        pCmi->fileUrl = sUrlRoot + pCmi->fileUrl.Trim();
        if (m_hasWC)
        {
            // firstfile = (e.g.) http://mydomain.com/repos/trunk/folder/file1
            // pCmi->sUrl = http://mydomain.com/repos/trunk/folder
            CString sUnescapedUrl = CPathUtils::PathUnescape(pCmi->sUrl);
            // find out until which char the urls are identical
            int i=0;
            while ((i<pCmi->fileUrl.GetLength())&&(i<sUnescapedUrl.GetLength())&&(pCmi->fileUrl[i]==sUnescapedUrl[i]))
                i++;
            int leftcount = m_path.GetWinPathString().GetLength()-(sUnescapedUrl.GetLength()-i);
            pCmi->wcPath = m_path.GetWinPathString().Left(leftcount);
            pCmi->wcPath += pCmi->fileUrl.Mid(i);
            pCmi->wcPath.Replace('/', '\\');
        }
    }

    return true;
}

bool CLogDlg::PopulateContextMenuForChangedPaths(ContextMenuInfoForChangedPathsPtr& pCmi, CIconMenu& popup, CIconMenu& clipSubMenu)
{
    INT_PTR selIndex = m_ChangedFileListCtrl.GetSelectionMark();

    if (popup.CreatePopupMenu())
    {
        bool bEntryAdded = false;
        if (m_ChangedFileListCtrl.GetSelectedCount() == 1)
        {
            if ((!pCmi->OneRev)||(IsDiffPossible (m_currentChangedArray[selIndex], pCmi->Rev1)))
            {
                popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
                popup.AppendMenuIcon(ID_DIFF_CONTENTONLY, IDS_LOG_POPUP_DIFF_CONTENTONLY, IDI_DIFF);
                popup.AppendMenuIcon(ID_BLAMEDIFF, IDS_LOG_POPUP_BLAMEDIFF, IDI_BLAME);
                popup.SetDefaultItem(ID_DIFF, FALSE);
                popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
                bEntryAdded = true;
            }
            else if (pCmi->OneRev)
            {
                popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
                popup.AppendMenuIcon(ID_DIFF_CONTENTONLY, IDS_LOG_POPUP_DIFF_CONTENTONLY, IDI_DIFF);
                popup.SetDefaultItem(ID_DIFF, FALSE);
                bEntryAdded = true;
            }
            if ((pCmi->Rev2 == pCmi->Rev1-1)||(pCmi->ChangedPaths.size() == 1))
            {
                if (bEntryAdded)
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
                popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
                if (m_hasWC && (!pCmi->wcPath.IsEmpty()) && PathFileExists(pCmi->wcPath))
                {
                    popup.AppendMenuIcon(ID_OPENLOCAL, IDS_LOG_POPUP_OPENLOCAL, IDI_OPEN);
                    popup.AppendMenuIcon(ID_OPENWITHLOCAL, IDS_LOG_POPUP_OPENWITHLOCAL, IDI_OPEN);
                }
                popup.AppendMenuIcon(ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);
                popup.AppendMenu(MF_SEPARATOR, NULL);
                if ((m_hasWC)&&(pCmi->OneRev)&&(!pCmi->wcPath.IsEmpty()))
                    popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
                // "Show Properties"
                popup.AppendMenuIcon(ID_POPPROPS, IDS_REPOBROWSE_SHOWPROP, IDI_PROPERTIES);
                // "Show Log"
                popup.AppendMenuIcon(ID_LOG, IDS_MENULOG, IDI_LOG);
                popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
                // "Show merge log"
                popup.AppendMenuIcon(ID_GETMERGELOGS, IDS_LOG_POPUP_GETMERGELOGS, IDI_LOG);
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
        else if (!pCmi->ChangedLogPathIndices.empty())
        {
            // more than one entry is selected
            popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE);
            bEntryAdded = true;
        }
        if (!pCmi->ChangedPaths.empty())
        {
            if(m_ChangedFileListCtrl.GetSelectedCount() > 1)
            {
                popup.AppendMenuIcon(ID_DIFF_MULTIPLE, IDS_LOG_POPUP_DIFF_MULTIPLE, IDI_DIFF);
                popup.AppendMenuIcon(ID_DIFF_MULTIPLE_CONTENTONLY, IDS_LOG_POPUP_DIFF_MULTIPLE_CONTENTONLY, IDI_DIFF);
                popup.SetDefaultItem(ID_DIFF_MULTIPLE, FALSE);
                popup.AppendMenuIcon(ID_OPENLOCAL_MULTIPLE, IDS_LOG_POPUP_OPENLOCAL_MULTIPLE, IDI_OPEN);
            }
            popup.AppendMenuIcon(ID_EXPORTTREE, IDS_MENUEXPORT, IDI_EXPORT);

            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURL, IDS_LOG_POPUP_CLIPBOARD_URL, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDRELPATH, IDS_LOG_POPUP_CLIPBOARD_RELPATH, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDFILENAMES, IDS_LOG_POPUP_CLIPBOARD_FILENAMES, IDI_COPYCLIP);

            CString temp;
            temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
            popup.InsertMenu((UINT)-1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)clipSubMenu.m_hMenu, temp);


            bEntryAdded = true;
        }

        if (!bEntryAdded)
            return false;
    }
    return true;
}

// borrowed from SVNStatusListCtrl -- extract??
bool CLogDlg::CheckMultipleDiffs( UINT selCount )
{
    if (selCount > max(3, (DWORD)CRegDWORD(L"Software\\TortoiseSVN\\NumDiffWarning", 15)))
    {
        CString message;
        message.Format(CString(MAKEINTRESOURCE(IDS_STATUSLIST_WARN_MAXDIFF)), selCount);
        CTaskDialog taskdlg(message,
                            CString(MAKEINTRESOURCE(IDS_STATUSLIST_WARN_MAXDIFF_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_STATUSLIST_WARN_MAXDIFF_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_STATUSLIST_WARN_MAXDIFF_TASK4)));
        taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskdlg.SetDefaultCommandControl(2);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        bool doIt = (taskdlg.DoModal(m_hWnd) == 1);
        return doIt;
    }
    return true;
}

void CLogDlg::ExecuteMultipleDiffChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi, bool ignoreprops )
{
    int nPaths = (int)pCmi->ChangedLogPathIndices.size();

    // warn if we exceed Software\\TortoiseSVN\\NumDiffWarning or 15 if not set
    if (!CheckMultipleDiffs(nPaths))
        return;

    for (int i = 0; i < nPaths; ++i)
    {
        INT_PTR selIndex = (INT_PTR)pCmi->ChangedLogPathIndices[i];
        ExecuteDiffChangedPaths(pCmi, selIndex, ignoreprops);
    }
}


void CLogDlg::ExecuteDiffChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi, INT_PTR selIndex, bool ignoreprops )
{
    if ((!pCmi->OneRev)|| IsDiffPossible (m_currentChangedArray[selIndex], pCmi->Rev1))
    {
        auto f = [=]()
        {
            CoInitialize(NULL);
            this->EnableWindow(FALSE);
            DoDiffFromLog(selIndex, pCmi->Rev1, pCmi->Rev2, false, false, ignoreprops);
            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        auto f = [=]()
        {
            CoInitialize(NULL);
            this->EnableWindow(FALSE);
            DiffSelectedFile(ignoreprops);
            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteBlameDiffChangedPaths( INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi )
{
    auto f = [=]()
    {
        CoInitialize(NULL);
        this->EnableWindow(FALSE);
        DoDiffFromLog(selIndex, pCmi->Rev1, pCmi->Rev2, true, false, false);
        this->EnableWindow(TRUE);
        this->SetFocus();
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteGnuDiff1ChangedPaths( INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi )
{
    auto f = [=]()
    {
        CoInitialize(NULL);
        this->EnableWindow(FALSE);
        DoDiffFromLog(selIndex, pCmi->Rev1, pCmi->Rev2, false, true, false);
        this->EnableWindow(TRUE);
        this->SetFocus();
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteRevertChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath )
{
    SetPromptApp(&theApp);
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        EnableOKButton();
        return;
    }
    m_bCancelled = false;
    CSVNProgressDlg dlg;
    if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
    {
        // a deleted path! Since the path isn't there anymore, merge
        // won't work. So just do a copy url->wc
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Copy);
        dlg.SetPathList(CTSVNPathList(CTSVNPath(pCmi->fileUrl)));
        dlg.SetUrl(pCmi->wcPath);
        dlg.SetRevision(pCmi->Rev2);
        dlg.SetPegRevision(pCmi->Rev2);
    }
    else
    {
        if (!PathFileExists(pCmi->wcPath))
        {
            // seems the path got renamed
            // tell the user how to work around this.
            TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_LOG_REVERTREV_ERROR), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
            EnableOKButton();
            return;      //exit
        }
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetOptions(ProgOptIgnoreAncestry);
        dlg.SetPathList(CTSVNPathList(CTSVNPath(pCmi->wcPath)));
        dlg.SetUrl(pCmi->fileUrl);
        dlg.SetSecondUrl(pCmi->fileUrl);
        SVNRevRangeArray revarray;
        revarray.AddRevRange(pCmi->Rev1, pCmi->Rev2);
        dlg.SetRevisionRanges(revarray);
    }
    if (ConfirmRevert(pCmi->wcPath))
    {
        dlg.DoModal();
    }
}

void CLogDlg::ExecuteShowPropertiesChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi )
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);

    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        EnableOKButton();
        return;
    }
    CPropDlg dlg;
    dlg.m_rev = pCmi->Rev1;
    dlg.m_Path = CTSVNPath(pCmi->fileUrl);
    dlg.DoModal();
    EnableOKButton();
}

void CLogDlg::ExecuteSaveAsChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi, INT_PTR selIndex )
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        EnableOKButton();
        return;
    }
    m_bCancelled = false;
    CString sRoot = GetRepositoryRoot(CTSVNPath(pCmi->fileUrl));
    // if more than one entry is selected, we save them
    // one by one into a folder the user has selected
    bool bTargetSelected = false;
    CTSVNPath TargetPath;
    if (m_ChangedFileListCtrl.GetSelectedCount() > 1)
    {
        CBrowseFolder browseFolder;
        browseFolder.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_SAVEFOLDERTOHINT)));
        browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE |
            BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
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
            revFilename.Format(L"%s-%ld%s", (LPCTSTR)temp.Left(rfind), pCmi->Rev1,
            (LPCTSTR)temp.Mid(rfind));
        else
            revFilename.Format(L"%s-%ld", (LPCTSTR)temp, pCmi->Rev1);
        bTargetSelected = CAppUtils::FileOpenSave(revFilename, NULL, IDS_LOG_POPUP_SAVE,
                                                  IDS_COMMONFILEFILTER, false, m_path.GetDirectory().GetWinPathString(), m_hWnd);
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
            for ( size_t i = 0; i < pCmi->ChangedLogPathIndices.size(); ++i)
            {
                const CLogChangedPath& changedlogpathi
                    = m_currentChangedArray[pCmi->ChangedLogPathIndices[i]];

                SVNRev getrev = (changedlogpathi.GetAction() == LOGACTIONS_DELETED) ?
                    pCmi->Rev2 : pCmi->Rev1;

                CString sInfoLine;
                sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION,
                    (LPCTSTR)CPathUtils::GetFileNameFromPath(changedlogpathi.GetPath()),
                    (LPCTSTR)getrev.ToString());
                progDlg.SetLine(1, sInfoLine, true);
                SetAndClearProgressInfo(&progDlg);
                progDlg.ShowModeless(m_hWnd);

                CTSVNPath tempfile = TargetPath;
                if (pCmi->ChangedPaths.size() > 1)
                {
                    // if multiple items are selected, then the TargetPath
                    // points to a folder and we have to append the filename
                    // to save to that folder.
                    CString sName = changedlogpathi.GetPath();
                    int slashpos = sName.ReverseFind('/');
                    if (slashpos >= 0)
                        sName = sName.Mid(slashpos);
                    tempfile.AppendPathString(sName);
                }
                CString filepath = sRoot + changedlogpathi.GetPath();
                progDlg.SetLine(2, filepath, true);
                if (!Export(CTSVNPath(filepath), tempfile, getrev, getrev))
                {
                    progDlg.Stop();
                    SetAndClearProgressInfo((HWND)NULL);
                    ShowErrorDialog(m_hWnd);
                    tempfile.Delete(false);
                    EnableOKButton();
                    return;
                }
                progDlg.SetProgress((DWORD)i+1, (DWORD)pCmi->ChangedLogPathIndices.size());
            }
            progDlg.Stop();
            SetAndClearProgressInfo((HWND)NULL);
            this->EnableWindow(TRUE);
            this->SetFocus();

        };  // end lambda definition
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        EnableOKButton();
    }

}

void CLogDlg::ExecuteExportTreeChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi )
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    m_bCancelled = false;

    bool bTargetSelected = false;
    CTSVNPath TargetPath;
    if (m_ChangedFileListCtrl.GetSelectedCount() > 0)
    {
        CBrowseFolder browseFolder;
        browseFolder.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_SAVEFOLDERTOHINT)));
        browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS
            | BIF_RETURNONLYFSDIRS;
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
            progDlg.SetTime(true);
            for ( size_t i = 0; i < pCmi->ChangedLogPathIndices.size(); ++i)
            {
                const CString& schangedlogpath
                    = m_currentChangedArray[pCmi->ChangedLogPathIndices[i]].GetPath();

                SVNRev getrev = pCmi->Rev1;

                CTSVNPath tempfile = TargetPath;
                tempfile.AppendPathString(schangedlogpath);

                CString sInfoLine;
                sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION,
                    (LPCTSTR)schangedlogpath, (LPCTSTR)getrev.ToString());
                progDlg.SetLine(1, sInfoLine, true);
                progDlg.SetLine(2, tempfile.GetWinPath(), true);
                progDlg.SetProgress64(i, pCmi->ChangedLogPathIndices.size());
                progDlg.ShowModeless(m_hWnd);

                SHCreateDirectoryEx(m_hWnd, tempfile.GetContainingDirectory().GetWinPath(),
                    NULL);
                CString filepath = m_sRepositoryRoot + schangedlogpath;
                if (!Export(CTSVNPath(filepath), tempfile, getrev, getrev,
                    true, true, svn_depth_empty))
                {
                    progDlg.Stop();
                    SetAndClearProgressInfo((HWND)NULL);
                    ShowErrorDialog(m_hWnd);
                    tempfile.Delete(false);
                    EnableOKButton();
                    break;
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

void CLogDlg::ExecuteOpenChangedPaths( INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi, bool bOpenWith )
{
    SVNRev getrev = m_currentChangedArray[selIndex].GetAction() == LOGACTIONS_DELETED ?
        pCmi->Rev2 : pCmi->Rev1;
    auto f = [=]()
    {
        CoInitialize(NULL);
        this->EnableWindow(FALSE);
        Open(bOpenWith,m_currentChangedArray[selIndex].GetPath(),getrev);
        this->EnableWindow(TRUE);
        this->SetFocus();
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteBlameChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath )
{
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        EnableOKButton();
        return;
    }
    CBlameDlg dlg;
    if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
        pCmi->Rev1--;
    dlg.EndRev = pCmi->Rev1;
    if (dlg.DoModal() == IDOK)
    {
        SVNRev startrev = dlg.StartRev;
        SVNRev endrev = dlg.EndRev;
        SVNRev pegrev = pCmi->Rev1;
        bool includeMerge = !!dlg.m_bIncludeMerge;
        bool textView = !!dlg.m_bTextView;
        auto f = [=]()
        {
            CoInitialize(NULL);
            this->EnableWindow(FALSE);
            CBlame blame;
            CString tempfile;
            tempfile = blame.BlameToTempFile(CTSVNPath(pCmi->fileUrl), startrev,
                endrev, pegrev, L"", includeMerge, TRUE, TRUE);
            if (!tempfile.IsEmpty())
            {
                if (textView)
                {
                    //open the default text editor for the result file
                    CAppUtils::StartTextViewer(tempfile);
                }
                else
                {
                    CString sParams = L"/path:\"" + pCmi->fileUrl + L"\" ";
                    CAppUtils::LaunchTortoiseBlame(tempfile,
                        CPathUtils::GetFileNameFromPath(pCmi->fileUrl),
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
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteShowLogChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath, bool bMergeLog )
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        return;
    }
    m_bCancelled = false;
    svn_revnum_t logrev = pCmi->Rev1;
    CString sCmd;
    if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
    {
        // if the item got deleted in this revision,
        // fetch the log from the previous revision where it
        // still existed.
        sCmd.Format(L"/command:log /path:\"%s\" /startrev:%ld /pegrev:%ld",
            (LPCTSTR)pCmi->fileUrl, logrev-1, logrev-1);
    }
    else
    {
        sCmd.Format(L"/command:log /path:\"%s\" /pegrev:%ld",
            (LPCTSTR)pCmi->fileUrl, logrev);
    }

    if (bMergeLog)
        sCmd += L" /merge";
    CAppUtils::RunTortoiseProc(sCmd);
    EnableOKButton();
}

void CLogDlg::ExecuteBrowseRepositoryChangedPaths( ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath )
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        EnableOKButton();
        return;
    }
    m_bCancelled = false;
    svn_revnum_t logrev = pCmi->Rev1;
    CString sCmd;
    if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
    {
        sCmd.Format(L"/command:repobrowser /path:\"%s\" /rev:%ld",
            (LPCTSTR)pCmi->fileUrl, logrev-1);
    }
    else
    {
        sCmd.Format(L"/command:repobrowser /path:\"%s\" /rev:%ld",
            (LPCTSTR)pCmi->fileUrl, logrev);
    }

    CAppUtils::RunTortoiseProc(sCmd);
    EnableOKButton();
}

void CLogDlg::ExecuteViewPathRevisionChangedPaths( INT_PTR selIndex )
{
    PLOGENTRYDATA pLogEntry2 = m_logEntries.GetVisible (m_LogList.GetSelectionMark());
    if (pLogEntry2)
    {
        SVNRev rev = pLogEntry2->GetRevision();
        CString relurl = m_currentChangedArray[selIndex].GetPath();
        CString url = m_ProjectProperties.sWebViewerPathRev;
        url = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
        url.Replace(L"%REVISION%", rev.ToString());
        url.Replace(L"%PATH%", relurl);
        relurl = relurl.Mid(relurl.Find('/'));
        url.Replace(L"%PATH1%", relurl);
        if (!url.IsEmpty())
            ShellExecute(this->m_hWnd, L"open", url, NULL, NULL, SW_SHOWDEFAULT);
    }
}

void CLogDlg::CopyChangedPathInfoToClipboard(ContextMenuInfoForChangedPathsPtr pCmi, int cmd)
{
    int nPaths = (int)pCmi->ChangedLogPathIndices.size();

    CString sClipboard;
    for (int i = 0; i < nPaths; ++i)
    {
        INT_PTR selIndex = (INT_PTR)pCmi->ChangedLogPathIndices[i];

        CLogChangedPath path = m_currentChangedArray[selIndex];
        switch (cmd)
        {
        case ID_COPYCLIPBOARDURL:
            sClipboard += (m_sRepositoryRoot + path.GetPath());
            break;
        case ID_COPYCLIPBOARDRELPATH:
            sClipboard += path.GetPath();
            break;
        case ID_COPYCLIPBOARDFILENAMES:
            sClipboard += CPathUtils::GetFileNameFromPath(path.GetPath());
            break;
        }
        sClipboard += L"\r\n";
    }
    CStringUtils::WriteAsciiStringToClipboard(sClipboard);
}

LRESULT CLogDlg::OnRefreshSelection( WPARAM /*wParam*/, LPARAM /*lParam*/ )
{
    // it's enough to deselect, then select again one item of the whole selection
    int selMark = m_LogList.GetSelectionMark();
    if (selMark>=0)
    {
        m_LogList.SetSelectionMark(selMark);
        m_LogList.SetItemState(selMark, 0, LVIS_SELECTED);
        m_LogList.SetItemState(selMark, LVIS_SELECTED, LVIS_SELECTED);
    }
    return 0;
}

bool CLogDlg::CreateToolbar()
{
    m_hwndToolbar = CreateWindowEx(0,
                                   TOOLBARCLASSNAME,
                                   (LPCWSTR)NULL,
                                   WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                   0, 0, 0, 0,
                                   GetSafeHwnd(),
                                   (HMENU)NULL,
                                   AfxGetResourceHandle(),
                                   NULL);
    if (m_hwndToolbar == INVALID_HANDLE_VALUE)
        return false;

    ::SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

#define MONITORMODE_TOOLBARBUTTONCOUNT  7
    TBBUTTON tbb[MONITORMODE_TOOLBARBUTTONCOUNT] = { 0 };
    // create an image list containing the icons for the toolbar
    m_hToolbarImages = ImageList_Create(24, 24, ILC_COLOR32 | ILC_MASK, MONITORMODE_TOOLBARBUTTONCOUNT, 4);
    if (m_hToolbarImages == NULL)
        return false;
    auto iString = ::SendMessage(m_hwndToolbar, TB_ADDSTRING,
                                 (WPARAM)AfxGetResourceHandle(), (LPARAM)IDS_MONITOR_TOOLBARTEXTS);
    int index = 0;
    HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITOR_GETALL), IMAGE_ICON, 0, 0, LR_VGACOLOR | LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_CHECKREPOSITORIESNOW;
    tbb[index].fsState = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = iString++;

    hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITOR_ADD), IMAGE_ICON, 0, 0, LR_VGACOLOR | LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_ADDPROJECT;
    tbb[index].fsState = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = iString++;

    tbb[index].iBitmap = 0;
    tbb[index].idCommand = 0;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_SEP;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITOR_EDIT), IMAGE_ICON, 0, 0, LR_VGACOLOR | LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_EDIT;
    tbb[index].fsState = BTNS_SHOWTEXT;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = iString++;

    hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITOR_REMOVE), IMAGE_ICON, 0, 0, LR_VGACOLOR | LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_REMOVE;
    tbb[index].fsState = BTNS_SHOWTEXT;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = iString++;

    tbb[index].iBitmap = 0;
    tbb[index].idCommand = 0;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_SEP;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITOR_OPTIONS), IMAGE_ICON, 0, 0, LR_VGACOLOR | LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    tbb[index].iBitmap = ImageList_AddIcon(m_hToolbarImages, hIcon);
    tbb[index].idCommand = ID_MISC_OPTIONS;
    tbb[index].fsState = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = iString++;

    ::SendMessage(m_hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)m_hToolbarImages);
    ::SendMessage(m_hwndToolbar, TB_ADDBUTTONS, (WPARAM)index, (LPARAM)(LPTBBUTTON)&tbb);
    ::SendMessage(m_hwndToolbar, TB_AUTOSIZE, 0, 0);
    return true;
}

void CLogDlg::InitMonitoringMode()
{
    CreateToolbar();

    // set up the control visibility and positions
    GetDlgItem(IDC_SHOWPATHS)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_CHECK_STOPONCOPY)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_INCLUDEMERGE)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_HIDENONMERGEABLE)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_GETALL)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_NEXTHUNDRED)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_REFRESH)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_STATBUTTON)->ShowWindow(SW_HIDE);
    GetDlgItem(IDHELP)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_SPLITTERLEFT)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_PROJTREE)->ShowWindow(SW_SHOW);

    CRect rect;
    ::GetClientRect(m_hwndToolbar, &rect);

    CRect rcDlg;
    GetClientRect(&rcDlg);

    CRect rcSearch;
    m_cFilter.GetWindowRect(&rcSearch);
    ScreenToClient(&rcSearch);
    CRect rcOK;
    GetDlgItem(IDOK)->GetWindowRect(&rcOK);
    ScreenToClient(&rcOK);

    ::SetWindowPos(m_hwndToolbar, NULL, rcSearch.left, 0, rcDlg.Width(), rect.Height(), SWP_SHOWWINDOW);
    AddAnchor(m_hwndToolbar, TOP_LEFT, TOP_RIGHT);

    int delta = 90;
    GetDlgItem(IDC_PROJTREE)->SetWindowPos(NULL, rcSearch.left, rcSearch.top + rect.Height(), delta, rcOK.bottom - rcSearch.top - rect.Height(), SWP_SHOWWINDOW);
    GetDlgItem(IDC_SPLITTERLEFT)->SetWindowPos(NULL, rcSearch.left + delta, rcSearch.top + rect.Height(), 4, rcOK.bottom - rcSearch.top, SWP_SHOWWINDOW);

    delta += 4;
    CSplitterControl::ChangePos(GetDlgItem(IDC_SEARCHEDIT), 0, rect.Height());
    CSplitterControl::ChangePos(GetDlgItem(IDC_FROMLABEL), 0, rect.Height());
    CSplitterControl::ChangePos(GetDlgItem(IDC_DATEFROM), 0, rect.Height());
    CSplitterControl::ChangePos(GetDlgItem(IDC_TOLABEL), 0, rect.Height());
    CSplitterControl::ChangePos(GetDlgItem(IDC_DATETO), 0, rect.Height());
    CSplitterControl::ChangeHeight(&m_LogList, -rect.Height(), CW_BOTTOMALIGN);

    CSplitterControl::ChangeWidth(&m_cFilter, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_LogList, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_wndSplitter1, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_MSGVIEW), -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_wndSplitter2, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_ChangedFileListCtrl, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangeWidth(&m_LogProgress, -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangePos(&m_LogProgress, 0, 20);
    CSplitterControl::ChangeWidth(GetDlgItem(IDC_LOGINFO), -delta, CW_RIGHTALIGN);
    CSplitterControl::ChangePos(GetDlgItem(IDC_LOGINFO), 0, 75);
    CSplitterControl::ChangeHeight(&m_ChangedFileListCtrl, 75, CW_TOPALIGN);


    DWORD exStyle = TVS_EX_AUTOHSCROLL | TVS_EX_DOUBLEBUFFER;
    m_projTree.SetExtendedStyle(exStyle, exStyle);
    SetWindowTheme(m_projTree.GetSafeHwnd(), L"Explorer", NULL);
    m_nMonitorUrlIcon = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITORURL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    m_nMonitorWCIcon = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITORWC), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    m_nErrorOvl = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MODIFIEDOVL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    if (m_nErrorOvl >= 0)
        SYS_IMAGE_LIST().SetOverlayImage(m_nErrorOvl, OVERLAY_MODIFIED);
    m_projTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);
    m_projTree.SetDroppedMessage(WM_TSVN_MONITOR_TREEDROP);

    // Set up the tray icon
    ChangeWindowMessageFilter(WM_TASKBARCREATED, MSGFLT_ADD);

    m_hMonitorIconNormal = LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITORNORMAL));
    m_hMonitorIconNewCommits = LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MONITORCOMMITS));

    m_SystemTray.cbSize = sizeof(NOTIFYICONDATA);
    m_SystemTray.uVersion = NOTIFYICON_VERSION_4;
    m_SystemTray.hWnd = GetSafeHwnd();
    m_SystemTray.hIcon = m_hMonitorIconNormal;
    m_SystemTray.uFlags = NIF_MESSAGE | NIF_ICON;
    m_SystemTray.uCallbackMessage = WM_TSVN_MONITOR_TASKBARCALLBACK;
    if (Shell_NotifyIcon(NIM_ADD, &m_SystemTray) == FALSE)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
        Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
    }

    // fill the project tree
    InitMonitorProjTree();
    AssumeCacheEnabled(true);
    CRegDWORD reg = CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs", 100);
    m_limit = (int)(DWORD)reg;

    SetTimer(MONITOR_TIMER, 200, NULL);
}

void CLogDlg::InitMonitorProjTree()
{
    CString sDataFilePath = CPathUtils::GetAppDataDirectory();
    sDataFilePath += L"\\MonitoringData.ini";
    m_monitoringFile.LoadFile(sDataFilePath);
    RefreshMonitorProjTree();
}

void CLogDlg::RefreshMonitorProjTree()
{
    CSimpleIni::TNamesDepend mitems;
    m_monitoringFile.GetAllSections(mitems);
    m_projTree.SetRedraw(FALSE);
    // remove all existing data from the control and free the memory
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
        delete pItem;
        m_projTree.SetItemData(hItem, NULL);
        return false;
    });
    m_projTree.DeleteAllItems();
    int itemcount = 0;
    bool hasUnreadItems = false;
    for (const auto& mitem : mitems)
    {
        CString Name = m_monitoringFile.GetValue(mitem, L"Name", L"");
        if (!Name.IsEmpty())
        {
            MonitorItem * pMonitorItem = new MonitorItem(Name);
            pMonitorItem->WCPathOrUrl = m_monitoringFile.GetValue(mitem, L"WCPathOrUrl", L"");
            pMonitorItem->interval = _wtoi(m_monitoringFile.GetValue(mitem, L"interval", L"5"));
            pMonitorItem->lastchecked = _wtoi64(m_monitoringFile.GetValue(mitem, L"lastchecked", L"0"));
            pMonitorItem->lastHEAD = _wtoi(m_monitoringFile.GetValue(mitem, L"lastHEAD", L"0"));
            pMonitorItem->UnreadItems = _wtoi(m_monitoringFile.GetValue(mitem, L"UnreadItems", L"0"));
            pMonitorItem->username = m_monitoringFile.GetValue(mitem, L"username", L"");
            pMonitorItem->password = m_monitoringFile.GetValue(mitem, L"password", L"");
            InsertMonitorItem(pMonitorItem, m_monitoringFile.GetValue(mitem, L"parentTreePath", L""));
            ++itemcount;
            if (pMonitorItem->UnreadItems)
                hasUnreadItems = true;
        }
    }
    if (itemcount == 0)
        m_projTree.ShowText(CString(MAKEINTRESOURCE(IDS_MONITOR_ADDHINT)), true);
    else
        m_projTree.ClearText();
    m_projTree.SetRedraw(TRUE);

    m_SystemTray.hIcon = hasUnreadItems ? m_hMonitorIconNewCommits : m_hMonitorIconNormal;
    m_SystemTray.uFlags = NIF_ICON;
    if (Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray) == FALSE)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
        Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
    }

}


HTREEITEM CLogDlg::InsertMonitorItem(MonitorItem * pMonitorItem, const CString& sParentPath /*= CString()*/)
{
    bool bUrl = !!::PathIsURL(pMonitorItem->WCPathOrUrl);
    TVINSERTSTRUCT tvinsert = { 0 };
    tvinsert.hParent = FindMonitorParent(sParentPath);
    tvinsert.hInsertAfter = TVI_SORT;
    tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_EXPANDEDIMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
    tvinsert.itemex.pszText = LPSTR_TEXTCALLBACK;
    tvinsert.itemex.cChildren = pMonitorItem->WCPathOrUrl.IsEmpty() ? 1 : 0;
    tvinsert.itemex.state = TVIS_EXPANDED | (pMonitorItem->UnreadItems ? TVIS_BOLD : 0);
    tvinsert.itemex.stateMask = TVIS_EXPANDED | TVIS_BOLD;
    tvinsert.itemex.lParam = (LPARAM)pMonitorItem;
    tvinsert.itemex.iImage = pMonitorItem->WCPathOrUrl.IsEmpty() ? m_nIconFolder : bUrl ? m_nMonitorUrlIcon : m_nMonitorWCIcon;
    tvinsert.itemex.iExpandedImage = pMonitorItem->WCPathOrUrl.IsEmpty() ? m_nOpenIconFolder : bUrl ? m_nMonitorUrlIcon : m_nMonitorWCIcon;
    tvinsert.itemex.iSelectedImage = pMonitorItem->WCPathOrUrl.IsEmpty() ? m_nIconFolder : bUrl ? m_nMonitorUrlIcon : m_nMonitorWCIcon;

    // mark the parent as having children
    if (tvinsert.hParent && (tvinsert.hParent != TVI_ROOT))
    {
        TVITEM tvItem;
        tvItem.mask = TVIF_CHILDREN | TVIF_HANDLE;
        tvItem.cChildren = 1;
        tvItem.hItem = tvinsert.hParent;
        m_projTree.SetItem(&tvItem);
        m_projTree.Expand(tvinsert.hParent, TVE_EXPAND);
    }
    return m_projTree.InsertItem(&tvinsert);
}

HTREEITEM CLogDlg::RecurseMonitorTree(HTREEITEM hItem, MonitorItemHandler handler)
{
    HTREEITEM hFound = hItem;
    while (hFound)
    {
        if (hFound == TVI_ROOT)
            hFound = m_projTree.GetNextItem(TVI_ROOT, TVGN_ROOT);
        if (hFound == NULL)
            return NULL;
        if (handler(hFound))
            return hFound;
        HTREEITEM hChild = m_projTree.GetChildItem(hFound);
        if (hChild)
        {
            hChild = RecurseMonitorTree(hChild, handler);
            if (hChild)
                return hChild;
        }
        hFound = m_projTree.GetNextItem(hFound, TVGN_NEXT);
    }
    return NULL;
}

HTREEITEM CLogDlg::FindMonitorParent(const CString& parentTreePath)
{
    if (parentTreePath.IsEmpty())
        return TVI_ROOT;
    HTREEITEM hRetItem = TVI_ROOT;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
    {
        if (GetTreePath(hItem).Compare(parentTreePath)==0)
        {
            hRetItem = hItem;
            return true;
        }
        return false;
    });
    return hRetItem;
}

CString CLogDlg::GetTreePath(HTREEITEM hItem)
{
    CString path;
    MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
    if (pItem)
    {
        path = pItem->Name;
        HTREEITEM hParent = m_projTree.GetParentItem(hItem);
        while (hParent)
        {
            pItem = (MonitorItem *)m_projTree.GetItemData(hParent);
            if (pItem)
                path = pItem->Name + L"\\" + path;
            hParent = m_projTree.GetParentItem(hParent);
        }
    }
    return path;
}

HTREEITEM CLogDlg::FindMonitorItem(const CString& wcpathorurl)
{
    HTREEITEM hRetItem = NULL;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
        if (pItem->WCPathOrUrl.CompareNoCase(wcpathorurl) == 0)
        {
            hRetItem = hItem;
            return true;
        }
        return false;
    });
    return hRetItem;
}

void CLogDlg::OnMonitorCheckNow()
{
    // clear the log view
    m_ChangedFileListCtrl.SetItemCountEx(0);
    m_ChangedFileListCtrl.Invalidate();
    m_LogList.SetItemCountEx(0);
    m_LogList.Invalidate();
    CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
    pMsgView->SetWindowText(L"");
    m_logEntries.ClearAll();
    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

    // mark all entries as 'never checked before'
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
        pItem->lastchecked = 0;
        return false;
    });
    // start the check timer
    SetTimer(MONITOR_TIMER, 1000, NULL);
}

void CLogDlg::OnMonitorAddProject()
{
    MonitorEditProject(nullptr);
}

void CLogDlg::OnMonitorEditProject()
{
    MonitorItem * pProject = nullptr;
    HTREEITEM hSelItem = m_projTree.GetSelectedItem();
    if (hSelItem)
    {
        pProject = (MonitorItem*)m_projTree.GetItemData(hSelItem);
    }
    MonitorEditProject(pProject);
}

void CLogDlg::OnMonitorRemoveProject()
{
    HTREEITEM hSelItem = m_projTree.GetSelectedItem();
    if (hSelItem)
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hSelItem);
        if (pItem)
        {
            CString sTask1;
            sTask1.Format(IDS_MONITOR_DELETE_TASK2, (LPCTSTR)pItem->Name);
            CTaskDialog taskdlg(sTask1,
                                CString(MAKEINTRESOURCE(IDS_MONITOR_DELETE_TASK1)),
                                L"TortoiseSVN",
                                0,
                                TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION |
                                TDF_POSITION_RELATIVE_TO_WINDOW);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_MONITOR_DELETE_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_MONITOR_DELETE_TASK4)));
            taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskdlg.SetDefaultCommandControl(2);
            taskdlg.SetMainIcon(TD_WARNING_ICON);
            if (taskdlg.DoModal(m_hWnd) != 1)
                return;
            HTREEITEM hChild = m_projTree.GetChildItem(hSelItem);
            if (hChild)
            {
                RecurseMonitorTree(hChild, [&](HTREEITEM hItem)->bool
                {
                    MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
                    delete pItem;
                    return false;
                });
            }
            delete pItem;
            m_projTree.DeleteItem(hSelItem);
            SaveMonitorProjects();
            RefreshMonitorProjTree();
        }
    }
}

void CLogDlg::OnMonitorOptions()
{
    CMonitorOptionsDlg dlg;
    dlg.DoModal();
}

void CLogDlg::MonitorEditProject(MonitorItem * pProject)
{
    CMonitorProjectDlg dlg(this);
    if (pProject)
    {
        dlg.m_sName = pProject->Name;
        dlg.m_sPathOrURL = pProject->WCPathOrUrl;
        dlg.m_sUsername = CStringUtils::Decrypt(pProject->username).get();
        dlg.m_sPassword = CStringUtils::Decrypt(pProject->password).get();
        dlg.m_monitorInterval = pProject->interval;
    }
    if (dlg.DoModal() == IDOK)
    {
        if (dlg.m_sName.IsEmpty())
            return;
        MonitorItem * pEditProject = pProject;
        if (pProject == nullptr)
            pEditProject = new MonitorItem();
        pEditProject->Name = dlg.m_sName;
        pEditProject->WCPathOrUrl = dlg.m_sPathOrURL;
        pEditProject->interval = dlg.m_monitorInterval;
        pEditProject->username = CStringUtils::Encrypt(dlg.m_sUsername);
        pEditProject->password = CStringUtils::Encrypt(dlg.m_sPassword);
        pEditProject->username.Remove('\r');
        pEditProject->password.Remove('\r');
        pEditProject->username.Replace('\n', ' ');
        pEditProject->password.Replace('\n', ' ');

        // insert the new item
        // if this was an edit, we don't have to do anything since
        // the tree control stores the pointer to the object, and we've just
        // updated that object
        if (pProject == nullptr)
        {
            InsertMonitorItem(pEditProject);
        }

        SaveMonitorProjects();
        RefreshMonitorProjTree();
    }
}

void CLogDlg::SaveMonitorProjects()
{
    m_monitoringFile.Reset();
    int count = 0;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
        CString sSection;
        sSection.Format(L"item_%03d", count++);
        HTREEITEM hParent = m_projTree.GetParentItem(hItem);
        CString sParentPath;
        if (hParent)
        {
            sParentPath = GetTreePath(hParent);
        }
        CString sTmp;
        m_monitoringFile.SetValue(sSection, L"Name", pItem->Name);
        m_monitoringFile.SetValue(sSection, L"parentTreePath", sParentPath);
        m_monitoringFile.SetValue(sSection, L"WCPathOrUrl", pItem->WCPathOrUrl);
        sTmp.Format(L"%lld", pItem->lastchecked);
        m_monitoringFile.SetValue(sSection, L"lastchecked", sTmp);
        sTmp.Format(L"%lld", pItem->lastHEAD);
        m_monitoringFile.SetValue(sSection, L"lastHEAD", sTmp);
        sTmp.Format(L"%d", pItem->UnreadItems);
        m_monitoringFile.SetValue(sSection, L"UnreadItems", sTmp);
        sTmp.Format(L"%d", pItem->interval);
        m_monitoringFile.SetValue(sSection, L"interval", sTmp);
        m_monitoringFile.SetValue(sSection, L"username", pItem->username);
        m_monitoringFile.SetValue(sSection, L"password", pItem->password);
        return false;
    });
}

void CLogDlg::MonitorTimer()
{
    if (m_bLogThreadRunning || m_bMonitorThreadRunning || netScheduler.GetRunningThreadCount())
    {
        SetTimer(MONITOR_TIMER, 60 * 1000, NULL);
        return;
    }

    __time64_t currenttime = NULL;
    _time64(&currenttime);

    CAutoReadWeakLock locker(m_monitorguard);
    if (!locker.IsAcquired())
    {
        SetTimer(MONITOR_TIMER, 1000, NULL);
        return;
    }

    m_monitorItemListForThread.clear();
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
        if (pItem->lastchecked + (pItem->interval * 60) < currenttime)
            m_monitorItemListForThread.push_back(*pItem);
        return false;
    });

    if (!m_monitorItemListForThread.empty())
    {
        InterlockedExchange(&m_bMonitorThreadRunning, TRUE);
        new async::CAsyncCall(this, &CLogDlg::MonitorThread, &netScheduler);
    }
    SetTimer(MONITOR_TIMER, 60 * 1000, NULL);
}

void CLogDlg::MonitorPopupTimer()
{
    if (CAppUtils::IsFullscreenWindowActive())
    {
        // restart the timer and wait until no fullscreen app is active
        SetTimer(MONITOR_POPUP_TIMER, 5000, NULL);
    }
    else
    {
        MonitorAlertWnd * pPopup = new MonitorAlertWnd(GetSafeHwnd());

        pPopup->SetAnimationType(CMFCPopupMenu::ANIMATION_TYPE::SLIDE);
        pPopup->SetAnimationSpeed(40);
        pPopup->SetTransparency(200);
        pPopup->SetSmallCaption(TRUE);
        pPopup->SetAutoCloseTime(5000);

        // Create indirect:
        CMFCDesktopAlertWndInfo params;

        params.m_hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME),
                                          IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        params.m_strText = m_sMonitorNotificationTitle + L"\n" + m_sMonitorNotificationText;
        params.m_strURL = CString(MAKEINTRESOURCE(IDS_MONITOR_NOTIFY_LINK));
        params.m_nURLCmdID = 101;

        pPopup->Create(this, params);

        KillTimer(MONITOR_POPUP_TIMER);
        m_sMonitorNotificationTitle.Empty();
        m_sMonitorNotificationText.Empty();
    }
}

void CLogDlg::MonitorThread()
{
    InterlockedExchange(&m_bMonitorThreadRunning, TRUE);

    __time64_t currenttime = NULL;
    _time64(&currenttime);

    CAutoReadLock locker(m_monitorguard);
    SVN svn(true);
    for (auto& item : m_monitorItemListForThread)
    {
        if (m_bCancelled)
            break;
        if (item.WCPathOrUrl.IsEmpty())
            continue;
        if (item.lastchecked + (item.interval * 60) < currenttime)
        {
            CString sCheckInfo;
            sCheckInfo.Format(IDS_MONITOR_CHECKPROJECT, item.Name);
            SetDlgItemText(IDC_LOGINFO, sCheckInfo);
            svn.SetAuthInfo(CStringUtils::Decrypt(item.username).get(), CStringUtils::Decrypt(item.password).get());
            svn_revnum_t head = svn.GetHEADRevision(CTSVNPath(item.WCPathOrUrl), false);
            if (head != item.lastHEAD)
            {
                // new head revision: fetch the log
                std::unique_ptr<const CCacheLogQuery> cachedData;
                CTSVNPath WCPathOrUrl(item.WCPathOrUrl);
                {
                    CAutoWriteLock pathlock(m_monitorpathguard);
                    m_pathCurrentlyChecked = item.WCPathOrUrl;
                }
                cachedData = svn.ReceiveLog(CTSVNPathList(CTSVNPath(WCPathOrUrl)), SVNRev::REV_HEAD, head, item.lastHEAD, m_limit, false, false, true);
                // Err will also be set if the user cancelled.
                if (m_bCancelled)
                    continue;
                if ((svn.GetSVNError() == nullptr) && (item.lastHEAD >= 0))
                {
                    CString sRoot = svn.GetRepositoryRoot(WCPathOrUrl);
                    CString sUrl = svn.GetURLFromPath(WCPathOrUrl);
                    CString relUrl = sUrl.Mid(sRoot.GetLength());
                    CCachedLogInfo* cache = cachedData->GetCache();
                    const CPathDictionary* paths = &cache->GetLogInfo().GetPaths();
                    CDictionaryBasedTempPath logPath(paths, (const char*)CUnicodeUtils::GetUTF8(relUrl));
                    CLogCacheUtility logUtil(cache, &m_ProjectProperties);

                    for (svn_revnum_t rev = item.lastHEAD + 1; rev <= head; ++rev)
                    {
                        if (logUtil.IsCached(rev))
                        {
                            auto pLogItem = logUtil.GetRevisionData(rev);
                            if (pLogItem)
                            {
                                pLogItem->Finalize(cache, logPath);
                                if (IsRevisionRelatedToUrl(logPath, pLogItem.get()))
                                {
                                    ++item.UnreadItems;
                                }
                            }
                        }
                    }
                    item.lastHEAD = head;
                }
                // we should never get asked for authentication here!
                item.authfailed = PromptShown();
            }
            item.lastchecked = currenttime;
            {
                CAutoWriteLock pathlock(m_monitorpathguard);
                m_pathCurrentlyChecked.Empty();
            }
            SetDlgItemText(IDC_LOGINFO, L"");
        }
        svn.SetAuthInfo(L"", L"");
    }
    SetDlgItemText(IDC_LOGINFO, L"");

    InterlockedExchange(&m_bMonitorThreadRunning, FALSE);
    PostMessage(WM_COMMAND, ID_LOGDLG_MONITOR_THREADFINISHED);
    RefreshCursor();
    m_bCancelled = false;
}

bool CLogDlg::IsRevisionRelatedToUrl(const CDictionaryBasedTempPath& basePath, PLOGENTRYDATA pLogItem)
{
    const auto& changedPathes = pLogItem->GetChangedPaths();
    for (size_t i = 0; i < changedPathes.GetCount(); ++i)
    {
        if (basePath.IsSameOrParentOf(changedPathes[i].GetCachedPath()))
            return true;
    }
    return false;
}

void CLogDlg::OnMonitorThreadFinished()
{
    CString sTemp;
    int changedprojects = 0;
    bool hasUnreadItems = false;
    {
        CAutoReadLock locker(m_monitorguard);
        for (const auto& item : m_monitorItemListForThread)
        {
            HTREEITEM hItem = FindMonitorItem(item.WCPathOrUrl);
            if (hItem)
            {
                MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
                if ((pItem->UnreadItems != item.UnreadItems) && (pItem->lastHEAD != item.lastHEAD))
                {
                    if (item.UnreadItems)
                    {
                        if (changedprojects)
                        {
                            m_sMonitorNotificationTitle.Format(IDS_MONITOR_NOTIFY_MULTITITLE, changedprojects + 1);
                            m_sMonitorNotificationText += L"\n";
                            sTemp.Format(IDS_MONITOR_NOTIFY_TEXT, (LPCWSTR)item.Name, item.UnreadItems - pItem->UnreadItems);
                            m_sMonitorNotificationText += sTemp;
                        }
                        else
                        {
                            m_sMonitorNotificationTitle.LoadString(IDS_MONITOR_NOTIFY_TITLE);
                            m_sMonitorNotificationText.Format(IDS_MONITOR_NOTIFY_TEXT, (LPCWSTR)item.Name, item.UnreadItems - pItem->UnreadItems);
                        }
                        hasUnreadItems = true;
                    }
                    ++changedprojects;
                }
                pItem->lastchecked = item.lastchecked;
                pItem->lastHEAD = item.lastHEAD;
                pItem->UnreadItems = item.UnreadItems;
                pItem->authfailed = item.authfailed;

                m_projTree.SetItemState(hItem, pItem->UnreadItems ? TVIS_BOLD : 0, TVIS_BOLD);
                m_projTree.SetItemState(hItem, pItem->authfailed ? INDEXTOOVERLAYMASK(OVERLAY_MODIFIED) : 0, TVIS_OVERLAYMASK);
            }
        }
    }
    {
        CAutoWriteLock locker(m_monitorguard);
        m_monitorItemListForThread.clear();
    }
    if (hasUnreadItems)
    {
        m_SystemTray.hIcon = m_hMonitorIconNewCommits;
        m_SystemTray.uFlags = NIF_ICON;
        if (Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray) == FALSE)
        {
            Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
            Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
        }
    }

    m_projTree.Invalidate();

    if (!m_sMonitorNotificationTitle.IsEmpty() && !m_sMonitorNotificationText.IsEmpty())
    {
        SetTimer(MONITOR_POPUP_TIMER, 10, NULL);
    }
    SaveMonitorProjects();
}

void CLogDlg::ShutDownMonitoring()
{
    SaveMonitorProjects();

    CString sDataFilePath = CPathUtils::GetAppDataDirectory();
    sDataFilePath += L"\\MonitoringData.ini";
    FILE * pFile = NULL;
    _tfopen_s(&pFile, sDataFilePath, L"wb");
    m_monitoringFile.SaveFile(pFile);
    fclose(pFile);

    // remove all existing data from the control and free the memory
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
        delete pItem;
        m_projTree.SetItemData(hItem, NULL);
        return false;
    });
    m_projTree.DeleteAllItems();
}


void CLogDlg::OnTvnSelchangedProjtree(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    *pResult = 0;
    ::SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_LOGDLG_MONITOR_EDIT, MAKELONG(!!(pNMTreeView->itemNew.state & TVIS_SELECTED), 0));
    ::SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_LOGDLG_MONITOR_REMOVE, MAKELONG(!!(pNMTreeView->itemNew.state & TVIS_SELECTED), 0));
}


void CLogDlg::OnTvnGetdispinfoProjtree(NMHDR *pNMHDR, LRESULT *pResult)
{
    static wchar_t textbuf[1024];
    LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
    *pResult = 0;
    textbuf[0] = 0;
    if (pTVDispInfo->item.mask & TVIF_TEXT)
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(pTVDispInfo->item.hItem);
        if (pItem)
        {
            if (pItem->UnreadItems)
                swprintf_s(textbuf, L"%s (%d)", (LPCWSTR)pItem->Name, pItem->UnreadItems);
            else
                wcscpy_s(textbuf, pItem->Name);
            pTVDispInfo->item.pszText = textbuf;
        }
    }
}

LRESULT CLogDlg::OnTaskbarCallBack(WPARAM /*wParam*/, LPARAM lParam)
{
    switch (lParam)
    {
        case WM_MOUSEMOVE:
        {
            // find the number of unread items
            int unreadItems = 0;
            int unreadProjects = 0;
            RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
            {
                MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
                if (pItem && pItem->UnreadItems)
                {
                    ++unreadProjects;
                    unreadItems += pItem->UnreadItems;
                }
                return false;
            });


            // update the tool tip data
            m_SystemTray.uFlags = NIF_TIP;
            if (unreadItems)
            {
                CString sFormat(MAKEINTRESOURCE(unreadItems == 1 ? IDS_MONITOR_NEWCOMMIT : IDS_MONITOR_NEWCOMMITS));
                swprintf_s(m_SystemTray.szTip, _countof(m_SystemTray.szTip), (LPCWSTR)sFormat, unreadItems, unreadProjects);
            }
            else
                swprintf_s(m_SystemTray.szTip, _countof(m_SystemTray.szTip), L"TortoiseSVN Commit Monitor");
            if (Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray) == FALSE)
            {
                Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
                Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
            }
        }
            break;
        case WM_LBUTTONDBLCLK:
            m_bKeepHidden = false;
            ShowWindow(SW_SHOW);
            SetForegroundWindow();
            m_SystemTray.hIcon = m_hMonitorIconNormal;
            m_SystemTray.uFlags = NIF_ICON;
            if (Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray) == FALSE)
            {
                Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
                Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
            }
            return TRUE;
        case NIN_KEYSELECT:
        case NIN_SELECT:
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
        {
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = ::LoadMenu(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MONITORTRAY));
            hMenu = ::GetSubMenu(hMenu, 0);

            // set the default entry
            MENUITEMINFO iinfo = { 0 };
            iinfo.cbSize = sizeof(MENUITEMINFO);
            iinfo.fMask = MIIM_STATE;
            GetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);
            iinfo.fState |= MFS_DEFAULT;
            SetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);

            // show the menu
            ::SetForegroundWindow(*this);
            int cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, NULL, *this, NULL);
            ::PostMessage(*this, WM_NULL, 0, 0);
            ::DestroyMenu(hMenu);
            switch (cmd)
            {
                case ID_POPUP_EXIT:
                {
                    m_bCancelled = true;
                    bool threadsStillRunning
                        = !netScheduler.WaitForEmptyQueueOrTimeout(8000)
                        || !diskScheduler.WaitForEmptyQueueOrTimeout(8000);

                    if (threadsStillRunning)
                    {
                        // end the process the hard way
                        TerminateProcess(GetCurrentProcess(), 0);
                    }
                    EndDialog(0);
                }
                    break;
                case ID_POPUP_SHOWMONITOR:
                    m_bKeepHidden = false;
                    ShowWindow(SW_SHOW);
                    SetForegroundWindow();
                    break;
            }
        }
            break;
    }
    return TRUE;

}

void CLogDlg::OnNMClickProjtree(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    CPoint pt;
    GetCursorPos(&pt);
    m_projTree.ScreenToClient(&pt);

    UINT unFlags = 0;
    HTREEITEM hItem = m_projTree.HitTest(pt, &unFlags);
    if ((unFlags & TVHT_ONITEM) && (hItem != NULL))
    {
        return MonitorShowProject(hItem);
    }
    *pResult = 0;
}

void CLogDlg::MonitorShowProject(HTREEITEM hItem)
{
    MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
    if (pItem)
    {
        m_ChangedFileListCtrl.SetItemCountEx(0);
        m_ChangedFileListCtrl.Invalidate();
        m_LogList.SetItemCountEx(0);
        m_LogList.Invalidate();
        CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
        pMsgView->SetWindowText(L"");

        m_nSortColumnPathList = 0;
        m_bAscendingPathList = false;
        SetSortArrow(&m_LogList, -1, true);
        m_logEntries.ClearAll();

        GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
        if (pItem->WCPathOrUrl.IsEmpty())
            return;

        // to avoid the log cache from being accessed from two threads
        // at the same time and waiting for the lock to get released,
        // force the monitoring thread to end now and then show the log
        // 
        // it's much faster to end the monitoring thread and wait a few milliseconds
        // than to start the log thread and have that thread wait until
        // the monitoring thread releases the lock for the cache - that might
        // take several seconds!
        if (m_bMonitorThreadRunning)
        {
            m_bCancelled = true;
            bool threadsStillRunning
                = !netScheduler.WaitForEmptyQueueOrTimeout(5000)
                || !diskScheduler.WaitForEmptyQueueOrTimeout(5000);

            // can we close the app cleanly?

            if (threadsStillRunning)
            {
                return;
            }
        }
        m_bCancelled = false;

        DialogEnableWindow(IDC_PROJTREE, FALSE);

        svn_revnum_t head = pItem->lastHEAD;
        if (head == 0)
            head = GetHEADRevision(CTSVNPath(pItem->WCPathOrUrl), false);

        m_path = CTSVNPath(pItem->WCPathOrUrl);
        m_pegrev = head;
        m_head = head;
        m_startrev = head;
        m_bStartRevIsHead = false;
        m_LogRevision = head;
        m_endrev = max(1, head - m_limit + 1);
        m_hasWC = m_path.IsUrl();
        m_bStrict = false;
        m_bSaveStrict = false;
        m_revUnread = head - pItem->UnreadItems;
        m_hasWC = !m_path.IsUrl();
        m_ProjectProperties = ProjectProperties();
        ReadProjectPropertiesAndBugTraqInfo();
        ConfigureColumnsForLogListControl();

        m_wcRev = SVNRev();
        if (::IsWindow(m_hWnd))
            UpdateData(FALSE);

        pItem->UnreadItems = 0;

        InterlockedExchange(&m_bLogThreadRunning, TRUE);
        new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);

        m_projTree.SetItemState(hItem, pItem->UnreadItems ? TVIS_BOLD : 0, TVIS_BOLD);
        m_projTree.SetItemState(hItem, pItem->authfailed ? INDEXTOOVERLAYMASK(OVERLAY_MODIFIED) : 0, TVIS_OVERLAYMASK);
    }

    bool hasUnreadItems = false;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem)->bool
    {
        MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(hItem);
        if (pItem && pItem->UnreadItems)
        {
            hasUnreadItems = true;
            return true;
        }
        return false;
    });
    m_SystemTray.hIcon = hasUnreadItems ? m_hMonitorIconNewCommits : m_hMonitorIconNormal;
    m_SystemTray.uFlags = NIF_ICON;
    if (Shell_NotifyIcon(NIM_MODIFY, &m_SystemTray) == FALSE)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_SystemTray);
        Shell_NotifyIcon(NIM_ADD, &m_SystemTray);
    }
}

LRESULT CLogDlg::OnMonitorNotifyClick(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_bKeepHidden = false;
    ShowWindow(SW_SHOW);
    SetForegroundWindow();
    return 0;
}


void CLogDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    if (m_bKeepHidden)
        lpwndpos->flags &= ~SWP_SHOWWINDOW;

    __super::OnWindowPosChanging(lpwndpos);
}

LRESULT CLogDlg::OnTaskbarCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    RefreshMonitorProjTree();
    return 0;
}

LRESULT CLogDlg::OnTreeDrop(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    SaveMonitorProjects();
    RefreshMonitorProjTree();
    return 0;
}

void CLogDlg::OnTvnEndlabeleditProjtree(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

    MonitorItem * pItem = (MonitorItem *)m_projTree.GetItemData(pTVDispInfo->item.hItem);
    if (pItem && pTVDispInfo->item.pszText)
    {
        CString sName = pTVDispInfo->item.pszText;
        if (sName != pItem->Name)
        {
            pItem->Name = sName;
            SaveMonitorProjects();
            RefreshMonitorProjTree();
        }
    }

    *pResult = 0;
}

void CLogDlg::OnInlineedit()
{
    if (GetFocus() == &m_projTree)
    {
        HTREEITEM hTreeItem = m_projTree.GetSelectedItem();
        m_projTree.EditLabel(hTreeItem);
    }
}

void CLogDlg::ShowContextMenuForMonitorTree(CWnd* /*pWnd*/, CPoint point)
{
    m_bCancelled = false;
    HTREEITEM hSelectedTreeItem = m_projTree.GetSelectedItem();
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        m_projTree.GetItemRect(hSelectedTreeItem, &rect, TRUE);
        m_projTree.ClientToScreen(&rect);
        point = rect.CenterPoint();
    }

    UINT uFlags;
    CPoint ptTree = point;
    m_projTree.ScreenToClient(&ptTree);
    HTREEITEM hItem = m_projTree.HitTest(ptTree, &uFlags);
    // in case the right-clicked item is not the selected one, select it
    if ((hItem) && (uFlags & TVHT_ONITEM) && (hItem != hSelectedTreeItem))
    {
        m_projTree.SelectItem(hItem);
    }

    // entry is selected, now show the popup menu
    CIconMenu popup;
    if (!popup.CreatePopupMenu())
        return;

    popup.AppendMenuIcon(ID_LOGDLG_MONITOR_EDIT, IDS_LOG_POPUP_MONITOREDIT, IDI_MONITOR_EDIT);
    popup.AppendMenuIcon(ID_LOGDLG_MONITOR_REMOVE, IDS_LOG_POPUP_MONITORREMOVE, IDI_MONITOR_REMOVE);


    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY |
                                   TPM_RIGHTBUTTON, point.x, point.y, this, 0);
    CLogWndHourglass wait;

    switch (cmd)
    {
        case ID_LOGDLG_MONITOR_EDIT:
            OnMonitorEditProject();
            break;
        case ID_LOGDLG_MONITOR_REMOVE:
            OnMonitorRemoveProject();
            break;
        default:
            break;
    } // switch (cmd)
}
