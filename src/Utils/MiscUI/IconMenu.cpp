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
		info.fMask |= MIIM_BITMAP;
		info.hbmpItem = HBMMENU_CALLBACK;
	}
	icons[nIDNewItem] = uIcon;
	return InsertMenuItem(nIDNewItem, &info);
}

void CIconMenu::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if ((lpDrawItemStruct==NULL)||(lpDrawItemStruct->CtlType != ODT_MENU))
		return;		//not for a menu
	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(icons[lpDrawItemStruct->itemID]), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	if (hIcon == NULL)
		return;
	DrawIconEx(lpDrawItemStruct->hDC,
		lpDrawItemStruct->rcItem.left - 16,
		lpDrawItemStruct->rcItem.top + (lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - 16) / 2,
		hIcon, 16, 16,
		0, NULL, DI_NORMAL);
	DestroyIcon(hIcon);
}

void CIconMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (lpMeasureItemStruct==NULL)
		return;
	lpMeasureItemStruct->itemWidth += 2;
	if (lpMeasureItemStruct->itemHeight < 16)
		lpMeasureItemStruct->itemHeight = 16;
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

