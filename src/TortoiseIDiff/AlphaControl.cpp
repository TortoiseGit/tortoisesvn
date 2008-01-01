// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006 - 2007 - Stefan Kueng

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
#include "AlphaControl.h"
#include <vector>
#include "commctrl.h"

CAlphaControl::CAlphaControl(HWND hwndControl)
	: hwndControl(hwndControl)
	, nBorder(8)
	, nMin(0)
	, nMax(255)
	, nCurr(128)
	, eTracking(TRACKING_NONE)
	, nLeftMarker(255)
	, nRightMarker(0)
{
	SetWindowLong(hwndControl, 0, (LONG)this);
}

void CAlphaControl::RegisterCustomControl()
{
	WNDCLASSEX wc = {0};

	wc.cbSize			= sizeof(wc);
	wc.lpszClassName	= sCAlphaControl;
	wc.hInstance		= GetModuleHandle(0);
	wc.lpfnWndProc		= stMsgHandler;
	wc.hCursor			= NULL;
	wc.hIcon			= 0;
	wc.lpszMenuName		= 0;
	wc.hbrBackground	= (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
	wc.style			= 0;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= sizeof(CAlphaControl*);
	wc.hIconSm			= 0;

	RegisterClassEx(&wc);
}

LRESULT CALLBACK CAlphaControl::stMsgHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CAlphaControl* pSelf = (CAlphaControl*)GetWindowLong(hwnd, 0);

	switch(msg)
	{
	// Allocate new class for this window.
	case WM_NCCREATE:
		pSelf = new CAlphaControl(hwnd);
		return (pSelf != NULL);	// This should always be true, since the new
		                        // operator *should* throw if there's no memory,
		                        // but I think it depends on your compiler settings.

	// Destroy the class for this window.
	case WM_NCDESTROY:
		delete pSelf;
		pSelf = NULL;
		break;
	};

	if(pSelf)
		return pSelf->MsgHandler(hwnd, msg, wParam, lParam);
	else
		return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CAlphaControl::MsgHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_SIZE:
		Size(LOWORD(lParam), HIWORD(lParam));
		break;	// We still want the defwindowproc to do its thing
	case WM_PAINT:
		return Paint();
	case WM_ERASEBKGND:
		return 1;
	case WM_MOUSEMOVE:
		return MouseMove(LOWORD(lParam), HIWORD(lParam));
	case WM_LBUTTONDOWN:
		return LButtonDown(LOWORD(lParam), HIWORD(lParam));
	case WM_LBUTTONUP:
		return LButtonUp(LOWORD(lParam), HIWORD(lParam));
	case WM_SETCURSOR:
		if (OnSetCursor(wParam, lParam))
			return TRUE;
		break;
	case TBM_GETPOS:
		return nCurr;
	case TBM_SETPOS:
		SetPos(lParam, wParam ? true : false);
		return 0;
	case ALPHA_GETLEFTPOS:
		return nLeftMarker;
	case ALPHA_GETRIGHTPOS:
		return nRightMarker;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CAlphaControl::Size(UINT x, UINT y)
{
	// Get rectangle for control
	rectGuage.left = nBorder;
	rectGuage.top = nBorder;
	rectGuage.right = x - nBorder;
	rectGuage.bottom = y - nBorder;

	// Find the pixel value of the threshold
	nThreshold = ValueToPixel(nCurr);

	// Find the pixel values of the markers
	nLeftMarkerPixel = ValueToPixel(nLeftMarker);
	nRightMarkerPixel = ValueToPixel(nRightMarker);

	// Get the size that we can fit markers in
	nMarkerHalfSize = (rectGuage.right - rectGuage.left) / 2;
	if(nMarkerHalfSize > nBorder) nMarkerHalfSize = nBorder;
}

void CAlphaControl::SetPos(LONG nVal, bool bRedraw)
{
	nCurr = nVal;
	nThreshold = ValueToPixel(nCurr);

	if(bRedraw)
	{
		RECT rectRedraw;
		GetClientRect(hwndControl, &rectRedraw);
		InvalidateRect(hwndControl, &rectRedraw, false);
	}
}

int CAlphaControl::PixelToValue(int nPixel)
{
	return int(float(rectGuage.bottom - nPixel) / (rectGuage.bottom - rectGuage.top) * (nMax - nMin) + nMin);
}

int CAlphaControl::ValueToPixel(int nValue)
{
	return int(rectGuage.bottom - (rectGuage.bottom - rectGuage.top) * (float(nValue - nMin) / (nMax - nMin)));
}

void CAlphaControl::DrawMarker(HDC& hdc, bool bLeft, UINT nHeight)
{
	// Whether the marker is on the left or right, the vertices will be in this pattern:
	//   2 +----+ 3
	//     |    |
	//   1 +    + 4  // One of the vertices on this line will be extruded into a point
	//     |    |
	// 0,6 +----+ 5

	std::vector<POINT> vPoints(7);

	if(bLeft)
	{
		vPoints[0].x = rectGuage.left - nMarkerHalfSize;
		vPoints[0].y = nHeight + nMarkerHalfSize;
		vPoints[1].x = vPoints[0].x;
		vPoints[1].y = nHeight;
		vPoints[2].x = vPoints[0].x;
		vPoints[2].y = nHeight - nMarkerHalfSize;
		vPoints[3].x = rectGuage.left;
		vPoints[3].y = vPoints[2].y;
		vPoints[4].x = rectGuage.left + nMarkerHalfSize;
		vPoints[4].y = nHeight;
		vPoints[5].x = vPoints[3].x;
		vPoints[5].y = vPoints[0].y;
	}
	else
	{
		vPoints[0].x = rectGuage.right;
		vPoints[0].y = nHeight + nMarkerHalfSize;
		vPoints[1].x = rectGuage.right - nMarkerHalfSize;
		vPoints[1].y = nHeight;
		vPoints[2].x = vPoints[0].x;
		vPoints[2].y = nHeight - nMarkerHalfSize;
		vPoints[3].x = rectGuage.right + nMarkerHalfSize;
		vPoints[3].y = vPoints[2].y;
		vPoints[4].x = vPoints[3].x;
		vPoints[4].y = nHeight;
		vPoints[5].x = vPoints[3].x;
		vPoints[5].y = vPoints[0].y;
	}

	vPoints[6] = vPoints[0];

	// Fill the marker
	// This is always the face color, regardless of whether the marker is being tracked or not.
	// This is because Windows doesn't appear to have an interface to get the checked brush it
	// usually uses to paint tracked trackbar thumbs.
	SelectObject(hdc, GetStockObject(DC_BRUSH));
	SetDCBrushColor(hdc, GetSysColor(COLOR_3DFACE));
	Polygon(hdc, &vPoints[0], 6);

	// Draw the very light side
	SelectObject(hdc, GetStockObject(DC_PEN));
	SetDCPenColor(hdc, GetSysColor(COLOR_3DHIGHLIGHT));
	Polyline(hdc, &vPoints[0], 4);

	// Draw the very dark side
	SetDCPenColor(hdc, GetSysColor(COLOR_3DDKSHADOW));
	Polyline(hdc, &vPoints[3], 4);

	// Inset the vertices by one pixel
	vPoints[0].y--;
	vPoints[1].x++;
	vPoints[2].y++;
	vPoints[3].y++;
	vPoints[4].x--;
	vPoints[5].y--;
	if(bLeft)
	{
		vPoints[0].x++;
		vPoints[2].x++;
	}
	else
	{
		vPoints[3].x--;
		vPoints[5].x--;
	}
	vPoints[6] = vPoints[0];

	// Draw the slightly light side
	SetDCPenColor(hdc, GetSysColor(COLOR_3DLIGHT));
	Polyline(hdc, &vPoints[0], 4);

	// Draw the slightly dark side
	SetDCPenColor(hdc, GetSysColor(COLOR_3DSHADOW));
	Polyline(hdc, &vPoints[3], 4);
}

LRESULT CAlphaControl::Paint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwndControl, &ps);

	RECT rectClient;
	GetClientRect(hwndControl, &rectClient);

	// Draw background
	{
		RECT rectBackground;
		rectBackground.left = rectBackground.top = 0;
		rectBackground.right = rectClient.right;
		rectBackground.bottom = rectGuage.top;
		FillRect(hdc, &rectBackground, (HBRUSH) (COLOR_BTNFACE+1));
		rectBackground.top = rectGuage.bottom;
		rectBackground.bottom = rectClient.bottom;
		FillRect(hdc, &rectBackground, (HBRUSH) (COLOR_BTNFACE+1));
		rectBackground.top = rectGuage.top;
		rectBackground.bottom = rectGuage.bottom;
		rectBackground.right = rectGuage.left;
		FillRect(hdc, &rectBackground, (HBRUSH) (COLOR_BTNFACE+1));
		rectBackground.left = rectGuage.right;
		rectBackground.right = rectClient.right;
		FillRect(hdc, &rectBackground, (HBRUSH) (COLOR_BTNFACE+1));
	}

	// Draw outline
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, rectGuage.left, rectGuage.top, rectGuage.right, rectGuage.bottom);

	// Inset the rectangle by one pixel
	rectGuage.left++;
	rectGuage.top++;
	rectGuage.right--;
	rectGuage.bottom--;

	{
		// Draw percentage
		LONG nOldTop = rectGuage.top, nOldBottom = rectGuage.bottom;
		rectGuage.top = nThreshold;
		FillRect(hdc, &rectGuage, (HBRUSH) (COLOR_HIGHLIGHT+1));

		// Draw blank space
		rectGuage.bottom = rectGuage.top;
		rectGuage.top = nOldTop;
		FillRect(hdc, &rectGuage, (HBRUSH) (COLOR_WINDOW+1));
		rectGuage.bottom = nOldBottom;
	}

	// Restore the top and bottom sides of the rect
	rectGuage.top--;
	rectGuage.bottom++;

	// Draw guage marks
	{
		LONG nOneThird = LONG((rectGuage.right - rectGuage.left) / 3.0f);
		LONG nTwoThird = rectGuage.right - nOneThird;
		nOneThird += rectGuage.left;
		for(int nEights = 1; nEights < 8; nEights++)
		{
			UINT nHeight = UINT(ValueToPixel(int((nEights / 8.0f) * (nMax - nMin) + nMin)));

			// Invert the pen depending on the percentage drawn
			if(nHeight < nThreshold)
				SelectObject(hdc, GetStockObject(BLACK_PEN));
			else
				SelectObject(hdc, GetStockObject(WHITE_PEN));

			MoveToEx(hdc, rectGuage.left, nHeight, NULL);
			if(nEights % 2 == 0)	// Quarters, draw a solid line
			{
				LineTo(hdc, rectGuage.right, nHeight);
			}
			else	// Eights, draw segmented line
			{
				LineTo(hdc, nOneThird, nHeight);
				MoveToEx(hdc, nTwoThird, nHeight, NULL);
				LineTo(hdc, rectGuage.right, nHeight);
			}
		}
	}

	// Restore the left and right sides of the rect
	rectGuage.left--;
	rectGuage.right++;

	// Draw the markers
	DrawMarker(hdc, true, nLeftMarkerPixel);
	DrawMarker(hdc, false, nRightMarkerPixel);

	SelectObject(hdc, GetStockObject(DC_BRUSH));

	EndPaint(hwndControl, &ps);

	return 0;
}

LRESULT CAlphaControl::MouseMove(UINT x, UINT y)
{
	if(eTracking != TRACKING_NONE)
	{
		// Do a little binary manipulation to get a negative value if necessary.
		int nVert = y & 0x7fff;
		if(y & 0x8000) nVert = -nVert;

		// Calculate the value position of whatever we're dragging
		if(nVert < rectGuage.top) nVert = rectGuage.top;
		else if(nVert > rectGuage.bottom) nVert = rectGuage.bottom;
		UINT nValuePos = PixelToValue(nVert);

		RECT rectInvalidate;
		rectInvalidate.left = rectGuage.left + 1;
		rectInvalidate.right = rectGuage.right - 1;
		rectInvalidate.top = rectInvalidate.bottom = nThreshold;

		if(eTracking == TRACKING_LEFT)
		{
			if(nValuePos != nLeftMarker)
			{
				nLeftMarker = nValuePos;

				// Find the area to invalidate, and calculate the new marker pos
				rectInvalidate.left = rectGuage.left - nMarkerHalfSize - 1;
				rectInvalidate.top = nLeftMarkerPixel;
				nLeftMarkerPixel = ValueToPixel(nLeftMarker);
				if(int(nLeftMarkerPixel) > rectInvalidate.top)
				{
					rectInvalidate.top -= nMarkerHalfSize;
					rectInvalidate.bottom = nLeftMarkerPixel + nMarkerHalfSize + 1;
				}
				else
				{
					rectInvalidate.bottom = rectInvalidate.top + nMarkerHalfSize + 1;
					rectInvalidate.top = nLeftMarkerPixel - nMarkerHalfSize;
				}
			}
		}
		else if(eTracking == TRACKING_RIGHT)
		{
			if(nValuePos != nRightMarker)
			{
				nRightMarker = nValuePos;

				// Find the area to invalidate, and calculate the new marker pos
				rectInvalidate.right = rectGuage.right + nMarkerHalfSize + 1;
				rectInvalidate.top = nRightMarkerPixel;
				nRightMarkerPixel = ValueToPixel(nRightMarker);
				if(int(nRightMarkerPixel) > rectInvalidate.top)
				{
					rectInvalidate.top -= nMarkerHalfSize;
					rectInvalidate.bottom = nRightMarkerPixel + nMarkerHalfSize + 1;
				}
				else
				{
					rectInvalidate.bottom = rectInvalidate.top + nMarkerHalfSize + 1;
					rectInvalidate.top = nRightMarkerPixel - nMarkerHalfSize;
				}
			}
		}

		// Adjust the value if it's outside the markers
		if ((eTracking == TRACKING_VALUE)||((nLeftMarker < nCurr)||(nRightMarker > nCurr)))
		{
			nCurr = nValuePos;

			// Find the area to invalidate, and calculate the new threshold
			UINT nTemp = ValueToPixel(nCurr);
			if(nTemp > nThreshold)
			{
				if(int(nTemp) > rectInvalidate.bottom) rectInvalidate.bottom = nTemp;
				if(int(nThreshold) < rectInvalidate.top) rectInvalidate.top = nThreshold;
			}
			else
			{
				if(int(nTemp) < rectInvalidate.top) rectInvalidate.top = nTemp;
				if(int(nThreshold) > rectInvalidate.bottom) rectInvalidate.bottom = nThreshold;
			}
			nThreshold = nTemp;

			// Send a message to the parent telling them we're moving the value
			SendMessage(GetParent(hwndControl), WM_VSCROLL, MAKEWPARAM(TB_THUMBTRACK, nCurr), LPARAM(hwndControl)); 
		}

		InvalidateRect(hwndControl, &rectInvalidate, false);
	}

	return 0;
}

LRESULT CAlphaControl::LButtonDown(UINT x, UINT y)
{
	if(eTracking == TRACKING_NONE)
	{
		if(x > rectGuage.left - nMarkerHalfSize && x < rectGuage.left + nMarkerHalfSize &&
			y > nLeftMarkerPixel - nMarkerHalfSize && y < nLeftMarkerPixel + nMarkerHalfSize)
		{
			eTracking = TRACKING_LEFT;
		}
		else if(x > rectGuage.right - nMarkerHalfSize && x < rectGuage.right + nMarkerHalfSize &&
			y > nRightMarkerPixel - nMarkerHalfSize && y < nRightMarkerPixel + nMarkerHalfSize)
		{
			eTracking = TRACKING_RIGHT;
		}
		else if(int(x) > rectGuage.left && int(x) < rectGuage.right && int(y) > rectGuage.top && int(y) < rectGuage.bottom)
		{
			eTracking = TRACKING_VALUE;
		}
		else
			return 0;	// Exit early

		MouseMove(x, y);
		SetCapture(hwndControl);
	}
	return 0;
}

LRESULT CAlphaControl::LButtonUp(UINT x, UINT y)
{
	MouseMove(x, y);
	if(eTracking != TRACKING_NONE)
	{
		// Send a message to the parent telling them we've moved the value
		SendMessage(GetParent(hwndControl), WM_VSCROLL, MAKEWPARAM(TB_THUMBPOSITION, nCurr), LPARAM(hwndControl)); 
		eTracking = TRACKING_NONE;
		ReleaseCapture();
	}
	return 0;
}

BOOL CAlphaControl::OnSetCursor(WPARAM wParam, LPARAM lParam)
{
	if ((hwndControl == (HWND)wParam)&&(LOWORD(lParam)==HTCLIENT))
	{
		if (eTracking == TRACKING_NONE)
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hwndControl, &pt);

			if ((pt.x > (rectGuage.left - LONG(nMarkerHalfSize))) && (pt.x < (rectGuage.left + LONG(nMarkerHalfSize))) &&
				(pt.y > (LONG(nLeftMarkerPixel) - LONG(nMarkerHalfSize))) && (pt.y < (LONG(nLeftMarkerPixel) + LONG(nMarkerHalfSize))))
			{
				SetCursor(LoadCursor(NULL, IDC_SIZENS));
			}
			else if ((pt.x > (rectGuage.right - LONG(nMarkerHalfSize))) && (pt.x < (rectGuage.right + LONG(nMarkerHalfSize))) &&
				(pt.y > (LONG(nRightMarkerPixel) - LONG(nMarkerHalfSize))) && (pt.y < (LONG(nRightMarkerPixel) + LONG(nMarkerHalfSize))))
			{
				SetCursor(LoadCursor(NULL, IDC_SIZENS));
			}
			else if ((int(pt.x) > rectGuage.left) && (int(pt.x) < rectGuage.right) && (int(pt.y) > rectGuage.top) && (int(pt.y) < rectGuage.bottom))
			{
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
			else
				return FALSE;	// Exit early
		}
		else
		{
			switch (eTracking)
			{
			case TRACKING_LEFT:
			case TRACKING_RIGHT:
				SetCursor(LoadCursor(NULL, IDC_SIZENS));
				break;
			case TRACKING_VALUE:
			default:
				SetCursor(LoadCursor(NULL, IDC_ARROW));
				break;
			}
		}
		return TRUE;
	}
	return FALSE;
}