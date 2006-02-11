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

IMPLEMENT_DYNAMIC(CRevisionGraphDlg, CResizableStandAloneDialog)
CRevisionGraphDlg::CRevisionGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRevisionGraphDlg::IDD, pParent)
	, m_SelectedEntry1(NULL)
	, m_SelectedEntry2(NULL)
	, m_bThreadRunning(FALSE)
	, m_pDlgTip(NULL)
	, m_bNoGraph(false)
	, m_nFontSize(12)
	, m_node_rect_width(NODE_RECT_WIDTH)
	, m_node_space_left(NODE_SPACE_LEFT)
	, m_node_space_right(NODE_SPACE_RIGHT)
	, m_node_space_line(NODE_SPACE_LINE)
	, m_node_rect_heigth(NODE_RECT_HEIGTH)
	, m_node_space_top(NODE_SPACE_TOP)
	, m_node_space_bottom(NODE_SPACE_BOTTOM)
	, m_nIconSize(32)
	, m_RoundRectPt(ROUND_RECT, ROUND_RECT)
	, m_fZoomFactor(1.0)
	, m_hAccel(NULL)
	, m_bFetchLogs(true)
	, m_bShowAll(false)
	, m_bArrangeByPath(false)
	, m_pProgress(NULL)
{
	m_ViewRect.SetRectEmpty();
	memset(&m_lfBaseFont, 0, sizeof(LOGFONT));	
	for (int i=0; i<MAXFONTS; i++)
	{
		m_apFonts[i] = NULL;
	}
}

CRevisionGraphDlg::~CRevisionGraphDlg()
{
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	if (m_pDlgTip)
		delete m_pDlgTip;
}

void CRevisionGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRevisionGraphDlg, CResizableStandAloneDialog)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_COMMAND(ID_FILE_SAVEGRAPHAS, OnFileSavegraphas)
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_VIEW_ZOOMIN, OnViewZoomin)
	ON_COMMAND(ID_VIEW_ZOOMOUT, OnViewZoomout)
	ON_COMMAND(ID_MENUEXIT, OnMenuexit)
	ON_COMMAND(ID_MENUHELP, OnMenuhelp)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_VIEW_COMPAREHEADREVISIONS, OnViewCompareheadrevisions)
	ON_COMMAND(ID_VIEW_COMPAREREVISIONS, OnViewComparerevisions)
	ON_COMMAND(ID_VIEW_UNIFIEDDIFF, OnViewUnifieddiff)
	ON_COMMAND(ID_VIEW_UNIFIEDDIFFOFHEADREVISIONS, OnViewUnifieddiffofheadrevisions)
	ON_COMMAND(ID_VIEW_SHOWALLREVISIONS, &CRevisionGraphDlg::OnViewShowallrevisions)
	ON_COMMAND(ID_VIEW_ARRANGEDBYPATH, &CRevisionGraphDlg::OnViewArrangedbypath)
END_MESSAGE_MAP()


// CRevisionGraphDlg message handlers

BOOL CRevisionGraphDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_pDlgTip = new CToolTipCtrl;
	if(!m_pDlgTip->Create(this))
	{
		TRACE("Unable to add tooltip!\n");
	}
	EnableToolTips();
	memset(&m_lfBaseFont, 0, sizeof(m_lfBaseFont));
	m_lfBaseFont.lfHeight = 0;
	m_lfBaseFont.lfWeight = FW_NORMAL;
	m_lfBaseFont.lfItalic = FALSE;
	m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
	m_lfBaseFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	m_lfBaseFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	m_lfBaseFont.lfQuality = DEFAULT_QUALITY;
	m_lfBaseFont.lfPitchAndFamily = DEFAULT_PITCH;

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_REVISIONGRAPH));
	
	m_dwTicks = GetTickCount();
	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("RevisionGraphDlg"));
	SetSizeGripVisibility(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRevisionGraphDlg::ProgressCallback(CString text, CString text2, DWORD done, DWORD total)
{
	if ((m_pProgress)&&((m_dwTicks+100)<GetTickCount()))
	{
		m_dwTicks = GetTickCount();
		m_pProgress->SetLine(1, text);
		m_pProgress->SetLine(2, text2);
		m_pProgress->SetProgress(done, total);
		if (m_pProgress->HasUserCancelled())
			return FALSE;
	}
	return TRUE;
}

UINT CRevisionGraphDlg::WorkerThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CRevisionGraphDlg*	pDlg;
	pDlg = (CRevisionGraphDlg*)pVoid;
	InterlockedExchange(&pDlg->m_bThreadRunning, TRUE);
	CoInitialize(NULL);
	pDlg->m_pProgress = new CProgressDlg();
	pDlg->m_pProgress->SetTitle(IDS_REVGRAPH_PROGTITLE);
	pDlg->m_pProgress->SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
	pDlg->m_pProgress->SetTime();
	pDlg->m_pProgress->ShowModeless(pDlg->m_hWnd);
	pDlg->m_bNoGraph = FALSE;
	if ((pDlg->m_bFetchLogs)&&(!pDlg->FetchRevisionData(pDlg->m_sPath)))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		pDlg->m_bNoGraph = TRUE;
		goto cleanup;
	}
	pDlg->m_bFetchLogs = false;	// we've got the logs, no need to fetch them a second time
	if (!pDlg->AnalyzeRevisionData(pDlg->m_sPath, pDlg->m_bShowAll, pDlg->m_bArrangeByPath))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		pDlg->m_bNoGraph = TRUE;
	}

cleanup:
	pDlg->m_pProgress->Stop();
	delete pDlg->m_pProgress;
	pDlg->m_pProgress = NULL;
	pDlg->InitView();
	CoUninitialize();
	InterlockedExchange(&pDlg->m_bThreadRunning, FALSE);
	pDlg->Invalidate();
	return 0;
}

void CRevisionGraphDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO sinfo = {0};
	sinfo.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_HORZ, &sinfo);

	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:      // Scroll to far left.
		sinfo.nPos = sinfo.nMin;
		break;
	case SB_RIGHT:      // Scroll to far right.
		sinfo.nPos = sinfo.nMax;
		break;
	case SB_ENDSCROLL:   // End scroll.
		break;
	case SB_LINELEFT:      // Scroll left.
		if (sinfo.nPos > sinfo.nMin)
			sinfo.nPos--;
		break;
	case SB_LINERIGHT:   // Scroll right.
		if (sinfo.nPos < sinfo.nMax)
			sinfo.nPos++;
		break;
	case SB_PAGELEFT:    // Scroll one page left.
		{
			if (sinfo.nPos > sinfo.nMin)
				sinfo.nPos = max(sinfo.nMin, sinfo.nPos - (int) sinfo.nPage);
		}
		break;
	case SB_PAGERIGHT:      // Scroll one page right.
		{
			if (sinfo.nPos < sinfo.nMax)
				sinfo.nPos = min(sinfo.nMax, sinfo.nPos + (int) sinfo.nPage);
		}
		break;
	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		sinfo.nPos = sinfo.nTrackPos;      // of the scroll box at the end of the drag operation.
		break;
	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		sinfo.nPos = sinfo.nTrackPos;     // position that the scroll box has been dragged to.
		break;
	}
	SetScrollInfo(SB_HORZ, &sinfo);
	Invalidate();
	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRevisionGraphDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO sinfo = {0};
	sinfo.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_VERT, &sinfo);

	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:      // Scroll to far left.
		sinfo.nPos = sinfo.nMin;
		break;
	case SB_RIGHT:      // Scroll to far right.
		sinfo.nPos = sinfo.nMax;
		break;
	case SB_ENDSCROLL:   // End scroll.
		break;
	case SB_LINELEFT:      // Scroll left.
		if (sinfo.nPos > sinfo.nMin)
			sinfo.nPos--;
		break;
	case SB_LINERIGHT:   // Scroll right.
		if (sinfo.nPos < sinfo.nMax)
			sinfo.nPos++;
		break;
	case SB_PAGELEFT:    // Scroll one page left.
		{
			if (sinfo.nPos > sinfo.nMin)
				sinfo.nPos = max(sinfo.nMin, sinfo.nPos - (int) sinfo.nPage);
		}
		break;
	case SB_PAGERIGHT:      // Scroll one page right.
		{
			if (sinfo.nPos < sinfo.nMax)
				sinfo.nPos = min(sinfo.nMax, sinfo.nPos + (int) sinfo.nPage);
		}
		break;
	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		sinfo.nPos = sinfo.nTrackPos;      // of the scroll box at the end of the drag operation.
		break;
	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		sinfo.nPos = sinfo.nTrackPos;     // position that the scroll box has been dragged to.
		break;
	}
	SetScrollInfo(SB_VERT, &sinfo);
	Invalidate();
	__super::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CRevisionGraphDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	SetScrollbars(GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ));
	Invalidate(FALSE);
}

void CRevisionGraphDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	ATLTRACE("right clicked on x=%d y=%d\n", point.x, point.y);
	bool bHit = false;
	bool bControl = !!(GetKeyState(VK_CONTROL)&0x8000);
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		if (reventry->drawrect.PtInRect(point))
		{
			if (bControl)
			{
				if (m_SelectedEntry1 == reventry)
				{
					if (m_SelectedEntry2)
					{
						m_SelectedEntry1 = m_SelectedEntry2;
						m_SelectedEntry2 = NULL;
					}
					else
						m_SelectedEntry1 = NULL;
				}
				else if (m_SelectedEntry2 == reventry)
					m_SelectedEntry2 = NULL;
				else if (m_SelectedEntry1)
					m_SelectedEntry2 = reventry;
				else
					m_SelectedEntry1 = reventry;
			}
			else
			{
				if (m_SelectedEntry1 == reventry)
					m_SelectedEntry1 = NULL;
				else
					m_SelectedEntry1 = reventry;
				m_SelectedEntry2 = NULL;
			}
			bHit = true;
			Invalidate();
			break;
		}
	}
	if ((!bHit)&&(!bControl))
	{
		m_SelectedEntry1 = NULL;
		m_SelectedEntry2 = NULL;
		Invalidate();
	}
	
	UINT uEnable = MF_BYCOMMAND;
	if ((m_SelectedEntry1 != NULL)&&(m_SelectedEntry2 != NULL))
		uEnable |= MF_ENABLED;
	else
		uEnable |= MF_GRAYED;

	EnableMenuItem(GetMenu()->m_hMenu, ID_VIEW_COMPAREREVISIONS, uEnable);
	EnableMenuItem(GetMenu()->m_hMenu, ID_VIEW_COMPAREHEADREVISIONS, uEnable);
	EnableMenuItem(GetMenu()->m_hMenu, ID_VIEW_UNIFIEDDIFF, uEnable);
	EnableMenuItem(GetMenu()->m_hMenu, ID_VIEW_UNIFIEDDIFFOFHEADREVISIONS, uEnable);
	
	__super::OnLButtonDown(nFlags, point);
}

INT_PTR CRevisionGraphDlg::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	if (m_bThreadRunning)
		return -1;
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		if (reventry->drawrect.PtInRect(point))
		{
			pTI->hwnd = this->m_hWnd;
			this->GetClientRect(&pTI->rect);
			pTI->uFlags  |= TTF_ALWAYSTIP | TTF_IDISHWND;
			pTI->uId = (UINT)m_hWnd;
			pTI->lpszText = LPSTR_TEXTCALLBACK;
			return 1;
		}
	}
	return -1;
}

BOOL CRevisionGraphDlg::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;

	CRevisionEntry * rentry = NULL;
	POINT point;
	GetCursorPos(&point);
	ScreenToClient(&point);
	if (pNMHDR->idFrom == (UINT)m_hWnd)
	{
		for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
		{
			CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
			if (reventry->drawrect.PtInRect(point))
			{
				rentry = reventry;
			}
		}
		if (rentry)
		{
			TCHAR date[SVN_DATE_BUFFER];
			SVN::formatDate(date, rentry->date);
			strTipText.Format(IDS_REVGRAPH_BOXTOOLTIP,
							(LPCTSTR)CUnicodeUtils::GetUnicode(rentry->url),
							(LPCTSTR)CUnicodeUtils::GetUnicode(rentry->author), 
							date,
							(LPCTSTR)CUnicodeUtils::GetUnicode(rentry->message));
		}
	}
	else
		return FALSE;

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

	return TRUE;    // message was handled
}

BOOL CRevisionGraphDlg::PreTranslateMessage(MSG* pMsg)
{
	if (::IsWindow(m_pDlgTip->m_hWnd) && pMsg->hwnd == m_hWnd)
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
			m_pDlgTip->Activate(TRUE);
			m_pDlgTip->RelayEvent(pMsg);
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
			pos = GetScrollPos(SB_VERT);
			SetScrollPos(SB_VERT, pos - SCROLL_STEP);
			Invalidate();
			break;
		case VK_DOWN:
			pos = GetScrollPos(SB_VERT);
			SetScrollPos(SB_VERT, pos + SCROLL_STEP);
			Invalidate();
			break;
		case VK_LEFT:
			pos = GetScrollPos(SB_HORZ);
			SetScrollPos(SB_HORZ, pos - SCROLL_STEP);
			Invalidate();
			break;
		case VK_RIGHT:
			pos = GetScrollPos(SB_HORZ);
			SetScrollPos(SB_HORZ, pos + SCROLL_STEP);
			Invalidate();
			break;
		case VK_PRIOR:
			pos = GetScrollPos(SB_VERT);
			SetScrollPos(SB_VERT, pos - 10*SCROLL_STEP);
			Invalidate();
			break;
		case VK_NEXT:
			pos = GetScrollPos(SB_VERT);
			SetScrollPos(SB_VERT, pos + 10*SCROLL_STEP);
			Invalidate();
			break;
		}
	}
	if ((m_hAccel)&&(pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST))
	{
		return TranslateAccelerator(m_hWnd,m_hAccel,pMsg);
	}
	return __super::PreTranslateMessage(pMsg);
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
		
		if (extension.CompareNoCase(_T(".wmf"))==0)
		{
			// save the graph as an enhanced metafile
			CMetaFileDC wmfDC;
			wmfDC.CreateEnhanced(NULL, tempfile, NULL, _T("TortoiseSVN\0Revision Graph\0\0"));
			CRect rect;
			rect = GetViewSize();
			DrawGraph(&wmfDC, rect, 0, 0, true);
			HENHMETAFILE hemf = wmfDC.CloseEnhanced();
			DeleteEnhMetaFile(hemf);
		}
		else
		{
			// to save the graph as a pixel picture (e.g. gif, png, jpeg, ...)
			// the user needs to have GDI+ installed. So check if GDI+ is 
			// available before we start using it.
			TCHAR gdifindbuf[MAX_PATH];
			_tcscpy_s(gdifindbuf, MAX_PATH, _T("gdiplus.dll"));
			if (PathFindOnPath(gdifindbuf, NULL))
			{
				ATLTRACE("gdi plus found!");
			}
			else
			{
				ATLTRACE("gdi plus not found!");
				CMessageBox::Show(m_hWnd, IDS_ERR_GDIPLUS_MISSING, IDS_APPNAME, MB_ICONERROR);
				return;
			}
			
			// save the graph as a pixel picture instead of a vector picture
			// create dc to paint on
			try
			{
				CWindowDC ddc(this);
				CDC dc;
				if (!dc.CreateCompatibleDC(&ddc))
				{
					LPVOID lpMsgBuf;
					if (!FormatMessage( 
						FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL ))
					{
						return;
					}
					MessageBox( (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
					LocalFree( lpMsgBuf );
					return;
				}
				CRect rect;
				rect = GetViewSize();
				HBITMAP hbm = ::CreateCompatibleBitmap(ddc.m_hDC, rect.Width(), rect.Height());
				if (hbm==0)
				{
					LPVOID lpMsgBuf;
					if (!FormatMessage( 
						FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL ))
					{
						return;
					}
					MessageBox( (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
					LocalFree( lpMsgBuf );
					return;
				}
				HBITMAP oldbm = (HBITMAP)dc.SelectObject(hbm);
				//paint the whole graph
				DrawGraph(&dc, rect, 0, 0, false);
				//now use GDI+ to save the picture
				CLSID   encoderClsid;
				GdiplusStartupInput gdiplusStartupInput;
				ULONG_PTR           gdiplusToken;
				CString sErrormessage;
				if (GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL )==Ok)
				{   
					{
						Bitmap bitmap(hbm, NULL);
						if (bitmap.GetLastStatus()==Ok)
						{
							// Get the CLSID of the encoder.
							int ret = 0;
							if (CUtils::GetFileExtFromPath(tempfile).CompareNoCase(_T(".png"))==0)
								ret = GetEncoderClsid(L"image/png", &encoderClsid);
							else if (CUtils::GetFileExtFromPath(tempfile).CompareNoCase(_T(".jpg"))==0)
								ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
							else if (CUtils::GetFileExtFromPath(tempfile).CompareNoCase(_T(".jpeg"))==0)
								ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
							else if (CUtils::GetFileExtFromPath(tempfile).CompareNoCase(_T(".bmp"))==0)
								ret = GetEncoderClsid(L"image/bmp", &encoderClsid);
							else if (CUtils::GetFileExtFromPath(tempfile).CompareNoCase(_T(".gif"))==0)
								ret = GetEncoderClsid(L"image/gif", &encoderClsid);
							else
							{
								tempfile += _T(".jpg");
								ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
							}
							if (ret >= 0)
							{
								CStringW tfile = CStringW(tempfile);
								bitmap.Save(tfile, &encoderClsid, NULL);
							}
							else
							{
								sErrormessage.Format(IDS_REVGRAPH_ERR_NOENCODER, CUtils::GetFileExtFromPath(tempfile));
							}
						}
						else
						{
							sErrormessage.LoadString(IDS_REVGRAPH_ERR_NOBITMAP);
						}
					}
					GdiplusShutdown(gdiplusToken);
				}
				else
				{
					sErrormessage.LoadString(IDS_REVGRAPH_ERR_GDIINIT);
				}
				dc.SelectObject(oldbm);
				dc.DeleteDC();
				if (!sErrormessage.IsEmpty())
				{
					CMessageBox::Show(m_hWnd, sErrormessage, _T("TortoiseSVN"), MB_ICONERROR);
				}
			}
			catch (CException * pE)
			{
				TCHAR szErrorMsg[2048];
				pE->GetErrorMessage(szErrorMsg, 2048);
				CMessageBox::Show(m_hWnd, szErrorMsg, _T("TortoiseSVN"), MB_ICONERROR);
			}
		}
	} // if (GetSaveFileName(&ofn)==TRUE)
	delete [] pszFilters;
}

BOOL CRevisionGraphDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	int orientation = GetKeyState(VK_CONTROL)&0x8000 ? SB_HORZ : SB_VERT;
	int pos = GetScrollPos(orientation);
	pos -= (zDelta);
	SetScrollPos(orientation, pos);
	Invalidate();
	return __super::OnMouseWheel(nFlags, zDelta, pt);
}

void CRevisionGraphDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CRevisionEntry * clickedentry = NULL;
	CPoint clientpoint = point;
	this->ScreenToClient(&clientpoint);
	ATLTRACE("right clicked on x=%d y=%d\n", clientpoint.x, clientpoint.y);
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		if (reventry->drawrect.PtInRect(clientpoint))
		{
			clickedentry = reventry;
			break;
		}
	}
	if ((m_SelectedEntry1 == NULL)&&(clickedentry == NULL))
		return;
	if (m_SelectedEntry1 == NULL)
	{
		m_SelectedEntry1 = clickedentry;
		Invalidate();
	}
	if ((m_SelectedEntry2 == NULL)&&(clickedentry != m_SelectedEntry1))
	{
		m_SelectedEntry1 = clickedentry;
		Invalidate();
	}
	if (m_SelectedEntry1 && m_SelectedEntry2)
	{
		if ((m_SelectedEntry2 != clickedentry)&&(m_SelectedEntry1 != clickedentry))
			return;
	}
	if (m_SelectedEntry1 == NULL)
		return;
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		if ((m_SelectedEntry1->action == CRevisionEntry::deleted)||((m_SelectedEntry2)&&(m_SelectedEntry2->action == CRevisionEntry::deleted)))
			return;	// we can't compare with deleted items

		bool bSameURL = (m_SelectedEntry2 && (strcmp(m_SelectedEntry1->url, m_SelectedEntry2->url)==0));
		CString temp;
		if (m_SelectedEntry1 && (m_SelectedEntry2 == NULL))
		{
			temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SHOWLOG, temp);
		}
		if (m_SelectedEntry1 && m_SelectedEntry2)
		{
			temp.LoadString(IDS_REVGRAPH_POPUP_COMPAREREVS);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPAREREVS, temp);
			if (!bSameURL)
			{
				temp.LoadString(IDS_REVGRAPH_POPUP_COMPAREHEADS);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPAREHEADS, temp);
			}

			temp.LoadString(IDS_REVGRAPH_POPUP_UNIDIFFREVS);
			popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UNIDIFFREVS, temp);
			if (!bSameURL)
			{
				temp.LoadString(IDS_REVGRAPH_POPUP_UNIDIFFHEADS);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UNIDIFFHEADS, temp);
			}
		}

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		if (m_SelectedEntry1 == NULL)
			return;
		switch (cmd)
		{
		case ID_COMPAREREVS:
			CompareRevs(false);
			break;
		case ID_COMPAREHEADS:
			CompareRevs(true);
			break;
		case ID_UNIDIFFREVS:
			UnifiedDiffRevs(false);
			break;
		case ID_UNIDIFFHEADS:
			UnifiedDiffRevs(true);
			break;
		case ID_SHOWLOG:
			{
				CString sCmd;
				CString URL = GetReposRoot() + CUnicodeUtils::GetUnicode(m_SelectedEntry1->url);
				sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /revstart:%ld"), 
					CUtils::GetAppDirectory()+_T("TortoiseProc.exe"), 
					(LPCTSTR)URL,
					m_SelectedEntry1->revision);

				if (!SVN::PathIsURL(m_sPath))
				{
					sCmd += _T(" /propspath:\"");
					sCmd += m_sPath;
					sCmd += _T("\"");
				}	

				CUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		}
	}
}

void CRevisionGraphDlg::OnViewZoomin()
{
	if (m_fZoomFactor < 2.0)
	{
		m_fZoomFactor = m_fZoomFactor + (m_fZoomFactor*0.1f);
		DoZoom(m_fZoomFactor);
	}
}

void CRevisionGraphDlg::OnViewZoomout()
{
	if ((m_node_space_left > 1) || (m_node_space_right > 1) || (m_node_space_line > 1) ||
		(m_node_rect_heigth > 1) || (m_node_space_top > 1) || (m_node_space_bottom > 1))
	{
		m_fZoomFactor = m_fZoomFactor - (m_fZoomFactor*0.1f);
		DoZoom(m_fZoomFactor);
	}
}

void CRevisionGraphDlg::OnMenuexit()
{
	if (!m_bThreadRunning)
		EndDialog(IDOK);
}

void CRevisionGraphDlg::OnMenuhelp()
{
	OnHelp();
}

void CRevisionGraphDlg::OnViewCompareheadrevisions()
{
	CompareRevs(true);
}

void CRevisionGraphDlg::OnViewComparerevisions()
{
	CompareRevs(false);
}

void CRevisionGraphDlg::OnViewUnifieddiff()
{
	UnifiedDiffRevs(false);
}

void CRevisionGraphDlg::OnViewUnifieddiffofheadrevisions()
{
	UnifiedDiffRevs(true);
}

void CRevisionGraphDlg::OnViewShowallrevisions()
{
	if (m_bThreadRunning)
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

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CRevisionGraphDlg::OnViewArrangedbypath()
{
	if (m_bThreadRunning)
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

	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CRevisionGraphDlg::OnCancel()
{
	if (!m_bThreadRunning)
		__super::OnCancel();
}












