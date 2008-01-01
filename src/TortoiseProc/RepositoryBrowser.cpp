// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "LogDlg.h"
#include "PropDlg.h"
#include "EditPropertiesDlg.h"
#include "Blame.h"
#include "BlameDlg.h"
#include "WaitCursorEx.h"
#include "Repositorybrowser.h"
#include "BrowseFolder.h"
#include "RenameDlg.h"
#include "RevisionGraphDlg.h"
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
	ID_GNUDIFF,
	ID_DIFF,
	ID_PREPAREDIFF,
	ID_UPDATE,

};

IMPLEMENT_DYNAMIC(CRepositoryBrowser, CResizableStandAloneDialog)

CRepositoryBrowser::CRepositoryBrowser(const CString& url, const SVNRev& rev)
	: CResizableStandAloneDialog(CRepositoryBrowser::IDD, NULL)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_bStandAlone(true)
	, m_InitialUrl(url)
	, m_initialRev(rev)
	, m_bInitDone(false)
	, m_blockEvents(false)
	, m_bSortAscending(true)
	, m_nSortedColumn(0)
	, m_pTreeDropTarget(NULL)
	, m_pListDropTarget(NULL)
	, m_bCancelled(false)
	, m_diffKind(svn_node_none)
	, m_hAccel(NULL)
    , bDragMode(FALSE)
{
}

CRepositoryBrowser::CRepositoryBrowser(const CString& url, const SVNRev& rev, CWnd* pParent)
	: CResizableStandAloneDialog(CRepositoryBrowser::IDD, pParent)
	, m_cnrRepositoryBar(&m_barRepository)
	, m_InitialUrl(url)
	, m_initialRev(rev)
	, m_bStandAlone(false)
	, m_bInitDone(false)
	, m_blockEvents(false)
	, m_bSortAscending(true)
	, m_nSortedColumn(0)
	, m_pTreeDropTarget(NULL)
	, m_pListDropTarget(NULL)
	, m_bCancelled(false)
	, m_diffKind(svn_node_none)
{
}

CRepositoryBrowser::~CRepositoryBrowser()
{
}

void CRepositoryBrowser::RecursiveRemove(HTREEITEM hItem)
{
	HTREEITEM childItem;
	if (m_RepoTree.ItemHasChildren(hItem))
	{
		for (childItem = m_RepoTree.GetChildItem(hItem);childItem != NULL; childItem = m_RepoTree.GetNextItem(childItem, TVGN_NEXT))
		{
			RecursiveRemove(childItem);
			CTreeItem * pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(childItem);
			delete pTreeItem;
			m_RepoTree.SetItemData(childItem, 0);
		}
	}

	if (hItem)
	{
		CTreeItem * pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(hItem);
		delete pTreeItem;
		m_RepoTree.SetItemData(hItem, 0);
	}
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
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_REPOLIST, &CRepositoryBrowser::OnLvnEndlabeleditRepolist)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_REPOTREE, &CRepositoryBrowser::OnTvnEndlabeleditRepotree)
	ON_WM_TIMER()
	ON_COMMAND(ID_URL_FOCUS, &CRepositoryBrowser::OnUrlFocus)
	ON_COMMAND(ID_EDIT_COPY, &CRepositoryBrowser::OnCopy)
	ON_COMMAND(ID_INLINEEDIT, &CRepositoryBrowser::OnInlineedit)
	ON_COMMAND(ID_REFRESHBROWSER, &CRepositoryBrowser::OnRefresh)
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

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_REPOBROWSER));

	m_cnrRepositoryBar.SubclassDlgItem(IDC_REPOS_BAR_CNR, this);
	m_barRepository.Create(&m_cnrRepositoryBar, 12345);
	m_barRepository.SetIRepo(this);

	m_pTreeDropTarget = new CTreeDropTarget(this);
	RegisterDragDrop(m_RepoTree.GetSafeHwnd(), m_pTreeDropTarget);
	// create the supported formats:
	FORMATETC ftetc={0}; 
	ftetc.cfFormat = CF_UNICODETEXT; 
	ftetc.dwAspect = DVASPECT_CONTENT; 
	ftetc.lindex = -1; 
	ftetc.tymed = TYMED_HGLOBAL; 
	m_pTreeDropTarget->AddSuportedFormat(ftetc); 
	ftetc.cfFormat=CF_HDROP; 
	m_pTreeDropTarget->AddSuportedFormat(ftetc);

	m_pListDropTarget = new CListDropTarget(this);
	RegisterDragDrop(m_RepoList.GetSafeHwnd(), m_pListDropTarget);
	// create the supported formats:
	ftetc.cfFormat = CF_UNICODETEXT; 
	m_pListDropTarget->AddSuportedFormat(ftetc); 
	ftetc.cfFormat=CF_HDROP; 
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
	// set the extended style of the listcontrol
	// the style LVS_EX_FULLROWSELECT interferes with the background watermark image but it's more important to be able to select in the whole row.
	CRegDWORD regFullRowSelect(_T("Software\\TortoiseSVN\\FullRowSelect"), TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	m_RepoList.SetExtendedStyle(exStyle);
	m_RepoList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
	m_RepoList.ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_INITWAIT)));

	m_RepoTree.SetImageList(&SYS_IMAGE_LIST(), TVSIL_NORMAL);

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

	m_InitialUrl = CPathUtils::PathUnescape(m_InitialUrl);
	if (m_InitialUrl.Find('?')>=0)
	{
		m_initialRev = SVNRev(m_InitialUrl.Mid(m_InitialUrl.Find('?')+1));
		m_InitialUrl = m_InitialUrl.Left(m_InitialUrl.Find('?'));
	}

	// We don't know if the url passed to us points to a file or a folder,
	// let's find out:
	SVNInfo info;
	const SVNInfoData * data = NULL;
	CString error;	// contains the first error of GetFirstFileInfo()
	do 
	{
		data = info.GetFirstFileInfo(CTSVNPath(m_InitialUrl),m_initialRev, m_initialRev);
		if ((data == NULL)||(data->kind != svn_node_dir))
		{
			// in case the url is not a valid directory, try the parent dir
			// until there's no more parent dir
			m_InitialUrl = m_InitialUrl.Left(m_InitialUrl.ReverseFind('/'));
			if ((m_InitialUrl.Compare(_T("http://")) == 0) ||
				(m_InitialUrl.Compare(_T("https://")) == 0)||
				(m_InitialUrl.Compare(_T("svn://")) == 0)||
				(m_InitialUrl.Compare(_T("svn+ssh://")) == 0)||
				(m_InitialUrl.Compare(_T("file:///")) == 0))
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
	else if (m_initialRev.IsHead())
	{
		m_barRepository.SetHeadRevision(data->rev);
	}
	m_InitialUrl.TrimRight('/');

	m_bCancelled = false;
	m_strReposRoot = data->reposRoot;
	m_sUUID = data->reposUUID;
	m_strReposRoot = CPathUtils::PathUnescape(m_strReposRoot);
	// now check the repository root for the url type, then
	// set the corresponding background image
	if (!m_strReposRoot.IsEmpty())
	{
		UINT nID = IDI_REPO_UNKNOWN;
		if (m_strReposRoot.Left(7).CompareNoCase(_T("http://"))==0)
			nID = IDI_REPO_HTTP;
		if (m_strReposRoot.Left(8).CompareNoCase(_T("https://"))==0)
			nID = IDI_REPO_HTTPS;
		if (m_strReposRoot.Left(6).CompareNoCase(_T("svn://"))==0)
			nID = IDI_REPO_SVN;
		if (m_strReposRoot.Left(10).CompareNoCase(_T("svn+ssh://"))==0)
			nID = IDI_REPO_SVNSSH;
		if (m_strReposRoot.Left(8).CompareNoCase(_T("file:///"))==0)
			nID = IDI_REPO_FILE;
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
	if ((m_InitialUrl.IsEmpty())||(m_strReposRoot.IsEmpty()))
	{
		return 0;
	}

	m_barRepository.GotoUrl(m_InitialUrl, m_initialRev, true);
	m_RepoList.ClearText();
	m_bInitDone = TRUE;
	return 0;
}

void CRepositoryBrowser::OnOK()
{
	RevokeDragDrop(m_RepoList.GetSafeHwnd());
	RevokeDragDrop(m_RepoTree.GetSafeHwnd());

	SaveColumnWidths(true);

	HTREEITEM hItem = m_RepoTree.GetRootItem();
	RecursiveRemove(hItem);

	m_barRepository.SaveHistory();
	CResizableStandAloneDialog::OnOK();
}

void CRepositoryBrowser::OnCancel()
{
	RevokeDragDrop(m_RepoList.GetSafeHwnd());
	RevokeDragDrop(m_RepoTree.GetSafeHwnd());

	SaveColumnWidths(true);

	HTREEITEM hItem = m_RepoTree.GetRootItem();
	RecursiveRemove(hItem);

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
	if (m_bThreadRunning)
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
				(pt.y >= rect.top)&&
				(pt.y <= rect.bottom))
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
		point.x = treelist.left+REPOBROWSER_CTRL_MIN_WIDTH;
    if (point.x > treelist.right-3) 
        return CStandAloneDialogTmpl<CResizableDialog>::OnLButtonDown(nFlags, point);
	if (point.x > treelist.right-REPOBROWSER_CTRL_MIN_WIDTH) 
		point.x = treelist.right-REPOBROWSER_CTRL_MIN_WIDTH;

	if ((point.y < treelist.top) || 
		(point.y > treelist.bottom))
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

/******************************************************************************/
/* repository information gathering                                           */
/******************************************************************************/

BOOL CRepositoryBrowser::ReportList(const CString& path, svn_node_kind_t kind, 
									svn_filesize_t size, bool has_props, 
									svn_revnum_t created_rev, apr_time_t time, 
									const CString& author, const CString& locktoken, 
									const CString& lockowner, const CString& lockcomment, 
									bool is_dav_comment, apr_time_t lock_creationdate, 
									apr_time_t lock_expirationdate, 
									const CString& absolutepath)
{
	static deque<CItem> * pDirList = NULL;
	static CTreeItem * pTreeItem = NULL;
	static CString dirPath;

	CString sParent = absolutepath;
	int slashpos = path.ReverseFind('/');
	bool abspath_has_slash = (absolutepath.GetAt(absolutepath.GetLength()-1) == '/');
	if ((slashpos > 0) && (!abspath_has_slash))
		sParent += _T("/");
	sParent += path.Left(slashpos);
	if (sParent.Compare(_T("/"))==0)
		sParent.Empty();
	if ((path.IsEmpty())||
		(pDirList == NULL)||
		(sParent.Compare(dirPath)))
	{
		HTREEITEM hItem = FindUrl(m_strReposRoot + sParent);
		pTreeItem = (CTreeItem*)m_RepoTree.GetItemData(hItem);
		pDirList = &(pTreeItem->children);

		dirPath = sParent;
	}
	if (path.IsEmpty())
		return TRUE;

	if (kind == svn_node_dir)
	{
		FindUrl(m_strReposRoot + absolutepath + (abspath_has_slash ? _T("") : _T("/")) + path);
		if (pTreeItem)
			pTreeItem->has_child_folders = true;
	}
	pDirList->push_back(CItem(path.Mid(slashpos+1), kind, size, has_props,
		created_rev, time, author, locktoken,
		lockowner, lockcomment, is_dav_comment,
		lock_creationdate, lock_expirationdate,
		m_strReposRoot+absolutepath+(abspath_has_slash ? _T("") : _T("/"))+path));
	if (pTreeItem)
		pTreeItem->children_fetched = true;
	return TRUE;
}

bool CRepositoryBrowser::ChangeToUrl(const CString& url, const SVNRev& rev)
{
	CWaitCursorEx wait;
	CString partUrl = url;
	HTREEITEM hItem = m_RepoTree.GetRootItem();
	if ((LONG(rev) != LONG(m_initialRev))||
		(m_strReposRoot.IsEmpty())||
		(m_strReposRoot.Compare(url.Left(m_strReposRoot.GetLength())))||
		(url.GetAt(m_strReposRoot.GetLength()) != '/'))
	{
		// if the revision changed, then invalidate everything
		RecursiveRemove(hItem);
		m_RepoTree.DeleteAllItems();
		m_RepoList.DeleteAllItems();
		m_RepoList.ShowText(CString(MAKEINTRESOURCE(IDS_REPOBROWSE_WAIT)), true);
		hItem = m_RepoTree.GetRootItem();
		if ((m_strReposRoot.IsEmpty())||(m_strReposRoot.Compare(url.Left(m_strReposRoot.GetLength())))||
			(url.GetAt(m_strReposRoot.GetLength()) != '/'))
		{
			// if the repository root has changed, initialize all data from scratch
			m_InitialUrl = url;
			InitRepo();
			if ((m_InitialUrl.IsEmpty())||(m_strReposRoot.IsEmpty()))
				return false;
		}
	}
	if (hItem == NULL)
	{
		// the tree view is empty, just fill in the repository root
		CTreeItem * pTreeItem = new CTreeItem();
		pTreeItem->unescapedname = m_strReposRoot;
		pTreeItem->url = m_strReposRoot;

		TVINSERTSTRUCT tvinsert = {0};
		tvinsert.hParent = TVI_ROOT;
		tvinsert.hInsertAfter = TVI_ROOT;
		tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvinsert.itemex.pszText = m_strReposRoot.GetBuffer(m_strReposRoot.GetLength());
		tvinsert.itemex.cChildren = 1;
		tvinsert.itemex.lParam = (LPARAM)pTreeItem;
		tvinsert.itemex.iImage = m_nIconFolder;
		tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;

		hItem = m_RepoTree.InsertItem(&tvinsert);
		m_strReposRoot.ReleaseBuffer();
	}
	if (hItem == NULL)
	{
		// something terrible happened!
		return false;
	}
	hItem = FindUrl(url);
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
		// extension
		temp = CPathUtils::GetFileExtFromPath(it->path);
		if (it->kind == svn_node_file)
			m_RepoList.SetItemText(index, 1, temp);
		// revision
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
		SVN::formatDate(date_native, (apr_time_t&)it->time, true);
		m_RepoList.SetItemText(index, 5, date_native);
		// lock owner
		m_RepoList.SetItemText(index, 6, it->lockowner);
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

HTREEITEM CRepositoryBrowser::FindUrl(const CString& fullurl, bool create /* = true */)
{
	return FindUrl(fullurl, fullurl, create, TVI_ROOT);
}

HTREEITEM CRepositoryBrowser::FindUrl(const CString& fullurl, const CString& url, bool create /* true */, HTREEITEM hItem /* = TVI_ROOT */)
{
	if (hItem == TVI_ROOT)
	{
		hItem = m_RepoTree.GetRootItem();
		if (fullurl.Compare(m_strReposRoot)==0)
			return hItem;
		return FindUrl(fullurl, url.Mid(m_strReposRoot.GetLength()+1), create, hItem);
	}
	HTREEITEM hSibling = hItem;
	if (m_RepoTree.GetNextItem(hItem, TVGN_CHILD))
	{
		hSibling = m_RepoTree.GetNextItem(hItem, TVGN_CHILD);
		do
		{
			CString sSibling = ((CTreeItem*)m_RepoTree.GetItemData(hSibling))->unescapedname;
			if (sSibling.Compare(url.Left(sSibling.GetLength()))==0)
			{
				if (sSibling.GetLength() == url.GetLength())
					return hSibling;
				if (url.GetAt(sSibling.GetLength()) == '/')
					return FindUrl(fullurl, url.Mid(sSibling.GetLength()+1), create, hSibling);
			}
		} while ((hSibling = m_RepoTree.GetNextItem(hSibling, TVGN_NEXT)) != NULL);	
	}
	if (!create)
		return NULL;
	// create tree items for every path part in the url
	CString sUrl = url;
	int slash = -1;
	HTREEITEM hNewItem = hItem;
	CString sTemp;
	while ((slash=sUrl.Find('/')) >= 0)
	{
		CTreeItem * pTreeItem = new CTreeItem();
		sTemp = sUrl.Left(slash);
		pTreeItem->unescapedname = sTemp;
		pTreeItem->url = fullurl.Left(fullurl.GetLength()-sUrl.GetLength()+slash);
		UINT state = pTreeItem->url.CompareNoCase(m_diffURL.GetSVNPathString()) ? 0 : TVIS_BOLD;
		TVINSERTSTRUCT tvinsert = {0};
		tvinsert.hParent = hNewItem;
		tvinsert.hInsertAfter = TVI_SORT;
		tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
		tvinsert.itemex.state = state;
		tvinsert.itemex.stateMask = state;
		tvinsert.itemex.pszText = sTemp.GetBuffer(sTemp.GetLength());
		tvinsert.itemex.cChildren = 1;
		tvinsert.itemex.lParam = (LPARAM)pTreeItem;
		tvinsert.itemex.iImage = m_nIconFolder;
		tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;

		hNewItem = m_RepoTree.InsertItem(&tvinsert);
		sTemp.ReleaseBuffer();
		sUrl = sUrl.Mid(slash+1);
		ATLTRACE(_T("created tree entry %s, url %s\n"), sTemp, pTreeItem->url);
	}
	CTreeItem * pTreeItem = new CTreeItem();
	sTemp = sUrl;
	pTreeItem->unescapedname = sTemp;
	pTreeItem->url = fullurl;
	UINT state = pTreeItem->url.CompareNoCase(m_diffURL.GetSVNPathString()) ? 0 : TVIS_BOLD;
	TVINSERTSTRUCT tvinsert = {0};
	tvinsert.hParent = hNewItem;
	tvinsert.hInsertAfter = TVI_SORT;
	tvinsert.itemex.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvinsert.itemex.state = state;
	tvinsert.itemex.stateMask = state;
	tvinsert.itemex.pszText = sTemp.GetBuffer(sTemp.GetLength());
	tvinsert.itemex.cChildren = 1;
	tvinsert.itemex.lParam = (LPARAM)pTreeItem;
	tvinsert.itemex.iImage = m_nIconFolder;
	tvinsert.itemex.iSelectedImage = m_nOpenIconFolder;

	hNewItem = m_RepoTree.InsertItem(&tvinsert);
	sTemp.ReleaseBuffer();
	m_RepoTree.SortChildren(hNewItem);
	return hNewItem;
}

bool CRepositoryBrowser::RefreshNode(const CString& url, bool force /* = false*/, bool recursive /* = false*/)
{
	HTREEITEM hNode = FindUrl(url);
	return RefreshNode(hNode, force, recursive);
}

bool CRepositoryBrowser::RefreshNode(HTREEITEM hNode, bool force /* = false*/, bool recursive /* = false*/)
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
	pTreeItem->children.clear();
	pTreeItem->has_child_folders = false;
	m_bCancelled = false;
	if (!List(CTSVNPath(pTreeItem->url), GetRevision(), GetRevision(), recursive ? svn_depth_infinity : svn_depth_immediates, true))
	{
		// error during list()
		m_RepoList.ShowText(GetLastErrorMessage());
		return false;
	}
	pTreeItem->children_fetched = true;
	// if there are no child folders, remove the '+' in front of the node
	{
		TVITEM tvitem = {0};
		tvitem.hItem = hNode;
		tvitem.mask = TVIF_CHILDREN;
		tvitem.cChildren = pTreeItem->has_child_folders ? 1 : 0;
		m_RepoTree.SetItem(&tvitem);
	}
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
		// Check if there is an Inplace Edit active:
		// inplace edits are done with an edit control, where the parent
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
		url += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(pItem->absolutepath))) + _T("\r\n");
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
		m_RepoList.SetFocus();
		m_RepoList.EditLabel(selIndex);
	}
	else
	{
		m_RepoTree.SetFocus();
		m_RepoTree.EditLabel(m_RepoTree.GetSelectedItem());
	}
	m_blockEvents = false;
}

void CRepositoryBrowser::OnRefresh()
{
	m_blockEvents = true;
	RefreshNode(m_RepoTree.GetSelectedItem(), true, !!(GetKeyState(VK_CONTROL)&0x8000));
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
					m_RepoList.ShowText(_T(" "), true);
					RefreshNode(hSelItem);
					m_RepoList.ClearText();
				}

				FillList(&pTreeItem->children);
				m_barRepository.ShowUrl(pTreeItem->url, GetRevision());
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
	LPNMITEMACTIVATE pNmItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	if (m_blockEvents)
		return;

	if (pNmItemActivate->iItem < 0)
		return;
	CItem * pItem = (CItem*)m_RepoList.GetItemData(pNmItemActivate->iItem);
	if ((pItem)&&(pItem->kind == svn_node_dir))
	{
		// a doubleclick on a folder results in selecting that folder
		ChangeToUrl(pItem->absolutepath, m_initialRev);
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
	CItem * pItem1 = (CItem*)pThis->m_RepoList.GetItemData(lParam1);
	CItem * pItem2 = (CItem*)pThis->m_RepoList.GetItemData(lParam2);
	int nRet = 0;
	switch (pThis->m_nSortedColumn)
	{
	case 1: // extension
		nRet = pThis->m_RepoList.GetItemText(lParam1, 1).CompareNoCase(pThis->m_RepoList.GetItemText(lParam2, 1));
		if (nRet != 0)
			break;
		// fall through
	case 2: // revision number
		nRet = pItem1->created_rev - pItem2->created_rev;
		if (nRet != 0)
			break;
		// fall through
	case 3: // author
		nRet = pItem1->author.CompareNoCase(pItem2->author);
		if (nRet != 0)
			break;
		// fall through
	case 4: // size
		nRet = int(pItem1->size - pItem2->size);
		if (nRet != 0)
			break;
		// fall through
	case 5: // date
		nRet = (pItem1->time - pItem2->time) > 0 ? 1 : -1;
		if (nRet != 0)
			break;
		// fall through
	case 6: // lock owner
		nRet = pItem1->lockowner.CompareNoCase(pItem2->lockowner);
		if (nRet != 0)
			break;
		// fall through
	case 0:	// filename
		nRet = CStringUtils::CompareNumerical(pItem1->path, pItem2->path);
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
				m_barRepository.ShowUrl(pItem->absolutepath, GetRevision());
		}
	}
}

void CRepositoryBrowser::OnLvnEndlabeleditRepolist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = 0;
	if (pDispInfo->item.pszText == NULL)
		return;
	// rename the item in the repository
	CItem * pItem = (CItem *)m_RepoList.GetItemData(pDispInfo->item.iItem);

	CWaitCursorEx wait_cursor;
	CInputLogDlg input(this);
	input.SetUUID(m_sUUID);
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
	if (input.DoModal() == IDOK)
	{
		m_bCancelled = false;
		if (!Move(CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(pItem->absolutepath)))),
			targetUrl,
			true, input.GetLogMessage()))
		{
			wait_cursor.Hide();
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}
		*pResult = TRUE;
		RefreshNode(m_RepoTree.GetSelectedItem(), true);
	}
}

void CRepositoryBrowser::OnTvnEndlabeleditRepotree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	*pResult = 0;
	if (pTVDispInfo->item.pszText == NULL)
		return;

	// rename the item in the repository
	CTreeItem * pItem = (CTreeItem *)m_RepoTree.GetItemData(m_RepoTree.GetSelectedItem());
	if (pItem == NULL)
		return;

	CWaitCursorEx wait_cursor;
	CInputLogDlg input(this);
	input.SetUUID(m_sUUID);
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
	if (input.DoModal() == IDOK)
	{
		m_bCancelled = false;
		if (!Move(CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(pItem->url)))),
			targetUrl,
			true, input.GetLogMessage()))
		{
			wait_cursor.Hide();
			CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}
		*pResult = TRUE;
		RefreshNode(m_RepoTree.GetSelectedItem(), true);
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
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (m_RepoList.HasText())
		return;
	CIDropSource* pdsrc = new CIDropSource;
	if (pdsrc == NULL)
		return;
	pdsrc->AddRef();

	CTSVNPathList sourceURLs;
	POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
	int index = -1;
	while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
	{
		CItem * pItem = (CItem *)m_RepoList.GetItemData(index);
		sourceURLs.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(pItem->absolutepath))));
	}

	SVNDataObject* pdobj = new SVNDataObject(sourceURLs, GetRevision(), GetRevision());
	if (pdobj == NULL)
	{
		delete pdsrc;
		return;
	}
	pdobj->AddRef();

	CDragSourceHelper dragsrchelper;
	dragsrchelper.InitializeFromWindow(m_RepoList.GetSafeHwnd(), pNMLV->ptAction, pdobj);
	// Initiate the Drag & Drop
	DWORD dwEffect;
	::DoDragDrop(pdobj, pdsrc, DROPEFFECT_MOVE|DROPEFFECT_COPY, &dwEffect);
	pdsrc->Release();
	pdobj->Release();
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
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	if (m_blockEvents)
		return;

	CTreeItem * pTreeItem = (CTreeItem *)pNMTreeView->itemNew.lParam;

	CIDropSource* pdsrc = new CIDropSource;
	if (pdsrc == NULL)
		return;
	pdsrc->AddRef();

	CTSVNPathList sourceURLs;
	sourceURLs.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(pTreeItem->url))));

	SVNDataObject* pdobj = new SVNDataObject(sourceURLs, GetRevision(), GetRevision());
	if (pdobj == NULL)
	{
		delete pdsrc;
		return;
	}
	pdobj->AddRef();

	CDragSourceHelper dragsrchelper;
	dragsrchelper.InitializeFromWindow(m_RepoTree.GetSafeHwnd(), pNMTreeView->ptDrag, pdobj);
	// Initiate the Drag & Drop
	DWORD dwEffect;
	::DoDragDrop(pdobj, pdsrc, DROPEFFECT_MOVE|DROPEFFECT_COPY, &dwEffect);
	pdsrc->Release();
	pdobj->Release();
}


bool CRepositoryBrowser::OnDrop(const CTSVNPath& target, const CTSVNPathList& pathlist, DWORD dwEffect)
{
	ATLTRACE(_T("dropped %ld items on %s, dwEffect is %ld\n"), pathlist.GetCount(), (LPCTSTR)target.GetSVNPathString(), dwEffect);
	if (pathlist.GetCount() == 0)
		return false;

	CString targetName = pathlist[0].GetFileOrDirectoryName();
	if (m_bRightDrag)
	{
		// right dragging means we have to show a context menu
		POINT pt;
		GetCursorPos(&pt);
		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			CString temp(MAKEINTRESOURCE(IDS_REPOBROWSE_COPYDROP));
			popup.AppendMenu(MF_STRING | MF_ENABLED, 1, temp);
			temp.LoadString(IDS_REPOBROWSE_MOVEDROP);
			popup.AppendMenu(MF_STRING | MF_ENABLED, 2, temp);
			if ((pathlist.GetCount() == 1)&&(PathIsURL(pathlist[0].GetSVNPathString())))
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
	// check the first item in the pathlist:
	// if it's an url, we do a copy or move operation
	// if it's a local path, we do an import
	if (PathIsURL(pathlist[0].GetSVNPathString()))
	{
		// drag-n-drop inside the repobrowser
		CInputLogDlg input(this);
		input.SetUUID(m_sUUID);
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
			m_bCancelled = false;
			CWaitCursorEx wait_cursor;
			BOOL bRet = FALSE;
			if (dwEffect == DROPEFFECT_COPY)
				if (pathlist.GetCount() == 1)
					bRet = Copy(pathlist, CTSVNPath(target.GetSVNPathString() + _T("/") + targetName), GetRevision(), GetRevision(), input.GetLogMessage(), false);
				else
					bRet = Copy(pathlist, target, GetRevision(), GetRevision(), input.GetLogMessage(), true);
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
				// if the copy/move operation was to the currently shown url,
				// update the current view. Otherwise mark the target URL as 'not fetched'.
				HTREEITEM hSelected = m_RepoTree.GetSelectedItem();
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
		input.SetUUID(m_sUUID);
		CString sHint;
		if (pathlist.GetCount() == 1)
			sHint.Format(IDS_INPUT_IMPORTFILEFULL, pathlist[0].GetWinPath(), (LPCTSTR)(target.GetSVNPathString() + _T("/") + pathlist[0].GetFileOrDirectoryName()));
		else
			sHint.Format(IDS_INPUT_IMPORTFILES, pathlist.GetCount());
		input.SetActionText(sHint);

		if (input.DoModal() == IDOK)
		{
			m_bCancelled = false;
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
				if (hSelected)
				{
					CTreeItem * pItem = (CTreeItem*)m_RepoTree.GetItemData(hSelected);
					if (pItem)
					{
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
	return CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(url.GetSVNPathString())));
}

void CRepositoryBrowser::OnContextMenu(CWnd* pWnd, CPoint point)
{
	HTREEITEM hSelectedTreeItem = NULL;
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
	m_bCancelled = false;
	CTSVNPathList urlList;
	CTSVNPathList urlListEscaped;
	int nFolders = 0;
	int nLocked = 0;
	if (pWnd == &m_RepoList)
	{
		CString urls;

		POSITION pos = m_RepoList.GetFirstSelectedItemPosition();
		int index = -1;
		while ((index = m_RepoList.GetNextSelectedItem(pos))>=0)
		{
			CItem * pItem = (CItem *)m_RepoList.GetItemData(index);
			CString absPath = pItem->absolutepath;
			absPath.Replace(_T("\\"), _T("%5C"));
			urlList.AddPath(CTSVNPath(absPath));
			urlListEscaped.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(absPath))));
			if (pItem->kind == svn_node_dir)
				nFolders++;
			if (!pItem->locktoken.IsEmpty())
				nLocked++;
		}
		if (urlList.GetCount() == 0)
		{
			// Right-click outside any list control items. It may be the background,
			// but it also could be the list control headers.
			CRect hr;
			m_RepoList.GetHeaderCtrl()->GetWindowRect(&hr);
			if (!hr.PtInRect(point))
			{
				// Seems to be a right-click on the listview background.
				// Use the currently selected item in the tree view as the source.
				m_blockEvents = true;
				hSelectedTreeItem = m_RepoTree.GetSelectedItem();
				if (hSelectedTreeItem)
				{
					m_RepoTree.SetItemState(hSelectedTreeItem, 0, TVIS_SELECTED);
					m_blockEvents = false;
					m_RepoTree.SetItemState(hSelectedTreeItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
					CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hSelectedTreeItem);
					if (pTreeItem)
					{
						urlList.AddPath(CTSVNPath(pTreeItem->url));
						urlListEscaped.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(pTreeItem->url))));
						nFolders++;
					}
				}
			}
		}
	}
	if ((pWnd == &m_RepoTree)||(urlList.GetCount() == 0))
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
			CTreeItem * pTreeItem = (CTreeItem *)m_RepoTree.GetItemData(hItem);
			if (pTreeItem)
			{
				urlList.AddPath(CTSVNPath(pTreeItem->url));
				urlListEscaped.AddPath(CTSVNPath(EscapeUrl(CTSVNPath(pTreeItem->url))));
				nFolders++;
			}
		}
	}

	if (urlList.GetCount() == 0)
		return;

	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		CString temp;
		if (urlList.GetCount() == 1)
		{
			if (nFolders == 0)
			{
				// Let "Open" be the very first entry, like in Explorer
				temp.LoadString(IDS_REPOBROWSE_OPEN);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPEN, temp);		// "open"
				temp.LoadString(IDS_LOG_POPUP_OPENWITH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_OPENWITH, temp);	// "open with..."
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SHOWLOG, temp);			// "Show Log..."
			// the revision graph on the repository root would be empty. We
			// don't show the context menu entry there.
			if (urlList[0].GetSVNPathString().Compare(m_strReposRoot)!=0)
			{
				temp.LoadString(IDS_MENUREVISIONGRAPH);							// "Revision graph"
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVGRAPH, temp);
			}
			if (nFolders == 0)
			{
				temp.LoadString(IDS_MENUBLAME);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BLAME, temp);		// "Blame..."
			}
			if (!m_ProjectProperties.sWebViewerRev.IsEmpty())
			{
				temp.LoadString(IDS_LOG_POPUP_VIEWREV);							// "View revision in webviewer"
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_VIEWREV, temp);
			}
			if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
			{
				temp.LoadString(IDS_LOG_POPUP_VIEWPATHREV);						// "View revision for path in webviewer"
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_VIEWPATHREV, temp);
			}
			if ((!m_ProjectProperties.sWebViewerPathRev.IsEmpty())||
				(!m_ProjectProperties.sWebViewerRev.IsEmpty()))
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			if (nFolders)
			{
				temp.LoadString(IDS_MENUEXPORT);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_EXPORT, temp);		// "Export"
			}
		}
		// We allow checkout of multiple folders at once (we do that one by one)
		if (nFolders == urlList.GetCount())
		{
			temp.LoadString(IDS_MENUCHECKOUT);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_CHECKOUT, temp);		// "Checkout.."
		}
		if (urlList.GetCount() == 1)
		{
			if (nFolders)
			{
				temp.LoadString(IDS_REPOBROWSE_REFRESH);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REFRESH, temp);		// "Refresh"
			}
			popup.AppendMenu(MF_SEPARATOR, NULL);				

			if (GetRevision().IsHead())
			{
				if (nFolders)
				{
					temp.LoadString(IDS_REPOBROWSE_MKDIR);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_MKDIR, temp);	// "create directory"

					temp.LoadString(IDS_REPOBROWSE_IMPORT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_IMPORT, temp);	// "Add/Import File"

					temp.LoadString(IDS_REPOBROWSE_IMPORTFOLDER);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_IMPORTFOLDER, temp);	// "Add/Import Folder"

					popup.AppendMenu(MF_SEPARATOR, NULL);
				}
				if (nLocked)
				{
					temp.LoadString(IDS_MENU_UNLOCKFORCE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_BREAKLOCK, temp);	// "Break Lock"
				}

				temp.LoadString(IDS_REPOBROWSE_RENAME);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_RENAME, temp);		// "Rename"
			}
		}
		if (urlList.GetCount() > 0)
		{
			if (GetRevision().IsHead())
			{
				temp.LoadString(IDS_REPOBROWSE_DELETE);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_DELETE, temp);		// "Remove"
			}
			if (nFolders == 0)
			{
				temp.LoadString(IDS_REPOBROWSE_SAVEAS);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);		// "Save as..."
			}
			if ((urlList.GetCount() == nFolders)||(nFolders == 0))
			{
				temp.LoadString(IDS_REPOBROWSE_COPYTOWC);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPYTOWC, temp);	// "Copy To Working Copy..."
			}
		}
		if (urlList.GetCount() == 1)
		{
			temp.LoadString(IDS_REPOBROWSE_COPY);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPYTO, temp);			// "Copy To..."

			temp.LoadString(IDS_REPOBROWSE_URLTOCLIPBOARD);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_URLTOCLIPBOARD, temp);	// "Copy URL to clipboard"

			popup.AppendMenu(MF_SEPARATOR, NULL);

			temp.LoadString(IDS_REPOBROWSE_SHOWPROP);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_PROPS, temp);			// "Show Properties"


			if (nFolders == 1)
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
				temp.LoadString(IDS_REPOBROWSE_PREPAREDIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_PREPAREDIFF, temp);	// "Mark for comparison"

				if ((m_diffKind == svn_node_dir)&&(!m_diffURL.IsEquivalentTo(urlList[0])))
				{
					temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF, temp);		// "Show differences as unified diff"

					temp.LoadString(IDS_REPOBROWSE_SHOWDIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_DIFF, temp);		// "Compare URLs"
				}
			}
		}
		if (urlList.GetCount() == 2)
		{
			if ((nFolders == 2)||(nFolders == 0))
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
				temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF, temp);		// "Show differences as unified diff"
				temp.LoadString(IDS_REPOBROWSE_SHOWDIFF);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_DIFF, temp);		// "Compare URLs"
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			temp.LoadString(IDS_MENULOG);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SHOWLOG, temp);
		}
		if ((urlList.GetCount() == 1) &&
			m_path.Exists() && 
			CTSVNPath(m_InitialUrl).IsAncestorOf(urlList[0]))
		{
			CTSVNPath wcPath = m_path;
			wcPath.AppendPathString(urlList[0].GetWinPathString().Mid(m_InitialUrl.GetLength()));
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
					temp.LoadString(IDS_LOG_POPUP_UPDATE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UPDATE, temp);		// "Update item to revision"
				}
			}
		}
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

		DialogEnableWindow(IDOK, FALSE);
		bool bOpenWith = false;
		switch (cmd)
		{
		case ID_UPDATE:
			{
				CTSVNPath wcPath = m_path;
				wcPath.AppendPathString(urlList[0].GetWinPathString().Mid(m_InitialUrl.GetLength()));
				CString sCmd;
				sCmd.Format(_T("\"%s\" /command:update /path:\"%s\" /rev"),
					CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"), wcPath.GetWinPath());

				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_PREPAREDIFF:
			{
				m_RepoTree.SetItemState(FindUrl(m_diffURL.GetSVNPathString(), false), 0, TVIS_BOLD);
				if (urlList.GetCount() == 1)
				{
					m_diffURL = urlList[0];
					m_diffKind = nFolders ? svn_node_dir : svn_node_file;
					// make the marked tree item bold
					if (m_diffKind == svn_node_dir)
					{
						m_RepoTree.SetItemState(FindUrl(m_diffURL.GetSVNPathString(), false), TVIS_BOLD, TVIS_BOLD);
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
				for (int i=0; i<urlList.GetCount(); ++i)
					url += CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(urlList[i].GetSVNPathString()))) + _T("\r\n");
				url.TrimRight(_T("\r\n"));
				CStringUtils::WriteAsciiStringToClipboard(url);
			}
			break;
		case ID_SAVEAS:
			{
				CTSVNPath tempfile;
				bool bSavePathOK = AskForSavePath(urlList, tempfile, nFolders > 0);
				if (bSavePathOK)
				{
					CWaitCursorEx wait_cursor;

					CString saveurl;
					CProgressDlg progDlg;
					int counter = 0;		// the file counter
					progDlg.SetTitle(IDS_REPOBROWSE_SAVEASPROGTITLE);
					progDlg.SetAnimation(IDR_DOWNLOAD);
					progDlg.ShowModeless(GetSafeHwnd());
					progDlg.SetProgress((DWORD)0, (DWORD)urlList.GetCount());
					SetAndClearProgressInfo(&progDlg);
					for (int i=0; i<urlList.GetCount(); ++i)
					{
						saveurl = EscapeUrl(urlList[i]);
						CTSVNPath savepath = tempfile;
						if (tempfile.IsDirectory())
							savepath.AppendPathString(urlList[i].GetFileOrDirectoryName());
						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, saveurl, GetRevision().ToString());
						progDlg.SetLine(1, sInfoLine);
						if (!Cat(CTSVNPath(saveurl), GetRevision(), GetRevision(), savepath)||(progDlg.HasUserCancelled()))
						{
							wait_cursor.Hide();
							progDlg.Stop();
							SetAndClearProgressInfo((HWND)NULL);
							if (!progDlg.HasUserCancelled())
								CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						}
						counter++;
						progDlg.SetProgress((DWORD)counter, (DWORD)urlList.GetCount());
					}
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
				}
			}
			break;
		case ID_SHOWLOG:
			{
				if (urlList.GetCount() == 2)
				{
					// get log of first URL
					CString sCopyFrom1, sCopyFrom2;
					SVNLogHelper helper;
					helper.SetRepositoryRoot(m_strReposRoot);
					SVNRev rev1 = helper.GetCopyFromRev(CTSVNPath(EscapeUrl(urlList[0])), GetRevision(), sCopyFrom1);
					if (!rev1.IsValid())
					{
						CMessageBox::Show(this->m_hWnd, helper.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						break;
					}
					SVNRev rev2 = helper.GetCopyFromRev(CTSVNPath(EscapeUrl(urlList[1])), GetRevision(), sCopyFrom2);
					if (!rev2.IsValid())
					{
						CMessageBox::Show(this->m_hWnd, helper.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						break;
					}
					if ((sCopyFrom1.IsEmpty())||(sCopyFrom1.Compare(sCopyFrom2)!=0))
					{
						// no common copyfrom URL, so showing a log between
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
						CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"), sCopyFrom1, rev1.ToString(), rev2.ToString());

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
					sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /startrev:%ld"), 
						CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"), EscapeUrl(urlList[0]), (LONG)GetRevision());

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
				url.Replace(_T("%REVISION%"), GetRevision().ToString());
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
		case ID_VIEWPATHREV:
			{
				CString relurl = EscapeUrl(urlList[0]);
				relurl = relurl.Mid(m_strReposRoot.GetLength());
				CString weburl = m_ProjectProperties.sWebViewerPathRev;
				weburl.Replace(_T("%REVISION%"), GetRevision().ToString());
				weburl.Replace(_T("%PATH%"), relurl);
				if (!weburl.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), weburl, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
		case ID_CHECKOUT:
			{
				CString itemsToCheckout;
				for (int i=0; i<urlList.GetCount(); ++i)
				{
					itemsToCheckout += EscapeUrl(urlList[i]) + _T("*");
				}
				itemsToCheckout.TrimRight('*');
				CString sCmd;
				sCmd.Format(_T("\"%s\" /command:checkout /url:\"%s\" /revision:%ld"), 
					CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe"), (LPCTSTR)itemsToCheckout, (LONG)GetRevision());

				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_EXPORT:
			{
				CExportDlg dlg;
				dlg.m_URL = EscapeUrl(urlList[0]);
				dlg.Revision = GetRevision();
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
				CRevisionGraphDlg dlg;
				dlg.SetPath(EscapeUrl(urlList[0]));
				dlg.DoModal();
			}
			break;
		case ID_OPENWITH:
			bOpenWith = true;
		case ID_OPEN:
			{
				// if we're on HEAD and the repository is available via http or https,
				// we just open the browser with that url.
				if (GetRevision().IsHead() && (bOpenWith==false))
				{
					if (urlList[0].GetSVNPathString().Left(4).CompareNoCase(_T("http")) == 0)
					{
						CString sBrowserUrl = EscapeUrl(urlList[0]);

						ShellExecute(NULL, _T("open"), sBrowserUrl, NULL, NULL, SW_SHOWNORMAL);
						break;
					}
				}
				// in all other cases, we have to 'cat' the file and open it.
				CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, urlList[0], GetRevision());
				CWaitCursorEx wait_cursor;
				CProgressDlg progDlg;
				progDlg.SetTitle(IDS_APPNAME);
				progDlg.SetAnimation(IDR_DOWNLOAD);
				CString sInfoLine;
				sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, urlList[0].GetFileOrDirectoryName(), GetRevision().ToString());
				progDlg.SetLine(1, sInfoLine);
				SetAndClearProgressInfo(&progDlg);
				progDlg.ShowModeless(m_hWnd);
				if (!Cat(urlList[0], GetRevision(), GetRevision(), tempfile))
				{
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
					wait_cursor.Hide();
					CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					break;;
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
					CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
					cmd += tempfile.GetWinPathString();
					CAppUtils::LaunchApplication(cmd, NULL, false);
				}
			}
			break;
		case ID_DELETE:
			{
				CWaitCursorEx wait_cursor;
				CInputLogDlg input(this);
				input.SetUUID(m_sUUID);
				input.SetProjectProperties(&m_ProjectProperties);
				CString hint;
				if (urlList.GetCount() == 1)
					hint.Format(IDS_INPUT_REMOVEONE, urlList[0].GetFileOrDirectoryName());
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
					RefreshNode(m_RepoTree.GetSelectedItem(), true);
				}
			}
			break;
		case ID_BREAKLOCK:
			{
				if (!Unlock(urlListEscaped, TRUE))
				{
					CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return;
				}
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
					input.SetUUID(m_sUUID);
					input.SetProjectProperties(&m_ProjectProperties);
					CString sHint;
					sHint.Format(IDS_INPUT_IMPORTFOLDER, (LPCTSTR)svnPath.GetSVNPathString(), (LPCTSTR)(urlList[0].GetSVNPathString()+_T("/")+filename));
					input.SetActionText(sHint);
					if (input.DoModal() == IDOK)
					{
						CProgressDlg progDlg;
						progDlg.SetTitle(IDS_APPNAME);
						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSIMPORT, filename);
						progDlg.SetLine(1, sInfoLine);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);
						if (!Import(svnPath, 
							CTSVNPath(EscapeUrl(CTSVNPath(urlList[0].GetSVNPathString()+_T("/")+filename))), 
							input.GetLogMessage(), 
							&m_ProjectProperties, 
							svn_depth_empty, 
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
					input.SetUUID(m_sUUID);
					input.SetProjectProperties(&m_ProjectProperties);
					CString sHint;
					sHint.Format(IDS_INPUT_IMPORTFILEFULL, path.GetWinPath(), (LPCTSTR)(urlList[0].GetSVNPathString()+_T("/")+filename));
					input.SetActionText(sHint);
					if (input.DoModal() == IDOK)
					{
						CProgressDlg progDlg;
						progDlg.SetTitle(IDS_APPNAME);
						CString sInfoLine;
						sInfoLine.Format(IDS_PROGRESSIMPORT, filename);
						progDlg.SetLine(1, sInfoLine);
						SetAndClearProgressInfo(&progDlg);
						progDlg.ShowModeless(m_hWnd);
						if (!Import(path, 
							CTSVNPath(EscapeUrl(CTSVNPath(urlList[0].GetSVNPathString()+_T("/")+filename))), 
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
						m_RepoTree.EditLabel(m_RepoTree.GetSelectedItem());
					}
				}
				else if (pWnd == &m_RepoTree)
				{
					m_RepoTree.SetFocus();
					m_RepoTree.EditLabel(m_RepoTree.GetSelectedItem());
				}
			}
			break;
		case ID_COPYTO:
			{
				CRenameDlg dlg;
				dlg.m_name = urlList[0].GetSVNPathString();
				dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_COPY);
				CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
				if (dlg.DoModal() == IDOK)
				{
					CWaitCursorEx wait_cursor;
					CInputLogDlg input(this);
					input.SetUUID(m_sUUID);
					input.SetProjectProperties(&m_ProjectProperties);
					CString sHint;
					sHint.Format(IDS_INPUT_COPY, (LPCTSTR)urlList[0].GetSVNPathString(), (LPCTSTR)dlg.m_name);
					input.SetActionText(sHint);
					if (!CTSVNPath(dlg.m_name).IsValidOnWindows())
					{
						if (CMessageBox::Show(GetSafeHwnd(), IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONINFORMATION|MB_YESNO) != IDYES)
							break;
					}
					if (input.DoModal() == IDOK)
					{
						if (!Copy(urlList, CTSVNPath(dlg.m_name), GetRevision(), GetRevision(), input.GetLogMessage()))
						{
							wait_cursor.Hide();
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							return;
						}
						if (GetRevision().IsHead())
						{
							RefreshNode(m_RepoTree.GetSelectedItem(), true);
						}
					}
				}
			}
			break;
		case ID_COPYTOWC:
			{
				CTSVNPath tempfile;
				bool bSavePathOK = AskForSavePath(urlList, tempfile, nFolders > 0);
				if (bSavePathOK)
				{
					CWaitCursorEx wait_cursor;

					CProgressDlg progDlg;
					progDlg.SetAnimation(IDR_DOWNLOAD);
					progDlg.SetTitle(IDS_APPNAME);
					SetAndClearProgressInfo(&progDlg);
					progDlg.ShowModeless(m_hWnd);

					bool bCopyAsChild = (urlList.GetCount() > 1);
					if (!Copy(urlList, tempfile, GetRevision(), GetRevision(), CString(), bCopyAsChild)||(progDlg.HasUserCancelled()))
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
				CRenameDlg dlg;
				dlg.m_name = _T("");
				dlg.m_windowtitle.LoadString(IDS_REPOBROWSE_MKDIR);
				CStringUtils::RemoveAccelerators(dlg.m_windowtitle);
				if (dlg.DoModal() == IDOK)
				{
					CWaitCursorEx wait_cursor;
					CInputLogDlg input(this);
					input.SetUUID(m_sUUID);
					input.SetProjectProperties(&m_ProjectProperties);
					CString sHint;
					sHint.Format(IDS_INPUT_MKDIR, (LPCTSTR)(urlList[0].GetSVNPathString()+_T("/")+dlg.m_name.Trim()));
					input.SetActionText(sHint);
					if (input.DoModal() == IDOK)
					{
						// when creating the new folder, also trim any whitespace chars from it
						if (!MakeDir(CTSVNPathList(CTSVNPath(EscapeUrl(CTSVNPath(urlList[0].GetSVNPathString()+_T("/")+dlg.m_name.Trim())))), input.GetLogMessage()))
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
				RefreshNode(urlList[0].GetSVNPathString(), true);
			}
			break;
		case ID_GNUDIFF:
			{
				m_bCancelled = false;
				SVNDiff diff(this, this->m_hWnd, true);
				if (urlList.GetCount() == 1)
					diff.ShowUnifiedDiff(CTSVNPath(EscapeUrl(urlList[0])), GetRevision(), 
										CTSVNPath(EscapeUrl(m_diffURL)), GetRevision());
				else
					diff.ShowUnifiedDiff(CTSVNPath(EscapeUrl(urlList[0])), GetRevision(), 
										CTSVNPath(EscapeUrl(urlList[1])), GetRevision());
			}
			break;
		case ID_DIFF:
			{
				m_bCancelled = false;
				SVNDiff diff(this, this->m_hWnd, true);
				diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
				if (urlList.GetCount() == 1)
					diff.ShowCompare(CTSVNPath(EscapeUrl(urlList[0])), GetRevision(), 
									CTSVNPath(EscapeUrl(m_diffURL)), GetRevision(), SVNRev(), true);
				else
					diff.ShowCompare(CTSVNPath(EscapeUrl(urlList[0])), GetRevision(), 
									CTSVNPath(EscapeUrl(urlList[1])), GetRevision(), SVNRev(), true);
			}
			break;
		case ID_PROPS:
			{
				if (GetRevision().IsHead())
				{
					CEditPropertiesDlg dlg;
					CTSVNPathList escapedlist;
					for (int i=0; i<urlList.GetCount(); ++i)
					{
						escapedlist.AddPath(CTSVNPath(EscapeUrl(urlList[i])));
					}
					dlg.SetPathList(escapedlist);
					dlg.SetRevision(GetHEADRevision(urlList[0]));
					dlg.DoModal();
				}
				else
				{
					CPropDlg dlg;
					dlg.m_rev = GetRevision();
					dlg.m_Path = CTSVNPath(EscapeUrl(urlList[0]));
					dlg.DoModal();
				}
			}
			break;
		case ID_BLAME:
			{
				CBlameDlg dlg;
				dlg.EndRev = GetRevision();
				if (dlg.DoModal() == IDOK)
				{
					CBlame blame;
					CString tempfile;
					CString logfile;
					tempfile = blame.BlameToTempFile(CTSVNPath(EscapeUrl(urlList[0])), dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, _T(""), TRUE, TRUE);
					if (!tempfile.IsEmpty())
					{
						if (dlg.m_bTextView)
						{
							//open the default text editor for the result file
							CAppUtils::StartTextViewer(tempfile);
						}
						else
						{
							CString sParams = _T("/path:\"") + urlList[0].GetSVNPathString() + _T("\" ");
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(urlList[0].GetFileOrDirectoryName()),sParams))
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
		default:
			break;
		}
		DialogEnableWindow(IDOK, TRUE);
	}
}


bool CRepositoryBrowser::AskForSavePath(const CTSVNPathList& urlList, CTSVNPath &tempfile, bool bFolder)
{
	bool bSavePathOK = false;
	if ((!bFolder)&&(urlList.GetCount() == 1))
	{
		CString savePath;
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


