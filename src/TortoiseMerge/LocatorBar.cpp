// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004 - Stefan Kueng

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
#include ".\locatorbar.h"


IMPLEMENT_DYNAMIC(CLocatorBar, CDialogBar)
CLocatorBar::CLocatorBar()
{
	m_pMainFrm = NULL;
	m_pCacheBitmap = NULL;
}

CLocatorBar::~CLocatorBar()
{
	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	} // if (m_pCacheBitmap) 
}

BEGIN_MESSAGE_MAP(CLocatorBar, CDialogBar)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

void CLocatorBar::DocumentUpdated()
{
	m_pMainFrm = (CMainFrame *)this->GetParentFrame();
	m_arLeft.RemoveAll();
	m_arRight.RemoveAll();
	m_arBottom.RemoveAll();
	DWORD state = 0;
	int identcount = 1;
	if (m_pMainFrm->m_pwndLeftView->m_arLineStates)
	{
		if (m_pMainFrm->m_pwndLeftView->m_arLineStates->GetCount())
			state = m_pMainFrm->m_pwndLeftView->m_arLineStates->GetAt(0);
		for (int i=0; i<m_pMainFrm->m_pwndLeftView->m_arLineStates->GetCount(); i++)
		{
			if (state == m_pMainFrm->m_pwndLeftView->m_arLineStates->GetAt(i))
			{
				identcount++;
			}
			else
			{
				m_arLeft.Add(MAKELONG(identcount, state));
				state = m_pMainFrm->m_pwndLeftView->m_arLineStates->GetAt(i);
				identcount = 1;
			} 
		} // for (int i=0; i<m_pMainFrm->m_pwndLeftView->m_arLineStates->GetCount(); i++) 
		m_arLeft.Add(MAKELONG(identcount, state));
	}

	if (m_pMainFrm->m_pwndRightView->m_arLineStates)
	{
		if (m_pMainFrm->m_pwndRightView->m_arLineStates->GetCount())
			state = m_pMainFrm->m_pwndRightView->m_arLineStates->GetAt(0);
		identcount = 1;
		for (int i=0; i<m_pMainFrm->m_pwndRightView->m_arLineStates->GetCount(); i++)
		{
			if (state == m_pMainFrm->m_pwndRightView->m_arLineStates->GetAt(i))
			{
				identcount++;
			}
			else
			{
				m_arRight.Add(MAKELONG(identcount, state));
				state = m_pMainFrm->m_pwndRightView->m_arLineStates->GetAt(i);
				identcount = 1;
			}
		} // for (int i=0; i<m_pMainFrm->m_pwndRightView->m_arLineStates->GetCount(); i++) 
		m_arRight.Add(MAKELONG(identcount, state));
	}

	if ((m_pMainFrm->m_pwndBottomView->m_arLineStates)&&(m_pMainFrm->m_pwndBottomView->m_arLineStates->GetCount()))
	{
		state = m_pMainFrm->m_pwndBottomView->m_arLineStates->GetAt(0);
		identcount = 1;
		for (int i=0; i<m_pMainFrm->m_pwndBottomView->m_arLineStates->GetCount(); i++)
		{
			if (state == m_pMainFrm->m_pwndBottomView->m_arLineStates->GetAt(i))
			{
				identcount++;
			}
			else
			{
				m_arBottom.Add(MAKELONG(identcount, state));
				state = m_pMainFrm->m_pwndBottomView->m_arLineStates->GetAt(i);
				identcount = 1;
			}
		} // for (int i=0; i<m_pMainFrm->m_pwndBottomView->m_arLineStates->GetCount(); i++) 
		m_arBottom.Add(MAKELONG(identcount, state));
		m_nLines = (int)max(m_pMainFrm->m_pwndBottomView->m_arLineStates->GetCount(), m_pMainFrm->m_pwndRightView->m_arLineStates->GetCount());
	} // if ((m_pMainFrm->m_pwndBottomView->m_arLineStates)&&(m_pMainFrm->m_pwndBottomView->m_arLineStates->GetCount()))
	else if (m_pMainFrm->m_pwndRightView->m_arLineStates)
		m_nLines = (int)max(0, m_pMainFrm->m_pwndRightView->m_arLineStates->GetCount());

	if (m_pMainFrm->m_pwndLeftView->m_arLineStates)
		m_nLines = (int)max(m_nLines, m_pMainFrm->m_pwndLeftView->m_arLineStates->GetCount());
	else
		m_nLines = 0;
	m_nLines++;
	Invalidate();
}

void CLocatorBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect(rect);
	int nTopLine = 0;
	int nBottomLine = 0;
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
	{
		nTopLine = m_pMainFrm->m_pwndLeftView->m_nTopLine;
		nBottomLine = nTopLine + m_pMainFrm->m_pwndLeftView->GetScreenLines();
	}
	CDC cacheDC;
	VERIFY(cacheDC.CreateCompatibleDC(&dc));

	if (m_pCacheBitmap == NULL)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(&dc, rect.Width(), rect.Height()));
	}
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);


	COLORREF color, color2;
	
	m_pMainFrm->m_Data.GetColors(CDiffData::DIFFSTATE_UNKNOWN, color, color2);
	cacheDC.FillSolidRect(rect, color);
	
	int barwidth = (rect.Width()/3);
	DWORD state = 0;
	int identcount = 0;
	int linecount = 0;
	
	if (m_pMainFrm->m_pwndLeftView->IsWindowVisible())
	{
		for (int i=0; i<m_arLeft.GetCount(); i++)
		{
			identcount = LOWORD(m_arLeft.GetAt(i));
			state = HIWORD(m_arLeft.GetAt(i));
			COLORREF color, color2;
			m_pMainFrm->m_Data.GetColors((CDiffData::DiffStates)state, color, color2);
			if ((CDiffData::DiffStates)state != CDiffData::DIFFSTATE_NORMAL)
				cacheDC.FillSolidRect(rect.left, rect.Height()*linecount/m_nLines, 
							barwidth, max(rect.Height()*identcount/m_nLines,1), color);
			linecount += identcount;
		} // for (int i=0; i<m_arLeft.GetCount(); i++) 
	} // if (m_pMainFrm->m_pwndLeftView->IsWindowVisible()) 
	state = 0;
	identcount = 0;
	linecount = 0;

	if (m_pMainFrm->m_pwndRightView->IsWindowVisible())
	{
		for (int i=0; i<m_arRight.GetCount(); i++)
		{
			identcount = LOWORD(m_arRight.GetAt(i));
			state = HIWORD(m_arRight.GetAt(i));
			COLORREF color, color2;
			m_pMainFrm->m_Data.GetColors((CDiffData::DiffStates)state, color, color2);
			if ((CDiffData::DiffStates)state != CDiffData::DIFFSTATE_NORMAL)
				cacheDC.FillSolidRect(rect.left + (rect.Width()*2/3), rect.Height()*linecount/m_nLines, 
							barwidth, max(rect.Height()*identcount/m_nLines,1), color);
			linecount += identcount;
		} // for (int i=0; i<m_arLeft.GetCount(); i++) 
	} // if (m_pMainFrm->m_pwndRightView->IsWindowVisible()) 
	state = 0;
	identcount = 0;
	linecount = 0;
	if (m_pMainFrm->m_pwndBottomView->IsWindowVisible())
	{
		for (int i=0; i<m_arBottom.GetCount(); i++)
		{
			identcount = LOWORD(m_arBottom.GetAt(i));
			state = HIWORD(m_arBottom.GetAt(i));
			COLORREF color, color2;
			m_pMainFrm->m_Data.GetColors((CDiffData::DiffStates)state, color, color2);
			if ((CDiffData::DiffStates)state != CDiffData::DIFFSTATE_NORMAL)
				cacheDC.FillSolidRect(rect.left + (rect.Width()/3), rect.Height()*linecount/m_nLines, 
							barwidth, max(rect.Height()*identcount/m_nLines,1), color);
			linecount += identcount;
		} // for (int i=0; i<m_arLeft.GetCount(); i++) 
	} // if (m_pMainFrm->m_pwndBottomView->IsWindowVisible()) 

	if (m_nLines == 0)
		m_nLines = 1;
	cacheDC.FillSolidRect(rect.left, rect.Height()*nTopLine/m_nLines,
		rect.Width(), 2, RGB(0,0,0));
	cacheDC.FillSolidRect(rect.left, rect.Height()*nBottomLine/m_nLines,
		rect.Width(), 2, RGB(0,0,0));
	//draw two vertical lines, so there are three rows visible indicating the three panes
	cacheDC.FillSolidRect(rect.left + (rect.Width()/3), rect.top, 1, rect.Height(), RGB(0,0,0));
	cacheDC.FillSolidRect(rect.left + (rect.Width()*2/3), rect.top, 1, rect.Height(), RGB(0,0,0));

	VERIFY(dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &cacheDC, 0, 0, SRCCOPY));

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

void CLocatorBar::OnSize(UINT nType, int cx, int cy)
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

BOOL CLocatorBar::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CLocatorBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(rect); 
	int nLine = point.y*m_nLines/rect.Height();

	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndBottomView))
	{
		nLine = nLine - (m_pMainFrm->m_pwndBottomView->GetScreenLines()/2);
	}

	if (nLine < 0)
		nLine = 0;
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndBottomView))
		m_pMainFrm->m_pwndBottomView->ScrollToLine(nLine);
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
		m_pMainFrm->m_pwndLeftView->ScrollToLine(nLine);
	if ((m_pMainFrm)&&(m_pMainFrm->m_pwndRightView))
		m_pMainFrm->m_pwndRightView->ScrollToLine(nLine);
	Invalidate();
	CDialogBar::OnLButtonDown(nFlags, point);
}

void CLocatorBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON)
	{
		CRect rect;
		GetClientRect(rect); 
		int nLine = point.y*m_nLines/rect.Height();

		if ((m_pMainFrm)&&(m_pMainFrm->m_pwndBottomView))
		{
			nLine = nLine - (m_pMainFrm->m_pwndBottomView->GetScreenLines()/2);
		}

		if (nLine < 0)
			nLine = 0;
		if ((m_pMainFrm)&&(m_pMainFrm->m_pwndBottomView))
			m_pMainFrm->m_pwndBottomView->ScrollToLine(nLine);
		if ((m_pMainFrm)&&(m_pMainFrm->m_pwndLeftView))
			m_pMainFrm->m_pwndLeftView->ScrollToLine(nLine);
		if ((m_pMainFrm)&&(m_pMainFrm->m_pwndRightView))
			m_pMainFrm->m_pwndRightView->ScrollToLine(nLine);
		Invalidate();
	}
}




