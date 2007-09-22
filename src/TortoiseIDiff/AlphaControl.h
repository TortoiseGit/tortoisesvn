// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2007 - TortoiseSVN

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
#pragma once
#include <windows.h>
#include <tchar.h>

#define ALPHA_GETLEFTPOS WM_USER+1
#define ALPHA_GETRIGHTPOS WM_USER+2

const TCHAR sCAlphaControl[] = _T("CAlphaControl");

/**
 * \ingroup TortoiseIDiff
 * The alpha blending slider control shown on the left side in TortoiseIDiff
 * for overlap mode.
 */
class CAlphaControl
{
public:
	CAlphaControl(HWND hwndControl);

	/// Registers the custom control with Windows.  Call this once when the program starts.
	static void RegisterCustomControl();
	/// Static message handler to pass off to the non-static handler.
	static LRESULT CALLBACK stMsgHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	/// Message handler
	LRESULT MsgHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	/// Resize handler
	void Size(UINT x, UINT y);

	/// Position change handler
	void SetPos(LONG nVal, bool bRedraw);

	/// Paint handler
	LRESULT Paint();

	/// Mouse movement handler
	LRESULT MouseMove(UINT x, UINT y);

	/// Mouse click handlers
	LRESULT LButtonDown(UINT x, UINT y);
	LRESULT LButtonUp(UINT x, UINT y);

private:
	/// Convert a pixel height to the value the slider would be at that height
	inline int PixelToValue(int nPixel);
	/// Convert the value of the slider to the pixel height the threshold would be at
	inline int ValueToPixel(int nValue);
	/// Draw a marker
	void DrawMarker(HDC& hdc, bool bLeft, UINT nHeight);
	BOOL OnSetCursor(WPARAM wParam, LPARAM lParam);

	HWND hwndControl;		///< Handle to this control
	UINT nBorder;			///< Size of border around the guage
	UINT nMin;				///< Minimum value for slider
	UINT nMax;				///< Maximum value for slider
	UINT nCurr;				///< Current value for slider
	RECT rectGuage;			///< Rectangle where the guage is drawn
	UINT nThreshold;		///< The vertical height in the guage that shows the percentage
	UINT nLeftMarker;		///< Vertical height of the left marker
	UINT nLeftMarkerPixel;	///< Vertical pixel height of the left marker
	UINT nRightMarker;		///< Vertical height of the right marker
	UINT nRightMarkerPixel;	///< Vertical pixel height of the right marker
	UINT nMarkerHalfSize;	///< Half the width/height of a marker

	enum
	{
		TRACKING_NONE,
		TRACKING_VALUE,
		TRACKING_LEFT,
		TRACKING_RIGHT
	} eTracking;			///< Which, if any, elements are being tracked
};
