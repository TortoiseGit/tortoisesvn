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
#include "commctrl.h"
#include "Commdlg.h"
#include "MainWindow.h"

#pragma comment(lib, "comctl32.lib")

stdstring	CMainWindow::leftpicpath;
stdstring	CMainWindow::leftpictitle;

stdstring	CMainWindow::rightpicpath;
stdstring	CMainWindow::rightpictitle;


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
	wcx.hIconSm	= LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
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
	RECT tbRect;
	if (clientrect == NULL)
		return;
	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);
	GetWindowRect(hwndTB, &tbRect);
	LONG tbHeight = tbRect.bottom-tbRect.top-1;
	HDWP hdwp = BeginDeferWindowPos(2);
	if (bOverlap)
	{
		if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, NULL, clientrect->left, clientrect->top+SLIDER_HEIGHT+tbHeight, clientrect->right-clientrect->left, clientrect->bottom-clientrect->top-SLIDER_HEIGHT-tbHeight, SWP_SHOWWINDOW);
		if (hdwp) hdwp = DeferWindowPos(hdwp, hTrackbar, NULL, clientrect->left, clientrect->top+tbHeight, clientrect->right-clientrect->left, SLIDER_HEIGHT, SWP_SHOWWINDOW);
	}
	else
	{
		if (bVertical)
		{
			RECT child;
			child.left = clientrect->left;
			child.top = clientrect->top+tbHeight;
			child.right = clientrect->right;
			child.bottom = nSplitterPos-(SPLITTER_BORDER/2);
			if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
			child.top = nSplitterPos+(SPLITTER_BORDER/2);
			child.bottom = clientrect->bottom;
			if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow2, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
		}
		else
		{
			RECT child;
			child.left = clientrect->left;
			child.top = clientrect->top+tbHeight;
			child.right = nSplitterPos-(SPLITTER_BORDER/2);
			child.bottom = clientrect->bottom;
			if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
			child.left = nSplitterPos+(SPLITTER_BORDER/2);
			child.right = clientrect->right;
			if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow2, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
		}
	}
	if (hdwp) EndDeferWindowPos(hdwp);
	InvalidateRect(*this, NULL, FALSE);
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
			// create a slider control
			hTrackbar = CreateTrackbar(*this, 0, 255);
			ShowWindow(hTrackbar, SW_HIDE);
			CreateToolbar();
		}
		break;
	case WM_COMMAND:
		{
			return DoCommand(LOWORD(wParam));
		}
		break;
	case WM_HSCROLL:
		{
			if (bOverlap)
			{
				if (LOWORD(wParam) == TB_THUMBTRACK)
				{
					// while tracking, only redraw after 50 milliseconds
					::SetTimer(*this, TIMER_ALPHASLIDER, 50, NULL);
				}
				else
					picWindow1.SetSecondPicAlpha((BYTE)SendMessage(hTrackbar, TBM_GETPOS, 0, 0));
			}
		}
		break;
	case WM_TIMER:
		{
			if ((wParam == TIMER_ALPHASLIDER)&&(bOverlap))
				picWindow1.SetSecondPicAlpha((BYTE)SendMessage(hTrackbar, TBM_GETPOS, 0, 0));
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rect;

			::GetClientRect(*this, &rect);
			hdc = BeginPaint(hwnd, &ps);
			SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
			::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
			EndPaint(hwnd, &ps);
		}
		break;
	case WM_SIZING:
		{
			RECT * pRect = (RECT *)lParam;
			switch (wParam)
			{
			case WMSZ_BOTTOM:
				if ((pRect->bottom-pRect->top)<WINDOW_MINHEIGTH)
					pRect->bottom = pRect->top+WINDOW_MINHEIGTH;
				break;
			case WMSZ_BOTTOMLEFT:
				if ((pRect->right-pRect->left)<WINDOW_MINWIDTH)
					pRect->left = pRect->right-WINDOW_MINWIDTH;
				if ((pRect->bottom-pRect->top)<WINDOW_MINHEIGTH)
					pRect->bottom = pRect->top+WINDOW_MINHEIGTH;
				break;
			case WMSZ_BOTTOMRIGHT:
				if ((pRect->right-pRect->left)<WINDOW_MINWIDTH)
					pRect->right = pRect->left+WINDOW_MINWIDTH;
				if ((pRect->bottom-pRect->top)<WINDOW_MINHEIGTH)
					pRect->bottom = pRect->top+WINDOW_MINHEIGTH;
				break;
			case WMSZ_LEFT:
				if ((pRect->right-pRect->left)<WINDOW_MINWIDTH)
					pRect->left = pRect->right-WINDOW_MINWIDTH;
				break;
			case WMSZ_RIGHT:
				if ((pRect->right-pRect->left)<WINDOW_MINWIDTH)
					pRect->right = pRect->left+WINDOW_MINWIDTH;
				break;
			case WMSZ_TOP:
				if ((pRect->bottom-pRect->top)<WINDOW_MINHEIGTH)
					pRect->top = pRect->bottom-WINDOW_MINHEIGTH;
				break;
			case WMSZ_TOPLEFT:
				if ((pRect->right-pRect->left)<WINDOW_MINWIDTH)
					pRect->left = pRect->right-WINDOW_MINWIDTH;
				if ((pRect->bottom-pRect->top)<WINDOW_MINHEIGTH)
					pRect->top = pRect->bottom-WINDOW_MINHEIGTH;
				break;
			case WMSZ_TOPRIGHT:
				if ((pRect->right-pRect->left)<WINDOW_MINWIDTH)
					pRect->right = pRect->left+WINDOW_MINWIDTH;
				if ((pRect->bottom-pRect->top)<WINDOW_MINHEIGTH)
					pRect->top = pRect->bottom-WINDOW_MINHEIGTH;
				break;
			}
		}
		break;
	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			if (bVertical)
				nSplitterPos = (rect.bottom-rect.top)/2;
			else
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
	case WM_SETCURSOR:
		{
			if ((HWND)wParam == *this)
			{
				RECT rect;
				POINT pt;
				GetClientRect(*this, &rect);
				GetCursorPos(&pt);
				ScreenToClient(*this, &pt);
				if (PtInRect(&rect, pt))
				{
					if (bVertical)
					{
						HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZENS));
						SetCursor(hCur);
					}
					else
					{
						HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
						SetCursor(hCur);
					}
					return TRUE;
				}
			}
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
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
	case WM_NOTIFY:
		{
			LPNMHDR pNMHDR = (LPNMHDR)lParam;
			if (pNMHDR->code == TTN_GETDISPINFO)
			{
				if ((HWND)wParam == hTrackbar)
				{
					LPTOOLTIPTEXT lpttt; 

					lpttt = (LPTOOLTIPTEXT) lParam; 
					lpttt->hinst = hInstance; 

					// Specify the resource identifier of the descriptive 
					// text for the given button. 
					TCHAR stringbuf[MAX_PATH] = {0};
					_stprintf_s(stringbuf, MAX_PATH, _T("%ld alpha"), (BYTE)SendMessage(hTrackbar, TBM_GETPOS, 0, 0));
					lpttt->lpszText = stringbuf;
				}
				else
				{
					LPTOOLTIPTEXT lpttt; 

					lpttt = (LPTOOLTIPTEXT) lParam; 
					lpttt->hinst = hInstance; 

					// Specify the resource identifier of the descriptive 
					// text for the given button. 
					TCHAR stringbuf[MAX_PATH] = {0};
					MENUITEMINFO mii;
					mii.cbSize = sizeof(MENUITEMINFO);
					mii.fMask = MIIM_TYPE;
					mii.dwTypeData = stringbuf;
					mii.cch = sizeof(stringbuf)/sizeof(TCHAR);
					GetMenuItemInfo(GetMenu(*this), lpttt->hdr.idFrom, FALSE, &mii);
					lpttt->lpszText = stringbuf;
				}
			}
		}
		break;
	case WM_DESTROY:
		bWindowClosed = TRUE;
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		ImageList_Destroy(hToolbarImgList);
		::DestroyWindow(m_hwnd);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
};

LRESULT CMainWindow::DoCommand(int id)
{
	switch (id) 
	{
	case ID_FILE_OPEN:
		{
			if (OpenDialog())
			{
				picWindow1.SetPic(leftpicpath, _T(""));
				picWindow1.FitImageInWindow();
				picWindow2.SetPic(rightpicpath, _T(""));
				picWindow2.FitImageInWindow();
				if (bOverlap)
				{
					picWindow1.SetSecondPic(picWindow2.GetPic(), rightpictitle, rightpicpath);
				}
				else
				{
					picWindow1.SetSecondPic();
				}
				RECT rect;
				GetClientRect(*this, &rect);
				PositionChildren(&rect);
			}
		}
		break;
	case ID_VIEW_IMAGEINFO:
		{
			bShowInfo = !bShowInfo;
			HMENU hMenu = GetMenu(*this);
			UINT uCheck = MF_BYCOMMAND;
			uCheck |= bShowInfo ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hMenu, ID_VIEW_IMAGEINFO, uCheck);

			picWindow1.ShowInfo(bShowInfo);
			picWindow2.ShowInfo(bShowInfo);

			// change the state of the toolbar button
			TBBUTTONINFO tbi;
			tbi.cbSize = sizeof(TBBUTTONINFO);
			tbi.dwMask = TBIF_STATE;
			tbi.fsState = bShowInfo ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
			SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_IMAGEINFO, (LPARAM)&tbi);
		}
		break;
	case ID_VIEW_OVERLAPIMAGES:
		{
			bOverlap = !bOverlap;
			HMENU hMenu = GetMenu(*this);
			UINT uCheck = MF_BYCOMMAND;
			uCheck |= bOverlap ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hMenu, ID_VIEW_OVERLAPIMAGES, uCheck);

			ShowWindow(picWindow2, bOverlap ? SW_HIDE : SW_SHOW);
			ShowWindow(hTrackbar, bOverlap ? SW_SHOW : SW_HIDE);
			if (bOverlap)
			{
				picWindow1.StopTimer();
				picWindow2.StopTimer();
				picWindow1.SetSecondPic(picWindow2.GetPic(), rightpictitle, rightpicpath);
				picWindow1.SetSecondPicAlpha(127);
				SendMessage(hTrackbar, TBM_SETPOS, (WPARAM)1, (LPARAM)127);
			}
			else
			{
				picWindow1.SetSecondPic();
			}

			// change the state of the toolbar button
			TBBUTTONINFO tbi;
			tbi.cbSize = sizeof(TBBUTTONINFO);
			tbi.dwMask = TBIF_STATE;
			tbi.fsState = bOverlap ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
			SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_OVERLAPIMAGES, (LPARAM)&tbi);

			RECT rect;
			GetClientRect(*this, &rect);
			PositionChildren(&rect);
			return 0;
		}
		break;
	case ID_VIEW_FITIMAGESINWINDOW:
		{
			picWindow1.FitImageInWindow();
			picWindow2.FitImageInWindow();
			RECT rect;
			GetClientRect(*this, &rect);
			PositionChildren(&rect);
		}
		break;
	case ID_VIEW_ORININALSIZE:
		{
			picWindow1.SetZoom(1.0);
			picWindow2.SetZoom(1.0);
			RECT rect;
			GetClientRect(*this, &rect);
			PositionChildren(&rect);
		}
		break;
	case ID_VIEW_ZOOMIN:
		{
			picWindow1.Zoom(true);
			picWindow2.Zoom(true);
			RECT rect;
			GetClientRect(*this, &rect);
			PositionChildren(&rect);
		}
		break;
	case ID_VIEW_ZOOMOUT:
		{
			picWindow1.Zoom(false);
			picWindow2.Zoom(false);
			RECT rect;
			GetClientRect(*this, &rect);
			PositionChildren(&rect);
		}
		break;
	case ID_VIEW_ARRANGEVERTICAL:
		{
			bVertical = !bVertical;
			RECT rect;
			GetClientRect(*this, &rect);
			if (bVertical)
			{
				nSplitterPos = (rect.bottom-rect.top)/2;
			}
			else
			{
				nSplitterPos = (rect.right-rect.left)/2;
			}
			HMENU hMenu = GetMenu(*this);
			UINT uCheck = MF_BYCOMMAND;
			uCheck |= bVertical ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hMenu, ID_VIEW_ARRANGEVERTICAL, uCheck);
			// change the state of the toolbar button
			TBBUTTONINFO tbi;
			tbi.cbSize = sizeof(TBBUTTONINFO);
			tbi.dwMask = TBIF_STATE;
			tbi.fsState = bVertical ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
			SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_ARRANGEVERTICAL, (LPARAM)&tbi);

			PositionChildren(&rect);
		}
		break;
	case IDM_EXIT:
		::PostQuitMessage(0);
		return 0;
		break;
	default:
		break;
	};
	return 1;
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
		pt.x = 0;
	if (pt.x > rect.right-4) 
		pt.x = rect.right-4;
	if (pt.y < 0)
		pt.y = 0;
	if (pt.y > rect.bottom-4)
		pt.y = rect.bottom-4;

	bDragMode = true;

	SetCapture(hwnd);

	hdc = GetWindowDC(hwnd);
	if (bVertical)
		DrawXorBar(hdc, clientrect.left, pt.y+2, clientrect.right-clientrect.left-2, 4);
	else
		DrawXorBar(hdc, pt.x+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
	ReleaseDC(hwnd, hdc);

	oldx = pt.x;
	oldy = pt.y;

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
		pt.x = 0;
	if (pt.x > rect.right-4) 
		pt.x = rect.right-4;
	if (pt.y < 0)
		pt.y = 0;
	if (pt.y > rect.bottom-4)
		pt.y = rect.bottom-4;

	hdc = GetWindowDC(hwnd);
	if (bVertical)
		DrawXorBar(hdc, clientrect.left, oldy+2, clientrect.right-clientrect.left-2, 4);			
	else
		DrawXorBar(hdc, oldx+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);			
	ReleaseDC(hwnd, hdc);

	oldx = pt.x;
	oldy = pt.y;

	bDragMode = false;

	//convert the splitter position back to screen coords.
	GetWindowRect(hwnd, &rect);
	pt.x += rect.left;
	pt.y += rect.top;

	//now convert into CLIENT coordinates
	ScreenToClient(hwnd, &pt);
	GetClientRect(hwnd, &rect);
	if (bVertical)
		nSplitterPos = pt.y;
	else
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

	//convert the mouse coordinates relative to the top-left of
	//the window
	ClientToScreen(hwnd, &pt);
	pt.x -= rect.left;
	pt.y -= rect.top;

	//same for the window coordinates - make them relative to 0,0
	OffsetRect(&rect, -rect.left, -rect.top);

	if (pt.x < 0)
		pt.x = 0;
	if (pt.x > rect.right-4) 
		pt.x = rect.right-4;
	if (pt.y < 0)
		pt.y = 0;
	if (pt.y > rect.bottom-4)
		pt.y = rect.bottom-4;

	if ((wParam & MK_LBUTTON) && ((bVertical && (pt.y != oldy)) || (!bVertical && (pt.x != oldx))))
	{
		hdc = GetWindowDC(hwnd);

		if (bVertical)
		{
			DrawXorBar(hdc, clientrect.left, oldy+2, clientrect.right-clientrect.left-2, 4);
			DrawXorBar(hdc, clientrect.left, pt.y+2, clientrect.right-clientrect.left-2, 4);
		}
		else
		{
			DrawXorBar(hdc, oldx+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
			DrawXorBar(hdc, pt.x+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
		}

		ReleaseDC(hwnd, hdc);

		oldx = pt.x;
		oldy = pt.y;
	}

	return 0;
}

HWND CMainWindow::CreateTrackbar(HWND hwndParent, UINT iMin, UINT iMax)
{ 
	HWND hwndTrack = CreateWindowEx( 
		0,									// no extended styles 
		TRACKBAR_CLASS,						// class name 
		_T("Trackbar Control"),				// title (caption) 
		WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_TOOLTIPS,	// style 
		10, 10,								// position 
		200, 30,							// size 
		hwndParent,							// parent window 
		(HMENU)TRACKBAR_ID,					// control identifier 
		hInstance,							// instance 
		NULL								// no WM_CREATE parameter 
		); 

	SendMessage(hwndTrack, TBM_SETRANGE, 
		(WPARAM) TRUE,						// redraw flag 
		(LPARAM) MAKELONG(iMin, iMax));		// min. & max. positions 
	SendMessage(hwndTrack, TBM_SETTIPSIDE, 
		(WPARAM) TBTS_TOP,						// redraw flag 
		(LPARAM) 0);		// min. & max. positions 

	return hwndTrack; 
}

bool CMainWindow::OpenDialog()
{
	return (DialogBox(hInstance, MAKEINTRESOURCE(IDD_OPEN), *this, (DLGPROC)OpenDlgProc)==IDOK);
}

BOOL CALLBACK CMainWindow::OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_INITDIALOG:
		{
			// center on the parent window
			HWND hParentWnd = ::GetParent(hwndDlg);
			RECT parentrect, childrect, centeredrect;
			GetWindowRect(hParentWnd, &parentrect);
			GetWindowRect(hwndDlg, &childrect);
			centeredrect.left = parentrect.left + ((parentrect.right-parentrect.left-childrect.right+childrect.left)/2);
			centeredrect.right = centeredrect.left + (childrect.right-childrect.left);
			centeredrect.top = parentrect.top + ((parentrect.bottom-parentrect.top-childrect.bottom+childrect.top)/2);
			centeredrect.bottom = centeredrect.top + (childrect.bottom-childrect.top);
			SetWindowPos(hwndDlg, NULL, centeredrect.left, centeredrect.top, centeredrect.right-centeredrect.left, centeredrect.bottom-centeredrect.top, SWP_SHOWWINDOW);
			SetFocus(hwndDlg);
		}
		break;
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_LEFTBROWSE:
			{
				TCHAR path[MAX_PATH] = {0};
				if (AskForFile(hwndDlg, path))
				{
					SetDlgItemText(hwndDlg, IDC_LEFTIMAGE, path);
				}
			}
			break;
		case IDC_RIGHTBROWSE:
			{
				TCHAR path[MAX_PATH] = {0};
				if (AskForFile(hwndDlg, path))
				{
					SetDlgItemText(hwndDlg, IDC_RIGHTIMAGE, path);
				}
			}
			break;
		case IDOK: 
			{
				TCHAR path[MAX_PATH];
				if (!GetDlgItemText(hwndDlg, IDC_LEFTIMAGE, path, MAX_PATH)) 
					*path = 0;
				leftpicpath = path;
				if (!GetDlgItemText(hwndDlg, IDC_RIGHTIMAGE, path, MAX_PATH))
					*path = 0;
				rightpicpath = path;
			}
			// Fall through. 
		case IDCANCEL: 
			EndDialog(hwndDlg, wParam); 
			return TRUE; 
		} 
	} 
	return FALSE; 
}

bool CMainWindow::AskForFile(HWND owner, TCHAR * path)
{
	OPENFILENAME ofn;		// common dialog box structure
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFile = path;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = ResString(hInst, IDS_OPENIMAGEFILE);
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	ofn.hInstance = hInst;
	TCHAR filters[] = _T("Images\0*.wmf;*.jpg;*jpeg;*.bmp;*.gif;*.png;*.ico;*.dib;*.emf\0All (*.*)\0*.*\0\0");
	ofn.lpstrFilter = filters;
	ofn.nFilterIndex = 1;
	// Display the Open dialog box. 
	if (GetOpenFileName(&ofn)==FALSE)
	{
		return false;
	}
	return true;
}

bool CMainWindow::CreateToolbar()
{
	// Ensure that the common control DLL is loaded. 
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	hwndTB = CreateWindowEx(0, 
							TOOLBARCLASSNAME, 
							(LPCTSTR)NULL,
							WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 
							0, 0, 0, 0, 
							*this,
							(HMENU)IDC_TORTOISEIDIFF, 
							hInstance, 
							NULL);
	if (hwndTB == INVALID_HANDLE_VALUE)
		return false;

	SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

	TBBUTTON tbb[9];
	// create an imagelist containing the icons for the toolbar
	hToolbarImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 9, 4);
	if (hToolbarImgList == NULL)
		return false;
	int index = 0;
	HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OVERLAP));
	tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon); 
	tbb[index].idCommand = ID_VIEW_OVERLAPIMAGES; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	tbb[index].iBitmap = 0; 
	tbb[index].idCommand = 0; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_SEP; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VERTICAL));
	tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon); 
	tbb[index].idCommand = ID_VIEW_ARRANGEVERTICAL; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FITINWINDOW));
	tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon); 
	tbb[index].idCommand = ID_VIEW_FITIMAGESINWINDOW; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ORIGSIZE));
	tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon); 
	tbb[index].idCommand = ID_VIEW_ORININALSIZE; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ZOOMIN));
	tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon); 
	tbb[index].idCommand = ID_VIEW_ZOOMIN; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ZOOMOUT));
	tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon); 
	tbb[index].idCommand = ID_VIEW_ZOOMOUT; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	tbb[index].iBitmap = 0; 
	tbb[index].idCommand = 0; 
	tbb[index].fsState = TBSTATE_ENABLED; 
	tbb[index].fsStyle = BTNS_SEP; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMGINFO));
	tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon); 
	tbb[index].idCommand = ID_VIEW_IMAGEINFO; 
	tbb[index].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED; 
	tbb[index].fsStyle = BTNS_BUTTON; 
	tbb[index].dwData = 0; 
	tbb[index++].iString = 0; 

	SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)hToolbarImgList);
	SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM)index, (LPARAM) (LPTBBUTTON) &tbb); 
	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0); 
	ShowWindow(hwndTB, SW_SHOW); 
	return true; 

}
