// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
{
	m_bThreadRunning = FALSE;
	m_lSelectedRev1 = -1;
	m_lSelectedRev2 = -1;
	m_pDlgTip = NULL;
	m_bNoGraph = FALSE;

	m_ViewRect.SetRectEmpty();
	
	m_nFontSize = 12;
	m_node_rect_width = NODE_RECT_WIDTH;
	m_node_space_left = NODE_SPACE_LEFT;
	m_node_space_right = NODE_SPACE_RIGHT;
	m_node_space_line = NODE_SPACE_LINE;
	m_node_rect_heigth = NODE_RECT_HEIGTH;
	m_node_space_top = NODE_SPACE_TOP;
	m_node_space_bottom = NODE_SPACE_BOTTOM;
	m_RoundRectPt = CPoint(ROUND_RECT, ROUND_RECT);
	for (int i=0; i<MAXFONTS; i++)
	{
		m_apFonts[i] = NULL;
	}
	m_nZoomFactor = 10;
	m_hAccel = 0;
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

	m_Progress.SetTitle(IDS_REVGRAPH_PROGTITLE);
	m_Progress.SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
	m_Progress.SetTime();

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

UINT CRevisionGraphDlg::WorkerThread(LPVOID pVoid)
{
	//get the status of all selected file/folders recursively
	//and show the ones which have to be committed to the user
	//in a listcontrol. 
	CRevisionGraphDlg*	pDlg;
	pDlg = (CRevisionGraphDlg*)pVoid;
	pDlg->m_bThreadRunning = TRUE;
	pDlg->m_Progress.ShowModeless(pDlg->m_hWnd);
	pDlg->m_bNoGraph = FALSE;
	if (!pDlg->FetchRevisionData(pDlg->m_sPath))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		pDlg->m_bNoGraph = TRUE;
		goto cleanup;
	}
	if (!pDlg->AnalyzeRevisionData(pDlg->m_sPath))
	{
		CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		pDlg->m_bNoGraph = TRUE;
	}
#ifdef DEBUG
	//pDlg->FillTestData();
#endif
cleanup:
	pDlg->m_Progress.Stop();
	pDlg->InitView();
	pDlg->m_bThreadRunning = FALSE;
	pDlg->Invalidate();
	return 0;
}

void CRevisionGraphDlg::InitView()
{
	m_ViewRect.SetRectEmpty();
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

INT_PTR CRevisionGraphDlg::GetIndexOfRevision(LONG rev) const
{
	for (INT_PTR i=0; i<m_arEntryPtrs.GetCount(); ++i)
	{
		if (((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->revision == rev)
			return i;
	}
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
		m_lfBaseFont.lfHeight = -MulDiv(m_nFontSize, GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
		// use the empty font name, so GDI takes the first font which matches
		// the specs. Maybe this will help render chinese/japanese chars correctly.
		_tcsncpy(m_lfBaseFont.lfFaceName, _T(""), 32);
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
			pDC->RoundRect(shadow, m_RoundRectPt);
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
			pDC->RoundRect(rect, m_RoundRectPt);
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
		CRect textrect = rect;
		textrect.left += 4;
		textrect.right -= 4;
		TEXTMETRIC textMetric;
		pDC->GetOutputTextMetrics(&textMetric);
		temp.Format(IDS_REVGRAPH_BOXREVISIONTITLE, rentry->revision);
		pDC->DrawText(temp, &r, DT_CALCRECT);
		pDC->ExtTextOut(textrect.left + ((rect.Width()-r.Width())/2), textrect.top + m_node_rect_heigth/4, ETO_CLIPPED, NULL, temp, NULL);
		pDC->SelectObject(GetFont(TRUE));
		temp = CUnicodeUtils::GetUnicode(rentry->url);
		r = textrect;
		temp.Replace('/','\\');
		pDC->DrawText(temp.GetBuffer(temp.GetLength()), temp.GetLength(), &r, DT_CALCRECT | DT_PATH_ELLIPSIS | DT_MODIFYSTRING);
		temp.ReleaseBuffer();
		temp.Replace('\\','/');
		pDC->ExtTextOut(textrect.left + 2 + ((textrect.Width()-4-r.Width())/2), textrect.top + m_node_rect_heigth/4 + m_node_rect_heigth/3, ETO_CLIPPED, &textrect, temp, NULL);
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
	
	memDC->FillSolidRect(rect, RGB(255,255,255));		// white background
	memDC->SetBkMode(TRANSPARENT);

	INT_PTR i = 0;
	while (i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) <= nVScrollPos)
		i++;
	i--;
	INT_PTR end = i;
	while (end*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) <= (nVScrollPos+m_ViewRect.bottom))
		end++;
	if (end > m_arEntryPtrs.GetCount())
		end = m_arEntryPtrs.GetCount();

	m_arNodeList.RemoveAll();
	m_arNodeRevList.RemoveAll();
	for ( ; i<end; ++i)
	{
		CRevisionEntry * entry = (CRevisionEntry*)m_arEntryPtrs.GetAt(i);
		CRect noderect;
		noderect.top = i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top - nVScrollPos;
		noderect.bottom = noderect.top + m_node_rect_heigth;
		noderect.left = (entry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left - nHScrollPos;
		noderect.right = noderect.left + m_node_rect_width;
		switch (entry->action)
		{
		case 'D':
			DrawNode(memDC, noderect, RGB(0,0,0), entry, TSVNOctangle, ((m_lSelectedRev1==entry->revision)||(m_lSelectedRev2==entry->revision)));
			break;
		case 'A':
			DrawNode(memDC, noderect, RGB(0,0,0), entry, TSVNRoundRect, ((m_lSelectedRev1==entry->revision)||(m_lSelectedRev2==entry->revision)));
			break;
		case 'R':
			DrawNode(memDC, noderect, RGB(0,0,0), entry, TSVNOctangle, ((m_lSelectedRev1==entry->revision)||(m_lSelectedRev2==entry->revision)));
			break;
		default:
			DrawNode(memDC, noderect, RGB(0,0,0), entry, TSVNRectangle, ((m_lSelectedRev1==entry->revision)||(m_lSelectedRev2==entry->revision)));
			break;
		}
		m_arNodeList.Add(noderect);
		m_arNodeRevList.Add(entry->revision);
	}
	DrawConnections(memDC, rect, nVScrollPos, nHScrollPos);
	if (!bDirectDraw)
		delete memDC;
}

void CRevisionGraphDlg::BuildConnections()
{
	CDWordArray connections;
	CDWordArray connections2;
	for (INT_PTR i=0; i<m_arConnections.GetCount(); ++i)
	{
		delete [] (CPoint*)m_arConnections.GetAt(i);
	}
	m_arConnections.RemoveAll();
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
				if (reventry->revision < reventry2->revision)
				{
					//      3-----4
					//      |
					//      |
					// 1----2

					//Starting point: 1
					pt[0].y = (i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += ((m_node_rect_heigth / (reventry->sourcearray.GetCount()+1))*(j+1));
					pt[0].x = ((reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + m_node_space_line;
					pt[1].x += (((m_node_space_left+m_node_space_right-2*m_node_space_line)/(connections.GetAt(reventry->level-1)+1))*connections2.GetAt(reventry->level-1));
					//line down: 3
					pt[2].x = pt[1].x;
					pt[2].y = (GetIndexOfRevision(sentry->revisionto)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[2].y += (m_node_rect_heigth / 2);
					//line to target: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry->revisionto)))->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += m_node_space_left;
					connections2.SetAt(reventry->level-1, connections2.GetAt(reventry->level-1)-1);
				}
				else
				{
					// 1----2
					//      |
					//      |
					//      3-----4

					//Starting point: 1
					pt[0].y = (i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += ((m_node_rect_heigth / (reventry->sourcearray.GetCount()+1))*(j+1));
					pt[0].x = ((reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + m_node_space_line;
					pt[1].x += (((m_node_space_left+m_node_space_right-2*m_node_space_line)/(connections.GetAt(reventry->level-1)+1))*connections2.GetAt(reventry->level-1));
					//line up: 3
					pt[2].x = pt[1].x;
					pt[2].y = (GetIndexOfRevision(sentry->revisionto)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[2].y += (m_node_rect_heigth / (reventry->sourcearray.GetCount()+1));
					//line to target: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry->revisionto)))->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += m_node_space_left;
					connections2.SetAt(reventry->level-1, connections2.GetAt(reventry->level-1)-1);
				}
			}
			else if (reventry->level > reventry2->level)
			{
				if (reventry->revision < reventry2->revision)
				{
					// 4----3
					//      |
					//      |
					//      2-----1

					//Starting point: 1
					pt[0].y = (i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += ((m_node_rect_heigth / (reventry->sourcearray.GetCount()+1))*(j+1));
					pt[0].x = ((reventry->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x - m_node_space_line;
					pt[1].x -= (((m_node_space_left+m_node_space_right-2*m_node_space_line)/(connections.GetAt(reventry->level-1)+1))*connections2.GetAt(reventry->level-1));
										//line down: 3
					pt[2].x = pt[1].x;
					pt[2].y = (GetIndexOfRevision(sentry->revisionto)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[2].y += (m_node_rect_heigth / (reventry->sourcearray.GetCount()+1));
					//line to target: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((((CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(sentry->revisionto)))->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += m_node_space_left+m_node_rect_width;
					connections2.SetAt(reventry->level-1, connections2.GetAt(reventry->level-1)-1);
				}
				else
				{
					//      3-----4
					//      |
					//      |
					// 1----2

					//Starting point: 1
					pt[0].y = (GetIndexOfRevision(sentry->revisionto)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += ((m_node_rect_heigth / (reventry->sourcearray.GetCount()+1))*(j+1));
					pt[0].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width);
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + m_node_space_line;
					pt[1].x += (((m_node_space_left+m_node_space_right-2*m_node_space_line)/(connections.GetAt(reventry->level-1)+1))*connections2.GetAt(reventry->level-1));
					//line up: 3
					pt[2].x = pt[1].x;
					pt[2].y = (i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[2].y += (m_node_rect_heigth / (reventry->sourcearray.GetCount()+1));
					//line to target: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((((CRevisionEntry*)m_arEntryPtrs.GetAt(i))->level-1)*(m_node_rect_width+m_node_space_left+m_node_space_right));
					pt[3].x += m_node_space_left;
					connections2.SetAt(reventry->level-1, connections2.GetAt(reventry->level-1)-1);
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
					// 1----2
					//      |
					//      |
					//      |
					// 4----3
					//Starting point: 1
					pt[0].y = (i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[0].y += (m_node_rect_heigth / 2);
					pt[0].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left) + m_node_rect_width;
					//line to middle of nodes: 2
					pt[1].y = pt[0].y;
					pt[1].x = pt[0].x + m_node_space_line;
					pt[1].x += (((m_node_space_left+m_node_space_right-2*m_node_space_line)/(connections.GetAt(reventry2->level-1)+1))*connections2.GetAt(reventry2->level-1));
					//line down: 3
					pt[2].x = pt[1].x;
					pt[2].y = (GetIndexOfRevision(sentry->revisionto)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
					pt[2].y += (m_node_rect_heigth / (reventry->sourcearray.GetCount()+1))*(j+1);
					//line to target: 4
					pt[3].y = pt[2].y;
					pt[3].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left) + m_node_rect_width;
					connections2.SetAt(reventry2->level-1, connections2.GetAt(reventry2->level-1)-1);
				}
				else
				{
					if (reventry->revision > reventry2->revision)
					{
						// 1
						// |
						// |
						// 2
						pt[0].y = (i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top) + m_node_rect_heigth;
						pt[0].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width/2);
						pt[1].y = pt[0].y;
						pt[1].x = pt[0].x;
						pt[2].y = (GetIndexOfRevision(sentry->revisionto)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
						pt[2].x = pt[0].x;
						pt[3].y = pt[2].y;
						pt[3].x = pt[2].x;
					}
					else
					{
						// 2
						// |
						// |
						// 1
						pt[0].y = (i*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top);
						pt[0].x = ((reventry2->level - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left + m_node_rect_width/2);
						pt[1].y = pt[0].y;
						pt[1].x = pt[0].x;
						pt[2].y = (GetIndexOfRevision(sentry->revisionto)*(m_node_rect_heigth+m_node_space_top+m_node_space_bottom) + m_node_space_top + m_node_rect_heigth);
						pt[2].x = pt[0].x;
						pt[3].y = pt[2].y;
						pt[3].x = pt[2].x;
					}
				}
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
		//if (viewrect.PtInRect(pt[0])||viewrect.PtInRect(pt[3]))
		{
			POINT p[4];
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
	m_ViewRect.right = level * (m_node_rect_width + m_node_space_left + m_node_space_right);
	m_ViewRect.bottom = m_arEntryPtrs.GetCount() * (m_node_rect_heigth + m_node_space_top + m_node_space_bottom);
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
	bool bHit = false;
	for (INT_PTR i=0; i<m_arNodeList.GetCount(); ++i)
	{
		if (m_arNodeList.GetAt(i).PtInRect(point))
		{
			if (m_lSelectedRev1 == (LONG)m_arNodeRevList.GetAt(i))
				m_lSelectedRev1 = -1;
			else if (m_lSelectedRev2 == (LONG)m_arNodeRevList.GetAt(i))
				m_lSelectedRev2 = -1;
			else if (m_lSelectedRev1 < 0)
				m_lSelectedRev1 = m_arNodeRevList.GetAt(i);
			else if (m_lSelectedRev2 < 0)
				m_lSelectedRev2 = m_arNodeRevList.GetAt(i);
			else
				m_lSelectedRev2 = m_arNodeRevList.GetAt(i);
			bHit = true;
			Invalidate();
		}
	}
	if (!bHit)
	{
		m_lSelectedRev1 = -1;
		m_lSelectedRev2 = -1;
		Invalidate();
	}
	
	UINT uEnable = MF_BYCOMMAND;
	if ((m_lSelectedRev2 >= 0)&&(m_lSelectedRev1 >= 0))
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
	_tcscpy (pszFilters, sFilter);
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
	int pos = GetScrollPos(SB_VERT);
	pos -= (zDelta);
	SetScrollPos(SB_VERT, pos);
	Invalidate();
	return __super::OnMouseWheel(nFlags, zDelta, pt);
}

void CRevisionGraphDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	if ((m_lSelectedRev1 < 0)||(m_lSelectedRev2 < 0))
		return;
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		CRevisionEntry * entry1 = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_lSelectedRev1));
		CRevisionEntry * entry2 = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_lSelectedRev2));
		if ((entry1->action == 'D')||(entry2->action == 'D'))
			return;	// we can't compare with deleted items

		bool bSameURL = (strcmp(entry1->url, entry2->url)==0);
		CString temp;
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

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
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
		}
	}
}

void CRevisionGraphDlg::CompareRevs(bool bHead)
{
	ASSERT(m_lSelectedRev1 >= 0);
	ASSERT(m_lSelectedRev2 >= 0);

	CRevisionEntry * entry1 = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_lSelectedRev1));
	CRevisionEntry * entry2 = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_lSelectedRev2));

	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN(sRepoRoot+CString(entry1->url));
	url2.SetFromSVN(sRepoRoot+CString(entry2->url));

	SVNDiff diff(&svn, this->m_hWnd);
	diff.ShowCompare(url1, (bHead ? SVNRev::REV_HEAD : entry1->revision),
		url2, (bHead ? SVNRev::REV_HEAD : entry2->revision),
		entry1->revision);
}

void CRevisionGraphDlg::UnifiedDiffRevs(bool bHead)
{
	ASSERT(m_lSelectedRev1 >= 0);
	ASSERT(m_lSelectedRev2 >= 0);

	CRevisionEntry * entry1 = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_lSelectedRev1));
	CRevisionEntry * entry2 = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_lSelectedRev2));

	SVN svn;
	CString sRepoRoot;
	if (SVN::PathIsURL(m_sPath))
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(m_sPath));
	else
		sRepoRoot = svn.GetRepositoryRoot(CTSVNPath(svn.GetURLFromPath(CTSVNPath(m_sPath))));

	CTSVNPath url1;
	CTSVNPath url2;
	url1.SetFromSVN(sRepoRoot+CString(entry1->url));
	url2.SetFromSVN(sRepoRoot+CString(entry2->url));

	SVNDiff diff(&svn, this->m_hWnd);
	diff.ShowUnifiedDiff(url1, (bHead ? SVNRev::REV_HEAD : entry1->revision),
						 url2, (bHead ? SVNRev::REV_HEAD : entry2->revision),
						 entry1->revision);
}

CTSVNPath CRevisionGraphDlg::DoUnifiedDiff(bool bHead, CString& sRoot, bool& bIsFolder)
{
	theApp.DoWaitCursor(1);
	CTSVNPath tempfile = CTempFiles::Instance().GetTempFilePath(true, CTSVNPath(_T("test.diff")));
	// find selected objects
	ASSERT(m_lSelectedRev1 >= 0);
	ASSERT(m_lSelectedRev2 >= 0);
	
	CRevisionEntry * entry1 = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_lSelectedRev1));
	CRevisionEntry * entry2 = (CRevisionEntry*)m_arEntryPtrs.GetAt(GetIndexOfRevision(m_lSelectedRev2));
	
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
	url1.SetFromSVN(sRepoRoot+CString(entry1->url));
	url2.SetFromSVN(sRepoRoot+CString(entry2->url));
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
		if (!svn.PegDiff(url1, SVNRev(entry1->revision), 
			bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(entry1->revision), 
			bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(entry2->revision), 
			TRUE, TRUE, FALSE, FALSE, CString(), tempfile))
		{
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			theApp.DoWaitCursor(-1);
			return CTSVNPath();
		}
	}
	else
	{
		if (!svn.Diff(url1, bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(entry1->revision), 
			url2, bHead ? SVNRev(SVNRev::REV_HEAD) : SVNRev(entry2->revision), 
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
	m_node_rect_width = NODE_RECT_WIDTH * nZoomFactor / 10;
	m_node_space_left = NODE_SPACE_LEFT * nZoomFactor / 10;
	m_node_space_right = NODE_SPACE_RIGHT * nZoomFactor / 10;
	m_node_space_line = NODE_SPACE_LINE * nZoomFactor / 10;
	m_node_rect_heigth = NODE_RECT_HEIGTH * nZoomFactor / 10;
	m_node_space_top = NODE_SPACE_TOP * nZoomFactor / 10;
	m_node_space_bottom = NODE_SPACE_BOTTOM * nZoomFactor / 10;
	m_nFontSize = 12 * nZoomFactor / 10;
	m_RoundRectPt.x = ROUND_RECT * nZoomFactor / 10;
	m_RoundRectPt.y = ROUND_RECT * nZoomFactor / 10;
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

void CRevisionGraphDlg::OnCancel()
{
	if (!m_bThreadRunning)
		__super::OnCancel();
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
	se = new source_entry;
	se->pathto = "/trunk";
	se->revisionto = 90;
	e->sourcearray.Add(se);
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 2;
	e->revision = 97;
	e->url = "/tags/version1";
	e->author = "kueng";
	e->message = "remove version 1";
	e->revisionfrom	= 95;
	e->pathfrom = "/tags/version1";
	e->action = 'D';
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
	se = new source_entry;
	se->pathto = "/tags/version1";
	se->revisionto = 97;
	e->sourcearray.Add(se);
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

	e = new CRevisionEntry();
	e->level = 3;
	e->revision = 80;
	e->url = "/branches/test";
	e->author = "kueng";
	e->message = "testing";
	se = new source_entry;
	se->pathto = "/branches/test2";
	se->revisionto = 75;
	e->sourcearray.Add(se);
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 2;
	e->revision = 75;
	e->url = "/branches/test";
	e->author = "kueng";
	e->message = "testing";
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 2;
	e->revision = 74;
	e->url = "/branches/test";
	e->author = "kueng";
	e->message = "testing";
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 3;
	e->revision = 73;
	e->url = "/branches/test";
	e->author = "kueng";
	e->message = "testing";
	se = new source_entry;
	se->pathto = "/branches/test2";
	se->revisionto = 74;
	e->sourcearray.Add(se);
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 2;
	e->revision = 64;
	e->url = "/branches/test";
	e->author = "kueng";
	e->message = "testing";
	se = new source_entry;
	se->pathto = "/branches/test2";
	se->revisionto = 63;
	e->sourcearray.Add(se);
	m_arEntryPtrs.Add(e);

	e = new CRevisionEntry();
	e->level = 3;
	e->revision = 63;
	e->url = "/branches/test";
	e->author = "kueng";
	e->message = "testing";
	m_arEntryPtrs.Add(e);
}
#endif //DEBUG









