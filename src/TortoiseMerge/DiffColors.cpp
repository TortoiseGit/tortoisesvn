// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2007-2008, 2010, 2013-2014, 2017, 2020-2021 - TortoiseSVN

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
//
#include "stdafx.h"
#include "DiffColors.h"
#include "Theme.h"

CDiffColors &CDiffColors::GetInstance()
{
    static CDiffColors instance;
    return instance;
}

CDiffColors::CDiffColors()
{
    LoadRegistry();
}

CDiffColors::~CDiffColors()
{
}

void CDiffColors::GetColors(DiffStates state, bool darkMode, COLORREF &crBkgnd, COLORREF &crText)
{
    if ((state < DIFFSTATE_END) && (state >= 0))
    {
        if (darkMode)
        {
            crBkgnd = static_cast<COLORREF>(static_cast<DWORD>(m_regDarkBackgroundColors[static_cast<int>(state)]));
            crText  = static_cast<COLORREF>(static_cast<DWORD>(m_regDarkForegroundColors[static_cast<int>(state)]));
        }
        else
        {
            crBkgnd = static_cast<COLORREF>(static_cast<DWORD>(m_regBackgroundColors[static_cast<int>(state)]));
            crText  = static_cast<COLORREF>(static_cast<DWORD>(m_regForegroundColors[static_cast<int>(state)]));
        }
    }
    else
    {
        if (darkMode)
        {
            crBkgnd = CTheme::darkBkColor;
            crText  = CTheme::darkTextColor;
        }
        else
        {
            crBkgnd = ::GetSysColor(COLOR_WINDOW);
            crText  = ::GetSysColor(COLOR_WINDOWTEXT);
        }
    }
}

void CDiffColors::SetColors(DiffStates state, bool darkMode, const COLORREF &crBkgnd, const COLORREF &crText)
{
    if ((state < DIFFSTATE_END) && (state >= 0))
    {
        if (darkMode)
        {
            m_regDarkBackgroundColors[static_cast<int>(state)] = crBkgnd;
            m_regDarkForegroundColors[static_cast<int>(state)] = crText;
        }
        else
        {
            m_regBackgroundColors[static_cast<int>(state)] = crBkgnd;
            m_regForegroundColors[static_cast<int>(state)] = crText;
        }
    }
}

void CDiffColors::LoadRegistry()
{
    m_regForegroundColors[DIFFSTATE_UNKNOWN]               = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorUnknownF", DIFFSTATE_UNKNOWN_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_NORMAL]                = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorNormalF", DIFFSTATE_NORMAL_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_REMOVED]               = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorRemovedF", DIFFSTATE_REMOVED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_REMOVEDWHITESPACE]     = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorRemovedWhitespaceF", DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_ADDED]                 = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorAddedF", DIFFSTATE_ADDED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_ADDEDWHITESPACE]       = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorAddedWhitespaceF", DIFFSTATE_ADDEDWHITESPACE_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_WHITESPACE]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorWhitespaceF", DIFFSTATE_WHITESPACE_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_WHITESPACE_DIFF]       = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorWhitespaceDiffF", DIFFSTATE_WHITESPACE_DIFF_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_EMPTY]                 = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorEmptyF", DIFFSTATE_EMPTY_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_CONFLICTED]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorConflictedF", DIFFSTATE_CONFLICTED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_CONFLICTED_IGNORED]    = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorConflictedIgnoredF", DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_CONFLICTADDED]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorConflictedAddedF", DIFFSTATE_CONFLICTADDED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_CONFLICTEMPTY]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorConflictedEmptyF", DIFFSTATE_CONFLICTEMPTY_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_CONFLICTRESOLVED]      = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ConflictResolvedF", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ConflictResolvedEmptyF", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_IDENTICALREMOVED]      = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorIdenticalRemovedF", DIFFSTATE_IDENTICALREMOVED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_IDENTICALADDED]        = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorIdenticalAddedF", DIFFSTATE_IDENTICALADDED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_THEIRSREMOVED]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorTheirsRemovedF", DIFFSTATE_THEIRSREMOVED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_THEIRSADDED]           = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorTheirsAddedF", DIFFSTATE_THEIRSADDED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_YOURSREMOVED]          = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorYoursRemovedF", DIFFSTATE_YOURSREMOVED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_YOURSADDED]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorYoursAddedF", DIFFSTATE_YOURSADDED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_EDITED]                = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorEditedF", DIFFSTATE_EDITED_DEFAULT_FG);
    m_regForegroundColors[DIFFSTATE_FILTEREDDIFF]          = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorFilteredF", DIFFSTATE_EDITED_DEFAULT_FG);

    m_regBackgroundColors[DIFFSTATE_UNKNOWN]               = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorUnknownB", DIFFSTATE_UNKNOWN_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_NORMAL]                = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorNormalB", DIFFSTATE_NORMAL_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_REMOVED]               = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorRemovedB", DIFFSTATE_REMOVED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_REMOVEDWHITESPACE]     = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorRemovedWhitespaceB", DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_ADDED]                 = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorAddedB", DIFFSTATE_ADDED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_ADDEDWHITESPACE]       = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorAddedWhitespaceB", DIFFSTATE_ADDEDWHITESPACE_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_WHITESPACE]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorWhitespaceB", DIFFSTATE_WHITESPACE_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_WHITESPACE_DIFF]       = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorWhitespaceDiffB", DIFFSTATE_WHITESPACE_DIFF_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_EMPTY]                 = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorEmptyB", DIFFSTATE_EMPTY_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_CONFLICTED]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorConflictedB", DIFFSTATE_CONFLICTED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_CONFLICTED_IGNORED]    = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorConflictedIgnoredB", DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_CONFLICTADDED]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorConflictedAddedB", DIFFSTATE_CONFLICTADDED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_CONFLICTEMPTY]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorConflictedEmptyB", DIFFSTATE_CONFLICTEMPTY_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_CONFLICTRESOLVED]      = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ConflictResolvedB", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ConflictResolvedEmptyB", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_IDENTICALREMOVED]      = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorIdenticalRemovedB", DIFFSTATE_IDENTICALREMOVED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_IDENTICALADDED]        = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorIdenticalAddedB", DIFFSTATE_IDENTICALADDED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_THEIRSREMOVED]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorTheirsRemovedB", DIFFSTATE_THEIRSREMOVED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_THEIRSADDED]           = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorTheirsAddedB", DIFFSTATE_THEIRSADDED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_YOURSREMOVED]          = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorYoursRemovedB", DIFFSTATE_YOURSREMOVED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_YOURSADDED]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorYoursAddedB", DIFFSTATE_YOURSADDED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_EDITED]                = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorEditedB", DIFFSTATE_EDITED_DEFAULT_BG);
    m_regBackgroundColors[DIFFSTATE_FILTEREDDIFF]          = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\ColorFilteredB", DIFFSTATE_FILTERED_DEFAULT_BG);

    m_regDarkForegroundColors[DIFFSTATE_UNKNOWN]               = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorUnknownF", DIFFSTATE_UNKNOWN_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_NORMAL]                = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorNormalF", DIFFSTATE_NORMAL_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_REMOVED]               = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorRemovedF", DIFFSTATE_REMOVED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_REMOVEDWHITESPACE]     = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorRemovedWhitespaceF", DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_ADDED]                 = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorAddedF", DIFFSTATE_ADDED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_ADDEDWHITESPACE]       = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorAddedWhitespaceF", DIFFSTATE_ADDEDWHITESPACE_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_WHITESPACE]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorWhitespaceF", DIFFSTATE_WHITESPACE_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_WHITESPACE_DIFF]       = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorWhitespaceDiffF", DIFFSTATE_WHITESPACE_DIFF_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_EMPTY]                 = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorEmptyF", DIFFSTATE_EMPTY_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_CONFLICTED]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorConflictedF", DIFFSTATE_CONFLICTED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_CONFLICTED_IGNORED]    = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorConflictedIgnoredF", DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_CONFLICTADDED]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorConflictedAddedF", DIFFSTATE_CONFLICTADDED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_CONFLICTEMPTY]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorConflictedEmptyF", DIFFSTATE_CONFLICTEMPTY_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_CONFLICTRESOLVED]      = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkConflictResolvedF", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkConflictResolvedEmptyF", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_IDENTICALREMOVED]      = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorIdenticalRemovedF", DIFFSTATE_IDENTICALREMOVED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_IDENTICALADDED]        = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorIdenticalAddedF", DIFFSTATE_IDENTICALADDED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_THEIRSREMOVED]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorTheirsRemovedF", DIFFSTATE_THEIRSREMOVED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_THEIRSADDED]           = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorTheirsAddedF", DIFFSTATE_THEIRSADDED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_YOURSREMOVED]          = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorYoursRemovedF", DIFFSTATE_YOURSREMOVED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_YOURSADDED]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorYoursAddedF", DIFFSTATE_YOURSADDED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_EDITED]                = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorEditedF", DIFFSTATE_EDITED_DEFAULT_DARK_FG);
    m_regDarkForegroundColors[DIFFSTATE_FILTEREDDIFF]          = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorFilteredF", DIFFSTATE_EDITED_DEFAULT_DARK_FG);

    m_regDarkBackgroundColors[DIFFSTATE_UNKNOWN]               = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorUnknownB", DIFFSTATE_UNKNOWN_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_NORMAL]                = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorNormalB", DIFFSTATE_NORMAL_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_REMOVED]               = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorRemovedB", DIFFSTATE_REMOVED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_REMOVEDWHITESPACE]     = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorRemovedWhitespaceB", DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_ADDED]                 = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorAddedB", DIFFSTATE_ADDED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_ADDEDWHITESPACE]       = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorAddedWhitespaceB", DIFFSTATE_ADDEDWHITESPACE_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_WHITESPACE]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorWhitespaceB", DIFFSTATE_WHITESPACE_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_WHITESPACE_DIFF]       = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorWhitespaceDiffB", DIFFSTATE_WHITESPACE_DIFF_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_EMPTY]                 = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorEmptyB", DIFFSTATE_EMPTY_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_CONFLICTED]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorConflictedB", DIFFSTATE_CONFLICTED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_CONFLICTED_IGNORED]    = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorConflictedIgnoredB", DIFFSTATE_CONFLICTED_IGNORED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_CONFLICTADDED]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorConflictedAddedB", DIFFSTATE_CONFLICTADDED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_CONFLICTEMPTY]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorConflictedEmptyB", DIFFSTATE_CONFLICTEMPTY_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_CONFLICTRESOLVED]      = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkConflictResolvedB", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_CONFLICTRESOLVEDEMPTY] = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkConflictResolvedEmptyB", DIFFSTATE_CONFLICTRESOLVED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_IDENTICALREMOVED]      = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorIdenticalRemovedB", DIFFSTATE_IDENTICALREMOVED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_IDENTICALADDED]        = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorIdenticalAddedB", DIFFSTATE_IDENTICALADDED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_THEIRSREMOVED]         = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorTheirsRemovedB", DIFFSTATE_THEIRSREMOVED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_THEIRSADDED]           = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorTheirsAddedB", DIFFSTATE_THEIRSADDED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_YOURSREMOVED]          = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorYoursRemovedB", DIFFSTATE_YOURSREMOVED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_YOURSADDED]            = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorYoursAddedB", DIFFSTATE_YOURSADDED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_EDITED]                = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorEditedB", DIFFSTATE_EDITED_DEFAULT_DARK_BG);
    m_regDarkBackgroundColors[DIFFSTATE_FILTEREDDIFF]          = CRegDWORD(L"Software\\TortoiseMerge\\Colors\\DarkColorFilteredB", DIFFSTATE_FILTERED_DEFAULT_DARK_BG);

    for (int i = 0; i < DIFFSTATE_END; ++i)
    {
        m_regForegroundColors[i].read();
        m_regBackgroundColors[i].read();
        m_regDarkForegroundColors[i].read();
        m_regDarkBackgroundColors[i].read();
    }
}
