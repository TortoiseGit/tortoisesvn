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

/**
 * The image view window.
 * Shows an image and provides methods to scale the image or alpha blend it
 * over another image.
 */
class CPicWindow : public CWindow
{
public:
	CPicWindow(HINSTANCE hInst, const WNDCLASSEX* wcx = NULL) : CWindow(hInst, wcx)
		, bValid(false)
		, bFirstpaint(true)
		, nHScrollPos(0)
		, nVScrollPos(0)
		, picscale(1.0)
		, pSecondPic(NULL)
		, alpha(255)
	{ 
		SetWindowTitle(_T("Picture Window"));
	};

	/// Registers the window class and creates the window
	bool RegisterAndCreateWindow(HWND hParent);

	/// Sets the image path and title to show
	void SetPic(stdstring path, stdstring title);
	/// Returns the CPicture image object. Used to get an already loaded image
	/// object without having to load it again.
	CPicture * GetPic() {return &picture;}
	/// Sets the path and title of the second image which is alpha blended over the original
	void SetSecondPic(CPicture * pPicture = NULL, const stdstring& sectit = _T(""), const stdstring& secpath = _T(""))
	{
		pSecondPic = pPicture;
		pictitle2 = sectit;
		picpath2 = secpath;
	}
	/// Returns the currently used alpha blending value (0-255)
	BYTE GetSecondPicAlpha() {return alpha;}
	/// Sets the alpha blending value
	void SetSecondPicAlpha(BYTE a) 
	{
		alpha = a; 
		InvalidateRect(*this, NULL, FALSE);
	}
	/// Resizes the image to fit into the window. Small images are not enlarged.
	void FitImageInWindow();
	/// Sets the zoom factor of the image
	void SetZoom(double dZoom);
	/// Returns the currently used zoom factor in which the image is shown.
	double GetZoom() {return picscale;}

protected:
	/// the message handler for this window
	LRESULT CALLBACK	WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	/// Draws the view title bar
	void				DrawViewTitle(HDC hDC, RECT * rect);
	/// Sets up the scrollbars as needed
	void				SetupScrollBars();
	/// Handles vertical scrolling
	void				OnVScroll(UINT nSBCode, UINT nPos);
	/// Handles horizontal scrolling
	void				OnHScroll(UINT nSBCode, UINT nPos);
	/// Returns the client rectangle, without the scrollbars and the view title.
	/// Basically the rectangle the image can use.
	void				GetClientRect(RECT * pRect);

	stdstring			picpath;			///< the path to the image we show
	stdstring			pictitle;			///< the string to show in the image view as a title
	CPicture			picture;			///< the picture object of the image
	bool				bValid;				///< true if the picture object is valid, i.e. if the image could be loaded and can be shown
	double				picscale;			///< the scale factor of the image
	bool				bFirstpaint;		///< true if the image is painted the first time. Used to initialize some stuff when the window is valid for sure.
	CPicture *			pSecondPic;			///< if set, this is the picture to draw transparently above the original
	stdstring 			pictitle2;			///< the title of the second picture
	stdstring 			picpath2;			///< the path of the second picture
	BYTE				alpha;				///< the alpha value for the transparency of the second picture
	// scrollbar info
	int					nVScrollPos;
	int					nHScrollPos;

};

