// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "IconStatic.h"
#include ".\iconstatic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CIconStatic::CIconStatic()
{
	m_strText = _T("");
	m_nIconID = 0;
}

CIconStatic::~CIconStatic()
{
	m_MemBMP.DeleteObject();
}


BEGIN_MESSAGE_MAP(CIconStatic, CStatic)
		ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

bool CIconStatic::Init(UINT nIconID)
{
	m_nIconID = nIconID;

	CString strText;	
	GetWindowText(strText);
	SetWindowText(_T(""));
	if(strText != _T(""))
		m_strText = strText;

	CRect rRect;
	GetClientRect(rRect);

	CDC *pDC = GetDC();
	CDC MemDC;
	CBitmap *pOldBMP;
	
	MemDC.CreateCompatibleDC(pDC);

	CFont *pOldFont = MemDC.SelectObject(GetFont());

	CRect rCaption(0,0,0,0);
	MemDC.DrawText(m_strText, rCaption, DT_CALCRECT);
	if(rCaption.Height() < 16)
		rCaption.bottom = rCaption.top + 16;
	rCaption.right += 25;
	if(rCaption.Width() > rRect.Width() - 16)
		rCaption.right = rCaption.left + rRect.Width() - 16;

	m_MemBMP.DeleteObject();
	m_MemBMP.CreateCompatibleBitmap(pDC, rCaption.Width(), rCaption.Height());
	pOldBMP = MemDC.SelectObject(&m_MemBMP);
	
	HMODULE hThemeDll = LoadLibrary(_T("UxTheme.dll"));
	BOOL bThemeDrawn = FALSE;
	if (hThemeDll)
	{
		PFNISAPPTHEMED pfnIsAppThemed = (PFNISAPPTHEMED)GetProcAddress(hThemeDll, "IsAppThemed");
		if (pfnIsAppThemed)
		{
			if ((*pfnIsAppThemed)())
			{
				HRESULT rr;
				PFNDRAWTHEMEPARENTBACKGROUND pfnDrawThemeParentBackground = (PFNDRAWTHEMEPARENTBACKGROUND)GetProcAddress(hThemeDll, "DrawThemeParentBackground");
				if (pfnDrawThemeParentBackground)
					rr = (*pfnDrawThemeParentBackground)(this->m_hWnd, MemDC.m_hDC, NULL);
				else
					MemDC.FillSolidRect(rCaption, pDC->GetBkColor());
				
				DrawState( MemDC.m_hDC, NULL, NULL,
					(LPARAM)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(m_nIconID), IMAGE_ICON, 16, 16, LR_VGACOLOR | LR_SHARED), 
					NULL, 3, 0, 16, 16, DST_ICON | DSS_NORMAL);

				rCaption.left += 22;
				PFNOPENTHEMEDATA pfnOpenThemeData = (PFNOPENTHEMEDATA)GetProcAddress(hThemeDll, "OpenThemeData");
				if (pfnOpenThemeData)
				{
					HTHEME hTheme = (*pfnOpenThemeData)(NULL, L"Button");
					if (hTheme)
					{
						PFNDRAWTHEMETEXT pfn = (PFNDRAWTHEMETEXT)GetProcAddress(hThemeDll, "DrawThemeText");
						if (pfn)
						{
#ifdef UNICODE
							(*pfn)(hTheme, MemDC.m_hDC, 4, 1, m_strText, m_strText.GetLength(), 
									DT_WORDBREAK | DT_CENTER | DT_WORD_ELLIPSIS, NULL, &rCaption);
#else
							CStringW m_strTextW(m_strText);
							(*pfn)(hTheme, MemDC.m_hDC, 4, 1, m_strTextW, m_strTextW.GetLength(), 
									DT_WORDBREAK | DT_CENTER | DT_WORD_ELLIPSIS, NULL, &rCaption);
#endif							
							PFNCLOSETHEMEDATA pfnCloseThemeData = (PFNCLOSETHEMEDATA)GetProcAddress(hThemeDll, "CloseThemeData");
							if (pfnCloseThemeData)
								(*pfnCloseThemeData)(hTheme);
							bThemeDrawn = TRUE;
						} // if (pfn)
					} // if (hTheme)
				} // if (pfnOpenThemeData) 
			} // if ((*pfnIsAppThemed)()) 
		} // if (pfnIsAppThemed)
	} // if (hThemeDll)
	if (!bThemeDrawn)
	{
		MemDC.FillSolidRect(rCaption, GetSysColor(COLOR_BTNFACE));

		DrawState( MemDC.m_hDC, NULL, NULL,
			(LPARAM)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(m_nIconID), IMAGE_ICON, 16, 16, LR_VGACOLOR | LR_SHARED), 
			NULL, 3, 0, 16, 16, DST_ICON | DSS_NORMAL);

		rCaption.left += 22;
		MemDC.SetTextColor(pDC->GetTextColor());
		MemDC.DrawText(m_strText, rCaption, DT_SINGLELINE | DT_LEFT | DT_END_ELLIPSIS);
	}
	ReleaseDC( pDC );

	MemDC.SelectObject(pOldBMP);
	MemDC.SelectObject(pOldFont);
	
	if(m_wndPicture.m_hWnd == NULL)
		m_wndPicture.Create(NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, CRect(0,0,0,0), this);

	m_wndPicture.SetWindowPos(NULL, rRect.left+8, rRect.top, rCaption.Width()+22, rCaption.Height(), SWP_SHOWWINDOW);
	m_wndPicture.SetBitmap(m_MemBMP);

	CWnd *pParent = GetParent();
	if(pParent == NULL)
		pParent = GetDesktopWindow();
	
	CRect r;
	GetWindowRect(r);
	r.bottom = r.top + 20;
	GetParent()->ScreenToClient(&r);
	GetParent()->RedrawWindow(r);

	return true;
}

bool CIconStatic::SetText(CString strText)
{
	m_strText = strText;
	return Init(m_nIconID);
}

bool CIconStatic::SetIcon(UINT nIconID)
{
	return Init(nIconID);
}

void CIconStatic::OnSysColorChange() {
	Init(m_nIconID);
}
