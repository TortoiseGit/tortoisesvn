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

IMPLEMENT_DYNAMIC(CLineDiffBar, CDialogBar)
CLineDiffBar::CLineDiffBar()
{
	m_pMainFrm = NULL;
	m_pCacheBitmap = NULL;
	m_nLineIndex = -1;
	m_nLineHeight = 0;
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
	}
	CRect rect;
	GetClientRect(rect);
	if (m_pMainFrm)
		m_pMainFrm->RecalcLayout();
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
			BOOL bInlineDiffs = m_pMainFrm->m_pwndLeftView->m_bShowInlineDiff;
			int nDiffBlockStart = m_pMainFrm->m_pwndLeftView->m_nDiffBlockStart;
			int nDiffBlockEnd = m_pMainFrm->m_pwndLeftView->m_nDiffBlockEnd;
			m_pMainFrm->m_pwndLeftView->m_bViewWhitespace = TRUE;
			m_pMainFrm->m_pwndLeftView->m_bShowInlineDiff = TRUE;
			m_pMainFrm->m_pwndRightView->m_bViewWhitespace = TRUE;
			m_pMainFrm->m_pwndRightView->m_bShowInlineDiff = TRUE;
			m_pMainFrm->m_pwndLeftView->m_nDiffBlockStart = -1;
			m_pMainFrm->m_pwndLeftView->m_nDiffBlockEnd = -1;
			m_pMainFrm->m_pwndRightView->m_nDiffBlockStart = -1;
			m_pMainFrm->m_pwndRightView->m_nDiffBlockEnd = -1;
			// Use left and right view to display lines next to each other
			m_pMainFrm->m_pwndLeftView->DrawSingleLine(&cacheDC, &upperrect, m_nLineIndex);
			m_pMainFrm->m_pwndRightView->DrawSingleLine(&cacheDC, &lowerrect, m_nLineIndex);
			m_pMainFrm->m_pwndLeftView->m_bViewWhitespace = bViewWhiteSpace;
			m_pMainFrm->m_pwndLeftView->m_bShowInlineDiff = bInlineDiffs;
			m_pMainFrm->m_pwndRightView->m_bViewWhitespace = bViewWhiteSpace;
			m_pMainFrm->m_pwndRightView->m_bShowInlineDiff = bInlineDiffs;
			m_pMainFrm->m_pwndLeftView->m_nDiffBlockStart = nDiffBlockStart;
			m_pMainFrm->m_pwndLeftView->m_nDiffBlockEnd = nDiffBlockEnd;
			m_pMainFrm->m_pwndRightView->m_nDiffBlockStart = nDiffBlockStart;
			m_pMainFrm->m_pwndRightView->m_nDiffBlockEnd = nDiffBlockEnd;
		}
	} 

	VERIFY(dc.BitBlt(rect.left, rect.top, width, height, &cacheDC, 0, 0, SRCCOPY));

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
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


