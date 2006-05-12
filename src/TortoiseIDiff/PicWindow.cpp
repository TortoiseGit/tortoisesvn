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
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rect;

			GetClientRect(*this, &rect);
			hdc = BeginPaint(hwnd, &ps); 
			DrawViewTitle(hdc, &rect);

			if (bValid)
			{
				// center the image in the window
				if (((rect.right - rect.left) > picture.m_Width)&&((rect.bottom - rect.top)> picture.m_Height))
				{
					// image is smaller than the window
					rect.left = ((rect.right - rect.left) - picture.m_Width)/2;
					rect.right = rect.left + picture.m_Width;
					rect.top = ((rect.bottom-rect.top)-picture.m_Height)/2;
					rect.bottom = rect.top + picture.m_Height;
				}
				else
				{
					// image is bigger than the window
					double xscale = double(rect.right-rect.left)/double(picture.m_Width);
					double yscale = double(rect.bottom-rect.top)/double(picture.m_Height);
					double scale = min(yscale, xscale);
					rect.left =  ((rect.right - rect.left) - LONG(double(picture.m_Width)*scale))/2;
					rect.right = rect.left + LONG(double(picture.m_Width)*scale);
					rect.top = ((rect.bottom-rect.top) - LONG(double(picture.m_Height)*scale))/2;
					rect.bottom = rect.top + LONG(double(picture.m_Height)*scale);
				}
				picture.Show(hdc, rect);
			}
			else
				TextOut(hdc, 0, 0, _T("This is a TEST!!!"), 17);

			EndPaint(hwnd, &ps);
		}
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

