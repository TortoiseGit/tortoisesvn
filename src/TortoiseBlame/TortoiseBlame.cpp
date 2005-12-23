// TortoiseBlame - a Viewer for Subversion Blames

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

#include "stdafx.h"
#include "TortoiseBlame.h"
#include "registry.h"
#define MAX_LOADSTRING 100

#pragma warning(push)
#pragma warning(disable:4127)		// conditional expression is constant

// Global Variables:
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szOrigFilename[MAX_PATH];
TCHAR searchstringnotfound[MAX_LOADSTRING];

const bool ShowDate = false;
const bool ShowAuthor = true;
const bool ShowLine = true;

static TortoiseBlame app;

TortoiseBlame::TortoiseBlame()
{
	hInstance = 0;
	currentDialog = 0;
	wMain = 0;
	wEditor = 0;

	m_font = 0;
	m_blamewidth = 0;
	m_revwidth = 0;
	m_datewidth = 0;
	m_authorwidth = 0;
	m_linewidth = 0;

	m_windowcolor = ::GetSysColor(COLOR_WINDOW);
	m_textcolor = ::GetSysColor(COLOR_WINDOWTEXT);
	m_texthighlightcolor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_mouserevcolor = InterColor(m_windowcolor, m_textcolor, 20);
	m_mouseauthorcolor = InterColor(m_windowcolor, m_textcolor, 10);
	m_selectedrevcolor = ::GetSysColor(COLOR_HIGHLIGHT);
	m_selectedauthorcolor = InterColor(m_selectedrevcolor, m_texthighlightcolor, 35);

	m_directPointer = 0;
	m_directFunction = 0;
}

TortoiseBlame::~TortoiseBlame()
{
	if (m_font)
		DeleteObject(m_font);
}

// Return a colour which is interpolated between c1 and c2.
// Slider controls the relative proportions as a percentage:
// Slider = 0 	represents pure c1
// Slider = 50	represents equal mixture
// Slider = 100	represents pure c2
COLORREF TortoiseBlame::InterColor(COLORREF c1, COLORREF c2, int Slider)
{
	int r, g, b;
	
	// Limit Slider to 0..100% range
	if (Slider < 0)
		Slider = 0;
	if (Slider > 100)
		Slider = 100;
	
	// The colour components have to be treated individually.
	r = (GetRValue(c2) * Slider + GetRValue(c1) * (100 - Slider)) / 100;
	g = (GetGValue(c2) * Slider + GetGValue(c1) * (100 - Slider)) / 100;
	b = (GetBValue(c2) * Slider + GetBValue(c1) * (100 - Slider)) / 100;
	
	return RGB(r, g, b);
}

LRESULT TortoiseBlame::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_directFunction)
	{
		return ((SciFnDirect) m_directFunction)(m_directPointer, Msg, wParam, lParam);
	}
	return ::SendMessage(wEditor, Msg, wParam, lParam);	
}

void TortoiseBlame::GetRange(int start, int end, char *text) 
{
	TEXTRANGE tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;
	SendMessage(wEditor, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}

void TortoiseBlame::SetTitle() 
{
	char title[MAX_PATH + 100];
	strcpy_s(title, MAX_PATH + 100, szTitle);
	strcat_s(title, MAX_PATH + 100, " - ");
	strcat_s(title, MAX_PATH + 100, szOrigFilename);
	::SetWindowText(wMain, title);
}

BOOL TortoiseBlame::OpenLogFile(const char *fileName)
{
	char logmsgbuf[10000+1];
	FILE * File;
	fopen_s(&File, fileName, "rb");
	if (File == 0)
	{
		return FALSE;
	}
	LONG rev = 0;
	std::string msg;
	int slength = 0;
	int reallength = 0;
	size_t len = 0;
	wchar_t wbuf[MAX_LOG_LENGTH+4];
	for (;;)
	{
		len = fread(&rev, sizeof(LONG), 1, File);
		if (len == 0)
		{
			fclose(File);
			return TRUE;
		}
		len = fread(&slength, sizeof(int), 1, File);
		if (len == 0)
		{
			fclose(File);
			return FALSE;
		}
		if (slength > MAX_LOG_LENGTH)
		{
			reallength = slength;
			slength = MAX_LOG_LENGTH;
		}
		else
			reallength = 0;
		len = fread(logmsgbuf, sizeof(char), slength, File);
		if (len < (size_t)slength)
		{
			fclose(File);
			return FALSE;
		}
		msg = std::string(logmsgbuf, slength);
		if (reallength)
		{
			fseek(File, reallength-MAX_LOG_LENGTH, SEEK_CUR);
			msg = msg + _T("\n...");
		}
		int len = ::MultiByteToWideChar(CP_UTF8, NULL, msg.c_str(), -1, wbuf, MAX_LOG_LENGTH);
		wbuf[len] = 0;
		len = ::WideCharToMultiByte(CP_ACP, NULL, wbuf, len, logmsgbuf, MAX_LOG_LENGTH, NULL, NULL);
		logmsgbuf[len] = 0;
		msg = std::string(logmsgbuf);
		logmessages[rev] = msg;
	}
}

BOOL TortoiseBlame::OpenFile(const char *fileName) 
{
	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SetTitle();
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);
	::ShowWindow(wEditor, SW_HIDE);
	std::ifstream File;
	File.open(fileName);
	if (!File.good())
	{
		return FALSE;
	}
	TCHAR line[100*1024];
	TCHAR * lineptr = NULL;
	TCHAR * trimptr = NULL;
	//ignore the first two lines, they're of no interest to us
	File.getline(line, sizeof(line)/sizeof(TCHAR));
	File.getline(line, sizeof(line)/sizeof(TCHAR));
	do
	{
		File.getline(line, sizeof(line)/sizeof(TCHAR));
		if (File.gcount()>0)
		{
			lineptr = &line[7];
			revs.push_back(_ttol(lineptr));
			lineptr += 7;
			dates.push_back(std::string(lineptr, 30));
			lineptr += 31;
			trimptr = lineptr + 30;
			while (*trimptr == ' ')
			{
				*trimptr = 0;
				trimptr--;
			}
			authors.push_back(std::string(lineptr));
			lineptr += 31;
			SendEditor(SCI_ADDTEXT, _tcslen(lineptr), reinterpret_cast<LPARAM>(static_cast<char *>(lineptr)));
			SendEditor(SCI_ADDTEXT, 1, (LPARAM)_T("\n"));
		}
	} while (File.gcount() > 0);

	SendEditor(SCI_SETUNDOCOLLECTION, 1);
	::SetFocus(wEditor);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_GOTOPOS, 0);
	SendEditor(SCI_SETREADONLY, TRUE);

	//check which lexer to use, depending on the filetype
	SetupLexer(fileName);
	::ShowWindow(wEditor, SW_SHOW);
	m_blamewidth = 0;
	::InvalidateRect(wMain, NULL, TRUE);
	RECT rc;
	GetWindowRect(wMain, &rc);
	SetWindowPos(wMain, 0, rc.left, rc.top, rc.right-rc.left-1, rc.bottom - rc.top, 0);
	return TRUE;
}

void TortoiseBlame::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face) 
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	if (face) 
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}

void TortoiseBlame::InitialiseEditor() 
{
	m_directFunction = SendMessage(wEditor, SCI_GETDIRECTFUNCTION, 0, 0);
	m_directPointer = SendMessage(wEditor, SCI_GETDIRECTPOINTER, 0, 0);
	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(STYLE_DEFAULT, black, white, (DWORD)CRegStdWORD(_T("Software\\TortoiseMerge\\LogFontSize"), 10), 
		((stdstring)(CRegStdString(_T("Software\\TortoiseMerge\\LogFontName"), _T("Courier New")))).c_str());
	SendEditor(SCI_SETTABWIDTH, (DWORD)CRegStdWORD(_T("Software\\TortoiseMerge\\TabSize"), 4));
	SendEditor(SCI_SETREADONLY, TRUE);
	LRESULT pix = SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)_T("_99999"));
	if (ShowLine)
		SendEditor(SCI_SETMARGINWIDTHN, 0, pix);
	else
		SendEditor(SCI_SETMARGINWIDTHN, 0);
	SendEditor(SCI_SETMARGINWIDTHN, 1);
	SendEditor(SCI_SETMARGINWIDTHN, 2);
	//Set the default windows colors for edit controls
	SendEditor(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	SendEditor(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	SendEditor(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	SendEditor(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	SendEditor(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));
}

void TortoiseBlame::StartSearch()
{
	if (currentDialog)
		return;
	// Initialize FINDREPLACE
	ZeroMemory(&fr, sizeof(fr));
	fr.lStructSize = sizeof(fr);
	fr.hwndOwner = wMain;
	fr.lpstrFindWhat = szFindWhat;
	fr.wFindWhatLen = 80;
	fr.Flags = FR_HIDEUPDOWN | FR_HIDEWHOLEWORD;

	currentDialog = FindText(&fr);
}

bool TortoiseBlame::DoSearch(LPSTR what, DWORD flags)
{
	int pos = SendEditor(SCI_GETCURRENTPOS);
	int line = SendEditor(SCI_LINEFROMPOSITION, pos);
	bool bFound = false;
	bool bCaseSensitive = !!(flags & FR_MATCHCASE);

	if(!bCaseSensitive)
	{
		char *p;
		size_t len = strlen(what);
		for (p = what; p < what + len; p++)
		{
			if (isupper(*p)&&__isascii(*p))
				*p = _tolower(*p);
		}
	}

	std::string sWhat = std::string(what);
	
	char buf[20];
	int i=0;
	for (i=line; (i<(int)authors.size())&&(!bFound); ++i)
	{
		int bufsize = SendEditor(SCI_GETLINE, i);
		char * linebuf = new char[bufsize+1];
		ZeroMemory(linebuf, bufsize+1);
		SendEditor(SCI_GETLINE, i, (LPARAM)linebuf);
		if (!bCaseSensitive)
		{
			char *p;
			for (p = linebuf; p < linebuf + bufsize; p++)
			{
				if (isupper(*p)&&__isascii(*p))
					*p = _tolower(*p);
			}
		}
		_stprintf_s(buf, 20, _T("%ld"), revs[i]);
		if (authors[i].compare(sWhat)==0)
			bFound = true;
		else if ((!bCaseSensitive)&&(_stricmp(authors[i].c_str(), what)==0))
			bFound = true;
		else if (strcmp(buf, what) == 0)
			bFound = true;
		else if (strstr(linebuf, what))
			bFound = true;
		delete [] linebuf;
	}
	if (!bFound)
	{
		for (i=0; (i<line)&&(!bFound); ++i)
		{
			int bufsize = SendEditor(SCI_GETLINE, i);
			char * linebuf = new char[bufsize+1];
			ZeroMemory(linebuf, bufsize+1);
			SendEditor(SCI_GETLINE, i, (LPARAM)linebuf);
			if (!bCaseSensitive)
			{
				char *p;
				for (p = linebuf; p < linebuf + bufsize; p++)
				{
					if (isupper(*p)&&__isascii(*p))
						*p = _tolower(*p);
				}
			}
			_stprintf_s(buf, 20, _T("%ld"), revs[i]);
			if (authors[i].compare(sWhat)==0)
				bFound = true;
			else if ((!bCaseSensitive)&&(_stricmp(authors[i].c_str(), what)==0))
				bFound = true;
			else if (strcmp(buf, what) == 0)
				bFound = true;
			else if (strstr(linebuf, what))
				bFound = true;
			delete [] linebuf;
		}
	}
	if (bFound)
	{
		--i;
		SendEditor(SCI_GOTOLINE, i);
		int selstart = SendEditor(SCI_GETCURRENTPOS);
		int selend = SendEditor(SCI_POSITIONFROMLINE, i+1);
		SendEditor(SCI_SETSELECTIONSTART, selstart);
		SendEditor(SCI_SETSELECTIONEND, selend);
		m_SelectedLine = i;
	}
	else
	{
		::MessageBox(wMain, searchstringnotfound, "TortoiseBlame", MB_ICONINFORMATION);
	}
	return true;
}

void TortoiseBlame::Notify(SCNotification *notification) 
{
	switch (notification->nmhdr.code) 
	{
	case SCN_SAVEPOINTREACHED:
		break;

	case SCN_SAVEPOINTLEFT:
		break;
	case SCN_PAINTED:
		InvalidateRect(wBlame, NULL, FALSE);
		break;
	}
}

void TortoiseBlame::Command(int id)
{
	switch (id) 
	{
	case IDM_EXIT:
		::PostQuitMessage(0);
		break;
	case ID_EDIT_FIND:
		StartSearch();
		break;
	default:
		break;
	};
}

LONG TortoiseBlame::GetBlameWidth()
{
	if (m_blamewidth)
		return m_blamewidth;
	LONG blamewidth = 0;
	SIZE width;
	CreateFont();
	HDC hDC = ::GetDC(wBlame);
	HFONT oldfont = (HFONT)::SelectObject(hDC, m_font);
	TCHAR buf[MAX_PATH];
	_stprintf_s(buf, MAX_PATH, _T("%8ld "), 88888888);
	::GetTextExtentPoint(hDC, buf, _tcslen(buf), &width);
	m_revwidth = width.cx + BLAMESPACE;
	blamewidth += m_revwidth;
	if (ShowDate)
	{
		_stprintf_s(buf, MAX_PATH, _T("%30s"), _T("31.08.2001 06:24:14"));
		::GetTextExtentPoint32(hDC, buf, _tcslen(buf), &width);
		m_datewidth = width.cx + BLAMESPACE;
		blamewidth += m_datewidth;
	}
	if (ShowAuthor)
	{
		SIZE maxwidth = {0};
		for (std::vector<std::string>::iterator I = authors.begin(); I != authors.end(); ++I)
		{
			::GetTextExtentPoint32(hDC, I->c_str(), I->size(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_authorwidth = maxwidth.cx + BLAMESPACE;
		blamewidth += m_authorwidth;
	}
	::SelectObject(hDC, oldfont);
	POINT pt = {blamewidth, 0};
	LPtoDP(hDC, &pt, 1);
	m_blamewidth = pt.x;
	ReleaseDC(wBlame, hDC);
	return m_blamewidth;
}

void TortoiseBlame::CreateFont()
{
	if (m_font)
		return;
	LOGFONT lf = {0};
	lf.lfWeight = 400;
	HDC hDC = ::GetDC(wBlame);
	lf.lfHeight = -MulDiv((DWORD)CRegStdWORD(_T("Software\\TortoiseMerge\\LogFontSize"), 10), GetDeviceCaps(hDC, LOGPIXELSY), 72);
	CRegStdString fontname = CRegStdString(_T("Software\\TortoiseMerge\\LogFontName"), _T("Courier New"));
	_tcscpy_s(lf.lfFaceName, 32, ((stdstring)fontname).c_str());
	m_font = ::CreateFontIndirect(&lf);
	ReleaseDC(wBlame, hDC);
}

void TortoiseBlame::DrawBlame(HDC hDC)
{
	if (hDC == NULL)
		return;
	if (m_font == NULL)
		return;
	HFONT oldfont = (HFONT)::SelectObject(hDC, m_font);
	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR linesonscreen = SendEditor(SCI_LINESONSCREEN);
	LONG_PTR heigth = SendEditor(SCI_TEXTHEIGHT);
	LONG_PTR Y = 0;
	TCHAR buf[MAX_PATH];
	RECT rc;
	BOOL sel = FALSE;
	GetClientRect(wBlame, &rc);
	for (LRESULT i=line; i<(line+linesonscreen); ++i)
	{
		sel = FALSE;
		if (i < (int)revs.size())
		{
			::SetBkColor(hDC, m_windowcolor);
			::SetTextColor(hDC, m_textcolor);
			if (authors[i].size()>0)
			{
				if (authors[i].compare(m_mouseauthor)==0)
					::SetBkColor(hDC, m_mouseauthorcolor);
				if (authors[i].compare(m_selectedauthor)==0)
				{
					::SetBkColor(hDC, m_selectedauthorcolor);
					::SetTextColor(hDC, m_texthighlightcolor);
					sel = TRUE;
				}
			}
			if ((revs[i] == m_mouserev)&&(!sel))
				::SetBkColor(hDC, m_mouserevcolor);
			if (revs[i] == m_selectedrev)
			{
				::SetBkColor(hDC, m_selectedrevcolor);
				::SetTextColor(hDC, m_texthighlightcolor);
			}
			_stprintf_s(buf, MAX_PATH, _T("%8ld       "), revs[i]);
			rc.right = rc.left + m_revwidth;
			::ExtTextOut(hDC, 0, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
			int Left = m_revwidth;
			if (ShowDate)
			{
				rc.right = rc.left + Left + m_datewidth;
				_stprintf_s(buf, MAX_PATH, _T("%30s            "), dates[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_datewidth;
			}
			if (ShowAuthor)
			{
				rc.right = rc.left + Left + m_authorwidth;
				_stprintf_s(buf, MAX_PATH, _T("%-30s            "), authors[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_authorwidth;
			}
			if ((i==m_SelectedLine)&&(currentDialog))
			{
				LOGBRUSH brush;
				brush.lbColor = m_textcolor;
				brush.lbHatch = 0;
				brush.lbStyle = BS_SOLID;
				HPEN pen = ExtCreatePen(PS_SOLID | PS_GEOMETRIC, 2, &brush, 0, NULL);
				HGDIOBJ hPenOld = SelectObject(hDC, pen);
				RECT rc2 = rc;
				rc2.top = Y;
				rc2.bottom = Y + heigth;
				::MoveToEx(hDC, rc2.left, rc2.top, NULL);
				::LineTo(hDC, rc2.right, rc2.top);
				::LineTo(hDC, rc2.right, rc2.bottom);
				::LineTo(hDC, rc2.left, rc2.bottom);
				::LineTo(hDC, rc2.left, rc2.top);
				SelectObject(hDC, hPenOld); 
				DeleteObject(pen); 
			}
			Y += heigth;
		}
		else
		{
			::SetBkColor(hDC, m_windowcolor);
			for (int i=0; i< MAX_PATH; ++i)
				buf[i]=' ';
			::ExtTextOut(hDC, 0, Y, ETO_CLIPPED, &rc, buf, MAX_PATH-1, 0);
			Y += heigth;
		}
	}
	::SelectObject(hDC, oldfont);
}

void TortoiseBlame::DrawHeader(HDC hDC)
{
	if (hDC == NULL)
		return;
	if (m_font == NULL)
		return;
	RECT rc;
	HFONT oldfont = (HFONT)::SelectObject(hDC, m_font);
	GetClientRect(wHeader, &rc);

	::SetBkColor(hDC, ::GetSysColor(COLOR_BTNFACE));

	::ExtTextOut(hDC, 0, 0, ETO_CLIPPED, &rc, _T("Revision"), 8, 0);
	int Left = m_revwidth;
	if (ShowDate)
	{
		::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, _T("Date"), 4, 0);
		Left += m_datewidth;
	}
	if (ShowAuthor)
	{
		::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, _T("Author"), 6, 0);
		Left += m_authorwidth;
	}
	::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, _T("Line"), 4, 0);

	::SelectObject(hDC, oldfont);
}

void TortoiseBlame::StringExpand(LPSTR str)
{
	char * cPos = str;
	do
	{
		cPos = strchr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos+1, cPos, strlen(cPos)*sizeof(char));
			*cPos = '\r';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
}
void TortoiseBlame::StringExpand(LPWSTR str)
{
	wchar_t * cPos = str;
	do
	{
		cPos = wcschr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos+1, cPos, wcslen(cPos)*sizeof(wchar_t));
			*cPos = '\r';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
}

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
ATOM				MyRegisterBlameClass(HINSTANCE hInstance);
ATOM				MyRegisterHeaderClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndBlameProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndHeaderProc(HWND, UINT, WPARAM, LPARAM);
UINT				uFindReplaceMsg;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE /*hPrevInstance*/,
                     LPTSTR    /*lpCmdLine*/,
                     int       nCmdShow)
{
	app.hInstance = hInstance;
	MSG msg;
	HACCEL hAccelTable;

	if (::LoadLibrary("SciLexer.DLL") == NULL)
		return FALSE;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TORTOISEBLAME, szWindowClass, MAX_LOADSTRING);
	LoadString(hInstance, IDS_SEARCHNOTFOUND, searchstringnotfound, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	MyRegisterBlameClass(hInstance);
	MyRegisterHeaderClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	ZeroMemory(szOrigFilename, MAX_PATH);
	char blamefile[MAX_PATH] = {0};
	char logfile[MAX_PATH] = {0};

	if (__argc > 1)
	{
		_tcscpy_s(blamefile, MAX_PATH, __argv[1]);
	}
	if (__argc > 2)
	{
		_tcscpy_s(logfile, MAX_PATH, __argv[2]);
	}
	if (__argc > 3)
	{
		_tcscpy_s(szOrigFilename, MAX_PATH, __argv[3]);
	}

	if (_tcslen(blamefile)==0)
	{
		MessageBox(NULL, _T("TortoiseBlame can't be started directly!"), _T("TortoiseBlame"), MB_ICONERROR);
		return 0;
	}

	app.SendEditor(SCI_SETCODEPAGE, GetACP());
	app.OpenLogFile(logfile);
	if (_tcslen(logfile)>0)
		app.OpenFile(blamefile);

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_TORTOISEBLAME);

	BOOL going = TRUE;
	msg.wParam = 0;
	while (going) 
	{
		going = GetMessage(&msg, NULL, 0, 0);
		if (app.currentDialog && going) 
		{
			if (!IsDialogMessage(app.currentDialog, &msg)) 
			{
				if (TranslateAccelerator(msg.hwnd, hAccelTable, &msg) == 0) 
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		} 
		else if (going) 
		{
			if (TranslateAccelerator(app.wMain, hAccelTable, &msg) == 0) 
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_TORTOISEBLAME);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_TORTOISEBLAME;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

ATOM MyRegisterBlameClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndBlameProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_TORTOISEBLAME);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseBlameBlame");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

ATOM MyRegisterHeaderClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndHeaderProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_TORTOISEBLAME);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseBlameHeader");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   app.wMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);   

   if (!app.wMain)
   {
      return FALSE;
   }

   ShowWindow(app.wMain, nCmdShow);
   UpdateWindow(app.wMain);

   //Create the tooltips

   INITCOMMONCONTROLSEX iccex; 
   app.hwndTT;                 // handle to the ToolTip control
   TOOLINFO ti;
   RECT rect;                  // for client area coordinates
   iccex.dwICC = ICC_WIN95_CLASSES;
   iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
   InitCommonControlsEx(&iccex);

   /* CREATE A TOOLTIP WINDOW */
   app.hwndTT = CreateWindowEx(WS_EX_TOPMOST,
	   TOOLTIPS_CLASS,
	   NULL,
	   WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
	   CW_USEDEFAULT,
	   CW_USEDEFAULT,
	   CW_USEDEFAULT,
	   CW_USEDEFAULT,
	   app.wBlame,
	   NULL,
	   app.hInstance,
	   NULL
	   );

   SetWindowPos(app.hwndTT,
	   HWND_TOPMOST,
	   0,
	   0,
	   0,
	   0,
	   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

   /* GET COORDINATES OF THE MAIN CLIENT AREA */
   GetClientRect (app.wBlame, &rect);

   /* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
   ti.cbSize = sizeof(TOOLINFO);
   ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;//TTF_SUBCLASS | TTF_PARSELINKS;
   ti.hwnd = app.wBlame;
   ti.hinst = app.hInstance;
   ti.uId = 0;
   ti.lpszText = LPSTR_TEXTCALLBACK;
   // ToolTip control will cover the whole window
   ti.rect.left = rect.left;    
   ti.rect.top = rect.top;
   ti.rect.right = rect.right;
   ti.rect.bottom = rect.bottom;

   /* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
   SendMessage(app.hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
   SendMessage(app.hwndTT, TTM_SETMAXTIPWIDTH, 0, 600);
   //SendMessage(app.hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELONG(50000, 0));
   //SendMessage(app.hwndTT, TTM_SETDELAYTIME, TTDT_RESHOW, MAKELONG(1000, 0));
   
   uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
   
   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == uFindReplaceMsg)
	{
		LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;

		// If the FR_DIALOGTERM flag is set, 
		// invalidate the handle identifying the dialog box. 
		if (lpfr->Flags & FR_DIALOGTERM)
		{
			app.currentDialog = NULL; 
			return 0; 
		} 
		if (lpfr->Flags & FR_FINDNEXT)
		{
			app.DoSearch(lpfr->lpstrFindWhat, lpfr->Flags);
		}
		return 0; 
	}
	switch (message) 
	{
	case WM_CREATE:
		app.wEditor = ::CreateWindow(
			"Scintilla",
			"Source",
			WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
			0, 0,
			100, 100,
			hWnd,
			0,
			app.hInstance,
			0);
		app.InitialiseEditor();
		::ShowWindow(app.wEditor, SW_SHOW);
		::SetFocus(app.wEditor);
		app.wBlame = ::CreateWindow(
			_T("TortoiseBlameBlame"), 
			_T("blame"), 
			WS_CHILD | WS_CLIPCHILDREN,
			CW_USEDEFAULT, 0, 
			CW_USEDEFAULT, 0, 
			hWnd, 
			NULL, 
			app.hInstance, 
			NULL);
		::ShowWindow(app.wBlame, SW_SHOW);
		app.wHeader = ::CreateWindow(
			_T("TortoiseBlameHeader"), 
			_T("header"), 
			WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
			CW_USEDEFAULT, 0, 
			CW_USEDEFAULT, 0, 
			hWnd, 
			NULL, 
			app.hInstance, 
			NULL);
		::ShowWindow(app.wHeader, SW_SHOW);
		return 0;

	case WM_SIZE:
		if (wParam != 1) 
		{
			RECT rc;
			RECT blamerc;
			RECT sourcerc;
			::GetClientRect(hWnd, &rc);
			::SetWindowPos(app.wHeader, 0, rc.left, rc.top, rc.right-rc.left, HEADER_HEIGHT, 0);
			rc.top += HEADER_HEIGHT;
			blamerc.left = rc.left;
			blamerc.top = rc.top;
			blamerc.right = app.GetBlameWidth() > (rc.right - rc.left) ? rc.right : app.GetBlameWidth() + rc.left;
			blamerc.bottom = rc.bottom;
			sourcerc.left = blamerc.right;
			sourcerc.top = rc.top;
			sourcerc.bottom = rc.bottom;
			sourcerc.right = rc.right;
			::SetWindowPos(app.wEditor, 0, sourcerc.left, sourcerc.top, sourcerc.right - sourcerc.left, sourcerc.bottom - sourcerc.top, 0);
			::SetWindowPos(app.wBlame, 0, blamerc.left, blamerc.top, blamerc.right - blamerc.left, blamerc.bottom - blamerc.top, 0);
		}
		return 0;

	case WM_COMMAND:
		app.Command(LOWORD(wParam));
		break;
	case WM_NOTIFY:
		app.Notify(reinterpret_cast<SCNotification *>(lParam));
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		::DestroyWindow(app.wEditor);
		::PostQuitMessage(0);
		return 0;
	case WM_SETFOCUS:
		::SetFocus(app.wBlame);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndBlameProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	TRACKMOUSEEVENT mevt;
	HDC hDC;
	switch (message) 
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hDC = BeginPaint(app.wBlame, &ps);
		app.DrawBlame(hDC);
		EndPaint(app.wBlame, &ps);
		break;
	case WM_COMMAND:
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case TTN_GETDISPINFO:
			{
				LPNMHDR pNMHDR = (LPNMHDR)lParam;
				NMTTDISPINFOA* pTTTA = (NMTTDISPINFOA*)pNMHDR;
				NMTTDISPINFOW* pTTTW = (NMTTDISPINFOW*)pNMHDR;
				POINT point;
				::GetCursorPos(&point);
				::ScreenToClient(app.wBlame, &point);
				LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
				LONG_PTR heigth = app.SendEditor(SCI_TEXTHEIGHT);
				line = line + (point.y/heigth);
				if (line >= (LONG)app.revs.size())
					break;
				LONG rev = app.revs[line];
				if (line >= (LONG)app.revs.size())
					break;

				ZeroMemory(app.m_szTip, sizeof(app.m_szTip));
				ZeroMemory(app.m_wszTip, sizeof(app.m_wszTip));
				std::map<LONG, std::string>::iterator iter;
				if ((iter = app.logmessages.find(rev)) != app.logmessages.end())
				{
					std::string msg;
					if (!ShowAuthor)
					{
						msg += app.authors[line];
					}
					if (!ShowDate)
					{
						if (!ShowAuthor) msg += "  ";
						msg += app.dates[line];
					}
					if (!ShowAuthor || !ShowDate)
						msg += '\n';
					msg += iter->second;
					if (pNMHDR->code == TTN_NEEDTEXTA)
					{
						lstrcpyn(app.m_szTip, msg.c_str(), MAX_LOG_LENGTH+5);
						app.StringExpand(app.m_szTip);
						pTTTA->lpszText = app.m_szTip;
					}
					else
					{
						pTTTW->lpszText = app.m_wszTip;
						::MultiByteToWideChar( CP_ACP , 0, msg.c_str(), -1, app.m_wszTip, MAX_LOG_LENGTH+5);
						app.StringExpand(app.m_wszTip);
					}
				}
			}
			break;
		}
		return 0;
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		return 0;
	case WM_MOUSELEAVE:
		app.m_mouserev = -2;
		app.m_mouseauthor.clear();
		app.ttVisible = FALSE;
		SendMessage(app.hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
		::InvalidateRect(app.wBlame, NULL, FALSE);
		break;
	case WM_MOUSEMOVE:
		{
			mevt.cbSize = sizeof(TRACKMOUSEEVENT);
			mevt.dwFlags = TME_LEAVE;
			mevt.dwHoverTime = HOVER_DEFAULT;
			mevt.hwndTrack = app.wBlame;
			::TrackMouseEvent(&mevt);
			POINT pt = {((int)(short)LOWORD(lParam)), ((int)(short)HIWORD(lParam))};
			ClientToScreen(app.wBlame, &pt);
			pt.x += 15;
			pt.y += 15;
			SendMessage(app.hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
			if (!app.ttVisible)
			{
				TOOLINFO ti;
				ti.cbSize = sizeof(TOOLINFO);
				ti.hwnd = app.wBlame;
				ti.uId = 0;
				SendMessage(app.hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
				app.ttVisible = TRUE;
			}
			int y = ((int)(short)HIWORD(lParam));
			LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
			LONG_PTR heigth = app.SendEditor(SCI_TEXTHEIGHT);
			line = line + (y/heigth);
			if (line < (LONG)app.revs.size())
			{
				if (app.authors[line].compare(app.m_mouseauthor) != 0)
				{
					app.m_mouseauthor = app.authors[line];
				}
				if (app.revs[line] != app.m_mouserev)
				{
					LONG revstore = app.m_mouserev;
					app.m_mouserev = app.revs[line];
					if (revstore != -2)
					{
						::InvalidateRect(app.wBlame, NULL, FALSE);
						SendMessage(app.hwndTT, TTM_UPDATE, 0, 0);
					}
				}
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			int y = ((int)(short)HIWORD(lParam));
			LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
			LONG_PTR heigth = app.SendEditor(SCI_TEXTHEIGHT);
			line = line + (y/heigth);
			if (line < (LONG)app.revs.size())
			{
				if (app.revs[line] != app.m_selectedrev)
				{
					app.m_selectedrev = app.revs[line];
					app.m_selectedauthor = app.authors[line];
				}
				else
				{
					app.m_selectedauthor.clear();
					app.m_selectedrev = -2;
				}
				::InvalidateRect(app.wBlame, NULL, FALSE);
			}
		}
		break;
	case WM_SETFOCUS:
		::SetFocus(app.wBlame);
		app.SendEditor(SCI_GRABFOCUS);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndHeaderProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	switch (message) 
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hDC = BeginPaint(app.wHeader, &ps);
		app.DrawHeader(hDC);
		EndPaint(app.wHeader, &ps);
		break;
	case WM_COMMAND:
		break;
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
#pragma warning(pop)
