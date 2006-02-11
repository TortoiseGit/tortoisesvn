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
	, m_nZoomFactor(10)
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

void CRevisionGraphDlg::InitView()
{
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
	m_arVertPositions.RemoveAll();
	m_targetsbottom.clear();
	m_targetsright.clear();
	m_ViewRect.SetRectEmpty();
	GetViewSize();
	BuildConnections();
	SetScrollbars(0,0,m_ViewRect.Width(),m_ViewRect.Height());
}

void CRevisionGraphDlg::SetScrollbars(int nVert, int nHorz, int oldwidth, int oldheight)
{
	CRect clientrect;
	GetClientRect(&clientrect);
	CRect * pRect = GetViewSize();
	SCROLLINFO ScrollInfo;
	ScrollInfo.cbSize = sizeof(SCROLLINFO);
	ScrollInfo.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &ScrollInfo);
	if ((nVert)||(oldheight==0))
		ScrollInfo.nPos = nVert;
	else
		ScrollInfo.nPos = ScrollInfo.nPos * pRect->Height() / oldheight;
	ScrollInfo.fMask = SIF_ALL;
	ScrollInfo.nMin = 0;
	ScrollInfo.nMax = pRect->bottom;
	ScrollInfo.nPage = clientrect.Height();
	ScrollInfo.nTrackPos = 0;
	SetScrollInfo(SB_VERT, &ScrollInfo);
	GetScrollInfo(SB_HORZ, &ScrollInfo);
	if ((nHorz)||(oldwidth==0))
		ScrollInfo.nPos = nHorz;
	else
		ScrollInfo.nPos = ScrollInfo.nPos * pRect->Width() / oldwidth;
	ScrollInfo.nMax = pRect->right;
	ScrollInfo.nPage = clientrect.Width();
	SetScrollInfo(SB_HORZ, &ScrollInfo);
}

INT_PTR CRevisionGraphDlg::GetIndexOfRevision(LONG rev) const
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		if (((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->revision == rev)
			return i;
	}
	return -1;
}

INT_PTR CRevisionGraphDlg::GetIndexOfRevision(source_entry * sentry)
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		if (((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->revision == sentry->revisionto)
			if (IsParentOrItself(sentry->pathto, ((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->url))
				return i;
	}
	ATLTRACE("no entry for %s - revision %ld\n", sentry->pathto, sentry->revisionto);
	return -1;
}

/************************************************************************/
/* Graphing functions                                                   */
/************************************************************************/
CFont* CRevisionGraphDlg::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/)
{
	int nIndex = 0;
	if (bBold)
		nIndex |= 1;
	if (bItalic)
		nIndex |= 2;
	if (m_apFonts[nIndex] == NULL)
	{
		m_apFonts[nIndex] = new CFont;
		m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		m_lfBaseFont.lfItalic = (BYTE) bItalic;
		m_lfBaseFont.lfStrikeOut = (BYTE) FALSE;
		CDC * pDC = GetDC();
		m_lfBaseFont.lfHeight = -MulDiv(m_nFontSize, GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72);
		ReleaseDC(pDC);
		// use the empty font name, so GDI takes the first font which matches
		// the specs. Maybe this will help render chinese/japanese chars correctly.
		_tcsncpy_s(m_lfBaseFont.lfFaceName, 32, _T("MS Shell Dlg 2"), 32);
		if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
		{
			delete m_apFonts[nIndex];
			m_apFonts[nIndex] = NULL;
			return CDialog::GetFont();
		}
	}
	return m_apFonts[nIndex];
}

BOOL CRevisionGraphDlg::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CRevisionGraphDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(&rect);

	if (m_bThreadRunning)
	{
		dc.FillSolidRect(rect, ::GetSysColor(COLOR_APPWORKSPACE));
		CResizableStandAloneDialog::OnPaint();
		return;
	}
	else if ((m_bNoGraph)||(m_arEntryPtrs.GetCount()==0))
	{
		CString sNoGraphText;
		sNoGraphText.LoadString(IDS_REVGRAPH_ERR_NOGRAPH);
		dc.FillSolidRect(rect, RGB(255,255,255));
		dc.ExtTextOut(20,20,ETO_CLIPPED,NULL,sNoGraphText,NULL);
		return;
	}
	GetViewSize();
	DrawGraph(&dc, rect, GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ), false);
}

void CRevisionGraphDlg::DrawOctangle(CDC * pDC, const CRect& rect)
{
	int cutLen = rect.Height() / 4;
	CPoint point1(rect.left, rect.top + cutLen);
	CPoint point2(rect.left + cutLen, rect.top);
	CPoint point3(rect.right - cutLen, rect.top);
	CPoint point4(rect.right, rect.top + cutLen);
	CPoint point5(rect.right, rect.bottom - cutLen);
	CPoint point6(rect.right - cutLen, rect.bottom);
	CPoint point7(rect.left + cutLen, rect.bottom);
	CPoint point8(rect.left, rect.bottom - cutLen);
	CPoint arrPoints[8] = {
							point1,
							point2,
							point3,
							point4,
							point5,
							point6,
							point7,
							point8};

		pDC->Polygon(arrPoints, 8);
}

void CRevisionGraphDlg::DrawNode(CDC * pDC, const CRect& rect,
								COLORREF contour, CRevisionEntry *rentry, NodeShape shape, 
								BOOL isSel, HICON hIcon, int penStyle /*= PS_SOLID*/)
{
#define minmax(x, y, z) (x > 0 ? min(y, z) : max(y, z))
	CPen* pOldPen = 0L;
	CBrush* pOldBrush = 0L;
	CFont* pOldFont = 0L;
	CPen pen, pen2;
	CBrush brush;
	COLORREF background = GetSysColor(COLOR_WINDOW);
	COLORREF textcolor = GetSysColor(COLOR_WINDOWTEXT);

	COLORREF selcolor = contour;
	int rval = (GetRValue(background)-GetRValue(contour))/2;
	int gval = (GetGValue(background)-GetGValue(contour))/2;
	int bval = (GetBValue(background)-GetBValue(contour))/2;
	selcolor = RGB(minmax(rval, GetRValue(contour)+rval, GetRValue(background)),
		minmax(gval, GetGValue(contour)+gval, GetGValue(background)),
		minmax(bval, GetBValue(contour)+bval, GetBValue(background)));

	COLORREF shadowc = RGB(abs(GetRValue(background)-GetRValue(textcolor))/2,
							abs(GetGValue(background)-GetGValue(textcolor))/2,
							abs(GetBValue(background)-GetBValue(textcolor))/2);

	TRY
	{
		// Prepare the shadow
		CRect shadow = rect;
		shadow.OffsetRect(SHADOW_OFFSET_PT);

		brush.CreateSolidBrush(shadowc);
		pOldBrush = pDC->SelectObject(&brush);
		pen.CreatePen(penStyle, 1, shadowc);
		pOldPen = pDC->SelectObject(&pen);

		// Draw the shadow
		switch( shape )
		{
		case TSVNRectangle:
			pDC->Rectangle(shadow);
			break;
		case TSVNRoundRect:
			pDC->RoundRect(shadow, m_RoundRectPt);
			break;
		case TSVNOctangle:
			DrawOctangle(pDC, shadow);
			break;
		case TSVNEllipse:
			pDC->Ellipse(shadow);
			break;
		default:
			ASSERT(FALSE);	//unknown type
			return;
		}

		// Prepare selection
		if( isSel )
		{
			brush.DeleteObject();
			brush.CreateSolidBrush(selcolor);
			pDC->SelectObject(&brush);
		}
		else
		{
			pDC->SelectObject(pOldBrush);
			pOldBrush = 0L;
		}

		pen2.CreatePen(penStyle, 1, contour);
		pDC->SelectObject(&pen2);

		// Draw the main shape
		switch( shape )
		{
		case TSVNRectangle:
			pDC->Rectangle(rect);
			break;
		case TSVNRoundRect:
			pDC->RoundRect(rect, m_RoundRectPt);
			break;
		case TSVNOctangle:
			DrawOctangle(pDC, rect);
			break;
		case TSVNEllipse:
			pDC->Ellipse(rect);
			break;
		default:
			ASSERT(FALSE);	//unknown type
			return;
		}
		
		COLORREF brightcol = contour;
		int rval = (GetRValue(background)-GetRValue(contour))*9/10;
		int gval = (GetGValue(background)-GetGValue(contour))*9/10;
		int bval = (GetBValue(background)-GetBValue(contour))*9/10;
		brightcol = RGB(minmax(rval, GetRValue(contour)+rval, GetRValue(background)),
						minmax(gval, GetGValue(contour)+gval, GetGValue(background)),
						minmax(bval, GetBValue(contour)+bval, GetBValue(background)));

		brush.DeleteObject();
		if (isSel)
			brush.CreateSolidBrush(selcolor);
		else
			brush.CreateSolidBrush(brightcol);
		pOldBrush = pDC->SelectObject(&brush);

		// Draw the main shape
		switch( shape )
		{
		case TSVNRectangle:
			pDC->Rectangle(rect);
			break;
		case TSVNRoundRect:
			pDC->RoundRect(rect, m_RoundRectPt);
			break;
		case TSVNOctangle:
			DrawOctangle(pDC, rect);
			break;
		case TSVNEllipse:
			pDC->Ellipse(rect);
			break;
		default:
			ASSERT(FALSE);	//unknown type
			return;
		}

		pDC->SetTextColor(textcolor);
		// draw the revision text
		pOldFont = pDC->SelectObject(GetFont(FALSE, TRUE));
		CString temp;
		CRect r;
		CRect textrect = rect;
		textrect.left += 10;
		textrect.right -= 10;
		TEXTMETRIC textMetric;
		pDC->GetOutputTextMetrics(&textMetric);
		temp.Format(IDS_REVGRAPH_BOXREVISIONTITLE, rentry->revision);
		pDC->DrawText(temp, &r, DT_CALCRECT);
		pDC->ExtTextOut(textrect.left + ((rect.Width()-r.Width())/2), textrect.top + m_node_rect_heigth/4, ETO_CLIPPED, NULL, temp, NULL);

		// draw the url
		pDC->SelectObject(GetFont(TRUE));
		temp = CUnicodeUtils::GetUnicode(rentry->url);
		if (temp.IsEmpty())
			temp = CUnicodeUtils::GetUnicode(rentry->realurl);
		r = textrect;
		temp.Replace('/','\\');
		pDC->DrawText(temp.GetBuffer(temp.GetLength()), temp.GetLength(), &r, DT_CALCRECT | DT_PATH_ELLIPSIS | DT_MODIFYSTRING);
		temp.ReleaseBuffer();
		temp.Replace('\\','/');
		pDC->ExtTextOut(textrect.left + 2 + ((textrect.Width()-4-r.Width())/2), textrect.top + m_node_rect_heigth/4 + m_node_rect_heigth/3, ETO_CLIPPED, &textrect, temp, NULL);

		// draw the icon
		CPoint iconpoint = CPoint(rect.left + m_nIconSize/6, rect.top + m_nIconSize/6);
		CSize iconsize = CSize(m_nIconSize, m_nIconSize);
		pDC->DrawState(iconpoint, iconsize, hIcon, DST_ICON, (HBRUSH)NULL);
		// Cleanup
		if (pOldFont != 0L)
		{
			pDC->SelectObject(pOldFont);
			pOldFont = 0L;
		}
		if (pOldPen != 0L)
		{
			pDC->SelectObject(pOldPen);
			pOldPen = 0L;
		}

		if (pOldBrush != 0L)
		{
			pDC->SelectObject(pOldBrush);
			pOldBrush = 0L;
		}

		pen.DeleteObject();
		pen2.DeleteObject();
		brush.DeleteObject();
	}
	CATCH_ALL(e)
	{
		if( pOldPen != 0L )
			pDC->SelectObject(pOldPen);

		if( pOldBrush != 0L )
			pDC->SelectObject(pOldBrush);
	}
	END_CATCH_ALL
}

void CRevisionGraphDlg::DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw)
{
	CDC * memDC;
	if (bDirectDraw)
	{
		memDC = pDC;
	}
	else
	{
		memDC = new CMemDC(pDC);
	}
	
	memDC->FillSolidRect(rect, GetSysColor(COLOR_WINDOW));
	memDC->SetBkMode(TRANSPARENT);

	for (int rectcounter = 0; rectcounter < m_arEntryPtrs.GetCount(); ++rectcounter)
	{
		((CRevisionEntry*)m_arEntryPtrs[rectcounter])->drawrect = CRect(0,0,0,0);
	}

	// find out which nodes are in the visible area of the client rect
	INT_PTR i = 0;
	INT_PTR end = 0;
	int vert = 0;
	while ((vert)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) <= nVScrollPos)
		vert++;
	if (vert>0)
		vert--;
	// vert is now the top vertical postion of the first nodes to draw
	while ((i<m_arEntryPtrs.GetCount())&&((int)m_arVertPositions[i] < vert))
		++i;
	end = i;
	while ((vert)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) <= (rect.bottom + nVScrollPos))
		vert++;
	while ((end<m_arEntryPtrs.GetCount())&&((int)m_arVertPositions[end] < vert))
		++end;

	if (i >= m_arEntryPtrs.GetCount())
		i = m_arEntryPtrs.GetCount()-1;
	if (end > m_arEntryPtrs.GetCount())
		end = m_arEntryPtrs.GetCount();

	INT_PTR start = i;

	HICON hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_DELETED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hAddedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_ADDED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hAddedWithHistoryIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_ADDEDPLUS), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_REPLACED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hRenamedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_RENAMED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hLastCommitIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_LASTCOMMIT), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);

	for ( ; ((i>=0)&&(i<end)); ++i)
	{
		CRevisionEntry * entry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		int vertpos = m_arVertPositions[i];
		CRect noderect;
		noderect.top = vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top - nVScrollPos;
		noderect.bottom = noderect.top + m_node_rect_heigth;
		noderect.left = (entry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left - nHScrollPos;
		noderect.right = noderect.left + m_node_rect_width;
		switch (entry->action)
		{
		case CRevisionEntry::deleted:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::DeletedNode), entry, TSVNOctangle, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hDeletedIcon);
			break;
		case CRevisionEntry::added:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::AddedNode), entry, TSVNRoundRect, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hAddedIcon);
			break;
		case CRevisionEntry::addedwithhistory:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::AddedNode), entry, TSVNRoundRect, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hAddedWithHistoryIcon);
			break;
		case CRevisionEntry::replaced:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::ReplacedNode), entry, TSVNOctangle, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hReplacedIcon);
			break;
		case CRevisionEntry::renamed:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::RenamedNode), entry, TSVNOctangle, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hRenamedIcon);
			break;
		case CRevisionEntry::lastcommit:
		case CRevisionEntry::initial:
			DrawNode(memDC, noderect, m_Colors.GetColor(CColors::LastCommitNode), entry, TSVNEllipse, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), hLastCommitIcon);
			break;
		default:
			DrawNode(memDC, noderect, GetSysColor(COLOR_WINDOWTEXT), entry, TSVNRectangle, ((m_SelectedEntry1==entry)||(m_SelectedEntry2==entry)), NULL);
			break;
		}
		entry->drawrect = noderect;
	}
	DestroyIcon(hDeletedIcon);
	DestroyIcon(hAddedIcon);
	DestroyIcon(hAddedWithHistoryIcon);
	DestroyIcon(hReplacedIcon);
	DestroyIcon(hRenamedIcon);
	DestroyIcon(hLastCommitIcon);

	DrawConnections(memDC, rect, nVScrollPos, nHScrollPos, start, end);
	if (!bDirectDraw)
		delete memDC;
}

void CRevisionGraphDlg::MarkSpaceLines(source_entry * entry, int level, svn_revnum_t startrev, svn_revnum_t endrev)
{
	int maxright = 0;
	int maxbottom = 0;
	std::set<CRevisionEntry*> rightset;
	std::set<CRevisionEntry*> bottomset;
	
	for (INT_PTR i=0; i < m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		bool incremented = false;
		if (reventry->level == level)
		{
			if ((reventry->revision >= startrev)&&(reventry->revision <= endrev))
			{
				reventry->rightlines++;
				incremented = true;
				maxright = max(reventry->rightlines, maxright);
				rightset.insert(reventry);
			}
			else if (reventry->revision < startrev)
			{
				for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
				{
					source_entry * sentry = (source_entry*)reventry->sourcearray[j];
					for (std::set<CRevisionEntry*>::iterator it = rightset.begin(); it!= rightset.end(); ++it)
					{
						if ((sentry->revisionto >= (*it)->revision)&&(!incremented))
						{
							// before we add this, we have to check if it's a top->bottom line and not one
							// which goes to the right around another node
							bool nodesinbetween = true;
							bool bStart = false;
							if ((*it)->level == level)
							{
								nodesinbetween = false;
								for (INT_PTR k=0; k<m_arEntryPtrs.GetCount(); ++k)
								{
									CRevisionEntry * tempentry = (CRevisionEntry*)m_arEntryPtrs[k];
									if (!bStart)
									{
										if (tempentry->revision == sentry->revisionto)
											bStart = true;
									}
									else
									{
										if (tempentry->revision <= reventry->revision)
											break;
										if ((tempentry->revision > reventry->revision)&&(tempentry->level == reventry->level))
										{
											nodesinbetween = true;
											break;
										}
									}
								}
							}
							if (nodesinbetween)
							{
								reventry->rightlines++;
								incremented = true;
								maxright = max(reventry->rightlines, maxright);
								rightset.insert(reventry);
							}
						}
					}
				}
			}
		}
		if (reventry->revision == endrev)
		{
			if (reventry->level > level)
			{
				reventry->bottomlines++;
				maxbottom = max(reventry->bottomlines, maxbottom);
				bottomset.insert(reventry);
			}
		}
	}
	
	for (std::set<CRevisionEntry*>::iterator it=rightset.begin(); it!=rightset.end(); ++it)
	{
		m_targetsright.insert(std::pair<source_entry*, CRevisionEntry*>(entry, (*it)));
	}
	for (std::set<CRevisionEntry*>::iterator it=bottomset.begin(); it!=bottomset.end(); ++it)
	{
		m_targetsbottom.insert(std::pair<source_entry*, CRevisionEntry*>(entry, (*it)));
	}

	std::multimap<source_entry*, CRevisionEntry*>::iterator beginright = m_targetsright.lower_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator endright = m_targetsright.upper_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator beginbottom = m_targetsbottom.lower_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator endbottom = m_targetsbottom.upper_bound(entry);

	for (std::multimap<source_entry*, CRevisionEntry*>::iterator it = beginright; it!=endright; ++it)
	{
		(it->second)->rightlines = maxright;
	}

	for (std::multimap<source_entry*, CRevisionEntry*>::iterator it = beginbottom; it!=endbottom; ++it)
	{
		(it->second)->bottomlines = maxbottom;
	}
}

void CRevisionGraphDlg::DecrementSpaceLines(source_entry * entry)
{
	
	std::multimap<source_entry*, CRevisionEntry*>::iterator beginright = m_targetsright.lower_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator endright = m_targetsright.upper_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator beginbottom = m_targetsbottom.lower_bound(entry);
	std::multimap<source_entry*, CRevisionEntry*>::iterator endbottom = m_targetsbottom.upper_bound(entry);
	
	for (std::multimap<source_entry*, CRevisionEntry*>::iterator it = beginright; it!=endright; ++it)
	{
		(it->second)->rightlinesleft--;
	}
	
	for (std::multimap<source_entry*, CRevisionEntry*>::iterator it = beginbottom; it!=endbottom; ++it)
	{
		(it->second)->bottomlinesleft--;
	}
}

void CRevisionGraphDlg::ClearEntryConnections()
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		reventry->bottomconnections = 0;
		reventry->leftconnections = 0;
		reventry->rightconnections = 0;
		reventry->bottomlines = 0;
		reventry->rightlines = 0;
	}
	m_targetsright.clear();
	m_targetsbottom.clear();
}

void CRevisionGraphDlg::CountEntryConnections()
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		reventry->leftconnections += reventry->sourcearray.GetCount();
		for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
		{
			source_entry * sentry = (source_entry*)reventry->sourcearray[j];
			INT_PTR index = GetIndexOfRevision(sentry);
			if (index < 0)
				continue;
			CRevisionEntry * reventryto = (CRevisionEntry*)m_arEntryPtrs[index];
			if (reventry->level == reventryto->level)
			{
				// if there are entries in between, then the connection
				// is right-up-left
				// without entries in between, the connection is straight up
				bool nodesinbetween = false;
				bool bStart = false;
				for (INT_PTR k=0; k<m_arEntryPtrs.GetCount(); ++k)
				{
					CRevisionEntry * tempentry = (CRevisionEntry*)m_arEntryPtrs[k];
					if (!bStart)
					{
						if (tempentry->revision == reventryto->revision)
							bStart = true;
					}
					else
					{
						if (tempentry->revision <= reventry->revision)
							break;
						if ((tempentry->revision > reventry->revision)&&(tempentry->level == reventry->level))
						{
							nodesinbetween = true;
							break;
						}
					}
				}
				if (nodesinbetween)
				{
					reventryto->rightconnections++;
					reventry->rightconnections++;
					MarkSpaceLines(sentry, reventry->level, reventry->revision, reventryto->revision);
				}
				else
				{
					reventryto->bottomconnections++;
				}
			}
			else
			{
				if (reventry->level < reventryto->level)
				{
					reventry->rightconnections++;
					reventryto->bottomconnections++;
				}
				else
				{
					reventry->leftconnections++;
					reventryto->bottomconnections++;
				}
				MarkSpaceLines(sentry, reventry->level, reventry->revision, reventryto->revision);
			}
		}
	}
}

void CRevisionGraphDlg::BuildConnections()
{
	// create an array which holds the vertical position of each
	// revision entry. Since there can be several entries in the
	// same revision, this speeds up the search for the right
	// position for drawing.
	m_arVertPositions.RemoveAll();
	svn_revnum_t vprev = 0;
	int currentvpos = 0;
	for (INT_PTR vp = 0; vp < m_arEntryPtrs.GetCount(); ++vp)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(vp);
		if (reventry->revision != vprev)
		{
			vprev = reventry->revision;
			currentvpos++;
		}
		m_arVertPositions.Add(currentvpos-1);
	}
	// delete all entries which we might have left
	// in the array and free the memory they use.
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
	
	ClearEntryConnections();
	CountEntryConnections();
	
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		reventry->bottomconnectionsleft = reventry->bottomconnections;
		reventry->leftconnectionsleft = reventry->leftconnections;
		reventry->rightconnectionsleft = reventry->rightconnections;
		reventry->bottomlinesleft = reventry->bottomlines;
		reventry->rightlinesleft = reventry->rightlines;
	}

	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		int vertpos = m_arVertPositions[i];
		for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
		{
			source_entry * sentry = (source_entry*)reventry->sourcearray.GetAt(j);
			INT_PTR index = GetIndexOfRevision(sentry);
			if (index < 0)
				continue;
			CRevisionEntry * reventry2 = ((CRevisionEntry*)m_arEntryPtrs.GetAt(index));
			
			// we always draw from bottom to top!			
			CPoint * pt = new CPoint[5];
			if (reventry->level < reventry2->level)
			{
				if (reventry->revision < reventry2->revision)
				{
					//       5
					//       |
					//    3--4
					//    |
					// 1--2
					
					// x-offset for line 2-3
					int xoffset = (reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/(reventry->rightlines+1);
					// y-offset for line 3-4
					int yoffset = (reventry2->bottomlinesleft)*(m_node_space_top+m_node_space_bottom)/(reventry2->bottomlines+1);
					
					//Starting point: 1
					pt[0].y = vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top;	// top of rect
					pt[0].y += (reventry->rightconnectionsleft)*(m_node_rect_heigth)/(reventry->rightconnections+1);
					reventry->rightconnectionsleft--;
					pt[0].x = (reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width;
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + xoffset;
					//line up: 3
					pt[2].x = pt[1].x;
					pt[2].y = ((m_arVertPositions[GetIndexOfRevision(sentry)])*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom)) + m_node_rect_heigth + m_node_space_top;
					pt[2].y += yoffset;
					//line to middle of target rect: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry)))->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += m_node_space_left + m_node_rect_width/2;
					//line up to target rect: 5
					pt[4].y = (m_arVertPositions[GetIndexOfRevision(sentry)]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth) + m_node_space_top;
					pt[4].x = pt[3].x;
				}
				else
				{
					// since we should *never* draw a connection from a higher to a lower
					// revision, assert!
					ATLASSERT(false);
				}
			}
			else if (reventry->level > reventry2->level)
			{
				if (reventry->revision < reventry2->revision)
				{
					// 5
					// |
					// 4----3
					//      |
					//      |
					//      2-----1

					// x-offset for line 2-3
					int xoffset = (reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/(reventry->rightlines+1);
					// y-offset for line 3-4
					int yoffset = (reventry2->bottomlinesleft)*(m_node_space_top+m_node_space_bottom)/(reventry2->bottomlines+1);

					//Starting point: 1
					pt[0].y = (vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += (reventry->leftconnectionsleft)*(m_node_rect_heigth)/(reventry->leftconnections+1);
					reventry->leftconnectionsleft--;
					pt[0].x = ((reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x - xoffset;
					//line up: 3
					pt[2].x = pt[1].x;
					pt[2].y = (m_arVertPositions[GetIndexOfRevision(sentry)]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth + m_node_space_top + m_node_space_bottom);
					pt[2].y += yoffset;
					//line to middle of target rect: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry)))->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += m_node_space_left + m_node_rect_width/2;
					//line up to target rect: 5
					pt[4].x = pt[3].x;
					pt[4].y = (m_arVertPositions[GetIndexOfRevision(sentry)]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_rect_heigth + m_node_space_top);
				}
				else
				{
					// since we should *never* draw a connection from a higher to a lower
					// revision, assert!
					ATLASSERT(false);
				}
			}
			else
			{
				// same level!
				// check first if there are other nodes in between the two connected ones
				BOOL nodesinbetween = FALSE;
				LONG startrev = min(reventry->revision, reventry2->revision);
				LONG endrev = max(reventry->revision, reventry2->revision);
				for (LONG k=startrev+1; k<endrev; ++k)
				{
					INT_PTR ind = GetIndexOfRevision(k);
					if (ind>=0)
					{
						if (((CRevisionEntry*)m_arEntryPtrs.GetAt(ind))->level == reventry2->level)
						{
							nodesinbetween = TRUE;
							break;
						}
					}
				}
				if (nodesinbetween)
				{
					// 4----3
					//      |
					//      |
					//      |
					// 1----2

					// x-offset for line 2-3
					int xoffset = (reventry->rightlinesleft)*(m_node_space_left+m_node_space_right)/(reventry->rightlines+1);
					// y-offset for line 3-4
					int yoffset = (reventry2->rightconnectionsleft)*(m_node_rect_heigth)/(reventry2->rightconnections+1);
					reventry2->rightconnectionsleft--;
										
					//Starting point: 1
					pt[0].y = (vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += (reventry->rightconnectionsleft)*(m_node_rect_heigth)/(reventry->rightconnections+1);
					reventry->rightconnectionsleft--;
					pt[0].x = ((reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left) + m_node_rect_width;
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + xoffset;
					//line down: 3
					pt[2].x = pt[1].x;
					pt[2].y = (m_arVertPositions[GetIndexOfRevision(sentry)]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[2].y += yoffset;
					//line to target: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left) + m_node_rect_width;
					pt[4].y = pt[3].y;
					pt[4].x = pt[3].x;
				}
				else
				{
					if (reventry->revision < reventry2->revision)
					{
						// 2
						// |
						// |
						// 1
						pt[0].y = (vertpos*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
						pt[0].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width/2);
						pt[1].y = pt[0].y;
						pt[1].x = pt[0].x;
						pt[2].y = (m_arVertPositions[GetIndexOfRevision(sentry)]*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top + m_node_rect_heigth);
						pt[2].x = pt[0].x;
						pt[3].y = pt[2].y;
						pt[3].x = pt[2].x;
						pt[4].y = pt[3].y;
						pt[4].x = pt[3].x;
					}
					else
					{
						ATLASSERT(false);
					}
				}
			}
			INT_PTR conindex = m_arConnections.Add(pt);

			// we add the connection index to each revision entry which the connection
			// passes by vertically. We use this to reduce the time to draw the connections,
			// because we know which nodes are in the visible area but not which connections.
			// By doing this, we can simply get the connections we have to draw from
			// the nodes we draw.
			for (EntryPtrsIterator it = m_mapEntryPtrs.lower_bound(reventry->revision); it != m_mapEntryPtrs.upper_bound(reventry2->revision); ++it)
			{
				it->second->connections.insert(conindex);
			}
			DecrementSpaceLines(sentry);
		}
	}
}

void CRevisionGraphDlg::DrawConnections(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos, INT_PTR start, INT_PTR end)
{
	CRect viewrect;
	viewrect.top = rect.top + nVScrollPos;
	viewrect.bottom = rect.bottom + nVScrollPos;
	viewrect.left = rect.left + nHScrollPos;
	viewrect.right = rect.right + nHScrollPos;


	std::set<INT_PTR> connections;
	for ( ; ((start>=0)&&(start<end)); ++start)
	{
		CRevisionEntry * entry = (CRevisionEntry*)m_arEntryPtrs.GetAt(start);
		for (std::set<INT_PTR>::iterator it = entry->connections.begin(); it != entry->connections.end(); ++it)
		{
			connections.insert(*it);
		}
	}

	CPen newpen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
	CPen * pOldPen = pDC->SelectObject(&newpen);

	POINT p[5];

	for (std::set<INT_PTR>::iterator it = connections.begin(); it != connections.end(); ++it)
	{
		CPoint * pt = (CPoint *)m_arConnections.GetAt(*it);
		// correct the scroll offset
		p[0].x = pt[0].x - nHScrollPos;
		p[1].x = pt[1].x - nHScrollPos;
		p[2].x = pt[2].x - nHScrollPos;
		p[3].x = pt[3].x - nHScrollPos;
		p[4].x = pt[4].x - nHScrollPos;
		p[0].y = pt[0].y - nVScrollPos;
		p[1].y = pt[1].y - nVScrollPos;
		p[2].y = pt[2].y - nVScrollPos;
		p[3].y = pt[3].y - nVScrollPos;
		p[4].y = pt[4].y - nVScrollPos;

		pDC->Polyline(p, 5);
	}

	pDC->SelectObject(pOldPen);
}

CRect * CRevisionGraphDlg::GetViewSize()
{
	if (m_ViewRect.Height() != 0)
		return &m_ViewRect;
	m_ViewRect.top = 0;
	m_ViewRect.left = 0;
	int level = 0;
	int revisions = 0;
	int lastrev = -1;
	size_t maxurllength = 0;
	CString url;
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs[i];
		if (level < reventry->level)
			level = reventry->level;
		if (lastrev != reventry->revision)
		{
			revisions++;
			lastrev = reventry->revision;
		}
		size_t len = strlen(reventry->url);
		if (maxurllength < len)
		{
			maxurllength = len;
			url = CUnicodeUtils::GetUnicode(reventry->url);
		}
	}

	// calculate the width of the nodes by looking
	// at the url lengths
	CRect r;
	CDC * pDC = this->GetDC();
	if (pDC)
	{
		CFont * pOldFont = pDC->SelectObject(GetFont(TRUE));
		pDC->DrawText(url, &r, DT_CALCRECT);
		// keep the width inside reasonable values.
		m_node_rect_width = min(500 * m_nZoomFactor / 10, r.Width()+40);
		m_node_rect_width = max(NODE_RECT_WIDTH * m_nZoomFactor / 10, m_node_rect_width);
		pDC->SelectObject(pOldFont);
	}
	ReleaseDC(pDC);

	m_ViewRect.right = level * (m_node_rect_width + m_node_space_left + m_node_space_right);
	m_ViewRect.bottom = revisions * (m_node_rect_heigth + m_node_space_top + m_node_space_bottom);
	CRect rect;
	GetClientRect(&rect);
	if (m_ViewRect.Width() < rect.Width())
	{
		m_ViewRect.left = rect.left;
		m_ViewRect.right = rect.right;
	}
	if (m_ViewRect.Height() < rect.Height())
	{
		m_ViewRect.top = rect.top;
		m_ViewRect.bottom = rect.bottom;
	}
	return &m_ViewRect;
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

int CRevisionGraphDlg::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	if (GetImageEncodersSize(&num, &size)!=Ok)
		return -1;
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	if (GetImageEncoders(num, size, pImageCodecInfo)==Ok)
	{
		for(UINT j = 0; j < num; ++j)
		{
			if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j;  // Success
			}
		}

	}
	free(pImageCodecInfo);
	return -1;  // Failure
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

void CRevisionGraphDlg::CompareRevs(bool bHead)
{
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);

	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry1->url));
	url2.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry2->url));

	SVNRev peg = (SVNRev)(bHead ? m_SelectedEntry1->revision : SVNRev());

	SVNDiff diff(&svn, this->m_hWnd);
	diff.ShowCompare(url1, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry1->revision),
		url2, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry2->revision),
		peg);
}

void CRevisionGraphDlg::UnifiedDiffRevs(bool bHead)
{
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);

	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry1->url));
	url2.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry2->url));

	SVNDiff diff(&svn, this->m_hWnd);
	diff.ShowUnifiedDiff(url1, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry1->revision),
						 url2, (bHead ? SVNRev::REV_HEAD : m_SelectedEntry2->revision),
						 m_SelectedEntry1->revision);
}

CTSVNPath CRevisionGraphDlg::DoUnifiedDiff(bool bHead, CString& sRoot, bool& bIsFolder)
{
	theApp.DoWaitCursor(1);
	CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, CTSVNPath(_T("test.diff")));
	// find selected objects
	ASSERT(m_SelectedEntry1 != NULL);
	ASSERT(m_SelectedEntry2 != NULL);
	
	// find out if m_sPath points to a file or a folder
	if (SVN::PathIsURL(m_sPath))
	{
		SVNInfo info;
		const SVNInfoData * infodata = info.GetFirstFileInfo(CTSVNPath(m_sPath), SVNRev::REV_HEAD, SVNRev::REV_HEAD);
		if (infodata)
		{
			bIsFolder = (infodata->kind == svn_node_dir);
		}
	}
	else
	{
		bIsFolder = CTSVNPath(m_sPath).IsDirectory();
	}
	
	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry1->url));
	url2.SetFromSVN(sRepoRoot+CUnicodeUtils::GetUnicode(m_SelectedEntry2->url));
	CTSVNPath url1_temp = url1;
	CTSVNPath url2_temp = url2;
	INT_PTR iMax = min(url1_temp.GetSVNPathString().GetLength(), url2_temp.GetSVNPathString().GetLength());
	INT_PTR i = 0;
	for ( ; ((i<iMax) && (url1_temp.GetSVNPathString().GetAt(i)==url2_temp.GetSVNPathString().GetAt(i))); ++i)
		;
	while (url1_temp.GetSVNPathString().GetLength()>i)
		url1_temp = url1_temp.GetContainingDirectory();

	if (bIsFolder)
		sRoot = url1_temp.GetSVNPathString();
	else
		sRoot = url1_temp.GetContainingDirectory().GetSVNPathString();
	
	if (url1.IsEquivalentTo(url2))
	{
		if (!svn.PegDiff(url1, SVNRev(m_SelectedEntry1->revision), 
			bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry1->revision), 
			bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry2->revision), 
			TRUE, TRUE, FALSE, FALSE, CString(), tempfile))
		{
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			theApp.DoWaitCursor(-1);
			return CTSVNPath();
		}
	}
	else
	{
		if (!svn.Diff(url1, bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry1->revision), 
			url2, bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(m_SelectedEntry2->revision), 
			TRUE, TRUE, FALSE, FALSE, CString(), false, tempfile))
		{
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);		
			theApp.DoWaitCursor(-1);
			return CTSVNPath();
		}
	}
	theApp.DoWaitCursor(-1);
	return tempfile;
}

void CRevisionGraphDlg::DoZoom(int nZoomFactor)
{
	m_node_space_left = NODE_SPACE_LEFT * nZoomFactor / 10;
	m_node_space_right = NODE_SPACE_RIGHT * nZoomFactor / 10;
	m_node_space_line = NODE_SPACE_LINE * nZoomFactor / 10;
	m_node_rect_heigth = NODE_RECT_HEIGTH * nZoomFactor / 10;
	m_node_space_top = NODE_SPACE_TOP * nZoomFactor / 10;
	m_node_space_bottom = NODE_SPACE_BOTTOM * nZoomFactor / 10;
	m_nFontSize = 12 * nZoomFactor / 10;
	m_RoundRectPt.x = ROUND_RECT * nZoomFactor / 10;
	m_RoundRectPt.y = ROUND_RECT * nZoomFactor / 10;
	m_nIconSize = 32 * nZoomFactor / 10;
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	InitView();
	Invalidate();
}

void CRevisionGraphDlg::OnViewZoomin()
{
	if (m_nZoomFactor < 20)
	{
		m_nZoomFactor++;
		DoZoom(m_nZoomFactor);
	}
}

void CRevisionGraphDlg::OnViewZoomout()
{
	if (m_nZoomFactor > 2)
	{
		m_nZoomFactor--;
		DoZoom(m_nZoomFactor);
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












