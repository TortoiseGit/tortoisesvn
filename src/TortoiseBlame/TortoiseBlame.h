// TortoiseBlame - a Viewer for Subversion Blames

// Copyright (C) 2003-2010, 2012-2014 - TortoiseSVN

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
#pragma once

#include "resource.h"
#include <Commdlg.h>
#include "Scintilla.h"
#include "SciLexer.h"
#include "registry.h"

#include <set>

const COLORREF black = RGB(0,0,0);
const COLORREF white = RGB(0xff,0xff,0xff);
const COLORREF red = RGB(0xFF, 0, 0);
const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
const COLORREF darkGreen = RGB(0, 0x80, 0);
const COLORREF darkBlue = RGB(0, 0, 0x80);
const COLORREF lightBlue = RGB(0xA6, 0xCA, 0xF0);
const int blockSize = 128 * 1024;

#define BLAMESPACE 20
#define HEADER_HEIGHT 40
#define LOCATOR_WIDTH 20

#define MAX_LOG_LENGTH 20000


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

typedef long int svn_revnum_t;

/**
 * \ingroup TortoiseBlame
 * Main class for TortoiseBlame.
 * Handles all child windows, loads the blame files, ...
 */
class TortoiseBlame
{
public:
    TortoiseBlame();
    ~TortoiseBlame();

    HINSTANCE hInstance;
    HINSTANCE hResource;
    HWND currentDialog;
    HWND wMain;
    HWND wEditor;
    HWND wBlame;
    HWND wHeader;
    HWND wLocator;
    HWND hwndTT;

    BOOL bIgnoreEOL;
    BOOL bIgnoreSpaces;
    BOOL bIgnoreAllSpaces;

    LRESULT SendEditor(UINT Msg, WPARAM wParam=0, LPARAM lParam=0);

    void SetTitle();
    BOOL OpenFile(const TCHAR *fileName);

    void Command(int id);
    void Notify(SCNotification *notification);

    void SetAStyle(int style, COLORREF fore, COLORREF back=::GetSysColor(COLOR_WINDOW), int size=-1, const char *face=0);
    void InitialiseEditor();
    void InitSize();
    LONG GetBlameWidth();
    void DrawBlame(HDC hDC);
    void DrawHeader(HDC hDC);
    void DrawLocatorBar(HDC hDC);
    void StartSearch();
    void StartSearchSel();
    void DoSearchNext();
    void CopySelectedLogToClipboard();
    void CopySelectedRevToClipboard();
    void BlamePreviousRevision();
    void DiffPreviousRevision() const;
    void ShowLog();
    bool DoSearch(LPTSTR what, DWORD flags);
    bool GotoLine(long line);
    bool ScrollToLine(long line);
    void GotoLineDlg();
    void SelectLine(int yPos, bool bAlwaysSelect) const;
    static INT_PTR CALLBACK GotoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SetSelectedLine(LONG line) { m_selectedLine=line;};

    LONG                        m_mouseRev;
    tstring                     m_mouseAuthor;
    LONG                        m_selectedRev;
    LONG                        m_selectedOrigRev;
    tstring                     m_selectedAuthor;
    tstring                     m_selectedDate;
    static long                 m_gotoLine;
    long                        m_lowestRev;
    long                        m_highestRev;
    int                         m_colorby;

    std::vector<svn_revnum_t>   m_revs;
    std::vector<svn_revnum_t>   m_mergedRevs;
    std::vector<tstring>        m_dates;
    std::vector<tstring>        m_mergedDates;
    std::vector<tstring>        m_authors;
    std::vector<tstring>        m_mergedAuthors;
    std::vector<tstring>        m_mergedPaths;
    std::map<LONG, tstring>     m_logMessages;
    std::set<svn_revnum_t>      m_revset;
    std::set<tstring>           m_authorset;
    std::map<svn_revnum_t, COLORREF>    m_revcolormap;
    std::map<tstring, COLORREF>         m_authorcolormap;
    char                        m_szTip[MAX_LOG_LENGTH*2+6];
    wchar_t                     m_wszTip[MAX_LOG_LENGTH*2+6];
    void StringExpand(LPSTR str) const;
    void StringExpand(LPWSTR str) const;
    BOOL                        m_ttVisible;
protected:
    void CreateFont();
    void SetupLexer(LPCTSTR fileName);
    void SetupCppLexer();
    COLORREF InterColor(COLORREF c1, COLORREF c2, int Slider) const;
    COLORREF GetLineColor(int line, bool bLocatorBar);
    void SetupColoring();
    static std::wstring GetAppDirectory();

    //std::vector<COLORREF>     m_colors;
    HFONT                       m_font;
    HFONT                       m_italicFont;
    LONG                        m_blameWidth;
    LONG                        m_revWidth;
    LONG                        m_dateWidth;
    LONG                        m_authorWidth;
    LONG                        m_pathWidth;
    LONG                        m_lineWidth;
    LONG                        m_selectedLine; ///< zero-based

    COLORREF                    m_mouseRevColor;
    COLORREF                    m_mouseAuthorColor;
    COLORREF                    m_selectedRevColor;
    COLORREF                    m_selectedAuthorColor;
    COLORREF                    m_windowColor;
    COLORREF                    m_textColor;
    COLORREF                    m_textHighLightColor;

    LRESULT                     m_directFunction;
    LRESULT                     m_directPointer;
    FINDREPLACE                 m_fr;
    TCHAR                       m_szFindWhat[80];

    CRegStdDWORD                m_regOldLinesColor;
    CRegStdDWORD                m_regNewLinesColor;
    CRegStdDWORD                m_regLocatorOldLinesColor;
    CRegStdDWORD                m_regLocatorNewLinesColor;
    CRegStdDWORD                m_regcolorby;

private:
    static void MakeLower(TCHAR* buffer, size_t length );
    static void RunCommand(const tstring& command);
};
