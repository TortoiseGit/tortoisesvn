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
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include ".\revisiongraphwnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

/************************************************************************/
/* Graphing functions                                                   */
/************************************************************************/
CFont* CRevisionGraphWnd::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/)
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
			return CWnd::GetFont();
		}
	}
	return m_apFonts[nIndex];
}

BOOL CRevisionGraphWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CRevisionGraphWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(&rect);
	if (m_bThreadRunning)
	{
		dc.FillSolidRect(rect, ::GetSysColor(COLOR_APPWORKSPACE));
		CWnd::OnPaint();
		return;
	}
	else if (m_bNoGraph || m_entryPtrs.empty())
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

void CRevisionGraphWnd::DrawOctangle(CDC * pDC, const CRect& rect)
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

void CRevisionGraphWnd::DrawNode(CDC * pDC, const CRect& rect,
								COLORREF contour, CRevisionEntry *rentry, NodeShape shape, 
								BOOL isSel, HICON hIcon, int penStyle /*= PS_SOLID*/)
{
#define minmax(x, y, z) (x > 0 ? min<long>(y, z) : max<long>(y, z))
	CPen* pOldPen = 0L;
	CBrush* pOldBrush = 0L;
	CFont* pOldFont = 0L;
	CPen pen, pen2;
	CBrush brush;
	COLORREF background = GetSysColor(COLOR_WINDOW);
	COLORREF textcolor = GetSysColor(COLOR_WINDOWTEXT);

    // special case: line deleted but deletion node removed

    if (   (rentry->next == NULL) 
        && (rentry->classification & CPathClassificator::IS_DELETED))
    {
        contour = m_Colors.GetColor(CColors::DeletedNode);
    }

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
		if (rect.Height() > 10)
		{
			CRect shadow = rect;
			CPoint shadowoffset = SHADOW_OFFSET_PT;
			if (rect.Height() < 40)
			{
				shadowoffset.x--;
				shadowoffset.y--;
			}
			if (rect.Height() < 30)
			{
				shadowoffset.x--;
				shadowoffset.y--;
			}
			if (rect.Height() < 20)
			{
				shadowoffset.x--;
				shadowoffset.y--;
			}
			shadow.OffsetRect(shadowoffset);

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

		if (m_nFontSize)
		{
			pDC->SetTextColor(textcolor);
			// draw the revision text
			pOldFont = pDC->SelectObject(GetFont(FALSE, TRUE));
			CString temp;
			CRect r = rect;
			CRect textrect = rect;
			textrect.left += (m_nIconSize+2);
			temp.Format(IDS_REVGRAPH_BOXREVISIONTITLE, rentry->revision);
			pDC->DrawText(temp, &r, DT_CALCRECT);
			int offset = (int)m_node_rect_height;
			bool bShowUrl = true;
			int th = r.Height();
			if ((th+2) < (m_node_rect_height/2))
			{
				offset = (offset - (m_nFontSize*2) - 2)/4;
				if (offset == 0)
				{
					bShowUrl = false;
					offset = ((int)m_node_rect_height - m_nFontSize - 2)/2;
				}
			}
			else
			{
				offset = (offset - m_nFontSize - 2)/2;
				bShowUrl = false;
			}
			if (offset > 0)
			{
				// only draw the revision text if the node rectangle is big enough for it
				pDC->ExtTextOut(textrect.left + ((textrect.Width()-r.Width())/2), textrect.top + offset, ETO_CLIPPED, NULL, temp, NULL);
			}

			if (bShowUrl)
			{
				// draw the url only if the rectangle is big enough, otherwise we only draw the revision
				pDC->SelectObject(GetFont(TRUE));
				temp = CUnicodeUtils::GetUnicode (rentry->path.GetPath().c_str());
				temp.Replace('/','\\');
				r = textrect;
				pDC->DrawText(temp.GetBuffer(temp.GetLength()), temp.GetLength(), &r, DT_CALCRECT | DT_PATH_ELLIPSIS | DT_MODIFYSTRING);
				temp.ReleaseBuffer();
				temp.Replace('\\','/');
				pDC->ExtTextOut(textrect.left + 2 + ((textrect.Width()-4-r.Width())/2), textrect.top + (2*offset) + th, ETO_CLIPPED, &textrect, temp, NULL);
			}
		}

		if (m_nIconSize)
		{
			// draw the icon
			CPoint iconpoint = CPoint(rect.left + m_nIconSize/6, rect.top + m_nIconSize/6);
			CSize iconsize = CSize(m_nIconSize, m_nIconSize);
			pDC->DrawState(iconpoint, iconsize, hIcon, DST_ICON, (HBRUSH)NULL);
		}
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

void CRevisionGraphWnd::DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw)
{
	if (m_node_rect_height < REVGRAPH_MIN_NODE_HIGHT)
		return;

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

	// find out which nodes are in the visible area of the client rect

	HICON hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_DELETED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hAddedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_ADDED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hAddedWithHistoryIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_ADDEDPLUS), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_REPLACED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hRenamedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_RENAMED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hLastCommitIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_LASTCOMMIT), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);
	HICON hTaggedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REVGRAPH_TAGGED), IMAGE_ICON, m_nIconSize, m_nIconSize, LR_DEFAULTCOLOR);

	for (size_t i = 0, count = m_entryPtrs.size(); i < count; ++i)
	{
		CRevisionEntry * entry = m_entryPtrs[i];

		CRect noderect;
		float top = (entry->row - 1)*(m_node_rect_height+m_node_space_top+m_node_space_bottom) + m_node_space_top - float(nVScrollPos);
		noderect.top = long(top);
		noderect.bottom = long(top + m_node_rect_height);
		float left = (entry->column - 1)*(m_node_rect_width+m_node_space_left+m_node_space_right) + m_node_space_left - float(nHScrollPos);
		noderect.left = long(left);
		noderect.right = long(left + m_node_rect_width);

		// skip it, if not visible

		if (   (noderect.right < rect.left) || (noderect.left > rect.right) 
			|| (noderect.bottom < rect.top) || (noderect.top > rect.bottom))
		{
			continue;
		}

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

    	// Draw the "tagged" icon

        if (m_nIconSize && !entry->tags.empty())
		{
			// draw the icon
			CPoint iconpoint = CPoint(noderect.right - 7*m_nIconSize/6, noderect.top + m_nIconSize/6);
			CSize iconsize = CSize(m_nIconSize, m_nIconSize);
			memDC->DrawState(iconpoint, iconsize, hTaggedIcon, DST_ICON, (HBRUSH)NULL);
		}
    }
	DestroyIcon(hDeletedIcon);
	DestroyIcon(hAddedIcon);
	DestroyIcon(hAddedWithHistoryIcon);
	DestroyIcon(hReplacedIcon);
	DestroyIcon(hRenamedIcon);
	DestroyIcon(hLastCommitIcon);
	DestroyIcon(hTaggedIcon);

	DrawConnections(memDC, rect, nVScrollPos, nHScrollPos);

	if ((!bDirectDraw)&&(m_Preview.GetSafeHandle())&&(m_bShowOverview))
	{
		// draw the overview image rectangle in the top right corner
		CMemDC memDC2(memDC, true);
		memDC2.SetWindowOrg(0, 0);
		HBITMAP oldhbm = (HBITMAP)memDC2.SelectObject(&m_Preview);
		memDC->BitBlt(rect.Width()-m_previewWidth, 0, m_previewWidth, m_previewHeight, 
			&memDC2, 0, 0, SRCCOPY);
		memDC2.SelectObject(oldhbm);
		// draw the border for the overview rectangle
		m_OverviewRect.left = rect.Width()-m_previewWidth;
		m_OverviewRect.top = 0;
		m_OverviewRect.right = rect.Width();
		m_OverviewRect.bottom = m_previewHeight;
		memDC->DrawEdge(&m_OverviewRect, EDGE_BUMP, BF_RECT);
		// now draw a rectangle where the current view is located in the overview
		LONG width = m_previewWidth * rect.Width() / m_ViewRect.Width();
		LONG height = m_previewHeight * rect.Height() / m_ViewRect.Height();
		LONG xpos = nHScrollPos * m_previewWidth / m_ViewRect.Width();
		LONG ypos = nVScrollPos * m_previewHeight / m_ViewRect.Height();
		RECT tempRect;
		tempRect.left = rect.Width()-m_previewWidth+xpos;
		tempRect.top = ypos;
		tempRect.right = tempRect.left + width;
		tempRect.bottom = tempRect.top + height;
		// make sure the position rect is not bigger than the preview window itself
		::IntersectRect(&m_OverviewPosRect, &m_OverviewRect, &tempRect);
		memDC->SetROP2(R2_MASKPEN);
		HGDIOBJ oldbrush = memDC->SelectObject(GetStockObject(GRAY_BRUSH));
		memDC->Rectangle(&m_OverviewPosRect);
		memDC->SetROP2(R2_NOT);
		memDC->SelectObject(oldbrush);
		memDC->DrawEdge(&m_OverviewPosRect, EDGE_BUMP, BF_RECT);
	}

	if (!bDirectDraw)
		delete memDC;
}

void CRevisionGraphWnd::DrawConnections(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos)
{
	CRect viewrect;
	viewrect.top = rect.top + nVScrollPos;
	viewrect.bottom = rect.bottom + nVScrollPos;
	viewrect.left = rect.left + nHScrollPos;
	viewrect.right = rect.right + nHScrollPos;

	CPen newpen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
	CPen * pOldPen = pDC->SelectObject(&newpen);

	for (size_t i = 0, count = m_arConnections.size(); i < count; ++i)
	{
		const CPoint* pt = m_arConnections[i].points;

		// skip connections that are definitely out of view

		if (   (max (pt[0].x, pt[3].x) < viewrect.left) 
			|| (min (pt[0].x, pt[3].x) > viewrect.right) 
			|| (max (pt[0].y, pt[3].y) < viewrect.top) 
			|| (min (pt[0].y, pt[3].y) > viewrect.bottom))
		{
			continue;
		}

		// correct the scroll offset

		CPoint p[4];
		p[0].x = pt[0].x - nHScrollPos;
		p[0].y = pt[0].y - nVScrollPos;
		p[1].x = pt[1].x - nHScrollPos;
		p[1].y = pt[1].y - nVScrollPos;
		p[2].x = pt[2].x - nHScrollPos;
		p[2].y = pt[2].y - nVScrollPos;
		p[3].x = pt[3].x - nHScrollPos;
		p[3].y = pt[3].y - nVScrollPos;

		// draw the connection

		pDC->PolyBezier (p, 4);
	}

	pDC->SelectObject(pOldPen);
}

void CRevisionGraphWnd::DrawRubberBand()
{
	CDC * pDC = GetDC();
	pDC->SetROP2(R2_NOT);
	pDC->SelectObject(GetStockObject(NULL_BRUSH));
	pDC->Rectangle(min(m_ptRubberStart.x, m_ptRubberEnd.x), min(m_ptRubberStart.y, m_ptRubberEnd.y), 
		max(m_ptRubberStart.x, m_ptRubberEnd.x), max(m_ptRubberStart.y, m_ptRubberEnd.y));
	ReleaseDC(pDC);
}

