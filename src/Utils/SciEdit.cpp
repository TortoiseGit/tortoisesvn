// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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
#include "StdAfx.h"
#include ".\sciedit.h"

IMPLEMENT_DYNAMIC(CSciEdit, CWnd)

CSciEdit::CSciEdit(void)
{
	m_DirectFunction = 0;
	m_DirectPointer = 0;
	pChecker = 0;
	pThesaur = 0;
	m_hModule = ::LoadLibrary(_T("SciLexer.DLL"));
}

CSciEdit::~CSciEdit(void)
{
	if (m_hModule)
		::FreeLibrary(m_hModule);
	if (pChecker)
		delete pChecker;
	if (pThesaur)
		delete pThesaur;
}

void CSciEdit::Init()
{
	//Setup the direct access data
	m_DirectFunction = SendMessage(SCI_GETDIRECTFUNCTION, 0, 0);
	m_DirectPointer = SendMessage(SCI_GETDIRECTPOINTER, 0, 0);
	Call(SCI_SETMARGINWIDTHN, 1, 0);
	
	TCHAR buffer[11];
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE,(TCHAR *)buffer,10);
	buffer[10]=0;
	int codepage=0;
	codepage=_tstoi((TCHAR *)buffer);
	Call(SCI_SETCODEPAGE, codepage);
	
	//Setup the spell checker and thesaurus
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
}

LRESULT CSciEdit::Call(UINT message, WPARAM wParam, LPARAM lParam)
{
	ASSERT(::IsWindow(m_hWnd)); //Window must be valid
	ASSERT(m_DirectFunction); //Direct function must be valid
	return ((SciFnDirect) m_DirectFunction)(m_DirectPointer, message, wParam, lParam);
}

void CSciEdit::SetText(const CString& sText)
{
	CStringA sTextA = CStringA(sText);
	Call(SCI_SETTEXT, 0, (LPARAM)(LPCSTR)sTextA);
}

CString CSciEdit::GetText()
{
	LRESULT len = Call(SCI_GETLENGTH);
	CStringA sTextA;
	Call(SCI_GETTEXT, len+1, (LPARAM)(LPCSTR)sTextA.GetBuffer(len+1));
	sTextA.ReleaseBuffer();
	CString sText = CString(sTextA);
	return sText;
}

CString CSciEdit::GetWordUnderCursor(bool bSelectWord)
{
	char textbuffer[100];
	TEXTRANGEA textrange;
	textrange.lpstrText = textbuffer;	
	int pos = Call(SCI_GETCURRENTPOS);
	textrange.chrg.cpMin = Call(SCI_WORDSTARTPOSITION, pos, TRUE);
	textrange.chrg.cpMax = Call(SCI_WORDENDPOSITION, textrange.chrg.cpMin, TRUE);
	Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
	if (bSelectWord)
	{
		Call(SCI_SETSEL, textrange.chrg.cpMin, textrange.chrg.cpMax);
		Call(SCI_SETCURRENTPOS, textrange.chrg.cpMin);
	}
	return CString(textbuffer);
}

void CSciEdit::SetFont(CString sFontName, int iFontSizeInPoints)
{
	Call(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)(LPCSTR)CStringA(sFontName));
	Call(SCI_STYLESETSIZE, STYLE_DEFAULT, iFontSizeInPoints);
	Call(SCI_STYLECLEARALL);
}

void CSciEdit::SetAutoCompletionList(const CAutoCompletionList& list, const TCHAR separator)
{
	//copy the autocompletion list.
	
	//SK: instead of creating a copy of that list, we could accept a pointer
	//to the list and use that instead. But then the caller would have to make
	//sure that the list persists over the lifetime of the control!
	m_autolist.RemoveAll();
	for (INT_PTR i=0; i<list.GetCount(); ++i)
		m_autolist.Add(list[i]);
	m_separator = separator;
}

void CSciEdit::CheckSpelling()
{
	if (pChecker == NULL)
		return;
	
	char textbuffer[100];
	TEXTRANGEA textrange;
	textrange.lpstrText = textbuffer;	
	
	LRESULT firstline = Call(SCI_GETFIRSTVISIBLELINE);
	LRESULT lastline = firstline + Call(SCI_LINESONSCREEN);
	textrange.chrg.cpMin = Call(SCI_POSITIONFROMLINE, firstline);
	textrange.chrg.cpMax = textrange.chrg.cpMin;
	LRESULT lastpos = Call(SCI_POSITIONFROMLINE, lastline) + Call(SCI_LINELENGTH, lastline);
	if (lastpos < 0)
		lastpos = Call(SCI_GETLENGTH)-textrange.chrg.cpMin;
	while (textrange.chrg.cpMax < lastpos)
	{
		textrange.chrg.cpMin = Call(SCI_WORDSTARTPOSITION, textrange.chrg.cpMax+1, TRUE);
		if (textrange.chrg.cpMin < textrange.chrg.cpMax)
			break;
		textrange.chrg.cpMax = Call(SCI_WORDENDPOSITION, textrange.chrg.cpMin, TRUE);
		if (textrange.chrg.cpMin == textrange.chrg.cpMax)
		{
			textrange.chrg.cpMax++;
			continue;
		}
		if (textrange.chrg.cpMax - textrange.chrg.cpMin < 100)
		{
			Call(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);
			if (strlen(textrange.lpstrText) > 3)
			{
				if (!pChecker->spell(textrange.lpstrText))
				{
					//mark word as misspelled
					Call(SCI_STARTSTYLING, textrange.chrg.cpMin, INDICS_MASK);
					Call(SCI_SETSTYLING, textrange.chrg.cpMax - textrange.chrg.cpMin, INDIC1_MASK);	
				}
				else
				{
					//mark word as correct (remove the squiggle line)
					Call(SCI_STARTSTYLING, textrange.chrg.cpMin, INDICS_MASK);
					Call(SCI_SETSTYLING, textrange.chrg.cpMax - textrange.chrg.cpMin, 0);			
				}
			}
			else
			{
				textrange.chrg.cpMin = textrange.chrg.cpMax;
			}
		}
		else
		{
			textrange.chrg.cpMin = textrange.chrg.cpMax;
		}
	}
}

void CSciEdit::SuggestSpellingAlternatives()
{
	if (pChecker == NULL)
		return;
	CString word = GetWordUnderCursor(true);
	if (word.IsEmpty())
		return;
	char ** wlst;
	int ns = pChecker->suggest(&wlst, CStringA(word));
	if (ns > 0)
	{
		CString suggestions;
		for (int i=0; i < ns; i++) 
		{
			suggestions += CString(wlst[i]) + m_separator;
			free(wlst[i]);
		} 
		free(wlst);
		suggestions.TrimRight(m_separator);
		if (suggestions.IsEmpty())
			return;
		Call(SCI_AUTOCSETSEPARATOR, (WPARAM)CStringA(m_separator).GetAt(0));
		Call(SCI_AUTOCSHOW, 0, (LPARAM)(LPCSTR)CStringA(suggestions));
		Call(SCI_AUTOCSETDROPRESTOFWORD, 1);
	}

}

void CSciEdit::DoAutoCompletion()
{
	if (m_autolist.GetCount()==0)
		return;
	CString word = GetWordUnderCursor();
	if (word.GetLength() < 3)
		return;		//don't autocomplete yet, word is too short
	CString sAutoCompleteList;
	
	for (INT_PTR index = 0; index < m_autolist.GetCount(); ++index)
	{
		int compare = word.CompareNoCase(m_autolist[index].Left(word.GetLength()));
		if (compare>0)
			continue;
		else if (compare == 0)
		{
			sAutoCompleteList += m_autolist[index] + m_separator;
		}
		else
			break;
	}
	sAutoCompleteList.TrimRight(m_separator);
	if (sAutoCompleteList.IsEmpty())
		return;
	Call(SCI_AUTOCSETSEPARATOR, (WPARAM)CStringA(m_separator).GetAt(0));
	Call(SCI_AUTOCSHOW, word.GetLength(), (LPARAM)(LPCSTR)CStringA(sAutoCompleteList));
}

BOOL CSciEdit::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
	if (message != WM_NOTIFY)
		return CWnd::OnChildNotify(message, wParam, lParam, pLResult);
	
	LPNMHDR lpnmhdr = (LPNMHDR) lParam;

	if(lpnmhdr->hwndFrom==m_hWnd)
	{
		switch(lpnmhdr->code)
		{
		case SCN_CHARADDED:
			CheckSpelling();
			DoAutoCompletion();
			return TRUE;
			break;
		}
	}
	return CWnd::OnChildNotify(message, wParam, lParam, pLResult);
}

//////////////////////////////////////////////////////////////////////////

void CAutoCompletionList::AddSorted(const CString& elem, bool bNoDuplicates /*= true*/)
{
	if (GetCount()==0)
		return InsertAt(0, elem);
	
	int nMin = 0;
	int nMax = GetUpperBound();
	while (nMin <= nMax)
	{
		UINT nHit = (UINT)(nMin + nMax) >> 1; // fast devide by 2
		int cmp = elem.CompareNoCase(GetAt(nHit));

		if (cmp > 0)
			nMin = nHit + 1;
		else if (cmp < 0)
			nMax = nHit - 1;
		else if (bNoDuplicates)
			return; // already in the array
	}
	return InsertAt(nMin, elem);
}
BEGIN_MESSAGE_MAP(CSciEdit, CWnd)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

void CSciEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_TAB)
	{
		if (GetKeyState(VK_CONTROL)&0x8000)
		{
			//Ctrl-Tab was pressed, this means we should provide the user with
			//a list of possible spell checking alternatives to the word under
			//the cursor
			SuggestSpellingAlternatives();
			return;
		}
	}
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}
