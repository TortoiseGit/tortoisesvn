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
#include "registry.h"
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include ".\BaseView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MARGINWIDTH 20
#define HEADERHEIGHT 10

#define TAB_CHARACTER				_T('\xBB')
#define SPACE_CHARACTER				_T('\xB7')

#define MAXFONTS 8

CBaseView * CBaseView::m_pwndLeft = NULL;
CBaseView * CBaseView::m_pwndRight = NULL;
CBaseView * CBaseView::m_pwndBottom = NULL;
CLocatorBar * CBaseView::m_pwndLocator = NULL;
CStatusBar * CBaseView::m_pwndStatusBar = NULL;
CMainFrame * CBaseView::m_pMainFrame = NULL;

CBaseView::CBaseView()
{
	m_pCacheBitmap = NULL;
	m_arDiffLines = NULL;
	m_arLineStates = NULL;
	m_nLineHeight = -1;
	m_nCharWidth = -1;
	m_nScreenChars = -1;
	m_nMaxLineLength = -1;
	m_nScreenLines = -1;
	m_nTopLine = 0;
	m_nOffsetChar = 0;
	m_bViewWhitespace = FALSE;
	m_nSelBlockStart = -1;
	m_nSelBlockEnd = -1;
	m_bModified = FALSE;
	m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	for (int i=0; i<MAXFONTS; i++)
	{
		m_apFonts[i] = NULL;
	}
	m_hConflictedIcon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CONFLICTEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hRemovedIcon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_REMOVEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hAddedIcon = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ADDEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
}

CBaseView::~CBaseView()
{
	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
	} // if (m_pCacheBitmap) 
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		} // if (m_apFonts[i] != NULL) 
		m_apFonts[i] = NULL;
	} // for (int i=0; i<MAXFONTS; i++)
}

BEGIN_MESSAGE_MAP(CBaseView, CView)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_MERGE_NEXTDIFFERENCE, OnMergeNextdifference)
	ON_COMMAND(ID_MERGE_PREVIOUSDIFFERENCE, OnMergePreviousdifference)
END_MESSAGE_MAP()


void CBaseView::DocumentUpdated()
{
	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	} // if (m_pCacheBitmap != NULL) 
	m_nLineHeight = -1;
	m_nCharWidth = -1;
	m_nScreenChars = -1;
	m_nMaxLineLength = -1;
	m_nScreenLines = -1;
	m_nTopLine = 0;
	m_bModified = FALSE;
	m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		} // if (m_apFonts[i] != NULL)  
		m_apFonts[i] = NULL;
	} // for (int i=0; i<MAXFONTS; i++) 
	m_nSelBlockStart = -1;
	m_nSelBlockEnd = -1;
	RecalcVertScrollBar();
	RecalcHorzScrollBar();
	UpdateStatusBar();
	Invalidate();
}

void CBaseView::UpdateStatusBar()
{
	int nRemovedLines = 0;
	int nAddedLines = 0;
	int nConflictedLines = 0;

	if (m_arLineStates)
	{
		for (int i=0; i<m_arLineStates->GetCount(); i++)
		{
			CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(i);
			switch (state)
			{
			case CDiffData::DIFFSTATE_ADDED:
			case CDiffData::DIFFSTATE_ADDEDWHITESPACE:
			case CDiffData::DIFFSTATE_IDENTICALADDED:
			case CDiffData::DIFFSTATE_THEIRSADDED:
			case CDiffData::DIFFSTATE_YOURSADDED:
			case CDiffData::DIFFSTATE_CONFLICTADDED:
				nAddedLines++;
				break;
			case CDiffData::DIFFSTATE_IDENTICALREMOVED:
			case CDiffData::DIFFSTATE_REMOVED:
			case CDiffData::DIFFSTATE_REMOVEDWHITESPACE:
			case CDiffData::DIFFSTATE_THEIRSREMOVED:
			case CDiffData::DIFFSTATE_YOURSREMOVED:
				nRemovedLines++;
				break;
			case CDiffData::DIFFSTATE_CONFLICTED:
				nConflictedLines++;
				break;
			} // switch (state) 
		} // for (int i=0; i<m_arLineStates->GetCount(); i++) 
	}

	CString sBarText;
	CString sTemp;

	if (nRemovedLines)
	{
		sTemp.Format(IDS_STATUSBAR_REMOVEDLINES, nRemovedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	} // if (nRemovedLines) 
	if (nAddedLines)
	{
		sTemp.Format(IDS_STATUSBAR_ADDEDLINES, nAddedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	} // if (nRemovedLines) 
	if (nConflictedLines)
	{
		sTemp.Format(IDS_STATUSBAR_CONFLICTEDLINES, nConflictedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	} // if (nRemovedLines) 
	if (m_pwndStatusBar)
	{
		UINT nID;
		UINT nStyle;
		int cxWidth;
		int nIndex = m_pwndStatusBar->CommandToIndex(m_nStatusBarID);
		m_pwndStatusBar->GetPaneInfo(nIndex, nID, nStyle, cxWidth);
		//calculate the width of the text
		CSize size = m_pwndStatusBar->GetDC()->GetTextExtent(sBarText);
		m_pwndStatusBar->SetPaneInfo(nIndex, nID, nStyle, size.cx+2);
		m_pwndStatusBar->SetPaneText(nIndex, sBarText);
	}
}

BOOL CBaseView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CView::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	CWnd *pParentWnd = CWnd::FromHandlePermanent(cs.hwndParent);
	if (pParentWnd == NULL || ! pParentWnd->IsKindOf(RUNTIME_CLASS(CSplitterWnd)))
	{
		//	View must always create its own scrollbars,
		//	if only it's not used within splitter
		cs.style |= (WS_HSCROLL | WS_VSCROLL);
	} // if (pParentWnd == NULL || ! pParentWnd->IsKindOf(RUNTIME_CLASS(CSplitterWnd))) 
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS);
	return TRUE;
}

CFont* CBaseView::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/, BOOL bStrikeOut /*= FALSE*/)
{
	int nIndex = 0;
	if (bBold)
		nIndex |= 1;
	if (bItalic)
		nIndex |= 2;
	if (bStrikeOut)
		nIndex |= 4;
	if (m_apFonts[nIndex] == NULL)
	{
		m_apFonts[nIndex] = new CFont;
		m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		m_lfBaseFont.lfItalic = (BYTE) bItalic;
		m_lfBaseFont.lfStrikeOut = (BYTE) bStrikeOut;
		if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
		{
			delete m_apFonts[nIndex];
			m_apFonts[nIndex] = NULL;
			return CView::GetFont();
		} // if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont)) 
	} // if (m_apFonts[nIndex] == NULL) 
	return m_apFonts[nIndex];
}

void CBaseView::CalcLineCharDim()
{
	CDC *pDC = GetDC();
	CFont *pOldFont = pDC->SelectObject(GetFont());
	CSize szCharExt = pDC->GetTextExtent(_T("X"));
	m_nLineHeight = szCharExt.cy;
	if (m_nLineHeight < 1)
		m_nLineHeight = 1;
	m_nCharWidth = szCharExt.cx;

	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);
}

int CBaseView::GetScreenChars()
{
	if (m_nScreenChars == -1)
	{
		CRect rect;
		GetClientRect(&rect);
		m_nScreenChars = (rect.Width() - MARGINWIDTH) / GetCharWidth();
	} // if (m_nScreenChars == -1) 
	return m_nScreenChars;
}

int CBaseView::GetLineHeight()
{
	if (m_nLineHeight == -1)
		CalcLineCharDim();
	return m_nLineHeight;
}

int CBaseView::GetCharWidth()
{
	if (m_nCharWidth == -1)
		CalcLineCharDim();
	return m_nCharWidth;
}

int CBaseView::GetMaxLineLength()
{
	if (m_nMaxLineLength == -1)
	{
		m_nMaxLineLength = 0;
		int nLineCount = GetLineCount();
		for (int i=0; i<nLineCount; i++)
		{
			int nActualLength = GetLineActualLength(i);
			if (m_nMaxLineLength < nActualLength)
				m_nMaxLineLength = nActualLength;
		} // for (int i=0; i<nLineCount; i++) 
	} // if (m_nMaxLineLength == -1) 
	return m_nMaxLineLength;
}

int CBaseView::GetLineActualLength(int index)
{
	if (m_arDiffLines == NULL)
		return 0;
	int nLineLength = m_arDiffLines->GetAt(index).GetLength();
	ASSERT(nLineLength >= 0);
	return nLineLength;
}

int CBaseView::GetLineLength(int index)
{
	if (m_arDiffLines == NULL)
		return 0;
	int nLineLength = m_arDiffLines->GetAt(index).GetLength();
	ASSERT(nLineLength >= 0);
	return nLineLength;
}

int CBaseView::GetLineCount()
{
	if (m_arDiffLines == NULL)
		return 1;
	int nLineCount = (int)m_arDiffLines->GetCount();
	ASSERT(nLineCount >= 0);
	return nLineCount;
}

LPCTSTR CBaseView::GetLineChars(int index)
{
	if (m_arDiffLines == NULL)
		return 0;
	return m_arDiffLines->GetAt(index);
}

int CBaseView::GetScreenLines()
{
	if (m_nScreenLines == -1)
	{
		CRect rect;
		GetClientRect(&rect);
		m_nScreenLines = rect.Height() / GetLineHeight();
	} // if (m_nScreenLines == -1) 
	return m_nScreenLines;
}

void CBaseView::RecalcVertScrollBar(BOOL bPositionOnly /*= FALSE*/)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	if (bPositionOnly)
	{
		si.fMask = SIF_POS;
		si.nPos = m_nTopLine;
	} // if (bPositionOnly) 
	else
	{
		if (GetScreenLines() >= GetLineCount() && m_nTopLine > 0)
		{
			m_nTopLine = 0;
			Invalidate();
			//UpdateCaret();
		} // if (GetScreenLines() >= GetLineCount() && m_nTopLine > 0) 
		si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = GetLineCount() - 1;
		si.nPage = GetScreenLines();
		si.nPos = m_nTopLine;
	}
	VERIFY(SetScrollInfo(SB_VERT, &si));
}

void CBaseView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (m_pwndLeft)
		m_pwndLeft->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndRight)
		m_pwndRight->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndBottom)
		m_pwndBottom->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::OnDoVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar, CBaseView * master)
{
	//	Note we cannot use nPos because of its 16-bit nature
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	VERIFY(master->GetScrollInfo(SB_VERT, &si));

	int nPageLines = GetScreenLines();
	int nLineCount = GetLineCount();

	int nNewTopLine;
	switch (nSBCode)
	{
	case SB_TOP:
		nNewTopLine = 0;
		break;
	case SB_BOTTOM:
		nNewTopLine = nLineCount - nPageLines + 1;
		break;
	case SB_LINEUP:
		nNewTopLine = m_nTopLine - 1;
		break;
	case SB_LINEDOWN:
		nNewTopLine = m_nTopLine + 1;
		break;
	case SB_PAGEUP:
		nNewTopLine = m_nTopLine - si.nPage + 1;
		break;
	case SB_PAGEDOWN:
		nNewTopLine = m_nTopLine + si.nPage - 1;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewTopLine = si.nTrackPos;
		break;
	default:
		return;
	} // switch (nSBCode) 

	if (nNewTopLine < 0)
		nNewTopLine = 0;
	if (nNewTopLine >= nLineCount)
		nNewTopLine = nLineCount - 1;
	ScrollToLine(nNewTopLine);
}

void CBaseView::RecalcHorzScrollBar(BOOL bPositionOnly /*= FALSE*/)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	if (bPositionOnly)
	{
		si.fMask = SIF_POS;
		si.nPos = m_nOffsetChar;
	} // if (bPositionOnly) 
	else
	{
		if (GetScreenChars() >= GetMaxLineLength() && m_nOffsetChar > 0)
		{
			m_nOffsetChar = 0;
			Invalidate();
			//UpdateCaret();
		} // if (GetScreenChars() >= GetMaxLineLength() && m_nOffsetChar > 0) 
		si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = GetMaxLineLength() - 1;
		si.nPage = GetScreenChars();
		si.nPos = m_nOffsetChar;
	}
	VERIFY(SetScrollInfo(SB_HORZ, &si));
}

void CBaseView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CView::OnHScroll(nSBCode, nPos, pScrollBar);
	if (m_pwndLeft)
		m_pwndLeft->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndRight)
		m_pwndRight->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndBottom)
		m_pwndBottom->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}
void CBaseView::OnDoHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar, CBaseView * master) 
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	VERIFY(master->GetScrollInfo(SB_HORZ, &si));

	int nPageChars = GetScreenChars();
	int nMaxLineLength = GetMaxLineLength();

	int nNewOffset;
	switch (nSBCode)
	{
	case SB_LEFT:
		nNewOffset = 0;
		break;
	case SB_BOTTOM:
		nNewOffset = nMaxLineLength - nPageChars + 1;
		break;
	case SB_LINEUP:
		nNewOffset = m_nOffsetChar - 1;
		break;
	case SB_LINEDOWN:
		nNewOffset = m_nOffsetChar + 1;
		break;
	case SB_PAGEUP:
		nNewOffset = m_nOffsetChar - si.nPage + 1;
		break;
	case SB_PAGEDOWN:
		nNewOffset = m_nOffsetChar + si.nPage - 1;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewOffset = si.nTrackPos;
		break;
	default:
		return;
	} // switch (nSBCode) 

	if (nNewOffset >= nMaxLineLength)
		nNewOffset = nMaxLineLength - 1;
	if (nNewOffset < 0)
		nNewOffset = 0;
	ScrollToChar(nNewOffset, TRUE);
	//UpdateCaret();
}

void CBaseView::ScrollToChar(int nNewOffsetChar, BOOL bTrackScrollBar /*= TRUE*/)
{
	if (m_nOffsetChar != nNewOffsetChar)
	{
		int nScrollChars = m_nOffsetChar - nNewOffsetChar;
		m_nOffsetChar = nNewOffsetChar;
		CRect rcScroll;
		GetClientRect(&rcScroll);
		rcScroll.left += MARGINWIDTH;
		rcScroll.top += GetLineHeight()+HEADERHEIGHT;
		ScrollWindow(nScrollChars * GetCharWidth(), 0, &rcScroll, &rcScroll);
		UpdateWindow();
		if (bTrackScrollBar)
			RecalcHorzScrollBar(TRUE);
	} // if (m_nOffsetChar != nNewOffsetChar) 
}

void CBaseView::ScrollToLine(int nNewTopLine, BOOL bTrackScrollBar /*= TRUE*/)
{
	if (m_nTopLine != nNewTopLine)
	{
		int nScrollLines = m_nTopLine - nNewTopLine;
		m_nTopLine = nNewTopLine;
		CRect rcScroll;
		GetClientRect(&rcScroll);
		rcScroll.top += GetLineHeight()+HEADERHEIGHT;
		ScrollWindow(0, nScrollLines * GetLineHeight(), &rcScroll, &rcScroll);
		UpdateWindow();
		if (bTrackScrollBar)
			RecalcVertScrollBar(TRUE);
	} // if (m_nTopLine != nNewTopLine) 
}


void CBaseView::DrawMargin(CDC *pdc, const CRect &rect, int nLineIndex)
{
	pdc->FillSolidRect(rect, ::GetSysColor(COLOR_SCROLLBAR));

	int nImageIndex = -1;
	if ((nLineIndex >= 0)&&(this->m_arLineStates)&&(this->m_arLineStates->GetCount()))
	{
		CDiffData::DiffStates state = (CDiffData::DiffStates)this->m_arLineStates->GetAt(nLineIndex);
		HICON icon = NULL;
		switch (state)
		{
		case CDiffData::DIFFSTATE_ADDED:
		case CDiffData::DIFFSTATE_ADDEDWHITESPACE:
		case CDiffData::DIFFSTATE_THEIRSADDED:
		case CDiffData::DIFFSTATE_YOURSADDED:
		case CDiffData::DIFFSTATE_IDENTICALADDED:
		case CDiffData::DIFFSTATE_CONFLICTADDED:
			icon = m_hAddedIcon;
			break;
		case CDiffData::DIFFSTATE_REMOVED:
		case CDiffData::DIFFSTATE_REMOVEDWHITESPACE:
		case CDiffData::DIFFSTATE_THEIRSREMOVED:
		case CDiffData::DIFFSTATE_YOURSREMOVED:
		case CDiffData::DIFFSTATE_IDENTICALREMOVED:
			icon = m_hRemovedIcon;
			break;
		case CDiffData::DIFFSTATE_CONFLICTED:
			icon = m_hConflictedIcon;
			break;
		default:
			break;
		} // switch (state)
		if (icon)
		{
			::DrawIconEx(pdc->m_hDC, rect.left + 2, rect.top + (rect.Height()-16)/2, icon, 16, 16, NULL, NULL, DI_NORMAL);
		}
	} // if (nLineIndex >= 0)
}

void CBaseView::OnDraw(CDC * pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);
	
	int nLineCount = GetLineCount();
	int nLineHeight = GetLineHeight();

	CDC cacheDC;
	VERIFY(cacheDC.CreateCompatibleDC(pDC));
	if (m_pCacheBitmap == NULL)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(pDC, rcClient.Width(), nLineHeight));
	} // if (m_pCacheBitmap == NULL) 
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

	CRect textrect(rcClient.left, rcClient.top, rcClient.Width(), nLineHeight+HEADERHEIGHT);
	pDC->FillSolidRect(textrect, ::GetSysColor(COLOR_SCROLLBAR));
	if (this->GetFocus() == this)
		pDC->DrawEdge(textrect, EDGE_BUMP, BF_RECT);
	else
		pDC->DrawEdge(textrect, EDGE_ETCHED, BF_RECT);

	pDC->SetBkColor(::GetSysColor(COLOR_SCROLLBAR));
	pDC->SetTextColor(RGB(200, 0, 0));
	pDC->SelectObject(GetFont(FALSE, FALSE, FALSE));
	int nStringLength = (GetCharWidth()*m_sWindowName.GetLength());
	pDC->ExtTextOut(max(rcClient.left + (rcClient.Width()-nStringLength)/2, 1), 
		rcClient.top+(HEADERHEIGHT/2), ETO_CLIPPED, textrect, m_sWindowName, NULL);
	
	CRect rcLine;
	rcLine = rcClient;
	rcLine.top += nLineHeight+HEADERHEIGHT;
	rcLine.bottom = rcLine.top + nLineHeight;
	CRect rcCacheMargin(0, 0, MARGINWIDTH, nLineHeight);
	CRect rcCacheLine(MARGINWIDTH, 0, rcLine.Width(), nLineHeight);

	int nCurrentLine = m_nTopLine;
	while (rcLine.top < rcClient.bottom)
	{
		if (nCurrentLine < nLineCount)
		{
			DrawMargin(&cacheDC, rcCacheMargin, nCurrentLine);
			DrawSingleLine(&cacheDC, rcCacheLine, nCurrentLine);
		} // if (nCurrentLine < nLineCount) 
		else
		{
			DrawMargin(&cacheDC, rcCacheMargin, -1);
			DrawSingleLine(&cacheDC, rcCacheLine, -1);
		}

		VERIFY(pDC->BitBlt(rcLine.left, rcLine.top, rcLine.Width(), rcLine.Height(), &cacheDC, 0, 0, SRCCOPY));

		nCurrentLine ++;
		rcLine.OffsetRect(0, nLineHeight);
	} // while (rcLine.top < rcClient.bottom) 

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

BOOL CBaseView::IsLineRemoved(int nLineIndex)
{
	DWORD state = 0;
	if (m_arLineStates)
		state = m_arLineStates->GetAt(nLineIndex);
	BOOL ret = FALSE;
	switch (state)
	{
	case CDiffData::DIFFSTATE_REMOVED:
	case CDiffData::DIFFSTATE_REMOVEDWHITESPACE:
	case CDiffData::DIFFSTATE_THEIRSREMOVED:
	case CDiffData::DIFFSTATE_YOURSREMOVED:
	case CDiffData::DIFFSTATE_IDENTICALREMOVED:
		ret = TRUE;
		break;
	default:
		ret = FALSE;
		break;
	} // switch (state)
	return ret;
}

void CBaseView::DrawSingleLine(CDC *pDC, const CRect &rc, int nLineIndex)
{
	ASSERT(nLineIndex >= -1 && nLineIndex < GetLineCount());

	if (nLineIndex == -1)
	{
		// Draw line beyond the text
		COLORREF bkGnd, crText;
		m_pMainFrame->m_Data.GetColors(CDiffData::DIFFSTATE_UNKNOWN, bkGnd, crText);
		pDC->FillSolidRect(rc, bkGnd);
		return;
	} // if (nLineIndex == -1) 

	// Acquire the background color for the current line
	BOOL bDrawWhitespace = FALSE;
	COLORREF crBkgnd, crText;
	if ((m_arLineStates)&&(m_arLineStates->GetCount()>nLineIndex))
	{
		m_pMainFrame->m_Data.GetColors((CDiffData::DiffStates)m_arLineStates->GetAt(nLineIndex), crBkgnd, crText);
		if ((nLineIndex >= m_nSelBlockStart)&&(nLineIndex <= m_nSelBlockEnd))
		{
			crBkgnd = (~crBkgnd)&0x00FFFFFF;
		}
	}
	else
		m_pMainFrame->m_Data.GetColors(CDiffData::DIFFSTATE_UNKNOWN, crBkgnd, crText);

	int nLength = GetLineLength(nLineIndex);
	if (nLength == 0)
	{
		// Draw the empty line
		CRect rect = rc;
		//if ((m_bFocused || m_bShowInactiveSelection) && IsInsideSelBlock(CPoint(0, nLineIndex)))
		//{
		//	pDC->FillSolidRect(rect.left, rect.top, GetCharWidth(), rect.Height(), GetColor(COLORINDEX_SELBKGND));
		//	rect.left += GetCharWidth();
		//}
		pDC->FillSolidRect(rect, crBkgnd);
		return;
	} // if (nLength == 0) 

	LPCTSTR pszChars = GetLineChars(nLineIndex);

	// Draw the line
	CPoint origin(rc.left - m_nOffsetChar * GetCharWidth(), rc.top);
	pDC->SetBkColor(crBkgnd);
	pDC->SetTextColor(crText);
	BOOL bColorSet = FALSE;

	pDC->SelectObject(GetFont(FALSE, FALSE, IsLineRemoved(nLineIndex)));
	if (nLength > 0)
	{
		CString line;
		ExpandChars(pszChars, 0, nLength, line);
		int nWidth = rc.right - origin.x;
		if (nWidth > 0)
		{
			int nCharWidth = GetCharWidth();
			int nCount = line.GetLength();
			int nCountFit = nWidth / nCharWidth + 1;
			if (nCount > nCountFit)
				nCount = nCountFit;

			VERIFY(pDC->ExtTextOut(origin.x, origin.y, ETO_CLIPPED, &rc, line, nCount, NULL));
		} // if (nWidth > 0) 
		origin.x += GetCharWidth() * line.GetLength();
	} // if (nLength > 0) 

	//	Draw whitespaces to the left of the text
	CRect frect = rc;
	if (origin.x > frect.left)
		frect.left = origin.x;
	if (frect.right > frect.left)
	{
		if (frect.right > frect.left)
			pDC->FillSolidRect(frect, crBkgnd);
	} // if (frect.right > frect.left) 
}

void CBaseView::ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, CString &line)
{
	if (nCount <= 0)
	{
		line = _T("");
		return;
	} // if (nCount <= 0) 

	int nTabSize = GetTabSize();

	int nActualOffset = 0;
	for (int i=0; i<nOffset; i++)
	{
		if (pszChars[i] == _T('\t'))
			nActualOffset += (nTabSize - nActualOffset % nTabSize);
		else
			nActualOffset ++;
	} // for (int i=0; i<nOffset; i++) 

	pszChars += nOffset;
	int nLength = nCount;

	int nTabCount = 0;
	for (i=0; i<nLength; i++)
	{
		if (pszChars[i] == _T('\t'))
			nTabCount ++;
	} // for (i=0; i<nLength; i++) 

	LPTSTR pszBuf = line.GetBuffer(nLength + nTabCount * (nTabSize - 1) + 1);
	int nCurPos = 0;
	if (nTabCount > 0 || m_bViewWhitespace)
	{
		for (i=0; i<nLength; i++)
		{
			if (pszChars[i] == _T('\t'))
			{
				int nSpaces = nTabSize - (nActualOffset + nCurPos) % nTabSize;
				if (m_bViewWhitespace)
				{
					pszBuf[nCurPos ++] = TAB_CHARACTER;
					nSpaces --;
				} // if (m_bViewWhitespace) 
				while (nSpaces > 0)
				{
					pszBuf[nCurPos ++] = _T(' ');
					nSpaces --;
				} // while (nSpaces > 0) 
			} // if (pszChars[I] == _T('\t')) 
			else
			{
				if (pszChars[i] == _T(' ') && m_bViewWhitespace)
					pszBuf[nCurPos] = SPACE_CHARACTER;
				else
					pszBuf[nCurPos] = pszChars[i];
				nCurPos ++;
			}
		} // for (i=0; i<Length; i++) 
	} // if (nTabCount > 0 || m_bViewWhitespace)  
	else
	{
		memcpy(pszBuf, pszChars, sizeof(TCHAR) * nLength);
		nCurPos = nLength;
	}
	pszBuf[nCurPos] = 0;
	line.ReleaseBuffer();
}

void CBaseView::ScrollAllToLine(int nNewTopLine, BOOL bTrackScrollBar)
{
	if (m_pwndLeft)
		m_pwndLeft->ScrollToLine(nNewTopLine, bTrackScrollBar);
	if (m_pwndRight)
		m_pwndRight->ScrollToLine(nNewTopLine, bTrackScrollBar);
	if (m_pwndBottom)
		m_pwndBottom->ScrollToLine(nNewTopLine, bTrackScrollBar);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

BOOL CBaseView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

int CBaseView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	memset(&m_lfBaseFont, 0, sizeof(m_lfBaseFont));
	lstrcpy(m_lfBaseFont.lfFaceName, _T("Courier New"));
	//lstrcpy(m_lfBaseFont.lfFaceName, _T("FixedSys"));
	m_lfBaseFont.lfHeight = 0;
	m_lfBaseFont.lfWeight = FW_NORMAL;
	m_lfBaseFont.lfItalic = FALSE;
	m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
	m_lfBaseFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	m_lfBaseFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	m_lfBaseFont.lfQuality = DEFAULT_QUALITY;
	m_lfBaseFont.lfPitchAndFamily = DEFAULT_PITCH;

	return 0;
}

void CBaseView::OnDestroy()
{
	CView::OnDestroy();
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
			m_apFonts[i] = NULL;
		} // if (m_apFonts[i] != NULL) 
	} // for (int i=0; i<MAXFONTS; i++) 
	if (m_pCacheBitmap != NULL)
	{
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	} // if (m_pCacheBitmap != NULL) 
}

void CBaseView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	} // if (m_pCacheBitmap != NULL) 
	m_nScreenLines = -1;
	m_nScreenChars = -1;
	RecalcVertScrollBar();
	RecalcHorzScrollBar();
}

BOOL CBaseView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_pwndLeft)
		m_pwndLeft->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndRight)
		m_pwndRight->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndBottom)
		m_pwndBottom->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CBaseView::OnDoMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	int nLineCount = GetLineCount();
	int nTopLine = m_nTopLine;
	nTopLine -= (zDelta/30);
	if (nTopLine < 0)
		nTopLine = 0;
	if (nTopLine >= nLineCount)
		nTopLine = nLineCount - 1;
	ScrollToLine(nTopLine, TRUE);
}

BOOL CBaseView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (nHitTest == HTCLIENT)
	{
		::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));	// Set To Arrow Cursor
		return TRUE;
	} // if (nHitTest == HTCLIENT) 
	return CView::OnSetCursor(pWnd, nHitTest, message);
}

void CBaseView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
	Invalidate();
}

void CBaseView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	Invalidate();
}

int CBaseView::GetLineFromPoint(CPoint point)
{
	ScreenToClient(&point);
	return (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
}

void CBaseView::OnContextMenu(CPoint point, int nLine)
{
}
BOOL CBaseView::IsStateSelectable(CDiffData::DiffStates state)
{
	return FALSE;
}

void CBaseView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	int nLine = GetLineFromPoint(point);

	if (nLine <= m_arLineStates->GetCount())
	{
		int nIndex = nLine - 1;
		CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nIndex);
		if ((state != CDiffData::DIFFSTATE_NORMAL) && (state != CDiffData::DIFFSTATE_UNKNOWN))
		{
			if (IsStateSelectable(state))
			{
				while (nIndex > 0)
				{
					if (state != m_arLineStates->GetAt(--nIndex))
						break;
				} // while (nIndex >= 0)
				m_nSelBlockStart = nIndex+1;
				while (nIndex < (m_arLineStates->GetCount()-1))
				{
					if (state != m_arLineStates->GetAt(++nIndex))
						break;
				} // while (nIndex < m_arLineStates->GetCount())
				if ((nIndex == (m_arLineStates->GetCount()-1))&&(state == m_arLineStates->GetAt(nIndex)))
					m_nSelBlockEnd = nIndex;
				else
					m_nSelBlockEnd = nIndex-1;
				Invalidate();
			} // if (IsStateSelectable(state)) 
		} // if ((state != CDiffData::DIFFSTATE_NORMAL) && (state != CDiffData::DIFFSTATE_UNKNOWN)) 
		OnContextMenu(point, nLine);
		m_nSelBlockStart = -1;
		m_nSelBlockEnd = -1;
		if (m_pwndLeft)
			m_pwndLeft->Invalidate();
		if (m_pwndRight)
			m_pwndRight->Invalidate();
		if (m_pwndBottom)
			m_pwndBottom->Invalidate();
		if (m_pwndLocator)
			m_pwndLocator->Invalidate();
	} // if (nLine <= m_arLineStates->GetCount()) 
}

void CBaseView::GoToFirstDifference()
{
	int nCenterPos = 0;
	if ((m_arLineStates)&&(0 < m_arLineStates->GetCount()))
	{
		CDiffData::DiffStates state = CDiffData::DIFFSTATE_NORMAL;
		while ((nCenterPos < m_arLineStates->GetCount()) &&
			(m_arLineStates->GetAt(nCenterPos++)==state))
			;
		while (nCenterPos < m_arLineStates->GetCount())
		{
			CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
			if ((linestate != CDiffData::DIFFSTATE_NORMAL) &&
				(linestate != CDiffData::DIFFSTATE_IDENTICAL) &&
				(linestate != CDiffData::DIFFSTATE_UNKNOWN))
				break;
			nCenterPos++;
		} // while (nCenterPos > m_arLineStates->GetCount()) 
		int nTopPos = nCenterPos - (m_nScreenLines/2);
		if (nTopPos < 0)
			nTopPos = 0;
		ScrollAllToLine(nTopPos);
	} // if ((m_arLineStates)&&(nCenterPos < m_arLineStates->GetCount())) 
}

void CBaseView::OnMergeNextdifference()
{
	int nCenterPos = m_nTopLine + (m_nScreenLines/2);
	if ((m_arLineStates)&&(m_nTopLine < m_arLineStates->GetCount()))
	{
		if (nCenterPos >= m_arLineStates->GetCount())
			nCenterPos = (int)m_arLineStates->GetCount()-1;
		CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while ((nCenterPos < m_arLineStates->GetCount()) &&
			(m_arLineStates->GetAt(nCenterPos++)==state))
			;
		while (nCenterPos < m_arLineStates->GetCount())
		{
			CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
			if ((linestate != CDiffData::DIFFSTATE_NORMAL) &&
				(linestate != CDiffData::DIFFSTATE_IDENTICAL) &&
				(linestate != CDiffData::DIFFSTATE_UNKNOWN))
				break;
			nCenterPos++;
		} // while (nCenterPos > m_arLineStates->GetCount()) 
		int nTopPos = nCenterPos - (m_nScreenLines/2);
		if (nTopPos < 0)
			nTopPos = 0;
		ScrollAllToLine(nTopPos);
	} // if ((m_arLineStates)&&(nCenterPos < m_arLineStates->GetCount())) 
}

void CBaseView::OnMergePreviousdifference()
{
	int nCenterPos = m_nTopLine + (m_nScreenLines/2);
	if ((m_arLineStates)&&(m_nTopLine < m_arLineStates->GetCount()))
	{
		if (nCenterPos >= m_arLineStates->GetCount())
			nCenterPos = (int)m_arLineStates->GetCount()-1;
		CDiffData::DiffStates state = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
		while ((nCenterPos >= 0) &&
			(m_arLineStates->GetAt(nCenterPos--)==state))
			;
		while (nCenterPos >= 0)
		{
			CDiffData::DiffStates linestate = (CDiffData::DiffStates)m_arLineStates->GetAt(nCenterPos);
			if ((linestate != CDiffData::DIFFSTATE_NORMAL) &&
				(linestate != CDiffData::DIFFSTATE_IDENTICAL) &&
				(linestate != CDiffData::DIFFSTATE_UNKNOWN))
				break;
			nCenterPos--;
		} // while (nCenterPos > m_arLineStates->GetCount()) 
		int nTopPos = nCenterPos - (m_nScreenLines/2);
		if (nTopPos < 0)
			nTopPos = 0;
		ScrollAllToLine(nTopPos);
	} // if ((m_arLineStates)&&(nCenterPos < m_arLineStates->GetCount())) 
}










