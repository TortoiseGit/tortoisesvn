// TortoiseBlame - a Viewer for Subversion Blames

// Copyright (C) 2003-2010 - TortoiseSVN

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

#include "stdafx.h"
#include "CmdLineParser.h"
#include "TortoiseBlame.h"
#include "registry.h"
#include "LangDll.h"
#include "auto_buffer.h"
#include "CreateProcessHelper.h"
#include "UnicodeUtils.h"
#include <ClipboardHelper.h>

#include <algorithm>
#include <cctype>

#define MAX_LOADSTRING 1000

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma warning(push)
#pragma warning(disable:4127)       // conditional expression is constant

// Global Variables:
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
TCHAR szViewtitle[MAX_PATH];
TCHAR szOrigPath[MAX_PATH];
TCHAR searchstringnotfound[MAX_LOADSTRING];

const bool ShowDate = false;
const bool ShowAuthor = true;
const bool ShowLine = true;
bool ShowPath = false;

static TortoiseBlame app;
long TortoiseBlame::m_gotoLine = 0;

TortoiseBlame::TortoiseBlame()
{
    hInstance = 0;
    hResource = 0;
    currentDialog = 0;
    wMain = 0;
    wEditor = 0;
    wLocator = 0;

    m_font = 0;
    m_italicFont = 0;
    m_blameWidth = 0;
    m_revWidth = 0;
    m_dateWidth = 0;
    m_authorWidth = 0;
    m_pathWidth = 0;
    m_lineWidth = 0;

    m_windowColor = ::GetSysColor(COLOR_WINDOW);
    m_textColor = ::GetSysColor(COLOR_WINDOWTEXT);
    m_textHighLightColor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_mouseRevColor = InterColor(m_windowColor, m_textColor, 20);
    m_mouseAuthorColor = InterColor(m_windowColor, m_textColor, 10);
    m_selectedRevColor = ::GetSysColor(COLOR_HIGHLIGHT);
    m_selectedAuthorColor = InterColor(m_selectedRevColor, m_textHighLightColor, 35);
    m_mouseRev = -2;

    m_selectedRev = -1;
    m_selectedOrigRev = -1;
    m_selectedLine = -1;
    m_directPointer = 0;
    m_directFunction = 0;

    m_lowestRev = LONG_MAX;
    m_highestRev = 0;
    m_colorAge = true;
}

TortoiseBlame::~TortoiseBlame()
{
    if (m_font)
        DeleteObject(m_font);
    if (m_italicFont)
        DeleteObject(m_italicFont);
}

std::wstring TortoiseBlame::GetAppDirectory()
{
    std::wstring path;
    DWORD len = 0;
    DWORD bufferlen = MAX_PATH;     // MAX_PATH is not the limit here!
    do
    {
        bufferlen += MAX_PATH;      // MAX_PATH is not the limit here!
        auto_buffer<TCHAR> pBuf(bufferlen);
        len = GetModuleFileName(NULL, pBuf, bufferlen);
        path = std::wstring(pBuf, len);
    } while(len == bufferlen);
    path = path.substr(0, path.rfind('\\') + 1);

    return path;
}

// Return a color which is interpolated between c1 and c2.
// Slider controls the relative proportions as a percentage:
// Slider = 0   represents pure c1
// Slider = 50  represents equal mixture
// Slider = 100 represents pure c2
COLORREF TortoiseBlame::InterColor(COLORREF c1, COLORREF c2, int Slider)
{
    int r, g, b;

    // Limit Slider to 0..100% range
    if (Slider < 0)
        Slider = 0;
    if (Slider > 100)
        Slider = 100;

    // The color components have to be treated individually.
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

void TortoiseBlame::SetTitle()
{
    TCHAR title[MAX_PATH + 100];
    _tcscpy_s(title, MAX_PATH + 100, szTitle);
    _tcscat_s(title, MAX_PATH + 100, _T(" - "));
    _tcscat_s(title, MAX_PATH + 100, szViewtitle);
    ::SetWindowText(wMain, title);
}

BOOL TortoiseBlame::OpenLogFile(const TCHAR *fileName)
{
    FILE * File;
    _tfopen_s(&File, fileName, _T("rb"));
    if (File == 0)
    {
        return FALSE;
    }
    LONG rev = 0;
    std::wstring msg;
    int slength = 0;
    int reallength = 0;
    size_t len = 0;
    char * stringbuf;
    for (;;)
    {
        len = fread(&rev, sizeof(LONG), 1, File);
        if (len == 0)
        {
            fclose(File);
            InitSize();
            return TRUE;
        }
        len = fread(&slength, sizeof(int), 1, File);
        if (len == 0)
        {
            fclose(File);
            InitSize();
            return FALSE;
        }
        if (slength > MAX_LOG_LENGTH)
        {
            reallength = slength;
            slength = MAX_LOG_LENGTH;
        }
        else
            reallength = 0;
        stringbuf = new char[slength+1];
        len = fread(stringbuf, sizeof(char), slength, File);
        stringbuf[slength] = 0;
        msg = CUnicodeUtils::StdGetUnicode(stringbuf);
        if (reallength)
        {
            fseek(File, reallength-MAX_LOG_LENGTH, SEEK_CUR);
            msg = msg + _T("\n...");
        }
        m_logMessages[rev] = msg;
    }
}

BOOL TortoiseBlame::OpenFile(const TCHAR *fileName)
{
    SendEditor(SCI_SETREADONLY, FALSE);
    SendEditor(SCI_CLEARALL);
    SendEditor(EM_EMPTYUNDOBUFFER);
    SetTitle();
    SendEditor(SCI_SETSAVEPOINT);
    SendEditor(SCI_CANCEL);
    SendEditor(SCI_SETUNDOCOLLECTION, 0);
    ::ShowWindow(wEditor, SW_HIDE);

    FILE * File = NULL;
    int retrycount = 5;
    while (retrycount)
    {
        _tfopen_s(&File, fileName, _T("rb"));
        if (File == 0)
        {
            Sleep(500);
            retrycount--;
        }
        else
            break;
    }
    if (File == NULL)
        return FALSE;

    m_lowestRev = LONG_MAX;
    m_highestRev = 0;
    bool bUTF8 = true;
    size_t len = 0;
    LONG linenumber = 0;
    svn_revnum_t rev = 0;
    svn_revnum_t merged_rev = 0;
    int strLen = 0;
    for (;;)
    {
        // line number
        len = fread(&linenumber, sizeof(LONG), 1, File);
        if (len == 0)
            break;
        // revision
        len = fread(&rev, sizeof(svn_revnum_t), 1, File);
        if (len == 0)
            break;
        m_revs.push_back(rev);
        // author
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            auto_buffer<char> stringbuf(strLen+1);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_authors.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        // date
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            auto_buffer<char> stringbuf(strLen+1);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_dates.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        // merged revision
        len = fread(&merged_rev, sizeof(svn_revnum_t), 1, File);
        if (len == 0)
            break;
        m_mergedRevs.push_back(merged_rev);
        if ((merged_rev > 0)&&(merged_rev < rev))
        {
            m_lowestRev = min(m_lowestRev, merged_rev);
            m_highestRev = max(m_highestRev, merged_rev);
        }
        else
        {
            m_lowestRev = min(m_lowestRev, rev);
            m_highestRev = max(m_highestRev, rev);
        }
        // merged author
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            auto_buffer<char> stringbuf(strLen+1);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_mergedAuthors.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        // merged date
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            auto_buffer<char> stringbuf(strLen+1);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_mergedDates.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        // merged path
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            auto_buffer<char> stringbuf(strLen+1);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_mergedPaths.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        // text line
        len = fread(&strLen, sizeof(int), 1, File);
        if (strLen)
        {
            auto_buffer<char> stringbuf(strLen+1);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf.get()[strLen] = 0;
            char * lineptr = stringbuf.get();
            // in case we find an UTF8 BOM at the beginning of the line, we remove it
            if (((unsigned char)lineptr[0] == 0xEF)&&((unsigned char)lineptr[1] == 0xBB)&&((unsigned char)lineptr[2] == 0xBF))
            {
                lineptr += 3;
            }
            if (((unsigned char)lineptr[0] == 0xBB)&&((unsigned char)lineptr[1] == 0xEF)&&((unsigned char)lineptr[2] == 0xBF))
            {
                lineptr += 3;
            }
            // check each line for illegal utf8 sequences. If one is found, we treat
            // the file as ASCII, otherwise we assume an UTF8 file.
            char * utf8CheckBuf = lineptr;
            while ((bUTF8)&&(*utf8CheckBuf))
            {
                if ((*utf8CheckBuf == 0xC0)||(*utf8CheckBuf == 0xC1)||(*utf8CheckBuf >= 0xF5))
                {
                    bUTF8 = false;
                    break;
                }
                if ((*utf8CheckBuf & 0xE0)==0xC0)
                {
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0)!=0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                }
                if ((*utf8CheckBuf & 0xF0)==0xE0)
                {
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0)!=0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0)!=0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                }
                if ((*utf8CheckBuf & 0xF8)==0xF0)
                {
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0)!=0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0)!=0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0)!=0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                }

                utf8CheckBuf++;
            }
            SendEditor(SCI_ADDTEXT, strlen(lineptr), reinterpret_cast<LPARAM>(static_cast<char *>(lineptr)));
        }
        SendEditor(SCI_ADDTEXT, 2, (LPARAM)"\r\n");
    };

    fclose(File);

    if (bUTF8)
        SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);

    SendEditor(SCI_SETUNDOCOLLECTION, 1);
    ::SetFocus(wEditor);
    SendEditor(EM_EMPTYUNDOBUFFER);
    SendEditor(SCI_SETSAVEPOINT);
    SendEditor(SCI_GOTOPOS, 0);
    SendEditor(SCI_SETSCROLLWIDTHTRACKING, TRUE);
    SendEditor(SCI_SETREADONLY, TRUE);

    //check which lexer to use, depending on the filetype
    SetupLexer(fileName);
    ::ShowWindow(wEditor, SW_SHOW);
    m_blameWidth = 0;
    ::InvalidateRect(wMain, NULL, TRUE);
    RECT rc;
    GetWindowRect(wMain, &rc);
    SetWindowPos(wMain, 0, rc.left, rc.top, rc.right-rc.left-1, rc.bottom - rc.top, 0);

    if (m_mergedPaths.size() == 0)
    {
        HMENU hMenu = GetMenu(wMain);
        EnableMenuItem(hMenu, ID_VIEW_MERGEPATH, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
    }

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
    SetAStyle(STYLE_DEFAULT, black, white, (DWORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\BlameFontSize"), 10),
        (CUnicodeUtils::StdGetUTF8((tstring)(CRegStdString(_T("Software\\TortoiseSVN\\BlameFontName"), _T("Courier New"))))).c_str());
    SendEditor(SCI_SETTABWIDTH, (DWORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\BlameTabSize"), 4));
    SendEditor(SCI_SETREADONLY, TRUE);
    LRESULT pix = SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"_99999");
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
    m_regOldLinesColor = CRegStdDWORD(_T("Software\\TortoiseSVN\\BlameOldColor"), RGB(230, 230, 255));
    m_regNewLinesColor = CRegStdDWORD(_T("Software\\TortoiseSVN\\BlameNewColor"), RGB(255, 230, 230));
    m_regLocatorOldLinesColor = CRegStdDWORD(_T("Software\\TortoiseSVN\\BlameLocatorOldColor"), RGB(230, 230, 255));
    m_regLocatorNewLinesColor = CRegStdDWORD(_T("Software\\TortoiseSVN\\BlameLocatorNewColor"), RGB(230, 0, 0));
}

void TortoiseBlame::SelectLine(int yPos, bool bAlwaysSelect)
{
    LONG line = (LONG)app.SendEditor(SCI_GETFIRSTVISIBLELINE);
    LONG height = (LONG)app.SendEditor(SCI_TEXTHEIGHT);
    line = line + (yPos/height);
    if (line < (LONG)app.m_revs.size())
    {
        app.SetSelectedLine(line);
        if ((bAlwaysSelect)||(app.m_revs[line] != app.m_selectedRev))
        {
            app.m_selectedRev = app.m_revs[line];
            app.m_selectedOrigRev = app.m_revs[line];
            app.m_selectedAuthor = app.m_authors[line];
            app.m_selectedDate = app.m_dates[line];
        }
        else
        {
            app.m_selectedAuthor.clear();
            app.m_selectedDate.clear();
            app.m_selectedRev = -2;
            app.m_selectedOrigRev = -2;
        }
        ::InvalidateRect(app.wBlame, NULL, FALSE);
    }
    else
    {
        app.SetSelectedLine(-1);
    }
}

void TortoiseBlame::StartSearch()
{
    if (currentDialog)
        return;
    bool bCase = false;
    // Initialize FINDREPLACE
    if (m_fr.Flags & FR_MATCHCASE)
        bCase = true;
    SecureZeroMemory(&m_fr, sizeof(m_fr));
    m_fr.lStructSize = sizeof(m_fr);
    m_fr.hwndOwner = wMain;
    m_fr.lpstrFindWhat = m_szFindWhat;
    m_fr.wFindWhatLen = 80;
    m_fr.Flags = FR_HIDEUPDOWN | FR_HIDEWHOLEWORD;
    m_fr.Flags |= bCase ? FR_MATCHCASE : 0;

    currentDialog = FindText(&m_fr);
}

bool TortoiseBlame::DoSearch(LPTSTR what, DWORD flags)
{
    TCHAR szWhat[80];
    int pos = (int)SendEditor(SCI_GETCURRENTPOS);
    int line = (int)SendEditor(SCI_LINEFROMPOSITION, pos);
    bool bFound = false;
    bool bCaseSensitive = !!(flags & FR_MATCHCASE);

    _tcscpy_s(szWhat, sizeof(szWhat)/sizeof(TCHAR), what);

    if(!bCaseSensitive)
    {
        MakeLower(szWhat, _tcslen(szWhat));
    }

    tstring sWhat = tstring(szWhat);

    int textSelStart = 0;
    int textSelEnd = 0;
    TCHAR buf[20];
    int i=0;
    for (i=line; (i<(int)m_authors.size())&&(!bFound); ++i)
    {
        const int bufsize = (int)SendEditor(SCI_GETLINE, i);
        auto_buffer<char> linebuf(bufsize+1);
        SecureZeroMemory(linebuf, bufsize+1);
        SendEditor(SCI_GETLINE, i, (LPARAM)linebuf.get());
        tstring sLine = CUnicodeUtils::StdGetUnicode(linebuf.get());
        if (!bCaseSensitive)
        {
            std::transform(sLine.begin(), sLine.end(), sLine.begin(), std::tolower);
        }
        _stprintf_s(buf, 20, _T("%ld"), m_revs[i]);
        if (m_authors[i].compare(sWhat)==0)
            bFound = true;
        else if ((!bCaseSensitive)&&(_tcsicmp(m_authors[i].c_str(), szWhat)==0))
            bFound = true;
        else if (_tcscmp(buf, szWhat) == 0)
            bFound = true;
        else if (_tcsstr(sLine.c_str(), szWhat))
        {
            textSelStart = (int)SendEditor(SCI_POSITIONFROMLINE, i) + int(_tcsstr(sLine.c_str(), szWhat) - sLine.c_str());
            textSelEnd = textSelStart + (int)CUnicodeUtils::StdGetUTF8(szWhat).size();
            if ((line != i)||(textSelEnd != pos))
                bFound = true;
        }
    }
    if (!bFound)
    {
        for (i=0; (i<line)&&(!bFound); ++i)
        {
            const int bufsize = (int)SendEditor(SCI_GETLINE, i);
            auto_buffer<char> linebuf(bufsize+1);
            SecureZeroMemory(linebuf, bufsize+1);
            SendEditor(SCI_GETLINE, i, (LPARAM)linebuf.get());
            tstring sLine = CUnicodeUtils::StdGetUnicode(linebuf.get());
            if (!bCaseSensitive)
            {
                std::transform(sLine.begin(), sLine.end(), sLine.begin(), std::tolower);
            }
            _stprintf_s(buf, 20, _T("%ld"), m_revs[i]);
            if (m_authors[i].compare(sWhat)==0)
                bFound = true;
            else if ((!bCaseSensitive)&&(_tcsicmp(m_authors[i].c_str(), szWhat)==0))
                bFound = true;
            else if (_tcscmp(buf, szWhat) == 0)
                bFound = true;
            else if (_tcsstr(sLine.c_str(), szWhat))
            {
                bFound = true;
                textSelStart = (int)SendEditor(SCI_POSITIONFROMLINE, i) + int(_tcsstr(sLine.c_str(), szWhat) - sLine.c_str());
                textSelEnd = textSelStart + (int)CUnicodeUtils::StdGetUTF8(szWhat).size();
            }
        }
    }
    if (bFound)
    {
        GotoLine(i);
        if (textSelStart == textSelEnd)
        {
            int selstart = (int)SendEditor(SCI_GETCURRENTPOS);
            int selend = (int)SendEditor(SCI_POSITIONFROMLINE, i);
            SendEditor(SCI_SETSELECTIONSTART, selstart);
            SendEditor(SCI_SETSELECTIONEND, selend);
        }
        else
        {
            SendEditor(SCI_SETSELECTIONSTART, textSelStart);
            SendEditor(SCI_SETSELECTIONEND, textSelEnd);
        }
        m_selectedLine = i-1;
    }
    else
    {
        ::MessageBox(wMain, searchstringnotfound, _T("TortoiseBlame"), MB_ICONINFORMATION);
    }
    return true;
}

bool TortoiseBlame::GotoLine(long line)
{
    --line;
    if (line < 0)
        return false;
    if ((unsigned long)line >= m_authors.size())
    {
        line = (long)m_authors.size()-1;
    }

    int nCurrentPos = (int)SendEditor(SCI_GETCURRENTPOS);
    int nCurrentLine = (int)SendEditor(SCI_LINEFROMPOSITION,nCurrentPos);
    int nFirstVisibleLine = (int)SendEditor(SCI_GETFIRSTVISIBLELINE);
    int nLinesOnScreen = (int)SendEditor(SCI_LINESONSCREEN);

    if ( line>=nFirstVisibleLine && line<=nFirstVisibleLine+nLinesOnScreen)
    {
        // no need to scroll
        SendEditor(SCI_GOTOLINE, line);
    }
    else
    {
        // Place the requested line one third from the top
        if ( line > nCurrentLine )
        {
            SendEditor(SCI_GOTOLINE, (WPARAM)(line+(int)nLinesOnScreen*(2/3.0)));
        }
        else
        {
            SendEditor(SCI_GOTOLINE, (WPARAM)(line-(int)nLinesOnScreen*(1/3.0)));
        }
    }

    // Highlight the line
    int nPosStart = (int)SendEditor(SCI_POSITIONFROMLINE,line);
    int nPosEnd = (int)SendEditor(SCI_GETLINEENDPOSITION,line);
    SendEditor(SCI_SETSEL,nPosEnd,nPosStart);

    return true;
}

bool TortoiseBlame::ScrollToLine(long line)
{
    if (line < 0)
        return false;

    int nCurrentLine = (int)SendEditor(SCI_GETFIRSTVISIBLELINE);

    int scrolldelta = line - nCurrentLine;
    SendEditor(SCI_LINESCROLL, 0, scrolldelta);

    return true;
}

void TortoiseBlame::CopySelectedLogToClipboard()
{
    if (m_selectedRev <= 0)
        return;
    std::map<LONG, tstring>::iterator iter;
    if ((iter = app.m_logMessages.find(m_selectedRev)) != app.m_logMessages.end())
    {
        tstring msg;
        msg += m_selectedAuthor;
        msg += _T("  ");
        msg += app.m_selectedDate;
        msg += _T("\n");
        msg += iter->second;
        msg += _T("\n");
        CClipboardHelper clipboardHelper;
        if (!clipboardHelper.Open(app.wBlame))
            return;

        EmptyClipboard();
        HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc((msg.size() + 1)*sizeof(TCHAR));
        TCHAR* pchData = (TCHAR*)GlobalLock(hClipboardData);
        _tcscpy_s(pchData, msg.size()+1, msg.c_str());
        GlobalUnlock(hClipboardData);
        SetClipboardData(CF_UNICODETEXT,hClipboardData);
    }
}

void TortoiseBlame::BlamePreviousRevision()
{
    LONG nRevisionTo = m_selectedOrigRev - 1;
    if ( nRevisionTo<1 )
    {
        return;
    }

    // We now determine the smallest revision number in the blame file (but ignore "-1")
    // We do this for two reasons:
    // 1. we respect the "From revision" which the user entered
    // 2. we speed up the call of "svn blame" because previous smaller revision numbers don't have any effect on the result
    LONG nSmallestRevision = -1;
    for (LONG line=0;line<(LONG)app.m_revs.size();line++)
    {
        const LONG nRevision = app.m_revs[line];
        if ( nRevision > 0 )
        {
            if ( nSmallestRevision < 1 )
            {
                nSmallestRevision = nRevision;
            }
            else
            {
                nSmallestRevision = min(nSmallestRevision,nRevision);
            }
        }
    }

    TCHAR bufStartRev[20];
    _stprintf_s(bufStartRev, 20, _T("%d"), nSmallestRevision);

    TCHAR bufEndRev[20];
    _stprintf_s(bufEndRev, 20, _T("%d"), nRevisionTo);

    TCHAR bufLine[20];
    _stprintf_s(bufLine, 20, _T("%d"), m_selectedLine+1); //using the current line is a good guess.

    tstring svnCmd = _T(" /command:blame ");
    svnCmd += _T(" /path:\"");
    svnCmd += szOrigPath;
    svnCmd += _T("\"");
    svnCmd += _T(" /startrev:");
    svnCmd += bufStartRev;
    svnCmd += _T(" /endrev:");
    svnCmd += bufEndRev;
    svnCmd += _T(" /line:");
    svnCmd += bufLine;
    if (bIgnoreEOL)
        svnCmd += _T(" /ignoreeol");
    if (bIgnoreSpaces)
        svnCmd += _T(" /ignorespaces");
    if (bIgnoreAllSpaces)
        svnCmd += _T(" /ignoreallspaces");
    RunCommand(svnCmd);
}

void TortoiseBlame::DiffPreviousRevision()
{
    LONG nRevisionTo = m_selectedOrigRev;
    if ( nRevisionTo<1 )
    {
        return;
    }

    LONG nRevisionFrom = nRevisionTo-1;

    TCHAR bufStartRev[20];
    _stprintf_s(bufStartRev, 20, _T("%d"), nRevisionFrom);

    TCHAR bufEndRev[20];
    _stprintf_s(bufEndRev, 20, _T("%d"), nRevisionTo);

    tstring svnCmd = _T(" /command:diff ");
    svnCmd += _T(" /path:\"");
    svnCmd += szOrigPath;
    svnCmd += _T("\"");
    svnCmd += _T(" /startrev:");
    svnCmd += bufStartRev;
    svnCmd += _T(" /endrev:");
    svnCmd += bufEndRev;
    RunCommand(svnCmd);
}

void TortoiseBlame::ShowLog()
{
    TCHAR bufRev[20];
    _stprintf_s(bufRev, 20, _T("%d"), m_selectedOrigRev);

    tstring svnCmd = _T(" /command:log ");
    svnCmd += _T(" /path:\"");
    svnCmd += szOrigPath;
    svnCmd += _T("\"");
    svnCmd += _T(" /startrev:");
    svnCmd += bufRev;
    svnCmd += _T(" /pegrev:");
    svnCmd += bufRev;
    RunCommand(svnCmd);
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
        InvalidateRect(wLocator, NULL, FALSE);
        break;
    case SCN_GETBKCOLOR:
        if ((m_colorAge)&&(notification->line < (int)m_revs.size()))
        {
            notification->lParam = InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), (m_revs[notification->line]-m_lowestRev)*100/((m_highestRev-m_lowestRev)+1));
        }
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
    case ID_COPYTOCLIPBOARD:
        CopySelectedLogToClipboard();
        break;
    case ID_BLAME_PREVIOUS_REVISION:
        BlamePreviousRevision();
        break;
    case ID_DIFF_PREVIOUS_REVISION:
        DiffPreviousRevision();
        break;
    case ID_SHOWLOG:
        ShowLog();
        break;
    case ID_EDIT_GOTOLINE:
        GotoLineDlg();
        break;
    case ID_VIEW_COLORAGEOFLINES:
        {
            m_colorAge = !m_colorAge;
            HMENU hMenu = GetMenu(wMain);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= m_colorAge ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_COLORAGEOFLINES, uCheck);
            m_blameWidth = 0;
            InitSize();
        }
        break;
    case ID_VIEW_MERGEPATH:
        {
            bool bUseMerged = (m_mergedPaths.size() != 0);
            ShowPath = bUseMerged && !ShowPath;
            HMENU hMenu = GetMenu(wMain);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= ShowPath ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_MERGEPATH, uCheck);
            m_blameWidth = 0;
            InitSize();
        }
    default:
        break;
    };
}

void TortoiseBlame::GotoLineDlg()
{
    if (DialogBox(hResource, MAKEINTRESOURCE(IDD_GOTODLG), wMain, GotoDlgProc)==IDOK)
    {
        GotoLine(m_gotoLine);
    }
}

INT_PTR CALLBACK TortoiseBlame::GotoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                {
                    HWND hEditCtrl = GetDlgItem(hwndDlg, IDC_LINENUMBER);
                    if (hEditCtrl)
                    {
                        TCHAR buf[MAX_PATH];
                        if (::GetWindowText(hEditCtrl, buf, MAX_PATH))
                        {
                            m_gotoLine = _ttol(buf);
                        }

                    }
                }
            // fall through
            case IDCANCEL:
                EndDialog(hwndDlg, wParam);
                break;
            }
        }
        break;
    }
    return FALSE;
}

LONG TortoiseBlame::GetBlameWidth()
{
    if (m_blameWidth)
        return m_blameWidth;
    LONG blamewidth = 0;
    SIZE width;
    CreateFont();
    HDC hDC = ::GetDC(wBlame);
    HFONT oldfont = (HFONT)::SelectObject(hDC, m_font);
    TCHAR buf[MAX_PATH];
    _stprintf_s(buf, MAX_PATH, _T("%8ld "), 88888888);
    ::GetTextExtentPoint(hDC, buf, (int)_tcslen(buf), &width);
    m_revWidth = width.cx + BLAMESPACE;
    blamewidth += m_revWidth;
    if (ShowDate)
    {
        _stprintf_s(buf, MAX_PATH, _T("%30s"), _T("31.08.2001 06:24:14"));
        ::GetTextExtentPoint32(hDC, buf, (int)_tcslen(buf), &width);
        m_dateWidth = width.cx + BLAMESPACE;
        blamewidth += m_dateWidth;
    }
    if (ShowAuthor)
    {
        SIZE maxwidth = {0};
        for (std::vector<tstring>::iterator I = m_authors.begin(); I != m_authors.end(); ++I)
        {
            ::GetTextExtentPoint32(hDC, I->c_str(), (int)I->size(), &width);
            if (width.cx > maxwidth.cx)
                maxwidth = width;
        }
        m_authorWidth = maxwidth.cx + BLAMESPACE;
        blamewidth += m_authorWidth;
    }
    if (ShowPath)
    {
        SIZE maxwidth = {0};
        for (std::vector<tstring>::iterator I = m_mergedPaths.begin(); I != m_mergedPaths.end(); ++I)
        {
            ::GetTextExtentPoint32(hDC, I->c_str(), (int)I->size(), &width);
            if (width.cx > maxwidth.cx)
                maxwidth = width;
        }
        m_pathWidth = maxwidth.cx + BLAMESPACE;
        blamewidth += m_pathWidth;
    }
    ::SelectObject(hDC, oldfont);
    POINT pt = {blamewidth, 0};
    LPtoDP(hDC, &pt, 1);
    m_blameWidth = pt.x;
    ReleaseDC(wBlame, hDC);
    return m_blameWidth;
}

void TortoiseBlame::CreateFont()
{
    if (m_font)
        return;
    LOGFONT lf = {0};
    lf.lfWeight = 400;
    HDC hDC = ::GetDC(wBlame);
    lf.lfHeight = -MulDiv((DWORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\BlameFontSize"), 10), GetDeviceCaps(hDC, LOGPIXELSY), 72);
    lf.lfCharSet = DEFAULT_CHARSET;
    CRegStdString fontname = CRegStdString(_T("Software\\TortoiseSVN\\BlameFontName"), _T("Courier New"));
    _tcscpy_s(lf.lfFaceName, 32, ((tstring)fontname).c_str());
    m_font = ::CreateFontIndirect(&lf);

    lf.lfItalic = TRUE;
    m_italicFont = ::CreateFontIndirect(&lf);

    ReleaseDC(wBlame, hDC);
}

void TortoiseBlame::DrawBlame(HDC hDC)
{
    if (hDC == NULL)
        return;
    if (m_font == NULL)
        return;

    HFONT oldfont = NULL;
    LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
    LONG_PTR linesonscreen = SendEditor(SCI_LINESONSCREEN);
    LONG_PTR height = SendEditor(SCI_TEXTHEIGHT);
    LONG_PTR Y = 0;
    TCHAR buf[MAX_PATH];
    RECT rc;
    BOOL sel = FALSE;
    GetClientRect(wBlame, &rc);
    for (LRESULT i=line; i<(line+linesonscreen); ++i)
    {
        sel = FALSE;
        if (i < (int)m_revs.size())
        {
            bool bUseMerged = ((m_mergedRevs[i] > 0)&&(m_mergedRevs[i] < m_revs[i]));
            if (bUseMerged)
                oldfont = (HFONT)::SelectObject(hDC, m_italicFont);
            else
                oldfont = (HFONT)::SelectObject(hDC, m_font);
            ::SetBkColor(hDC, m_windowColor);
            ::SetTextColor(hDC, m_textColor);
            tstring author = bUseMerged ? m_mergedAuthors[i] : m_authors[i];
            if (author.size() > 0)
            {
                if (author.compare(m_mouseAuthor)==0)
                    ::SetBkColor(hDC, m_mouseAuthorColor);
                if (author.compare(m_selectedAuthor)==0)
                {
                    ::SetBkColor(hDC, m_selectedAuthorColor);
                    ::SetTextColor(hDC, m_textHighLightColor);
                    sel = TRUE;
                }
            }
            svn_revnum_t rev = bUseMerged ? m_mergedRevs[i] : m_revs[i];
            if ((rev == m_mouseRev)&&(!sel))
                ::SetBkColor(hDC, m_mouseRevColor);
            if (rev == m_selectedRev)
            {
                ::SetBkColor(hDC, m_selectedRevColor);
                ::SetTextColor(hDC, m_textHighLightColor);
            }
            _stprintf_s(buf, MAX_PATH, _T("%8ld       "), rev);
            rc.right = rc.left + m_revWidth;
            ::ExtTextOut(hDC, 0, (int)Y, ETO_CLIPPED, &rc, buf, (UINT)_tcslen(buf), 0);
            int Left = m_revWidth;
            if (ShowDate)
            {
                rc.right = rc.left + Left + m_dateWidth;
                _stprintf_s(buf, MAX_PATH, _T("%30s            "), bUseMerged ? m_mergedDates[i].c_str() : m_dates[i].c_str());
                ::ExtTextOut(hDC, Left, (int)Y, ETO_CLIPPED, &rc, buf, (UINT)_tcslen(buf), 0);
                Left += m_dateWidth;
            }
            if (ShowAuthor)
            {
                rc.right = rc.left + Left + m_authorWidth;
                _stprintf_s(buf, MAX_PATH, _T("%-30s            "), author.c_str());
                ::ExtTextOut(hDC, Left, (int)Y, ETO_CLIPPED, &rc, buf, (UINT)_tcslen(buf), 0);
                Left += m_authorWidth;
            }
            if ((ShowPath)&&(m_mergedPaths.size()))
            {
                rc.right = rc.left + Left + m_pathWidth;
                _stprintf_s(buf, MAX_PATH, _T("%-60s            "), m_mergedPaths[i].c_str());
                ::ExtTextOut(hDC, Left, (int)Y, ETO_CLIPPED, &rc, buf, (UINT)_tcslen(buf), 0);
                Left += m_authorWidth;
            }
            if ((i==m_selectedLine)&&(currentDialog))
            {
                LOGBRUSH brush;
                brush.lbColor = m_textColor;
                brush.lbHatch = 0;
                brush.lbStyle = BS_SOLID;
                HPEN pen = ExtCreatePen(PS_SOLID | PS_GEOMETRIC, 2, &brush, 0, NULL);
                HGDIOBJ hPenOld = SelectObject(hDC, pen);
                RECT rc2 = rc;
                rc2.top = (LONG)Y;
                rc2.bottom = (LONG)(Y + height);
                ::MoveToEx(hDC, rc2.left, rc2.top, NULL);
                ::LineTo(hDC, rc2.right, rc2.top);
                ::LineTo(hDC, rc2.right, rc2.bottom);
                ::LineTo(hDC, rc2.left, rc2.bottom);
                ::LineTo(hDC, rc2.left, rc2.top);
                SelectObject(hDC, hPenOld);
                DeleteObject(pen);
            }
            Y += height;
            ::SelectObject(hDC, oldfont);
        }
        else
        {
            ::SetBkColor(hDC, m_windowColor);
            std::fill_n(buf, MAX_PATH, _T(' '));
            ::ExtTextOut(hDC, 0, (int)Y, ETO_CLIPPED, &rc, buf, MAX_PATH-1, 0);
            Y += height;
        }
    }
}

void TortoiseBlame::DrawHeader(HDC hDC)
{
    if (hDC == NULL)
        return;

    RECT rc;
    HFONT oldfont = (HFONT)::SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
    GetClientRect(wHeader, &rc);

    ::SetBkColor(hDC, ::GetSysColor(COLOR_BTNFACE));

    TCHAR szText[MAX_LOADSTRING];
    LoadString(app.hResource, IDS_HEADER_REVISION, szText, MAX_LOADSTRING);
    ::ExtTextOut(hDC, LOCATOR_WIDTH, 0, ETO_CLIPPED, &rc, szText, (UINT)_tcslen(szText), 0);
    int Left = m_revWidth+LOCATOR_WIDTH;
    if (ShowDate)
    {
        LoadString(app.hResource, IDS_HEADER_DATE, szText, MAX_LOADSTRING);
        ::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, (UINT)_tcslen(szText), 0);
        Left += m_dateWidth;
    }
    if (ShowAuthor)
    {
        LoadString(app.hResource, IDS_HEADER_AUTHOR, szText, MAX_LOADSTRING);
        ::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, (UINT)_tcslen(szText), 0);
        Left += m_authorWidth;
    }
    if (ShowPath)
    {
        LoadString(app.hResource, IDS_HEADER_PATH, szText, MAX_LOADSTRING);
        ::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, (UINT)_tcslen(szText), 0);
        Left += m_pathWidth;
    }
    LoadString(app.hResource, IDS_HEADER_LINE, szText, MAX_LOADSTRING);
    ::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, (UINT)_tcslen(szText), 0);

    ::SelectObject(hDC, oldfont);
}

void TortoiseBlame::DrawLocatorBar(HDC hDC)
{
    if (hDC == NULL)
        return;

    LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
    LONG_PTR linesonscreen = SendEditor(SCI_LINESONSCREEN);
    LONG_PTR Y = 0;
    COLORREF blackColor = GetSysColor(COLOR_WINDOWTEXT);

    RECT rc;
    GetClientRect(wLocator, &rc);
    RECT lineRect = rc;
    LONG height = rc.bottom-rc.top;
    LONG currentLine = 0;

    // draw the colored bar
    for (std::vector<LONG>::const_iterator it = m_revs.begin(); it != m_revs.end(); ++it)
    {
        currentLine++;
        // get the line color
        COLORREF cr = InterColor(DWORD(m_regLocatorOldLinesColor), DWORD(m_regLocatorNewLinesColor), (*it - m_lowestRev)*100/((m_highestRev-m_lowestRev)+1));
        if ((currentLine > line)&&(currentLine <= (line + linesonscreen)))
        {
            cr = InterColor(cr, blackColor, 10);
        }
        SetBkColor(hDC, cr);
        lineRect.top = (LONG)Y;
        lineRect.bottom = (LONG)(currentLine * height / m_revs.size());
        ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
        Y = lineRect.bottom;
    }

    if (m_revs.size())
    {
        // now draw two lines indicating the scroll position of the source view
        SetBkColor(hDC, blackColor);
        lineRect.top = (LONG)(line * height / m_revs.size());
        lineRect.bottom = lineRect.top+1;
        ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
        lineRect.top = (LONG)((line + linesonscreen) * height / m_revs.size());
        lineRect.bottom = lineRect.top+1;
        ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
    }
}

void TortoiseBlame::StringExpand(LPSTR str)
{
    char * cPos = str;
    do
    {
        cPos = strchr(cPos, '\n');
        if (cPos)
        {
            memmove(cPos+1, cPos, strlen(cPos));
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
            wmemmove(cPos+1, cPos, wcslen(cPos));
            *cPos = '\r';
            cPos++;
            cPos++;
        }
    } while (cPos != NULL);
}

void TortoiseBlame::MakeLower(TCHAR* buffer, size_t len)
{
    for (TCHAR *p = buffer; p < buffer + len; p++)
    {
        if (_istupper(*p)&&_istascii(*p))
            *p = _totlower(*p);
    }
}

void TortoiseBlame::RunCommand(const tstring& command)
{
    tstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
    CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), const_cast<TCHAR*>(command.c_str()));
}

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hResource);
ATOM                MyRegisterBlameClass(HINSTANCE hResource);
ATOM                MyRegisterHeaderClass(HINSTANCE hResource);
ATOM                MyRegisterLocatorClass(HINSTANCE hResource);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndBlameProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndHeaderProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndLocatorProc(HWND, UINT, WPARAM, LPARAM);
UINT                uFindReplaceMsg;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE /*hPrevInstance*/,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    app.hInstance = hInstance;
    MSG msg;
    HACCEL hAccelTable;

    if (::LoadLibrary(_T("SciLexer.DLL")) == NULL)
        return FALSE;

    CRegStdDWORD loc = CRegStdDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
    long langId = loc;

    CLangDll langDLL;
    app.hResource = langDLL.Init(_T("TortoiseBlame"), langId);
    if (app.hResource == NULL)
        app.hResource = app.hInstance;

    // Initialize global strings
    LoadString(app.hResource, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(app.hResource, IDC_TORTOISEBLAME, szWindowClass, MAX_LOADSTRING);
    LoadString(app.hResource, IDS_SEARCHNOTFOUND, searchstringnotfound, MAX_LOADSTRING);
    MyRegisterClass(app.hResource);
    MyRegisterBlameClass(app.hResource);
    MyRegisterHeaderClass(app.hResource);
    MyRegisterLocatorClass(app.hResource);

    // Perform application initialization:
    if (!InitInstance (app.hResource, nCmdShow))
    {
        langDLL.Close();
        return FALSE;
    }

    SecureZeroMemory(szViewtitle, MAX_PATH);
    SecureZeroMemory(szOrigPath, MAX_PATH);
    TCHAR blamefile[MAX_PATH] = {0};
    TCHAR logfile[MAX_PATH] = {0};

    CCmdLineParser parser(lpCmdLine);


    if (__argc > 1)
    {
        _tcscpy_s(blamefile, MAX_PATH, __wargv[1]);
    }
    if (__argc > 2)
    {
        _tcscpy_s(logfile, MAX_PATH, __wargv[2]);
    }
    if (__argc > 3)
    {
        _tcscpy_s(szViewtitle, MAX_PATH, __wargv[3]);
        if (parser.HasVal(_T("revrange")))
        {
            _tcscat_s(szViewtitle, MAX_PATH, _T(" : "));
            _tcscat_s(szViewtitle, MAX_PATH, parser.GetVal(_T("revrange")));
        }
    }
    if ((_tcslen(blamefile)==0) || parser.HasKey(_T("?")) || parser.HasKey(_T("help")))
    {
        TCHAR szInfo[MAX_LOADSTRING];
        LoadString(app.hResource, IDS_COMMANDLINE_INFO, szInfo, MAX_LOADSTRING);
        MessageBox(NULL, szInfo, _T("TortoiseBlame"), MB_ICONERROR);
        langDLL.Close();
        return 0;
    }

    if ( parser.HasKey(_T("path")) )
    {
        _tcscpy_s(szOrigPath, MAX_PATH, parser.GetVal(_T("path")));
    }
    app.bIgnoreEOL = parser.HasKey(_T("ignoreeol"));
    app.bIgnoreSpaces = parser.HasKey(_T("ignorespaces"));
    app.bIgnoreAllSpaces = parser.HasKey(_T("ignoreallspaces"));

    app.SendEditor(SCI_SETCODEPAGE, GetACP());
    app.OpenFile(blamefile);
    if (_tcslen(logfile)>0)
        app.OpenLogFile(logfile);

    if (parser.HasKey(_T("line")))
    {
        app.GotoLine(parser.GetLongVal(_T("line")));
    }

    CheckMenuItem(GetMenu(app.wMain), ID_VIEW_COLORAGEOFLINES, MF_CHECKED|MF_BYCOMMAND);


    hAccelTable = LoadAccelerators(app.hResource, (LPCTSTR)IDC_TORTOISEBLAME);

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
    langDLL.Close();
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hResource)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hResource;
    wcex.hIcon          = LoadIcon(hResource, (LPCTSTR)IDI_TORTOISEBLAME);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = (LPCTSTR)IDC_TORTOISEBLAME;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

ATOM MyRegisterBlameClass(HINSTANCE hResource)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndBlameProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hResource;
    wcex.hIcon          = LoadIcon(hResource, (LPCTSTR)IDI_TORTOISEBLAME);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = _T("TortoiseBlameBlame");
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

ATOM MyRegisterHeaderClass(HINSTANCE hResource)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndHeaderProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hResource;
    wcex.hIcon          = LoadIcon(hResource, (LPCTSTR)IDI_TORTOISEBLAME);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = _T("TortoiseBlameHeader");
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

ATOM MyRegisterLocatorClass(HINSTANCE hResource)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndLocatorProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hResource;
    wcex.hIcon          = LoadIcon(hResource, (LPCTSTR)IDI_TORTOISEBLAME);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = _T("TortoiseBlameLocator");
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hResource, int nCmdShow)
{
   app.wMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hResource, NULL);

   if (!app.wMain)
   {
      return FALSE;
   }

   CRegStdDWORD pos(_T("Software\\TortoiseSVN\\TBlamePos"), 0);
   CRegStdDWORD width(_T("Software\\TortoiseSVN\\TBlameSize"), 0);
   CRegStdDWORD state(_T("Software\\TortoiseSVN\\TBlameState"), 0);
   if (DWORD(pos) && DWORD(width))
   {
       RECT rc;
       rc.left = LOWORD(DWORD(pos));
       rc.top = HIWORD(DWORD(pos));
       rc.right = rc.left + LOWORD(DWORD(width));
       rc.bottom = rc.top + HIWORD(DWORD(width));
       HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
       if (hMon)
       {
           // only restore the window position if the monitor is valid
           MoveWindow(app.wMain, LOWORD(DWORD(pos)), HIWORD(DWORD(pos)),
               LOWORD(DWORD(width)), HIWORD(DWORD(width)), FALSE);
       }
   }
   if (DWORD(state) == SW_MAXIMIZE)
       ShowWindow(app.wMain, SW_MAXIMIZE);
   else
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
       app.hResource,
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
   ti.hinst = app.hResource;
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

void TortoiseBlame::InitSize()
{
    RECT rc;
    RECT blamerc;
    RECT sourcerc;
    ::GetClientRect(wMain, &rc);
    ::SetWindowPos(wHeader, 0, rc.left, rc.top, rc.right-rc.left, HEADER_HEIGHT, 0);
    rc.top += HEADER_HEIGHT;
    blamerc.left = rc.left;
    blamerc.top = rc.top;
    LONG w = GetBlameWidth();
    blamerc.right = w > abs(rc.right - rc.left) ? rc.right : w + rc.left;
    blamerc.bottom = rc.bottom;
    sourcerc.left = blamerc.right;
    sourcerc.top = rc.top;
    sourcerc.bottom = rc.bottom;
    sourcerc.right = rc.right;
    if (m_colorAge)
    {
        ::OffsetRect(&blamerc, LOCATOR_WIDTH, 0);
        ::OffsetRect(&sourcerc, LOCATOR_WIDTH, 0);
        sourcerc.right -= LOCATOR_WIDTH;
    }
    InvalidateRect(wMain, NULL, FALSE);
    ::SetWindowPos(wEditor, 0, sourcerc.left, sourcerc.top, sourcerc.right - sourcerc.left, sourcerc.bottom - sourcerc.top, 0);
    ::SetWindowPos(wBlame, 0, blamerc.left, blamerc.top, blamerc.right - blamerc.left, blamerc.bottom - blamerc.top, 0);
    if (m_colorAge)
        ::SetWindowPos(wLocator, 0, 0, blamerc.top, LOCATOR_WIDTH, blamerc.bottom - blamerc.top, SWP_SHOWWINDOW);
    else
        ::ShowWindow(wLocator, SW_HIDE);
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
            _T("Scintilla"),
            _T("Source"),
            WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
            0, 0,
            100, 100,
            hWnd,
            0,
            app.hResource,
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
            app.hResource,
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
            app.hResource,
            NULL);
        ::ShowWindow(app.wHeader, SW_SHOW);
        app.wLocator = ::CreateWindow(
            _T("TortoiseBlameLocator"),
            _T("locator"),
            WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
            CW_USEDEFAULT, 0,
            CW_USEDEFAULT, 0,
            hWnd,
            NULL,
            app.hResource,
            NULL);
        ::ShowWindow(app.wLocator, SW_SHOW);
        return 0;

    case WM_SIZE:
        if (wParam != 1)
        {
            app.InitSize();
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
        {
            CRegStdDWORD pos(_T("Software\\TortoiseSVN\\TBlamePos"), 0);
            CRegStdDWORD width(_T("Software\\TortoiseSVN\\TBlameSize"), 0);
            CRegStdDWORD state(_T("Software\\TortoiseSVN\\TBlameState"), 0);
            RECT rc;
            GetWindowRect(app.wMain, &rc);
            if ((rc.left >= 0)&&(rc.top >= 0))
            {
                pos = MAKELONG(rc.left, rc.top);
                width = MAKELONG(rc.right-rc.left, rc.bottom-rc.top);
            }
            WINDOWPLACEMENT wp = {0};
            wp.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(app.wMain, &wp);
            state = wp.showCmd;
            ::DestroyWindow(app.wEditor);
            ::PostQuitMessage(0);
        }
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
        app.Command(LOWORD(wParam));
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
                DWORD ptW = GetMessagePos();
                point.x = GET_X_LPARAM(ptW);
                point.y = GET_Y_LPARAM(ptW);
                ::ScreenToClient(app.wBlame, &point);
                LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
                LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
                line = line + (point.y/height);
                if (line >= (LONG)app.m_revs.size())
                    break;
                if (line < 0)
                    break;
                bool bUseMerged = ((app.m_mergedRevs[line] > 0)&&(app.m_mergedRevs[line] < app.m_revs[line]));
                LONG rev = bUseMerged ? app.m_mergedRevs[line] : app.m_revs[line];
                LONG origrev = -1;
                origrev = app.m_revs[line];

                SecureZeroMemory(app.m_szTip, sizeof(app.m_szTip));
                SecureZeroMemory(app.m_wszTip, sizeof(app.m_wszTip));
                std::map<LONG, tstring>::iterator iter;
                if ((iter = app.m_logMessages.find(rev)) != app.m_logMessages.end())
                {
                    tstring msg;
                    if (!ShowAuthor)
                    {
                        msg += app.m_authors[line];
                    }
                    if (!ShowDate)
                    {
                        if (!ShowAuthor) msg += _T("  ");
                        msg += app.m_dates[line];
                    }
                    if (!ShowAuthor || !ShowDate)
                        msg += '\n';
                    msg += iter->second;
                    if (rev != origrev)
                    {
                        // add the merged revision
                        std::map<LONG, tstring>::iterator iter2;
                        if ((iter2 = app.m_logMessages.find(origrev)) != app.m_logMessages.end())
                        {
                            if (!msg.empty())
                                msg += _T("\n------------------\n");
                            TCHAR revBuf[100];
                            _stprintf_s(revBuf, 100, _T("merged in r%ld:\n----\n"), origrev);
                            msg += revBuf;
                            msg += iter2->second;
                        }
                    }

                    // an empty tooltip string will deactivate the tooltips,
                    // which means we must make sure that the tooltip won't
                    // be empty.
                    if (msg.empty())
                        msg = _T(" ");

                    if (pNMHDR->code == TTN_NEEDTEXTA)
                    {
                        lstrcpynA(app.m_szTip, CUnicodeUtils::StdGetUTF8(msg).c_str(), MAX_LOG_LENGTH*2);
                        app.StringExpand(app.m_szTip);
                        pTTTA->lpszText = app.m_szTip;
                    }
                    else
                    {
                        pTTTW->lpszText = app.m_wszTip;
                        lstrcpyn(app.m_wszTip, msg.c_str(), MAX_LOG_LENGTH*2);
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
        app.m_mouseRev = -2;
        app.m_mouseAuthor.clear();
        app.m_ttVisible = FALSE;
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
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ClientToScreen(app.wBlame, &pt);
            pt.x += 15;
            pt.y += 15;
            SendMessage(app.hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
            if (!app.m_ttVisible)
            {
                TOOLINFO ti;
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd = app.wBlame;
                ti.uId = 0;
                SendMessage(app.hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
            }
            int y = GET_Y_LPARAM(lParam);
            LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
            LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
            line = line + (y/height);
            app.m_ttVisible = (line < (LONG)app.m_revs.size());
            if ( app.m_ttVisible )
            {
                if (app.m_authors[line].compare(app.m_mouseAuthor) != 0)
                {
                    app.m_mouseAuthor = app.m_authors[line];
                }
                if (app.m_revs[line] != app.m_mouseRev)
                {
                    app.m_mouseRev = app.m_revs[line];
                    ::InvalidateRect(app.wBlame, NULL, FALSE);
                    SendMessage(app.hwndTT, TTM_UPDATE, 0, 0);
                }
            }
        }
        break;
    case WM_LBUTTONDOWN:
        {
            int y = GET_Y_LPARAM(lParam);
            app.SelectLine(y, false);
        }
        break;
    case WM_SETFOCUS:
        ::SetFocus(app.wBlame);
        app.SendEditor(SCI_GRABFOCUS);
        break;
    case WM_CONTEXTMENU:
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            if ((xPos == -1)||(yPos == -1))
            {
                // requested from keyboard, not mouse pointer
                // use the center of the window
                RECT rect;
                GetWindowRect(app.wBlame, &rect);
                xPos = rect.right-rect.left;
                yPos = rect.bottom-rect.top;
            }
            POINT yPt = {0, yPos};
            ScreenToClient(app.wBlame, &yPt);
            app.SelectLine(yPt.y, true);
            if (app.m_selectedRev <= 0)
                break;;

            HMENU hMenu = LoadMenu(app.hResource, MAKEINTRESOURCE(IDR_BLAMEPOPUP));
            HMENU hPopMenu = GetSubMenu(hMenu, 0);

            if ( _tcslen(szOrigPath)==0 )
            {
                // Without knowing the original path we cannot blame the previous revision
                // because we don't know which filename to pass to tortoiseproc.
                EnableMenuItem(hPopMenu,ID_BLAME_PREVIOUS_REVISION, MF_DISABLED|MF_GRAYED);
            }

            TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, xPos, yPos, 0, app.wBlame, NULL);
            DestroyMenu(hMenu);
        }
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

LRESULT CALLBACK WndLocatorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hDC;
    switch (message)
    {
    case WM_PAINT:
        hDC = BeginPaint(app.wLocator, &ps);
        app.DrawLocatorBar(hDC);
        EndPaint(app.wLocator, &ps);
        break;
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON)
        {
            RECT rect;
            ::GetClientRect(hWnd, &rect);
            int nLine = (int)(HIWORD(lParam)*app.m_revs.size()/(rect.bottom-rect.top));

            if (nLine < 0)
                nLine = 0;
            app.ScrollToLine(nLine);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#pragma warning(pop)