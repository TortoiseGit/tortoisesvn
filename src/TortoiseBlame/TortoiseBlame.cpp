// TortoiseBlame - a Viewer for Subversion Blames

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "CreateProcessHelper.h"
#include "UnicodeUtils.h"
#include <ClipboardHelper.h>
#include "TaskbarUUID.h"
#include "BlameIndexColors.h"
#include "../Utils/CrashReport.h"
#include "../Utils/SysInfo.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <strsafe.h>

#define MAX_LOADSTRING 1000

#define STYLE_MARK 11

#define COLORBYNONE     0
#define COLORBYAGE      1
#define COLORBYAGECONT  2
#define COLORBYAUTHOR   3


#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Shlwapi.lib")


// Global Variables:
TCHAR szTitle[MAX_LOADSTRING] = { 0 };                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING] = { 0 };            // the main window class name
std::wstring szViewtitle;
std::wstring szOrigPath;
std::wstring szPegRev;
TCHAR searchstringnotfound[MAX_LOADSTRING] = { 0 };

#pragma warning(push)
#pragma warning(disable: 4127)       // conditional expression is constant due to the following bools
const bool ShowDate = false;
const bool ShowAuthor = true;
const bool ShowLine = true;
bool ShowPath = false;

static TortoiseBlame app;
long TortoiseBlame::m_gotoLine = 0;
std::wstring        uuid;

COLORREF colorset[MAX_BLAMECOLORS];

TortoiseBlame::TortoiseBlame()
    : hInstance(0)
    , hResource(0)
    , currentDialog(0)
    , wMain(0)
    , wEditor(0)
    , wLocator(0)
    , wBlame(0)
    , wHeader(0)
    , hwndTT(0)
    , bIgnoreEOL(false)
    , bIgnoreSpaces(false)
    , bIgnoreAllSpaces(false)
    , m_ttVisible(false)
    , m_font(0)
    , m_italicFont(0)
    , m_blameWidth(0)
    , m_revWidth(0)
    , m_dateWidth(0)
    , m_authorWidth(0)
    , m_pathWidth(0)
    , m_lineWidth(0)
    , m_mouseRev(-2)
    , m_windowColor(GetSysColor(COLOR_WINDOW))
    , m_textColor(GetSysColor(COLOR_WINDOWTEXT))
    , m_textHighLightColor(GetSysColor(COLOR_HIGHLIGHTTEXT))
    , m_selectedRev(-1)
    , m_selectedOrigRev(-1)
    , m_selectedLine(-1)
    , m_directPointer(0)
    , m_directFunction(0)
    , m_lowestRev(LONG_MAX)
    , m_highestRev(0)
{
    m_szTip[0]      = 0;
    m_wszTip[0]     = 0;
    m_szFindWhat[0] = 0;
    colorset[0]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor1",  BLAMEINDEXCOLOR1);
    colorset[1]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor2",  BLAMEINDEXCOLOR2);
    colorset[2]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor3",  BLAMEINDEXCOLOR3);
    colorset[3]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor4",  BLAMEINDEXCOLOR4);
    colorset[4]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor5",  BLAMEINDEXCOLOR5);
    colorset[5]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor6",  BLAMEINDEXCOLOR6);
    colorset[6]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor7",  BLAMEINDEXCOLOR7);
    colorset[7]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor8",  BLAMEINDEXCOLOR8);
    colorset[8]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor9",  BLAMEINDEXCOLOR9);
    colorset[9]     = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor10", BLAMEINDEXCOLOR10);
    colorset[10]    = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor11", BLAMEINDEXCOLOR11);
    colorset[11]    = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameIndexColor12", BLAMEINDEXCOLOR12);
    m_regcolorby    = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameColorBy",  COLORBYAGE);
    m_colorby       = m_regcolorby;

    m_mouseRevColor = InterColor(m_windowColor, m_textColor, 20);
    m_mouseAuthorColor = InterColor(m_windowColor, m_textColor, 10);
    m_selectedRevColor = GetSysColor(COLOR_HIGHLIGHT);
    m_selectedAuthorColor = InterColor(m_selectedRevColor, m_textHighLightColor, 35);
    SecureZeroMemory(&m_fr, sizeof(m_fr));
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
        std::unique_ptr<TCHAR[]> pBuf(new TCHAR[bufferlen]);
        len = GetModuleFileName(NULL, pBuf.get(), bufferlen);
        path = std::wstring(pBuf.get(), len);
    } while(len == bufferlen);
    path = path.substr(0, path.rfind('\\') + 1);

    return path;
}

// Return a color which is interpolated between c1 and c2.
// Slider controls the relative proportions as a percentage:
// Slider = 0   represents pure c1
// Slider = 50  represents equal mixture
// Slider = 100 represents pure c2
COLORREF TortoiseBlame::InterColor(COLORREF c1, COLORREF c2, int Slider) const
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
#define MAX_PATH_LENGTH 80
    ASSERT(dialogname.GetLength() < MAX_PATH_LENGTH);
    WCHAR pathbuf[MAX_PATH] = {0};
    if (szViewtitle.size() >= MAX_PATH)
    {
        std::wstring str = szViewtitle;
        std::wregex rx(L"^(\\w+:|(?:\\\\|/+))((?:\\\\|/+)[^\\\\/]+(?:\\\\|/)[^\\\\/]+(?:\\\\|/)).*((?:\\\\|/)[^\\\\/]+(?:\\\\|/)[^\\\\/]+)$");
        std::wstring replacement = L"$1$2...$3";
        std::wstring str2 = std::regex_replace(str, rx, replacement);
        if (str2.size() >= MAX_PATH)
            str2 = str2.substr(0, MAX_PATH-2);
        PathCompactPathEx(pathbuf, str2.c_str(), MAX_PATH_LENGTH-(UINT)wcslen(szTitle), 0);
    }
    else
        PathCompactPathEx(pathbuf, szViewtitle.c_str(), MAX_PATH_LENGTH-(UINT)wcslen(szTitle), 0);
    std::wstring title;
    switch (DWORD(CRegStdDWORD(L"Software\\TortoiseSVN\\DialogTitles", 0)))
    {
    case 0: // url/path - dialogname - appname
        title  = pathbuf;
        title += L" - ";
        title += szTitle;
        title += L" - TortoiseSVN";
        break;
    case 1: // dialogname - url/path - appname
        title = szTitle;
        title += L" - ";
        title += pathbuf;
        title += L" - TortoiseSVN";
        break;
    }
    SetWindowText(wMain, title.c_str());
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
    SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);
    ::ShowWindow(wEditor, SW_HIDE);

    FILE * File = NULL;
    int retrycount = 10;
    while (retrycount)
    {
        _tfopen_s(&File, fileName, L"rb");
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
    LONG linenumber = 0;
    svn_revnum_t rev = 0;
    svn_revnum_t merged_rev = 0;
    int strLen = 0;
    bool bHasMergePaths = false;
    for (;;)
    {
        rev = 0;
        merged_rev = 0;
        // line number
        size_t len = fread(&linenumber, sizeof(LONG), 1, File);
        if (len == 0)
            break;
        // revision
        len = fread(&rev, sizeof(svn_revnum_t), 1, File);
        if (len == 0)
            break;
        m_revs.push_back(rev);
        m_revset.insert(rev);
        // author
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            std::unique_ptr<char[]> stringbuf(new char[strLen+1]);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_authors.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
            m_authorset.insert(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        else
        {
            m_authors.push_back(L"");
            m_authorset.insert(L"");
        }
        // date
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            std::unique_ptr<char[]> stringbuf(new char[strLen+1]);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_dates.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        else
            m_dates.push_back(L"");
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
            std::unique_ptr<char[]> stringbuf(new char[strLen+1]);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_mergedAuthors.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        else
            m_mergedAuthors.push_back(L"");
        // merged date
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            std::unique_ptr<char[]> stringbuf(new char[strLen+1]);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_mergedDates.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
        }
        else
            m_mergedDates.push_back(L"");
        // merged path
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            std::unique_ptr<char[]> stringbuf(new char[strLen+1]);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            if (len == 0)
                break;
            stringbuf[strLen] = 0;
            m_mergedPaths.push_back(CUnicodeUtils::StdGetUnicode(stringbuf.get()));
            bHasMergePaths = true;
        }
        else
            m_mergedPaths.push_back(L"");
        // text line
        len = fread(&strLen, sizeof(int), 1, File);
        if (strLen)
        {
            std::unique_ptr<char[]> stringbuf(new char[strLen+1]);
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
            unsigned char * utf8CheckBuf = (unsigned char*)lineptr;
            bool bUTF8 = false;
            while (*utf8CheckBuf)
            {
                if ((*utf8CheckBuf == 0xC0)||(*utf8CheckBuf == 0xC1)||(*utf8CheckBuf >= 0xF5))
                {
                    bUTF8 = false;
                    break;
                }
                else if ((*utf8CheckBuf & 0xE0)==0xC0)
                {
                    bUTF8 = true;
                    utf8CheckBuf++;
                    if (*utf8CheckBuf == 0)
                        break;
                    if ((*utf8CheckBuf & 0xC0)!=0x80)
                    {
                        bUTF8 = false;
                        break;
                    }
                }
                else if ((*utf8CheckBuf & 0xF0)==0xE0)
                {
                    bUTF8 = true;
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
                else if ((*utf8CheckBuf & 0xF8)==0xF0)
                {
                    bUTF8 = true;
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
                else if (*utf8CheckBuf >= 0x80)
                {
                    bUTF8 = false;
                    break;
                }

                utf8CheckBuf++;
            }
            if (!bUTF8)
            {
                std::string utf8line = CUnicodeUtils::StdAnsiToUTF8(lineptr);
                SendEditor(SCI_ADDTEXT, utf8line.length(), reinterpret_cast<LPARAM>(utf8line.c_str()));
            }
            else
                SendEditor(SCI_ADDTEXT, strlen(lineptr), reinterpret_cast<LPARAM>(static_cast<char *>(lineptr)));
        }
        if (len == 0)
            break;
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            std::unique_ptr<char[]> stringbuf(new char[strLen+1]);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            stringbuf[strLen] = 0;
            tstring msg = CUnicodeUtils::StdGetUnicode(stringbuf.get());
            if (rev)
            {
                if (msg.size() > MAX_LOG_LENGTH)
                {
                    msg = msg.substr(0, MAX_LOG_LENGTH-5);
                    msg = msg + L"\n...";
                }
                m_logMessages[rev] = msg;
            }
        }
        len = fread(&strLen, sizeof(int), 1, File);
        if (len == 0)
            break;
        if (strLen)
        {
            std::unique_ptr<char[]> stringbuf(new char[strLen+1]);
            len = fread(stringbuf.get(), sizeof(char), strLen, File);
            stringbuf[strLen] = 0;
            tstring msg = CUnicodeUtils::StdGetUnicode(stringbuf.get());
            if (merged_rev)
            {
                if (msg.size() > MAX_LOG_LENGTH)
                {
                    msg = msg.substr(0, MAX_LOG_LENGTH-5);
                    msg = msg + L"\n...";
                }
                m_logMessages[merged_rev] = msg;
            }
        }

        SendEditor(SCI_ADDTEXT, 2, (LPARAM)"\r\n");
    };

    fclose(File);

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
    ::InvalidateRect(wMain, NULL, TRUE);
    RECT rc;
    GetWindowRect(wMain, &rc);
    SetWindowPos(wMain, 0, rc.left, rc.top, rc.right-rc.left-1, rc.bottom - rc.top, 0);

    m_blameWidth = 0;
    SetupColoring();
    InitSize();

    if (!bHasMergePaths)
    {
        HMENU hMenu = GetMenu(wMain);
        EnableMenuItem(hMenu, ID_VIEW_MERGEPATH, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
    }

    if ( (m_revs.size() != m_mergedRevs.size()) ||
        (m_mergedRevs.size() != m_dates.size()) ||
        (m_dates.size() != m_mergedDates.size()) ||
        (m_mergedDates.size() != m_authors.size()) ||
        (m_authors.size() != m_mergedAuthors.size()) ||
        (m_mergedAuthors.size() != m_mergedPaths.size()) )
    {
        m_revs.clear();
        m_mergedRevs.clear();
        m_dates.clear();
        m_mergedDates.clear();
        m_authors.clear();
        m_mergedAuthors.clear();
        m_mergedPaths.clear();
        return FALSE;
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
    SetAStyle(STYLE_DEFAULT, black, white, (DWORD)CRegStdDWORD(L"Software\\TortoiseSVN\\BlameFontSize", 10),
        (CUnicodeUtils::StdGetUTF8((tstring)(CRegStdString(L"Software\\TortoiseSVN\\BlameFontName", L"Courier New")))).c_str());
    //SetAStyle(STYLE_MARK, black, ::GetSysColor(COLOR_HIGHLIGHT));
    SendEditor(SCI_INDICSETSTYLE, STYLE_MARK, INDIC_ROUNDBOX);
    SendEditor(SCI_INDICSETFORE, STYLE_MARK, darkBlue);
    SendEditor(SCI_INDICSETUNDER, STYLE_MARK, TRUE);

    SendEditor(SCI_SETTABWIDTH, (DWORD)CRegStdDWORD(L"Software\\TortoiseSVN\\BlameTabSize", 4));
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
    CRegStdDWORD used2d(L"Software\\TortoiseSVN\\ScintillaDirect2D", FALSE);
    if (SysInfo::Instance().IsWin7OrLater() && DWORD(used2d))
    {
        SendEditor(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE);
        SendEditor(SCI_SETBUFFEREDDRAW, 0);
    }
    m_regOldLinesColor = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameOldColor", BLAMEOLDCOLOR);
    m_regNewLinesColor = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameNewColor", BLAMENEWCOLOR);
    m_regLocatorOldLinesColor = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameLocatorOldColor", BLAMEOLDCOLORBAR);
    m_regLocatorNewLinesColor = CRegStdDWORD(L"Software\\TortoiseSVN\\BlameLocatorNewColor", BLAMENEWCOLORBAR);
}

void TortoiseBlame::SelectLine(int yPos, bool bAlwaysSelect) const
{
    LONG line = (LONG)app.SendEditor(SCI_GETFIRSTVISIBLELINE);
    LONG height = (LONG)app.SendEditor(SCI_TEXTHEIGHT);
    line = line + (yPos/height);
    if (line < (LONG)app.m_revs.size())
    {
        app.SetSelectedLine(line);
        bool bUseMerged = ((m_mergedRevs[line] > 0)&&(m_mergedRevs[line] < m_revs[line]));
        bool selChange = bUseMerged ? app.m_mergedRevs[line] != app.m_selectedRev : app.m_revs[line] != app.m_selectedRev;
        if ((bAlwaysSelect)||(selChange))
        {
            app.m_selectedRev = bUseMerged ? app.m_mergedRevs[line] : app.m_revs[line];
            app.m_selectedOrigRev = bUseMerged ? app.m_mergedRevs[line] : app.m_revs[line];
            app.m_selectedAuthor = bUseMerged ? app.m_mergedAuthors[line] : app.m_authors[line];
            app.m_selectedDate = bUseMerged ? app.m_mergedDates[line] : app.m_dates[line];
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

void TortoiseBlame::StartSearchSel()
{
    int selTextLen = (int)SendEditor(SCI_GETSELTEXT);
    if (selTextLen == 0)
        return;

    std::unique_ptr<char[]> seltextbuffer(new char[selTextLen+1]);
    SendEditor(SCI_GETSELTEXT, 0, (LPARAM)(char*)seltextbuffer.get());
    if (seltextbuffer[0] == 0)
        return;
    wcsncpy_s(m_szFindWhat, _countof(m_szFindWhat), CUnicodeUtils::StdGetUnicode(seltextbuffer.get()).c_str(), _countof(m_szFindWhat)-1);
    m_fr.Flags = FR_HIDEUPDOWN | FR_HIDEWHOLEWORD;
    m_fr.Flags |= FR_MATCHCASE;
    DoSearch(m_szFindWhat, m_fr.Flags);
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

void TortoiseBlame::DoSearchNext()
{
    if (wcslen(m_szFindWhat)==0)
        return StartSearchSel();
    DoSearch(m_szFindWhat, m_fr.Flags);
}

bool TortoiseBlame::DoSearch(LPTSTR what, DWORD flags)
{
    TCHAR szWhat[80] = { 0 };
    int pos = (int)SendEditor(SCI_GETCURRENTPOS);
    int line = (int)SendEditor(SCI_LINEFROMPOSITION, pos);
    bool bFound = false;
    bool bCaseSensitive = !!(flags & FR_MATCHCASE);

    wcscpy_s(szWhat, what);

    if(!bCaseSensitive)
    {
        MakeLower(szWhat, wcslen(szWhat));
    }

    tstring sWhat = tstring(szWhat);

    int textSelStart = 0;
    int textSelEnd = 0;
    TCHAR buf[20] = { 0 };
    int i=0;
    for (i=line; (i<(int)m_authors.size())&&(!bFound); ++i)
    {
        const int bufsize = (int)SendEditor(SCI_GETLINE, i);
        std::unique_ptr<char[]> linebuf(new char[bufsize+1]);
        SecureZeroMemory(linebuf.get(), bufsize+1);
        SendEditor(SCI_GETLINE, i, (LPARAM)linebuf.get());
        tstring sLine = CUnicodeUtils::StdGetUnicode(linebuf.get());
        if (!bCaseSensitive)
        {
            std::transform(sLine.begin(), sLine.end(), sLine.begin(), std::tolower);
        }
        swprintf_s(buf, L"%ld", m_revs[i]);
        if (m_authors[i].compare(sWhat)==0)
            bFound = true;
        else if ((!bCaseSensitive)&&(_wcsicmp(m_authors[i].c_str(), szWhat)==0))
            bFound = true;
        else if (wcscmp(buf, szWhat) == 0)
            bFound = true;
        else if (wcsstr(sLine.c_str(), szWhat))
        {
            textSelStart = (int)SendEditor(SCI_POSITIONFROMLINE, i) + int(wcsstr(sLine.c_str(), szWhat) - sLine.c_str());
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
            std::unique_ptr<char[]> linebuf(new char[bufsize+1]);
            SecureZeroMemory(linebuf.get(), bufsize+1);
            SendEditor(SCI_GETLINE, i, (LPARAM)linebuf.get());
            tstring sLine = CUnicodeUtils::StdGetUnicode(linebuf.get());
            if (!bCaseSensitive)
            {
                std::transform(sLine.begin(), sLine.end(), sLine.begin(), std::tolower);
            }
            swprintf_s(buf, L"%ld", m_revs[i]);
            if (m_authors[i].compare(sWhat)==0)
                bFound = true;
            else if ((!bCaseSensitive)&&(_wcsicmp(m_authors[i].c_str(), szWhat)==0))
                bFound = true;
            else if (wcscmp(buf, szWhat) == 0)
                bFound = true;
            else if (wcsstr(sLine.c_str(), szWhat))
            {
                bFound = true;
                textSelStart = (int)SendEditor(SCI_POSITIONFROMLINE, i) + int(wcsstr(sLine.c_str(), szWhat) - sLine.c_str());
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
        ::MessageBox(wMain, searchstringnotfound, L"TortoiseBlame", MB_ICONINFORMATION);
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
        msg += L"  ";
        msg += app.m_selectedDate;
        msg += L"\n";
        msg += iter->second;
        msg += L"\n";
        CClipboardHelper clipboardHelper;
        if (!clipboardHelper.Open(app.wBlame))
            return;

        EmptyClipboard();
        HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc((msg.size() + 1)*sizeof(TCHAR));
        TCHAR* pchData = (TCHAR*)GlobalLock(hClipboardData);
        wcscpy_s(pchData, msg.size()+1, msg.c_str());
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

    TCHAR bufStartRev[20] = { 0 };
    swprintf_s(bufStartRev, L"%ld", nSmallestRevision);

    TCHAR bufEndRev[20] = { 0 };
    swprintf_s(bufEndRev, L"%ld", nRevisionTo);

    TCHAR bufLine[20] = { 0 };
    swprintf_s(bufLine, L"%d", m_selectedLine+1); //using the current line is a good guess.

    tstring svnCmd = L" /command:blame ";
    svnCmd += L" /path:\"";
    svnCmd += szOrigPath;
    svnCmd += L"\"";
    svnCmd += L" /startrev:";
    svnCmd += bufStartRev;
    svnCmd += L" /endrev:";
    svnCmd += bufEndRev;
    svnCmd += szPegRev;
    svnCmd += L" /line:";
    svnCmd += bufLine;
    if (bIgnoreEOL)
        svnCmd += L" /ignoreeol";
    if (bIgnoreSpaces)
        svnCmd += L" /ignorespaces";
    if (bIgnoreAllSpaces)
        svnCmd += L" /ignoreallspaces";
    if (!uuid.empty())
    {
        svnCmd += L" /groupuuid:\"";
        svnCmd += uuid;
        svnCmd += L"\"";
    }
    RunCommand(svnCmd);
}

void TortoiseBlame::DiffPreviousRevision() const
{
    LONG nRevisionTo = m_selectedOrigRev;
    if ( nRevisionTo<1 )
    {
        return;
    }

    LONG nRevisionFrom = nRevisionTo-1;

    TCHAR bufStartRev[20] = { 0 };
    swprintf_s(bufStartRev, L"%ld", nRevisionFrom);

    TCHAR bufEndRev[20] = { 0 };
    swprintf_s(bufEndRev, L"%ld", nRevisionTo);

    tstring svnCmd = L" /command:diff ";
    svnCmd += L" /path:\"";
    svnCmd += szOrigPath;
    svnCmd += L"\"";
    svnCmd += L" /startrev:";
    svnCmd += bufStartRev;
    svnCmd += L" /endrev:";
    svnCmd += bufEndRev;
    svnCmd += szPegRev;
    if (!uuid.empty())
    {
        svnCmd += L" /groupuuid:\"";
        svnCmd += uuid;
        svnCmd += L"\"";
    }
    RunCommand(svnCmd);
}

void TortoiseBlame::ShowLog()
{
    TCHAR bufRev[20] = { 0 };
    swprintf_s(bufRev, _countof(bufRev), L"%ld", m_selectedOrigRev);

    tstring svnCmd = L" /command:log ";
    svnCmd += L" /path:\"";
    svnCmd += szOrigPath;
    svnCmd += L"\"";
    svnCmd += L" /startrev:";
    svnCmd += bufRev;
    svnCmd += szPegRev;
    if (!uuid.empty())
    {
        svnCmd += L" /groupuuid:\"";
        svnCmd += uuid;
        svnCmd += L"\"";
    }
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
        if ((m_colorby != COLORBYNONE)&&(notification->line < (int)m_revs.size()))
        {
            notification->lParam = GetLineColor(notification->line, false);
        }
        break;
    case SCN_UPDATEUI:
        {
            LRESULT firstline = SendEditor(SCI_GETFIRSTVISIBLELINE);
            LRESULT lastline = firstline + SendEditor(SCI_LINESONSCREEN);
            long startstylepos = (long)SendEditor(SCI_POSITIONFROMLINE, firstline);
            long endstylepos = (long)SendEditor(SCI_POSITIONFROMLINE, lastline) + (long)SendEditor(SCI_LINELENGTH, lastline);
            if (endstylepos < 0)
                endstylepos = (long)SendEditor(SCI_GETLENGTH)-startstylepos;

            int len = endstylepos - startstylepos + 1;
            // reset indicators
            SendEditor(SCI_SETINDICATORCURRENT, STYLE_MARK);
            SendEditor(SCI_INDICATORCLEARRANGE, startstylepos, len);

            int selTextLen = (int)SendEditor(SCI_GETSELTEXT);
            if (selTextLen == 0)
                break;

            std::unique_ptr<char[]> seltextbuffer(new char[selTextLen + 1]);
            SendEditor(SCI_GETSELTEXT, 0, (LPARAM)(char*)seltextbuffer.get());
            if (seltextbuffer[0] == 0)
                break;

            std::unique_ptr<char[]> textbuffer(new char[len + 1]);
            TEXTRANGEA textrange;
            textrange.lpstrText = textbuffer.get();
            textrange.chrg.cpMin = startstylepos;
            textrange.chrg.cpMax = endstylepos;
            SendEditor(SCI_GETTEXTRANGE, 0, (LPARAM)&textrange);

            char * startPos = strstr(textbuffer.get(), seltextbuffer.get());
            while (startPos)
            {
                SendEditor(SCI_INDICATORFILLRANGE, startstylepos + ((char*)startPos - (char*)textbuffer.get()), selTextLen-1);
                startPos = strstr(startPos+1, seltextbuffer.get());
            }
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
    case ID_FILE_SETTINGS:
        {
            tstring svnCmd = L" /command:settings /page:19";
            RunCommand(svnCmd);
        }
        break;
    case ID_EDIT_FIND:
        StartSearch();
        break;
    case ID_EDIT_FINDSEL:
        StartSearchSel();
        break;
    case ID_EDIT_FINDNEXT:
        DoSearchNext();
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
    case ID_VIEW_COLORBYAUTHOR:
        {
            if (m_colorby == COLORBYAUTHOR)
                m_colorby = COLORBYNONE;
            else
                m_colorby = COLORBYAUTHOR;

            m_regcolorby = m_colorby;
            SetupColoring();
            InitSize();
        }
        break;
    case ID_VIEW_COLORAGEOFLINES:
        {
            if (m_colorby == COLORBYAGE)
                m_colorby = COLORBYNONE;
            else
                m_colorby = COLORBYAGE;

            m_regcolorby = m_colorby;
            SetupColoring();
            InitSize();
        }
        break;
    case ID_VIEW_COLORBYAGE:
        {
            if (m_colorby == COLORBYAGECONT)
                m_colorby = COLORBYNONE;
            else
                m_colorby = COLORBYAGECONT;

            m_regcolorby = m_colorby;
            SetupColoring();
            InitSize();
        }
        break;
    case ID_VIEW_MERGEPATH:
        {
            bool bUseMerged = !m_mergedPaths.empty();
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
                        TCHAR buf[MAX_PATH] = { 0 };
                        if (::GetWindowText(hEditCtrl, buf, _countof(buf)))
                        {
                            m_gotoLine = _wtol(buf);
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
    TCHAR buf[MAX_PATH] = { 0 };
    swprintf_s(buf, L"*%8ld ", 88888888);
    ::GetTextExtentPoint(hDC, buf, (int)wcslen(buf), &width);
    m_revWidth = width.cx + BLAMESPACE;
    blamewidth += m_revWidth;
    if (ShowDate)
    {
        swprintf_s(buf, L"%30s", L"31.08.2001 06:24:14");
        ::GetTextExtentPoint32(hDC, buf, (int)wcslen(buf), &width);
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
    lf.lfWeight = FW_NORMAL;
    HDC hDC = ::GetDC(wBlame);
    lf.lfHeight = -MulDiv((DWORD)CRegStdDWORD(L"Software\\TortoiseSVN\\BlameFontSize", 10), GetDeviceCaps(hDC, LOGPIXELSY), 72);
    lf.lfCharSet = DEFAULT_CHARSET;
    CRegStdString fontname = CRegStdString(L"Software\\TortoiseSVN\\BlameFontName", L"Courier New");
    wcscpy_s(lf.lfFaceName, ((tstring)fontname).c_str());
    m_font = ::CreateFontIndirect(&lf);

    lf.lfItalic = TRUE;
    lf.lfWeight = FW_BLACK;
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
    TCHAR buf[MAX_PATH] = { 0 };
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
            tstring author;
            if (i < (int)m_authors.size())
                author = bUseMerged ? m_mergedAuthors[i] : m_authors[i];
            if (!author.empty())
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
            if ((rev == m_selectedRev)&&(rev >= 0))
            {
                ::SetBkColor(hDC, m_selectedRevColor);
                ::SetTextColor(hDC, m_textHighLightColor);
            }
            if (rev >= 0)
                swprintf_s(buf, L"%c%8ld       ", bUseMerged ? '*' : ' ', rev);
            else
                wcscpy_s(buf, L"     ----       ");
            rc.right = rc.left + m_revWidth;
            ::ExtTextOut(hDC, 0, (int)Y, ETO_CLIPPED, &rc, buf, (UINT)wcslen(buf), 0);
            int Left = m_revWidth;
            if (ShowDate)
            {
                rc.right = rc.left + Left + m_dateWidth;
                swprintf_s(buf, L"%30s            ", bUseMerged ? m_mergedDates[i].c_str() : m_dates[i].c_str());
                ::ExtTextOut(hDC, Left, (int)Y, ETO_CLIPPED, &rc, buf, (UINT)wcslen(buf), 0);
                Left += m_dateWidth;
            }
            if (ShowAuthor)
            {
                rc.right = rc.left + Left + m_authorWidth;
                swprintf_s(buf, L"%-30s            ", author.c_str());
                ::ExtTextOut(hDC, Left, (int)Y, ETO_CLIPPED, &rc, buf, (UINT)wcslen(buf), 0);
                Left += m_authorWidth;
            }
            if ((ShowPath)&&(m_mergedPaths.size()))
            {
                rc.right = rc.left + Left + m_pathWidth;
                swprintf_s(buf, L"%-60s            ", m_mergedPaths[i].c_str());
                ::ExtTextOut(hDC, Left, (int)Y, ETO_CLIPPED, &rc, buf, (UINT)wcslen(buf), 0);
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
            std::fill_n(buf, _countof(buf), ' ');
            ::ExtTextOut(hDC, 0, (int)Y, ETO_CLIPPED, &rc, buf, _countof(buf)-1, 0);
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

    RECT edgerc = rc;
    edgerc.bottom = edgerc.top + HEADER_HEIGHT/2;
    DrawEdge(hDC, &edgerc, EDGE_BUMP, BF_FLAT|BF_RECT|BF_ADJUST);

    // draw the path first
    WCHAR pathbuf[MAX_PATH] = {0};
    if (szViewtitle.size() >= MAX_PATH)
    {
        std::wstring str = szViewtitle;
        std::wregex rx(L"^(\\w+:|(?:\\\\|/+))((?:\\\\|/+)[^\\\\/]+(?:\\\\|/)[^\\\\/]+(?:\\\\|/)).*((?:\\\\|/)[^\\\\/]+(?:\\\\|/)[^\\\\/]+)$");
        std::wstring replacement = L"$1$2...$3";
        std::wstring str2 = std::regex_replace(str, rx, replacement);
        if (str2.size() >= MAX_PATH)
            str2 = str2.substr(0, MAX_PATH-2);
        wcscpy_s(pathbuf, str2.c_str());
        PathCompactPath(hDC, pathbuf, edgerc.right-edgerc.left-LOCATOR_WIDTH);
    }
    else
    {
        wcscpy_s(pathbuf, szViewtitle.c_str());
        PathCompactPath(hDC, pathbuf, edgerc.right-edgerc.left-LOCATOR_WIDTH);
    }
    DrawText(hDC, pathbuf, -1, &edgerc, DT_SINGLELINE|DT_VCENTER);

    rc.top = rc.top + HEADER_HEIGHT/2;
    DrawEdge(hDC, &rc, EDGE_BUMP, BF_FLAT|BF_RECT|BF_ADJUST);

    RECT drawRc = rc;

    TCHAR szText[MAX_LOADSTRING] = { 0 };
    LoadString(app.hResource, IDS_HEADER_REVISION, szText, MAX_LOADSTRING);
    drawRc.left = LOCATOR_WIDTH;
    DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE|DT_VCENTER);
    int Left = m_revWidth+LOCATOR_WIDTH;
    if (ShowDate)
    {
        LoadString(app.hResource, IDS_HEADER_DATE, szText, MAX_LOADSTRING);
        drawRc.left = Left;
        DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE|DT_VCENTER);
        Left += m_dateWidth;
    }
    if (ShowAuthor)
    {
        LoadString(app.hResource, IDS_HEADER_AUTHOR, szText, MAX_LOADSTRING);
        drawRc.left = Left;
        DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE|DT_VCENTER);
        Left += m_authorWidth;
    }
    if (ShowPath)
    {
        LoadString(app.hResource, IDS_HEADER_PATH, szText, MAX_LOADSTRING);
        drawRc.left = Left;
        DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE|DT_VCENTER);
        Left += m_pathWidth;
    }
    LoadString(app.hResource, IDS_HEADER_LINE, szText, MAX_LOADSTRING);
    drawRc.left = Left;
    DrawText(hDC, szText, -1, &drawRc, DT_SINGLELINE|DT_VCENTER);

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
        // get the line color
        COLORREF cr = GetLineColor(currentLine, true);
        currentLine++;
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

    if (!m_revs.empty())
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

void TortoiseBlame::StringExpand(LPSTR str) const
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
void TortoiseBlame::StringExpand(LPWSTR str) const
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
    CharLowerBuff(buffer, (DWORD)len);
}

void TortoiseBlame::RunCommand(const tstring& command)
{
    tstring tortoiseProcPath = GetAppDirectory() + L"TortoiseProc.exe";
    CCreateProcessHelper::CreateProcessDetached(tortoiseProcPath.c_str(), const_cast<TCHAR*>(command.c_str()));
}

static void ProcessWindowsMessage(HWND window, HACCEL table, MSG& message)
{
    if (TranslateAccelerator(window, table, &message) == 0)
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
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

//Get the message identifier
const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE /*hPrevInstance*/,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    app.hInstance = hInstance;

    SetDllDirectory(L"");
    CCrashReportTSVN crasher(L"TortoiseBlame " _T(APP_X64_STRING));
    CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());

    HMODULE hSciLexerDll = ::LoadLibrary(L"SciLexer.DLL");
    if (hSciLexerDll == NULL)
        return FALSE;

    SetTaskIDPerUUID();

    CRegStdDWORD loc = CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033);
    long langId = loc;

    CLangDll langDLL;
    app.hResource = langDLL.Init(L"TortoiseBlame", langId);
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
        FreeLibrary(hSciLexerDll);
        return FALSE;
    }

    TCHAR blamefile[MAX_PATH] = {0};

    CCmdLineParser parser(lpCmdLine);

    if (parser.HasVal(L"groupuuid"))
    {
        uuid = parser.GetVal(L"groupuuid");
    }

    if (__argc > 1)
    {
        wcscpy_s(blamefile, __wargv[1]);
    }
    if (__argc > 2)
    {
        if ( parser.HasKey(L"path") )
            szViewtitle = parser.GetVal(L"path");
        else if (__wargv[3])
            szViewtitle = __wargv[3];
        if (parser.HasVal(L"revrange"))
        {
            szViewtitle += L" : ";
            szViewtitle += parser.GetVal(L"revrange");
        }
    }
    if ((blamefile[0]==0) || parser.HasKey(L"?") || parser.HasKey(L"help"))
    {
        TCHAR szInfo[MAX_LOADSTRING] = { 0 };
        LoadString(app.hResource, IDS_COMMANDLINE_INFO, szInfo, MAX_LOADSTRING);
        MessageBox(NULL, szInfo, L"TortoiseBlame", MB_ICONERROR);
        langDLL.Close();
        FreeLibrary(hSciLexerDll);
        return 0;
    }

    if ( parser.HasKey(L"path") )
    {
        szOrigPath = parser.GetVal(L"path");
    }

    if ( parser.HasKey(L"pegrev") )
    {
        szPegRev = L" /pegrev:";
        szPegRev += parser.GetVal(L"pegrev");
    }

    app.bIgnoreEOL = parser.HasKey(L"ignoreeol");
    app.bIgnoreSpaces = parser.HasKey(L"ignorespaces");
    app.bIgnoreAllSpaces = parser.HasKey(L"ignoreallspaces");

    app.SendEditor(SCI_SETCODEPAGE, GetACP());
    app.OpenFile(blamefile);

    if (parser.HasKey(L"line"))
    {
        app.GotoLine(parser.GetLongVal(L"line"));
    }

    HACCEL hAccelTable = LoadAccelerators(app.hResource, (LPCTSTR)IDC_TORTOISEBLAME);
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (app.currentDialog == 0)
        {
            ProcessWindowsMessage(app.wMain, hAccelTable, msg);
            continue;
        }

        if (!IsDialogMessage(app.currentDialog, &msg))
        {
            ProcessWindowsMessage(msg.hwnd, hAccelTable, msg);
        }
    }
    langDLL.Close();
    FreeLibrary(hSciLexerDll);
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
    wcex.lpszClassName  = L"TortoiseBlameBlame";
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
    wcex.lpszClassName  = L"TortoiseBlameHeader";
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
    wcex.lpszClassName  = L"TortoiseBlameLocator";
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

   CRegStdDWORD pos(L"Software\\TortoiseSVN\\TBlamePos", 0);
   CRegStdDWORD width(L"Software\\TortoiseSVN\\TBlameSize", 0);
   CRegStdDWORD state(L"Software\\TortoiseSVN\\TBlameState", 0);
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
    if (m_colorby != COLORBYNONE)
    {
        ::OffsetRect(&blamerc, LOCATOR_WIDTH, 0);
        ::OffsetRect(&sourcerc, LOCATOR_WIDTH, 0);
        sourcerc.right -= LOCATOR_WIDTH;
    }
    InvalidateRect(wMain, NULL, FALSE);
    ::SetWindowPos(wEditor, 0, sourcerc.left, sourcerc.top, sourcerc.right - sourcerc.left, sourcerc.bottom - sourcerc.top, 0);
    ::SetWindowPos(wBlame, 0, blamerc.left, blamerc.top, blamerc.right - blamerc.left, blamerc.bottom - blamerc.top, 0);
    if (m_colorby != COLORBYNONE)
        ::SetWindowPos(wLocator, 0, 0, blamerc.top, LOCATOR_WIDTH, blamerc.bottom - blamerc.top, SWP_SHOWWINDOW);
    else
        ::ShowWindow(wLocator, SW_HIDE);
}

COLORREF TortoiseBlame::GetLineColor(int line, bool bLocatorBar)
{
    switch (m_colorby)
    {
    case COLORBYAGE:
        return m_revcolormap[m_revs[line]];
        break;
    case COLORBYAGECONT:
        if (bLocatorBar)
            return InterColor(DWORD(m_regLocatorOldLinesColor), DWORD(m_regLocatorNewLinesColor), (m_revs[line]-m_lowestRev)*100/((m_highestRev-m_lowestRev)+1));
        else
            return InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), (m_revs[line]-m_lowestRev)*100/((m_highestRev-m_lowestRev)+1));
        break;
    case COLORBYAUTHOR:
        return m_authorcolormap[m_authors[line]];
        break;
    case COLORBYNONE:
    default:
        return m_windowColor;
    }
}

void TortoiseBlame::SetupColoring()
{
    HMENU hMenu = GetMenu(wMain);
    CheckMenuItem(hMenu, ID_VIEW_COLORBYAUTHOR, MF_BYCOMMAND | ((m_colorby == COLORBYAUTHOR) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_VIEW_COLORAGEOFLINES, MF_BYCOMMAND | ((m_colorby == COLORBYAGE) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_VIEW_COLORBYAGE, MF_BYCOMMAND | ((m_colorby == COLORBYAGECONT) ? MF_CHECKED : MF_UNCHECKED));
    m_blameWidth = 0;
    m_authorcolormap.clear();
    int colindex = 0;
    switch (m_colorby)
    {
    case COLORBYAGE:
        {
            for (auto it = m_revset.cbegin(); it != m_revset.cend(); ++it)
            {
                m_revcolormap[*it] = colorset[colindex++ % MAX_BLAMECOLORS];
            }
        }
        break;
    case COLORBYAGECONT:
        {
            // nothing to do
        }
        break;
    case COLORBYAUTHOR:
        {
            for (auto it = m_authorset.cbegin(); it != m_authorset.cend(); ++it)
            {
                m_authorcolormap[*it] = colorset[colindex++ % MAX_BLAMECOLORS];
            }
        }
        break;
    }
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
    if (message == TaskBarButtonCreated)
    {
        SetUUIDOverlayIcon(hWnd);
    }
    switch (message)
    {
    case WM_CREATE:
        app.wEditor = ::CreateWindow(
            L"Scintilla",
            L"Source",
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
            L"TortoiseBlameBlame",
            L"blame",
            WS_CHILD | WS_CLIPCHILDREN,
            CW_USEDEFAULT, 0,
            CW_USEDEFAULT, 0,
            hWnd,
            NULL,
            app.hResource,
            NULL);
        ::ShowWindow(app.wBlame, SW_SHOW);
        app.wHeader = ::CreateWindow(
            L"TortoiseBlameHeader",
            L"header",
            WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
            CW_USEDEFAULT, 0,
            CW_USEDEFAULT, 0,
            hWnd,
            NULL,
            app.hResource,
            NULL);
        ::ShowWindow(app.wHeader, SW_SHOW);
        app.wLocator = ::CreateWindow(
            L"TortoiseBlameLocator",
            L"locator",
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
            CRegStdDWORD pos(L"Software\\TortoiseSVN\\TBlamePos", 0);
            CRegStdDWORD width(L"Software\\TortoiseSVN\\TBlameSize", 0);
            CRegStdDWORD state(L"Software\\TortoiseSVN\\TBlameState", 0);
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
    switch (message)
    {
    case WM_CREATE:
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(app.wBlame, &ps);
            app.DrawBlame(hDC);
            EndPaint(app.wBlame, &ps);
        }
        break;
    case WM_COMMAND:
        app.Command(LOWORD(wParam));
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case TTN_GETDISPINFO:
            {
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
                LONG origrev = app.m_revs[line];

                SecureZeroMemory(app.m_szTip, sizeof(app.m_szTip));
                SecureZeroMemory(app.m_wszTip, sizeof(app.m_wszTip));
                std::map<LONG, tstring>::iterator iter;
                tstring msg;
                if ((iter = app.m_logMessages.find(rev)) != app.m_logMessages.end())
                {
                    if (!ShowAuthor)
                    {
                        msg += app.m_authors[line];
                    }
                    if (!ShowDate)
                    {
                        if (!ShowAuthor)
                            msg += L"  ";
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
                                msg += L"\n------------------\n";
                            TCHAR revBuf[100] = { 0 };
                            swprintf_s(revBuf, L"merged in r%ld:\n----\n", origrev);
                            msg += revBuf;
                            msg += iter2->second;
                        }
                    }
                }
                if (msg.size() > MAX_LOG_LENGTH)
                {
                    msg = msg.substr(0, MAX_LOG_LENGTH-5);
                    msg = msg + L"\n...";
                }

                // an empty tooltip string will deactivate the tooltips,
                // which means we must make sure that the tooltip won't
                // be empty.
                if (msg.empty())
                    msg = L" ";

                LPNMHDR pNMHDR = (LPNMHDR)lParam;
                if (pNMHDR->code == TTN_NEEDTEXTA)
                {
                    NMTTDISPINFOA* pTTTA = (NMTTDISPINFOA*)pNMHDR;
                    StringCchCopyA(app.m_szTip, _countof(app.m_szTip), CUnicodeUtils::StdGetUTF8(msg).c_str());
                    app.StringExpand(app.m_szTip);
                    pTTTA->lpszText = app.m_szTip;
                }
                else
                {
                    NMTTDISPINFOW* pTTTW = (NMTTDISPINFOW*)pNMHDR;
                    pTTTW->lpszText = app.m_wszTip;
                    StringCchCopy(app.m_wszTip, _countof(app.m_wszTip), msg.c_str());
                    app.StringExpand(app.m_wszTip);
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
            TRACKMOUSEEVENT mevt;
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
                break;

            HMENU hMenu = LoadMenu(app.hResource, MAKEINTRESOURCE(IDR_BLAMEPOPUP));
            HMENU hPopMenu = GetSubMenu(hMenu, 0);

            if ( szOrigPath.empty() )
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
    switch (message)
    {
    case WM_CREATE:
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(app.wHeader, &ps);
            app.DrawHeader(hDC);
            EndPaint(app.wHeader, &ps);
        }
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
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(app.wLocator, &ps);
            app.DrawLocatorBar(hDC);
            EndPaint(app.wLocator, &ps);
        }
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
