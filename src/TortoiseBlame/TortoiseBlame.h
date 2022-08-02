// TortoiseBlame - a Viewer for Subversion Blames

// Copyright (C) 2003-2010, 2012-2014, 2017-2018, 2020-2022 - TortoiseSVN

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

// ReSharper disable once CppUnusedIncludeDirective
#include "ILexer.h"
#include "Scintilla.h"
#include "registry.h"

#include <set>
#include <deque>

const COLORREF black     = RGB(0, 0, 0);
const COLORREF white     = RGB(0xff, 0xff, 0xff);
const COLORREF red       = RGB(0xFF, 0, 0);
const COLORREF offWhite  = RGB(0xFF, 0xFB, 0xF0);
const COLORREF darkGreen = RGB(0, 0x80, 0);
const COLORREF darkBlue  = RGB(0, 0, 0x80);
const COLORREF lightBlue = RGB(0xA6, 0xCA, 0xF0);
const int      blockSize = 128 * 1024;

#define BLAMESPACE    20
#define HEADER_HEIGHT 40
#define LOCATOR_WIDTH 20

#define MAX_LOG_LENGTH 20000

#ifndef GET_X_LPARAM
#    define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#    define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

// ReSharper disable once CppInconsistentNaming
using svn_revnum_t = long int;

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
    HWND      currentDialog;
    HWND      wMain;
    HWND      wEditor;
    HWND      wBlame;
    HWND      wHeader;
    HWND      wLocator;
    HWND      hwndTT;

    BOOL bIgnoreEOL;
    BOOL bIgnoreSpaces;
    BOOL bIgnoreAllSpaces;

    LRESULT SendEditor(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const;

    void SetTitle() const;
    BOOL OpenFile(const wchar_t* fileName);

    void Command(int id);
    void Notify(SCNotification* notification);

    void                    SetAStyle(int style, COLORREF fore, COLORREF back = ::GetSysColor(COLOR_WINDOW), int size = -1, const char* face = nullptr) const;
    void                    InitialiseEditor();
    void                    InitSize();
    LONG                    GetBlameWidth();
    void                    DrawBlame(HDC hDC) const;
    void                    DrawHeader(HDC hDC) const;
    void                    DrawLocatorBar(HDC hDC);
    void                    StartSearch();
    void                    StartSearchSel();
    void                    StartSearchSelReverse();
    void                    DoSearchNext();
    void                    DoSearchPrev();
    void                    CopySelectedLogToClipboard() const;
    void                    CopySelectedRevToClipboard() const;
    void                    BlamePreviousRevision() const;
    void                    DiffPreviousRevision() const;
    void                    ShowLog() const;
    bool                    DoSearch(LPWSTR what, DWORD flags);
    bool                    GotoLine(long line) const;
    bool                    ScrollToLine(long line) const;
    void                    GotoLineDlg() const;
    void                    SelectLine(int yPos, bool bAlwaysSelect) const;
    static INT_PTR CALLBACK GotoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SetSelectedLine(LONG line) { m_selectedLine = line; };

    void SetTheme(bool bDark);
    void CreateFont(int fontSize);

    int          m_themeCallbackId;
    LONG         m_mouseRev;
    std::wstring m_mouseAuthor;
    LONG         m_selectedRev;
    LONG         m_selectedOrigRev;
    std::wstring m_selectedAuthor;
    std::wstring m_selectedDate;
    static long  m_gotoLine;
    long         m_lowestRev;
    long         m_highestRev;
    int          m_colorBy;

    std::deque<svn_revnum_t>         m_revs;
    std::deque<svn_revnum_t>         m_mergedRevs;
    std::deque<std::wstring>         m_dates;
    std::deque<std::wstring>         m_mergedDates;
    std::deque<std::wstring>         m_authors;
    std::deque<std::wstring>         m_mergedAuthors;
    std::deque<std::wstring>         m_mergedPaths;
    std::map<LONG, std::wstring>     m_logMessages;
    std::set<svn_revnum_t>           m_revSet;
    std::set<std::wstring>           m_authorSet;
    std::map<svn_revnum_t, COLORREF> m_revColorMap;
    std::map<std::wstring, COLORREF> m_authorColorMap;
    char                             m_szTip[MAX_LOG_LENGTH * 2 + 6];
    wchar_t                          m_wszTip[MAX_LOG_LENGTH * 2 + 6];
    void                             StringExpand(LPSTR str) const;
    void                             StringExpand(LPWSTR str) const;
    BOOL                             m_ttVisible;

protected:
    void                SetupLexer(LPCWSTR fileName) const;
    void                SetupCppLexer() const;
    static COLORREF     InterColor(COLORREF c1, COLORREF c2, int slider);
    COLORREF            GetLineColor(Sci_Position line, bool bLocatorBar);
    void                SetupColoring();
    static std::wstring GetAppDirectory();

    //std::vector<COLORREF>     m_colors;
    HFONT m_font;
    HFONT m_italicFont;
    HFONT m_uiFont;
    LONG  m_blameWidth;
    LONG  m_revWidth;
    LONG  m_dateWidth;
    LONG  m_authorWidth;
    LONG  m_pathWidth;
    LONG  m_lineWidth;
    LONG  m_selectedLine; ///< zero-based

    COLORREF m_mouseRevColor;
    COLORREF m_mouseAuthorColor;
    COLORREF m_selectedRevColor;
    COLORREF m_selectedAuthorColor;
    COLORREF m_windowColor;
    COLORREF m_textColor;
    COLORREF m_textHighLightColor;

    LRESULT     m_directFunction;
    LRESULT     m_directPointer;
    FINDREPLACE m_fr;
    wchar_t     m_szFindWhat[80];

    CRegStdDWORD m_regOldLinesColor;
    CRegStdDWORD m_regNewLinesColor;
    CRegStdDWORD m_regLocatorOldLinesColor;
    CRegStdDWORD m_regLocatorNewLinesColor;
    CRegStdDWORD m_regDarkOldLinesColor;
    CRegStdDWORD m_regDarkNewLinesColor;
    CRegStdDWORD m_regDarkLocatorOldLinesColor;
    CRegStdDWORD m_regDarkLocatorNewLinesColor;
    CRegStdDWORD m_regColorBy;

private:
    static void MakeLower(wchar_t* buffer, size_t length);
    static void RunCommand(const std::wstring& command);
};
