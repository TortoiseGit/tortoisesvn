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

void CSciEdit::SetFont(CString sFontName, int iFontSizeInPoints)
{
	Call(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)(LPCSTR)CStringA(sFontName));
	Call(SCI_STYLESETSIZE, STYLE_DEFAULT, iFontSizeInPoints);
	Call(SCI_STYLECLEARALL);
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
			return TRUE;
			break;
		}
	}
	return CWnd::OnChildNotify(message, wParam, lParam, pLResult);
}
