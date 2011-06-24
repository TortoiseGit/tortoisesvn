// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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

#include "StandAloneDlg.h"
#include "TSVNPath.h"
#include "ProjectProperties.h"
#include "SVN.h"
#include "Colors.h"
#include "SVNExternals.h"
#include "..\IBugTraqProvider\IBugTraqProvider_h.h"
#include "Win7.h"

class CCmdLineParser;

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/**
 * \ingroup TortoiseProc
 * Options which can be used to configure the way the dialog box works
 */
typedef enum
{
    ProgOptNone                     = 0,
    ProgOptRecursive                = 0x01,
    ProgOptNonRecursive             = 0x00,
    /// Don't actually do the merge - just practice it
    ProgOptDryRun                   = 0x04,
    ProgOptIgnoreExternals          = 0x08,
    ProgOptKeeplocks                = 0x10,
    /// for locking this means steal the lock, for unlocking it means breaking the lock
    ProgOptForce                    = 0x20,
    ProgOptSwitchAfterCopy          = 0x40,
    ProgOptIncludeIgnored           = 0x80,
    ProgOptIgnoreAncestry           = 0x100,
    ProgOptEolDefault               = 0x200,
    ProgOptEolCRLF                  = 0x400,
    ProgOptEolLF                    = 0x800,
    ProgOptEolCR                    = 0x1000,
    ProgOptSkipConflictCheck        = 0x2000,
    ProgOptRecordOnly               = 0x4000,
    ProgOptStickyDepth              = 0x8000,
    ProgOptUseAutoprops             = 0x10000,
    ProgOptIgnoreKeywords           = 0x20000,
} ProgressOptions;

typedef enum
{
    CLOSE_MANUAL = 0,
    CLOSE_NOERRORS,
    CLOSE_NOCONFLICTS,
    CLOSE_NOMERGES,
    CLOSE_LOCAL
} ProgressCloseOptions;

#define WM_SHOWCONFLICTRESOLVER (WM_APP + 100)

/**
 * \ingroup TortoiseProc
 * Handles different Subversion commands and shows the notify messages
 * in a listbox. Since several Subversion commands have similar notify
 * messages they are grouped together in this single class.
 */
class CSVNProgressDlg : public CResizableStandAloneDialog, public SVN
{
public:
    typedef enum
    {
        SVNProgress_Add,
        SVNProgress_Checkout,
        SVNProgress_SparseCheckout,
        SVNProgress_SingleFileCheckout,
        SVNProgress_Commit,
        SVNProgress_Copy,
        SVNProgress_Export,
        SVNProgress_Import,
        SVNProgress_Lock,
        SVNProgress_Merge,
        SVNProgress_MergeReintegrate,
        SVNProgress_MergeAll,
        SVNProgress_Rename,
        SVNProgress_Resolve,
        SVNProgress_Revert,
        SVNProgress_Switch,
        SVNProgress_SwitchBackToParent,
        SVNProgress_Unlock,
        SVNProgress_Update,
    } Command;


    DECLARE_DYNAMIC(CSVNProgressDlg)

public:

    CSVNProgressDlg(CWnd* pParent = NULL);
    virtual ~CSVNProgressDlg();


    void SetCommand(Command cmd) {m_Command = cmd;}
    void SetAutoClose(const CCmdLineParser& parser);
    void SetAutoClose(DWORD ac) {m_dwCloseOnEnd = ac;}
    void SetAutoCloseLocal(BOOL bCloseLocal) {m_bCloseLocalOnEnd = bCloseLocal;}
    void SetHidden (bool hidden) {m_hidden = hidden;}
    void SetOptions(DWORD opts) {m_options = opts;}
    void SetPathList(const CTSVNPathList& pathList) {m_targetPathList = pathList;}
    void SetUrl(const CString& url) {m_url.SetFromUnknown(url);}
    void SetSecondUrl(const CString& url) {m_url2.SetFromUnknown(url);}
    void SetCommitMessage(const CString& msg) {m_sMessage = msg;}
    void SetExternals(const SVNExternals& ext) { m_externals = ext; }
    void SetPathDepths(const std::map<CString,svn_depth_t>& pathmap) { m_pathdepths = pathmap; }
    void SetRevision(const SVNRev& rev) {m_Revision = rev;}
    void SetRevisionEnd(const SVNRev& rev) {m_RevisionEnd = rev;}

    void SetDiffOptions(const CString& opts) {m_diffoptions = opts;}
    void SetDepth(svn_depth_t depth = svn_depth_unknown) {m_depth = depth;}
    void SetPegRevision(SVNRev pegrev = SVNRev()) {m_pegRev = pegrev;}
    void SetProjectProperties(ProjectProperties props) {m_ProjectProperties = props;}
    void SetChangeList(const CString& changelist, bool keepchangelist) {m_changelist = changelist; m_keepchangelist = keepchangelist;}
    void SetSelectedList(const CTSVNPathList& selPaths);
    void SetRevisionRanges(const SVNRevRangeArray& revArray) {m_revisionArray = revArray;}
    void SetBugTraqProvider(const CComPtr<IBugTraqProvider> pBugtraqProvider) { m_BugTraqProvider = pBugtraqProvider;}
    void SetRevisionProperties(const RevPropHash revProps) {m_revProps = revProps;}
    /**
     * If the number of items for which the operation is done on is known
     * beforehand, that number can be set here. It is then used to show a more
     * accurate progress bar during the operation.
     */
    void SetItemCount(INT_PTR count) {if(count) m_itemCountTotal = count;}

    bool SetBackgroundImage(UINT nID);

    bool DidErrorsOccur() {return m_bErrorsOccurred;}
    bool DidConflictsOccur() {return m_nConflicts > 0;}

    enum { IDD = IDD_SVNPROGRESS };

private:
    class NotificationData
    {
    public:
        NotificationData() :
          action((svn_wc_notify_action_t)-1),
              kind(svn_node_none),
              content_state(svn_wc_notify_state_inapplicable),
              prop_state(svn_wc_notify_state_inapplicable),
              rev(0),
              color(::GetSysColor(COLOR_WINDOWTEXT)),
              bConflictedActionItem(false),
              bTreeConflict(false),
              bAuxItem(false),
              lock_state(svn_wc_notify_lock_state_unchanged),
              bConflictSummary(false)
          {
              merge_range.end = 0;
              merge_range.start = 0;
          }
    public:
        // The text we put into the first column (the SVN action for normal items, just text for aux items)
        CString                 sActionColumnText;
        CTSVNPath               path;
        CTSVNPath               basepath;
        CTSVNPath               url;
        CString                 changelistname;
        CString                 propertyName;

        svn_wc_notify_action_t  action;
        svn_node_kind_t         kind;
        CString                 mime_type;
        svn_wc_notify_state_t   content_state;
        svn_wc_notify_state_t   prop_state;
        svn_wc_notify_lock_state_t lock_state;
        svn_merge_range_t       merge_range;
        svn_revnum_t            rev;
        COLORREF                color;
        CString                 owner;                      ///< lock owner
        bool                    bConflictedActionItem;      ///< Is this item a conflict?
        bool                    bTreeConflict;              ///< item is tree conflict
        bool                    bAuxItem;                   ///< Set if this item is not a true 'SVN action'
        bool                    bConflictSummary;           ///< if true, the entry is "one or more items are in a conflicted state"
        CString                 sPathColumnText;

    };
protected:
    //implement the virtual methods from SVN base class
    virtual BOOL Notify(const CTSVNPath& path, const CTSVNPath& url, svn_wc_notify_action_t action,
        svn_node_kind_t kind, const CString& mime_type,
        svn_wc_notify_state_t content_state,
        svn_wc_notify_state_t prop_state, LONG rev,
        const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
        const CString& changelistname,
        const CString& propertyName,
        svn_merge_range_t * range,
        svn_error_t * err, apr_pool_t * pool);
    virtual svn_wc_conflict_choice_t    ConflictResolveCallback(const svn_wc_conflict_description2_t *description, CString& mergedfile);
    virtual BOOL                        OnInitDialog();
    virtual BOOL                        Cancel();
    virtual void                        OnCancel();
    virtual BOOL                        PreTranslateMessage(MSG* pMsg);
    virtual void                        DoDataExchange(CDataExchange* pDX);

    afx_msg void    OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void    OnLvnGetdispinfoSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void    OnNMDblclkSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void    OnBnClickedLogbutton();
    afx_msg void    OnBnClickedOk();
    afx_msg void    OnBnClickedNoninteractive();
    afx_msg void    OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void    OnClose();
    afx_msg void    OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg LRESULT OnSVNProgress(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnLvnBegindragSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void    OnSize(UINT nType, int cx, int cy);
    LRESULT         OnShowConflictResolver(WPARAM, LPARAM);
    afx_msg LRESULT OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCloseOnEnd(WPARAM /*wParam*/, LPARAM /*lParam*/);
    afx_msg void    OnBnClickedRetrynohooks();

    DECLARE_MESSAGE_MAP()

    void            Sort();
    static bool     SortCompare(const NotificationData* pElem1, const NotificationData* pElem2);

    static BOOL     m_bAscending;
    static int      m_nSortedColumn;
    CStringList     m_ExtStack;

private:
    static UINT ProgressThreadEntry(LPVOID pVoid);
    UINT        ProgressThread();
    virtual void OnOK();
    void        ReportSVNError();
    void        ReportError(const CString& sError);
    void        ReportHookFailed(const CString& error);
    void        ReportWarning(const CString& sWarning);
    void        ReportNotification(const CString& sNotification);
    void        ReportCmd(const CString& sCmd);
    void        ReportString(CString sMessage, const CString& sMsgKind, COLORREF color = ::GetSysColor(COLOR_WINDOWTEXT));
    void        AddItemToList(NotificationData * data);
    CString     BuildInfoString();
    CString     GetPathFromColumnText(const CString& sColumnText);
    bool        IsCommittingToTag(CString& url);
    void        OnCommitFinished();
    bool        CheckUpdateAndRetry();

    /**
     * Resizes the columns of the progress list so that the headings are visible.
     */
    void        ResizeColumns();

    /// Predicate function to tell us if a notification data item is auxiliary or not
    static bool NotificationDataIsAux(const NotificationData* pData);

    // the commands to execute
    bool        CmdAdd(CString& sWindowTitle, bool& localoperation);
    bool        CmdCheckout(CString& sWindowTitle, bool& localoperation);
    bool        CmdSparseCheckout(CString& sWindowTitle, bool& localoperation);
    bool        CmdSingleFileCheckout(CString& sWindowTitle, bool& localoperation);
    bool        CmdCommit(CString& sWindowTitle, bool& localoperation);
    bool        CmdCopy(CString& sWindowTitle, bool& localoperation);
    bool        CmdExport(CString& sWindowTitle, bool& localoperation);
    bool        CmdImport(CString& sWindowTitle, bool& localoperation);
    bool        CmdLock(CString& sWindowTitle, bool& localoperation);
    bool        CmdMerge(CString& sWindowTitle, bool& localoperation);
    bool        CmdMergeAll(CString& sWindowTitle, bool& localoperation);
    bool        CmdMergeReintegrate(CString& sWindowTitle, bool& localoperation);
    bool        CmdRename(CString& sWindowTitle, bool& localoperation);
    bool        CmdResolve(CString& sWindowTitle, bool& localoperation);
    bool        CmdRevert(CString& sWindowTitle, bool& localoperation);
    bool        CmdSwitch(CString& sWindowTitle, bool& localoperation);
    bool        CmdSwitchBackToParent(CString& sWindowTitle, bool& localoperation);
    bool        CmdUnlock(CString& sWindowTitle, bool& localoperation);
    bool        CmdUpdate(CString& sWindowTitle, bool& localoperation);

private:
    typedef std::map<CStringA, svn_revnum_t> StringRevMap;
    typedef std::map<CString, svn_revnum_t> StringWRevMap;
    typedef std::vector<NotificationData *> NotificationDataVect;


    CString                 m_mergedfile;
    NotificationDataVect    m_arData;

    CWinThread*             m_pThread;
    volatile LONG           m_bThreadRunning;

    ProjectProperties       m_ProjectProperties;
    CListCtrl               m_ProgList;
    Command                 m_Command;
    int                     m_options;  // Use values from the ProgressOptions enum
    svn_depth_t             m_depth;
    CTSVNPathList           m_targetPathList;
    CTSVNPathList           m_selectedPaths;
    CTSVNPath               m_url;
    CTSVNPath               m_url2;
    CString                 m_sMessage;
    CString                 m_diffoptions;
    SVNRev                  m_Revision;
    SVNRev                  m_RevisionEnd;
    SVNRev                  m_pegRev;
    SVNRevRangeArray        m_revisionArray;
    CString                 m_changelist;
    bool                    m_keepchangelist;
    RevPropHash             m_revProps;
    SVNExternals            m_externals;
    std::map<CString,svn_depth_t> m_pathdepths;

    DWORD                   m_dwCloseOnEnd;
    DWORD                   m_bCloseLocalOnEnd;
    bool                    m_hidden;
    bool                    m_bRetryDone;

    CTSVNPath               m_basePath;
    StringRevMap            m_UpdateStartRevMap;
    StringRevMap            m_FinishedRevMap;

    TCHAR                   m_columnbuf[MAX_PATH];

    BOOL                    m_bCancelled;
    int                     m_nConflicts;
    bool                    m_bConflictWarningShown;
    bool                    m_bWarningShown;
    bool                    m_bErrorsOccurred;
    bool                    m_bMergesAddsDeletesOccurred;
    bool                    m_bHookError;
    bool                    m_bNoHooks;

    int                     iFirstResized;
    BOOL                    bSecondResized;
    int                     nEnsureVisibleCount;

    CString                 m_sTotalBytesTransferred;

    CColors                 m_Colors;

    bool                    m_bLockWarning;
    bool                    m_bLockExists;
    bool                    m_bFinishedItemAdded;
    bool                    m_bLastVisible;

    INT_PTR                 m_itemCount;
    INT_PTR                 m_itemCountTotal;

    bool                    m_AlwaysConflicted;

    CComPtr<IBugTraqProvider> m_BugTraqProvider;
    CComPtr<ITaskbarList3>  m_pTaskbarList;

    // some strings different methods can use
    CString                 sIgnoredIncluded;
    CString                 sExtExcluded;
    CString                 sExtIncluded;
    CString                 sIgnoreAncestry;
    CString                 sRespectAncestry;
    CString                 sDryRun;
    CString                 sRecordOnly;
    CString                 sForce;
};

static UINT WM_TASKBARBTNCREATED = RegisterWindowMessage(_T("TaskbarButtonCreated"));
static UINT TORTOISESVN_CLOSEONEND_MSG = RegisterWindowMessage(_T("TORTOISESVN_CLOSEONEND_MSG"));
