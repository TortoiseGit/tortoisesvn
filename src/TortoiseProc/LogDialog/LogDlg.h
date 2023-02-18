// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020-2023 - TortoiseSVN

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
#pragma once

#include "resource.h"
#include "SVN.h"
#include "ProjectProperties.h"
#include "StandAloneDlg.h"
#include "TSVNPath.h"
#include "registry.h"
#include "SplitterControl.h"
#include "Colors.h"
#include "LogDlgHelper.h"
#include "FilterEdit.h"
#include "LogDlgFilter.h"
#include "SVNRev.h"
#include "SubTooltipListCtrl.h"
#include "HintCtrl.h"
#include "JobScheduler.h"
#include "ListViewAccServer.h"
#include "SimpleIni.h"
#include "DragDropTreeCtrl.h"
#include "ReaderWriterLock.h"
#include "ThemeControls.h"

// import EnvDTE for opening files in Visual Studio through COM
#pragma warning(push)
#pragma warning(disable : 4471)
#include "dte80a.tlh"
#pragma warning(pop)

#define MERGE_REVSELECTSTART    1
#define MERGE_REVSELECTEND      2
#define MERGE_REVSELECTSTARTEND 3 ///< both
#define MERGE_REVSELECTMINUSONE 4 ///< first with N-1

#define LOGFILTER_ALL       0xFFFF
#define LOGFILTER_MESSAGES  0x0001
#define LOGFILTER_PATHS     0x0002
#define LOGFILTER_AUTHORS   0x0004
#define LOGFILTER_REVS      0x0008
#define LOGFILTER_REGEX     0x0010
#define LOGFILTER_BUGID     0x0020
#define LOGFILTER_CASE      0x0040
#define LOGFILTER_DATE      0x0080
#define LOGFILTER_DATERANGE 0x0100

#define LOGFILTER_TIMER     101
#define MONITOR_TIMER       102
#define MONITOR_POPUP_TIMER 103

using GENERICCOMPAREFN = int(__cdecl*)(const void* elem1, const void* elem2);

enum RefreshEnum
{
    None,
    Simple,
    Cache
};

class MonitorItem
{
public:
    MonitorItem(const CString& name, const CString& path = CString())
        : name(name)
        , wcPathOrUrl(path)
        , interval(5)
        , minMinutesInterval(0)
        , lastChecked(0)
        , lastCheckedRobots(0)
        , lastHead(0)
        , unreadFirst(0)
        , unreadItems(0)
        , authFailed(false)
        , parentPath(false)
    {
    }
    MonitorItem()
        : interval(5)
        , minMinutesInterval(0)
        , lastChecked(0)
        , lastCheckedRobots(0)
        , lastHead(0)
        , unreadFirst(0)
        , unreadItems(0)
        , authFailed(false)
        , parentPath(false)
    {
    }
    ~MonitorItem() {}

    CString                  name;
    CString                  wcPathOrUrl;
    int                      interval;
    int                      minMinutesInterval;
    __time64_t               lastChecked;
    __time64_t               lastCheckedRobots;
    svn_revnum_t             lastHead;
    svn_revnum_t             unreadFirst;
    int                      unreadItems;
    bool                     authFailed;
    bool                     parentPath;
    CString                  lastErrorMsg;
    CString                  userName;
    CString                  password;
    CString                  sMsgRegex;
    std::wregex              msgRegex;
    std::vector<std::string> authorsToIgnore;
    CString                  root;
    CString                  uuid;
    ProjectProperties        projectProperties;
};

using MonitorItemHandler = std::function<bool(HTREEITEM)>;

// fwd declaration
class CIconMenu;

/**
 * \ingroup TortoiseProc
 * Shows log messages of a single file or folder in a listbox.
 */
class CLogDlg : public CResizableStandAloneDialog
    , public SVN
    , IFilterEditValidator
    , IListCtrlTooltipProvider
    , ListViewAccProvider
{
    DECLARE_DYNAMIC(CLogDlg)

    friend class CStoreSelection;
    friend class CMonitorTreeTarget;

public:
    CLogDlg(CWnd* pParent = nullptr); // standard constructor
    ~CLogDlg() override;

    void SetParams(const CTSVNPath& path, const SVNRev& pegRev, const SVNRev& startRev, const SVNRev& endRev,
                   BOOL bStrict     = CRegDWORD(L"Software\\TortoiseSVN\\LastLogStrict", FALSE),
                   BOOL bSaveStrict = TRUE,
                   int  limit       = static_cast<int>(static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\NumberOfLogs",
                                                                             100))));
    void SetFilter(const CString& findStr, LONG findType, bool findRegex, const CString& sDateFrom, const CString& sDateTo);
    void SetIncludeMerge(bool bInclude = true) { m_bIncludeMerges = bInclude; }
    void SetProjectPropertiesPath(const CTSVNPath& path) { m_projectProperties.ReadProps(path); }
    bool IsThreadRunning() { return !netScheduler.WaitForEmptyQueueOrTimeout(0); }
    void SetDialogTitle(const CString& sTitle) { m_sTitle = sTitle; }
    void SetSelect(bool bSelect) { m_bSelect = bSelect; }
    void ContinuousSelection(bool bCont = true) { m_bSelectionMustBeContinuous = bCont; }
    void SetMergePath(const CTSVNPath& mergepath) { m_mergePath = mergepath; }

    const SVNRevRangeArray& GetSelectedRevRanges() const { return m_selectedRevs; }
    void                    SetSelectedRevRanges(const SVNRevRangeArray& revArray);
    void                    SetMonitoringMode(bool starthidden)
    {
        m_bMonitoringMode = true;
        m_bKeepHidden     = starthidden;
    }

    // Dialog Data
    enum
    {
        IDD = IDD_LOGMESSAGE
    };

protected:
    //implement the virtual methods from SVN base class
    BOOL Log(svn_revnum_t rev, const std::string& author, const std::string& message,
             apr_time_t time, const MergeInfo* mergeInfo) override;
    BOOL Cancel() override;
    bool Validate(LPCWSTR string) override;

    virtual bool FilterConditionChanged();

    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support

    afx_msg LRESULT OnRefreshSelection(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTaskbarCreated(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnShowDlgMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnReloadIniMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnClickedInfoIcon(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnClickedCancelFilter(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnToastNotification(WPARAM wParam, LPARAM lParam);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void    OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void    OnBnClickedGetall();
    afx_msg void    OnNMDblclkChangedFileList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMDblclkLoglist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnItemchangedLoglist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnBnClickedHelp();
    afx_msg void    OnEnLinkMsgview(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnBnClickedStatbutton();
    afx_msg void    OnNMCustomdrawLoglist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnNMCustomdrawChangedFileList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnGetdispinfoLoglist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnGetdispinfoChangedFileList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnEnChangeSearchedit();
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnDtnDatetimechangeDateto(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnDtnDatetimechangeDatefrom(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnColumnclickChangedFileList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnBnClickedNexthundred();
    afx_msg void    OnBnClickedHidepaths();
    afx_msg void    OnBnClickedCheckStoponcopy();
    afx_msg void    OnLvnOdfinditemLoglist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnDtnDropdownDatefrom(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnDtnDropdownDateto(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnSize(UINT nType, int cx, int cy);
    afx_msg void    OnBnClickedIncludemerge();
    afx_msg void    OnBnClickedRefresh();
    afx_msg void    OnRefresh();
    afx_msg void    OnFind();
    afx_msg void    OnFocusFilter();
    afx_msg void    OnEditCopy();
    afx_msg void    OnCopyRevisions();
    afx_msg void    OnLogCancel();
    afx_msg void    OnLvnKeydownLoglist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnLvnKeydownFilelist(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnEnscrollMsgview();
    afx_msg void    OnDestroy();
    afx_msg void    OnClose();
    afx_msg void    OnMonitorCheckNow();
    afx_msg void    OnMonitorAddProject();
    afx_msg void    OnMonitorEditProject();
    afx_msg void    OnMonitorRemoveProject();
    afx_msg void    OnMonitorOptions();
    afx_msg void    OnMonitorMarkAllAsRead();
    afx_msg void    OnMonitorClearErrors();
    afx_msg void    OnMonitorUpdateAll();
    afx_msg void    OnMonitorThreadFinished();
    afx_msg void    OnTvnSelchangedProjtree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnTvnGetdispinfoProjtree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnTaskbarCallBack(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnNMClickProjtree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnMonitorNotifyClick(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMonitorNotifySnarlReply(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg LRESULT OnTreeDrop(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnTvnEndlabeleditProjtree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnInlineedit();
    afx_msg BOOL    OnQueryEndSession();
    afx_msg LRESULT OnTaskbarButtonCreated(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnLvnBegindragLogmsg(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnSysColorChange();
    afx_msg LRESULT OnDPIChanged(WPARAM wParam, LPARAM lParam);

    void OnCancel() override;
    void OnOK() override;
    BOOL OnInitDialog() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;
    BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

    void FillLogMessageCtrl(bool bShow = true);
    void DoDiffFromLog(INT_PTR selIndex, svn_revnum_t rev1, svn_revnum_t rev2, bool blame, bool unified, bool ignoreprops);

    DECLARE_MESSAGE_MAP()

private:
    void              LogThread();
    void              StatusThread();
    void              Refresh(bool autoGoOnline = false);
    void              SetTheme(bool bDark) const;
    static BOOL       IsDiffPossible(const CLogChangedPath& changedPath, svn_revnum_t rev);
    BOOL              Open(bool bOpenWith, CString changedPath, svn_revnum_t rev);
    void              EditAuthor(const std::vector<PLOGENTRYDATA>& logs);
    void              EditLogMessage(size_t index);
    void              DoSizeV1(int delta);
    void              DoSizeV2(int delta);
    void              SetSplitterRange();
    void              SetFilterCueText();
    void              CopySelectionToClipBoard(bool bIncludeChangedList) const;
    void              CopySelectionToClipBoardRev() const;
    void              CopySelectionToClipBoardViewerRev() const;
    void              CopySelectionToClipBoardViewerPathRev() const;
    void              CopySelectionToClipBoardTsvnShowCompare() const;
    void              CopySelectionToClipBoardBugId();
    std::set<CString> GetSelectedBugIds();
    void              CopySelectionToClipBoardBugUrl();
    void              CopyCommaSeparatedRevisionsToClipboard() const;
    void              CopyChangedSelectionToClipBoard() const;
    void              CopyCommaSeparatedAuthorsToClipboard() const;
    void              CopyMessagesToClipboard() const;
    void              CopyChangedPathInfoToClipboard(ContextMenuInfoForChangedPathsPtr pCmi, int cmd) const;

    CTSVNPathList GetChangedPathsAndMessageSketchFromSelectedRevisions(CString&              sMessageSketch,
                                                                       CLogChangedPathArray& currentChangedArray) const;
    void          RecalculateShownList(svn_revnum_t revToKeep = -1);
    static void   SetSortArrow(CListCtrl* control, int nColumn, bool bAscending);
    void          SortByColumn(int nSortColumn, bool bAscending);
    void          SortAndFilter(svn_revnum_t revToKeep = -1);
    bool          IsSelectionContinuous() const;
    void          EnableOKButton();
    void          GetAll(bool bForceAll = false);
    void          UpdateSelectedRevs();
    void          UpdateLogInfoLabel();
    void          SaveSplitterPos() const;
    static bool   ValidateRegexp(LPCWSTR regexpStr, std::wregex& pat, bool bMatchCase);
    void          CheckRegexpTooltip();
    void          DiffSelectedFile(bool ignoreProps);
    void          DiffSelectedRevWithPrevious();
    void          SetDlgTitle(bool bOffline);
    void          SelectAllVisibleRevisions();
    void          AddMainAnchors();
    void          RemoveMainAnchors();
    void          AdjustDateFilterVisibility();
    void          ReportNoUrlOfFile(const CString& filepath) const;
    void          ReportNoUrlOfFile(LPCWSTR filepath) const;
    CRect         DrawListColumnBackground(const CListCtrl& listCtrl, const NMLVCUSTOMDRAW* pLVCD, PLOGENTRYDATA pLogEntry) const;
    LRESULT       DrawListItemWithMatches(CListCtrl& listCtrl, NMLVCUSTOMDRAW* pLVCD, PLOGENTRYDATA pLogEntry);

    // extracted from OnInitDialog()...
    void SubclassControls();
    void SetupDialogFonts();
    void RestoreSavedDialogSettings();
    void SetupLogMessageViewControl();
    void SetupLogListControl();
    void LoadIconsForActionColumns();
    void ConfigureColumnsForLogListControl();
    void ConfigureColumnsForChangedFileListControl();
    void SetupFilterControlBitmaps();
    void ConfigureResizableControlAnchors();
    void RestoreLogDlgWindowAndSplitters();
    void AdjustControlSizesForLocalization();
    void GetOriginalControlRectangles();
    void SetupDatePickerControls();
    void ConfigureDialogForPickingRevisionsOrShowingLog();
    void SetupButtonMenu();
    void ReadProjectPropertiesAndBugTraqInfo();
    void SetupToolTips();
    void InitializeTaskBarListPtr();
    void CenterThisWindow();
    void SetupAccessibility();
    void ExtraInitialization();

    int ShownCountWithStopped() const { return static_cast<int>(m_logEntries.GetVisibleCount()) + (m_bStrictStopped ? 1 : 0); }

    LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

    void ResizeAllListCtrlCols(bool bOnlyVisible);

    void        ShowContextMenuForRevisions(CWnd* pWnd, CPoint point);
    void        PopulateContextMenuForRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi, CIconMenu& popup, CIconMenu& clipSubMenu);
    bool        GetContextMenuInfoForRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    static void AdjustContextMenuAnchorPointIfKeyboardInvoked(CPoint& point, int selIndex, CListCtrl& listControl);
    bool        VerifyContextMenuForRevisionsAllowed(int selIndex) const;
    void        ExecuteGnuDiff1MenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteGnuDiff2MenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteRevertRevisionMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const;
    void        ExecuteMergeRevisionMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteRevertToRevisionMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const;
    void        ExecuteCopyMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteCompareWithWorkingCopyMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteCompareTwoMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteCompareWithPreviousMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteBlameCompareMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteBlameTwoMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteWithPreviousMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteSaveAsMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteOpenMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi, bool bOpenWith);
    void        ExecuteBlameMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteUpdateMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const;
    void        ExecuteFindEntryMenuRevisions();
    void        ExecuteRepoBrowseMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const;
    void        ExecuteRevisionPropsMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    static void ExecuteExportMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    static void ExecuteCheckoutMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteViewRevMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi) const;
    void        ExecuteViewPathRevMenuRevisions(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteGetMergeLogs(std::shared_ptr<CContextMenuInfoForRevisions>& pCmi);
    void        ExecuteAddCodeCollaboratorReview();
    CString     GetSpaceSeparatedSelectedRevisions() const;
    CString     GetUrlOfTrunk();

    void ShowContextMenuForChangedPaths(CWnd* pWnd, CPoint point);
    void ExecuteViewPathRevisionChangedPaths(INT_PTR selIndex) const;
    void ExecuteBrowseRepositoryChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath);
    void ExecuteShowLogChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath, bool bMergeLog);
    void ExecuteShowMergedLogs(ContextMenuInfoForChangedPathsPtr pCmi);
    void ExecuteBlameChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath);
    void ExecuteOpenChangedPaths(INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi, bool bOpenWith);
    void ExecuteExportTreeChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi);
    void ExecuteSaveAsChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, INT_PTR selIndex);
    void ExecuteShowPropertiesChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi);
    void ExecuteCompareChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, INT_PTR selIndex);
    void ExecuteDiffChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, INT_PTR selIndex, bool ignoreprops);
    void ExecuteGnuDiff1ChangedPaths(INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi);
    void ExecuteBlameDiffChangedPaths(INT_PTR selIndex, ContextMenuInfoForChangedPathsPtr pCmi);
    void ExecuteRevertChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, const CLogChangedPath& changedlogpath);
    bool VerifyContextMenuForChangedPathsAllowed(INT_PTR selIndex) const;
    bool GetContextMenuInfoForChangedPaths(ContextMenuInfoForChangedPathsPtr& pCmi);
    bool PopulateContextMenuForChangedPaths(ContextMenuInfoForChangedPathsPtr& pCmi, CIconMenu& popup, CIconMenu& clipSubMenu) const;
    void ExecuteMultipleDiffChangedPaths(ContextMenuInfoForChangedPathsPtr pCmi, bool ignoreprops);
    bool CheckMultipleDiffs(UINT selCount) const;
    int  OpenWorkingCopyFileWithRegisteredProgram(CString& fullPath) const;
    void OpenSelectedWcFilesWithRegistedProgram(std::vector<size_t>& changedlogpathindices);

    CString GetToolTipText(int nItem, int nSubItem) override;
    bool    DoFindItemLogList(LPNMLVFINDITEM pFindInfo, size_t startIndex, size_t endIndex,
                              const CString& whatToFind, LRESULT* pResult) const;
    void    NotifyTargetOnOk();
    void    CreateFindDialog();
    void    DoOpenFileWith(bool bReadOnly, bool bOpenWith, const CTSVNPath& tempfile) const;
    bool    ConfirmRevert(const CString& path, bool bToRev = false) const;

    // selection management

    void AutoStoreSelection();
    void AutoRestoreSelection(bool bClear = false);

    // ListViewAccProvider
    CString            GetListviewHelpString(HWND hControl, int index) override;
    void               DetectVisualStudioRunningThread();
    bool               OpenSelectedWcFilesWithVisualStudio(std::vector<size_t>& changedlogpathindices);
    static bool        OpenOneFileInVisualStudio(CString&                         filename,
                                                 CComPtr<EnvDTE::ItemOperations>& pItemOperations);
    CString            GetSUrl();
    CString            GetWcPathFromUrl(CString fileUrl);
    void               OpenSelectedFilesInVisualStudio(std::vector<size_t>&             changedlogpathindices,
                                                       CComPtr<EnvDTE::ItemOperations>& pItemOperations);
    static void        ActivateVisualStudioWindow(CComPtr<EnvDTE::_DTE>& pDte);
    static bool        IsProcessRunningInHighIntegrity(DWORD pid);
    static PTOKEN_USER GetUserTokenFromProcessId(DWORD pid);
    static bool        RunningInSameUserContextWithSameProcessIntegrity(DWORD pidVisualStudio);

    // MonitoringMode
    void InitMonitoringMode();

    void        RegisterSnarl() const;
    static void UnRegisterSnarl();

    bool CreateToolbar();
    void DoSizeV3(int delta);
    void InitMonitorProjTree();
    void RefreshMonitorProjTree();
    void MonitorEditProject(MonitorItem* pProject, const CString& sParentPath = CString());

    HTREEITEM           InsertMonitorItem(MonitorItem* pMonitorItem, const CString& sParentPath = CString());
    HTREEITEM           FindMonitorParent(const CString& parentTreePath) const;
    HTREEITEM           FindMonitorItem(const CString& wcpathorurl) const;
    HTREEITEM           RecurseMonitorTree(HTREEITEM hItem, MonitorItemHandler handler) const;
    void                SaveMonitorProjects(bool todisk);
    void                MonitorTimer();
    void                MonitorPopupTimer();
    void                MonitorThread();
    void                ShutDownMonitoring();
    CString             GetTreePath(HTREEITEM hItem) const;
    static bool         IsRevisionRelatedToUrl(const CDictionaryBasedTempPath& basePath, PLOGENTRYDATA pLogItem);
    void                MonitorShowProject(HTREEITEM hItem, LRESULT* pResult);
    void                ShowContextMenuForMonitorTree(CWnd* pWnd, CPoint point);
    static int CALLBACK TreeSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);
    void                MonitorShowDlg();
    void                MonitorHideDlg();
    void                OnDrop(const CTSVNPathList& pathList, const CString& parent);

public:
    CWnd*             m_pNotifyWindow;
    ProjectProperties m_projectProperties;
    WORD              m_wParam;

private:
    int                            m_themeCallbackId;
    CFont                          m_unreadFont;
    CFont                          m_wcRevFont;
    CString                        m_sRelativeRoot;
    CString                        m_sRepositoryRoot;
    CString                        m_sURL;
    CString                        m_sUuid; ///< empty if the log cache is not used
    CHintCtrl<CSubTooltipListCtrl> m_logList;
    CHintCtrl<CListCtrl>           m_changedFileListCtrl;
    CFilterEdit                    m_cFilter;
    CLogDlgFilter                  m_filter;
    CProgressCtrl                  m_logProgress;
    CThemeMFCMenuButton            m_btnShow;
    CMenu                          m_btnMenu;
    CTSVNPath                      m_path;
    CTSVNPath                      m_mergePath;
    SVNRev                         m_pegRev;
    SVNRev                         m_startRev;
    bool                           m_bStartRevIsHead;
    svn_revnum_t                   m_head; ///< only used in Range case of log
    RefreshEnum                    m_nRefresh;
    svn_revnum_t                   m_tempRev;                  ///< only used during ReceiveLog
    int                            m_cMergedRevisionsReceived; ///< only used during ReceiveLog
    SVNRev                         m_logRevision;
    SVNRev                         m_endRev;
    SVNRev                         m_wcRev;
    SVNRevRangeArray               m_selectedRevs;
    SVNRevRangeArray               m_selectedRevsOneRange;
    bool                           m_bSelectionMustBeContinuous;
    bool                           m_bCancelled;
    volatile LONG                  m_bLogThreadRunning;
    BOOL                           m_bStrict;
    bool                           m_bStrictStopped;
    BOOL                           m_bIncludeMerges;
    BOOL                           m_bSaveStrict;
    BOOL                           m_bHideNonMergeables;

    bool                   m_bSingleRevision;
    CLogChangedPathArray   m_currentChangedArray;
    CTSVNPathList          m_currentChangedPathList;
    bool                   m_hasWC;
    int                    m_nSearchIndex;
    bool                   m_bFilterWithRegex;
    bool                   m_bFilterCaseSensitively;
    static const UINT      FIND_DIALOG_MESSAGE;
    CFindReplaceDialog*    m_pFindDialog;
    CFont                  m_logFont;
    CString                m_sMessageBuf;
    CSplitterControl       m_wndSplitter1;
    CSplitterControl       m_wndSplitter2;
    CRect                  m_dlgOrigRect;
    CRect                  m_msgViewOrigRect;
    CRect                  m_logListOrigRect;
    CRect                  m_chgOrigRect;
    CString                m_sFilterText;
    DWORD                  m_selectedFilters;
    CDateTimeCtrl          m_dateFrom;
    CDateTimeCtrl          m_dateTo;
    __time64_t             m_tFrom;
    __time64_t             m_tTo;
    __time64_t             m_timeFromSetFromCmdLine;
    __time64_t             m_timeToSetFromCmdLine;
    int                    m_limit;
    int                    m_nSortColumn;
    bool                   m_bAscending;
    int                    m_nSortColumnPathList;
    bool                   m_bAscendingPathList;
    CRegDWORD              m_regLastStrict;
    CRegDWORD              m_regMaxBugIDColWidth;
    CButton                m_cShowPaths;
    bool                   m_bShowedAll;
    CString                m_sTitle;
    bool                   m_bSelect;
    bool                   m_bShowBugtraqColumn;
    CString                m_sLogInfo;
    std::set<svn_revnum_t> m_mergedRevs;
    SVNRev                 m_copyFromRev;
    CString                m_sMultiLogFormat;
    RECT                   m_lastTooltipRect;

    CTime      m_timFrom;
    CTime      m_timTo;
    CColors    m_colors;
    CImageList m_imgList;
    HICON      m_hModifiedIcon;
    HICON      m_hReplacedIcon;
    HICON      m_hAddedIcon;
    HICON      m_hDeletedIcon;
    HICON      m_hMergedIcon;
    HICON      m_hReverseMergedIcon;
    HICON      m_hMovedIcon;
    HICON      m_hMoveReplacedIcon;
    int        m_nIconFolder;
    int        m_nOpenIconFolder;

    HACCEL m_hAccel;

    std::unique_ptr<CStoreSelection> m_pStoreSelection;
    bool                             m_bEnsureSelection;
    CLogDataVector                   m_logEntries;
    size_t                           m_prevLogEntriesSize;

    CComPtr<ITaskbarList3> m_pTaskbarList;

    async::CJobScheduler netScheduler;
    async::CJobScheduler diskScheduler;
    async::CJobScheduler vsRunningScheduler;

    ListViewAccServer* m_pLogListAccServer;
    ListViewAccServer* m_pChangedListAccServer;

    bool m_bVisualStudioRunningAtStart;

    // MonitoringMode
    bool                                m_bMonitoringMode;
    bool                                m_bKeepHidden;
    HWND                                m_hwndToolbar;
    CImageList                          m_toolbarImages;
    CRect                               m_projTreeOrigRect;
    CSplitterControl                    m_wndSplitterLeft;
    CHintCtrl<CDragDropTreeCtrl>        m_projTree;
    CSimpleIni                          m_monitoringFile;
    volatile LONG                       m_bMonitorThreadRunning;
    std::vector<MonitorItem>            m_monitorItemListForThread;
    std::multimap<CString, MonitorItem> m_monitorItemParentPathList;
    int                                 m_nMonitorUrlIcon;
    int                                 m_nMonitorWCIcon;
    int                                 m_nErrorOvl;
    CString                             m_sMonitorNotificationTitle;
    CString                             m_sMonitorNotificationText;
    CReaderWriterLock                   m_monitorGuard;
    CReaderWriterLock                   m_monitorPathGuard;
    CString                             m_pathCurrentlyChecked;
    NOTIFYICONDATA                      m_systemTray;
    HICON                               m_hMonitorIconNormal;
    HICON                               m_hMonitorIconNewCommits;
    static const UINT                   WM_TASKBARCREATED;
    static const UINT                   WM_TSVN_COMMITMONITOR_SHOWDLGMSG;
    static const UINT                   WM_TSVN_COMMITMONITOR_RELOADINI;
    static const UINT                   WM_TASKBARBUTTON_CREATED;
    svn_revnum_t                        m_revUnread;
    bool                                m_bPlaySound;
    bool                                m_bShowNotification;
    int                                 m_defaultMonitorInterval;
    CString                             m_sMonitorMsgRegex;
    bool                                m_bSystemShutDown;
    std::vector<std::string>            m_monitorAuthorsToIgnore;
    CMonitorTreeTarget*                 m_pTreeDropTarget;
};
static UINT WM_REVSELECTED     = RegisterWindowMessage(L"TORTOISESVN_REVSELECTED_MSG");
static UINT WM_REVLIST         = RegisterWindowMessage(L"TORTOISESVN_REVLIST_MSG");
static UINT WM_REVLISTONERANGE = RegisterWindowMessage(L"TORTOISESVN_REVLISTONERANGE_MSG");
