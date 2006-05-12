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
#include "MainWindow.h"

bool CMainWindow::RegisterAndCreateWindow()
{
	WNDCLASSEX wcx; 

	// Fill in the window class structure with default parameters 
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = CWindow::stWinMsgHandler;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hCursor = LoadCursor(NULL, IDC_SIZEWE);
	wcx.lpszClassName = ResString(hInstance, IDS_APP_TITLE);
	wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
	wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
	wcx.hIconSm	= LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	if (RegisterWindow(&wcx))
	{
		if (Create(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, NULL))
		{
			ShowWindow(m_hwnd, SW_SHOW);
			UpdateWindow(m_hwnd);
			return true;
		}
	}
	return false;
}

void CMainWindow::PositionChildren(RECT * clientrect /* = NULL */)
{
	if (clientrect == NULL)
		return;
	RECT child;
	child.left = clientrect->left;
	child.top = clientrect->top;
	child.right = nSplitterPos-nSplitterBorder;
	child.bottom = clientrect->bottom;
	SetWindowPos(picWindow1, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_SHOWWINDOW);
	child.left = nSplitterPos+nSplitterBorder;
	child.right = clientrect->right;
	SetWindowPos(picWindow2, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_SHOWWINDOW);
}

LRESULT CALLBACK CMainWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		{
			m_hwnd = hwnd;
			picWindow1.RegisterAndCreateWindow(hwnd);
			picWindow1.SetPic(leftpicpath, leftpictitle);
			picWindow2.RegisterAndCreateWindow(hwnd);
			picWindow2.SetPic(rightpicpath, rightpictitle);
			// center the splitter
			RECT rect;
			GetClientRect(hwnd, &rect);
			nSplitterPos = (rect.right-rect.left)/2;
		}
		break;
	case WM_COMMAND:
		{
			DoCommand(LOWORD(wParam));
		}
		break;
	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			nSplitterPos = (rect.right-rect.left)/2;
			PositionChildren(&rect);
		}
		break;
	case WM_MOVE:
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			PositionChildren(&rect);
		}
		break;
	case WM_LBUTTONDOWN:
		Splitter_OnLButtonDown(hwnd, uMsg, wParam, lParam);
		break;
	case WM_LBUTTONUP:
		Splitter_OnLButtonUp(hwnd, uMsg, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		Splitter_OnMouseMove(hwnd, uMsg, wParam, lParam);
		break;
	case WM_DESTROY:
		bWindowClosed = TRUE;
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		::DestroyWindow(m_hwnd);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
};

void CMainWindow::DoCommand(int id)
{
	switch (id) 
	{
	case IDM_EXIT:
		::PostQuitMessage(0);
		break;
	default:
		break;
	};
}

// splitter stuff
void CMainWindow::DrawXorBar(HDC hdc, int x1, int y1, int width, int height)
{
	static WORD _dotPatternBmp[8] = 
	{ 
		0x0055, 0x00aa, 0x0055, 0x00aa, 
		0x0055, 0x00aa, 0x0055, 0x00aa
	};

	HBITMAP hbm;
	HBRUSH  hbr, hbrushOld;

	hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
	hbr = CreatePatternBrush(hbm);

	SetBrushOrgEx(hdc, x1, y1, 0);
	hbrushOld = (HBRUSH)SelectObject(hdc, hbr);

	PatBlt(hdc, x1, y1, width, height, PATINVERT);

	SelectObject(hdc, hbrushOld);

	DeleteObject(hbr);
	DeleteObject(hbm);
}

LRESULT CMainWindow::Splitter_OnLButtonDown(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	HDC hdc;
	RECT rect;
	RECT clientrect;

	pt.x = (short)LOWORD(lParam);  // horizontal position of cursor 
	pt.y = (short)HIWORD(lParam);

	GetClientRect(hwnd, &clientrect);
	GetWindowRect(hwnd, &rect);
	POINT zero = {0,0};
	ClientToScreen(hwnd, &zero);
	OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(hwnd, &pt);
	pt.x -= rect.left;
	pt.y -= rect.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&rect, -rect.left, -rect.top);

	if (pt.x < 0) 
	{
		pt.x = 0;
	}
	if (pt.x > rect.right-4) 
	{
		pt.x = rect.right-4;
	}

	bDragMode = true;

	SetCapture(hwnd);

	hdc = GetWindowDC(hwnd);
	DrawXorBar(hdc, pt.x+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
	ReleaseDC(hwnd, hdc);

	oldx = pt.x;

	return 0;
}


LRESULT CMainWindow::Splitter_OnLButtonUp(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT rect;
	RECT clientrect;

	POINT pt;
	pt.x = (short)LOWORD(lParam);  // horizontal position of cursor 
	pt.y = (short)HIWORD(lParam);

	if (bDragMode == FALSE)
		return 0;

	GetClientRect(hwnd, &clientrect);
	GetWindowRect(hwnd, &rect);
	POINT zero = {0,0};
	ClientToScreen(hwnd, &zero);
	OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

	ClientToScreen(hwnd, &pt);
	pt.x -= rect.left;
	pt.y -= rect.top;

	OffsetRect(&rect, -rect.left, -rect.top);

	if (pt.x < 0)
	{
		pt.x = 0;
	}
	if (pt.x > rect.right-4) 
	{
		pt.x = rect.right-4;
	}

	hdc = GetWindowDC(hwnd);
	DrawXorBar(hdc, oldx+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);			
	ReleaseDC(hwnd, hdc);

	oldx = pt.x;

	bDragMode = false;

	//convert the splitter position back to screen coords.
	GetWindowRect(hwnd, &rect);
	pt.x += rect.left;
	pt.y += rect.top;

	//now convert into CLIENT coordinates
	ScreenToClient(hwnd, &pt);
	GetClientRect(hwnd, &rect);
	nSplitterPos = pt.x;

	//position the child controls
	PositionChildren(&rect);

	ReleaseCapture();

	return 0;
}

LRESULT CMainWindow::Splitter_OnMouseMove(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT rect;
	RECT clientrect;

	POINT pt;

	if (bDragMode == FALSE)
		return 0;

	pt.x = (short)LOWORD(lParam);  // horizontal position of cursor 
	pt.y = (short)HIWORD(lParam);

	GetClientRect(hwnd, &clientrect);
	GetWindowRect(hwnd, &rect);
	POINT zero = {0,0};
	ClientToScreen(hwnd, &zero);
	OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

	if (pt.x < 0)
	{
		pt.x = 0;
	}
	if (pt.x > rect.right-4) 
	{
		pt.x = rect.right-4;
	}

	if (pt.x != oldx && wParam & MK_LBUTTON)
	{
		hdc = GetWindowDC(hwnd);

		DrawXorBar(hdc, oldx+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
		DrawXorBar(hdc, pt.x+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);

		ReleaseDC(hwnd, hdc);

		oldx = pt.x;
	}

	return 0;
}
