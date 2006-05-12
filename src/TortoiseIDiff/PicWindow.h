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
	{ 
		SetWindowTitle(_T("Picture Window"));
	};

	bool RegisterAndCreateWindow(HWND hParent);

	void SetPic(stdstring path, stdstring title);

protected:
	// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void				DrawViewTitle(HDC hDC, RECT * rect);

	stdstring			picpath;
	stdstring			pictitle;
	CPicture			picture;
	bool				bValid;
	int					charwidth;
};

