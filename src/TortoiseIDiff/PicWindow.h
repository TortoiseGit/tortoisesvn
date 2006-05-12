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
#pragma once
#include "BaseWindow.h"
#include "TortoiseIDiff.h"
#include "Picture.h"

#define HEADER_HEIGHT 30

class CPicWindow : public CWindow
{
public:
	CPicWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) : CWindow(hInst, wcx)
		, bValid(false)
		, bFirstpaint(true)
		, nHScrollPos(0)
		, nVScrollPos(0)
		, picscale(1.0)
	{ 
		SetWindowTitle(_T("Picture Window"));
	};

	bool RegisterAndCreateWindow(HWND hParent);

	void SetPic(stdstring path, stdstring title);

protected:
	// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void				DrawViewTitle(HDC hDC, RECT * rect);
	void				SetupScrollBars();
	void				OnVScroll(UINT nSBCode, UINT nPos);
	void				OnHScroll(UINT nSBCode, UINT nPos);
	void				GetClientRect(RECT * pRect);

	stdstring			picpath;			///< the path to the image we show
	stdstring			pictitle;			///< the string to show in the image view as a title
	CPicture			picture;			///< the picture object of the image
	bool				bValid;				///< true if the picture object is valid, i.e. if the image could be loaded and can be shown
	double				picscale;			///< the scale factor of the image
	bool				bFirstpaint;			///< true if the image is painted the first time. Used to initialize some stuff when the window is valid for sure.
	// scrollbar info
	int					nVScrollPos;
	int					nHScrollPos;

};

