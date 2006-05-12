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
#include "PicWindow.h"
#include "TortoiseIDiff.h"

class CMainWindow : public CWindow
{
public:
	CMainWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) : CWindow(hInst, wcx)
		, picWindow1(hInst)
		, picWindow2(hInst)
		, oldx(-4)
		, bMoved(false)
		, bDragMode(false)
		, nSplitterPos(100)
		, nSplitterBorder(4)
	{ 
		SetWindowTitle((LPCTSTR)ResString(hInstance, IDS_APP_TITLE));
	};

	bool RegisterAndCreateWindow();

	void SetLeft(stdstring leftpath, stdstring lefttitle) {leftpicpath=leftpath; leftpictitle=lefttitle;}
	void SetRight(stdstring rightpath, stdstring righttitle) {rightpicpath=rightpath; rightpictitle=righttitle;}

protected:
	// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void				PositionChildren(RECT * clientrect = NULL);
	void				DoCommand(int id);

	// splitter methods
	void				DrawXorBar(HDC hdc, int x1, int y1, int width, int height);
	LRESULT				Splitter_OnLButtonDown(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT				Splitter_OnLButtonUp(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT				Splitter_OnMouseMove(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	// command line params
	stdstring			leftpicpath;
	stdstring			leftpictitle;

	stdstring			rightpicpath;
	stdstring			rightpictitle;

	// image data
	CPicWindow		picWindow1;
	CPicWindow		picWindow2;

	// splitter data
	int				oldx;
	bool			bMoved;
	bool			bDragMode;
	int				nSplitterPos;
	int				nSplitterBorder;


};

