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

#define SLIDER_HEIGHT 30
#define SPLITTER_BORDER 2
#define TRACKBAR_ID 101

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
		, bOverlap(false)
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
	LRESULT				DoCommand(int id);
	HWND				CreateTrackbar(HWND hwndParent, UINT iMin, UINT iMax);
	bool				OpenDialog();
	static BOOL CALLBACK OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static bool			AskForFile(HWND owner, TCHAR * path);

	// splitter methods
	void				DrawXorBar(HDC hdc, int x1, int y1, int width, int height);
	LRESULT				Splitter_OnLButtonDown(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT				Splitter_OnLButtonUp(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT				Splitter_OnMouseMove(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	// command line params
	static stdstring	leftpicpath;
	static stdstring	leftpictitle;

	static stdstring	rightpicpath;
	static stdstring	rightpictitle;

	// image data
	CPicWindow		picWindow1;
	CPicWindow		picWindow2;

	// splitter data
	int				oldx;
	bool			bMoved;
	bool			bDragMode;
	int				nSplitterPos;

	// one/two pane view
	bool			bOverlap;
	HWND			hTrackbar;

};

