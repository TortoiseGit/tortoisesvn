// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include "LocatorBar.h"
#include "LeftView.h"
#include "RightView.h"
#include "BottomView.h"


COLORREF CLineDiffBar::m_BinDiffColors [8] = { 
	RGB(0x00, 0x99, 0x00),
	RGB(0x00, 0xab, 0x00),
	RGB(0x00, 0xbd, 0x00),
	RGB(0x00, 0xcf, 0x00),
	RGB(0x00, 0xe1, 0x00),
	RGB(0x50, 0xf3, 0x50),
	RGB(0xa0, 0xff, 0xa0),
	RGB(0xd8, 0xff, 0xd8),
};

IMPLEMENT_DYNAMIC(CLineDiffBar, CDialogBar)
CLineDiffBar::CLineDiffBar()
{
	m_pMainFrm = NULL;
	m_pCacheBitmap = NULL;
	m_nLineIndex = -1;
	m_nLineHeight = 0;

	m_bBinaryDiff = TRUE;
}

CLineDiffBar::~CLineDiffBar()
{
	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	} // if (m_pCacheBitmap) 
}

BEGIN_MESSAGE_MAP(CLineDiffBar, CDialogBar)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CLineDiffBar::DocumentUpdated()
{
	//resize according to the fontsize
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
	{
		m_nLineHeight = m_pMainFrm->m_pwndLeftView->GetLineHeight();
	} // if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
	CRect rect;
	GetClientRect(rect);
	if (m_pMainFrm)
		m_pMainFrm->RecalcLayout();
	m_nIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
	m_bBinaryDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
}

CSize CLineDiffBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CSize size;
	if (bStretch) // if not docked stretch to fit
	{
		size.cx = bHorz ? 32767 : m_sizeDefault.cx;
		size.cy = bHorz ? 2*m_nLineHeight : 32767;
		BOOL bDiffBar = CRegDWORD(_T("Software\\TortoiseMerge\\DiffBar"), TRUE);
		if (!bDiffBar)
			size.cy = 0;
		if (m_pMainFrm)
		{
			// hide the line bar if either the right view is hidden (one pane view)
			// or the bottom view is not hidden (three way merge)
			// hiding is done by setting the height of the pane to zero
			if ((m_pMainFrm->m_pwndRightView)&&(m_pMainFrm->m_pwndRightView->IsHidden()))
				size.cy = 0;
			if ((m_pMainFrm->m_pwndBottomView)&&(!m_pMainFrm->m_pwndBottomView->IsHidden()))
				size.cy = 0;
		} // if (m_pMainFrm) 

		if (size.cy > 0)
		{
			// Convert client to window sizes
			CRect rc(CPoint(0, 0), size);
			AdjustWindowRectEx(&rc, GetStyle(), FALSE, GetExStyle());
			size = rc.Size();
		}
	} // if (bStretch) // if not docked stretch to fit 
	else
	{
		size = m_sizeDefault;
	}
	return size;
}

void CLineDiffBar::ShowLines(int nLineIndex)
{
	m_nLineIndex = nLineIndex;
	Invalidate();
}

void CLineDiffBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	int height = rect.Height();
	int width = rect.Width();

	CDC cacheDC;
	VERIFY(cacheDC.CreateCompatibleDC(&dc));
	cacheDC.FillSolidRect(&rect, ::GetSysColor(COLOR_WINDOW));
	if (m_pCacheBitmap == NULL)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(&dc, width, height));
	}
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

	CRect upperrect = CRect(rect.left, rect.top, rect.right, rect.bottom/2);
	CRect lowerrect = CRect(rect.left, rect.bottom/2, rect.right, rect.bottom);

	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView)&&(m_pMainFrm->m_pwndRightView))
	{
		if ((m_pMainFrm->m_pwndLeftView->IsWindowVisible())&&(m_pMainFrm->m_pwndRightView->IsWindowVisible()))
		{
			BOOL bViewWhiteSpace = m_pMainFrm->m_pwndLeftView->m_bViewWhitespace;
			if ((1 != m_nIgnoreWS)||(bViewWhiteSpace))
			{
				m_pMainFrm->m_pwndLeftView->m_bViewWhitespace = TRUE;
				m_pMainFrm->m_pwndRightView->m_bViewWhitespace = TRUE;
			}
			if (m_bBinaryDiff) {
				DrawInlineDiff(cacheDC, &upperrect, &lowerrect, m_nLineIndex);
			} else {
				// Use left and right view to display lines next to each other
				m_pMainFrm->m_pwndLeftView->DrawSingleLine(&cacheDC, &upperrect, m_nLineIndex);
				m_pMainFrm->m_pwndRightView->DrawSingleLine(&cacheDC, &lowerrect, m_nLineIndex);
			}
			m_pMainFrm->m_pwndLeftView->m_bViewWhitespace = bViewWhiteSpace;
			m_pMainFrm->m_pwndRightView->m_bViewWhitespace = bViewWhiteSpace;
		}
	} 

	VERIFY(dc.BitBlt(rect.left, rect.top, width, height, &cacheDC, 0, 0, SRCCOPY));

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}


const TCHAR *CLineDiffBar::Search(const TCHAR *wholestring, size_t wholestringlen, 
								  const TCHAR *longeststring, size_t longeststringlen, 
								  size_t *max) 
{
	const TCHAR *start, *s, *n, *maxp;
	if (wholestringlen == -1)
		wholestringlen = _tcslen(wholestring);
	if (longeststringlen == -1)
		longeststringlen = _tcslen(longeststring);

	maxp=0;
	*max=0;
	for (start=wholestring; start<=wholestring+wholestringlen; start++)
	{
		// if the string doesn't match, try again
		if (*start != *longeststring)
			continue;

		s = start;
		n = longeststring;
		while (*s==*n && *s != '\0' && *n != '\0')
		{
			s++;
			n++;
		}
		if (*max<(size_t)(s-start))
		{
			*max=s-start;
			maxp=start;
		}
		if ((size_t)(n-longeststring) >=longeststringlen)
			break;
	}
	return(maxp);
}


#define DIFF_COPY   1
#define DIFF_INSERT 2

void CLineDiffBar::InLineDiff(CDWordArray & result, CString & base, CString & your) 
{
	const TCHAR * match = NULL;
	size_t match_length = 0;
	BOOL bInsert = FALSE;
	for (size_t i = 0; i < (size_t)your.GetLength(); i += match_length) 
	{
		match = Search(base.GetBuffer(), base.GetLength(), 
			           your.GetBuffer() + i, your.GetLength() - i, &match_length);
		if (match == NULL || match_length <= 5) 
		{
			match_length = 1;
			if (!bInsert)
			{
				result.Add(DIFF_INSERT);
				result.Add(your.GetLength() - i);
				result.Add(match_length);
				bInsert = TRUE;
			} 
			else
			{
				result[result.GetSize() - 1] += match_length;
			}
		}
		else
		{
			bInsert = FALSE;
			result.Add(DIFF_COPY);
			result.Add(match - base.GetBuffer());
			result.Add(match_length);
		}
		ASSERT(match_length > 0);
	}
}


void CLineDiffBar::DrawInlineDiff(CDC &dc, const CRect *upperrect, const CRect *lowerrect, int line)
{
	ASSERT(m_pMainFrm);
	ASSERT(m_pMainFrm->m_pwndLeftView);
	ASSERT(m_pMainFrm->m_pwndRightView);
	// Only call this function in two-way diff mode.

	// Fill Area completely
	COLORREF bkGnd, crText;
	m_pMainFrm->m_Data.GetColors(CDiffData::DIFFSTATE_UNKNOWN, bkGnd, crText);
	dc.FillSolidRect(upperrect, bkGnd);
	dc.FillSolidRect(lowerrect, bkGnd);

	if ((m_pMainFrm->m_pwndLeftView->m_arLineStates == 0)||(m_pMainFrm->m_pwndRightView->m_arLineStates == 0))
		return;
	// Do nothing, if line was removed or added.
	if (line < 0 || 
		line >= m_pMainFrm->m_pwndLeftView->m_arLineStates->GetCount() ||
		line >= m_pMainFrm->m_pwndRightView->m_arLineStates->GetCount() ||
		CDiffData::DIFFSTATE_UNKNOWN == m_pMainFrm->m_pwndRightView->m_arLineStates->GetAt(line) ||
		CDiffData::DIFFSTATE_UNKNOWN == m_pMainFrm->m_pwndLeftView->m_arLineStates->GetAt(line)) 
	{
		return;
	}

	// Retrieve Expanded lines.
	CString lline;
	CString rline;
	LPCTSTR ptstr;
	ptstr = m_pMainFrm->m_pwndLeftView->GetLineChars(line);
	m_pMainFrm->m_pwndLeftView->ExpandChars(ptstr, 0, m_pMainFrm->m_pwndLeftView->GetLineLength(line), lline);
	ptstr = m_pMainFrm->m_pwndRightView->GetLineChars(line);
    m_pMainFrm->m_pwndRightView->ExpandChars(ptstr, 0, m_pMainFrm->m_pwndRightView->GetLineLength(line), rline);

	// Determine binary diff operations
	CDWordArray inlinediff;
	inlinediff.SetSize(0, 30); // Prohibit excessive reallocation (grow for 10 operations each).
	InLineDiff(inlinediff, lline, rline);

	// Prepare for drawing
	dc.SelectObject(m_pMainFrm->m_pwndLeftView->GetFont(FALSE, FALSE, FALSE));
	COLORREF r_col, r_txtcol;     // Deleted
	m_pMainFrm->m_Data.GetColors(CDiffData::DIFFSTATE_REMOVED,  r_col, r_txtcol);
	COLORREF a_col, a_txtcol;	  // Insert
	m_pMainFrm->m_Data.GetColors(CDiffData::DIFFSTATE_ADDED,  a_col, a_txtcol);
	COLORREF col, txtcol;         // Normal color
	m_pMainFrm->m_Data.GetColors(CDiffData::DIFFSTATE_NORMAL, col, txtcol);

	// Draw lines
	CPoint u_origin(upperrect->left - m_pMainFrm->m_pwndLeftView->m_nOffsetChar * m_pMainFrm->m_pwndLeftView->GetCharWidth(), 
		            upperrect->top);
	CPoint l_origin(lowerrect->left - m_pMainFrm->m_pwndLeftView->m_nOffsetChar * m_pMainFrm->m_pwndLeftView->GetCharWidth(), 
		            lowerrect->top);
	dc.SetBkColor(r_col);
	dc.SetTextColor(r_txtcol);
	dc.ExtTextOut(u_origin.x, u_origin.y, ETO_CLIPPED, upperrect, lline, lline.GetLength(), NULL);
	int pos = 0;
	for (int i = 0; i < inlinediff.GetSize(); i += 3) 
	{
		if (inlinediff[i] == DIFF_COPY) 
		{
			dc.SetBkColor(m_BinDiffColors[ (i/3) % (sizeof(m_BinDiffColors)/sizeof(COLORREF)) ]);
			dc.SetTextColor(txtcol);
			dc.ExtTextOut(u_origin.x + inlinediff[i+1] * m_pMainFrm->m_pwndLeftView->GetCharWidth(), u_origin.y, ETO_CLIPPED, upperrect,
				          lline.Mid(inlinediff[i+1], inlinediff[i+2]), inlinediff[i+2], NULL);
		} 
		else 
		{
			dc.SetBkColor(a_col);
			dc.SetTextColor(a_txtcol);
		}
		dc.ExtTextOut(l_origin.x + pos * m_pMainFrm->m_pwndLeftView->GetCharWidth(), l_origin.y, ETO_CLIPPED, lowerrect, 
			          rline.Mid(pos, inlinediff[i+2]), inlinediff[i+2], NULL);
		pos += inlinediff[i+2];
	}
}

void CLineDiffBar::OnSize(UINT nType, int cx, int cy)
{
	CDialogBar::OnSize(nType, cx, cy);

	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	} // if (m_pCacheBitmap != NULL)
	Invalidate();
}

BOOL CLineDiffBar::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}


