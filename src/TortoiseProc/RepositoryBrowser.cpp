// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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

#include "MessageBox.h"
#include "InputLogDlg.h"
#include "PropDlg.h"
#include "EditPropertiesDlg.h"
#include "Blame.h"
#include "BlameDlg.h"
#include "WaitCursorEx.h"
#include "Repositorybrowser.h"
#include "BrowseFolder.h"
#include "RenameDlg.h"
#include "RevisionGraph\RevisionGraphDlg.h"
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
#include "XPTheme.h"
#include "IconMenu.h"
#include "SysInfo.h"
#include "Shlwapi.h"
#include "RepositoryBrowserSelection.h"


#define OVERLAY_EXTERNAL		1

enum RepoBrowserContextMenuCommands
{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	ID_OPEN = 1,
	ID_OPENWITH,
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
	ID_URLTOCLIPBOARD,
	ID_PROPS,
	ID_REVPROPS,
	ID_GNUDIFF,
	ID_DIFF,
	ID_PREPAREDIFF,
	ID_UPDATE,
	ID_CREATELINK,

};

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableStandAloneDialog)

CRepositoryBrowser::CRepositoryBrowser(const CString& url, const SVNRev& rev)
	: CResizableStandAloneDialog(CRepositoryBrowser::IDD, NULL)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_bStandAlone(true)
	, m_InitialUrl(url)
	, m_bInitDone(false)
	, m_blockEvents(false)
	, m_bSortAscending(true)
	, m_nSortedColumn(0)
	, m_pTreeDropTarget(NULL)
	, m_pListDropTarget(NULL)
	, m_diffKind(svn_node_none)
	, m_hAccel(NULL)
    , bDragMode(FALSE)
{
    m_repository.revision = rev;
}

CRepositoryBrowser::CRepositoryBrowser(const CString& url, const SVNRev& rev, CWnd* pParent)
	: CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_bStandAlone(false)
	, m_InitialUrl(url)
	, m_bInitDone(false)
	, m_blockEvents(false)
	, m_bSortAscending(true)
	, m_nSortedColumn(0)
	, m_pTreeDropTarget(NULL)
	, m_pListDropTarget(NULL)
	, m_diffKind(svn_node_none)
	, m_hAccel(NULL)
	, bDragMode(FALSE)
{
    m_repository.revision = rev;
}

CRepositoryBrowser::~CRepositoryBrowser()
{
}

void CRepositoryBrowser::RecursiveRemove(HTREEITEM hItem, bool bChildrenOnly /* = false */)
{
    // remove nodes from tree view

	HTREEITEM childItem;
	if (m_RepoTree.ItemHasChildren(hItem))
	{
		for (childItem = m_RepoTree.GetChildItem(hItem);childItem != NULL; childItem = m_RepoTree.GetNextItem(childItem, TVGN_NEXT))
		{
			RecursiveRemove(childItem);
			if (bChildrenOnly)
			{
				CTreeItem * pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(childItem);
				delete pTreeItem;
				m_RepoTree.SetItemData(childItem, 0);
				m_RepoTree.DeleteItem(childItem);
			}
		}
	}

	if ((hItem)&&(!bChildrenOnly))
	{
		CTreeItem * pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(hItem);
		delete pTreeItem;
		m_RepoTree.SetItemData(hItem, 0);
	}
}

void CRepositoryBrowser::ClearUI()
{
	RecursiveRemove (m_RepoTree.GetRootItem());

	m_RepoTree.DeleteAllItems();
	m_RepoList.DeleteAllItems();
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

	ExtendFrameIntoClientArea(0, 0, 0, IDC_REPOTREE);
	m_aeroControls.SubclassControl(GetDlgItem(IDC_F5HINT)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDCANCEL)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDOK)->GetSafeHwnd());
	m_aeroControls.SubclassControl(GetDlgItem(IDHELP)->GetSafeHwnd());

	GetWindowText(m_origDlgTitle);

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_REPOBROWSER));

	m_nExternalOvl = SYS_IMAGE_LIST().AddIcon((HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_EXTERNALOVL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	SYS_IMAGE_LIST().SetOverlayImage(m_nExternalOvl, OVERLAY_EXTERNAL);

	m_cnrRepositoryBar.SubclassDlgItem(IDC_REPOS_BAR_CNR, this);
	m_barRepository.Create(&m_cnrRepositoryBar, 12345);
	m_barRepository.SetIRepo(this);

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

	if (m_bStandAlone)
	{
		GetDlgItem(IDCANCEL)->ShowWindow(FALSE);

		// reposition the buttons
		CRect rect_cancel;
		GetDlgItem(IDCANCEL)->GetWindowRect(rect_cancel);
		ScreenToClient(rect_cancel);
		GetDlgItem(IDOK)->MoveWindow(rect_cancel);
	}

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_nOpenIconFolder = SYS_IMAGE_LIST().GetDirOpenIconIndex();
	// set up the list control
	// set the extended style of the list control
	// the style LVS_EX_FULLROWSELECT interferes with the background watermark image but it's more important to be able to select in the whole row.
	CRegDWORD regFullRowSelect(_T("Software\\TortoiseSVN\\FullRowSelect"), TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	m_RepoList.SetExtendedStyle(exStyle);
	m_RepoList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	m_RepoList.ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_INITWAIT)));

	m_RepoTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);

	CXPTheme theme;
	theme.SetWindowTheme(m_RepoList.GetSafeHwnd(), L"Explorer", NULL);
	theme.SetWindowTheme(m_RepoTree.GetSafeHwnd(), L"Explorer", NULL);


	AddAnchor(IDC_REPOS_BAR_CNR, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_F5HINT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_REPOTREE, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(IDC_REPOLIST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	EnableSaveRestore(_T("RepositoryBrowser"));
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	m_bThreadRunning = true;
	if (AfxBeginThread(InitThreadEntry, this)==NULL)
	{
		m_bThreadRunning = false;
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	return TRUE;
}

void CRepositoryBrowser::InitRepo()
{
	CWaitCursorEx wait;

    // stop running requests 
    // (reduce time needed to change the repository or revision)

    m_lister.Cancel();

    // repository properties

	m_InitialUrl = CPathUtils::PathUnescape(m_InitialUrl);
	int questionMarkIndex = -1;
	if ((questionMarkIndex = m_InitialUrl.Find('?'))>=0)
	{
		// url can be in the form
		// http://host/repos/path?[p=PEG][&r=REV]
		CString revString = m_InitialUrl.Mid(questionMarkIndex+1);
		if (revString.Find(_T('&'))>=0)
		{
			revString = revString.Mid(revString.Find('&')+1); // we don't support peg revisions for the url
		}
		revString.Trim(_T("r="));
		if (!revString.IsEmpty())
		{
			m_repository.revision = SVNRev(revString);
		}
		m_InitialUrl = m_InitialUrl.Left(questionMarkIndex);
	}

    m_repository.root 
        = GetRepositoryRootAndUUID (CTSVNPath (m_InitialUrl), m_repository.uuid);

    // let's (try to) access all levels in the folder path 

    if (!m_repository.root.IsEmpty())
        for ( CString path = m_InitialUrl
            ; path.GetLength() >= m_repository.root.GetLength()
            ; path = path.Left (path.ReverseFind ('/')))
        {
            m_lister.Enqueue (path, m_repository, true);
        }

    // (try to) fetch the HEAD revision

    svn_revnum_t headRevision = GetHEADRevision (CTSVNPath (m_InitialUrl));

    // let's see whether the URL was a directory 

    std::deque<CItem> dummy;
    CString error 
        = m_lister.GetList (m_InitialUrl, m_repository, true, dummy);

    // the only way CQuery::List will return the following error
    // is by calling it with a file path instead of a dir path

    CString wasFileError;
    wasFileError.LoadStringW (IDS_ERR_ERROR);

    // maybe, we already know that this was a (valid) file path

    if (error == wasFileError)
    {
	    m_InitialUrl = m_InitialUrl.Left (m_InitialUrl.ReverseFind ('/'));
        error = m_lister.GetList (m_InitialUrl, m_repository, true, dummy);
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
			    if ((m_InitialUrl.Compare(_T("http://")) == 0) ||
				    (m_InitialUrl.Compare(_T("https://")) == 0)||
				    (m_InitialUrl.Compare(_T("svn://")) == 0)||
				    (m_InitialUrl.Compare(_T("svn+ssh://")) == 0)||
				    (m_InitialUrl.Compare(_T("file:///")) == 0)||
				    (m_InitialUrl.Compare(_T("file://")) == 0))
			    {
				    m_InitialUrl.Empty();
			    }
			    if (error.IsEmpty())
			    {
				    if (((data)&&(data->kind == svn_node_dir))||(data == NULL))
					    error = info.GetLastErrorMsg();
			    }
		    }
	    } while(!m_InitialUrl.IsEmpty() && ((data == NULL) || (data->kind != svn_node_dir)));

	    if (data == NULL)
	    {
		    m_InitialUrl.Empty();
		    m_RepoList.ShowText(error, true);
		    return;
	    }
	    else if (m_repository.revision.IsHead())
	    {
		    m_barRepository.SetHeadRevision(data->rev);
	    }

        m_repository.root = data->reposRoot;
    	m_repository.uuid = data->reposUUID;
    }

    m_InitialUrl.TrimRight('/');
	m_repository.root = CPathUtils::PathUnescape (m_repository.root);

	// the initial url can be in the format file:///\, but the
	// repository root returned would still be file://
	// to avoid string length comparison faults, we adjust
	// the repository root here to match the initial url
	if ((m_InitialUrl.Left(9).CompareNoCase(_T("file:///\\")) == 0) &&
		(m_repository.root.Left(9).CompareNoCase(_T("file:///\\")) != 0))
		m_repository.root.Replace(_T("file://"), _T("file:///\\"));

	SetWindowText(m_repository.root + _T(" - ") + m_origDlgTitle);
	// now check the repository root for the url type, then
	// set the corresponding background image
	if (!m_repository.root.IsEmpty())
	{
		UINT nID = IDI_REPO_UNKNOWN;
        if (m_repository.root.Left(7).CompareNoCase(_T("http://"))==0)
			nID = IDI_REPO_HTTP;
		if (m_repository.root.Left(8).CompareNoCase(_T("https://"))==0)
			nID = IDI_REPO_HTTPS;
		if (m_repository.root.Left(6).CompareNoCase(_T("svn://"))==0)
			nID = IDI_REPO_SVN;
		if (m_repository.root.Left(10).CompareNoCase(_T("svn+ssh://"))==0)
			nID = IDI_REPO_SVNSSH;
		if (m_repository.root.Left(7).CompareNoCase(_T("file://"))==0)
			nID = IDI_REPO_FILE;

		CXPTheme theme;
		if (theme.IsAppThemed())
			CAppUtils::SetListCtrlBackgroundImage(m_RepoList.GetSafeHwnd(), nID);
	}
}

UINT CRepositoryBrowser::InitThreadEntry(LPVOID pVoid)
{
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
	DialogEnableWindow(IDCANCEL, FALSE);

	InitRepo();

	PostMessage(WM_AFTERINIT);
	DialogEnableWindow(IDOK, TRUE);
	DialogEnableWindow(IDCANCEL, TRUE);
	
	m_bThreadRunning = false;

	RefreshCursor();
	return 0;
}

LRESULT CRepositoryBrowser::OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_InitialUrl.IsEmpty())
		return 0;

	m_barRepository.SetRevision (m_repository.revision);
	m_barRepository.ShowUrl (m_InitialUrl, m_repository.revision);
    ChangeToUrl (m_InitialUrl, m_repository.revision, true);

	m_bInitDone = TRUE;
	return 0;
}

void CRepositoryBrowser::OnOK()
{
	RevokeDragDrop(m_RepoList.GetSafeHwnd());
	RevokeDragDrop(m_RepoTree.GetSafeHwnd());
	if (m_blockEvents)
		return;

	if (GetFocus() == &m_RepoList)
	{
		// list control has focus: 'enter' the folder

		if (m_RepoList.GetSelectedCount() != 1)
			return;

		POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
		OpenFromList (m_RepoList.GetNextSelectedItem (pos));
		return;
	}

	SaveColumnWidths(true);

    ClearUI();

	m_barRepository.SaveHistory();
	CResizableStandAloneDialog::OnOK();
}

void CRepositoryBrowser::OnCancel()
{
	RevokeDragDrop(m_RepoList.GetSafeHwnd());
	RevokeDragDrop(m_RepoTree.GetSafeHwnd());

	SaveColumnWidths(true);

    ClearUI();

	__super::OnCancel();
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
	if (pWnd == this)
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
	CDC * pDC;
	RECT rect, tree, list, treelist, treelistclient;

	if (bDragMode == FALSE)
		return;

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
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if ((nFlags & MK_LBUTTON) && (point.x != oldx))
	{
		pDC = GetDC();

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
	CDC * pDC;
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

	pDC = GetDC();
	DrawXorBar(pDC, point.x+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);
	ReleaseDC(pDC);

	oldx = point.x;
	oldy = point.y;

	CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
}

void CRepositoryBrowser::OnLButtonUp(UINT nFlags, CPoint point)
{
	CDC * pDC;
	RECT rect, tree, list, treelist, treelistclient;

	if (bDragMode == FALSE)
		return;

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

	pDC = GetDC();
	DrawXorBar(pDC, oldx+2, treelistclient.top, 4, treelistclient.bottom-treelistclient.top-2);			
	ReleaseDC(pDC);

	oldx = point.x;
	oldy = point.y;

	bDragMode = false;
	ReleaseCapture();

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

	CStandAloneDialogTmpl<CResizableDialog>::OnLButtonUp(nFlags, point);
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
          || (url.GetAt (root.GetLength()) != '/');

	CString partUrl = url;
	if ((LONG(rev) != LONG(m_repository.revision)) || urlHasDifferentRoot)
	{
		// if the revision changed, then invalidate everything
        ClearUI();
    	m_RepoList.ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_WAIT)), true);

        // if the repository root has changed, initialize all data from scratch
		// and clear the project properties we might have loaded previously
		m_ProjectProperties = ProjectProperties();
		m_InitialUrl = url;
        m_repository.revision = rev;
		InitRepo();

		if (m_repository.root.IsEmpty())
			return false;

        root = m_repository.root;
	}

	HTREEITEM hItem = AutoInsert (url);
	if (hItem == NULL)
		return false;

	CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hItem);
	if (pTreeItem == NULL)
		return FALSE;

	if (!m_RepoList.HasText())
		m_RepoList.ShowText(_T(" "), true);

	RefreshNode(hItem);
	m_RepoTree.Expand(hItem, TVE_EXPAND);
	FillList(&pTreeItem->children);

	m_blockEvents = true;
	m_RepoTree.EnsureVisible(hItem);
	m_RepoTree.SelectItem(hItem);
	m_blockEvents = false;

	m_RepoList.ClearText();

	return true;
}

void CRepositoryBrowser::FillList(deque<CItem> * pItems)
{
	if (pItems == NULL)
		return;

	CWaitCursorEx wait;
	m_RepoList.SetRedraw(false);
	m_RepoList.DeleteAllItems();
    m_RepoList.ClearText();

	int c = ((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_RepoList.DeleteColumn(c--);

	c = 0;
	CString temp;
	//
	// column 0: contains tree
	temp.LoadString(IDS_LOG_FILE);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 1: file extension
	temp.LoadString(IDS_STATUSLIST_COLEXT);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 2: revision number
	temp.LoadString(IDS_LOG_REVISION);
	m_RepoList.InsertColumn(c++, temp, LVCFMT_RIGHT);
	//
	// column 3: author
	temp.LoadString(IDS_LOG_AUTHOR);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 4: size
	temp.LoadString(IDS_LOG_SIZE);
	m_RepoList.InsertColumn(c++, temp, LVCFMT_RIGHT);
	//
	// column 5: date
	temp.LoadString(IDS_LOG_DATE);
	m_RepoList.InsertColumn(c++, temp);
	//
	// column 6: lock owner
	temp.LoadString(IDS_STATUSLIST_COLLOCK);
	m_RepoList.InsertColumn(c++, temp);

	// now fill in the data

	TCHAR date_native[SVN_DATE_BUFFER];
	int nCount = 0;
	for (deque<CItem>::const_iterator it = pItems->begin(); it != pItems->end(); ++it)
	{
		int icon_idx;
		if (it->kind == svn_node_dir)
			icon_idx = 	m_nIconFolder;
		else
			icon_idx = SYS_IMAGE_LIST().GetFileIconIndex(it->path);
		int index = m_RepoList.InsertItem(nCount, it->path, icon_idx);

		if (it->is_external)
			m_RepoList.SetItemState(index, INDEXTOOVERLAYMASK(OVERLAY_EXTERNAL), LVIS_OVERLAYMASK);

		// extension
		temp = CPathUtils::GetFileExtFromPath(it->path);
		if (it->kind == svn_node_file)
			m_RepoList.SetItemText(index, 1, temp);

		// revision
        if ((it->created_rev == SVN_IGNORED_REVNUM) && (it->is_external))
            temp = it->time != 0 ? _T("") : _T("HEAD");
        else
		    temp.Format(_T("%ld"), it->created_rev);
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
        if ((it->time == 0) && (it->is_external))
            date_native[0] = 0;
        else
    		SVN::formatDate(date_native, (apr_time_t&)it->time, true);
		m_RepoList.SetItemText(index, 5, date_native);

		// lock owner
		m_RepoList.SetItemText(index, 6, it->lockowner);

        // pointer to the CItem structure
		m_RepoList.SetItemData(index, (DWORD_PTR)&(*it));
	}

	ListView_SortItemsEx(m_RepoList, ListSort, this);
	SetSortArrow();

	for (int col = 0; col <= (((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1); col++)
	{
		m_RepoList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
	}
	for (int col = 0; col <= (((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1); col++)
	{
		m_arColumnAutoWidths[col] = m_RepoList.GetColumnWidth(col);
	}

	CRegString regColWidths(_T("Software\\TortoiseSVN\\RepoBrowserColumnWidth"));
	if (!CString(regColWidths).IsEmpty())
	{
		StringToWidthArray(regColWidths, m_arColumnWidths);
		
		int maxcol = ((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1;
		for (int col = 1; col <= maxcol; col++)
		{
			if (m_arColumnWidths[col] == 0)
				m_RepoList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
			else
				m_RepoList.SetColumnWidth(col, m_arColumnWidths[col]);
		}
	}

	m_RepoList.SetRedraw(true);
}

HTREEITEM CRepositoryBrowser::FindUrl (const CString& fullurl)
{
	return FindUrl (fullurl, fullurl, TVI_ROOT);
}

HTREEITEM CRepositoryBrowser::FindUrl(const CString& fullurl, const CString& url, HTREEITEM hItem /* = TVI_ROOT */)
{
    CString root = m_repository.root;

	if (hItem == TVI_ROOT)
	{
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
			if (pTItem)
			{
				CString sSibling = pTItem->unescapedname;
				if (sSibling.Compare(url.Left(sSibling.GetLength()))==0)
				{
					if (sSibling.GetLength() == url.GetLength())
						return hSibling;
					if (url.GetAt(sSibling.GetLength()) == '/')
						return FindUrl(fullurl, url.Mid(sSibling.GetLength()+1), hSibling);
				}
			}
		} while ((hSibling = m_RepoTree.GetNextItem(hSibling, TVGN_NEXT)) != NULL);	
	}

    return NULL;
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
            currentPath = m_repository.root;
            name = m_repository.root;
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
	        node = m_RepoTree.GetRootItem();
	        if (node == NULL)
	        {
		        // the tree view is empty, just fill in the repository root
		        CTreeItem * pTreeItem = new CTreeItem();
		        pTreeItem->unescapedname = name;
                pTreeItem->url = currentPath;
                pTreeItem->logicalPath = currentPath;
                pTreeItem->repository = m_repository;

		        TVINSERTSTRUCT tvinsert = {0};
		        tvinsert.hParent = TVI_ROOT;
		        tvinsert.hInsertAfter = TVI_ROOT;
		        tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		        tvinsert.itemex.pszText = currentPath.GetBuffer (currentPath.GetLength());
		        tvinsert.itemex.cChildren = 1;
		        tvinsert.itemex.lParam = (LPARAM)pTreeItem;
		        tvinsert.itemex.iImage = m_nIconFolder;
		        tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;

		        node = m_RepoTree.InsertItem(&tvinsert);
		        currentPath.ReleaseBuffer();
	        }
        }
        else
        {
            // get all children

        	CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData (node);
            std::deque<CItem>& children = pTreeItem->children;

            if (!pTreeItem->children_fetched)
            {
                // not yet fetched -> do it now

                CString error = m_lister.GetList ( pTreeItem->url
                                                 , pTreeItem->repository
                                                 , true
                                                 , children);
                if (!error.IsEmpty())
                    return NULL;

                for (size_t i = 0, count = children.size(); i < count; ++i)
                    if (children[i].kind == svn_node_dir)
                    {
                        pTreeItem->has_child_folders = true;
                        break;
                    }

                // since we don't add *all* sub-nodes, 
                // mark parent as 'incomplete'
                    
                pTreeItem->children_fetched = false;
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
                return NULL;

            // auto-add the node for the current path level

            node = AutoInsert (node, *item);
        }

        // pre-fetch data for the next level

    	CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData (node);
        if ((pTreeItem != NULL) && !pTreeItem->children_fetched)
            m_lister.Enqueue ( pTreeItem->url
                             , pTreeItem->repository
                             , true);
    }
    while (currentPath != path);

    // done

	return node;
}

HTREEITEM CRepositoryBrowser::AutoInsert (HTREEITEM hParent, const CItem& item)
{
    // look for an existing sub-node by comparing names

	for ( HTREEITEM hSibling = m_RepoTree.GetNextItem (hParent, TVGN_CHILD)
        ; hSibling != NULL
        ; hSibling = m_RepoTree.GetNextItem (hSibling, TVGN_NEXT))
    {
		CTreeItem * pTreeItem = ((CTreeItem*)m_RepoTree.GetItemData (hSibling));
        if (pTreeItem && (pTreeItem->unescapedname == item.path))
            return hSibling;
    }

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
        if (items[i].kind == svn_node_dir)
            newItems.insert (std::make_pair (items[i].path, &items[i]));

	for ( HTREEITEM hSibling = m_RepoTree.GetNextItem (hParent, TVGN_CHILD)
        ; hSibling != NULL
        ; hSibling = m_RepoTree.GetNextItem (hSibling, TVGN_NEXT))
    {
		CTreeItem * pTreeItem = ((CTreeItem*)m_RepoTree.GetItemData (hSibling));
        if (pTreeItem)
            newItems.erase (pTreeItem->unescapedname);
    }

    if (newItems.empty())
        return;

    // Create missing sub-nodes

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
	CTreeItem* pTreeItem = new CTreeItem();
    CString name = item.path;

    pTreeItem->unescapedname = name;
    pTreeItem->url = item.absolutepath;
    pTreeItem->logicalPath = parentTreeItem->logicalPath + '/' + name;
    pTreeItem->repository = item.repository;
    pTreeItem->is_external = item.is_external;

    bool isSelectedForDiff 
        = pTreeItem->url.CompareNoCase (m_diffURL.GetSVNPathString()) == 0;

    UINT state = isSelectedForDiff ? TVIS_BOLD : 0;
    UINT stateMask = state;
    if (item.is_external)
    {
        state |= INDEXTOOVERLAYMASK (OVERLAY_EXTERNAL);
        stateMask |= TVIS_OVERLAYMASK;
    }

	TVINSERTSTRUCT tvinsert = {0};
	tvinsert.hParent = hParent;
	tvinsert.hInsertAfter = TVI_SORT;
	tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvinsert.itemex.state = state;
	tvinsert.itemex.stateMask = stateMask;
	tvinsert.itemex.pszText = name.GetBuffer (name.GetLength());
	tvinsert.itemex.cChildren = 1;
	tvinsert.itemex.lParam = (LPARAM)pTreeItem;
    tvinsert.itemex.iImage = m_nIconFolder;
	tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;

	HTREEITEM hNewItem = m_RepoTree.InsertItem(&tvinsert);
	name.ReleaseBuffer();

    return hNewItem;
}

void CRepositoryBrowser::Sort (HTREEITEM parent)
{
	TVSORTCB tvs;
	tvs.hParent = parent;
	tvs.lpfnCompare = TreeSort;
	tvs.lParam = (LPARAM)this;

	m_RepoTree.SortChildrenCB (&tvs);
}

void CRepositoryBrowser::RefreshChildren (CTreeItem * pTreeItem)
{
	if (pTreeItem == NULL)
		return;

	pTreeItem->children.clear();
	pTreeItem->has_child_folders = false;

    CString error = m_lister.GetList ( pTreeItem->url
                                     , pTreeItem->repository
                                     , true
                                     , pTreeItem->children);
    if (!error.IsEmpty())
	{
		// error during list()
        m_RepoList.DeleteAllItems();
		m_RepoList.ShowText (error, true);
		return;
	}

    // update node status and add sub-nodes for all sub-dirs

	pTreeItem->children_fetched = true;
    for (size_t i = 0, count = pTreeItem->children.size(); i < count; ++i)
    {
        const CItem& item = pTreeItem->children[i];
        if (item.kind == svn_node_dir)
        {
            pTreeItem->has_child_folders = true;
            m_lister.Enqueue ( item.absolutepath
                             , item.repository
                             , item.has_props);
        }
    }
}

bool CRepositoryBrowser::RefreshNode(HTREEITEM hNode, bool force /* = false*/)
{
	if (hNode == NULL)
		return false;

	SaveColumnWidths();
	CWaitCursorEx wait;
	CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hNode);
	HTREEITEM hSel1 = m_RepoTree.GetSelectedItem();
	if (m_RepoTree.ItemHasChildren(hNode))
	{
		HTREEITEM hChild = m_RepoTree.GetChildItem(hNode);
		HTREEITEM hNext;
		m_blockEvents = true;
		while (hChild)
		{
			hNext = m_RepoTree.GetNextItem(hChild, TVGN_NEXT);
			RecursiveRemove(hChild);
			m_RepoTree.DeleteItem(hChild);
			hChild = hNext;
		}
		m_blockEvents = false;
	}
	if (pTreeItem == NULL)
		return false;

    RefreshChildren (pTreeItem);

    if (pTreeItem->has_child_folders)
        AutoInsert (hNode, pTreeItem->children);

	// if there are no child folders, remove the '+' in front of the node
	{
		TVITEM tvitem = {0};
		tvitem.hItem = hNode;
		tvitem.mask = TVIF_CHILDREN;
		tvitem.cChildren = pTreeItem->has_child_folders ? 1 : 0;
		m_RepoTree.SetItem(&tvitem);
	}
    if (pTreeItem->children_fetched)
	    if ((force)||(hSel1 == hNode)||(hSel1 != m_RepoTree.GetSelectedItem()))
	    {
		    FillList(&pTreeItem->children);
	    }

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
			// Do a direct translation.
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
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
	CTSVNPathList urlList;
    std::vector<SRepositoryInfo> repositories;
	bool bTreeItem = false;

	POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	int index = -1;
	while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
	{
		CItem * pItem = (CItem *)m_RepoList.GetItemData(index);
		CString absPath = pItem->absolutepath;
		absPath.Replace(_T("\\"), _T("%5C"));
		urlList.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(absPath))));
        repositories.push_back (pItem->repository);
	}
	if ((urlList.GetCount() == 0))
	{
		HTREEITEM hItem = m_RepoTree.GetSelectedItem();
		CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hItem);
		if (pTreeItem)
		{
			urlList.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(pTreeItem->url))));
            repositories.push_back (pTreeItem->repository);
			bTreeItem = true;
		}
	}

	if (urlList.GetCount() == 0)
		return;


	CWaitCursorEx wait_cursor;
	CInputLogDlg input(this);
    input.SetUUID (repositories[0].uuid);
	input.SetProjectProperties(&m_ProjectProperties);
	CString hint;
	if (urlList.GetCount() == 1)
		hint.Format(IDS_INPUT_REMOVEONE, (LPCTSTR)urlList[0].GetFileOrDirectoryName());
	else
		hint.Format(IDS_INPUT_REMOVEMORE, urlList.GetCount());
	input.SetActionText(hint);
	if (input.DoModal() == IDOK)
	{
		if (!Remove(urlList, true, false, input.GetLogMessage()))
		{
			wait_cursor.Hide();
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}
		if (bTreeItem)
			RefreshNode(m_RepoTree.GetParentItem(m_RepoTree.GetSelectedItem()), true);
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
	int index = -1;
	while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
	{
		CItem * pItem = (CItem *)m_RepoList.GetItemData(index);
		url += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(pItem->absolutepath)));
		if (!GetRevision().IsHead())
		{
			url += _T("?r=") + GetRevision().ToString();
		}
		url += _T("\r\n");
	}
	if (!url.IsEmpty())
	{
		url.TrimRight(_T("\r\n"));
		CStringUtils::WriteAsciiStringToClipboard(url);
	}
}

void CRepositoryBrowser::OnInlineedit()
{
	POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	int selIndex = m_RepoList.GetNextSelectedItem(pos);
	m_blockEvents = true;
	if (selIndex >= 0)
	{
	CItem * pItem = (CItem *)m_RepoList.GetItemData(selIndex);
        if (!pItem->is_external)
        {
		    m_RepoList.SetFocus();
		    m_RepoList.EditLabel(selIndex);
        }
	}
	else
	{
		m_RepoTree.SetFocus();
		HTREEITEM hTreeItem = m_RepoTree.GetSelectedItem();
		if (hTreeItem != m_RepoTree.GetRootItem())
        {
    		CTreeItem* pItem = (CTreeItem*)m_RepoTree.GetItemData (hTreeItem);
            if (!pItem->is_external)
    			m_RepoTree.EditLabel(hTreeItem);
        }
	}
	m_blockEvents = false;
}

void CRepositoryBrowser::OnRefresh()
{
    CWaitCursorEx waitCursor;

    m_blockEvents = true;

    bool fullPrefetch = !!(GetKeyState(VK_CONTROL) & 0x8000);

    // try to get the tree node to refresh

    HTREEITEM hSelected = m_RepoTree.GetSelectedItem();
    if (hSelected == NULL)
        hSelected = m_RepoTree.GetRootItem();

    if (hSelected == NULL)
    {
        // empty -> try re-init

        InitRepo();
        return;
    }

    // invalidate the cache

    InvalidateData (hSelected);

    // prefetch the whole sub-tree?

    CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData (hSelected);
    if (fullPrefetch && (pItem != NULL))
    {
        // initialize crawler with current tree node

        typedef std::pair<CString, SRepositoryInfo> TEntry;
        std::deque<TEntry> urls;

        TEntry initial (pItem->url, pItem->repository);
        urls.push_back (initial);
        m_lister.Enqueue (pItem->url, initial.second, true);

        // breadth-first. 
        // This should maximize the interval between enqueueing 
        // the request and fetching its result

        while (!urls.empty())
        {
            // extract next url

            TEntry entry = urls.front();
            urls.pop_front();

            // enqueue sub-nodes for listing

            std::deque<CItem> children;
            m_lister.GetList (entry.first, entry.second, true, children);

            for (size_t i = 0, count = children.size(); i < count; ++i)
            {
                const CItem& item = children[i];
                const CString& url = item.absolutepath;

                if (item.kind == svn_node_dir)
                {
                    urls.push_back (TEntry (url, item.repository));
                    m_lister.Enqueue ( url
                                     , item.repository
                                     , item.has_props);
                }
            }
        }
    }

    // refresh the current node

    RefreshNode(m_RepoTree.GetSelectedItem(), true);

    m_blockEvents = false;
}

void CRepositoryBrowser::OnTvnSelchangedRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	if (m_blockEvents)
		return;

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
		HTREEITEM hSelItem = m_RepoTree.GetSelectedItem();
		if (hSelItem)
		{
			CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hSelItem);
			if (pTreeItem)
			{
				if (!pTreeItem->children_fetched)
				{
                    m_RepoList.DeleteAllItems();
					RefreshNode(hSelItem);
				}
                else
                {
    				FillList(&pTreeItem->children);
                }

                m_barRepository.ShowUrl ( pTreeItem->url
                                        , pTreeItem->repository.revision);
			}
		}
	}

	__super::OnTimer(nIDEvent);
}

void CRepositoryBrowser::OnTvnItemexpandingRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	if (m_blockEvents)
		return;

	CTreeItem * pTreeItem = (CTreeItem *)pNMTreeView->itemNew.lParam;

	if (pTreeItem == NULL)
		return;

	if (pNMTreeView->action == TVE_COLLAPSE)
	{
		// user wants to collapse a tree node.
		// if we don't know anything about the children
		// of the node, we suppress the collapsing but fetch the info instead
		if (!pTreeItem->children_fetched)
		{
			RefreshNode(pNMTreeView->itemNew.hItem);
			*pResult = 1;
			return;
		}
		return;
	}

	// user wants to expand a tree node.
	// check if we already know its children - if not we have to ask the repository!

	if (!pTreeItem->children_fetched)
	{
		RefreshNode(pNMTreeView->itemNew.hItem);
	}
	else
	{
		// if there are no child folders, remove the '+' in front of the node
		if (!pTreeItem->has_child_folders)
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
    CWaitCursorEx wait;

	LPNMITEMACTIVATE pNmItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	if (m_blockEvents)
		return;

	if (pNmItemActivate->iItem < 0)
		return;

    OpenFromList (pNmItemActivate->iItem);
}

void CRepositoryBrowser::OpenFromList (int item)
{
    if (item < 0)
        return;

    CWaitCursorEx wait;

    CItem * pItem = (CItem*)m_RepoList.GetItemData (item);
	if ((pItem) && (pItem->kind == svn_node_dir))
	{
        // a double click on a folder results in selecting that folder

        HTREEITEM hItem = AutoInsert (m_RepoTree.GetSelectedItem(), *pItem);

        m_blockEvents = true;
	    m_RepoTree.EnsureVisible (hItem);
	    m_RepoTree.SelectItem (hItem);
        RefreshNode (hItem);
	    m_blockEvents = false;
	}
	else if ((pItem) && (pItem->kind == svn_node_file))
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
	progDlg.SetAnimation(IDR_DOWNLOAD);
	CString sInfoLine;
	sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url.GetFileOrDirectoryName(), (LPCTSTR)revision.ToString());
	progDlg.SetLine(1, sInfoLine, true);
	SetAndClearProgressInfo(&progDlg);
	progDlg.ShowModeless(m_hWnd);
	if (!Cat(urlEscaped, revision, revision, tempfile))
	{
		progDlg.Stop();
		SetAndClearProgressInfo((HWND)NULL);
		wait_cursor.Hide();
		CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}
	progDlg.Stop();
	SetAndClearProgressInfo((HWND)NULL);
	// set the file as read-only to tell the app which opens the file that it's only
	// a temporary file and must not be edited.
	SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	if (!bOpenWith)
	{
		int ret = (int)ShellExecute(NULL, _T("open"), tempfile.GetWinPathString(), NULL, NULL, SW_SHOWNORMAL);
		if (ret <= HINSTANCE_ERROR)
			bOpenWith = true;
	}
	else
	{
		CString c = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
		c += tempfile.GetWinPathString() + _T(" ");
		CAppUtils::LaunchApplication(c, NULL, false);
	}
}

void CRepositoryBrowser::OnHdnItemclickRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// a click on a header means sorting the items
	if (m_nSortedColumn != phdr->iItem)
		m_bSortAscending = true;
	else
		m_bSortAscending = !m_bSortAscending;
	m_nSortedColumn = phdr->iItem;

	m_blockEvents = true;
	ListView_SortItemsEx(m_RepoList, ListSort, this);
	SetSortArrow();
	m_blockEvents = false;
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
	case 0:	// filename
		nRet = StrCmpLogicalW(pItem1->path, pItem2->path);
		if (nRet != 0)
			break;
		// fall through
	case 1: // extension
		nRet = pThis->m_RepoList.GetItemText(static_cast<int>(lParam1), 1)
                 .CompareNoCase(pThis->m_RepoList.GetItemText(static_cast<int>(lParam2), 1));
		if (nRet == 0)	// if extensions are the same, use the filename to sort
			nRet = StrCmpLogicalW(pItem1->path, pItem2->path);
		if (nRet != 0)
			break;
		// fall through
	case 2: // revision number
		nRet = pItem1->created_rev - pItem2->created_rev;
		if (nRet == 0)	// if extensions are the same, use the filename to sort
			nRet = StrCmpLogicalW(pItem1->path, pItem2->path);
		if (nRet != 0)
			break;
		// fall through
	case 3: // author
		nRet = pItem1->author.CompareNoCase(pItem2->author);
		if (nRet == 0)	// if extensions are the same, use the filename to sort
			nRet = StrCmpLogicalW(pItem1->path, pItem2->path);
		if (nRet != 0)
			break;
		// fall through
	case 4: // size
		nRet = int(pItem1->size - pItem2->size);
		if (nRet == 0)	// if extensions are the same, use the filename to sort
			nRet = StrCmpLogicalW(pItem1->path, pItem2->path);
		if (nRet != 0)
			break;
		// fall through
	case 5: // date
		nRet = (pItem1->time - pItem2->time) > 0 ? 1 : -1;
		if (nRet == 0)	// if extensions are the same, use the filename to sort
			nRet = StrCmpLogicalW(pItem1->path, pItem2->path);
		if (nRet != 0)
			break;
		// fall through
	case 6: // lock owner
		nRet = pItem1->lockowner.CompareNoCase(pItem2->lockowner);
		if (nRet == 0)	// if extensions are the same, use the filename to sort
			nRet = StrCmpLogicalW(pItem1->path, pItem2->path);
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
	return StrCmpLogicalW(Item1->unescapedname, Item2->unescapedname);
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
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_blockEvents)
		return;
	if (m_RepoList.HasText())
		return;
	if (pNMLV->uChanged & LVIF_STATE)
	{
		if (pNMLV->uNewState & LVIS_SELECTED)
		{
			CItem * pItem = (CItem*)m_RepoList.GetItemData(pNMLV->iItem);
			if (pItem)
				m_barRepository.ShowUrl ( pItem->absolutepath
                                        , pItem->repository.revision);
		}
	}
}

void CRepositoryBrowser::OnLvnBeginlabeleditRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *info = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

    // disable rename for externals
	CItem * item = (CItem *)m_RepoList.GetItemData (info->item.iItem);
    *pResult = (item == NULL) || (item->is_external)
             ? TRUE
             : FALSE;
}

void CRepositoryBrowser::OnLvnEndlabeleditRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = 0;
	if (pDispInfo->item.pszText == NULL)
		return;
	// rename the item in the repository
	CItem * pItem = (CItem *)m_RepoList.GetItemData(pDispInfo->item.iItem);

	CInputLogDlg input(this);
    input.SetUUID (pItem->repository.uuid);
	input.SetProjectProperties(&m_ProjectProperties);
	CTSVNPath targetUrl = CTSVNPath(EscapeUrl(CTSVNPath(pItem->absolutepath.Left(pItem->absolutepath.ReverseFind('/')+1)+pDispInfo->item.pszText)));
	if (!targetUrl.IsValidOnWindows())
	{
		if (CMessageBox::Show(GetSafeHwnd(), IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONINFORMATION|MB_YESNO) != IDYES)
			return;
	}
	CString sHint;
	sHint.Format(IDS_INPUT_RENAME, (LPCTSTR)(pItem->absolutepath), (LPCTSTR)targetUrl.GetSVNPathString());
	input.SetActionText(sHint);
    input.SetForceFocus (true);
	if (input.DoModal() == IDOK)
	{
    	CWaitCursorEx wait_cursor;

		if (!Move(CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(pItem->absolutepath)))),
			targetUrl,
			true, input.GetLogMessage()))
		{
			wait_cursor.Hide();
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}
		*pResult = 0;

        InvalidateData (m_RepoTree.GetSelectedItem());
		RefreshNode(m_RepoTree.GetSelectedItem(), true);
	}
}

void CRepositoryBrowser::OnTvnBeginlabeleditRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTVDISPINFO* info = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);

    // disable rename for externals
	CTreeItem* item = (CTreeItem *)m_RepoTree.GetItemData (info->item.hItem);
    *pResult = (item == NULL) || (item->is_external)
             ? TRUE
             : FALSE;
}

void CRepositoryBrowser::OnTvnEndlabeleditRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	*pResult = 0;
	if (pTVDispInfo->item.pszText == NULL)
		return;

	// rename the item in the repository
	HTREEITEM hSelectedItem = pTVDispInfo->item.hItem;
	CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData(hSelectedItem);
	if (pItem == NULL)
		return;

    SRepositoryInfo repository = pItem->repository;

	CInputLogDlg input(this);
	input.SetUUID(pItem->repository.uuid);
	input.SetProjectProperties(&m_ProjectProperties);
	CTSVNPath targetUrl = CTSVNPath(EscapeUrl(CTSVNPath(pItem->url.Left(pItem->url.ReverseFind('/')+1)+pTVDispInfo->item.pszText)));
	if (!targetUrl.IsValidOnWindows())
	{
		if (CMessageBox::Show(GetSafeHwnd(), IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONINFORMATION|MB_YESNO) != IDYES)
			return;
	}
	CString sHint;
	sHint.Format(IDS_INPUT_RENAME, (LPCTSTR)(pItem->url), (LPCTSTR)targetUrl.GetSVNPathString());
	input.SetActionText(sHint);
    input.SetForceFocus (true);
	if (input.DoModal() == IDOK)
	{
        CWaitCursorEx wait_cursor;

		if (!Move(CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(pItem->url)))),
			targetUrl,
			true, input.GetLogMessage()))
		{
			wait_cursor.Hide();
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}

        HTREEITEM parent = m_RepoTree.GetParentItem (hSelectedItem);
        InvalidateData (parent);

        *pResult = TRUE;
		if (pItem->url.Compare(m_barRepository.GetCurrentUrl()) == 0)
		{
			m_barRepository.ShowUrl(targetUrl.GetSVNPathString(), m_barRepository.GetCurrentRev());
		}

        pItem->url = targetUrl.GetSVNPathString();
		pItem->unescapedname = pTVDispInfo->item.pszText;
        pItem->repository = repository;
		m_RepoTree.SetItemData(hSelectedItem, (DWORD_PTR)pItem);

        RefreshChildren ((CTreeItem*)m_RepoTree.GetItemData (parent));

        if (hSelectedItem == m_RepoTree.GetSelectedItem())
		    RefreshNode (hSelectedItem, true);
	}
}

void CRepositoryBrowser::OnLvnBeginrdragRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_bRightDrag = true;
	*pResult = 0;
	OnBeginDrag(pNMHDR);
}

void CRepositoryBrowser::OnLvnBegindragRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_bRightDrag = false;
	*pResult = 0;
	OnBeginDrag(pNMHDR);
}

void CRepositoryBrowser::OnBeginDrag(NMHDR *pNMHDR)
{
    // get selected paths

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (m_RepoList.HasText())
		return;

    CRepositoryBrowserSelection selection;

    POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	int index = -1;
	while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
        selection.Add ((CItem *)m_RepoList.GetItemData(index));

    BeginDrag(m_RepoList, selection, pNMLV->ptAction, true);
}

void CRepositoryBrowser::OnTvnBegindragRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_bRightDrag = false;
	*pResult = 0;
	OnBeginDragTree(pNMHDR);
}

void CRepositoryBrowser::OnTvnBeginrdragRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_bRightDrag = true;
	*pResult = 0;
	OnBeginDragTree(pNMHDR);
}

void CRepositoryBrowser::OnBeginDragTree(NMHDR *pNMHDR)
{
    // get selected paths

	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	if (m_blockEvents)
		return;

    CRepositoryBrowserSelection selection;
    selection.Add ((CTreeItem *)pNMTreeView->itemNew.lParam);

	BeginDrag(m_RepoTree, selection, pNMTreeView->ptDrag, false);
}

bool CRepositoryBrowser::OnDrop(const CTSVNPath& target, const CString& root, const CTSVNPathList& pathlist, const SVNRev& srcRev, DWORD dwEffect, POINTL /*pt*/)
{
	ATLTRACE(_T("dropped %ld items on %s, source revision is %s, dwEffect is %ld\n"), pathlist.GetCount(), (LPCTSTR)target.GetSVNPathString(), srcRev.ToString(), dwEffect);
	if (pathlist.GetCount() == 0)
		return false;

    if (!CTSVNPath(root).IsAncestorOf (pathlist[0]))
        return false;

	CString targetName = pathlist[0].GetFileOrDirectoryName();
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
			CString temp(MAKEINTRESOURCE(IDS_REPOBROWSE_COPYDROP));
			popup.AppendMenu(MF_STRING | MF_ENABLED, 1, temp);
			temp.LoadString(IDS_REPOBROWSE_MOVEDROP);
			popup.AppendMenu(MF_STRING | MF_ENABLED, 2, temp);
			if ((pathlist.GetCount() == 1)&&(PathIsURL(pathlist[0])))
			{
				// these entries are only shown if *one* item was dragged, and if the
				// item is not one dropped from e.g. the explorer but from the repository
				// browser itself.
				popup.AppendMenu(MF_SEPARATOR, 3);
				temp.LoadString(IDS_REPOBROWSE_COPYRENAMEDROP);
				popup.AppendMenu(MF_STRING | MF_ENABLED, 4, temp);
				temp.LoadString(IDS_REPOBROWSE_MOVERENAMEDROP);
				popup.AppendMenu(MF_STRING | MF_ENABLED, 5, temp);
			}
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pt.x, pt.y, this, 0);
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
					CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
					if (dlg.DoModal() != IDOK)
						return false;
					targetName = dlg.m_name;
					if (!CTSVNPath(targetName).IsValidOnWindows())
					{
						if (CMessageBox::Show(GetSafeHwnd(), IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONINFORMATION|MB_YESNO) != IDYES)
							return false;
					}
				}
				break;
			case 5: // move rename drop
				{
					dwEffect = DROPEFFECT_MOVE;
					CRenameDlg dlg;
					dlg.m_name = targetName;
					dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_RENAME);
					CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
					if (dlg.DoModal() != IDOK)
						return false;
					targetName = dlg.m_name;
					if (!CTSVNPath(targetName).IsValidOnWindows())
					{
						if (CMessageBox::Show(GetSafeHwnd(), IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONINFORMATION|MB_YESNO) != IDYES)
							return false;
					}
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
				UINT msgResult = CMessageBox::Show(GetSafeHwnd(), IDS_WARN_CONFIRM_MOVE_SPECIAL_DIRECTORY, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO);
				if (IDYES != msgResult)
				{
					return false;
				}
			}
		}

		// drag-n-drop inside the repobrowser
		CInputLogDlg input(this);
		input.SetUUID(m_repository.uuid);
		input.SetProjectProperties(&m_ProjectProperties);
		CString sHint;
		if (pathlist.GetCount() == 1)
		{
			if (dwEffect == DROPEFFECT_COPY)
				sHint.Format(IDS_INPUT_COPY, (LPCTSTR)pathlist[0].GetSVNPathString(), (LPCTSTR)(target.GetSVNPathString()+_T("/")+targetName));
			else
				sHint.Format(IDS_INPUT_MOVE, (LPCTSTR)pathlist[0].GetSVNPathString(), (LPCTSTR)(target.GetSVNPathString()+_T("/")+targetName));
		}
		else
		{
			if (dwEffect == DROPEFFECT_COPY)
				sHint.Format(IDS_INPUT_COPYMORE, pathlist.GetCount(), (LPCTSTR)target.GetSVNPathString());
			else
				sHint.Format(IDS_INPUT_MOVEMORE, pathlist.GetCount(), (LPCTSTR)target.GetSVNPathString());
		}
		input.SetActionText(sHint);
		if (input.DoModal() == IDOK)
		{
			CWaitCursorEx wait_cursor;
			BOOL bRet = FALSE;
			if (dwEffect == DROPEFFECT_COPY)
				if (pathlist.GetCount() == 1)
					bRet = Copy(pathlist, CTSVNPath(target.GetSVNPathString() + _T("/") + targetName), srcRev, srcRev, input.GetLogMessage(), false);
				else
					bRet = Copy(pathlist, target, srcRev, srcRev, input.GetLogMessage(), true);
			else
				if (pathlist.GetCount() == 1)
					bRet = Move(pathlist, CTSVNPath(target.GetSVNPathString() + _T("/") + targetName), TRUE, input.GetLogMessage(), false);
				else
					bRet = Move(pathlist, target, TRUE, input.GetLogMessage(), true);
			if (!bRet)
			{
				wait_cursor.Hide();
				CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			else if (GetRevision().IsHead())
			{
				// mark the target as dirty
				HTREEITEM hTarget = FindUrl(target.GetSVNPathString());
                InvalidateData (hTarget);
				if (hTarget)
				{
					CTreeItem * pItem = (CTreeItem*)m_RepoTree.GetItemData(hTarget);
					if (pItem)
					{
						// mark the target as 'dirty'
						pItem->children_fetched = false;
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
					CTreeItem * pItem = (CTreeItem*)m_RepoTree.GetItemData(hSelected);
					if (pItem)
					{
						// mark the target as 'dirty'
						pItem->children_fetched = false;
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
			if (CMessageBox::Show(m_hWnd, IDS_REPOBROWSE_MULTIIMPORT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)!=IDYES)
				return false;
		}

		CInputLogDlg input(this);
		input.SetProjectProperties(&m_ProjectProperties);
        input.SetUUID(m_repository.root);
		CString sHint;
		if (pathlist.GetCount() == 1)
			sHint.Format(IDS_INPUT_IMPORTFILEFULL, pathlist[0].GetWinPath(), (LPCTSTR)(target.GetSVNPathString() + _T("/") + pathlist[0].GetFileOrDirectoryName()));
		else
			sHint.Format(IDS_INPUT_IMPORTFILES, pathlist.GetCount());
		input.SetActionText(sHint);

		if (input.DoModal() == IDOK)
		{
			for (int importindex = 0; importindex<pathlist.GetCount(); ++importindex)
			{
				CString filename = pathlist[importindex].GetFileOrDirectoryName();
				if (!Import(pathlist[importindex], 
					CTSVNPath(target.GetSVNPathString()+_T("/")+filename), 
					input.GetLogMessage(), &m_ProjectProperties, svn_depth_infinity, TRUE, FALSE))
				{
					CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return false;
				}
			}
			if (GetRevision().IsHead())
			{
				// if the import operation was to the currently shown url,
				// update the current view. Otherwise mark the target URL as 'not fetched'.
				HTREEITEM hSelected = m_RepoTree.GetSelectedItem();
                InvalidateData (hSelected);
				if (hSelected)
				{
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
			m_RepoList.GetItemRect(m_RepoList.GetNextSelectedItem(pos), &rect, LVIR_LABEL);
			m_RepoList.ClientToScreen(&rect);
			point = rect.CenterPoint();
		}
	}

    CRepositoryBrowserSelection selection;
	if (pWnd == &m_RepoList)
	{
		CString urls;

		POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
		int index = -1;
		while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
            selection.Add ((CItem *)m_RepoList.GetItemData (index));

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
				m_blockEvents = true;
				hSelectedTreeItem = m_RepoTree.GetSelectedItem();
				if (hSelectedTreeItem)
				{
					m_RepoTree.SetItemState(hSelectedTreeItem, 0, TVIS_SELECTED);
					m_blockEvents = false;
					m_RepoTree.SetItemState(hSelectedTreeItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
                    selection.Add ((CTreeItem *)m_RepoTree.GetItemData (hSelectedTreeItem));
				}
			}
		}
	}
    if ((pWnd == &m_RepoTree)|| selection.IsEmpty())
	{
		UINT uFlags;
		CPoint ptTree = point;
		m_RepoTree.ScreenToClient(&ptTree);
		HTREEITEM hItem = m_RepoTree.HitTest(ptTree, &uFlags);
		// in case the right-clicked item is not the selected one,
		// use the TVIS_DROPHILITED style to indicate on which item
		// the context menu will work on
		if ((hItem) && (uFlags & TVHT_ONITEM) && (hItem != m_RepoTree.GetSelectedItem()))
		{
			m_blockEvents = true;
			hSelectedTreeItem = m_RepoTree.GetSelectedItem();
			m_RepoTree.SetItemState(hSelectedTreeItem, 0, TVIS_SELECTED);
			m_blockEvents = false;
			m_RepoTree.SetItemState(hItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
		}
		if (hItem)
		{
			hChosenTreeItem = hItem;
            selection.Add ((CTreeItem *)m_RepoTree.GetItemData (hItem));
		}
	}

    if (selection.GetRepositoryCount() != 1)
        return;

	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
        if (selection.GetPathCount (0) == 1)
		{
            if (selection.GetFolderCount (0) == 0)
			{
				// Let "Open" be the very first entry, like in Explorer
				popup.AppendMenuIcon(ID_OPEN, IDS_REPOBROWSE_OPEN, IDI_OPEN);		// "open"
				popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);	// "open with..."
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			popup.AppendMenuIcon(ID_SHOWLOG, IDS_REPOBROWSE_SHOWLOG, IDI_LOG);			// "Show Log..."
			// the revision graph on the repository root would be empty. We
			// don't show the context menu entry there.
            if (!selection.IsRoot (0, 0))
			{
				popup.AppendMenuIcon(ID_REVGRAPH, IDS_MENUREVISIONGRAPH, IDI_REVISIONGRAPH); // "Revision graph"
			}
			if (!selection.IsFolder (0, 0))
			{
				popup.AppendMenuIcon(ID_BLAME, IDS_MENUBLAME, IDI_BLAME);		// "Blame..."
			}
			if (!m_ProjectProperties.sWebViewerRev.IsEmpty())
			{
				popup.AppendMenuIcon(ID_VIEWREV, IDS_LOG_POPUP_VIEWREV);		// "View revision in webviewer"
			}
			if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
			{
				popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);	// "View revision for path in webviewer"
			}
			if ((!m_ProjectProperties.sWebViewerPathRev.IsEmpty())||
				(!m_ProjectProperties.sWebViewerRev.IsEmpty()))
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}

            // we can export files and folders alike

            popup.AppendMenuIcon(ID_EXPORT, IDS_MENUEXPORT, IDI_EXPORT);		// "Export"
		}
		// We allow checkout of multiple folders at once (we do that one by one)
        if (selection.GetFolderCount (0) == selection.GetPathCount (0))
		{
			popup.AppendMenuIcon(ID_CHECKOUT, IDS_MENUCHECKOUT, IDI_CHECKOUT);		// "Checkout.."
		}
        if (selection.GetPathCount (0) == 1)
		{
			if (selection.IsFolder (0, 0))
			{
				popup.AppendMenuIcon(ID_REFRESH, IDS_REPOBROWSE_REFRESH, IDI_REFRESH);		// "Refresh"
			}
			popup.AppendMenu(MF_SEPARATOR, NULL);				

            if (selection.GetRepository(0).revision.IsHead())
			{
				if (selection.IsFolder (0, 0))
				{
					popup.AppendMenuIcon(ID_MKDIR, IDS_REPOBROWSE_MKDIR, IDI_MKDIR);	// "create directory"
					popup.AppendMenuIcon(ID_IMPORT, IDS_REPOBROWSE_IMPORT, IDI_IMPORT);	// "Add/Import File"
					popup.AppendMenuIcon(ID_IMPORTFOLDER, IDS_REPOBROWSE_IMPORTFOLDER, IDI_IMPORT);	// "Add/Import Folder"
					popup.AppendMenu(MF_SEPARATOR, NULL);
				}

                if (!selection.IsExternal (0, 0))
				    popup.AppendMenuIcon(ID_RENAME, IDS_REPOBROWSE_RENAME, IDI_RENAME);		// "Rename"
			}
			if (selection.IsLocked (0, 0))
			{
				popup.AppendMenuIcon(ID_BREAKLOCK, IDS_MENU_UNLOCKFORCE, IDI_UNLOCK);	// "Break Lock"
			}
		}

        if (selection.GetRepository(0).revision.IsHead())
		{
			popup.AppendMenuIcon(ID_DELETE, IDS_REPOBROWSE_DELETE, IDI_DELETE);		// "Remove"
		}
		if (selection.GetFolderCount(0) == 0)
		{
			popup.AppendMenuIcon(ID_SAVEAS, IDS_REPOBROWSE_SAVEAS, IDI_SAVEAS);		// "Save as..."
		}
		if (   (selection.GetFolderCount(0) == selection.GetPathCount(0))
            || (selection.GetFolderCount(0) == 0))
		{
			popup.AppendMenuIcon(ID_COPYTOWC, IDS_REPOBROWSE_COPYTOWC);	// "Copy To Working Copy..."
		}

        if (selection.GetPathCount(0) == 1)
		{
			popup.AppendMenuIcon(ID_COPYTO, IDS_REPOBROWSE_COPY, IDI_COPY);			// "Copy To..."
			popup.AppendMenuIcon(ID_URLTOCLIPBOARD, IDS_REPOBROWSE_URLTOCLIPBOARD, IDI_COPYCLIP);	// "Copy URL to clipboard"
			popup.AppendMenu(MF_SEPARATOR, NULL);
			popup.AppendMenuIcon(ID_PROPS, IDS_REPOBROWSE_SHOWPROP, IDI_PROPERTIES);			// "Show Properties"
			// Revision properties are not associated to paths
			// so we only show that context menu on the repository root
            if (selection.IsRoot (0, 0))
			{
				popup.AppendMenuIcon(ID_REVPROPS, IDS_REPOBROWSE_SHOWREVPROP, IDI_PROPERTIES);	// "Show Revision Properties"
			}
			if (selection.IsFolder (0, 0))
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
				popup.AppendMenuIcon(ID_PREPAREDIFF, IDS_REPOBROWSE_PREPAREDIFF);	// "Mark for comparison"

                CTSVNPath root (selection.GetRepository(0).root);
                if (   (m_diffKind == svn_node_dir)
                    && !m_diffURL.IsEquivalentTo (selection.GetURL (0, 0))
                    && root.IsAncestorOf (m_diffURL))
				{
					popup.AppendMenuIcon(ID_GNUDIFF, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);		// "Show differences as unified diff"
					popup.AppendMenuIcon(ID_DIFF, IDS_REPOBROWSE_SHOWDIFF, IDI_DIFF);		// "Compare URLs"
				}
			}
		}

        if (   (selection.GetPathCount (0) == 1)
            && (selection.GetFolderCount (0) != 1))
		{
			popup.AppendMenu(MF_SEPARATOR, NULL);
			popup.AppendMenuIcon(ID_GNUDIFF, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);		// "Show differences as unified diff"
			popup.AppendMenuIcon(ID_DIFF, IDS_REPOBROWSE_SHOWDIFF, ID_DIFF);		// "Compare URLs"
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
					popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATE, IDI_UPDATE);		// "Update item to revision"
				}
			}
			else
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
				popup.AppendMenuIcon(ID_UPDATE, IDS_LOG_POPUP_UPDATE, IDI_UPDATE);		// "Update item to revision"
			}
		}
		popup.AppendMenuIcon(ID_CREATELINK, IDS_REPOBROWSE_CREATELINK, IDI_LINK);
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);

		if (pWnd == &m_RepoTree)
		{
			UINT uFlags;
			CPoint ptTree = point;
			m_RepoTree.ScreenToClient(&ptTree);
			HTREEITEM hItem = m_RepoTree.HitTest(ptTree, &uFlags);
			// restore the previously selected item state
			if ((hItem) && (uFlags & TVHT_ONITEM) && (hItem != m_RepoTree.GetSelectedItem()))
			{
				m_blockEvents = true;
				m_RepoTree.SetItemState(hSelectedTreeItem, TVIS_SELECTED, TVIS_SELECTED);
				m_blockEvents = false;
				m_RepoTree.SetItemState(hItem, 0, TVIS_DROPHILITED);
			}
		}
		if (hSelectedTreeItem)
		{
			m_blockEvents = true;
			m_RepoTree.SetItemState(hSelectedTreeItem, 0, TVIS_DROPHILITED);
			m_RepoTree.SetItemState(hSelectedTreeItem, TVIS_SELECTED, TVIS_SELECTED);
			m_blockEvents = false;
		}
		DialogEnableWindow(IDOK, FALSE);
		bool bOpenWith = false;
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
					sCmd.Format(_T("\"%s\" /command:update /pathfile:\"%s\" /rev /deletepathfile"),
						(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), tempFile.GetWinPath());

					CAppUtils::LaunchApplication(sCmd, NULL, false);
				}
			}
			break;
		case ID_PREPAREDIFF:
			{
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
			{
				CString url;
				for (size_t i=0; i < selection.GetPathCount(0); ++i)
                {
                    CString path = selection.GetURL (0, i).GetSVNPathString();
					url += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8 (path)));
					if (!GetRevision().IsHead())
					{
						url += _T("?r=") + GetRevision().ToString();
					}
					url += _T("\r\n");
                }
				url.TrimRight(_T("\r\n"));
				CStringUtils::WriteAsciiStringToClipboard(url);
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
					DWORD counter = 0;		// the file counter
                    DWORD pathCount = (DWORD)selection.GetPathCount(0);
                    const SVNRev& revision = selection.GetRepository(0).revision;

                    progDlg.SetTitle(IDS_REPOBROWSE_SAVEASPROGTITLE);
					progDlg.SetAnimation(IDR_DOWNLOAD);
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
						sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, (LPCTSTR)saveurl, (LPCTSTR)revision.ToString());
						progDlg.SetLine(1, sInfoLine, true);
						if (!Cat(CTSVNPath(saveurl), revision, revision, savepath)||(progDlg.HasUserCancelled()))
						{
							wait_cursor.Hide();
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							if (!progDlg.HasUserCancelled())
								CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
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

					// get log of first URL
					CString sCopyFrom1, sCopyFrom2;
					SVNLogHelper helper;
					SVNRev rev1 = helper.GetCopyFromRev(firstPath, pegRevision, sCopyFrom1);
					if (!rev1.IsValid())
					{
						CMessageBox::Show(this->m_hWnd, helper.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						break;
					}
					SVNRev rev2 = helper.GetCopyFromRev(secondUrl, pegRevision, sCopyFrom2);
					if (!rev2.IsValid())
					{
						CMessageBox::Show(this->m_hWnd, helper.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						break;
					}
					if ((sCopyFrom1.IsEmpty())||(sCopyFrom1.Compare(sCopyFrom2)!=0)||(svn_revnum_t(rev1) == 0)||(svn_revnum_t(rev1) == 0))
					{
						// no common copy from URL, so showing a log between
						// the two urls is not possible.
						CMessageBox::Show(m_hWnd, IDS_ERR_NOCOMMONCOPYFROM, IDS_APPNAME, MB_ICONERROR);
						break;							
					}
					if ((LONG)rev1 < (LONG)rev2)
					{
						SVNRev temp = rev1;
						rev1 = rev2;
						rev2 = temp;
					}
					CString sCmd;
					sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /startrev:%s /endrev:%s"),
						(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)sCopyFrom1, (LPCTSTR)rev1.ToString(), (LPCTSTR)rev2.ToString());

					ATLTRACE(sCmd);
					if (!m_path.IsUrl())
					{
						sCmd += _T(" /propspath:\"");
						sCmd += m_path.GetWinPathString();
						sCmd += _T("\"");
					}	

					CAppUtils::LaunchApplication(sCmd, NULL, false);
				}
				else
				{
					CString sCmd;
					sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /startrev:%s"), 
						(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), (LPCTSTR)firstPath.GetSVNPathString(), (LPCTSTR)pegRevision.ToString());

					if (!m_path.IsUrl())
					{
						sCmd += _T(" /propspath:\"");
						sCmd += m_path.GetWinPathString();
						sCmd += _T("\"");
					}	

					CAppUtils::LaunchApplication(sCmd, NULL, false);
				}
			}
			break;
		case ID_VIEWREV:
			{
				CString url = m_ProjectProperties.sWebViewerRev;
				url.Replace(_T("%REVISION%"), selection.GetRepository(0).revision.ToString());
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
		case ID_VIEWPATHREV:
			{
                const SRepositoryInfo& repository = selection.GetRepository(0);
                CString relurl = selection.GetURLEscaped (0, 0).GetSVNPathString();
                relurl = relurl.Mid (repository.root.GetLength());
				CString weburl = m_ProjectProperties.sWebViewerPathRev;
                weburl.Replace(_T("%REVISION%"), repository.revision.ToString());
				weburl.Replace(_T("%PATH%"), relurl);
				if (!weburl.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), weburl, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
		case ID_CHECKOUT:
			{
				CString itemsToCheckout;
                for (size_t i=0; i < selection.GetPathCount(0); ++i)
				{
                    itemsToCheckout += selection.GetURL (0, i).GetSVNPathString() + _T("*");
				}
				itemsToCheckout.TrimRight('*');
				CString sCmd;
				sCmd.Format ( _T("\"%s\" /command:checkout /url:\"%s\" /revision:%s")
                            , (LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"))
                            , (LPCTSTR)itemsToCheckout
                            , (LPCTSTR)selection.GetRepository(0).revision.ToString());

				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_EXPORT:
			{
				CExportDlg dlg;
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
					if (dlg.m_eolStyle.CompareNoCase(_T("CRLF"))==0)
						opts |= ProgOptEolCRLF;
					if (dlg.m_eolStyle.CompareNoCase(_T("CR"))==0)
						opts |= ProgOptEolCR;
					if (dlg.m_eolStyle.CompareNoCase(_T("LF"))==0)
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
				sCmd.Format ( _T("\"%s\" /command:revisiongraph /path:\"%s\" /pegrev:%s")
                            , (LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"))
                            , (LPCTSTR)selection.GetURLEscaped (0, 0).GetSVNPathString()
                            , (LPCTSTR)selection.GetRepository (0).revision.ToString());

				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_OPENWITH:
			bOpenWith = true;
		case ID_OPEN:
			{
				OpenFile(selection.GetURL (0,0), selection.GetURLEscaped (0,0), bOpenWith);
			}
			break;
		case ID_DELETE:
			{
				CWaitCursorEx wait_cursor;
				CInputLogDlg input(this);
                input.SetUUID(selection.GetRepository(0).uuid);
				input.SetProjectProperties(&m_ProjectProperties);
				CString hint;
                if (selection.GetPathCount (0) == 1)
					hint.Format(IDS_INPUT_REMOVEONE, (LPCTSTR)selection.GetURL (0, 0).GetFileOrDirectoryName());
				else
					hint.Format(IDS_INPUT_REMOVEMORE, selection.GetPathCount (0));

				input.SetActionText(hint);
				if (input.DoModal() == IDOK)
				{
                    InvalidateDataParents (selection);

                    if (!Remove (selection.GetURLsEscaped (0), true, false, input.GetLogMessage()))
					{
						wait_cursor.Hide();
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return;
					}
					if (hChosenTreeItem)
					{
						HTREEITEM hParent = m_RepoTree.GetParentItem(hChosenTreeItem);
						RecursiveRemove(hChosenTreeItem);
						RefreshNode(hParent);
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
					CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return;
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
					CInputLogDlg input(this);
					input.SetUUID(selection.GetRepository(0).uuid);
					input.SetProjectProperties(&m_ProjectProperties);

                    const CTSVNPath& url = selection.GetURL (0, 0);
					CString sHint;
					sHint.Format(IDS_INPUT_IMPORTFOLDER, (LPCTSTR)svnPath.GetSVNPathString(), (LPCTSTR)(url.GetSVNPathString()+_T("/")+filename));
					input.SetActionText(sHint);
					if (input.DoModal() == IDOK)
					{
                        InvalidateDataParents (selection);

						CProgressDlg progDlg;
						progDlg.SetTitle(IDS_APPNAME);
						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSIMPORT, (LPCTSTR)filename);
						progDlg.SetLine(1, sInfoLine, true);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);
						if (!Import(svnPath, 
							CTSVNPath(EscapeUrl(CTSVNPath(url.GetSVNPathString()+_T("/")+filename))), 
							input.GetLogMessage(), 
							&m_ProjectProperties, 
							svn_depth_infinity, 
							FALSE, FALSE))
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							wait_cursor.Hide();
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						}
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
				if (CAppUtils::FileOpenSave(openPath, NULL, IDS_REPOBROWSE_IMPORT, IDS_COMMONFILEFILTER, true, m_hWnd))
				{
					CTSVNPath path(openPath);
					CWaitCursorEx wait_cursor;
					CString filename = path.GetFileOrDirectoryName();
					CInputLogDlg input(this);
					input.SetUUID(selection.GetRepository(0).uuid);
					input.SetProjectProperties(&m_ProjectProperties);

                    const CTSVNPath& url = selection.GetURL (0, 0);
                    CString sHint;
					sHint.Format(IDS_INPUT_IMPORTFILEFULL, path.GetWinPath(), (LPCTSTR)(url.GetSVNPathString()+_T("/")+filename));
					input.SetActionText(sHint);
					if (input.DoModal() == IDOK)
					{
                        InvalidateDataParents (selection);

						CProgressDlg progDlg;
						progDlg.SetTitle(IDS_APPNAME);
						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSIMPORT, (LPCTSTR)filename);
						progDlg.SetLine(1, sInfoLine, true);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);
						if (!Import(path, 
							CTSVNPath(EscapeUrl(CTSVNPath(url.GetSVNPathString()+_T("/")+filename))), 
							input.GetLogMessage(), 
							&m_ProjectProperties,
							svn_depth_empty, 
							TRUE, FALSE))
						{
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							wait_cursor.Hide();
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						}
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
					POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
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

				CRenameDlg dlg;
                dlg.m_name = path.GetSVNPathString();
				dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_COPY);
				CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
				if (dlg.DoModal() == IDOK)
				{
					CWaitCursorEx wait_cursor;
					CInputLogDlg input(this);
                    input.SetUUID(selection.GetRepository(0).uuid);
					input.SetProjectProperties(&m_ProjectProperties);
					CString sHint;
					sHint.Format(IDS_INPUT_COPY, (LPCTSTR)path.GetSVNPathString(), (LPCTSTR)dlg.m_name);
					input.SetActionText(sHint);
					if (!CTSVNPath(dlg.m_name).IsValidOnWindows())
					{
						if (CMessageBox::Show(GetSafeHwnd(), IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONINFORMATION|MB_YESNO) != IDYES)
							break;
					}
					if (input.DoModal() == IDOK)
					{
                        InvalidateDataParents (selection);

						if (!Copy (selection.GetURLsEscaped (0), CTSVNPath(dlg.m_name), revision, revision, input.GetLogMessage()))
						{
							wait_cursor.Hide();
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						}
						if (revision.IsHead())
						{
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
					progDlg.SetAnimation(IDR_DOWNLOAD);
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
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return;
					}
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
				}
			}
			break;
		case ID_MKDIR:
			{
                const CTSVNPath& path = selection.GetURL (0, 0);
				CRenameDlg dlg;
				dlg.m_name = _T("");
				dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_MKDIR);
				CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
				if (dlg.DoModal() == IDOK)
				{
					CWaitCursorEx wait_cursor;
					CInputLogDlg input(this);
					input.SetUUID(selection.GetRepository(0).uuid);
					input.SetProjectProperties(&m_ProjectProperties);
					CString sHint;
					sHint.Format(IDS_INPUT_MKDIR, (LPCTSTR)(path.GetSVNPathString()+_T("/")+dlg.m_name.Trim()));
					input.SetActionText(sHint);
					if (input.DoModal() == IDOK)
					{
                        InvalidateDataParents (selection);

						// when creating the new folder, also trim any whitespace chars from it
						if (!MakeDir(CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(path.GetSVNPathString()+_T("/")+dlg.m_name.Trim())))), input.GetLogMessage(), true))
						{
							wait_cursor.Hide();
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						}
						RefreshNode(m_RepoTree.GetSelectedItem(), true);
					}
				}
			}
			break;
		case ID_REFRESH:
			{
				RefreshNode(m_RepoTree.GetSelectedItem(), true);
			}
			break;
		case ID_GNUDIFF:
			{
                const CTSVNPath& path = selection.GetURLEscaped (0, 0);
                const SVNRev& revision = selection.GetRepository (0).revision;

				SVNDiff diff(this, this->m_hWnd, true);
                if (selection.GetPathCount(0) == 1)
				{
					if (PromptShown())
						diff.ShowUnifiedDiff (path, revision, 
											CTSVNPath(EscapeUrl(m_diffURL)), revision);
					else
						CAppUtils::StartShowUnifiedDiff(m_hWnd, path, revision, 
											CTSVNPath(EscapeUrl(m_diffURL)), revision);
				}
				else
				{
                    const CTSVNPath& path2 = selection.GetURLEscaped (0, 1);
					if (PromptShown())
						diff.ShowUnifiedDiff(path, revision, 
											path2, revision);
					else
						CAppUtils::StartShowUnifiedDiff(m_hWnd, path, revision, 
											path2, revision);
				}
			}
			break;
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
						CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
					else
						CAppUtils::StartShowCompare(m_hWnd, path, revision, 
										CTSVNPath(EscapeUrl(m_diffURL)), revision, SVNRev(), SVNRev(), 
										!!(GetAsyncKeyState(VK_SHIFT) & 0x8000), true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
				}
				else
				{
                    const CTSVNPath& path2 = selection.GetURLEscaped (0, 1);
					if (PromptShown())
						diff.ShowCompare(path, revision, 
										path2, revision, SVNRev(), true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
					else
						CAppUtils::StartShowCompare(m_hWnd, path, revision, 
										path2, revision, SVNRev(), SVNRev(), 
										!!(GetAsyncKeyState(VK_SHIFT) & 0x8000), true, false, nFolders > 0 ? svn_node_dir : svn_node_file);
				}
			}
			break;
		case ID_PROPS:
			{
                const SVNRev& revision = selection.GetRepository(0).revision;
				if (revision.IsHead())
				{
					CEditPropertiesDlg dlg;
					dlg.SetProjectProperties(&m_ProjectProperties);
					dlg.SetUUID(selection.GetRepository(0).uuid);
                    dlg.SetPathList(selection.GetURLsEscaped(0));
					dlg.SetRevision(GetHEADRevision(selection.GetURL (0, 0)));
					dlg.DoModal();
				}
				else
				{
					CPropDlg dlg;
					dlg.m_rev = revision;
                    dlg.m_Path = selection.GetURLEscaped (0,0);
					dlg.DoModal();
				}
			}
			break;
		case ID_REVPROPS:
			{
				CEditPropertiesDlg dlg;
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
				CBlameDlg dlg;
				dlg.EndRev = selection.GetRepository(0).revision;
				if (dlg.DoModal() == IDOK)
				{
					CBlame blame;
					CString tempfile;
					CString logfile;
                    const CTSVNPath& path = selection.GetURLEscaped (0,0);

					tempfile = blame.BlameToTempFile ( path
                                                     , dlg.StartRev
                                                     , dlg.EndRev
                                                     , dlg.EndRev
                                                     , logfile
                                                     , SVN::GetOptionsString (!!dlg.m_bIgnoreEOL, !!dlg.m_IgnoreSpaces)
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
							CString sParams = _T("/path:\"") + path.GetSVNPathString() + _T("\" ");
                            if(!CAppUtils::LaunchTortoiseBlame ( tempfile
                                                               , logfile
                                                               , CPathUtils::GetFileNameFromPath(selection.GetURL (0,0).GetFileOrDirectoryName())
                                                               , sParams
                                                               , dlg.StartRev
                                                               , dlg.EndRev))
							{
								break;
							}
						}
					}
					else
					{
						CMessageBox::Show(this->m_hWnd, blame.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
				}

			}
			break;
		case ID_CREATELINK:
			{
				CTSVNPath tempFile;
				if (AskForSavePath(selection, tempFile, false))
				{
					if (tempFile.GetFileExtension().Compare(_T(".url")))
					{
						tempFile.AppendRawString(_T(".url"));
					}
					CString urlCmd = selection.GetURLEscaped(0, 0).GetSVNPathString() + _T("?r=") + selection.GetRepository(0).revision.ToString();
					CAppUtils::CreateShortcutToURL((LPCTSTR)urlCmd, tempFile.GetWinPath());
				}
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
		bSavePathOK = CAppUtils::FileOpenSave(savePath, NULL, IDS_REPOBROWSE_SAVEAS, IDS_COMMONFILEFILTER, false, m_hWnd);
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
		CString hex = WidthString.Mid(i*8, 8);
		if ( hex.IsEmpty() )
		{
			// This case only occurs when upgrading from an older
			// TSVN version in which there were fewer columns.
			WidthArray[i] = 0;
		}
		else
		{
			WidthArray[i] = _tcstol(hex, &endchar, 16);
		}
	}
	return true;
}

CString CRepositoryBrowser::WidthArrayToString(int WidthArray[])
{
	CString sResult;
	TCHAR buf[10];
	for (int i=0; i<7; ++i)
	{
		_stprintf_s(buf, 10, _T("%08X"), WidthArray[i]);
		sResult += buf;
	}
	return sResult;
}

void CRepositoryBrowser::SaveColumnWidths(bool bSaveToRegistry /* = false */)
{
	CRegString regColWidth(_T("Software\\TortoiseSVN\\RepoBrowserColumnWidth"));
	int maxcol = ((CHeaderCtrl*)(m_RepoList.GetDlgItem(0)))->GetItemCount()-1;
	// first clear the width array
	for (int col = 0; col < 7; ++col)
		m_arColumnWidths[col] = 0;
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
    CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData (node);
    InvalidateData (node, pItem->repository.revision);
}

void CRepositoryBrowser::InvalidateData (HTREEITEM node, const SVNRev& revision)
{
	CTreeItem * pItem = NULL;
	if (node != NULL) 
		pItem = (CTreeItem *)m_RepoTree.GetItemData (node);
    if (pItem == NULL)
        m_lister.Refresh (revision);
    else
        m_lister.RefreshSubTree (revision, pItem->url);
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
	// must be exactly one path	
	if ( (selection.GetRepositoryCount() != 1)
        || (selection.GetPathCount(0) != 1))
	{
		return;
	}
	
	// build copy source / content
	CIDropSource* pdsrc = new CIDropSource;
	if (pdsrc == NULL)
		return;

	pdsrc->AddRef();

    const SVNRev& revision = selection.GetRepository(0).revision;
    SVNDataObject* pdobj = new SVNDataObject(selection.GetURLsEscaped(0), revision, revision);
	if (pdobj == NULL)
	{
		delete pdsrc;
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
	::DoDragDrop(pdobj, pdsrc, DROPEFFECT_MOVE|DROPEFFECT_COPY, &dwEffect);
	pdsrc->Release();
	pdobj->Release();
}