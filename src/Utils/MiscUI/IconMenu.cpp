// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "StdAfx.h"
#include "IconMenu.h"

#include "gdiplus.h"

CIconMenu::CIconMenu(void) : CMenu()
{
	OSVERSIONINFOEX inf;
	SecureZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	winVersion = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	if(winVersion >= 0x600)
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&m_gdipToken, &gdiplusStartupInput, NULL);
	}
}

CIconMenu::~CIconMenu(void)
{
	std::map<UINT, HBITMAP>::iterator it;
	for (it = bitmaps.begin(); it != bitmaps.end(); ++it)
	{
		::DeleteObject(it->second);
	}
	bitmaps.clear();
	if(m_gdipToken)
		Gdiplus::GdiplusShutdown(m_gdipToken);
}

BOOL CIconMenu::AppendMenuIcon(UINT_PTR nIDNewItem, LPCTSTR lpszNewItem, UINT uIcon /* = 0 */)
{
	TCHAR menutextbuffer[255] = {0};
	_tcscpy_s(menutextbuffer, 255, lpszNewItem);

	if (uIcon == 0)
		return CMenu::AppendMenu(MF_STRING | MF_ENABLED, nIDNewItem, menutextbuffer);

	MENUITEMINFO info = {0};
	info.cbSize = sizeof(info);
	info.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID;
	info.fType = MFT_STRING;
	info.wID = nIDNewItem;
	info.dwTypeData = menutextbuffer;
	if (winVersion >= 0x600)
	{
		info.fMask |= MIIM_BITMAP;
		info.hbmpItem = IconToBitmapPARGB32(uIcon);
	}
	else
	{
		info.fMask |= MIIM_CHECKMARKS;
		info.hbmpChecked = IconToBitmap(uIcon);
		info.hbmpUnchecked = IconToBitmap(uIcon);
	}
	return InsertMenuItem(nIDNewItem, &info);
}

HBITMAP CIconMenu::IconToBitmapPARGB32(UINT uIcon)
{
	std::map<UINT, HBITMAP>::iterator bitmap_it = bitmaps.lower_bound(uIcon);
	if (bitmap_it != bitmaps.end() && bitmap_it->first == uIcon)
		return bitmap_it->second;
	if (!m_gdipToken)
		return NULL;
	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(uIcon), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	if (!hIcon)
		return NULL;
	Gdiplus::Bitmap icon(hIcon);
	Gdiplus::Bitmap bmp(16, 16, PixelFormat32bppPARGB);
	Gdiplus::Graphics g(&bmp);
	g.DrawImage(&icon, 0, 0, 16, 16);
	DestroyIcon(hIcon);
	HBITMAP hBmp = NULL;
	bmp.GetHBITMAP(Gdiplus::Color(255, 0, 0, 0), &hBmp);

	if(hBmp)
		bitmaps.insert(bitmap_it, std::make_pair(uIcon, hBmp));
	return hBmp;
}

HBITMAP CIconMenu::IconToBitmap(UINT uIcon)
{
	std::map<UINT, HBITMAP>::iterator bitmap_it = bitmaps.lower_bound(uIcon);
	if (bitmap_it != bitmaps.end() && bitmap_it->first == uIcon)
		return bitmap_it->second;

	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(uIcon), IMAGE_ICON, 12, 12, LR_DEFAULTCOLOR);
	if (!hIcon)
		return NULL;

	RECT rect;

	rect.right = ::GetSystemMetrics(SM_CXMENUCHECK);
	rect.bottom = ::GetSystemMetrics(SM_CYMENUCHECK);

	rect.left = rect.top = 0;

	HWND desktop = ::GetDesktopWindow();
	if (desktop == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	HDC screen_dev = ::GetDC(desktop);
	if (screen_dev == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	// Create a compatible DC
	HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
	if (dst_hdc == NULL)
	{
		DestroyIcon(hIcon);
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}

	// Create a new bitmap of icon size
	HBITMAP bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
	if (bmp == NULL)
	{
		DestroyIcon(hIcon);
		::DeleteDC(dst_hdc);
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}

	// Select it into the compatible DC
	HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
	if (old_dst_bmp == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	// Fill the background of the compatible DC with the white color
	// that is taken by menu routines as transparent
	::SetBkColor(dst_hdc, RGB(255, 255, 255));
	::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	// Draw the icon into the compatible DC
	::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);

	// Restore settings
	::SelectObject(dst_hdc, old_dst_bmp);
	::DeleteDC(dst_hdc);
	::ReleaseDC(desktop, screen_dev); 
	DestroyIcon(hIcon);
	if (bmp)
		bitmaps.insert(bitmap_it, std::make_pair(uIcon, bmp));
	return bmp;
}
