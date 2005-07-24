// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#include "SpellEdit.h"

#include "resource.h"
#include ".\spelledit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSpellEdit::CSpellEdit()
{
	pChecker = NULL;
	pThesaur = NULL;
	m_ErrColor = RGB(200, 0, 0);
	m_timer = 0;
	m_nMarginLine = 0;
}

CSpellEdit::~CSpellEdit()
{
	if (pChecker)
		delete pChecker;
	if (pThesaur)
		delete pThesaur;
}

BEGIN_MESSAGE_MAP(CSpellEdit, CEdit)
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


void CSpellEdit::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == m_timer)
	{
		SetTimer(m_timer, 4000, NULL);

		RECT clientRect;
		GetClientRect(&clientRect);

		CClientDC dc(this); // device context for painting
		dc.SelectObject((*GetFont()));
		CPen* pOldPen;
		SIZE sizei, sizeo;
		GetTextExtentPoint32(dc, _T("i"), 1, &sizei);
		GetTextExtentPoint32(dc, _T("O"), 1, &sizeo);
		if ((sizei.cx == sizeo.cx)&&(m_nMarginLine > 0))
		{
			CPen margpen(PS_SOLID, 0, ::GetSysColor(COLOR_GRAYTEXT));
			pOldPen = dc.SelectObject( &margpen );
			POINT pt = {0,0};
			RECT rect;
			SendMessage((UINT)EM_GETRECT, 0, (LPARAM)&rect);
			int pos = CharFromPos(pt);
			pos = LOWORD(pos) - LineIndex(HIWORD(pos));
			dc.MoveTo(m_nMarginLine*sizei.cx-(pos*sizei.cx)-rect.left, rect.top);
			dc.LineTo(m_nMarginLine*sizei.cx-(pos*sizei.cx)-rect.left, rect.bottom);
			dc.SelectObject(pOldPen);
		}

		if ((pThesaur==NULL)&&(pChecker==NULL))
			return;

		CPen mypen(PS_SOLID, 0, m_ErrColor);
		pOldPen = dc.SelectObject( &mypen );

		CRgn rgn;
		rgn.CreateRectRgnIndirect(&clientRect);
		dc.SelectObject(rgn);
		
		int nFirstVisible = GetFirstVisibleLine();
		int i = nFirstVisible;
		CPoint CharPos(0,0);
		do
		{
			int nLineLength = LineLength(LineIndex(i));
			if (nLineLength > 0)
			{
				TCHAR * pBuf = new TCHAR[nLineLength+2];
				ZeroMemory(pBuf, (nLineLength+2)*sizeof(TCHAR));
				GetLine(i, pBuf, nLineLength+1);
				int nLineIndex = LineIndex(i);
				CString line = CString(pBuf, nLineLength);
				int pos = -1;
				int oldpos = -1;

				// iterate over all words in the line
				do
				{
					pos++;
					oldpos = pos;
					pos = line.Find(' ', pos);

					CSize size;
					CString word;
					size = dc.GetTextExtent(line.Left(oldpos));
					CSize wordsize;
					if (pos >= 0)
					{
						word = line.Mid(oldpos, pos-oldpos);
						wordsize = dc.GetTextExtent(word);
					}
					else
					{
						word = line.Mid(oldpos, line.GetLength()-oldpos);
						wordsize = dc.GetTextExtent(word);
					}
					CStringA worda = CStringA(word);
					worda.Trim(".!_#$%&()* +,-/:;<=>[]\\^`{|}~\t \x0a\x0d\x01\'\"");

					if (!pChecker->spell(worda))
					{
						// underline misspelled words
						CharPos = PosFromChar(oldpos + nLineIndex);
						dc.MoveTo(CharPos.x, CharPos.y+wordsize.cy);
						dc.LineTo(CharPos.x+wordsize.cx, CharPos.y+wordsize.cy);
					}
					else // if (!pChecker->spell(worda))
					{    // Undo underlining of previously misspelled word    
						CPen eraserPen(PS_SOLID, 0, ::GetSysColor(COLOR_WINDOW));    
						CPen* errorPen = dc.SelectObject(&eraserPen);    
						CharPos = PosFromChar(oldpos + nLineIndex);    
						dc.MoveTo(CharPos.x, CharPos.y+wordsize.cy);    
						dc.LineTo(CharPos.x+wordsize.cx, CharPos.y+wordsize.cy);	    
						dc.SelectObject(errorPen);
					}				
				} while ((pos >= 0)&&(CharPos.x < clientRect.right));
				//underline words
				delete [] pBuf;
			} // if (nLineLength > 0) 
			i++;
		} while ((i < this->GetLineCount())&&(CharPos.y < clientRect.bottom));
		dc.SelectObject(pOldPen);
	} // if (nIDEvent == SPELLTIMER)
	CEdit::OnTimer(nIDEvent);
}

void CSpellEdit::PreSubclassWindow()
{
	TCHAR buf[MAX_PATH];
	CString sFolder;
	CString sFile;

	// look for dictionary files and use them if found
	if (GetModuleFileName(NULL, buf, MAX_PATH))
	{
		sFolder = CString(buf);
		sFolder = sFolder.Left(sFolder.ReverseFind('\\'));
		sFolder += _T("\\");
		long langId = GetUserDefaultLCID();

		do
		{
			GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, sizeof(buf));
			sFile = buf;
			sFile += _T("_");
			GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, sizeof(buf));
			sFile += buf;
			if ((PathFileExists(sFolder + sFile + _T(".aff"))) &&
				(PathFileExists(sFolder + sFile + _T(".dic"))))
			{
				if (pChecker)
					delete pChecker;
				pChecker = new MySpell(CStringA(sFolder + sFile + _T(".aff")), CStringA(sFolder + sFile + _T(".dic")));
			}
			if ((PathFileExists(sFolder + _T("dic\\") + sFile + _T(".aff"))) &&
				(PathFileExists(sFolder + _T("dic\\") + sFile + _T(".dic"))))
			{
				if (pChecker)
					delete pChecker;
				pChecker = new MySpell(CStringA(sFolder + _T("dic\\") + sFile + _T(".aff")), CStringA(sFolder + _T("dic\\") + sFile + _T(".dic")));
			}
#if 0
			if ((PathFileExists(sFolder + sFile + _T(".idx"))) &&
				(PathFileExists(sFolder + sFile + _T(".dat"))))
			{
				if (pThesaur)
					delete pThesaur;
				pThesaur = new MyThes(CStringA(sFolder + sFile + _T(".idx")), CStringA(sFolder + sFile + _T(".dat")));
			}
			if ((PathFileExists(sFolder + _T("dic\\") + sFile + _T(".idx"))) &&
				(PathFileExists(sFolder + _T("dic\\") + sFile + _T(".dat"))))
			{
				if (pThesaur)
					delete pThesaur;
				pThesaur = new MyThes(CStringA(sFolder + _T("dic\\") + sFile + _T(".idx")), CStringA(sFolder + _T("dic\\") + sFile + _T(".dat")));
			}
#endif
			DWORD lid = SUBLANGID(langId);
			lid--;
			if (lid > 0)
			{
				langId = MAKELANGID(PRIMARYLANGID(langId), lid);
			}
			else if (langId == 1033)
				langId = 0;
			else
				langId = 1033;
		} while ((langId)&&(pChecker==NULL));
	}

	int of = 0;
	do
	{
		m_timer = SetTimer(SPELLTIMER+of, 4000, NULL);
		of++;
	} while (m_timer == 0);

	CEdit::PreSubclassWindow();
}

void CSpellEdit::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if ((pThesaur==NULL)&&(pChecker==NULL))
		return CEdit::OnContextMenu(pWnd, point);

	SetFocus();
	CPoint clientpoint = point;
	ScreenToClient(&clientpoint);
	int n = CharFromPos(clientpoint);
	int nCharIndex = LOWORD(n);
	SetSel(nCharIndex, nCharIndex);
	CMenu menu;

	HINSTANCE hInstance = GetModuleHandle(_T("User32.dll"));
	ASSERT(hInstance);     
	HMENU hMenu = ::LoadMenu(        
		hInstance,              // handle to application instance
		MAKEINTRESOURCE(1));    // menu name string or menu-resource identifier
	// check if we actually found the menu
	if (hMenu)
	{
		// yes, we found one
		hMenu = GetSubMenu(hMenu, 0);
		menu.Attach(hMenu);
	}
	else
	{
		menu.CreatePopupMenu();
		BOOL bReadOnly = GetStyle() & ES_READONLY;
		DWORD flags = CanUndo() && !bReadOnly ? 0 : MF_GRAYED;
#ifdef IDS_SPELLEDIT_UNDO
		m_i18l.LoadString(IDS_SPELLEDIT_UNDO);
		menu.InsertMenu(0, MF_BYPOSITION | flags, EM_UNDO,
			m_i18l);
#else
		menu.InsertMenu(0, MF_BYPOSITION | flags, EM_UNDO,
			MES_UNDO);
#endif

		menu.InsertMenu(1, MF_BYPOSITION | MF_SEPARATOR);

		DWORD sel = GetSel();
		flags = LOWORD(sel) == HIWORD(sel) ? MF_GRAYED : 0;
#ifdef IDS_SPELLEDIT_COPY
		m_i18l.LoadString(IDS_SPELLEDIT_COPY);
		menu.InsertMenu(2, MF_BYPOSITION | flags, WM_COPY,
			m_i18l);
#else
		menu.InsertMenu(2, MF_BYPOSITION | flags, WM_COPY,
			MES_COPY);
#endif

		flags = (flags == MF_GRAYED || bReadOnly) ? MF_GRAYED : 0;
#ifdef IDS_SPELLEDIT_CUT
		m_i18l.LoadString(IDS_SPELLEDIT_CUT);
		menu.InsertMenu(2, MF_BYPOSITION | flags, WM_CUT,
			m_i18l);
#else
		menu.InsertMenu(2, MF_BYPOSITION | flags, WM_CUT,
			MES_CUT);
#endif
#ifdef IDS_SPELLEDIT_DELETE
		m_i18l.LoadString(IDS_SPELLEDIT_DELETE);
		menu.InsertMenu(4, MF_BYPOSITION | flags, WM_CLEAR,
			m_i18l);
#else
		menu.InsertMenu(4, MF_BYPOSITION | flags, WM_CLEAR,
			MES_DELETE);
#endif
		flags = IsClipboardFormatAvailable(CF_TEXT) &&
			!bReadOnly ? 0 : MF_GRAYED;
#ifdef IDS_SPELLEDIT_PASTE
		m_i18l.LoadString(IDS_SPELLEDIT_PASTE);
		menu.InsertMenu(4, MF_BYPOSITION | flags, WM_PASTE,
			m_i18l);
#else
		menu.InsertMenu(4, MF_BYPOSITION | flags, WM_PASTE,
			MES_PASTE);
#endif
		menu.InsertMenu(6, MF_BYPOSITION | MF_SEPARATOR);

		int len = GetWindowTextLength();
		flags = (!len || (LOWORD(sel) == 0 && HIWORD(sel) ==
			len)) ? MF_GRAYED : 0;
#ifdef IDS_SPELLEDIT_SELECTALL
		m_i18l.LoadString(IDS_SPELLEDIT_SELECTALL);
		menu.InsertMenu(7, MF_BYPOSITION | flags, ME_SELECTALL,
			m_i18l);
#else
		menu.InsertMenu(7, MF_BYPOSITION | flags, ME_SELECTALL,
			MES_SELECTALL);
#endif
	}

	//find the word under the cursor
	CString word;
	GetWindowText(word);
	int nStartChar, nEndChar;
	GetSel(nStartChar, nEndChar);

	while ((nStartChar >= 0)&&(word.GetAt(nStartChar) != ' ')&&(word.GetAt(nStartChar) != '\n'))
		nStartChar--;
	nStartChar++;
	word = word.Mid(nStartChar);
	int p = word.FindOneOf(_T(".!_#$%&()* +,-/:;<=>[]\\^`{|}~\t \x0a\x0d\x01\'\""));
	if (p>0)
		word = word.Left(p);
	nEndChar = nStartChar + word.GetLength();
	CStringA worda = CStringA(word);
	int nCorrections = 0;
	int nThesaurs = 0;
	CMenu thesaurs;
	thesaurs.CreatePopupMenu();
	CMenu corrections;
	corrections.CreatePopupMenu();

	// add the found corrections to the submenu
	if ((pChecker)&&(!worda.IsEmpty()))
	{
		char ** wlst;
		int ns = pChecker->suggest(&wlst,worda);
		if (ns > 0)
		{
			for (int i=0; i < ns; i++) 
			{
				CString sug = CString(wlst[i]);
				corrections.InsertMenu((UINT)-1, 0, i+1, sug);
				free(wlst[i]);
			} 
			free(wlst);
		}

		if ((ns > 0)&&(point.x >= 0))
		{
			menu.InsertMenu((UINT)-1, MF_BYPOSITION | MF_SEPARATOR);
#ifdef IDS_SPELLEDIT_CORRECTIONS
			m_i18l.LoadString(IDS_SPELLEDIT_CORRECTIONS);
			menu.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)corrections.m_hMenu, m_i18l);
#else
			menu.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)corrections.m_hMenu, _T("Corrections"));
#endif
			nCorrections = ns;
		}
	} // if (pChecker)

	// add found thesauri to submenu's
	CPtrArray menuArray;
	if ((pThesaur)&&(!worda.IsEmpty()))
	{
		mentry * pmean;
		worda.MakeLower();
		int count = pThesaur->Lookup(worda, worda.GetLength(),&pmean);
		int menuid = 50;		//offset (spell check menu items start with 1, thesaurus entries with 50)
		if (count)
		{
			mentry * pm = pmean;
			for (int  i=0; i < count; i++) 
			{
				CMenu * submenu = new CMenu();
				menuArray.Add(submenu);
				submenu->CreateMenu();
				for (int j=0; j < pm->count; j++) 
				{
					CString sug = CString(pm->psyns[j]);
					submenu->InsertMenu((UINT)-1, 0, menuid++, sug);
				}
				thesaurs.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)(submenu->m_hMenu), CString(pm->defn));
				pm++;
			}
		}  
		if ((count > 0)&&(point.x >= 0))
		{
#ifdef IDS_SPELLEDIT_THESAURUS
			m_i18l.LoadString(IDS_SPELLEDIT_THESAURUS);
			menu.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)thesaurs.m_hMenu, m_i18l);
#else
			menu.InsertMenu((UINT)-1, MF_POPUP, (UINT_PTR)thesaurs.m_hMenu, _T("Thesaurus"));
#endif
			nThesaurs = menuid;
		}

		pThesaur->CleanUpAfterLookup(&pmean, count);
	}
	if (point.x == -1 || point.y == -1)  
	{    
		CRect rc;    
		GetClientRect(&rc);    
		point = rc.CenterPoint();    
		ClientToScreen(&point);  
	}
	int nCmd = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | 	
		TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, this);  

	// delete the submenu's
	for (int i=0; i<menuArray.GetCount(); i++)
	{
		delete menuArray.GetAt(i);
	}

	if (nCmd < 0)      
		return;    
	switch (nCmd)    
	{    
	case EM_UNDO:    
	case WM_CUT:    
	case WM_COPY:    
	case WM_CLEAR:    
	case WM_PASTE:  
	case WM_UNDO:
		SendMessage(nCmd);  
		break;
	case ME_SELECTALL:        
		SendMessage(EM_SETSEL, 0, -1);  
		break;
	default:
		{
			if (nCmd)
			{
				if (nCmd <= nCorrections)
				{
					SetSel(nStartChar, nEndChar);
					CString temp;
					corrections.GetMenuString(nCmd, temp, 0);
					ReplaceSel(temp);
				} 
				else if (nCmd <= nThesaurs)
				{
					SetSel(nStartChar, nEndChar);
					CString temp;
					thesaurs.GetMenuString(nCmd, temp, 0);
					ReplaceSel(temp);
				}
				else
					SendMessage(WM_COMMAND, nCmd);
			}
			else
				SendMessage(WM_COMMAND, nCmd);
		}
	} // switch (nCmd)
	menu.DestroyMenu();    
}

void CSpellEdit::SetDictPaths(CString sAff, CString sDic)
{
	if (pChecker)
	{
		delete pChecker;
	}
	pChecker = new MySpell(CStringA(sAff), CStringA(sDic));
}

void CSpellEdit::SetThesaurPaths(CString sIdx, CString sDat)
{
	if (pThesaur)
	{
		delete pThesaur;
	}
	pThesaur = new MyThes(CStringA(sIdx), CStringA(sDat));
}

void CSpellEdit::WordWrap(BOOL wrap)
{
	// preserve original control's state. 
	CFont* pFont = GetFont(); 
	CString sSaveText;
	GetWindowText(sSaveText); 

	// create new edit control with appropriate style and size. 
	DWORD dwStyle = GetStyle() & ~(ES_AUTOHSCROLL|WS_HSCROLL|WS_VISIBLE); 
	if (!wrap) 
		dwStyle |= ES_AUTOHSCROLL|WS_HSCROLL; 

	CWnd* pParent = GetParent(); 
	CRect rect; 
	GetWindowRect(rect); 
	pParent->ScreenToClient(rect); 
	CWnd* pFocus = GetFocus(); 

	UINT_PTR nID = GetDlgCtrlID(); 

	HWND hWnd = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("edit"), NULL, dwStyle, 
		rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 
		pParent->m_hWnd, (HMENU)nID, AfxGetInstanceHandle(), NULL); 

	if (hWnd == NULL) 
	{ 
		return; 
	} 

	// set the window text to nothing to make sure following set doesn't fail 
	SetWindowText(NULL); 

	// restore visual state 
	::SetWindowText(hWnd, sSaveText); 
	if (pFont != NULL) 
	{ 
		ASSERT(pFont->m_hObject != NULL); 
		::SendMessage(hWnd, WM_SETFONT, (WPARAM)pFont->m_hObject, 0); 
	} 

	// detach old window, attach new 
	SetDlgCtrlID((UINT)nID+1); 
	HWND hWndOld = Detach(); 
	::SetWindowLongPtr(hWndOld, GWL_WNDPROC, (LONG_PTR)*GetSuperWndProcAddr()); 
	ASSERT(m_hWnd == NULL); 
	SubclassWindow(hWnd); 
	ASSERT(m_hWnd == hWnd); 

	GetClientRect(&rect); 
	SetWindowPos(NULL, 0, 0, 0, 0, 
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW); 
	SetWindowPos(NULL, 0, 0, 0, 0, 
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_DRAWFRAME); 
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE); 
	UpdateWindow(); 

	// destroy old 
	::SetWindowPos(hWndOld, NULL, 0, 0, 0, 0, 
		SWP_HIDEWINDOW|SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE| 
		SWP_NOZORDER); 
	::DestroyWindow(hWndOld); 

	if (pFocus == this) 
		SetFocus(); 

	ASSERT_VALID(this); 
	return; 

}
