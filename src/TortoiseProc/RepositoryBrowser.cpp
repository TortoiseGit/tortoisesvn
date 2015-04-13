// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015 - TortoiseSVN

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

#include "InputLogDlg.h"
#include "PropDlg.h"
#include "Properties/EditPropertiesDlg.h"
#include "Blame.h"
#include "BlameDlg.h"
#include "WaitCursorEx.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "RenameDlg.h"
#include "RevisionGraph/RevisionGraphDlg.h"
#include "CheckoutDlg.h"
#include "ExportDlg.h"
#include "SVNProgressDlg.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "BrowseFolder.h"
#include "SVNDiff.h"
#include "SysImageList.h"
#include "RepoDrags.h"
#include "SVNInfo.h"
#include "SVNDataObject.h"
#include "SVNLogHelper.h"
#include "IconMenu.h"
#include <Shlwapi.h>
#include "RepositoryBrowserSelection.h"
#include "Commands/EditFileCommand.h"
#include "AsyncCall.h"
#include "DiffOptionsDlg.h"
#include "Callback.h"
#include "SVNStatus.h"

#include <fstream>
#include <sstream>
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")

#define OVERLAY_EXTERNAL        1

static const apr_uint32_t direntAllExceptHasProps = (SVN_DIRENT_KIND | SVN_DIRENT_SIZE | SVN_DIRENT_CREATED_REV | SVN_DIRENT_TIME | SVN_DIRENT_LAST_AUTHOR);


bool CRepositoryBrowser::s_bSortLogical = true;

enum RepoBrowserContextMenuCommands
{
    // needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
    ID_OPEN = 1,
    ID_OPENWITH,
    ID_EDITFILE,
    ID_SHOWLOG,
    ID_REVGRAPH,
    ID_BLAME,
    ID_VIEWREV,
    ID_VIEWPATHREV,
    ID_EXPORT,
    ID_CHECKOUT,
    ID_REFRESH,
    ID_SAVEAS,
    ID_MKDIR,
    ID_IMPORT,
    ID_IMPORTFOLDER,
    ID_BREAKLOCK,
    ID_DELETE,
    ID_RENAME,
    ID_COPYTOWC,
    ID_COPYTO,
    ID_FULLTOCLIPBOARD,
    ID_URLTOCLIPBOARD,
    ID_NAMETOCLIPBOARD,
    ID_AUTHORTOCLIPBOARD,
    ID_REVISIONTOCLIPBOARD,
    ID_PROPS,
    ID_REVPROPS,
    ID_GNUDIFF,
    ID_DIFF,
    ID_DIFF_CONTENTONLY,
    ID_PREPAREDIFF,
    ID_UPDATE,
    ID_CREATELINK,
    ID_ADDTOBOOKMARKS,
    ID_REMOVEBOOKMARKS,
};

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableStandAloneDialog)

CRepositoryBrowser::CRepositoryBrowser(const CString& url, const SVNRev& rev)
    : CResizableStandAloneDialog(CRepositoryBrowser::IDD, NULL)
    , m_cnrRepositoryBar(&m_barRepository)
    , m_bStandAlone(true)
    , m_bSparseCheckoutMode(false)
    , m_InitialUrl(url)
    , m_bInitDone(false)
    , m_blockEvents(0)
    , m_bSortAscending(true)
    , m_nSortedColumn(0)
    , m_pTreeDropTarget(NULL)
    , m_pListDropTarget(NULL)
    , m_diffKind(svn_node_none)
    , m_hAccel(NULL)
    , m_cancelled(false)
    , bDragMode(FALSE)
    , m_backgroundJobs(0, 1, true)
    , m_pListCtrlTreeItem(nullptr)
    , m_bThreadRunning(false)
    , m_nIconFolder(0)
    , m_nOpenIconFolder(0)
    , m_nExternalOvl(0)
    , m_nSVNParentPath(0)
    , m_bRightDrag(false)
    , oldy(0)
    , oldx(0)
    , m_nBookmarksIcon(0)
    , m_bTrySVNParentPath(true)
{
    ConstructorInit(rev);
}

CRepositoryBrowser::CRepositoryBrowser(const CString& url, const SVNRev& rev, CWnd* pParent)
    : CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
    , m_cnrRepositoryBar(&m_barRepository)
    , m_bStandAlone(false)
    , m_bSparseCheckoutMode(false)
    , m_InitialUrl(url)
    , m_bInitDone(false)
    , m_blockEvents(0)
    , m_bSortAscending(true)
    , m_nSortedColumn(0)
    , m_pTreeDropTarget(NULL)
    , m_pListDropTarget(NULL)
    , m_diffKind(svn_node_none)
    , m_hAccel(NULL)
    , m_cancelled(false)
    , bDragMode(FALSE)
    , m_backgroundJobs(0, 1, true)
    , m_pListCtrlTreeItem(nullptr)
    , m_bThreadRunning(false)
    , m_nIconFolder(0)
    , m_nOpenIconFolder(0)
    , m_nExternalOvl(0)
    , m_nSVNParentPath(0)
    , m_bRightDrag(false)
    , oldy(0)
    , oldx(0)
    , m_nBookmarksIcon(0)
    , m_bTrySVNParentPath(true)
{
    ConstructorInit(rev);
}


void CRepositoryBrowser::ConstructorInit(const SVNRev& rev)
{
    SecureZeroMemory(&m_arColumnWidths, sizeof(m_arColumnWidths));
    SecureZeroMemory(&m_arColumnAutoWidths, sizeof(m_arColumnAutoWidths));
    m_repository.revision = rev;
    s_bSortLogical   = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
    if (s_bSortLogical)
        s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
    std::fill_n(m_arColumnWidths, _countof(m_arColumnWidths), 0);
    m_bFetchChildren    = false;
    m_bShowExternals    = !!CRegDWORD(L"Software\\TortoiseSVN\\RepoBrowserShowExternals", true);
    m_bShowLocks        = !!CRegDWORD(L"Software\\TortoiseSVN\\RepoBrowserShowLocks", true);
    m_bTrySVNParentPath = !!CRegDWORD(L"Software\\TortoiseSVN\\RepoBrowserTrySVNParentPath", true);
    LoadBookmarks();
}

CRepositoryBrowser::~CRepositoryBrowser()
{
    SaveBookmarks();
}

void CRepositoryBrowser::RecursiveRemove(HTREEITEM hItem, bool bChildrenOnly /* = false */)
{
    // remove nodes from tree view

    CAutoWriteLock locker(m_guard);
    if (m_RepoTree.ItemHasChildren(hItem))
    {
        HTREEITEM childItem;
        for (childItem = m_RepoTree.GetChildItem(hItem);childItem != NULL;)
        {
            RecursiveRemove(childItem);
            if (bChildrenOnly)
            {
                CTreeItem * pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(childItem);
                if (pTreeItem && !pTreeItem->bookmark && (m_pListCtrlTreeItem == pTreeItem))
                {
                    m_RepoList.DeleteAllItems();
                    m_pListCtrlTreeItem = nullptr;
                }
                delete pTreeItem;
                m_RepoTree.SetItemData(childItem, 0);
                HTREEITEM hDelItem = childItem;
                childItem = m_RepoTree.GetNextItem(childItem, TVGN_NEXT);
                m_RepoTree.DeleteItem(hDelItem);
            }
            else
                childItem = m_RepoTree.GetNextItem(childItem, TVGN_NEXT);
        }
    }

    if ((hItem)&&(!bChildrenOnly))
    {
        CTreeItem * pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(hItem);
        if (pTreeItem && !pTreeItem->bookmark && (m_pListCtrlTreeItem == pTreeItem))
        {
            m_RepoList.DeleteAllItems();
            m_pListCtrlTreeItem = nullptr;
        }
        delete pTreeItem;
        m_RepoTree.SetItemData(hItem, 0);
    }
}

void CRepositoryBrowser::ClearUI()
{
    CAutoWriteLock locker(m_guard);
    m_RepoList.DeleteAllItems();

    HTREEITEM hRoot = m_RepoTree.GetRootItem();
    do
    {
        RecursiveRemove(hRoot);
        m_RepoTree.DeleteItem(hRoot);
        hRoot = m_RepoTree.GetRootItem();
    } while (hRoot);

    m_RepoTree.DeleteAllItems();
}

BOOL CRepositoryBrowser::Cancel()
{
    return m_cancelled;
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_REPOTREE, m_RepoTree);
    DDX_Control(pDX, IDC_REPOLIST, m_RepoList);
}

BEGIN_MESSAGE_MAP(CRepositoryBrowser, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_WM_SETCURSOR()
    ON_REGISTERED_MESSAGE(WM_AFTERINIT, OnAfterInitDialog)
    ON_MESSAGE(WM_REFRESHURL, OnRefreshURL)
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_NOTIFY(TVN_SELCHANGED, IDC_REPOTREE, &CRepositoryBrowser::OnTvnSelchangedRepotree)
    ON_NOTIFY(TVN_ITEMEXPANDING, IDC_REPOTREE, &CRepositoryBrowser::OnTvnItemexpandingRepotree)
    ON_NOTIFY(NM_DBLCLK, IDC_REPOLIST, &CRepositoryBrowser::OnNMDblclkRepolist)
    ON_NOTIFY(HDN_ITEMCLICK, 0, &CRepositoryBrowser::OnHdnItemclickRepolist)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_REPOLIST, &CRepositoryBrowser::OnLvnItemchangedRepolist)
    ON_NOTIFY(LVN_BEGINDRAG, IDC_REPOLIST, &CRepositoryBrowser::OnLvnBegindragRepolist)
    ON_NOTIFY(LVN_BEGINRDRAG, IDC_REPOLIST, &CRepositoryBrowser::OnLvnBeginrdragRepolist)
    ON_WM_CONTEXTMENU()
    ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_REPOLIST, &CRepositoryBrowser::OnLvnBeginlabeleditRepolist)
    ON_NOTIFY(LVN_ENDLABELEDIT, IDC_REPOLIST, &CRepositoryBrowser::OnLvnEndlabeleditRepolist)
    ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_REPOTREE, &CRepositoryBrowser::OnTvnBeginlabeleditRepotree)
    ON_NOTIFY(TVN_ENDLABELEDIT, IDC_REPOTREE, &CRepositoryBrowser::OnTvnEndlabeleditRepotree)
    ON_WM_TIMER()
    ON_COMMAND(ID_URL_FOCUS, &CRepositoryBrowser::OnUrlFocus)
    ON_COMMAND(ID_EDIT_COPY, &CRepositoryBrowser::OnCopy)
    ON_COMMAND(ID_INLINEEDIT, &CRepositoryBrowser::OnInlineedit)
    ON_COMMAND(ID_REFRESHBROWSER, &CRepositoryBrowser::OnRefresh)
    ON_COMMAND(ID_DELETEBROWSERITEM, &CRepositoryBrowser::OnDelete)
    ON_COMMAND(ID_URL_UP, &CRepositoryBrowser::OnGoUp)
    ON_NOTIFY(TVN_BEGINDRAG, IDC_REPOTREE, &CRepositoryBrowser::OnTvnBegindragRepotree)
    ON_NOTIFY(TVN_BEGINRDRAG, IDC_REPOTREE, &CRepositoryBrowser::OnTvnBeginrdragRepotree)
    ON_NOTIFY(TVN_ITEMCHANGED, IDC_REPOTREE, &CRepositoryBrowser::OnTvnItemChangedRepotree)
    ON_WM_CAPTURECHANGED()
    ON_REGISTERED_MESSAGE(WM_SVNAUTHCANCELLED, OnAuthCancelled)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_REPOLIST, &CRepositoryBrowser::OnNMCustomdrawRepolist)
    ON_COMMAND(ID_URL_HISTORY_BACK, &CRepositoryBrowser::OnUrlHistoryBack)
    ON_COMMAND(ID_URL_HISTORY_FORWARD, &CRepositoryBrowser::OnUrlHistoryForward)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_REPOTREE, &CRepositoryBrowser::OnNMCustomdrawRepotree)
    ON_NOTIFY(TVN_ITEMCHANGING, IDC_REPOTREE, &CRepositoryBrowser::OnTvnItemChangingRepotree)
    ON_NOTIFY(NM_SETCURSOR, IDC_REPOTREE, &CRepositoryBrowser::OnNMSetCursorRepotree)
END_MESSAGE_MAP()

SVNRev CRepositoryBrowser::GetRevision() const
{
    return m_barRepository.GetCurrentRev();
}

CString CRepositoryBrowser::GetPath() const
{
    return m_barRepository.GetCurrentUrl();
}

BOOL CRepositoryBrowser::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_REPOS_BAR_CNR, IDC_REPOS_BAR_CNR, IDC_REPOS_BAR_CNR, IDC_REPOTREE);
    m_aeroControls.SubclassControl(this, IDC_INFOLABEL);
    m_aeroControls.SubclassOkCancelHelp(this);

    GetWindowText(m_origDlgTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), m_origDlgTitle);

    m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_REPOBROWSER));

    m_nExternalOvl = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_EXTERNALOVL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    if (m_nExternalOvl >= 0)
        SYS_IMAGE_LIST().SetOverlayImage(m_nExternalOvl, OVERLAY_EXTERNAL);
    m_nSVNParentPath = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CACHE), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));

    m_cnrRepositoryBar.SubclassDlgItem(IDC_REPOS_BAR_CNR, this);
    m_barRepository.Create(&m_cnrRepositoryBar, 12345);
    m_barRepository.SetIRepo(this);
    if (m_bSparseCheckoutMode)
    {
        m_cnrRepositoryBar.ShowWindow(SW_HIDE);
        m_barRepository.ShowWindow(SW_HIDE);
        SetWindowLongPtr(m_RepoTree.GetSafeHwnd(), GWL_STYLE, GetWindowLongPtr(m_RepoTree.GetSafeHwnd(), GWL_STYLE) | TVS_CHECKBOXES);
    }
    else
    {
        m_pTreeDropTarget = new CTreeDropTarget(this);
        RegisterDragDrop(m_RepoTree.GetSafeHwnd(), m_pTreeDropTarget);
        // create the supported formats:
        FORMATETC ftetc={0};
        ftetc.cfFormat = CF_SVNURL;
        ftetc.dwAspect = DVASPECT_CONTENT;
        ftetc.lindex = -1;
        ftetc.tymed = TYMED_HGLOBAL;
        m_pTreeDropTarget->AddSuportedFormat(ftetc);
        ftetc.cfFormat = CF_HDROP;
        m_pTreeDropTarget->AddSuportedFormat(ftetc);
        ftetc.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DROPDESCRIPTION);
        m_pTreeDropTarget->AddSuportedFormat(ftetc);

        m_pListDropTarget = new CListDropTarget(this);
        RegisterDragDrop(m_RepoList.GetSafeHwnd(), m_pListDropTarget);
        // create the supported formats:
        ftetc.cfFormat = CF_SVNURL;
        m_pListDropTarget->AddSuportedFormat(ftetc);
        ftetc.cfFormat = CF_HDROP;
        m_pListDropTarget->AddSuportedFormat(ftetc);
        ftetc.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DROPDESCRIPTION);
        m_pListDropTarget->AddSuportedFormat(ftetc);
    }

    if (m_bStandAlone)
    {
        // reposition the buttons
        CRect rect_cancel;
        GetDlgItem(IDCANCEL)->GetWindowRect(rect_cancel);
        GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
        ScreenToClient(rect_cancel);
        GetDlgItem(IDOK)->MoveWindow(rect_cancel);
        CRect inforect;
        GetDlgItem(IDC_INFOLABEL)->GetWindowRect(inforect);
        inforect.right += rect_cancel.Width();
        ScreenToClient(inforect);
        GetDlgItem(IDC_INFOLABEL)->MoveWindow(inforect);
    }

    m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
    m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();

    if (m_bSparseCheckoutMode)
    {
        m_RepoList.ShowWindow(SW_HIDE);
    }
    else
    {
        // set up the list control
        // set the extended style of the list control
        // the style LVS_EX_FULLROWSELECT interferes with the background watermark image but it's more important to be able to select in the whole row.
        CRegDWORD regFullRowSelect(L"Software\\TortoiseSVN\\FullRowSelect", TRUE);
        DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
        if (DWORD(regFullRowSelect))
            exStyle |= LVS_EX_FULLROWSELECT;
        m_RepoList.SetExtendedStyle(exStyle);
        m_RepoList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
        ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_INITWAIT)));
    }
    m_nBookmarksIcon = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_BOOKMARKS), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    m_RepoTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);
    // TVS_EX_FADEINOUTEXPANDOS style must not be set:
    // if it is set, there's a UI glitch when editing labels:
    // the text vanishes if the mouse cursor is moved away from
    // the edit control
    DWORD exStyle = TVS_EX_AUTOHSCROLL | TVS_EX_DOUBLEBUFFER;
    if (m_bSparseCheckoutMode)
        exStyle |= TVS_EX_MULTISELECT;
    m_RepoTree.SetExtendedStyle(exStyle, exStyle);

    SetWindowTheme(m_RepoList.GetSafeHwnd(), L"Explorer", NULL);
    SetWindowTheme(m_RepoTree.GetSafeHwnd(), L"Explorer", NULL);

    AddAnchor(IDC_REPOS_BAR_CNR, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
    AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDC_INFOLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    EnableSaveRestore(L"RepositoryBrowser");
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));

    DWORD xPos = CRegDWORD(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\RepobrowserDivider");
    bDragMode = true;
    if (!m_bSparseCheckoutMode)
    {
        // 10 dialog units: 6 units left border of the tree control, 4 units between
        // the tree and the list control
        CRect drc(0, 0, 10, 10);
        MapDialogRect(&drc);
        HandleDividerMove(CPoint(xPos + drc.right, 10), false);
    }
    else
    {
        // have the tree control use the whole space of the dialog
        RemoveAnchor(IDC_REPOTREE);
        RECT rctree;
        RECT rcbar;
        RECT rc;
        m_RepoTree.GetWindowRect(&rctree);
        m_barRepository.GetWindowRect(&rcbar);
        UnionRect(&rc, &rcbar, &rctree);
        ScreenToClient(&rc);
        m_RepoTree.MoveWindow(&rc, FALSE);
        AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
    }
    SetPromptParentWindow(m_hWnd);
    m_bThreadRunning = true;
    if (AfxBeginThread(InitThreadEntry, this)==NULL)
    {
        m_bThreadRunning = false;
        OnCantStartThread();
    }
    m_barRepository.SetFocusToURL();
    return FALSE;
}

void CRepositoryBrowser::InitRepo()
{
    CWaitCursorEx wait;

    // stop running requests
    // (reduce time needed to change the repository or revision)

    m_lister.Cancel();

    if (m_bSparseCheckoutMode && !m_wcPath.IsEmpty())
    {
        new async::CAsyncCall ( this
            , &CRepositoryBrowser::GetStatus
            , &m_backgroundJobs);
    }

    // repository properties

    m_InitialUrl = CPathUtils::PathUnescape(m_InitialUrl);
    int questionMarkIndex = -1;
    if ((questionMarkIndex = m_InitialUrl.Find('?'))>=0)
    {
        // url can be in the form
        // http://host/repos/path?[p=PEG][&r=REV]
        CString revString = m_InitialUrl.Mid(questionMarkIndex+1);
        CString pegRevString = revString;

        int ampPos = revString.Find('&');
        if (ampPos >= 0)
        {
            revString = revString.Mid (ampPos+1);
            pegRevString = pegRevString.Left (ampPos);
        }

        pegRevString.Trim(L"p=");
        if (!pegRevString.IsEmpty())
            m_repository.peg_revision = SVNRev(pegRevString);

        revString.Trim(L"r=");
        if (!revString.IsEmpty())
            m_repository.revision = SVNRev(revString);

        m_InitialUrl = m_InitialUrl.Left(questionMarkIndex);
    }

    if (!svn_dirent_is_absolute(CUnicodeUtils::GetUTF8(m_InitialUrl)) &&
        !svn_path_is_url(CUnicodeUtils::GetUTF8(m_InitialUrl)))
    {
        CString sError;
        sError.Format(IDS_ERR_MUSTBEURLORPATH, (LPCTSTR)m_InitialUrl);
        ShowText(sError, true);
        m_InitialUrl.Empty();
        return;
    }
    // Since an url passed here isn't necessarily the real/final url, for
    // example in case of redirects or if the urls is a putty session name,
    // we have to first find out the real url here and store that real/final
    // url in m_InitialUrl.
    SVNInfo inf;
    const SVNInfoData * pInfData = inf.GetFirstFileInfo(CTSVNPath(m_InitialUrl), m_repository.peg_revision.IsValid() ? m_repository.peg_revision : m_repository.revision, m_repository.revision);
    if (pInfData)
    {
        m_repository.root = CPathUtils::PathUnescape(pInfData->reposRoot);
        m_repository.uuid = pInfData->reposUUID;
        m_InitialUrl = CPathUtils::PathUnescape(pInfData->url);
    }
    else
    {
        if ((m_InitialUrl.Left(7) == L"http://")||(m_InitialUrl.Left(8) == L"https://"))
        {
            if (TrySVNParentPath())
                return;
        }
        m_repository.root = CPathUtils::PathUnescape(GetRepositoryRootAndUUID (CTSVNPath (m_InitialUrl), true, m_repository.uuid));
    }

    // problem: SVN reports the repository root without the port number if it's
    // the default port!
    m_InitialUrl += L"/";
    if (m_InitialUrl.Left(6) == L"svn://")
        m_InitialUrl.Replace(L":3690/", L"/");
    if (m_InitialUrl.Left(7) == L"http://")
        m_InitialUrl.Replace(L":80/", L"/");
    if (m_InitialUrl.Left(8) == L"https://")
        m_InitialUrl.Replace(L":443/", L"/");
    m_InitialUrl.TrimRight('/');

    m_backgroundJobs.WaitForEmptyQueue();

    // (try to) fetch the HEAD revision

    svn_revnum_t headRevision = GetHEADRevision (CTSVNPath (m_InitialUrl), false);

    // let's see whether the URL was a directory

    CString userCancelledError;
    userCancelledError.LoadStringW (IDS_SVN_USERCANCELLED);

    SVNRev pegRev = m_repository.peg_revision.IsValid() ? m_repository.peg_revision : m_repository.revision;

    std::deque<CItem> dummy;
    CString redirectedUrl;
    CString error
        = m_cancelled
        ? userCancelledError
        : m_lister.GetList (m_InitialUrl, pegRev, m_repository, 0, false, dummy, redirectedUrl);

    if (!redirectedUrl.IsEmpty() && svn_path_is_url(CUnicodeUtils::GetUTF8(m_InitialUrl)))
        m_InitialUrl = CPathUtils::PathUnescape(redirectedUrl);

    // the only way CQuery::List will return the following error
    // is by calling it with a file path instead of a dir path

    CString wasFileError;
    wasFileError.LoadStringW (IDS_ERR_ERROR);

    // maybe, we already know that this was a (valid) file path

    if (error == wasFileError)
    {
        m_InitialUrl = m_InitialUrl.Left (m_InitialUrl.ReverseFind ('/'));
        error = m_lister.GetList (m_InitialUrl, pegRev, m_repository, 0, false, dummy, redirectedUrl);
    }

    // exit upon cancel

    if (m_cancelled)
    {
        m_InitialUrl.Empty();
        ShowText(error, true);
        return;
    }

    // let's (try to) access all levels in the folder path
    if (!m_repository.root.IsEmpty())
    {
        for ( CString path = m_InitialUrl
            ; path.GetLength() >= m_repository.root.GetLength()
            ; path = path.Left (path.ReverseFind ('/')))
        {
            m_lister.Enqueue (path, pegRev, m_repository, 0, !m_bSparseCheckoutMode && m_bShowExternals);
        }
    }

    // did our optimistic strategy work?

    if (error.IsEmpty() && (headRevision >= 0))
    {
        // optimistic strategy was successful

        if (m_repository.revision.IsHead())
            m_barRepository.SetHeadRevision (headRevision);
    }
    else
    {
        // We don't know if the url passed to us points to a file or a folder,
        // let's find out:
        SVNInfo info;
        const SVNInfoData * data = NULL;
        do
        {
            data = info.GetFirstFileInfo ( CTSVNPath(m_InitialUrl)
                                         , m_repository.revision
                                         , m_repository.revision);
            if ((data == NULL)||(data->kind != svn_node_dir))
            {
                // in case the url is not a valid directory, try the parent dir
                // until there's no more parent dir
                m_InitialUrl = m_InitialUrl.Left(m_InitialUrl.ReverseFind('/'));
                if ((m_InitialUrl.Compare(L"http://") == 0) ||
                    (m_InitialUrl.Compare(L"http:/") == 0)||
                    (m_InitialUrl.Compare(L"https://") == 0)||
                    (m_InitialUrl.Compare(L"https:/") == 0)||
                    (m_InitialUrl.Compare(L"svn://") == 0)||
                    (m_InitialUrl.Compare(L"svn:/") == 0)||
                    (m_InitialUrl.Compare(L"svn+ssh:/") == 0)||
                    (m_InitialUrl.Compare(L"svn+ssh://") == 0)||
                    (m_InitialUrl.Compare(L"file:///") == 0)||
                    (m_InitialUrl.Compare(L"file://") == 0)||
                    (m_InitialUrl.Compare(L"file:/") == 0))
                {
                    m_InitialUrl.Empty();
                }
                if (error.IsEmpty())
                {
                    if (((data)&&(data->kind == svn_node_dir))||(data == NULL))
                        error = info.GetLastErrorMessage();
                }
            }
        } while(!m_InitialUrl.IsEmpty() && ((data == NULL) || (data->kind != svn_node_dir)));

        if (data == NULL)
        {
            m_InitialUrl.Empty();
            ShowText(error, true);
            return;
        }
        else if (m_repository.revision.IsHead())
        {
            m_barRepository.SetHeadRevision(data->rev);
        }

        m_repository.root = CPathUtils::PathUnescape(data->reposRoot);
        m_repository.uuid = data->reposUUID;
    }

    m_InitialUrl.TrimRight('/');

    // the initial url can be in the format file:///\, but the
    // repository root returned would still be file://
    // to avoid string length comparison faults, we adjust
    // the repository root here to match the initial url
    if ((m_InitialUrl.Left(9).CompareNoCase(L"file:///\\") == 0) &&
        (m_repository.root.Left(9).CompareNoCase(L"file:///\\") != 0))
        m_repository.root.Replace(L"file://", L"file:///\\");

    // now check the repository root for the url type, then
    // set the corresponding background image
    if (!m_repository.root.IsEmpty())
    {
        UINT nID = IDI_REPO_UNKNOWN;
        if (m_repository.root.Left(7).CompareNoCase(L"http://")==0)
            nID = IDI_REPO_HTTP;
        if (m_repository.root.Left(8).CompareNoCase(L"https://")==0)
            nID = IDI_REPO_HTTPS;
        if (m_repository.root.Left(6).CompareNoCase(L"svn://")==0)
            nID = IDI_REPO_SVN;
        if (m_repository.root.Left(10).CompareNoCase(L"svn+ssh://")==0)
            nID = IDI_REPO_SVNSSH;
        if (m_repository.root.Left(7).CompareNoCase(L"file://")==0)
            nID = IDI_REPO_FILE;

        if (IsAppThemed())
            CAppUtils::SetListCtrlBackgroundImage(m_RepoList.GetSafeHwnd(), nID);
    }
}

UINT CRepositoryBrowser::InitThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return ((CRepositoryBrowser*)pVoid)->InitThread();
}

//this is the thread function which calls the subversion function
UINT CRepositoryBrowser::InitThread()
{
    // In this thread, we try to find out the repository root.
    // Since this is a remote operation, it can take a while, that's
    // Why we do this inside a thread.

    // force the cursor to change
    RefreshCursor();

    DialogEnableWindow(IDOK, FALSE);

    InitRepo();

    // give user the chance to hit the cancel button
    // as long as we are waiting for the data to come in

    if (!m_cancelled)
        m_lister.WaitForJobsToFinish();

    // notify main thread that we are done

    PostMessage(WM_AFTERINIT);
    DialogEnableWindow(IDOK, TRUE);

    if (m_bStandAlone)
      GetDlgItem(IDCANCEL)->ShowWindow(FALSE);

    m_bThreadRunning = false;
    m_cancelled = false;

    RefreshCursor();
    return 0;
}

LRESULT CRepositoryBrowser::OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_cancelled || m_InitialUrl.IsEmpty())
        return 0;

    m_barRepository.SetRevision (m_repository.revision);
    m_barRepository.ShowUrl (m_InitialUrl, m_repository.revision);
    ChangeToUrl (m_InitialUrl, m_repository.revision, true);
    RefreshBookmarks();

    m_bInitDone = TRUE;
    return 0;
}

LRESULT CRepositoryBrowser::OnRefreshURL(WPARAM /*wParam*/, LPARAM lParam)
{
    const TCHAR* url = reinterpret_cast<const TCHAR*>(lParam);

    HTREEITEM item = FindUrl(url);
    if (item)
        RefreshNode (item, true);

    // Memory was obtained from tscdup(), so free() must be called
    free((void*)url);
    return 0;
}

void CRepositoryBrowser::OnOK()
{
    if (m_blockEvents)
        return;
    if (m_RepoTree.GetEditControl())
    {
        // in case there's an edit control, end the ongoing
        // editing of the item instead of dismissing the dialog.
        m_RepoTree.EndEditLabelNow(false);
        return;
    }

    StoreSelectedURLs();
    if ((GetFocus() == &m_RepoList)&&((GetKeyState(VK_MENU)&0x8000) == 0))
    {
        // list control has focus: 'enter' the folder

        if (m_RepoList.GetSelectedCount() != 1)
            return;

        POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
        if (pos)
            OpenFromList (m_RepoList.GetNextSelectedItem (pos));
        return;
    }

    if (m_EditFileCommand)
    {
        if (m_EditFileCommand->StopWaitingForEditor())
            return;
    }

    m_cancelled = TRUE;
    m_lister.Cancel();


    m_backgroundJobs.WaitForEmptyQueue();
    if (!m_bSparseCheckoutMode)
    {
        SaveColumnWidths(true);
        SaveDividerPosition();
        m_barRepository.SaveHistory();
    }
    else
    {
        m_checkoutDepths.clear();
        m_updateDepths.clear();
        HTREEITEM hRoot = m_RepoTree.GetRootItem();
        CheckoutDepthForItem(hRoot);
        FilterInfinityDepthItems(m_checkoutDepths);
        FilterInfinityDepthItems(m_updateDepths);
    }

    ClearUI();

    RevokeDragDrop(m_RepoList.GetSafeHwnd());
    RevokeDragDrop(m_RepoTree.GetSafeHwnd());

    CResizableStandAloneDialog::OnOK();
}

void CRepositoryBrowser::OnCancel()
{
    if (m_EditFileCommand)
    {
        if (m_EditFileCommand->StopWaitingForEditor())
            return;
    }
    m_cancelled = TRUE;
    m_lister.Cancel();

    if (m_bThreadRunning)
        return;

    RevokeDragDrop(m_RepoList.GetSafeHwnd());
    RevokeDragDrop(m_RepoTree.GetSafeHwnd());

    m_backgroundJobs.WaitForEmptyQueue();
    if (!m_bSparseCheckoutMode)
    {
        SaveColumnWidths(true);
        SaveDividerPosition();
    }

    ClearUI();

    __super::OnCancel();
}

LPARAM CRepositoryBrowser::OnAuthCancelled(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_cancelled = TRUE;
    m_lister.Cancel();
    return 0;
}

void CRepositoryBrowser::OnBnClickedHelp()
{
    OnHelp();
}

/******************************************************************************/
/* tree and list view resizing                                                */
/******************************************************************************/

BOOL CRepositoryBrowser::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if ((m_bThreadRunning)&&(!IsCursorOverWindowBorder()))
    {
        HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
        SetCursor(hCur);
        return TRUE;
    }
    if ((pWnd == this)&&(!m_bSparseCheckoutMode))
    {
        RECT rect;
        POINT pt;
        GetClientRect(&rect);
        GetCursorPos(&pt);
        ScreenToClient(&pt);
        if (PtInRect(&rect, pt))
        {
            ClientToScreen(&pt);
            // are we right of the tree control?
            GetDlgItem(IDC_REPOTREE)->GetWindowRect(&rect);
            if ((pt.x > rect.right)&&
                (pt.y >= rect.top+3)&&
                (pt.y <= rect.bottom-3))
            {
                // but left of the list control?
                GetDlgItem(IDC_REPOLIST)->GetWindowRect(&rect);
                if (pt.x < rect.left)
                {
                    HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
                    SetCursor(hCur);
                    return TRUE;
                }
            }
        }
    }
    return CStandAloneDialogTmpl<CResizableDialog>::OnSetCursor(pWnd, nHitTest, message);
}

void CRepositoryBrowser::OnMouseMove(UINT nFlags, CPoint point)
{
    if (bDragMode == FALSE)
        return;

    RECT rect, tree, list, treelist, treelistclient;
    // create an union of the tree and list control rectangle
    GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
    if (m_bSparseCheckoutMode)
        treelist = tree;
    else
        UnionRect(&treelist, &tree, &list);
    treelistclient = treelist;
    ScreenToClient(&treelistclient);

    //convert the mouse coordinates relative to the top-left of
    //the window
    ClientToScreen(&point);
    GetClientRect(&rect);
    ClientToScreen(&rect);
    point.x -= rect.left;
    point.y -= treelist.top;

    //same for the window coordinates - make them relative to 0,0
    OffsetRect(&treelist, -treelist.left, -treelist.top);

    if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
        point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
    if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH)
        point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

    if ((nFlags & MK_LBUTTON) && (point.x != oldx))
    {
        CDC * pDC = GetDC();

        if (pDC)
        {
            DrawXorBar(pDC, oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
            DrawXorBar(pDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);

            ReleaseDC(pDC);
        }

        oldx = point.x;
        oldy = point.y;
    }

    CStandAloneDialogTmpl<CResizableDialog>::OnMouseMove(nFlags, point);
}

void CRepositoryBrowser::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_bSparseCheckoutMode)
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);

    RECT rect, tree, list, treelist, treelistclient;

    // create an union of the tree and list control rectangle
    GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
    UnionRect(&treelist, &tree, &list);
    treelistclient = treelist;
    ScreenToClient(&treelistclient);

    //convert the mouse coordinates relative to the top-left of
    //the window
    ClientToScreen(&point);
    GetClientRect(&rect);
    ClientToScreen(&rect);
    point.x -= rect.left;
    point.y -= treelist.top;

    //same for the window coordinates - make them relative to 0,0
    OffsetRect(&treelist, -treelist.left, -treelist.top);

    if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
    if (point.x > treelist.right-3)
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
    if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH)
        point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

    if ((point.y < treelist.top+3) ||
        (point.y > treelist.bottom-3))
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);

    bDragMode = true;

    SetCapture();

    CDC * pDC = GetDC();
    DrawXorBar(pDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
    ReleaseDC(pDC);

    oldx = point.x;
    oldy = point.y;

    CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
}

void CRepositoryBrowser::HandleDividerMove(CPoint point, bool bDraw)
{
    RECT rect, tree, list, treelist, treelistclient;

    // create an union of the tree and list control rectangle
    GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
    UnionRect(&treelist, &tree, &list);
    treelistclient = treelist;
    ScreenToClient(&treelistclient);

    ClientToScreen(&point);
    GetClientRect(&rect);
    ClientToScreen(&rect);

    CPoint point2 = point;
    if (point2.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
        point2.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
    if (point2.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH)
        point2.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

    point.x -= rect.left;
    point.y -= treelist.top;

    OffsetRect(&treelist, -treelist.left, -treelist.top);

    if (point.x < treelist.left+REPOBROWSER_CTRL_MIN_WIDTH)
        point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
    if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH)
        point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

    if (bDraw)
    {
        CDC * pDC = GetDC();
        DrawXorBar(pDC, oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
        ReleaseDC(pDC);
    }

    oldx = point.x;
    oldy = point.y;

    //position the child controls
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&treelist);
    treelist.right = point2.x - 2;
    ScreenToClient(&treelist);
    RemoveAnchor(IDC_REPOTREE);
    GetDlgItem(IDC_REPOTREE)->MoveWindow(&treelist);
    GetDlgItem(IDC_REPOLIST)->GetWindowRect(&treelist);
    treelist.left = point2.x + 2;
    ScreenToClient(&treelist);
    RemoveAnchor(IDC_REPOLIST);
    GetDlgItem(IDC_REPOLIST)->MoveWindow(&treelist);

    AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
    AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);
}

void CRepositoryBrowser::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (bDragMode == FALSE)
        return;

    if (!m_bSparseCheckoutMode)
    {
        HandleDividerMove(point, true);

        bDragMode = false;
        ReleaseCapture();
    }

    CStandAloneDialogTmpl<CResizableDialog>::OnLButtonUp(nFlags, point);
}

void CRepositoryBrowser::OnCaptureChanged(CWnd *pWnd)
{
    bDragMode = false;

    __super::OnCaptureChanged(pWnd);
}

void CRepositoryBrowser::DrawXorBar(CDC * pDC, int x1, int y1, int width, int height)
{
    static WORD _dotPatternBmp[8] =
    {
        0x0055, 0x00aa, 0x0055, 0x00aa,
        0x0055, 0x00aa, 0x0055, 0x00aa
    };

    HBITMAP hbm;
    HBRUSH  hbr, hbrushOld;

    hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
    hbr = CreatePatternBrush(hbm);

    pDC->SetBrushOrg(x1, y1);
    hbrushOld = (HBRUSH)pDC->SelectObject(hbr);

    PatBlt(pDC->GetSafeHdc(), x1, y1, width, height, PATINVERT);

    pDC->SelectObject(hbrushOld);

    DeleteObject(hbr);
    DeleteObject(hbm);
}

bool CRepositoryBrowser::ChangeToUrl(CString& url, SVNRev& rev, bool bAlreadyChecked)
{
    CWaitCursorEx wait;
    if (!bAlreadyChecked)
    {
        // check if the entered url is valid
        SVNInfo info;
        const SVNInfoData * data = NULL;
        CString orig_url = url;
        do
        {
            data = info.GetFirstFileInfo(CTSVNPath(url), rev, rev);
            if (data && !rev.IsHead())
            {
                rev = data->rev;
            }
            if ((data == NULL)||(data->kind != svn_node_dir))
            {
                // in case the url is not a valid directory, try the parent dir
                // until there's no more parent dir
                url = url.Left(url.ReverseFind('/'));
            }
        } while(!url.IsEmpty() && ((data == NULL) || (data->kind != svn_node_dir)));
        if (url.IsEmpty())
            url = orig_url;
    }

    CString root = m_repository.root;
    bool urlHasDifferentRoot
        =    root.IsEmpty()
          || root.Compare (url.Left (root.GetLength()))
          || ((url.GetAt(root.GetLength()) != '/') && ((url.GetLength() > root.GetLength()) && (url.GetAt(root.GetLength()) != '/')));

    CString partUrl = url;
    if ((LONG(rev) != LONG(m_repository.revision)) || urlHasDifferentRoot)
    {
        ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_WAIT)), true);

        // if the repository root has changed, initialize all data from scratch
        // and clear the project properties we might have loaded previously
        if (urlHasDifferentRoot)
            m_ProjectProperties = ProjectProperties();
        m_InitialUrl = url;
        m_repository.revision = rev;
        // if the revision changed, then invalidate everything
        m_bFetchChildren = false;
        ClearUI();
        InitRepo();
        RefreshBookmarks();

        if (m_repository.root.IsEmpty())
            return false;

        m_path = CTSVNPath(m_repository.root);
        CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), m_origDlgTitle);
        root = m_repository.root;

        if (m_InitialUrl.Compare(url))
        {
            // url changed (redirect), update the combobox control
            url = m_InitialUrl;
        }
    }

    HTREEITEM hItem = AutoInsert (url);
    if (hItem == NULL)
        return false;

    CAutoReadLock locker(m_guard);
    CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hItem);
    if (pTreeItem == NULL)
        return FALSE;

    if (!m_RepoList.HasText())
        ShowText(L" ", true);

    m_bFetchChildren = !!CRegDWORD(L"Software\\TortoiseSVN\\RepoBrowserPrefetch", true);

    RefreshNode(hItem);
    m_RepoTree.Expand(hItem, TVE_EXPAND);
    FillList(pTreeItem);

    ++m_blockEvents;
    m_RepoTree.EnsureVisible(hItem);
    m_RepoTree.SelectItem(hItem);
    --m_blockEvents;

    m_RepoList.ClearText();
    m_RepoTree.ClearText();

    return true;
}

void CRepositoryBrowser::FillList(CTreeItem * pTreeItem)
{
    if (pTreeItem == NULL)
        return;

    CAutoWriteLock locker(m_guard);
    CWaitCursorEx wait;
    m_RepoList.SetRedraw(false);
    m_RepoList.DeleteAllItems();
    m_RepoList.ClearText();
    m_RepoTree.ClearText();
    m_pListCtrlTreeItem = pTreeItem;

    int c = ((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1;
    while (c>=0)
        m_RepoList.DeleteColumn(c--);

    c = 0;
    CString temp;
    //
    // column 0: contains tree
    temp.LoadString(IDS_LOG_FILE);
    m_RepoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 1: file extension
    temp.LoadString(IDS_STATUSLIST_COLEXT);
    m_RepoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 2: revision number
    temp.LoadString(IDS_LOG_REVISION);
    m_RepoList.InsertColumn(c++, temp, LVCFMT_RIGHT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 3: author
    temp.LoadString(IDS_LOG_AUTHOR);
    m_RepoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 4: size
    temp.LoadString(IDS_LOG_SIZE);
    m_RepoList.InsertColumn(c++, temp, LVCFMT_RIGHT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 5: date
    temp.LoadString(IDS_LOG_DATE);
    m_RepoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    if (m_bShowLocks)
    {
        //
        // column 6: lock owner
        temp.LoadString(IDS_STATUSLIST_COLLOCK);
        m_RepoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
        //
        // column 7: lock comment
        temp.LoadString(IDS_STATUSLIST_COLLOCKCOMMENT);
        m_RepoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    }

    int files = 0;
    int folders = 0;
    // special case: error to show
    if (!pTreeItem->error.IsEmpty() && pTreeItem->children.empty())
    {
        ShowText (pTreeItem->error, true);
    }
    else
    {
        // now fill in the data

        int nCount = 0;
        const deque<CItem>& items = pTreeItem->children;
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            int icon_idx;
            if (it->kind == svn_node_dir)
            {
                icon_idx =  m_nIconFolder;
                folders++;
            }
            else
            {
                icon_idx = SYS_IMAGE_LIST().GetFileIconIndex(it->path);
                files++;
            }
            int index = m_RepoList.InsertItem(nCount, it->path, icon_idx);
            SetListItemInfo(index, &(*it));
        }

        ListView_SortItemsEx(m_RepoList, ListSort, this);
        SetSortArrow();
    }
    if (m_arColumnWidths[0] == 0)
    {
        CRegString regColWidths(L"Software\\TortoiseSVN\\RepoBrowserColumnWidth");
        StringToWidthArray(regColWidths, m_arColumnWidths);
    }

    int maxcol = ((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1;
    for (int col = 0; col <= maxcol; col++)
    {
        if (m_arColumnWidths[col] == 0)
        {
            m_RepoList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
            m_arColumnAutoWidths[col] = m_RepoList.GetColumnWidth(col);
        }
        else
            m_RepoList.SetColumnWidth(col, m_arColumnWidths[col]);
    }

    m_RepoList.SetRedraw(true);

    temp.FormatMessage(IDS_REPOBROWSE_INFO,
                      (LPCTSTR)pTreeItem->unescapedname,
                      files, folders, files+folders);
    if (!pTreeItem->error.IsEmpty() && pTreeItem->children.empty())
        SetDlgItemText(IDC_INFOLABEL, L"");
    else
        SetDlgItemText(IDC_INFOLABEL, temp);

    if (m_UrlHistory.empty() || m_UrlHistory.front() != pTreeItem->url)
        m_UrlHistory.push_front(pTreeItem->url);
    if (m_UrlHistoryForward.empty() || m_UrlHistoryForward.front() != GetPath())
        m_UrlHistoryForward.clear();
}

HTREEITEM CRepositoryBrowser::FindUrl (const CString& fullurl)
{
    return FindUrl (fullurl, fullurl, TVI_ROOT);
}

HTREEITEM CRepositoryBrowser::FindUrl(const CString& fullurl, const CString& url, HTREEITEM hItem /* = TVI_ROOT */)
{
    CAutoReadLock locker(m_guard);
    if (hItem == TVI_ROOT)
    {
        CString root = m_repository.root;
        hItem = m_RepoTree.GetRootItem();
        if (fullurl.Compare(root)==0)
            return hItem;
        return FindUrl(fullurl, url.Mid(root.GetLength()+1), hItem);
    }
    HTREEITEM hSibling = hItem;
    if (m_RepoTree.GetNextItem(hItem, TVGN_CHILD))
    {
        hSibling = m_RepoTree.GetNextItem(hItem, TVGN_CHILD);
        do
        {
            CTreeItem * pTItem = ((CTreeItem*)m_RepoTree.GetItemData(hSibling));
            if (pTItem == 0)
                continue;
            CString sSibling = pTItem->unescapedname;
            if (sSibling.Compare(url.Left(sSibling.GetLength()))==0)
            {
                if (sSibling.GetLength() == url.GetLength())
                    return hSibling;
                if (url.GetAt(sSibling.GetLength()) == '/')
                    return FindUrl(fullurl, url.Mid(sSibling.GetLength()+1), hSibling);
            }
        } while ((hSibling = m_RepoTree.GetNextItem(hSibling, TVGN_NEXT)) != NULL);
    }

    return NULL;
}

void CRepositoryBrowser::FetchChildren (HTREEITEM node)
{
    CAutoReadLock lockerr(m_guard);
    CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData (node);
    if (pTreeItem->children_fetched)
        return;

    // block new background queries just for now to maximize foreground speed

    auto queryBlocker = m_lister.SuspendJobs();

    // standard list plus immediate externals

    std::deque<CItem>& children = pTreeItem->children;
    {
        CAutoWriteLock lockerw(m_guard);
        if (pTreeItem == m_pListCtrlTreeItem)
        {
            m_RepoList.DeleteAllItems();
            m_pListCtrlTreeItem = nullptr;
        }
        CString redirectedUrl;
        children.clear();
        pTreeItem->has_child_folders = false;
        pTreeItem->error = m_lister.GetList ( pTreeItem->url
                                            , pTreeItem->is_external
                                            ? pTreeItem->repository.peg_revision
                                            : SVNRev()
                                            , pTreeItem->repository
                                            , !m_bSparseCheckoutMode && m_bShowExternals ? SVN_DIRENT_ALL : direntAllExceptHasProps
                                            , !m_bSparseCheckoutMode && m_bShowExternals
                                            , children
                                            , redirectedUrl);
        if (!pTreeItem->error.IsEmpty() && pTreeItem->unversioned)
        {
            pTreeItem->error.Empty();
            pTreeItem->has_child_folders = true;
        }
    }

    int childfoldercount = 0;
    for (size_t i = 0, count = pTreeItem->children.size(); i < count; ++i)
    {
        const CItem& item = pTreeItem->children[i];
        if ((item.kind == svn_node_dir) && (!item.absolutepath.IsEmpty()))
        {
            pTreeItem->has_child_folders = true;
            ++childfoldercount;
            if (m_bFetchChildren)
            {
                m_lister.Enqueue(item.absolutepath
                                 , item.is_external ? item.repository.peg_revision : SVNRev()
                                 , item.repository
                                 , !m_bSparseCheckoutMode && m_bShowExternals ? SVN_DIRENT_ALL : direntAllExceptHasProps
                                 , item.has_props && !m_bSparseCheckoutMode && m_bShowExternals);
                if (childfoldercount > 30)
                    break;
            }
        }
    }
    if ((pTreeItem->has_child_folders) || (m_bSparseCheckoutMode))
        AutoInsert(node, pTreeItem->children);
    // if there are no child folders, remove the '+' in front of the node
    {
        TVITEM tvitem = { 0 };
        tvitem.hItem = node;
        tvitem.mask = TVIF_CHILDREN;
        tvitem.cChildren = pTreeItem->has_child_folders || (m_bSparseCheckoutMode && pTreeItem->children.size()) ? 1 : 0;
        m_RepoTree.SetItem(&tvitem);
    }


    // add parent sub-tree externals
    if (m_bShowExternals)
    {
        CString relPath;
        for (
            ; node && pTreeItem->error.IsEmpty()
            ; node = m_RepoTree.GetParentItem(node))
        {
            CTreeItem* parentItem = (CTreeItem *)m_RepoTree.GetItemData(node);
            if (parentItem == NULL)
                continue;
            if (parentItem->svnparentpathroot)
                continue;
            parentItem->error = m_lister.AddSubTreeExternals(parentItem->url
                                                             , parentItem->is_external
                                                             ? parentItem->repository.peg_revision
                                                             : SVNRev()
                                                             , parentItem->repository
                                                             , relPath
                                                             , children);
            if (!parentItem->error.IsEmpty() && parentItem->unversioned)
            {
                parentItem->error.Empty();
                parentItem->has_child_folders = true;
            }
            relPath = parentItem->unescapedname + '/' + relPath;
        }
    }

    // done

    pTreeItem->children_fetched = true;
}

HTREEITEM CRepositoryBrowser::AutoInsert (const CString& path)
{
    CString currentPath;
    HTREEITEM node = NULL;
    do
    {
        // name of the node to add

        CString name;

        // select next path level

        if (currentPath.IsEmpty())
        {
            if (m_bSparseCheckoutMode)
            {
                currentPath = m_InitialUrl;
                name = m_InitialUrl;
            }
            else
            {
                currentPath = m_repository.root;
                name = m_repository.root;
            }
        }
        else
        {
            int start = currentPath.GetLength() + 1;
            int pos = path.Find ('/', start);
            if (pos < 0)
                pos = path.GetLength();

            name = path.Mid (start, pos - start);
            currentPath = path.Left (pos);
        }

        // root node?

        if (node == NULL)
        {
            CAutoWriteLock locker(m_guard);
            node = m_RepoTree.GetRootItem();
            if (node)
            {
                CAutoReadLock locker(m_guard);
                CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData (node);
                if (pTreeItem == nullptr)
                    node = NULL;
                else if (pTreeItem->bookmark || pTreeItem->dummy)
                    node = NULL;
            }
            if (node == NULL)
            {
                // the tree view is empty, just fill in the repository root
                CTreeItem * pTreeItem = new CTreeItem();
                pTreeItem->unescapedname = name;
                pTreeItem->url = currentPath;
                pTreeItem->logicalPath = currentPath;
                pTreeItem->repository = m_repository;
                pTreeItem->kind = svn_node_dir;

                TVINSERTSTRUCT tvinsert = {0};
                tvinsert.hParent = TVI_ROOT;
                tvinsert.hInsertAfter = TVI_FIRST;
                tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
                tvinsert.itemex.pszText = currentPath.GetBuffer (currentPath.GetLength());
                tvinsert.itemex.cChildren = 1;
                tvinsert.itemex.lParam = (LPARAM)pTreeItem;
                tvinsert.itemex.iImage = m_nIconFolder;
                tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;
                if (m_bSparseCheckoutMode)
                {
                    tvinsert.itemex.state = INDEXTOSTATEIMAGEMASK(2);   // root item is always checked
                    tvinsert.itemex.stateMask = TVIS_STATEIMAGEMASK;
                }

                node = m_RepoTree.InsertItem(&tvinsert);
                currentPath.ReleaseBuffer();
            }
        }
        else
        {
            // get all children

            CAutoReadLock locker(m_guard);
            CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData (node);
            std::deque<CItem>& children = pTreeItem->children;

            if (!pTreeItem->children_fetched)
            {
                // not yet fetched -> do it now

                if (!RefreshNode (node))
                    return NULL;

                for (size_t i = 0, count = children.size(); i < count; ++i)
                    if (children[i].kind == svn_node_dir)
                    {
                        pTreeItem->has_child_folders = true;
                        break;
                    }
            }

            // select the one that matches the node name we are looking for

            const CItem* item = NULL;
            for (size_t i = 0, count = children.size(); i < count; ++i)
                if (   (children[i].kind == svn_node_dir)
                    && (children[i].path == name))
                {
                    item = &children[i];
                    break;
                }

            if (item == NULL)
            {
                if (path.Compare(m_InitialUrl) == 0)
                {
                    // add this path manually.
                    // The path is valid, that was checked in the init function already.
                    // But it's possible that we don't have read access to its parent
                    // folder, so by adding the valid path manually we give the user
                    // a chance to still browse this folder in case he has read
                    // access to it
                    CTreeItem * pNodeTreeItem = (CTreeItem *)m_RepoTree.GetItemData(node);
                    if (pNodeTreeItem && !pNodeTreeItem->error.IsEmpty())
                    {
                        TVITEM tvitem = {0};
                        tvitem.hItem = node;
                        tvitem.mask = TVIF_CHILDREN;
                        tvitem.cChildren = 1;
                        m_RepoTree.SetItem(&tvitem);
                    }
                    CItem manualItem(currentPath.Mid (currentPath.ReverseFind ('/') + 1), L"", svn_node_dir, 0, true, -1, 0, L"", L"", L"", L"", false, 0, 0, currentPath, pTreeItem->repository);
                    node = AutoInsert (node, manualItem);
                }
                else
                    return NULL;
            }
            else
            {
                // auto-add the node for the current path level
                node = AutoInsert (node, *item);
            }
        }

        // pre-fetch data for the next level

        if (m_bFetchChildren)
        {
            CAutoReadLock locker(m_guard);
            CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData (node);
            if ((pTreeItem != NULL) && !pTreeItem->children_fetched)
            {
                m_lister.Enqueue(pTreeItem->url
                                 , pTreeItem->is_external ? pTreeItem->repository.peg_revision : SVNRev()
                                 , pTreeItem->repository
                                 , !m_bSparseCheckoutMode && m_bShowExternals ? SVN_DIRENT_ALL : direntAllExceptHasProps
                                 , !m_bSparseCheckoutMode && m_bShowExternals);
            }
        }
    }
    while (currentPath != path);

    // done

    return node;
}

HTREEITEM CRepositoryBrowser::AutoInsert (HTREEITEM hParent, const CItem& item)
{
    // look for an existing sub-node by comparing names

    {
        CAutoReadLock locker(m_guard);
        for ( HTREEITEM hSibling = m_RepoTree.GetNextItem (hParent, TVGN_CHILD)
            ; hSibling != NULL
            ; hSibling = m_RepoTree.GetNextItem (hSibling, TVGN_NEXT))
        {
            CTreeItem * pTreeItem = ((CTreeItem*)m_RepoTree.GetItemData (hSibling));
            if (pTreeItem && (pTreeItem->unescapedname == item.path))
                return hSibling;
        }
    }

    CAutoWriteLock locker(m_guard);
    // no such sub-node. Create one

    HTREEITEM hNewItem
        = Insert (hParent, (CTreeItem*)m_RepoTree.GetItemData (hParent), item);

    Sort (hParent);

    return hNewItem;
}

void CRepositoryBrowser::AutoInsert (HTREEITEM hParent, const std::deque<CItem>& items)
{
    typedef std::map<CString, const CItem*> TMap;
    TMap newItems;

    // determine what nodes need to be added

    for (size_t i = 0, count = items.size(); i < count; ++i)
        if ((items[i].kind == svn_node_dir)||(m_bSparseCheckoutMode))
            newItems.insert (std::make_pair (items[i].path, &items[i]));

    {
        CAutoReadLock locker(m_guard);
        for ( HTREEITEM hSibling = m_RepoTree.GetNextItem (hParent, TVGN_CHILD)
            ; hSibling != NULL
            ; hSibling = m_RepoTree.GetNextItem (hSibling, TVGN_NEXT))
        {
            CTreeItem * pTreeItem = ((CTreeItem*)m_RepoTree.GetItemData (hSibling));
            if (pTreeItem)
                newItems.erase (pTreeItem->unescapedname);
        }
    }

    if (newItems.empty())
        return;

    // Create missing sub-nodes

    CAutoWriteLock locker(m_guard);
    CTreeItem* parentTreeItem = (CTreeItem*)m_RepoTree.GetItemData (hParent);

    for ( TMap::const_iterator iter = newItems.begin(), end = newItems.end()
        ; iter != end
        ; ++iter)
    {
        Insert (hParent, parentTreeItem, *iter->second);
    }

    Sort (hParent);
}

HTREEITEM CRepositoryBrowser::Insert
    ( HTREEITEM hParent
    , CTreeItem* parentTreeItem
    , const CItem& item)
{
    CAutoWriteLock locker(m_guard);
    CTreeItem* pTreeItem = new CTreeItem();
    CString name = item.path;

    pTreeItem->unescapedname = name;
    pTreeItem->url = item.absolutepath;
    pTreeItem->logicalPath = parentTreeItem->logicalPath + '/' + name;
    pTreeItem->repository = item.repository;
    pTreeItem->is_external = item.is_external;
    pTreeItem->kind = item.kind;
    pTreeItem->unversioned = item.unversioned;

    bool isSelectedForDiff
        = pTreeItem->url.CompareNoCase (m_diffURL.GetSVNPathString()) == 0;

    UINT state = isSelectedForDiff ? TVIS_BOLD : 0;
    UINT stateMask = state;
    if (item.is_external)
    {
        state |= INDEXTOOVERLAYMASK (OVERLAY_EXTERNAL);
        stateMask |= TVIS_OVERLAYMASK;
    }
    if (m_bSparseCheckoutMode)
    {
        state |= INDEXTOSTATEIMAGEMASK(m_RepoTree.GetCheck(hParent)||(hParent==TVI_ROOT) ? 2 : 1);
        stateMask |= TVIS_STATEIMAGEMASK;
    }

    TVINSERTSTRUCT tvinsert = {0};
    tvinsert.hParent = hParent;
    tvinsert.hInsertAfter = TVI_FIRST;
    tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
    tvinsert.itemex.state = state;
    tvinsert.itemex.stateMask = stateMask;
    tvinsert.itemex.pszText = name.GetBuffer (name.GetLength());
    tvinsert.itemex.lParam = (LPARAM)pTreeItem;
    if (item.kind == svn_node_dir)
    {
        tvinsert.itemex.cChildren = 1;
        tvinsert.itemex.iImage = m_nIconFolder;
        tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;
    }
    else
    {
        pTreeItem->children_fetched = true;
        tvinsert.itemex.cChildren = 0;
        tvinsert.itemex.iImage = SYS_IMAGE_LIST().GetFileIconIndex(item.path);
        tvinsert.itemex.iSelectedImage = tvinsert.itemex.iImage;
    }
    HTREEITEM hNewItem = m_RepoTree.InsertItem(&tvinsert);
    name.ReleaseBuffer();

    if (m_bSparseCheckoutMode)
    {
        auto it = m_wcDepths.find(pTreeItem->url);
        if (it != m_wcDepths.end())
        {
            switch (it->second)
            {
            case svn_depth_infinity:
            case svn_depth_immediates:
            case svn_depth_files:
            case svn_depth_empty:
            case svn_depth_unknown:
                m_RepoTree.SetCheck(hNewItem);
                break;
            default:
                m_RepoTree.SetCheck(hNewItem, false);
                break;
            }
        }
        else
        {
            if (!m_wcPath.IsEmpty() || m_RepoTree.GetRootItem() == hParent)
            {
                // node without depth in already checked out WC or item on second (below root) level.
                // this nodes should be unchecked
                m_RepoTree.SetCheck(hNewItem, false);
            }
            else
            {
                // other nodes inherit parent state
                m_RepoTree.SetCheck(hNewItem, m_RepoTree.GetCheck(hParent));
            }
        }
    }

    return hNewItem;
}

void CRepositoryBrowser::Sort (HTREEITEM parent)
{
    CAutoWriteLock locker(m_guard);
    TVSORTCB tvs;
    tvs.hParent = parent;
    tvs.lpfnCompare = TreeSort;
    tvs.lParam = (LPARAM)this;

    m_RepoTree.SortChildrenCB (&tvs);
}

void CRepositoryBrowser::RefreshChildren (HTREEITEM node)
{
    CAutoReadLock locker(m_guard);
    CTreeItem* pTreeItem = (CTreeItem*)m_RepoTree.GetItemData (node);
    if (pTreeItem == NULL)
        return;

    pTreeItem->children_fetched = false;
    InvalidateData(node);
    FetchChildren (node);

    // update node status and add sub-nodes for all sub-dirs
    int childfoldercount = 0;
    for (size_t i = 0, count = pTreeItem->children.size(); i < count; ++i)
    {
        const CItem& item = pTreeItem->children[i];
        if ((item.kind == svn_node_dir)&&(!item.absolutepath.IsEmpty()))
        {
            pTreeItem->has_child_folders = true;
            if (m_bFetchChildren)
            {
                ++childfoldercount;
                m_lister.Enqueue(item.absolutepath
                                 , item.is_external ? item.repository.peg_revision : SVNRev()
                                 , item.repository
                                 , !m_bSparseCheckoutMode && m_bShowExternals ? SVN_DIRENT_ALL : direntAllExceptHasProps
                                 , item.has_props && !m_bSparseCheckoutMode && m_bShowExternals);
                if (childfoldercount > 30)
                    break;
            }
        }
    }
}

bool CRepositoryBrowser::RefreshNode(HTREEITEM hNode, bool force /* = false*/)
{
    if (hNode == NULL)
        return false;

    SaveColumnWidths();
    CWaitCursorEx wait;
    CAutoReadLock locker(m_guard);
    // block all events until the list control is refreshed as well
    ++m_blockEvents;
    CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hNode);
    if (!pTreeItem || pTreeItem->svnparentpathroot)
    {
        --m_blockEvents;
        return false;
    }
    HTREEITEM hSel1 = m_RepoTree.GetSelectedItem();
    if (m_RepoTree.ItemHasChildren(hNode))
    {
        HTREEITEM hChild = m_RepoTree.GetChildItem(hNode);
        HTREEITEM hNext;

        CAutoWriteLock locker(m_guard);
        while (hChild)
        {
            hNext = m_RepoTree.GetNextItem(hChild, TVGN_NEXT);
            RecursiveRemove(hChild);
            m_RepoTree.DeleteItem(hChild);
            hChild = hNext;
        }
    }

    RefreshChildren (hNode);

    if ((pTreeItem->has_child_folders)||(m_bSparseCheckoutMode))
        AutoInsert (hNode, pTreeItem->children);

    // if there are no child folders, remove the '+' in front of the node
    {
        TVITEM tvitem = {0};
        tvitem.hItem = hNode;
        tvitem.mask = TVIF_CHILDREN;
        tvitem.cChildren = pTreeItem->has_child_folders || !pTreeItem->children_fetched || (m_bSparseCheckoutMode && pTreeItem->children.size()) ? 1 : 0;
        m_RepoTree.SetItem(&tvitem);
    }
    if (pTreeItem->children_fetched && pTreeItem->error.IsEmpty())
        if ((force)||(hSel1 == hNode)||(hSel1 != m_RepoTree.GetSelectedItem())||(m_pListCtrlTreeItem == nullptr))
            FillList(pTreeItem);
    --m_blockEvents;
    return true;
}

BOOL CRepositoryBrowser::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message>=WM_KEYFIRST && pMsg->message<=WM_KEYLAST)
    {
        // Check if there is an in place Edit active:
        // in place edits are done with an edit control, where the parent
        // is the control with the editable item (tree or list control here)
        HWND hWndFocus = ::GetFocus();
        if (hWndFocus)
            hWndFocus = ::GetParent(hWndFocus);
        if (hWndFocus && ((hWndFocus == m_RepoTree.GetSafeHwnd())||(hWndFocus == m_RepoList.GetSafeHwnd())))
        {
            if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE) && (hWndFocus == m_RepoTree.GetSafeHwnd()))
            {
                // ending editing on the tree control does not seem to work properly:
                // while editing ends when the escape key is pressed, the key message
                // is passed on which then also cancels the whole repobrowser dialog.
                // To prevent this, end editing here directly and prevent the escape key message
                // from being passed on.
                m_RepoTree.EndEditLabelNow(true);
                return TRUE;
            }

            // Do a direct translation.
            if (!::IsDialogMessage(m_hWnd, pMsg) &&
                !::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
            {
                ::TranslateMessage(pMsg);
                ::DispatchMessage(pMsg);
            }
            return TRUE;
        }
        if (m_hAccel)
        {
            if (pMsg->message == WM_KEYDOWN)
            {
                switch (pMsg->wParam)
                {
                case 'C':
                case VK_INSERT:
                case VK_DELETE:
                case VK_BACK:
                    {
                        if ((pMsg->hwnd == m_barRepository.GetSafeHwnd())||(::IsChild(m_barRepository.GetSafeHwnd(), pMsg->hwnd)))
                            return __super::PreTranslateMessage(pMsg);
                    }
                    break;
                }
            }
            int ret = TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
            if (ret)
                return TRUE;
        }
    }
    return __super::PreTranslateMessage(pMsg);
}

void CRepositoryBrowser::OnDelete()
{
    if (m_bSparseCheckoutMode)
        return;

    CTSVNPathList urlList;
    std::vector<SRepositoryInfo> repositories;
    bool bTreeItem = false;

    {
        CAutoReadLock locker(m_guard);
        POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
        if (pos == NULL)
            return;
        int index = -1;
        while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
        {
            CItem * pItem = (CItem *)m_RepoList.GetItemData(index);
            CString absPath = pItem->absolutepath;
            absPath.Replace(L"\\", L"%5C");
            urlList.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(absPath))));
            repositories.push_back (pItem->repository);
        }
        if ((urlList.GetCount() == 0))
        {
            HTREEITEM hItem = m_RepoTree.GetSelectedItem();
            CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hItem);
            if (pTreeItem)
            {
                if (pTreeItem->repository.root.Compare(pTreeItem->url)==0)
                    return; // can't delete the repository root!

                urlList.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(pTreeItem->url))));
                repositories.push_back (pTreeItem->repository);
                bTreeItem = true;
            }
        }
    }

    if (urlList.GetCount() == 0)
        return;

    CString sLogMsg;
    if (!RunStartCommit(urlList, sLogMsg))
        return;
    CWaitCursorEx wait_cursor;
    CInputLogDlg input(this);
    input.SetPathList(urlList);
    input.SetRootPath(CTSVNPath(m_InitialUrl));
    input.SetLogText(sLogMsg);
    input.SetUUID (repositories[0].uuid);
    input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEDEL);
    CString hint;
    if (urlList.GetCount() == 1)
        hint.Format(IDS_INPUT_REMOVEONE, (LPCTSTR)urlList[0].GetFileOrDirectoryName());
    else
        hint.Format(IDS_INPUT_REMOVEMORE, urlList.GetCount());
    input.SetActionText(hint);
    if (input.DoModal() == IDOK)
    {
        sLogMsg = input.GetLogMessage();
        if (!RunPreCommit(urlList, svn_depth_unknown, sLogMsg))
            return;
        bool bRet = Remove(urlList, true, false, sLogMsg, input.m_revProps);
        RunPostCommit(urlList, svn_depth_unknown, m_commitRev, sLogMsg);
        if (!bRet || !PostCommitErr.IsEmpty())
        {
            wait_cursor.Hide();
            ShowErrorDialog(m_hWnd);
            if (!bRet)
                return;
        }
        if (bTreeItem)
        {
            // do a full refresh: just refreshing the parent of the
            // deleted tree node won't work if the list view
            // shows part of that tree.
            OnRefresh();
        }
        else
            RefreshNode(m_RepoTree.GetSelectedItem(), true);
    }
}

void CRepositoryBrowser::OnGoUp()
{
    m_barRepository.OnGoUp();
}

void CRepositoryBrowser::OnUrlFocus()
{
    m_barRepository.SetFocusToURL();
}

void CRepositoryBrowser::OnCopy()
{
    // Ctrl-C : copy the selected item urls to the clipboard
    CString url;
    POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
    if (pos == NULL)
        return;
    int index = -1;
    CAutoReadLock locker(m_guard);
    while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
    {
        CItem * pItem = (CItem *)m_RepoList.GetItemData(index);
        url += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(pItem->absolutepath)));
        if (!GetRevision().IsHead())
        {
            url += L"?r=" + GetRevision().ToString();
        }
        url += L"\r\n";
    }
    if (!url.IsEmpty())
    {
        url.TrimRight(L"\r\n");
        CStringUtils::WriteAsciiStringToClipboard(url);
    }
}

void CRepositoryBrowser::OnInlineedit()
{
    if (m_bSparseCheckoutMode)
        return;
    ++m_blockEvents;
    OnOutOfScope(--m_blockEvents);
    if (GetFocus() == &m_RepoList)
    {
        POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
        if (pos == NULL)
            return;
        int selIndex = m_RepoList.GetNextSelectedItem(pos);
        CAutoReadLock locker(m_guard);
        CItem * pItem = (CItem *)m_RepoList.GetItemData(selIndex);
        if (!pItem->is_external)
        {
            m_RepoList.SetFocus();
            m_RepoList.EditLabel(selIndex);
        }
    }
    else
    {
        CAutoReadLock locker(m_guard);
        m_RepoTree.SetFocus();
        HTREEITEM hTreeItem = m_RepoTree.GetSelectedItem();
        if (hTreeItem != m_RepoTree.GetRootItem())
        {
            CTreeItem* pItem = (CTreeItem*)m_RepoTree.GetItemData (hTreeItem);
            if (!pItem->is_external && !pItem->bookmark && !pItem->dummy)
                m_RepoTree.EditLabel(hTreeItem);
        }
    }
}

void CRepositoryBrowser::OnRefresh()
{
    CWaitCursorEx waitCursor;

    ++m_blockEvents;

    // try to get the tree node to refresh

    HTREEITEM hSelected = m_RepoTree.GetSelectedItem();
    if (hSelected == NULL)
        hSelected = m_RepoTree.GetRootItem();

    if (hSelected == NULL)
    {
        --m_blockEvents;
        if (m_InitialUrl.IsEmpty())
            return;
        // try re-init
        InitRepo();
        return;
    }

    CAutoReadLock locker(m_guard);
    // invalidate the cache

    InvalidateData (hSelected);

    // refresh the current node

    RefreshNode(m_RepoTree.GetSelectedItem(), true);

    --m_blockEvents;
}

void CRepositoryBrowser::OnTvnSelchangedRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    if (m_blockEvents)
        return;

    if (m_bSparseCheckoutMode)
        return;

    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    if (pNMTreeView->action == TVC_BYKEYBOARD)
        SetTimer(REPOBROWSER_FETCHTIMER, 300, NULL);
    else
        OnTimer(REPOBROWSER_FETCHTIMER);
}

void CRepositoryBrowser::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == REPOBROWSER_FETCHTIMER)
    {
        KillTimer(REPOBROWSER_FETCHTIMER);
        // find the currently selected item
        CAutoReadLock locker(m_guard);
        HTREEITEM hSelItem = m_RepoTree.GetSelectedItem();
        if (hSelItem)
        {
            CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hSelItem);
            if (pTreeItem && !pTreeItem->dummy)
            {
                if (pTreeItem->bookmark)
                {
                    if (!pTreeItem->url.IsEmpty())
                    {
                        SVNRev r = SVNRev::REV_HEAD;
                        CString url = pTreeItem->url;
                        ChangeToUrl(url, r, false);
                        m_barRepository.ShowUrl (url, r);
                    }
                }
                else
                {
                    if (!pTreeItem->children_fetched && pTreeItem->error.IsEmpty())
                    {
                        CAutoWriteLock locker(m_guard);
                        m_RepoList.DeleteAllItems();
                        FetchChildren(hSelItem);
                    }

                    FillList(pTreeItem);

                    m_barRepository.ShowUrl ( pTreeItem->url
                                            , pTreeItem->repository.revision);
                }
            }
        }
    }

    __super::OnTimer(nIDEvent);
}

void CRepositoryBrowser::OnTvnItemexpandingRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    if (m_blockEvents)
        return;

    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

    CAutoReadLock locker(m_guard);
    CTreeItem * pTreeItem = (CTreeItem *)pNMTreeView->itemNew.lParam;

    if (pTreeItem == NULL)
        return;
    if (pTreeItem->bookmark)
        return;
    if (pTreeItem->dummy)
        return;

    if (pNMTreeView->action == TVE_COLLAPSE)
    {
        // user wants to collapse a tree node.
        // if we don't know anything about the children
        // of the node, we suppress the collapsing but fetch the info instead
        if (!pTreeItem->children_fetched)
        {
            FetchChildren(pNMTreeView->itemNew.hItem);
            *pResult = 1;
            return;
        }
        return;
    }

    // user wants to expand a tree node.
    // check if we already know its children - if not we have to ask the repository!

    if (!pTreeItem->children_fetched)
    {
        FetchChildren(pNMTreeView->itemNew.hItem);
    }
    else
    {
        // if there are no child folders, remove the '+' in front of the node

        if (!pTreeItem->has_child_folders && (!m_bSparseCheckoutMode && (pTreeItem->children.empty())))
        {
            TVITEM tvitem = {0};
            tvitem.hItem = pNMTreeView->itemNew.hItem;
            tvitem.mask = TVIF_CHILDREN;
            tvitem.cChildren = 0;
            m_RepoTree.SetItem(&tvitem);
        }
    }
}

void CRepositoryBrowser::OnNMDblclkRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    if (m_blockEvents)
        return;

    LPNMITEMACTIVATE pNmItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    if (pNmItemActivate->iItem < 0)
        return;

    CWaitCursorEx wait;
    OpenFromList (pNmItemActivate->iItem);
}

void CRepositoryBrowser::OpenFromList (int item)
{
    if (item < 0)
        return;

    CAutoReadLock locker(m_guard);
    CItem * pItem = (CItem*)m_RepoList.GetItemData (item);
    if (pItem == 0)
        return;

    CWaitCursorEx wait;

    if ((pItem->kind == svn_node_dir)||m_bSparseCheckoutMode)
    {
        // a double click on a folder results in selecting that folder

        HTREEITEM hItem = AutoInsert (m_RepoTree.GetSelectedItem(), *pItem);

        m_RepoTree.EnsureVisible (hItem);
        m_RepoTree.SelectItem (hItem);
    }
    else if (pItem->kind == svn_node_file)
    {
        CRepositoryBrowserSelection selection;
        selection.Add (pItem);
        OpenFile(selection.GetURL (0,0), selection.GetURLEscaped (0,0), false);
    }
}

void CRepositoryBrowser::OpenFile(const CTSVNPath& url, const CTSVNPath& urlEscaped, bool bOpenWith)
{
    const SVNRev& revision = GetRevision();
    CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(false, url, revision);
    CWaitCursorEx wait_cursor;
    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_APPNAME);
    CString sInfoLine;
    sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url.GetFileOrDirectoryName(), (LPCTSTR)revision.ToString());
    progDlg.SetLine(1, sInfoLine, true);
    SetAndClearProgressInfo(&progDlg);
    progDlg.ShowModeless(m_hWnd);
    if (!Export(urlEscaped, tempfile, revision, revision))
    {
        progDlg.Stop();
        SetAndClearProgressInfo((HWND)NULL);
        wait_cursor.Hide();
        ShowErrorDialog(m_hWnd);
        return;
    }
    progDlg.Stop();
    SetAndClearProgressInfo((HWND)NULL);
    // set the file as read-only to tell the app which opens the file that it's only
    // a temporary file and must not be edited.
    SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    if (bOpenWith)
    {
        OPENASINFO oi = { 0 };
        oi.pcszFile = tempfile.GetWinPath();
        oi.oaifInFlags = OAIF_EXEC;
        SHOpenWithDialog(GetSafeHwnd(), &oi);
    }
    else
    {
        ShellExecute ( NULL
                      , L"open"
                      , tempfile.GetWinPathString()
                      , NULL
                      , NULL
                      , SW_SHOWNORMAL);
    }
}

void CRepositoryBrowser::EditFile(CTSVNPath url, CTSVNPath urlEscaped)
{
    SVNRev revision = GetRevision();
    CString paramString
        = L"/closeonend:1 /closeforlocal /hideprogress /revision:";
    CCmdLineParser parameters (paramString + revision.ToString());

    m_EditFileCommand = std::unique_ptr<EditFileCommand>(new EditFileCommand());
    m_EditFileCommand->SetPaths (CTSVNPathList (url), url);
    m_EditFileCommand->SetParser (parameters);

    if (m_EditFileCommand->Execute() && revision.IsHead())
    {
        CString dir = urlEscaped.GetContainingDirectory().GetSVNPathString();
        m_lister.RefreshSubTree (revision, dir);

        // Memory will be allocated by malloc() - call free() once the message has been handled
        const TCHAR* lParam = _wcsdup((LPCTSTR)dir);
        PostMessage(WM_REFRESHURL, 0, reinterpret_cast<LPARAM>(lParam));
    }
}

void CRepositoryBrowser::OnHdnItemclickRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
    // a click on a header means sorting the items
    CAutoWriteLock locker(m_guard);
    if (m_nSortedColumn != phdr->iItem)
        m_bSortAscending = true;
    else
        m_bSortAscending = !m_bSortAscending;
    m_nSortedColumn = phdr->iItem;

    ++m_blockEvents;
    ListView_SortItemsEx(m_RepoList, ListSort, this);
    SetSortArrow();
    --m_blockEvents;
    *pResult = 0;
}

int CRepositoryBrowser::ListSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3)
{
    CRepositoryBrowser * pThis = (CRepositoryBrowser*)lParam3;
    CItem * pItem1 = (CItem*)pThis->m_RepoList.GetItemData(static_cast<int>(lParam1));
    CItem * pItem2 = (CItem*)pThis->m_RepoList.GetItemData(static_cast<int>(lParam2));
    int nRet = 0;
    switch (pThis->m_nSortedColumn)
    {
    case 0: // filename
        nRet = SortStrCmp(pItem1->path, pItem2->path);
        if (nRet != 0)
            break;
        // fall through
    case 1: // extension
        nRet = pThis->m_RepoList.GetItemText(static_cast<int>(lParam1), 1)
                 .CompareNoCase(pThis->m_RepoList.GetItemText(static_cast<int>(lParam2), 1));
        if (nRet == 0)  // if extensions are the same, use the filename to sort
            nRet = SortStrCmp(pItem1->path, pItem2->path);
        if (nRet != 0)
            break;
        // fall through
    case 2: // revision number
        nRet = pItem1->created_rev - pItem2->created_rev;
        if (nRet == 0)  // if extensions are the same, use the filename to sort
            nRet = SortStrCmp(pItem1->path, pItem2->path);
        if (nRet != 0)
            break;
        // fall through
    case 3: // author
        nRet = pItem1->author.CompareNoCase(pItem2->author);
        if (nRet == 0)  // if extensions are the same, use the filename to sort
            nRet = SortStrCmp(pItem1->path, pItem2->path);
        if (nRet != 0)
            break;
        // fall through
    case 4: // size
        nRet = int(pItem1->size - pItem2->size);
        if (nRet == 0)  // if extensions are the same, use the filename to sort
            nRet = SortStrCmp(pItem1->path, pItem2->path);
        if (nRet != 0)
            break;
        // fall through
    case 5: // date
        nRet = (pItem1->time - pItem2->time) > 0 ? 1 : -1;
        break;
    case 6: // lock owner
        nRet = pItem1->lockowner.CompareNoCase(pItem2->lockowner);
        if (nRet == 0)  // if extensions are the same, use the filename to sort
            nRet = SortStrCmp(pItem1->path, pItem2->path);
        break;
    }

    if (!pThis->m_bSortAscending)
        nRet = -nRet;

    // we want folders on top, then the files
    if (pItem1->kind != pItem2->kind)
    {
        if (pItem1->kind == svn_node_dir)
            nRet = -1;
        else
            nRet = 1;
    }

    return nRet;
}

int CRepositoryBrowser::TreeSort(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParam3*/)
{
    CTreeItem * Item1 = (CTreeItem*)lParam1;
    CTreeItem * Item2 = (CTreeItem*)lParam2;

    // directories are always above the other nodes
    if(Item1->kind == svn_node_dir && Item2->kind != svn_node_dir)
        return -1;

    if(Item1->kind != svn_node_dir && Item2->kind == svn_node_dir)
        return 1;

    return SortStrCmp(Item1->unescapedname, Item2->unescapedname);
}

void CRepositoryBrowser::SetSortArrow()
{
    CHeaderCtrl * pHeader = m_RepoList.GetHeaderCtrl();
    HDITEM HeaderItem = {0};
    HeaderItem.mask = HDI_FORMAT;
    for (int i=0; i<pHeader->GetItemCount(); ++i)
    {
        pHeader->GetItem(i, &HeaderItem);
        HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        pHeader->SetItem(i, &HeaderItem);
    }

    pHeader->GetItem(m_nSortedColumn, &HeaderItem);
    HeaderItem.fmt |= (m_bSortAscending ? HDF_SORTUP : HDF_SORTDOWN);
    pHeader->SetItem(m_nSortedColumn, &HeaderItem);
}

void CRepositoryBrowser::OnLvnItemchangedRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    if (m_blockEvents)
        return;
    if (m_RepoList.HasText())
        return;
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if (pNMLV->uChanged & LVIF_STATE)
    {
        if ((pNMLV->uNewState & LVIS_SELECTED) &&
            (pNMLV->iItem >= 0) &&
            (pNMLV->iItem < m_RepoList.GetItemCount()))
        {
            CAutoReadLock locker(m_guard);
            CItem * pItem = (CItem*)m_RepoList.GetItemData(pNMLV->iItem);
            if (pItem)
            {
                m_barRepository.ShowUrl ( pItem->absolutepath
                    , pItem->repository.revision);
                CString temp;
                CString rev;

                if (pItem->is_external)
                {
                    temp.FormatMessage(IDS_REPOBROWSE_INFOEXT,
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 0),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 2),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 5));
                }
                else if (pItem->kind == svn_node_dir)
                {
                    temp.FormatMessage(IDS_REPOBROWSE_INFODIR,
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 0),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 2),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 3),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 5));
                }
                else
                {
                    temp.FormatMessage(IDS_REPOBROWSE_INFOFILE,
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 0),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 2),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 3),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 4),
                        (LPCTSTR)m_RepoList.GetItemText(pNMLV->iItem, 5));
                }
                SetDlgItemText(IDC_INFOLABEL, temp);
            }
            else
                SetDlgItemText(IDC_INFOLABEL, L"");
        }
    }
}

void CRepositoryBrowser::OnLvnBeginlabeleditRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *info = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

    CAutoReadLock locker(m_guard);
    // disable rename for externals and the root
    CItem * item = (CItem *)m_RepoList.GetItemData (info->item.iItem);
    *pResult = (item == NULL) || (item->is_external) || (item->absolutepath.Compare(GetRepoRoot()) == 0)
             ? TRUE
             : FALSE;
}

void CRepositoryBrowser::OnLvnEndlabeleditRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    if (pDispInfo->item.pszText == NULL)
        return;

    CTSVNPath targetUrl;
    CString uuid;
    CString absolutepath;
    {
        CAutoReadLock locker(m_guard);
        // rename the item in the repository
        CItem * pItem = (CItem *)m_RepoList.GetItemData(pDispInfo->item.iItem);

        targetUrl = CTSVNPath(EscapeUrl(CTSVNPath(pItem->absolutepath.Left(pItem->absolutepath.ReverseFind('/')+1)+pDispInfo->item.pszText)));
        if(!CheckAndConfirmPath(targetUrl))
            return;
        uuid = pItem->repository.uuid;
        absolutepath = pItem->absolutepath;
    }

    CString sLogMsg;
    CTSVNPathList plist = CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(absolutepath))));
    if (!RunStartCommit(plist, sLogMsg))
        return;
    CInputLogDlg input(this);
    input.SetPathList(plist);
    input.SetRootPath(CTSVNPath(m_InitialUrl));
    input.SetLogText(sLogMsg);
    input.SetUUID (uuid);
    input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEMOVE);
    CString sHint;
    sHint.FormatMessage(IDS_INPUT_RENAME, (LPCTSTR)(absolutepath), (LPCTSTR)targetUrl.GetUIPathString());
    input.SetActionText(sHint);
    //input.SetForceFocus (true);
    if (input.DoModal() == IDOK)
    {
        sLogMsg = input.GetLogMessage();
        if (!RunPreCommit(plist, svn_depth_unknown, sLogMsg))
            return;
        CWaitCursorEx wait_cursor;

        bool bRet = Move(plist,
                         targetUrl,
                         sLogMsg,
                         false, false, false, false,
                         input.m_revProps);
        RunPostCommit(plist, svn_depth_unknown, m_commitRev, sLogMsg);
        if (!bRet || !PostCommitErr.IsEmpty())
        {
            wait_cursor.Hide();
            ShowErrorDialog(m_hWnd);
            if (!bRet)
                return;
        }
        *pResult = 0;

        InvalidateData (m_RepoTree.GetSelectedItem());
        RefreshNode(m_RepoTree.GetSelectedItem(), true);
    }
}

void CRepositoryBrowser::OnTvnBeginlabeleditRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    if (m_bSparseCheckoutMode)
    {
        *pResult = TRUE;
        return;
    }

    NMTVDISPINFO* info = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
    // disable rename for externals
    CAutoReadLock locker(m_guard);
    CTreeItem* item = (CTreeItem *)m_RepoTree.GetItemData (info->item.hItem);
    *pResult = (item == NULL) || (item->is_external) || (item->url.Compare(GetRepoRoot()) == 0)
             ? TRUE
             : FALSE;
}

void CRepositoryBrowser::OnTvnEndlabeleditRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    if (m_bSparseCheckoutMode)
        return;

    LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
    if (pTVDispInfo->item.pszText == NULL)
        return;

    // rename the item in the repository
    HTREEITEM hSelectedItem = pTVDispInfo->item.hItem;
    CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData(hSelectedItem);
    if (pItem == NULL)
        return;

    SRepositoryInfo repository = pItem->repository;

    CTSVNPath targetUrl = CTSVNPath(EscapeUrl(CTSVNPath(pItem->url.Left(pItem->url.ReverseFind('/')+1)+pTVDispInfo->item.pszText)));
    if(!CheckAndConfirmPath(targetUrl))
        return;
    CTSVNPathList plist = CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(pItem->url))));
    CString sLogMsg;
    if (!RunStartCommit(plist, sLogMsg))
        return;
    CInputLogDlg input(this);
    input.SetPathList(plist);
    input.SetRootPath(CTSVNPath(m_InitialUrl));
    input.SetLogText(sLogMsg);
    input.SetUUID(pItem->repository.uuid);
    input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEMOVE);
    CString sHint;
    sHint.FormatMessage(IDS_INPUT_RENAME, (LPCTSTR)(pItem->url), (LPCTSTR)targetUrl.GetUIPathString());
    input.SetActionText(sHint);
    //input.SetForceFocus (true);
    if (input.DoModal() == IDOK)
    {
        sLogMsg = input.GetLogMessage();
        if (!RunPreCommit(plist, svn_depth_unknown, sLogMsg))
            return;
        CWaitCursorEx wait_cursor;

        bool bRet = Move(plist,
                         targetUrl,
                         sLogMsg,
                         false, false, false, false,
                         input.m_revProps);
        RunPostCommit(plist, svn_depth_unknown, m_commitRev, sLogMsg);
        if (!bRet || !PostCommitErr.IsEmpty())
        {
            wait_cursor.Hide();
            ShowErrorDialog(m_hWnd);
            if (!bRet)
                return;
        }

        HTREEITEM parent = m_RepoTree.GetParentItem (hSelectedItem);
        InvalidateData (parent);

        *pResult = TRUE;
        if (pItem->url.Compare(m_barRepository.GetCurrentUrl()) == 0)
        {
            m_barRepository.ShowUrl(targetUrl.GetSVNPathString(), m_barRepository.GetCurrentRev());
        }

        pItem->url = targetUrl.GetUIPathString();
        pItem->unescapedname = pTVDispInfo->item.pszText;
        pItem->repository = repository;
        m_RepoTree.SetItemData(hSelectedItem, (DWORD_PTR)pItem);

        RefreshChildren (parent);

        if (hSelectedItem == m_RepoTree.GetSelectedItem())
            RefreshNode (hSelectedItem, true);
    }
}

void CRepositoryBrowser::OnCbenDragbeginUrlcombo(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    // build copy source / content
    std::unique_ptr<CIDropSource> pdsrc(new CIDropSource);
    if (pdsrc == NULL)
        return;

    pdsrc->AddRef();

    const SVNRev& revision = GetRevision();
    SVNDataObject* pdobj = new SVNDataObject(CTSVNPathList(CTSVNPath(GetPath())), revision, revision, true);
    if (pdobj == NULL)
    {
        return;
    }
    pdobj->AddRef();
    pdobj->SetAsyncMode(TRUE);
    CDragSourceHelper dragsrchelper;
    POINT point;
    GetCursorPos(&point);
    dragsrchelper.InitializeFromWindow(m_barRepository.GetComboWindow(), point, pdobj);
    pdsrc->m_pIDataObj = pdobj;
    pdsrc->m_pIDataObj->AddRef();

    // Initiate the Drag & Drop
    DWORD dwEffect;
    ::DoDragDrop(pdobj, pdsrc.get(), DROPEFFECT_LINK, &dwEffect);
    pdsrc->Release();
    pdsrc.release();
    pdobj->Release();

    *pResult = 0;
}

void CRepositoryBrowser::OnLvnBeginrdragRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
    OnLvnBegindragRepolist(pNMHDR, pResult);
}

void CRepositoryBrowser::OnLvnBegindragRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    OnBeginDrag(pNMHDR);
}

void CRepositoryBrowser::OnBeginDrag(NMHDR *pNMHDR)
{
    if (m_RepoList.HasText())
        return;

    // get selected paths

    CRepositoryBrowserSelection selection;

    CAutoReadLock locker(m_guard);
    POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
    if (pos == NULL)
        return;
    int index = -1;
    while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
        selection.Add ((CItem *)m_RepoList.GetItemData(index));

    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    BeginDrag(m_RepoList, selection, pNMLV->ptAction, true);
}

void CRepositoryBrowser::OnTvnBegindragRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    if (m_bSparseCheckoutMode)
        return;
    OnBeginDragTree(pNMHDR);
}

void CRepositoryBrowser::OnTvnBeginrdragRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    OnTvnBegindragRepotree(pNMHDR, pResult);
}

void CRepositoryBrowser::OnBeginDragTree(NMHDR *pNMHDR)
{
    if (m_blockEvents)
        return;

    // get selected paths

    CRepositoryBrowserSelection selection;
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    CTreeItem * pItem = (CTreeItem *)pNMTreeView->itemNew.lParam;
    if (pItem && (pItem->bookmark || pItem->dummy))
        return;

    selection.Add (pItem);

    BeginDrag(m_RepoTree, selection, pNMTreeView->ptDrag, false);
}

bool CRepositoryBrowser::OnDrop(const CTSVNPath& target, const CString& root, const CTSVNPathList& pathlist, const SVNRev& srcRev, DWORD dwEffect, POINTL /*pt*/)
{
    if (m_bSparseCheckoutMode)
        return false;
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": dropped %ld items on %s, source revision is %s, dwEffect is %ld\n", pathlist.GetCount(), (LPCTSTR)target.GetSVNPathString(), srcRev.ToString(), dwEffect);
    if (pathlist.GetCount() == 0)
        return false;

    if (!CTSVNPath(root).IsAncestorOf (pathlist[0]) && pathlist[0].IsUrl())
        return false;

    CString targetName = pathlist[0].GetFileOrDirectoryName();
    bool sameParent = pathlist[0].GetContainingDirectory() == target;

    if (m_bRightDrag)
    {
        // right dragging means we have to show a context menu
        POINT pt;
        DWORD ptW = GetMessagePos();
        pt.x = GET_X_LPARAM(ptW);
        pt.y = GET_Y_LPARAM(ptW);
        CMenu popup;
        if (popup.CreatePopupMenu())
        {
            CString temp;
            if (!sameParent)
            {
                temp.LoadString(IDS_REPOBROWSE_COPYDROP);
                popup.AppendMenu(MF_STRING | MF_ENABLED, 1, temp);
                if (PathIsURL(pathlist[0]))
                {
                    temp.LoadString(IDS_REPOBROWSE_MOVEDROP);
                    popup.AppendMenu(MF_STRING | MF_ENABLED, 2, temp);
                }
            }

            if ((pathlist.GetCount() == 1)&&(PathIsURL(pathlist[0])))
            {
                // these entries are only shown if *one* item was dragged, and if the
                // item is not one dropped from e.g. the explorer but from the repository
                // browser itself.
                if (!sameParent)
                    popup.AppendMenu(MF_SEPARATOR, 3);

                temp.LoadString(IDS_REPOBROWSE_COPYRENAMEDROP);
                popup.AppendMenu(MF_STRING | MF_ENABLED, 4, temp);
                temp.LoadString(IDS_REPOBROWSE_MOVERENAMEDROP);
                popup.AppendMenu(MF_STRING | MF_ENABLED, 5, temp);
            }
            int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, this, 0);
            switch (cmd)
            {
            default:// nothing clicked
                return false;
            case 1: // copy drop
                dwEffect = DROPEFFECT_COPY;
                break;
            case 2: // move drop
                dwEffect = DROPEFFECT_MOVE;
                break;
            case 4: // copy rename drop
                {
                    dwEffect = DROPEFFECT_COPY;
                    CRenameDlg dlg;
                    dlg.m_name = targetName;
                    dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_RENAME);
                    dlg.SetRenameRequired(sameParent);
                    CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
                    if (dlg.DoModal() != IDOK)
                        return false;
                    targetName = dlg.m_name;
                    if(!CheckAndConfirmPath(CTSVNPath(targetName)))
                        return false;
                }
                break;
            case 5: // move rename drop
                {
                    dwEffect = DROPEFFECT_MOVE;
                    CRenameDlg dlg;
                    dlg.m_name = targetName;
                    dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_RENAME);
                    dlg.SetRenameRequired(sameParent);
                    CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
                    if (dlg.DoModal() != IDOK)
                        return false;
                    targetName = dlg.m_name;
                    if(!CheckAndConfirmPath(CTSVNPath(targetName)))
                        return false;
                }
                break;
            }
        }

    }
    // check the first item in the path list:
    // if it's an url, we do a copy or move operation
    // if it's a local path, we do an import
    if (PathIsURL(pathlist[0]))
    {
        // If any of the paths are 'special' (branches, tags, or trunk) and we are
        // about to perform a move, we should warn the user and get them to confirm
        // that this is what they intended. Yes, I *have* accidentally moved the
        // trunk when I was trying to create a tag! :)
        if (DROPEFFECT_COPY != dwEffect)
        {
            bool pathListIsSpecial = false;
            int pathCount = pathlist.GetCount();
            for (int i=0 ; i<pathCount ; ++i)
            {
                const CTSVNPath& path = pathlist[i];
                if (path.IsSpecialDirectory())
                {
                    pathListIsSpecial = true;
                    break;
                }
            }
            if (pathListIsSpecial)
            {
                CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY_TASK1)),
                                    CString(MAKEINTRESOURCE(IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY_TASK3)));
                taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY_TASK4)));
                taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskdlg.SetDefaultCommandControl(2);
                taskdlg.SetMainIcon(TD_WARNING_ICON);
                if (taskdlg.DoModal(m_hWnd) != 1)
                    return false;
            }
        }

        // drag-n-drop inside the repobrowser
        CString sLogMsg;
        if (!RunStartCommit(pathlist, sLogMsg))
            return false;
        CInputLogDlg input(this);
        input.SetPathList(pathlist);
        input.SetRootPath(CTSVNPath(m_InitialUrl));
        input.SetLogText(sLogMsg);
        input.SetUUID(m_repository.uuid);
        input.SetProjectProperties(&m_ProjectProperties, dwEffect == DROPEFFECT_COPY ? PROJECTPROPNAME_LOGTEMPLATEBRANCH : PROJECTPROPNAME_LOGTEMPLATEMOVE);
        CString sHint;
        if (pathlist.GetCount() == 1)
        {
            if (dwEffect == DROPEFFECT_COPY)
                sHint.FormatMessage(IDS_INPUT_COPY, (LPCTSTR)pathlist[0].GetSVNPathString(), (LPCTSTR)(target.GetSVNPathString()+L"/"+targetName));
            else
                sHint.FormatMessage(IDS_INPUT_MOVE, (LPCTSTR)pathlist[0].GetSVNPathString(), (LPCTSTR)(target.GetSVNPathString()+L"/"+targetName));
        }
        else
        {
            if (dwEffect == DROPEFFECT_COPY)
                sHint.FormatMessage(IDS_INPUT_COPYMORE, pathlist.GetCount(), (LPCTSTR)target.GetSVNPathString());
            else
                sHint.FormatMessage(IDS_INPUT_MOVEMORE, pathlist.GetCount(), (LPCTSTR)target.GetSVNPathString());
        }
        input.SetActionText(sHint);
        if (input.DoModal() == IDOK)
        {
            sLogMsg = input.GetLogMessage();
            if (!RunPreCommit(pathlist, svn_depth_unknown, sLogMsg))
                return false;
            CWaitCursorEx wait_cursor;
            BOOL bRet = FALSE;
            if (dwEffect == DROPEFFECT_COPY)
                if (pathlist.GetCount() == 1)
                    bRet = Copy(pathlist, CTSVNPath(target.GetSVNPathString() + L"/" + targetName), srcRev, srcRev, sLogMsg, false, false, false, false, SVNExternals(), input.m_revProps);
                else
                    bRet = Copy(pathlist, target, srcRev, srcRev, sLogMsg, true, false, false, false, SVNExternals(), input.m_revProps);
            else
                if (pathlist.GetCount() == 1)
                    bRet = Move(pathlist, CTSVNPath(target.GetSVNPathString() + L"/" + targetName), sLogMsg, false, false, false, false, input.m_revProps);
                else
                    bRet = Move(pathlist, target, sLogMsg, true, false, false, false, input.m_revProps);
            RunPostCommit(pathlist, svn_depth_unknown, m_commitRev, sLogMsg);
            if (!bRet || !PostCommitErr.IsEmpty())
            {
                wait_cursor.Hide();
                ShowErrorDialog(m_hWnd);
            }
            else if (GetRevision().IsHead())
            {
                // mark the target as dirty
                HTREEITEM hTarget = FindUrl(target.GetSVNPathString());
                InvalidateData (hTarget);
                if (hTarget)
                {
                    CAutoWriteLock locker(m_guard);
                    CTreeItem * pItem = (CTreeItem*)m_RepoTree.GetItemData(hTarget);
                    if (pItem)
                    {
                        // mark the target as 'dirty'
                        pItem->children_fetched = false;
                        pItem->error.Empty();
                        RecursiveRemove(hTarget, true);
                        TVITEM tvitem = {0};
                        tvitem.hItem = hTarget;
                        tvitem.mask = TVIF_CHILDREN;
                        tvitem.cChildren = 1;
                        m_RepoTree.SetItem(&tvitem);
                    }
                }
                if (dwEffect == DROPEFFECT_MOVE)
                {
                    // if items were moved, we have to
                    // invalidate all sources too
                    for (int i=0; i<pathlist.GetCount(); ++i)
                    {
                        HTREEITEM hSource = FindUrl(pathlist[i].GetSVNPathString());
                        InvalidateData (hSource);
                        if (hSource)
                        {
                            CAutoWriteLock locker(m_guard);
                            CTreeItem * pItem = (CTreeItem*)m_RepoTree.GetItemData(hSource);
                            if (pItem)
                            {
                                // the source has moved, so remove it!
                                RecursiveRemove(hSource);
                                m_RepoTree.DeleteItem(hSource);
                            }
                        }
                    }
                }

                // if the copy/move operation was to the currently shown url,
                // update the current view. Otherwise mark the target URL as 'not fetched'.
                HTREEITEM hSelected = m_RepoTree.GetSelectedItem();
                InvalidateData (hSelected);

                if (hSelected)
                {
                    CAutoWriteLock locker(m_guard);
                    CTreeItem * pItem = (CTreeItem*)m_RepoTree.GetItemData(hSelected);
                    if (pItem)
                    {
                        // mark the target as 'dirty'
                        pItem->children_fetched = false;
                        pItem->error.Empty();
                        if ((dwEffect == DROPEFFECT_MOVE)||(pItem->url.Compare(target.GetSVNPathString())==0))
                        {
                            // Refresh the current view
                            RefreshNode(hSelected, true);
                        }
                    }
                }
            }
        }
    }
    else
    {
        // import files dragged onto us
        if (pathlist.GetCount() > 1)
        {
            CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_MULTIIMPORT_TASK1)),
                                CString(MAKEINTRESOURCE(IDS_REPOBROWSE_MULTIIMPORT_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_REPOBROWSE_MULTIIMPORT_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_REPOBROWSE_MULTIIMPORT_TASK4)));
            taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskdlg.SetDefaultCommandControl(2);
            taskdlg.SetMainIcon(TD_WARNING_ICON);
            if (taskdlg.DoModal(m_hWnd) != 1)
                return false;
        }

        CString sLogMsg;
        if (!RunStartCommit(pathlist, sLogMsg))
            return false;
        CInputLogDlg input(this);
        input.SetPathList(pathlist);
        input.SetRootPath(CTSVNPath(m_InitialUrl));
        input.SetLogText(sLogMsg);
        input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEIMPORT);
        input.SetUUID(m_repository.root);
        CString sHint;
        if (pathlist.GetCount() == 1)
            sHint.FormatMessage(IDS_INPUT_IMPORTFILEFULL, pathlist[0].GetWinPath(), (LPCTSTR)(target.GetSVNPathString() + L"/" + pathlist[0].GetFileOrDirectoryName()));
        else
            sHint.Format(IDS_INPUT_IMPORTFILES, pathlist.GetCount());
        input.SetActionText(sHint);

        if (input.DoModal() == IDOK)
        {
            sLogMsg = input.GetLogMessage();
            if (!RunPreCommit(pathlist, svn_depth_unknown, sLogMsg))
                return false;
            for (int importindex = 0; importindex<pathlist.GetCount(); ++importindex)
            {
                CString filename = pathlist[importindex].GetFileOrDirectoryName();
                bool bRet = Import(pathlist[importindex],
                                   CTSVNPath(target.GetSVNPathString()+L"/"+filename),
                                   sLogMsg, &m_ProjectProperties, svn_depth_infinity, true, true, false, input.m_revProps);
                if (!bRet || !PostCommitErr.IsEmpty())
                {
                    ShowErrorDialog(m_hWnd);
                    if (!bRet)
                        break;
                }
            }
            RunPostCommit(pathlist, svn_depth_unknown, m_commitRev, sLogMsg);
            if (GetRevision().IsHead())
            {
                // if the import operation was to the currently shown url,
                // update the current view. Otherwise mark the target URL as 'not fetched'.
                HTREEITEM hSelected = m_RepoTree.GetSelectedItem();
                InvalidateData (hSelected);
                if (hSelected)
                {
                    CAutoWriteLock locker(m_guard);
                    CTreeItem * pItem = (CTreeItem*)m_RepoTree.GetItemData(hSelected);
                    if (pItem)
                    {
                        // don't access outdated query results
                        if (pItem->url.Compare(target.GetSVNPathString())==0)
                        {
                            // Refresh the current view
                            RefreshNode(hSelected, true);
                        }
                        else
                        {
                            // only mark the target as 'dirty'
                            pItem->children_fetched = false;
                            pItem->error.Empty();
                        }
                    }
                }
            }
        }

    }
    return true;
}

CString CRepositoryBrowser::EscapeUrl(const CTSVNPath& url)
{
    CStringA prepUrl = CUnicodeUtils::GetUTF8(url.GetSVNPathString());
    prepUrl.Replace("%", "%25");
    return CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(prepUrl));
}

void CRepositoryBrowser::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (m_bSparseCheckoutMode)
        return;

    HTREEITEM hSelectedTreeItem = NULL;
    HTREEITEM hChosenTreeItem = NULL;
    if ((point.x == -1) && (point.y == -1))
    {
        if (pWnd == &m_RepoTree)
        {
            CRect rect;
            m_RepoTree.GetItemRect(m_RepoTree.GetSelectedItem(), &rect, TRUE);
            m_RepoTree.ClientToScreen(&rect);
            point = rect.CenterPoint();
        }
        else
        {
            CRect rect;
            POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
            if (pos)
            {
                m_RepoList.GetItemRect(m_RepoList.GetNextSelectedItem(pos), &rect, LVIR_LABEL);
                m_RepoList.ClientToScreen(&rect);
                point = rect.CenterPoint();
            }
        }
    }

    bool bSVNParentPathUrl = false;
    bool bIsBookmark = false;
    CRepositoryBrowserSelection selection;
    if (pWnd == &m_RepoList)
    {
        CString urls;
        CAutoReadLock locker(m_guard);

        POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
        if (pos)
        {
            int index = -1;
            while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
                selection.Add ((CItem *)m_RepoList.GetItemData (index));
        }

        if (selection.IsEmpty())
        {
            // Right-click outside any list control items. It may be the background,
            // but it also could be the list control headers.
            CRect hr;
            m_RepoList.GetHeaderCtrl()->GetWindowRect(&hr);
            if (!hr.PtInRect(point))
            {
                // Seems to be a right-click on the list view background.
                // Use the currently selected item in the tree view as the source.
                ++m_blockEvents;
                hSelectedTreeItem = m_RepoTree.GetSelectedItem();
                if (hSelectedTreeItem)
                {
                    m_RepoTree.SetItemState(hSelectedTreeItem, 0, TVIS_SELECTED);
                    --m_blockEvents;
                    m_RepoTree.SetItemState(hSelectedTreeItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
                    CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData (hSelectedTreeItem);
                    bSVNParentPathUrl = pTreeItem->svnparentpathroot;
                    selection.Add (pTreeItem);
                }
                else
                    --m_blockEvents;
            }
        }
    }
    if ((pWnd == &m_RepoTree)|| selection.IsEmpty())
    {
        CAutoReadLock locker(m_guard);
        UINT uFlags;
        CPoint ptTree = point;
        m_RepoTree.ScreenToClient(&ptTree);
        HTREEITEM hItem = m_RepoTree.HitTest(ptTree, &uFlags);
        // in case the right-clicked item is not the selected one,
        // use the TVIS_DROPHILITED style to indicate on which item
        // the context menu will work on
        if ((hItem) && (uFlags & TVHT_ONITEM) && (hItem != m_RepoTree.GetSelectedItem()))
        {
            ++m_blockEvents;
            hSelectedTreeItem = m_RepoTree.GetSelectedItem();
            m_RepoTree.SetItemState(hSelectedTreeItem, 0, TVIS_SELECTED);
            --m_blockEvents;
            m_RepoTree.SetItemState(hItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
        }
        if (hItem)
        {
            hChosenTreeItem = hItem;
            CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData (hItem);
            bSVNParentPathUrl = pTreeItem->svnparentpathroot;
            if (pTreeItem->dummy)
                return;
            if (pTreeItem->bookmark)
            {
                bIsBookmark = true;
                if (pTreeItem->url.IsEmpty())
                    return;
            }
            selection.Add (pTreeItem);
        }
    }

    if (selection.GetRepositoryCount() != 1)
        return;

    CIconMenu popup;
    CIconMenu clipSubMenu;
    if (popup.CreatePopupMenu())
    {
        if (!bSVNParentPathUrl && !bIsBookmark)
        {
            if (selection.GetPathCount (0) == 1)
            {
                if (selection.GetFolderCount (0) == 0)
                {
                    // Let "Open" be the very first entry, like in Explorer
                    popup.AppendMenuIcon(ID_OPEN, IDS_REPOBROWSE_OPEN, IDI_OPEN);       // "open"
                    popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);    // "open with..."
                    popup.AppendMenuIcon(ID_EDITFILE, IDS_REPOBROWSE_EDITFILE);     // "edit"
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                }
            }
            if ( (selection.GetPathCount (0) == 1) ||
                ((selection.GetPathCount (0) == 2) && (selection.GetFolderCount (0) != 1)) )
            {
                popup.AppendMenuIcon(ID_SHOWLOG, IDS_REPOBROWSE_SHOWLOG, IDI_LOG);          // "Show Log..."
            }
            if (selection.GetPathCount (0) == 1)
            {
                // the revision graph on the repository root would be empty. We
                // don't show the context menu entry there.
                if (!selection.IsRoot (0, 0))
                {
                    popup.AppendMenuIcon(ID_REVGRAPH, IDS_MENUREVISIONGRAPH, IDI_REVISIONGRAPH); // "Revision graph"
                }
                if (!selection.IsFolder (0, 0))
                {
                    popup.AppendMenuIcon(ID_BLAME, IDS_MENUBLAME, IDI_BLAME);       // "Blame..."
                }
                if (!m_ProjectProperties.sWebViewerRev.IsEmpty())
                {
                    popup.AppendMenuIcon(ID_VIEWREV, IDS_LOG_POPUP_VIEWREV);        // "View revision in webviewer"
                }
                if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
                {
                    popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);    // "View revision for path in webviewer"
                }
                if ((!m_ProjectProperties.sWebViewerPathRev.IsEmpty())||
                    (!m_ProjectProperties.sWebViewerRev.IsEmpty()))
                {
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                }

                // we can export files and folders alike, but for files we have "save as"
                if (selection.IsFolder (0, 0))
                    popup.AppendMenuIcon(ID_EXPORT, IDS_MENUEXPORT, IDI_EXPORT);        // "Export"
            }

            // We allow checkout of multiple paths at once (we do that one by one)
            popup.AppendMenuIcon(ID_CHECKOUT, IDS_MENUCHECKOUT, IDI_CHECKOUT);      // "Checkout.."
        }

        if ((selection.GetPathCount (0) == 1) && !bIsBookmark)
        {
            if (selection.IsFolder (0, 0))
            {
                popup.AppendMenuIcon(ID_REFRESH, IDS_REPOBROWSE_REFRESH, IDI_REFRESH);      // "Refresh"
            }
            popup.AppendMenu(MF_SEPARATOR, NULL);

            if (!bSVNParentPathUrl)
            {
                if (selection.GetRepository(0).revision.IsHead())
                {
                    if (selection.IsFolder (0, 0))
                    {
                        popup.AppendMenuIcon(ID_MKDIR, IDS_REPOBROWSE_MKDIR, IDI_MKDIR);    // "create directory"
                        popup.AppendMenuIcon(ID_IMPORT, IDS_REPOBROWSE_IMPORT, IDI_IMPORT); // "Add/Import File"
                        popup.AppendMenuIcon(ID_IMPORTFOLDER, IDS_REPOBROWSE_IMPORTFOLDER, IDI_IMPORT); // "Add/Import Folder"
                        popup.AppendMenu(MF_SEPARATOR, NULL);
                    }

                    if (!selection.IsExternal (0, 0) && !selection.IsRoot (0, 0))
                        popup.AppendMenuIcon(ID_RENAME, IDS_REPOBROWSE_RENAME, IDI_RENAME);     // "Rename"
                }
                if (selection.IsLocked (0, 0))
                {
                    popup.AppendMenuIcon(ID_BREAKLOCK, IDS_MENU_UNLOCKFORCE, IDI_UNLOCK);   // "Break Lock"
                }
            }
            popup.AppendMenuIcon(ID_ADDTOBOOKMARKS, IDS_REPOBROWSE_ADDBOOKMARK, IDI_BOOKMARKS); // "add to bookmarks"
        }

        if (!bSVNParentPathUrl && !bIsBookmark)
        {
            if (selection.GetRepository(0).revision.IsHead() && !selection.IsRoot (0, 0))
            {
                popup.AppendMenuIcon(ID_DELETE, IDS_REPOBROWSE_DELETE, IDI_DELETE);     // "Remove"
            }
            if (selection.GetFolderCount(0) == 0)
            {
                popup.AppendMenuIcon(ID_SAVEAS, IDS_REPOBROWSE_SAVEAS, IDI_SAVEAS);     // "Save as..."
            }
        }
        if (clipSubMenu.CreatePopupMenu())
        {
            if (pWnd == &m_RepoList)
                clipSubMenu.AppendMenuIcon(ID_FULLTOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_FULL, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_URLTOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_URL, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_NAMETOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_FILENAMES, IDI_COPYCLIP);
            if (pWnd == &m_RepoList)
            {
                clipSubMenu.AppendMenuIcon(ID_REVISIONTOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_REVS, IDI_COPYCLIP);
                clipSubMenu.AppendMenuIcon(ID_AUTHORTOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_AUTHORS, IDI_COPYCLIP);
            }

            CString temp;
            temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
            popup.InsertMenu((UINT)-1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)clipSubMenu.m_hMenu, temp);

        }
        if (!bSVNParentPathUrl && !bIsBookmark)
        {
            if (   (selection.GetFolderCount(0) == selection.GetPathCount(0))
                || (selection.GetFolderCount(0) == 0))
            {
                popup.AppendMenuIcon(ID_COPYTOWC, IDS_REPOBROWSE_COPYTOWC); // "Copy To Working Copy..."
            }

            if (selection.GetPathCount(0) == 1)
            {
                popup.AppendMenuIcon(ID_COPYTO, IDS_REPOBROWSE_COPY, IDI_COPY);         // "Copy To..."
                popup.AppendMenu(MF_SEPARATOR, NULL);
                popup.AppendMenuIcon(ID_PROPS, IDS_REPOBROWSE_SHOWPROP, IDI_PROPERTIES);            // "Show Properties"
                // Revision properties are not associated to paths
                // so we only show that context menu on the repository root
                if (selection.IsRoot (0, 0))
                {
                    popup.AppendMenuIcon(ID_REVPROPS, IDS_REPOBROWSE_SHOWREVPROP, IDI_PROPERTIES);  // "Show Revision Properties"
                }
                if (selection.IsFolder (0, 0))
                {
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                    popup.AppendMenuIcon(ID_PREPAREDIFF, IDS_REPOBROWSE_PREPAREDIFF);   // "Mark for comparison"

                    CTSVNPath root (selection.GetRepository(0).root);
                    if (   (m_diffKind == svn_node_dir)
                        && !m_diffURL.IsEquivalentTo (selection.GetURL (0, 0))
                        && root.IsAncestorOf (m_diffURL))
                    {
                        popup.AppendMenuIcon(ID_GNUDIFF, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);      // "Show differences as unified diff"
                        popup.AppendMenuIcon(ID_DIFF, IDS_REPOBROWSE_SHOWDIFF, IDI_DIFF);       // "Compare URLs"
                        popup.AppendMenuIcon(ID_DIFF_CONTENTONLY, IDS_REPOBROWSE_SHOWDIFF_CONTENTONLY, IDI_DIFF);       // "Compare URLs"
                    }
                }
            }

            if (   (selection.GetPathCount (0) == 2)
                && (selection.GetFolderCount (0) != 1))
            {
                popup.AppendMenu(MF_SEPARATOR, NULL);
                popup.AppendMenuIcon(ID_GNUDIFF, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);      // "Show differences as unified diff"
                popup.AppendMenuIcon(ID_DIFF, IDS_REPOBROWSE_SHOWDIFF, ID_DIFF);        // "Compare URLs"
                popup.AppendMenuIcon(ID_DIFF_CONTENTONLY, IDS_REPOBROWSE_SHOWDIFF_CONTENTONLY, IDI_DIFF);       // "Compare URLs"
                popup.AppendMenu(MF_SEPARATOR, NULL);
            }

            if (m_path.Exists() &&
                CTSVNPath(m_InitialUrl).IsAncestorOf (selection.GetURL (0, 0)))
            {
                CTSVNPath wcPath = m_path;
                wcPath.AppendPathString(selection.GetURL (0, 0).GetWinPathString().Mid(m_InitialUrl.GetLength()));
                if (!wcPath.Exists())
                {
                    bool bWCPresent = false;
                    while (!bWCPresent && m_path.IsAncestorOf(wcPath))
                    {
                        bWCPresent = wcPath.GetContainingDirectory().Exists();
                        wcPath = wcPath.GetContainingDirectory();
                    }
                    if (bWCPresent)
                    {
                        popup.AppendMenu(MF_SEPARATOR, NULL);
                        popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATEREV, IDI_UPDATE);      // "Update item to revision"
                    }
                }
                else
                {
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                    popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATEREV, IDI_UPDATE);      // "Update item to revision"
                }
            }
        }
        popup.AppendMenuIcon(ID_CREATELINK, IDS_REPOBROWSE_CREATELINK, IDI_LINK);
        if (bIsBookmark)
            popup.AppendMenuIcon(ID_REMOVEBOOKMARKS, IDS_REPOBROWSE_REMOVEBOOKMARK, IDI_BOOKMARKS);
        int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, 0);

        auto stopJobs (m_lister.SuspendJobs());
        if (pWnd == &m_RepoTree)
        {
            CAutoWriteLock locker(m_guard);
            UINT uFlags;
            CPoint ptTree = point;
            m_RepoTree.ScreenToClient(&ptTree);
            HTREEITEM hItem = m_RepoTree.HitTest(ptTree, &uFlags);
            // restore the previously selected item state
            if ((hItem) && (uFlags & TVHT_ONITEM) && (hItem != m_RepoTree.GetSelectedItem()))
            {
                ++m_blockEvents;
                m_RepoTree.SetItemState(hSelectedTreeItem, TVIS_SELECTED, TVIS_SELECTED);
                --m_blockEvents;
                m_RepoTree.SetItemState(hItem, 0, TVIS_DROPHILITED);
            }
        }
        if (hSelectedTreeItem)
        {
            CAutoWriteLock locker(m_guard);
            ++m_blockEvents;
            m_RepoTree.SetItemState(hSelectedTreeItem, 0, TVIS_DROPHILITED);
            m_RepoTree.SetItemState(hSelectedTreeItem, TVIS_SELECTED, TVIS_SELECTED);
            --m_blockEvents;
        }
        DialogEnableWindow(IDOK, FALSE);
        bool bOpenWith = false;
        bool bIgnoreProps = false;
        switch (cmd)
        {
        case ID_UPDATE:
            {
                CTSVNPathList updateList;
                for (size_t i=0; i < selection.GetPathCount(0); ++i)
                {
                    CTSVNPath wcPath = m_path;
                    const CTSVNPath& url = selection.GetURL (0, i);
                    wcPath.AppendPathString(url.GetWinPathString().Mid(m_InitialUrl.GetLength()));
                    updateList.AddPath(wcPath);
                }
                if (updateList.GetCount())
                {
                    CTSVNPath tempFile = CTempFiles::Instance().GetTempFilePath(false);
                    VERIFY(updateList.WriteToFile(tempFile.GetWinPathString()));
                    CString sCmd;
                    sCmd.Format(L"/command:update /pathfile:\"%s\" /rev /deletepathfile",
                        tempFile.GetWinPath());

                    CAppUtils::RunTortoiseProc(sCmd);
                }
            }
            break;
        case ID_PREPAREDIFF:
            {
                CAutoWriteLock locker(m_guard);
                m_RepoTree.SetItemState(FindUrl(m_diffURL.GetSVNPathString()), 0, TVIS_BOLD);
                if (selection.GetPathCount(0) == 1)
                {
                    if (m_diffURL.IsEquivalentTo (selection.GetURL (0, 0)))
                    {
                        m_diffURL.Reset();
                        m_diffKind = svn_node_none;
                    }
                    else
                    {
                        m_diffURL = selection.GetURL (0, 0);
                        m_diffKind = selection.IsFolder (0, 0) ? svn_node_dir : svn_node_file;
                        // make the marked tree item bold
                        if (m_diffKind == svn_node_dir)
                        {
                            m_RepoTree.SetItemState(FindUrl(m_diffURL.GetSVNPathString()), TVIS_BOLD, TVIS_BOLD);
                        }
                    }
                }
                else
                {
                    m_diffURL.Reset();
                    m_diffKind = svn_node_none;
                }
            }
            break;
        case ID_URLTOCLIPBOARD:
        case ID_FULLTOCLIPBOARD:
        case ID_NAMETOCLIPBOARD:
        case ID_AUTHORTOCLIPBOARD:
        case ID_REVISIONTOCLIPBOARD:
            {
                CString sClipboard;

                if (pWnd == &m_RepoList)
                {
                    POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
                    if (pos)
                    {
                        int index = -1;
                        while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
                        {
                            CItem * pItem = (CItem *)m_RepoList.GetItemData (index);
                            if ((cmd == ID_URLTOCLIPBOARD) || (cmd == ID_FULLTOCLIPBOARD))
                            {
                                CString path = pItem->absolutepath;
                                path.Replace (L"\\", L"%5C");
                                sClipboard += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8 (path)));
                            }
                            if (cmd == ID_FULLTOCLIPBOARD)
                                sClipboard += L", ";
                            if (cmd == ID_URLTOCLIPBOARD)
                            {
                                if (!GetRevision().IsHead())
                                {
                                    sClipboard += L"?r=" + GetRevision().ToString();
                                }
                            }
                            if (cmd == ID_NAMETOCLIPBOARD)
                            {
                                CString u = pItem->absolutepath;
                                u.Replace (L"\\", L"%5C");
                                CTSVNPath path = CTSVNPath(u);
                                CString name = path.GetFileOrDirectoryName();
                                sClipboard += name;
                            }
                            if ((cmd == ID_AUTHORTOCLIPBOARD) || (cmd == ID_FULLTOCLIPBOARD))
                            {
                                sClipboard += pItem->author;
                            }
                            if (cmd == ID_FULLTOCLIPBOARD)
                                sClipboard += L", ";
                            if ((cmd == ID_REVISIONTOCLIPBOARD) || (cmd == ID_FULLTOCLIPBOARD))
                            {
                                CString temp;
                                temp.Format(L"%ld", pItem->created_rev);
                                sClipboard += temp;
                            }
                            sClipboard += L"\r\n";
                        }
                    }
                }
                else
                {
                    for (size_t i=0; i < selection.GetPathCount(0); ++i)
                    {
                        if ((cmd == ID_URLTOCLIPBOARD) || (cmd == ID_FULLTOCLIPBOARD))
                        {
                            CString path = selection.GetURL (0, i).GetSVNPathString();
                            sClipboard += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8 (path)));
                        }
                        if (cmd == ID_URLTOCLIPBOARD)
                        {
                            if (!GetRevision().IsHead())
                            {
                                sClipboard += L"?r=" + GetRevision().ToString();
                            }
                        }
                        if (cmd == ID_NAMETOCLIPBOARD)
                        {
                            CString name = selection.GetURL (0, i).GetFileOrDirectoryName();
                            sClipboard += name;
                        }
                        sClipboard += L"\r\n";
                    }
                }
                sClipboard.TrimRight(L"\r\n");
                CStringUtils::WriteAsciiStringToClipboard(sClipboard);
            }
            break;
        case ID_SAVEAS:
            {
                CTSVNPath tempfile;
                bool bSavePathOK = AskForSavePath (selection, tempfile, selection.GetFolderCount(0) > 0);
                if (bSavePathOK)
                {
                    CWaitCursorEx wait_cursor;

                    CString saveurl;
                    CProgressDlg progDlg;
                    DWORD counter = 0;      // the file counter
                    DWORD pathCount = (DWORD)selection.GetPathCount(0);
                    const SVNRev& revision = selection.GetRepository(0).revision;

                    progDlg.SetTitle(IDS_REPOBROWSE_SAVEASPROGTITLE);
                    progDlg.ShowModeless(GetSafeHwnd());
                    progDlg.SetProgress(0, pathCount);
                    SetAndClearProgressInfo(&progDlg);

                    for (DWORD i=0; i < pathCount; ++i)
                    {
                        saveurl = selection.GetURLEscaped (0, i).GetSVNPathString();
                        CTSVNPath savepath = tempfile;
                        if (tempfile.IsDirectory())
                            savepath.AppendPathString(selection.GetURL (0, i).GetFileOrDirectoryName());
                        CString sInfoLine;
                        sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)saveurl, (LPCTSTR)revision.ToString());
                        progDlg.SetLine(1, sInfoLine, true);
                        if (!Export(CTSVNPath(saveurl), savepath, revision, revision)||(progDlg.HasUserCancelled()))
                        {
                            wait_cursor.Hide();
                            progDlg.Stop();
                            SetAndClearProgressInfo((HWND)NULL);
                            if (!progDlg.HasUserCancelled())
                                ShowErrorDialog(m_hWnd);
                            DialogEnableWindow(IDOK, TRUE);
                            return;
                        }
                        counter++;
                        progDlg.SetProgress(counter, pathCount);
                    }
                    progDlg.Stop();
                    SetAndClearProgressInfo((HWND)NULL);
                }
            }
            break;
        case ID_SHOWLOG:
            {
                const CTSVNPath& firstPath = selection.GetURLEscaped(0, 0);
                const SVNRev& pegRevision = selection.GetRepository(0).revision;

                if (   (selection.GetPathCount(0) == 2)
                    || (!m_diffURL.IsEmpty() && !m_diffURL.IsEquivalentTo (selection.GetURL (0,0))))
                {
                    CTSVNPath secondUrl = selection.GetPathCount(0) == 2
                                        ? selection.GetURLEscaped(0, 1)
                                        : m_diffURL;

                    // get last common URL and revision
                    auto commonSource = SVNLogHelper().GetCommonSource
                                            (firstPath, pegRevision, secondUrl, pegRevision);
                    if (!commonSource.second.IsNumber())
                    {
                        // no common copy from URL, so showing a log between
                        // the two urls is not possible.
                        TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_NOCOMMONCOPYFROM), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
                        break;
                    }

                    // show log from pegrev to there

                    CString sCmd;
                    sCmd.Format(L"/command:log /path:\"%s\" /startrev:%s /endrev:%s",
                        (LPCTSTR)firstPath.GetSVNPathString(),
                        (LPCTSTR)pegRevision.ToString(),
                        (LPCTSTR)commonSource.second.ToString());

                    if (!m_path.IsUrl())
                    {
                        sCmd += L" /propspath:\"";
                        sCmd += m_path.GetWinPathString();
                        sCmd += L"\"";
                    }

                    CAppUtils::RunTortoiseProc(sCmd);
                }
                else
                {
                    CString sCmd;
                    sCmd.Format(L"/command:log /path:\"%s\" /startrev:%s",
                        (LPCTSTR)firstPath.GetSVNPathString(), (LPCTSTR)pegRevision.ToString());

                    if (!m_path.IsUrl())
                    {
                        sCmd += L" /propspath:\"";
                        sCmd += m_path.GetWinPathString();
                        sCmd += L"\"";
                    }

                    CAppUtils::RunTortoiseProc(sCmd);
                }
            }
            break;
        case ID_VIEWREV:
            {
                const SRepositoryInfo& repository = selection.GetRepository(0);
                CString weburl = m_ProjectProperties.sWebViewerRev;
                weburl = CAppUtils::GetAbsoluteUrlFromRelativeUrl(repository.root, weburl);
                SVNRev headrev = selection.GetRepository(0).revision.IsHead() ? m_barRepository.GetHeadRevision() : selection.GetRepository(0).revision;
                weburl.Replace(L"%REVISION%", headrev.ToString());
                if (!weburl.IsEmpty())
                    ShellExecute(this->m_hWnd, L"open", weburl, NULL, NULL, SW_SHOWDEFAULT);
            }
            break;
        case ID_VIEWPATHREV:
            {
                const SRepositoryInfo& repository = selection.GetRepository(0);
                CString relurl = selection.GetURLEscaped (0, 0).GetSVNPathString();
                relurl = relurl.Mid (repository.root.GetLength());
                CString weburl = m_ProjectProperties.sWebViewerPathRev;
                weburl = CAppUtils::GetAbsoluteUrlFromRelativeUrl(repository.root, weburl);
                SVNRev headrev = selection.GetRepository(0).revision.IsHead() ? m_barRepository.GetHeadRevision() : selection.GetRepository(0).revision;
                weburl.Replace(L"%REVISION%", headrev.ToString());
                weburl.Replace(L"%PATH%", relurl);
                if (!weburl.IsEmpty())
                    ShellExecute(this->m_hWnd, L"open", weburl, NULL, NULL, SW_SHOWDEFAULT);
            }
            break;
        case ID_CHECKOUT:
            {
                CString itemsToCheckout;
                for (size_t i=0; i < selection.GetPathCount(0); ++i)
                {
                    itemsToCheckout += selection.GetURL (0, i).GetSVNPathString() + L"*";
                }
                itemsToCheckout.TrimRight('*');
                CString sCmd;
                sCmd.Format ( L"/command:checkout /url:\"%s\" /revision:%s"
                            , (LPCTSTR)itemsToCheckout
                            , (LPCTSTR)selection.GetRepository(0).revision.ToString());

                CAppUtils::RunTortoiseProc(sCmd);
            }
            break;
        case ID_EXPORT:
            {
                CExportDlg dlg(this);
                dlg.m_URL = selection.GetURL (0, 0).GetSVNPathString();
                dlg.Revision = selection.GetRepository (0).revision;
                if (dlg.DoModal()==IDOK)
                {
                    CTSVNPath exportDirectory;
                    exportDirectory.SetFromWin(dlg.m_strExportDirectory, true);

                    CSVNProgressDlg progDlg;
                    int opts = 0;
                    if (dlg.m_bNoExternals)
                        opts |= ProgOptIgnoreExternals;
                    if (dlg.m_eolStyle.CompareNoCase(L"CRLF")==0)
                        opts |= ProgOptEolCRLF;
                    if (dlg.m_eolStyle.CompareNoCase(L"CR")==0)
                        opts |= ProgOptEolCR;
                    if (dlg.m_eolStyle.CompareNoCase(L"LF")==0)
                        opts |= ProgOptEolLF;
                    progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Export);
                    progDlg.SetOptions(opts);
                    progDlg.SetPathList(CTSVNPathList(exportDirectory));
                    progDlg.SetUrl(dlg.m_URL);
                    progDlg.SetRevision(dlg.Revision);
                    progDlg.SetDepth(dlg.m_depth);
                    progDlg.DoModal();
                }
            }
            break;
        case ID_REVGRAPH:
            {
                CString sCmd;
                sCmd.Format ( L"/command:revisiongraph /path:\"%s\" /pegrev:%s"
                            , (LPCTSTR)selection.GetURLEscaped (0, 0).GetSVNPathString()
                            , (LPCTSTR)selection.GetRepository (0).revision.ToString());

                CAppUtils::RunTortoiseProc(sCmd);
            }
            break;
        case ID_OPENWITH:
            bOpenWith = true;
        case ID_OPEN:
            {
                OpenFile(selection.GetURL (0,0), selection.GetURLEscaped (0,0), bOpenWith);
            }
            break;
        case ID_EDITFILE:
            {
                new async::CAsyncCall ( this
                                      , &CRepositoryBrowser::EditFile
                                      , selection.GetURL (0,0)
                                      , selection.GetURLEscaped (0,0)
                                      , &m_backgroundJobs);
            }
            break;
        case ID_DELETE:
            {
                CWaitCursorEx wait_cursor;
                CString sLogMsg;
                if (!RunStartCommit(selection.GetURLsEscaped(0), sLogMsg))
                    break;
                CInputLogDlg input(this);
                input.SetPathList(selection.GetURLsEscaped(0));
                input.SetRootPath(CTSVNPath(m_InitialUrl));
                input.SetLogText(sLogMsg);
                input.SetUUID(selection.GetRepository(0).uuid);
                input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEDEL);
                CString hint;
                if (selection.GetPathCount (0) == 1)
                    hint.Format(IDS_INPUT_REMOVEONE, (LPCTSTR)selection.GetURL (0, 0).GetFileOrDirectoryName());
                else
                    hint.Format(IDS_INPUT_REMOVEMORE, selection.GetPathCount (0));

                input.SetActionText(hint);
                if (input.DoModal() == IDOK)
                {
                    sLogMsg = input.GetLogMessage();
                    if (!RunPreCommit(selection.GetURLsEscaped(0), svn_depth_unknown, sLogMsg))
                        break;
                    bool bRet = Remove (selection.GetURLsEscaped (0), true, false, sLogMsg, input.m_revProps);
                    RunPostCommit(selection.GetURLsEscaped(0), svn_depth_unknown, m_commitRev, sLogMsg);
                    if (!bRet || !PostCommitErr.IsEmpty())
                    {
                        wait_cursor.Hide();
                        ShowErrorDialog(m_hWnd);
                        if (!bRet)
                            break;
                    }
                    m_barRepository.SetHeadRevision(GetCommitRevision());
                    if (hChosenTreeItem)
                    {
                        // do a full refresh: just refreshing the parent of the
                        // deleted tree node won't work if the list view
                        // shows part of that tree.
                        OnRefresh();
                    }
                    else
                        RefreshNode(m_RepoTree.GetSelectedItem(), true);
                }
            }
            break;
        case ID_BREAKLOCK:
            {
                if (!Unlock (selection.GetURLsEscaped (0), TRUE))
                {
                    ShowErrorDialog(m_hWnd);
                    break;
                }
                InvalidateData (m_RepoTree.GetSelectedItem());
                RefreshNode(m_RepoTree.GetSelectedItem(), true);
            }
            break;
        case ID_IMPORTFOLDER:
            {
                CString path;
                CBrowseFolder folderBrowser;
                folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
                if (folderBrowser.Show(GetSafeHwnd(), path)==CBrowseFolder::OK)
                {
                    CTSVNPath svnPath(path);
                    CWaitCursorEx wait_cursor;
                    CString filename = svnPath.GetFileOrDirectoryName();
                    CString sLogMsg;
                    if (!RunStartCommit(selection.GetURLsEscaped(0), sLogMsg))
                        break;
                    CInputLogDlg input(this);
                    input.SetPathList(selection.GetURLsEscaped(0));
                    input.SetRootPath(CTSVNPath(m_InitialUrl));
                    input.SetLogText(sLogMsg);
                    input.SetUUID(selection.GetRepository(0).uuid);
                    input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEIMPORT);

                    const CTSVNPath& url = selection.GetURL (0, 0);
                    CString sHint;
                    sHint.FormatMessage(IDS_INPUT_IMPORTFOLDER, (LPCTSTR)svnPath.GetSVNPathString(), (LPCTSTR)(url.GetSVNPathString()+L"/"+filename));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sLogMsg = input.GetLogMessage();
                        if (!RunPreCommit(selection.GetURLsEscaped(0), svn_depth_unknown, sLogMsg))
                            break;
                        InvalidateDataParents (selection);

                        CProgressDlg progDlg;
                        progDlg.SetTitle(IDS_APPNAME);
                        CString sInfoLine;
                        sInfoLine.FormatMessage(IDS_PROGRESSIMPORT, (LPCTSTR)filename);
                        progDlg.SetLine(1, sInfoLine, true);
                        SetAndClearProgressInfo(&progDlg);
                        progDlg.ShowModeless(m_hWnd);
                        bool bRet = Import(svnPath,
                                           CTSVNPath(EscapeUrl(CTSVNPath(url.GetSVNPathString()+L"/"+filename))),
                                           sLogMsg,
                                           &m_ProjectProperties,
                                           svn_depth_infinity,
                                           true, false, false, input.m_revProps);
                        RunPostCommit(selection.GetURLsEscaped(0), svn_depth_unknown, m_commitRev, sLogMsg);
                        if (!bRet || !PostCommitErr.IsEmpty())
                        {
                            progDlg.Stop();
                            SetAndClearProgressInfo((HWND)NULL);
                            wait_cursor.Hide();
                            ShowErrorDialog(m_hWnd);
                            if (!bRet)
                                break;
                        }
                        m_barRepository.SetHeadRevision(GetCommitRevision());
                        progDlg.Stop();
                        SetAndClearProgressInfo((HWND)NULL);
                        RefreshNode(m_RepoTree.GetSelectedItem(), true);
                    }
                }
            }
            break;
        case ID_IMPORT:
            {
                // Display the Open dialog box.
                CString openPath;
                if (CAppUtils::FileOpenSave(openPath, NULL, IDS_REPOBROWSE_IMPORT, IDS_COMMONFILEFILTER, true, m_path.IsUrl() ? CString() : m_path.GetDirectory().GetWinPathString(), m_hWnd))
                {
                    CTSVNPath path(openPath);
                    CWaitCursorEx wait_cursor;
                    CString filename = path.GetFileOrDirectoryName();
                    CString sLogMsg;
                    if (!RunStartCommit(selection.GetURLsEscaped(0), sLogMsg))
                        break;
                    CInputLogDlg input(this);
                    input.SetPathList(selection.GetURLsEscaped(0));
                    input.SetRootPath(CTSVNPath(m_InitialUrl));
                    input.SetLogText(sLogMsg);
                    input.SetUUID(selection.GetRepository(0).uuid);
                    input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEIMPORT);

                    const CTSVNPath& url = selection.GetURL (0, 0);
                    CString sHint;
                    sHint.FormatMessage(IDS_INPUT_IMPORTFILEFULL, path.GetWinPath(), (LPCTSTR)(url.GetSVNPathString()+L"/"+filename));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sLogMsg = input.GetLogMessage();
                        if (!RunPreCommit(selection.GetURLsEscaped(0), svn_depth_unknown, sLogMsg))
                            break;
                        InvalidateDataParents (selection);

                        CProgressDlg progDlg;
                        progDlg.SetTitle(IDS_APPNAME);
                        CString sInfoLine;
                        sInfoLine.FormatMessage(IDS_PROGRESSIMPORT, (LPCTSTR)filename);
                        progDlg.SetLine(1, sInfoLine, true);
                        SetAndClearProgressInfo(&progDlg);
                        progDlg.ShowModeless(m_hWnd);
                        bool bRet = Import(path,
                                           CTSVNPath(EscapeUrl(CTSVNPath(url.GetSVNPathString()+L"/"+filename))),
                                           sLogMsg,
                                           &m_ProjectProperties,
                                           svn_depth_empty,
                                           true, true, false, input.m_revProps);
                        RunPostCommit(selection.GetURLsEscaped(0), svn_depth_unknown, m_commitRev, sLogMsg);
                        if (!bRet || !PostCommitErr.IsEmpty())
                        {
                            progDlg.Stop();
                            SetAndClearProgressInfo((HWND)NULL);
                            wait_cursor.Hide();
                            ShowErrorDialog(m_hWnd);
                            if (!bRet)
                                break;
                        }
                        m_barRepository.SetHeadRevision(GetCommitRevision());
                        progDlg.Stop();
                        SetAndClearProgressInfo((HWND)NULL);
                        RefreshNode(m_RepoTree.GetSelectedItem(), true);
                    }
                }
            }
            break;
        case ID_RENAME:
            {
                if (pWnd == &m_RepoList)
                {
                    CAutoReadLock locker(m_guard);
                    POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
                    if (pos == NULL)
                        break;
                    int selIndex = m_RepoList.GetNextSelectedItem(pos);
                    if (selIndex >= 0)
                    {
                        m_RepoList.SetFocus();
                        m_RepoList.EditLabel(selIndex);
                    }
                    else
                    {
                        m_RepoTree.SetFocus();
                        HTREEITEM hTreeItem = m_RepoTree.GetSelectedItem();
                        if (hTreeItem != m_RepoTree.GetRootItem())
                            m_RepoTree.EditLabel(hTreeItem);
                    }
                }
                else if (pWnd == &m_RepoTree)
                {
                    CAutoReadLock locker(m_guard);
                    m_RepoTree.SetFocus();
                    if (hChosenTreeItem != m_RepoTree.GetRootItem())
                        m_RepoTree.EditLabel(hChosenTreeItem);
                }
            }
            break;
        case ID_COPYTO:
            {
                const CTSVNPath& path = selection.GetURL (0, 0);
                const SVNRev& revision = selection.GetRepository(0).revision;

                CRenameDlg dlg(this);
                dlg.m_name = path.GetSVNPathString();
                dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_COPY);
                dlg.SetRenameRequired(GetRevision().IsHead() != FALSE);
                CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
                if (dlg.DoModal() == IDOK)
                {
                    CWaitCursorEx wait_cursor;
                    if(!CheckAndConfirmPath(CTSVNPath(dlg.m_name)))
                        break;
                    CString sLogMsg;
                    if (!RunStartCommit(selection.GetURLsEscaped(0), sLogMsg))
                        break;
                    CInputLogDlg input(this);
                    input.SetPathList(selection.GetURLsEscaped(0));
                    input.SetRootPath(CTSVNPath(m_InitialUrl));
                    input.SetLogText(sLogMsg);
                    input.SetUUID(selection.GetRepository(0).uuid);
                    input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEBRANCH);
                    CString sHint;
                    CString sCopyUrl = path.GetSVNPathString() + L"@" + revision.ToString();
                    sHint.FormatMessage(IDS_INPUT_COPY, (LPCTSTR)sCopyUrl, (LPCTSTR)dlg.m_name);
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sLogMsg = input.GetLogMessage();
                        if (!RunPreCommit(selection.GetURLsEscaped(0), svn_depth_unknown, sLogMsg))
                            break;
                        InvalidateDataParents (selection);

                        bool bRet = Copy(selection.GetURLsEscaped(0), CTSVNPath(dlg.m_name), revision, revision, sLogMsg, true, true, false, false, SVNExternals(), input.m_revProps);
                        RunPostCommit(selection.GetURLsEscaped(0), svn_depth_unknown, m_commitRev, sLogMsg);
                        if (!bRet || !PostCommitErr.IsEmpty())
                        {
                            wait_cursor.Hide();
                            ShowErrorDialog(m_hWnd);
                            if (!bRet)
                                break;
                        }
                        if (revision.IsHead())
                        {
                            m_barRepository.SetHeadRevision(GetCommitRevision());
                            RefreshNode(m_RepoTree.GetSelectedItem(), true);
                        }
                    }
                }
            }
            break;
        case ID_COPYTOWC:
            {
                const SVNRev& revision = selection.GetRepository(0).revision;

                CTSVNPath tempfile;
                bool bSavePathOK = AskForSavePath (selection, tempfile, false);
                if (bSavePathOK)
                {
                    CWaitCursorEx wait_cursor;

                    CProgressDlg progDlg;
                    progDlg.SetTitle(IDS_APPNAME);
                    SetAndClearProgressInfo(&progDlg);
                    progDlg.ShowModeless(m_hWnd);

                    bool bCopyAsChild = (selection.GetPathCount (0) > 1);
                    if (!Copy (selection.GetURLsEscaped(0), tempfile, revision, revision, CString(), bCopyAsChild)||(progDlg.HasUserCancelled()))
                    {
                        progDlg.Stop();
                        SetAndClearProgressInfo((HWND)NULL);
                        wait_cursor.Hide();
                        progDlg.Stop();
                        if (!progDlg.HasUserCancelled())
                            ShowErrorDialog(m_hWnd);
                        break;
                    }
                    m_barRepository.SetHeadRevision(GetCommitRevision());
                    progDlg.Stop();
                    SetAndClearProgressInfo((HWND)NULL);
                }
            }
            break;
        case ID_MKDIR:
            {
                const CTSVNPath& path = selection.GetURL (0, 0);
                CRenameDlg dlg(this);
                dlg.m_name = L"";
                dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_MKDIR);
                CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
                if (dlg.DoModal() == IDOK)
                {
                    CWaitCursorEx wait_cursor;
                    CTSVNPathList plist = CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(path.GetSVNPathString()+L"/"+dlg.m_name.Trim()))));
                    CString sLogMsg;
                    if (!RunStartCommit(plist, sLogMsg))
                        break;
                    CInputLogDlg input(this);
                    input.SetPathList(plist);
                    input.SetRootPath(CTSVNPath(m_InitialUrl));
                    input.SetLogText(sLogMsg);
                    input.SetUUID(selection.GetRepository(0).uuid);
                    input.SetProjectProperties(&m_ProjectProperties, PROJECTPROPNAME_LOGTEMPLATEMKDIR);
                    CString sHint;
                    sHint.Format(IDS_INPUT_MKDIR, (LPCTSTR)(path.GetSVNPathString()+L"/"+dlg.m_name.Trim()));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sLogMsg = input.GetLogMessage();
                        if (!RunPreCommit(plist, svn_depth_unknown, sLogMsg))
                            break;
                        InvalidateDataParents (selection);

                        // when creating the new folder, also trim any whitespace chars from it
                        bool bRet = MakeDir(plist, sLogMsg, true, input.m_revProps);
                        RunPostCommit(plist, svn_depth_unknown, m_commitRev, sLogMsg);
                        if (!bRet || !PostCommitErr.IsEmpty())
                        {
                            wait_cursor.Hide();
                            ShowErrorDialog(m_hWnd);
                            if (!bRet)
                                break;
                        }
                        m_barRepository.SetHeadRevision(GetCommitRevision());
                        RefreshNode(m_RepoTree.GetSelectedItem(), true);
                    }
                }
            }
            break;
        case ID_REFRESH:
            {
                OnRefresh();
            }
            break;
        case ID_GNUDIFF:
            {
                const CTSVNPath& path = selection.GetURLEscaped (0, 0);
                const SVNRev& revision = selection.GetRepository (0).revision;
                CString options;
                if (GetAsyncKeyState(VK_SHIFT)&0x8000)
                {
                    CDiffOptionsDlg dlg;
                    if (dlg.DoModal() == IDOK)
                        options = dlg.GetDiffOptionsString();
                    else
                        break;
                }
                SVNDiff diff(this, this->m_hWnd, true);
                if (selection.GetPathCount(0) == 1)
                {
                    if (PromptShown())
                        diff.ShowUnifiedDiff (path, revision,
                                            CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), options, false, false, false);
                    else
                        CAppUtils::StartShowUnifiedDiff(m_hWnd, path, revision,
                                            CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), SVNRev(), options, false, false, false, false);
                }
                else
                {
                    const CTSVNPath& path2 = selection.GetURLEscaped (0, 1);
                    if (PromptShown())
                        diff.ShowUnifiedDiff(path, revision,
                                            path2, revision, SVNRev(), options, false, false, false);
                    else
                        CAppUtils::StartShowUnifiedDiff(m_hWnd, path, revision,
                                            path2, revision, SVNRev(), SVNRev(), options, false, false, false, false);
                }
            }
            break;
        case ID_DIFF_CONTENTONLY:
            bIgnoreProps = true;
        case ID_DIFF:
            {
                const CTSVNPath& path = selection.GetURLEscaped (0, 0);
                const SVNRev& revision = selection.GetRepository (0).revision;
                size_t nFolders = selection.GetFolderCount (0);

                SVNDiff diff(this, this->m_hWnd, true);
                diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
                if (selection.GetPathCount (0) == 1)
                {
                    if (PromptShown())
                        diff.ShowCompare(path, revision,
                        CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), bIgnoreProps, L"", true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
                    else
                        CAppUtils::StartShowCompare(m_hWnd, path, revision,
                                        CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), SVNRev(), bIgnoreProps, L"",
                                        !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
                }
                else
                {
                    const CTSVNPath& path2 = selection.GetURLEscaped (0, 1);
                    if (PromptShown())
                        diff.ShowCompare(path, revision,
                                        path2, revision, SVNRev(), bIgnoreProps, L"", true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
                    else
                        CAppUtils::StartShowCompare(m_hWnd, path, revision,
                                        path2, revision, SVNRev(), SVNRev(), bIgnoreProps, L"",
                                        !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
                }
            }
            break;
        case ID_PROPS:
            {
                const SVNRev& revision = selection.GetRepository(0).revision;
                if (revision.IsHead())
                {
                    CEditPropertiesDlg dlg(this);
                    dlg.SetProjectProperties(&m_ProjectProperties);
                    dlg.SetUUID(selection.GetRepository(0).uuid);
                    dlg.SetPathList(selection.GetURLsEscaped(0));
                    dlg.SetRevision(GetHEADRevision(selection.GetURL (0, 0)));
                    dlg.UrlIsFolder(selection.GetFolderCount(0)!=0);
                    dlg.DoModal();
                }
                else
                {
                    CPropDlg dlg(this);
                    dlg.m_rev = revision;
                    dlg.m_Path = selection.GetURLEscaped (0,0);
                    dlg.DoModal();
                }
            }
            break;
        case ID_REVPROPS:
            {
                CEditPropertiesDlg dlg(this);
                dlg.SetProjectProperties(&m_ProjectProperties);
                dlg.SetUUID(selection.GetRepository(0).uuid);
                dlg.SetPathList(selection.GetURLsEscaped(0));
                dlg.SetRevision(selection.GetRepository(0).revision);
                dlg.RevProps(true);
                dlg.DoModal();
            }
            break;
        case ID_BLAME:
            {
                CBlameDlg dlg(this);
                dlg.EndRev = selection.GetRepository(0).revision;
                dlg.PegRev = m_repository.revision;
                if (dlg.DoModal() == IDOK)
                {
                    CBlame blame;
                    CString tempfile;
                    const CTSVNPath& path = selection.GetURLEscaped (0,0);

                    tempfile = blame.BlameToTempFile ( path
                                                     , dlg.StartRev
                                                     , dlg.EndRev
                                                     , dlg.PegRev
                                                     , SVN::GetOptionsString (!!dlg.m_bIgnoreEOL, dlg.m_IgnoreSpaces)
                                                     , dlg.m_bIncludeMerge
                                                     , TRUE
                                                     , TRUE);
                    if (!tempfile.IsEmpty())
                    {
                        if (dlg.m_bTextView)
                        {
                            //open the default text editor for the result file
                            CAppUtils::StartTextViewer(tempfile);
                        }
                        else
                        {
                            CString sParams = L"/path:\"" + path.GetSVNPathString() + L"\" ";
                            if(!CAppUtils::LaunchTortoiseBlame ( tempfile
                                                               , CPathUtils::GetFileNameFromPath(selection.GetURL (0,0).GetFileOrDirectoryName())
                                                               , sParams
                                                               , dlg.StartRev
                                                               , dlg.EndRev
                                                               , dlg.PegRev))
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        blame.ShowErrorDialog(m_hWnd);
                    }
                }

            }
            break;
        case ID_CREATELINK:
            {
                CTSVNPath tempFile;
                if (AskForSavePath(selection, tempFile, false))
                {
                    if (tempFile.GetFileExtension().Compare(L".url"))
                    {
                        tempFile.AppendRawString(L".url");
                    }
                    CString urlCmd = selection.GetURLEscaped(0, 0).GetSVNPathString();
                    SVNRev r = selection.GetRepository(0).revision;
                    if (r.IsNumber())
                        urlCmd += L"?r=" + r.ToString();
                    CAppUtils::CreateShortcutToURL((LPCTSTR)urlCmd, tempFile.GetWinPath());
                }
            }
            break;
        case ID_ADDTOBOOKMARKS:
            {
                m_bookmarks.insert((LPCWSTR)selection.GetURL(0, 0).GetSVNPathString());
                RefreshBookmarks();
            }
            break;
        case ID_REMOVEBOOKMARKS:
            {
                m_bookmarks.erase((LPCWSTR)selection.GetURL(0, 0).GetSVNPathString());
                RefreshBookmarks();
            }
            break;
        default:
            break;
        }
        DialogEnableWindow(IDOK, TRUE);
    }
}

bool CRepositoryBrowser::AskForSavePath
    ( const CRepositoryBrowserSelection& selection
    , CTSVNPath &tempfile
    , bool bFolder)
{
    bool bSavePathOK = false;
    if ((!bFolder)&&(selection.GetPathCount(0) == 1))
    {
        CString savePath = selection.GetURL (0, 0).GetFilename();
        bSavePathOK = CAppUtils::FileOpenSave(savePath, NULL, IDS_REPOBROWSE_SAVEAS, IDS_COMMONFILEFILTER, false, m_path.IsUrl() ? CString() : m_path.GetDirectory().GetWinPathString(), m_hWnd);
        if (bSavePathOK)
            tempfile.SetFromWin(savePath);
    }
    else
    {
        CBrowseFolder browser;
        CString sTempfile;
        browser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
        browser.Show(GetSafeHwnd(), sTempfile);
        if (!sTempfile.IsEmpty())
        {
            bSavePathOK = true;
            tempfile.SetFromWin(sTempfile);
        }
    }
    return bSavePathOK;
}

bool CRepositoryBrowser::StringToWidthArray(const CString& WidthString, int WidthArray[])
{
    TCHAR * endchar;
    for (int i=0; i<7; ++i)
    {
        CString hex;
        if (WidthString.GetLength() >= i*8+8)
            hex = WidthString.Mid(i*8, 8);
        if ( hex.IsEmpty() )
        {
            // This case only occurs when upgrading from an older
            // TSVN version in which there were fewer columns.
            WidthArray[i] = 0;
        }
        else
        {
            WidthArray[i] = wcstol(hex, &endchar, 16);
        }
    }
    return true;
}

CString CRepositoryBrowser::WidthArrayToString(int WidthArray[])
{
    CString sResult;
    TCHAR buf[10] = { 0 };
    for (int i=0; i<7; ++i)
    {
        swprintf_s(buf, L"%08X", WidthArray[i]);
        sResult += buf;
    }
    return sResult;
}

void CRepositoryBrowser::SaveColumnWidths(bool bSaveToRegistry /* = false */)
{
    CRegString regColWidth(L"Software\\TortoiseSVN\\RepoBrowserColumnWidth");
    int maxcol = ((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1;
    // first clear the width array
    std::fill_n(m_arColumnWidths, _countof(m_arColumnWidths), 0);
    for (int col = 0; col <= maxcol; ++col)
    {
        m_arColumnWidths[col] = m_RepoList.GetColumnWidth(col);
        if (m_arColumnWidths[col] == m_arColumnAutoWidths[col])
            m_arColumnWidths[col] = 0;
    }
    if (bSaveToRegistry)
    {
        CString sWidths = WidthArrayToString(m_arColumnWidths);
        regColWidth = sWidths;
    }
}

void CRepositoryBrowser::InvalidateData (HTREEITEM node)
{
    if (node == NULL)
        return;
    SVNRev r;
    {
        CAutoReadLock locker(m_guard);
        CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData (node);
        if (pItem->bookmark || pItem->dummy)
            return;
        r = pItem->repository.revision;
    }
    InvalidateData (node, r);
}

void CRepositoryBrowser::InvalidateData (HTREEITEM node, const SVNRev& revision)
{
    CString url;
    {
        CAutoReadLock locker(m_guard);
        CTreeItem * pItem = NULL;
        if (node != NULL)
        {
            pItem = (CTreeItem *)m_RepoTree.GetItemData (node);
            if (pItem->bookmark || pItem->dummy)
                return;
            url = pItem->url;
        }
    }
    if (url.IsEmpty())
        m_lister.Refresh (revision);
    else
        m_lister.RefreshSubTree (revision, url);
}

void CRepositoryBrowser::InvalidateDataParents
    (const CRepositoryBrowserSelection& selection)
{
    for (size_t i = 0; i < selection.GetRepositoryCount(); ++i)
    {
        const SVNRev& revision = selection.GetRepository (i).revision;
        for (size_t k = 0, count = selection.GetPathCount (i); k < count; ++k)
        {
            const CString& url = selection.GetURL (i, k).GetSVNPathString();
            InvalidateData (m_RepoTree.GetParentItem (FindUrl (url)), revision);
        }
    }
}

void CRepositoryBrowser::BeginDrag(const CWnd& window,
    CRepositoryBrowserSelection& selection, POINT& point, bool setAsyncMode)
{
    if (m_bSparseCheckoutMode)
        return;

    // must be exactly one repository
    if (selection.GetRepositoryCount() != 1)
    {
        return;
    }

    // build copy source / content
    std::unique_ptr<CIDropSource> pdsrc(new CIDropSource);
    if (pdsrc == NULL)
        return;

    pdsrc->AddRef();

    const SVNRev& revision = selection.GetRepository(0).revision;
    SVNDataObject* pdobj = new SVNDataObject(selection.GetURLsEscaped(0), revision, revision);
    if (pdobj == NULL)
    {
        return;
    }
    pdobj->AddRef();
    if(setAsyncMode)
        pdobj->SetAsyncMode(TRUE);
    CDragSourceHelper dragsrchelper;
    dragsrchelper.InitializeFromWindow(window.GetSafeHwnd(), point, pdobj);
    pdsrc->m_pIDataObj = pdobj;
    pdsrc->m_pIDataObj->AddRef();

    // Initiate the Drag & Drop
    DWORD dwEffect;
    ::DoDragDrop(pdobj, pdsrc.get(), DROPEFFECT_MOVE|DROPEFFECT_COPY, &dwEffect);
    pdsrc->Release();
    pdsrc.release();
    pdobj->Release();
}

void CRepositoryBrowser::StoreSelectedURLs()
{
    CAutoReadLock locker(m_guard);
    CRepositoryBrowserSelection selection;

    // selections on the RHS list take precedence

    POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
    if (pos != NULL)
    {
        int index = -1;
        while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
            selection.Add ((CItem *)m_RepoList.GetItemData (index));
    }

    // followed by the LHS tree view

    if (selection.IsEmpty())
    {
        HTREEITEM hSelectedTreeItem = m_RepoTree.GetSelectedItem();
        if (hSelectedTreeItem)
            selection.Add ((CTreeItem *)m_RepoTree.GetItemData (hSelectedTreeItem));
    }

    // in line with the URL selected in the edit box?
    // path edit box has highest prio

    CString editPath = GetPath();

    m_selectedURLs = selection.Contains (CTSVNPath (editPath))
        ? selection.GetURLs (0).CreateAsteriskSeparatedString()
        : editPath;
}

const CString& CRepositoryBrowser::GetSelectedURLs() const
{
    return m_selectedURLs;
}

void CRepositoryBrowser::OnTvnItemChangedRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMTVITEMCHANGE *pNMTVItemChange = reinterpret_cast<NMTVITEMCHANGE*>(pNMHDR);
    *pResult = 0;

    if (!m_bSparseCheckoutMode || m_blockEvents)
        return;

    if (pNMTVItemChange->uStateOld == 0 && pNMTVItemChange->uStateNew == 0)
        return; // No change

    BOOL bPrevState = (BOOL)(((pNMTVItemChange->uStateOld & TVIS_STATEIMAGEMASK)>>12)-1);   // Old check box state
    if (bPrevState < 0) // On startup there's no previous state
        bPrevState = 0; // so assign as false (unchecked)

    // New check box state
    BOOL bChecked=(BOOL)(((pNMTVItemChange->uStateNew & TVIS_STATEIMAGEMASK)>>12)-1);
    if (bChecked < 0) // On non-checkbox notifications assume false
        bChecked = 0;

    if (bPrevState == bChecked) // No change in check box
        return;

    bChecked = m_RepoTree.GetCheck(pNMTVItemChange->hItem);

    m_blockEvents++;

    CheckTreeItem(pNMTVItemChange->hItem, !!bChecked);

    if (pNMTVItemChange->uStateNew & TVIS_SELECTED)
    {
        HTREEITEM hItem = m_RepoTree.GetSelectedItem();
        HTREEITEM hRoot = m_RepoTree.GetRootItem();
        while (hItem)
        {
            if ((hItem != pNMTVItemChange->hItem)&&(hItem != hRoot))
            {
                m_RepoTree.SetCheck(hItem, !!bChecked);
                CheckTreeItem(hItem, !!bChecked);
            }
            hItem = m_RepoTree.GetNextItem(hItem, TVGN_NEXTSELECTED);
        }
    }
    m_blockEvents--;
}

bool CRepositoryBrowser::CheckoutDepthForItem( HTREEITEM hItem )
{
    CAutoReadLock locker(m_guard);
    bool bChecked = !!m_RepoTree.GetCheck(hItem);
    CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData (hItem);
    if (!bChecked)
    {
        m_updateDepths[pItem->url] = svn_depth_exclude;
        HTREEITEM hParent = m_RepoTree.GetParentItem(hItem);
        while (hParent)
        {
            CTreeItem * pParent = (CTreeItem *)m_RepoTree.GetItemData (hParent);
            std::map<CString,svn_depth_t>::iterator it = m_checkoutDepths.find(pParent->url);
            if (it != m_checkoutDepths.end())
            {
                if ((it->second == svn_depth_infinity)&&(pItem->kind == svn_node_dir))
                    m_checkoutDepths[pParent->url] = svn_depth_files;
                else
                    m_checkoutDepths[pParent->url] = svn_depth_empty;
            }

            it = m_updateDepths.find(pParent->url);
            if (it != m_updateDepths.end())
            {
                m_updateDepths[pParent->url] = svn_depth_unknown;
            }
            hParent = m_RepoTree.GetParentItem(hParent);
        }
    }
    if (bChecked)
    {
        m_checkoutDepths[pItem->url] = svn_depth_infinity;
        m_updateDepths[pItem->url] = svn_depth_infinity;
    }

    HTREEITEM hChild = m_RepoTree.GetChildItem(hItem);
    while (hChild)
    {
        CheckoutDepthForItem(hChild);
        hChild = m_RepoTree.GetNextItem(hChild, TVGN_NEXT);
    }

    return bChecked;
}

void CRepositoryBrowser::CheckParentsOfTreeItem( HTREEITEM hItem )
{
    HTREEITEM hParent = m_RepoTree.GetParentItem(hItem);
    while (hParent)
    {
        m_RepoTree.SetCheck(hParent, TRUE);
        hParent = m_RepoTree.GetParentItem(hParent);
    }
}

void CRepositoryBrowser::CheckTreeItemRecursive( HTREEITEM hItem, bool bCheck )
{
    // process item
    m_RepoTree.SetCheck(hItem, bCheck);

    // process all children
    HTREEITEM hChild = m_RepoTree.GetChildItem(hItem);
    while (hChild)
    {
        CheckTreeItemRecursive(hChild, bCheck);
        hChild = m_RepoTree.GetNextItem(hChild, TVGN_NEXT);
    }
}

bool CRepositoryBrowser::HaveAllChildrenSameCheckState( HTREEITEM hItem, bool bChecked /* = false */ )
{
    // analyze all children
    HTREEITEM hChild = m_RepoTree.GetChildItem(hItem);
    while (hChild)
    {
        // child item doesn't have expected state. interrupt walk and return false
        if(!!m_RepoTree.GetCheck(hChild) != bChecked)
            return false;

        // check condition on all descendants.
        if(!HaveAllChildrenSameCheckState(hChild))
            return false;

        hChild = m_RepoTree.GetNextItem(hChild, TVGN_NEXT);
    }

    return true;
}

// Check item has next logic for emulate tri-state checkbox:
// if user click collapsed item or selected more then one item or item without children - proposed check
// state propogated to whole subtree. (depth: 'Fully recursive' or 'Exclude')
// otherwise:
// if user click unchecked item - only this item will be checked. (Depth: 'Only this item')
// if user click checked item and all children unchecked then item and all children
// will be checked. (subtree depth: 'Fully recursive')
//
// summary: item will be cyclicaly switched:
// unchecked -> checked only item -> checked whole subtree -> unchecked whole subtree
// for collapsed items (and when more than one item selected) will be cyclicaly switched:
// unchecked -> checked whole subtree -> unchecked whole subtree
void CRepositoryBrowser::CheckTreeItem( HTREEITEM hItem, bool bCheck )
{
    bool itemExpanded = !!(m_RepoTree.GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED);
    bool multiselectMode = m_RepoTree.GetSelectedCount() > 1;

    // tri-state logic will be emulated for expanded, checked items, in single selection mode,
    // with at least one child, when all children unchecked
    if(!bCheck && !multiselectMode && itemExpanded && m_RepoTree.GetChildItem(hItem) != NULL && HaveAllChildrenSameCheckState(hItem))
    {
        // instead of reseting item itself - set all children to checked state
        CheckTreeItemRecursive(hItem, TRUE);
        return;
    }

    if(bCheck)
    {
        if(!itemExpanded || multiselectMode)
        {
            // perform check recursively for special cases
            CheckTreeItemRecursive(hItem, bCheck);
        }
        else
        {
            // 'Only this item' depth
            m_RepoTree.SetCheck(hItem, bCheck);
        }

        // check all parents
        CheckParentsOfTreeItem(hItem);
    }
    else
    {
        // uncheck item and all children
        CheckTreeItemRecursive(hItem, bCheck);

        // force the root item to be checked
        if (hItem == m_RepoTree.GetRootItem())
        {
            m_RepoTree.SetCheck(hItem);
        }
    }
}

bool CRepositoryBrowser::CheckAndConfirmPath(const CTSVNPath& targetUrl)
{
    if (targetUrl.IsValidOnWindows())
        return true;

    CString sInfo;
    sInfo.Format(IDS_WARN_NOVALIDPATH_TASK1, (LPCTSTR)targetUrl.GetUIPathString());
    CTaskDialog taskdlg(sInfo,
                        CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK2)),
                        L"TortoiseSVN",
                        0,
                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK3)));
    taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK4)));
    taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskdlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK5)));
    taskdlg.SetDefaultCommandControl(2);
    taskdlg.SetMainIcon(TD_WARNING_ICON);
    return (taskdlg.DoModal(GetExplorerHWND()) == 1);
}

void CRepositoryBrowser::SaveDividerPosition()
{
    RECT rc;
    // use GetWindowRect instead of GetClientRect to account for possible scrollbars.
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&rc);
    CRegDWORD xPos(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\RepobrowserDivider");
    xPos = (rc.right - rc.left) - (2 * GetSystemMetrics(SM_CXBORDER));
}

int CRepositoryBrowser::SortStrCmp( PCWSTR str1, PCWSTR str2 )
{
    if (s_bSortLogical)
        return StrCmpLogicalW(str1, str2);
    return StrCmpI(str1, str2);
}

void CRepositoryBrowser::ShowText( const CString& sText, bool forceupdate /*= false*/ )
{
    if (m_bSparseCheckoutMode)
        m_RepoTree.ShowText(sText, forceupdate);
    else
        m_RepoList.ShowText(sText, forceupdate);
}

void CRepositoryBrowser::FilterInfinityDepthItems(std::map<CString,svn_depth_t>& depths)
{
    if (depths.empty())
        return;

    // now go through the whole list and remove all children of items that have infinity depth
    for (std::map<CString,svn_depth_t>::iterator it = depths.begin(); it != depths.end(); ++it)
    {
        if (it->second != svn_depth_infinity)
            continue;

        for (std::map<CString,svn_depth_t>::iterator it2 = depths.begin(); it2 != depths.end(); ++it2)
        {
            if (it->first.Compare(it2->first)==0)
                continue;

            CString url1 = it->first + L"/";
            if (url1.Compare(it2->first.Left(url1.GetLength()))==0)
            {
                std::map<CString,svn_depth_t>::iterator kill = it2;
                --it2;
                depths.erase(kill);
            }
        }
    }
}

void CRepositoryBrowser::SetListItemInfo( int index, const CItem * it )
{
    if (it->is_external)
        m_RepoList.SetItemState(index, INDEXTOOVERLAYMASK(OVERLAY_EXTERNAL), LVIS_OVERLAYMASK);

    // extension
    CString temp = CPathUtils::GetFileExtFromPath(it->path);
    if (it->kind == svn_node_file)
        m_RepoList.SetItemText(index, 1, temp);

    // pointer to the CItem structure
    m_RepoList.SetItemData(index, (DWORD_PTR)&(*it));

    if (it->unversioned)
        return;

    // revision
    if ((it->created_rev == SVN_IGNORED_REVNUM) && (it->is_external))
        temp = it->time != 0 ? L"" : L"HEAD";
    else
        temp.Format(L"%ld", it->created_rev);
    m_RepoList.SetItemText(index, 2, temp);

    // author
    m_RepoList.SetItemText(index, 3, it->author);

    // size
    if (it->kind == svn_node_file)
    {
        StrFormatByteSize(it->size, temp.GetBuffer(20), 20);
        temp.ReleaseBuffer();
        m_RepoList.SetItemText(index, 4, temp);
    }

    // date
    TCHAR date_native[SVN_DATE_BUFFER] = { 0 };
    if ((it->time == 0) && (it->is_external))
        date_native[0] = 0;
    else
        SVN::formatDate(date_native, (apr_time_t&)it->time, true);
    m_RepoList.SetItemText(index, 5, date_native);

    if (m_bShowLocks)
    {
        // lock owner
        m_RepoList.SetItemText(index, 6, it->lockowner);
        // lock comment
        m_RepoList.SetItemText(index, 7, m_ProjectProperties.MakeShortMessage(it->lockcomment));
    }
}


void CRepositoryBrowser::OnNMCustomdrawRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_RepoList.HasText())
        return;

    // Draw incomplete and unversioned items in gray.
    if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        // Tell Windows to paint the control itself.
        *pResult = CDRF_DODEFAULT;

        if (m_RepoList.GetItemCount() > (int)pLVCD->nmcd.dwItemSpec)
        {
            COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
            CAutoReadLock locker(m_guard);
            CItem * pItem = (CItem*)m_RepoList.GetItemData((int)pLVCD->nmcd.dwItemSpec);
            if (pItem && pItem->unversioned)
            {
                crText = GetSysColor(COLOR_GRAYTEXT);
            }
            // Store the color back in the NMLVCUSTOMDRAW struct.
            pLVCD->clrText = crText;
        }
    }
}

void CRepositoryBrowser::OnNMCustomdrawRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMTVCUSTOMDRAW* pTVCD = reinterpret_cast<NMTVCUSTOMDRAW*>(pNMHDR);
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;
    // Draw incomplete and unversioned items in gray.
    if (CDDS_PREPAINT == pTVCD->nmcd.dwDrawStage)
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if (CDDS_ITEMPREPAINT == pTVCD->nmcd.dwDrawStage)
    {
        // Tell Windows to paint the control itself.
        *pResult = CDRF_DODEFAULT;

        CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData((HTREEITEM)pTVCD->nmcd.dwItemSpec);
        if (pItem && pItem->unversioned)
        {
            // Store the color back in the NMLVCUSTOMDRAW struct.
            pTVCD->clrText = GetSysColor(COLOR_GRAYTEXT);
        }
        if (pItem && pItem->dummy)
        {
            // don't draw empty items, they're used for spacing
            *pResult = CDRF_DOERASE | CDRF_SKIPDEFAULT;
        }
    }
}

void CRepositoryBrowser::OnTvnItemChangingRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMTVITEMCHANGE *pNMTVItemChange = reinterpret_cast<NMTVITEMCHANGE *>(pNMHDR);
    *pResult = 0;
    CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData((HTREEITEM)pNMTVItemChange->hItem);
    if (pItem && pItem->dummy)
        *pResult = 1;
}

void CRepositoryBrowser::OnNMSetCursorRepotree(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    *pResult = 0;
    POINT pt;
    GetCursorPos(&pt);
    m_RepoTree.ScreenToClient(&pt);
    UINT flags = TVHT_ONITEM | TVHT_ONITEMINDENT | TVHT_ONITEMRIGHT | TVHT_ONITEMBUTTON | TVHT_TOLEFT | TVHT_TORIGHT;
    HTREEITEM hItem = m_RepoTree.HitTest(pt, &flags);
    if (hItem)
    {
        CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData(hItem);
        if (pItem && pItem->dummy)
        {
            SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
            *pResult = 1;
        }
    }
}

bool CRepositoryBrowser::TrySVNParentPath()
{
    if (m_bSparseCheckoutMode)
        return false;
    if (!m_bTrySVNParentPath)
        return false;

    CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true);
    // custom callback object required to bypass invalid SSL certificates
    // and handle possible authentication dialogs
    std::unique_ptr<CCallback> callback(new CCallback());
    callback->SetAuthParentWindow(GetSafeHwnd());

    HRESULT hResUDL = URLDownloadToFile(NULL, m_InitialUrl+L"/", tempfile.GetWinPath(), 0, callback.get());
    if (hResUDL != S_OK)
    {
        hResUDL = URLDownloadToFile(NULL, m_InitialUrl, tempfile.GetWinPath(), 0, callback.get());
    }
    if (hResUDL == S_OK)
    {
        // set up the SVNParentPath url as the repo root, even though it isn't a real repo root
        m_repository.root = m_InitialUrl;
        m_repository.revision = SVNRev::REV_HEAD;
        m_repository.peg_revision = SVNRev::REV_HEAD;

        // insert our pseudo repo root into the tree view.
        CTreeItem * pTreeItem = new CTreeItem();
        pTreeItem->unescapedname = m_InitialUrl;
        pTreeItem->url = m_InitialUrl;
        pTreeItem->logicalPath = m_InitialUrl;
        pTreeItem->repository = m_repository;
        pTreeItem->kind = svn_node_dir;
        pTreeItem->svnparentpathroot = true;
        TVINSERTSTRUCT tvinsert = {0};
        tvinsert.hParent = TVI_ROOT;
        tvinsert.hInsertAfter = TVI_ROOT;
        tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
        tvinsert.itemex.pszText = m_InitialUrl.GetBuffer (m_InitialUrl.GetLength());
        tvinsert.itemex.cChildren = 1;
        tvinsert.itemex.lParam = (LPARAM)pTreeItem;
        tvinsert.itemex.iImage = m_nSVNParentPath;
        tvinsert.itemex.iSelectedImage = m_nSVNParentPath;

        HTREEITEM hRoot = m_RepoTree.InsertItem(&tvinsert);
        m_InitialUrl.ReleaseBuffer();

        // we got a web page! But we can't be sure that it's the page from SVNParentPath.
        // Use a regex to parse the website and find out...
        std::ifstream fs(tempfile.GetWinPath());
        string in;
        if (!fs.bad())
        {
            in.reserve((unsigned int)fs.rdbuf()->in_avail());
            char c;
            while (fs.get(c))
            {
                if (in.capacity() == in.size())
                    in.reserve(in.capacity() * 3);
                in.append(1, c);
            }
            fs.close();

            // make sure this is a html page from an SVNParentPathList
            // we do this by checking for header titles looking like
            // "<h2>Revision XX: /</h2> - if we find that, it's a html
            // page from inside a repository
            const char * reTitle = "<\\s*h2\\s*>[^/]+/\\s*<\\s*/\\s*h2\\s*>";
            // xsl transformed pages don't have an easy way to determine
            // the inside from outside of a repository.
            // We therefore check for <index rev="0" to make sure it's either
            // an empty repository or really an SVNParentPathList
            const char * reTitle2 = "<\\s*index\\s*rev\\s*=\\s*\"0\"";
            const tr1::regex titex(reTitle, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
            const tr1::regex titex2(reTitle2, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
            if (tr1::regex_search(in.begin(), in.end(), titex, tr1::regex_constants::match_default))
            {
                TRACE(L"found repository url instead of SVNParentPathList\n");
                return false;
            }

            const char * re = "<\\s*LI\\s*>\\s*<\\s*A\\s+[^>]*HREF\\s*=\\s*\"([^\"]*)\"\\s*>([^<]+)<\\s*/\\s*A\\s*>\\s*<\\s*/\\s*LI\\s*>";
            const char * re2 = "<\\s*DIR\\s*name\\s*=\\s*\"([^\"]*)\"\\s*HREF\\s*=\\s*\"([^\"]*)\"\\s*/\\s*>";

            const tr1::regex expression(re, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
            const tr1::regex expression2(re2, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
            int nCountNewEntries = 0;
            const tr1::sregex_iterator end;
            for (tr1::sregex_iterator i(in.begin(), in.end(), expression); i != end; ++i)
            {
                const tr1::smatch match = *i;
                // what[0] contains the whole string
                // what[1] contains the url part.
                // what[2] contains the name
                CString sMatch = CUnicodeUtils::GetUnicode(std::string(match[1]).c_str());
                sMatch.TrimRight('/');
                CString url = m_InitialUrl + L"/" + sMatch;
                CItem item;
                item.absolutepath = url;
                item.kind = svn_node_dir;
                item.path = sMatch;
                SRepositoryInfo repoinfo;
                repoinfo.root = url;
                repoinfo.revision = SVNRev::REV_HEAD;
                repoinfo.peg_revision = SVNRev::REV_HEAD;
                item.repository = repoinfo;
                AutoInsert(hRoot, item);
                ++nCountNewEntries;
            }
            if (!regex_search(in.begin(), in.end(), titex2))
            {
                TRACE(L"found repository url instead of SVNParentPathList\n");
                return false;
            }
            for (tr1::sregex_iterator i(in.begin(), in.end(), expression2); i != end; ++i)
            {
                const tr1::smatch match = *i;
                // what[0] contains the whole string
                // what[1] contains the url part.
                // what[2] contains the name
                CString sMatch = CUnicodeUtils::GetUnicode(std::string(match[1]).c_str());
                sMatch.TrimRight('/');
                CString url = m_InitialUrl + L"/" + sMatch;
                CItem item;
                item.absolutepath = url;
                item.kind = svn_node_dir;
                item.path = sMatch;
                SRepositoryInfo repoinfo;
                repoinfo.root = url;
                repoinfo.revision = SVNRev::REV_HEAD;
                repoinfo.peg_revision = SVNRev::REV_HEAD;
                item.repository = repoinfo;
                AutoInsert(hRoot, item);
                ++nCountNewEntries;
            }
            return (nCountNewEntries>0);
        }
    }
    return false;
}

void CRepositoryBrowser::OnUrlHistoryBack()
{
    if (m_UrlHistory.empty())
        return;

    CString url = m_UrlHistory.front();
    if (url == GetPath())
    {
        m_UrlHistory.pop_front();
        if (m_UrlHistory.empty())
            return;
        url = m_UrlHistory.front();
    }
    SVNRev r = GetRevision();
    m_UrlHistoryForward.push_front(GetPath());
    ChangeToUrl(url, r, true);
    m_UrlHistory.pop_front();
    m_barRepository.ShowUrl (url, r);
}

void CRepositoryBrowser::OnUrlHistoryForward()
{
    if (m_UrlHistoryForward.empty())
        return;

    CString url = m_UrlHistoryForward.front();
    if (url == GetPath())
    {
        m_UrlHistoryForward.pop_front();
        if (m_UrlHistoryForward.empty())
            return;
        url = m_UrlHistoryForward.front();
    }
    SVNRev r = GetRevision();
    m_barRepository.ShowUrl (url, r);
    ChangeToUrl(url, r, true);
    m_UrlHistoryForward.pop_front();
    m_barRepository.ShowUrl (url, r);
}

void CRepositoryBrowser::GetStatus()
{
    if (!m_bSparseCheckoutMode || m_wcPath.IsEmpty())
        return;

    CTSVNPath retPath;
    SVNStatus status(m_pbCancel, true);
    svn_client_status_t * s = NULL;
    s = status.GetFirstFileStatus(m_wcPath, retPath, false, svn_depth_infinity, true, true);
    while ((s) && (!m_pbCancel))
    {
        CStringA url = s->repos_root_url;
        url += '/';
        url += s->repos_relpath;
        m_wcDepths[CUnicodeUtils::GetUnicode(url)] = s->depth;

        s = status.GetNextFileStatus(retPath);
    }
}

bool CRepositoryBrowser::RunStartCommit( const CTSVNPathList& pathlist, CString& sLogMsg )
{
    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(CTSVNPath(m_InitialUrl), m_ProjectProperties);
    if (CHooks::Instance().StartCommit(GetSafeHwnd(), pathlist, sLogMsg, exitcode, error))
    {
        if (exitcode)
        {
            CString sErrorMsg;
            sErrorMsg.Format(IDS_ERR_HOOKFAILED, (LPCWSTR)error);

            CTaskDialog taskdlg(sErrorMsg,
                                CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
            taskdlg.SetDefaultCommandControl(1);
            taskdlg.SetMainIcon(TD_ERROR_ICON);
            return (taskdlg.DoModal(GetSafeHwnd()) == 1);
        }
    }
    return true;
}

bool CRepositoryBrowser::RunPreCommit( const CTSVNPathList& pathlist, svn_depth_t depth, CString& sMsg )
{
    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(CTSVNPath(m_InitialUrl), m_ProjectProperties);
    if (CHooks::Instance().PreCommit(GetSafeHwnd(), pathlist, depth, sMsg, exitcode, error))
    {
        if (exitcode)
        {
            CString sErrorMsg;
            sErrorMsg.Format(IDS_ERR_HOOKFAILED, (LPCWSTR)error);

            CTaskDialog taskdlg(sErrorMsg,
                                CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
            taskdlg.SetDefaultCommandControl(1);
            taskdlg.SetMainIcon(TD_ERROR_ICON);
            return (taskdlg.DoModal(GetSafeHwnd()) == 1);
        }
    }
    return true;
}

bool CRepositoryBrowser::RunPostCommit( const CTSVNPathList& pathlist, svn_depth_t depth, svn_revnum_t revEnd, const CString& sMsg )
{
    DWORD exitcode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(CTSVNPath(m_InitialUrl), m_ProjectProperties);
    if (CHooks::Instance().PostCommit(GetSafeHwnd(), pathlist, depth, revEnd, sMsg, exitcode, error))
    {
        if (exitcode)
        {
            CString temp;
            temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
            ::MessageBox(GetSafeHwnd(), temp, L"TortoiseSVN", MB_ICONERROR);
            return false;
        }
    }
    return true;
}

HTREEITEM CRepositoryBrowser::FindBookmarkRoot()
{
    HTREEITEM hRoot = m_RepoTree.GetRootItem();
    while (hRoot)
    {
        CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData(hRoot);
        if (pItem && pItem->bookmark)
            return hRoot;
        hRoot = m_RepoTree.GetNextItem(hRoot, TVGN_NEXT);
    }
    return NULL;
}

void CRepositoryBrowser::RefreshBookmarks()
{
    HTREEITEM hBookmarkRoot = FindBookmarkRoot();
    if (hBookmarkRoot)
        RecursiveRemove(hBookmarkRoot, true);
    else
    {
        if (m_bSparseCheckoutMode)
            return;
        // the tree view is empty, just fill in the repository root
        HTREEITEM hDummy = m_RepoTree.InsertItem(L"", 0, 0, TVI_ROOT, TVI_LAST);
        //m_RepoTree.SetItemStateEx(hDummy, TVIS_EX_DISABLED);
        auto pDummy = new CTreeItem();
        pDummy->dummy = true;
        pDummy->kind = svn_node_dir;
        m_RepoTree.SetItemData(hDummy, (DWORD_PTR)pDummy);

        CTreeItem * pTreeItem = new CTreeItem();
        pTreeItem->kind = svn_node_dir;
        pTreeItem->bookmark = true;

        CString sBook(MAKEINTRESOURCE(IDS_BOOKMARKS));
        TVINSERTSTRUCT tvinsert = {0};
        tvinsert.hParent = TVI_ROOT;
        tvinsert.hInsertAfter = TVI_LAST;
        tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
        tvinsert.itemex.pszText = sBook.GetBuffer();
        tvinsert.itemex.cChildren = 1;
        tvinsert.itemex.lParam = (LPARAM)pTreeItem;
        tvinsert.itemex.iImage = m_nBookmarksIcon;
        tvinsert.itemex.iSelectedImage = m_nBookmarksIcon;
        hBookmarkRoot = m_RepoTree.InsertItem(&tvinsert);
        sBook.ReleaseBuffer();
    }
    if (hBookmarkRoot)
    {
        for (const auto& bm : m_bookmarks)
        {
            CString bookmark = bm.c_str();
            CTreeItem * pTreeItem = new CTreeItem();
            pTreeItem->kind = svn_node_file;
            pTreeItem->bookmark = true;
            pTreeItem->url = bookmark;

            CString sBook(MAKEINTRESOURCE(IDS_BOOKMARKS));
            TVINSERTSTRUCT tvinsert = {0};
            tvinsert.hParent = hBookmarkRoot;
            tvinsert.hInsertAfter = TVI_LAST;
            tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
            tvinsert.itemex.pszText = bookmark.GetBuffer();
            tvinsert.itemex.cChildren = 0;
            tvinsert.itemex.lParam = (LPARAM)pTreeItem;
            tvinsert.itemex.iImage = m_nIconFolder;
            tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;
            m_RepoTree.InsertItem(&tvinsert);
            bookmark.ReleaseBuffer();
        }
    }
}

void CRepositoryBrowser::LoadBookmarks()
{
    m_bookmarks.clear();
    CString sFilepath = CPathUtils::GetAppDataDirectory() + L"repobrowserbookmarks";
    CStdioFile file;
    if (file.Open(sFilepath, CFile::typeBinary | CFile::modeRead | CFile::shareDenyWrite))
    {
        CString strLine;
        while (file.ReadString(strLine))
        {
            m_bookmarks.insert((LPCWSTR)strLine);
        }
        file.Close();
    }
}

void CRepositoryBrowser::SaveBookmarks()
{
    CString sFilepath = CPathUtils::GetAppDataDirectory() + L"repobrowserbookmarks";

    CStdioFile file;
    if (file.Open(sFilepath, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate))
    {
        for (const auto& bm : m_bookmarks)
        {
            file.WriteString(bm.c_str());
            file.WriteString(L"\n");
        }
        file.Close();
    }
}



