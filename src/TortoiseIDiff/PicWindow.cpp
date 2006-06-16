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
#include "shellapi.h"
#include "commctrl.h"
#include "PicWindow.h"
#include "math.h"

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "shell32.lib")

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
	wcx.hIconSm	= LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
	RegisterWindow(&wcx);
	if (CreateEx(WS_EX_ACCEPTFILES | WS_EX_CLIENTEDGE, WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, hParent))
	{
		ShowWindow(m_hwnd, SW_SHOW);
		UpdateWindow(m_hwnd);
		CreateButtons();
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
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		InvalidateRect(*this, NULL, FALSE);
		break;
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		Paint(hwnd);
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
	case WM_MOUSEWHEEL:
		{
			short fwKeys = GET_KEYSTATE_WPARAM(wParam);
			short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			RECT rect;
			GetClientRect(&rect);
			LONG width = picture.m_Width;
			LONG height = picture.m_Height;
			if (pSecondPic)
			{
				width = max(width, pSecondPic->m_Width);
				height = max(height, pSecondPic->m_Height);
			}
			if (fwKeys & MK_SHIFT)
			{
				// shift means scrolling sideways
				nHScrollPos -= zDelta;
				if (nHScrollPos > width-rect.right+rect.left)
					nHScrollPos = width-rect.right+rect.left;
				if (nHScrollPos < 0)
					nHScrollPos = 0;
				SetupScrollBars();
				InvalidateRect(*this, NULL, FALSE);
				PositionChildren();
			}
			else if (fwKeys & MK_CONTROL)
			{
				// control means adjusting the scale factor
				Zoom(zDelta>0);
				SetupScrollBars();
				InvalidateRect(*this, NULL, FALSE);
				SetWindowPos(*this, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOREPOSITION|SWP_NOMOVE);
				PositionChildren();
			}
			else
			{
				nVScrollPos -= zDelta;
				if (nVScrollPos > (height-rect.bottom+rect.top))
					nVScrollPos = height-rect.bottom+rect.top;
				if (nVScrollPos < 0)
					nVScrollPos = 0;
				SetupScrollBars();
				InvalidateRect(*this, NULL, FALSE);
				PositionChildren();
			}
		}
		break;
	case WM_LBUTTONDOWN:
		SetFocus(*this);
		break;
	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)wParam;		
			TCHAR szFileName[MAX_PATH];
			// we only use the first file dropped (if multiple files are dropped)
			DragQueryFile(hDrop, 0, szFileName, sizeof(szFileName));
			SetPic(szFileName, _T(""));
			FitImageInWindow();
			InvalidateRect(*this, NULL, TRUE);
		}
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case LEFTBUTTON_ID:
				{
					nCurrentDimension--;
					if (nCurrentDimension < 1)
						nCurrentDimension = 1;
					nCurrentFrame--;
					if (nCurrentFrame < 1)
						nCurrentFrame = 1;
					picture.SetActiveFrame(nCurrentFrame >= nCurrentDimension ? nCurrentFrame : nCurrentDimension);
					InvalidateRect(*this, NULL, FALSE);
					PositionChildren();
					return 0;
				}
				break;
			case RIGHTBUTTON_ID:
				{
					nCurrentDimension++;
					if (nCurrentDimension > picture.GetNumberOfDimensions())
						nCurrentDimension = picture.GetNumberOfDimensions();
					nCurrentFrame++;
					if (nCurrentFrame > picture.GetNumberOfFrames(0))
						nCurrentFrame = picture.GetNumberOfFrames(0);
					picture.SetActiveFrame(nCurrentFrame >= nCurrentDimension ? nCurrentFrame : nCurrentDimension);
					InvalidateRect(*this, NULL, FALSE);
					PositionChildren();
					return 0;
				}
				break;
			case PLAYBUTTON_ID:
				{
					if (bPlaying)
					{
						bPlaying = false;
						SendMessage(hwndPlayBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hPlay);
						KillTimer(*this, ID_ANIMATIONTIMER);
					}
					else
					{
						bPlaying = true;
						SendMessage(hwndPlayBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hStop);
						SetTimer(*this, ID_ANIMATIONTIMER, 0, NULL);
					}
					return 0;
				}
				break;
			}
			// pass the message to our parent
			SendMessage(GetParent(*this), WM_COMMAND, wParam, lParam);
		}
		break;
	case WM_TIMER:
		{
			if (wParam == ID_ANIMATIONTIMER)
			{
				nCurrentFrame++;
				if (nCurrentFrame > picture.GetNumberOfFrames(0))
					nCurrentFrame = 1;
				long delay = picture.SetActiveFrame(nCurrentFrame);
				delay = max(100, delay);
				SetTimer(*this, ID_ANIMATIONTIMER, delay, NULL);
				InvalidateRect(*this, NULL, FALSE);
			}
		}
		break;
	case WM_DESTROY:
		DestroyIcon(hLeft);
		DestroyIcon(hRight);
		DestroyIcon(hPlay);
		DestroyIcon(hStop);
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
	picture.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	bValid = picture.Load(picpath);
	nDimensions = picture.GetNumberOfDimensions();
	if (nDimensions)
		nFrames = picture.GetNumberOfFrames(0);
	if (bValid)
	{
		picscale = 1.0;
		InvalidateRect(*this, NULL, FALSE);
		PositionChildren();
	}
}

void CPicWindow::DrawViewTitle(HDC hDC, RECT * rect)
{
	RECT textrect;
	textrect.left = rect->left;
	textrect.top = rect->top;
	textrect.right = rect->right;
	textrect.bottom = rect->top + HEADER_HEIGHT;
	if (HasMultipleImages())
		textrect.bottom += HEADER_HEIGHT;

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

	stdstring realtitle = *title;
	stdstring imgnumstring;

	if (pSecondPic)
	{
		realtitle = realtitle + _T(" - ") + (pictitle2.empty() ? picpath2 : pictitle2);
	}

	if (HasMultipleImages())
	{
		TCHAR buf[MAX_PATH];
		if (nFrames > 1)
			_stprintf_s(buf, sizeof(buf)/sizeof(TCHAR), (const TCHAR *)ResString(hInstance, IDS_DIMENSIONSANDFRAMES), nCurrentFrame, nFrames);
		else
			_stprintf_s(buf, sizeof(buf)/sizeof(TCHAR), (const TCHAR *)ResString(hInstance, IDS_DIMENSIONSANDFRAMES), nCurrentDimension, nDimensions);
		imgnumstring = buf;
	}

	SIZE stringsize;
	if (GetTextExtentPoint32(hDC, realtitle.c_str(), realtitle.size(), &stringsize))
	{
		int nStringLength = stringsize.cx;

		ExtTextOut(hDC, 
			max(textrect.left + ((textrect.right-textrect.left)-nStringLength)/2, 1),
			textrect.top + (HEADER_HEIGHT/2) - stringsize.cy/2,
			ETO_CLIPPED,
			&textrect,
			realtitle.c_str(),
			realtitle.size(),
			NULL);
	}
	if (HasMultipleImages())
	{
		if (GetTextExtentPoint32(hDC, imgnumstring.c_str(), imgnumstring.size(), &stringsize))
		{
			int nStringLength = stringsize.cx;

			ExtTextOut(hDC, 
				max(textrect.left + ((textrect.right-textrect.left)-nStringLength)/2, 1),
				textrect.top + HEADER_HEIGHT + (HEADER_HEIGHT/2) - stringsize.cy/2,
				ETO_CLIPPED,
				&textrect,
				imgnumstring.c_str(),
				imgnumstring.size(),
				NULL);
		}
	}
}

void CPicWindow::SetupScrollBars()
{
	RECT rect;
	GetClientRect(&rect);

	SCROLLINFO si = {sizeof(si)};

	si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;

	LONG width = picture.m_Width;
	LONG height = picture.m_Height;
	if (pSecondPic)
	{
		width = max(width, pSecondPic->m_Width);
		height = max(height, pSecondPic->m_Height);
	}
	si.nPos  = nVScrollPos;
	si.nPage = rect.bottom-rect.top;
	si.nMin  = 0;
	si.nMax  = int(double(height)*picscale);
	SetScrollInfo(*this, SB_VERT, &si, TRUE);

	si.nPos  = nHScrollPos;
	si.nPage = rect.right-rect.left;
	si.nMin  = 0;
	si.nMax  = int(double(width)*picscale);
	SetScrollInfo(*this, SB_HORZ, &si, TRUE);

	bool bPicWidthBigger = (int(double(width)*picscale) > (rect.right-rect.left));
	bool bPicHeigthBigger = (int(double(height)*picscale) > (rect.bottom-rect.top));
	// set the scroll position so that the image is drawn centered in the window
	// if the window is bigger than the image
	if (!bPicWidthBigger)
	{
		nHScrollPos = -((rect.right-rect.left)-int(double(width)*picscale))/2;
	}
	if (!bPicHeigthBigger)
	{
		nVScrollPos = -((rect.bottom-rect.top)-int(double(height)*picscale))/2;
	}
	// if the image is smaller than the window, we don't need the scrollbars
	ShowScrollBar(*this, SB_HORZ, bPicWidthBigger);
	ShowScrollBar(*this, SB_VERT, bPicHeigthBigger);
	PositionChildren();
}

void CPicWindow::OnVScroll(UINT nSBCode, UINT nPos)
{
	RECT rect;
	GetClientRect(&rect);
	switch (nSBCode)
	{
	case SB_BOTTOM:
		nVScrollPos = LONG(double(picture.GetHeight())*picscale);;
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
	LONG height = LONG(double(picture.GetHeight())*picscale);
	if (pSecondPic)
	{
		height = max(height, LONG(double(pSecondPic->GetHeight())*picscale));
	}
	if (nVScrollPos > (height-rect.bottom+rect.top))
		nVScrollPos = height-rect.bottom+rect.top;
	if (nVScrollPos < 0)
		nVScrollPos = 0;
	SetupScrollBars();
	InvalidateRect(*this, NULL, TRUE);
	PositionChildren();
}

void CPicWindow::OnHScroll(UINT nSBCode, UINT nPos)
{
	RECT rect;
	GetClientRect(&rect);
	switch (nSBCode)
	{
	case SB_RIGHT:
		nHScrollPos = LONG(double(picture.GetWidth())*picscale);
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
	LONG width = LONG(double(picture.GetWidth())*picscale);
	if (pSecondPic)
	{
		width = max(width, LONG(double(pSecondPic->GetWidth())*picscale));
	}
	if (nHScrollPos > width-rect.right+rect.left)
		nHScrollPos = width-rect.right+rect.left;
	if (nHScrollPos < 0)
		nHScrollPos = 0;
	SetupScrollBars();
	InvalidateRect(*this, NULL, TRUE);
	PositionChildren();
}

void CPicWindow::GetClientRect(RECT * pRect)
{
	::GetClientRect(*this, pRect);
	pRect->top += HEADER_HEIGHT;
	if (HasMultipleImages())
	{
		pRect->top += HEADER_HEIGHT;
	}
}

void CPicWindow::SetZoom(double dZoom)
{
	// Set the interpolation mode depending on zoom
	if(dZoom < 1.0){	// Zoomed out, use high quality bicubic
		picture.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	}
	else if(!((int)(dZoom*100.0)%100)){	// "Even" zoom sizes should be shown w-o any interpolation
		picture.SetInterpolationMode(InterpolationModeNearestNeighbor);
	}
	else{	// Arbitrary zoomed in, use bilinear that is semi-smoothed
		picture.SetInterpolationMode(InterpolationModeBilinear);
	}

	picscale = dZoom;
	SetupScrollBars();
	InvalidateRect(*this, NULL, TRUE);
	PositionChildren();
}

void CPicWindow::Zoom(bool in)
{
	double zoomFactor;

	// Find correct zoom factor	and quantize picscale
	if(!in && picscale <= 0.2){
		picscale = 0.1;
		zoomFactor = 0;
	}
	else if((in && picscale < 1.0) || (!in && picscale <= 1.0)){
		picscale = 0.1 * RoundDouble(picscale/0.1, 0);	// Quantize to 0.1
		zoomFactor = 0.1;
	}
	else if((in && picscale < 2.0) || (!in && picscale <= 2.0)){
		picscale = 0.25 * RoundDouble(picscale/0.25, 0);	// Quantize to 0.25
		zoomFactor = 0.25;
	}
	else{
		picscale = RoundDouble(picscale,0);
		zoomFactor = 1;
	}

	// Set zoom
	if(in){
		SetZoom(picscale+zoomFactor);
	}
	else{
		SetZoom(picscale-zoomFactor);
	}
}

double CPicWindow::RoundDouble(double doValue, int nPrecision)
{
	static const double doBase = 10.0;
	double doComplete5, doComplete5i;

	doComplete5 = doValue * pow(doBase, (double) (nPrecision + 1));

	if(doValue < 0.0){
		doComplete5 -= 5.0;
	}
	else{
		doComplete5 += 5.0;
	}

	doComplete5 /= doBase;
	modf(doComplete5, &doComplete5i);

	return doComplete5i / pow(doBase, (double) nPrecision);
}
void CPicWindow::FitImageInWindow()
{
	RECT rect;
	GetClientRect(&rect);
	if (rect.right-rect.left)
	{
		if (((rect.right - rect.left) > picture.m_Width)&&((rect.bottom - rect.top)> picture.m_Height))
		{
			// image is smaller than the window
			SetZoom(1.0);
		}
		else
		{
			// image is bigger than the window
			double xscale = double(rect.right-rect.left)/double(picture.m_Width);
			double yscale = double(rect.bottom-rect.top)/double(picture.m_Height);
			SetZoom(min(yscale, xscale));
		}
		SetupScrollBars();
	}
	PositionChildren();
}

void CPicWindow::Paint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect, fullrect;

	::GetClientRect(*this, &fullrect);
	hdc = BeginPaint(hwnd, &ps);
	{
		CMemDC memDC(hdc);

		GetClientRect(&rect);
		if (bFirstpaint)
		{
			FitImageInWindow();
			bFirstpaint = false;
		}
		if (bValid)
		{
			RECT picrect;
			picrect.left =  rect.left-nHScrollPos;
			picrect.right = (picrect.left + LONG(double(picture.m_Width)*picscale));
			picrect.top = rect.top-nVScrollPos;
			picrect.bottom = (picrect.top + LONG(double(picture.m_Height)*picscale));

			SetBkColor(memDC, ::GetSysColor(COLOR_WINDOW));
			::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

			if ((rect.left < picrect.left)&&(rect.top < picrect.top)&&(rect.right > picrect.right)&&(rect.bottom > picrect.bottom))
			{
				RECT border;
				border.left = picrect.left-1;
				border.top = picrect.top-1;
				border.right = picrect.right+1;
				border.bottom = picrect.bottom+1;
				FillRect(memDC, &border, (HBRUSH)(COLOR_3DDKSHADOW+1));
			}
			picture.Show(memDC, picrect);
			if (pSecondPic)
			{
				CMemDC memDC2(hdc);

				RECT picrect2;
				picrect2.left =  rect.left-nHScrollPos;
				picrect2.right = (picrect2.left + LONG(double(pSecondPic->m_Width)*picscale));
				picrect2.top = rect.top-nVScrollPos;
				picrect2.bottom = (picrect2.top + LONG(double(pSecondPic->m_Height)*picscale));

				SetBkColor(memDC2, ::GetSysColor(COLOR_WINDOW));
				::ExtTextOut(memDC2, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

				if ((rect.left < picrect2.left)&&(rect.top < picrect2.top)&&(rect.right > picrect2.right)&&(rect.bottom > picrect2.bottom))
				{
					RECT border;
					border.left = picrect2.left-1;
					border.top = picrect2.top-1;
					border.right = picrect2.right+1;
					border.bottom = picrect2.bottom+1;
					FillRect(memDC2, &border, (HBRUSH)(COLOR_3DDKSHADOW+1));
				}

				pSecondPic->Show(memDC2, picrect2);
				BLENDFUNCTION blender;
				blender.AlphaFormat = 0;
				blender.BlendFlags = 0;
				blender.BlendOp = AC_SRC_OVER;
				blender.SourceConstantAlpha = alpha;
				AlphaBlend(memDC, 
					rect.left,
					rect.top,
					rect.right-rect.left,
					rect.bottom-rect.top,
					memDC2,
					rect.left,
					rect.top,
					rect.right-rect.left,
					rect.bottom-rect.top,
					blender);
			}
			if (bShowInfo)
			{
				RECT inforect;
				inforect.left = rect.left+4;
				inforect.top = rect.top;
				inforect.right = rect.right;
				inforect.bottom = rect.bottom;

				TCHAR infostring[4096];
				if (pSecondPic)
				{
					_stprintf_s(infostring, sizeof(infostring)/sizeof(TCHAR), 
						(TCHAR const *)ResString(hInstance, IDS_DUALIMAGEINFO),
						pictitle.empty() ? picpath.c_str() : pictitle.c_str(),
						picture.GetFileSizeAsText().c_str(),
						picture.GetWidth(), picture.GetHeight(),
						picture.GetHorizontalResolution(), picture.GetVerticalResolution(),
						picture.GetColorDepth(),
						(UINT)(GetZoom()*100.0),
						pictitle2.empty() ? picpath2.c_str() : pictitle2.c_str(),
						pSecondPic->GetFileSizeAsText().c_str(),
						pSecondPic->GetWidth(), pSecondPic->GetHeight(),
						pSecondPic->GetHorizontalResolution(), pSecondPic->GetVerticalResolution(),
						pSecondPic->GetColorDepth());
				}
				else
				{
					_stprintf_s(infostring, sizeof(infostring)/sizeof(TCHAR), 
						(TCHAR const *)ResString(hInstance, IDS_IMAGEINFO),
						picture.GetFileSizeAsText().c_str(), 
						picture.GetWidth(), picture.GetHeight(),
						picture.GetHorizontalResolution(), picture.GetVerticalResolution(),
						picture.GetColorDepth(),
						(UINT)(GetZoom()*100.0));
				}
				// set the font
				HFONT hFont = CreateFont(-MulDiv(8, GetDeviceCaps(memDC, LOGPIXELSY), 72), 0, 0, 0, FW_DONTCARE, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("MS Shell Dlg"));
				HFONT hFontOld = (HFONT)SelectObject(memDC, (HGDIOBJ)hFont);
				// find out how big the rectangle for the text has to be
				DrawText(memDC, infostring, -1, &inforect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_VCENTER | DT_CALCRECT);

				// the text should be drawn with a four pixel offset to the window borders
				inforect.top = rect.bottom - (inforect.bottom-inforect.top) - 4;
				inforect.bottom = rect.bottom-4;

				// first draw an edge rectangle
				RECT edgerect;
				edgerect.left = inforect.left-4;
				edgerect.top = inforect.top-4;
				edgerect.right = inforect.right+4;
				edgerect.bottom = inforect.bottom+4;
				::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &edgerect, NULL, 0, NULL);
				DrawEdge(memDC, &edgerect, EDGE_BUMP, BF_RECT | BF_SOFT);

				SetTextColor(memDC, GetSysColor(COLOR_WINDOWTEXT));
				DrawText(memDC, infostring, -1, &inforect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_VCENTER);
				SelectObject(memDC, (HGDIOBJ)hFontOld);
				DeleteObject(hFont);
			}
		}
		else
		{
			SetBkColor(memDC, ::GetSysColor(COLOR_WINDOW));
			::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
			SIZE stringsize;
			ResString str = ResString(hInstance, IDS_INVALIDIMAGEINFO);
			if (GetTextExtentPoint32(memDC, str, _tcslen(str), &stringsize))
			{
				int nStringLength = stringsize.cx;

				ExtTextOut(memDC, 
					max(rect.left + ((rect.right-rect.left)-nStringLength)/2, 1),
					rect.top + ((rect.bottom-rect.top) - stringsize.cy)/2,
					ETO_CLIPPED,
					&rect,
					str,
					_tcslen(str),
					NULL);
			}
		}
		DrawViewTitle(memDC, &fullrect);
	}
	EndPaint(hwnd, &ps);
}

bool CPicWindow::CreateButtons()
{
	// Ensure that the common control DLL is loaded. 
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	hwndLeftBtn = CreateWindowEx(0, 
								_T("BUTTON"), 
								(LPCTSTR)NULL,
								WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT, 
								0, 0, 0, 0, 
								*this,
								(HMENU)LEFTBUTTON_ID,
								hInstance, 
								NULL);
	if (hwndLeftBtn == INVALID_HANDLE_VALUE)
		return false;
	hLeft = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_BACKWARD), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
	SendMessage(hwndLeftBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hLeft);
	hwndRightBtn = CreateWindowEx(0, 
								_T("BUTTON"), 
								(LPCTSTR)NULL,
								WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT, 
								0, 0, 0, 0, 
								*this,
								(HMENU)RIGHTBUTTON_ID,
								hInstance, 
								NULL);
	if (hwndRightBtn == INVALID_HANDLE_VALUE)
		return false;
	hRight = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_FORWARD), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
	SendMessage(hwndRightBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hRight);
	hwndPlayBtn = CreateWindowEx(0, 
								_T("BUTTON"), 
								(LPCTSTR)NULL,
								WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT, 
								0, 0, 0, 0, 
								*this,
								(HMENU)PLAYBUTTON_ID,
								hInstance, 
								NULL);
	if (hwndPlayBtn == INVALID_HANDLE_VALUE)
		return false;
	hPlay = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_START), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
	hStop = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_STOP), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
	SendMessage(hwndPlayBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hPlay);
	return true;
}

void CPicWindow::PositionChildren()
{
	RECT rect;
	::GetClientRect(*this, &rect);
	if (HasMultipleImages())
	{
		SetWindowPos(hwndLeftBtn, HWND_TOP, rect.left+3, rect.top + HEADER_HEIGHT + (HEADER_HEIGHT-16)/2, 16, 16, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
		SetWindowPos(hwndRightBtn, HWND_TOP, rect.left+23, rect.top + HEADER_HEIGHT + (HEADER_HEIGHT-16)/2, 16, 16, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
		if (nFrames > 1)
			SetWindowPos(hwndPlayBtn, HWND_TOP, rect.left+43, rect.top + HEADER_HEIGHT + (HEADER_HEIGHT-16)/2, 16, 16, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
		else
			ShowWindow(hwndPlayBtn, SW_HIDE);
	}
	else
	{
		ShowWindow(hwndLeftBtn, SW_HIDE);
		ShowWindow(hwndRightBtn, SW_HIDE);
		ShowWindow(hwndPlayBtn, SW_HIDE);
	}
}

bool CPicWindow::HasMultipleImages()
{
	return (((nDimensions > 1)||(nFrames > 1))&&(pSecondPic == NULL));
}
