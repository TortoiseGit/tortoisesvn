// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004 - Stefan Kueng

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
#pragma once

#include "svn_diff.h"
#include "apr_pools.h"
#include "FileTextLines.h"
#include "Registry.h"

#define DIFFSTATE_UNKNOWN_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_NORMAL_DEFAULT_FG					::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_REMOVED_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_FG		::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_ADDED_DEFAULT_FG					::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_ADDEDWHITESPACE_DEFAULT_FG		::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_WHITESPACE_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_EMPTY_DEFAULT_FG					::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTED_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTADDED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTEMPTY_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICAL_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICALREMOVED_DEFAULT_FG		::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICALADDED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_THEIRSREMOVED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_THEIRSADDED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_YOURSREMOVED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_YOURSADDED_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)

#define DIFFSTATE_UNKNOWN_DEFAULT_BG				::GetSysColor(COLOR_WINDOW)
#define DIFFSTATE_NORMAL_DEFAULT_BG					::GetSysColor(COLOR_WINDOW)
#define DIFFSTATE_REMOVED_DEFAULT_BG				RGB(255,100,100)
#define DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_BG		RGB(255,100,100)
#define DIFFSTATE_ADDED_DEFAULT_BG					RGB(255,255,0)
#define DIFFSTATE_ADDEDWHITESPACE_DEFAULT_BG		RGB(255,255,0)
#define DIFFSTATE_WHITESPACE_DEFAULT_BG				RGB(180,180,255)
#define DIFFSTATE_EMPTY_DEFAULT_BG					RGB(200,200,200)
#define DIFFSTATE_CONFLICTED_DEFAULT_BG				RGB(255,0,0)
#define DIFFSTATE_CONFLICTADDED_DEFAULT_BG			RGB(255,100,0)
#define DIFFSTATE_CONFLICTEMPTY_DEFAULT_BG			RGB(255,50,50)
#define DIFFSTATE_IDENTICAL_DEFAULT_BG				RGB(180,255,180)
#define DIFFSTATE_IDENTICALREMOVED_DEFAULT_BG		RGB(255,200,200)
#define DIFFSTATE_IDENTICALADDED_DEFAULT_BG			RGB(180,255,180)
#define DIFFSTATE_THEIRSREMOVED_DEFAULT_BG			RGB(255,120,200)
#define DIFFSTATE_THEIRSADDED_DEFAULT_BG			RGB(120,255,180)
#define DIFFSTATE_YOURSREMOVED_DEFAULT_BG			RGB(255,200,120)
#define DIFFSTATE_YOURSADDED_DEFAULT_BG				RGB(180,255,120)

class CDiffData
{
public:
	CDiffData(void);
	virtual ~CDiffData(void);

	enum DiffStates
	{
		DIFFSTATE_UNKNOWN,
		DIFFSTATE_NORMAL,
		DIFFSTATE_REMOVED,
		DIFFSTATE_REMOVEDWHITESPACE,
		DIFFSTATE_ADDED,
		DIFFSTATE_ADDEDWHITESPACE,
		DIFFSTATE_WHITESPACE,
		DIFFSTATE_EMPTY,
		DIFFSTATE_CONFLICTED,
		DIFFSTATE_CONFLICTADDED,
		DIFFSTATE_CONFLICTEMPTY,
		DIFFSTATE_IDENTICAL,
		DIFFSTATE_IDENTICALREMOVED,
		DIFFSTATE_IDENTICALADDED,
		DIFFSTATE_THEIRSREMOVED,
		DIFFSTATE_THEIRSADDED,
		DIFFSTATE_YOURSREMOVED,
		DIFFSTATE_YOURSADDED,
		DIFFSTATE_END
	} ;

	BOOL						Load();
	void						LoadRegistry();
	int							GetLineCount();
	int							GetLineActualLength(int index);
	LPCTSTR						GetLineChars(int index);
	void						GetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText);
	void						SetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText);
	CString						GetError() {return m_sError;}

public:
	CString						m_sBaseFile;
	CString						m_sTheirFile;
	CString						m_sYourFile;
	CString						m_sMergedFile;
	CString						m_sDiffFile;
	CString						m_sPatchPath;

	svn_diff_t *				m_diffYourBase;
	svn_diff_t *				m_diffTheirBase;
	svn_diff_t *				m_diffTheirYourBase;

	CFileTextLines				m_arBaseFile;
	CFileTextLines				m_arTheirFile;
	CFileTextLines				m_arYourFile;

	CStringArray				m_arDiffYourBaseBoth;
	CDWordArray					m_arStateYourBaseBoth;
	CStringArray				m_arDiffYourBaseLeft;
	CDWordArray					m_arStateYourBaseLeft;
	CStringArray				m_arDiffYourBaseRight;
	CDWordArray					m_arStateYourBaseRight;

	CStringArray				m_arDiffTheirBaseBoth;
	CDWordArray					m_arStateTheirBaseBoth;
	CStringArray				m_arDiffTheirBaseLeft;
	CDWordArray					m_arStateTheirBaseLeft;
	CStringArray				m_arDiffTheirBaseRight;
	CDWordArray					m_arStateTheirBaseRight;

	CStringArray				m_arDiff3;
	CDWordArray					m_arStateDiff3;

	CString						m_sError;

	static int					abort_on_pool_failure (int retcode);
protected:
	CRegDWORD					m_regForegroundColors[DIFFSTATE_END];
	CRegDWORD					m_regBackgroundColors[DIFFSTATE_END];
};
