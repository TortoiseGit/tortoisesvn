// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN

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
#pragma once

#include "svn_diff.h"
#include "apr_pools.h"
#include "FileTextLines.h"
#include "Registry.h"
#include "WorkingFile.h"
#include "ViewData.h"

#define DIFFSTATE_UNKNOWN_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_NORMAL_DEFAULT_FG					::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_REMOVED_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_FG		::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_ADDED_DEFAULT_FG					::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_ADDEDWHITESPACE_DEFAULT_FG		::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_WHITESPACE_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_WHITESPACE_DIFF_DEFAULT_FG		::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_EMPTY_DEFAULT_FG					::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTED_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTADDED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTEMPTY_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICALREMOVED_DEFAULT_FG		::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_IDENTICALADDED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_THEIRSREMOVED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_THEIRSADDED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_YOURSREMOVED_DEFAULT_FG			::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_YOURSADDED_DEFAULT_FG				::GetSysColor(COLOR_WINDOWTEXT)
#define DIFFSTATE_CONFLICTRESOLVED_DEFAULT_FG		::GetSysColor(COLOR_WINDOWTEXT)

#define DIFFSTATE_UNKNOWN_DEFAULT_BG				::GetSysColor(COLOR_WINDOW)
#define DIFFSTATE_NORMAL_DEFAULT_BG					::GetSysColor(COLOR_WINDOW)
#define DIFFSTATE_REMOVED_DEFAULT_BG				RGB(255,200,100)
#define DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_BG		DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_ADDED_DEFAULT_BG					RGB(255,255,0)
#define DIFFSTATE_ADDEDWHITESPACE_DEFAULT_BG		DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_WHITESPACE_DEFAULT_BG				DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_WHITESPACE_DIFF_DEFAULT_BG		DIFFSTATE_NORMAL_DEFAULT_BG
#define DIFFSTATE_EMPTY_DEFAULT_BG					RGB(200,200,200)
#define DIFFSTATE_CONFLICTED_DEFAULT_BG				RGB(255,100,100)
#define DIFFSTATE_CONFLICTADDED_DEFAULT_BG			RGB(255,100,100)
#define DIFFSTATE_CONFLICTEMPTY_DEFAULT_BG			RGB(255,100,100)
#define DIFFSTATE_IDENTICALREMOVED_DEFAULT_BG		RGB(255,200,200)
#define DIFFSTATE_IDENTICALADDED_DEFAULT_BG			RGB(180,255,180)
#define DIFFSTATE_THEIRSREMOVED_DEFAULT_BG			RGB(255,200,255)
#define DIFFSTATE_THEIRSADDED_DEFAULT_BG			RGB(120,255,180)
#define DIFFSTATE_YOURSREMOVED_DEFAULT_BG			RGB(255,200,255)
#define DIFFSTATE_YOURSADDED_DEFAULT_BG				RGB(180,255,120)
#define DIFFSTATE_CONFLICTRESOLVED_DEFAULT_BG		RGB(200,255,200)


#define DIFF_EMPTYLINENUMBER						((DWORD)-1)
class CDiffData
{
public:
	CDiffData(void);
	virtual ~CDiffData(void);


	BOOL						Load();
	void						SetBlame(bool bBlame = true) {m_bBlame = bBlame;}
	void						LoadRegistry();
	int							GetLineCount();
	int							GetLineActualLength(int index);
	LPCTSTR						GetLineChars(int index);
	void						GetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText);
	void						SetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText);
	CString						GetError() const  {return m_sError;}

	bool	IsBaseFileInUse() const		{ return m_baseFile.InUse(); }
	bool	IsTheirFileInUse() const	{ return m_theirFile.InUse(); }
	bool	IsYourFileInUse() const		{ return m_yourFile.InUse(); }

private:
	bool DoTwoWayDiff(const CString& sBaseFilename, const CString& sYourFilename, DWORD dwIgnoreWS, bool bIgnoreEOL, apr_pool_t * pool);
	bool DoThreeWayDiff(const CString& sBaseFilename, const CString& sYourFilename, const CString& sTheirFilename, DWORD dwIgnoreWS, bool bIgnoreEOL, bool bIgnoreCase, apr_pool_t * pool);


public:
	CWorkingFile				m_baseFile;
	CWorkingFile				m_theirFile;
	CWorkingFile				m_yourFile;
	CWorkingFile				m_mergedFile;

	CString						m_sDiffFile;
	CString						m_sPatchPath;
	CString						m_sPatchOriginal;
	CString						m_sPatchPatched;

public:
	CFileTextLines				m_arBaseFile;
	CFileTextLines				m_arTheirFile;
	CFileTextLines				m_arYourFile;

	CViewData					m_YourBaseBoth;				///< one-pane view, diff between 'yours' and 'base' (in three-pane view: right view)
	CViewData					m_YourBaseLeft;				///< two-pane view, diff between 'yours' and 'base', left view
	CViewData					m_YourBaseRight;			///< two-pane view, diff between 'yours' and 'base', right view

	CViewData					m_TheirBaseBoth;			///< one-pane view, diff between 'theirs' and 'base' (in three-pane view: left view)
	CViewData					m_TheirBaseLeft;			///< two-pane view, diff between 'theirs' and 'base', left view
	CViewData					m_TheirBaseRight;			///< two-pane view, diff between 'theris' and 'base', right view

	CViewData					m_Diff3;					///< thee-pane view, bottom pane

	// the following three arrays are used to check for conflicts even in case the
	// user has ignored spaces/eols.
	CStdDWORDArray				m_arDiff3LinesBase;
	CStdDWORDArray				m_arDiff3LinesYour;
	CStdDWORDArray				m_arDiff3LinesTheir;

	CString						m_sError;

	static int					abort_on_pool_failure (int retcode);
protected:
	CRegDWORD					m_regForegroundColors[DIFFSTATE_END];
	CRegDWORD					m_regBackgroundColors[DIFFSTATE_END];
	bool						m_bBlame;
};
