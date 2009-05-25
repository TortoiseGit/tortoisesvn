// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
#include "LinkControl.h"


#define PROP_OBJECT_PTR			MAKEINTATOM(ga.atom)
#define PROP_ORIGINAL_PROC		MAKEINTATOM(ga.atom)

class CGlobalAtom
{
public:
	CGlobalAtom(void)
	{ atom = GlobalAddAtom(TEXT("_LinkControl_Object_Pointer_")
			 TEXT("\\{62671D58-E5E8-46e0-A818-FD6547EC60B8}")); }
	~CGlobalAtom(void)
	{ DeleteAtom(atom); }

	ATOM atom;
};

static CGlobalAtom ga;

const UINT CLinkControl::LK_LINKITEMCLICKED
= ::RegisterWindowMessage(_T("LK_LINKITEMCLICKED"));


HCURSOR  CLinkControl::g_hLinkCursor     = NULL;
HFONT    CLinkControl::g_UnderlineFont   = NULL;
int      CLinkControl::g_counter         = 0;


CLinkControl::CLinkControl(void)
	: m_bOverControl(false)
	, m_StdFont(NULL)
	, m_pfnOrigCtlProc(NULL)
{
}

CLinkControl::~CLinkControl(void)
{
}

bool CLinkControl::ConvertStaticToLink(HWND hwndCtl)
{
	// Subclass the parent so we can draw the controls ourselves
	HWND hwndParent = GetParent(hwndCtl);
	if (hwndParent)
	{
		WNDPROC pfnOrigProc = (WNDPROC) GetWindowLongPtr(hwndParent, GWLP_WNDPROC);
		if (pfnOrigProc != _HyperlinkParentProc)
		{
			SetProp(hwndParent, PROP_ORIGINAL_PROC, (HANDLE)pfnOrigProc);
			SetWindowLongPtr(hwndParent, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)_HyperlinkParentProc);
		}
	}

	// Make sure the control will send notifications.

	LONG_PTR Style = GetWindowLongPtr(hwndCtl, GWL_STYLE);
	SetWindowLongPtr(hwndCtl, GWL_STYLE, Style | SS_NOTIFY);

	// Create an updated font by adding an underline.

	m_StdFont = (HFONT) SendMessage(hwndCtl, WM_GETFONT, 0, 0);

	if (g_counter++ == 0)
	{
		createGlobalResources();
	}

	// Subclass the existing control.

	m_pfnOrigCtlProc = (WNDPROC)GetWindowLongPtr(hwndCtl, GWLP_WNDPROC);
	SetProp(hwndCtl, PROP_OBJECT_PTR, (HANDLE)this);
	SetWindowLongPtr(hwndCtl, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)_HyperlinkProc);

	return true;
}

bool CLinkControl::ConvertStaticToLink(HWND hwndParent, UINT uiCtlId)
{
	return ConvertStaticToLink(GetDlgItem(hwndParent, uiCtlId));
}

LRESULT CALLBACK CLinkControl::_HyperlinkParentProc(HWND hwnd, UINT message,
												 WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnOrigProc = (WNDPROC)GetProp(hwnd, PROP_ORIGINAL_PROC);

	switch (message)
	{
	case WM_DESTROY:
		{
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) pfnOrigProc);
			RemoveProp(hwnd, PROP_ORIGINAL_PROC);
		}
		break;
	}
	return CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
}

void CLinkControl::DrawFocusRect(HWND hwnd)
{
	HWND hwndParent = ::GetParent(hwnd);

	if (hwndParent)
	{
		// calculate where to draw focus rectangle, in screen coords
		RECT rc;
		GetWindowRect(hwnd, &rc);

		InflateRect(&rc, 1, 1);					 // add one pixel all around
		::ScreenToClient(hwndParent, (LPPOINT)&rc);
		::ScreenToClient(hwndParent, ((LPPOINT)&rc) + 1);		 
		HDC dcParent = GetDC(hwndParent);
		::DrawFocusRect(dcParent, &rc);
		ReleaseDC(hwndParent, dcParent);
	}
}

LRESULT CALLBACK CLinkControl::_HyperlinkProc(HWND hwnd, UINT message,
										   WPARAM wParam, LPARAM lParam)
{
	CLinkControl *pHyperLink = (CLinkControl *)GetProp(hwnd, PROP_OBJECT_PTR);

	switch (message)
	{
	case WM_MOUSEMOVE:
		{
			if (pHyperLink->m_bOverControl)
			{
				RECT rect;
				GetClientRect(hwnd, &rect);

				POINT pt = { LOWORD(lParam), HIWORD(lParam) };

				if (!PtInRect(&rect, pt))
					ReleaseCapture();
			}
			else
			{
				pHyperLink->m_bOverControl = TRUE;
				SendMessage(hwnd, WM_SETFONT, (WPARAM)CLinkControl::g_UnderlineFont, FALSE);
				InvalidateRect(hwnd, NULL, FALSE);
				SetCapture(hwnd);
			}
		}
		break;
	case WM_SETCURSOR:
		{
			SetCursor(CLinkControl::g_hLinkCursor);
			return TRUE;
		}
		break;
	case WM_CAPTURECHANGED:
		{
			pHyperLink->m_bOverControl = FALSE;
			SendMessage(hwnd, WM_SETFONT, (WPARAM)pHyperLink->m_StdFont, FALSE);
			InvalidateRect(hwnd, NULL, FALSE);
		}
		break;
	case WM_KEYUP:
		{
			if (wParam != VK_SPACE)
				break;
		}
		// Fall through
	case WM_LBUTTONUP:
		{
			PostMessage(::GetParent(hwnd), LK_LINKITEMCLICKED, (WPARAM)hwnd, (LPARAM)0);
		}
		break;
	case WM_SETFOCUS:	
		// Fall through
	case WM_KILLFOCUS:
		{
			CLinkControl::DrawFocusRect(hwnd);
		}
		break;
	case WM_DESTROY:
		{
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)pHyperLink->m_pfnOrigCtlProc);

			SendMessage(hwnd, WM_SETFONT, (WPARAM)pHyperLink->m_StdFont, 0);

			if (--CLinkControl::g_counter <= 0)
				destroyGlobalResources();

			RemoveProp(hwnd, PROP_OBJECT_PTR);
		}
		break;
	}

	return CallWindowProc(pHyperLink->m_pfnOrigCtlProc, hwnd, message,
						  wParam, lParam);
}

void CLinkControl::createUnderlineFont(void)
{
	LOGFONT lf;
	GetObject(m_StdFont, sizeof(lf), &lf);
	lf.lfUnderline = TRUE;

	g_UnderlineFont = CreateFontIndirect(&lf);
}

void CLinkControl::createLinkCursor(void)
{
	g_hLinkCursor = ::LoadCursor(NULL, IDC_HAND); // Load Windows' hand cursor
	if (!g_hLinkCursor)    // if not available, use the standard Arrow cursor
	{
		g_hLinkCursor = ::LoadCursor(NULL, IDC_ARROW);
	}
}
