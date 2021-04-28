// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2021 - TortoiseSVN

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
#include "ExportDlg.h"
#include "SVNProgressDlg.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
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
#include "SmartHandle.h"
#include "DPIAware.h"

#include <fstream>
#include <sstream>
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")

#define OVERLAY_EXTERNAL 1

static const apr_uint32_t direntAllExceptHasProps = (SVN_DIRENT_KIND | SVN_DIRENT_SIZE | SVN_DIRENT_CREATED_REV | SVN_DIRENT_TIME | SVN_DIRENT_LAST_AUTHOR);

bool CRepositoryBrowser::m_bSortLogical = true;

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
    ID_URLTOCLIPBOARDREV,
    ID_URLTOCLIPBOARDVIEWERREV,
    ID_URLTOCLIPBOARDVIEWERPATHREV,
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
    ID_SWITCHTO,
};

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableStandAloneDialog)

CRepositoryBrowser::CRepositoryBrowser(const CString& url, const SVNRev& rev)
    : CResizableStandAloneDialog(CRepositoryBrowser::IDD, nullptr)
    , m_bInitDone(false)
    , m_cnrRepositoryBar(&m_barRepository)
    , m_hAccel(nullptr)
    , m_cancelled(false)
    , m_bStandAlone(true)
    , m_bSparseCheckoutMode(false)
    , m_initialUrl(url)
    , m_bThreadRunning(false)
    , m_bTrySVNParentPath(true)
    , m_pListCtrlTreeItem(nullptr)
    , m_nBookmarksIcon(0)
    , m_nIconFolder(0)
    , m_nOpenIconFolder(0)
    , m_nExternalOvl(0)
    , m_nSVNParentPath(0)
    , m_blockEvents(0)
    , m_bSortAscending(true)
    , m_nSortedColumn(0)
    , m_pTreeDropTarget(nullptr)
    , m_pListDropTarget(nullptr)
    , m_bRightDrag(false)
    , m_oldY(0)
    , m_oldX(0)
    , bDragMode(FALSE)
    , m_diffKind(svn_node_none)
    , m_backgroundJobs(0, 1, true)
{
    ConstructorInit(rev);
}

CRepositoryBrowser::CRepositoryBrowser(const CString& url, const SVNRev& rev, CWnd* pParent)
    : CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
    , m_bInitDone(false)
    , m_cnrRepositoryBar(&m_barRepository)
    , m_hAccel(nullptr)
    , m_cancelled(false)
    , m_bStandAlone(false)
    , m_bSparseCheckoutMode(false)
    , m_initialUrl(url)
    , m_bThreadRunning(false)
    , m_bTrySVNParentPath(true)
    , m_pListCtrlTreeItem(nullptr)
    , m_nBookmarksIcon(0)
    , m_nIconFolder(0)
    , m_nOpenIconFolder(0)
    , m_nExternalOvl(0)
    , m_nSVNParentPath(0)
    , m_blockEvents(0)
    , m_bSortAscending(true)
    , m_nSortedColumn(0)
    , m_pTreeDropTarget(nullptr)
    , m_pListDropTarget(nullptr)
    , m_bRightDrag(false)
    , m_oldY(0)
    , m_oldX(0)
    , bDragMode(FALSE)
    , m_diffKind(svn_node_none)
    , m_backgroundJobs(0, 1, true)
{
    ConstructorInit(rev);
}

void CRepositoryBrowser::ConstructorInit(const SVNRev& rev)
{
    SecureZeroMemory(&m_arColumnWidths, sizeof(m_arColumnWidths));
    SecureZeroMemory(&m_arColumnAutoWidths, sizeof(m_arColumnAutoWidths));
    m_repository.revision        = rev;
    m_repository.isSVNParentPath = false;
    m_bSortLogical               = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
    if (m_bSortLogical)
        m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
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
    if (m_repoTree.ItemHasChildren(hItem))
    {
        for (HTREEITEM childItem = m_repoTree.GetChildItem(hItem); childItem != nullptr;)
        {
            RecursiveRemove(childItem);
            if (bChildrenOnly)
            {
                CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(childItem));
                if (pTreeItem && !pTreeItem->m_bookmark && (m_pListCtrlTreeItem == pTreeItem))
                {
                    m_repoList.DeleteAllItems();
                    m_pListCtrlTreeItem = nullptr;
                }
                delete pTreeItem;
                m_repoTree.SetItemData(childItem, 0);
                HTREEITEM hDelItem = childItem;
                childItem          = m_repoTree.GetNextItem(childItem, TVGN_NEXT);
                m_repoTree.DeleteItem(hDelItem);
            }
            else
                childItem = m_repoTree.GetNextItem(childItem, TVGN_NEXT);
        }
    }

    if ((hItem) && (!bChildrenOnly))
    {
        CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hItem));
        if (pTreeItem && !pTreeItem->m_bookmark && (m_pListCtrlTreeItem == pTreeItem))
        {
            m_repoList.DeleteAllItems();
            m_pListCtrlTreeItem = nullptr;
        }
        delete pTreeItem;
        m_repoTree.SetItemData(hItem, 0);
    }
}

void CRepositoryBrowser::ClearUI()
{
    CAutoWriteLock locker(m_guard);
    m_repoList.DeleteAllItems();

    HTREEITEM hRoot = m_repoTree.GetRootItem();
    do
    {
        RecursiveRemove(hRoot);
        m_repoTree.DeleteItem(hRoot);
        hRoot = m_repoTree.GetRootItem();
    } while (hRoot);

    m_repoTree.DeleteAllItems();
}

BOOL CRepositoryBrowser::Cancel()
{
    return m_cancelled;
}

void CRepositoryBrowser::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_REPOTREE, m_repoTree);
    DDX_Control(pDX, IDC_REPOLIST, m_repoList);
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
    ON_WM_SYSCOLORCHANGE()
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

    m_hAccel = LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_ACC_REPOBROWSER));

    m_nExternalOvl = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_EXTERNALOVL, 0, 0));
    if (m_nExternalOvl >= 0)
        SYS_IMAGE_LIST().SetOverlayImage(m_nExternalOvl, OVERLAY_EXTERNAL);
    m_nSVNParentPath = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_CACHE, 0, 0));

    m_cnrRepositoryBar.SubclassDlgItem(IDC_REPOS_BAR_CNR, this);
    m_barRepository.Create(&m_cnrRepositoryBar, 12345);
    m_barRepository.SetIRepo(this);
    if (m_bSparseCheckoutMode)
    {
        m_cnrRepositoryBar.ShowWindow(SW_HIDE);
        m_barRepository.ShowWindow(SW_HIDE);
        m_repoTree.ModifyStyle(0, TVS_CHECKBOXES);
    }
    else
    {
        m_pTreeDropTarget = new CTreeDropTarget(this);
        RegisterDragDrop(m_repoTree.GetSafeHwnd(), m_pTreeDropTarget);
        // create the supported formats:
        FORMATETC ftetc = {0};
        ftetc.cfFormat  = CF_SVNURL;
        ftetc.dwAspect  = DVASPECT_CONTENT;
        ftetc.lindex    = -1;
        ftetc.tymed     = TYMED_HGLOBAL;
        m_pTreeDropTarget->AddSuportedFormat(ftetc);
        ftetc.cfFormat = CF_HDROP;
        m_pTreeDropTarget->AddSuportedFormat(ftetc);
        ftetc.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_DROPDESCRIPTION));
        m_pTreeDropTarget->AddSuportedFormat(ftetc);

        m_pListDropTarget = new CListDropTarget(this);
        RegisterDragDrop(m_repoList.GetSafeHwnd(), m_pListDropTarget);
        // create the supported formats:
        ftetc.cfFormat = CF_SVNURL;
        m_pListDropTarget->AddSuportedFormat(ftetc);
        ftetc.cfFormat = CF_HDROP;
        m_pListDropTarget->AddSuportedFormat(ftetc);
        ftetc.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_DROPDESCRIPTION));
        m_pListDropTarget->AddSuportedFormat(ftetc);
    }

    if (m_bStandAlone)
    {
        // reposition the buttons
        CRect rectCancel;
        GetDlgItem(IDCANCEL)->GetWindowRect(rectCancel);
        GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
        ScreenToClient(rectCancel);
        GetDlgItem(IDOK)->MoveWindow(rectCancel);
        CRect inforect;
        GetDlgItem(IDC_INFOLABEL)->GetWindowRect(inforect);
        inforect.right += rectCancel.Width();
        ScreenToClient(inforect);
        GetDlgItem(IDC_INFOLABEL)->MoveWindow(inforect);
    }

    m_nIconFolder     = SYS_IMAGE_LIST().GetDirIconIndex();
    m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();

    if (m_bSparseCheckoutMode)
    {
        m_repoList.ShowWindow(SW_HIDE);
    }
    else
    {
        // set up the list control
        // set the extended style of the list control
        // the style LVS_EX_FULLROWSELECT interferes with the background watermark image but it's more important to be able to select in the whole row.
        CRegDWORD regFullRowSelect(L"Software\\TortoiseSVN\\FullRowSelect", TRUE);
        DWORD     exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
        if (static_cast<DWORD>(regFullRowSelect))
            exStyle |= LVS_EX_FULLROWSELECT;
        m_repoList.SetExtendedStyle(exStyle);
        m_repoList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
        ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_INITWAIT)));
    }
    m_nBookmarksIcon = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_BOOKMARKS, 0, 0));
    m_repoTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);
    // TVS_EX_FADEINOUTEXPANDOS style must not be set:
    // if it is set, there's a UI glitch when editing labels:
    // the text vanishes if the mouse cursor is moved away from
    // the edit control
    DWORD exStyle = TVS_EX_AUTOHSCROLL | TVS_EX_DOUBLEBUFFER;
    if (m_bSparseCheckoutMode)
        exStyle |= TVS_EX_MULTISELECT;
    m_repoTree.SetExtendedStyle(exStyle, exStyle);
    SetWindowTheme(m_repoList.GetSafeHwnd(), L"Explorer", nullptr);
    SetWindowTheme(m_repoTree.GetSafeHwnd(), L"Explorer", nullptr);

    int borderWidth = 0;
    if (IsAppThemed())
    {
        CAutoThemeData hTheme = OpenThemeData(m_repoTree, L"TREEVIEW");
        GetThemeMetric(hTheme, nullptr, TVP_TREEITEM, TREIS_NORMAL, TMT_BORDERSIZE, &borderWidth);
    }
    else
    {
        borderWidth = GetSystemMetrics(SM_CYBORDER);
    }
    m_repoTree.SetItemHeight(static_cast<SHORT>(m_repoTree.GetItemHeight() + 2 * borderWidth));

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
    if (!m_bSparseCheckoutMode)
    {
        // 10 dialog units: 6 units left border of the tree control, 4 units between
        // the tree and the list control
        CRect drc(0, 0, 10, 10);
        MapDialogRect(&drc);
        HandleDividerMove(CPoint(xPos + drc.right, CDPIAware::Instance().Scale(GetSafeHwnd(), 10)));
    }
    else
    {
        // have the tree control use the whole space of the dialog
        RemoveAnchor(IDC_REPOTREE);
        RECT rcTree{};
        RECT rcBar{};
        RECT rc{};
        m_repoTree.GetWindowRect(&rcTree);
        m_barRepository.GetWindowRect(&rcBar);
        UnionRect(&rc, &rcBar, &rcTree);
        ScreenToClient(&rc);
        m_repoTree.MoveWindow(&rc, FALSE);
        AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
    }
    SetPromptParentWindow(m_hWnd);
    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(InitThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
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
        new async::CAsyncCall(this, &CRepositoryBrowser::GetStatus, &m_backgroundJobs);
    }

    // repository properties

    m_initialUrl          = CPathUtils::PathUnescape(m_initialUrl);
    int questionMarkIndex = -1;
    if ((questionMarkIndex = m_initialUrl.Find('?')) >= 0)
    {
        // url can be in the form
        // http://host/repos/path?[p=PEG][&r=REV]
        CString revString    = m_initialUrl.Mid(questionMarkIndex + 1);
        CString pegRevString = revString;

        int ampPos = revString.Find('&');
        if (ampPos >= 0)
        {
            revString    = revString.Mid(ampPos + 1);
            pegRevString = pegRevString.Left(ampPos);
        }

        pegRevString.Trim(L"p=");
        if (!pegRevString.IsEmpty())
            m_repository.pegRevision = SVNRev(pegRevString);

        revString.Trim(L"r=");
        if (!revString.IsEmpty())
            m_repository.revision = SVNRev(revString);

        m_repository.isSVNParentPath = false;
        m_initialUrl                 = m_initialUrl.Left(questionMarkIndex);
    }

    if (!svn_dirent_is_absolute(CUnicodeUtils::GetUTF8(m_initialUrl)) &&
        !svn_path_is_url(CUnicodeUtils::GetUTF8(m_initialUrl)))
    {
        CString sError;
        sError.Format(IDS_ERR_MUSTBEURLORPATH, static_cast<LPCWSTR>(m_initialUrl));
        ShowText(sError, true);
        m_initialUrl.Empty();
        return;
    }
    // Since an url passed here isn't necessarily the real/final url, for
    // example in case of redirects or if the urls is a putty session name,
    // we have to first find out the real url here and store that real/final
    // url in m_InitialUrl.
    SVNInfo            inf;
    const SVNInfoData* pInfData = inf.GetFirstFileInfo(CTSVNPath(m_initialUrl), m_repository.pegRevision.IsValid() ? m_repository.pegRevision : m_repository.revision, m_repository.revision);
    if (pInfData)
    {
        m_repository.root = CPathUtils::PathUnescape(pInfData->reposRoot);
        m_repository.uuid = pInfData->reposUuid;
        m_initialUrl      = CPathUtils::PathUnescape(pInfData->url);
    }
    else
    {
        if ((m_initialUrl.Left(7) == L"http://") || (m_initialUrl.Left(8) == L"https://"))
        {
            if (TrySVNParentPath())
                return;
        }
        m_repository.root = CPathUtils::PathUnescape(GetRepositoryRootAndUUID(CTSVNPath(m_initialUrl), true, m_repository.uuid));
    }

    // problem: SVN reports the repository root without the port number if it's
    // the default port!
    m_initialUrl += L"/";
    if (m_initialUrl.Left(6) == L"svn://")
        m_initialUrl.Replace(L":3690/", L"/");
    if (m_initialUrl.Left(7) == L"http://")
        m_initialUrl.Replace(L":80/", L"/");
    if (m_initialUrl.Left(8) == L"https://")
        m_initialUrl.Replace(L":443/", L"/");
    m_initialUrl.TrimRight('/');

    m_backgroundJobs.WaitForEmptyQueue();

    // (try to) fetch the HEAD revision

    svn_revnum_t headRevision = GetHEADRevision(CTSVNPath(m_initialUrl), false);

    // let's see whether the URL was a directory

    CString userCancelledError;
    userCancelledError.LoadStringW(IDS_SVN_USERCANCELLED);

    SVNRev pegRev = m_repository.pegRevision.IsValid() ? m_repository.pegRevision : m_repository.revision;

    std::deque<CItem> dummy;
    CString           redirectedUrl;
    CString           error = m_cancelled
                                  ? userCancelledError
                                  : m_lister.GetList(m_initialUrl, pegRev, m_repository, 0, false, dummy, redirectedUrl);

    if (!redirectedUrl.IsEmpty() && svn_path_is_url(CUnicodeUtils::GetUTF8(m_initialUrl)))
        m_initialUrl = CPathUtils::PathUnescape(redirectedUrl);

    // the only way CQuery::List will return the following error
    // is by calling it with a file path instead of a dir path

    CString wasFileError;
    wasFileError.LoadStringW(IDS_ERR_ERROR);

    // maybe, we already know that this was a (valid) file path

    if (error == wasFileError)
    {
        m_initialUrl = m_initialUrl.Left(m_initialUrl.ReverseFind('/'));
        error        = m_lister.GetList(m_initialUrl, pegRev, m_repository, 0, false, dummy, redirectedUrl);
    }

    // exit upon cancel

    if (m_cancelled)
    {
        m_initialUrl.Empty();
        ShowText(error, true);
        return;
    }

    // let's (try to) access all levels in the folder path
    if (!m_repository.root.IsEmpty())
    {
        for (CString path = m_initialUrl; path.GetLength() >= m_repository.root.GetLength(); path = path.Left(path.ReverseFind('/')))
        {
            m_lister.Enqueue(path, pegRev, m_repository, 0, !m_bSparseCheckoutMode && m_bShowExternals);
        }
    }

    // did our optimistic strategy work?

    if (error.IsEmpty() && (headRevision >= 0))
    {
        // optimistic strategy was successful

        if (m_repository.revision.IsHead())
            m_barRepository.SetHeadRevision(headRevision);
    }
    else
    {
        // We don't know if the url passed to us points to a file or a folder,
        // let's find out:
        SVNInfo            info;
        const SVNInfoData* data = nullptr;
        do
        {
            data = info.GetFirstFileInfo(CTSVNPath(m_initialUrl), m_repository.revision, m_repository.revision);
            if ((data == nullptr) || (data->kind != svn_node_dir))
            {
                // in case the url is not a valid directory, try the parent dir
                // until there's no more parent dir
                auto lastSlashPos = m_initialUrl.ReverseFind('/');
                if (data && data->kind == svn_node_file)
                    m_initialFilename = m_initialUrl.Mid(lastSlashPos + 1);
                m_initialUrl = m_initialUrl.Left(lastSlashPos);
                if ((m_initialUrl.Compare(L"http://") == 0) ||
                    (m_initialUrl.Compare(L"http:/") == 0) ||
                    (m_initialUrl.Compare(L"https://") == 0) ||
                    (m_initialUrl.Compare(L"https:/") == 0) ||
                    (m_initialUrl.Compare(L"svn://") == 0) ||
                    (m_initialUrl.Compare(L"svn:/") == 0) ||
                    (m_initialUrl.Compare(L"svn+ssh:/") == 0) ||
                    (m_initialUrl.Compare(L"svn+ssh://") == 0) ||
                    (m_initialUrl.Compare(L"file:///") == 0) ||
                    (m_initialUrl.Compare(L"file://") == 0) ||
                    (m_initialUrl.Compare(L"file:/") == 0))
                {
                    m_initialUrl.Empty();
                }
                if (error.IsEmpty())
                {
                    if (((data) && (data->kind == svn_node_dir)) || (data == nullptr))
                        error = info.GetLastErrorMessage();
                }
            }
        } while (!m_initialUrl.IsEmpty() && ((data == nullptr) || (data->kind != svn_node_dir)));

        if (data == nullptr)
        {
            m_initialUrl.Empty();
            ShowText(error, true);
            return;
        }
        else if (m_repository.revision.IsHead())
        {
            m_barRepository.SetHeadRevision(data->rev);
        }

        m_repository.root = CPathUtils::PathUnescape(data->reposRoot);
        m_repository.uuid = data->reposUuid;
    }

    m_initialUrl.TrimRight('/');

    // the initial url can be in the format file:///\, but the
    // repository root returned would still be file://
    // to avoid string length comparison faults, we adjust
    // the repository root here to match the initial url
    if ((m_initialUrl.Left(9).CompareNoCase(L"file:///\\") == 0) &&
        (m_repository.root.Left(9).CompareNoCase(L"file:///\\") != 0))
        m_repository.root.Replace(L"file://", L"file:///\\");

    // now check the repository root for the url type, then
    // set the corresponding background image
    if (!m_repository.root.IsEmpty())
    {
        UINT nID = IDI_REPO_UNKNOWN;
        if (m_repository.root.Left(7).CompareNoCase(L"http://") == 0)
            nID = IDI_REPO_HTTP;
        if (m_repository.root.Left(8).CompareNoCase(L"https://") == 0)
            nID = IDI_REPO_HTTPS;
        if (m_repository.root.Left(6).CompareNoCase(L"svn://") == 0)
            nID = IDI_REPO_SVN;
        if (m_repository.root.Left(10).CompareNoCase(L"svn+ssh://") == 0)
            nID = IDI_REPO_SVNSSH;
        if (m_repository.root.Left(7).CompareNoCase(L"file://") == 0)
            nID = IDI_REPO_FILE;

        if (IsAppThemed())
        {
            CAppUtils::SetListCtrlBackgroundImage(m_repoList.GetSafeHwnd(), nID);
        }
    }
}

UINT CRepositoryBrowser::InitThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return static_cast<CRepositoryBrowser*>(pVoid)->InitThread();
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

    InterlockedExchange(&m_bThreadRunning, FALSE);
    m_cancelled = false;

    RefreshCursor();
    return 0;
}

LRESULT CRepositoryBrowser::OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_cancelled || m_initialUrl.IsEmpty())
        return 0;

    m_barRepository.SetRevision(m_repository.revision);
    m_barRepository.ShowUrl(m_initialUrl, m_repository.revision);
    ChangeToUrl(m_initialUrl, m_repository.revision, true);
    if (!m_initialFilename.IsEmpty())
    {
        auto count = m_repoList.GetItemCount();
        for (int i = 0; i < count; ++i)
        {
            if (m_initialFilename.Compare(m_repoList.GetItemText(i, 0)) == 0)
            {
                m_repoList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
                break;
            }
        }
    }
    RefreshBookmarks();

    m_bInitDone = TRUE;
    return 0;
}

LRESULT CRepositoryBrowser::OnRefreshURL(WPARAM /*wParam*/, LPARAM lParam)
{
    const wchar_t* url = reinterpret_cast<const wchar_t*>(lParam);

    HTREEITEM item = FindUrl(url);
    if (item)
        RefreshNode(item, true);

    // Memory was obtained from tscdup(), so free() must be called
    free(static_cast<void*>(const_cast<wchar_t*>(url)));
    return 0;
}

void CRepositoryBrowser::OnOK()
{
    if (m_blockEvents)
        return;
    if (m_repoTree.GetEditControl())
    {
        // in case there's an edit control, end the ongoing
        // editing of the item instead of dismissing the dialog.
        m_repoTree.EndEditLabelNow(false);
        return;
    }

    StoreSelectedURLs();
    if ((GetFocus() == &m_repoList) && ((GetKeyState(VK_MENU) & 0x8000) == 0))
    {
        // list control has focus: 'enter' the folder

        if (m_repoList.GetSelectedCount() != 1)
            return;

        POSITION pos = m_repoList.GetFirstSelectedItemPosition();
        if (pos)
            OpenFromList(m_repoList.GetNextSelectedItem(pos));
        return;
    }

    if (m_editFileCommand)
    {
        if (m_editFileCommand->StopWaitingForEditor())
            return;
    }

    m_cancelled = TRUE;
    m_lister.Cancel();

    if (m_repoList.GetSelectedCount() == 1)
    {
        POSITION pos = m_repoList.GetFirstSelectedItemPosition();
        if (pos)
        {
            int           selIndex = m_repoList.GetNextSelectedItem(pos);
            CAutoReadLock locker(m_guard);
            CItem*        pItem = reinterpret_cast<CItem*>(m_repoList.GetItemData(selIndex));
            if (pItem)
                m_barRepository.ShowUrl(pItem->m_absolutePath, pItem->m_repository.revision);
        }
    }

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
        HTREEITEM hRoot = m_repoTree.GetRootItem();
        CheckoutDepthForItem(hRoot);
        FilterInfinityDepthItems(m_checkoutDepths);
        FilterInfinityDepthItems(m_updateDepths);
        FilterUnknownDepthItems(m_updateDepths);
    }

    ++m_blockEvents;
    ClearUI();
    --m_blockEvents;

    RevokeDragDrop(m_repoList.GetSafeHwnd());
    RevokeDragDrop(m_repoTree.GetSafeHwnd());

    CResizableStandAloneDialog::OnOK();
}

void CRepositoryBrowser::OnCancel()
{
    if (m_editFileCommand)
    {
        if (m_editFileCommand->StopWaitingForEditor())
            return;
    }
    m_cancelled = TRUE;
    m_lister.Cancel();

    if (m_bThreadRunning)
        return;

    RevokeDragDrop(m_repoList.GetSafeHwnd());
    RevokeDragDrop(m_repoTree.GetSafeHwnd());

    m_backgroundJobs.WaitForEmptyQueue();
    if (!m_bSparseCheckoutMode)
    {
        SaveColumnWidths(true);
        SaveDividerPosition();
    }

    ++m_blockEvents;
    ClearUI();
    --m_blockEvents;

    __super::OnCancel();
}

LPARAM CRepositoryBrowser::OnAuthCancelled(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_cancelled = TRUE;
    m_lister.Cancel();
    return 0;
}

void CRepositoryBrowser::OnSysColorChange()
{
    __super::OnSysColorChange();
    CTheme::Instance().OnSysColorChanged();
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
    if ((m_bThreadRunning) && (!IsCursorOverWindowBorder()))
    {
        HCURSOR hCur = LoadCursor(nullptr, IDC_WAIT);
        SetCursor(hCur);
        return TRUE;
    }
    if ((pWnd == this) && (!m_bSparseCheckoutMode))
    {
        RECT  rect;
        POINT pt;
        GetClientRect(&rect);
        GetCursorPos(&pt);
        ScreenToClient(&pt);
        if (PtInRect(&rect, pt))
        {
            ClientToScreen(&pt);
            // are we right of the tree control?
            GetDlgItem(IDC_REPOTREE)->GetWindowRect(&rect);
            if ((pt.x > rect.right) &&
                (pt.y >= rect.top + 3) &&
                (pt.y <= rect.bottom - 3))
            {
                // but left of the list control?
                GetDlgItem(IDC_REPOLIST)->GetWindowRect(&rect);
                if (pt.x < rect.left)
                {
                    HCURSOR hCur = LoadCursor(nullptr, IDC_SIZEWE);
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

    RECT rect{}, tree{}, list{}, treeList{}, treeListClient{};
    // create an union of the tree and list control rectangle
    GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
    if (m_bSparseCheckoutMode)
        treeList = tree;
    else
        UnionRect(&treeList, &tree, &list);
    treeListClient = treeList;
    ScreenToClient(&treeListClient);

    //convert the mouse coordinates relative to the top-left of
    //the window
    ClientToScreen(&point);
    GetClientRect(&rect);
    ClientToScreen(&rect);
    point.x -= rect.left;
    point.y -= treeList.top;

    //same for the window coordinates - make them relative to 0,0
    OffsetRect(&treeList, -treeList.left, -treeList.top);

    auto minWidth = CDPIAware::Instance().Scale(GetSafeHwnd(), REPOBROWSER_CTRL_MIN_WIDTH);

    if (point.x < treeList.left + minWidth)
        point.x = treeList.left + minWidth;
    if (point.x > treeList.right - minWidth)
        point.x = treeList.right - minWidth;

    if ((nFlags & MK_LBUTTON) && (point.x != m_oldX))
    {
        HandleDividerMove(point);

        m_oldX = point.x;
        m_oldY = point.y;
    }

    CStandAloneDialogTmpl<CResizableDialog>::OnMouseMove(nFlags, point);
}

void CRepositoryBrowser::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_bSparseCheckoutMode)
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);

    RECT rect{}, tree{}, list{}, treeList{}, treeListClient{};

    // create an union of the tree and list control rectangle
    GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
    UnionRect(&treeList, &tree, &list);
    treeListClient = treeList;
    ScreenToClient(&treeListClient);

    //convert the mouse coordinates relative to the top-left of
    //the window
    ClientToScreen(&point);
    GetClientRect(&rect);
    ClientToScreen(&rect);
    point.x -= rect.left;
    point.y -= treeList.top;

    //same for the window coordinates - make them relative to 0,0
    OffsetRect(&treeList, -treeList.left, -treeList.top);

    auto minWidth = CDPIAware::Instance().Scale(GetSafeHwnd(), REPOBROWSER_CTRL_MIN_WIDTH);
    auto divWidth = CDPIAware::Instance().Scale(GetSafeHwnd(), 3);

    if (point.x < treeList.left + minWidth)
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
    if (point.x > treeList.right - divWidth)
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
    if (point.x > treeList.right - minWidth)
        point.x = treeList.right - minWidth;

    if ((point.y < treeList.top + divWidth) ||
        (point.y > treeList.bottom - divWidth))
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);

    bDragMode = true;

    SetCapture();

    m_oldX = point.x;
    m_oldY = point.y;

    CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
}

void CRepositoryBrowser::HandleDividerMove(CPoint point)
{
    RECT rect{}, tree{}, list{}, treeList{}, treeListClient{};

    // create an union of the tree and list control rectangle
    GetDlgItem(IDC_REPOLIST)->GetWindowRect(&list);
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&tree);
    UnionRect(&treeList, &tree, &list);
    treeListClient = treeList;
    ScreenToClient(&treeListClient);

    ClientToScreen(&point);
    GetClientRect(&rect);
    ClientToScreen(&rect);

    auto minWidth = CDPIAware::Instance().Scale(GetSafeHwnd(), REPOBROWSER_CTRL_MIN_WIDTH);

    CPoint point2 = point;
    if (point2.x < treeList.left + minWidth)
        point2.x = treeList.left + minWidth;
    if (point2.x > treeList.right - minWidth)
        point2.x = treeList.right - minWidth;

    point.x -= rect.left;
    point.y -= treeList.top;

    OffsetRect(&treeList, -treeList.left, -treeList.top);

    if (point.x < treeList.left + minWidth)
        point.x = treeList.left + minWidth;
    if (point.x > treeList.right - minWidth)
        point.x = treeList.right - minWidth;

    m_oldX = point.x;
    m_oldY = point.y;

    auto divWidth = CDPIAware::Instance().Scale(GetSafeHwnd(), 2);
    //position the child controls
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&treeList);
    treeList.right = point2.x - divWidth;
    ScreenToClient(&treeList);
    RemoveAnchor(IDC_REPOTREE);
    GetDlgItem(IDC_REPOTREE)->MoveWindow(&treeList);
    GetDlgItem(IDC_REPOLIST)->GetWindowRect(&treeList);
    treeList.left = point2.x + divWidth;
    ScreenToClient(&treeList);
    RemoveAnchor(IDC_REPOLIST);
    GetDlgItem(IDC_REPOLIST)->MoveWindow(&treeList);

    AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
    AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);
}

void CRepositoryBrowser::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (bDragMode == FALSE)
        return;

    if (!m_bSparseCheckoutMode)
    {
        HandleDividerMove(point);

        bDragMode = false;
        ReleaseCapture();
    }

    CStandAloneDialogTmpl<CResizableDialog>::OnLButtonUp(nFlags, point);
}

void CRepositoryBrowser::OnCaptureChanged(CWnd* pWnd)
{
    bDragMode = false;

    __super::OnCaptureChanged(pWnd);
}

bool CRepositoryBrowser::ChangeToUrl(CString& url, SVNRev& rev, bool bAlreadyChecked)
{
    CWaitCursorEx wait;
    if (!bAlreadyChecked)
    {
        // check if the entered url is valid
        SVNInfo            info;
        const SVNInfoData* data    = nullptr;
        CString            origURL = url;
        do
        {
            data = info.GetFirstFileInfo(CTSVNPath(url), rev, rev);
            if (data && !rev.IsHead())
            {
                rev = data->rev;
            }
            if ((data == nullptr) || (data->kind != svn_node_dir))
            {
                // in case the url is not a valid directory, try the parent dir
                // until there's no more parent dir
                url = url.Left(url.ReverseFind('/'));
            }
        } while (!url.IsEmpty() && ((data == nullptr) || (data->kind != svn_node_dir)));
        if (url.IsEmpty())
            url = origURL;
    }

    CString root                = m_repository.root;
    bool    urlHasDifferentRoot = root.IsEmpty() || root.Compare(url.Left(root.GetLength())) || ((url.GetAt(root.GetLength()) != '/') && ((url.GetLength() > root.GetLength()) && (url.GetAt(root.GetLength()) != '/')));

    if ((static_cast<LONG>(rev) != static_cast<LONG>(m_repository.revision)) || urlHasDifferentRoot || m_repository.isSVNParentPath)
    {
        ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_WAIT)), true);

        // if the repository root has changed, initialize all data from scratch
        // and clear the project properties we might have loaded previously
        if (urlHasDifferentRoot)
            m_projectProperties = ProjectProperties();
        m_initialUrl                 = url;
        m_repository.revision        = rev;
        m_repository.isSVNParentPath = false;

        // if the revision changed, then invalidate everything
        m_bFetchChildren = false;
        ClearUI();
        InitRepo();
        RefreshBookmarks();

        if (m_repository.root.IsEmpty())
            return false;
        if (urlHasDifferentRoot)
            m_path = CTSVNPath(m_repository.root);
        CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), m_origDlgTitle);
        root = m_repository.root;

        if (m_initialUrl.Compare(url))
        {
            // url changed (redirect), update the combobox control
            url = m_initialUrl;
        }
    }

    HTREEITEM hItem = AutoInsert(url);
    if (hItem == nullptr)
        return false;

    CAutoReadLock locker(m_guard);
    CTreeItem*    pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hItem));
    if (pTreeItem == nullptr)
        return FALSE;

    if (!m_repoList.HasText())
        ShowText(L" ", true);

    m_bFetchChildren = !!CRegDWORD(L"Software\\TortoiseSVN\\RepoBrowserPrefetch", true);

    RefreshNode(hItem);
    m_repoTree.Expand(hItem, TVE_EXPAND);
    FillList(pTreeItem);

    ++m_blockEvents;
    m_repoTree.EnsureVisible(hItem);
    m_repoTree.SelectItem(hItem);
    --m_blockEvents;

    m_repoList.ClearText();
    m_repoTree.ClearText();

    return true;
}

void CRepositoryBrowser::SetPegRev(SVNRev& pegRev)
{
    m_repository.pegRevision = pegRev;
}

void CRepositoryBrowser::FillList(CTreeItem* pTreeItem)
{
    if (pTreeItem == nullptr)
        return;

    CAutoWriteLock locker(m_guard);
    CWaitCursorEx  wait;
    m_repoList.SetRedraw(false);
    m_repoList.DeleteAllItems();
    m_repoList.ClearText();
    m_repoTree.ClearText();
    m_pListCtrlTreeItem = pTreeItem;

    int c = m_repoList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_repoList.DeleteColumn(c--);

    c = 0;
    CString temp;
    //
    // column 0: contains tree
    temp.LoadString(IDS_LOG_FILE);
    m_repoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 1: file extension
    temp.LoadString(IDS_STATUSLIST_COLEXT);
    m_repoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 2: revision number
    temp.LoadString(IDS_LOG_REVISION);
    m_repoList.InsertColumn(c++, temp, LVCFMT_RIGHT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 3: author
    temp.LoadString(IDS_LOG_AUTHOR);
    m_repoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 4: size
    temp.LoadString(IDS_LOG_SIZE);
    m_repoList.InsertColumn(c++, temp, LVCFMT_RIGHT, LVSCW_AUTOSIZE_USEHEADER);
    //
    // column 5: date
    temp.LoadString(IDS_LOG_DATE);
    m_repoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    if (m_bShowLocks)
    {
        //
        // column 6: lock owner
        temp.LoadString(IDS_STATUSLIST_COLLOCK);
        m_repoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
        //
        // column 7: lock comment
        temp.LoadString(IDS_STATUSLIST_COLLOCKCOMMENT);
        m_repoList.InsertColumn(c++, temp, LVCFMT_LEFT, LVSCW_AUTOSIZE_USEHEADER);
    }

    int files   = 0;
    int folders = 0;
    // special case: error to show
    if (!pTreeItem->m_error.IsEmpty() && pTreeItem->m_children.empty())
    {
        ShowText(pTreeItem->m_error, true);
    }
    else
    {
        // now fill in the data

        int                      nCount = 0;
        const std::deque<CItem>& items  = pTreeItem->m_children;
        for (auto it = items.begin(); it != items.end(); ++it)
        {
            int iconIdx;
            if (it->m_kind == svn_node_dir)
            {
                iconIdx = m_nIconFolder;
                folders++;
            }
            else
            {
                iconIdx = SYS_IMAGE_LIST().GetPathIconIndex(it->m_path);
                files++;
            }
            int index = m_repoList.InsertItem(nCount, it->m_path, iconIdx);
            SetListItemInfo(index, &(*it));
        }

        ListView_SortItemsEx(m_repoList, ListSort, this);
        SetSortArrow();
    }
    if (m_arColumnWidths[0] == 0)
    {
        CRegString regColWidths(L"Software\\TortoiseSVN\\RepoBrowserColumnWidth");
        StringToWidthArray(regColWidths, m_arColumnWidths);
    }

    int maxCol = m_repoList.GetHeaderCtrl()->GetItemCount() - 1;
    for (int col = 0; col <= maxCol; col++)
    {
        if (m_arColumnWidths[col] == 0)
        {
            m_repoList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
            m_arColumnAutoWidths[col] = m_repoList.GetColumnWidth(col);
        }
        else
            m_repoList.SetColumnWidth(col, m_arColumnWidths[col]);
    }

    m_repoList.SetRedraw(true);

    temp.FormatMessage(IDS_REPOBROWSE_INFO,
                       static_cast<LPCWSTR>(pTreeItem->m_unescapedName),
                       files, folders, files + folders);
    if (!pTreeItem->m_error.IsEmpty() && pTreeItem->m_children.empty())
        SetDlgItemText(IDC_INFOLABEL, L"");
    else
        SetDlgItemText(IDC_INFOLABEL, temp);

    if (m_urlHistory.empty() || m_urlHistory.front() != pTreeItem->m_url)
        m_urlHistory.push_front(pTreeItem->m_url);
    if (m_urlHistoryForward.empty() || m_urlHistoryForward.front() != GetPath())
        m_urlHistoryForward.clear();
}

HTREEITEM CRepositoryBrowser::FindUrl(const CString& fullUrl)
{
    return FindUrl(fullUrl, fullUrl, TVI_ROOT);
}

HTREEITEM CRepositoryBrowser::FindUrl(const CString& fullUrl, const CString& url, HTREEITEM hItem /* = TVI_ROOT */)
{
    CAutoReadLock locker(m_guard);
    if (hItem == TVI_ROOT)
    {
        CString root = m_repository.root;
        hItem        = m_repoTree.GetRootItem();
        if (fullUrl.Compare(root) == 0)
            return hItem;
        return FindUrl(fullUrl, url.Mid(root.GetLength() + 1), hItem);
    }
    HTREEITEM hSibling = hItem;
    if (m_repoTree.GetNextItem(hItem, TVGN_CHILD))
    {
        hSibling = m_repoTree.GetNextItem(hItem, TVGN_CHILD);
        do
        {
            CTreeItem* pTItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSibling));
            if (pTItem == nullptr)
                continue;
            CString sSibling = pTItem->m_unescapedName;
            if (sSibling.Compare(url.Left(sSibling.GetLength())) == 0)
            {
                if (sSibling.GetLength() == url.GetLength())
                    return hSibling;
                if (url.GetAt(sSibling.GetLength()) == '/')
                    return FindUrl(fullUrl, url.Mid(sSibling.GetLength() + 1), hSibling);
            }
        } while ((hSibling = m_repoTree.GetNextItem(hSibling, TVGN_NEXT)) != nullptr);
    }

    return nullptr;
}

void CRepositoryBrowser::FetchChildren(HTREEITEM node)
{
    CAutoReadLock lockerR(m_guard);
    CTreeItem*    pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
    if (pTreeItem->m_childrenFetched)
        return;

    // block new background queries just for now to maximize foreground speed

    auto queryBlocker = m_lister.SuspendJobs();

    // standard list plus immediate externals

    std::deque<CItem>& children = pTreeItem->m_children;
    {
        CAutoWriteLock lockerW(m_guard);
        if (pTreeItem == m_pListCtrlTreeItem)
        {
            m_repoList.DeleteAllItems();
            m_pListCtrlTreeItem = nullptr;
        }
        CString redirectedUrl;
        children.clear();
        pTreeItem->m_hasChildFolders = false;
        pTreeItem->m_error           = m_lister.GetList(pTreeItem->m_url, pTreeItem->m_isExternal ? pTreeItem->m_repository.pegRevision : SVNRev(), pTreeItem->m_repository, !m_bSparseCheckoutMode && m_bShowExternals ? SVN_DIRENT_ALL : direntAllExceptHasProps, !m_bSparseCheckoutMode && m_bShowExternals, children, redirectedUrl);
        if (!pTreeItem->m_error.IsEmpty() && pTreeItem->m_unversioned)
        {
            pTreeItem->m_error.Empty();
            pTreeItem->m_hasChildFolders = true;
        }
    }

    int childFolderCount = 0;
    for (size_t i = 0, count = pTreeItem->m_children.size(); i < count; ++i)
    {
        const CItem& item = pTreeItem->m_children[i];
        if ((item.m_kind == svn_node_dir) && (!item.m_absolutePath.IsEmpty()))
        {
            pTreeItem->m_hasChildFolders = true;
            ++childFolderCount;
            if (m_bFetchChildren)
            {
                m_lister.Enqueue(item.m_absolutePath, item.m_isExternal ? item.m_repository.pegRevision : SVNRev(), item.m_repository, !m_bSparseCheckoutMode && m_bShowExternals ? SVN_DIRENT_ALL : direntAllExceptHasProps, item.m_hasProps && !m_bSparseCheckoutMode && m_bShowExternals);
                if (childFolderCount > 30)
                    break;
            }
        }
    }
    if ((pTreeItem->m_hasChildFolders) || (m_bSparseCheckoutMode))
        AutoInsert(node, pTreeItem->m_children);
    // if there are no child folders, remove the '+' in front of the node
    {
        TVITEM tvItem    = {0};
        tvItem.hItem     = node;
        tvItem.mask      = TVIF_CHILDREN;
        tvItem.cChildren = pTreeItem->m_hasChildFolders || (m_bSparseCheckoutMode && pTreeItem->m_children.size()) ? 1 : 0;
        m_repoTree.SetItem(&tvItem);
    }

    // add parent sub-tree externals
    if (m_bShowExternals)
    {
        CString relPath;
        for (
            ; node && pTreeItem->m_error.IsEmpty(); node = m_repoTree.GetParentItem(node))
        {
            CTreeItem* parentItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
            if (parentItem == nullptr)
                continue;
            if (parentItem->m_svnParentPathRoot)
                continue;
            parentItem->m_error = m_lister.AddSubTreeExternals(parentItem->m_url,
                                                               parentItem->m_isExternal ? parentItem->m_repository.pegRevision : SVNRev(),
                                                               parentItem->m_repository,
                                                               relPath,
                                                               children);
            if (!parentItem->m_error.IsEmpty() && parentItem->m_unversioned)
            {
                parentItem->m_error.Empty();
                parentItem->m_hasChildFolders = true;
            }
            relPath = parentItem->m_unescapedName + '/' + relPath;
        }
    }

    // done

    pTreeItem->m_childrenFetched = true;
}

HTREEITEM CRepositoryBrowser::AutoInsert(const CString& path)
{
    CString   currentPath;
    HTREEITEM node = nullptr;
    do
    {
        // name of the node to add

        CString name;

        // select next path level

        if (currentPath.IsEmpty())
        {
            if (m_bSparseCheckoutMode)
            {
                currentPath = m_initialUrl;
                name        = m_initialUrl;
            }
            else
            {
                currentPath = m_repository.root;
                name        = m_repository.root;
            }
        }
        else
        {
            int start = currentPath.GetLength() + 1;
            int pos   = path.Find('/', start);
            if (pos < 0)
                pos = path.GetLength();

            name        = path.Mid(start, pos - start);
            currentPath = path.Left(pos);
        }

        // root node?

        if (node == nullptr)
        {
            CAutoWriteLock locker(m_guard);
            node = m_repoTree.GetRootItem();
            if (node)
            {
                CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
                if (pTreeItem == nullptr)
                    node = nullptr;
                else if (pTreeItem->m_bookmark || pTreeItem->m_dummy)
                    node = nullptr;
            }
            if (node == nullptr)
            {
                // the tree view is empty, just fill in the repository root
                CTreeItem* pTreeItem       = new CTreeItem();
                pTreeItem->m_unescapedName = name;
                pTreeItem->m_url           = currentPath;
                pTreeItem->m_logicalPath   = currentPath;
                pTreeItem->m_repository    = m_repository;
                pTreeItem->m_kind          = svn_node_dir;

                TVINSERTSTRUCT tvInsert        = {nullptr};
                tvInsert.hParent               = TVI_ROOT;
                tvInsert.hInsertAfter          = TVI_FIRST;
                tvInsert.itemex.mask           = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
                tvInsert.itemex.pszText        = currentPath.GetBuffer(currentPath.GetLength());
                tvInsert.itemex.cChildren      = 1;
                tvInsert.itemex.lParam         = reinterpret_cast<LPARAM>(pTreeItem);
                tvInsert.itemex.iImage         = m_nIconFolder;
                tvInsert.itemex.iSelectedImage = m_nOpenIconFolder;
                if (m_bSparseCheckoutMode)
                {
                    tvInsert.itemex.state     = INDEXTOSTATEIMAGEMASK(2); // root item is always checked
                    tvInsert.itemex.stateMask = TVIS_STATEIMAGEMASK;
                }

                node = m_repoTree.InsertItem(&tvInsert);
                currentPath.ReleaseBuffer();
            }
        }
        else
        {
            // get all children

            CAutoReadLock      locker(m_guard);
            CTreeItem*         pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
            std::deque<CItem>& children  = pTreeItem->m_children;

            if (!pTreeItem->m_childrenFetched)
            {
                // not yet fetched -> do it now

                if (!RefreshNode(node))
                    return nullptr;

                for (size_t i = 0, count = children.size(); i < count; ++i)
                    if (children[i].m_kind == svn_node_dir)
                    {
                        pTreeItem->m_hasChildFolders = true;
                        break;
                    }
            }

            // select the one that matches the node name we are looking for

            const CItem* item = nullptr;
            for (size_t i = 0, count = children.size(); i < count; ++i)
                if ((children[i].m_kind == svn_node_dir) && (children[i].m_path == name))
                {
                    item = &children[i];
                    break;
                }

            if (item == nullptr)
            {
                if (path.Compare(m_initialUrl) == 0)
                {
                    // add this path manually.
                    // The path is valid, that was checked in the init function already.
                    // But it's possible that we don't have read access to its parent
                    // folder, so by adding the valid path manually we give the user
                    // a chance to still browse this folder in case he has read
                    // access to it
                    CTreeItem* pNodeTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
                    if (pNodeTreeItem && !pNodeTreeItem->m_error.IsEmpty())
                    {
                        TVITEM tvItem    = {0};
                        tvItem.hItem     = node;
                        tvItem.mask      = TVIF_CHILDREN;
                        tvItem.cChildren = 1;
                        m_repoTree.SetItem(&tvItem);
                    }
                    CItem manualItem(currentPath.Mid(currentPath.ReverseFind('/') + 1), L"", svn_node_dir, 0, true, -1, 0, L"", L"", L"", L"", false, 0, 0, currentPath, pTreeItem->m_repository);
                    node = AutoInsert(node, manualItem);
                }
                else
                    return nullptr;
            }
            else
            {
                // auto-add the node for the current path level
                node = AutoInsert(node, *item);
            }
        }

        // pre-fetch data for the next level

        if (m_bFetchChildren)
        {
            CAutoReadLock locker(m_guard);
            CTreeItem*    pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
            if ((pTreeItem != nullptr) && !pTreeItem->m_childrenFetched)
            {
                m_lister.Enqueue(pTreeItem->m_url, pTreeItem->m_isExternal ? pTreeItem->m_repository.pegRevision : SVNRev(), pTreeItem->m_repository, !m_bSparseCheckoutMode && m_bShowExternals ? SVN_DIRENT_ALL : direntAllExceptHasProps, !m_bSparseCheckoutMode && m_bShowExternals);
            }
        }
    } while (currentPath != path);

    // done

    return node;
}

HTREEITEM CRepositoryBrowser::AutoInsert(HTREEITEM hParent, const CItem& item)
{
    // look for an existing sub-node by comparing names

    {
        CAutoReadLock locker(m_guard);
        for (HTREEITEM hSibling = m_repoTree.GetNextItem(hParent, TVGN_CHILD); hSibling != nullptr; hSibling = m_repoTree.GetNextItem(hSibling, TVGN_NEXT))
        {
            CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSibling));
            if (pTreeItem && (pTreeItem->m_unescapedName == item.m_path))
                return hSibling;
        }
    }

    CAutoWriteLock locker(m_guard);
    // no such sub-node. Create one

    HTREEITEM hNewItem = Insert(hParent, reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hParent)), item);

    Sort(hParent);

    return hNewItem;
}

void CRepositoryBrowser::AutoInsert(HTREEITEM hParent, const std::deque<CItem>& items)
{
    std::map<CString, const CItem*> newItems;

    // determine what nodes need to be added

    for (size_t i = 0, count = items.size(); i < count; ++i)
        if ((items[i].m_kind == svn_node_dir) || (m_bSparseCheckoutMode))
            newItems.emplace(items[i].m_path, &items[i]);

    {
        CAutoReadLock locker(m_guard);
        for (HTREEITEM hSibling = m_repoTree.GetNextItem(hParent, TVGN_CHILD); hSibling != nullptr; hSibling = m_repoTree.GetNextItem(hSibling, TVGN_NEXT))
        {
            CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSibling));
            if (pTreeItem)
                newItems.erase(pTreeItem->m_unescapedName);
        }
    }

    if (newItems.empty())
        return;

    // Create missing sub-nodes

    CAutoWriteLock locker(m_guard);
    CTreeItem*     parentTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hParent));

    for (auto iter = newItems.cbegin(), end = newItems.cend(); iter != end; ++iter)
    {
        Insert(hParent, parentTreeItem, *iter->second);
    }

    Sort(hParent);
}

HTREEITEM CRepositoryBrowser::Insert(HTREEITEM hParent, CTreeItem* parentTreeItem, const CItem& item)
{
    CAutoWriteLock locker(m_guard);
    CTreeItem*     pTreeItem = new CTreeItem();
    CString        name      = item.m_path;

    pTreeItem->m_unescapedName = name;
    pTreeItem->m_url           = item.m_absolutePath;
    pTreeItem->m_revision      = item.m_createdRev;
    pTreeItem->m_logicalPath   = parentTreeItem->m_logicalPath + '/' + name;
    pTreeItem->m_repository    = item.m_repository;
    pTreeItem->m_isExternal    = item.m_isExternal;
    pTreeItem->m_kind          = item.m_kind;
    pTreeItem->m_unversioned   = item.m_unversioned;

    bool isSelectedForDiff = pTreeItem->m_url.CompareNoCase(m_diffURL.GetSVNPathString()) == 0;

    UINT state     = isSelectedForDiff ? TVIS_BOLD : 0;
    UINT stateMask = state;
    if (item.m_isExternal)
    {
        state |= INDEXTOOVERLAYMASK(OVERLAY_EXTERNAL);
        stateMask |= TVIS_OVERLAYMASK;
    }
    if (m_bSparseCheckoutMode)
    {
        state |= INDEXTOSTATEIMAGEMASK(m_repoTree.GetCheck(hParent) || (hParent == TVI_ROOT) ? 2 : 1);
        stateMask |= TVIS_STATEIMAGEMASK;
    }

    TVINSERTSTRUCT tvInsert   = {nullptr};
    tvInsert.hParent          = hParent;
    tvInsert.hInsertAfter     = TVI_FIRST;
    tvInsert.itemex.mask      = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
    tvInsert.itemex.state     = state;
    tvInsert.itemex.stateMask = stateMask;
    tvInsert.itemex.pszText   = name.GetBuffer(name.GetLength());
    tvInsert.itemex.lParam    = reinterpret_cast<LPARAM>(pTreeItem);
    if (item.m_kind == svn_node_dir)
    {
        tvInsert.itemex.cChildren      = 1;
        tvInsert.itemex.iImage         = m_nIconFolder;
        tvInsert.itemex.iSelectedImage = m_nOpenIconFolder;
    }
    else
    {
        pTreeItem->m_childrenFetched   = true;
        tvInsert.itemex.cChildren      = 0;
        tvInsert.itemex.iImage         = SYS_IMAGE_LIST().GetPathIconIndex(item.m_path);
        tvInsert.itemex.iSelectedImage = tvInsert.itemex.iImage;
    }
    HTREEITEM hNewItem = m_repoTree.InsertItem(&tvInsert);
    name.ReleaseBuffer();

    if (m_bSparseCheckoutMode)
    {
        auto it = m_wcDepths.find(pTreeItem->m_url);
        if (it != m_wcDepths.end())
        {
            switch (it->second)
            {
                case svn_depth_infinity:
                case svn_depth_immediates:
                case svn_depth_files:
                case svn_depth_empty:
                case svn_depth_unknown:
                    m_repoTree.SetCheck(hNewItem);
                    break;
                default:
                    m_repoTree.SetCheck(hNewItem, false);
                    break;
            }
        }
        else
        {
            if (!m_wcPath.IsEmpty() || m_repoTree.GetRootItem() == hParent)
            {
                // node without depth in already checked out WC or item on second (below root) level.
                // this nodes should be unchecked
                m_repoTree.SetCheck(hNewItem, false);
            }
            else
            {
                // other nodes inherit parent state
                m_repoTree.SetCheck(hNewItem, m_repoTree.GetCheck(hParent));
            }
        }
    }

    return hNewItem;
}

void CRepositoryBrowser::Sort(HTREEITEM parent)
{
    CAutoWriteLock locker(m_guard);
    TVSORTCB       tvs;
    tvs.hParent     = parent;
    tvs.lpfnCompare = TreeSort;
    tvs.lParam      = reinterpret_cast<LPARAM>(this);

    m_repoTree.SortChildrenCB(&tvs);
}

void CRepositoryBrowser::RefreshChildren(HTREEITEM node)
{
    CAutoReadLock locker(m_guard);
    CTreeItem*    pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
    if (pTreeItem == nullptr)
        return;

    pTreeItem->m_childrenFetched = false;
    InvalidateData(node);
    FetchChildren(node);

    // update node status and add sub-nodes for all sub-dirs
    int childfoldercount = 0;
    for (size_t i = 0, count = pTreeItem->m_children.size(); i < count; ++i)
    {
        const CItem& item = pTreeItem->m_children[i];
        if ((item.m_kind == svn_node_dir) && (!item.m_absolutePath.IsEmpty()))
        {
            pTreeItem->m_hasChildFolders = true;
            if (m_bFetchChildren)
            {
                ++childfoldercount;
                m_lister.Enqueue(item.m_absolutePath, item.m_isExternal ? item.m_repository.pegRevision : SVNRev(), item.m_repository, !m_bSparseCheckoutMode && m_bShowExternals ? SVN_DIRENT_ALL : direntAllExceptHasProps, item.m_hasProps && !m_bSparseCheckoutMode && m_bShowExternals);
                if (childfoldercount > 30)
                    break;
            }
        }
    }
}

bool CRepositoryBrowser::RefreshNode(HTREEITEM hNode, bool force /* = false*/)
{
    if (hNode == nullptr)
        return false;

    SaveColumnWidths();
    CWaitCursorEx wait;
    CAutoReadLock locker(m_guard);
    // block all events until the list control is refreshed as well
    ++m_blockEvents;
    CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hNode));
    if (!pTreeItem || pTreeItem->m_svnParentPathRoot)
    {
        --m_blockEvents;
        return false;
    }
    HTREEITEM hSel1 = m_repoTree.GetSelectedItem();
    if (m_repoTree.ItemHasChildren(hNode))
    {
        HTREEITEM hChild = m_repoTree.GetChildItem(hNode);

        CAutoWriteLock writeLocker(m_guard);
        while (hChild)
        {
            HTREEITEM hNext = m_repoTree.GetNextItem(hChild, TVGN_NEXT);
            RecursiveRemove(hChild);
            m_repoTree.DeleteItem(hChild);
            hChild = hNext;
        }
    }

    RefreshChildren(hNode);

    if ((pTreeItem->m_hasChildFolders) || (m_bSparseCheckoutMode))
        AutoInsert(hNode, pTreeItem->m_children);

    // if there are no child folders, remove the '+' in front of the node
    {
        TVITEM tvItem    = {0};
        tvItem.hItem     = hNode;
        tvItem.mask      = TVIF_CHILDREN;
        tvItem.cChildren = pTreeItem->m_hasChildFolders || !pTreeItem->m_childrenFetched || (m_bSparseCheckoutMode && pTreeItem->m_children.size()) ? 1 : 0;
        m_repoTree.SetItem(&tvItem);
    }
    if (pTreeItem->m_childrenFetched && pTreeItem->m_error.IsEmpty())
        if ((force) || (hSel1 == hNode) || (hSel1 != m_repoTree.GetSelectedItem()) || (m_pListCtrlTreeItem == nullptr))
            FillList(pTreeItem);
    --m_blockEvents;
    return true;
}

BOOL CRepositoryBrowser::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
    {
        // Check if there is an in place Edit active:
        // in place edits are done with an edit control, where the parent
        // is the control with the editable item (tree or list control here)
        HWND hWndFocus = ::GetFocus();
        if (hWndFocus)
            hWndFocus = ::GetParent(hWndFocus);
        if (hWndFocus && ((hWndFocus == m_repoTree.GetSafeHwnd()) || (hWndFocus == m_repoList.GetSafeHwnd())))
        {
            if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE) && (hWndFocus == m_repoTree.GetSafeHwnd()))
            {
                // ending editing on the tree control does not seem to work properly:
                // while editing ends when the escape key is pressed, the key message
                // is passed on which then also cancels the whole repobrowser dialog.
                // To prevent this, end editing here directly and prevent the escape key message
                // from being passed on.
                m_repoTree.EndEditLabelNow(true);
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
                        if ((pMsg->hwnd == m_barRepository.GetSafeHwnd()) || (::IsChild(m_barRepository.GetSafeHwnd(), pMsg->hwnd)))
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

    CTSVNPathList                urlList;
    std::vector<SRepositoryInfo> repositories;
    bool                         bTreeItem = false;

    {
        CAutoReadLock locker(m_guard);
        POSITION      pos = m_repoList.GetFirstSelectedItemPosition();
        if (pos == nullptr)
            return;
        int index = -1;
        while ((index = m_repoList.GetNextSelectedItem(pos)) >= 0)
        {
            CItem*  pItem   = reinterpret_cast<CItem*>(m_repoList.GetItemData(index));
            CString absPath = pItem->m_absolutePath;
            absPath.Replace(L"\\", L"%5C");
            urlList.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(absPath))));
            repositories.push_back(pItem->m_repository);
        }
        if ((urlList.GetCount() == 0))
        {
            HTREEITEM  hItem     = m_repoTree.GetSelectedItem();
            CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hItem));
            if (pTreeItem)
            {
                if (pTreeItem->m_repository.root.Compare(pTreeItem->m_url) == 0)
                    return; // can't delete the repository root!

                urlList.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(pTreeItem->m_url))));
                repositories.push_back(pTreeItem->m_repository);
                bTreeItem = true;
            }
        }
    }

    if (urlList.GetCount() == 0)
        return;

    CString sLogMsg;
    if (!RunStartCommit(urlList, sLogMsg))
        return;
    CWaitCursorEx waitCursor;
    CInputLogDlg  input(this);
    input.SetPathList(urlList);
    input.SetRootPath(CTSVNPath(m_initialUrl));
    input.SetLogText(sLogMsg);
    input.SetUUID(repositories[0].uuid);
    input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEDEL);
    CString hint;
    if (urlList.GetCount() == 1)
        hint.Format(IDS_INPUT_REMOVEONE, static_cast<LPCWSTR>(urlList[0].GetFileOrDirectoryName()));
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
        if (!bRet || !m_postCommitErr.IsEmpty())
        {
            waitCursor.Hide();
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
            RefreshNode(m_repoTree.GetSelectedItem(), true);
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
    CString  url;
    POSITION pos = m_repoList.GetFirstSelectedItemPosition();
    if (pos == nullptr)
        return;
    int           index = -1;
    CAutoReadLock locker(m_guard);
    while ((index = m_repoList.GetNextSelectedItem(pos)) >= 0)
    {
        CItem* pItem = reinterpret_cast<CItem*>(m_repoList.GetItemData(index));
        url += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(pItem->m_absolutePath)));
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
    if (GetFocus() == &m_repoList)
    {
        POSITION pos = m_repoList.GetFirstSelectedItemPosition();
        if (pos == nullptr)
            return;
        int           selIndex = m_repoList.GetNextSelectedItem(pos);
        CAutoReadLock locker(m_guard);
        CItem*        pItem = reinterpret_cast<CItem*>(m_repoList.GetItemData(selIndex));
        if (!pItem->m_isExternal)
        {
            m_repoList.SetFocus();
            m_repoList.EditLabel(selIndex);
        }
    }
    else
    {
        CAutoReadLock locker(m_guard);
        m_repoTree.SetFocus();
        HTREEITEM hTreeItem = m_repoTree.GetSelectedItem();
        if (hTreeItem != m_repoTree.GetRootItem())
        {
            CTreeItem* pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hTreeItem));
            if (!pItem->m_isExternal && !pItem->m_bookmark && !pItem->m_dummy)
                m_repoTree.EditLabel(hTreeItem);
        }
    }
}

void CRepositoryBrowser::OnRefresh()
{
    CWaitCursorEx waitCursor;

    ++m_blockEvents;

    // try to get the tree node to refresh

    HTREEITEM hSelected = m_repoTree.GetSelectedItem();
    if (hSelected == nullptr)
        hSelected = m_repoTree.GetRootItem();

    if (hSelected == nullptr)
    {
        --m_blockEvents;
        if (m_initialUrl.IsEmpty())
            return;
        // try re-init
        InitRepo();
        return;
    }

    CAutoReadLock locker(m_guard);
    // invalidate the cache

    InvalidateData(hSelected);

    // refresh the current node

    RefreshNode(m_repoTree.GetSelectedItem(), true);

    --m_blockEvents;
}

void CRepositoryBrowser::OnTvnSelchangedRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    if (m_blockEvents)
        return;

    if (m_bSparseCheckoutMode)
        return;

    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    if (pNMTreeView->action == TVC_BYKEYBOARD)
        SetTimer(REPOBROWSER_FETCHTIMER, 300, nullptr);
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
        HTREEITEM     hSelItem = m_repoTree.GetSelectedItem();
        if (hSelItem)
        {
            CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSelItem));
            if (pTreeItem && !pTreeItem->m_dummy)
            {
                if (pTreeItem->m_bookmark)
                {
                    if (!pTreeItem->m_url.IsEmpty())
                    {
                        SVNRev  r   = SVNRev::REV_HEAD;
                        CString url = pTreeItem->m_url;
                        ChangeToUrl(url, r, false);
                        m_barRepository.ShowUrl(url, r);
                    }
                }
                else
                {
                    if (!pTreeItem->m_childrenFetched && pTreeItem->m_error.IsEmpty())
                    {
                        CAutoWriteLock writelocker(m_guard);
                        m_repoList.DeleteAllItems();
                        FetchChildren(hSelItem);
                    }

                    FillList(pTreeItem);

                    m_barRepository.ShowUrl(pTreeItem->m_url, pTreeItem->m_repository.revision);
                }
            }
        }
    }

    __super::OnTimer(nIDEvent);
}

void CRepositoryBrowser::OnTvnItemexpandingRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    if (m_blockEvents)
        return;

    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

    CAutoReadLock locker(m_guard);
    CTreeItem*    pTreeItem = reinterpret_cast<CTreeItem*>(pNMTreeView->itemNew.lParam);

    if (pTreeItem == nullptr)
        return;
    if (pTreeItem->m_bookmark)
        return;
    if (pTreeItem->m_dummy)
        return;

    if (pNMTreeView->action == TVE_COLLAPSE)
    {
        // user wants to collapse a tree node.
        // if we don't know anything about the children
        // of the node, we suppress the collapsing but fetch the info instead
        if (!pTreeItem->m_childrenFetched)
        {
            FetchChildren(pNMTreeView->itemNew.hItem);
            *pResult = 1;
            return;
        }
        return;
    }

    // user wants to expand a tree node.
    // check if we already know its children - if not we have to ask the repository!

    if (!pTreeItem->m_childrenFetched)
    {
        FetchChildren(pNMTreeView->itemNew.hItem);
    }
    else
    {
        // if there are no child folders, remove the '+' in front of the node

        if (!pTreeItem->m_hasChildFolders && (!m_bSparseCheckoutMode && (pTreeItem->m_children.empty())))
        {
            TVITEM tvitem    = {0};
            tvitem.hItem     = pNMTreeView->itemNew.hItem;
            tvitem.mask      = TVIF_CHILDREN;
            tvitem.cChildren = 0;
            m_repoTree.SetItem(&tvitem);
        }
    }
}

void CRepositoryBrowser::OnNMDblclkRepolist(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    if (m_blockEvents)
        return;

    LPNMITEMACTIVATE pNmItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    if (pNmItemActivate->iItem < 0)
        return;

    CWaitCursorEx wait;
    OpenFromList(pNmItemActivate->iItem);
}

void CRepositoryBrowser::OpenFromList(int item)
{
    if (item < 0)
        return;

    CAutoReadLock locker(m_guard);
    CItem*        pItem = reinterpret_cast<CItem*>(m_repoList.GetItemData(item));
    if (pItem == nullptr)
        return;

    CWaitCursorEx wait;

    if ((pItem->m_kind == svn_node_dir) || m_bSparseCheckoutMode)
    {
        // a double click on a folder results in selecting that folder

        HTREEITEM hItem = AutoInsert(m_repoTree.GetSelectedItem(), *pItem);

        m_repoTree.EnsureVisible(hItem);
        m_repoTree.SelectItem(hItem);
    }
    else if (pItem->m_kind == svn_node_file)
    {
        CRepositoryBrowserSelection selection;
        selection.Add(pItem);
        OpenFile(selection.GetURL(0, 0), selection.GetURLEscaped(0, 0), false);
    }
}

void CRepositoryBrowser::OpenFile(const CTSVNPath& url, const CTSVNPath& urlEscaped, bool bOpenWith)
{
    const SVNRev& revision = GetRevision();
    CTSVNPath     tempfile = CTempFiles::Instance().GetTempFilePath(false, url, revision);
    CWaitCursorEx wait_cursor;
    CProgressDlg  progDlg;
    progDlg.SetTitle(IDS_APPNAME);
    CString sInfoLine;
    sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, static_cast<LPCWSTR>(url.GetFileOrDirectoryName()), static_cast<LPCWSTR>(revision.ToString()));
    progDlg.SetLine(1, sInfoLine, true);
    SetAndClearProgressInfo(&progDlg);
    progDlg.ShowModeless(m_hWnd);
    if (!Export(urlEscaped, tempfile, revision, revision))
    {
        progDlg.Stop();
        SetAndClearProgressInfo(static_cast<HWND>(nullptr));
        wait_cursor.Hide();
        ShowErrorDialog(m_hWnd);
        return;
    }
    progDlg.Stop();
    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
    // set the file as read-only to tell the app which opens the file that it's only
    // a temporary file and must not be edited.
    SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    if (bOpenWith)
    {
        OPENASINFO oi  = {nullptr};
        oi.pcszFile    = tempfile.GetWinPath();
        oi.oaifInFlags = OAIF_EXEC;
        SHOpenWithDialog(GetSafeHwnd(), &oi);
    }
    else
    {
        ShellExecute(nullptr, L"open", tempfile.GetWinPathString(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void CRepositoryBrowser::EditFile(CTSVNPath url, CTSVNPath urlEscaped)
{
    SVNRev         revision    = GetRevision();
    CString        paramString = L"/closeonend:1 /closeforlocal /hideprogress /revision:";
    CCmdLineParser parameters(paramString + revision.ToString());

    m_editFileCommand = std::make_unique<EditFileCommand>();
    m_editFileCommand->SetPaths(CTSVNPathList(url), url);
    m_editFileCommand->SetParser(parameters);

    CoInitialize(nullptr);
    OnOutOfScope(CoUninitialize());

    if (m_editFileCommand->Execute() && revision.IsHead())
    {
        CString dir = urlEscaped.GetContainingDirectory().GetSVNPathString();
        m_lister.RefreshSubTree(revision, dir);

        // Memory will be allocated by malloc() - call free() once the message has been handled
        const wchar_t* lParam = _wcsdup(static_cast<LPCWSTR>(dir));
        PostMessage(WM_REFRESHURL, 0, reinterpret_cast<LPARAM>(lParam));
    }
}

void CRepositoryBrowser::OnHdnItemclickRepolist(NMHDR* pNMHDR, LRESULT* pResult)
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
    ListView_SortItemsEx(m_repoList, ListSort, this);
    SetSortArrow();
    --m_blockEvents;
    *pResult = 0;
}

int CRepositoryBrowser::ListSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3)
{
    CRepositoryBrowser* pThis  = reinterpret_cast<CRepositoryBrowser*>(lParam3);
    CItem*              pItem1 = reinterpret_cast<CItem*>(pThis->m_repoList.GetItemData(static_cast<int>(lParam1)));
    CItem*              pItem2 = reinterpret_cast<CItem*>(pThis->m_repoList.GetItemData(static_cast<int>(lParam2)));
    int                 nRet   = 0;
    switch (pThis->m_nSortedColumn)
    {
        case 0: // filename
            nRet = SortStrCmp(pItem1->m_path, pItem2->m_path);
            if (nRet != 0)
                break;
            [[fallthrough]];
        case 1: // extension
            nRet = pThis->m_repoList.GetItemText(static_cast<int>(lParam1), 1)
                       .CompareNoCase(pThis->m_repoList.GetItemText(static_cast<int>(lParam2), 1));
            if (nRet == 0) // if extensions are the same, use the filename to sort
                nRet = SortStrCmp(pItem1->m_path, pItem2->m_path);
            if (nRet != 0)
                break;
            [[fallthrough]];
        case 2: // revision number
            nRet = pItem1->m_createdRev - pItem2->m_createdRev;
            if (nRet == 0) // if extensions are the same, use the filename to sort
                nRet = SortStrCmp(pItem1->m_path, pItem2->m_path);
            if (nRet != 0)
                break;
            [[fallthrough]];
        case 3: // author
            nRet = pItem1->m_author.CompareNoCase(pItem2->m_author);
            if (nRet == 0) // if extensions are the same, use the filename to sort
                nRet = SortStrCmp(pItem1->m_path, pItem2->m_path);
            if (nRet != 0)
                break;
            [[fallthrough]];
        case 4: // size
            nRet = static_cast<int>(pItem1->m_size - pItem2->m_size);
            if (nRet == 0) // if extensions are the same, use the filename to sort
                nRet = SortStrCmp(pItem1->m_path, pItem2->m_path);
            if (nRet != 0)
                break;
            [[fallthrough]];
        case 5: // date
            nRet = (pItem1->m_time - pItem2->m_time) > 0 ? 1 : -1;
            break;
        case 6: // lock owner
            nRet = pItem1->m_lockOwner.CompareNoCase(pItem2->m_lockOwner);
            if (nRet == 0) // if extensions are the same, use the filename to sort
                nRet = SortStrCmp(pItem1->m_path, pItem2->m_path);
            break;
    }

    if (!pThis->m_bSortAscending)
        nRet = -nRet;

    // we want folders on top, then the files
    if (pItem1->m_kind != pItem2->m_kind)
    {
        if (pItem1->m_kind == svn_node_dir)
            nRet = -1;
        else
            nRet = 1;
    }

    return nRet;
}

int CRepositoryBrowser::TreeSort(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParam3*/)
{
    CTreeItem* item1 = reinterpret_cast<CTreeItem*>(lParam1);
    CTreeItem* item2 = reinterpret_cast<CTreeItem*>(lParam2);

    // directories are always above the other nodes
    if (item1->m_kind == svn_node_dir && item2->m_kind != svn_node_dir)
        return -1;

    if (item1->m_kind != svn_node_dir && item2->m_kind == svn_node_dir)
        return 1;

    return SortStrCmp(item1->m_unescapedName, item2->m_unescapedName);
}

void CRepositoryBrowser::SetSortArrow() const
{
    CHeaderCtrl* pHeader    = m_repoList.GetHeaderCtrl();
    HDITEM       headerItem = {0};
    headerItem.mask         = HDI_FORMAT;
    for (int i = 0; i < pHeader->GetItemCount(); ++i)
    {
        pHeader->GetItem(i, &headerItem);
        headerItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        pHeader->SetItem(i, &headerItem);
    }

    pHeader->GetItem(m_nSortedColumn, &headerItem);
    headerItem.fmt |= (m_bSortAscending ? HDF_SORTUP : HDF_SORTDOWN);
    pHeader->SetItem(m_nSortedColumn, &headerItem);
}

void CRepositoryBrowser::OnLvnItemchangedRepolist(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    if (m_blockEvents)
        return;
    if (m_repoList.HasText())
        return;
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if (pNMLV->uChanged & LVIF_STATE)
    {
        if ((pNMLV->uNewState & LVIS_SELECTED) &&
            (pNMLV->iItem >= 0) &&
            (pNMLV->iItem < m_repoList.GetItemCount()))
        {
            CAutoReadLock locker(m_guard);
            CItem*        pItem = reinterpret_cast<CItem*>(m_repoList.GetItemData(pNMLV->iItem));
            if (pItem)
            {
                CString temp;

                if (pItem->m_isExternal)
                {
                    temp.FormatMessage(IDS_REPOBROWSE_INFOEXT,
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 0)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 2)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 5)));
                }
                else if (pItem->m_kind == svn_node_dir)
                {
                    temp.FormatMessage(IDS_REPOBROWSE_INFODIR,
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 0)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 2)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 3)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 5)));
                }
                else
                {
                    temp.FormatMessage(IDS_REPOBROWSE_INFOFILE,
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 0)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 2)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 3)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 4)),
                                       static_cast<LPCWSTR>(m_repoList.GetItemText(pNMLV->iItem, 5)));
                }
                SetDlgItemText(IDC_INFOLABEL, temp);
            }
            else
                SetDlgItemText(IDC_INFOLABEL, L"");
        }
    }
}

void CRepositoryBrowser::OnLvnBeginlabeleditRepolist(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVDISPINFO* info = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

    CAutoReadLock locker(m_guard);
    // disable rename for externals and the root
    CItem* item = reinterpret_cast<CItem*>(m_repoList.GetItemData(info->item.iItem));
    *pResult    = (item == nullptr) || (item->m_isExternal) || (item->m_absolutePath.Compare(GetRepoRoot()) == 0)
                      ? TRUE
                      : FALSE;
}

void CRepositoryBrowser::OnLvnEndlabeleditRepolist(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult                = 0;
    NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    if (pDispInfo->item.pszText == nullptr)
        return;

    CTSVNPath targetUrl;
    CString   uuid;
    CString   absolutepath;
    {
        CAutoReadLock locker(m_guard);
        // rename the item in the repository
        CItem* pItem = reinterpret_cast<CItem*>(m_repoList.GetItemData(pDispInfo->item.iItem));

        targetUrl = CTSVNPath(EscapeUrl(CTSVNPath(pItem->m_absolutePath.Left(pItem->m_absolutePath.ReverseFind('/') + 1) + pDispInfo->item.pszText)));
        if (!CheckAndConfirmPath(targetUrl))
            return;
        uuid         = pItem->m_repository.uuid;
        absolutepath = pItem->m_absolutePath;
    }

    CString       sLogMsg;
    CTSVNPathList plist = CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(absolutepath))));
    if (!RunStartCommit(plist, sLogMsg))
        return;
    CInputLogDlg input(this);
    input.SetPathList(plist);
    input.SetRootPath(CTSVNPath(m_initialUrl));
    input.SetLogText(sLogMsg);
    input.SetUUID(uuid);
    input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEMOVE);
    CString sHint;
    sHint.FormatMessage(IDS_INPUT_RENAME, static_cast<LPCWSTR>(absolutepath), static_cast<LPCWSTR>(targetUrl.GetUIPathString()));
    input.SetActionText(sHint);
    //input.SetForceFocus (true);
    if (input.DoModal() == IDOK)
    {
        sLogMsg = input.GetLogMessage();
        if (!RunPreCommit(plist, svn_depth_unknown, sLogMsg))
            return;
        CWaitCursorEx waitCursor;

        bool bRet = Move(plist,
                         targetUrl,
                         sLogMsg,
                         false, false, false, false,
                         input.m_revProps);
        RunPostCommit(plist, svn_depth_unknown, m_commitRev, sLogMsg);
        if (!bRet || !m_postCommitErr.IsEmpty())
        {
            waitCursor.Hide();
            ShowErrorDialog(m_hWnd);
            if (!bRet)
                return;
        }
        *pResult = 0;

        InvalidateData(m_repoTree.GetSelectedItem());
        RefreshNode(m_repoTree.GetSelectedItem(), true);
    }
}

void CRepositoryBrowser::OnTvnBeginlabeleditRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
    if (m_bSparseCheckoutMode)
    {
        *pResult = TRUE;
        return;
    }

    NMTVDISPINFO* info = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
    // disable rename for externals
    CAutoReadLock locker(m_guard);
    CTreeItem*    item = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(info->item.hItem));
    *pResult           = (item == nullptr) || (item->m_isExternal) || (item->m_url.Compare(GetRepoRoot()) == 0)
                             ? TRUE
                             : FALSE;
}

void CRepositoryBrowser::OnTvnEndlabeleditRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    if (m_bSparseCheckoutMode)
        return;

    LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
    if (pTVDispInfo->item.pszText == nullptr)
        return;

    // rename the item in the repository
    HTREEITEM  hSelectedItem = pTVDispInfo->item.hItem;
    CTreeItem* pItem         = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSelectedItem));
    if (pItem == nullptr)
        return;

    SRepositoryInfo repository = pItem->m_repository;

    CTSVNPath targetUrl = CTSVNPath(EscapeUrl(CTSVNPath(pItem->m_url.Left(pItem->m_url.ReverseFind('/') + 1) + pTVDispInfo->item.pszText)));
    if (!CheckAndConfirmPath(targetUrl))
        return;
    CTSVNPathList plist = CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(pItem->m_url))));
    CString       sLogMsg;
    if (!RunStartCommit(plist, sLogMsg))
        return;
    CInputLogDlg input(this);
    input.SetPathList(plist);
    input.SetRootPath(CTSVNPath(m_initialUrl));
    input.SetLogText(sLogMsg);
    input.SetUUID(pItem->m_repository.uuid);
    input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEMOVE);
    CString sHint;
    sHint.FormatMessage(IDS_INPUT_RENAME, static_cast<LPCWSTR>(pItem->m_url), static_cast<LPCWSTR>(targetUrl.GetUIPathString()));
    input.SetActionText(sHint);
    //input.SetForceFocus (true);
    if (input.DoModal() == IDOK)
    {
        sLogMsg = input.GetLogMessage();
        if (!RunPreCommit(plist, svn_depth_unknown, sLogMsg))
            return;
        CWaitCursorEx waitCursor;

        bool bRet = Move(plist,
                         targetUrl,
                         sLogMsg,
                         false, false, false, false,
                         input.m_revProps);
        RunPostCommit(plist, svn_depth_unknown, m_commitRev, sLogMsg);
        if (!bRet || !m_postCommitErr.IsEmpty())
        {
            waitCursor.Hide();
            ShowErrorDialog(m_hWnd);
            if (!bRet)
                return;
        }

        HTREEITEM parent = m_repoTree.GetParentItem(hSelectedItem);
        InvalidateData(parent);

        *pResult = TRUE;
        if (pItem->m_url.Compare(m_barRepository.GetCurrentUrl()) == 0)
        {
            m_barRepository.ShowUrl(targetUrl.GetSVNPathString(), m_barRepository.GetCurrentRev());
        }

        pItem->m_url           = targetUrl.GetUIPathString();
        pItem->m_unescapedName = pTVDispInfo->item.pszText;
        pItem->m_repository    = repository;
        m_repoTree.SetItemData(hSelectedItem, reinterpret_cast<DWORD_PTR>(pItem));

        RefreshChildren(parent);

        if (hSelectedItem == m_repoTree.GetSelectedItem())
            RefreshNode(hSelectedItem, true);
    }
}

void CRepositoryBrowser::OnCbenDragbeginUrlcombo(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // build copy source / content
    std::unique_ptr<CIDropSource> pdSrc(new CIDropSource);
    if (pdSrc == nullptr)
        return;

    pdSrc->AddRef();

    const SVNRev&  revision = GetRevision();
    SVNDataObject* pdObj    = new SVNDataObject(CTSVNPathList(CTSVNPath(GetPath())), revision, revision, true);
    if (pdObj == nullptr)
    {
        return;
    }
    pdObj->AddRef();
    pdObj->SetAsyncMode(TRUE);
    CDragSourceHelper dragSrcHelper;
    POINT             point;
    GetCursorPos(&point);
    dragSrcHelper.InitializeFromWindow(m_barRepository.GetComboWindow(), point, pdObj);
    pdSrc->m_pIDataObj = pdObj;
    pdSrc->m_pIDataObj->AddRef();

    // Initiate the Drag & Drop
    DWORD dwEffect;
    ::DoDragDrop(pdObj, pdSrc.get(), DROPEFFECT_LINK, &dwEffect);
    pdSrc->Release();
    pdSrc.release();
    pdObj->Release();

    *pResult = 0;
}

void CRepositoryBrowser::OnLvnBeginrdragRepolist(NMHDR* pNMHDR, LRESULT* pResult)
{
    OnLvnBegindragRepolist(pNMHDR, pResult);
}

void CRepositoryBrowser::OnLvnBegindragRepolist(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    OnBeginDrag(pNMHDR);
}

void CRepositoryBrowser::OnBeginDrag(NMHDR* pNMHDR)
{
    if (m_repoList.HasText())
        return;

    // get selected paths

    CRepositoryBrowserSelection selection;

    CAutoReadLock locker(m_guard);
    POSITION      pos = m_repoList.GetFirstSelectedItemPosition();
    if (pos == nullptr)
        return;
    int index = -1;
    while ((index = m_repoList.GetNextSelectedItem(pos)) >= 0)
        selection.Add(reinterpret_cast<CItem*>(m_repoList.GetItemData(index)));

    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    BeginDrag(m_repoList, selection, pNMLV->ptAction, true);
}

void CRepositoryBrowser::OnTvnBegindragRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    if (m_bSparseCheckoutMode)
        return;
    OnBeginDragTree(pNMHDR);
}

void CRepositoryBrowser::OnTvnBeginrdragRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
    OnTvnBegindragRepotree(pNMHDR, pResult);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRepositoryBrowser::OnBeginDragTree(NMHDR* pNMHDR)
{
    if (m_blockEvents)
        return;

    // get selected paths

    CRepositoryBrowserSelection selection;
    LPNMTREEVIEW                pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    CTreeItem*                  pItem       = reinterpret_cast<CTreeItem*>(pNMTreeView->itemNew.lParam);
    if (pItem && (pItem->m_bookmark || pItem->m_dummy))
        return;

    selection.Add(pItem);

    BeginDrag(m_repoTree, selection, pNMTreeView->ptDrag, false);
}

bool CRepositoryBrowser::OnDrop(const CTSVNPath& target, const CString& root, const CTSVNPathList& pathlist, const SVNRev& srcRev, DWORD dwEffect, POINTL /*pt*/)
{
    if (m_bSparseCheckoutMode)
        return false;
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": dropped %ld items on %s, source revision is %s, dwEffect is %ld\n", pathlist.GetCount(), static_cast<LPCWSTR>(target.GetSVNPathString()), static_cast<LPCWSTR>(srcRev.ToString()), dwEffect);
    if (pathlist.GetCount() == 0)
        return false;

    if (!CTSVNPath(root).IsAncestorOf(pathlist[0]) && pathlist[0].IsUrl())
        return false;

    CString targetName = pathlist[0].GetFileOrDirectoryName();
    bool    sameParent = pathlist[0].GetContainingDirectory() == target;

    if (m_bRightDrag)
    {
        // right dragging means we have to show a context menu
        POINT pt;
        DWORD ptW = GetMessagePos();
        pt.x      = GET_X_LPARAM(ptW);
        pt.y      = GET_Y_LPARAM(ptW);
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

            if ((pathlist.GetCount() == 1) && (PathIsURL(pathlist[0])))
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
            int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, this, nullptr);
            switch (cmd)
            {
                default: // nothing clicked
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
                    if (!CheckAndConfirmPath(CTSVNPath(targetName)))
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
                    if (!CheckAndConfirmPath(CTSVNPath(targetName)))
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
            int  pathCount         = pathlist.GetCount();
            for (int i = 0; i < pathCount; ++i)
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
                CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY_TASK1)),
                                    CString(MAKEINTRESOURCE(IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY_TASK2)),
                                    L"TortoiseSVN",
                                    0,
                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY_TASK3)));
                taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY_TASK4)));
                taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskDlg.SetDefaultCommandControl(2);
                taskDlg.SetMainIcon(TD_WARNING_ICON);
                if (taskDlg.DoModal(m_hWnd) != 100)
                    return false;
            }
        }

        // drag-n-drop inside the repobrowser
        CString sLogMsg;
        if (!RunStartCommit(pathlist, sLogMsg))
            return false;
        CInputLogDlg input(this);
        input.SetPathList(pathlist);
        input.SetRootPath(CTSVNPath(m_initialUrl));
        input.SetLogText(sLogMsg);
        input.SetUUID(m_repository.uuid);
        input.SetProjectProperties(&m_projectProperties, dwEffect == DROPEFFECT_COPY ? PROJECTPROPNAME_LOGTEMPLATEBRANCH : PROJECTPROPNAME_LOGTEMPLATEMOVE);
        CString sHint;
        if (pathlist.GetCount() == 1)
        {
            if (dwEffect == DROPEFFECT_COPY)
                sHint.FormatMessage(IDS_INPUT_COPY, static_cast<LPCWSTR>(pathlist[0].GetSVNPathString()), static_cast<LPCWSTR>(target.GetSVNPathString() + L"/" + targetName));
            else
                sHint.FormatMessage(IDS_INPUT_MOVE, static_cast<LPCWSTR>(pathlist[0].GetSVNPathString()), static_cast<LPCWSTR>(target.GetSVNPathString() + L"/" + targetName));
        }
        else
        {
            if (dwEffect == DROPEFFECT_COPY)
                sHint.FormatMessage(IDS_INPUT_COPYMORE, pathlist.GetCount(), static_cast<LPCWSTR>(target.GetSVNPathString()));
            else
                sHint.FormatMessage(IDS_INPUT_MOVEMORE, pathlist.GetCount(), static_cast<LPCWSTR>(target.GetSVNPathString()));
        }
        input.SetActionText(sHint);
        if (input.DoModal() == IDOK)
        {
            sLogMsg = input.GetLogMessage();
            if (!RunPreCommit(pathlist, svn_depth_unknown, sLogMsg))
                return false;
            CWaitCursorEx waitCursor;
            BOOL          bRet = FALSE;
            if (dwEffect == DROPEFFECT_COPY)
                if (pathlist.GetCount() == 1)
                    bRet = Copy(pathlist, CTSVNPath(target.GetSVNPathString() + L"/" + targetName), srcRev, srcRev, sLogMsg, false, false, false, false, SVNExternals(), input.m_revProps);
                else
                    bRet = Copy(pathlist, target, srcRev, srcRev, sLogMsg, true, false, false, false, SVNExternals(), input.m_revProps);
            else if (pathlist.GetCount() == 1)
                bRet = Move(pathlist, CTSVNPath(target.GetSVNPathString() + L"/" + targetName), sLogMsg, false, false, false, false, input.m_revProps);
            else
                bRet = Move(pathlist, target, sLogMsg, true, false, false, false, input.m_revProps);
            RunPostCommit(pathlist, svn_depth_unknown, m_commitRev, sLogMsg);
            if (!bRet || !m_postCommitErr.IsEmpty())
            {
                waitCursor.Hide();
                ShowErrorDialog(m_hWnd);
            }
            else if (GetRevision().IsHead())
            {
                // mark the target as dirty
                HTREEITEM hTarget = FindUrl(target.GetSVNPathString());
                InvalidateData(hTarget);
                if (hTarget)
                {
                    CAutoWriteLock locker(m_guard);
                    CTreeItem*     pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hTarget));
                    if (pItem)
                    {
                        // mark the target as 'dirty'
                        pItem->m_childrenFetched = false;
                        pItem->m_error.Empty();
                        RecursiveRemove(hTarget, true);
                        TVITEM tvItem    = {0};
                        tvItem.hItem     = hTarget;
                        tvItem.mask      = TVIF_CHILDREN;
                        tvItem.cChildren = 1;
                        m_repoTree.SetItem(&tvItem);
                    }
                }
                if (dwEffect == DROPEFFECT_MOVE)
                {
                    // if items were moved, we have to
                    // invalidate all sources too
                    for (int i = 0; i < pathlist.GetCount(); ++i)
                    {
                        HTREEITEM hSource = FindUrl(pathlist[i].GetSVNPathString());
                        InvalidateData(hSource);
                        if (hSource)
                        {
                            CAutoWriteLock locker(m_guard);
                            CTreeItem*     pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSource));
                            if (pItem)
                            {
                                // the source has moved, so remove it!
                                RecursiveRemove(hSource);
                                m_repoTree.DeleteItem(hSource);
                            }
                        }
                    }
                }

                // if the copy/move operation was to the currently shown url,
                // update the current view. Otherwise mark the target URL as 'not fetched'.
                HTREEITEM hSelected = m_repoTree.GetSelectedItem();
                InvalidateData(hSelected);

                if (hSelected)
                {
                    CAutoWriteLock locker(m_guard);
                    CTreeItem*     pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSelected));
                    if (pItem)
                    {
                        // mark the target as 'dirty'
                        pItem->m_childrenFetched = false;
                        pItem->m_error.Empty();
                        if ((dwEffect == DROPEFFECT_MOVE) || (pItem->m_url.Compare(target.GetSVNPathString()) == 0))
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
            CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_MULTIIMPORT_TASK1)),
                                CString(MAKEINTRESOURCE(IDS_REPOBROWSE_MULTIIMPORT_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_REPOBROWSE_MULTIIMPORT_TASK3)));
            taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_REPOBROWSE_MULTIIMPORT_TASK4)));
            taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskDlg.SetDefaultCommandControl(2);
            taskDlg.SetMainIcon(TD_WARNING_ICON);
            if (taskDlg.DoModal(m_hWnd) != 100)
                return false;
        }

        CString sLogMsg;
        if (!RunStartCommit(pathlist, sLogMsg))
            return false;
        CInputLogDlg input(this);
        input.SetPathList(pathlist);
        input.SetRootPath(CTSVNPath(m_initialUrl));
        input.SetLogText(sLogMsg);
        input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEIMPORT);
        input.SetUUID(m_repository.root);
        CString sHint;
        if (pathlist.GetCount() == 1)
            sHint.FormatMessage(IDS_INPUT_IMPORTFILEFULL, pathlist[0].GetWinPath(), static_cast<LPCWSTR>(target.GetSVNPathString() + L"/" + pathlist[0].GetFileOrDirectoryName()));
        else
            sHint.Format(IDS_INPUT_IMPORTFILES, pathlist.GetCount());
        input.SetActionText(sHint);

        if (input.DoModal() == IDOK)
        {
            sLogMsg = input.GetLogMessage();
            if (!RunPreCommit(pathlist, svn_depth_unknown, sLogMsg))
                return false;
            for (int importIndex = 0; importIndex < pathlist.GetCount(); ++importIndex)
            {
                CString fileName = pathlist[importIndex].GetFileOrDirectoryName();
                bool    bRet     = Import(pathlist[importIndex],
                                   CTSVNPath(target.GetSVNPathString() + L"/" + fileName),
                                   sLogMsg, &m_projectProperties, svn_depth_infinity, true, true, false, input.m_revProps);
                if (!bRet || !m_postCommitErr.IsEmpty())
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
                HTREEITEM hSelected = m_repoTree.GetSelectedItem();
                InvalidateData(hSelected);
                if (hSelected)
                {
                    CAutoWriteLock locker(m_guard);
                    CTreeItem*     pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSelected));
                    if (pItem)
                    {
                        // don't access outdated query results
                        if (pItem->m_url.Compare(target.GetSVNPathString()) == 0)
                        {
                            // Refresh the current view
                            RefreshNode(hSelected, true);
                        }
                        else
                        {
                            // only mark the target as 'dirty'
                            pItem->m_childrenFetched = false;
                            pItem->m_error.Empty();
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

    HTREEITEM hSelectedTreeItem = nullptr;
    HTREEITEM hChosenTreeItem   = nullptr;
    if ((point.x == -1) && (point.y == -1))
    {
        if (pWnd == &m_repoTree)
        {
            CRect rect;
            m_repoTree.GetItemRect(m_repoTree.GetSelectedItem(), &rect, TRUE);
            m_repoTree.ClientToScreen(&rect);
            point = rect.CenterPoint();
        }
        else
        {
            CRect    rect;
            POSITION pos = m_repoList.GetFirstSelectedItemPosition();
            if (pos)
            {
                m_repoList.GetItemRect(m_repoList.GetNextSelectedItem(pos), &rect, LVIR_LABEL);
                m_repoList.ClientToScreen(&rect);
                point = rect.CenterPoint();
            }
        }
    }

    bool                        bSVNParentPathUrl = false;
    bool                        bIsBookmark       = false;
    CRepositoryBrowserSelection selection;
    if (pWnd == &m_repoList)
    {
        CAutoReadLock locker(m_guard);

        POSITION pos = m_repoList.GetFirstSelectedItemPosition();
        if (pos)
        {
            int index = -1;
            while ((index = m_repoList.GetNextSelectedItem(pos)) >= 0)
                selection.Add(reinterpret_cast<CItem*>(m_repoList.GetItemData(index)));
        }

        if (selection.IsEmpty())
        {
            // Right-click outside any list control items. It may be the background,
            // but it also could be the list control headers.
            CRect hr;
            m_repoList.GetHeaderCtrl()->GetWindowRect(&hr);
            if (!hr.PtInRect(point))
            {
                // Seems to be a right-click on the list view background.
                // Use the currently selected item in the tree view as the source.
                ++m_blockEvents;
                hSelectedTreeItem = m_repoTree.GetSelectedItem();
                if (hSelectedTreeItem)
                {
                    m_repoTree.SetItemState(hSelectedTreeItem, 0, TVIS_SELECTED);
                    --m_blockEvents;
                    m_repoTree.SetItemState(hSelectedTreeItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
                    CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSelectedTreeItem));
                    bSVNParentPathUrl    = pTreeItem->m_svnParentPathRoot;
                    selection.Add(pTreeItem);
                }
                else
                    --m_blockEvents;
            }
        }
    }
    if ((pWnd == &m_repoTree) || selection.IsEmpty())
    {
        CAutoReadLock locker(m_guard);
        UINT          uFlags;
        CPoint        ptTree = point;
        m_repoTree.ScreenToClient(&ptTree);
        HTREEITEM hItem = m_repoTree.HitTest(ptTree, &uFlags);
        // in case the right-clicked item is not the selected one,
        // use the TVIS_DROPHILITED style to indicate on which item
        // the context menu will work on
        if ((hItem) && (uFlags & TVHT_ONITEM) && (hItem != m_repoTree.GetSelectedItem()))
        {
            ++m_blockEvents;
            hSelectedTreeItem = m_repoTree.GetSelectedItem();
            m_repoTree.SetItemState(hSelectedTreeItem, 0, TVIS_SELECTED);
            --m_blockEvents;
            m_repoTree.SetItemState(hItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
        }
        if (hItem)
        {
            hChosenTreeItem      = hItem;
            CTreeItem* pTreeItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hItem));
            bSVNParentPathUrl    = pTreeItem->m_svnParentPathRoot;
            if (pTreeItem->m_dummy)
                return;
            if (pTreeItem->m_bookmark)
            {
                bIsBookmark = true;
                if (pTreeItem->m_url.IsEmpty())
                    return;
            }
            selection.Add(pTreeItem);
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
            if (selection.GetPathCount(0) == 1)
            {
                if (selection.GetFolderCount(0) == 0)
                {
                    // Let "Open" be the very first entry, like in Explorer
                    popup.AppendMenuIcon(ID_OPEN, IDS_REPOBROWSE_OPEN, IDI_OPEN);        // "open"
                    popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN); // "open with..."
                    popup.AppendMenuIcon(ID_EDITFILE, IDS_REPOBROWSE_EDITFILE);          // "edit"
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                }
            }
            if ((selection.GetPathCount(0) == 1) ||
                ((selection.GetPathCount(0) == 2) && (selection.GetFolderCount(0) != 1)))
            {
                popup.AppendMenuIcon(ID_SHOWLOG, IDS_REPOBROWSE_SHOWLOG, IDI_LOG); // "Show Log..."
            }
            if (selection.GetPathCount(0) == 1)
            {
                // the revision graph on the repository root would be empty. We
                // don't show the context menu entry there.
                if (!selection.IsRoot(0, 0))
                {
                    popup.AppendMenuIcon(ID_REVGRAPH, IDS_MENUREVISIONGRAPH, IDI_REVISIONGRAPH); // "Revision graph"
                }
                if (!selection.IsFolder(0, 0))
                {
                    popup.AppendMenuIcon(ID_BLAME, IDS_MENUBLAME, IDI_BLAME); // "Blame..."
                }
                if (!m_projectProperties.sWebViewerRev.IsEmpty())
                {
                    popup.AppendMenuIcon(ID_VIEWREV, IDS_LOG_POPUP_VIEWREV); // "View revision in webviewer"
                }
                if (!m_projectProperties.sWebViewerPathRev.IsEmpty())
                {
                    popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV); // "View revision for path in webviewer"
                }
                if ((!m_projectProperties.sWebViewerPathRev.IsEmpty()) ||
                    (!m_projectProperties.sWebViewerRev.IsEmpty()))
                {
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                }

                // we can export files and folders alike, but for files we have "save as"
                if (selection.IsFolder(0, 0))
                    popup.AppendMenuIcon(ID_EXPORT, IDS_MENUEXPORT, IDI_EXPORT); // "Export"
            }

            // We allow checkout of multiple paths at once (we do that one by one)
            popup.AppendMenuIcon(ID_CHECKOUT, IDS_MENUCHECKOUT, IDI_CHECKOUT); // "Checkout.."
        }

        if ((selection.GetPathCount(0) == 1) && !bIsBookmark)
        {
            if (selection.IsFolder(0, 0))
            {
                popup.AppendMenuIcon(ID_REFRESH, IDS_REPOBROWSE_REFRESH, IDI_REFRESH); // "Refresh"
            }
            popup.AppendMenu(MF_SEPARATOR, NULL);

            if (!bSVNParentPathUrl)
            {
                if (selection.GetRepository(0).revision.IsHead())
                {
                    if (selection.IsFolder(0, 0))
                    {
                        popup.AppendMenuIcon(ID_MKDIR, IDS_REPOBROWSE_MKDIR, IDI_MKDIR);                // "create directory"
                        popup.AppendMenuIcon(ID_IMPORT, IDS_REPOBROWSE_IMPORT, IDI_IMPORT);             // "Add/Import File"
                        popup.AppendMenuIcon(ID_IMPORTFOLDER, IDS_REPOBROWSE_IMPORTFOLDER, IDI_IMPORT); // "Add/Import Folder"
                        popup.AppendMenu(MF_SEPARATOR, NULL);
                    }

                    if (!selection.IsExternal(0, 0) && !selection.IsRoot(0, 0))
                        popup.AppendMenuIcon(ID_RENAME, IDS_REPOBROWSE_RENAME, IDI_RENAME); // "Rename"
                }
                if (selection.IsLocked(0, 0))
                {
                    popup.AppendMenuIcon(ID_BREAKLOCK, IDS_MENU_UNLOCKFORCE, IDI_UNLOCK); // "Break Lock"
                }
            }
            popup.AppendMenuIcon(ID_ADDTOBOOKMARKS, IDS_REPOBROWSE_ADDBOOKMARK, IDI_BOOKMARKS); // "add to bookmarks"
        }

        if (!bSVNParentPathUrl && !bIsBookmark)
        {
            if (selection.GetRepository(0).revision.IsHead() && !selection.IsRoot(0, 0) && !selection.IsExternal(0, 0))
            {
                popup.AppendMenuIcon(ID_DELETE, IDS_REPOBROWSE_DELETE, IDI_DELETE); // "Remove"
            }
            if (selection.GetFolderCount(0) == 0)
            {
                popup.AppendMenuIcon(ID_SAVEAS, IDS_REPOBROWSE_SAVEAS, IDI_SAVEAS); // "Save as..."
            }
        }
        if (clipSubMenu.CreatePopupMenu())
        {
            if (pWnd == &m_repoList)
                clipSubMenu.AppendMenuIcon(ID_FULLTOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_FULL, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_URLTOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_URL, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_URLTOCLIPBOARDREV, IDS_LOG_POPUP_CLIPBOARD_URLREV, IDI_COPYCLIP);
            if (!m_projectProperties.sWebViewerRev.IsEmpty())
                clipSubMenu.AppendMenuIcon(ID_URLTOCLIPBOARDVIEWERREV, IDS_LOG_POPUP_CLIPBOARD_URLVIEWERREV, IDI_COPYCLIP);
            if (!m_projectProperties.sWebViewerPathRev.IsEmpty())
                clipSubMenu.AppendMenuIcon(ID_URLTOCLIPBOARDVIEWERPATHREV, IDS_LOG_POPUP_CLIPBOARD_URLVIEWERPATHREV, IDI_COPYCLIP);
            clipSubMenu.AppendMenuIcon(ID_NAMETOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_FILENAMES, IDI_COPYCLIP);
            if (pWnd == &m_repoList)
            {
                clipSubMenu.AppendMenuIcon(ID_REVISIONTOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_REVS, IDI_COPYCLIP);
                clipSubMenu.AppendMenuIcon(ID_AUTHORTOCLIPBOARD, IDS_LOG_POPUP_CLIPBOARD_AUTHORS, IDI_COPYCLIP);
            }

            CString temp;
            temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
            popup.InsertMenu(static_cast<UINT>(-1), MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(clipSubMenu.m_hMenu), temp);
        }
        if (!bSVNParentPathUrl && !bIsBookmark)
        {
            if ((selection.GetFolderCount(0) == selection.GetPathCount(0)) || (selection.GetFolderCount(0) == 0))
            {
                popup.AppendMenuIcon(ID_COPYTOWC, IDS_REPOBROWSE_COPYTOWC); // "Copy To Working Copy..."
            }

            if (selection.GetPathCount(0) == 1)
            {
                popup.AppendMenuIcon(ID_COPYTO, IDS_REPOBROWSE_COPY, IDI_COPY); // "Copy To..."
                popup.AppendMenu(MF_SEPARATOR, NULL);
                popup.AppendMenuIcon(ID_PROPS, IDS_REPOBROWSE_SHOWPROP, IDI_PROPERTIES); // "Show Properties"
                // Revision properties are not associated to paths
                // so we only show that context menu on the repository root
                if (selection.IsRoot(0, 0))
                {
                    popup.AppendMenuIcon(ID_REVPROPS, IDS_REPOBROWSE_SHOWREVPROP, IDI_PROPERTIES); // "Show Revision Properties"
                }
                if (selection.IsFolder(0, 0))
                {
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                    popup.AppendMenuIcon(ID_PREPAREDIFF, IDS_REPOBROWSE_PREPAREDIFF); // "Mark for comparison"

                    CTSVNPath root(selection.GetRepository(0).root);
                    if ((m_diffKind == svn_node_dir) && !m_diffURL.IsEquivalentTo(selection.GetURL(0, 0)) && root.IsAncestorOf(m_diffURL))
                    {
                        popup.AppendMenuIcon(ID_GNUDIFF, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);                        // "Show differences as unified diff"
                        popup.AppendMenuIcon(ID_DIFF, IDS_REPOBROWSE_SHOWDIFF, IDI_DIFF);                         // "Compare URLs"
                        popup.AppendMenuIcon(ID_DIFF_CONTENTONLY, IDS_REPOBROWSE_SHOWDIFF_CONTENTONLY, IDI_DIFF); // "Compare URLs"
                    }
                }
            }

            if ((selection.GetPathCount(0) == 2) && (selection.GetFolderCount(0) != 1))
            {
                popup.AppendMenu(MF_SEPARATOR, NULL);
                popup.AppendMenuIcon(ID_GNUDIFF, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);                        // "Show differences as unified diff"
                popup.AppendMenuIcon(ID_DIFF, IDS_REPOBROWSE_SHOWDIFF, ID_DIFF);                          // "Compare URLs"
                popup.AppendMenuIcon(ID_DIFF_CONTENTONLY, IDS_REPOBROWSE_SHOWDIFF_CONTENTONLY, IDI_DIFF); // "Compare URLs"
                popup.AppendMenu(MF_SEPARATOR, NULL);
            }

            if (m_path.Exists() &&
                CTSVNPath(m_initialUrl).IsAncestorOf(selection.GetURL(0, 0)))
            {
                CTSVNPath wcPath = m_path;
                wcPath.AppendPathString(selection.GetURL(0, 0).GetWinPathString().Mid(m_initialUrl.GetLength()));
                if (!wcPath.Exists())
                {
                    bool bWCPresent = false;
                    while (!bWCPresent && m_path.IsAncestorOf(wcPath))
                    {
                        bWCPresent = wcPath.GetContainingDirectory().Exists();
                        wcPath     = wcPath.GetContainingDirectory();
                    }
                    if (bWCPresent)
                    {
                        popup.AppendMenu(MF_SEPARATOR, NULL);
                        popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATEREV, IDI_UPDATE); // "Update item to revision"
                    }
                }
                else
                {
                    popup.AppendMenu(MF_SEPARATOR, NULL);
                    popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATEREV, IDI_UPDATE); // "Update item to revision"
                }
            }
            if (m_path.Exists() && (selection.GetPathCount(0) == 1))
            {
                popup.AppendMenuIcon(ID_SWITCHTO, IDS_REVGRAPH_POPUP_SWITCH, IDI_SWITCH); // Switch WC to path and revision
            }
        }
        popup.AppendMenuIcon(ID_CREATELINK, IDS_REPOBROWSE_CREATELINK, IDI_LINK);
        if (bIsBookmark)
            popup.AppendMenuIcon(ID_REMOVEBOOKMARKS, IDS_REPOBROWSE_REMOVEBOOKMARK, IDI_BOOKMARKS);
        int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, nullptr);

        auto stopJobs(m_lister.SuspendJobs());
        if (pWnd == &m_repoTree)
        {
            CAutoWriteLock locker(m_guard);
            UINT           uFlags;
            CPoint         ptTree = point;
            m_repoTree.ScreenToClient(&ptTree);
            HTREEITEM hItem = m_repoTree.HitTest(ptTree, &uFlags);
            // restore the previously selected item state
            if ((hItem) && (uFlags & TVHT_ONITEM) && (hItem != m_repoTree.GetSelectedItem()))
            {
                ++m_blockEvents;
                m_repoTree.SetItemState(hSelectedTreeItem, TVIS_SELECTED, TVIS_SELECTED);
                --m_blockEvents;
                m_repoTree.SetItemState(hItem, 0, TVIS_DROPHILITED);
            }
        }
        if (hSelectedTreeItem)
        {
            CAutoWriteLock locker(m_guard);
            ++m_blockEvents;
            m_repoTree.SetItemState(hSelectedTreeItem, 0, TVIS_DROPHILITED);
            m_repoTree.SetItemState(hSelectedTreeItem, TVIS_SELECTED, TVIS_SELECTED);
            --m_blockEvents;
        }
        DialogEnableWindow(IDOK, FALSE);
        bool bOpenWith    = false;
        bool bIgnoreProps = false;
        switch (cmd)
        {
            case ID_UPDATE:
            {
                CTSVNPathList updateList;
                for (size_t i = 0; i < selection.GetPathCount(0); ++i)
                {
                    CTSVNPath        wcPath = m_path;
                    const CTSVNPath& url    = selection.GetURL(0, i);
                    wcPath.AppendPathString(url.GetWinPathString().Mid(m_initialUrl.GetLength()));
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
            case ID_SWITCHTO:
            {
                if (selection.GetPathCount(0) == 1)
                {
                    auto          switchToUrl = selection.GetURL(0, 0);
                    const SVNRev& revision    = selection.GetRepository(0).revision;
                    CString       sCmd;
                    sCmd.Format(L"/command:switch /path:\"%s\" /rev:%s /url:\"%s\"",
                                m_path.GetWinPath(), static_cast<LPCWSTR>(revision.ToString()), static_cast<LPCWSTR>(switchToUrl.GetSVNPathString()));

                    CAppUtils::RunTortoiseProc(sCmd);
                }
            }
            break;
            case ID_PREPAREDIFF:
            {
                CAutoWriteLock locker(m_guard);
                m_repoTree.SetItemState(FindUrl(m_diffURL.GetSVNPathString()), 0, TVIS_BOLD);
                if (selection.GetPathCount(0) == 1)
                {
                    if (m_diffURL.IsEquivalentTo(selection.GetURL(0, 0)))
                    {
                        m_diffURL.Reset();
                        m_diffKind = svn_node_none;
                    }
                    else
                    {
                        m_diffURL  = selection.GetURL(0, 0);
                        m_diffKind = selection.IsFolder(0, 0) ? svn_node_dir : svn_node_file;
                        // make the marked tree item bold
                        if (m_diffKind == svn_node_dir)
                        {
                            m_repoTree.SetItemState(FindUrl(m_diffURL.GetSVNPathString()), TVIS_BOLD, TVIS_BOLD);
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
            case ID_URLTOCLIPBOARDREV:
            case ID_URLTOCLIPBOARDVIEWERREV:
            case ID_URLTOCLIPBOARDVIEWERPATHREV:
            case ID_FULLTOCLIPBOARD:
            case ID_NAMETOCLIPBOARD:
            case ID_AUTHORTOCLIPBOARD:
            case ID_REVISIONTOCLIPBOARD:
            {
                CString sClipboard;

                if (pWnd == &m_repoList)
                {
                    POSITION pos = m_repoList.GetFirstSelectedItemPosition();
                    if (pos)
                    {
                        int index = -1;
                        while ((index = m_repoList.GetNextSelectedItem(pos)) >= 0)
                        {
                            CItem* pItem = reinterpret_cast<CItem*>(m_repoList.GetItemData(index));
                            if (cmd == ID_URLTOCLIPBOARDVIEWERREV)
                            {
                                CString webUrl = GetUrlWebViewerRev(selection);
                                if (!webUrl.IsEmpty())
                                    sClipboard += webUrl;
                            }
                            if (cmd == ID_URLTOCLIPBOARDVIEWERPATHREV)
                            {
                                CString webUrl = GetUrlWebViewerPathRev(selection);
                                if (!webUrl.IsEmpty())
                                    sClipboard += webUrl;
                            }
                            if ((cmd == ID_URLTOCLIPBOARD) || (cmd == ID_URLTOCLIPBOARDREV) || (cmd == ID_FULLTOCLIPBOARD))
                            {
                                CString path = pItem->m_absolutePath;
                                path.Replace(L"\\", L"%5C");
                                sClipboard += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(path)));
                                if (cmd == ID_URLTOCLIPBOARDREV)
                                {
                                    sClipboard += L"/?r=" + SVNRev(pItem->m_createdRev).ToString();
                                }
                            }
                            if (cmd == ID_FULLTOCLIPBOARD)
                                sClipboard += L", ";
                            if (cmd == ID_NAMETOCLIPBOARD)
                            {
                                CString u = pItem->m_absolutePath;
                                u.Replace(L"\\", L"%5C");
                                CTSVNPath path = CTSVNPath(u);
                                CString   name = path.GetFileOrDirectoryName();
                                sClipboard += name;
                            }
                            if ((cmd == ID_AUTHORTOCLIPBOARD) || (cmd == ID_FULLTOCLIPBOARD))
                            {
                                sClipboard += pItem->m_author;
                            }
                            if (cmd == ID_FULLTOCLIPBOARD)
                                sClipboard += L", ";
                            if ((cmd == ID_REVISIONTOCLIPBOARD) || (cmd == ID_FULLTOCLIPBOARD))
                            {
                                CString temp;
                                temp.Format(L"%ld", pItem->m_createdRev);
                                sClipboard += temp;
                            }
                            if (cmd == ID_FULLTOCLIPBOARD)
                            {
                                wchar_t dateNative[SVN_DATE_BUFFER] = {0};
                                if ((pItem->m_time == 0) && (pItem->m_isExternal))
                                    dateNative[0] = 0;
                                else
                                    SVN::formatDate(dateNative, static_cast<apr_time_t&>(pItem->m_time), true);

                                CString temp;
                                temp.Format(L", %s", dateNative);
                                sClipboard += temp;
                            }
                            sClipboard += L"\r\n";
                        }
                    }
                }
                else
                {
                    for (size_t i = 0; i < selection.GetPathCount(0); ++i)
                    {
                        if (cmd == ID_URLTOCLIPBOARDVIEWERREV)
                        {
                            CString webUrl = GetUrlWebViewerRev(selection);
                            if (!webUrl.IsEmpty())
                                sClipboard += webUrl;
                        }
                        if (cmd == ID_URLTOCLIPBOARDVIEWERPATHREV)
                        {
                            CString webUrl = GetUrlWebViewerPathRev(selection);
                            if (!webUrl.IsEmpty())
                                sClipboard += webUrl;
                        }
                        if ((cmd == ID_URLTOCLIPBOARD) || (cmd == ID_FULLTOCLIPBOARD) || (cmd == ID_URLTOCLIPBOARDREV))
                        {
                            CString path = selection.GetURL(0, i).GetSVNPathString();
                            sClipboard += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(path)));
                            if (cmd == ID_URLTOCLIPBOARDREV)
                            {
                                sClipboard += L"/?r=" + selection.GetRevision(0, i).ToString();
                            }
                        }
                        if (cmd == ID_NAMETOCLIPBOARD)
                        {
                            CString name = selection.GetURL(0, i).GetFileOrDirectoryName();
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
                CTSVNPath tempFile;
                bool      bSavePathOK = AskForSavePath(selection, tempFile, selection.GetFolderCount(0) > 0);
                if (bSavePathOK)
                {
                    CWaitCursorEx waitCursor;

                    CString       saveUrl;
                    CProgressDlg  progDlg;
                    DWORD         counter   = 0; // the file counter
                    DWORD         pathCount = static_cast<DWORD>(selection.GetPathCount(0));
                    const SVNRev& revision  = selection.GetRepository(0).revision;

                    progDlg.SetTitle(IDS_REPOBROWSE_SAVEASPROGTITLE);
                    progDlg.ShowModeless(GetSafeHwnd());
                    progDlg.SetProgress(0, pathCount);
                    SetAndClearProgressInfo(&progDlg);

                    for (DWORD i = 0; i < pathCount; ++i)
                    {
                        saveUrl            = selection.GetURLEscaped(0, i).GetSVNPathString();
                        CTSVNPath savePath = tempFile;
                        if (tempFile.IsDirectory())
                            savePath.AppendPathString(selection.GetURL(0, i).GetFileOrDirectoryName());
                        CString sInfoLine;
                        sInfoLine.FormatMessage(IDS_PROGRESSGETFILEREVISION, static_cast<LPCWSTR>(saveUrl), static_cast<LPCWSTR>(revision.ToString()));
                        progDlg.SetLine(1, sInfoLine, true);
                        if (!Export(CTSVNPath(saveUrl), savePath, revision, revision) || (progDlg.HasUserCancelled()))
                        {
                            waitCursor.Hide();
                            progDlg.Stop();
                            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            if (!progDlg.HasUserCancelled())
                                ShowErrorDialog(m_hWnd);
                            DialogEnableWindow(IDOK, TRUE);
                            return;
                        }
                        counter++;
                        progDlg.SetProgress(counter, pathCount);
                    }
                    progDlg.Stop();
                    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                }
            }
            break;
            case ID_SHOWLOG:
            {
                const CTSVNPath& firstPath   = selection.GetURLEscaped(0, 0);
                const SVNRev&    pegRevision = selection.GetRepository(0).revision;

                if ((selection.GetPathCount(0) == 2) || (!m_diffURL.IsEmpty() && !m_diffURL.IsEquivalentTo(selection.GetURL(0, 0))))
                {
                    CTSVNPath secondUrl = selection.GetPathCount(0) == 2
                                              ? selection.GetURLEscaped(0, 1)
                                              : m_diffURL;

                    // get last common URL and revision
                    auto commonSource = SVNLogHelper().GetCommonSource(firstPath, pegRevision, secondUrl, pegRevision);
                    if (!commonSource.second.IsNumber())
                    {
                        // no common copy from URL, so showing a log between
                        // the two urls is not possible.
                        TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_NOCOMMONCOPYFROM), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
                        break;
                    }

                    // show log from pegrev to there

                    CString sCmd;
                    sCmd.Format(L"/command:log /path:\"%s\" /startrev:%s /endrev:%s",
                                static_cast<LPCWSTR>(firstPath.GetSVNPathString()),
                                static_cast<LPCWSTR>(pegRevision.ToString()),
                                static_cast<LPCWSTR>(commonSource.second.ToString()));

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
                                static_cast<LPCWSTR>(firstPath.GetSVNPathString()), static_cast<LPCWSTR>(pegRevision.ToString()));

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
                CString webUrl = GetUrlWebViewerRev(selection);
                if (!webUrl.IsEmpty())
                    ShellExecute(this->m_hWnd, L"open", webUrl, nullptr, nullptr, SW_SHOWDEFAULT);
            }
            break;
            case ID_VIEWPATHREV:
            {
                CString webUrl = GetUrlWebViewerPathRev(selection);
                if (!webUrl.IsEmpty())
                    ShellExecute(this->m_hWnd, L"open", webUrl, nullptr, nullptr, SW_SHOWDEFAULT);
            }
            break;
            case ID_CHECKOUT:
            {
                CString itemsToCheckout;
                for (size_t i = 0; i < selection.GetPathCount(0); ++i)
                {
                    itemsToCheckout += selection.GetURL(0, i).GetSVNPathString() + L"*";
                }
                itemsToCheckout.TrimRight('*');
                CString sCmd;
                sCmd.Format(L"/command:checkout /url:\"%s\" /revision:%s", static_cast<LPCWSTR>(itemsToCheckout), static_cast<LPCWSTR>(selection.GetRepository(0).revision.ToString()));

                CAppUtils::RunTortoiseProc(sCmd);
            }
            break;
            case ID_EXPORT:
            {
                CExportDlg dlg(this);
                dlg.m_URL    = selection.GetURL(0, 0).GetSVNPathString();
                dlg.Revision = selection.GetRepository(0).revision;
                if (dlg.DoModal() == IDOK)
                {
                    CTSVNPath exportDirectory;
                    exportDirectory.SetFromWin(dlg.m_strExportDirectory, true);

                    CSVNProgressDlg progDlg;
                    int             opts = 0;
                    if (dlg.m_bNoExternals)
                        opts |= ProgOptIgnoreExternals;
                    if (dlg.m_eolStyle.CompareNoCase(L"CRLF") == 0)
                        opts |= ProgOptEolCRLF;
                    if (dlg.m_eolStyle.CompareNoCase(L"CR") == 0)
                        opts |= ProgOptEolCR;
                    if (dlg.m_eolStyle.CompareNoCase(L"LF") == 0)
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
                sCmd.Format(L"/command:revisiongraph /path:\"%s\" /pegrev:%s", static_cast<LPCWSTR>(selection.GetURLEscaped(0, 0).GetSVNPathString()), static_cast<LPCWSTR>(selection.GetRepository(0).revision.ToString()));

                CAppUtils::RunTortoiseProc(sCmd);
            }
            break;
            case ID_OPENWITH:
                bOpenWith = true;
                [[fallthrough]];
            case ID_OPEN:
            {
                OpenFile(selection.GetURL(0, 0), selection.GetURLEscaped(0, 0), bOpenWith);
            }
            break;
            case ID_EDITFILE:
            {
                if (!m_editFileCommand || !m_editFileCommand->IsWaiting())
                {
                    new async::CAsyncCall(this, &CRepositoryBrowser::EditFile, selection.GetURL(0, 0), selection.GetURLEscaped(0, 0), &m_backgroundJobs);
                }
                else
                {
                    ::MessageBox(GetSafeHwnd(), CString(MAKEINTRESOURCE(IDS_ERR_EDIT_LIMIT_REACHED)), L"TortoiseSVN", MB_ICONERROR);
                }
            }
            break;
            case ID_DELETE:
            {
                CWaitCursorEx waitCursor;
                CString       sLogMsg;
                if (!RunStartCommit(selection.GetURLsEscaped(0, false), sLogMsg))
                    break;
                CInputLogDlg input(this);
                input.SetPathList(selection.GetURLsEscaped(0, false));
                input.SetRootPath(CTSVNPath(m_initialUrl));
                input.SetLogText(sLogMsg);
                input.SetUUID(selection.GetRepository(0).uuid);
                input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEDEL);
                CString hint;
                if (selection.GetPathCount(0) == 1)
                    hint.Format(IDS_INPUT_REMOVEONE, static_cast<LPCWSTR>(selection.GetURL(0, 0).GetFileOrDirectoryName()));
                else
                    hint.Format(IDS_INPUT_REMOVEMORE, selection.GetPathCount(0));

                input.SetActionText(hint);
                if (input.DoModal() == IDOK)
                {
                    sLogMsg = input.GetLogMessage();
                    if (!RunPreCommit(selection.GetURLsEscaped(0, false), svn_depth_unknown, sLogMsg))
                        break;
                    bool bRet = Remove(selection.GetURLsEscaped(0, false), true, false, sLogMsg, input.m_revProps);
                    RunPostCommit(selection.GetURLsEscaped(0, false), svn_depth_unknown, m_commitRev, sLogMsg);
                    if (!bRet || !m_postCommitErr.IsEmpty())
                    {
                        waitCursor.Hide();
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
                        RefreshNode(m_repoTree.GetSelectedItem(), true);
                }
            }
            break;
            case ID_BREAKLOCK:
            {
                if (!Unlock(selection.GetURLsEscaped(0, true), TRUE))
                {
                    ShowErrorDialog(m_hWnd);
                    break;
                }
                InvalidateData(m_repoTree.GetSelectedItem());
                RefreshNode(m_repoTree.GetSelectedItem(), true);
            }
            break;
            case ID_IMPORTFOLDER:
            {
                CString       path;
                CBrowseFolder folderBrowser;
                folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
                if (folderBrowser.Show(GetSafeHwnd(), path) == CBrowseFolder::OK)
                {
                    CTSVNPath     svnPath(path);
                    CWaitCursorEx waitCursor;
                    CString       fileName = svnPath.GetFileOrDirectoryName();
                    CString       sLogMsg;
                    if (!RunStartCommit(selection.GetURLsEscaped(0, true), sLogMsg))
                        break;
                    CInputLogDlg input(this);
                    input.SetPathList(selection.GetURLsEscaped(0, true));
                    input.SetRootPath(CTSVNPath(m_initialUrl));
                    input.SetLogText(sLogMsg);
                    input.SetUUID(selection.GetRepository(0).uuid);
                    input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEIMPORT);

                    const CTSVNPath& url = selection.GetURL(0, 0);
                    CString          sHint;
                    sHint.FormatMessage(IDS_INPUT_IMPORTFOLDER, static_cast<LPCWSTR>(svnPath.GetSVNPathString()), static_cast<LPCWSTR>(url.GetSVNPathString() + L"/" + fileName));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sLogMsg = input.GetLogMessage();
                        if (!RunPreCommit(selection.GetURLsEscaped(0, true), svn_depth_unknown, sLogMsg))
                            break;
                        InvalidateDataParents(selection);

                        CProgressDlg progDlg;
                        progDlg.SetTitle(IDS_APPNAME);
                        CString sInfoLine;
                        sInfoLine.FormatMessage(IDS_PROGRESSIMPORT, static_cast<LPCWSTR>(fileName));
                        progDlg.SetLine(1, sInfoLine, true);
                        SetAndClearProgressInfo(&progDlg);
                        progDlg.ShowModeless(m_hWnd);
                        bool bRet = Import(svnPath,
                                           CTSVNPath(EscapeUrl(CTSVNPath(url.GetSVNPathString() + L"/" + fileName))),
                                           sLogMsg,
                                           &m_projectProperties,
                                           svn_depth_infinity,
                                           true, false, false, input.m_revProps);
                        RunPostCommit(selection.GetURLsEscaped(0, true), svn_depth_unknown, m_commitRev, sLogMsg);
                        if (!bRet || !m_postCommitErr.IsEmpty())
                        {
                            progDlg.Stop();
                            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            waitCursor.Hide();
                            ShowErrorDialog(m_hWnd);
                            if (!bRet)
                                break;
                        }
                        m_barRepository.SetHeadRevision(GetCommitRevision());
                        progDlg.Stop();
                        SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        RefreshNode(m_repoTree.GetSelectedItem(), true);
                    }
                }
            }
            break;
            case ID_IMPORT:
            {
                // Display the Open dialog box.
                CString openPath;
                if (CAppUtils::FileOpenSave(openPath, nullptr, IDS_REPOBROWSE_IMPORT, IDS_COMMONFILEFILTER, true, m_path.IsUrl() ? CString() : m_path.GetDirectory().GetWinPathString(), m_hWnd))
                {
                    CTSVNPath     path(openPath);
                    CWaitCursorEx waitCursor;
                    CString       fileName = path.GetFileOrDirectoryName();
                    CString       sLogMsg;
                    if (!RunStartCommit(selection.GetURLsEscaped(0, true), sLogMsg))
                        break;
                    CInputLogDlg input(this);
                    input.SetPathList(selection.GetURLsEscaped(0, true));
                    input.SetRootPath(CTSVNPath(m_initialUrl));
                    input.SetLogText(sLogMsg);
                    input.SetUUID(selection.GetRepository(0).uuid);
                    input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEIMPORT);

                    const CTSVNPath& url = selection.GetURL(0, 0);
                    CString          sHint;
                    sHint.FormatMessage(IDS_INPUT_IMPORTFILEFULL, path.GetWinPath(), static_cast<LPCWSTR>(url.GetSVNPathString() + L"/" + fileName));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sLogMsg = input.GetLogMessage();
                        if (!RunPreCommit(selection.GetURLsEscaped(0, true), svn_depth_unknown, sLogMsg))
                            break;
                        InvalidateDataParents(selection);

                        CProgressDlg progDlg;
                        progDlg.SetTitle(IDS_APPNAME);
                        CString sInfoLine;
                        sInfoLine.FormatMessage(IDS_PROGRESSIMPORT, static_cast<LPCWSTR>(fileName));
                        progDlg.SetLine(1, sInfoLine, true);
                        SetAndClearProgressInfo(&progDlg);
                        progDlg.ShowModeless(m_hWnd);
                        bool bRet = Import(path,
                                           CTSVNPath(EscapeUrl(CTSVNPath(url.GetSVNPathString() + L"/" + fileName))),
                                           sLogMsg,
                                           &m_projectProperties,
                                           svn_depth_empty,
                                           true, true, false, input.m_revProps);
                        RunPostCommit(selection.GetURLsEscaped(0, true), svn_depth_unknown, m_commitRev, sLogMsg);
                        if (!bRet || !m_postCommitErr.IsEmpty())
                        {
                            progDlg.Stop();
                            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                            waitCursor.Hide();
                            ShowErrorDialog(m_hWnd);
                            if (!bRet)
                                break;
                        }
                        m_barRepository.SetHeadRevision(GetCommitRevision());
                        progDlg.Stop();
                        SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        RefreshNode(m_repoTree.GetSelectedItem(), true);
                    }
                }
            }
            break;
            case ID_RENAME:
            {
                if (pWnd == &m_repoList)
                {
                    CAutoReadLock locker(m_guard);
                    POSITION      pos = m_repoList.GetFirstSelectedItemPosition();
                    if (pos == nullptr)
                        break;
                    int selIndex = m_repoList.GetNextSelectedItem(pos);
                    if (selIndex >= 0)
                    {
                        m_repoList.SetFocus();
                        m_repoList.EditLabel(selIndex);
                    }
                    else
                    {
                        m_repoTree.SetFocus();
                        HTREEITEM hTreeItem = m_repoTree.GetSelectedItem();
                        if (hTreeItem != m_repoTree.GetRootItem())
                            m_repoTree.EditLabel(hTreeItem);
                    }
                }
                else if (pWnd == &m_repoTree)
                {
                    CAutoReadLock locker(m_guard);
                    m_repoTree.SetFocus();
                    if (hChosenTreeItem != m_repoTree.GetRootItem())
                        m_repoTree.EditLabel(hChosenTreeItem);
                }
            }
            break;
            case ID_COPYTO:
            {
                const CTSVNPath& path     = selection.GetURL(0, 0);
                const SVNRev&    revision = selection.GetRepository(0).revision;

                CRenameDlg dlg(this);
                dlg.m_name = path.GetSVNPathString();
                dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_COPY);
                dlg.m_label.LoadString(IDS_REPO_BROWSEURL);
                dlg.m_infoLabel.Format(IDS_PROC_NEWNAMECOPY, static_cast<LPCWSTR>(path.GetSVNPathString()));
                dlg.SetRenameRequired(GetRevision().IsHead() != FALSE);
                CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
                if (dlg.DoModal() == IDOK)
                {
                    CWaitCursorEx waitCursor;
                    if (!CheckAndConfirmPath(CTSVNPath(dlg.m_name)))
                        break;
                    CString sLogMsg;
                    if (!RunStartCommit(selection.GetURLsEscaped(0, true), sLogMsg))
                        break;
                    CInputLogDlg input(this);
                    input.SetPathList(selection.GetURLsEscaped(0, true));
                    input.SetRootPath(CTSVNPath(m_initialUrl));
                    input.SetLogText(sLogMsg);
                    input.SetUUID(selection.GetRepository(0).uuid);
                    input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEBRANCH);
                    CString sHint;
                    CString sCopyUrl = path.GetSVNPathString() + L"@" + revision.ToString();
                    sHint.FormatMessage(IDS_INPUT_COPY, static_cast<LPCWSTR>(sCopyUrl), static_cast<LPCWSTR>(dlg.m_name));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sLogMsg = input.GetLogMessage();
                        if (!RunPreCommit(selection.GetURLsEscaped(0, true), svn_depth_unknown, sLogMsg))
                            break;
                        InvalidateDataParents(selection);

                        bool bRet = Copy(selection.GetURLsEscaped(0, true), CTSVNPath(dlg.m_name), revision, revision, sLogMsg, true, true, false, false, SVNExternals(), input.m_revProps);
                        RunPostCommit(selection.GetURLsEscaped(0, true), svn_depth_unknown, m_commitRev, sLogMsg);
                        if (!bRet || !m_postCommitErr.IsEmpty())
                        {
                            waitCursor.Hide();
                            ShowErrorDialog(m_hWnd);
                            if (!bRet)
                                break;
                        }
                        if (revision.IsHead())
                        {
                            m_barRepository.SetHeadRevision(GetCommitRevision());
                            RefreshNode(m_repoTree.GetSelectedItem(), true);
                        }
                    }
                }
            }
            break;
            case ID_COPYTOWC:
            {
                const SVNRev& revision = selection.GetRepository(0).revision;

                CTSVNPath tempFile;
                bool      bSavePathOK = AskForSavePath(selection, tempFile, false);
                if (bSavePathOK)
                {
                    CWaitCursorEx waitCursor;

                    CProgressDlg progDlg;
                    progDlg.SetTitle(IDS_APPNAME);
                    SetAndClearProgressInfo(&progDlg);
                    progDlg.ShowModeless(m_hWnd);

                    bool bCopyAsChild = (selection.GetPathCount(0) > 1);
                    if (!Copy(selection.GetURLsEscaped(0, true), tempFile, revision, revision, CString(), bCopyAsChild) || (progDlg.HasUserCancelled()))
                    {
                        progDlg.Stop();
                        SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                        waitCursor.Hide();
                        progDlg.Stop();
                        if (!progDlg.HasUserCancelled())
                            ShowErrorDialog(m_hWnd);
                        break;
                    }
                    m_barRepository.SetHeadRevision(GetCommitRevision());
                    progDlg.Stop();
                    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                }
            }
            break;
            case ID_MKDIR:
            {
                const CTSVNPath& path = selection.GetURL(0, 0);
                CRenameDlg       dlg(this);
                dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_MKDIR);
                CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
                if (dlg.DoModal() == IDOK)
                {
                    CWaitCursorEx wait_cursor;
                    CTSVNPathList plist = CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(path.GetSVNPathString() + L"/" + dlg.m_name.Trim()))));
                    CString       sLogMsg;
                    if (!RunStartCommit(plist, sLogMsg))
                        break;
                    CInputLogDlg input(this);
                    input.SetPathList(plist);
                    input.SetRootPath(CTSVNPath(m_initialUrl));
                    input.SetLogText(sLogMsg);
                    input.SetUUID(selection.GetRepository(0).uuid);
                    input.SetProjectProperties(&m_projectProperties, PROJECTPROPNAME_LOGTEMPLATEMKDIR);
                    CString sHint;
                    sHint.Format(IDS_INPUT_MKDIR, static_cast<LPCWSTR>(path.GetSVNPathString() + L"/" + dlg.m_name.Trim()));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sLogMsg = input.GetLogMessage();
                        if (!RunPreCommit(plist, svn_depth_unknown, sLogMsg))
                            break;
                        InvalidateDataParents(selection);

                        // when creating the new folder, also trim any whitespace chars from it
                        bool bRet = MakeDir(plist, sLogMsg, true, input.m_revProps);
                        RunPostCommit(plist, svn_depth_unknown, m_commitRev, sLogMsg);
                        if (!bRet || !m_postCommitErr.IsEmpty())
                        {
                            wait_cursor.Hide();
                            ShowErrorDialog(m_hWnd);
                            if (!bRet)
                                break;
                        }
                        m_barRepository.SetHeadRevision(GetCommitRevision());
                        RefreshNode(m_repoTree.GetSelectedItem(), true);
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
                const CTSVNPath& path     = selection.GetURLEscaped(0, 0);
                const SVNRev&    revision = selection.GetRepository(0).revision;
                CString          options;
                bool             prettyPrint = true;
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    CDiffOptionsDlg dlg;
                    if (dlg.DoModal() == IDOK)
                    {
                        options     = dlg.GetDiffOptionsString();
                        prettyPrint = dlg.GetPrettyPrint();
                    }
                    else
                        break;
                }
                SVNDiff diff(this, this->m_hWnd, true);
                if (selection.GetPathCount(0) == 1)
                {
                    if (PromptShown())
                        diff.ShowUnifiedDiff(path, revision,
                                             CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), prettyPrint, options, false, false, false);
                    else
                        CAppUtils::StartShowUnifiedDiff(m_hWnd, path, revision,
                                                        CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), SVNRev(), prettyPrint, options, false, false, false, false);
                }
                else
                {
                    const CTSVNPath& path2 = selection.GetURLEscaped(0, 1);
                    if (PromptShown())
                        diff.ShowUnifiedDiff(path, revision,
                                             path2, revision, SVNRev(), prettyPrint, options, false, false, false);
                    else
                        CAppUtils::StartShowUnifiedDiff(m_hWnd, path, revision,
                                                        path2, revision, SVNRev(), SVNRev(), prettyPrint, options, false, false, false, false);
                }
            }
            break;
            case ID_DIFF_CONTENTONLY:
                bIgnoreProps = true;
                [[fallthrough]];
            case ID_DIFF:
            {
                const CTSVNPath& path     = selection.GetURLEscaped(0, 0);
                const SVNRev&    revision = selection.GetRepository(0).revision;
                size_t           nFolders = selection.GetFolderCount(0);

                SVNDiff diff(this, this->m_hWnd, true);
                diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
                if (selection.GetPathCount(0) == 1)
                {
                    if (PromptShown())
                        diff.ShowCompare(path, revision,
                                         CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), bIgnoreProps, true, L"", true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
                    else
                        CAppUtils::StartShowCompare(m_hWnd, path, revision,
                                                    CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), SVNRev(), bIgnoreProps, true, L"",
                                                    !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
                }
                else
                {
                    const CTSVNPath& path2 = selection.GetURLEscaped(0, 1);
                    if (PromptShown())
                        diff.ShowCompare(path, revision,
                                         path2, revision, SVNRev(), bIgnoreProps, true, L"", true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
                    else
                        CAppUtils::StartShowCompare(m_hWnd, path, revision,
                                                    path2, revision, SVNRev(), SVNRev(), bIgnoreProps, true, L"",
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
                    dlg.SetProjectProperties(&m_projectProperties);
                    dlg.SetUUID(selection.GetRepository(0).uuid);
                    dlg.SetPathList(selection.GetURLsEscaped(0, true));
                    dlg.SetRevision(GetHEADRevision(selection.GetURL(0, 0)));
                    dlg.UrlIsFolder(selection.GetFolderCount(0) != 0);
                    dlg.DoModal();
                }
                else
                {
                    CPropDlg dlg(this);
                    dlg.m_rev  = revision;
                    dlg.m_Path = selection.GetURLEscaped(0, 0);
                    dlg.DoModal();
                }
            }
            break;
            case ID_REVPROPS:
            {
                CEditPropertiesDlg dlg(this);
                dlg.SetProjectProperties(&m_projectProperties);
                dlg.SetUUID(selection.GetRepository(0).uuid);
                dlg.SetPathList(selection.GetURLsEscaped(0, true));
                dlg.SetRevision(selection.GetRepository(0).revision);
                dlg.RevProps(true);
                dlg.DoModal();
            }
            break;
            case ID_BLAME:
            {
                CBlameDlg dlg(this);
                dlg.m_endRev = selection.GetRepository(0).revision;
                dlg.m_pegRev = m_repository.revision;
                if (dlg.DoModal() == IDOK)
                {
                    CBlame           blame;
                    CString          tempfile;
                    const CTSVNPath& path = selection.GetURLEscaped(0, 0);

                    tempfile = blame.BlameToTempFile(path, dlg.m_startRev, dlg.m_endRev, dlg.m_pegRev, SVN::GetOptionsString(!!dlg.m_bIgnoreEOL, dlg.m_ignoreSpaces), dlg.m_bIncludeMerge, TRUE, TRUE);
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
                            if (!CAppUtils::LaunchTortoiseBlame(tempfile, CPathUtils::GetFileNameFromPath(selection.GetURL(0, 0).GetFileOrDirectoryName()), sParams, dlg.m_startRev, dlg.m_endRev, dlg.m_pegRev))
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
                    SVNRev  r      = selection.GetRepository(0).revision;
                    if (r.IsNumber())
                        urlCmd += L"?r=" + r.ToString();
                    CAppUtils::CreateShortcutToURL(static_cast<LPCWSTR>(urlCmd), tempFile.GetWinPath());
                }
            }
            break;
            case ID_ADDTOBOOKMARKS:
            {
                m_bookmarks.insert(static_cast<LPCWSTR>(selection.GetURL(0, 0).GetSVNPathString()));
                RefreshBookmarks();
            }
            break;
            case ID_REMOVEBOOKMARKS:
            {
                m_bookmarks.erase(static_cast<LPCWSTR>(selection.GetURL(0, 0).GetSVNPathString()));
                RefreshBookmarks();
            }
            break;
            default:
                break;
        }
        DialogEnableWindow(IDOK, TRUE);
    }
}

bool CRepositoryBrowser::AskForSavePath(const CRepositoryBrowserSelection& selection, CTSVNPath& tempFile, bool bFolder) const
{
    bool bSavePathOk = false;
    if ((!bFolder) && (selection.GetPathCount(0) == 1))
    {
        CString savePath = selection.GetURL(0, 0).GetFilename();
        bSavePathOk      = CAppUtils::FileOpenSave(savePath, nullptr, IDS_REPOBROWSE_SAVEAS, IDS_COMMONFILEFILTER, false, m_path.IsUrl() ? CString() : m_path.GetDirectory().GetWinPathString(), m_hWnd);
        if (bSavePathOk)
            tempFile.SetFromWin(savePath);
    }
    else
    {
        CBrowseFolder browser;
        CString       sTempfile;
        browser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
        browser.Show(GetSafeHwnd(), sTempfile);
        if (!sTempfile.IsEmpty())
        {
            bSavePathOk = true;
            tempFile.SetFromWin(sTempfile);
        }
    }
    return bSavePathOk;
}

bool CRepositoryBrowser::StringToWidthArray(const CString& widthString, int widthArray[])
{
    wchar_t* endChar = nullptr;
    for (int i = 0; i < 7; ++i)
    {
        CString hex;
        if (widthString.GetLength() >= i * 8 + 8)
            hex = widthString.Mid(i * 8, 8);
        if (hex.IsEmpty())
        {
            // This case only occurs when upgrading from an older
            // TSVN version in which there were fewer columns.
            widthArray[i] = 0;
        }
        else
        {
            widthArray[i] = wcstol(hex, &endChar, 16);
        }
    }
    return true;
}

CString CRepositoryBrowser::WidthArrayToString(int widthArray[]) const
{
    CString sResult;
    wchar_t buf[10] = {0};
    for (int i = 0; i < 7; ++i)
    {
        swprintf_s(buf, L"%08X", widthArray[i]);
        sResult += buf;
    }
    return sResult;
}

void CRepositoryBrowser::SaveColumnWidths(bool bSaveToRegistry /* = false */)
{
    CRegString regColWidth(L"Software\\TortoiseSVN\\RepoBrowserColumnWidth");
    int        maxCol = m_repoList.GetHeaderCtrl()->GetItemCount() - 1;
    // first clear the width array
    std::fill_n(m_arColumnWidths, _countof(m_arColumnWidths), 0);
    for (int col = 0; col <= maxCol; ++col)
    {
        m_arColumnWidths[col] = m_repoList.GetColumnWidth(col);
        if (m_arColumnWidths[col] == m_arColumnAutoWidths[col])
            m_arColumnWidths[col] = 0;
    }
    if (bSaveToRegistry)
    {
        CString sWidths = WidthArrayToString(m_arColumnWidths);
        regColWidth     = sWidths;
    }
}

void CRepositoryBrowser::InvalidateData(HTREEITEM node)
{
    if (node == nullptr)
        return;
    SVNRev r;
    {
        CAutoReadLock locker(m_guard);
        CTreeItem*    pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
        if (pItem->m_bookmark || pItem->m_dummy)
            return;
        r = pItem->m_repository.revision;
    }
    InvalidateData(node, r);
}

void CRepositoryBrowser::InvalidateData(HTREEITEM node, const SVNRev& revision)
{
    CString url;
    {
        CAutoReadLock locker(m_guard);
        CTreeItem*    pItem = nullptr;
        if (node != nullptr)
        {
            pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(node));
            if (pItem->m_bookmark || pItem->m_dummy)
                return;
            url = pItem->m_url;
        }
    }
    if (url.IsEmpty())
        m_lister.Refresh(revision);
    else
        m_lister.RefreshSubTree(revision, url);
}

void CRepositoryBrowser::InvalidateDataParents(const CRepositoryBrowserSelection& selection)
{
    for (size_t i = 0; i < selection.GetRepositoryCount(); ++i)
    {
        const SVNRev& revision = selection.GetRepository(i).revision;
        for (size_t k = 0, count = selection.GetPathCount(i); k < count; ++k)
        {
            const CString& url = selection.GetURL(i, k).GetSVNPathString();
            InvalidateData(m_repoTree.GetParentItem(FindUrl(url)), revision);
        }
    }
}

void CRepositoryBrowser::BeginDrag(const CWnd&                  window,
                                   CRepositoryBrowserSelection& selection, POINT& point, bool setAsyncMode) const
{
    if (m_bSparseCheckoutMode)
        return;

    // must be exactly one repository
    if (selection.GetRepositoryCount() != 1)
    {
        return;
    }

    // build copy source / content
    auto pdSrc = std::make_unique<CIDropSource>();
    if (pdSrc == nullptr)
        return;

    pdSrc->AddRef();

    const SVNRev&  revision = selection.GetRepository(0).revision;
    SVNDataObject* pdObj    = new SVNDataObject(selection.GetURLsEscaped(0, true), revision, revision);
    if (pdObj == nullptr)
    {
        return;
    }
    pdObj->AddRef();
    if (setAsyncMode)
        pdObj->SetAsyncMode(TRUE);
    CDragSourceHelper dragsrchelper;
    dragsrchelper.InitializeFromWindow(window.GetSafeHwnd(), point, pdObj);
    pdSrc->m_pIDataObj = pdObj;
    pdSrc->m_pIDataObj->AddRef();

    // Initiate the Drag & Drop
    DWORD dwEffect;
    ::DoDragDrop(pdObj, pdSrc.get(), DROPEFFECT_MOVE | DROPEFFECT_COPY, &dwEffect);
    pdSrc->Release();
    pdSrc.release();
    pdObj->Release();
}

void CRepositoryBrowser::StoreSelectedURLs()
{
    CAutoReadLock               locker(m_guard);
    CRepositoryBrowserSelection selection;

    // selections on the RHS list take precedence

    POSITION pos = m_repoList.GetFirstSelectedItemPosition();
    if (pos != nullptr)
    {
        int index = -1;
        while ((index = m_repoList.GetNextSelectedItem(pos)) >= 0)
            selection.Add(reinterpret_cast<CItem*>(m_repoList.GetItemData(index)));
    }

    // followed by the LHS tree view

    if (selection.IsEmpty())
    {
        HTREEITEM hSelectedTreeItem = m_repoTree.GetSelectedItem();
        if (hSelectedTreeItem)
            selection.Add(reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hSelectedTreeItem)));
    }

    // in line with the URL selected in the edit box?
    // path edit box has highest prio

    CString editPath = GetPath();

    m_selectedUrLs = selection.Contains(CTSVNPath(editPath))
                         ? selection.GetURLs(0).CreateAsteriskSeparatedString()
                         : editPath;
}

const CString& CRepositoryBrowser::GetSelectedURLs() const
{
    return m_selectedUrLs;
}

void CRepositoryBrowser::OnTvnItemChangedRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMTVITEMCHANGE* pNMTVItemChange = reinterpret_cast<NMTVITEMCHANGE*>(pNMHDR);
    *pResult                        = 0;

    if (!m_bSparseCheckoutMode || m_blockEvents)
        return;

    if (pNMTVItemChange->uStateOld == 0 && pNMTVItemChange->uStateNew == 0)
        return; // No change

    BOOL bPrevState = static_cast<BOOL>(((pNMTVItemChange->uStateOld & TVIS_STATEIMAGEMASK) >> 12) - 1); // Old check box state
    if (bPrevState < 0)                                                                                  // On startup there's no previous state
        bPrevState = 0;                                                                                  // so assign as false (unchecked)

    // New check box state
    BOOL bChecked = static_cast<BOOL>(((pNMTVItemChange->uStateNew & TVIS_STATEIMAGEMASK) >> 12) - 1);
    if (bChecked < 0) // On non-checkbox notifications assume false
        bChecked = 0;

    if (bPrevState == bChecked) // No change in check box
        return;

    bChecked = m_repoTree.GetCheck(pNMTVItemChange->hItem);

    m_blockEvents++;

    CheckTreeItem(pNMTVItemChange->hItem, !!bChecked);

    if (pNMTVItemChange->uStateNew & TVIS_SELECTED)
    {
        HTREEITEM hItem = m_repoTree.GetSelectedItem();
        HTREEITEM hRoot = m_repoTree.GetRootItem();
        while (hItem)
        {
            if ((hItem != pNMTVItemChange->hItem) && (hItem != hRoot))
            {
                m_repoTree.SetCheck(hItem, !!bChecked);
                CheckTreeItem(hItem, !!bChecked);
            }
            hItem = m_repoTree.GetNextItem(hItem, TVGN_NEXTSELECTED);
        }
    }
    m_blockEvents--;
}

bool CRepositoryBrowser::CheckoutDepthForItem(HTREEITEM hItem)
{
    CAutoReadLock locker(m_guard);
    bool          bChecked = !!m_repoTree.GetCheck(hItem);
    CTreeItem*    pItem    = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hItem));
    if (!bChecked)
    {
        m_updateDepths[pItem->m_url] = svn_depth_exclude;
        HTREEITEM hParent            = m_repoTree.GetParentItem(hItem);
        while (hParent)
        {
            CTreeItem*                               pParent = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hParent));
            std::map<CString, svn_depth_t>::iterator it      = m_checkoutDepths.find(pParent->m_url);
            if (it != m_checkoutDepths.end())
            {
                if ((it->second == svn_depth_infinity) && (pItem->m_kind == svn_node_dir))
                    m_checkoutDepths[pParent->m_url] = svn_depth_files;
                else
                    m_checkoutDepths[pParent->m_url] = svn_depth_empty;
            }

            it = m_updateDepths.find(pParent->m_url);
            if (it != m_updateDepths.end())
            {
                m_updateDepths[pParent->m_url] = svn_depth_unknown;
            }
            hParent = m_repoTree.GetParentItem(hParent);
        }
    }
    if (bChecked)
    {
        m_checkoutDepths[pItem->m_url] = svn_depth_infinity;
        m_updateDepths[pItem->m_url]   = svn_depth_infinity;

        // If the directory was not expanded and a sticky depth is already set for it in the
        // working copy, we can simply fetch the node with unknown depth, causing SVN to update
        // the directory with working-copy depth. Otherwise, if we have no sticky depth yet, we
        // can assume that the user has just selected the node without expanding it. In that case,
        // we want to update the directory in a fully recursive way.
        //
        // Furthermore, if the user has re-checked the item without expanding its children, we assume
        // that the user is requesting a fully recursive checkout of the directory.
        if (!pItem->m_childrenFetched && !pItem->m_checkboxToggled && (pItem->m_kind == svn_node_dir))
        {
            auto foundWCDepth = m_wcDepths.find(pItem->m_url);
            if (foundWCDepth != m_wcDepths.end())
            {
                m_updateDepths[pItem->m_url] = svn_depth_unknown;
            }
        }
    }

    HTREEITEM hChild = m_repoTree.GetChildItem(hItem);
    while (hChild)
    {
        CheckoutDepthForItem(hChild);
        hChild = m_repoTree.GetNextItem(hChild, TVGN_NEXT);
    }

    return bChecked;
}

void CRepositoryBrowser::CheckParentsOfTreeItem(HTREEITEM hItem)
{
    HTREEITEM hParent = m_repoTree.GetParentItem(hItem);
    while (hParent)
    {
        m_repoTree.SetCheck(hParent, TRUE);
        hParent = m_repoTree.GetParentItem(hParent);
    }
}

void CRepositoryBrowser::CheckTreeItemRecursive(HTREEITEM hItem, bool bCheck)
{
    // process item
    m_repoTree.SetCheck(hItem, bCheck);

    // process all children
    HTREEITEM hChild = m_repoTree.GetChildItem(hItem);
    while (hChild)
    {
        CheckTreeItemRecursive(hChild, bCheck);
        hChild = m_repoTree.GetNextItem(hChild, TVGN_NEXT);
    }
}

bool CRepositoryBrowser::HaveAllChildrenSameCheckState(HTREEITEM hItem, bool bChecked /* = false */) const
{
    // analyze all children
    HTREEITEM hChild = m_repoTree.GetChildItem(hItem);
    while (hChild)
    {
        // child item doesn't have expected state. interrupt walk and return false
        if (!!m_repoTree.GetCheck(hChild) != bChecked)
            return false;

        // check condition on all descendants.
        if (!HaveAllChildrenSameCheckState(hChild))
            return false;

        hChild = m_repoTree.GetNextItem(hChild, TVGN_NEXT);
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
void CRepositoryBrowser::CheckTreeItem(HTREEITEM hItem, bool bCheck)
{
    CTreeItem* pItem           = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hItem));
    bool       itemExpanded    = !!(m_repoTree.GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED);
    bool       multiSelectMode = m_repoTree.GetSelectedCount() > 1;

    if (pItem)
    {
        pItem->m_checkboxToggled = true;
    }

    // tri-state logic will be emulated for expanded, checked items, in single selection mode,
    // with at least one child, when all children unchecked
    if (!bCheck && !multiSelectMode && itemExpanded && m_repoTree.GetChildItem(hItem) != nullptr && HaveAllChildrenSameCheckState(hItem))
    {
        // instead of reseting item itself - set all children to checked state
        CheckTreeItemRecursive(hItem, TRUE);
        return;
    }

    if (bCheck)
    {
        if (!itemExpanded || multiSelectMode)
        {
            // perform check recursively for special cases
            CheckTreeItemRecursive(hItem, bCheck);
        }
        else
        {
            // 'Only this item' depth
            m_repoTree.SetCheck(hItem, bCheck);
        }

        // check all parents
        CheckParentsOfTreeItem(hItem);
    }
    else
    {
        if (m_bSparseCheckoutMode && !m_wcDepths.empty())
        {
            // clear the wc depth state of this item so it won't get used
            // later in the OnOK handler in case this item is not expanded.
            if (pItem)
            {
                m_wcDepths.erase(pItem->m_url);
            }
        }
        // uncheck item and all children
        CheckTreeItemRecursive(hItem, bCheck);

        // force the root item to be checked
        if (hItem == m_repoTree.GetRootItem())
        {
            m_repoTree.SetCheck(hItem);
        }
    }
}

bool CRepositoryBrowser::CheckAndConfirmPath(const CTSVNPath& targetUrl)
{
    if (targetUrl.IsValidOnWindows())
        return true;

    CString sInfo;
    sInfo.Format(IDS_WARN_NOVALIDPATH_TASK1, static_cast<LPCWSTR>(targetUrl.GetUIPathString()));
    CTaskDialog taskDlg(sInfo,
                        CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK2)),
                        L"TortoiseSVN",
                        0,
                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK3)));
    taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK4)));
    taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskDlg.SetExpansionArea(CString(MAKEINTRESOURCE(IDS_WARN_NOVALIDPATH_TASK5)));
    taskDlg.SetDefaultCommandControl(2);
    taskDlg.SetMainIcon(TD_WARNING_ICON);
    return (taskDlg.DoModal(GetExplorerHWND()) == 100);
}

void CRepositoryBrowser::SaveDividerPosition() const
{
    RECT rc{};
    // use GetWindowRect instead of GetClientRect to account for possible scrollbars.
    GetDlgItem(IDC_REPOTREE)->GetWindowRect(&rc);
    CRegDWORD xPos(L"Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\RepobrowserDivider");
    xPos = (rc.right - rc.left) - (2 * GetSystemMetrics(SM_CXBORDER));
}

int CRepositoryBrowser::SortStrCmp(PCWSTR str1, PCWSTR str2)
{
    if (m_bSortLogical)
        return StrCmpLogicalW(str1, str2);
    return StrCmpI(str1, str2);
}

void CRepositoryBrowser::ShowText(const CString& sText, bool forceupdate /*= false*/)
{
    if (m_bSparseCheckoutMode)
        m_repoTree.ShowText(sText, forceupdate);
    else
        m_repoList.ShowText(sText, forceupdate);
}

void CRepositoryBrowser::FilterInfinityDepthItems(std::map<CString, svn_depth_t>& depths)
{
    if (depths.empty())
        return;

    // now go through the whole list and remove all children of items that have infinity depth
    for (std::map<CString, svn_depth_t>::iterator it = depths.begin(); it != depths.end(); ++it)
    {
        if (it->second != svn_depth_infinity)
            continue;

        for (std::map<CString, svn_depth_t>::iterator it2 = depths.begin(); it2 != depths.end(); ++it2)
        {
            if (it->first.Compare(it2->first) == 0)
                continue;

            CString url1 = it->first + L"/";
            if (url1.Compare(it2->first.Left(url1.GetLength())) == 0)
            {
                std::map<CString, svn_depth_t>::iterator kill = it2;
                --it2;
                depths.erase(kill);
            }
        }
    }
}

void CRepositoryBrowser::FilterUnknownDepthItems(std::map<CString, svn_depth_t>& depths)
{
    if (depths.empty())
        return;

    // now go through the whole list and remove all children with unknown depth having a parent with unknown depth
    for (std::map<CString, svn_depth_t>::iterator it = depths.begin(); it != depths.end(); ++it)
    {
        if (it->second != svn_depth_unknown)
            continue;

        for (std::map<CString, svn_depth_t>::iterator it2 = depths.begin(); it2 != depths.end(); ++it2)
        {
            if (it2->second != svn_depth_unknown)
                continue;
            if (it->first.Compare(it2->first) == 0)
                continue;

            CString url1 = it->first + L"/";
            if (url1.Compare(it2->first.Left(url1.GetLength())) == 0)
            {
                // only remove items which are already checked out, i.e. are
                // already present in the working copy.
                // if an item isn't in the working copy yet, we need to
                // check out/update the item even if it has unknown depth
                auto wcit = m_wcDepths.find(it2->first);
                if (wcit != m_wcDepths.end())
                {
                    std::map<CString, svn_depth_t>::iterator kill = it2;
                    --it2;
                    depths.erase(kill);
                }
            }
        }
    }
}

void CRepositoryBrowser::SetListItemInfo(int index, const CItem* it)
{
    if (it->m_isExternal)
        m_repoList.SetItemState(index, INDEXTOOVERLAYMASK(OVERLAY_EXTERNAL), LVIS_OVERLAYMASK);

    // extension
    CString temp = CPathUtils::GetFileExtFromPath(it->m_path);
    if (it->m_kind == svn_node_file)
        m_repoList.SetItemText(index, 1, temp);

    // pointer to the CItem structure
    m_repoList.SetItemData(index, reinterpret_cast<DWORD_PTR>(&(*it)));

    if (it->m_unversioned)
        return;

    // revision
    if ((it->m_createdRev == SVN_IGNORED_REVNUM) && (it->m_isExternal))
        temp = it->m_time != 0 ? L"" : L"HEAD";
    else
        temp.Format(L"%ld", it->m_createdRev);
    m_repoList.SetItemText(index, 2, temp);

    // author
    m_repoList.SetItemText(index, 3, it->m_author);

    // size
    if (it->m_kind == svn_node_file)
    {
        StrFormatByteSize(it->m_size, temp.GetBuffer(20), 20);
        temp.ReleaseBuffer();
        m_repoList.SetItemText(index, 4, temp);
    }

    // date
    wchar_t dateNative[SVN_DATE_BUFFER] = {0};
    if ((it->m_time == 0) && (it->m_isExternal))
        dateNative[0] = 0;
    else
        SVN::formatDate(dateNative, const_cast<apr_time_t&>(it->m_time), true);
    m_repoList.SetItemText(index, 5, dateNative);

    if (m_bShowLocks)
    {
        // lock owner
        m_repoList.SetItemText(index, 6, it->m_lockOwner);
        // lock comment
        m_repoList.SetItemText(index, 7, m_projectProperties.MakeShortMessage(it->m_lockComment));
    }
}

void CRepositoryBrowser::OnNMCustomdrawRepolist(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_repoList.HasText())
        return;

    // Draw incomplete and unversioned items in gray.
    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        // Tell Windows to paint the control itself.
        *pResult = CDRF_DODEFAULT;

        if (m_repoList.GetItemCount() > static_cast<int>(pLVCD->nmcd.dwItemSpec))
        {
            COLORREF      crText = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
            CAutoReadLock locker(m_guard);
            CItem*        pItem = reinterpret_cast<CItem*>(m_repoList.GetItemData(static_cast<int>(pLVCD->nmcd.dwItemSpec)));
            if (pItem && pItem->m_unversioned)
            {
                crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
            }
            // Store the color back in the NMLVCUSTOMDRAW struct.
            pLVCD->clrText = crText;
        }
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRepositoryBrowser::OnNMCustomdrawRepotree(NMHDR* pNMHDR, LRESULT* pResult)
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

        CTreeItem* pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(reinterpret_cast<HTREEITEM>(pTVCD->nmcd.dwItemSpec)));
        if (pItem && pItem->m_unversioned)
        {
            // Store the color back in the NMLVCUSTOMDRAW struct.
            pTVCD->clrText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
        }
        if (pItem && pItem->m_dummy)
        {
            // don't draw empty items, they're used for spacing
            *pResult = CDRF_DOERASE | CDRF_SKIPDEFAULT;
        }
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRepositoryBrowser::OnTvnItemChangingRepotree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMTVITEMCHANGE* pNMTVItemChange = reinterpret_cast<NMTVITEMCHANGE*>(pNMHDR);
    *pResult                        = 0;
    CTreeItem* pItem                = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(static_cast<HTREEITEM>(pNMTVItemChange->hItem)));
    if (pItem && pItem->m_dummy)
        *pResult = 1;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CRepositoryBrowser::OnNMSetCursorRepotree(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    *pResult = 0;
    POINT pt;
    GetCursorPos(&pt);
    m_repoTree.ScreenToClient(&pt);
    UINT      flags = TVHT_ONITEM | TVHT_ONITEMINDENT | TVHT_ONITEMRIGHT | TVHT_ONITEMBUTTON | TVHT_TOLEFT | TVHT_TORIGHT;
    HTREEITEM hItem = m_repoTree.HitTest(pt, &flags);
    if (hItem)
    {
        CTreeItem* pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hItem));
        if (pItem && pItem->m_dummy)
        {
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
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

    // since html pages returned by SVNParentPath contain the listed repository
    // urls with a trailing slash if the page is requested with additional trailing slashes,
    // we have to remove them here first. Otherwise we'll end up with repo urls
    // like https://server.com/repos/repo1//trunk
    m_initialUrl.TrimRight(L"/\\ \t\r\n");

    HRESULT hResUdl = URLDownloadToFile(nullptr, m_initialUrl + L"/", tempfile.GetWinPath(), 0, callback.get());
    if (hResUdl != S_OK)
    {
        hResUdl = URLDownloadToFile(nullptr, m_initialUrl, tempfile.GetWinPath(), 0, callback.get());
    }
    if (hResUdl == S_OK)
    {
        // set up the SVNParentPath url as the repo root, even though it isn't a real repo root
        m_repository.root            = m_initialUrl;
        m_repository.revision        = SVNRev::REV_HEAD;
        m_repository.pegRevision    = SVNRev::REV_HEAD;
        m_repository.isSVNParentPath = true;

        // insert our pseudo repo root into the tree view.
        CTreeItem* pTreeItem           = new CTreeItem();
        pTreeItem->m_unescapedName     = m_initialUrl;
        pTreeItem->m_url               = m_initialUrl;
        pTreeItem->m_revision          = m_repository.revision;
        pTreeItem->m_logicalPath       = m_initialUrl;
        pTreeItem->m_repository        = m_repository;
        pTreeItem->m_kind              = svn_node_dir;
        pTreeItem->m_svnParentPathRoot = true;
        pTreeItem->m_childrenFetched   = true;
        TVINSERTSTRUCT tvinsert        = {nullptr};
        tvinsert.hParent               = TVI_ROOT;
        tvinsert.hInsertAfter          = TVI_ROOT;
        tvinsert.itemex.mask           = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
        tvinsert.itemex.pszText        = m_initialUrl.GetBuffer(m_initialUrl.GetLength());
        tvinsert.itemex.cChildren      = 1;
        tvinsert.itemex.lParam         = reinterpret_cast<LPARAM>(pTreeItem);
        tvinsert.itemex.iImage         = m_nSVNParentPath;
        tvinsert.itemex.iSelectedImage = m_nSVNParentPath;

        HTREEITEM hRoot = m_repoTree.InsertItem(&tvinsert);
        m_initialUrl.ReleaseBuffer();

        // we got a web page! But we can't be sure that it's the page from SVNParentPath.
        // Use a regex to parse the website and find out...
        std::ifstream fs(tempfile.GetWinPath());
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

            // make sure this is a html page from an SVNParentPathList
            // we do this by checking for header titles looking like
            // "<h2>Revision XX: /</h2> - if we find that, it's a html
            // page from inside a repository
            const char* reTitle = "<\\s*h2\\s*>[^/]+/\\s*<\\s*/\\s*h2\\s*>";
            // xsl transformed pages don't have an easy way to determine
            // the inside from outside of a repository.
            // We therefore check for <index rev="0" to make sure it's either
            // an empty repository or really an SVNParentPathList
            const char*      reTitle2 = "<\\s*index\\s*rev\\s*=\\s*\"0\"";
            const std::regex titEx(reTitle, std::regex_constants::icase | std::regex_constants::ECMAScript);
            const std::regex titEx2(reTitle2, std::regex_constants::icase | std::regex_constants::ECMAScript);
            if (std::regex_search(in.begin(), in.end(), titEx, std::regex_constants::match_default))
            {
                TRACE(L"found repository url instead of SVNParentPathList\n");
                return false;
            }

            const char* re  = "<\\s*LI\\s*>\\s*<\\s*A\\s+[^>]*HREF\\s*=\\s*\"([^\"]*)\"\\s*>([^<]+)<\\s*/\\s*A\\s*>\\s*<\\s*/\\s*LI\\s*>";
            const char* re2 = "<\\s*DIR\\s*name\\s*=\\s*\"([^\"]*)\"\\s*HREF\\s*=\\s*\"([^\"]*)\"\\s*/\\s*>";

            const std::regex           expression(re, std::regex_constants::icase | std::regex_constants::ECMAScript);
            const std::regex           expression2(re2, std::regex_constants::icase | std::regex_constants::ECMAScript);
            int                        nCountNewEntries = 0;
            const std::sregex_iterator end;
            for (std::sregex_iterator i(in.begin(), in.end(), expression); i != end; ++i)
            {
                const std::smatch match = *i;
                // what[0] contains the whole string
                // what[1] contains the url part.
                // what[2] contains the name
                CString sMatch = CUnicodeUtils::GetUnicode(std::string(match[1]).c_str());
                sMatch.TrimRight('/');
                sMatch      = CPathUtils::PathUnescape(sMatch);
                CString url = m_initialUrl + L"/" + sMatch;
                CItem   item;
                item.m_absolutePath = url;
                item.m_kind         = svn_node_dir;
                item.m_path         = sMatch;
                SRepositoryInfo repoInfo;
                repoInfo.root         = url;
                repoInfo.revision     = SVNRev::REV_HEAD;
                repoInfo.pegRevision = SVNRev::REV_HEAD;
                item.m_repository       = repoInfo;
                AutoInsert(hRoot, item);
                ++nCountNewEntries;
            }
            if (!regex_search(in.begin(), in.end(), titEx2))
            {
                return (nCountNewEntries > 0);
            }
            for (std::sregex_iterator i(in.begin(), in.end(), expression2); i != end; ++i)
            {
                const std::smatch match = *i;
                // what[0] contains the whole string
                // what[1] contains the url part.
                // what[2] contains the name
                CString sMatch = CUnicodeUtils::GetUnicode(std::string(match[1]).c_str());
                sMatch.TrimRight('/');
                sMatch      = CPathUtils::PathUnescape(sMatch);
                CString url = m_initialUrl + L"/" + sMatch;
                CItem   item;
                item.m_absolutePath = url;
                item.m_kind         = svn_node_dir;
                item.m_path         = sMatch;
                SRepositoryInfo repoInfo;
                repoInfo.root         = url;
                repoInfo.revision     = SVNRev::REV_HEAD;
                repoInfo.pegRevision = SVNRev::REV_HEAD;
                item.m_repository       = repoInfo;
                AutoInsert(hRoot, item);
                ++nCountNewEntries;
            }
            return (nCountNewEntries > 0);
        }
    }
    return false;
}

void CRepositoryBrowser::OnUrlHistoryBack()
{
    if (m_urlHistory.empty())
        return;

    CString url = m_urlHistory.front();
    if (url == GetPath())
    {
        m_urlHistory.pop_front();
        if (m_urlHistory.empty())
            return;
        url = m_urlHistory.front();
    }
    SVNRev r = GetRevision();
    m_urlHistoryForward.push_front(GetPath());
    ChangeToUrl(url, r, true);
    m_urlHistory.pop_front();
    m_barRepository.ShowUrl(url, r);
}

void CRepositoryBrowser::OnUrlHistoryForward()
{
    if (m_urlHistoryForward.empty())
        return;

    CString url = m_urlHistoryForward.front();
    if (url == GetPath())
    {
        m_urlHistoryForward.pop_front();
        if (m_urlHistoryForward.empty())
            return;
        url = m_urlHistoryForward.front();
    }
    SVNRev r = GetRevision();
    m_barRepository.ShowUrl(url, r);
    ChangeToUrl(url, r, true);
    m_urlHistoryForward.pop_front();
    m_barRepository.ShowUrl(url, r);
}

void CRepositoryBrowser::GetStatus()
{
    if (!m_bSparseCheckoutMode || m_wcPath.IsEmpty())
        return;

    CTSVNPath            retPath;
    SVNStatus            status(m_pbCancel, true);
    svn_client_status_t* s = nullptr;
    s                      = status.GetFirstFileStatus(m_wcPath, retPath, false, svn_depth_infinity, true, true);
    while ((s) && (!m_pbCancel))
    {
        CStringA url = s->repos_root_url;
        url += '/';
        url += s->repos_relpath;
        m_wcDepths[CUnicodeUtils::GetUnicode(url)] = s->depth;

        s = status.GetNextFileStatus(retPath);
    }
}

bool CRepositoryBrowser::RunStartCommit(const CTSVNPathList& pathlist, CString& sLogMsg) const
{
    DWORD   exitCode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(CTSVNPath(m_initialUrl), m_projectProperties);
    if (CHooks::Instance().StartCommit(GetSafeHwnd(), pathlist, sLogMsg, exitCode, error))
    {
        if (exitCode)
        {
            CString sErrorMsg;
            sErrorMsg.Format(IDS_ERR_HOOKFAILED, static_cast<LPCWSTR>(error));

            CTaskDialog taskDlg(sErrorMsg,
                                CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
            taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
            taskDlg.SetDefaultCommandControl(1);
            taskDlg.SetMainIcon(TD_ERROR_ICON);
            return (taskDlg.DoModal(GetSafeHwnd()) == 200);
        }
    }
    return true;
}

bool CRepositoryBrowser::RunPreCommit(const CTSVNPathList& pathlist, svn_depth_t depth, CString& sMsg) const
{
    DWORD   exitCode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(CTSVNPath(m_initialUrl), m_projectProperties);
    if (CHooks::Instance().PreCommit(GetSafeHwnd(), pathlist, depth, sMsg, exitCode, error))
    {
        if (exitCode)
        {
            CString sErrorMsg;
            sErrorMsg.Format(IDS_ERR_HOOKFAILED, static_cast<LPCWSTR>(error));

            CTaskDialog taskDlg(sErrorMsg,
                                CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
            taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
            taskDlg.SetDefaultCommandControl(1);
            taskDlg.SetMainIcon(TD_ERROR_ICON);
            return (taskDlg.DoModal(GetSafeHwnd()) == 100);
        }
    }
    return true;
}

bool CRepositoryBrowser::RunPostCommit(const CTSVNPathList& pathlist, svn_depth_t depth, svn_revnum_t revEnd, const CString& sMsg) const
{
    DWORD   exitCode = 0;
    CString error;
    CHooks::Instance().SetProjectProperties(CTSVNPath(m_initialUrl), m_projectProperties);
    if (CHooks::Instance().PostCommit(GetSafeHwnd(), pathlist, depth, revEnd, sMsg, exitCode, error))
    {
        if (exitCode)
        {
            CString temp;
            temp.Format(IDS_ERR_HOOKFAILED, static_cast<LPCWSTR>(error));
            ::MessageBox(GetSafeHwnd(), temp, L"TortoiseSVN", MB_ICONERROR);
            return false;
        }
    }
    return true;
}

HTREEITEM CRepositoryBrowser::FindBookmarkRoot() const
{
    HTREEITEM hRoot = m_repoTree.GetRootItem();
    while (hRoot)
    {
        CTreeItem* pItem = reinterpret_cast<CTreeItem*>(m_repoTree.GetItemData(hRoot));
        if (pItem && pItem->m_bookmark)
            return hRoot;
        hRoot = m_repoTree.GetNextItem(hRoot, TVGN_NEXT);
    }
    return nullptr;
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
        HTREEITEM hDummy = m_repoTree.InsertItem(L"", 0, 0, TVI_ROOT, TVI_LAST);
        //m_RepoTree.SetItemStateEx(hDummy, TVIS_EX_DISABLED);
        auto pDummy     = new CTreeItem();
        pDummy->m_dummy = true;
        pDummy->m_kind  = svn_node_dir;
        m_repoTree.SetItemData(hDummy, reinterpret_cast<DWORD_PTR>(pDummy));

        CTreeItem* pTreeItem  = new CTreeItem();
        pTreeItem->m_kind     = svn_node_dir;
        pTreeItem->m_bookmark = true;

        CString        sBook(MAKEINTRESOURCE(IDS_BOOKMARKS));
        TVINSERTSTRUCT tvInsert        = {nullptr};
        tvInsert.hParent               = TVI_ROOT;
        tvInsert.hInsertAfter          = TVI_LAST;
        tvInsert.itemex.mask           = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
        tvInsert.itemex.pszText        = sBook.GetBuffer();
        tvInsert.itemex.cChildren      = 1;
        tvInsert.itemex.lParam         = reinterpret_cast<LPARAM>(pTreeItem);
        tvInsert.itemex.iImage         = m_nBookmarksIcon;
        tvInsert.itemex.iSelectedImage = m_nBookmarksIcon;
        hBookmarkRoot                  = m_repoTree.InsertItem(&tvInsert);
        sBook.ReleaseBuffer();
    }
    if (hBookmarkRoot)
    {
        for (const auto& bm : m_bookmarks)
        {
            CString    bookmark   = bm.c_str();
            CTreeItem* pTreeItem  = new CTreeItem();
            pTreeItem->m_kind     = svn_node_file;
            pTreeItem->m_bookmark = true;
            pTreeItem->m_url      = bookmark;

            TVINSERTSTRUCT tvInsert        = {nullptr};
            tvInsert.hParent               = hBookmarkRoot;
            tvInsert.hInsertAfter          = TVI_LAST;
            tvInsert.itemex.mask           = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
            tvInsert.itemex.pszText        = bookmark.GetBuffer();
            tvInsert.itemex.cChildren      = 0;
            tvInsert.itemex.lParam         = reinterpret_cast<LPARAM>(pTreeItem);
            tvInsert.itemex.iImage         = m_nIconFolder;
            tvInsert.itemex.iSelectedImage = m_nOpenIconFolder;
            m_repoTree.InsertItem(&tvInsert);
            bookmark.ReleaseBuffer();
        }
    }
}

void CRepositoryBrowser::LoadBookmarks()
{
    m_bookmarks.clear();
    CString    sFilepath = CPathUtils::GetAppDataDirectory() + L"repobrowserbookmarks";
    CStdioFile file;
    if (file.Open(sFilepath, CFile::typeBinary | CFile::modeRead | CFile::shareDenyWrite))
    {
        CString strLine;
        while (file.ReadString(strLine))
        {
            m_bookmarks.insert(static_cast<LPCWSTR>(strLine));
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

CString CRepositoryBrowser::GetUrlWebViewerRev(CRepositoryBrowserSelection& selection)
{
    const SRepositoryInfo& repository = selection.GetRepository(0);
    CString                webUrl     = m_projectProperties.sWebViewerRev;
    webUrl                            = CAppUtils::GetAbsoluteUrlFromRelativeUrl(repository.root, webUrl);
    SVNRev headRev                    = selection.GetRepository(0).revision.IsHead() ? m_barRepository.GetHeadRevision() : selection.GetRepository(0).revision;
    webUrl.Replace(L"%REVISION%", headRev.ToString());
    return webUrl;
}

CString CRepositoryBrowser::GetUrlWebViewerPathRev(CRepositoryBrowserSelection& selection)
{
    const SRepositoryInfo& repository = selection.GetRepository(0);
    CString                relUrl     = selection.GetURLEscaped(0, 0).GetSVNPathString();
    relUrl                            = relUrl.Mid(repository.root.GetLength());
    CString webUrl                    = m_projectProperties.sWebViewerPathRev;
    webUrl                            = CAppUtils::GetAbsoluteUrlFromRelativeUrl(repository.root, webUrl);
    SVNRev headRev                    = selection.GetRepository(0).revision.IsHead() ? m_barRepository.GetHeadRevision() : selection.GetRepository(0).revision;
    webUrl.Replace(L"%REVISION%", headRev.ToString());
    webUrl.Replace(L"%PATH%", relUrl);
    return webUrl;
}
