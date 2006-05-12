// TortoiseIDiff - an image diff viewer in TortoiseSVN

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
#include "StdAfx.h"
#include "PicWindow.h"

bool CPicWindow::RegisterAndCreateWindow(HWND hParent)
{
	WNDCLASSEX wcx; 

	// Fill in the window class structure with default parameters 
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = CWindow::stWinMsgHandler;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.lpszClassName = _T("TortoiseIDiffPicWindow");
	wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
	wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
	wcx.hIconSm	= LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterWindow(&wcx);
	if (Create(WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, hParent))
	{
		ShowWindow(m_hwnd, SW_SHOW);
		UpdateWindow(m_hwnd);
		return true;
	}
	return false;
}

LRESULT CALLBACK CPicWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		break;
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rect;

			::GetClientRect(*this, &rect);
			hdc = BeginPaint(hwnd, &ps);
			{
				CMemDC memDC(hdc);
				DrawViewTitle(memDC, &rect);

				GetClientRect(&rect);
				if (bFirstpaint)
				{
					// make the image fit into the window: adjust the scale factor and scroll
					// positions
					if (rect.right-rect.left)
					{
						if (((rect.right - rect.left) > picture.m_Width)&&((rect.bottom - rect.top)> picture.m_Height))
						{
							// image is smaller than the window
							picscale = 1.0;
						}
						else
						{
							// image is bigger than the window
							double xscale = double(rect.right-rect.left)/double(picture.m_Width);
							double yscale = double(rect.bottom-rect.top)/double(picture.m_Height);
							picscale = min(yscale, xscale);
						}
						SetupScrollBars();
						bFirstpaint = false;
					}
				}
				if (bValid)
				{
					RECT picrect;
					picrect.left =  rect.left-nHScrollPos;
					picrect.right = (picrect.left + LONG(double(picture.m_Width)*picscale));
					picrect.top = rect.top-nVScrollPos;
					picrect.bottom = (picrect.top + LONG(double(picture.m_Height)*picscale));

					// first, 'erase' the parts which the image doesn't cover
					RECT uncovered;
					SetBkColor(memDC, ::GetSysColor(COLOR_WINDOW));
					if (picrect.left)
					{
						// rectangle left of the pic
						uncovered.left = rect.left;
						uncovered.right = picrect.left;
						uncovered.top = rect.top;
						uncovered.bottom = rect.bottom;
						::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &uncovered, NULL, 0, NULL);
					}
					if (picrect.top)
					{
						// rectangle above the pic
						uncovered.left = rect.left;
						uncovered.top = rect.top;
						uncovered.right = rect.right;
						uncovered.bottom = picrect.top;
						::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &uncovered, NULL, 0, NULL);
					}
					if (picrect.right < rect.right)
					{
						// rectangle right of the pic
						uncovered.left = picrect.right;
						uncovered.top = rect.top;
						uncovered.right = rect.right;
						uncovered.bottom = rect.bottom;
						::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &uncovered, NULL, 0, NULL);
					}
					if (picrect.bottom < rect.bottom)
					{
						// rectangle below the pic
						uncovered.left = rect.left;
						uncovered.top = picrect.bottom;
						uncovered.right = rect.right;
						uncovered.bottom = rect.bottom;
						::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &uncovered, NULL, 0, NULL);
					}

					picture.Show(memDC, picrect);
				}
				else
					TextOut(memDC, 0, 0, _T("This is a TEST!!!"), 17);

			}
			EndPaint(hwnd, &ps);
		}
		break;
	case WM_SIZE:
		SetupScrollBars();
		break;
	case WM_VSCROLL:
		OnVScroll(LOWORD(wParam), HIWORD(wParam));
		break;
	case WM_HSCROLL:
		OnHScroll(LOWORD(wParam), HIWORD(wParam));
		break;
	case WM_DESTROY:
		bWindowClosed = TRUE;
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
};

void CPicWindow::SetPic(stdstring path, stdstring title)
{
	picpath=path;pictitle=title;
	bValid = picture.Load(picpath);
}

void CPicWindow::DrawViewTitle(HDC hDC, RECT * rect)
{
	RECT textrect;
	textrect.left = rect->left;
	textrect.top = rect->top;
	textrect.right = rect->right;
	textrect.bottom = rect->top + HEADER_HEIGHT;

	COLORREF crBk, crFg;
	crBk = ::GetSysColor(COLOR_SCROLLBAR);
	crFg = ::GetSysColor(COLOR_WINDOWTEXT);
	SetBkColor(hDC, crBk);
	::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &textrect, NULL, 0, NULL);

	if (GetFocus() == *this)
		DrawEdge(hDC, &textrect, EDGE_BUMP, BF_RECT);
	else
		DrawEdge(hDC, &textrect, EDGE_ETCHED, BF_RECT);

	SetTextColor(hDC, crFg);

	// use the path if no title is set.
	stdstring * title = pictitle.empty() ? &picpath : &pictitle;

	SIZE stringsize;
	if (GetTextExtentPoint32(hDC, title->c_str(), title->size(), &stringsize))
	{
		int nStringLength = stringsize.cx;

		ExtTextOut(hDC, 
			max(textrect.left + ((textrect.right-textrect.left)-nStringLength)/2, 1),
			textrect.top + (HEADER_HEIGHT/2) - stringsize.cy/2,
			ETO_CLIPPED,
			&textrect,
			title->c_str(),
			title->size(),
			NULL);
	}
}

void CPicWindow::SetupScrollBars()
{
	RECT rect;
	GetClientRect(&rect);

	SCROLLINFO si = {sizeof(si)};

	si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;

	si.nPos  = nVScrollPos;
	si.nPage = rect.bottom-rect.top;
	si.nMin  = 0;
	si.nMax  = picture.m_Height;
	SetScrollInfo(*this, SB_VERT, &si, TRUE);

	si.nPos  = nHScrollPos;
	si.nPage = rect.right-rect.left;
	si.nMin  = 0;
	si.nMax  = picture.m_Width;
	SetScrollInfo(*this, SB_HORZ, &si, TRUE);

	bool bPicWidthBigger = (int(double(picture.m_Width)/picscale) > (rect.right-rect.left));
	bool bPicHeigthBigger = (int(double(picture.m_Height)/picscale) > (rect.bottom-rect.top));
	// set the scroll position so that the image is drawn centered in the window
	// if the window is bigger than the image
	if (!bPicWidthBigger)
	{
		nHScrollPos = -((rect.right-rect.left)-int(double(picture.m_Width)/picscale))/2;
	}
	if (!bPicHeigthBigger)
	{
		nVScrollPos = -((rect.bottom-rect.top)-int(double(picture.m_Height)/picscale))/2;
	}
	// if the image is smaller than the window, we don't need the scrollbars
	ShowScrollBar(*this, SB_HORZ, bPicWidthBigger);
	ShowScrollBar(*this, SB_VERT, bPicHeigthBigger);

}

void CPicWindow::OnVScroll(UINT nSBCode, UINT nPos)
{
	RECT rect;
	GetClientRect(&rect);
	switch (nSBCode)
	{
	case SB_BOTTOM:
		nVScrollPos = picture.m_Height;
		break;
	case SB_TOP:
		nVScrollPos = 0;
		break;
	case SB_LINEDOWN:
		nVScrollPos++;
		break;
	case SB_LINEUP:
		nVScrollPos--;
		break;
	case SB_PAGEDOWN:
		nVScrollPos += (rect.bottom-rect.top);
		break;
	case SB_PAGEUP:
		nVScrollPos -= (rect.bottom-rect.top);
		break;
	case SB_THUMBPOSITION:
		nVScrollPos = nPos;
		break;
	default:
		return;
	}
	if (nVScrollPos > (picture.m_Height-rect.bottom+rect.top))
		nVScrollPos = picture.m_Height-rect.bottom+rect.top;
	if (nVScrollPos < 0)
		nVScrollPos = 0;
	SetupScrollBars();
	InvalidateRect(*this, NULL, TRUE);
}

void CPicWindow::OnHScroll(UINT nSBCode, UINT nPos)
{
	RECT rect;
	GetClientRect(&rect);
	switch (nSBCode)
	{
	case SB_RIGHT:
		nHScrollPos = picture.m_Width;
		break;
	case SB_LEFT:
		nHScrollPos = 0;
		break;
	case SB_LINERIGHT:
		nHScrollPos++;
		break;
	case SB_LINELEFT:
		nHScrollPos--;
		break;
	case SB_PAGERIGHT:
		nHScrollPos += (rect.right-rect.left);
		break;
	case SB_PAGELEFT:
		nHScrollPos -= (rect.right-rect.left);
		break;
	case SB_THUMBPOSITION:
		nHScrollPos = nPos;
		break;
	default:
		return;
	}
	if (nHScrollPos > picture.m_Width)
		nHScrollPos = picture.m_Width;
	if (nHScrollPos < 0)
		nHScrollPos = 0;
	SetupScrollBars();
	InvalidateRect(*this, NULL, TRUE);
}

void CPicWindow::GetClientRect(RECT * pRect)
{
	::GetClientRect(*this, pRect);
	pRect->top += HEADER_HEIGHT;
}