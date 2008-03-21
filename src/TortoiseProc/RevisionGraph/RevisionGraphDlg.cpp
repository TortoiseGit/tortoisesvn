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
#include "MemDC.h"
#include <gdiplus.h>
#include "Revisiongraphdlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include "RevGraphFilterDlg.h"
#include ".\revisiongraphdlg.h"
#include "RepositoryInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

IMPLEMENT_DYNAMIC(CRevisionGraphDlg, CResizableStandAloneDialog)
CRevisionGraphDlg::CRevisionGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRevisionGraphDlg::IDD, pParent)
	, m_hAccel(NULL)
	, m_bFetchLogs(true)
	, m_fZoomFactor(1.0)
{
	DWORD dwOpts = CRegStdWORD(_T("Software\\TortoiseSVN\\RevisionGraphOptions"), 0x211);

	m_options.groupBranches = ((dwOpts & 0x01) != 0);
	m_options.includeSubPathChanges = ((dwOpts & 0x02) != 0);
	m_options.oldestAtTop = ((dwOpts & 0x04) != 0);
	m_options.showHEAD = ((dwOpts & 0x08) != 0);
	m_options.reduceCrossLines = ((dwOpts & 0x10) != 0);
	m_options.exactCopySources = ((dwOpts & 0x20) != 0);
	m_options.foldTags = ((dwOpts & 0x80) != 0);
	m_options.removeDeletedOnes = ((dwOpts & 0x100) != 0);
	m_options.showWCRev = ((dwOpts & 0x200) != 0);
}

CRevisionGraphDlg::~CRevisionGraphDlg()
{
	CRegStdWORD regOpts = CRegStdWORD(_T("Software\\TortoiseSVN\\RevisionGraphOptions"), 1);
	DWORD dwOpts = 0;
	dwOpts |= m_options.groupBranches ? 0x01 : 0;
	dwOpts |= m_options.includeSubPathChanges ? 0x02 : 0;
	dwOpts |= m_options.oldestAtTop ? 0x04 : 0;
	dwOpts |= m_options.showHEAD ? 0x08 : 0;
	dwOpts |= m_options.reduceCrossLines ? 0x10 : 0;
	dwOpts |= m_options.exactCopySources ? 0x20 : 0;
	dwOpts |= m_options.foldTags ? 0x80 : 0;
    dwOpts |= m_options.removeDeletedOnes ? 0x100 : 0;
    dwOpts |= m_options.showWCRev ? 0x200 : 0;
	regOpts = dwOpts;
}

void CRevisionGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRevisionGraphDlg, CResizableStandAloneDialog)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_VIEW_ZOOMIN, OnViewZoomin)
	ON_COMMAND(ID_VIEW_ZOOMOUT, OnViewZoomout)
	ON_COMMAND(ID_VIEW_ZOOM100, OnViewZoom100)
	ON_COMMAND(ID_VIEW_ZOOMALL, OnViewZoomAll)
	ON_COMMAND(ID_MENUEXIT, OnMenuexit)
	ON_COMMAND(ID_MENUHELP, OnMenuhelp)
	ON_COMMAND(ID_VIEW_COMPAREHEADREVISIONS, OnViewCompareheadrevisions)
	ON_COMMAND(ID_VIEW_COMPAREREVISIONS, OnViewComparerevisions)
	ON_COMMAND(ID_VIEW_UNIFIEDDIFF, OnViewUnifieddiff)
	ON_COMMAND(ID_VIEW_UNIFIEDDIFFOFHEADREVISIONS, OnViewUnifieddiffofheadrevisions)
	ON_COMMAND(ID_VIEW_SHOWALLREVISIONS, &CRevisionGraphDlg::OnViewShowallrevisions)
	ON_COMMAND(ID_VIEW_GROUPBRANCHES, &CRevisionGraphDlg::OnViewArrangedbypath)
	ON_COMMAND(ID_FILE_SAVEGRAPHAS, &CRevisionGraphDlg::OnFileSavegraphas)
	ON_COMMAND(ID_VIEW_TOPDOWN, &CRevisionGraphDlg::OnViewTopDown)
	ON_COMMAND(ID_VIEW_SHOWHEAD, &CRevisionGraphDlg::OnViewShowHEAD)
	ON_COMMAND(ID_VIEW_EXACTCOPYSOURCE, &CRevisionGraphDlg::OnViewExactCopySource)
	ON_COMMAND(ID_VIEW_FOLDTAGS, &CRevisionGraphDlg::OnViewFoldTags)
	ON_COMMAND(ID_VIEW_REDUCECROSSLINES, &CRevisionGraphDlg::OnViewReduceCrosslines)
	ON_COMMAND(ID_VIEW_REMOVEDELETEDONES, &CRevisionGraphDlg::OnViewRemoveDeletedOnes)
	ON_COMMAND(ID_VIEW_SHOWWCREV, &CRevisionGraphDlg::OnViewShowWCRev)
	ON_CBN_SELCHANGE(ID_REVGRAPH_ZOOMCOMBO, OnChangeZoom)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_COMMAND(ID_VIEW_FILTER, &CRevisionGraphDlg::OnViewFilter)
	ON_COMMAND(ID_VIEW_SHOWOVERVIEW, &CRevisionGraphDlg::OnViewShowoverview)
END_MESSAGE_MAP()

BOOL CRevisionGraphDlg::InitializeToolbar()
{
	// set up the toolbar
	// add the tool bar to the dialog
	m_ToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_WRAPABLE | TBSTYLE_TRANSPARENT | CBRS_SIZE_DYNAMIC);
	m_ToolBar.LoadToolBar(IDR_REVGRAPHBAR);
	m_ToolBar.ShowWindow(SW_SHOW);
	m_ToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);

	// toolbars aren't true-color without some tweaking:
	{
		CImageList	cImageList;
		CBitmap		cBitmap;
		BITMAP		bmBitmap;

		cBitmap.Attach(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_REVGRAPHBAR),
			IMAGE_BITMAP, 0, 0,
			LR_DEFAULTSIZE|LR_CREATEDIBSECTION));
		cBitmap.GetBitmap(&bmBitmap);

		CSize		cSize(bmBitmap.bmWidth, bmBitmap.bmHeight); 
		int			nNbBtn = cSize.cx/20;
		RGBTRIPLE *	rgb	= (RGBTRIPLE*)(bmBitmap.bmBits);
		COLORREF	rgbMask	= RGB(rgb[0].rgbtRed, rgb[0].rgbtGreen, rgb[0].rgbtBlue);

		cImageList.Create(20, cSize.cy, ILC_COLOR32|ILC_MASK, nNbBtn, 0);
		cImageList.Add(&cBitmap, rgbMask);
		m_ToolBar.SendMessage(TB_SETIMAGELIST, 0, (LPARAM)cImageList.m_hImageList);
		cImageList.Detach(); 
		cBitmap.Detach();
	}
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

#define SNAP_WIDTH 60 //the width of the combo box
	// set up the ComboBox control as a snap mode select box
	// First get the index of the placeholders position in the toolbar
	int index = 0;
	while (m_ToolBar.GetItemID(index) != ID_REVGRAPH_ZOOMCOMBO) index++;

	// next convert that button to a separator and get its position
	m_ToolBar.SetButtonInfo(index, ID_REVGRAPH_ZOOMCOMBO, TBBS_SEPARATOR,
		SNAP_WIDTH);
	RECT rect;
	m_ToolBar.GetItemRect(index, &rect);

	// expand the rectangle to allow the combo box room to drop down
	rect.top+=3;
	rect.bottom += 200;

	// then create the combo box and show it
	if (!m_ToolBar.m_ZoomCombo.CreateEx(WS_EX_RIGHT, WS_CHILD|WS_VISIBLE|CBS_AUTOHSCROLL|CBS_DROPDOWN,
		rect, &m_ToolBar, ID_REVGRAPH_ZOOMCOMBO))
	{
		TRACE0("Failed to create combo-box\n");
		return FALSE;
	}
	m_ToolBar.m_ZoomCombo.ShowWindow(SW_SHOW);

	// set toolbar button styles

	UINT styles[] = { TBBS_CHECKBOX|TBBS_CHECKED
					, TBBS_CHECKBOX
					, 0};

	UINT itemIDs[] = { ID_VIEW_GROUPBRANCHES 
					 , 0		// separate styles by "0"
					 , ID_VIEW_SHOWOVERVIEW
					 , ID_VIEW_TOPDOWN
					 , ID_VIEW_SHOWHEAD
					 , ID_VIEW_EXACTCOPYSOURCE
					 , ID_VIEW_FOLDTAGS
					 , ID_VIEW_REDUCECROSSLINES
                     , ID_VIEW_REMOVEDELETEDONES
                     , ID_VIEW_SHOWWCREV
					 , 0};

	for (UINT* itemID = itemIDs, *style = styles; *style != 0; ++itemID)
	{
		if (*itemID == 0)
		{
			++style;
			continue;
		}

		int index = 0;
		while (m_ToolBar.GetItemID(index) != *itemID)
			index++;
		m_ToolBar.SetButtonStyle(index, m_ToolBar.GetButtonStyle(index)|*style);
	}

	// fill the combo box

	TCHAR* texts[] = { _T("5%")
					 , _T("10%")
					 , _T("20%")
					 , _T("40%")
					 , _T("50%")
					 , _T("100%")
					 , NULL};

	COMBOBOXEXITEM cbei;
	ZeroMemory(&cbei, sizeof cbei);
	cbei.mask = CBEIF_TEXT;

	for (TCHAR** text = texts; *text != NULL; ++text)
	{
		cbei.pszText = *text;
		m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	}

	m_ToolBar.m_ZoomCombo.SetCurSel(0);

	return TRUE;
}

BOOL CRevisionGraphDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	EnableToolTips();

	// set up the status bar
	m_StatusBar.Create(WS_CHILD|WS_VISIBLE|SBT_OWNERDRAW,
		CRect(0,0,0,0), this, 1);
	int strPartDim[2]= {120, -1};
	m_StatusBar.SetParts(2, strPartDim);

	if (InitializeToolbar() != TRUE)
		return FALSE;

	SetOption(ID_VIEW_GROUPBRANCHES, m_options.groupBranches);
	SetOption(ID_VIEW_SHOWALLREVISIONS, m_options.includeSubPathChanges);
	SetOption(ID_VIEW_TOPDOWN, m_options.oldestAtTop);
	SetOption(ID_VIEW_SHOWHEAD, m_options.showHEAD);
	SetOption(ID_VIEW_EXACTCOPYSOURCE, m_options.exactCopySources);
	SetOption(ID_VIEW_FOLDTAGS, m_options.foldTags);
	SetOption(ID_VIEW_REDUCECROSSLINES, m_options.reduceCrossLines);
    SetOption(ID_VIEW_REMOVEDELETEDONES, m_options.removeDeletedOnes);
    SetOption(ID_VIEW_SHOWWCREV, m_options.showWCRev);

	CMenu * pMenu = GetMenu();
	if (pMenu)
	{
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowRevGraphOverview"), FALSE);
		m_Graph.m_bShowOverview = (BOOL)(DWORD)reg;
		pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | (DWORD(reg) ? MF_CHECKED : 0));
		int tbstate = m_ToolBar.GetToolBarCtrl().GetState(ID_VIEW_SHOWOVERVIEW);
		m_ToolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbstate | (DWORD(reg) ? TBSTATE_CHECKED : 0));
	}

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_REVISIONGRAPH));

	RECT graphrect;
	GetGraphRect(&graphrect);
	m_Graph.Init(this, &graphrect);
	m_Graph.SetOwner(this);
	m_Graph.UpdateWindow();

	EnableSaveRestore(_T("RevisionGraphDlg"));

	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
}

UINT CRevisionGraphDlg::WorkerThread(LPVOID pVoid)
{
	CRevisionGraphDlg*	pDlg;
	pDlg = (CRevisionGraphDlg*)pVoid;
	InterlockedExchange(&pDlg->m_Graph.m_bThreadRunning, TRUE);
	CoInitialize(NULL);

    pDlg->m_Graph.m_bNoGraph = FALSE;
    if (pDlg->m_bFetchLogs)
    {
	    pDlg->m_Graph.m_pProgress = new CProgressDlg();
	    pDlg->m_Graph.m_pProgress->SetTitle(IDS_REVGRAPH_PROGTITLE);
	    pDlg->m_Graph.m_pProgress->SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
	    pDlg->m_Graph.m_pProgress->SetTime();
	    pDlg->m_Graph.m_pProgress->SetProgress(0, 100);

	    if (!pDlg->m_Graph.FetchRevisionData(pDlg->m_Graph.m_sPath, pDlg->m_options))
	    {
		    CMessageBox::Show(pDlg->m_hWnd, pDlg->m_Graph.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		    pDlg->m_Graph.m_bNoGraph = TRUE;
	    }

        pDlg->m_Graph.m_pProgress->Stop();
        delete pDlg->m_Graph.m_pProgress;
        pDlg->m_Graph.m_pProgress = NULL;

    	pDlg->m_bFetchLogs = false;	// we've got the logs, no need to fetch them a second time
    }

    // standard plus user settings

    if (pDlg->m_Graph.m_bNoGraph == FALSE)
    {
	    pDlg->m_Graph.AnalyzeRevisionData(pDlg->m_Graph.m_sPath, pDlg->m_options);
	    pDlg->UpdateStatusBar();
    }

	CoUninitialize();
	InterlockedExchange(&pDlg->m_Graph.m_bThreadRunning, FALSE);

    pDlg->m_Graph.SendMessage (CRevisionGraphWnd::WM_WORKERTHREADDONE, 0, 0);
	return 0;
}

void CRevisionGraphDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	CRect rect;
	GetClientRect(&rect);
	if (IsWindow(m_ToolBar))
	{
		RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
	}
	if (IsWindow(m_StatusBar))
	{
		CRect statusbarrect;
		m_StatusBar.GetClientRect(&statusbarrect);
		statusbarrect.top = rect.bottom - statusbarrect.top + statusbarrect.bottom;
		m_StatusBar.MoveWindow(&statusbarrect);
	}
	if (IsWindow(m_Graph))
	{
		CRect rect;
		GetGraphRect(&rect);
		m_Graph.MoveWindow(&rect);
	}
}

BOOL CRevisionGraphDlg::PreTranslateMessage(MSG* pMsg)
{
#define SCROLL_STEP  20
	if (pMsg->message == WM_KEYDOWN)
	{
		int pos = 0;
		switch (pMsg->wParam)
		{
		case VK_UP:
			pos = m_Graph.GetScrollPos(SB_VERT);
			m_Graph.SetScrollPos(SB_VERT, pos - SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_DOWN:
			pos = m_Graph.GetScrollPos(SB_VERT);
			m_Graph.SetScrollPos(SB_VERT, pos + SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_LEFT:
			pos = m_Graph.GetScrollPos(SB_HORZ);
			m_Graph.SetScrollPos(SB_HORZ, pos - SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_RIGHT:
			pos = m_Graph.GetScrollPos(SB_HORZ);
			m_Graph.SetScrollPos(SB_HORZ, pos + SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_PRIOR:
			pos = m_Graph.GetScrollPos(SB_VERT);
			m_Graph.SetScrollPos(SB_VERT, pos - 10*SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_NEXT:
			pos = m_Graph.GetScrollPos(SB_VERT);
			m_Graph.SetScrollPos(SB_VERT, pos + 10*SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_F5:
	        m_Graph.SetDlgTitle (false);

        	LogCache::CRepositoryInfo& cachedProperties 
                = m_Graph.svn.GetLogCachePool()->GetRepositoryInfo();
            cachedProperties.ResetHeadRevision (CTSVNPath (m_Graph.GetReposRoot()));

            m_bFetchLogs = true;
            StartWorkerThread();

			break;
		}
	}
	if ((m_hAccel)&&(pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST))
	{
		return TranslateAccelerator(m_hWnd,m_hAccel,pMsg);
	}
	return __super::PreTranslateMessage(pMsg);
}

void CRevisionGraphDlg::OnViewZoomin()
{
	if (m_fZoomFactor < 2.0)
	{
		m_fZoomFactor = m_fZoomFactor + (m_fZoomFactor*0.1f);
		m_Graph.DoZoom(m_fZoomFactor);
		UpdateZoomBox();
	}
}

void CRevisionGraphDlg::OnViewZoomout()
{
	if ((m_Graph.m_node_space_left > 1) || (m_Graph.m_node_space_right > 1) || (m_Graph.m_node_space_line > 1) ||
		(m_Graph.m_node_rect_height > 1) || (m_Graph.m_node_space_top > 1) || (m_Graph.m_node_space_bottom > 1))
	{
		m_fZoomFactor = m_fZoomFactor - (m_fZoomFactor*0.1f);
		m_Graph.DoZoom(m_fZoomFactor);
		UpdateZoomBox();
	}
}

void CRevisionGraphDlg::OnViewZoom100()
{
	m_fZoomFactor = 1.0;
	m_Graph.DoZoom(m_fZoomFactor);
	UpdateZoomBox();
}

void CRevisionGraphDlg::OnViewZoomAll()
{
	// zoom the graph so that it is completely visible in the window
	CRect windowrect;
	m_Graph.DoZoom(1.0);
	GetGraphRect(windowrect);
	CRect * viewrect = m_Graph.GetViewSize();
	float horzfact = float(viewrect->Width())/float(windowrect.Width());
	float vertfact = float(viewrect->Height())/float(windowrect.Height());
	float fZoom = 1.0f/(max(horzfact, vertfact));
	if (fZoom > 1.0f)
		fZoom = 1.0f;
	int trycounter = 0;
	m_fZoomFactor = fZoom;
	while ((trycounter < 5)&&((viewrect->Width()>windowrect.Width())||(viewrect->Height()>windowrect.Height())))
	{
		m_fZoomFactor = fZoom;
		m_Graph.DoZoom(m_fZoomFactor);
		viewrect = m_Graph.GetViewSize();
		fZoom *= 0.95f;
		trycounter++;
	}
	UpdateZoomBox();
}

void CRevisionGraphDlg::OnMenuexit()
{
	if (!m_Graph.m_bThreadRunning)
		EndDialog(IDOK);
}

void CRevisionGraphDlg::OnMenuhelp()
{
	OnHelp();
}

void CRevisionGraphDlg::OnViewCompareheadrevisions()
{
	m_Graph.CompareRevs(true);
}

void CRevisionGraphDlg::OnViewComparerevisions()
{
	m_Graph.CompareRevs(false);
}

void CRevisionGraphDlg::OnViewUnifieddiff()
{
	m_Graph.UnifiedDiffRevs(false);
}

void CRevisionGraphDlg::OnViewUnifieddiffofheadrevisions()
{
	m_Graph.UnifiedDiffRevs(true);
}

void CRevisionGraphDlg::SetOption(int controlID, bool option)
{
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;
	int tbstate = m_ToolBar.GetToolBarCtrl().GetState(controlID);
	if (option)
	{
		pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_CHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(controlID, tbstate | TBSTATE_CHECKED);
	}
	else
	{
		pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_UNCHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(controlID, tbstate & (~TBSTATE_CHECKED));
	}
}

void CRevisionGraphDlg::OnToggleOption(int controlID, bool& option)
{
	if (m_Graph.m_bThreadRunning)
	{
		int state = m_ToolBar.GetToolBarCtrl().GetState(controlID);
		if (state & TBSTATE_CHECKED)
			state &= ~TBSTATE_CHECKED;
		else
			state |= TBSTATE_CHECKED;
		m_ToolBar.GetToolBarCtrl().SetState(controlID, state);
		return;
	}
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;
	int tbstate = m_ToolBar.GetToolBarCtrl().GetState(controlID);
	UINT state = pMenu->GetMenuState(controlID, MF_BYCOMMAND);
	if (state & MF_CHECKED)
	{
		pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_UNCHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(controlID, tbstate & (~TBSTATE_CHECKED));
		option = false;
	}
	else
	{
		pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_CHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(controlID, tbstate | TBSTATE_CHECKED);
		option = true;
	}

    StartWorkerThread();
}

void CRevisionGraphDlg::StartWorkerThread()
{
	if (InterlockedExchange(&m_Graph.m_bThreadRunning, TRUE) == TRUE)
        return;

	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	    InterlockedExchange(&m_Graph.m_bThreadRunning, FALSE);
	}
}

void CRevisionGraphDlg::OnViewShowallrevisions()
{
    OnToggleOption (ID_VIEW_SHOWALLREVISIONS, m_options.includeSubPathChanges);
}

void CRevisionGraphDlg::OnViewArrangedbypath()
{
    OnToggleOption (ID_VIEW_GROUPBRANCHES, m_options.groupBranches);
}

void CRevisionGraphDlg::OnViewTopDown()
{
    OnToggleOption (ID_VIEW_TOPDOWN, m_options.oldestAtTop);
}

void CRevisionGraphDlg::OnViewShowHEAD()
{
    OnToggleOption (ID_VIEW_SHOWHEAD, m_options.showHEAD);
}

void CRevisionGraphDlg::OnViewExactCopySource()
{
    OnToggleOption (ID_VIEW_EXACTCOPYSOURCE, m_options.exactCopySources);
}

void CRevisionGraphDlg::OnViewFoldTags()
{
    OnToggleOption (ID_VIEW_FOLDTAGS, m_options.foldTags);
}

void CRevisionGraphDlg::OnViewReduceCrosslines()
{
    OnToggleOption (ID_VIEW_REDUCECROSSLINES, m_options.reduceCrossLines);
}

void CRevisionGraphDlg::OnViewRemoveDeletedOnes()
{
    OnToggleOption (ID_VIEW_REMOVEDELETEDONES, m_options.removeDeletedOnes);
}

void CRevisionGraphDlg::OnViewShowWCRev()
{
    OnToggleOption (ID_VIEW_SHOWWCREV, m_options.showWCRev);
}

void CRevisionGraphDlg::OnCancel()
{
	if (!m_Graph.m_bThreadRunning)
		__super::OnCancel();
}

void CRevisionGraphDlg::OnOK()
{
	OnChangeZoom();
}

void CRevisionGraphDlg::OnFileSavegraphas()
{
	CString tempfile;
	int filterindex = 0;
	if (CAppUtils::FileOpenSave(tempfile, &filterindex, IDS_REVGRAPH_SAVEPIC, IDS_PICTUREFILEFILTER, false, m_hWnd))
	{
		// if the user doesn't specify a file extension, default to
		// wmf and add that extension to the filename. But only if the
		// user chose the 'pictures' filter. The filename isn't changed
		// if the 'All files' filter was chosen.
		CString extension;
		int dotPos = tempfile.ReverseFind('.');
		int slashPos = tempfile.ReverseFind('\\');
		if (dotPos > slashPos)
			extension = tempfile.Mid(dotPos);
		if ((filterindex == 1)&&(extension.IsEmpty()))
		{
			extension = _T(".wmf");
			tempfile += extension;
		}
		m_Graph.SaveGraphAs(tempfile);
	}
}

void CRevisionGraphDlg::GetGraphRect(LPRECT rect)
{
	RECT statusbarrect;
	RECT toolbarrect;
	GetClientRect(rect);
	m_StatusBar.GetClientRect(&statusbarrect);
	rect->bottom += statusbarrect.top-statusbarrect.bottom;
	m_ToolBar.GetClientRect(&toolbarrect);
	rect->top -= toolbarrect.top-toolbarrect.bottom;
}

void CRevisionGraphDlg::UpdateStatusBar()
{
	CString sFormat;
	sFormat.Format(IDS_REVGRAPH_STATUSBARURL, m_Graph.m_sPath);
	m_StatusBar.SetText(sFormat,1,0);
	sFormat.Format(IDS_REVGRAPH_STATUSBARNUMNODES, m_Graph.m_entryPtrs.size());
	m_StatusBar.SetText(sFormat,0,0);
}

void CRevisionGraphDlg::OnChangeZoom()
{
	if (!IsWindow(m_Graph.GetSafeHwnd()))
		return;
	CString strText;
	CString strItem;
	CComboBoxEx* pCBox = (CComboBoxEx*)m_ToolBar.GetDlgItem(ID_REVGRAPH_ZOOMCOMBO);
	pCBox->GetWindowText(strItem);
	if (strItem.IsEmpty())
		return;
	m_fZoomFactor = (float)(_tstof(strItem)/100.0);
	UpdateZoomBox();
	ATLTRACE(_T("OnChangeZoom to %s\n"), strItem);
	m_Graph.DoZoom(m_fZoomFactor);
}

void CRevisionGraphDlg::UpdateZoomBox()
{
	CString strText;
	CString strItem;
	CComboBoxEx* pCBox = (CComboBoxEx*)m_ToolBar.GetDlgItem(ID_REVGRAPH_ZOOMCOMBO);
	pCBox->GetWindowText(strItem);
	strText.Format(_T("%.0f%%"), (m_fZoomFactor*100.0));
	if (strText.Compare(strItem) != 0)
		pCBox->SetWindowText(strText);
}

BOOL CRevisionGraphDlg::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;

	UINT nID = pNMHDR->idFrom;

	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool 
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (nID != 0) // will be zero on a separator
	{
		strTipText.LoadString(nID);
	}

	*pResult = 0;
	if (strTipText.IsEmpty())
		return TRUE;

	if (strTipText.GetLength() >= MAX_TT_LENGTH)
		strTipText = strTipText.Left(MAX_TT_LENGTH);

	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);
		pTTTA->lpszText = m_szTip;
		WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
	}
	else
	{
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);
		lstrcpyn(m_wszTip, strTipText, strTipText.GetLength()+1);
		pTTTW->lpszText = m_wszTip;
	}
	// bring the tooltip window above other pop up windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	return TRUE;    // message was handled
}

void CRevisionGraphDlg::OnViewFilter()
{
	CRevGraphFilterDlg dlg;
	dlg.SetMaxRevision(m_Graph.GetHeadRevision());
	dlg.SetFilterString(m_sFilter);
	if (dlg.DoModal()==IDOK)
	{
		// user pressed OK to dismiss the dialog, which means
		// we have to accept the new filter settings and apply them
		svn_revnum_t minrev, maxrev;
		dlg.GetRevisionRange(minrev, maxrev);
		m_sFilter = dlg.GetFilterString();
		m_Graph.SetFilter(minrev, maxrev, m_sFilter);
		InterlockedExchange(&m_Graph.m_bThreadRunning, TRUE);
		if (AfxBeginThread(WorkerThread, this)==NULL)
		{
			CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		}
	}
}

void CRevisionGraphDlg::OnViewShowoverview()
{
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;
	int tbstate = m_ToolBar.GetToolBarCtrl().GetState(ID_VIEW_SHOWOVERVIEW);
	UINT state = pMenu->GetMenuState(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND);
	if (state & MF_CHECKED)
	{
		pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | MF_UNCHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbstate & (~TBSTATE_CHECKED));
		m_Graph.m_bShowOverview = false;
	}
	else
	{
		pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | MF_CHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbstate | TBSTATE_CHECKED);
		m_Graph.m_bShowOverview = true;
	}

	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowRevGraphOverview"), FALSE);
	reg = m_Graph.m_bShowOverview;
	if (m_Graph.m_bShowOverview)
	{
		m_Graph.BuildPreview();
	}
	m_Graph.Invalidate(FALSE);
}







