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

#include "TSVNPath.h"
#include "RepositoryBar.h"
#include "StandAloneDlg.h"
#include "ProjectProperties.h"
#include "LogDialog\LogDlg.h"
#include "HintListCtrl.h"
#include "RepositoryLister.h"

#define REPOBROWSER_CTRL_MIN_WIDTH  20
#define REPOBROWSER_FETCHTIMER      101

using namespace std;

class CInputLogDlg;
class CTreeDropTarget;
class CListDropTarget;
class CRepositoryBrowserSelection;


/**
 * \ingroup TortoiseProc
 * helper class which holds the information for a tree item
 * in the repository browser.
 */
class CTreeItem
{
public:
    CTreeItem()
        : children_fetched(false)
        , has_child_folders(false)
        , is_external(false)
        , kind(svn_node_unknown)
    {
    }

    CString         unescapedname;
    SRepositoryInfo repository;
    CString         url;                        ///< escaped URL
    CString         logicalPath;                ///< concatenated unescapedname values
    bool            is_external;                ///< if set, several operations may not be available
    bool            children_fetched;           ///< whether the contents of the folder are known/fetched or not
    bool            has_child_folders;
    deque<CItem>    children;
    CString         error;
    svn_node_kind_t kind;
};


/**
 * \ingroup TortoiseProc
 * Dialog to browse a repository.
 */
class CRepositoryBrowser : public CResizableStandAloneDialog, public SVN, public IRepo
{
    DECLARE_DYNAMIC(CRepositoryBrowser)
friend class CBaseDropTarget;
friend class CTreeDropTarget;
friend class CListDropTarget;

public:
    CRepositoryBrowser(const CString& url, const SVNRev& rev);                  ///< standalone repository browser
    CRepositoryBrowser(const CString& url, const SVNRev& rev, CWnd* pParent);   ///< dependent repository browser
    virtual ~CRepositoryBrowser();

    /// Returns the currently displayed revision only (for convenience)
    SVNRev GetRevision() const;
    /// Returns the currently displayed URL's path only (for convenience)
    CString GetPath() const;
    /// Returns the paths currently selected in the tree view / list view (for convenience)
    const CString& GetSelectedURLs() const;

    /// switches to the \c url at \c rev. If the url is valid and exists,
    /// the repository browser will show the content of that url.
    bool ChangeToUrl(CString& url, SVNRev& rev, bool bAlreadyChecked);

    CString GetRepoRoot() { return m_repository.root; }
    std::map<CString,svn_depth_t> GetCheckoutDepths() { return m_checkoutDepths; }
    std::map<CString,svn_depth_t> GetUpdateDepths() { return m_updateDepths; }

    void OnCbenDragbeginUrlcombo(NMHDR *pNMHDR, LRESULT *pResult);

    void SetSparseCheckoutMode() { m_bSparseCheckoutMode = true; m_bStandAlone = false; }

    /// overwrite SVN callbacks
    virtual BOOL Cancel();

    enum 
    { 
        IDD = IDD_REPOSITORY_BROWSER,
        WM_REFRESHURL = WM_USER + 10
    };

    /// the project properties if the repository browser was started from a working copy
    ProjectProperties m_ProjectProperties;
    /// the local path of the working copy
    CTSVNPath m_path;
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    afx_msg void OnBnClickedHelp();
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnTvnSelchangedRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnItemexpandingRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnItemChangedRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMClickRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnKeydownRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMDblclkRepolist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnHdnItemclickRepolist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnItemchangedRepolist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnBegindragRepolist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnBeginrdragRepolist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnBegindragRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnBeginrdragRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void OnLvnBeginlabeleditRepolist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnEndlabeleditRepolist(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnBeginlabeleditRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnEndlabeleditRepotree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnCaptureChanged(CWnd *pWnd);

    afx_msg void OnUrlFocus();
    afx_msg void OnCopy();
    afx_msg void OnInlineedit();
    afx_msg void OnRefresh();
    afx_msg void OnDelete();
    afx_msg void OnGoUp();

    DECLARE_MESSAGE_MAP()

    /// initializes variables for the constructors
    void ConstructorInit(const SVNRev& rev);
    /// called after the init thread has finished
    LRESULT OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/);
    /// called to update the tree node for a specific URL
    LRESULT OnRefreshURL(WPARAM /*wParam*/, LPARAM lParam);
    /// draws the bar when the tree and list control are resized
    void DrawXorBar(CDC * pDC, int x1, int y1, int width, int height);

    /// recursively removes all items from \c hItem on downwards.
    void RecursiveRemove(HTREEITEM hItem, bool bChildrenOnly = false);
    /// remove all tree nodes and empty the list view
    void ClearUI();
    /// searches the tree item for the specified \c fullurl.
    HTREEITEM FindUrl(const CString& fullurl);
    /// searches the tree item for the specified \c fullurl.
    HTREEITEM FindUrl(const CString& fullurl, const CString& url, HTREEITEM hItem = TVI_ROOT);

    /// read list of children; including externals
    void FetchChildren (HTREEITEM node);

    /// Find and return the node that corresponds to the specified
    /// logical \ref path. Add such node (including parents) if it
    /// does not exist, yet.
    HTREEITEM AutoInsert (const CString& path);
    /// Find and return the sub-node to \ref hParent that follows
    /// the spec in \ref item. Add such sub-node if it does not
    /// exist, yet.
    HTREEITEM AutoInsert (HTREEITEM hParent, const CItem& item);
    /// Like the previous version but inserts more than one item.
    void AutoInsert (HTREEITEM hParent, const std::deque<CItem>& items);
    /// Actual sub-node creation
    HTREEITEM Insert (HTREEITEM hParent, CTreeItem* parentTreeItem, const CItem& item);
    /// Sort tree sub-nodes
    void Sort (HTREEITEM parent);

    void RefreshChildren(HTREEITEM node);
    /**
     * Refetches the information for \c hNode. If \c force is true, then the list
     * control is refilled again.
     */
    bool RefreshNode(HTREEITEM hNode, bool force = false);
    /// Fills the list control with all the children of \c pTreeItem.
    void FillList(CTreeItem * pTreeItem);
    /// Open / enter folder for entry number \ref item
    void OpenFromList (int item);
    /// Open the file in the default application
    void OpenFile(const CTSVNPath& url, const CTSVNPath& urlEscaped, bool bOpenWith);
    /// C/o & lock the file and open it in the default application for modification
    void EditFile(CTSVNPath url, CTSVNPath urlEscaped);
    /// Sets the sort arrow in the list view header according to the currently used sorting.
    void SetSortArrow();
    /// compares strings for sorting
    static int SortStrCmp(PCWSTR str1, PCWSTR str2);
    /// called when a drag-n-drop operation starts
    void OnBeginDrag(NMHDR *pNMHDR);
    void OnBeginDragTree(NMHDR *pNMHDR);
    /// called when a drag-n-drop operation ends and the user dropped something on us.
    bool OnDrop(const CTSVNPath& target, const CString& root, const CTSVNPathList& pathlist, const SVNRev& srcRev, DWORD dwEffect, POINTL pt);
    /**
     * Since all urls we store and use are not properly escaped but "UI friendly", this
     * method converts those urls to a properly escaped url which we can use in
     * Subversion API calls.
     */
    CString EscapeUrl(const CTSVNPath& url);
    /// Initializes the repository browser with a new root url
    void InitRepo();
    /// Helper function to show the "File Save" dialog
    bool AskForSavePath (const CRepositoryBrowserSelection& selection, CTSVNPath &tempfile, bool bFolder);

    /// Saves the column widths
    void SaveColumnWidths(bool bSaveToRegistry = false);
    /// converts a string to an array of column widths
    bool StringToWidthArray(const CString& WidthString, int WidthArray[]);
    /// converts an array of column widths to a string
    CString WidthArrayToString(int WidthArray[]);
    void SetRightDrag(bool bRightDrag) {m_bRightDrag = bRightDrag;}

    /// remove items for the associated URL sub-tree
    /// from the \ref m_lister cache.
    void InvalidateData (HTREEITEM node);
    void InvalidateData (HTREEITEM node, const SVNRev& revision);

    /// assume that the selected urls may have become invalid
    /// and reset the the cache accordingly
    void InvalidateDataParents (const CRepositoryBrowserSelection& selection);

    static UINT InitThreadEntry(LPVOID pVoid);
    UINT InitThread();

    static int CALLBACK TreeSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);
    static int CALLBACK ListSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);

    void BeginDrag(const CWnd& window, CRepositoryBrowserSelection& selection,
        POINT& point, bool setAsyncMode);

    /// extract info from controls before they get destroyed
    void StoreSelectedURLs();

    /// resizes the control so that the divider is at position 'point'
    void HandleDividerMove(CPoint point, bool bDraw);
    bool CheckoutDepthForItem( HTREEITEM hItem );
    void CheckTreeItem( HTREEITEM hItem );
    void HandleCheckedItemForXP( HTREEITEM item );
    bool CheckAndConfirmPath(const CTSVNPath& path);
    void SaveDividerPosition();

protected:
    bool                m_bInitDone;
    CRepositoryBar      m_barRepository;
    CRepositoryBarCnr   m_cnrRepositoryBar;

    CTreeCtrl           m_RepoTree;
    CHintListCtrl       m_RepoList;

    SRepositoryInfo     m_repository;

    HACCEL              m_hAccel;

private:
    bool                m_cancelled;

    bool                m_bStandAlone;
    bool                m_bSparseCheckoutMode;
    CString             m_InitialUrl;
    CString             m_selectedURLs; ///< only valid after <OK>
    bool                m_bThreadRunning;
    static const UINT   m_AfterInitMessage;

    int                 m_nIconFolder;
    int                 m_nOpenIconFolder;
    int                 m_nExternalOvl;

    volatile bool       m_blockEvents;

    static bool         s_bSortLogical;
    bool                m_bSortAscending;
    int                 m_nSortedColumn;
    int                 m_arColumnWidths[7];
    int                 m_arColumnAutoWidths[7];

    CTreeDropTarget *   m_pTreeDropTarget;
    CListDropTarget *   m_pListDropTarget;
    bool                m_bRightDrag;

    int                 oldy, oldx;
    bool                bDragMode;

    svn_node_kind_t     m_diffKind;
    CTSVNPath           m_diffURL;

    CString             m_origDlgTitle;

    CRepositoryLister   m_lister;
    std::map<CString,svn_depth_t> m_checkoutDepths;
    std::map<CString,svn_depth_t> m_updateDepths;

    /// used to execute user ops (e.g. context menu actions) in the background
    async::CJobScheduler m_backgroundJobs;
};

static UINT WM_AFTERINIT = RegisterWindowMessage(_T("TORTOISESVN_AFTERINIT_MSG"));

