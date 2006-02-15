// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "MemDC.h"
#include <gdiplus.h>
#include "Revisiongraphdlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "Utils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include ".\revisiongraphdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

// CRevisionGraphDlg dialog

IMPLEMENT_DYNAMIC(CRevisionGraphDlg, CStandAloneDialog)
CRevisionGraphDlg::CRevisionGraphDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CRevisionGraphDlg::IDD, pParent)
	, m_hAccel(NULL)
	, m_bFetchLogs(true)
	, m_bShowAll(false)
	, m_bArrangeByPath(false)
	, m_fZoomFactor(1.0)
{
}

CRevisionGraphDlg::~CRevisionGraphDlg()
{
}

void CRevisionGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRevisionGraphDlg, CStandAloneDialog)
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
	ON_COMMAND(ID_VIEW_ARRANGEDBYPATH, &CRevisionGraphDlg::OnViewArrangedbypath)
	ON_COMMAND(ID_FILE_SAVEGRAPHAS, &CRevisionGraphDlg::OnFileSavegraphas)
	ON_CBN_SELCHANGE(ID_REVGRAPH_ZOOMCOMBO, OnChangeZoom)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
END_MESSAGE_MAP()


// CRevisionGraphDlg message handlers

BOOL CRevisionGraphDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	EnableToolTips();

	// set up the status bar
	m_StatusBar.Create(WS_CHILD|WS_VISIBLE|SBT_OWNERDRAW,
		CRect(0,0,0,0), this, 1);
	int strPartDim[2]= {260, -1};
	m_StatusBar.SetParts(2, strPartDim);

	// set up the toolbar
	//add the tool bar to the dialog
	m_ToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_WRAPABLE| CBRS_SIZE_DYNAMIC);
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

		cImageList.Create(20, cSize.cy, ILC_COLOR24|ILC_MASK, nNbBtn, 0);
		cImageList.Add(&cBitmap, rgbMask);
		m_ToolBar.SendMessage(TB_SETIMAGELIST, 0, (LPARAM)cImageList.m_hImageList);
		cImageList.Detach(); 
		cBitmap.Detach();
	}
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

#define SNAP_WIDTH 60 //the width of the combo box
	//set up the ComboBox control as a snap mode select box
	//First get the index of the placeholder's position in the toolbar
	int index = 0;
	while (m_ToolBar.GetItemID(index) != ID_REVGRAPH_ZOOMCOMBO) index++;

	//next convert that button to a seperator and get its position
	m_ToolBar.SetButtonInfo(index, ID_REVGRAPH_ZOOMCOMBO, TBBS_SEPARATOR,
		SNAP_WIDTH);
	RECT rect;
	m_ToolBar.GetItemRect(index, &rect);

	//expand the rectangle to allow the combo box room to drop down
	rect.top+=3;
	rect.bottom += 200;

	// then .Create the combo box and show it
	if (!m_ToolBar.m_ZoomCombo.CreateEx(WS_EX_RIGHT, WS_CHILD|WS_VISIBLE|CBS_AUTOHSCROLL|CBS_DROPDOWN,
		rect, &m_ToolBar, ID_REVGRAPH_ZOOMCOMBO))
	{
		TRACE0("Failed to create combo-box\n");
		return FALSE;
	}
	m_ToolBar.m_ZoomCombo.ShowWindow(SW_SHOW);

	//fill the combo box
	COMBOBOXEXITEM cbei;
	ZeroMemory(&cbei, sizeof cbei);
	cbei.mask = CBEIF_TEXT;
	cbei.pszText = _T("5%");
	m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	cbei.pszText = _T("10%");
	m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	cbei.pszText = _T("20%");
	m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	cbei.pszText = _T("40%");
	m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	cbei.pszText = _T("50%");
	m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	cbei.pszText = _T("80%");
	m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	cbei.pszText = _T("100%");
	m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	m_ToolBar.m_ZoomCombo.SetCurSel(0);

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_REVISIONGRAPH));

	RECT graphrect;
	GetGraphRect(&graphrect);
	m_Graph.Init(this, &graphrect);
	m_Graph.SetOwner(this);

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
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CRevisionGraphDlg*	pDlg;
	pDlg = (CRevisionGraphDlg*)pVoid;
	InterlockedExchange(&pDlg->m_Graph.m_bThreadRunning, TRUE);
	CoInitialize(NULL);
	pDlg->m_Graph.m_pProgress = new CProgressDlg();
	pDlg->m_Graph.m_pProgress->SetTitle(IDS_REVGRAPH_PROGTITLE);
	pDlg->m_Graph.m_pProgress->SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
	pDlg->m_Graph.m_pProgress->SetTime();
	pDlg->m_Graph.m_pProgress->ShowModeless(pDlg->m_hWnd);
	pDlg->m_Graph.m_bNoGraph = FALSE;
	if ((pDlg->m_bFetchLogs)&&(!pDlg->m_Graph.FetchRevisionData(pDlg->m_Graph.m_sPath)))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->m_Graph.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		pDlg->m_Graph.m_bNoGraph = TRUE;
		goto cleanup;
	}
	pDlg->m_bFetchLogs = false;	// we've got the logs, no need to fetch them a second time
	if (!pDlg->m_Graph.AnalyzeRevisionData(pDlg->m_Graph.m_sPath, pDlg->m_bShowAll, pDlg->m_bArrangeByPath))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->m_Graph.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		pDlg->m_Graph.m_bNoGraph = TRUE;
	}
	pDlg->UpdateStatusBar();
cleanup:
	pDlg->m_Graph.m_pProgress->Stop();
	delete pDlg->m_Graph.m_pProgress;
	pDlg->m_Graph.m_pProgress = NULL;
	pDlg->m_Graph.InitView();
	CoUninitialize();
	InterlockedExchange(&pDlg->m_Graph.m_bThreadRunning, FALSE);
	pDlg->Invalidate();
	return 0;
}

void CRevisionGraphDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if (IsWindow(m_StatusBar))
	{
		CRect rect;
		GetClientRect(&rect);
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
	if (::IsWindow(m_Graph.m_pDlgTip->m_hWnd) && pMsg->hwnd == m_hWnd)
	{
		switch(pMsg->message)
		{
		case WM_LBUTTONDOWN: 
		case WM_MOUSEMOVE:
		case WM_LBUTTONUP: 
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN: 
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			// This will reactivate the tooltip
			m_Graph.m_pDlgTip->Activate(TRUE);
			break;
		}
	}
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
		(m_Graph.m_node_rect_heigth > 1) || (m_Graph.m_node_space_top > 1) || (m_Graph.m_node_space_bottom > 1))
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

void CRevisionGraphDlg::OnViewShowallrevisions()
{
	if (m_Graph.m_bThreadRunning)
		return;
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;
	UINT state = pMenu->GetMenuState(ID_VIEW_SHOWALLREVISIONS, MF_BYCOMMAND);
	if (state & MF_CHECKED)
	{
		pMenu->CheckMenuItem(ID_VIEW_SHOWALLREVISIONS, MF_BYCOMMAND | MF_UNCHECKED);
		m_bShowAll = false;
	}
	else
	{
		pMenu->CheckMenuItem(ID_VIEW_SHOWALLREVISIONS, MF_BYCOMMAND | MF_CHECKED);
		m_bShowAll = true;
	}

	InterlockedExchange(&m_Graph.m_bThreadRunning, TRUE);
	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CRevisionGraphDlg::OnViewArrangedbypath()
{
	if (m_Graph.m_bThreadRunning)
		return;
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;
	UINT state = pMenu->GetMenuState(ID_VIEW_ARRANGEDBYPATH, MF_BYCOMMAND);
	if (state & MF_CHECKED)
	{
		pMenu->CheckMenuItem(ID_VIEW_ARRANGEDBYPATH, MF_BYCOMMAND | MF_UNCHECKED);
		m_bArrangeByPath = false;
	}
	else
	{
		pMenu->CheckMenuItem(ID_VIEW_ARRANGEDBYPATH, MF_BYCOMMAND | MF_CHECKED);
		m_bArrangeByPath = true;
	}

	InterlockedExchange(&m_Graph.m_bThreadRunning, TRUE);
	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
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
	CString temp;
	// ask for the filename to save the picture
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	temp.LoadString(IDS_REVGRAPH_SAVEPIC);
	CUtils::RemoveAccelerators(temp);
	if (temp.IsEmpty())
		ofn.lpstrTitle = NULL;
	else
		ofn.lpstrTitle = temp;
	ofn.Flags = OFN_OVERWRITEPROMPT;

	CString sFilter;
	sFilter.LoadString(IDS_PICTUREFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
	// Replace '|' delimeters with '\0's
	TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
	while (ptr != pszFilters)
	{
		if (*ptr == '|')
			*ptr = '\0';
		ptr--;
	} // while (ptr != pszFilters) 
	ofn.lpstrFilter = pszFilters;
	ofn.nFilterIndex = 1;
	// Display the Open dialog box. 
	CString tempfile;
	if (GetSaveFileName(&ofn)==TRUE)
	{
		tempfile = CString(ofn.lpstrFile);
		// if the user doesn't specify a file extension, default to
		// wmf and add that extension to the filename. But only if the
		// user chose the 'pictures' filter. The filename isn't changed
		// if the 'All files' filter was chosen.
		CString extension;
		int dotPos = tempfile.ReverseFind('.');
		int slashPos = tempfile.ReverseFind('\\');
		if (dotPos > slashPos)
			extension = tempfile.Mid(dotPos);
		if ((ofn.nFilterIndex == 1)&&(extension.IsEmpty()))
		{
			extension = _T(".wmf");
			tempfile += extension;
		}
		m_Graph.SaveGraphAs(tempfile);
	} // if (GetSaveFileName(&ofn)==TRUE)
	delete [] pszFilters;
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
	sFormat.Format(IDS_REVGRAPH_STATUSBARNUMNODES, m_Graph.m_arEntryPtrs.GetCount());
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
	ATLTRACE("OnChangeZoom to %ws\n", strItem);
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
	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	return TRUE;    // message was handled
}







