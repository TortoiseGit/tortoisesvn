// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2022 - TortoiseSVN

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
#include "Callback.h"
#include "SVNDataObject.h"
#include "RenameDlg.h"
#include "../../ext/snarl/SnarlInterface.h"
#include "ToastNotifications.h"
#include "DPIAware.h"
#include "Theme.h"
#include "QuickHashSet.h"
#include <tlhelp32.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>

#include "../LogCache/Streams/StreamException.h"
#include <Commands/SyncCommand.h>

#define ICONITEMBORDER      5
#define MIN_CTRL_HEIGHT     (CDPIAware::Instance().Scale(GetSafeHwnd(), 20))
#define MIN_SPLITTER_HEIGHT (CDPIAware::Instance().Scale(GetSafeHwnd(), 10))

const UINT CLogDlg::FIND_DIALOG_MESSAGE              = RegisterWindowMessage(FINDMSGSTRING);
const UINT CLogDlg::WM_TASKBARCREATED                = RegisterWindowMessage(L"TaskbarCreated");
const UINT CLogDlg::WM_TSVN_COMMITMONITOR_SHOWDLGMSG = RegisterWindowMessage(_T("TSVNCommitMonitor_ShowDlgMsg"));
const UINT CLogDlg::WM_TSVN_COMMITMONITOR_RELOADINI  = RegisterWindowMessage(_T("TSVNCommitMonitor_ReloadIni"));
const UINT CLogDlg::WM_TASKBARBUTTON_CREATED         = RegisterWindowMessage(L"TaskbarButtonCreated");

#define WM_TSVN_REFRESH_SELECTION       (WM_APP + 1)
#define WM_TSVN_MONITOR_TASKBARCALLBACK (WM_APP + 2)
#define WM_TSVN_MONITOR_NOTIFY_CLICK    (WM_APP + 3)
#define WM_TSVN_MONITOR_TREEDROP        (WM_APP + 4)
#define WM_TSVN_MONITOR_SNARLREPLY      (WM_APP + 5)

#define OVERLAY_MODIFIED 1

auto g_snarlInterface = Snarl::V42::SnarlInterface();
UINT g_snarlGlobalMsg = 0;

#define SNARL_APP_ID                 L"TSVN/ProjectMonitor"
#define SNARL_CLASS_ID               L"TSVN/ProjectMonitorNotification"
#define SNARL_NOTIFY_CLICK_EVENT     1000
#define SNARL_NOTIFY_CLICK_EVENT_STR L"@1000"

class MonitorAlertWnd : public CMFCDesktopAlertWnd
{
public:
    MonitorAlertWnd(HWND hParent)
        : CMFCDesktopAlertWnd()
        , m_hParent(hParent)
    {
    }
    ~MonitorAlertWnd() override {}

    BOOL OnClickLinkButton(UINT uiCmdID) override
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
    ID_COPYCLIPBOARDURLREV,
    ID_COPYCLIPBOARDURLVIEWERREV,
    ID_COPYCLIPBOARDURLVIEWERPATHREV,
    ID_COPYCLIPBOARDURLTSVNSHOWCOMPARE,
    ID_COPYCLIPBOARDBUGID,
    ID_COPYCLIPBOARDBUGURL,
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
    ID_CODE_COLLABORATOR,
    ID_EXPLORE
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
    , m_pNotifyWindow(nullptr)
    , m_wParam(0)
    , m_themeCallbackId(0)
    , m_bStartRevIsHead(true)
    , m_head(-1)
    , m_nRefresh(None)
    , m_tempRev(0)
    , m_cMergedRevisionsReceived(0)
    , m_bSelectionMustBeContinuous(false)
    , m_bCancelled(FALSE)
    , m_bLogThreadRunning(FALSE)
    , m_bStrict(false)
    , m_bStrictStopped(false)
    , m_bIncludeMerges(FALSE)
    , m_bSaveStrict(false)
    , m_bHideNonMergeables(FALSE)
    , m_bSingleRevision(true)
    , m_hasWC(false)
    , m_nSearchIndex(0)
    , m_pFindDialog(nullptr)
    , m_selectedFilters(LOGFILTER_ALL)
    , m_tFrom(0)
    , m_tTo(0)
    , m_timeFromSetFromCmdLine(0)
    , m_timeToSetFromCmdLine(0)
    , m_limit(0)
    , m_nSortColumn(0)
    , m_bAscending(FALSE)
    , m_nSortColumnPathList(0)
    , m_bAscendingPathList(false)
    , m_regLastStrict(L"Software\\TortoiseSVN\\LastLogStrict", FALSE)
    , m_regMaxBugIDColWidth(L"Software\\TortoiseSVN\\MaxBugIDColWidth", 200)
    , m_bShowedAll(false)
    , m_bSelect(false)
    , m_bShowBugtraqColumn(false)
    , m_sLogInfo(L"")
    , m_copyFromRev(0)
    , m_lastTooltipRect({0})
    , m_hModifiedIcon(nullptr)
    , m_hReplacedIcon(nullptr)
    , m_hAddedIcon(nullptr)
    , m_hDeletedIcon(nullptr)
    , m_hMergedIcon(nullptr)
    , m_hReverseMergedIcon(nullptr)
    , m_hMovedIcon(nullptr)
    , m_hMoveReplacedIcon(nullptr)
    , m_nIconFolder(0)
    , m_nOpenIconFolder(0)
    , m_hAccel(nullptr)
    , m_pStoreSelection(nullptr)
    , m_bEnsureSelection(false)
    , m_prevLogEntriesSize(0)
    , netScheduler(1, 0, true)
    , diskScheduler(1, 0, true)
    , vsRunningScheduler(1, 0, true)
    , m_pLogListAccServer(nullptr)
    , m_pChangedListAccServer(nullptr)
    , m_bVisualStudioRunningAtStart(false)
    , m_bMonitoringMode(false)
    , m_bKeepHidden(false)
    , m_hwndToolbar(nullptr)
    , m_bMonitorThreadRunning(FALSE)
    , m_nMonitorUrlIcon(0)
    , m_nMonitorWCIcon(0)
    , m_nErrorOvl(0)
    , m_hMonitorIconNormal(nullptr)
    , m_hMonitorIconNewCommits(nullptr)
    , m_revUnread(0)
    , m_bPlaySound(true)
    , m_bShowNotification(true)
    , m_defaultMonitorInterval(30)
    , m_bSystemShutDown(false)
    , m_pTreeDropTarget(nullptr)
{
    SecureZeroMemory(&m_systemTray, sizeof(m_systemTray));
    m_bFilterWithRegex =
        !!CRegDWORD(L"Software\\TortoiseSVN\\UseRegexFilter", FALSE);
    m_bFilterCaseSensitively =
        !!CRegDWORD(L"Software\\TortoiseSVN\\FilterCaseSensitively", FALSE);
    m_sMultiLogFormat = CRegString(L"Software\\TortoiseSVN\\LogMultiRevFormat", L"r%1!ld!\n%2!s!\n---------------------\n");
    m_sMultiLogFormat.Replace(L"\\r", L"\r");
    m_sMultiLogFormat.Replace(L"\\n", L"\n");

    // just in case the user sets an impossible/illegal format string: try to use that format
    // string and handle possible exceptions. In case of an exception, fall back to the default.
    if (!CStringUtils::ValidateFormatString(m_sMultiLogFormat, 0, L"test"))
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
        Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
    }
    if (m_pStoreSelection)
    {
        m_pStoreSelection->ClearSelection();
    }
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LOGLIST, m_logList);
    DDX_Control(pDX, IDC_LOGMSG, m_changedFileListCtrl);
    DDX_Control(pDX, IDC_PROGRESS, m_logProgress);
    DDX_Control(pDX, IDC_SPLITTERTOP, m_wndSplitter1);
    DDX_Control(pDX, IDC_SPLITTERBOTTOM, m_wndSplitter2);
    DDX_Control(pDX, IDC_SPLITTERLEFT, m_wndSplitterLeft);
    DDX_Check(pDX, IDC_CHECK_STOPONCOPY, m_bStrict);
    DDX_Text(pDX, IDC_SEARCHEDIT, m_sFilterText);
    DDX_Control(pDX, IDC_DATEFROM, m_dateFrom);
    DDX_Control(pDX, IDC_DATETO, m_dateTo);
    DDX_Control(pDX, IDC_SHOWPATHS, m_cShowPaths);
    DDX_Control(pDX, IDC_GETALL, m_btnShow);
    DDX_Text(pDX, IDC_LOGINFO, m_sLogInfo);
    DDX_Check(pDX, IDC_INCLUDEMERGE, m_bIncludeMerges);
    DDX_Control(pDX, IDC_SEARCHEDIT, m_cFilter);
    DDX_Check(pDX, IDC_HIDENONMERGEABLE, m_bHideNonMergeables);
    DDX_Control(pDX, IDC_PROJTREE, m_projTree);
}

BEGIN_MESSAGE_MAP(CLogDlg, CResizableStandAloneDialog)
    ON_REGISTERED_MESSAGE(FIND_DIALOG_MESSAGE, OnFindDialogMessage)
    ON_REGISTERED_MESSAGE(WM_TASKBARCREATED, OnTaskbarCreated)
    ON_REGISTERED_MESSAGE(WM_TSVN_COMMITMONITOR_SHOWDLGMSG, OnShowDlgMsg)
    ON_REGISTERED_MESSAGE(WM_TSVN_COMMITMONITOR_RELOADINI, OnReloadIniMsg)
    ON_REGISTERED_MESSAGE(WM_TOASTNOTIFICATION, OnToastNotification)
    ON_MESSAGE(WM_TSVN_REFRESH_SELECTION, OnRefreshSelection)
    ON_MESSAGE(WM_TSVN_MONITOR_TASKBARCALLBACK, OnTaskbarCallBack)
    ON_MESSAGE(WM_TSVN_MONITOR_NOTIFY_CLICK, OnMonitorNotifyClick)
    ON_MESSAGE(WM_TSVN_MONITOR_TREEDROP, OnTreeDrop)
    ON_MESSAGE(WM_TSVN_MONITOR_SNARLREPLY, OnMonitorNotifySnarlReply)
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
    ON_BN_CLICKED(IDC_LOGCANCEL, &CLogDlg::OnLogCancel)
    ON_COMMAND(ID_LOGDLG_REFRESH, &CLogDlg::OnRefresh)
    ON_COMMAND(ID_LOGDLG_FIND, &CLogDlg::OnFind)
    ON_COMMAND(ID_LOGDLG_FOCUSFILTER, &CLogDlg::OnFocusFilter)
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
    ON_COMMAND(ID_MISC_MARKALLASREAD, &CLogDlg::OnMonitorMarkAllAsRead)
    ON_COMMAND(ID_LOGDLG_MONITOR_CLEARERRORS, &CLogDlg::OnMonitorClearErrors)
    ON_COMMAND(ID_MISC_UPDATE, &CLogDlg::OnMonitorUpdateAll)
    ON_COMMAND(ID_MISC_OPTIONS, &CLogDlg::OnMonitorOptions)
    ON_COMMAND(ID_LOGDLG_MONITOR_THREADFINISHED, &CLogDlg::OnMonitorThreadFinished)
    ON_NOTIFY(TVN_SELCHANGED, IDC_PROJTREE, &CLogDlg::OnTvnSelchangedProjtree)
    ON_NOTIFY(TVN_GETDISPINFO, IDC_PROJTREE, &CLogDlg::OnTvnGetdispinfoProjtree)
    ON_NOTIFY(NM_CLICK, IDC_PROJTREE, &CLogDlg::OnNMClickProjtree)
    ON_WM_WINDOWPOSCHANGING()
    ON_NOTIFY(TVN_ENDLABELEDIT, IDC_PROJTREE, &CLogDlg::OnTvnEndlabeleditProjtree)
    ON_COMMAND(ID_INLINEEDIT, &CLogDlg::OnInlineedit)
    ON_WM_QUERYENDSESSION()
    ON_REGISTERED_MESSAGE(WM_TASKBARBUTTON_CREATED, OnTaskbarButtonCreated)
    ON_NOTIFY(LVN_BEGINDRAG, IDC_LOGMSG, &CLogDlg::OnLvnBegindragLogmsg)
    ON_WM_SYSCOLORCHANGE()
    ON_MESSAGE(WM_DPICHANGED, OnDPIChanged)
END_MESSAGE_MAP()

void CLogDlg::SetParams(const CTSVNPath& path, const SVNRev& pegRev, const SVNRev& startRev, const SVNRev& endRev,
                        BOOL bStrict /* = FALSE */, BOOL bSaveStrict /* = TRUE */, int limit)
{
    m_path            = path;
    m_pegRev          = pegRev;
    m_startRev        = startRev;
    m_bStartRevIsHead = !!m_startRev.IsHead();
    m_logRevision     = startRev;
    m_endRev          = endRev;
    m_hasWC           = !path.IsUrl();
    m_bStrict         = bStrict;
    m_bSaveStrict     = bSaveStrict;
    m_limit           = limit;
    if (::IsWindow(m_hWnd))
        UpdateData(FALSE);
}

void CLogDlg::SetFilter(const CString& findStr, LONG findType, bool findRegex, const CString& sDateFrom, const CString& sDateTo)
{
    m_sFilterText = findStr;
    if (findType)
        m_selectedFilters = findType;
    m_bFilterWithRegex = findRegex;
    if (!sDateFrom.IsEmpty())
    {
        SVNRev rDate(sDateFrom);
        if (rDate.IsValid())
        {
            // / 1000000L to convert svn time to windows time
            m_timeFromSetFromCmdLine = rDate.GetDate() / 1000000L;
        }
    }
    if (!sDateTo.IsEmpty())
    {
        SVNRev rDate(sDateTo);
        if (rDate.IsValid())
        {
            // / 1000000L to convert svn time to windows time
            m_timeToSetFromCmdLine = rDate.GetDate() / 1000000L;
        }
    }
}

void CLogDlg::SetSelectedRevRanges(const SVNRevRangeArray& revArray)
{
    m_pStoreSelection = nullptr;
    m_pStoreSelection = std::make_unique<CStoreSelection>(this, revArray);
    if (revArray.GetCount() && revArray.GetLowestRevision().IsValid() && revArray.GetLowestRevision().IsNumber() && (static_cast<svn_revnum_t>(revArray.GetLowestRevision()) > 0))
    {
        m_bEnsureSelection = true;
        if (m_endRev.IsValid() && m_endRev.IsNumber())
            m_endRev = min(static_cast<svn_revnum_t>(m_endRev), static_cast<svn_revnum_t>(revArray.GetLowestRevision()));
        else
            m_endRev = revArray.GetLowestRevision();
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
    m_logFont.DeleteObject();
    m_unreadFont.DeleteObject();
    m_wcRevFont.DeleteObject();
    CFont*  font = m_logList.GetFont();
    LOGFONT lf   = {0};
    font->GetLogFont(&lf);
    lf.lfWeight = FW_DEMIBOLD;
    m_unreadFont.CreateFontIndirect(&lf);
    lf.lfWeight = FW_BOLD;
    m_wcRevFont.CreateFontIndirect(&lf);
    CAppUtils::CreateFontForLogs(GetSafeHwnd(), m_logFont);
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
        temp.Format(IDS_LOG_SHOWNEXT, static_cast<int>(static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs", 100))));

    SetDlgItemText(IDC_NEXTHUNDRED, temp);

    // Show paths checkbox
    int checkState = static_cast<int>(static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\LogShowPaths", BST_UNCHECKED)));
    m_cShowPaths.SetCheck(checkState);

    switch (static_cast<LONG>(CRegDWORD(L"Software\\TortoiseSVN\\ShowAllEntry")))
    {
        default:
        case 0:
            m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWALL)));
            break;
        case 1:
            m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWRANGE)));
            break;
    }
}

void CLogDlg::SetupLogMessageViewControl()
{
    auto pWnd = GetDlgItem(IDC_MSGVIEW);
    // set the font to use in the log message view, configured in the settings dialog
    pWnd->SetFont(&m_logFont);
    // make the log message rich edit control send a message when the mouse pointer is over a link
    pWnd->SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK | ENM_SCROLL);

    CHARFORMAT2 format = {0};
    format.cbSize      = sizeof(CHARFORMAT2);
    format.dwMask      = CFM_COLOR | CFM_BACKCOLOR;
    if (CTheme::Instance().IsDarkTheme())
    {
        format.crTextColor = CTheme::darkTextColor;
        format.crBackColor = CTheme::darkBkColor;
    }
    else
    {
        format.crTextColor = GetSysColor(COLOR_WINDOWTEXT);
        format.crBackColor = GetSysColor(COLOR_WINDOW);
    }
    pWnd->SendMessage(EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));
    pWnd->SendMessage(EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(format.crBackColor));
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
        dwStyle |= LVS_EX_CHECKBOXES | LVS_EX_AUTOCHECKSELECT;
    m_logList.SetExtendedStyle(dwStyle);
    m_logList.SetTooltipProvider(this);

    // Configure fake imagelist for LogList with 1px width and height = GetSystemMetrics(SM_CYSMICON)
    // to set minimum item height: we draw icons in actions column, but on High-DPI
    // displays font height may be less than small icon height.
    ASSERT((m_logList.GetStyle() & LVS_SHAREIMAGELISTS) == 0);
    HIMAGELIST hImageList = ImageList_Create(1, GetSystemMetrics(SM_CYSMICON), 0, 1, 0);
    ListView_SetImageList(m_logList, hImageList, LVSIL_SMALL);
}

void CLogDlg::LoadIconsForActionColumns()
{
    // load the icons for the action columns
    int cx = GetSystemMetrics(SM_CXSMICON);
    int cy = GetSystemMetrics(SM_CYSMICON);

    m_hModifiedIcon      = CCommonAppUtils::LoadIconEx(IDI_ACTIONMODIFIED, cx, cy);
    m_hReplacedIcon      = CCommonAppUtils::LoadIconEx(IDI_ACTIONREPLACED, cx, cy);
    m_hAddedIcon         = CCommonAppUtils::LoadIconEx(IDI_ACTIONADDED, cx, cy);
    m_hDeletedIcon       = CCommonAppUtils::LoadIconEx(IDI_ACTIONDELETED, cx, cy);
    m_hMergedIcon        = CCommonAppUtils::LoadIconEx(IDI_ACTIONMERGED, cx, cy);
    m_hReverseMergedIcon = CCommonAppUtils::LoadIconEx(IDI_ACTIONREVERSEMERGED, cx, cy);
    m_hMovedIcon         = CCommonAppUtils::LoadIconEx(IDI_ACTIONREPLACED, cx, cy);
    m_hMoveReplacedIcon  = CCommonAppUtils::LoadIconEx(IDI_ACTIONREPLACED, cx, cy);
}

void CLogDlg::ConfigureColumnsForLogListControl()
{
    CString temp;
    // set up the columns
    int c = m_logList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_logList.DeleteColumn(c--);
    temp.LoadString(IDS_LOG_REVISION);
    m_logList.InsertColumn(0, temp);

    // make the revision column right aligned
    LVCOLUMN column;
    column.mask = LVCF_FMT;
    column.fmt  = LVCFMT_RIGHT;
    m_logList.SetColumn(0, &column);

    temp.LoadString(IDS_LOG_ACTIONS);
    m_logList.InsertColumn(1, temp);
    temp.LoadString(IDS_LOG_AUTHOR);
    m_logList.InsertColumn(2, temp);
    temp.LoadString(IDS_LOG_DATE);
    m_logList.InsertColumn(3, temp);
    if (m_bShowBugtraqColumn)
    {
        temp = m_projectProperties.sLabel;
        if (temp.IsEmpty())
            temp.LoadString(IDS_LOG_BUGIDS);
        m_logList.InsertColumn(4, temp);
    }
    temp.LoadString(IDS_LOG_MESSAGE);
    m_logList.InsertColumn(m_bShowBugtraqColumn ? 5 : 4, temp);
    m_logList.SetRedraw(false);
    ResizeAllListCtrlCols(true);
    m_logList.SetRedraw(true);

    if (!CTheme::Instance().IsDarkTheme())
        SetWindowTheme(m_logList.GetSafeHwnd(), L"Explorer", nullptr);
    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::ConfigureColumnsForChangedFileListControl()
{
    CString temp;
    m_changedFileListCtrl.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
    m_changedFileListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    int c = m_changedFileListCtrl.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_changedFileListCtrl.DeleteColumn(c--);
    temp.LoadString(IDS_PROGRS_PATH);
    m_changedFileListCtrl.InsertColumn(0, temp);
    temp.LoadString(IDS_PROGRS_ACTION);
    m_changedFileListCtrl.InsertColumn(1, temp);
    temp.LoadString(IDS_LOG_COPYFROM);
    m_changedFileListCtrl.InsertColumn(2, temp);
    temp.LoadString(IDS_LOG_REVISION);
    m_changedFileListCtrl.InsertColumn(3, temp);
    m_changedFileListCtrl.SetRedraw(false);
    CAppUtils::ResizeAllListCtrlCols(&m_changedFileListCtrl);
    m_changedFileListCtrl.SetRedraw(true);

    if (!CTheme::Instance().IsDarkTheme())
        SetWindowTheme(m_changedFileListCtrl.GetSafeHwnd(), L"Explorer", nullptr);
}

void CLogDlg::SetupFilterControlBitmaps()
{
    // the filter control has a 'cancel' button (the red 'X'), we need to load its bitmap
    m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED, 14, 14);
    m_cFilter.SetInfoIcon(IDI_LOGFILTER, 19, 19);
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
    DWORD yPos1 = CRegDWORD(m_bMonitoringMode ? L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1M" : L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1");
    DWORD yPos2 = CRegDWORD(m_bMonitoringMode ? L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2M" : L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2");
    DWORD xPos  = CRegDWORD(m_bMonitoringMode ? L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer3M" : L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer3");
    RECT  rcDlg, rcLogList, rcChgMsg, rcProjTree;
    GetClientRect(&rcDlg);
    m_logList.GetWindowRect(&rcLogList);
    ScreenToClient(&rcLogList);
    m_changedFileListCtrl.GetWindowRect(&rcChgMsg);
    ScreenToClient(&rcChgMsg);
    m_projTree.GetWindowRect(&rcProjTree);
    ScreenToClient(&rcProjTree);

    if (yPos1 && (static_cast<LONG>(yPos1) < rcDlg.bottom - CDPIAware::Instance().Scale(GetSafeHwnd(), 185)))
    {
        RECT rectSplitter;
        m_wndSplitter1.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        int delta = yPos1 - rectSplitter.top;

        if ((rcLogList.bottom + delta > rcLogList.top) && (rcLogList.bottom + delta < rcChgMsg.bottom - CDPIAware::Instance().Scale(GetSafeHwnd(), 30)))
        {
            m_wndSplitter1.SetWindowPos(nullptr, rectSplitter.left, yPos1, 0, 0, SWP_NOSIZE);
            DoSizeV1(delta);
        }
    }
    if (yPos2 && (static_cast<LONG>(yPos2) < rcDlg.bottom - CDPIAware::Instance().Scale(GetSafeHwnd(), 153)))
    {
        RECT rectSplitter;
        m_wndSplitter2.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        int delta = yPos2 - rectSplitter.top;

        if ((rcChgMsg.top + delta < rcChgMsg.bottom) && (rcChgMsg.top + delta > rcLogList.top + CDPIAware::Instance().Scale(GetSafeHwnd(), 30)))
        {
            m_wndSplitter2.SetWindowPos(nullptr, rectSplitter.left, yPos2, 0, 0, SWP_NOSIZE);
            DoSizeV2(delta);
        }
    }
    if (m_bMonitoringMode)
    {
        if (xPos == 0)
            xPos = CDPIAware::Instance().Scale(GetSafeHwnd(), 80);
        RECT rectSplitter;
        m_wndSplitterLeft.GetWindowRect(&rectSplitter);
        ScreenToClient(&rectSplitter);
        int delta = xPos - rectSplitter.left;
        if ((rcProjTree.right + delta > rcProjTree.left) && (rcProjTree.right + delta < m_logListOrigRect.Width()))
        {
            m_wndSplitterLeft.SetWindowPos(nullptr, xPos, rectSplitter.top, 0, 0, SWP_NOSIZE);
            DoSizeV3(delta);
        }
    }
    SetSplitterRange();
    Invalidate();
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
    GetClientRect(m_dlgOrigRect);
    m_logList.GetClientRect(m_logListOrigRect);
    GetDlgItem(IDC_MSGVIEW)->GetClientRect(m_msgViewOrigRect);
    m_changedFileListCtrl.GetClientRect(m_chgOrigRect);
    m_projTree.GetClientRect(m_projTreeOrigRect);
}

void CLogDlg::SetupDatePickerControls()
{
    m_dateFrom.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS | MCS_NOTODAY | MCS_NOTRAILINGDATES | MCS_NOSELCHANGEONNAV);
    m_dateTo.SendMessage(DTM_SETMCSTYLE, 0, MCS_WEEKNUMBERS | MCS_NOTODAY | MCS_NOTRAILINGDATES | MCS_NOSELCHANGEONNAV);

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
            DialogEnableWindow(IDOK, (m_logList.GetSelectedCount() != 0) && (IsSelectionContinuous()));
        else
            DialogEnableWindow(IDOK, m_logList.GetSelectedCount() != 0);
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
    m_btnMenu.AppendMenu(MF_STRING | MF_BYCOMMAND, ID_CMD_SHOWALL,
                         CString(MAKEINTRESOURCE(IDS_LOG_SHOWALL)));
    m_btnMenu.AppendMenu(MF_STRING | MF_BYCOMMAND, ID_CMD_SHOWRANGE,
                         CString(MAKEINTRESOURCE(IDS_LOG_SHOWRANGE)));
    m_btnShow.m_hMenu         = m_btnMenu.GetSafeHmenu();
    m_btnShow.m_bOSMenu       = TRUE;
    m_btnShow.m_bRightArrow   = TRUE;
    m_btnShow.m_bDefaultClick = TRUE;
    m_btnShow.m_bTransparent  = TRUE;
}

void CLogDlg::ReadProjectPropertiesAndBugTraqInfo()
{
    // if there is a working copy, load the project properties
    // to get information about the bugtraq: integration
    if (m_hasWC)
        m_projectProperties.ReadProps(m_path);

    // the bugtraq issue id column is only shown if the bugtraq:url or bugtraq:regex is set
    if ((!m_projectProperties.sUrl.IsEmpty()) || (!m_projectProperties.GetCheckRe().IsEmpty()))
        m_bShowBugtraqColumn = true;
}

void CLogDlg::SetupToolTips()
{
    EnableToolTips();
    CheckRegexpTooltip();
}

void CLogDlg::InitializeTaskBarListPtr()
{
    m_pTaskbarList.Release();
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
    m_pLogListAccServer     = ListViewAccServer::CreateProvider(m_logList.GetSafeHwnd(), this);
    m_pChangedListAccServer = ListViewAccServer::CreateProvider(m_changedFileListCtrl.GetSafeHwnd(), this);
}

void CLogDlg::ExtraInitialization()
{
    m_hAccel          = LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_ACC_LOGDLG));
    m_nIconFolder     = SYS_IMAGE_LIST().GetDirIconIndex();
    m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();
    m_sMessageBuf.Preallocate(100000);
    m_mergedRevs.clear();
}

BOOL CLogDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
        [this]() {
            SetTheme(CTheme::Instance().IsDarkTheme());
        });

    ExtraInitialization();
    InitializeTaskBarListPtr();
    if (!m_bMonitoringMode)
    {
        ExtendFrameIntoClientArea(IDC_LOGMSG, IDC_SEARCHEDIT, IDC_LOGMSG, IDC_LOGMSG);

        SubclassControls();
    }
    else
        InitMonitoringMode();

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
    SetFilterCueText();
    ConfigureResizableControlAnchors();
    SetupDatePickerControls();
    SetPromptParentWindow(m_hWnd);
    CenterThisWindow();
    EnableSaveRestore(m_bMonitoringMode ? L"MonitorLogDlg" : L"LogDlg");
    SetSplitterRange();
    RestoreLogDlgWindowAndSplitters();
    ConfigureDialogForPickingRevisionsOrShowingLog();
    SetupButtonMenu();
    SetupAccessibility();
    SetupToolTips();

    SetTheme(CTheme::Instance().IsDarkTheme());

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
        return;
    }
    if (m_sTitle.IsEmpty())
        GetWindowText(m_sTitle);

    if (bOffline)
    {
        CString sTemp;
        sTemp.FormatMessage(IDS_LOG_DLGTITLEOFFLINE, static_cast<LPCWSTR>(m_sTitle));
        CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sTemp);
    }
    else
    {
        CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), m_sTitle);
    }
}

void CLogDlg::CheckRegexpTooltip()
{
    CWnd* pWnd = GetDlgItem(IDC_SEARCHEDIT);

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
            DialogEnableWindow(IDOK, (m_logList.GetSelectedCount() != 0) && (IsSelectionContinuous()));
        else
            DialogEnableWindow(IDOK, m_logList.GetSelectedCount() != 0);
    }
    else
        DialogEnableWindow(IDOK, TRUE);
}

namespace
{
bool IsAllWhitespace(const std::wstring& text, long first, long last)
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
    auto end   = ranges.end();

    auto target = begin;
    for (auto source = begin + 1; source != end; ++source)
        if (IsAllWhitespace(text, target->cpMax, source->cpMin))
            target->cpMax = source->cpMax;
        else
            *(++target) = *source;

    ranges.erase(++target, end);
}
} // namespace

namespace
{
struct SMarkerInfo
{
    CString      sText;
    std::wstring text;

    std::vector<CHARRANGE> ranges;
    std::vector<CHARRANGE> idRanges;
    std::vector<CHARRANGE> revRanges;
    std::vector<CHARRANGE> urlRanges;

    BOOL RunRegex(ProjectProperties* project)
    {
        // turn bug ID's into links if the bugtraq: properties have been set
        // and we can find a match of those in the log message
        idRanges = project->FindBugIDPositions(sText);

        // underline all revisions mentioned in the message
        revRanges = CAppUtils::FindRegexMatches(text, project->GetLogRevRegex(), L"\\d+");

        urlRanges = CAppUtils::FindURLMatches(sText);

        return TRUE;
    }
};

} // namespace

void CLogDlg::FillLogMessageCtrl(bool bShow /* = true*/)
{
    // we fill here the log message rich edit control,
    // and also populate the changed files list control
    // according to the selected revision(s).

    CWnd* pMsgView = GetDlgItem(IDC_MSGVIEW);
    if (pMsgView == nullptr)
        return; // can happen if the dialog is already closed, but the threads are still running
    // empty the log message view
    pMsgView->SetWindowText(L" ");
    // empty the changed files list
    m_changedFileListCtrl.SetRedraw(FALSE);
    OnOutOfScope(m_changedFileListCtrl.SetRedraw(TRUE));

    m_currentChangedArray.RemoveAll();
    m_changedFileListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_changedFileListCtrl.SetItemCountEx(0);

    // if we're not here to really show a selected revision, just
    // get out of here after clearing the views, which is what is intended
    // if that flag is not set.
    if (!bShow)
    {
        // force a redraw
        m_changedFileListCtrl.Invalidate();
        return;
    }

    // depending on how many revisions are selected, we have to do different
    // tasks.
    int selCount = m_logList.GetSelectedCount();
    if (selCount == 0)
    {
        // if nothing is selected, we have nothing more to do
        return;
    }
    else
    {
        m_currentChangedPathList.Clear();
        m_bSingleRevision = selCount == 1;

        // if one revision is selected, we have to fill the log message view
        // with the corresponding log message, and also fill the changed files
        // list fully.
        POSITION pos      = m_logList.GetFirstSelectedItemPosition();
        size_t   selIndex = m_logList.GetNextSelectedItem(pos);
        if (selIndex >= m_logEntries.GetVisibleCount())
        {
            return;
        }
        m_nSearchIndex          = static_cast<int>(selIndex);
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(selIndex);
        if (pLogEntry == nullptr)
            return;

        pLogEntry->SetUnread(false);

        pMsgView->SetRedraw(FALSE);
        OnOutOfScope(pMsgView->SetRedraw(TRUE); pMsgView->Invalidate());

        SMarkerInfo info;
        if (m_bSingleRevision)
        {
            info.sText = CUnicodeUtils::GetUnicode(pLogEntry->GetMessage().c_str());
        }
        else
        {
            m_currentChangedArray.RemoveAll();
            m_currentChangedPathList = GetChangedPathsAndMessageSketchFromSelectedRevisions(info.sText,
                                                                                            m_currentChangedArray);
        }

        // the rich edit control doesn't count the CR char!
        // to be exact: CRLF is treated as one char.
        info.sText.Remove(L'\r');

        info.text = info.sText;

        async::CFuture<BOOL> regexRunner(&info, &SMarkerInfo::RunRegex, &m_projectProperties);

        // set the log message text
        pMsgView->SetWindowText(info.sText);

        // mark filter matches
        if (m_selectedFilters & LOGFILTER_MESSAGES)
        {
            CLogDlgFilter filter(m_sFilterText, m_bFilterWithRegex, m_selectedFilters, m_bFilterCaseSensitively, m_tFrom, m_tTo, false, &m_mergedRevs, !!m_bHideNonMergeables, m_copyFromRev, 0);

            info.ranges = filter.GetMatchRanges(info.text);

            // combine ranges only separated by whitespace
            ReduceRanges(info.ranges, info.text);

            CAppUtils::SetCharFormat(pMsgView, CFM_COLOR, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::FilterMatch), true), info.ranges);
        }

        if (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\StyleCommitMessages", TRUE)) == TRUE)
            CAppUtils::FormatTextInRichEditControl(pMsgView);

        // fill in the changed files list control
        if (m_bSingleRevision)
        {
            m_currentChangedArray = pLogEntry->GetChangedPaths();
            if ((m_cShowPaths.GetState() & 0x0003) == BST_CHECKED)
                m_currentChangedArray.RemoveIrrelevantPaths();
        }

        regexRunner.GetResult();
        CAppUtils::SetCharFormat(pMsgView, CFM_LINK, CFE_LINK, info.idRanges);
        CAppUtils::SetCharFormat(pMsgView, CFM_LINK, CFE_LINK, info.revRanges);
        CAppUtils::SetCharFormat(pMsgView, CFM_LINK, CFE_LINK, info.urlRanges);
        CHARRANGE range;
        range.cpMin = 0;
        range.cpMax = 0;
        pMsgView->SendMessage(EM_EXSETSEL, NULL, reinterpret_cast<LPARAM>(&range));
        m_tooltips.DelTool(pMsgView, 1);
        m_lastTooltipRect = {0};
    }

    // sort according to the settings
    if (m_nSortColumnPathList > 0)
    {
        m_currentChangedArray.Sort(m_nSortColumnPathList, m_bAscendingPathList);
        SetSortArrow(&m_changedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
    }
    else
        SetSortArrow(&m_changedFileListCtrl, -1, false);

    // redraw the views
    if (m_bSingleRevision)
    {
        m_changedFileListCtrl.SetItemCountEx(static_cast<int>(m_currentChangedArray.GetCount()));
        m_changedFileListCtrl.RedrawItems(0, static_cast<int>(m_currentChangedArray.GetCount()));
    }
    else if (m_currentChangedPathList.GetCount())
    {
        m_changedFileListCtrl.SetItemCountEx(static_cast<int>(m_currentChangedPathList.GetCount()));
        m_changedFileListCtrl.RedrawItems(0, static_cast<int>(m_currentChangedPathList.GetCount()));
    }
    else
    {
        m_changedFileListCtrl.SetItemCountEx(0);
        m_changedFileListCtrl.Invalidate();
    }

    CAppUtils::ResizeAllListCtrlCols(&m_changedFileListCtrl);
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
        entry = static_cast<LONG>(reg);

    reg = static_cast<DWORD>(entry);

    switch (entry)
    {
        default:
        case ID_CMD_SHOWALL:
            m_endRev   = 0;
            m_startRev = m_logRevision;
            if (m_bStrict)
                m_bShowedAll = true;
            m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWALL)));
            break;
        case ID_CMD_SHOWRANGE:
        {
            // ask for a revision range
            CRevisionRangeDlg dlg;
            dlg.SetStartRevision(m_startRev);
            dlg.SetEndRevision((m_endRev >= 0) ? m_endRev : SVNRev(0));
            if (dlg.DoModal() != IDOK)
                return;
            m_endRev   = dlg.GetEndRevision();
            m_startRev = dlg.GetStartRevision();
            if ((m_endRev.IsNumber() || m_endRev.IsHead()) && (m_startRev.IsNumber() || m_startRev.IsHead()))
            {
                if ((static_cast<svn_revnum_t>(m_startRev) < static_cast<svn_revnum_t>(m_endRev)) ||
                    (m_endRev.IsHead()))
                {
                    auto temp  = m_startRev;
                    m_startRev = m_endRev;
                    m_endRev   = temp;
                }
            }
            m_bShowedAll = false;
            m_btnShow.SetWindowText(CString(MAKEINTRESOURCE(IDS_LOG_SHOWRANGE)));
        }
        break;
    }
    m_changedFileListCtrl.SetItemCountEx(0);
    m_changedFileListCtrl.Invalidate();
    // We need to create CStoreSelection on the heap or else
    // the variable will run out of the scope before the
    // thread ends. Therefore we let the thread delete
    // the instance.
    AutoStoreSelection();

    m_logList.SetItemCountEx(0);
    m_logList.Invalidate();
    CWnd* pMsgView = GetDlgItem(IDC_MSGVIEW);
    if ((m_startRev > m_head) && (m_head > 0))
    {
        CString sTemp;
        sTemp.FormatMessage(IDS_ERR_NOSUCHREVISION, static_cast<LPCWSTR>(m_startRev.ToString()));
        m_logList.ShowText(sTemp, true);
        return;
    }
    pMsgView->SetWindowText(L"");

    SetSortArrow(&m_logList, -1, true);

    m_logEntries.ClearAll();

    m_bCancelled = FALSE;
    m_limit      = 0;

    InterlockedExchange(&m_bLogThreadRunning, TRUE);
    new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::OnBnClickedRefresh()
{
    m_limit = 0;
    Refresh(true);
}

void CLogDlg::Refresh(bool autoGoOnline)
{
    //does the user force the cache to refresh (shift or control key down)?
    m_nRefresh = ((GetKeyState(VK_CONTROL) < 0) || (GetKeyState(VK_SHIFT) < 0)) ? Cache : Simple;

    // refreshing means re-downloading the already shown log messages
    UpdateData();
    if (m_logEntries.size())
        m_startRev = m_logEntries.GetMaxRevision();
    if ((m_startRev < m_head) && (m_nRefresh == Cache))
    {
        m_startRev = -1;
        m_nRefresh = Simple;
    }
    if (m_startRev >= m_head)
    {
        m_startRev = -1;
    }
    if ((m_limit == 0) || (m_bStrict) || (static_cast<int>(m_logEntries.size() - 1) > m_limit))
    {
        if (m_logEntries.size() != 0)
        {
            m_endRev = m_logEntries.GetMinRevision();
        }
    }
    m_bCancelled = FALSE;
    m_wcRev      = SVNRev();

    // We need to create CStoreSelection on the heap or else
    // the variable will run out of the scope before the
    // thread ends. Therefore we let the thread delete
    // the instance.
    AutoStoreSelection();

    m_changedFileListCtrl.SetItemCountEx(0);
    m_changedFileListCtrl.Invalidate();
    m_logList.SetItemCountEx(0);
    m_logList.Invalidate();
    CWnd* pMsgView = GetDlgItem(IDC_MSGVIEW);
    pMsgView->SetWindowText(L"");

    SetSortArrow(&m_logList, -1, true);
    m_logEntries.ClearAll();

    // reset the cached HEAD property & go on-line

    if (autoGoOnline)
    {
        SetDlgTitle(false);
        GetLogCachePool()->GetRepositoryInfo().ResetHeadRevision(m_sUuid, m_sRepositoryRoot);
    }

    InterlockedExchange(&m_bLogThreadRunning, TRUE);
    new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);
    GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

void CLogDlg::SetTheme(bool bDark) const
{
    if (m_hwndToolbar)
    {
        DarkModeHelper::Instance().AllowDarkModeForWindow(m_hwndToolbar, bDark);
        SetWindowTheme(m_hwndToolbar, L"Explorer", nullptr);
        auto hTT = reinterpret_cast<HWND>(::SendMessage(m_hwndToolbar, TB_GETTOOLTIPS, 0, 0));
        if (hTT)
        {
            DarkModeHelper::Instance().AllowDarkModeForWindow(hTT, bDark);
            SetWindowTheme(hTT, L"Explorer", nullptr);
        }
    }
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
    svn_revnum_t rev = m_logEntries[m_logEntries.size() - 1]->GetRevision();

    if (rev < 1)
        return; // do nothing! No more revisions to get
    m_startRev   = rev;
    m_endRev     = 0;
    m_bCancelled = FALSE;
    m_nRefresh   = None;

    // rev is is revision we already have and we will receive it again
    // -> fetch one extra revision to get NumberOfLogs *new* revisions

    m_limit = static_cast<int>(static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs", 100))) + 1;
    InterlockedExchange(&m_bLogThreadRunning, TRUE);
    SetSortArrow(&m_logList, -1, true);

    // clear the list controls: after the thread is finished
    // the controls will be populated again.
    m_changedFileListCtrl.SetItemCountEx(0);
    m_changedFileListCtrl.Invalidate();
    // We need to create CStoreSelection on the heap or else
    // the variable will run out of the scope before the
    // thread ends. Therefore we let the thread delete
    // the instance.
    AutoStoreSelection();

    m_logList.SetItemCountEx(0);
    m_logList.Invalidate();

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

void CLogDlg::SaveSplitterPos() const
{
    if (!IsIconic())
    {
        CRegDWORD regPos1(m_bMonitoringMode ? L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1M" : L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer1");
        CRegDWORD regPos2(m_bMonitoringMode ? L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2M" : L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer2");
        CRegDWORD regPos3(m_bMonitoringMode ? L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer3M" : L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\LogDlgSizer3");
        RECT      rectSplitter;
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
    if ((temp.Compare(temp2) == 0) && !GetDlgItem(IDOK)->IsWindowVisible())
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
        MonitorHideDlg();
        return;
    }
    bool bWasCancelled = m_bCancelled;
    // canceling means stopping the working thread if it's still running.
    // we do this by using the Subversion cancel callback.

    m_bCancelled = true;

    // We want to close the dialog -> give the background threads some time
    // to actually finish. Otherwise, we might not save the latest data.

    bool threadsStillRunning = !netScheduler.WaitForEmptyQueueOrTimeout(5000) || !diskScheduler.WaitForEmptyQueueOrTimeout(5000);

    // can we close the app cleanly?

    if (threadsStillRunning)
    {
        if (bWasCancelled)
        {
            // end the process the hard way
            if (m_bMonitoringMode)
                Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
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
        if ((temp.Compare(temp2) != 0))
        {
            // store settings

            UpdateData();
            if (m_bSaveStrict)
                m_regLastStrict = m_bStrict;
            CRegDWORD reg(L"Software\\TortoiseSVN\\ShowAllEntry");
            reg = static_cast<DWORD>(m_btnShow.m_nMenuResult);

            CRegDWORD reg2(L"Software\\TortoiseSVN\\LogShowPaths");
            reg2 = static_cast<DWORD>(m_cShowPaths.GetCheck());
            SaveSplitterPos();
        }

        // close app

        __super::OnCancel();
    }
}

void CLogDlg::MonitorHideDlg()
{
    // remove selection, show empty log list
    m_projTree.SelectItem(nullptr);
    MonitorShowProject(nullptr, nullptr);

    ShowWindow(SW_HIDE);
    SaveMonitorProjects(true);
    SaveSplitterPos();
    SaveWindowRect(L"MonitorLogDlg", false);
}

void CLogDlg::OnClose()
{
    if (m_bLogThreadRunning)
    {
        m_bCancelled = true;

        bool threadsStillRunning = !netScheduler.WaitForEmptyQueueOrTimeout(5000) || !diskScheduler.WaitForEmptyQueueOrTimeout(5000);

        if (threadsStillRunning)
            return;
    }
    if (m_bMonitoringMode && !m_bSystemShutDown)
    {
        MonitorHideDlg();
    }
    else
        __super::OnClose();
}

void CLogDlg::OnDestroy()
{
    CTheme::Instance().RemoveRegisteredCallback(m_themeCallbackId);
    if (m_pLogListAccServer)
    {
        ListViewAccServer::ClearProvider(m_logList.GetSafeHwnd());
        m_pLogListAccServer->Release();
        delete m_pLogListAccServer;
        m_pLogListAccServer = nullptr;
    }
    if (m_pChangedListAccServer)
    {
        ListViewAccServer::ClearProvider(m_changedFileListCtrl.GetSafeHwnd());
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

    __time64_t tTime = time / 1000000L;
    m_logEntries.Add(rev, tTime, author, message, &m_projectProperties, mergeInfo);

    // end of child list

    if (rev == SVN_INVALID_REVNUM)
        return TRUE;

    // update progress

    if (m_startRev == -1)
        m_startRev = rev;

    PLOGENTRYDATA logItem = m_logEntries[m_logEntries.size() - 1];
    if (logItem->GetDepth() > 0)
        m_cMergedRevisionsReceived++;

    if (m_limit != 0)
    {
        m_logProgress.SetPos(static_cast<int>(m_logEntries.size() - m_prevLogEntriesSize - m_cMergedRevisionsReceived));
        if (!m_bMonitoringMode && m_pTaskbarList)
        {
            int l, u;
            m_logProgress.GetRange(l, u);
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
            m_pTaskbarList->SetProgressValue(m_hWnd, m_logEntries.size() - m_prevLogEntriesSize - l - m_cMergedRevisionsReceived, u - l);
        }
    }
    else if (m_startRev.IsNumber() && m_endRev.IsNumber())
    {
        if (!logItem->HasParent() && rev < m_tempRev)
        {
            m_tempRev = rev;
            m_logProgress.SetPos(static_cast<svn_revnum_t>(m_startRev) - rev + static_cast<svn_revnum_t>(m_endRev));
            if (!m_bMonitoringMode && m_pTaskbarList)
            {
                int l, u;
                m_logProgress.GetRange(l, u);
                m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
                m_pTaskbarList->SetProgressValue(m_hWnd, static_cast<svn_revnum_t>(m_startRev) - rev + static_cast<svn_revnum_t>(m_endRev) - l, u - l);
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
    OnOutOfScope(InterlockedExchange(&m_bLogThreadRunning, FALSE));

    {
        CAutoReadLock pathLock(m_monitorPathGuard);
        if (m_path.IsEmpty() && m_path.IsEquivalentToWithoutCase(CTSVNPath(m_pathCurrentlyChecked)))
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
    if (m_bMonitoringMode)
        DialogEnableWindow(IDC_PROJTREE, FALSE);

    CString temp;
    temp.LoadString(IDS_PROGRESSWAIT);
    m_logList.ShowText(temp, true);
    // change the text of the close button to "Cancel" since now the thread
    // is running, and simply closing the dialog doesn't work.
    if (!GetDlgItem(IDOK)->IsWindowVisible())
    {
        temp.LoadString(IDS_MSGBOX_CANCEL);
        SetDlgItemText(IDC_LOGCANCEL, temp);
    }
    // We use a progress bar while getting the logs
    m_logProgress.SetRange32(0, 100);
    m_logProgress.SetPos(0);
    GetDlgItem(IDC_HIDENONMERGEABLE)->ShowWindow(FALSE);
    m_logProgress.ShowWindow(TRUE);

    // we need the UUID to unambiguously identify the log cache
    BOOL succeeded = true;
    if (LogCache::CSettings::GetEnabled())
    {
        if (!m_bMonitoringMode || m_sUuid.IsEmpty())
            m_sUuid = GetLogCachePool()->GetRepositoryInfo().GetRepositoryUUID(m_path);
        if (m_sUuid.IsEmpty())
            succeeded = false;
    }

    // get the repository root url, because the changed-files-list has the
    // paths shown there relative to the repository root.
    CTSVNPath rootPath;
    if (succeeded && (!m_bMonitoringMode || m_sRepositoryRoot.IsEmpty()))
    {
        // e.g. when we show the "next 100", we already have proper
        // start and end revs.
        // -> we don't need to look for the head revision in these cases

        if (m_bStartRevIsHead || (m_startRev == SVNRev::REV_HEAD) || (m_endRev == SVNRev::REV_HEAD) || (m_head < 0))
        {
            // expensive repository lookup
            int maxHeadAge = LogCache::CSettings::GetMaxHeadAge();
            LogCache::CSettings::SetMaxHeadAge(0);
            svn_revnum_t head = -1;
            succeeded         = GetRootAndHead(m_path, rootPath, head);
            m_head            = head;
            if ((m_startRev == SVNRev::REV_HEAD) ||
                (m_bStartRevIsHead && ((m_nRefresh == Simple) || (m_nRefresh == Cache))))
            {
                m_startRev = head;
            }
            if (m_endRev == SVNRev::REV_HEAD)
                m_endRev = head;
            LogCache::CSettings::SetMaxHeadAge(maxHeadAge);
        }
        else
        {
            // far less expensive root lookup

            rootPath.SetFromSVN(GetRepositoryRoot(m_path));
            succeeded = !rootPath.IsEmpty();
        }
    }

    if (!m_bMonitoringMode || m_sRepositoryRoot.IsEmpty())
        m_sRepositoryRoot = rootPath.GetSVNPathString();
    m_sURL = m_path.GetSVNPathString();

    // if the log dialog is started from a working copy, we need to turn that
    // local path into an url here
    if (succeeded)
    {
        if (!m_path.IsUrl())
        {
            m_sURL = GetURLFromPath(m_path);
        }
        auto sURLEscaped = CPathUtils::PathUnescape(m_sURL);
        m_sRelativeRoot  = sURLEscaped.Mid(CPathUtils::PathUnescape(m_sRepositoryRoot).GetLength());
        if (m_path.IsUrl() && (sURLEscaped == m_sURL) && (m_path.IsEquivalentToWithoutCase(CTSVNPath(sURLEscaped))))
        {
            // sometimes we get unescaped urls passed to the log command (in m_path),
            // in these cases we need to escape them here
            m_path.SetFromSVN(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(m_sURL)));
        }
        m_sURL = sURLEscaped;
    }

    if (succeeded && !m_mergePath.IsEmpty())
    {
        // in case we got a merge path set, retrieve the merge info
        // of that path and check whether one of the merge URLs
        // match the URL we show the log for.
        SVNPool localPool(m_pool);
        svn_error_clear(m_err);
        apr_hash_t* mergeInfo = nullptr;
        const char* svnPath   = m_mergePath.GetSVNApiPath(localPool);

        SVNTRACE(
            m_err = svn_client_mergeinfo_get_merged(&mergeInfo, svnPath, SVNRev(SVNRev::REV_WC),
                                                    m_pCtx, localPool),
            svnPath

        )
        if (m_err == nullptr)
        {
            // now check the relative paths
            apr_hash_index_t* hi  = nullptr;
            const void*       key = nullptr;
            void*             val = nullptr;

            if (mergeInfo)
            {
                // unfortunately it's not defined whether the urls have a trailing
                // slash or not, so we have to trim such a slash before comparing
                // the sUrl with the mergeinfo url
                CStringA sUrl = CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(m_sURL));
                sUrl.TrimRight('/');
                CTSVNPath pUrl;
                pUrl.SetFromSVN(sUrl);

                for (hi = apr_hash_first(localPool, mergeInfo); hi; hi = apr_hash_next(hi))
                {
                    apr_hash_this(hi, &key, nullptr, &val);
                    CStringA sKey = static_cast<char*>(const_cast<void*>(key));
                    sKey.TrimRight('/');
                    CTSVNPath pKey;
                    pKey.SetFromSVN(sKey);
                    if ((sUrl.Compare(sKey) == 0) || pUrl.IsAncestorOf(pKey))
                    {
                        apr_array_header_t* arr = static_cast<apr_array_header_t*>(val);
                        if (val)
                        {
                            for (long i = 0; i < arr->nelts; ++i)
                            {
                                svn_merge_range_t* pRange = APR_ARRAY_IDX(arr, i, svn_merge_range_t*);
                                if (pRange)
                                {
                                    for (svn_revnum_t re = pRange->start + 1; re <= pRange->end; ++re)
                                    {
                                        m_mergedRevs.insert(re);
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }

        bool bFindCopyFrom = !!static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\LogFindCopyFrom", TRUE));
        if (bFindCopyFrom && m_bStrict)
        {
            SVNLogHelper helper;
            CString      sCopyFrom;
            CTSVNPath    mergeUrl = CTSVNPath(GetURLFromPath(m_mergePath) + L"/");
            SVNRev       rev      = helper.GetCopyFromRev(mergeUrl, SVNRev::REV_HEAD, sCopyFrom);
            if (sCopyFrom.Compare(m_sURL) == 0)
            {
                m_copyFromRev = rev;
                // add the copyfrom revision to the already merged revs so it's shown in gray
                m_mergedRevs.insert(rev);
                if (static_cast<svn_revnum_t>(m_startRev) > static_cast<svn_revnum_t>(m_endRev))
                    m_endRev = m_copyFromRev;
            }
        }
    }

    m_logProgress.SetPos(1);
    m_prevLogEntriesSize = m_logEntries.size();
    if (!m_bMonitoringMode && m_pTaskbarList)
    {
        m_pTaskbarList->SetProgressState(m_hWnd, TBPF_INDETERMINATE);
    }
    if (m_limit != 0)
    {
        m_cMergedRevisionsReceived = 0;
        m_logProgress.SetRange32(0, m_limit);
    }
    else
    {
        m_logProgress.SetRange32(m_endRev, m_startRev);
        m_tempRev = m_startRev;
    }

    if (!m_pegRev.IsValid())
        m_pegRev = m_startRev;
    size_t startcount = m_logEntries.size();
    m_bStrictStopped  = false;

    std::unique_ptr<const CCacheLogQuery> cachedData;
    if (succeeded)
    {
        if (m_bEnsureSelection)
        {
            // ensure that the end revision is fetched, so adjust the limit
            if (m_limit && m_startRev.IsNumber() && (static_cast<svn_revnum_t>(m_startRev) > 0))
            {
                m_limit  = max(m_limit, static_cast<int>(static_cast<svn_revnum_t>(m_startRev) - static_cast<svn_revnum_t>(m_endRev)));
                m_endRev = 0;
            }
            m_bEnsureSelection = false;
        }
        cachedData = ReceiveLog(CTSVNPathList(m_path), m_pegRev, m_startRev, m_endRev, m_limit,
                                !!m_bStrict, !!m_bIncludeMerges, m_nRefresh == Cache);
        if ((cachedData.get() == nullptr) && (!m_path.IsUrl()))
        {
            // try again with REV_WC as the start revision, just in case the path doesn't
            // exist anymore in HEAD.
            // Also, make sure we use these parameters for further requests (like "next 100").

            m_pegRev   = SVNRev::REV_WC;
            m_startRev = SVNRev::REV_WC;

            cachedData = ReceiveLog(CTSVNPathList(m_path), m_pegRev, m_startRev, m_endRev, m_limit,
                                    !!m_bStrict, !!m_bIncludeMerges, m_nRefresh == Cache);
        }

        // Err will also be set if the user cancelled.

        if (m_err && (m_err->apr_err == SVN_ERR_CANCELLED))
        {
            svn_error_clear(m_err);
            m_err = nullptr;
        }

        succeeded = m_err == nullptr;

        // make sure the m_logEntries is consistent

        if (cachedData.get() != nullptr)
            m_logEntries.Finalize(std::move(cachedData), m_sRelativeRoot,
                                  !LogCache::CSettings::GetEnabled());
        else
            m_logEntries.ClearAll();

        if (m_bMonitoringMode && (m_revUnread > 0))
        {
            // mark all entries that are new as unread, so they're shown
            // in bold in the list control
            std::wregex rx;
            try
            {
                if (!m_sMonitorMsgRegex.IsEmpty())
                    rx = std::wregex(m_sMonitorMsgRegex, std::regex_constants::ECMAScript | std::regex_constants::icase);
            }
            catch (std::exception&)
            {
                m_sMonitorMsgRegex.Empty();
            }
            for (size_t i = 0; i < m_logEntries.GetVisibleCount(); ++i)
            {
                PLOGENTRYDATA pEntry = m_logEntries.GetVisible(i);
                if (pEntry && (pEntry->GetRevision() > m_revUnread))
                {
                    bool bIgnore = false;
                    for (const auto& authorToIgnore : m_monitorAuthorsToIgnore)
                    {
                        if (_stricmp(pEntry->GetAuthor().c_str(), authorToIgnore.c_str()) == 0)
                        {
                            bIgnore = true;
                            break;
                        }
                    }
                    if (!bIgnore && !m_sMonitorMsgRegex.IsEmpty())
                    {
                        try
                        {
                            if (std::regex_match(pEntry->GetMessageW().cbegin(),
                                                 pEntry->GetMessageW().cend(),
                                                 rx))
                            {
                                bIgnore = true;
                            }
                        }
                        catch (std::exception&)
                        {
                        }
                    }
                    if (bIgnore)
                        continue;
                    pEntry->SetUnread(true);
                }
            }
        }
    }
    m_logList.ClearText();
    if (!succeeded)
    {
        temp.LoadString(IDS_LOG_CLEARERROR);
        m_logList.ShowText(GetLastErrorMessage() + L"\n\n" + temp, true);
        FillLogMessageCtrl(false);
        if (!m_bMonitoringMode && m_pTaskbarList)
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
        }
    }

    if (m_bStrict && (m_logEntries.GetMinRevision() > 1) && (m_limit > 0 ? (startcount + m_limit > m_logEntries.size()) : (m_endRev < m_logEntries.GetMinRevision())))
    {
        m_bStrictStopped = true;
    }
    m_logList.SetItemCountEx(ShownCountWithStopped());

    m_tFrom   = m_logEntries.GetMinDate();
    m_tTo     = m_logEntries.GetMaxDate();
    m_timFrom = m_tFrom;
    m_timTo   = m_tTo;
    m_dateFrom.SetRange(&m_timFrom, &m_timTo);
    m_dateTo.SetRange(&m_timFrom, &m_timTo);
    m_dateFrom.SetTime(&m_timFrom);
    m_dateTo.SetTime(&m_timTo);
    if (m_timeFromSetFromCmdLine)
    {
        m_timFrom = m_timeFromSetFromCmdLine;
        m_dateFrom.SetTime(&m_timFrom);
        m_tFrom = m_timeFromSetFromCmdLine;
    }
    if (m_timeToSetFromCmdLine)
    {
        m_timTo = m_timeToSetFromCmdLine;
        m_dateTo.SetTime(&m_timTo);
        m_tTo = m_timeToSetFromCmdLine;
    }

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
    bool                       doRetry          = false;
    SetDlgTitle(cachedProperties.IsOffline(m_sUuid, m_sRepositoryRoot, false, L"", doRetry));

    m_logProgress.ShowWindow(FALSE);
    if (!m_bMonitoringMode)
        GetDlgItem(IDC_HIDENONMERGEABLE)->ShowWindow(!m_mergedRevs.empty() || (static_cast<svn_revnum_t>(m_copyFromRev) > 0));
    else
        DialogEnableWindow(IDC_PROJTREE, TRUE);

    if (!m_bMonitoringMode && m_pTaskbarList)
    {
        m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
    }
    m_bCancelled = false;
    InterlockedExchange(&m_bLogThreadRunning, FALSE);
    if (m_pStoreSelection == nullptr)
    {
        // If no selection has been set then this must be the first time
        // the revisions are shown. Let's preselect the topmost revision.
        if (m_logList.GetItemCount() > 0)
        {
            int selIndex = 0;
            if (m_bMonitoringMode && m_revUnread)
            {
                // in monitoring mode, select the first _unread_ revision
                for (; selIndex < static_cast<int>(m_logEntries.size()); ++selIndex)
                {
                    if (m_logEntries.GetVisible(selIndex)->GetRevision() < m_revUnread)
                        break;
                }
            }
            m_logList.SetSelectionMark(selIndex);
            m_logList.SetItemState(selIndex, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
    m_revUnread = 0;
    if (!GetDlgItem(IDOK)->IsWindowVisible())
    {
        temp.LoadString(IDS_MSGBOX_OK);
        SetDlgItemText(IDC_LOGCANCEL, temp);
    }
    RefreshCursor();

    if (m_bMonitoringMode)
        SVNReInit();
    // make sure the filter is applied (if any) now, after we refreshed/fetched
    // the log messages
    PostMessage(WM_TIMER, LOGFILTER_TIMER);
}

//this is the thread function which calls the subversion function
void CLogDlg::StatusThread()
{
    bool bAllowStatusCheck = m_bMonitoringMode || !!static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\LogStatusCheck", TRUE));
    if ((bAllowStatusCheck) && (!m_wcRev.IsValid()))
    {
        // fetch the revision the wc path is on so we can mark it
        CTSVNPath revWCPath = m_projectProperties.GetPropsPath();
        if (!m_path.IsUrl())
            revWCPath = m_path;

        if (revWCPath.IsUrl() || revWCPath.IsEmpty())
            return;

        svn_revnum_t minRev = 0, maxRev = 0;
        SVN          svn;
        svn.SetCancelBool(&m_bCancelled);
        if (svn.GetWCMinMaxRevs(revWCPath, true, minRev, maxRev) && (maxRev))
        {
            m_wcRev = maxRev;
            // force a redraw of the log list control to make sure the wc rev is
            // redrawn in bold
            m_logList.Invalidate(FALSE);
        }
    }
}

void CLogDlg::CopySelectionToClipBoard() const
{
    if ((GetKeyState(VK_CONTROL) & 0x8000) && ((GetKeyState(L'C') & 0x8000) == 0) &&
        ((GetKeyState(VK_INSERT) & 0x8000) == 0))
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
void CLogDlg::CopyCommaSeparatedRevisionsToClipboard() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();

    std::set<svn_revnum_t> setSelectedRevisions;

    if (pos != nullptr)
    {
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry)
            {
                setSelectedRevisions.insert(pLogEntry->GetRevision());
            }
        }
    }
    if (setSelectedRevisions.size() > 0)
    {
        CString sRevisions;
        for (auto it = setSelectedRevisions.begin(); it != setSelectedRevisions.end(); ++it)
        {
            if (!sRevisions.IsEmpty())
                sRevisions += ", ";
            sRevisions += SVNRev(*it).ToString();
        }
        CStringUtils::WriteAsciiStringToClipboard(sRevisions, GetSafeHwnd());
    }
}

void CLogDlg::CopyCommaSeparatedAuthorsToClipboard() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    CString  sAuthors;
    CString  sAuthor;

    if (pos != nullptr)
    {
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
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

void CLogDlg::CopyMessagesToClipboard() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    CString  sMessages;
    CString  sMessage;

    if (pos != nullptr)
    {
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry)
            {
                sMessage.Format(L"%s\r\n----\r\n", CUnicodeUtils::StdGetUnicode(pLogEntry->GetMessageW()).c_str());
                sMessages += sMessage;
            }
        }

        CStringUtils::WriteAsciiStringToClipboard(sMessages, GetSafeHwnd());
    }
}

void CLogDlg::CopySelectionToClipBoardRev() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    if (pos != nullptr)
    {
        CString sClipdata;
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry == nullptr)
                continue;
            CString sLogCopyText;
            sLogCopyText.Format(L"%s/?r=%ld\r\n", static_cast<LPCWSTR>(m_sURL), pLogEntry->GetRevision());
            sClipdata += sLogCopyText;
        }
        CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
    }
}

void CLogDlg::CopySelectionToClipBoardViewerRev() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    if (pos != nullptr)
    {
        CString sClipdata;
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry == nullptr)
                continue;

            CString sUrl = m_projectProperties.sWebViewerRev;
            CString rev;
            rev.Format(L"%ld", pLogEntry->GetRevision());
            CLogChangedPathArray array = pLogEntry->GetChangedPaths();
            sUrl.Replace(L"%REVISION%", rev);
            sClipdata += sUrl;
            sClipdata += L"\r\n";
        }
        CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
    }
}

void CLogDlg::CopySelectionToClipBoardViewerPathRev() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    if (pos != nullptr)
    {
        CString sClipdata;
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry == nullptr)
                continue;

            CString sUrl = m_projectProperties.sWebViewerPathRev;
            CString rev;
            rev.Format(L"%ld", pLogEntry->GetRevision());
            sUrl.Replace(L"%REVISION%", rev);
            sUrl.Replace(L"%PATH%", static_cast<LPCWSTR>(m_sRelativeRoot));
            sClipdata += sUrl;
            sClipdata += L"\r\n";
        }
        CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
    }
}

void CLogDlg::CopySelectionToClipBoardTsvnShowCompare() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    if (pos != nullptr)
    {
        CString sClipdata;
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry == nullptr)
                continue;

            CString sUrl = L"tsvncmd:command:showcompare?url1:";
            sUrl += m_sRepositoryRoot;
            sUrl += m_sRelativeRoot;
            sUrl += "?revision1:";
            sUrl += SVNRev(pLogEntry->GetRevision() - 1).ToString();
            sUrl += "?url2:";
            sUrl += m_sRepositoryRoot;
            sUrl += m_sRelativeRoot;
            sUrl += "?revision2:";
            sUrl += SVNRev(pLogEntry->GetRevision()).ToString();
            sClipdata += sUrl;
            sClipdata += L"\r\n";
        }
        CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
    }
}

std::set<CString> CLogDlg::GetSelectedBugIds()
{
    std::set<CString> setAllBugIDs;
    POSITION          pos = m_logList.GetFirstSelectedItemPosition();
    if (pos != nullptr)
    {
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry == nullptr)
                continue;

            CString logMessage = CUnicodeUtils::StdGetUnicode(pLogEntry->GetMessageW()).c_str();

            std::set<CString> setBugIDs = m_projectProperties.FindBugIDs(logMessage);
            setAllBugIDs.merge(setBugIDs);
        }
    }
    return setAllBugIDs;
}

void CLogDlg::CopySelectionToClipBoardBugId()
{
    std::set<CString>           selectedBugIDs = GetSelectedBugIds();
    CString                     sClipdata;
    std::set<CString>::iterator it;
    for (it = selectedBugIDs.begin(); it != selectedBugIDs.end(); ++it)
    {
        if (!sClipdata.IsEmpty())
        {
            sClipdata += ", ";
        }
        sClipdata += *it;
    }
    CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
}

void CLogDlg::CopySelectionToClipBoardBugUrl()
{
    std::set<CString>           selectedBugIDs = GetSelectedBugIds();
    CString                     sClipdata;
    std::set<CString>::iterator it;
    for (it = selectedBugIDs.begin(); it != selectedBugIDs.end(); ++it)
    {
        if (!sClipdata.IsEmpty())
        {
            sClipdata += "\r\n";
        }
        sClipdata += m_projectProperties.GetBugIDUrl(*it);
    }
    CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
}

void CLogDlg::CopySelectionToClipBoard(bool bIncludeChangedList) const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    if (pos != nullptr)
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
            int     index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry == nullptr)
                continue;
            if (bIncludeChangedList)
            {
                CString                     sPaths;
                const CLogChangedPathArray& cPathArray = pLogEntry->GetChangedPaths();
                for (size_t cpPathIndex = 0; cpPathIndex < cPathArray.GetCount(); ++cpPathIndex)
                {
                    const CLogChangedPath& cPath = cPathArray[cpPathIndex];
                    sPaths += CUnicodeUtils::GetUnicode(cPath.GetActionString().c_str()) + L" : " + cPath.GetPath();
                    if (cPath.GetCopyFromPath().IsEmpty())
                        sPaths += L"\r\n";
                    else
                    {
                        CString sCopyFrom;
                        sCopyFrom.Format(L" (%s: %s, %s, %ld)\r\n",
                                         static_cast<LPCWSTR>(CString(MAKEINTRESOURCE(IDS_LOG_COPYFROM))),
                                         static_cast<LPCWSTR>(cPath.GetCopyFromPath()),
                                         static_cast<LPCWSTR>(CString(MAKEINTRESOURCE(IDS_LOG_REVISION))),
                                         cPath.GetCopyFromRev());
                        sPaths += sCopyFrom;
                    }
                }
                sPaths.Trim();
                CString nlMessage = CUnicodeUtils::GetUnicode(pLogEntry->GetMessage().c_str());
                nlMessage.Remove(L'\r');
                nlMessage.Replace(L"\n", L"\r\n");
                sLogCopyText.Format(L"%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n",
                                    static_cast<LPCWSTR>(sRev), pLogEntry->GetRevision(),
                                    static_cast<LPCWSTR>(sAuthor), static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(pLogEntry->GetAuthor().c_str())),
                                    static_cast<LPCWSTR>(sDate),
                                    static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(pLogEntry->GetDateString().c_str())),
                                    static_cast<LPCWSTR>(sMessage), static_cast<LPCWSTR>(nlMessage),
                                    static_cast<LPCWSTR>(sPaths));
            }
            else
            {
                CString nlMessage = CUnicodeUtils::GetUnicode(pLogEntry->GetMessage().c_str());
                nlMessage.Remove(L'\r');
                nlMessage.Replace(L"\n", L"\r\n");
                sLogCopyText.Format(L"%s: %d\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n",
                                    static_cast<LPCWSTR>(sRev), pLogEntry->GetRevision(),
                                    static_cast<LPCWSTR>(sAuthor), static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(pLogEntry->GetAuthor().c_str())),
                                    static_cast<LPCWSTR>(sDate),
                                    static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(pLogEntry->GetDateString().c_str())),
                                    static_cast<LPCWSTR>(sMessage), static_cast<LPCWSTR>(nlMessage));
            }
            sClipdata += sLogCopyText;
        }
        CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
    }
}

void CLogDlg::CopyChangedSelectionToClipBoard() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    if (pos == nullptr)
        return; // nothing is selected, get out of here

    CString sPaths;

    POSITION pos2 = m_changedFileListCtrl.GetFirstSelectedItemPosition();
    while (pos2)
    {
        int nItem = m_changedFileListCtrl.GetNextSelectedItem(pos2);
        sPaths += m_currentChangedArray[nItem].GetPath();
        sPaths += L"\r\n";
    }
    sPaths.Trim();
    CStringUtils::WriteAsciiStringToClipboard(sPaths, GetSafeHwnd());
}

BOOL CLogDlg::IsDiffPossible(const CLogChangedPath& changedPath, svn_revnum_t rev)
{
    if ((rev > 1) && (changedPath.GetAction() != LOGACTIONS_DELETED))
    {
        if (changedPath.GetAction() == LOGACTIONS_ADDED) // file is added
        {
            if (changedPath.GetCopyFromRev() == 0)
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
    int selCount = m_logList.GetSelectedCount();
    if (pWnd == &m_logList)
    {
        ShowContextMenuForRevisions(pWnd, point);
    }
    else if (pWnd == &m_changedFileListCtrl)
    {
        ShowContextMenuForChangedPaths(pWnd, point);
    }
    else if (pWnd == &m_projTree)
    {
        ShowContextMenuForMonitorTree(pWnd, point);
    }
    else if ((selCount > 0) && (pWnd == GetDlgItem(IDC_MSGVIEW)))
    {
        POSITION pos      = m_logList.GetFirstSelectedItemPosition();
        int      selIndex = -1;
        if (pos)
            selIndex = m_logList.GetNextSelectedItem(pos);
        if ((point.x == -1) && (point.y == -1))
        {
            CRect rect;
            GetDlgItem(IDC_MSGVIEW)->GetClientRect(&rect);
            ClientToScreen(&rect);
            point = rect.CenterPoint();
        }
        CString sMenuItemText;
        CMenu   popup;
        if (popup.CreatePopupMenu())
        {
            // add the 'default' entries
            sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
            popup.AppendMenu(MF_STRING | MF_ENABLED, WM_COPY, sMenuItemText);
            sMenuItemText.LoadString(IDS_SCIEDIT_SELECTALL);
            popup.AppendMenu(MF_STRING | MF_ENABLED, EM_SETSEL, sMenuItemText);

            if ((selIndex >= 0) && (selCount == 1))
            {
                popup.AppendMenu(MF_SEPARATOR);
                sMenuItemText.LoadString(IDS_LOG_POPUP_EDITLOG);
                popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EDITLOG, sMenuItemText);
            }

            int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY |
                                               TPM_RIGHTBUTTON,
                                           point.x, point.y, this, nullptr);
            switch (cmd)
            {
                case 0:
                    break; // no command selected
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

bool CLogDlg::IsSelectionContinuous() const
{
    if (m_logList.GetSelectedCount() == 1)
    {
        // if only one revision is selected, the selection is of course
        // continuous
        return true;
    }

    POSITION pos         = m_logList.GetFirstSelectedItemPosition();
    bool     bContinuous = m_logEntries.GetVisibleCount() == m_logEntries.size();
    if (bContinuous)
    {
        auto itemIndex = m_logList.GetNextSelectedItem(pos);
        while (pos)
        {
            auto nextIndex = m_logList.GetNextSelectedItem(pos);
            if (nextIndex - itemIndex > 1)
            {
                bContinuous = false;
                break;
            }
            itemIndex = nextIndex;
        }
    }
    return bContinuous;
}

LRESULT CLogDlg::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_pFindDialog != nullptr);
    if (m_pFindDialog == nullptr)
        return 0;

    if (m_pFindDialog->IsTerminating())
    {
        // invalidate the handle identifying the dialog box.
        m_pFindDialog = nullptr;
        return 0;
    }

    if (m_pFindDialog->FindNext())
    {
        //read data from dialog
        CString     findText   = m_pFindDialog->GetFindString();
        bool        bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
        std::wregex pat;
        bool        bRegex = ValidateRegexp(findText, pat, bMatchCase);

        bool          scanRelevantPathsOnly = (m_cShowPaths.GetState() & 0x0003) == BST_CHECKED;
        CLogDlgFilter filter(findText, bRegex, LOGFILTER_ALL, bMatchCase, m_tFrom, m_tTo, scanRelevantPathsOnly, &m_mergedRevs, !!m_bHideNonMergeables, m_copyFromRev, -1);

        for (size_t i = m_nSearchIndex; i < m_logEntries.GetVisibleCount(); i++)
        {
            if (filter(*m_logEntries.GetVisible(i)))
            {
                m_logList.EnsureVisible(static_cast<int>(i), FALSE);
                m_logList.SetItemState(m_logList.GetSelectionMark(), 0, LVIS_SELECTED);
                m_logList.SetItemState(static_cast<int>(i), LVIS_SELECTED, LVIS_SELECTED);
                m_logList.SetSelectionMark(static_cast<int>(i));

                FillLogMessageCtrl();
                UpdateData(FALSE);

                m_nSearchIndex = static_cast<int>(i + 1);
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
    revisions.reserve(m_logEntries.GetVisibleCount());

    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    if (pos)
    {
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index < static_cast<int>(m_logEntries.GetVisibleCount()))
            {
                PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
                if (pLogEntry)
                    revisions.push_back(pLogEntry->GetRevision());
            }
        }

        m_selectedRevs.AddRevisions(revisions);
        svn_revnum_t lowerRev  = m_selectedRevs.GetLowestRevision();
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
        if (m_sFilterText.IsEmpty())
            MonitorHideDlg();
        return;
    }
    // since the log dialog is also used to select revisions for other
    // dialogs, we have to do some work before closing this dialog
    if ((GetKeyState(VK_MENU) & 0x8000) == 0)
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
    if (!netScheduler.WaitForEmptyQueueOrTimeout(0) || !diskScheduler.WaitForEmptyQueueOrTimeout(1000))
    {
        return;
    }

    if (m_pNotifyWindow || m_bSelect)
        UpdateSelectedRevs();

    NotifyTargetOnOk();

    UpdateData();
    if (m_bSaveStrict)
        m_regLastStrict = m_bStrict;
    CRegDWORD reg(L"Software\\TortoiseSVN\\ShowAllEntry");
    reg = static_cast<DWORD>(m_btnShow.m_nMenuResult);
    CRegDWORD reg2(L"Software\\TortoiseSVN\\LogHidePaths");
    reg2 = static_cast<DWORD>(m_cShowPaths.GetCheck());
    SaveSplitterPos();

    CString buttonText;
    GetDlgItemText(IDOK, buttonText);
    CString temp;
    temp.LoadString(IDS_MSGBOX_CANCEL);
    // only exit if the button text matches, and that will match only if the thread isn't running anymore
    if (temp.Compare(buttonText) != 0)
        __super::OnOK();
}

void CLogDlg::NotifyTargetOnOk()
{
    if (m_pNotifyWindow == nullptr)
        return;

    bool         bSentMessage = false;
    svn_revnum_t lowerRev     = m_selectedRevsOneRange.GetHighestRevision();
    svn_revnum_t higherRev    = m_selectedRevsOneRange.GetLowestRevision();
    if (m_selectedRevsOneRange.GetCount() == 0)
    {
        lowerRev  = m_selectedRevs.GetLowestRevision();
        higherRev = m_selectedRevs.GetHighestRevision();
    }
    if (m_logList.GetSelectedCount() == 1)
    {
        // if only one revision is selected, check if the path/url with which the dialog was started
        // was directly affected in that revision. If it was, then check if our path was copied
        // from somewhere. If it was copied, use the copy from revision as lowerRev
        POSITION      pos       = m_logList.GetFirstSelectedItemPosition();
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_logList.GetNextSelectedItem(pos));

        if ((pLogEntry) && (lowerRev == higherRev))
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
                                             reinterpret_cast<LPARAM>(&m_selectedRevs));
                bSentMessage = true;
                break;
            }
        }
    }
    if (!bSentMessage)
    {
        if (m_selectedRevs.GetCount() > 0)
        {
            m_pNotifyWindow->SendMessage(WM_REVSELECTED,
                                         m_wParam & (MERGE_REVSELECTSTART | MERGE_REVSELECTMINUSONE), lowerRev);
            m_pNotifyWindow->SendMessage(WM_REVSELECTED,
                                         m_wParam & (MERGE_REVSELECTEND | MERGE_REVSELECTMINUSONE), higherRev);
        }
        m_pNotifyWindow->SendMessage(WM_REVLIST,
                                     m_selectedRevs.GetCount(), reinterpret_cast<LPARAM>(&m_selectedRevs));
        if (m_selectedRevsOneRange.GetCount())
            m_pNotifyWindow->SendMessage(WM_REVLISTONERANGE, 0, reinterpret_cast<LPARAM>(&m_selectedRevsOneRange));
    }
}

void CLogDlg::CreateFindDialog()
{
    if (m_pFindDialog == nullptr)
    {
        m_pFindDialog = new CFindReplaceDialog();
        m_pFindDialog->Create(TRUE, nullptr, nullptr, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);
        CTheme::Instance().SetThemeForDialog(m_pFindDialog->GetSafeHwnd(), CTheme::Instance().IsDarkTheme());
    }
    else
    {
        m_pFindDialog->SetFocus();
        return;
    }
}

int CLogDlg::OpenWorkingCopyFileWithRegisteredProgram(CString& fullPath) const
{
    if (!PathFileExists(static_cast<LPCWSTR>(fullPath)))
        return -1;

    return static_cast<int>(reinterpret_cast<INT_PTR>(ShellExecute(this->m_hWnd, nullptr, fullPath, nullptr, nullptr, SW_SHOWNORMAL)));
}

void CLogDlg::DoOpenFileWith(bool bReadOnly, bool bOpenWith, const CTSVNPath& tempfile) const
{
    if (bReadOnly)
        SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    INT_PTR ret = 0;
    if (!bOpenWith)
        ret = reinterpret_cast<INT_PTR>(ShellExecute(this->m_hWnd, nullptr, tempfile.GetWinPath(), nullptr, nullptr, SW_SHOWNORMAL));
    if ((ret <= HINSTANCE_ERROR) || bOpenWith)
    {
        OPENASINFO oi  = {nullptr};
        oi.pcszFile    = tempfile.GetWinPath();
        oi.oaifInFlags = OAIF_EXEC;
        SHOpenWithDialog(GetSafeHwnd(), &oi);
    }
}

void CLogDlg::OnNMDblclkChangedFileList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // a double click on an entry in the changed-files list has happened
    *pResult = 0;

    DiffSelectedFile(true);
}

void CLogDlg::DiffSelectedFile(bool ignoreProps)
{
    if ((m_bLogThreadRunning) || (m_logList.HasText()))
        return;
    UpdateLogInfoLabel();
    INT_PTR selIndex = m_changedFileListCtrl.GetSelectionMark();
    if (selIndex < 0)
        return;
    if (m_changedFileListCtrl.GetSelectedCount() == 0)
        return;
    // find out if there's an entry selected in the log list
    POSITION      pos       = m_logList.GetFirstSelectedItemPosition();
    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_logList.GetNextSelectedItem(pos));
    if (pLogEntry == nullptr)
        return;
    svn_revnum_t rev1 = pLogEntry->GetRevision();
    svn_revnum_t rev2 = rev1;
    if (pos)
    {
        while (pos)
        {
            // there's at least a second entry selected in the log list: several revisions selected!
            int index = m_logList.GetNextSelectedItem(pos);
            if (index < static_cast<int>(m_logEntries.GetVisibleCount()))
            {
                pLogEntry = m_logEntries.GetVisible(index);
                if (pLogEntry)
                {
                    const long entryRevision = static_cast<long>(pLogEntry->GetRevision());
                    rev1                     = max(rev1, entryRevision);
                    rev2                     = min(rev2, entryRevision);
                }
            }
        }
        rev2--;
        // now we have both revisions selected in the log list, so we can do a diff of the selected
        // entry in the changed files list with these two revisions.
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            DoDiffFromLog(selIndex, rev1, rev2, false, false, ignoreProps);
            this->EnableWindow(TRUE);
            this->SetFocus();
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();
        rev2                              = rev1 - 1;
        // nothing or only one revision selected in the log list

        const CLogChangedPath& changedPath = m_currentChangedArray[selIndex];

        if (IsDiffPossible(changedPath, rev1))
        {
            // diffs with renamed files are possible
            if (!changedPath.GetCopyFromPath().IsEmpty())
                rev2 = changedPath.GetCopyFromRev();
            else
            {
                // if the path was modified but the parent path was 'added with history'
                // then we have to use the copy from revision of the parent path
                CTSVNPath cPath = CTSVNPath(changedPath.GetPath());
                for (size_t fList = 0; fList < paths.GetCount(); ++fList)
                {
                    const CLogChangedPath& path = paths[fList];
                    CTSVNPath              p    = CTSVNPath(path.GetPath());
                    if (p.IsAncestorOf(cPath))
                    {
                        if (!path.GetCopyFromPath().IsEmpty())
                            rev2 = path.GetCopyFromRev();
                    }
                }
            }
            auto f = [=]() {
                CoInitialize(nullptr);
                OnOutOfScope(CoUninitialize());
                this->EnableWindow(FALSE);
                DoDiffFromLog(selIndex, rev1, rev2, false, false, ignoreProps);
                this->EnableWindow(TRUE);
                this->SetFocus();
            };
            new async::CAsyncCall(f, &netScheduler);
        }
        else
        {
            CTSVNPath tempFile  = CTempFiles::Instance().GetTempFilePath(false,
                                                                        CTSVNPath(changedPath.GetPath()));
            CTSVNPath tempFile2 = CTempFiles::Instance().GetTempFilePath(false,
                                                                         CTSVNPath(changedPath.GetPath()));
            SVNRev    r         = rev1;
            // deleted files must be opened from the revision before the deletion
            if (changedPath.GetAction() == LOGACTIONS_DELETED)
                r = rev1 - 1;
            m_bCancelled = false;

            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            CString sInfoLine;
            sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION,
                                    static_cast<LPCWSTR>(m_sRepositoryRoot + changedPath.GetPath()), static_cast<LPCWSTR>(r.ToString()));
            progDlg.SetLine(1, sInfoLine, true);
            SetAndClearProgressInfo(&progDlg);
            progDlg.ShowModeless(m_hWnd);

            CTSVNPath url = CTSVNPath(m_sRepositoryRoot + changedPath.GetPath());
            if (!Export(url, tempFile, r, r))
            {
                m_bCancelled = false;
                if (!Export(url, tempFile, SVNRev::REV_HEAD, r))
                {
                    progDlg.Stop();
                    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                    ShowErrorDialog(m_hWnd);
                    return;
                }
            }
            CString mimetype;
            CAppUtils::GetMimeType(url, mimetype, r);

            progDlg.Stop();
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));

            CString sName1, sName2;
            sName1.Format(L"%s - Revision %ld",
                          static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(changedPath.GetPath())), static_cast<svn_revnum_t>(rev1));
            sName2.Format(L"%s - Revision %ld",
                          static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(changedPath.GetPath())), static_cast<svn_revnum_t>(rev1) - 1);
            CAppUtils::DiffFlags flags;
            flags.AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
            if (changedPath.GetAction() == LOGACTIONS_DELETED)
                CAppUtils::StartExtDiff(tempFile, tempFile2, sName2, sName1,
                                        url, url, r, SVNRev(), r, flags, 0, url.GetFileOrDirectoryName(), mimetype);
            else
                CAppUtils::StartExtDiff(tempFile2, tempFile, sName2, sName1,
                                        url, url, r, SVNRev(), r, flags, 0, url.GetFileOrDirectoryName(), mimetype);
        }
    }
}

void CLogDlg::OnNMDblclkLoglist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // a double click on an entry in the revision list has happened
    *pResult = 0;

    if (m_logList.HasText())
    {
        m_logList.ClearText();
        FillLogMessageCtrl();
        if (!m_bMonitoringMode && m_pTaskbarList)
        {
            m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
        }
        if (m_bMonitoringMode)
        {
            // reset the auth-failed flag
            HTREEITEM hItem = m_projTree.GetSelectedItem();
            if (hItem)
            {
                MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
                if (pItem)
                {
                    pItem->authFailed = false;
                    pItem->lastErrorMsg.Empty();
                    m_projTree.SetItemState(hItem, 0, TVIS_OVERLAYMASK);
                    SaveMonitorProjects(false);
                }
            }
        }
        return;
    }
    if (CRegDWORD(L"Software\\TortoiseSVN\\DiffByDoubleClickInLog", FALSE))
        DiffSelectedRevWithPrevious();
}

void CLogDlg::DiffSelectedRevWithPrevious()
{
    if ((m_bLogThreadRunning) || (m_logList.HasText()))
        return;
    UpdateLogInfoLabel();
    int selIndex = m_logList.GetSelectionMark();
    if (selIndex < 0)
        return;
    int selCount = m_logList.GetSelectedCount();
    if (selCount != 1)
        return;

    // Find selected entry in the log list
    POSITION      pos       = m_logList.GetFirstSelectedItemPosition();
    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_logList.GetNextSelectedItem(pos));
    if (pLogEntry == nullptr)
        return;
    long      rev1 = pLogEntry->GetRevision();
    long      rev2 = rev1 - 1;
    CTSVNPath path = m_path;

    // See how many files under the relative root were changed in selected revision
    int    nChanged         = 0;
    size_t lastChangedIndex = static_cast<size_t>(-1);

    const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();
    for (size_t c = 0; c < paths.GetCount(); ++c)
        if (paths[c].IsRelevantForStartPath())
        {
            ++nChanged;
            lastChangedIndex = c;
        }

    svn_node_kind_t nodeKind = svn_node_unknown;
    if (!m_path.IsUrl())
        nodeKind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;

    if (nChanged == 1)
    {
        // We're looking at the log for a directory and only one file under
        // dir was changed in the revision. Do diff on that file instead of whole directory

        const CLogChangedPath& cPath = pLogEntry->GetChangedPaths()[lastChangedIndex];
        path.SetFromWin(m_sRepositoryRoot + cPath.GetPath());
        nodeKind = cPath.GetNodeKind() < 0 ? svn_node_unknown : cPath.GetNodeKind();
    }

    m_bCancelled = FALSE;
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    CLogWndHourglass wait;

    if (PromptShown())
    {
        SVNDiff diff(this, m_hWnd, true);
        diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
        diff.SetHEADPeg(m_logRevision);
        diff.ShowCompare(path, rev2, path, rev1, SVNRev(), false, true, L"", false, false, nodeKind);
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, path, rev2, path, rev1, SVNRev(),
                                    m_logRevision, false, true, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), false, false, nodeKind);
    }

    EnableOKButton();
}

void CLogDlg::DoDiffFromLog(INT_PTR selIndex, svn_revnum_t rev1, svn_revnum_t rev2, bool blame, bool unified, bool ignoreprops)
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
            return; //exit
        }
    }
    m_bCancelled = FALSE;
    filepath     = GetRepositoryRoot(CTSVNPath(filepath));
    if (filepath.IsEmpty())
    {
        ShowErrorDialog(m_hWnd);
        EnableOKButton();
        return; //exit
    }
    // filepath is in escaped form. But since the changedpath
    // is not, we have to unescape the filepath here first,
    // so the escaped parts won't get escaped again in case changedpath
    // needs escaping as well
    filepath = CPathUtils::PathUnescape(filepath);

    svn_node_kind_t nodeKind = svn_node_unknown;

    if (m_currentChangedArray.GetCount() <= static_cast<size_t>(selIndex))
        return;
    const CLogChangedPath& changedPath = m_currentChangedArray[selIndex];
    nodeKind                           = changedPath.GetNodeKind();
    CString firstFile                  = changedPath.GetPath();
    CString secondFile                 = firstFile;
    if ((rev2 == rev1 - 1) && (changedPath.GetCopyFromRev() > 0)) // is it an added file with history?
    {
        secondFile = changedPath.GetCopyFromPath();
        rev2       = changedPath.GetCopyFromRev();
    }

    firstFile  = filepath + firstFile.Trim();
    secondFile = filepath + secondFile.Trim();

    SVNDiff diff(this, this->m_hWnd, true);
    diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
    // since we have the direct urls here and revisions, we set the head peg
    // revision to rev1.
    diff.SetHEADPeg(rev1);
    if (unified)
    {
        bool    prettyPrint = true;
        CString options;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        {
            CDiffOptionsDlg dlg(this);
            if (dlg.DoModal() == IDOK)
            {
                options     = dlg.GetDiffOptionsString();
                prettyPrint = dlg.GetPrettyPrint();
            }
            else
            {
                EnableOKButton();
                return;
            }
        }
        if (PromptShown())
            diff.ShowUnifiedDiff(CTSVNPath(secondFile), rev2, CTSVNPath(firstFile), rev1,
                                 SVNRev(), prettyPrint, options, false, ignoreprops);
        else
            CAppUtils::StartShowUnifiedDiff(m_hWnd, CTSVNPath(secondFile), rev2, CTSVNPath(firstFile),
                                            rev1, SVNRev(), m_logRevision, prettyPrint, options, false, false, blame, ignoreprops);
    }
    else
    {
        diff.ShowCompare(CTSVNPath(secondFile), rev2, CTSVNPath(firstFile), rev1, SVNRev(),
                         ignoreprops, true, L"", false, blame, nodeKind);
    }
    EnableOKButton();
}

BOOL CLogDlg::Open(bool bOpenWith, CString fileUrl, svn_revnum_t rev)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);

    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_APPNAME);
    CString sInfoLine;
    sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, static_cast<LPCWSTR>(fileUrl),
                            static_cast<LPCWSTR>(SVNRev(rev).ToString()));
    progDlg.SetLine(1, sInfoLine, true);
    SetAndClearProgressInfo(&progDlg);
    progDlg.ShowModeless(m_hWnd);

    CTSVNPath tempFile = CTempFiles::Instance().GetTempFilePath(false, CTSVNPath(fileUrl), rev);
    m_bCancelled       = false;
    if (!Export(CTSVNPath(fileUrl), tempFile, SVNRev(rev), rev))
    {
        progDlg.Stop();
        SetAndClearProgressInfo(static_cast<HWND>(nullptr));
        ShowErrorDialog(m_hWnd);
        EnableOKButton();
        return FALSE;
    }
    progDlg.Stop();
    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
    DoOpenFileWith(true, bOpenWith, tempFile);
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

    CString name      = CString(SVN_PROP_REVISION_AUTHOR);
    CString value     = RevPropertyGet(name, CTSVNPath(url), logs[0]->GetRevision());
    CString sOldValue = value;
    value.Replace(L"\n", L"\r\n");

    CRenameDlg dlg(this);
    dlg.m_label.LoadString(IDS_LOG_AUTHOR);
    dlg.m_name = value;
    dlg.m_windowTitle.LoadString(IDS_LOG_AUTHOREDITTITLE);
    // gather all authors for autocomplete
    std::set<std::string> authorsA;
    for (size_t i = 0; i < m_logEntries.size(); ++i)
    {
        authorsA.insert(m_logEntries[i]->GetAuthor());
    }
    std::deque<std::wstring> authors;
    for (const auto& author : authorsA)
    {
        authors.push_back(CUnicodeUtils::StdGetUnicode(author));
    }
    dlg.SetCustomAutoComplete(authors);
    if (dlg.DoModal() == IDOK)
    {
        if (sOldValue.Compare(dlg.m_name))
        {
            dlg.m_name.Remove(L'\r');

            LogCache::CCachedLogInfo* toUpdate = GetLogCache(CTSVNPath(m_sRepositoryRoot));

            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
            progDlg.SetTime(true);
            progDlg.SetShowProgressBar(true);
            progDlg.ShowModeless(m_hWnd);
            for (DWORD i = 0; (i < logs.size()) && (!progDlg.HasUserCancelled()); ++i)
            {
                if (!RevPropertySet(name, dlg.m_name, sOldValue, CTSVNPath(url),
                                    logs[i]->GetRevision()))
                {
                    progDlg.Stop();
                    ShowErrorDialog(m_hWnd);
                    break;
                }
                logs[i]->SetAuthor(CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(dlg.m_name)));
                m_logList.Invalidate();

                // update the log cache
                if (toUpdate == nullptr)
                    continue;
                // log caching is active
                LogCache::CCachedLogInfo newInfo;
                newInfo.Insert(logs[i]->GetRevision(), logs[i]->GetAuthor().c_str(), "", 0, LogCache::CRevisionInfoContainer::HAS_AUTHOR);
                toUpdate->Update(newInfo);
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
                if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            progDlg.Stop();
        }
    }
    EnableOKButton();
}

void CLogDlg::EditLogMessage(size_t index)
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
    if (pLogEntry == nullptr)
        return;

    m_bCancelled      = FALSE;
    CString value     = RevPropertyGet(name, CTSVNPath(url), pLogEntry->GetRevision());
    CString sOldValue = value;
    value.Replace(L"\n", L"\r\n");
    CInputDlg dlg(this);
    dlg.m_sHintText.LoadString(IDS_LOG_MESSAGE);
    dlg.m_sInputText = value;
    dlg.m_sTitle.LoadString(IDS_LOG_MESSAGEEDITTITLE);
    if (dlg.DoModal() == IDOK)
    {
        if (sOldValue.Compare(dlg.m_sInputText))
        {
            dlg.m_sInputText.Remove(L'\r');
            if (!RevPropertySet(name, dlg.m_sInputText, sOldValue, CTSVNPath(url),
                                pLogEntry->GetRevision()))
            {
                ShowErrorDialog(m_hWnd);
            }
            else
            {
                pLogEntry->SetMessage(CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(dlg.m_sInputText)));

                // update the log cache
                LogCache::CCachedLogInfo* toUpdate = GetLogCache(CTSVNPath(m_sRepositoryRoot));
                if (toUpdate != nullptr)
                {
                    // log caching is active
                    LogCache::CCachedLogInfo newInfo;
                    newInfo.Insert(pLogEntry->GetRevision(), "", pLogEntry->GetMessage().c_str(), 0, LogCache::CRevisionInfoContainer::HAS_COMMENT);

                    toUpdate->Update(newInfo);
                    try
                    {
                        toUpdate->Save();
                    }
                    catch (CStreamException&)
                    {
                        // can't save the file right now
                    }
                }
                FillLogMessageCtrl();
                m_logList.Invalidate();
            }
        }
    }
    EnableOKButton();
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
    CWnd* wndFocus = GetFocus();

    // Skip Ctrl-C when copying text out of the log message or search filter
    BOOL bSkipAccelerator = (pMsg->message == WM_KEYDOWN &&
                             (pMsg->wParam == L'C' || pMsg->wParam == VK_INSERT) &&
                             (wndFocus == GetDlgItem(IDC_MSGVIEW) || wndFocus == GetDlgItem(IDC_SEARCHEDIT)) &&
                             GetKeyState(VK_CONTROL) & 0x8000);

    // Skip the 'Delete' key if the filter box is not empty
    bSkipAccelerator = bSkipAccelerator || (pMsg->message == WM_KEYDOWN &&
                                            (pMsg->wParam == VK_DELETE) &&
                                            (GetDlgItem(IDC_SEARCHEDIT)->GetWindowTextLength() > 0));
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            if (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\CtrlEnter", TRUE)))
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
                MonitorShowProject(hItem, nullptr);
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
    if (g_snarlGlobalMsg && (pMsg->message == g_snarlGlobalMsg))
    {
        if (pMsg->wParam == Snarl::V42::SnarlEnums::SnarlLaunched)
        {
            RegisterSnarl();
        }
        else if (pMsg->wParam == Snarl::V42::SnarlEnums::SnarlQuit)
        {
            UnRegisterSnarl();
        }

        return 1;
    }

    return __super::PreTranslateMessage(pMsg);
}

BOOL CLogDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (m_bLogThreadRunning || (!m_bMonitorThreadRunning && (netScheduler.GetRunningThreadCount())))
    {
        if (!IsCursorOverWindowBorder() && ((pWnd) && (pWnd != GetDlgItem(IDC_LOGCANCEL))))
        {
            HCURSOR hCur = LoadCursor(nullptr, IDC_WAIT);
            SetCursor(hCur);
            return TRUE;
        }
    }
    if ((pWnd) && (pWnd == GetDlgItem(IDC_MSGVIEW) || pWnd == GetDlgItem(IDC_SEARCHEDIT)))
        return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);

    HCURSOR hCur = LoadCursor(nullptr, IDC_ARROW);
    SetCursor(hCur);
    return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CLogDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CLogDlg::SelectAllVisibleRevisions()
{
    m_logList.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
    if (m_bStrict && m_bStrictStopped)
        m_logList.SetItemState(m_logList.GetItemCount() - 1, 0, LVIS_SELECTED);
    m_logList.RedrawWindow();
}

void CLogDlg::OnLvnItemchangedLoglist(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult           = 0;
    if ((m_bLogThreadRunning) || (m_logList.HasText()))
        return;
    if (pNMLV->iItem >= 0)
    {
        if (pNMLV->iSubItem != 0)
            return;

        size_t item = pNMLV->iItem;
        if ((item == m_logEntries.GetVisibleCount()) && (m_bStrict) && (m_bStrictStopped))
        {
            // remove the selected state
            if (pNMLV->uChanged & LVIF_STATE)
            {
                m_logList.SetItemState(pNMLV->iItem, 0, LVIS_SELECTED);
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
                    pLogEntry->SetChecked((pNMLV->uNewState & LVIS_SELECTED) != 0);
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

void CLogDlg::OnEnLinkMsgview(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult        = 0;
    ENLINK* pEnLink = reinterpret_cast<ENLINK*>(pNMHDR);
    if ((pEnLink->msg != WM_LBUTTONUP) && (pEnLink->msg != WM_SETCURSOR))
        return;

    auto      pEdit = reinterpret_cast<CRichEditCtrl*>(GetDlgItem(IDC_MSGVIEW));
    CHARRANGE selRange;
    pEdit->GetSel(selRange);
    if (selRange.cpMax != selRange.cpMin)
        return;

    CString url, msg;
    GetDlgItemText(IDC_MSGVIEW, msg);
    msg.Replace(L"\r\n", L"\n");
    url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax - pEnLink->chrg.cpMin);
    if (!::PathIsURL(url))
    {
        // not a full url: either a bug ID or a revision.
        std::set<CString> bugIDs      = m_projectProperties.FindBugIDs(msg);
        bool              bBugIDFound = false;
        for (std::set<CString>::iterator it = bugIDs.begin(); it != bugIDs.end(); ++it)
        {
            if (it->Compare(url) == 0)
            {
                url         = m_projectProperties.GetBugIDUrl(url);
                url         = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
                bBugIDFound = true;
                break;
            }
        }
        if ((!bBugIDFound) && (pEnLink->msg != WM_SETCURSOR))
        {
            // check if it's an email address
            auto atPos = url.Find('@');
            if ((atPos > 0) && (url.ReverseFind('.') > atPos) && !::PathIsURL(url))
            {
                ShellExecute(this->m_hWnd, nullptr, L"mailto:" + url, nullptr, nullptr, SW_SHOWDEFAULT);
            }
            // now check whether it matches a revision
            const std::wregex           regMatch(m_projectProperties.GetLogRevRegex(),
                                       std::regex_constants::icase | std::regex_constants::ECMAScript);
            const std::wsregex_iterator end;
            std::wstring                s = static_cast<LPCWSTR>(msg);
            for (std::wsregex_iterator it(s.begin(), s.end(), regMatch); it != end; ++it)
            {
                std::wstring      matchedString = (*it)[0];
                const std::wregex regRevMatch(L"\\d+");
                std::wstring      ss = matchedString;
                for (std::wsregex_iterator it2(ss.begin(), ss.end(), regRevMatch); it2 != end; ++it2)
                {
                    std::wstring matchedRevString = (*it2)[0];
                    if (url.Compare(matchedRevString.c_str()) == 0)
                    {
                        svn_revnum_t rev = _wtol(matchedRevString.c_str());
                        CTraceToOutputDebugString::Instance()(__FUNCTION__ ": found revision %ld\n", rev);
                        // do we already show this revision? If yes, just select that
                        // revision and 'scroll' to it
                        for (size_t i = 0; i < m_logEntries.GetVisibleCount(); ++i)
                        {
                            PLOGENTRYDATA data = m_logEntries.GetVisible(i);
                            if (!data)
                                continue;
                            if (data->GetRevision() != rev)
                                continue;
                            int selMark = m_logList.GetSelectionMark();
                            if (selMark >= 0)
                            {
                                m_logList.SetItemState(selMark, 0, LVIS_SELECTED);
                            }
                            m_logList.EnsureVisible(static_cast<int>(i), FALSE);
                            m_logList.SetSelectionMark(static_cast<int>(i));
                            m_logList.SetItemState(static_cast<int>(i), LVIS_SELECTED, LVIS_SELECTED);
                            PostMessage(WM_TSVN_REFRESH_SELECTION, 0, 0);
                            return;
                        }

                        // if we get here, then the linked revision is not shown in this dialog:
                        // start a new log dialog for the repository root and this revision
                        CString sCmd;
                        sCmd.Format(L"/command:log /path:\"%s\" /startrev:%ld /propspath:\"%s\"",
                                    static_cast<LPCWSTR>(m_sRepositoryRoot), rev, static_cast<LPCWSTR>(m_path.GetWinPath()));
                        CAppUtils::RunTortoiseProc(sCmd);
                        return;
                    }
                }
            }
        }
    }

    if ((!url.IsEmpty()) && (pEnLink->msg == WM_SETCURSOR))
    {
        CWnd* pMsgView = GetDlgItem(IDC_MSGVIEW);
        if (pMsgView)
        {
            RECT   rc;
            POINTL pt;
            pMsgView->SendMessage(EM_POSFROMCHAR, reinterpret_cast<WPARAM>(&pt), pEnLink->chrg.cpMin);
            rc.left = pt.x;
            rc.top  = pt.y;
            pMsgView->SendMessage(EM_POSFROMCHAR, reinterpret_cast<WPARAM>(&pt), pEnLink->chrg.cpMax);
            rc.right  = pt.x;
            rc.bottom = pt.y + 12;
            if ((m_lastTooltipRect.left != rc.left) || (m_lastTooltipRect.top != rc.top))
            {
                m_tooltips.DelTool(pMsgView, 1);
                m_tooltips.AddTool(pMsgView, url, &rc, 1);
                m_lastTooltipRect = rc;
            }
        }
        return;
    }
    if (!url.IsEmpty())
        ShellExecute(this->m_hWnd, L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
}

void CLogDlg::OnBnClickedStatbutton()
{
    if ((m_bLogThreadRunning) || (m_logList.HasText()))
        return;
    if (m_logEntries.GetVisibleCount() == 0)
        return; // nothing is shown, so no statistics.

    // the statistics dialog expects the log entries to be sorted by date
    // and we must remove duplicate entries created by merge info etc.

    std::map<__time64_t, PLOGENTRYDATA> revsByDate;

    std::set<svn_revnum_t> revisionsCovered;
    for (size_t i = 0; i < m_logEntries.GetVisibleCount(); ++i)
    {
        PLOGENTRYDATA entry = m_logEntries.GetVisible(i);
        if (entry)
        {
            if (revisionsCovered.insert(entry->GetRevision()).second)
                revsByDate.emplace(entry->GetDate(), entry);
        }
    }

    // create arrays which are aware of the current filter
    CStringArray arAuthorsFiltered;
    CDWordArray  arDatesFiltered;
    CDWordArray  arFileChangesFiltered;
    for (auto iter = revsByDate.rbegin(), end = revsByDate.rend(); iter != end; ++iter)
    {
        PLOGENTRYDATA pLogEntry = iter->second;
        CString       strAuthor = CUnicodeUtils::GetUnicode(pLogEntry->GetAuthor().c_str());
        if (strAuthor.IsEmpty())
        {
            strAuthor.LoadString(IDS_STATGRAPH_EMPTYAUTHOR);
        }
        arAuthorsFiltered.Add(strAuthor);
        arDatesFiltered.Add(static_cast<DWORD>(pLogEntry->GetDate()));
        arFileChangesFiltered.Add(static_cast<DWORD>(pLogEntry->GetChangedPaths().GetCount()));
    }
    CStatGraphDlg dlg;
    dlg.m_parAuthors     = &arAuthorsFiltered;
    dlg.m_parDates       = &arDatesFiltered;
    dlg.m_parFileChanges = &arFileChangesFiltered;
    dlg.m_path           = m_path;
    dlg.DoModal();
    OnTimer(LOGFILTER_TIMER);
}

void CLogDlg::OnNMCustomdrawLoglist(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_bLogThreadRunning)
        return;

    static COLORREF crText = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);

    switch (pLVCD->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
        {
            *pResult = CDRF_NOTIFYITEMDRAW;
            return;
        }
        case CDDS_ITEMPREPAINT:
        {
            // This is the prepaint stage for an item. Here's where we set the
            // item's text color.

            // Tell Windows to send draw notifications for each subitem.
            *pResult = CDRF_NOTIFYSUBITEMDRAW;
            crText   = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
            if (m_logEntries.GetVisibleCount() > pLVCD->nmcd.dwItemSpec)
            {
                PLOGENTRYDATA data = m_logEntries.GetVisible(pLVCD->nmcd.dwItemSpec);
                if (data)
                {
                    if (data->GetChangedPaths().ContainsSelfCopy())
                    {
                        if (!IsAppThemed())
                            pLVCD->clrTextBk = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_MENU));
                    }
                    if (data->GetChangedPaths().ContainsCopies())
                        crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Modified), true);
                    if ((data->GetDepth()) || (m_mergedRevs.find(data->GetRevision()) != m_mergedRevs.end()))
                        crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
                    if ((m_copyFromRev > data->GetRevision()) && !m_mergePath.IsEmpty())
                        crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
                    if ((data->GetRevision() == m_wcRev) || data->GetUnread())
                    {
                        SelectObject(pLVCD->nmcd.hdc, data->GetUnread() ? m_unreadFont : m_wcRevFont);
                        // We changed the font, so we're returning CDRF_NEWFONT. This
                        // tells the control to recalculate the extent of the text.
                        *pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
                    }
                }
            }
            if (m_logEntries.GetVisibleCount() == pLVCD->nmcd.dwItemSpec)
            {
                if (m_bStrictStopped)
                    crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
            }
            // Store the color back in the NMLVCUSTOMDRAW struct.
            pLVCD->clrText = crText;
            return;
        }
        case CDDS_ITEMPREPAINT | CDDS_ITEM | CDDS_SUBITEM:
        {
            *pResult                = CDRF_DODEFAULT;
            pLVCD->clrText          = crText;
            PLOGENTRYDATA pLogEntry = nullptr;
            if (m_logEntries.GetVisibleCount() > pLVCD->nmcd.dwItemSpec)
                pLogEntry = m_logEntries.GetVisible(pLVCD->nmcd.dwItemSpec);
            if ((m_bStrictStopped) && (m_logEntries.GetVisibleCount() == pLVCD->nmcd.dwItemSpec))
            {
                pLVCD->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS);
            }
            switch (pLVCD->iSubItem)
            {
                case 0: // revision
                    if (pLogEntry == nullptr)
                        return;
                    if ((m_selectedFilters & LOGFILTER_REVS) && (m_filter.IsFilterActive()))
                    {
                        *pResult = DrawListItemWithMatches(m_logList, pLVCD, pLogEntry);
                        return;
                    }
                    break;
                case 1: // actions
                {
                    if (pLogEntry == nullptr)
                        return;

                    int nIcons     = 0;
                    int iconWidth  = ::GetSystemMetrics(SM_CXSMICON);
                    int iconHeight = ::GetSystemMetrics(SM_CYSMICON);

                    CRect  rect = DrawListColumnBackground(m_logList, pLVCD, pLogEntry);
                    CMemDC myDC(*CDC::FromHandle(pLVCD->nmcd.hdc), rect);
                    BitBlt(myDC.GetDC(), rect.left, rect.top, rect.Width(), rect.Height(), pLVCD->nmcd.hdc, rect.left, rect.top, SRCCOPY);

                    // Draw the icon(s) into the compatible DC
                    auto  iconItemBorder = CDPIAware::Instance().Scale(GetSafeHwnd(), ICONITEMBORDER);
                    DWORD actions        = pLogEntry->GetChangedPaths().GetActions();
                    if (actions & LOGACTIONS_MODIFIED)
                        ::DrawIconEx(myDC.GetDC(), rect.left + iconItemBorder, rect.top,
                                     m_hModifiedIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_ADDED)
                        ::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconWidth + iconItemBorder,
                                     rect.top, m_hAddedIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_DELETED)
                        ::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconWidth + iconItemBorder,
                                     rect.top, m_hDeletedIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_REPLACED)
                        ::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconWidth + iconItemBorder,
                                     rect.top, m_hReplacedIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_MOVED)
                        ::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconWidth + iconItemBorder,
                                     rect.top, m_hMovedIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
                    nIcons++;

                    if (actions & LOGACTIONS_MOVEREPLACED)
                        ::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconWidth + iconItemBorder,
                                     rect.top, m_hMoveReplacedIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
                    nIcons++;

                    if ((pLogEntry->GetDepth()) ||
                        (m_mergedRevs.find(pLogEntry->GetRevision()) != m_mergedRevs.end()))
                    {
                        if (pLogEntry->IsSubtractiveMerge())
                            ::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconWidth + iconItemBorder,
                                         rect.top, m_hReverseMergedIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
                        else
                            ::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconWidth + iconItemBorder,
                                         rect.top, m_hMergedIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
                    }
                    nIcons++;

                    *pResult = CDRF_SKIPDEFAULT;
                    return;
                }
                case 2: // author
                    if (pLogEntry == nullptr)
                        return;
                    if ((m_selectedFilters & LOGFILTER_AUTHORS) && (m_filter.IsFilterActive()))
                    {
                        *pResult = DrawListItemWithMatches(m_logList, pLVCD, pLogEntry);
                        return;
                    }
                    break;
                case 3: // date
                    if (pLogEntry == nullptr)
                        return;
                    if ((m_selectedFilters & LOGFILTER_DATE) && (m_filter.IsFilterActive()))
                    {
                        *pResult = DrawListItemWithMatches(m_logList, pLVCD, pLogEntry);
                        return;
                    }
                    break;
                case 4: //message or bug id
                    if (pLogEntry == nullptr)
                        return;
                    if (m_bShowBugtraqColumn)
                    {
                        if ((m_selectedFilters & LOGFILTER_BUGID) && (m_filter.IsFilterActive()))
                        {
                            *pResult = DrawListItemWithMatches(m_logList, pLVCD, pLogEntry);
                            return;
                        }
                        break;
                    }
                    [[fallthrough]];
                case 5: // log msg
                    if (pLogEntry == nullptr)
                        return;
                    if ((m_selectedFilters & LOGFILTER_MESSAGES) && (m_filter.IsFilterActive()))
                    {
                        *pResult = DrawListItemWithMatches(m_logList, pLVCD, pLogEntry);
                        return;
                    }
                    break;
            }
        }
        break;
    }
    *pResult = CDRF_DODEFAULT;
}

void CLogDlg::OnNMCustomdrawChangedFileList(NMHDR* pNMHDR, LRESULT* pResult)
{
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_bLogThreadRunning)
        return;

    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    // First thing - check the draw stage. If it's the control's prepaint
    // stage, then tell Windows we want messages for every item.

    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        // Tell Windows to send draw notifications for each subitem.
        *pResult = CDRF_NOTIFYSUBITEMDRAW;

        COLORREF crText  = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
        bool     bGrayed = false;
        if ((m_cShowPaths.GetState() & 0x0003) == BST_UNCHECKED)
        {
            if (m_currentChangedArray.GetCount() > pLVCD->nmcd.dwItemSpec)
            {
                if (!m_currentChangedArray[pLVCD->nmcd.dwItemSpec].IsRelevantForStartPath())
                {
                    crText  = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
                    bGrayed = true;
                }
            }
            else if (static_cast<DWORD_PTR>(m_currentChangedPathList.GetCount()) > pLVCD->nmcd.dwItemSpec)
            {
                if (m_currentChangedPathList[pLVCD->nmcd.dwItemSpec].GetSVNPathString().Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot) != 0)
                {
                    crText  = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
                    bGrayed = true;
                }
            }
        }

        if ((!bGrayed) && (m_currentChangedArray.GetCount() > pLVCD->nmcd.dwItemSpec))
        {
            DWORD action = m_currentChangedArray[pLVCD->nmcd.dwItemSpec].GetAction();
            if (action == LOGACTIONS_MODIFIED)
                crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Modified), true);
            if (action == LOGACTIONS_REPLACED)
                crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Deleted), true);
            if (action == LOGACTIONS_ADDED)
                crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Added), true);
            if (action == LOGACTIONS_DELETED)
                crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Deleted), true);
            if (action == LOGACTIONS_MOVED)
                crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Added), true);
            if (action == LOGACTIONS_MOVEREPLACED)
                crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Deleted), true);
        }
        if (m_currentChangedArray.GetCount() > pLVCD->nmcd.dwItemSpec)
        {
            svn_tristate_t textModifies  = m_currentChangedArray[pLVCD->nmcd.dwItemSpec].GetTextModifies();
            svn_tristate_t propsModifies = m_currentChangedArray[pLVCD->nmcd.dwItemSpec].GetPropsModifies();
            if ((propsModifies == svn_tristate_true) && (textModifies != svn_tristate_true))
            {
                // property only modification, content of entry hasn't changed: show in gray
                crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
            }
        }

        // Store the color back in the NMLVCUSTOMDRAW struct.
        pLVCD->clrText = crText;
    }
    else if (pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_ITEM | CDDS_SUBITEM))
    {
        if ((m_selectedFilters & LOGFILTER_PATHS) && (m_filter.IsFilterActive()))
        {
            *pResult = DrawListItemWithMatches(m_changedFileListCtrl, pLVCD, nullptr);
            return;
        }
    }
}

CRect CLogDlg::DrawListColumnBackground(const CListCtrl& listCtrl, const NMLVCUSTOMDRAW* pLVCD,
                                        PLOGENTRYDATA pLogEntry) const
{
    // Get the selected state of the
    // item being drawn.
    LVITEM rItem    = {0};
    rItem.mask      = LVIF_STATE;
    rItem.iItem     = static_cast<int>(pLVCD->nmcd.dwItemSpec);
    rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    listCtrl.GetItem(&rItem);

    CRect rect;
    listCtrl.GetSubItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem, LVIR_BOUNDS, rect);

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
        OnOutOfScope(CloseThemeData(hTheme));

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
                if (pLVCD->clrTextBk == CTheme::Instance().GetThemeColor(GetSysColor(COLOR_MENU)))
                {
                    HBRUSH brush = ::CreateSolidBrush(CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_MENU)));
                    if (brush)
                    {
                        ::FillRect(pLVCD->nmcd.hdc, &rect, brush);
                        ::DeleteObject(brush);
                    }
                }
            }
        }

        if (IsThemeBackgroundPartiallyTransparent(hTheme, LVP_LISTITEM, state))
            DrawThemeParentBackground(m_hWnd, pLVCD->nmcd.hdc, &rect);

        // don't draw the background here:
        // if we changed the background, we've already painted it.
        // for the default background: just leave it as is because it was already
        // painted!
        // DrawThemeBackground(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, state, &rect, NULL);
    }
    else
    {
        HBRUSH brush;
        if (rItem.state & LVIS_SELECTED)
        {
            if (::GetFocus() == listCtrl.m_hWnd)
                brush = ::CreateSolidBrush(CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_HIGHLIGHT)));
            else
                brush = ::CreateSolidBrush(CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_BTNFACE)));
        }
        else
        {
            if (pLogEntry && pLogEntry->GetChangedPaths().ContainsSelfCopy())
                brush = ::CreateSolidBrush(CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_MENU)));
            else
                brush = ::CreateSolidBrush(CTheme::Instance().IsDarkTheme() ? CTheme::darkBkColor : GetSysColor(COLOR_WINDOW));
        }
        if (brush == nullptr)
            return rect;

        ::FillRect(pLVCD->nmcd.hdc, &rect, brush);
        ::DeleteObject(brush);
    }

    return rect;
}

LRESULT CLogDlg::DrawListItemWithMatches(CListCtrl& listCtrl, NMLVCUSTOMDRAW* pLVCD,
                                         PLOGENTRYDATA pLogEntry)
{
    std::wstring text;
    text = static_cast<LPCWSTR>(listCtrl.GetItemText(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem));
    if (text.empty())
        return CDRF_DODEFAULT;

    std::wstring           matchText = text;
    std::vector<CHARRANGE> ranges    = m_filter.GetMatchRanges(matchText);
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

        listCtrl.GetItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), &labelRC, LVIR_LABEL);
        listCtrl.GetItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), &iconRC, LVIR_ICON);
        listCtrl.GetItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), &boundsRC, LVIR_BOUNDS);

        DrawListColumnBackground(listCtrl, pLVCD, pLogEntry);
        int leftMargin = labelRC.left - boundsRC.left;
        if (pLVCD->iSubItem)
        {
            leftMargin -= iconRC.Width();
        }
        if (pLVCD->iSubItem != 0)
            listCtrl.GetSubItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem, LVIR_BOUNDS, rect);

        int borderWidth = 0;
        if (IsAppThemed())
        {
            CAutoThemeData hTheme = OpenThemeData(m_hWnd, L"LISTVIEW");
            GetThemeMetric(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, LISS_NORMAL, TMT_BORDERSIZE,
                           &borderWidth);
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
            leftMargin = 4;
        }

        LVITEM item    = {0};
        item.iItem     = static_cast<int>(pLVCD->nmcd.dwItemSpec);
        item.iSubItem  = 0;
        item.mask      = LVIF_IMAGE | LVIF_STATE;
        item.stateMask = static_cast<UINT>(-1);
        listCtrl.GetItem(&item);

        // draw the icon for the first column
        if (pLVCD->iSubItem == 0)
        {
            rect       = boundsRC;
            rect.right = rect.left + listCtrl.GetColumnWidth(0) - 2 * borderWidth;
            rect.left  = iconRC.left;

            if (item.iImage >= 0)
            {
                POINT pt;
                pt.x = rect.left;
                pt.y = rect.top;
                CDC dc;
                dc.Attach(pLVCD->nmcd.hdc);
                listCtrl.GetImageList(LVSIL_SMALL)->Draw(&dc, item.iImage, pt, ILD_TRANSPARENT);
                dc.Detach();
                leftMargin -= iconRC.left;
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
                if ((state) && (listCtrl.GetExtendedStyle() & LVS_EX_CHECKBOXES))
                {
                    CAutoThemeData hTheme = OpenThemeData(m_hWnd, L"BUTTON");
                    DrawThemeBackground(hTheme, pLVCD->nmcd.hdc, BP_CHECKBOX, state, &irc, nullptr);
                }
            }
        }
        InflateRect(&rect, -(2 * borderWidth), 0);

        rect.left += leftMargin;
        RECT rc = rect;

        // is the column left- or right-aligned? (we don't handle centered (yet))
        LVCOLUMN column;
        column.mask = LVCF_FMT;
        listCtrl.GetColumn(pLVCD->iSubItem, &column);
        if (column.fmt & LVCFMT_RIGHT)
        {
            DrawText(pLVCD->nmcd.hdc, text.c_str(), -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
            rect.left = rect.right - (rc.right - rc.left);
            if (!IsAppThemed())
            {
                rect.left += 2 * borderWidth;
                rect.right += 2 * borderWidth;
            }
        }
        COLORREF textColor = pLVCD->clrText;
        if ((item.state & LVIS_SELECTED) && !IsAppThemed())
        {
            if (::GetFocus() == listCtrl.GetSafeHwnd())
                textColor = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
            else
                textColor = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
        }
        SetTextColor(pLVCD->nmcd.hdc, textColor);
        SetBkMode(pLVCD->nmcd.hdc, TRANSPARENT);
        for (std::vector<CHARRANGE>::iterator it = ranges.begin(); it != ranges.end(); ++it)
        {
            rc = rect;
            if (it->cpMin - drawPos)
            {
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMin - drawPos, &rc,
                         DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMin - drawPos, &rc,
                         DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
                rect.left = rc.right;
            }
            rc      = rect;
            drawPos = it->cpMin;
            if (it->cpMax - drawPos)
            {
                SetTextColor(pLVCD->nmcd.hdc, CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::FilterMatch), true));
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMax - drawPos, &rc,
                         DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
                DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), it->cpMax - drawPos, &rc,
                         DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
                rect.left = rc.right;
                SetTextColor(pLVCD->nmcd.hdc, textColor);
            }
            rc      = rect;
            drawPos = it->cpMax;
        }
        DrawText(pLVCD->nmcd.hdc, text.substr(drawPos).c_str(), -1, &rc,
                 DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
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
    m_changedFileListCtrl.GetClientRect(changeListViewRect);
    CRect messageViewRect;
    GetDlgItem(IDC_MSGVIEW)->GetClientRect(messageViewRect);

    int messageViewDelta    = max(-delta, 20 - messageViewRect.Height());
    int changeFileListDelta = -delta - messageViewDelta;

    // set new sizes & positions

    auto hdwp = BeginDeferWindowPos(4);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_logList, 0, 0, 0, delta);
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_MSGVIEW), 0, delta, 0, delta + messageViewDelta);
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_SPLITTERBOTTOM), 0, -changeFileListDelta, 0, -changeFileListDelta);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_changedFileListCtrl, 0, -changeFileListDelta, 0, 0);
    EndDeferWindowPos(hdwp);

    AddMainAnchors();
    ArrangeLayout();
    SetSplitterRange();
    m_logList.Invalidate();
    m_changedFileListCtrl.Invalidate();
    GetDlgItem(IDC_MSGVIEW)->Invalidate();
}

void CLogDlg::DoSizeV2(int delta)
{
    RemoveMainAnchors();

    // first, reduce the middle section to a minimum.
    // if that is not sufficient, minimize the top section

    CRect logViewRect;
    m_logList.GetClientRect(logViewRect);
    CRect messageViewRect;
    GetDlgItem(IDC_MSGVIEW)->GetClientRect(messageViewRect);

    int messageViewDelta = max(delta, 20 - messageViewRect.Height());
    int logListDelta     = delta - messageViewDelta;

    // set new sizes & positions
    auto hdwp = BeginDeferWindowPos(4);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_logList, 0, 0, 0, logListDelta);
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_SPLITTERTOP), 0, logListDelta, 0, logListDelta);
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_MSGVIEW), 0, logListDelta, 0, logListDelta + messageViewDelta);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_changedFileListCtrl, 0, delta, 0, 0);
    EndDeferWindowPos(hdwp);

    AddMainAnchors();
    ArrangeLayout();
    SetSplitterRange();
    GetDlgItem(IDC_MSGVIEW)->Invalidate();
    m_changedFileListCtrl.Invalidate();
}

void CLogDlg::DoSizeV3(int delta)
{
    RemoveMainAnchors();

    auto hdwp = BeginDeferWindowPos(9);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_projTree, 0, 0, delta, 0);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_cFilter, delta, 0, 0, 0);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_logList, delta, 0, 0, 0);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_wndSplitter1, delta, 0, 0, 0);
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_MSGVIEW), delta, 0, 0, 0);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_wndSplitter2, delta, 0, 0, 0);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_changedFileListCtrl, delta, 0, 0, 0);
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_logProgress, delta, 0, 0, 0);
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_LOGINFO), delta, 0, 0, 0);
    EndDeferWindowPos(hdwp);

    AddMainAnchors();
    ArrangeLayout();
    SetSplitterRange();
    GetDlgItem(IDC_MSGVIEW)->Invalidate();
    m_logList.Invalidate();
    m_changedFileListCtrl.Invalidate();
    m_cFilter.Redraw();
}

LRESULT CLogDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_NOTIFY:
            if (wParam == IDC_SPLITTERTOP)
            {
                SpcNMHDR* pHdr = reinterpret_cast<SpcNMHDR*>(lParam);
                DoSizeV1(pHdr->delta);
            }
            else if (wParam == IDC_SPLITTERBOTTOM)
            {
                SpcNMHDR* pHdr = reinterpret_cast<SpcNMHDR*>(lParam);
                DoSizeV2(pHdr->delta);
            }
            else if (wParam == IDC_SPLITTERLEFT)
            {
                SpcNMHDR* pHdr = reinterpret_cast<SpcNMHDR*>(lParam);
                DoSizeV3(pHdr->delta);
            }
            else
            {
                auto pNMHDR = reinterpret_cast<LPNMHDR>(lParam);
                if ((pNMHDR->code == NM_CUSTOMDRAW) && (pNMHDR->hwndFrom == m_hwndToolbar))
                {
                    if (CTheme::Instance().IsDarkTheme())
                    {
                        auto pNMTB = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);
                        switch (pNMTB->nmcd.dwDrawStage)
                        {
                            case CDDS_PREPAINT:
                                return CDRF_NOTIFYITEMDRAW;
                            case CDDS_ITEMPREPAINT:
                            {
                                pNMTB->clrText = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
                                return CDRF_DODEFAULT | TBCDRF_USECDCOLORS;
                            }
                        }
                    }
                }
            }
            break;
    }

    return CResizableDialog::DefWindowProc(message, wParam, lParam);
}

void CLogDlg::SetSplitterRange()
{
    if ((m_logList) && (m_changedFileListCtrl))
    {
        CRect rcTop;
        m_logList.GetWindowRect(rcTop);
        ScreenToClient(rcTop);

        CRect rcMiddle;
        GetDlgItem(IDC_MSGVIEW)->GetWindowRect(rcMiddle);
        ScreenToClient(rcMiddle);

        CRect rcBottom;
        m_changedFileListCtrl.GetWindowRect(rcBottom);
        ScreenToClient(rcBottom);

        m_wndSplitter1.SetRange(rcTop.top + MIN_CTRL_HEIGHT, rcBottom.bottom - (2 * MIN_CTRL_HEIGHT + MIN_SPLITTER_HEIGHT));
        m_wndSplitter2.SetRange(rcTop.top + (2 * MIN_CTRL_HEIGHT + MIN_SPLITTER_HEIGHT), rcBottom.bottom - MIN_CTRL_HEIGHT);
        m_wndSplitterLeft.SetRange(CDPIAware::Instance().Scale(GetSafeHwnd(), 80), rcTop.right - m_logListOrigRect.Width());
    }
}

LRESULT CLogDlg::OnClickedInfoIcon(WPARAM /*wParam*/, LPARAM lParam)
{
    RECT*  rect  = reinterpret_cast<LPRECT>(lParam);
    CPoint point = CPoint(rect->left, rect->bottom);
#define LOGMENUFLAGS(x) (MF_STRING | MF_ENABLED | ((m_selectedFilters & x) ? MF_CHECKED : MF_UNCHECKED))
    CMenu popup;
    if (popup.CreatePopupMenu())
    {
        CString temp;
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
        int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                                             point.x, point.y, this, nullptr);
        switch (selection)
        {
            case 0:
                break;

            case LOGFILTER_REGEX:
                m_bFilterWithRegex                                  = !m_bFilterWithRegex;
                CRegDWORD(L"Software\\TortoiseSVN\\UseRegexFilter") = static_cast<DWORD>(m_bFilterWithRegex);
                CheckRegexpTooltip();
                break;

            case LOGFILTER_CASE:
                m_bFilterCaseSensitively                                   = !m_bFilterCaseSensitively;
                CRegDWORD(L"Software\\TortoiseSVN\\FilterCaseSensitively") = static_cast<DWORD>(m_bFilterCaseSensitively);
                break;

            default:

                m_selectedFilters ^= selection;
                SetFilterCueText();
                AdjustDateFilterVisibility();
        }

        if (selection != 0)
            SetTimer(LOGFILTER_TIMER, 1000, nullptr);
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
    m_logEntries.ClearFilter(!!m_bHideNonMergeables, &m_mergedRevs, m_copyFromRev);
    m_timFrom = m_logEntries.GetMinDate();
    m_timTo   = m_logEntries.GetMaxDate();
    m_dateFrom.SetTime(&m_timFrom);
    m_dateTo.SetTime(&m_timTo);
    m_dateFrom.SetRange(&m_timFrom, &m_timTo);
    m_dateTo.SetRange(&m_timFrom, &m_timTo);

    m_filter = CLogDlgFilter();

    m_logList.SetItemCountEx(0);
    m_logList.SetItemCountEx(ShownCountWithStopped());
    m_logList.RedrawItems(0, ShownCountWithStopped());
    m_logList.SetRedraw(false);
    ResizeAllListCtrlCols(true);
    m_logList.SetRedraw(true);
    m_logList.Invalidate();
    m_changedFileListCtrl.Invalidate();

    GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_SEARCHEDIT)->SetFocus();

    AutoRestoreSelection(true);

    DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning) || (m_logEntries.GetVisibleCount() == 0))));

    return 0L;
}

void CLogDlg::SetFilterCueText()
{
    CString temp(MAKEINTRESOURCE(IDS_LOG_FILTER_BY));

    if (m_selectedFilters & LOGFILTER_MESSAGES)
    {
        temp += L" ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_MESSAGES));
    }
    if (m_selectedFilters & LOGFILTER_PATHS)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_PATHS));
    }
    if (m_selectedFilters & LOGFILTER_AUTHORS)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_AUTHORS));
    }
    if (m_selectedFilters & LOGFILTER_REVS)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_REVS));
    }
    if (m_selectedFilters & LOGFILTER_BUGID)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_BUGIDS));
    }
    if (m_selectedFilters & LOGFILTER_DATE)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_DATE));
    }
    if (m_selectedFilters & LOGFILTER_DATERANGE)
    {
        if (!temp.IsEmpty())
            temp += L", ";
        temp += CString(MAKEINTRESOURCE(IDS_LOG_FILTER_DATERANGE));
    }

    // to make the cue banner text appear more to the right of the edit control
    temp = L"   " + temp;
    m_cFilter.SetCueBanner(temp);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CLogDlg::OnLvnGetdispinfoLoglist(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult                = 0;
    NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

    // Create a pointer to the item
    LV_ITEM* pItem = &(pDispInfo)->item;

    // Which item number?
    size_t        itemId    = pItem->iItem;
    PLOGENTRYDATA pLogEntry = nullptr;
    if (itemId < m_logEntries.GetVisibleCount())
        pLogEntry = m_logEntries.GetVisible(pItem->iItem);

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
            UINT state = m_logList.GetItemState(pItem->iItem, LVIS_SELECTED);
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
        lstrcpyn(pItem->pszText, L"", pItem->cchTextMax - 1);

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
                    size_t   len     = wcslen(pItem->pszText);
                    wchar_t* pBuf    = pItem->pszText + len;
                    DWORD    nSpaces = m_logEntries.GetMaxDepth() - pLogEntry->GetDepth();
                    while ((pItem->cchTextMax > static_cast<int>(len)) && (nSpaces))
                    {
                        *pBuf = L' ';
                        pBuf++;
                        nSpaces--;
                    }
                    *pBuf = 0;
                }
                break;
            case 1: // action -- dummy text, not drawn. Used to trick the auto-column resizing to not
                // go below the icons. Note this is required for Ctrl+ auto-resizing, manual auto-resizing
                // could be handled in ResizeAllListCtrlCols().
                if (pLogEntry)
                    lstrcpyn(pItem->pszText, L"Action column" /*L"XXXXXXXXXXXXXXXX"*/, pItem->cchTextMax - 1);
                break;
            case 2: //author
                if (pLogEntry)
                {
                    lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetAuthor()).c_str(),
                             pItem->cchTextMax - 1);
                }
                break;
            case 3: //date
                if (pLogEntry)
                {
                    lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetDateString()).c_str(),
                             pItem->cchTextMax - 1);
                }
                break;
            case 4: //message or bug id
                if (m_bShowBugtraqColumn)
                {
                    if (pLogEntry)
                    {
                        lstrcpyn(pItem->pszText, CUnicodeUtils::StdGetUnicode(pLogEntry->GetBugIDs()).c_str(),
                                 pItem->cchTextMax - 1);
                    }
                    break;
                }
                [[fallthrough]];
            case 5:
                if (pLogEntry)
                {
                    // Add as many characters as possible from the short log message
                    // to the list control. If the message is longer than
                    // allowed width, add "..." as an indication.
                    const int dotsLen      = 3;
                    CString   shortMessage = pLogEntry->GetShortMessageUTF16();
                    if (shortMessage.GetLength() > pItem->cchTextMax && pItem->cchTextMax > dotsLen)
                    {
                        lstrcpyn(pItem->pszText, static_cast<LPCWSTR>(shortMessage), pItem->cchTextMax - dotsLen);
                        lstrcpyn(pItem->pszText + pItem->cchTextMax - dotsLen - 1, L"...", dotsLen + 1);
                    }
                    else
                        lstrcpyn(pItem->pszText, static_cast<LPCWSTR>(shortMessage), pItem->cchTextMax - 1);
                }
                else if ((itemId == m_logEntries.GetVisibleCount()) && m_bStrict && m_bStrictStopped)
                {
                    CString sTemp;
                    sTemp.LoadString(IDS_LOG_STOPONCOPY_HINT);
                    lstrcpyn(pItem->pszText, sTemp, pItem->cchTextMax - 1LL);
                }
                break;
            default:
                ASSERT(false);
        }
    }
}

void CLogDlg::OnLvnGetdispinfoChangedFileList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    //Create a pointer to the item
    LV_ITEM* pItem = &(pDispInfo)->item;

    if (m_bLogThreadRunning)
    {
        if (pItem->mask & LVIF_TEXT)
            lstrcpyn(pItem->pszText, L"", pItem->cchTextMax - 1LL);
        return;
    }
    if (m_bSingleRevision && (static_cast<size_t>(pItem->iItem) >= m_currentChangedArray.GetCount()))
    {
        if (pItem->mask & LVIF_TEXT)
            lstrcpyn(pItem->pszText, L"", pItem->cchTextMax - 1LL);
        return;
    }
    if (!m_bSingleRevision && (pItem->iItem >= m_currentChangedPathList.GetCount()))
    {
        if (pItem->mask & LVIF_TEXT)
            lstrcpyn(pItem->pszText, L"", pItem->cchTextMax - 1LL);
        return;
    }

    //Does the list need text information?
    if (pItem->mask & LVIF_TEXT)
    {
        //Which column?
        switch (pItem->iSubItem)
        {
            case 0: //path
                lstrcpyn(pItem->pszText, static_cast<LPCWSTR>(m_currentChangedArray.GetCount() > 0 ? m_currentChangedArray[pItem->iItem].GetPath() : m_currentChangedPathList[pItem->iItem].GetSVNPathString()), pItem->cchTextMax - 1);
                break;

            case 1: //Action
                lstrcpyn(pItem->pszText, m_bSingleRevision && m_currentChangedArray.GetCount() > static_cast<size_t>(pItem->iItem) ? static_cast<LPCWSTR>(CUnicodeUtils::GetUnicode(m_currentChangedArray[pItem->iItem].GetActionString().c_str())) : L"", pItem->cchTextMax - 1);
                break;

            case 2: //copyfrom path
                lstrcpyn(pItem->pszText, m_bSingleRevision && m_currentChangedArray.GetCount() > static_cast<size_t>(pItem->iItem) ? static_cast<LPCWSTR>(m_currentChangedArray[pItem->iItem].GetCopyFromPath()) : L"", pItem->cchTextMax - 1);
                break;

            case 3: //revision
                svn_revnum_t revision = 0;
                if (m_bSingleRevision && m_currentChangedArray.GetCount() > static_cast<size_t>(pItem->iItem))
                    revision = m_currentChangedArray[pItem->iItem].GetCopyFromRev();

                if (revision == 0)
                    lstrcpyn(pItem->pszText, L"", pItem->cchTextMax - 1LL);
                else
                    swprintf_s(pItem->pszText, pItem->cchTextMax, L"%ld", revision);
                break;
        }
    }
    if (pItem->mask & LVIF_IMAGE)
    {
        int iconIdx = 0;
        if (m_currentChangedArray.GetCount() > static_cast<size_t>(pItem->iItem))
        {
            const CLogChangedPath& cPath = m_currentChangedArray[pItem->iItem];
            if (cPath.GetNodeKind() == svn_node_dir)
                iconIdx = m_nIconFolder;
            else
                iconIdx = SYS_IMAGE_LIST().GetPathIconIndex(CTSVNPath(cPath.GetPath()));
        }
        else
        {
            iconIdx = SYS_IMAGE_LIST().GetPathIconIndex(m_currentChangedPathList[pItem->iItem]);
        }
        pDispInfo->item.iImage = iconIdx;
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
        m_logEntries.Filter(m_tFrom, m_tTo, !!m_bHideNonMergeables, &m_mergedRevs, m_copyFromRev);
        m_logList.SetItemCountEx(0);
        m_logList.SetItemCountEx(ShownCountWithStopped());
        m_logList.RedrawItems(0, ShownCountWithStopped());
        m_logList.SetRedraw(false);
        ResizeAllListCtrlCols(true);
        m_logList.SetRedraw(true);
        m_logList.Invalidate();
        m_changedFileListCtrl.Invalidate();
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
        DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning) || (m_logEntries.GetVisibleCount() == 0))));

        AutoRestoreSelection(true);
        return;
    }
    if (Validate(m_sFilterText) && FilterConditionChanged())
        SetTimer(LOGFILTER_TIMER, 1000, nullptr);
    else
        KillTimer(LOGFILTER_TIMER);
}

bool CLogDlg::ValidateRegexp(LPCWSTR regexpStr, std::wregex& pat, bool bMatchCase /* = false */)
{
    try
    {
        std::regex_constants::syntax_option_type type = std::regex_constants::ECMAScript;
        if (!bMatchCase)
            type |= std::regex_constants::icase;
        pat = std::wregex(regexpStr, type);
        return true;
    }
    catch (std::exception&)
    {
    }
    return false;
}

bool CLogDlg::Validate(LPCWSTR string)
{
    if (!m_bFilterWithRegex)
        return true;
    std::wregex pat;
    return ValidateRegexp(string, pat, false);
}

bool CLogDlg::FilterConditionChanged()
{
    // actually filter the data

    bool          scanRelevantPathsOnly = (m_cShowPaths.GetState() & 0x0003) == BST_CHECKED;
    CLogDlgFilter filter(m_sFilterText, m_bFilterWithRegex, m_selectedFilters, m_bFilterCaseSensitively, m_tFrom, m_tTo, scanRelevantPathsOnly, &m_mergedRevs, !!m_bHideNonMergeables, m_copyFromRev, NO_REVISION);

    return m_filter != filter;
}

void CLogDlg::RecalculateShownList(svn_revnum_t revToKeep)
{
    // actually filter the data

    bool          scanRelevantPathsOnly = (m_cShowPaths.GetState() & 0x0003) == BST_CHECKED;
    CLogDlgFilter filter(m_sFilterText, m_bFilterWithRegex, m_selectedFilters, m_bFilterCaseSensitively, m_tFrom, m_tTo, scanRelevantPathsOnly, &m_mergedRevs, !!m_bHideNonMergeables, m_copyFromRev, revToKeep);
    m_filter = filter;
    m_logEntries.Filter(filter);
}

void CLogDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == LOGFILTER_TIMER)
    {
        if (m_bLogThreadRunning || IsIconic())
        {
            // thread still running! So just restart the timer.
            SetTimer(LOGFILTER_TIMER, 1000, nullptr);
            return;
        }
        CWnd* focusWnd                 = GetFocus();
        bool  bSetFocusToFilterControl = ((focusWnd != GetDlgItem(IDC_DATEFROM)) &&
                                         (focusWnd != GetDlgItem(IDC_DATETO)) &&
                                         (focusWnd != GetDlgItem(IDC_LOGLIST)) &&
                                         (!m_bMonitoringMode || !m_sFilterText.IsEmpty()));
        if (m_sFilterText.IsEmpty())
        {
            DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning) ||
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

        m_logList.SetItemCountEx(0);
        m_logList.SetItemCountEx(ShownCountWithStopped());
        m_logList.RedrawItems(0, ShownCountWithStopped());
        m_logList.SetRedraw(false);
        ResizeAllListCtrlCols(true);
        m_logList.SetRedraw(true);
        m_logList.Invalidate();
        if (m_logList.GetItemCount() == 1)
        {
            m_logList.SetSelectionMark(0);
            m_logList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
        }
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_SEARCHEDIT)->ShowWindow(SW_SHOW);
        if (bSetFocusToFilterControl)
            GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
        else if (m_bMonitoringMode)
            GetDlgItem(IDC_LOGLIST)->SetFocus();

        AutoRestoreSelection(m_sFilterText.IsEmpty());
    } // if (nIDEvent == LOGFILTER_TIMER)
    if (nIDEvent == MONITOR_TIMER)
        MonitorTimer();
    if (nIDEvent == MONITOR_POPUP_TIMER)
        MonitorPopupTimer();
    DialogEnableWindow(IDC_STATBUTTON, !(((m_bLogThreadRunning) || (m_logEntries.GetVisibleCount() == 0))));
    __super::OnTimer(nIDEvent);
}

void CLogDlg::OnDtnDatetimechangeDateto(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    CTime cTime;
    m_dateTo.GetTime(cTime);
    try
    {
        CTime time(cTime.GetYear(), cTime.GetMonth(), cTime.GetDay(), 23, 59, 59);
        if (time.GetTime() != m_tTo)
        {
            m_tTo = static_cast<DWORD>(time.GetTime());
            SetTimer(LOGFILTER_TIMER, 10, nullptr);
        }
    }
    catch (CAtlException)
    {
    }

    *pResult = 0;
}

void CLogDlg::OnDtnDatetimechangeDatefrom(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    CTime cTime;
    m_dateFrom.GetTime(cTime);
    try
    {
        CTime time(cTime.GetYear(), cTime.GetMonth(), cTime.GetDay(), 0, 0, 0);
        if (time.GetTime() != m_tFrom)
        {
            m_tFrom = static_cast<DWORD>(time.GetTime());
            SetTimer(LOGFILTER_TIMER, 10, nullptr);
        }
    }
    catch (CAtlException)
    {
    }

    *pResult = 0;
}

CTSVNPathList CLogDlg::GetChangedPathsAndMessageSketchFromSelectedRevisions(CString&              sMessageSketch,
                                                                            CLogChangedPathArray& currentChangedArray) const
{
    CTSVNPathList pathList;
    sMessageSketch.Empty();

    quick_hash_set<LogCache::index_t> pathIDsAdded;
    POSITION                          pos = m_logList.GetFirstSelectedItemPosition();
    if (pos != nullptr)
    {
        while (pos)
        {
            size_t nextpos = m_logList.GetNextSelectedItem(pos);
            if (nextpos >= m_logEntries.GetVisibleCount())
                continue;

            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(nextpos);
            if (pLogEntry)
            {
                CString sRevMsg;
                sRevMsg.FormatMessage(m_sMultiLogFormat, //L"r%1!ld!\n%2!s!\n---------------------\n",
                                      pLogEntry->GetRevision(),
                                      static_cast<LPCWSTR>(pLogEntry->GetShortMessageUTF16()));
                sMessageSketch += sRevMsg;
                const CLogChangedPathArray& cPathArray = pLogEntry->GetChangedPaths();
                for (size_t cpPathIndex = 0; cpPathIndex < cPathArray.GetCount(); ++cpPathIndex)
                {
                    const CLogChangedPath& cPath = cPathArray[cpPathIndex];

                    LogCache::index_t pathID = cPath.GetCachedPath().GetIndex();
                    if (pathIDsAdded.contains(pathID))
                        continue;

                    pathIDsAdded.insert(pathID);

                    if (((m_cShowPaths.GetState() & 0x0003) != BST_CHECKED) || cPath.IsRelevantForStartPath())
                    {
                        CTSVNPath path;
                        path.SetFromSVN(cPath.GetPath());

                        pathList.AddPath(path);
                        currentChangedArray.Add(cPath);
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

    m_logEntries.Sort(static_cast<CLogDataVector::SortColumn>(nSortColumn), bAscending);
}

void CLogDlg::SortAndFilter(svn_revnum_t revToKeep)
{
    SortByColumn(m_nSortColumn, m_bAscending);
    RecalculateShownList(revToKeep);
}

void CLogDlg::OnLvnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
{
    if ((m_bLogThreadRunning) || (m_logList.HasText()))
        return; //no sorting while the arrays are filled
    LPNMLISTVIEW pNMLV   = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    const int    nColumn = pNMLV->iSubItem;
    m_bAscending         = nColumn == m_nSortColumn ? !m_bAscending : TRUE;
    m_nSortColumn        = nColumn;
    SortAndFilter();
    SetSortArrow(&m_logList, m_nSortColumn, !!m_bAscending);

    // clear the selection states
    for (POSITION pos = m_logList.GetFirstSelectedItemPosition(); pos != nullptr;)
        m_logList.SetItemState(m_logList.GetNextSelectedItem(pos), 0, LVIS_SELECTED);

    m_logList.SetSelectionMark(-1);

    m_logList.Invalidate();
    UpdateLogInfoLabel();
    // the "next 100" button only makes sense if the log messages
    // are sorted by revision in descending order
    if ((m_nSortColumn) || (m_bAscending))
    {
        DialogEnableWindow(IDC_NEXTHUNDRED, false);
    }
    else
    {
        DialogEnableWindow(IDC_NEXTHUNDRED, true);
    }
    *pResult = 0;
}

void CLogDlg::SetSortArrow(CListCtrl* control, int nColumn, bool bAscending)
{
    if (control == nullptr)
        return;
    // set the sort arrow
    CHeaderCtrl* pHeader    = control->GetHeaderCtrl();
    HDITEM       headerItem = {0};
    headerItem.mask         = HDI_FORMAT;
    for (int i = 0; i < pHeader->GetItemCount(); ++i)
    {
        pHeader->GetItem(i, &headerItem);
        headerItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        pHeader->SetItem(i, &headerItem);
    }
    if (nColumn >= 0)
    {
        pHeader->GetItem(nColumn, &headerItem);
        headerItem.fmt |= (bAscending ? HDF_SORTUP : HDF_SORTDOWN);
        pHeader->SetItem(nColumn, &headerItem);
    }
}
void CLogDlg::OnLvnColumnclickChangedFileList(NMHDR* pNMHDR, LRESULT* pResult)
{
    if ((m_bLogThreadRunning) || (m_logList.HasText()))
        return; //no sorting while the arrays are filled

    LPNMLISTVIEW pNMLV   = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    const int    nColumn = pNMLV->iSubItem;

    // multi-rev selection shows paths only -> can only sort by column
    if ((m_currentChangedPathList.GetCount() > 0) && (nColumn > 0))
        return;

    CString  selPath;
    POSITION pos = m_changedFileListCtrl.GetFirstSelectedItemPosition();
    if (pos)
    {
        int posIndex = m_changedFileListCtrl.GetNextSelectedItem(pos);
        if (m_currentChangedArray.GetCount() > 0)
            selPath = m_currentChangedArray[posIndex].GetPath();
        else
            selPath = m_currentChangedPathList[posIndex].GetSVNPathString();
    }

    // clear the selection
    int iItem = -1;
    while ((iItem = m_changedFileListCtrl.GetNextItem(-1, LVNI_SELECTED)) >= 0)
        m_changedFileListCtrl.SetItemState(iItem, 0, LVIS_SELECTED);

    m_bAscendingPathList  = nColumn == m_nSortColumnPathList ? !m_bAscendingPathList : TRUE;
    m_nSortColumnPathList = nColumn;
    if (m_currentChangedArray.GetCount() > 0)
        m_currentChangedArray.Sort(m_nSortColumnPathList, m_bAscendingPathList);
    else
        m_currentChangedPathList.SortByPathname(!m_bAscendingPathList);

    if (!selPath.IsEmpty())
    {
        if (m_currentChangedArray.GetCount() > 0)
        {
            for (int i = 0; i < static_cast<int>(m_currentChangedArray.GetCount()); ++i)
            {
                if (selPath.Compare(m_currentChangedArray[i].GetPath()) == 0)
                {
                    m_changedFileListCtrl.SetSelectionMark(i);
                    m_changedFileListCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
                    m_changedFileListCtrl.EnsureVisible(i, FALSE);
                    break;
                }
            }
        }
        else
        {
            for (int i = 0; i < static_cast<int>(m_currentChangedPathList.GetCount()); ++i)
            {
                if (selPath.Compare(m_currentChangedPathList[i].GetSVNPathString()) == 0)
                {
                    m_changedFileListCtrl.SetSelectionMark(i);
                    m_changedFileListCtrl.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
                    m_changedFileListCtrl.EnsureVisible(i, FALSE);
                    break;
                }
            }
        }
    }

    SetSortArrow(&m_changedFileListCtrl, m_nSortColumnPathList, m_bAscendingPathList);
    m_changedFileListCtrl.Invalidate();
    *pResult = 0;
}

void CLogDlg::ResizeAllListCtrlCols(bool bOnlyVisible)
{
    CHeaderCtrl* pHdrCtrl = m_logList.GetHeaderCtrl();
    if (pHdrCtrl == nullptr)
        return;

    const int maxCol   = pHdrCtrl->GetItemCount() - 1;
    size_t    startRow = 0;
    size_t    endRow   = m_logList.GetItemCount();
    if (bOnlyVisible)
    {
        if (endRow > 0)
            startRow = m_logList.GetTopIndex();
        endRow = startRow + m_logList.GetCountPerPage();
    }
    for (int col = 0; col <= maxCol; col++)
    {
        wchar_t textBuf[MAX_PATH + 1] = {0};
        HDITEM  hdi                   = {0};
        hdi.mask                      = HDI_TEXT;
        hdi.pszText                   = textBuf;
        hdi.cchTextMax                = _countof(textBuf) - 1;
        pHdrCtrl->GetItem(col, &hdi);
        int cx = m_logList.GetStringWidth(textBuf) + CDPIAware::Instance().Scale(GetSafeHwnd(), 20); // 20 pixels for col separator and margin
        for (size_t index = startRow; index < endRow; ++index)
        {
            // get the width of the string and add 14 pixels for the column separator and margins
            int lineWidth = m_logList.GetStringWidth(m_logList.GetItemText(static_cast<int>(index), col)) + 14;
            if (index < m_logEntries.GetVisibleCount())
            {
                PLOGENTRYDATA pCurLogEntry = m_logEntries.GetVisible(index);
                if ((pCurLogEntry) && (pCurLogEntry->GetRevision() == m_wcRev))
                {
                    HFONT hFont = reinterpret_cast<HFONT>(m_logList.SendMessage(WM_GETFONT));
                    // set the bold font and ask for the string width again
                    m_logList.SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(m_wcRevFont.GetSafeHandle()), NULL);
                    lineWidth = m_logList.GetStringWidth(m_logList.GetItemText(static_cast<int>(index), col)) + 14;
                    // restore the system font
                    m_logList.SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(hFont), NULL);
                }
                if (pCurLogEntry && pCurLogEntry->GetUnread())
                {
                    HFONT hFont = reinterpret_cast<HFONT>(m_logList.SendMessage(WM_GETFONT));
                    // set the bold font and ask for the string width again
                    m_logList.SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(m_unreadFont.GetSafeHandle()), NULL);
                    lineWidth = m_logList.GetStringWidth(m_logList.GetItemText(static_cast<int>(index), col)) + 14;
                    // restore the system font
                    m_logList.SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(hFont), NULL);
                }
            }
            if (index == 0)
            {
                // add the image size
                CImageList* pImgList = m_logList.GetImageList(LVSIL_SMALL);
                if ((pImgList) && (pImgList->GetImageCount()))
                {
                    IMAGEINFO imgInfo{};
                    pImgList->GetImageInfo(0, &imgInfo);
                    lineWidth += (imgInfo.rcImage.right - imgInfo.rcImage.left);
                    lineWidth += 3; // 3 pixels between icon and text
                }
            }
            if (cx < lineWidth)
                cx = lineWidth;
        }
        // Adjust columns "Actions" containing icons
        if (col == 1)
        {
            const int nMinimumWidth = CDPIAware::Instance().Scale(GetSafeHwnd(), ICONITEMBORDER) + GetSystemMetrics(SM_CXSMICON) * 7;
            if (cx < nMinimumWidth)
            {
                cx = nMinimumWidth;
            }
        }
        if ((col == 0) && m_bSelect)
        {
            cx += GetSystemMetrics(SM_CXSMICON); // add space for the checkbox
        }
        // keep the bug id column small
        if ((col == 4) && (m_bShowBugtraqColumn))
        {
            if (cx > static_cast<int>(static_cast<DWORD>(m_regMaxBugIDColWidth)))
            {
                cx = static_cast<int>(static_cast<DWORD>(m_regMaxBugIDColWidth));
            }
        }

        m_logList.SetColumnWidth(col, cx);
    }
}

void CLogDlg::OnBnClickedHidepaths()
{
    FillLogMessageCtrl();
    m_changedFileListCtrl.Invalidate();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CLogDlg::OnLvnOdfinditemLoglist(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult                 = -1;
    LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);

    if (pFindInfo->lvfi.flags & LVFI_PARAM)
        return;
    if ((pFindInfo->iStart < 0) || (pFindInfo->iStart >= static_cast<int>(m_logEntries.GetVisibleCount())))
        return;
    if (pFindInfo->lvfi.psz == nullptr)
        return;

    CString sCmp = pFindInfo->lvfi.psz;
    if (DoFindItemLogList(pFindInfo, pFindInfo->iStart, m_logEntries.GetVisibleCount(),
                          sCmp, pResult))
    {
        return;
    }
    if (pFindInfo->lvfi.flags & LVFI_WRAP)
    {
        if (DoFindItemLogList(pFindInfo, 0, pFindInfo->iStart, sCmp, pResult))
            return;
    }
    *pResult = -1;
}

bool CLogDlg::DoFindItemLogList(LPNMLVFINDITEM pFindInfo, size_t startIndex,
                                size_t endIndex, const CString& whatToFind, LRESULT* pResult) const
{
    CString sRev;
    for (size_t i = startIndex; i < endIndex; ++i)
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(i);
        if (pLogEntry)
        {
            sRev.Format(L"%ld", pLogEntry->GetRevision());
            if (pFindInfo->lvfi.flags & LVFI_PARTIAL)
            {
                if (whatToFind.Compare(sRev.Left(whatToFind.GetLength())) == 0)
                {
                    *pResult = i;
                    return true;
                }
            }
            else
            {
                if (whatToFind.Compare(sRev) == 0)
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

    m_endRev = 0;

    // now, restart the query

    Refresh();
}

void CLogDlg::OnBnClickedIncludemerge()
{
    m_endRev = 0;

    m_limit = 0;
    Refresh();
}

void CLogDlg::UpdateLogInfoLabel()
{
    svn_revnum_t rev1         = 0;
    svn_revnum_t rev2         = 0;
    long         selectedRevs = 0;
    size_t       changedPaths = 0;
    if (m_logEntries.GetVisibleCount())
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(0);
        if (pLogEntry)
        {
            rev1         = pLogEntry->GetRevision();
            pLogEntry    = m_logEntries.GetVisible(m_logEntries.GetVisibleCount() - 1);
            rev2         = pLogEntry ? pLogEntry->GetRevision() : 0;
            selectedRevs = m_logList.GetSelectedCount();

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
    sTemp.FormatMessage(IDS_LOG_LOGINFOSTRING, m_logEntries.GetVisibleCount(), rev2, rev1, selectedRevs,
                        changedPaths);
    m_sLogInfo = sTemp;
    UpdateData(FALSE);
    GetDlgItem(IDC_LOGINFO)->Invalidate();
}

bool CLogDlg::VerifyContextMenuForRevisionsAllowed(int selIndex) const
{
    if (selIndex < 0)
        return false; // nothing selected, nothing to do with a context menu

    // if the user selected the info text telling about not all revisions shown due to
    // the "stop on copy/rename" option, we also don't show the context menu
    if ((m_bStrictStopped) && (selIndex == static_cast<int>(m_logEntries.GetVisibleCount())))
        return false;

    return true;
}

void CLogDlg::AdjustContextMenuAnchorPointIfKeyboardInvoked(CPoint& point, int selIndex,
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

bool CLogDlg::GetContextMenuInfoForRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    // calculate some information the context menu commands can use
    pCmi->pathURL       = GetURLFromPath(m_path);
    CString  relPathURL = pCmi->pathURL.Mid(m_sRepositoryRoot.GetLength());
    POSITION pos        = m_logList.GetFirstSelectedItemPosition();
    int      indexNext  = m_logList.GetNextSelectedItem(pos);
    if ((indexNext < 0) || (indexNext >= static_cast<int>(m_logEntries.GetVisibleCount())))
        return false;
    pCmi->selLogEntry = m_logEntries.GetVisible(indexNext);
    if (pCmi->selLogEntry == nullptr)
        return false;
    pCmi->revSelected = pCmi->selLogEntry->GetRevision();
    pCmi->revPrevious = static_cast<svn_revnum_t>(pCmi->revSelected) - 1;

    const CLogChangedPathArray& paths = pCmi->selLogEntry->GetChangedPaths();
    if (paths.GetCount() <= 2)
    {
        for (size_t i = 0; i < paths.GetCount(); ++i)
        {
            const CLogChangedPath& changedPath = paths[i];
            if (changedPath.GetCopyFromRev() && (changedPath.GetPath().Compare(relPathURL) == 0))
                pCmi->revPrevious = changedPath.GetCopyFromRev();
        }
    }

    if (pos)
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_logList.GetNextSelectedItem(pos));
        if (pLogEntry)
            pCmi->revSelected2 = pLogEntry->GetRevision();
    }
    pCmi->allFromTheSameAuthor = true;

    std::vector<svn_revnum_t> revisions;
    revisions.reserve(m_logEntries.GetVisibleCount());

    POSITION      pos2      = m_logList.GetFirstSelectedItemPosition();
    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_logList.GetNextSelectedItem(pos2));
    if (pLogEntry == nullptr)
        return false;
    revisions.push_back(pLogEntry->GetRevision());
    pCmi->selEntries.push_back(pLogEntry);

    const std::string& firstAuthor = pLogEntry->GetAuthor();
    while (pos2)
    {
        int index2 = m_logList.GetNextSelectedItem(pos2);
        if (index2 < static_cast<int>(m_logEntries.GetVisibleCount()))
        {
            pLogEntry = m_logEntries.GetVisible(index2);
            if (pLogEntry)
            {
                revisions.push_back(pLogEntry->GetRevision());
                pCmi->selEntries.push_back(pLogEntry);
                if (firstAuthor != pLogEntry->GetAuthor())
                    pCmi->allFromTheSameAuthor = false;
            }
        }
    }

    pCmi->revisionRanges.AddRevisions(revisions);
    pCmi->revLowest  = pCmi->revisionRanges.GetLowestRevision();
    pCmi->revHighest = pCmi->revisionRanges.GetHighestRevision();

    return true;
}

void CLogDlg::PopulateContextMenuForRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi, CIconMenu& popup, CIconMenu& clipSubMenu)
{
    if ((m_logList.GetSelectedCount() == 1) && (pCmi->selLogEntry->GetDepth() == 0))
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
            }
            popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
            popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
            popup.AppendMenuIcon(ID_BLAMEWITHPREVIOUS, IDS_LOG_POPUP_BLAMEWITHPREVIOUS, IDI_BLAME);
            popup.AppendMenu(MF_SEPARATOR, NULL);
        }
        if (!m_projectProperties.sWebViewerRev.IsEmpty())
        {
            popup.AppendMenuIcon(ID_VIEWREV, IDS_LOG_POPUP_VIEWREV);
        }
        if (!m_projectProperties.sWebViewerPathRev.IsEmpty())
        {
            popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);
        }
        if ((!m_projectProperties.sWebViewerPathRev.IsEmpty()) ||
            (!m_projectProperties.sWebViewerRev.IsEmpty()))
        {
            popup.AppendMenu(MF_SEPARATOR, NULL);
        }

        popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
        popup.AppendMenuIcon(ID_GETMERGELOGS, IDS_LOG_POPUP_GETMERGELOGS, IDI_LOG);
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
    else if (m_logList.GetSelectedCount() >= 2)
    {
        bool bAddSeparator = false;
        if (IsSelectionContinuous() || (m_logList.GetSelectedCount() == 2))
        {
            popup.AppendMenuIcon(ID_COMPARETWO, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
            popup.AppendMenuIcon(ID_GNUDIFF2, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
        }
        if (m_logList.GetSelectedCount() == 2)
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

    if ((!pCmi->selEntries.empty()) && (pCmi->allFromTheSameAuthor))
    {
        popup.AppendMenuIcon(ID_EDITAUTHOR, IDS_LOG_POPUP_EDITAUTHOR);
    }
    if (m_logList.GetSelectedCount() == 1)
    {
        popup.AppendMenuIcon(ID_EDITLOG, IDS_LOG_POPUP_EDITLOG);
        // "Show Revision Properties"
        popup.AppendMenuIcon(ID_REVPROPS, IDS_REPOBROWSE_SHOWREVPROP, IDI_PROPERTIES);
        popup.AppendMenu(MF_SEPARATOR, NULL);
    }
    if (m_logList.GetSelectedCount() != 0)
    {
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDFULL, IDS_LOG_POPUP_CLIPBOARD_FULL, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURLREV, IDS_LOG_POPUP_CLIPBOARD_URLREV, IDI_COPYCLIP);
        if (!m_projectProperties.sWebViewerRev.IsEmpty())
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURLVIEWERREV, IDS_LOG_POPUP_CLIPBOARD_URLVIEWERREV, IDI_COPYCLIP);
        if (!m_projectProperties.sWebViewerPathRev.IsEmpty())
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURLVIEWERPATHREV, IDS_LOG_POPUP_CLIPBOARD_URLVIEWERPATHREV, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURLTSVNSHOWCOMPARE, IDS_LOG_POPUP_CLIPBOARD_TSVNSHOWCOMPARE, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDFULLNOPATHS, IDS_LOG_POPUP_CLIPBOARD_FULLNOPATHS, IDI_COPYCLIP);
        if (GetSelectedBugIds().size() > 0)
        {
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDBUGID, IDS_LOG_POPUP_CLIPBOARD_BUGID, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDBUGURL, IDS_LOG_POPUP_CLIPBOARD_BUGURL, IDI_COPYCLIP);
        }
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDREVS, IDS_LOG_POPUP_CLIPBOARD_REVS, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDAUTHORS, IDS_LOG_POPUP_CLIPBOARD_AUTHORS, IDI_COPYCLIP);
        clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDMESSAGES, IDS_LOG_POPUP_CLIPBOARD_MSGS, IDI_COPYCLIP);

        CString temp;
        temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
        popup.InsertMenu(static_cast<UINT>(-1), MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(clipSubMenu.m_hMenu), temp);
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
    int selIndex = m_logList.GetSelectionMark();
    if (!VerifyContextMenuForRevisionsAllowed(selIndex))
        return;
    AdjustContextMenuAnchorPointIfKeyboardInvoked(point, selIndex, m_logList);

    m_nSearchIndex = selIndex;

    // grab extra revision info that the Execute methods will use
    std::shared_ptr<CContextMenuInfoForRevisions> pCmi(new CContextMenuInfoForRevisions());
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
                                       TPM_RIGHTBUTTON,
                                   point.x, point.y, this, nullptr);
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
        case ID_GETMERGELOGS:
            ExecuteGetMergeLogs(pCmi);
            break;
        case ID_EDITLOG:
            EditLogMessage(selIndex);
            break;
        case ID_EDITAUTHOR:
            EditAuthor(pCmi->selEntries);
            break;
        case ID_REVPROPS:
            ExecuteRevisionPropsMenuRevisions(pCmi);
            break;
        case ID_COPYCLIPBOARDFULL:
            CopySelectionToClipBoard(true);
            break;
        case ID_COPYCLIPBOARDURLREV:
            CopySelectionToClipBoardRev();
            break;
        case ID_COPYCLIPBOARDURLVIEWERREV:
            CopySelectionToClipBoardViewerRev();
            break;
        case ID_COPYCLIPBOARDURLVIEWERPATHREV:
            CopySelectionToClipBoardViewerPathRev();
            break;
        case ID_COPYCLIPBOARDURLTSVNSHOWCOMPARE:
            CopySelectionToClipBoardTsvnShowCompare();
            break;
        case ID_COPYCLIPBOARDBUGID:
            CopySelectionToClipBoardBugId();
            break;
        case ID_COPYCLIPBOARDBUGURL:
            CopySelectionToClipBoardBugUrl();
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
    CString repositoryRootUrl = GetRepositoryRoot(m_path);
    CString selectedUrl       = GetSUrl();
    int     slashPos          = selectedUrl.Find(L"/", repositoryRootUrl.GetLength() + 1);
    if (slashPos == -1)
        return selectedUrl;
    CString returnedString = selectedUrl.Left(slashPos);
    return returnedString;
}

void CLogDlg::ExecuteAddCodeCollaboratorReview()
{
    CString revisions = GetSpaceSeparatedSelectedRevisions();
    if (revisions.IsEmpty())
        return;
    CodeCollaboratorInfo codeCollaborator(revisions, GetUrlOfTrunk());
    if (!codeCollaborator.IsUserInfoSet() || (GetKeyState(VK_CONTROL) & 0x8000))
    {
        CodeCollaboratorSettingsDlg dlg(this);
        dlg.DoModal();
        return;
    }

    CAppUtils::LaunchApplication(codeCollaborator.GetCommandLine(), NULL, false);
}

CString CLogDlg::GetSpaceSeparatedSelectedRevisions() const
{
    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    CString  sRevisions;

    if (pos != nullptr)
    {
        CString sRevision;
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index >= static_cast<int>(m_logEntries.GetVisibleCount()))
                continue;
            PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(index);
            if (pLogEntry)
            {
                sRevision.Format(L"%ld ", pLogEntry->GetRevision());
                sRevisions += sRevision;
            }
        }
    }
    return sRevisions;
}

void CLogDlg::ExecuteGnuDiff1MenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    CString options;
    bool    prettyPrint = true;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        CDiffOptionsDlg dlg(this);
        if (dlg.DoModal() == IDOK)
        {
            options     = dlg.GetDiffOptionsString();
            prettyPrint = dlg.GetPrettyPrint();
        }
        else
            return;
    }
    if (PromptShown())
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            SVNDiff diff(this, this->m_hWnd, true);
            diff.SetHEADPeg(m_logRevision);
            diff.ShowUnifiedDiff(m_path, pCmi->revPrevious, m_path, pCmi->revSelected, SVNRev(), prettyPrint, options, false, false, false);
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowUnifiedDiff(m_hWnd, m_path, pCmi->revPrevious, m_path,
                                        pCmi->revSelected, SVNRev(), m_logRevision, prettyPrint, options, false, false, false, false);
}

void CLogDlg::ExecuteGnuDiff2MenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    SVNRev r1 = pCmi->revSelected;
    SVNRev r2 = pCmi->revSelected2;
    if (m_logList.GetSelectedCount() > 2)
    {
        r1 = pCmi->revHighest;
        r2 = pCmi->revLowest;
    }
    // use the previous revision of the lowest rev so the lowest rev
    // is included in the diff
    r2 = r2 - static_cast<svn_revnum_t>(1);
    CString options;
    bool    prettyPrint = true;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        CDiffOptionsDlg dlg(this);
        if (dlg.DoModal() == IDOK)
        {
            options     = dlg.GetDiffOptionsString();
            prettyPrint = dlg.GetPrettyPrint();
        }
        else
            return;
    }
    if (PromptShown())
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            SVNDiff diff(this, this->m_hWnd, true);
            diff.SetHEADPeg(m_logRevision);
            diff.ShowUnifiedDiff(m_path, r2, m_path, r1, SVNRev(), prettyPrint, options, false, false, false);
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowUnifiedDiff(m_hWnd, m_path, r2, m_path, r1, SVNRev(), m_logRevision, prettyPrint, options, false, false, false, false);
}

void CLogDlg::ExecuteRevertRevisionMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const
{
    // we need an URL to complete this command, so error out if we can't get an URL
    if (pCmi->pathURL.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetUIPathString());
        return; //exit
    }

    if (ConfirmRevert(m_path.GetUIPathString()))
    {
        CSVNProgressDlg dlg;
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetOptions(ProgOptIgnoreAncestry | ProgOptAllowMixedRev);
        dlg.SetPathList(CTSVNPathList(m_path));
        dlg.SetUrl(pCmi->pathURL);
        dlg.SetSecondUrl(pCmi->pathURL);
        pCmi->revisionRanges.AdjustForMerge(true);
        dlg.SetRevisionRanges(pCmi->revisionRanges);
        dlg.SetPegRevision(m_logRevision);
        dlg.DoModal();
    }
}

void CLogDlg::ExecuteMergeRevisionMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    // we need an URL to complete this command, so error out if we can't get an URL
    if (pCmi->pathURL.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetUIPathString());
        return; //exit
    }

    CString path         = m_path.GetWinPathString();
    bool    bGotSavePath = false;
    if ((m_logList.GetSelectedCount() == 1) && (!m_path.IsDirectory()))
    {
        bGotSavePath = CAppUtils::FileOpenSave(path, nullptr, IDS_LOG_MERGETO, IDS_COMMONFILEFILTER,
                                               true, m_path.IsUrl() ? CString() : m_path.GetDirectory().GetWinPathString(), GetSafeHwnd());
    }
    else
    {
        CBrowseFolder folderBrowser;
        folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_MERGETO)));
        bGotSavePath = (folderBrowser.Show(GetSafeHwnd(), path, path) == CBrowseFolder::OK);
    }
    if (bGotSavePath)
    {
        svn_revnum_t minRev    = 0;
        svn_revnum_t maxRev    = 0;
        bool         bSwitched = false;
        bool         bModified = false;
        bool         bSparse   = false;

        if (GetWCRevisionStatus(CTSVNPath(path), true, minRev, maxRev, bSwitched, bModified, bSparse))
        {
            if (bModified)
            {
                CString sTask1;
                sTask1.Format(IDS_MERGE_WCDIRTYASK_TASK1, static_cast<LPCWSTR>(path));
                CTaskDialog taskDlg(sTask1,
                                    CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION |
                                        TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK3)));
                taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_MERGE_WCDIRTYASK_TASK4)));
                taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskDlg.SetDefaultCommandControl(200);
                taskDlg.SetMainIcon(TD_WARNING_ICON);
                if (taskDlg.DoModal(m_hWnd) != 100)
                    return;
            }
        }
        CSVNProgressDlg dlg;
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetOptions(ProgOptIgnoreAncestry | ProgOptAllowMixedRev);
        dlg.SetPathList(CTSVNPathList(CTSVNPath(path)));
        dlg.SetUrl(pCmi->pathURL);
        dlg.SetSecondUrl(pCmi->pathURL);
        pCmi->revisionRanges.AdjustForMerge(false);
        dlg.SetRevisionRanges(pCmi->revisionRanges);
        dlg.SetPegRevision(m_logRevision);
        dlg.DoModal();
    }
}

void CLogDlg::ExecuteRevertToRevisionMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const
{
    // we need an URL to complete this command, so error out if we can't get an URL
    if (pCmi->pathURL.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetUIPathString());
        return; //exit
    }

    if (ConfirmRevert(m_path.GetWinPath(), true))
    {
        CSVNProgressDlg dlg;
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetOptions(ProgOptIgnoreAncestry | ProgOptAllowMixedRev);
        dlg.SetPathList(CTSVNPathList(m_path));
        dlg.SetUrl(pCmi->pathURL);
        dlg.SetSecondUrl(pCmi->pathURL);
        SVNRevRangeArray revarray;
        revarray.AddRevRange(SVNRev::REV_HEAD, pCmi->revSelected);
        dlg.SetRevisionRanges(revarray);
        dlg.SetPegRevision(m_logRevision);
        dlg.DoModal();
    }
}

void CLogDlg::ExecuteCopyMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    // we need an URL to complete this command, so error out if we can't get an URL
    if (pCmi->pathURL.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetUIPathString());
        return; //exit
    }

    CCopyDlg dlg;
    dlg.m_url     = pCmi->pathURL;
    dlg.m_path    = m_path;
    dlg.m_copyRev = pCmi->revSelected;
    if (dlg.DoModal() == IDOK)
    {
        CTSVNPath    url          = CTSVNPath(dlg.m_url);
        SVNRev       copyRev      = dlg.m_copyRev;
        CString      logmsg       = dlg.m_sLogMessage;
        SVNExternals exts         = dlg.GetExternalsToTag();
        bool         bMakeParents = !!dlg.m_bMakeParents;
        auto         f            = [=]() mutable {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            // should we show a progress dialog here? Copies are done really fast
            // and without much network traffic.
            if (!Copy(CTSVNPathList(CTSVNPath(pCmi->pathURL)), url, copyRev, m_startRev, logmsg, bMakeParents, bMakeParents, false, exts.NeedsTagging(), exts))
                ShowErrorDialog(m_hWnd);
            else
            {
                TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_SUCCESS), MAKEINTRESOURCE(IDS_LOG_COPY_SUCCESS), TDCBF_OK_BUTTON, TD_INFORMATION_ICON, nullptr);
            }
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteCompareWithWorkingCopyMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    //user clicked on the menu item "compare with working copy"
    if (PromptShown())
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            SVNDiff diff(this, m_hWnd, true);
            diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
            diff.SetHEADPeg(m_logRevision);
            diff.ShowCompare(m_path, SVNRev::REV_WC, m_path, pCmi->revSelected, SVNRev(), false, true, L"");
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, m_path, SVNRev::REV_WC, m_path,
                                    pCmi->revSelected, SVNRev(), m_logRevision,
                                    false, true, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
    }
}

void CLogDlg::ExecuteCompareTwoMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    SVNRev r1 = pCmi->revSelected;
    SVNRev r2 = pCmi->revSelected2;
    if (m_logList.GetSelectedCount() > 2)
    {
        r1 = pCmi->revHighest;
        r2 = pCmi->revLowest;
    }
    svn_node_kind_t nodeKind = svn_node_unknown;
    if (!m_path.IsUrl())
    {
        nodeKind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
    }
    //user clicked on the menu item "compare revisions"
    if (PromptShown())
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            SVNDiff diff(this, m_hWnd, true);
            diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
            diff.SetHEADPeg(m_logRevision);
            diff.ShowCompare(CTSVNPath(pCmi->pathURL), r2, CTSVNPath(pCmi->pathURL),
                             r1, SVNRev(), false, true, L"", false, false, nodeKind);
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
        CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pCmi->pathURL), r2, CTSVNPath(pCmi->pathURL), r1,
                                    SVNRev(), m_logRevision, false, true, L"", !!(GetAsyncKeyState(VK_SHIFT) & 0x8000),
                                    false, false, nodeKind);
}

void CLogDlg::ExecuteCompareWithPreviousMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    svn_node_kind_t nodeKind = svn_node_unknown;
    if (!m_path.IsUrl())
    {
        nodeKind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
    }
    if (PromptShown())
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            SVNDiff diff(this, m_hWnd, true);
            diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
            diff.SetHEADPeg(m_logRevision);
            diff.ShowCompare(CTSVNPath(pCmi->pathURL), pCmi->revPrevious,
                             CTSVNPath(pCmi->pathURL), pCmi->revSelected, SVNRev(),
                             false, true, L"", false, false, nodeKind);
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pCmi->pathURL),
                                    pCmi->revPrevious, CTSVNPath(pCmi->pathURL), pCmi->revSelected,
                                    SVNRev(), m_logRevision, false, true, L"",
                                    !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), false, false, nodeKind);
    }
}

void CLogDlg::ExecuteBlameCompareMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    //user clicked on the menu item "compare with working copy"
    //now first get the revision which is selected
    if (PromptShown())
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            SVNDiff diff(this, this->m_hWnd, true);
            diff.SetHEADPeg(m_logRevision);
            diff.ShowCompare(m_path, SVNRev::REV_BASE, m_path, pCmi->revSelected, SVNRev(), false, true, L"", false, true);
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, m_path, SVNRev::REV_BASE, m_path,
                                    pCmi->revSelected, SVNRev(), m_logRevision, false, true, L"", false, false, true);
    }
}

void CLogDlg::ExecuteBlameTwoMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    //user clicked on the menu item "compare and blame revisions"
    svn_node_kind_t nodekind = svn_node_unknown;
    if (!m_path.IsUrl())
    {
        nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
    }
    if (PromptShown())
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            SVNDiff diff(this, this->m_hWnd, true);
            diff.SetHEADPeg(m_logRevision);
            diff.ShowCompare(CTSVNPath(pCmi->pathURL), pCmi->revSelected2,
                             CTSVNPath(pCmi->pathURL), pCmi->revSelected, SVNRev(),
                             false, true, L"", false, true, nodekind);
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pCmi->pathURL),
                                    pCmi->revSelected2, CTSVNPath(pCmi->pathURL), pCmi->revSelected,
                                    SVNRev(), m_logRevision, false, true, L"", false, false, true, nodekind);
    }
}

void CLogDlg::ExecuteWithPreviousMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    //user clicked on the menu item "Compare and Blame with previous revision"
    svn_node_kind_t nodekind = svn_node_unknown;
    if (!m_path.IsUrl())
    {
        nodekind = m_path.IsDirectory() ? svn_node_dir : svn_node_file;
    }
    if (PromptShown())
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());

            SVNDiff diff(this, this->m_hWnd, true);
            diff.SetHEADPeg(m_logRevision);
            diff.ShowCompare(CTSVNPath(pCmi->pathURL), pCmi->revPrevious,
                             CTSVNPath(pCmi->pathURL), pCmi->revSelected, SVNRev(),
                             false, true, L"", false, true, nodekind);
        };
        new async::CAsyncCall(f, &netScheduler);
        netScheduler.WaitForEmptyQueue();
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, CTSVNPath(pCmi->pathURL),
                                    pCmi->revPrevious, CTSVNPath(pCmi->pathURL), pCmi->revSelected,
                                    SVNRev(), m_logRevision, false, true, L"", false, false, true, nodekind);
    }
}

void CLogDlg::ExecuteSaveAsMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    //now first get the revision which is selected
    CString revFilename;
    CString strWinPath = CPathUtils::GetFileNameFromPath(pCmi->pathURL);
    strWinPath         = CPathUtils::PathUnescape(strWinPath);
    int rfind          = strWinPath.ReverseFind('.');
    if (rfind > 0)
        revFilename.Format(L"%s-%s%s", static_cast<LPCWSTR>(strWinPath.Left(rfind)),
                           static_cast<LPCWSTR>(pCmi->revSelected.ToString()), static_cast<LPCWSTR>(strWinPath.Mid(rfind)));
    else
        revFilename.Format(L"%s-%s", static_cast<LPCWSTR>(strWinPath), static_cast<LPCWSTR>(pCmi->revSelected.ToString()));
    if (CAppUtils::FileOpenSave(revFilename, nullptr, IDS_LOG_POPUP_SAVE, IDS_COMMONFILEFILTER, false, m_path.IsUrl() ? CString() : m_path.GetDirectory().GetWinPathString(), m_hWnd))
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            CTSVNPath tempfile;
            tempfile.SetFromWin(revFilename);
            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            CString sInfoLine;
            sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(),
                                    static_cast<LPCWSTR>(pCmi->revSelected.ToString()));
            progDlg.SetLine(1, sInfoLine, true);
            SetAndClearProgressInfo(&progDlg);
            progDlg.ShowModeless(m_hWnd);
            if (!Export(m_path, tempfile, SVNRev(SVNRev::REV_HEAD), pCmi->revSelected))
            {
                // try again with another peg revision
                if (!Export(m_path, tempfile, pCmi->revSelected, pCmi->revSelected))
                {
                    progDlg.Stop();
                    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                    ShowErrorDialog(m_hWnd);
                    EnableOKButton();
                }
            }
            progDlg.Stop();
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteOpenMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi, bool bOpenWith)
{
    auto f = [=]() {
        CoInitialize(nullptr);
        OnOutOfScope(CoUninitialize());
        this->EnableWindow(FALSE);
        OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

        CProgressDlg progDlg;
        progDlg.SetTitle(IDS_APPNAME);
        CString sInfoLine;
        sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(),
                                static_cast<LPCWSTR>(pCmi->revSelected.ToString()));
        progDlg.SetLine(1, sInfoLine, true);
        SetAndClearProgressInfo(&progDlg);
        progDlg.ShowModeless(m_hWnd);
        CTSVNPath tempFile = CTempFiles::Instance().GetTempFilePath(false, m_path, pCmi->revSelected);
        bool      bSuccess = true;
        if (!Export(m_path, tempFile, SVNRev(SVNRev::REV_HEAD), pCmi->revSelected))
        {
            bSuccess = false;
            // try again, but with the selected revision as the peg revision
            if (!Export(m_path, tempFile, pCmi->revSelected, pCmi->revSelected))
            {
                progDlg.Stop();
                SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                ShowErrorDialog(m_hWnd);
                EnableOKButton();
            }
            else
                bSuccess = true;
        }
        if (bSuccess)
        {
            progDlg.Stop();
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
            DoOpenFileWith(true, bOpenWith, tempFile);
        }
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteBlameMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    CBlameDlg dlg;
    dlg.m_endRev = pCmi->revSelected;
    dlg.m_pegRev = m_pegRev;
    if (dlg.DoModal() == IDOK)
    {
        SVNRev  startRev     = dlg.m_startRev;
        SVNRev  endRev       = dlg.m_endRev;
        bool    includeMerge = !!dlg.m_bIncludeMerge;
        bool    textViewer   = !!dlg.m_bTextView;
        CString options      = SVN::GetOptionsString(!!dlg.m_bIgnoreEOL, dlg.m_ignoreSpaces);

        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

            CBlame  blame;
            CString tempFile = blame.BlameToTempFile(m_path, startRev, endRev, m_pegRev,
                                                     options, includeMerge, TRUE, TRUE);
            if (!tempFile.IsEmpty())
            {
                if (textViewer)
                {
                    //open the default text editor for the result file
                    CAppUtils::StartTextViewer(tempFile);
                }
                else
                {
                    CString sParams = L"/path:\"" + m_path.GetSVNPathString() + L"\" ";
                    CAppUtils::LaunchTortoiseBlame(tempFile,
                                                   CPathUtils::GetFileNameFromPath(m_path.GetFileOrDirectoryName()),
                                                   sParams,
                                                   startRev,
                                                   endRev,
                                                   m_pegRev);
                }
            }
            else
            {
                blame.ShowErrorDialog(m_hWnd);
            }
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteUpdateMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const
{
    CString sCmd;
    sCmd.Format(L"/command:update /path:\"%s\" /rev:%ld",
                static_cast<LPCWSTR>(m_path.GetWinPath()), static_cast<LONG>(pCmi->revSelected));
    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteFindEntryMenuRevisions()
{
    m_nSearchIndex = m_logList.GetSelectionMark();
    if (m_nSearchIndex < 0)
        m_nSearchIndex = 0;
    CreateFindDialog();
}

void CLogDlg::ExecuteRepoBrowseMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const
{
    CString sCmd;
    sCmd.Format(L"/command:repobrowser /path:\"%s\" /rev:%s /pegrev:%s",
                static_cast<LPCWSTR>(pCmi->pathURL), static_cast<LPCWSTR>(pCmi->revSelected.ToString()), static_cast<LPCWSTR>(m_startRev.ToString()));

    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteRevisionPropsMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    CEditPropertiesDlg dlg;
    dlg.SetProjectProperties(&m_projectProperties);
    dlg.SetPathList(CTSVNPathList(CTSVNPath(pCmi->pathURL)));
    dlg.SetRevision(pCmi->revSelected);
    dlg.RevProps(true);
    dlg.DoModal();
}

void CLogDlg::ExecuteExportMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    CString sCmd;
    sCmd.Format(L"/command:export /path:\"%s\" /revision:%ld",
                static_cast<LPCWSTR>(pCmi->pathURL), static_cast<LONG>(pCmi->revSelected));
    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteCheckoutMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    CString sCmd;
    CString url = L"tsvn:" + pCmi->pathURL;
    sCmd.Format(L"/command:checkout /url:\"%s\" /revision:%ld",
                static_cast<LPCWSTR>(url), static_cast<LONG>(pCmi->revSelected));
    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteViewRevMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const
{
    CString url = m_projectProperties.sWebViewerRev;
    url         = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
    url.Replace(L"%REVISION%", pCmi->revSelected.ToString());
    if (!url.IsEmpty())
        ShellExecute(this->m_hWnd, L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
}

void CLogDlg::ExecuteViewPathRevMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    CString relurl = pCmi->pathURL;
    CString sRoot  = GetRepositoryRoot(CTSVNPath(relurl));
    relurl         = relurl.Mid(sRoot.GetLength());
    CString url    = m_projectProperties.sWebViewerPathRev;
    url            = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
    url.Replace(L"%REVISION%", pCmi->revSelected.ToString());
    url.Replace(L"%PATH%", relurl);
    if (!url.IsEmpty())
        ShellExecute(this->m_hWnd, L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
}

void CLogDlg::ExecuteGetMergeLogs(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    OnOutOfScope(EnableOKButton());
    m_bCancelled        = false;
    svn_revnum_t logRev = pCmi->revSelected;
    CString      sCmd;

    sCmd.Format(L"/command:log /path:\"%s\" /startrev:%ld /endrev:%ld /merge",
                static_cast<LPCWSTR>(pCmi->pathURL), logRev, logRev);

    if (m_pegRev.IsValid())
        sCmd.AppendFormat(L" /pegrev:%s", static_cast<LPCWSTR>(m_pegRev.ToString()));

    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ShowContextMenuForChangedPaths(CWnd* /*pWnd*/, CPoint point)
{
    m_bCancelled     = false;
    INT_PTR selIndex = m_changedFileListCtrl.GetSelectionMark();
    AdjustContextMenuAnchorPointIfKeyboardInvoked(point, static_cast<int>(selIndex), m_changedFileListCtrl);

    if (!VerifyContextMenuForChangedPathsAllowed(selIndex))
        return;

    // grab extra revision info that the Execute methods will use
    ContextMenuInfoForChangedPathsPtr pCmi(new CContextMenuInfoForChangedPaths());
    if (!GetContextMenuInfoForChangedPaths(pCmi))
        return;

    // need this shortcut that can't easily go into the helper class
    const CLogChangedPath& changedLogPath = m_currentChangedArray[selIndex];

    //entry is selected, now show the popup menu
    CIconMenu popup;
    CIconMenu clipSubMenu;
    if (!clipSubMenu.CreatePopupMenu())
        return;

    PopulateContextMenuForChangedPaths(pCmi, popup, clipSubMenu);
    int              cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY |
                                       TPM_RIGHTBUTTON,
                                   point.x, point.y, this, nullptr);
    CLogWndHourglass wait;

    switch (cmd)
    {
        case ID_COMPARE:
            ExecuteCompareChangedPaths(pCmi, selIndex);
            break;
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
            ExecuteRevertChangedPaths(pCmi, changedLogPath);
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
                OpenSelectedWcFilesWithVisualStudio(pCmi->changedLogPathIndices);
            else
                OpenSelectedWcFilesWithRegistedProgram(pCmi->changedLogPathIndices);
            break;
        case ID_BLAME:
            ExecuteBlameChangedPaths(pCmi, changedLogPath);
            break;
        case ID_GETMERGELOGS:
            ExecuteShowMergedLogs(pCmi);
            break;
        case ID_LOG:
            ExecuteShowLogChangedPaths(pCmi, changedLogPath, false);
            break;
        case ID_REPOBROWSE:
            ExecuteBrowseRepositoryChangedPaths(pCmi, changedLogPath);
            break;
        case ID_VIEWPATHREV:
            ExecuteViewPathRevisionChangedPaths(selIndex);
            break;
        case ID_COPYCLIPBOARDURL:
        case ID_COPYCLIPBOARDURLREV:
        case ID_COPYCLIPBOARDURLVIEWERREV:
        case ID_COPYCLIPBOARDURLVIEWERPATHREV:
        case ID_COPYCLIPBOARDURLTSVNSHOWCOMPARE:
        case ID_COPYCLIPBOARDRELPATH:
        case ID_COPYCLIPBOARDFILENAMES:
            CopyChangedPathInfoToClipboard(pCmi, cmd);
            break;

        default:
            break;
    } // switch (cmd)
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CLogDlg::OnDtnDropdownDatefrom(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // the date control should not show the "today" button
    CMonthCalCtrl* pCtrl = m_dateFrom.GetMonthCalCtrl();
    if (pCtrl)
        pCtrl->ModifyStyle(0, MCS_NOTODAY);
    *pResult = 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CLogDlg::OnDtnDropdownDateto(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // the date control should not show the "today" button
    CMonthCalCtrl* pCtrl = m_dateTo.GetMonthCalCtrl();
    if (pCtrl)
        pCtrl->ModifyStyle(0, MCS_NOTODAY);
    *pResult = 0;
}

void CLogDlg::OnSize(UINT nType, int cx, int cy)
{
    __super::OnSize(nType, cx, cy);
    if ((m_logList) && (m_changedFileListCtrl) && (nType == 0) && (cx > 0) && (cy > 0))
    {
        // correct the splitter positions if they're out of bounds
        CRect rcTop;
        m_logList.GetWindowRect(rcTop);
        ScreenToClient(rcTop);

        CRect rcMiddle;
        GetDlgItem(IDC_MSGVIEW)->GetWindowRect(rcMiddle);
        ScreenToClient(rcMiddle);

        CRect rcBottom;
        m_changedFileListCtrl.GetWindowRect(rcBottom);
        ScreenToClient(rcBottom);

        CRect rcBottomLimit;
        GetDlgItem(IDC_LOGINFO)->GetWindowRect(rcBottomLimit);
        ScreenToClient(rcBottomLimit);

        auto minCtrlHeight = MIN_CTRL_HEIGHT;

        // the IDC_LOGINFO and the changed file list control
        // have a space of 3 dlg units between them (check in the dlg resource editor)
        CRect dlgUnitRect(0, 0, 3, 3);
        MapDialogRect(&dlgUnitRect);

        if ((rcTop.Height() < minCtrlHeight) ||
            (rcMiddle.Height() < minCtrlHeight) ||
            (rcBottom.Height() < minCtrlHeight) ||
            (rcBottom.bottom > rcBottomLimit.top - dlgUnitRect.bottom))
        {
            // controls sizes and splitters need adjusting
            RemoveMainAnchors();

            auto hdwp           = BeginDeferWindowPos(5);
            auto hdwpFlags      = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOZORDER;
            auto splitterHeight = MIN_SPLITTER_HEIGHT;

            if ((rcBottom.bottom > rcBottomLimit.top - dlgUnitRect.bottom) || (rcBottom.Height() < minCtrlHeight))
            {
                // the bottom of the changed files list control is
                // below the point it should get, so move it upwards.
                // or the control is too small and needs extending.
                rcBottom.bottom = rcBottomLimit.top + dlgUnitRect.bottom;
                rcBottom.top    = min(rcBottom.top, rcBottom.bottom - minCtrlHeight);
                DeferWindowPos(hdwp, m_changedFileListCtrl.GetSafeHwnd(), nullptr, rcBottom.left, rcBottom.top, rcBottom.Width(), rcBottom.Height(), hdwpFlags);
                if (rcBottom.top < rcMiddle.bottom + splitterHeight)
                {
                    // we also need to move splitter2 and rcMiddle.bottom upwards
                    CRect rcSplitter2;
                    m_wndSplitter2.GetWindowRect(rcSplitter2);
                    ScreenToClient(rcSplitter2);
                    rcSplitter2.top    = rcBottom.top - splitterHeight;
                    rcSplitter2.bottom = rcBottom.top;
                    DeferWindowPos(hdwp, m_wndSplitter2.GetSafeHwnd(), nullptr, rcSplitter2.left, rcSplitter2.top, rcSplitter2.Width(), rcSplitter2.Height(), hdwpFlags);
                    rcMiddle.bottom = rcSplitter2.top;
                    if (rcMiddle.Height() < minCtrlHeight)
                    {
                        // now the message view is too small, we have to
                        // move splitter1 upwards and resize the top view
                        CRect rcSplitter1;
                        m_wndSplitter1.GetWindowRect(rcSplitter1);
                        ScreenToClient(rcSplitter1);
                        rcMiddle.top       = min(rcMiddle.top, rcMiddle.bottom - minCtrlHeight);
                        rcSplitter1.top    = rcMiddle.top - splitterHeight;
                        rcSplitter1.bottom = rcMiddle.top;
                        DeferWindowPos(hdwp, m_wndSplitter1.GetSafeHwnd(), nullptr, rcSplitter1.left, rcSplitter1.top, rcSplitter1.Width(), rcSplitter1.Height(), hdwpFlags);
                        rcTop.bottom = rcSplitter1.top;
                        DeferWindowPos(hdwp, m_logList.GetSafeHwnd(), nullptr, rcTop.left, rcTop.top, rcTop.Width(), rcTop.Height(), hdwpFlags);
                    }
                    rcMiddle.top = min(rcMiddle.top, rcMiddle.bottom - minCtrlHeight);
                    DeferWindowPos(hdwp, GetDlgItem(IDC_MSGVIEW)->GetSafeHwnd(), nullptr, rcMiddle.left, rcMiddle.top, rcMiddle.Width(), rcMiddle.Height(), hdwpFlags);
                }
            }
            if (rcTop.Height() < minCtrlHeight)
            {
                // the log list view is too small. Extend its height down and move splitter1 down.
                rcTop.bottom = rcTop.top + minCtrlHeight;
                DeferWindowPos(hdwp, m_logList.GetSafeHwnd(), nullptr, rcTop.left, rcTop.top, rcTop.Width(), rcTop.Height(), hdwpFlags);
                CRect rcSplitter1;
                m_wndSplitter1.GetWindowRect(rcSplitter1);
                ScreenToClient(rcSplitter1);
                rcSplitter1.top    = rcTop.bottom;
                rcSplitter1.bottom = rcSplitter1.top + splitterHeight;
                DeferWindowPos(hdwp, m_wndSplitter1.GetSafeHwnd(), nullptr, rcSplitter1.left, rcSplitter1.top, rcSplitter1.Width(), rcSplitter1.Height(), hdwpFlags);
                // since splitter1 moves down, also adjust the message view
                rcMiddle.top = rcSplitter1.bottom;
                DeferWindowPos(hdwp, GetDlgItem(IDC_MSGVIEW)->GetSafeHwnd(), nullptr, rcMiddle.left, rcMiddle.top, rcMiddle.Width(), rcMiddle.Height(), hdwpFlags);
            }
            if (rcMiddle.Height() < minCtrlHeight)
            {
                // the message view is too small. Extend its height down and move splitter2 down;
                rcMiddle.bottom = rcMiddle.top + minCtrlHeight;
                DeferWindowPos(hdwp, GetDlgItem(IDC_MSGVIEW)->GetSafeHwnd(), nullptr, rcMiddle.left, rcMiddle.top, rcMiddle.Width(), rcMiddle.Height(), hdwpFlags);
                CRect rcSplitter2;
                m_wndSplitter2.GetWindowRect(rcSplitter2);
                ScreenToClient(rcSplitter2);
                rcSplitter2.top    = rcMiddle.bottom;
                rcSplitter2.bottom = rcSplitter2.top + splitterHeight;
                DeferWindowPos(hdwp, m_wndSplitter2.GetSafeHwnd(), nullptr, rcSplitter2.left, rcSplitter2.top, rcSplitter2.Width(), rcSplitter2.Height(), hdwpFlags);
                // since splitter2 moves down, also adjust the changed files list control
                rcBottom.top = rcSplitter2.bottom;
                DeferWindowPos(hdwp, m_changedFileListCtrl.GetSafeHwnd(), nullptr, rcBottom.left, rcBottom.top, rcBottom.Width(), rcBottom.Height(), hdwpFlags);
            }
            EndDeferWindowPos(hdwp);

            AddMainAnchors();
            ArrangeLayout();
        }

        m_wndSplitter1.SetRange(rcTop.top + MIN_CTRL_HEIGHT, rcBottom.bottom - (2 * MIN_CTRL_HEIGHT + MIN_SPLITTER_HEIGHT));
        m_wndSplitter2.SetRange(rcTop.top + (2 * MIN_CTRL_HEIGHT + MIN_SPLITTER_HEIGHT), rcBottom.bottom - MIN_CTRL_HEIGHT);
        m_wndSplitterLeft.SetRange(CDPIAware::Instance().Scale(GetSafeHwnd(), 80), rcTop.right - m_logListOrigRect.Width());

        m_logList.Invalidate();
        m_changedFileListCtrl.Invalidate();
        GetDlgItem(IDC_MSGVIEW)->Invalidate();
    }
}

void CLogDlg::OnRefresh()
{
    if (GetDlgItem(IDC_GETALL)->IsWindowEnabled())
    {
        m_limit = 0;
        Refresh(true);
    }
}

void CLogDlg::OnFind()
{
    CreateFindDialog();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CLogDlg::OnFocusFilter()
{
    GetDlgItem(IDC_SEARCHEDIT)->SetFocus();
}

void CLogDlg::OnEditCopy()
{
    if (GetFocus() == &m_changedFileListCtrl)
        CopyChangedSelectionToClipBoard();
    else
        CopySelectionToClipBoard();
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
    if ((pLVKeyDown->wVKey == 'A') && (GetKeyState(VK_CONTROL) < 0))
    {
        // Ctrl-A: select all visible revision
        SelectAllVisibleRevisions();
    }

    *pResult = 0;
}

void CLogDlg::OnLvnKeydownFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

    // Ctrl-A: select all files
    if ((pLVKeyDown->wVKey == 'A') && (GetKeyState(VK_CONTROL) < 0))
        m_changedFileListCtrl.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);

    *pResult = 0;
}

CString CLogDlg::GetToolTipText(int nItem, int nSubItem)
{
    if ((nSubItem == 1) && (m_logEntries.GetVisibleCount() > static_cast<size_t>(nItem)))
    {
        PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(nItem);
        if (pLogEntry == nullptr)
            return CString();

        // Draw the icon(s) into the compatible DC

        DWORD actions = pLogEntry->GetChangedPaths().GetActions();

        std::string actionText;
        if (actions & LOGACTIONS_MODIFIED)
        {
            actionText += CLogChangedPath::GetActionString(LOGACTIONS_MODIFIED);
        }

        if (actions & LOGACTIONS_ADDED)
        {
            if (!actionText.empty())
                actionText += "\r\n";
            actionText += CLogChangedPath::GetActionString(LOGACTIONS_ADDED);
        }

        if (actions & LOGACTIONS_DELETED)
        {
            if (!actionText.empty())
                actionText += "\r\n";
            actionText += CLogChangedPath::GetActionString(LOGACTIONS_DELETED);
        }

        if (actions & LOGACTIONS_REPLACED)
        {
            if (!actionText.empty())
                actionText += "\r\n";
            actionText += CLogChangedPath::GetActionString(LOGACTIONS_REPLACED);
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

        CString sToolTipText = CUnicodeUtils::GetUnicode(actionText.c_str());
        if ((pLogEntry->GetDepth()) || (m_mergedRevs.find(pLogEntry->GetRevision()) != m_mergedRevs.end()))
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
    if (m_pStoreSelection == nullptr)
        m_pStoreSelection = std::make_unique<CStoreSelection>(this);
    else
        m_pStoreSelection->AddSelections();
}

void CLogDlg::AutoRestoreSelection(bool bClear)
{
    if (m_pStoreSelection != nullptr)
    {
        if (bClear)
            m_pStoreSelection = nullptr;
        else
            m_pStoreSelection->RestoreSelection();
        FillLogMessageCtrl();
        UpdateLogInfoLabel();
        if (m_bSelect)
            EnableOKButton();
    }
}

CString CLogDlg::GetListviewHelpString(HWND hControl, int index)
{
    CString sHelpText;
    if (hControl == m_logList.GetSafeHwnd())
    {
        if (m_logEntries.GetVisibleCount() > static_cast<size_t>(index))
        {
            PLOGENTRYDATA data = m_logEntries.GetVisible(index);
            if (data)
            {
                if ((data->GetDepth()) || (m_mergedRevs.find(data->GetRevision()) == m_mergedRevs.end()))
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
    else if (hControl == m_changedFileListCtrl.GetSafeHwnd())
    {
        if (static_cast<int>(m_currentChangedArray.GetCount()) > index)
        {
            svn_tristate_t textModifies  = m_currentChangedArray[index].GetTextModifies();
            svn_tristate_t propsModifies = m_currentChangedArray[index].GetPropsModifies();
            if ((propsModifies == svn_tristate_true) && (textModifies != svn_tristate_true))
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
    AddAnchor(IDC_SEARCHEDIT, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_FROMLABEL, TOP_RIGHT);
    AddAnchor(IDC_DATEFROM, TOP_RIGHT);
    AddAnchor(IDC_TOLABEL, TOP_RIGHT);
    AddAnchor(IDC_DATETO, TOP_RIGHT);
    AddAnchor(IDC_LOGLIST, TOP_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_SPLITTERTOP, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_MSGVIEW, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_SPLITTERBOTTOM, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_LOGMSG, MIDDLE_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_LOGINFO, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SHOWPATHS, BOTTOM_LEFT);
    AddAnchor(IDC_CHECK_STOPONCOPY, BOTTOM_LEFT);
    AddAnchor(IDC_INCLUDEMERGE, BOTTOM_LEFT);
    AddAnchor(IDC_NEXTHUNDRED, BOTTOM_LEFT);
    AddAnchor(IDC_REFRESH, BOTTOM_LEFT);
    AddAnchor(IDC_GETALL, m_bMonitoringMode ? BOTTOM_RIGHT : BOTTOM_LEFT);
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
    RemoveAnchor(IDC_FROMLABEL);
    RemoveAnchor(IDC_DATEFROM);
    RemoveAnchor(IDC_TOLABEL);
    RemoveAnchor(IDC_DATETO);
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
    int nCmdShow = (m_selectedFilters & LOGFILTER_DATERANGE) ? SW_SHOW : SW_HIDE;
    GetDlgItem(IDC_FROMLABEL)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_DATEFROM)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_TOLABEL)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_DATETO)->ShowWindow(nCmdShow);

    RemoveAnchor(IDC_SEARCHEDIT);
    RemoveAnchor(IDC_FROMLABEL);
    RemoveAnchor(IDC_DATEFROM);
    RemoveAnchor(IDC_TOLABEL);
    RemoveAnchor(IDC_DATETO);
    if (m_selectedFilters & LOGFILTER_DATERANGE)
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
    ReportNoUrlOfFile(static_cast<LPCWSTR>(filepath));
}

void CLogDlg::ReportNoUrlOfFile(LPCWSTR filepath) const
{
    CString messageString;
    messageString.Format(IDS_ERR_NOURLOFFILE, filepath);
    ::MessageBox(this->m_hWnd, messageString, L"TortoiseSVN", MB_ICONERROR);
}

bool CLogDlg::ConfirmRevert(const CString& path, bool bToRev /*= false*/) const
{
    CString msg;
    if (bToRev)
        msg.Format(IDS_LOG_REVERT_CONFIRM_TASK6, static_cast<LPCWSTR>(path));
    else
        msg.Format(IDS_LOG_REVERT_CONFIRM_TASK1, static_cast<LPCWSTR>(path));
    CTaskDialog taskDlg(msg,
                        CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK2)),
                        L"TortoiseSVN",
                        0,
                        TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION |
                            TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK3)));
    taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK4)));
    taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskDlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_LOG_REVERT_CONFIRM_TASK5)));
    taskDlg.SetDefaultCommandControl(200);
    taskDlg.SetMainIcon(TD_INFORMATION_ICON);
    return (taskDlg.DoModal(m_hWnd) == 100);
}

// this to be called on a thread so we don't delay the startup of the dialog
// and we can take advantage of multiple cores...
void CLogDlg::DetectVisualStudioRunningThread()
{
    PROCESSENTRY32 entry;
    entry.dwSize                  = sizeof(PROCESSENTRY32);
    m_bVisualStudioRunningAtStart = false;

    CAutoGeneralHandle snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapShot, &entry))
    {
        while (Process32Next(snapShot, &entry))
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
// https://stackoverflow.com/questions/350323/open-a-file-in-visual-studio-at-a-specific-line-number
bool CLogDlg::OpenSelectedWcFilesWithVisualStudio(std::vector<size_t>& changedlogpathindices)
{
    CLSID   clsid{};
    HRESULT result = ::CLSIDFromProgID(L"VisualStudio.DTE", &clsid);
    if (FAILED(result))
        return false;

    // grab active pointer to VS
    CComPtr<IUnknown> pUnk;
    result = ::GetActiveObject(clsid, nullptr, &pUnk);
    if (FAILED(result))
        return false;

    // cast to our smart pointer type for EnvDTE
    CComPtr<EnvDTE::_DTE> pDte;
    pDte = pUnk;

    // get the item operations pointer we need
    CComPtr<EnvDTE::ItemOperations> pItemOperations;
    result = pDte->get_ItemOperations(&pItemOperations);
    if (FAILED(result))
        return false;

    // make sure we got a selection
    if (m_changedFileListCtrl.GetSelectedCount() <= 0)
        return false;

    // preparation
    DialogEnableWindow(IDOK, FALSE);

    // do the deed...
    OpenSelectedFilesInVisualStudio(changedlogpathindices, pItemOperations);

    // re-enable and end wait
    EnableOKButton();

    ActivateVisualStudioWindow(pDte);
    return true;
}

void CLogDlg::OpenSelectedWcFilesWithRegistedProgram(std::vector<size_t>& changedlogpathindices)
{
    int       openCount      = 0;
    const int MaxFilesToOpen = 20;

    // loop over all the selections
    for (size_t i = 0; i < changedlogpathindices.size(); ++i)
    {
        CString wcPath = GetWcPathFromUrl(m_currentChangedArray[changedlogpathindices[i]].GetPath());
        if (!PathFileExists(static_cast<LPCWSTR>(wcPath)))
            continue;
        OpenWorkingCopyFileWithRegisteredProgram(wcPath);
        if (++openCount >= MaxFilesToOpen)
            break;
    }
}

void CLogDlg::OpenSelectedFilesInVisualStudio(std::vector<size_t>&             changedlogpathindices,
                                              CComPtr<EnvDTE::ItemOperations>& pItemOperations)
{
    int       openCount      = 0;
    const int MaxFilesToOpen = 100;

    // loop over all the selections
    for (size_t i = 0; i < changedlogpathindices.size(); ++i)
    {
        CString wcPath = GetWcPathFromUrl(m_currentChangedArray[changedlogpathindices[i]].GetPath());
        if (!PathFileExists(static_cast<LPCWSTR>(wcPath)))
            continue;
        CString extension = PathFindExtension(static_cast<LPCWSTR>(wcPath));
        extension         = extension.MakeLower();
        // following extensions might make sense to review in VisualStudio
        if (extension == L".cpp" || extension == L".h" || extension == L".cs" ||
            extension == L".rc" || extension == L".resx" || extension == L".xaml" ||
            extension == L".js" || extension == L".html" || extension == L".htm" ||
            extension == L".asp" || extension == L".aspx" || extension == L".php" ||
            extension == L".css" || extension == L".xml")
        {
            // we arbitrarily limit the number of files per code review to 100
            if (++openCount >= MaxFilesToOpen)
                break;
            OpenOneFileInVisualStudio(wcPath, pItemOperations);
        }
    }
}

CString CLogDlg::GetWcPathFromUrl(CString url)
{
    CString fileUrl = GetRepositoryRoot(m_path) + url.Trim();
    // firstfile = (e.g.) http://mydomain.com/repos/trunk/folder/file1
    // sUrl = http://mydomain.com/repos/trunk/folder
    CString sUnescapedUrl = CPathUtils::PathUnescape(GetSUrl());
    // find out until which char the urls are identical
    int j = 0;
    while ((j < fileUrl.GetLength()) && (j < sUnescapedUrl.GetLength()) && (fileUrl[j] == sUnescapedUrl[j]))
    {
        j++;
    }
    int     leftCount = m_path.GetWinPathString().GetLength() - (sUnescapedUrl.GetLength() - j);
    CString wcPath    = m_path.GetWinPathString().Left(leftCount);
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

bool CLogDlg::OpenOneFileInVisualStudio(CString&                         filename,
                                        CComPtr<EnvDTE::ItemOperations>& pItemOperations)
{
    _bstr_t                 bstrKind(EnvDTE::vsViewKindTextView);
    CComPtr<EnvDTE::Window> pWindow;
    _bstr_t                 bstrFileName(filename);

    // ok, open one file in VS
    HRESULT result = pItemOperations->OpenFile(bstrFileName, bstrKind, &pWindow);
    if (FAILED(result))
        return false;
    return true;
}

// The run in VS won't work if the process owner for VS is different than the current user
// Also, if the same user runs VS as administrator, they run in "High Integrity" mode
// and we don't want to show the Open in Visual Studio menu item in either case.
bool CLogDlg::RunningInSameUserContextWithSameProcessIntegrity(DWORD pidVisualStudio)
{
    DWORD       tortoisePid            = GetCurrentProcessId();
    PTOKEN_USER pUserTokenTortoise     = GetUserTokenFromProcessId(tortoisePid);
    PTOKEN_USER pUserTokenVisualStudio = GetUserTokenFromProcessId(pidVisualStudio);
    BOOL        isSameOwner            = FALSE;
    if (pUserTokenTortoise != nullptr && pUserTokenVisualStudio != nullptr)
        isSameOwner = EqualSid((pUserTokenTortoise->User).Sid,
                               (pUserTokenVisualStudio->User).Sid);
    if (pUserTokenTortoise)
        LocalFree(pUserTokenTortoise);
    if (pUserTokenVisualStudio)
        LocalFree(pUserTokenVisualStudio);

    // check if the process integrity matches, problem if dissimilar
    bool vsHighIntegrity       = IsProcessRunningInHighIntegrity(pidVisualStudio);
    bool tortoiseHighIntegrity = IsProcessRunningInHighIntegrity(tortoisePid);
    bool integrityMatches      = !(vsHighIntegrity ^ tortoiseHighIntegrity);
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
        if (!GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwLengthNeeded))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                PTOKEN_USER pUserToken = static_cast<PTOKEN_USER>(LocalAlloc(0, dwLengthNeeded));
                if (pUserToken != nullptr)
                {
                    if (GetTokenInformation(hToken, TokenUser,
                                            pUserToken, dwLengthNeeded, &dwLengthNeeded))
                    {
                        return pUserToken;
                    }
                    LocalFree(pUserToken);
                    return nullptr; // no token info
                }
                return nullptr; // LocalAlloc() failed
            }
            return nullptr; // GetLastError() returned a weird error
        }
        return nullptr; // no token info :-(
    }
    return nullptr; // can't get a process token
}

// adapted from http://msdn.microsoft.com/en-us/library/bb625966.aspx
bool CLogDlg::IsProcessRunningInHighIntegrity(DWORD pid)
{
    bool                   runningHighIntegrity = false;
    DWORD                  dwLengthNeeded       = 0;
    DWORD                  dwError              = ERROR_SUCCESS;
    PTOKEN_MANDATORY_LABEL pTIL                 = nullptr;
    CAutoGeneralHandle     hProcess             = OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
    CAutoGeneralHandle     hToken;

    // yuck2
    if (OpenProcessToken(hProcess, TOKEN_QUERY, hToken.GetPointer()))
    {
        // Get the Integrity level.
        if (!GetTokenInformation(hToken, TokenIntegrityLevel,
                                 nullptr, 0, &dwLengthNeeded))
        {
            dwError = GetLastError();
            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                pTIL = static_cast<PTOKEN_MANDATORY_LABEL>(LocalAlloc(0,
                                                                      dwLengthNeeded));
                if (pTIL != nullptr)
                {
                    if (GetTokenInformation(hToken, TokenIntegrityLevel,
                                            pTIL, dwLengthNeeded, &dwLengthNeeded))
                    {
                        DWORD dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
                                                                     static_cast<DWORD>(static_cast<UCHAR>(*GetSidSubAuthorityCount(pTIL->Label.Sid) - 1)));

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

void CLogDlg::ActivateVisualStudioWindow(CComPtr<EnvDTE::_DTE>& pDte)
{
    CComPtr<EnvDTE::Window> pMainWindow;
    HRESULT                 result = pDte->get_MainWindow(&pMainWindow);
    if (FAILED(result))
        return;
    long lhwnd = 0;
    pMainWindow->get_HWnd(&lhwnd);
    HWND hwnd = static_cast<HWND>(LongToHandle(lhwnd));
    if (IsWindow(hwnd))
        ::SetForegroundWindow(hwnd);
}

bool CLogDlg::VerifyContextMenuForChangedPathsAllowed(INT_PTR selIndex) const
{
    if (selIndex < 0)
        return false;
    int s = m_logList.GetSelectionMark();
    if (s < 0)
        return false;

    POSITION pos = m_logList.GetFirstSelectedItemPosition();
    if (pos == nullptr)
        return false; // nothing is selected, get out of here
    else
        return true;
}

bool CLogDlg::GetContextMenuInfoForChangedPaths(ContextMenuInfoForChangedPathsPtr& pCmi)
{
    POSITION pos      = m_logList.GetFirstSelectedItemPosition();
    INT_PTR  selIndex = m_changedFileListCtrl.GetSelectionMark();

    PLOGENTRYDATA pLogEntry = m_logEntries.GetVisible(m_logList.GetNextSelectedItem(pos));
    if (pLogEntry == nullptr)
        return false;
    pCmi->rev1   = pLogEntry->GetRevision();
    pCmi->rev2   = pCmi->rev1;
    pCmi->oneRev = true;
    if (pos)
    {
        while (pos)
        {
            int index = m_logList.GetNextSelectedItem(pos);
            if (index < static_cast<int>(m_logEntries.GetVisibleCount()))
            {
                pLogEntry = m_logEntries.GetVisible(index);
                if (pLogEntry)
                {
                    pCmi->rev1   = max(pCmi->rev1, static_cast<svn_revnum_t>(pLogEntry->GetRevision()));
                    pCmi->rev2   = min(pCmi->rev2, static_cast<svn_revnum_t>(pLogEntry->GetRevision()));
                    pCmi->oneRev = false;
                }
            }
        }
        if (!pCmi->oneRev)
            pCmi->rev2--;

        POSITION pos2 = m_changedFileListCtrl.GetFirstSelectedItemPosition();
        while (pos2)
        {
            int nItem = m_changedFileListCtrl.GetNextSelectedItem(pos2);
            pCmi->changedPaths.push_back(m_currentChangedPathList[nItem].GetSVNPathString());
            pCmi->changedLogPathIndices.push_back(static_cast<size_t>(nItem));
        }
    }
    else
    {
        // only one revision is selected in the log dialog top pane
        // but multiple items could be selected  in the changed items list
        pCmi->rev2 = pCmi->rev1 - 1;

        POSITION pos2 = m_changedFileListCtrl.GetFirstSelectedItemPosition();
        while (pos2)
        {
            const CLogChangedPathArray& paths = pLogEntry->GetChangedPaths();

            int nItem = m_changedFileListCtrl.GetNextSelectedItem(pos2);
            pCmi->changedLogPathIndices.push_back(static_cast<size_t>(nItem));
            if ((m_cShowPaths.GetState() & 0x0003) == BST_CHECKED)
            {
                // some items are hidden! So find out which item the user really clicked on
                INT_PTR selRealIndex = -1;
                for (INT_PTR hiddenIndex = 0; hiddenIndex < static_cast<INT_PTR>(paths.GetCount()); ++hiddenIndex)
                {
                    if (paths[hiddenIndex].IsRelevantForStartPath())
                        selRealIndex++;
                    if (selRealIndex == nItem)
                    {
                        nItem = static_cast<int>(hiddenIndex);
                        break;
                    }
                }
            }

            const CLogChangedPath& changedLogPath = paths[nItem];
            if (m_changedFileListCtrl.GetSelectedCount() == 1)
            {
                if (!changedLogPath.GetCopyFromPath().IsEmpty())
                    pCmi->rev2 = changedLogPath.GetCopyFromRev();
                else
                {
                    // if the path was modified but the parent path was 'added with history'
                    // then we have to use the copy from revision of the parent path
                    CTSVNPath cPath = CTSVNPath(changedLogPath.GetPath());
                    for (size_t fList = 0; fList < paths.GetCount(); ++fList)
                    {
                        CTSVNPath p = CTSVNPath(paths[fList].GetPath());
                        if (p.IsAncestorOf(cPath))
                        {
                            if (!paths[fList].GetCopyFromPath().IsEmpty())
                                pCmi->rev2 = paths[fList].GetCopyFromRev();
                        }
                    }
                }
            }

            pCmi->changedPaths.push_back(changedLogPath.GetPath());
        }
    }

    pCmi->sUrl = GetSUrl();

    // find the working copy path of the selected item from the URL
    CString sUrlRoot          = GetRepositoryRoot(m_path);
    CString sUrlRootUnescaped = CPathUtils::PathUnescape(sUrlRoot);
    if (!sUrlRoot.IsEmpty())
    {
        const CLogChangedPath& changedLogPath = m_currentChangedArray[selIndex];
        pCmi->fileUrl                         = changedLogPath.GetPath();
        pCmi->fileUrl                         = sUrlRootUnescaped + pCmi->fileUrl.Trim();
        if (m_hasWC)
        {
            // sUnescapedUrl = (e.g.) http://mydomain.com/repos/trunk/folder/file1
            // pCmi->sUrl    = (e.g.) http://mydomain.com/repos/trunk/folder
            auto                      wcRoot        = GetWCRootFromPath(m_path);
            auto                      wcRootUrl     = GetURLFromPath(wcRoot);
            CString                   sUnescapedUrl = CPathUtils::PathUnescape(wcRootUrl);
            std::wstring              url1          = static_cast<LPCWSTR>(sUnescapedUrl);
            std::wstring              url2          = static_cast<LPCWSTR>(pCmi->fileUrl);
            std::vector<std::wstring> vec1;
            std::vector<std::wstring> vec2;
            stringtok(vec1, url1, true, L"\\/");
            stringtok(vec2, url2, true, L"\\/");
            int i = 0;
            while (i < vec1.size() && i < vec2.size() && vec1[i] == vec2[i])
                ++i;

            pCmi->wcPath = wcRoot.GetWinPathString();
            while (i < vec2.size())
            {
                pCmi->wcPath += L"\\";
                pCmi->wcPath += vec2[i].c_str();
                ++i;
            }

            pCmi->wcPath.Replace('/', '\\');
            if (!PathFileExists(pCmi->wcPath))
                pCmi->wcPath.Empty();
        }
    }

    return true;
}

bool CLogDlg::PopulateContextMenuForChangedPaths(ContextMenuInfoForChangedPathsPtr& pCmi, CIconMenu& popup, CIconMenu& clipSubMenu) const
{
    INT_PTR selIndex = m_changedFileListCtrl.GetSelectionMark();

    if (popup.CreatePopupMenu())
    {
        bool bEntryAdded = false;
        if (m_changedFileListCtrl.GetSelectedCount() == 1)
        {
            if ((!pCmi->oneRev) || (IsDiffPossible(m_currentChangedArray[selIndex], pCmi->rev1)))
            {
                popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
                popup.AppendMenuIcon(ID_DIFF_CONTENTONLY, IDS_LOG_POPUP_DIFF_CONTENTONLY, IDI_DIFF);
                popup.AppendMenuIcon(ID_BLAMEDIFF, IDS_LOG_POPUP_BLAMEDIFF, IDI_BLAME);
                popup.SetDefaultItem(ID_DIFF, FALSE);
                popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
                if (m_hasWC && !pCmi->wcPath.IsEmpty())
                {
                    popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
                }
                bEntryAdded = true;
            }
            else if (pCmi->oneRev)
            {
                popup.AppendMenuIcon(ID_DIFF, IDS_LOG_POPUP_DIFF, IDI_DIFF);
                popup.AppendMenuIcon(ID_DIFF_CONTENTONLY, IDS_LOG_POPUP_DIFF_CONTENTONLY, IDI_DIFF);
                popup.SetDefaultItem(ID_DIFF, FALSE);
                bEntryAdded = true;
            }
            if ((pCmi->rev2 == pCmi->rev1 - 1) || (pCmi->changedPaths.size() == 1))
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
                if ((m_hasWC) && (pCmi->oneRev) && (!pCmi->wcPath.IsEmpty()))
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
                if (!m_projectProperties.sWebViewerPathRev.IsEmpty())
                {
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                    popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);
                }
                if (popup.GetDefaultItem(0, FALSE) == -1)
                    popup.SetDefaultItem(ID_OPEN, FALSE);
            }
        }
        else if (!pCmi->changedLogPathIndices.empty())
        {
            // more than one entry is selected
            popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE);
            bEntryAdded = true;
        }
        if (!pCmi->changedPaths.empty())
        {
            if (m_changedFileListCtrl.GetSelectedCount() > 1)
            {
                popup.AppendMenuIcon(ID_DIFF_MULTIPLE, IDS_LOG_POPUP_DIFF_MULTIPLE, IDI_DIFF);
                popup.AppendMenuIcon(ID_DIFF_MULTIPLE_CONTENTONLY, IDS_LOG_POPUP_DIFF_MULTIPLE_CONTENTONLY, IDI_DIFF);
                popup.SetDefaultItem(ID_DIFF_MULTIPLE, FALSE);
                popup.AppendMenuIcon(ID_OPENLOCAL_MULTIPLE, IDS_LOG_POPUP_OPENLOCAL_MULTIPLE, IDI_OPEN);
            }
            popup.AppendMenuIcon(ID_EXPORTTREE, IDS_MENUEXPORT, IDI_EXPORT);

            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURL, IDS_LOG_POPUP_CLIPBOARD_URL, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURLREV, IDS_LOG_POPUP_CLIPBOARD_URLREV, IDI_COPYCLIP);
            if (!m_projectProperties.sWebViewerPathRev.IsEmpty())
                clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURLVIEWERPATHREV, IDS_LOG_POPUP_CLIPBOARD_URLVIEWERPATHREV, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDURLTSVNSHOWCOMPARE, IDS_LOG_POPUP_CLIPBOARD_TSVNSHOWCOMPARE, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDRELPATH, IDS_LOG_POPUP_CLIPBOARD_RELPATH, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDFILENAMES, IDS_LOG_POPUP_CLIPBOARD_FILENAMES, IDI_COPYCLIP);

            CString temp;
            temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
            popup.InsertMenu(static_cast<UINT>(-1), MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(clipSubMenu.m_hMenu), temp);

            bEntryAdded = true;
        }

        if (!bEntryAdded)
            return false;
    }
    return true;
}

// borrowed from SVNStatusListCtrl -- extract??
bool CLogDlg::CheckMultipleDiffs(UINT selCount) const
{
    if (selCount > max(static_cast<DWORD>(3), static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\NumDiffWarning", 15))))
    {
        CString message;
        message.Format(CString(MAKEINTRESOURCE(IDS_STATUSLIST_WARN_MAXDIFF)), selCount);
        CTaskDialog taskDlg(message,
                            CString(MAKEINTRESOURCE(IDS_STATUSLIST_WARN_MAXDIFF_TASK2)),
                            L"TortoiseSVN",
                            0,
                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_STATUSLIST_WARN_MAXDIFF_TASK3)));
        taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_STATUSLIST_WARN_MAXDIFF_TASK4)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetDefaultCommandControl(200);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        bool doIt = (taskDlg.DoModal(m_hWnd) == 100);
        return doIt;
    }
    return true;
}

void CLogDlg::ExecuteMultipleDiffChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, bool ignoreprops)
{
    int nPaths = static_cast<int>(pCmi->changedLogPathIndices.size());

    // warn if we exceed Software\\TortoiseSVN\\NumDiffWarning or 15 if not set
    if (!CheckMultipleDiffs(nPaths))
        return;

    for (int i = 0; i < nPaths; ++i)
    {
        INT_PTR selIndex = static_cast<INT_PTR>(pCmi->changedLogPathIndices[i]);
        ExecuteDiffChangedPaths(pCmi, selIndex, ignoreprops);
    }
}

void CLogDlg::ExecuteCompareChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, INT_PTR selIndex)
{
    SVNRev getRev = m_currentChangedArray[selIndex].GetAction() == LOGACTIONS_DELETED ? pCmi->rev2 : pCmi->rev1;
    auto   f      = [=]() {
        CoInitialize(nullptr);
        OnOutOfScope(CoUninitialize());
        this->EnableWindow(FALSE);
        OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());

        DialogEnableWindow(IDOK, FALSE);
        SetPromptApp(&theApp);
        CTSVNPath tsvnfilepath = CTSVNPath(pCmi->fileUrl);

        CProgressDlg progDlg;
        progDlg.SetTitle(IDS_APPNAME);
        CString sInfoLine;
        sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, static_cast<LPCWSTR>(pCmi->fileUrl),
                                static_cast<LPCWSTR>(getRev.ToString()));
        progDlg.SetLine(1, sInfoLine, true);
        SetAndClearProgressInfo(&progDlg);
        progDlg.ShowModeless(m_hWnd);

        CTSVNPath tempFile = CTempFiles::Instance().GetTempFilePath(false, tsvnfilepath, getRev);
        m_bCancelled       = false;
        if (!Export(tsvnfilepath, tempFile, getRev, getRev))
        {
            progDlg.Stop();
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
            ShowErrorDialog(m_hWnd);
            EnableOKButton();
            return;
        }
        progDlg.Stop();
        SetAndClearProgressInfo(static_cast<HWND>(nullptr));

        CString sName1, sName2;
        sName1.Format(L"%s - Revision %ld", static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(pCmi->fileUrl)), static_cast<svn_revnum_t>(getRev));
        sName2.Format(IDS_DIFF_WCNAME, static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(pCmi->fileUrl)));
        CAppUtils::DiffFlags flags;
        flags.AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
        CString mimetype;
        CAppUtils::GetMimeType(tsvnfilepath, mimetype);

        CAppUtils::StartExtDiff(tempFile, CTSVNPath(pCmi->wcPath), sName1, sName2, tsvnfilepath, tsvnfilepath,
                                getRev, getRev, getRev, flags, 0,
                                CPathUtils::GetFileNameFromPath(pCmi->fileUrl), mimetype);
        EnableOKButton();
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteDiffChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, INT_PTR selIndex, bool ignoreprops)
{
    if ((!pCmi->oneRev) || IsDiffPossible(m_currentChangedArray[selIndex], pCmi->rev1))
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());
            DoDiffFromLog(selIndex, pCmi->rev1, pCmi->rev2, false, false, ignoreprops);
        };
        new async::CAsyncCall(f, &netScheduler);
    }
    else
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());
            if (pCmi->changedLogPathIndices.size() > 1)
                DoDiffFromLog(selIndex, pCmi->rev1, pCmi->rev1, false, false, ignoreprops);
            else
                DiffSelectedFile(ignoreprops);
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteBlameDiffChangedPaths(INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi)
{
    auto f = [=]() {
        CoInitialize(nullptr);
        OnOutOfScope(CoUninitialize());
        this->EnableWindow(FALSE);
        OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());
        DoDiffFromLog(selIndex, pCmi->rev1, pCmi->rev2, true, false, false);
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteGnuDiff1ChangedPaths(INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi)
{
    auto f = [=]() {
        CoInitialize(nullptr);
        OnOutOfScope(CoUninitialize());
        this->EnableWindow(FALSE);
        OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());
        DoDiffFromLog(selIndex, pCmi->rev1, pCmi->rev2, false, true, false);
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteRevertChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath)
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
        dlg.SetRevision(pCmi->rev2);
        dlg.SetPegRevision(pCmi->rev2);
    }
    else
    {
        if (!PathFileExists(pCmi->wcPath))
        {
            // seems the path got renamed
            // tell the user how to work around this.
            TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_LOG_REVERTREV_ERROR), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
            EnableOKButton();
            return; //exit
        }
        dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        dlg.SetOptions(ProgOptIgnoreAncestry | ProgOptAllowMixedRev);
        dlg.SetPathList(CTSVNPathList(CTSVNPath(pCmi->wcPath)));
        dlg.SetUrl(pCmi->fileUrl);
        dlg.SetSecondUrl(pCmi->fileUrl);
        SVNRevRangeArray revArray;
        revArray.AddRevRange(pCmi->rev1, pCmi->rev2);
        dlg.SetRevisionRanges(revArray);
    }
    if (ConfirmRevert(pCmi->wcPath))
    {
        dlg.DoModal();
    }
}

void CLogDlg::ExecuteShowPropertiesChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    OnOutOfScope(EnableOKButton());

    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        return;
    }
    CPropDlg dlg;
    dlg.m_rev  = pCmi->rev1;
    dlg.m_path = CTSVNPath(pCmi->fileUrl);
    dlg.DoModal();
}

void CLogDlg::ExecuteSaveAsChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, INT_PTR selIndex)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    OnOutOfScope(EnableOKButton());
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        return;
    }
    m_bCancelled              = false;
    CString sRoot             = GetRepositoryRoot(CTSVNPath(pCmi->fileUrl));
    CString sUrlRootUnescaped = CPathUtils::PathUnescape(sRoot);
    // if more than one entry is selected, we save them
    // one by one into a folder the user has selected
    bool      bTargetSelected = false;
    CTSVNPath targetPath;
    if (m_changedFileListCtrl.GetSelectedCount() > 1)
    {
        CBrowseFolder browseFolder;
        browseFolder.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_SAVEFOLDERTOHINT)));
        browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE |
                               BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
        CString strSaveAsDirectory;
        if (browseFolder.Show(GetSafeHwnd(), strSaveAsDirectory) == CBrowseFolder::OK)
        {
            targetPath      = CTSVNPath(strSaveAsDirectory);
            bTargetSelected = true;
        }
    }
    else
    {
        // Display the Open dialog box.
        CString revFilename;
        CString temp  = CPathUtils::GetFileNameFromPath(m_currentChangedArray[selIndex].GetPath());
        int     rFind = temp.ReverseFind('.');
        if (rFind > 0)
            revFilename.Format(L"%s-%ld%s", static_cast<LPCWSTR>(temp.Left(rFind)), pCmi->rev1,
                               static_cast<LPCWSTR>(temp.Mid(rFind)));
        else
            revFilename.Format(L"%s-%ld", static_cast<LPCWSTR>(temp), pCmi->rev1);
        bTargetSelected = CAppUtils::FileOpenSave(revFilename, nullptr, IDS_LOG_POPUP_SAVE,
                                                  IDS_COMMONFILEFILTER, false, m_path.IsUrl() ? CString() : m_path.GetDirectory().GetWinPathString(), m_hWnd);
        targetPath.SetFromWin(revFilename);
    }
    if (bTargetSelected)
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());
            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            for (size_t i = 0; i < pCmi->changedLogPathIndices.size(); ++i)
            {
                const CLogChangedPath& changedLogPathI = m_currentChangedArray[pCmi->changedLogPathIndices[i]];

                SVNRev getrev = (changedLogPathI.GetAction() == LOGACTIONS_DELETED) ? pCmi->rev2 : pCmi->rev1;

                CString sInfoLine;
                sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION,
                                        static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(changedLogPathI.GetPath())),
                                        static_cast<LPCWSTR>(getrev.ToString()));
                progDlg.SetLine(1, sInfoLine, true);
                SetAndClearProgressInfo(&progDlg);
                progDlg.ShowModeless(m_hWnd);

                CTSVNPath tempfile = targetPath;
                if (pCmi->changedPaths.size() > 1)
                {
                    // if multiple items are selected, then the TargetPath
                    // points to a folder and we have to append the filename
                    // to save to that folder.
                    CString sName    = changedLogPathI.GetPath();
                    int     slashPos = sName.ReverseFind('/');
                    if (slashPos >= 0)
                        sName = sName.Mid(slashPos);
                    tempfile.AppendPathString(sName);
                }
                CString filepath = sUrlRootUnescaped + changedLogPathI.GetPath();
                progDlg.SetLine(2, filepath, true);
                if (!Export(CTSVNPath(filepath), tempfile, getrev, getrev))
                {
                    progDlg.Stop();
                    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                    ShowErrorDialog(m_hWnd);
                    tempfile.Delete(false);
                    return;
                }
                progDlg.SetProgress(static_cast<DWORD>(i) + 1, static_cast<DWORD>(pCmi->changedLogPathIndices.size()));
            }
            progDlg.Stop();
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
        }; // end lambda definition
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteExportTreeChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    m_bCancelled = false;

    bool      bTargetSelected = false;
    CTSVNPath targetPath;
    if (m_changedFileListCtrl.GetSelectedCount() > 0)
    {
        CBrowseFolder browseFolder;
        browseFolder.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_SAVEFOLDERTOHINT)));
        browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
        CString strSaveAsDirectory;
        if (browseFolder.Show(GetSafeHwnd(), strSaveAsDirectory) == CBrowseFolder::OK)
        {
            targetPath      = CTSVNPath(strSaveAsDirectory);
            bTargetSelected = true;
        }
    }
    if (bTargetSelected)
    {
        auto f = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());
            CProgressDlg progDlg;
            progDlg.SetTitle(IDS_APPNAME);
            progDlg.SetTime(true);
            for (size_t i = 0; i < pCmi->changedLogPathIndices.size(); ++i)
            {
                if (m_currentChangedArray[pCmi->changedLogPathIndices[i]].GetAction() == LOGACTIONS_DELETED)
                    continue;
                if (m_currentChangedArray[pCmi->changedLogPathIndices[i]].GetNodeKind() == svn_node_dir)
                    continue;

                const CString& schangedlogpath = m_currentChangedArray[pCmi->changedLogPathIndices[i]].GetPath();

                SVNRev getRev = pCmi->rev1;

                CTSVNPath tempFile = targetPath;
                tempFile.AppendPathString(schangedlogpath);

                CString sInfoLine;
                sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION,
                                        static_cast<LPCWSTR>(schangedlogpath), static_cast<LPCWSTR>(getRev.ToString()));
                progDlg.SetLine(1, sInfoLine, true);
                progDlg.SetLine(2, tempFile.GetWinPath(), true);
                progDlg.SetProgress64(i, pCmi->changedLogPathIndices.size());
                progDlg.ShowModeless(m_hWnd);
                if (progDlg.HasUserCancelled())
                    break;

                SHCreateDirectoryEx(m_hWnd, tempFile.GetContainingDirectory().GetWinPath(), nullptr);
                CString filepath = CPathUtils::PathUnescape(m_sRepositoryRoot) + schangedlogpath;
                if (!Export(CTSVNPath(filepath), tempFile, getRev, getRev,
                            true, true, svn_depth_empty))
                {
                    progDlg.Stop();
                    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                    ShowErrorDialog(m_hWnd);
                    tempFile.Delete(false);
                    EnableOKButton();
                    break;
                }
            }
            progDlg.Stop();
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteOpenChangedPaths(INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi, bool bOpenWith)
{
    SVNRev getrev = m_currentChangedArray[selIndex].GetAction() == LOGACTIONS_DELETED ? pCmi->rev2 : pCmi->rev1;
    auto   f      = [=]() {
        CoInitialize(nullptr);
        OnOutOfScope(CoUninitialize());
        this->EnableWindow(FALSE);
        OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());
        Open(bOpenWith, pCmi->fileUrl, getrev);
    };
    new async::CAsyncCall(f, &netScheduler);
}

void CLogDlg::ExecuteBlameChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath)
{
    OnOutOfScope(EnableOKButton());
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        return;
    }
    CBlameDlg dlg;
    if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
        pCmi->rev1--;
    dlg.m_endRev = pCmi->rev1;
    if (dlg.DoModal() == IDOK)
    {
        SVNRev  startRev     = dlg.m_startRev;
        SVNRev  endRev       = dlg.m_endRev;
        SVNRev  pegRev       = pCmi->rev1;
        bool    includeMerge = !!dlg.m_bIncludeMerge;
        bool    textView     = !!dlg.m_bTextView;
        CString options      = SVN::GetOptionsString(!!dlg.m_bIgnoreEOL, dlg.m_ignoreSpaces);
        auto    f            = [=]() {
            CoInitialize(nullptr);
            OnOutOfScope(CoUninitialize());
            this->EnableWindow(FALSE);
            OnOutOfScope(this->EnableWindow(TRUE); this->SetFocus());
            CBlame  blame;
            CString tempFile = blame.BlameToTempFile(CTSVNPath(pCmi->fileUrl), startRev,
                                                     endRev, pegRev, options, includeMerge, TRUE, TRUE);
            if (!tempFile.IsEmpty())
            {
                if (textView)
                {
                    //open the default text editor for the result file
                    CAppUtils::StartTextViewer(tempFile);
                }
                else
                {
                    CString sParams = L"/path:\"" + pCmi->fileUrl + L"\" ";
                    CAppUtils::LaunchTortoiseBlame(tempFile,
                                                   CPathUtils::GetFileNameFromPath(pCmi->fileUrl),
                                                   sParams,
                                                   startRev,
                                                   endRev,
                                                   pegRev);
                }
            }
            else
            {
                blame.ShowErrorDialog(m_hWnd);
            }
        };
        new async::CAsyncCall(f, &netScheduler);
    }
}

void CLogDlg::ExecuteShowLogChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath, bool bMergeLog)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    OnOutOfScope(EnableOKButton());
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        return;
    }
    m_bCancelled        = false;
    svn_revnum_t logRev = pCmi->rev1;
    CString      sCmd;
    if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
    {
        // if the item got deleted in this revision,
        // fetch the log from the previous revision where it
        // still existed.
        sCmd.Format(L"/command:log /path:\"%s\" /startrev:%ld /pegrev:%ld",
                    static_cast<LPCWSTR>(pCmi->fileUrl), logRev - 1, logRev - 1);
    }
    else
    {
        sCmd.Format(L"/command:log /path:\"%s\" /pegrev:%ld",
                    static_cast<LPCWSTR>(pCmi->fileUrl), logRev);
    }
    if (m_hasWC)
    {
        CString sTmp;
        sTmp.Format(L" /propspath:\"%s\"", m_path.GetWinPath());
        sCmd += sTmp;
    }
    if (bMergeLog)
        sCmd += L" /merge";
    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteShowMergedLogs(ContextMenuInfoForChangedPathsPtr pCmi)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    OnOutOfScope(EnableOKButton());
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        return;
    }
    m_bCancelled        = false;
    svn_revnum_t logRev = pCmi->rev1;
    CString      sCmd;

    sCmd.Format(L"/command:log /path:\"%s\" /pegrev:%ld /startrev:%ld /endrev:%ld /merge",
                static_cast<LPCWSTR>(pCmi->fileUrl), logRev, logRev, logRev);

    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteBrowseRepositoryChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath)
{
    DialogEnableWindow(IDOK, FALSE);
    SetPromptApp(&theApp);
    OnOutOfScope(EnableOKButton());
    if (pCmi->sUrl.IsEmpty())
    {
        ReportNoUrlOfFile(m_path.GetWinPath());
        return;
    }
    m_bCancelled        = false;
    svn_revnum_t logRev = pCmi->rev1;
    CString      sCmd;
    if (changedlogpath.GetAction() == LOGACTIONS_DELETED)
    {
        sCmd.Format(L"/command:repobrowser /path:\"%s\" /rev:%ld",
                    static_cast<LPCWSTR>(pCmi->fileUrl), logRev - 1);
    }
    else
    {
        sCmd.Format(L"/command:repobrowser /path:\"%s\" /rev:%ld",
                    static_cast<LPCWSTR>(pCmi->fileUrl), logRev);
    }

    CAppUtils::RunTortoiseProc(sCmd);
}

void CLogDlg::ExecuteViewPathRevisionChangedPaths(INT_PTR selIndex) const
{
    PLOGENTRYDATA pLogEntry2 = m_logEntries.GetVisible(m_logList.GetSelectionMark());
    if (pLogEntry2)
    {
        SVNRev  rev    = pLogEntry2->GetRevision();
        CString relUrl = m_currentChangedArray[selIndex].GetPath();
        CString url    = m_projectProperties.sWebViewerPathRev;
        url            = CAppUtils::GetAbsoluteUrlFromRelativeUrl(m_sRepositoryRoot, url);
        url.Replace(L"%REVISION%", rev.ToString());
        url.Replace(L"%PATH%", relUrl);
        relUrl = relUrl.Mid(relUrl.Find('/'));
        url.Replace(L"%PATH1%", relUrl);
        if (!url.IsEmpty())
            ShellExecute(this->m_hWnd, L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
    }
}

void CLogDlg::CopyChangedPathInfoToClipboard(ContextMenuInfoForChangedPathsPtr pCmi, int cmd) const
{
    int nPaths = static_cast<int>(pCmi->changedLogPathIndices.size());

    CString sClipboard;
    for (int i = 0; i < nPaths; ++i)
    {
        INT_PTR selIndex = static_cast<INT_PTR>(pCmi->changedLogPathIndices[i]);

        CLogChangedPath path = m_currentChangedArray[selIndex];
        switch (cmd)
        {
            case ID_COPYCLIPBOARDURL:
                sClipboard += (m_sRepositoryRoot + path.GetPath());
                break;
            case ID_COPYCLIPBOARDURLREV:
                sClipboard += ((m_sRepositoryRoot + path.GetPath()) + L"/?r=" + SVNRev(pCmi->rev1).ToString());
                break;
            case ID_COPYCLIPBOARDURLVIEWERREV:
            {
                CString url = m_projectProperties.sWebViewerRev;
                url.Replace(L"%REVISION%", SVNRev(pCmi->rev1).ToString());
                if (!url.IsEmpty())
                    sClipboard += url;
            }
            break;
            case ID_COPYCLIPBOARDURLVIEWERPATHREV:
            {
                CString url = m_projectProperties.sWebViewerPathRev;
                url.Replace(L"%PATH%", path.GetPath());
                url.Replace(L"%REVISION%", SVNRev(pCmi->rev1).ToString());
                if (!url.IsEmpty())
                    sClipboard += url;
            }
            break;
            case ID_COPYCLIPBOARDURLTSVNSHOWCOMPARE:
            {
                CString url = L"tsvncmd:command:showcompare?url1:";
                url += m_sRepositoryRoot;
                url += path.GetPath();
                url += "?revision1:";
                url += SVNRev(pCmi->rev1).ToString();
                url += "?url2:";
                url += m_sRepositoryRoot;
                url += path.GetPath();
                url += "?revision2:";
                url += SVNRev(pCmi->rev2).ToString();
                sClipboard += url;
            }
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

LRESULT CLogDlg::OnRefreshSelection(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // it's enough to deselect, then select again one item of the whole selection
    int selMark = m_logList.GetSelectionMark();
    if (selMark >= 0)
    {
        m_logList.SetSelectionMark(selMark);
        m_logList.SetItemState(selMark, 0, LVIS_SELECTED);
        m_logList.SetItemState(selMark, LVIS_SELECTED, LVIS_SELECTED);
    }
    return 0;
}

bool CLogDlg::CreateToolbar()
{
    m_hwndToolbar = CreateWindowEx(TBSTYLE_EX_DOUBLEBUFFER,
                                   TOOLBARCLASSNAME,
                                   nullptr,
                                   WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                   0, 0, 0, 0,
                                   GetSafeHwnd(),
                                   nullptr,
                                   AfxGetResourceHandle(),
                                   nullptr);
    if (m_hwndToolbar == INVALID_HANDLE_VALUE)
        return false;

    ::SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, static_cast<WPARAM>(sizeof(TBBUTTON)), 0);

#define MONITORMODE_TOOLBARBUTTONCOUNT 11
    TBBUTTON tbb[MONITORMODE_TOOLBARBUTTONCOUNT] = {0};
    // create an image list containing the icons for the toolbar
    const int iconSizeX = static_cast<int>(24 * CDPIAware::Instance().ScaleFactor(GetSafeHwnd()));
    const int iconSizeY = static_cast<int>(24 * CDPIAware::Instance().ScaleFactor(GetSafeHwnd()));
    if (!m_toolbarImages.Create(iconSizeX, iconSizeY, ILC_COLOR32 | ILC_MASK | ILC_HIGHQUALITYSCALE, MONITORMODE_TOOLBARBUTTONCOUNT, 4))
        return false;
    auto  iString        = ::SendMessage(m_hwndToolbar, TB_ADDSTRING,
                                 reinterpret_cast<WPARAM>(AfxGetResourceHandle()), static_cast<LPARAM>(IDS_MONITOR_TOOLBARTEXTS));
    int   index          = 0;
    HICON hIcon          = CCommonAppUtils::LoadIconEx(IDI_MONITOR_GETALL, iconSizeX, iconSizeY);
    tbb[index].iBitmap   = m_toolbarImages.Add(hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_CHECKREPOSITORIESNOW;
    tbb[index].fsState   = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = iString++;

    tbb[index].iBitmap   = 0;
    tbb[index].idCommand = 0;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_SEP;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    hIcon                = CCommonAppUtils::LoadIconEx(IDI_MONITOR_ADD, iconSizeX, iconSizeY);
    tbb[index].iBitmap   = m_toolbarImages.Add(hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_ADDPROJECT;
    tbb[index].fsState   = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = iString++;

    hIcon                = CCommonAppUtils::LoadIconEx(IDI_MONITOR_EDIT, iconSizeX, iconSizeY);
    tbb[index].iBitmap   = m_toolbarImages.Add(hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_EDIT;
    tbb[index].fsState   = BTNS_SHOWTEXT;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = iString++;

    hIcon                = CCommonAppUtils::LoadIconEx(IDI_MONITOR_REMOVE, iconSizeX, iconSizeY);
    tbb[index].iBitmap   = m_toolbarImages.Add(hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_REMOVE;
    tbb[index].fsState   = BTNS_SHOWTEXT;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = iString++;

    tbb[index].iBitmap   = 0;
    tbb[index].idCommand = 0;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_SEP;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    hIcon                = CCommonAppUtils::LoadIconEx(IDI_MONITOR_MARKALLASREAD, iconSizeX, iconSizeY);
    tbb[index].iBitmap   = m_toolbarImages.Add(hIcon);
    tbb[index].idCommand = ID_MISC_MARKALLASREAD;
    tbb[index].fsState   = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = iString++;

    hIcon                = CCommonAppUtils::LoadIconEx(IDI_MONITOR_CLEARERRORS, iconSizeX, iconSizeY);
    tbb[index].iBitmap   = m_toolbarImages.Add(hIcon);
    tbb[index].idCommand = ID_LOGDLG_MONITOR_CLEARERRORS;
    tbb[index].fsState   = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = iString++;

    hIcon                = CCommonAppUtils::LoadIconEx(IDI_MONITOR_UPDATE, iconSizeX, iconSizeY);
    tbb[index].iBitmap   = m_toolbarImages.Add(hIcon);
    tbb[index].idCommand = ID_MISC_UPDATE;
    tbb[index].fsState   = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = iString++;

    tbb[index].iBitmap   = 0;
    tbb[index].idCommand = 0;
    tbb[index].fsState   = TBSTATE_ENABLED;
    tbb[index].fsStyle   = BTNS_SEP;
    tbb[index].dwData    = 0;
    tbb[index++].iString = 0;

    hIcon                = CCommonAppUtils::LoadIconEx(IDI_MONITOR_OPTIONS, iconSizeX, iconSizeY);
    tbb[index].iBitmap   = m_toolbarImages.Add(hIcon);
    tbb[index].idCommand = ID_MISC_OPTIONS;
    tbb[index].fsState   = TBSTATE_ENABLED | BTNS_SHOWTEXT;
    tbb[index].fsStyle   = BTNS_BUTTON;
    tbb[index].dwData    = 0;
    tbb[index++].iString = iString++;

    ::SendMessage(m_hwndToolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(m_toolbarImages.GetSafeHandle()));
    ::SendMessage(m_hwndToolbar, TB_ADDBUTTONS, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&tbb));
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
    CRect rcOk;
    GetDlgItem(IDOK)->GetWindowRect(&rcOk);
    ScreenToClient(&rcOk);
    CRect rcGetAll;
    GetDlgItem(IDC_GETALL)->GetWindowRect(&rcGetAll);
    ScreenToClient(&rcGetAll);

    ::SetWindowPos(m_hwndToolbar, nullptr, rcSearch.left, 0, rcDlg.Width(), rect.Height(), SWP_SHOWWINDOW);
    AddAnchor(m_hwndToolbar, TOP_LEFT, TOP_RIGHT);

    int delta = 90;
    GetDlgItem(IDC_PROJTREE)->SetWindowPos(nullptr, rcSearch.left, rcSearch.top + rect.Height(), delta, rcOk.bottom - rcSearch.top - rect.Height(), SWP_SHOWWINDOW);
    GetDlgItem(IDC_SPLITTERLEFT)->SetWindowPos(nullptr, rcSearch.left + delta, rcSearch.top + rect.Height(), 8, rcOk.bottom - rcSearch.top - rect.Height(), SWP_SHOWWINDOW);
    GetDlgItem(IDC_GETALL)->SetWindowPos(nullptr, rcOk.left - rcGetAll.Width() / 2, rcOk.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);

    delta += 8;
    auto hdwp = BeginDeferWindowPos(12);
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_SEARCHEDIT), delta, rect.Height(), 0, rect.Height());
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_FROMLABEL), 0, rect.Height(), 0, rect.Height());
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_DATEFROM), 0, rect.Height(), 0, rect.Height());
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_TOLABEL), 0, rect.Height(), 0, rect.Height());
    hdwp      = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_DATETO), 0, rect.Height(), 0, rect.Height());
    hdwp      = CSplitterControl::ChangeRect(hdwp, &m_logList, delta, rect.Height(), 0, 0);

    hdwp = CSplitterControl::ChangeRect(hdwp, &m_wndSplitter1, delta, 0, 0, 0);
    hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_MSGVIEW), delta, 0, 0, 0);
    hdwp = CSplitterControl::ChangeRect(hdwp, &m_wndSplitter2, delta, 0, 0, 0);
    hdwp = CSplitterControl::ChangeRect(hdwp, &m_changedFileListCtrl, delta, 0, 0, 75);
    hdwp = CSplitterControl::ChangeRect(hdwp, &m_logProgress, delta, 20, -rcGetAll.Width(), 20);
    hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_LOGINFO), delta, 75, 0, 75);
    EndDeferWindowPos(hdwp);

    DWORD exStyle = TVS_EX_AUTOHSCROLL | TVS_EX_DOUBLEBUFFER;
    m_projTree.SetExtendedStyle(exStyle, exStyle);
    if (!CTheme::Instance().IsDarkTheme())
        SetWindowTheme(m_projTree.GetSafeHwnd(), L"Explorer", nullptr);
    m_nMonitorUrlIcon = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_MONITORURL, 0, 0));
    m_nMonitorWCIcon  = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_MONITORWC, 0, 0));
    m_nErrorOvl       = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_MODIFIEDOVL, 0, 0));
    if (m_nErrorOvl >= 0)
        SYS_IMAGE_LIST().SetOverlayImage(m_nErrorOvl, OVERLAY_MODIFIED);
    m_projTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);
    m_projTree.SetDroppedMessage(WM_TSVN_MONITOR_TREEDROP);

    // Set up the tray icon
    ChangeWindowMessageFilter(WM_TASKBARCREATED, MSGFLT_ADD);

    m_hMonitorIconNormal     = CCommonAppUtils::LoadIconEx(IDI_MONITORNORMAL, 0, 0);
    m_hMonitorIconNewCommits = CCommonAppUtils::LoadIconEx(IDI_MONITORCOMMITS, 0, 0);

    m_systemTray.cbSize           = sizeof(NOTIFYICONDATA);
    m_systemTray.uVersion         = NOTIFYICON_VERSION_4;
    m_systemTray.uID              = 101;
    m_systemTray.hWnd             = GetSafeHwnd();
    m_systemTray.hIcon            = m_hMonitorIconNormal;
    m_systemTray.uFlags           = NIF_MESSAGE | NIF_ICON;
    m_systemTray.uCallbackMessage = WM_TSVN_MONITOR_TASKBARCALLBACK;
    if (Shell_NotifyIcon(NIM_ADD, &m_systemTray) == FALSE)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
        Shell_NotifyIcon(NIM_ADD, &m_systemTray);
    }

    // set up drop support
    m_pTreeDropTarget = new CMonitorTreeTarget(this);
    RegisterDragDrop(m_projTree.GetSafeHwnd(), m_pTreeDropTarget);
    // create the supported formats:
    FORMATETC ftetc = {0};
    ftetc.cfFormat  = CF_SVNURL;
    ftetc.dwAspect  = DVASPECT_CONTENT;
    ftetc.lindex    = -1;
    ftetc.tymed     = TYMED_HGLOBAL;
    m_pTreeDropTarget->AddSuportedFormat(ftetc);
    ftetc.cfFormat = CF_HDROP;
    m_pTreeDropTarget->AddSuportedFormat(ftetc);
    ftetc.cfFormat = CF_UNICODETEXT;
    m_pTreeDropTarget->AddSuportedFormat(ftetc);
    ftetc.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_DROPDESCRIPTION));
    m_pTreeDropTarget->AddSuportedFormat(ftetc);
    ftetc.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_SHELLURL));
    m_pTreeDropTarget->AddSuportedFormat(ftetc);
    ftetc.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_INETURL));
    m_pTreeDropTarget->AddSuportedFormat(ftetc);

    // fill the project tree
    InitMonitorProjTree();
    AssumeCacheEnabled(true);
    CRegDWORD reg = CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs", 100);
    m_limit       = static_cast<int>(static_cast<DWORD>(reg));

    RegisterSnarl();

    SetTimer(MONITOR_TIMER, 200, nullptr);
}

void CLogDlg::InitMonitorProjTree()
{
    CString sDataFilePath = CPathUtils::GetAppDataDirectory();
    sDataFilePath += L"\\MonitoringData.ini";
    m_monitoringFile.SetMultiLine();
    m_monitoringFile.SetUnicode();
    if (PathFileExists(sDataFilePath))
    {
        int                      retryCount = 5;
        SI_Error                 err        = SI_OK;
        CSimpleIni::TNamesDepend mItems;
        do
        {
            err = m_monitoringFile.LoadFile(sDataFilePath);
            if (err == SI_FILE)
            {
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error loading %s, retrycount %d\n", static_cast<LPCWSTR>(sDataFilePath), retryCount);
                Sleep(500);
            }
            else
                m_monitoringFile.GetAllSections(mItems);
        } while ((err == SI_FILE) && retryCount--);

        if ((err == SI_FILE) || mItems.empty())
        {
            // try again with the backup file
            sDataFilePath = CPathUtils::GetAppDataDirectory() + L"\\MonitoringData_backup.ini";
            if (PathFileExists(sDataFilePath))
            {
                retryCount = 5;
                err        = SI_OK;
                do
                {
                    err = m_monitoringFile.LoadFile(sDataFilePath);
                    if (err == SI_FILE)
                    {
                        CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error loading %s, retrycount %d\n", static_cast<LPCWSTR>(sDataFilePath), retryCount);
                        Sleep(500);
                    }
                } while ((err == SI_FILE) && retryCount--);
            }
            if (err == SI_FILE)
                TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_MONITORINILOAD), nullptr, TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
        }
    }
    m_bPlaySound             = _wtoi(m_monitoringFile.GetValue(L"global", L"PlaySound", L"1")) != 0;
    m_bShowNotification      = _wtoi(m_monitoringFile.GetValue(L"global", L"ShowNotifications", L"1")) != 0;
    m_defaultMonitorInterval = _wtoi(m_monitoringFile.GetValue(L"global", L"DefaultCheckInterval", L"30"));

    CRegDWORD regFirstStart(L"Software\\TortoiseSVN\\MonitorFirstStart", 0);
    if (static_cast<DWORD>(regFirstStart) == 0)
    {
        CRegString regStart(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run\\TortoiseSVN Monitor");
        regStart      = CPathUtils::GetAppPath() + L" /tray";
        regFirstStart = 1;
    }

    RefreshMonitorProjTree();
}

void CLogDlg::RefreshMonitorProjTree()
{
    static bool firstCall = true;

    CSimpleIni::TNamesDepend mItems;
    m_monitoringFile.GetAllSections(mItems);
    m_projTree.SetRedraw(FALSE);
    // remove all existing data from the control and free the memory
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        m_projTree.SetItemData(hItem, NULL);
        delete pItem;
        return false;
    });
    m_projTree.DeleteAllItems();
    int  itemcount         = 0;
    bool hasUnreadItems    = false;
    bool bHasWorkingCopies = false;
    for (const auto& mitem : mItems)
    {
        CString Name = m_monitoringFile.GetValue(mitem, L"Name", L"");
        if (!Name.IsEmpty())
        {
            MonitorItem* pMonitorItem        = new MonitorItem(Name);
            pMonitorItem->wcPathOrUrl        = m_monitoringFile.GetValue(mitem, L"wcPathOrUrl", L"");
            pMonitorItem->interval           = _wtoi(m_monitoringFile.GetValue(mitem, L"interval", L"30"));
            pMonitorItem->minMinutesInterval = _wtoi(m_monitoringFile.GetValue(mitem, L"minMinutesInterval", L"0"));
            pMonitorItem->lastChecked        = _wtoi64(m_monitoringFile.GetValue(mitem, L"lastChecked", L"0"));
            pMonitorItem->lastCheckedRobots  = _wtoi64(m_monitoringFile.GetValue(mitem, L"lastCheckedRobots", L"0"));
            pMonitorItem->lastHead           = _wtol(m_monitoringFile.GetValue(mitem, L"lastHead", L"0"));
            pMonitorItem->unreadItems        = _wtoi(m_monitoringFile.GetValue(mitem, L"unreadItems", L"0"));
            pMonitorItem->userName           = m_monitoringFile.GetValue(mitem, L"username", L"");
            pMonitorItem->password           = m_monitoringFile.GetValue(mitem, L"password", L"");
            pMonitorItem->unreadFirst        = _wtol(m_monitoringFile.GetValue(mitem, L"unreadFirst", L"0"));
            pMonitorItem->uuid               = m_monitoringFile.GetValue(mitem, L"uuid", L"");
            pMonitorItem->root               = m_monitoringFile.GetValue(mitem, L"root", L"");
            pMonitorItem->sMsgRegex          = m_monitoringFile.GetValue(mitem, L"MsgRegex", L"");
            pMonitorItem->projectProperties.LoadFromIni(m_monitoringFile, mitem);
            pMonitorItem->lastErrorMsg = m_monitoringFile.GetValue(mitem, L"lastErrorMsg", L"");
            pMonitorItem->parentPath   = _wtoi(m_monitoringFile.GetValue(mitem, L"parentPath", L"0"));
            pMonitorItem->authFailed   = _wtol(m_monitoringFile.GetValue(mitem, L"authFailed", L"0")) != 0;
            try
            {
                pMonitorItem->msgRegex = std::wregex(pMonitorItem->sMsgRegex, std::regex_constants::ECMAScript | std::regex_constants::icase);
            }
            catch (std::exception&)
            {
                pMonitorItem->msgRegex = std::wregex();
                pMonitorItem->sMsgRegex.Empty();
            }
            stringtok(pMonitorItem->authorsToIgnore, CUnicodeUtils::StdGetUTF8(m_monitoringFile.GetValue(mitem, L"ignoreauthors", L"")), true, " ,;");
            InsertMonitorItem(pMonitorItem, m_monitoringFile.GetValue(mitem, L"parentTreePath", L""));
            ++itemcount;
            if (pMonitorItem->unreadItems)
                hasUnreadItems = true;
            if (!CTSVNPath(pMonitorItem->wcPathOrUrl).IsUrl())
                bHasWorkingCopies = true;
        }
    }
    if (itemcount == 0)
        m_projTree.ShowText(CString(MAKEINTRESOURCE(IDS_MONITOR_ADDHINT)), true);
    else
    {
        m_projTree.ClearText();

        std::vector<HTREEITEM> folders;
        RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
            if (m_projTree.ItemHasChildren(hItem))
                folders.push_back(hItem);
            return false;
        });
        folders.push_back(TVI_ROOT);
        for (const auto& parent : folders)
        {
            TVSORTCB tvs;
            tvs.hParent     = parent;
            tvs.lpfnCompare = TreeSort;
            tvs.lParam      = reinterpret_cast<LPARAM>(this);

            m_projTree.SortChildrenCB(&tvs);
        }
        if (firstCall)
        {
            // successfully loaded the ini file, and it contains
            // at least one project: assume the ini file is not
            // corrupt.
            // Create a backup of the (assumed good) ini file now
            CString sAppDataDir = CPathUtils::GetAppDataDirectory();
            CopyFile(sAppDataDir + L"\\MonitoringData.ini", sAppDataDir + L"\\MonitoringData_backup.ini", FALSE);
        }
    }
    m_projTree.SetRedraw(TRUE);

    m_systemTray.hIcon  = hasUnreadItems ? m_hMonitorIconNewCommits : m_hMonitorIconNormal;
    m_systemTray.uFlags = NIF_ICON;

    if (Shell_NotifyIcon(NIM_MODIFY, &m_systemTray) == FALSE)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
        m_systemTray.uFlags = NIF_MESSAGE | NIF_ICON;
        Shell_NotifyIcon(NIM_ADD, &m_systemTray);
    }
    ::SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_MISC_UPDATE, MAKELONG(bHasWorkingCopies, 0));

    if (m_pTaskbarList)
    {
        m_pTaskbarList->SetOverlayIcon(GetSafeHwnd(), hasUnreadItems ? m_hMonitorIconNewCommits : m_hMonitorIconNormal, L"");
    }

    firstCall = false;
}

int CLogDlg::TreeSort(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParam3*/)
{
    MonitorItem* item1 = reinterpret_cast<MonitorItem*>(lParam1);
    MonitorItem* item2 = reinterpret_cast<MonitorItem*>(lParam2);

    return StrCmpLogicalW(item1->name, item2->name);
}

HTREEITEM CLogDlg::InsertMonitorItem(MonitorItem* pMonitorItem, const CString& sParentPath /*= CString()*/)
{
    bool           bUrl            = !!::PathIsURL(pMonitorItem->wcPathOrUrl);
    TVINSERTSTRUCT tvInsert        = {nullptr};
    tvInsert.hParent               = FindMonitorParent(sParentPath);
    tvInsert.hInsertAfter          = TVI_SORT;
    tvInsert.itemex.mask           = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_EXPANDEDIMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
    tvInsert.itemex.pszText        = LPSTR_TEXTCALLBACK;
    tvInsert.itemex.cChildren      = (pMonitorItem->wcPathOrUrl.IsEmpty() || pMonitorItem->parentPath) ? 1 : 0;
    tvInsert.itemex.state          = TVIS_EXPANDED | (pMonitorItem->unreadItems ? TVIS_BOLD : 0) | ((pMonitorItem->authFailed || !pMonitorItem->lastErrorMsg.IsEmpty()) ? INDEXTOOVERLAYMASK(OVERLAY_MODIFIED) : 0);
    tvInsert.itemex.stateMask      = TVIS_EXPANDED | TVIS_OVERLAYMASK | TVIS_BOLD;
    tvInsert.itemex.lParam         = reinterpret_cast<LPARAM>(pMonitorItem);
    tvInsert.itemex.iImage         = (pMonitorItem->wcPathOrUrl.IsEmpty() || pMonitorItem->parentPath) ? m_nIconFolder : bUrl ? m_nMonitorUrlIcon
                                                                                                                              : m_nMonitorWCIcon;
    tvInsert.itemex.iExpandedImage = (pMonitorItem->wcPathOrUrl.IsEmpty() || pMonitorItem->parentPath) ? m_nOpenIconFolder : bUrl ? m_nMonitorUrlIcon
                                                                                                                                  : m_nMonitorWCIcon;
    tvInsert.itemex.iSelectedImage = (pMonitorItem->wcPathOrUrl.IsEmpty() || pMonitorItem->parentPath) ? m_nIconFolder : bUrl ? m_nMonitorUrlIcon
                                                                                                                              : m_nMonitorWCIcon;

    // mark the parent as having children
    if (tvInsert.hParent && (tvInsert.hParent != TVI_ROOT))
    {
        TVITEM tvItem;
        tvItem.mask      = TVIF_CHILDREN | TVIF_HANDLE;
        tvItem.cChildren = 1;
        tvItem.hItem     = tvInsert.hParent;
        m_projTree.SetItem(&tvItem);
        m_projTree.Expand(tvInsert.hParent, TVE_EXPAND);
    }
    return m_projTree.InsertItem(&tvInsert);
}

HTREEITEM CLogDlg::RecurseMonitorTree(HTREEITEM hItem, MonitorItemHandler handler) const
{
    HTREEITEM hFound = hItem;
    while (hFound)
    {
        if (hFound == TVI_ROOT)
            hFound = m_projTree.GetNextItem(TVI_ROOT, TVGN_ROOT);
        if (hFound == nullptr)
            return nullptr;
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
    return nullptr;
}

HTREEITEM CLogDlg::FindMonitorParent(const CString& parentTreePath) const
{
    if (parentTreePath.IsEmpty())
        return TVI_ROOT;
    HTREEITEM hRetItem = TVI_ROOT;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        if (GetTreePath(hItem).Compare(parentTreePath) == 0)
        {
            hRetItem = hItem;
            return true;
        }
        return false;
    });
    return hRetItem;
}

CString CLogDlg::GetTreePath(HTREEITEM hItem) const
{
    CString      path;
    MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
    if (pItem)
    {
        path              = pItem->name;
        HTREEITEM hParent = m_projTree.GetParentItem(hItem);
        while (hParent)
        {
            pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hParent));
            if (pItem)
                path = pItem->name + L"\\" + path;
            hParent = m_projTree.GetParentItem(hParent);
        }
    }
    return path;
}

HTREEITEM CLogDlg::FindMonitorItem(const CString& wcpathorurl) const
{
    HTREEITEM hRetItem = nullptr;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        if (pItem->wcPathOrUrl.CompareNoCase(wcpathorurl) == 0)
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
    if (m_bLogThreadRunning || m_bMonitorThreadRunning || netScheduler.GetRunningThreadCount())
    {
        SetDlgItemText(IDC_LOGINFO, CString(MAKEINTRESOURCE(IDS_MONITOR_THREADRUNNING)));
        return;
    }
    // mark all entries as 'never checked before'
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        pItem->lastChecked = 0;
        return false;
    });
    // start the check timer
    SetTimer(MONITOR_TIMER, 1000, nullptr);
}

void CLogDlg::OnMonitorMarkAllAsRead()
{
    // mark all entries as unread
    HTREEITEM hItem = m_projTree.GetSelectedItem();
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hLocalItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hLocalItem));
        pItem->unreadItems = 0;
        pItem->unreadFirst = 0;
        m_projTree.SetItemState(hLocalItem, pItem->unreadItems ? TVIS_BOLD : 0, TVIS_BOLD);
        m_projTree.SetItemState(hLocalItem, pItem->authFailed ? INDEXTOOVERLAYMASK(OVERLAY_MODIFIED) : 0, TVIS_OVERLAYMASK);
        return false;
    });

    if (hItem)
    {
        // re-select the item so all revisions are marked as read as well
        LRESULT result = 0;
        MonitorShowProject(hItem, &result);
    }
    SaveMonitorProjects(false);
    m_projTree.Invalidate();
}

void CLogDlg::OnMonitorClearErrors()
{
    // clear all errors
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        pItem->authFailed  = false;
        pItem->lastErrorMsg.Empty();
        m_projTree.SetItemState(hItem, pItem->authFailed ? INDEXTOOVERLAYMASK(OVERLAY_MODIFIED) : 0, TVIS_OVERLAYMASK);
        return false;
    });

    SaveMonitorProjects(false);
    m_projTree.Invalidate();
}

void CLogDlg::OnMonitorAddProject()
{
    MonitorEditProject(nullptr);
}

void CLogDlg::OnMonitorEditProject()
{
    MonitorItem* pProject = nullptr;
    HTREEITEM    hSelItem = m_projTree.GetSelectedItem();
    if (hSelItem)
    {
        pProject = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hSelItem));
    }
    MonitorEditProject(pProject);
}

void CLogDlg::OnMonitorRemoveProject()
{
    if (!m_bMonitoringMode)
        return;
    HTREEITEM hSelItem = m_projTree.GetSelectedItem();
    if (hSelItem)
    {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hSelItem));
        if (pItem)
        {
            CString sTask1;
            sTask1.Format(IDS_MONITOR_DELETE_TASK2, static_cast<LPCWSTR>(pItem->name));
            CTaskDialog taskDlg(sTask1,
                                CString(MAKEINTRESOURCE(IDS_MONITOR_DELETE_TASK1)),
                                L"TortoiseSVN",
                                0,
                                TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION |
                                    TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_MONITOR_DELETE_TASK3)));
            taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_MONITOR_DELETE_TASK4)));
            taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskDlg.SetDefaultCommandControl(200);
            taskDlg.SetMainIcon(TD_WARNING_ICON);
            if (taskDlg.DoModal(m_hWnd) != 100)
                return;
            HTREEITEM hChild = m_projTree.GetChildItem(hSelItem);
            if (hChild)
            {
                RecurseMonitorTree(hChild, [&](HTREEITEM hItem) -> bool {
                    MonitorItem* pLocalItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
                    delete pLocalItem;
                    return false;
                });
            }
            delete pItem;
            m_projTree.DeleteItem(hSelItem);
            SaveMonitorProjects(true);
            RefreshMonitorProjTree();
        }
    }
}

void CLogDlg::OnMonitorOptions()
{
    CMonitorOptionsDlg dlg(this);
    dlg.m_bPlaySound        = m_bPlaySound;
    dlg.m_bShowNotification = m_bShowNotification;
    dlg.m_defaultInterval   = m_defaultMonitorInterval;
    dlg.DoModal();
    m_bPlaySound             = !!dlg.m_bPlaySound;
    m_bShowNotification      = !!dlg.m_bShowNotification;
    m_defaultMonitorInterval = dlg.m_defaultInterval;

    SaveMonitorProjects(false);
}

void CLogDlg::MonitorEditProject(MonitorItem* pProject, const CString& sParentPath)
{
    CMonitorProjectDlg dlg(this);
    dlg.m_monitorInterval = m_defaultMonitorInterval;
    if (pProject)
    {
        dlg.m_sName           = pProject->name;
        dlg.m_sPathOrURL      = pProject->wcPathOrUrl;
        dlg.m_sUsername       = CStringUtils::Decrypt(pProject->userName).get();
        dlg.m_sPassword       = CStringUtils::Decrypt(pProject->password).get();
        dlg.m_monitorInterval = pProject->interval;
        dlg.m_sIgnoreRegex    = pProject->sMsgRegex;
        dlg.m_isParentPath    = pProject->parentPath;
        dlg.m_sIgnoreUsers.Empty();
        for (const auto& s : pProject->authorsToIgnore)
        {
            dlg.m_sIgnoreUsers += CUnicodeUtils::GetUnicode(s.c_str());
            dlg.m_sIgnoreUsers += L" ";
        }
    }
    if (dlg.DoModal() == IDOK)
    {
        if (dlg.m_sName.IsEmpty())
            return;
        MonitorItem* pEditProject = pProject;
        if (pProject == nullptr)
            pEditProject = new MonitorItem();
        pEditProject->name = dlg.m_sName;
        if (SVN::PathIsURL(CTSVNPath(dlg.m_sPathOrURL)))
            pEditProject->wcPathOrUrl = CPathUtils::PathUnescape(dlg.m_sPathOrURL);
        else
            pEditProject->wcPathOrUrl = dlg.m_sPathOrURL;
        if (!pEditProject->wcPathOrUrl.IsEmpty())
        {
            // remove quotes in case the user put the url/path in quotes
            pEditProject->wcPathOrUrl.Trim(L"\" \t");
        }
        pEditProject->interval   = dlg.m_monitorInterval;
        pEditProject->userName   = CStringUtils::Encrypt(dlg.m_sUsername);
        pEditProject->password   = CStringUtils::Encrypt(dlg.m_sPassword);
        pEditProject->parentPath = dlg.m_isParentPath;
        pEditProject->sMsgRegex = dlg.m_sIgnoreRegex;
        try
        {
            pEditProject->msgRegex = std::wregex(pEditProject->sMsgRegex, std::regex_constants::ECMAScript | std::regex_constants::icase);
        }
        catch (std::exception&)
        {
            pEditProject->msgRegex = std::wregex();
            pEditProject->sMsgRegex.Empty();
        }
        stringtok(pEditProject->authorsToIgnore, CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(dlg.m_sIgnoreUsers)), true, " ,;");

        // insert the new item
        // if this was an edit, we don't have to do anything since
        // the tree control stores the pointer to the object, and we've just
        // updated that object
        if (pProject == nullptr)
        {
            InsertMonitorItem(pEditProject, sParentPath);
        }

        SaveMonitorProjects(false);
        RefreshMonitorProjTree();
        return;
    }
}

void CLogDlg::SaveMonitorProjects(bool todisk)
{
    m_monitoringFile.Reset();
    m_monitoringFile.SetMultiLine();
    int     count = 0;
    CString sTmp;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        CString      sSection;
        sSection.Format(L"item_%03d", count++);
        HTREEITEM hParent = m_projTree.GetParentItem(hItem);
        CString   sParentPath;
        if (hParent)
        {
            sParentPath = GetTreePath(hParent);
        }
        m_monitoringFile.SetValue(sSection, L"Name", pItem->name);
        m_monitoringFile.SetValue(sSection, L"parentTreePath", sParentPath);
        m_monitoringFile.SetValue(sSection, L"wcPathOrUrl", pItem->wcPathOrUrl);
        sTmp.Format(L"%lld", pItem->lastChecked);
        m_monitoringFile.SetValue(sSection, L"lastChecked", sTmp);
        sTmp.Format(L"%lld", pItem->lastCheckedRobots);
        m_monitoringFile.SetValue(sSection, L"lastCheckedRobots", sTmp);
        sTmp.Format(L"%ld", pItem->lastHead);
        m_monitoringFile.SetValue(sSection, L"lastHead", sTmp);
        sTmp.Format(L"%d", pItem->unreadItems);
        m_monitoringFile.SetValue(sSection, L"unreadItems", sTmp);
        sTmp.Format(L"%ld", pItem->unreadFirst);
        m_monitoringFile.SetValue(sSection, L"unreadFirst", sTmp);
        sTmp.Format(L"%d", pItem->interval);
        m_monitoringFile.SetValue(sSection, L"interval", sTmp);
        sTmp.Format(L"%d", pItem->minMinutesInterval);
        m_monitoringFile.SetValue(sSection, L"minMinutesInterval", sTmp);
        m_monitoringFile.SetValue(sSection, L"username", pItem->userName);
        m_monitoringFile.SetValue(sSection, L"password", pItem->password);
        m_monitoringFile.SetValue(sSection, L"MsgRegex", pItem->sMsgRegex);
        m_monitoringFile.SetValue(sSection, L"uuid", pItem->uuid);
        m_monitoringFile.SetValue(sSection, L"root", pItem->root);
        m_monitoringFile.SetValue(sSection, L"lastErrorMsg", pItem->lastErrorMsg);
        m_monitoringFile.SetValue(sSection, L"authFailed", pItem->authFailed ? L"1" : L"0");
        m_monitoringFile.SetValue(sSection, L"parentPath", pItem->parentPath ? L"1" : L"0");
        pItem->projectProperties.SaveToIni(m_monitoringFile, sSection);
        sTmp.Empty();
        for (const auto& s : pItem->authorsToIgnore)
        {
            sTmp += CUnicodeUtils::GetUnicode(s.c_str());
            sTmp += L" ";
        }
        m_monitoringFile.SetValue(sSection, L"ignoreauthors", sTmp);

        return false;
    });

    m_monitoringFile.SetValue(L"global", L"PlaySound", m_bPlaySound ? L"1" : L"0");
    m_monitoringFile.SetValue(L"global", L"ShowNotifications", m_bShowNotification ? L"1" : L"0");
    sTmp.Format(L"%d", m_defaultMonitorInterval);
    m_monitoringFile.SetValue(L"global", L"DefaultCheckInterval", sTmp);

    if (todisk)
    {
        CString sDataFilePath = CPathUtils::GetAppDataDirectory();
        sDataFilePath += L"\\MonitoringData.ini";
        CString sTempfile  = CTempFiles::Instance().GetTempFilePathString();
        FILE*   pFile      = nullptr;
        errno_t err        = 0;
        int     retrycount = 5;
        do
        {
            err = _tfopen_s(&pFile, sTempfile, L"wb");
            if ((err == 0) && pFile)
            {
                m_monitoringFile.SaveFile(pFile);
                err = fclose(pFile);
            }
            if (err)
            {
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error saving %s, retrycount %d\n", static_cast<LPCWSTR>(sTempfile), retrycount);
                Sleep(500);
            }
        } while (err && retrycount--);
        if (err == 0)
        {
            if (!CopyFile(sTempfile, sDataFilePath, FALSE))
            {
                CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error copying %s to %s, Error: %u\n", static_cast<LPCWSTR>(sTempfile), static_cast<LPCWSTR>(sDataFilePath), GetLastError());
            }
        }
        else
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Error saving %s - saving failed\n", static_cast<LPCWSTR>(sTempfile));
        SyncCommand syncCmd;
        syncCmd.Execute();
    }
}

void CLogDlg::MonitorTimer()
{
    if (m_bLogThreadRunning || m_bMonitorThreadRunning || netScheduler.GetRunningThreadCount())
    {
        SetTimer(MONITOR_TIMER, 60 * 1000, nullptr);
        return;
    }

    __time64_t currentTime = 0;
    _time64(&currentTime);

    CAutoReadWeakLock locker(m_monitorGuard);
    if (!locker.IsAcquired())
    {
        SetTimer(MONITOR_TIMER, 1000, nullptr);
        return;
    }

    m_monitorItemListForThread.clear();
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        if (!pItem->wcPathOrUrl.IsEmpty() &&
            ((pItem->lastChecked + (max(pItem->minMinutesInterval, pItem->interval) * 60LL)) < currentTime))
            m_monitorItemListForThread.push_back(*pItem);
        return false;
    });

    if (!m_monitorItemListForThread.empty())
    {
        m_bCancelled = false;
        InterlockedExchange(&m_bMonitorThreadRunning, TRUE);
        new async::CAsyncCall(this, &CLogDlg::MonitorThread, &netScheduler);
    }
    SetTimer(MONITOR_TIMER, 60 * 1000, nullptr);
}

void CLogDlg::MonitorPopupTimer()
{
    QUERY_USER_NOTIFICATION_STATE ns = QUNS_ACCEPTS_NOTIFICATIONS;
    SHQueryUserNotificationState(&ns);
    if (ns != QUNS_ACCEPTS_NOTIFICATIONS)
    {
        // restart the timer and wait until notifications can be shown
        SetTimer(MONITOR_POPUP_TIMER, 5000, nullptr);
    }
    else
    {
        if (m_bShowNotification)
        {
            if (Snarl::V42::SnarlInterface::IsSnarlRunning())
            {
                auto token = g_snarlInterface.Notify(SNARL_CLASS_ID, m_sMonitorNotificationTitle, m_sMonitorNotificationText);
                g_snarlInterface.AddAction(token, CString(MAKEINTRESOURCE(IDS_MONITOR_NOTIFY_LINK)), SNARL_NOTIFY_CLICK_EVENT_STR);
            }
            else
            {
                bool toastShown = false;
                if (IsWindows10OrGreater() && static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\UseWin10ToastNotifications", TRUE)))
                {
                    std::vector<std::wstring> lines;
                    if (!m_sMonitorNotificationTitle.IsEmpty())
                        lines.push_back(static_cast<LPCWSTR>(m_sMonitorNotificationTitle));
                    if (!m_sMonitorNotificationText.IsEmpty())
                        lines.push_back(static_cast<LPCWSTR>(m_sMonitorNotificationText));
                    if (!lines.empty())
                    {
                        ToastNotifications toastNotifier;
                        auto               hr = toastNotifier.ShowToast(GetSafeHwnd(), L"TSVN.MONITOR.1", CPathUtils::GetAppDirectory() + L"tsvn-logo.png", lines);
                        toastShown            = SUCCEEDED(hr);
                    }
                }
                if (!toastShown)
                {
                    MonitorAlertWnd* pPopup = new MonitorAlertWnd(GetSafeHwnd());

                    pPopup->SetAnimationType(CMFCPopupMenu::ANIMATION_TYPE::FADE);
                    pPopup->SetAnimationSpeed(40);
                    pPopup->SetTransparency(200);
                    pPopup->SetSmallCaption(TRUE);
                    pPopup->SetAutoCloseTime(5000);

                    // Create indirect:
                    CMFCDesktopAlertWndInfo params;

                    params.m_hIcon     = CCommonAppUtils::LoadIconEx(IDR_MAINFRAME, 0, 0);
                    params.m_strText   = m_sMonitorNotificationTitle + L"\n" + m_sMonitorNotificationText;
                    params.m_strURL    = CString(MAKEINTRESOURCE(IDS_MONITOR_NOTIFY_LINK));
                    params.m_nURLCmdID = 101;
                    pPopup->Create(this, params);
                }
            }
        }

        if (m_bPlaySound)
            PlaySound(L"MailBeep", nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);

        KillTimer(MONITOR_POPUP_TIMER);
        m_sMonitorNotificationTitle.Empty();
        m_sMonitorNotificationText.Empty();
    }
}

void CLogDlg::MonitorThread()
{
    InterlockedExchange(&m_bMonitorThreadRunning, TRUE);

    __time64_t currentTime = 0;
    _time64(&currentTime);

    CAutoReadLock locker(m_monitorGuard);
    m_monitorItemParentPathList.clear();
    for (auto& item : m_monitorItemListForThread)
    {
        if (m_bCancelled)
            break;
        SVN svn(true);
        if (item.wcPathOrUrl.IsEmpty())
            continue;
        if (item.authFailed)
            continue; // if authentication failed before, don't try again
        if (item.parentPath)
        {
            if ((item.lastChecked + (max(item.minMinutesInterval, item.interval) * 60LL)) < currentTime)
            {
                // we have to include the authentication in the URL itself
                auto tempFile = CTempFiles::Instance().GetTempFilePath(true);
                auto callback = std::make_unique<CCallback>();
                callback->SetAuthData(CStringUtils::Decrypt(item.userName).get(), CStringUtils::Decrypt(item.password).get());
                DeleteFile(tempFile.GetWinPath());
                HRESULT hResUdl = URLDownloadToFile(nullptr, item.wcPathOrUrl, tempFile.GetWinPath(), 0, callback.get());
                if (m_bCancelled)
                    continue;
                if (hResUdl == S_OK)
                {
                    // we got a web page! But we can't be sure that it's the page from SVNParentPath.
                    // Use a regex to parse the website and find out...
                    std::ifstream fs(tempFile.GetWinPath());
                    std::string   in;
                    if (!fs.bad())
                    {
                        in.reserve(static_cast<unsigned>(fs.rdbuf()->in_avail()));
                        char c;
                        while (fs.get(c))
                        {
                            if (in.capacity() == in.size())
                                in.reserve(in.capacity() * 3);
                            in.append(1, c);
                        }
                        fs.close();
                        DeleteFile(tempFile.GetWinPath());

                        // make sure this is a html page from an SVNParentPathList
                        // we do this by checking for header titles looking like
                        // "<h2>Revision XX: /</h2> - if we find that, it's a html
                        // page from inside a repository
                        // some repositories show
                        // "<h2>projectname - Revision XX: /trunk</h2>
                        const char* reTitle = "<\\s*h2\\s*>[^/]+Revision\\s*\\d+:[^<]+<\\s*/\\s*h2\\s*>";
                        // xsl transformed pages don't have an easy way to determine
                        // the inside from outside of a repository.
                        // We therefore check for <index rev="0" to make sure it's either
                        // an empty repository or really an SVNParentPathList
                        const char*      reTitle2 = "<\\s*index\\s*rev\\s*=\\s*\"0\"";
                        const std::regex titex(reTitle, std::regex_constants::icase | std::regex_constants::ECMAScript);
                        const std::regex titex2(reTitle2, std::regex_constants::icase | std::regex_constants::ECMAScript);
                        if (std::regex_search(in.begin(), in.end(), titex, std::regex_constants::match_default))
                        {
                            CTraceToOutputDebugString::Instance()(_T("found repository url instead of SVNParentPathList\n"));
                            continue;
                        }

                        const char* re  = "<\\s*LI\\s*>\\s*<\\s*A\\s+[^>]*HREF\\s*=\\s*\"([^\"]*)\"\\s*>([^<]+)<\\s*/\\s*A\\s*>\\s*<\\s*/\\s*LI\\s*>";
                        const char* re2 = "<\\s*DIR\\s*name\\s*=\\s*\"([^\"]*)\"\\s*HREF\\s*=\\s*\"([^\"]*)\"\\s*/\\s*>";

                        const std::regex           expression(re, std::regex_constants::icase | std::regex_constants::ECMAScript);
                        const std::regex           expression2(re2, std::regex_constants::icase | std::regex_constants::ECMAScript);
                        std::wstring               popupText;
                        const std::sregex_iterator end;
                        for (std::sregex_iterator i(in.begin(), in.end(), expression); i != end; ++i)
                        {
                            if (m_bCancelled)
                                break;
                            const std::smatch match = *i;
                            // what[0] contains the whole string
                            // what[1] contains the url part.
                            // what[2] contains the name
                            auto url = CUnicodeUtils::GetUnicode(std::string(match[1]).c_str());
                            url      = item.wcPathOrUrl + _T("/") + url;

                            // we found a new URL, add it to our list
                            MonitorItem mi       = item;
                            mi.name              = CPathUtils::PathUnescape(std::string(match[2]).c_str()).TrimRight('/');
                            mi.wcPathOrUrl       = url;
                            mi.authFailed        = false;
                            mi.parentPath        = false;
                            mi.lastChecked       = 0;
                            mi.lastCheckedRobots = 0;
                            mi.lastErrorMsg.Empty();
                            mi.lastHead    = 0;
                            mi.unreadFirst = 0;
                            mi.unreadItems = 0;
                            mi.uuid.Empty();
                            m_monitorItemParentPathList.insert(std::pair<CString, MonitorItem>(item.wcPathOrUrl, mi));
                        }
                        if (m_bCancelled)
                            continue;
                        if (!regex_search(in.begin(), in.end(), titex2))
                        {
                            CTraceToOutputDebugString::Instance()(_T("found repository url instead of SVNParentPathList\n"));
                            continue;
                        }
                        for (std::sregex_iterator i(in.begin(), in.end(), expression2); i != end; ++i)
                        {
                            if (m_bCancelled)
                                break;
                            const std::smatch match = *i;
                            // what[0] contains the whole string
                            // what[1] contains the url part.
                            // what[2] contains the name
                            auto url = CUnicodeUtils::GetUnicode(std::string(match[1]).c_str());
                            url      = item.wcPathOrUrl + _T("/") + url;

                            MonitorItem mi       = item;
                            mi.name              = CPathUtils::PathUnescape(std::string(match[2]).c_str()).TrimRight('/');
                            mi.wcPathOrUrl       = url;
                            mi.authFailed        = false;
                            mi.parentPath        = false;
                            mi.lastChecked       = 0;
                            mi.lastCheckedRobots = 0;
                            mi.lastErrorMsg.Empty();
                            mi.lastHead    = 0;
                            mi.unreadFirst = 0;
                            mi.unreadItems = 0;
                            mi.uuid.Empty();
                            m_monitorItemParentPathList.insert(std::pair<CString, MonitorItem>(item.wcPathOrUrl, mi));
                        }
                        if (m_bCancelled)
                            continue;
                    }
                }
                DeleteFile(tempFile.GetWinPath());
            }
        }
        else
        {
            CTSVNPath wcPathOrUrl(item.wcPathOrUrl);
            if ((item.lastChecked + (max(item.minMinutesInterval, item.interval) * 60LL)) < currentTime)
            {
                CString sCheckInfo;
                sCheckInfo.Format(IDS_MONITOR_CHECKPROJECT, static_cast<LPCWSTR>(item.name));
                if (!m_bCancelled)
                    SetDlgItemText(IDC_LOGINFO, sCheckInfo);
                svn.SetAuthInfo(CStringUtils::Decrypt(item.userName).get(), CStringUtils::Decrypt(item.password).get());
                svn_revnum_t head = svn.GetHEADRevision(wcPathOrUrl, false);
                if (m_bCancelled)
                    continue;
                if (item.lastHead < 0)
                    item.lastHead = 0;
                if ((head > 0) && (head != item.lastHead))
                {
                    // new head revision: fetch the log
                    std::unique_ptr<const CCacheLogQuery> cachedData;
                    {
                        CAutoWriteLock pathlock(m_monitorPathGuard);
                        m_pathCurrentlyChecked = item.wcPathOrUrl;
                    }
                    cachedData = svn.ReceiveLog(CTSVNPathList(wcPathOrUrl), SVNRev::REV_HEAD, head, item.lastHead, m_limit, false, false, true);
                    // Err will also be set if the user cancelled.
                    if (m_bCancelled)
                        continue;
                    if ((svn.GetSVNError() == nullptr) && (item.lastHead >= 0))
                    {
                        CString                  sUuid;
                        CString                  sRoot  = svn.GetRepositoryRootAndUUID(wcPathOrUrl, true, sUuid);
                        CString                  sUrl   = svn.GetURLFromPath(wcPathOrUrl);
                        CString                  relUrl = sUrl.Mid(sRoot.GetLength());
                        CCachedLogInfo*          cache  = cachedData->GetCache();
                        const CPathDictionary*   paths  = &cache->GetLogInfo().GetPaths();
                        CDictionaryBasedTempPath logPath(paths, static_cast<const char*>(CUnicodeUtils::GetUTF8(relUrl)));
                        CLogCacheUtility         logUtil(cache, &m_projectProperties);

                        for (svn_revnum_t rev = item.lastHead + 1; rev <= head; ++rev)
                        {
                            if (logUtil.IsCached(rev))
                            {
                                auto pLogItem = logUtil.GetRevisionData(rev);
                                if (pLogItem)
                                {
                                    bool bIgnore = false;
                                    for (const auto& authorToIgnore : item.authorsToIgnore)
                                    {
                                        if (_stricmp(pLogItem->GetAuthor().c_str(), authorToIgnore.c_str()) == 0)
                                        {
                                            bIgnore = true;
                                            break;
                                        }
                                    }
                                    if (!bIgnore && !item.sMsgRegex.IsEmpty())
                                    {
                                        try
                                        {
                                            if (std::regex_match(pLogItem->GetMessageW().cbegin(),
                                                                 pLogItem->GetMessageW().cend(),
                                                                 item.msgRegex))
                                            {
                                                bIgnore = true;
                                            }
                                        }
                                        catch (std::exception&)
                                        {
                                        }
                                    }
                                    if (bIgnore)
                                        continue;

                                    pLogItem->Finalize(cache, logPath);
                                    if (IsRevisionRelatedToUrl(logPath, pLogItem.get()))
                                    {
                                        ++item.unreadItems;
                                    }
                                }
                            }
                        }
                        if (item.unreadFirst == 0)
                            item.unreadFirst = item.lastHead;
                        item.lastHead = head;
                        item.root     = sRoot;
                        item.uuid     = sUuid;
                        item.lastErrorMsg.Empty();
                    }
                    else
                    {
                        auto svnError = svn.GetSVNError();
                        if (svnError)
                        {
                            if ((SVN_ERROR_IN_CATEGORY(svnError->apr_err, SVN_ERR_AUTHN_CATEGORY_START)) ||
                                (SVN_ERROR_IN_CATEGORY(svnError->apr_err, SVN_ERR_AUTHZ_CATEGORY_START)) ||
                                (svnError->apr_err == SVN_ERR_RA_DAV_FORBIDDEN))
                            {
                                item.authFailed = true;
                            }
                            item.lastErrorMsg = svn.GetLastErrorMessage();
                        }
                        else
                            item.lastErrorMsg.Empty();
                    }
                    // we should never get asked for authentication here!
                    item.authFailed = item.authFailed || PromptShown();
                }
                else
                {
                    auto svnError = svn.GetSVNError();
                    if (svnError)
                    {
                        if ((SVN_ERROR_IN_CATEGORY(svnError->apr_err, SVN_ERR_AUTHN_CATEGORY_START)) ||
                            (SVN_ERROR_IN_CATEGORY(svnError->apr_err, SVN_ERR_AUTHZ_CATEGORY_START)) ||
                            (svnError->apr_err == SVN_ERR_RA_DAV_FORBIDDEN) ||
                            (svnError->apr_err == SVN_ERR_WC_NOT_WORKING_COPY))
                        {
                            item.authFailed = true;
                        }
                        item.lastErrorMsg = svn.GetLastErrorMessage();
                    }
                    else
                        item.lastErrorMsg.Empty();
                }
                item.lastChecked = currentTime;
                {
                    CAutoWriteLock pathLock(m_monitorPathGuard);
                    m_pathCurrentlyChecked.Empty();
                }
                if (!m_bCancelled)
                    SetDlgItemText(IDC_LOGINFO, L"");
            }
            if (!item.authFailed && ((item.lastCheckedRobots + (60 * 60 * 24)) < currentTime))
            {
                if (m_bCancelled)
                    continue;
                // try to read the project properties
                ProjectProperties props;
                if (!props.ReadProps(wcPathOrUrl))
                {
                    if (wcPathOrUrl.IsUrl() && (wcPathOrUrl.GetSVNPathString().Find(L"trunk") < 0))
                    {
                        CTSVNPath trunkPath = wcPathOrUrl;
                        trunkPath.AppendPathString(L"trunk");
                        if (props.ReadProps(trunkPath))
                            item.projectProperties = props;
                    }
                }
                else
                    item.projectProperties = props;

                if (m_bCancelled)
                    continue;
                std::wstring sRobotsURL = static_cast<LPCWSTR>(svn.GetURLFromPath(wcPathOrUrl));
                sRobotsURL += _T("/svnrobots.txt");
                std::wstring sRootRobotsURL;
                std::wstring sDomainRobotsURL = sRobotsURL.substr(0, sRobotsURL.find('/', sRobotsURL.find(':') + 3)) + _T("/svnrobots.txt");
                sRootRobotsURL                = svn.GetRepositoryRoot(wcPathOrUrl);
                if (!sRootRobotsURL.empty())
                    sRootRobotsURL += _T("/svnrobots.txt");
                CTSVNPath sFile = CTempFiles::Instance().GetTempFilePath(true);
                OnOutOfScope(DeleteFile(sFile.GetWinPath()));
                std::string in;
                auto        callback = std::make_unique<CCallback>();
                if (callback == nullptr)
                    continue;
                {
                    auto         du = CStringUtils::Decrypt(item.userName);
                    auto         dp = CStringUtils::Decrypt(item.password);
                    std::wstring sU;
                    std::wstring sP;
                    auto         pU = du.get();
                    if (pU)
                        sU = pU;
                    auto pP = dp.get();
                    if (pP)
                        sP = pP;
                    callback->SetAuthData(sU, sP);
                }
                if (m_bCancelled)
                    continue;
                if ((!sDomainRobotsURL.empty()) && (URLDownloadToFile(nullptr, sDomainRobotsURL.c_str(), sFile.GetWinPath(), 0, callback.get()) == S_OK))
                {
                    std::ifstream fs(sFile.GetWinPath());
                    if (!fs.bad())
                    {
                        OnOutOfScope(fs.close());
                        in.reserve(static_cast<unsigned>(fs.rdbuf()->in_avail()));
                        char c;
                        while (fs.get(c))
                        {
                            if (in.capacity() == in.size())
                                in.reserve(in.capacity() * 3);
                            in.append(1, c);
                        }
                        if ((in.find("<html>") != std::string::npos) ||
                            (in.find("<HTML>") != std::string::npos) ||
                            (in.find("<head>") != std::string::npos) ||
                            (in.find("<HEAD>") != std::string::npos))
                            in.clear();
                    }
                }
                if (m_bCancelled)
                    continue;
                if (in.empty() && (!sRootRobotsURL.empty()) && (svn.Export(CTSVNPath(sRootRobotsURL.c_str()), sFile, SVNRev::REV_HEAD, SVNRev::REV_HEAD)))
                {
                    std::ifstream fs(sFile.GetWinPath());
                    if (!fs.bad())
                    {
                        OnOutOfScope(fs.close());
                        in.reserve(static_cast<unsigned>(fs.rdbuf()->in_avail()));
                        char c;
                        while (fs.get(c))
                        {
                            if (in.capacity() == in.size())
                                in.reserve(in.capacity() * 3);
                            in.append(1, c);
                        }
                    }
                }
                if (m_bCancelled)
                    continue;
                if (in.empty() && svn.Export(CTSVNPath(sRobotsURL.c_str()), sFile, SVNRev::REV_HEAD, SVNRev::REV_HEAD))
                {
                    std::ifstream fs(sFile.GetWinPath());
                    if (!fs.bad())
                    {
                        OnOutOfScope(fs.close());
                        in.reserve(static_cast<unsigned>(fs.rdbuf()->in_avail()));
                        char c;
                        while (fs.get(c))
                        {
                            if (in.capacity() == in.size())
                                in.reserve(in.capacity() * 3);
                            in.append(1, c);
                        }
                    }
                }
                // the format of the svnrobots.txt file is as follows:
                // # comment
                // checkinterval = XXX
                //
                // with 'checkinterval' being the minimum amount of time to wait
                // between checks in minutes.

                std::istringstream iss(in);
                std::string        line;
                int                minutes = 0;
                while (getline(iss, line))
                {
                    if (line.length())
                    {
                        if (line.at(0) != '#')
                        {
                            if ((line.length() > 13) && (line.substr(0, 13).compare("checkinterval") == 0))
                            {
                                std::string num = line.substr(line.find('=') + 1);
                                minutes         = atoi(num.c_str());
                            }
                        }
                    }
                }
                item.lastCheckedRobots  = currentTime;
                item.minMinutesInterval = minutes;
            }

            svn.SetAuthInfo(L"", L"");
        }
    }
    // if the thread is cancelled, then don't update the log label
    // here to avoid a deadlock situation in MonitorShowProject() when
    // waiting for this thread to finish: since there the message queue
    // does not run, the below call to SetDlgItemText() can't finish
    // and the thread won't end.
    if (!m_bCancelled)
        SetDlgItemText(IDC_LOGINFO, L"");

    InterlockedExchange(&m_bMonitorThreadRunning, FALSE);
    PostMessage(WM_COMMAND, ID_LOGDLG_MONITOR_THREADFINISHED);
    RefreshCursor();
    m_bCancelled = false;
}

bool CLogDlg::IsRevisionRelatedToUrl(const CDictionaryBasedTempPath& basePath, PLOGENTRYDATA pLogItem)
{
    const auto& changedPaths = pLogItem->GetChangedPaths();
    for (size_t i = 0; i < changedPaths.GetCount(); ++i)
    {
        if (basePath.IsSameOrParentOf(changedPaths[i].GetCachedPath()))
            return true;
    }
    return false;
}

void CLogDlg::OnMonitorThreadFinished()
{
    SetDlgItemText(IDC_LOGINFO, L"");

    CString sTemp;
    int     changedProjects = 0;
    bool    hasUnreadItems  = false;
    bool    hasNewChildren  = false;
    {
        CAutoReadLock locker(m_monitorGuard);
        for (const auto& item : m_monitorItemListForThread)
        {
            HTREEITEM hItem = FindMonitorItem(item.wcPathOrUrl);
            if (hItem)
            {
                MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
                if ((pItem->unreadItems != item.unreadItems) && (pItem->lastHead != item.lastHead))
                {
                    if (item.unreadItems)
                    {
                        if (changedProjects)
                        {
                            m_sMonitorNotificationTitle.Format(IDS_MONITOR_NOTIFY_MULTITITLE, changedProjects + 1);
                            m_sMonitorNotificationText += L"\n";
                            sTemp.Format(IDS_MONITOR_NOTIFY_TEXT, static_cast<LPCWSTR>(item.name), item.unreadItems - pItem->unreadItems);
                            m_sMonitorNotificationText += sTemp;
                        }
                        else
                        {
                            m_sMonitorNotificationTitle.LoadString(IDS_MONITOR_NOTIFY_TITLE);
                            m_sMonitorNotificationText.Format(IDS_MONITOR_NOTIFY_TEXT, static_cast<LPCWSTR>(item.name), item.unreadItems - pItem->unreadItems);
                        }
                        hasUnreadItems = true;
                    }
                    ++changedProjects;
                }
                if ((pItem->lastHead != item.lastHead) || (!item.lastErrorMsg.IsEmpty()))
                {
                    // to avoid overwriting a "mark as read" operation,
                    // only overwrite these values if the last found
                    // HEAD revision has changed: in that case there would be new
                    // unread items anyway and overwriting the "mark all as read"
                    // is almost expected.
                    pItem->lastHead     = item.lastHead;
                    pItem->unreadItems  = item.unreadItems;
                    pItem->unreadFirst  = item.unreadFirst;
                    pItem->authFailed   = item.authFailed;
                    pItem->lastErrorMsg = item.lastErrorMsg;
                    pItem->root         = item.root;
                    pItem->uuid         = item.uuid;
                    if (hItem == m_projTree.GetSelectedItem() && IsWindowVisible())
                    {
                        // refresh the current view
                        LRESULT lresult = 0;
                        MonitorShowProject(hItem, &lresult);
                    }
                }
                if (pItem->lastCheckedRobots != item.lastCheckedRobots)
                {
                    pItem->projectProperties = item.projectProperties;
                }
                pItem->lastChecked        = item.lastChecked;
                pItem->lastCheckedRobots  = item.lastCheckedRobots;
                pItem->minMinutesInterval = item.minMinutesInterval;

                m_projTree.SetItemState(hItem, pItem->unreadItems ? TVIS_BOLD : 0, TVIS_BOLD);
                m_projTree.SetItemState(hItem, ((pItem->authFailed || !pItem->lastErrorMsg.IsEmpty()) ? INDEXTOOVERLAYMASK(OVERLAY_MODIFIED) : 0), TVIS_OVERLAYMASK);
            }
        }
        for (const auto& mip : m_monitorItemParentPathList)
        {
            auto hItem = FindMonitorItem(mip.second.wcPathOrUrl);
            if (hItem == nullptr)
            {
                auto item = new MonitorItem();
                *item     = mip.second;

                auto hParent = FindMonitorItem(mip.first);
                if (hParent)
                {
                    InsertMonitorItem(item, GetTreePath(hParent));
                    hasNewChildren = true;
                }
            }
        }
    }
    {
        CAutoWriteLock locker(m_monitorGuard);
        m_monitorItemListForThread.clear();
    }
    if (hasUnreadItems)
    {
        m_systemTray.hIcon  = m_hMonitorIconNewCommits;
        m_systemTray.uFlags = NIF_ICON;
        if (Shell_NotifyIcon(NIM_MODIFY, &m_systemTray) == FALSE)
        {
            Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
            m_systemTray.uFlags = NIF_MESSAGE | NIF_ICON;
            Shell_NotifyIcon(NIM_ADD, &m_systemTray);
        }
        if (m_pTaskbarList)
        {
            m_pTaskbarList->SetOverlayIcon(GetSafeHwnd(), m_hMonitorIconNewCommits, L"");
        }
    }
    m_monitorItemParentPathList.clear();
    m_projTree.Invalidate();

    if (!m_sMonitorNotificationTitle.IsEmpty() && !m_sMonitorNotificationText.IsEmpty())
    {
        SetTimer(MONITOR_POPUP_TIMER, 10, nullptr);
    }
    SaveMonitorProjects(true);

    if (hasNewChildren)
        RefreshMonitorProjTree();

    SVNReInit();
}

void CLogDlg::ShutDownMonitoring()
{
    SaveMonitorProjects(true);

    // remove all existing data from the control and free the memory
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        delete pItem;
        m_projTree.SetItemData(hItem, NULL);
        return false;
    });
    m_projTree.DeleteAllItems();

    Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
    UnRegisterSnarl();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CLogDlg::OnTvnSelchangedProjtree(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    *pResult                 = 0;
    ::SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_LOGDLG_MONITOR_EDIT, MAKELONG(!!(pNMTreeView->itemNew.state & TVIS_SELECTED), 0));
    ::SendMessage(m_hwndToolbar, TB_ENABLEBUTTON, ID_LOGDLG_MONITOR_REMOVE, MAKELONG(!!(pNMTreeView->itemNew.state & TVIS_SELECTED), 0));
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CLogDlg::OnTvnGetdispinfoProjtree(NMHDR* pNMHDR, LRESULT* pResult)
{
    static wchar_t textBuf[1024];
    LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
    *pResult                   = 0;
    textBuf[0]                 = 0;
    if (pTVDispInfo->item.mask & TVIF_TEXT)
    {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(pTVDispInfo->item.hItem));
        if (pItem)
        {
            if (pItem->unreadItems)
                swprintf_s(textBuf, L"%s (%d)", static_cast<LPCWSTR>(pItem->name), pItem->unreadItems);
            else
                wcscpy_s(textBuf, pItem->name);
            pTVDispInfo->item.pszText = textBuf;
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
            int unreadItems    = 0;
            int unreadProjects = 0;
            RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
                MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
                if (pItem && pItem->unreadItems)
                {
                    ++unreadProjects;
                    unreadItems += pItem->unreadItems;
                }
                return false;
            });

            // update the tool tip data
            m_systemTray.uFlags = NIF_TIP;
            if (unreadItems)
            {
                CString sFormat(MAKEINTRESOURCE(unreadItems == 1 ? IDS_MONITOR_NEWCOMMIT : IDS_MONITOR_NEWCOMMITS));
                swprintf_s(m_systemTray.szTip, _countof(m_systemTray.szTip), static_cast<LPCWSTR>(sFormat), unreadItems, unreadProjects);
            }
            else
            {
                CString sFormat(MAKEINTRESOURCE(IDS_MONITOR_DLGTITLE));
                swprintf_s(m_systemTray.szTip, _countof(m_systemTray.szTip), static_cast<LPCWSTR>(sFormat));
            }
            if (Shell_NotifyIcon(NIM_MODIFY, &m_systemTray) == FALSE)
            {
                Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
                m_systemTray.uFlags = NIF_MESSAGE | NIF_TIP;
                Shell_NotifyIcon(NIM_ADD, &m_systemTray);
            }
        }
        break;
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            MonitorShowDlg();
            return TRUE;
        case NIN_KEYSELECT:
        case NIN_SELECT:
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
        {
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = ::LoadMenu(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MONITORTRAY));
            hMenu       = ::GetSubMenu(hMenu, 0);

            // set the default entry
            MENUITEMINFO iinfo = {0};
            iinfo.cbSize       = sizeof(MENUITEMINFO);
            iinfo.fMask        = MIIM_STATE;
            GetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);
            iinfo.fState |= MFS_DEFAULT;
            SetMenuItemInfo(hMenu, 0, MF_BYPOSITION, &iinfo);

            // show the menu
            ::SetForegroundWindow(*this);
            int cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, NULL, *this, nullptr);
            ::PostMessage(*this, WM_NULL, 0, 0);
            ::DestroyMenu(hMenu);
            switch (cmd)
            {
                case ID_POPUP_EXIT:
                {
                    m_bCancelled             = true;
                    bool threadsStillRunning = !netScheduler.WaitForEmptyQueueOrTimeout(8000) || !diskScheduler.WaitForEmptyQueueOrTimeout(8000);

                    if (threadsStillRunning)
                    {
                        // end the process the hard way
                        if (m_bMonitoringMode)
                            Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
                        TerminateProcess(GetCurrentProcess(), 0);
                    }
                    EndDialog(0);
                }
                break;
                case ID_POPUP_SHOWMONITOR:
                    MonitorShowDlg();
                    break;
                case ID_POPUP_UPDATEALL:
                    OnMonitorUpdateAll();
                    break;
            }
        }
        break;
    }
    return TRUE;
}

void CLogDlg::OnNMClickProjtree(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    CPoint pt;
    GetCursorPos(&pt);
    m_projTree.ScreenToClient(&pt);

    UINT      unFlags = 0;
    HTREEITEM hItem   = m_projTree.HitTest(pt, &unFlags);
    *pResult          = 0;
    if ((unFlags & TVHT_ONITEM) && (hItem != nullptr))
    {
        MonitorShowProject(hItem, pResult);
    }
}

void CLogDlg::MonitorShowProject(HTREEITEM hItem, LRESULT* pResult)
{
    if (pResult)
        *pResult = 0;
    if (hItem == nullptr)
    {
        m_changedFileListCtrl.SetItemCountEx(0);
        m_changedFileListCtrl.Invalidate();
        m_logList.SetItemCountEx(0);
        m_logList.Invalidate();
        CWnd* pMsgView = GetDlgItem(IDC_MSGVIEW);
        pMsgView->SetWindowText(L"");

        m_nSortColumnPathList = 0;
        m_bAscendingPathList  = false;
        SetSortArrow(&m_logList, -1, true);
        m_logEntries.ClearAll();
        m_monitorAuthorsToIgnore.clear();
        m_sMonitorMsgRegex.Empty();
        m_path.Reset();
        OnClickedCancelFilter(0, 0);

        GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

        return;
    }
    MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
    if (pItem)
    {
        m_changedFileListCtrl.SetItemCountEx(0);
        m_changedFileListCtrl.Invalidate();
        m_logList.SetItemCountEx(0);
        m_logList.Invalidate();
        CWnd* pMsgView = GetDlgItem(IDC_MSGVIEW);
        pMsgView->SetWindowText(L"");

        m_nSortColumnPathList = 0;
        m_bAscendingPathList  = false;
        SetSortArrow(&m_logList, -1, true);
        m_logEntries.ClearAll();
        m_monitorAuthorsToIgnore.clear();
        m_sMonitorMsgRegex.Empty();
        OnClickedCancelFilter(0, 0);
        GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
        if (pItem->wcPathOrUrl.IsEmpty() || pItem->parentPath)
            return;
        if (pItem->authFailed && !pItem->lastErrorMsg.IsEmpty())
        {
            CString temp(MAKEINTRESOURCE(IDS_LOG_CLEARERROR));
            m_logList.ShowText(pItem->lastErrorMsg + L"\n\n" + temp, true);
            FillLogMessageCtrl(false);
            m_path.Reset();
            GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
            return;
        }
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
            SetDlgItemText(IDC_LOGINFO, CString(MAKEINTRESOURCE(IDS_LOG_MONITOR_BUSY)));
            GetDlgItem(IDC_LOGINFO)->UpdateWindow();

            m_bCancelled             = true;
            bool threadsStillRunning = !netScheduler.WaitForEmptyQueueOrTimeout(5000) || !diskScheduler.WaitForEmptyQueueOrTimeout(5000);

            // can we close the app cleanly?

            if (threadsStillRunning)
            {
                m_projTree.SelectItem(nullptr);
                if (pResult)
                    *pResult = 1; // prevent default processing
                // restart the timer
                SetTimer(MONITOR_TIMER, 60 * 1000, nullptr);
                return;
            }
        }
        m_bCancelled = false;

        // while the user is looking at the logs, delay the monitoring
        // thread from starting too soon again
        SetTimer(MONITOR_TIMER, 60 * 1000, nullptr);

        svn_revnum_t head = pItem->lastHead;
        if (head == 0)
            head = GetHEADRevision(CTSVNPath(pItem->wcPathOrUrl), false);

        m_limit = static_cast<int>(static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs", 100))) + 1;

        m_path                   = CTSVNPath(pItem->wcPathOrUrl);
        m_pegRev                 = head;
        m_head                   = head;
        m_startRev               = head;
        m_bStartRevIsHead        = false;
        m_logRevision            = head;
        m_bStrict                = false;
        m_bSaveStrict            = false;
        m_revUnread              = pItem->unreadFirst;
        m_endRev                 = 1;
        m_hasWC                  = !m_path.IsUrl();
        m_projectProperties      = pItem->projectProperties;
        m_monitorAuthorsToIgnore = pItem->authorsToIgnore;
        m_sMonitorMsgRegex       = pItem->sMsgRegex;
        m_sUuid                  = pItem->uuid;
        m_sRepositoryRoot        = pItem->root;

        // the bugtraq issue id column is only shown if the bugtraq:url or bugtraq:regex is set
        if ((!m_projectProperties.sUrl.IsEmpty()) || (!m_projectProperties.GetCheckRe().IsEmpty()))
            m_bShowBugtraqColumn = true;

        ConfigureColumnsForLogListControl();

        m_wcRev = SVNRev();
        if (::IsWindow(m_hWnd))
            UpdateData(FALSE);

        pItem->unreadItems = 0;
        pItem->unreadFirst = 0;

        InterlockedExchange(&m_bLogThreadRunning, TRUE);
        new async::CAsyncCall(this, &CLogDlg::LogThread, &netScheduler);

        m_projTree.SetItemState(hItem, pItem->unreadItems ? TVIS_BOLD : 0, TVIS_BOLD);
        m_projTree.SetItemState(hItem, pItem->authFailed ? INDEXTOOVERLAYMASK(OVERLAY_MODIFIED) : 0, TVIS_OVERLAYMASK);
    }

    bool hasUnreadItems = false;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hLocalItem) -> bool {
        MonitorItem* pLocalItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hLocalItem));
        if (pLocalItem && pLocalItem->unreadItems)
        {
            hasUnreadItems = true;
            return true;
        }
        return false;
    });
    m_systemTray.hIcon  = hasUnreadItems ? m_hMonitorIconNewCommits : m_hMonitorIconNormal;
    m_systemTray.uFlags = NIF_ICON;
    if (Shell_NotifyIcon(NIM_MODIFY, &m_systemTray) == FALSE)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
        m_systemTray.uFlags = NIF_MESSAGE | NIF_ICON;
        Shell_NotifyIcon(NIM_ADD, &m_systemTray);
    }
}

LRESULT CLogDlg::OnMonitorNotifyClick(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    MonitorShowDlg();
    return 0;
}

LRESULT CLogDlg::OnMonitorNotifySnarlReply(WPARAM wParam, LPARAM /*lParam*/)
{
    int eventCode = wParam & 0xffff;
    int data      = (wParam & 0xffffffff) >> 16; // "Convert" to 32 bit, to shut up 64bit warning
    if ((eventCode == Snarl::V42::SnarlEnums::NotifyAction) && (data == SNARL_NOTIFY_CLICK_EVENT))
        MonitorShowDlg();
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
    SaveMonitorProjects(false);
    RefreshMonitorProjTree();
    return 0;
}

LRESULT CLogDlg::OnTreeDrop(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    SaveMonitorProjects(false);
    RefreshMonitorProjTree();
    return 0;
}

void CLogDlg::OnTvnEndlabeleditProjtree(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

    MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(pTVDispInfo->item.hItem));
    if (pItem && pTVDispInfo->item.pszText)
    {
        CString sName = pTVDispInfo->item.pszText;
        if (sName != pItem->name)
        {
            pItem->name = sName;
            SaveMonitorProjects(false);
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
    m_bCancelled                = false;
    HTREEITEM hSelectedTreeItem = m_projTree.GetSelectedItem();
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        m_projTree.GetItemRect(hSelectedTreeItem, &rect, TRUE);
        m_projTree.ClientToScreen(&rect);
        point = rect.CenterPoint();
    }

    UINT   uFlags = 0;
    CPoint ptTree = point;
    m_projTree.ScreenToClient(&ptTree);
    HTREEITEM hItem = m_projTree.HitTest(ptTree, &uFlags);
    // in case the right-clicked item is not the selected one, select it
    if ((hItem) && (uFlags & TVHT_ONITEM) && (hItem != hSelectedTreeItem))
    {
        m_projTree.SelectItem(hItem);
    }

    CIconMenu popup;
    if (!popup.CreatePopupMenu())
        return;
    MonitorItem* pItem = nullptr;

    if (hItem == nullptr)
    {
        popup.AppendMenuIcon(ID_LOGDLG_MONITOR_ADDPROJECT, IDS_LOG_POPUP_MONITORADD, IDI_MONITOR_ADD);
    }
    else
    {
        pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        if (pItem == nullptr)
            return;

        // entry is selected, now show the popup menu
        popup.AppendMenuIcon(ID_LOGDLG_MONITOR_EDIT, IDS_LOG_POPUP_MONITOREDIT, IDI_MONITOR_EDIT);
        popup.AppendMenuIcon(ID_LOGDLG_MONITOR_REMOVE, IDS_LOG_POPUP_MONITORREMOVE, IDI_MONITOR_REMOVE);
        popup.AppendMenu(MF_SEPARATOR, NULL);
        popup.AppendMenuIcon(ID_LOGDLG_MONITOR_ADDSUBPROJECT, IDS_LOG_POPUP_MONITORADDSUB, IDI_MONITOR_ADD);

        if (!::PathIsURL(pItem->wcPathOrUrl) && PathFileExists(pItem->wcPathOrUrl) && !pItem->wcPathOrUrl.IsEmpty() && !pItem->parentPath)
        {
            popup.AppendMenu(MF_SEPARATOR, NULL);
            popup.AppendMenuIcon(ID_UPDATE, IDS_MENUUPDATE, IDI_UPDATE);
            popup.AppendMenuIcon(ID_EXPLORE, IDS_LOG_POPUP_EXPLORE, IDI_EXPLORER);
            popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_OPENURL, IDI_URL);
            popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
        }
        else if (::PathIsURL(pItem->wcPathOrUrl) && !pItem->wcPathOrUrl.IsEmpty() && !pItem->parentPath)
        {
            popup.AppendMenu(MF_SEPARATOR, NULL);
            popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_OPENURL, IDI_URL);
            popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
        }
    }
    int              cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                                   point.x, point.y, this, nullptr);
    CLogWndHourglass wait;

    switch (cmd)
    {
        case ID_LOGDLG_MONITOR_ADDPROJECT:
            MonitorEditProject(nullptr);
            break;
        case ID_LOGDLG_MONITOR_ADDSUBPROJECT:
            MonitorEditProject(nullptr, GetTreePath(hItem));
            break;
        case ID_LOGDLG_MONITOR_EDIT:
            OnMonitorEditProject();
            break;
        case ID_LOGDLG_MONITOR_REMOVE:
            OnMonitorRemoveProject();
            break;
        case ID_UPDATE:
        {
            CString sCmd;
            sCmd.Format(L"/command:update /path:\"%s\"", static_cast<LPCWSTR>(pItem->wcPathOrUrl));
            CAppUtils::RunTortoiseProc(sCmd);
        }
        break;
        case ID_EXPLORE:
        {
            PCIDLIST_ABSOLUTE __unaligned pidl = ILCreateFromPath(static_cast<LPCWSTR>(pItem->wcPathOrUrl));
            if (pidl)
            {
                SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
                CoTaskMemFree(static_cast<LPVOID>(const_cast<PIDLIST_ABSOLUTE>(pidl)));
            }
        }
        break;
        case ID_VIEWPATHREV:
        {
            CString url = GetURLFromPath(CTSVNPath(pItem->wcPathOrUrl));
            if (url.IsEmpty())
            {
                ShowErrorDialog(GetSafeHwnd());
                break;
            }
            ShellExecute(this->m_hWnd, L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
        }
        break;
        case ID_REPOBROWSE:
        {
            CString sCmd;
            sCmd.Format(L"/command:repobrowser /path:\"%s\"",
                        static_cast<LPCWSTR>(pItem->wcPathOrUrl));

            CAppUtils::RunTortoiseProc(sCmd);
        }
        break;
        default:
            break;
    } // switch (cmd)
}

BOOL CLogDlg::OnQueryEndSession()
{
    SaveMonitorProjects(true);
    m_bSystemShutDown = true;
    if (!__super::OnQueryEndSession())
        return FALSE;

    return TRUE;
}

LRESULT CLogDlg::OnReloadIniMsg(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    SaveMonitorProjects(false);
    InitMonitorProjTree();
    return 0;
}

LRESULT CLogDlg::OnShowDlgMsg(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    MonitorShowDlg();
    return 0;
}

LRESULT CLogDlg::OnToastNotification(WPARAM wParam, LPARAM /*lParam*/)
{
    switch (wParam)
    {
        case ToastNotificationAction::Activate: // notification activated
            MonitorShowDlg();
            return TRUE;
        case ToastNotificationAction::Dismiss_UserCanceled: // The user dismissed this toast
        case ToastNotificationAction::Dismiss_TimedOut:     // The toast has timed out
            return TRUE;
        case ToastNotificationAction::Dismiss_ApplicationHidden: // The application hid the toast using ToastNotifier.hide()
        case ToastNotificationAction::Dismiss_NotActivated:      // Toast not activated
        case ToastNotificationAction::Failed:                    // The toast encountered an error
        default:
            break;
    }
    return TRUE;
}

void CLogDlg::MonitorShowDlg()
{
    m_bKeepHidden = false;
    // remove selection, show empty log list
    m_projTree.SelectItem(nullptr);
    MonitorShowProject(nullptr, nullptr);
    OnClickedCancelFilter(0, 0);
    ShowWindow(SW_SHOW);
    SetForegroundWindow();
    m_systemTray.hIcon  = m_hMonitorIconNormal;
    m_systemTray.uFlags = NIF_ICON;
    if (Shell_NotifyIcon(NIM_MODIFY, &m_systemTray) == FALSE)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_systemTray);
        m_systemTray.uFlags = NIF_MESSAGE | NIF_ICON;
        Shell_NotifyIcon(NIM_ADD, &m_systemTray);
    }
}

BOOL CLogDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    static CString sMarkAllAsReadTooltip(MAKEINTRESOURCE(IDS_MONITOR_MARKASREADTT));
    static CString sUpdateAllTooltip(MAKEINTRESOURCE(IDS_MONITOR_UPDATEALLTT));

    LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
    if ((lpnmhdr->code == TBN_GETINFOTIP) && (lpnmhdr->hwndFrom == m_hwndToolbar))
    {
        LPNMTBGETINFOTIP lptbgit = reinterpret_cast<LPNMTBGETINFOTIPW>(lParam);
        switch (lptbgit->iItem)
        {
            case ID_MISC_MARKALLASREAD:
                lptbgit->pszText = const_cast<wchar_t*>(static_cast<LPCWSTR>(sMarkAllAsReadTooltip));
                break;
            case ID_MISC_UPDATE:
                lptbgit->pszText = const_cast<wchar_t*>(static_cast<LPCWSTR>(sUpdateAllTooltip));
                break;
        }
    }

    return __super::OnNotify(wParam, lParam, pResult);
}

auto CLogDlg::RegisterSnarl() const -> void
{
    g_snarlGlobalMsg = Snarl::V42::SnarlInterface::Broadcast();
    if (Snarl::V42::SnarlInterface::IsSnarlRunning())
    {
        wchar_t com[MAX_PATH + 100] = {0};
        GetModuleFileName(nullptr, com, MAX_PATH);

        CString sIconPath = com;
        sIconPath += L",-1";
        g_snarlInterface.Register(SNARL_APP_ID, CString(MAKEINTRESOURCE(IDS_MONITOR_DLGTITLE)), sIconPath, nullptr, GetSafeHwnd(), WM_TSVN_MONITOR_SNARLREPLY, Snarl::V42::SnarlEnums::AppFlagNone);
        g_snarlInterface.AddClass(SNARL_CLASS_ID, L"Commit Notification");
    }
}

void CLogDlg::UnRegisterSnarl()
{
    if (Snarl::V42::SnarlInterface::IsSnarlRunning())
    {
        g_snarlInterface.RemoveClass(SNARL_CLASS_ID);
        g_snarlInterface.Unregister(SNARL_APP_ID);
    }
    g_snarlGlobalMsg = 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CLogDlg::OnMonitorUpdateAll()
{
    // find all working copy paths, write them to a temp file
    CTSVNPathList pathlist;
    RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
        MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
        CTSVNPath    path(pItem->wcPathOrUrl);
        if (!path.IsUrl())
        {
            pathlist.AddPath(path);
        }
        return false;
    });
    if (pathlist.GetCount() == 0)
        return;
    CTSVNPath tempFile = CTempFiles::Instance().GetTempFilePath(false);
    if (pathlist.WriteToFile(tempFile.GetWinPathString()))
    {
        CString sCmd;
        sCmd.Format(L"/command:update /pathfile:\"%s\" /deletepathfile",
                    static_cast<LPCWSTR>(tempFile.GetWinPath()));
        CAppUtils::RunTortoiseProc(sCmd);
    }
}

void CLogDlg::OnDrop(const CTSVNPathList& pathList, const CString& parent)
{
    bool bAdded = false;
    for (int i = 0; i < pathList.GetCount(); ++i)
    {
        auto path = pathList[i];
        if (path.IsUrl() || SVNHelper::IsVersioned(path, false))
        {
            CString name;
            if (path.IsUrl())
            {
                name = CAppUtils::GetProjectNameFromURL(path.GetSVNPathString());
                if (name.IsEmpty())
                    name = path.GetFileOrDirectoryName();
            }
            else
                name = path.GetFileOrDirectoryName();

            if (name.IsEmpty())
                continue;

            auto pItem         = new MonitorItem();
            pItem->name        = name;
            pItem->wcPathOrUrl = path.IsUrl() ? path.GetSVNPathString() : path.GetWinPathString();
            pItem->interval    = m_defaultMonitorInterval;
            InsertMonitorItem(pItem, parent);
            bAdded = true;
        }
    }
    if (bAdded)
    {
        SaveMonitorProjects(true);
        RefreshMonitorProjTree();
    }
}

LRESULT CLogDlg::OnTaskbarButtonCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    setUuidOverlayIcon(m_hWnd);
    if (m_bMonitoringMode)
    {
        SaveMonitorProjects(true);
        RefreshMonitorProjTree();
        // find the first project with unread items
        HTREEITEM hUnreadItem = nullptr;
        RecurseMonitorTree(TVI_ROOT, [&](HTREEITEM hItem) -> bool {
            MonitorItem* pItem = reinterpret_cast<MonitorItem*>(m_projTree.GetItemData(hItem));
            if (pItem->unreadItems)
            {
                hUnreadItem = hItem;
                return true;
            }
            return false;
        });
        if (hUnreadItem)
        {
            m_projTree.SelectItem(hUnreadItem);
            MonitorShowProject(hUnreadItem, nullptr);
        }
    }
    return 0;
}

void CLogDlg::OnLvnBegindragLogmsg(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult           = 0;

    ContextMenuInfoForChangedPathsPtr pCmi(new CContextMenuInfoForChangedPaths());
    if (!GetContextMenuInfoForChangedPaths(pCmi))
        return;

    CTSVNPathList selectedUrls;
    CString       sRoot = GetRepositoryRoot(CTSVNPath(pCmi->fileUrl));

    for (size_t i = 0; i < pCmi->changedLogPathIndices.size(); ++i)
    {
        const CLogChangedPath& changedLogPathI = m_currentChangedArray[pCmi->changedLogPathIndices[i]];

        if (changedLogPathI.GetAction() == LOGACTIONS_DELETED)
            continue;

        CString filepath = sRoot + changedLogPathI.GetPath();
        selectedUrls.AddPath(CTSVNPath(filepath));
    }

    // build copy source / content
    auto pdSrc = std::make_unique<CIDropSource>();
    if (pdSrc == nullptr)
        return;

    pdSrc->AddRef();

    SVNDataObject* pdObj = new SVNDataObject(selectedUrls, pCmi->rev1, pCmi->rev1);
    if (pdObj == nullptr)
    {
        return;
    }
    pdObj->AddRef();
    pdObj->SetAsyncMode(TRUE);
    CDragSourceHelper dragsrchelper;
    dragsrchelper.InitializeFromWindow(m_changedFileListCtrl.GetSafeHwnd(), pNMLV->ptAction, pdObj);
    pdSrc->m_pIDataObj = pdObj;
    pdSrc->m_pIDataObj->AddRef();

    // Initiate the Drag & Drop
    DWORD dwEffect;
    ::DoDragDrop(pdObj, pdSrc.get(), DROPEFFECT_MOVE | DROPEFFECT_COPY, &dwEffect);
    pdSrc->Release();
    pdSrc.release();
    pdObj->Release();
}

void CLogDlg::OnSysColorChange()
{
    __super::OnSysColorChange();
    CTheme::Instance().OnSysColorChanged();
    SendDlgItemMessage(IDC_MSGVIEW, WM_SYSCOLORCHANGE, 0, 0);
    CMFCVisualManager::GetInstance()->RedrawAll();
}

LRESULT CLogDlg::OnDPIChanged(WPARAM wParam, LPARAM lParam)
{
    CDPIAware::Instance().Invalidate();
    if (m_bMonitoringMode)
    {
        RemoveAnchor(m_hwndToolbar);
        m_toolbarImages.DeleteImageList();
        ::CloseWindow(m_hwndToolbar);
        ::DestroyWindow(m_hwndToolbar);
        CreateToolbar();
        CRect rcSearch;
        m_cFilter.GetWindowRect(&rcSearch);
        ScreenToClient(&rcSearch);
        CRect rect;
        ::GetClientRect(m_hwndToolbar, &rect);
        CRect rcDlg;
        GetClientRect(&rcDlg);
        ::SetWindowPos(m_hwndToolbar, nullptr, rcSearch.left, 0, rcDlg.Width(), rect.Height(), SWP_SHOWWINDOW);
        AddAnchor(m_hwndToolbar, TOP_LEFT, TOP_RIGHT);
    }
    SetupDialogFonts();
    SetupLogMessageViewControl();
    return __super::OnDPIChanged(wParam, lParam);
}
