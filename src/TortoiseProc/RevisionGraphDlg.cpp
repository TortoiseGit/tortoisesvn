// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "MemDC.h"
#include ".\revisiongraphdlg.h"

// CRevisionGraphDlg dialog

IMPLEMENT_DYNAMIC(CRevisionGraphDlg, CResizableDialog)
CRevisionGraphDlg::CRevisionGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CRevisionGraphDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bThreadRunning = FALSE;
	m_lSelectedRev = -1;
	m_pDlgTip = NULL;
}

CRevisionGraphDlg::~CRevisionGraphDlg()
{
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
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
	CResizableDialog::DoDataExchange(pDX);
}

// The system calls this function to obtain the cursor to display while the user drags
// the minimized window.
HCURSOR CRevisionGraphDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BEGIN_MESSAGE_MAP(CRevisionGraphDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_ERASEBKGND()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
END_MESSAGE_MAP()


// CRevisionGraphDlg message handlers

BOOL CRevisionGraphDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

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

	CString temp;
	temp.LoadString(IDS_REVGRAPH_PROGTITLE);
	m_Progress.SetTitle(temp);
	temp.LoadString(IDS_REVGRAPH_PROGCANCEL);
	m_Progress.SetCancelMsg(temp);
	m_Progress.SetTime();

	m_dwTicks = GetTickCount();
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &WorkerThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRevisionGraphDlg::ProgressCallback(CString text, CString text2, DWORD done, DWORD total)
{
	if ((m_dwTicks+100)<GetTickCount())
	{
		m_dwTicks = GetTickCount();
		m_Progress.SetLine(1, text);
		m_Progress.SetLine(2, text2);
		m_Progress.SetProgress(done, total);
		if (m_Progress.HasUserCancelled())
			return FALSE;
	}
	return TRUE;
}

DWORD WINAPI WorkerThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CRevisionGraphDlg*	pDlg;
	pDlg = (CRevisionGraphDlg*)pVoid;
	pDlg->m_bThreadRunning = TRUE;
	pDlg->m_Progress.ShowModeless(pDlg->m_hWnd);
	pDlg->FetchRevisionData(pDlg->m_sPath);
	pDlg->AnalyzeRevisionData(pDlg->m_sPath);
	pDlg->m_Progress.Stop();
	//pDlg->FillTestData();
	pDlg->InitView();
	pDlg->m_bThreadRunning = FALSE;
	pDlg->Invalidate();
	return 0;
}

void CRevisionGraphDlg::InitView()
{
	GetViewSize();
	BuildConnections();
	SetScrollbars();
}

void CRevisionGraphDlg::SetScrollbars(int nVert, int nHorz)
{
	CRect clientrect;
	GetClientRect(&clientrect);
	CRect * pRect = GetViewSize();
	SCROLLINFO ScrollInfo;
	ScrollInfo.cbSize = sizeof(SCROLLINFO);
	ScrollInfo.fMask = SIF_ALL;
	ScrollInfo.nMin = 0;
	ScrollInfo.nMax = pRect->bottom;
	ScrollInfo.nPage = clientrect.Height();
	ScrollInfo.nPos = nVert;
	ScrollInfo.nTrackPos = 0;
	SetScrollInfo(SB_VERT, &ScrollInfo);
	ScrollInfo.nMax = pRect->right;
	ScrollInfo.nPage = clientrect.Width();
	ScrollInfo.nPos = nHorz;
	SetScrollInfo(SB_HORZ, &ScrollInfo);
}

INT_PTR CRevisionGraphDlg::GetIndexOfRevision(LONG rev)
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		if (((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->revision == rev)
			return i;
	}
	ASSERT(FALSE);
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
		m_lfBaseFont.lfHeight = -MulDiv(10, GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
		_tcsncpy(m_lfBaseFont.lfFaceName, _T("Courier New"), 32);
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

	if (IsIconic())
	{
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		if (m_bThreadRunning)
		{
			CResizableDialog::OnPaint();
			return;
		}
		GetViewSize();
		DrawGraph(&dc, rect, GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ));
	}
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
								BOOL isSel, int penStyle /*= PS_SOLID*/)
{
	CPen* pOldPen = 0L;
	CBrush* pOldBrush = 0L;
	CFont* pOldFont = 0L;
	CPen pen, pen2;
	CBrush brush;
	COLORREF selcolor = RGB_DEF_SEL;
	COLORREF shadowc = RGB_DEF_SHADOW;

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
			pDC->RoundRect(shadow, ROUND_RECT_PT);
			break;
		case TSVNOctangle:
			DrawOctangle(pDC, shadow);
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
			pDC->RoundRect(rect, ROUND_RECT_PT);
			break;
		case TSVNOctangle:
			DrawOctangle(pDC, rect);
			break;
		default:
			ASSERT(FALSE);	//unknown type
			return;
		}
		pOldFont = pDC->SelectObject(GetFont(FALSE, TRUE));
		CString temp;
		CRect r;
		TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);
		temp.Format(IDS_REVGRAPH_BOXREVISIONTITLE, rentry->revision);
		pDC->DrawText(temp, &r, DT_CALCRECT);
		pDC->ExtTextOut(rect.left + ((rect.Width()-r.Width())/2), rect.top + NODE_RECT_HEIGTH/4, ETO_CLIPPED, NULL, temp, NULL);
		pDC->SelectObject(GetFont(TRUE));
		temp = CUnicodeUtils::GetUnicode(rentry->url);
		r = rect;
		temp.Replace('/','\\');
		pDC->DrawText(temp.GetBuffer(temp.GetLength()), temp.GetLength(), &r, DT_CALCRECT | DT_PATH_ELLIPSIS | DT_MODIFYSTRING);
		temp.ReleaseBuffer();
		temp.Replace('\\','/');
		pDC->ExtTextOut(rect.left + ((rect.Width()-r.Width())/2), rect.top + NODE_RECT_HEIGTH/4 + NODE_RECT_HEIGTH/3, ETO_CLIPPED, &rect, temp, NULL);
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

void CRevisionGraphDlg::DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos)
{
	CMemDC memDC(pDC);
	memDC.FillSolidRect(rect, RGB(255,255,255));		// white background
	memDC.SetBkMode(TRANSPARENT);

	INT_PTR i = 0;
	while (i*(NODE_RECT_HEIGTH+NODE_SPACE_TOP+NODE_SPACE_BOTTOM) <= nVScrollPos)
		i++;
	i--;
	INT_PTR end = i;
	while (end*(NODE_RECT_HEIGTH+NODE_SPACE_TOP+NODE_SPACE_BOTTOM) <= (nVScrollPos+m_ViewRect.bottom))
		end++;
	if (end > m_arEntryPtrs.GetCount())
		end = m_arEntryPtrs.GetCount();

	m_arNodeList.RemoveAll();
	m_arNodeRevList.RemoveAll();
	for ( ; i<end; ++i)
	{
		CRevisionEntry * entry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		CRect noderect;
		noderect.top = i*(NODE_RECT_HEIGTH+NODE_SPACE_TOP+NODE_SPACE_BOTTOM) + NODE_SPACE_TOP - nVScrollPos;
		noderect.bottom = noderect.top + NODE_RECT_HEIGTH;
		noderect.left = (entry->level - 1)*(NODE_RECT_WIDTH+NODE_SPACE_LEFT+NODE_SPACE_RIGHT) + NODE_SPACE_LEFT - nHScrollPos;
		noderect.right = noderect.left + NODE_RECT_WIDTH;
		switch (entry->action)
		{
		case 'D':
			DrawNode(&memDC, noderect, RGB(0,0,0), entry, TSVNOctangle, m_lSelectedRev==entry->revision);
			break;
		case 'A':
			DrawNode(&memDC, noderect, RGB(0,0,0), entry, TSVNRoundRect, m_lSelectedRev==entry->revision);
			break;
		case 'R':
			DrawNode(&memDC, noderect, RGB(0,0,0), entry, TSVNOctangle, m_lSelectedRev==entry->revision);
			break;
		default:
			DrawNode(&memDC, noderect, RGB(0,0,0), entry, TSVNRectangle, m_lSelectedRev==entry->revision);
			break;
		}
		m_arNodeList.Add(noderect);
		m_arNodeRevList.Add(entry->revision);
	}
	DrawConnections(&memDC, rect, nVScrollPos, nHScrollPos);

}

void CRevisionGraphDlg::BuildConnections()
{
	CDWordArray connections;
	CDWordArray connections2;
	// check how many connections there are between each level
	// this is used to add a slight offset to the vertical lines
	// so they're not overlapping
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		if (connections.GetCount()<reventry->level)
			connections.SetAtGrow(reventry->level-1, 0);

		int leveloffset = -1;
		for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
		{
			source_entry * sentry = (source_entry*)reventry->sourcearray.GetAt(j);
			if (reventry->level < ((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry->revisionto)))->level)
				leveloffset = -1;
			else
				leveloffset = 0;
			if (connections.GetCount()<reventry->level+1+leveloffset)
				connections.SetAtGrow(reventry->level+leveloffset, 0);
			connections.SetAt(reventry->level+leveloffset, connections.GetAt(reventry->level+leveloffset)+1);
		}
	}
	for (INT_PTR i=0; i<connections.GetCount(); ++i)
		connections2.SetAtGrow(i, connections.GetAt(i));
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		CRevisionEntry * reventry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		for (INT_PTR j=0; j<reventry->sourcearray.GetCount(); ++j)
		{
			source_entry * sentry = (source_entry*)reventry->sourcearray.GetAt(j);
			CRevisionEntry * reventry2 = ((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry->revisionto)));
			CPoint * pt = new CPoint[4];
			if (reventry->level < reventry2->level)
			{
				// 1----2
				//      |
				//      |
				//      3-----4

				//Starting point: 1
				pt[0].y = (i*(NODE_RECT_HEIGTH+NODE_SPACE_TOP+NODE_SPACE_BOTTOM) + NODE_SPACE_TOP);
				pt[0].y += ((NODE_RECT_HEIGTH / (reventry->sourcearray.GetCount()+1))*(j+1));
				pt[0].x = ((reventry->level - 1)*(NODE_RECT_WIDTH+NODE_SPACE_LEFT+NODE_SPACE_RIGHT) + NODE_SPACE_LEFT + NODE_RECT_WIDTH);
				//line to middle of nodes: 2
				pt[1].y = pt[0].y;
				pt[1].x = pt[0].x + NODE_SPACE_LINE;
				pt[1].x += (((NODE_SPACE_LEFT+NODE_SPACE_RIGHT-2*NODE_SPACE_LINE)/(connections.GetAt(reventry->level-1)+1))*connections2.GetAt(reventry->level-1));
				//line down: 3
				pt[2].x = pt[1].x;
				pt[2].y = (GetIndexOfRevision(sentry->revisionto)*(NODE_RECT_HEIGTH+NODE_SPACE_TOP+NODE_SPACE_BOTTOM) + NODE_SPACE_TOP);
				pt[2].y += (NODE_RECT_HEIGTH / 2);
				//line to target: 4
				pt[3].y = pt[2].y;
				pt[3].x = ((((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry->revisionto)))->level-1)*(NODE_RECT_WIDTH+NODE_SPACE_LEFT+NODE_SPACE_RIGHT));
				pt[3].x += NODE_SPACE_LEFT;
				connections2.SetAt(reventry->level-1, connections2.GetAt(reventry->level-1)-1);
			}
			else if (reventry->level > reventry2->level)
			{
				//      2-----1
				//      |
				//      |
				// 4----3

				//Starting point: 1
				pt[0].y = (i*(NODE_RECT_HEIGTH+NODE_SPACE_TOP+NODE_SPACE_BOTTOM) + NODE_SPACE_TOP);
				pt[0].y += ((NODE_RECT_HEIGTH / (reventry2->sourcearray.GetCount()+1))*(j+1));
				pt[0].x = ((reventry2->level - 1)*(NODE_RECT_WIDTH+NODE_SPACE_LEFT+NODE_SPACE_RIGHT) + NODE_SPACE_LEFT);
				//line to middle of nodes: 2
				pt[1].y = pt[0].y;
				pt[1].x = pt[0].x - NODE_SPACE_LINE;
				pt[1].x -= (((NODE_SPACE_LEFT+NODE_SPACE_RIGHT-2*NODE_SPACE_LINE)/(connections.GetAt(reventry2->level-1)+1))*connections2.GetAt(reventry2->level-1));
				//line down: 3
				pt[2].x = pt[1].x;
				pt[2].y = (GetIndexOfRevision(sentry->revisionto)*(NODE_RECT_HEIGTH+NODE_SPACE_TOP+NODE_SPACE_BOTTOM) + NODE_SPACE_TOP);
				pt[2].y += (NODE_RECT_HEIGTH / (reventry->sourcearray.GetCount()+1));
				//line to target: 4
				pt[3].y = pt[2].y;
				pt[3].x = ((((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry->revisionto)))->level-1)*(NODE_RECT_WIDTH+NODE_SPACE_LEFT+NODE_SPACE_RIGHT));
				pt[3].x += NODE_SPACE_LEFT+NODE_RECT_WIDTH;
				connections2.SetAt(reventry2->level-1, connections2.GetAt(reventry2->level-1)-1);
			}
			else
			{
				// same level!
			}
			m_arConnections.Add(pt);
		}
	}
}

void CRevisionGraphDlg::DrawConnections(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos)
{
	CRect viewrect;
	viewrect.top = rect.top + nVScrollPos;
	viewrect.bottom = rect.bottom + nVScrollPos;
	viewrect.left = rect.left + nHScrollPos;
	viewrect.right = rect.right + nHScrollPos;
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		CPoint * pt = (CPoint*)m_arConnections.GetAt(i);
		// only draw the lines if they're at least partially visible
		if (viewrect.PtInRect(pt[0])||viewrect.PtInRect(pt[3]))
		{
			CPoint p[4];
			// correct the scroll offset
			p[0].x = pt[0].x - nHScrollPos;
			p[1].x = pt[1].x - nHScrollPos;
			p[2].x = pt[2].x - nHScrollPos;
			p[3].x = pt[3].x - nHScrollPos;
			p[0].y = pt[0].y - nVScrollPos;
			p[1].y = pt[1].y - nVScrollPos;
			p[2].y = pt[2].y - nVScrollPos;
			p[3].y = pt[3].y - nVScrollPos;

			pDC->Polyline(p, 4);
		}
	}
}

CRect * CRevisionGraphDlg::GetViewSize()
{
	if (m_ViewRect.Height() != 0)
		return &m_ViewRect;
	m_ViewRect.top = 0;
	m_ViewRect.left = 0;
	int level = 0;
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		if (level < ((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->level)
			level = ((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->level;
	}
	m_ViewRect.right = level * (NODE_RECT_WIDTH + NODE_SPACE_LEFT + NODE_SPACE_RIGHT);
	m_ViewRect.bottom = m_arEntryPtrs.GetCount() * (NODE_RECT_HEIGTH + NODE_SPACE_TOP + NODE_SPACE_BOTTOM);
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
	for (INT_PTR i=0; i<m_arNodeList.GetCount(); ++i)
	{
		if (m_arNodeList.GetAt(i).PtInRect(point))
		{
			if (m_lSelectedRev == (LONG)m_arNodeRevList.GetAt(i))
				m_lSelectedRev = -1;
			else
				m_lSelectedRev = m_arNodeRevList.GetAt(i);
			Invalidate();
		}
	}
	__super::OnLButtonDown(nFlags, point);
}

int CRevisionGraphDlg::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	for (INT_PTR i=0; i<m_arNodeList.GetCount(); ++i)
	{
		if (m_arNodeList.GetAt(i).PtInRect(point))
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
		for (INT_PTR i=0; i<m_arNodeList.GetCount(); ++i)
		{
			if (m_arNodeList.GetAt(i).PtInRect(point))
			{
				rentry = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_arNodeRevList.GetAt(i)));
			}
		}
		if (rentry)
		{
			TCHAR date[200];
			SVN::formatDate(date, rentry->date);
			strTipText.Format(IDS_REVGRAPH_BOXTOOLTIP, 
							CUnicodeUtils::GetUnicode(rentry->author), 
							date,
							CUnicodeUtils::GetUnicode(rentry->message));
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
#ifdef UNICODE
		pTTTA->lpszText = m_szTip;
		WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
#else
		lstrcpyn(m_szTip, strTipText, strTipText.GetLength()+1);
		pTTTA->lpszText = m_szTip;
#endif
	} // if (pNMHDR->code == TTN_NEEDTEXTA)
	else
	{
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);
#ifdef UNICODE
		lstrcpyn(m_wszTip, strTipText, strTipText.GetLength()+1);
		pTTTW->lpszText = m_wszTip;
#else
		pTTTW->lpszText = m_wszTip;
		::MultiByteToWideChar( CP_ACP , 0, strTipText, -1, m_wszTip, strTipText.GetLength()+1 );
#endif
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
	return __super::PreTranslateMessage(pMsg);
}

#ifdef DEBUG
void CRevisionGraphDlg::FillTestData()
{
	CRevisionEntry * e = new CRevisionEntry();
	e->level = 2;
	e->revision = 100;
	e->url = "/tags/version 23";
	e->author = "kueng";
	e->message = "tagged version 23";
	e->revisionfrom	= 99;
	e->pathfrom = "/trunk";
	m_arEntryPtrs.Add(e);
	
	e = new CRevisionEntry();
	e->level = 1;
	e->revision = 99;
	e->url = "/trunk";
	e->author = "kueng";
	e->message = "something else";
	source_entry * se = new source_entry;
	se->pathto = "/tags/version 23";
	se->revisionto = 100;
	e->sourcearray.Add(se);
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 2;
	e->revision = 96;
	e->url = "/tags/version2";
	e->author = "kueng";
	e->message = "tagged version 2";
	e->revisionfrom	= 99;
	e->pathfrom = "/trunk";
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 2;
	e->revision = 95;
	e->url = "/tags/version1";
	e->author = "kueng";
	e->message = "tagged version 1";
	e->revisionfrom	= 99;
	e->pathfrom = "/trunk";
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 3;
	e->revision = 92;
	e->url = "/branches/testingrenamed";
	e->author = "kueng";
	e->message = "something else renamed";
	e->pathfrom = "/branches/testing";
	e->revisionfrom = 91;
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 2;
	e->revision = 91;
	e->url = "/branches/testing";
	e->author = "kueng";
	e->message = "something else";
	e->pathfrom = "/trunk";
	e->revisionfrom = 80;
	se = new source_entry;
	se->pathto = "/branches/testingrenamed";
	se->revisionto = 92;
	e->sourcearray.Add(se);
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 1;
	e->revision = 90;
	e->url = "/trunk";
	e->author = "kueng";
	e->message = "a start";
	se = new source_entry;
	se->pathto = "/tags/version1";
	se->revisionto = 95;
	e->sourcearray.Add(se);
	se = new source_entry;
	se->pathto = "/tags/version2";
	se->revisionto = 96;
	e->sourcearray.Add(se);
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 4;
	e->revision = 85;
	e->url = "/branches/test2";
	e->author = "kueng";
	e->message = "testing again";
	e->pathfrom = "/branches/test";
	e->revisionfrom = 82;
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 3;
	e->revision = 82;
	e->url = "/branches/test";
	e->author = "kueng";
	e->message = "testing";
	se = new source_entry;
	se->pathto = "/branches/test2";
	se->revisionto = 85;
	e->sourcearray.Add(se);
	m_arEntryPtrs.Add(e);

}
#endif //DEBUG