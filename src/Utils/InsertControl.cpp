// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "stdafx.h"
#include "InsertControl.h"

static LPCTSTR g_szOldWndProc = _T("InsertControlOldProc");
static LPCTSTR g_szDataLeft = _T("InsertControlDataLeft");
static LPCTSTR g_szDataRight = _T("InsertControlDataRight");


struct Data
{
	UINT m_uControlWidth;
	UINT m_uControlHeigth;
	HWND m_hwndControl;
};

static LRESULT CALLBACK SubClassedWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC	WndProc = reinterpret_cast<WNDPROC>(::GetProp(hwnd, g_szOldWndProc));
	Data *pDataLeft = reinterpret_cast<Data *>(::GetProp(hwnd, g_szDataLeft));
	Data *pDataRight = reinterpret_cast<Data *>(::GetProp(hwnd, g_szDataRight));
	ASSERT(WndProc);

	switch (message)
	{
	case WM_NCDESTROY:
		{
			// Restore the old window procedure and clean up our props
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
			RemoveProp(hwnd, g_szOldWndProc);
			RemoveProp(hwnd, g_szDataLeft);
			RemoveProp(hwnd, g_szDataRight);
			// free allocated memory
			delete pDataLeft;
			delete pDataRight;
		}
		break;
	case WM_NCHITTEST:
		{
			// Call the original window proc so mouse clicks get through
			LRESULT lr = CallWindowProc(WndProc, hwnd, message, wParam, lParam);
			if (lr == HTNOWHERE)
			{
				lr = HTTRANSPARENT;
			}
			return lr;
		}
	case WM_NCCALCSIZE:
		{
			//	Adjust the client rect to fit into the edit control
			LRESULT lr = CallWindowProc(WndProc, hwnd, message, wParam, lParam);
			LPNCCALCSIZE_PARAMS lpnccs = reinterpret_cast<LPNCCALCSIZE_PARAMS>(lParam);
			if (pDataRight)
			{
				lpnccs->rgrc[0].right -= pDataRight->m_uControlWidth;
			}
			if (pDataLeft)
			{
				lpnccs->rgrc[0].left += pDataLeft->m_uControlWidth;
			}
			return lr;
		}
		break;
	case WM_SIZE:
	case WM_MOVE:
		{
			CRect rc;
			if (pDataRight)
			{
				::GetClientRect(hwnd, rc);
				rc.left = rc.right;
				rc.right = rc.left + pDataRight->m_uControlWidth;
				// center the control vertically
				rc.top += (((rc.bottom - rc.top) - pDataRight->m_uControlHeigth)/2);
				rc.bottom -= (((rc.bottom - rc.top) - pDataRight->m_uControlHeigth)/2);
				::MapWindowPoints(hwnd, GetParent(hwnd), (LPPOINT)&rc, 2);

				//	Move the control into the edit control, but don't adjust it's z-order
				::SetWindowPos(pDataRight->m_hwndControl, NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER);
			}
			if (pDataLeft)
			{
				::GetClientRect(hwnd, rc);
				rc.right = rc.left;
				rc.left = rc.left - pDataLeft->m_uControlWidth;
				// center the control vertically
				rc.top += (((rc.bottom - rc.top) - pDataLeft->m_uControlHeigth)/2);
				rc.bottom -= (((rc.bottom - rc.top) - pDataLeft->m_uControlHeigth)/2);
				::MapWindowPoints(hwnd, GetParent(hwnd), (LPPOINT)&rc, 2);

				//	Move the control into the edit control, but don't adjust it's z-order
				::SetWindowPos(pDataLeft->m_hwndControl, NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER);
			}

		}
		break;
	case WM_NCPAINT:
		{
			// the edit control should draw the borders first.
			CallWindowProc(WndProc, hwnd, message, wParam, lParam);
			
			RECT rect;
			if (pDataRight)
			{
				GetWindowRect(hwnd, &rect);
				OffsetRect(&rect, -rect.left, -rect.top);
				rect.left = rect.right;
				rect.left = rect.left - pDataRight->m_uControlWidth - 2;
				// erase the background of the NC area.
				HDC hdc = GetWindowDC(hwnd);
				FillRect(hdc, &rect, GetSysColorBrush(COLOR_WINDOW));    
				ReleaseDC(hwnd, hdc);

				// force a redraw of the inserted control			
				RedrawWindow(pDataRight->m_hwndControl, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_ERASENOW | RDW_ALLCHILDREN);
			}
			if (pDataLeft)
			{
				GetWindowRect(hwnd, &rect);
				OffsetRect(&rect, -rect.left, -rect.top);
				rect.right = rect.left;
				rect.right = rect.left + pDataLeft->m_uControlWidth + 2;
				// erase the background of the NC area.
				HDC hdc = GetWindowDC(hwnd);
				FillRect(hdc, &rect, GetSysColorBrush(COLOR_WINDOW));    
				ReleaseDC(hwnd, hdc);

				// force a redraw of the inserted control			
				RedrawWindow(pDataLeft->m_hwndControl, NULL, NULL, RDW_UPDATENOW | RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_ERASENOW | RDW_ALLCHILDREN);
			}
		}
		break;
	}

	return CallWindowProc(WndProc, hwnd, message, wParam, lParam);
}

bool InsertControl(HWND hwndEdit, HWND hwndControl, UINT uStyle)
{
	if (uStyle == INSERTCONTROL_LEFT || uStyle == INSERTCONTROL_RIGHT)
	{
		if (IsWindow(hwndEdit) && IsWindow(hwndControl))
		{
			// Check if we already have the control subclassed
			WNDPROC	WndProc = reinterpret_cast<WNDPROC>(::GetProp(hwndEdit, g_szOldWndProc));
			if (WndProc == 0)
			{
				// Subclass the edit control so we can catch the messages we need
				FARPROC lpfnWndProc = reinterpret_cast<FARPROC>(SetWindowLongPtr( hwndEdit, GWLP_WNDPROC, (LONG_PTR) SubClassedWndProc));
				ASSERT(lpfnWndProc != NULL);
				VERIFY(::SetProp(hwndEdit, g_szOldWndProc, reinterpret_cast<HANDLE>(lpfnWndProc)));
			}

			//	Create our data object. We later give this to our subclassed edit control so we can 
			Data *pData = new Data;

			CRect rcControl;
			::GetWindowRect(hwndControl, rcControl);

			pData->m_uControlWidth = rcControl.Width();
			pData->m_uControlHeigth = rcControl.Height();
			pData->m_hwndControl = hwndControl;

			if (uStyle == INSERTCONTROL_LEFT)
				VERIFY(::SetProp(hwndEdit, g_szDataLeft, reinterpret_cast<HANDLE>(pData)));
			else
				VERIFY(::SetProp(hwndEdit, g_szDataRight, reinterpret_cast<HANDLE>(pData)));

			// Make the edit control be aware of the changes to the NC area
			SetWindowPos(hwndEdit, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

			return TRUE;
		}
	}
	return FALSE;
}




