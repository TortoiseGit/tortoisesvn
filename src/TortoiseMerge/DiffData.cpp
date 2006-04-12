// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include "diff.h"
#include "TempFiles.h"
#include "registry.h"
#include "Resource.h"
#include "Diffdata.h"
#include "UnicodeUtils.h"
#include "SVNAdminDir.h"

#pragma warning(push)
#pragma warning(disable: 4702) // unreachable code
int CDiffData::abort_on_pool_failure (int /*retcode*/)
{
	abort ();
	return -1;
}
#pragma warning(pop)

CDiffData::CDiffData(void)
{
	apr_initialize();
	g_SVNAdminDir.Init();

	m_bBlame = false;

	m_regForegroundColors[DIFFSTATE_UNKNOWN] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorUnknownF"), DIFFSTATE_UNKNOWN_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_NORMAL] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorNormalF"), DIFFSTATE_NORMAL_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_REMOVED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorRemovedF"), DIFFSTATE_REMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_REMOVEDWHITESPACE] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorRemovedWhitespaceF"), DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_ADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorAddedF"), DIFFSTATE_ADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_ADDEDWHITESPACE] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorAddedWhitespaceF"), DIFFSTATE_ADDEDWHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_WHITESPACE] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorWhitespaceF"), DIFFSTATE_WHITESPACE_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_WHITESPACE_DIFF] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorWhitespaceDiffF"), DIFFSTATE_WHITESPACE_DIFF_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_EMPTY] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorEmptyF"), DIFFSTATE_EMPTY_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorConflictedF"), DIFFSTATE_CONFLICTED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorConflictedAddedF"), DIFFSTATE_CONFLICTADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_CONFLICTEMPTY] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorConflictedEmptyF"), DIFFSTATE_CONFLICTEMPTY_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_IDENTICALREMOVED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorIdenticalRemovedF"), DIFFSTATE_IDENTICALREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_IDENTICALADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorIdenticalAddedF"), DIFFSTATE_IDENTICALADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_THEIRSREMOVED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorTheirsRemovedF"), DIFFSTATE_THEIRSREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_THEIRSADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorTheirsAddedF"), DIFFSTATE_THEIRSADDED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_YOURSREMOVED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorYoursRemovedF"), DIFFSTATE_YOURSREMOVED_DEFAULT_FG);
	m_regForegroundColors[DIFFSTATE_YOURSADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorYoursAddedF"), DIFFSTATE_YOURSADDED_DEFAULT_FG);

	m_regBackgroundColors[DIFFSTATE_UNKNOWN] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorUnknownB"), DIFFSTATE_UNKNOWN_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_NORMAL] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorNormalB"), DIFFSTATE_NORMAL_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_REMOVED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorRemovedB"), DIFFSTATE_REMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_REMOVEDWHITESPACE] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorRemovedWhitespaceB"), DIFFSTATE_REMOVEDWHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_ADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorAddedB"), DIFFSTATE_ADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_ADDEDWHITESPACE] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorAddedWhitespaceB"), DIFFSTATE_ADDEDWHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_WHITESPACE] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorWhitespaceB"), DIFFSTATE_WHITESPACE_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_WHITESPACE_DIFF] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorWhitespaceDiffB"), DIFFSTATE_WHITESPACE_DIFF_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_EMPTY] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorEmptyB"), DIFFSTATE_EMPTY_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorConflictedB"), DIFFSTATE_CONFLICTED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorConflictedAddedB"), DIFFSTATE_CONFLICTADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_CONFLICTEMPTY] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorConflictedEmptyB"), DIFFSTATE_CONFLICTEMPTY_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_IDENTICALREMOVED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorIdenticalRemovedB"), DIFFSTATE_IDENTICALREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_IDENTICALADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorIdenticalAddedB"), DIFFSTATE_IDENTICALADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_THEIRSREMOVED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorTheirsRemovedB"), DIFFSTATE_THEIRSREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_THEIRSADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorTheirsAddedB"), DIFFSTATE_THEIRSADDED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_YOURSREMOVED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorYoursRemovedB"), DIFFSTATE_YOURSREMOVED_DEFAULT_BG);
	m_regBackgroundColors[DIFFSTATE_YOURSADDED] = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorYoursAddedB"), DIFFSTATE_YOURSADDED_DEFAULT_BG);

	m_sPatchOriginal = _T(": original");
	m_sPatchPatched = _T(": patched");
}

CDiffData::~CDiffData(void)
{
	g_SVNAdminDir.Close();
	apr_terminate();
}

void CDiffData::LoadRegistry()
{
	for (int i=0; i<DIFFSTATE_END; i++)
	{
		m_regForegroundColors[i].read();
		m_regBackgroundColors[i].read();
	} // for (int i=0; i<DIFFSTATE_END; i++) 
}

void CDiffData::GetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText)
{
	if ((state < DIFFSTATE_END)&&(state >= 0))
	{
		crBkgnd = (COLORREF)(DWORD)m_regBackgroundColors[(int)state];
		crText = (COLORREF)(DWORD)m_regForegroundColors[(int)state];
	}
	else
	{
		crBkgnd = ::GetSysColor(COLOR_WINDOW);
		crText = ::GetSysColor(COLOR_WINDOWTEXT);
	}
}

void CDiffData::SetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText)
{
	if ((state < DIFFSTATE_END)&&(state >= 0))
	{
		m_regBackgroundColors[(int)state] = crBkgnd;
		m_regForegroundColors[(int)state] = crText;
	}
}

int CDiffData::GetLineCount()
{
	int count = 0;
	count = (int)m_arBaseFile.GetCount();
	count = (int)(count > m_arTheirFile.GetCount() ? count : m_arTheirFile.GetCount());
	count = (int)(count > m_arYourFile.GetCount() ? count : m_arYourFile.GetCount());
	return count;
}

int CDiffData::GetLineActualLength(int index)
{
	int count = 0;
	if (index < m_arBaseFile.GetCount())
		count = (count > m_arBaseFile.GetAt(index).GetLength() ? count : m_arBaseFile.GetAt(index).GetLength());
	if (index < m_arTheirFile.GetCount())
		count = (count > m_arTheirFile.GetAt(index).GetLength() ? count : m_arTheirFile.GetAt(index).GetLength());
	if (index < m_arYourFile.GetCount())
		count = (count > m_arYourFile.GetAt(index).GetLength() ? count : m_arYourFile.GetAt(index).GetLength());
	return count;
}

LPCTSTR CDiffData::GetLineChars(int index)
{
	if (index < m_arBaseFile.GetCount())
		return m_arBaseFile.GetAt(index);
	if (index < m_arTheirFile.GetCount())
		return m_arTheirFile.GetAt(index);
	if (index < m_arYourFile.GetCount())
		return m_arYourFile.GetAt(index);
	return NULL;
}

BOOL CDiffData::Load()
{
	CString sConvertedBaseFilename, sConvertedTheirFilename, sConvertedYourFilename;
	apr_pool_t * pool;

	apr_pool_create_ex (&pool, NULL, abort_on_pool_failure, NULL);

	m_arBaseFile.RemoveAll();
	m_arYourFile.RemoveAll();
	m_arTheirFile.RemoveAll();
	m_arDiffYourBaseBoth.RemoveAll();
	m_arStateYourBaseBoth.RemoveAll();
	m_arLinesYourBaseBoth.RemoveAll();
	m_arDiffYourBaseLeft.RemoveAll();
	m_arStateYourBaseLeft.RemoveAll();
	m_arLinesYourBaseLeft.RemoveAll();
	m_arDiffYourBaseRight.RemoveAll();
	m_arStateYourBaseRight.RemoveAll();
	m_arLinesYourBaseRight.RemoveAll();
	m_arDiffTheirBaseBoth.RemoveAll();
	m_arStateTheirBaseBoth.RemoveAll();
	m_arLinesTheirBaseBoth.RemoveAll();
	m_arDiffTheirBaseLeft.RemoveAll();
	m_arStateTheirBaseLeft.RemoveAll();
	m_arLinesTheirBaseLeft.RemoveAll();
	m_arDiffTheirBaseRight.RemoveAll();
	m_arStateTheirBaseRight.RemoveAll();
	m_arLinesTheirBaseRight.RemoveAll();
	m_arDiff3.RemoveAll();
	m_arStateDiff3.RemoveAll();
	m_arLinesDiff3.RemoveAll();

	CTempFiles tempfiles;
	CRegDWORD regIgnoreWS = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreWS"));
	CRegDWORD regIgnoreEOL = CRegDWORD(_T("Software\\TortoiseMerge\\IgnoreEOL"), TRUE);
	CRegDWORD regIgnoreCase = CRegDWORD(_T("Software\\TortoiseMerge\\CaseInsensitive"), FALSE);
	DWORD dwIgnoreWS = regIgnoreWS;
	BOOL bIgnoreEOL = ((DWORD)regIgnoreEOL)!=0;
	BOOL bIgnoreCase = ((DWORD)regIgnoreCase)!=0;
	if (IsBaseFileInUse())
	{
		if (!m_arBaseFile.Load(m_baseFile.GetFilename()))
		{
			m_sError = m_arBaseFile.GetErrorString();
			return FALSE;
		}
		CFileTextLines converted(m_arBaseFile);
		sConvertedBaseFilename = tempfiles.GetTempFilePath();
		converted.Save(sConvertedBaseFilename, dwIgnoreWS, bIgnoreEOL, bIgnoreCase, m_bBlame);
	}

	if (IsTheirFileInUse())
	{
		// m_arBaseFile.GetCount() is passed as a hint for the number of lines in this file
		// It's a fair guess that the files will be roughly the same size
		if (!m_arTheirFile.Load(m_theirFile.GetFilename(),m_arBaseFile.GetCount()))
		{
			m_sError = m_arTheirFile.GetErrorString();
			return FALSE;
		}
		CFileTextLines converted(m_arTheirFile);
		sConvertedTheirFilename = tempfiles.GetTempFilePath();
		converted.Save(sConvertedTheirFilename, dwIgnoreWS, bIgnoreEOL, bIgnoreCase, m_bBlame);
	} // if (IsTheirFileInUse())

	if (IsYourFileInUse())
	{
		// m_arBaseFile.GetCount() is passed as a hint for the number of lines in this file
		// It's a fair guess that the files will be roughly the same size
		if (!m_arYourFile.Load(m_yourFile.GetFilename(),m_arBaseFile.GetCount()))
		{
			m_sError = m_arYourFile.GetErrorString();
			return FALSE;
		}
		CFileTextLines converted(m_arYourFile);
		sConvertedYourFilename = tempfiles.GetTempFilePath();
		converted.Save(sConvertedYourFilename, dwIgnoreWS, bIgnoreEOL, bIgnoreCase, m_bBlame);
	} // if (IsYourFileInUse()) 

	// Calculate the number of lines in the largest of the three files
	int lengthHint = max(m_arBaseFile.GetCount(), m_arTheirFile.GetCount());
	lengthHint = max(lengthHint, m_arYourFile.GetCount());

	m_arDiffYourBaseBoth.Reserve(lengthHint);
	m_arStateYourBaseBoth.Reserve(lengthHint);
	m_arDiffYourBaseLeft.Reserve(lengthHint);
	m_arStateYourBaseLeft.Reserve(lengthHint);
	m_arDiffYourBaseRight.Reserve(lengthHint);
	m_arStateYourBaseRight.Reserve(lengthHint);
	m_arDiffTheirBaseBoth.Reserve(lengthHint);
	m_arStateTheirBaseBoth.Reserve(lengthHint);
	m_arDiffTheirBaseLeft.Reserve(lengthHint);
	m_arStateTheirBaseLeft.Reserve(lengthHint);
	m_arDiffTheirBaseRight.Reserve(lengthHint);
	m_arStateTheirBaseRight.Reserve(lengthHint);

	// Is this a two-way diff?
	if (IsBaseFileInUse() && IsYourFileInUse() && !IsTheirFileInUse())
	{
		if (!DoTwoWayDiff(sConvertedBaseFilename, sConvertedYourFilename, dwIgnoreWS, pool))
		{
			apr_pool_destroy (pool);					// free the allocated memory
			return FALSE;
		}
	}

	if (IsBaseFileInUse() && IsTheirFileInUse() && !IsYourFileInUse())
	{
		ASSERT(FALSE);
	}

	if (IsBaseFileInUse() && IsTheirFileInUse() && IsYourFileInUse())
	{
		m_arDiff3.Reserve(lengthHint);
		m_arStateDiff3.Reserve(lengthHint);

		if (!DoThreeWayDiff(sConvertedBaseFilename, sConvertedYourFilename, sConvertedTheirFilename, pool))
		{
			apr_pool_destroy (pool);					// free the allocated memory
			return FALSE;
		}
	}

	apr_pool_destroy (pool);					// free the allocated memory
	return TRUE;
}


bool
CDiffData::DoTwoWayDiff(const CString& sBaseFilename, const CString& sYourFilename, DWORD dwIgnoreWS, apr_pool_t * pool)
{
	// convert CString filenames (UTF-16 or ANSI) to UTF-8
	CStringA sBaseFilenameUtf8 = CUnicodeUtils::GetUTF8(sBaseFilename);
	CStringA sYourFilenameUtf8 = CUnicodeUtils::GetUTF8(sYourFilename);

	svn_diff_t * diffYourBase = NULL;
	svn_error_t * svnerr = NULL;

	svnerr = svn_diff_file_diff(&diffYourBase, sBaseFilenameUtf8, sYourFilenameUtf8, pool);
	if (svnerr)
	{
		TRACE(_T("diff-error in CDiffData::Load()\n"));
		CString sMsg = CString(svnerr->message);
		while (svnerr->child)
		{
			svnerr = svnerr->child;
			sMsg += _T("\n");
			sMsg += CString(svnerr->message);
		} // while (m_err->child)
		apr_pool_destroy (pool);					// free the allocated memory
		m_sError.Format(IDS_ERR_DIFF_DIFF, sMsg);
		return false;
	} // if (m_svnerr)
	svn_diff_t * tempdiff = diffYourBase;
	LONG baseline = 0;
	LONG yourline = 0;

	while (tempdiff)
	{
		for (int i=0; i<tempdiff->original_length; i++)
		{
			if (baseline >= m_arBaseFile.GetCount())
			{
				m_sError.LoadString(IDS_ERR_DIFF_NEWLINES);
				return false;
			}
			const CString& sCurrentBaseLine = m_arBaseFile.GetAt(baseline);
			if (tempdiff->type == svn_diff__type_common)
			{
				if (yourline >= m_arYourFile.GetCount())
				{
					m_sError.LoadString(IDS_ERR_DIFF_NEWLINES);
					return false;
				}
				const CString& sCurrentYourLine = m_arYourFile.GetAt(yourline);
				m_arDiffYourBaseBoth.Add(sCurrentBaseLine);
				if (sCurrentBaseLine != sCurrentYourLine)
				{
					if (dwIgnoreWS == 2 || dwIgnoreWS == 3)
					{
						CString s1 = m_arBaseFile.GetAt(baseline);
						CString s2 = sCurrentYourLine;
						
						if ( dwIgnoreWS == 2 )
						{
							s1.TrimLeft(_T(" \t"));
							s2.TrimLeft(_T(" \t"));
						}
						else
						{
							s1.TrimRight(_T(" \t"));
							s2.TrimRight(_T(" \t"));
						}

						if (s1 != s2)
						{
							m_arStateYourBaseBoth.Add(DIFFSTATE_REMOVEDWHITESPACE);
							m_arLinesYourBaseBoth.Add(yourline);
							m_arDiffYourBaseBoth.Add(sCurrentYourLine);
							m_arStateYourBaseBoth.Add(DIFFSTATE_ADDEDWHITESPACE);
							m_arLinesYourBaseBoth.Add(yourline);
						}
						else
						{
							m_arStateYourBaseBoth.Add(DIFFSTATE_NORMAL);
							m_arLinesYourBaseBoth.Add(yourline);
						}
					}
					else if (dwIgnoreWS == 0)
					{
						m_arStateYourBaseBoth.Add(DIFFSTATE_REMOVEDWHITESPACE);
						m_arLinesYourBaseBoth.Add(yourline);
						m_arDiffYourBaseBoth.Add(sCurrentYourLine);
						m_arStateYourBaseBoth.Add(DIFFSTATE_ADDEDWHITESPACE);
						m_arLinesYourBaseBoth.Add(yourline);
					}
					else
					{
						m_arStateYourBaseBoth.Add(DIFFSTATE_NORMAL);
						m_arLinesYourBaseBoth.Add(yourline);
					}
				}
				else
				{
					m_arStateYourBaseBoth.Add(DIFFSTATE_NORMAL);
					m_arLinesYourBaseBoth.Add(yourline);
				}
				yourline++;		//in both files
			}
			else
			{
				m_arDiffYourBaseBoth.Add(sCurrentBaseLine);
				m_arStateYourBaseBoth.Add(DIFFSTATE_REMOVED);
				m_arLinesYourBaseBoth.Add(yourline);
			}
			baseline++;
		}
		if (tempdiff->type == svn_diff__type_diff_modified)
		{
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
				m_arStateYourBaseBoth.Add(DIFFSTATE_ADDED);
				m_arLinesYourBaseBoth.Add(yourline);
				yourline++;
			}
		}
		tempdiff = tempdiff->next;
	}

	tempdiff = diffYourBase;
	baseline = 0;
	yourline = 0;
	while (tempdiff)
	{
		if (tempdiff->type == svn_diff__type_common)
		{
			for (int i=0; i<tempdiff->original_length; i++)
			{
				const CString& sCurrentYourLine = m_arYourFile.GetAt(yourline);
				const CString& sCurrentBaseLine = m_arBaseFile.GetAt(baseline);
				m_arDiffYourBaseLeft.Add(sCurrentBaseLine);
				m_arDiffYourBaseRight.Add(sCurrentYourLine);
				m_arLinesYourBaseLeft.Add(baseline);
				m_arLinesYourBaseRight.Add(yourline);
				if (sCurrentBaseLine != sCurrentYourLine)
				{
					if (dwIgnoreWS == 2 || dwIgnoreWS == 3)
					{
						CString s1 = sCurrentBaseLine;
						CString s2 = sCurrentYourLine;
						if ( dwIgnoreWS == 2 )
						{
							s1 = s1.TrimLeft(_T(" \t"));
							s2 = s2.TrimLeft(_T(" \t"));
						}
						else
						{
							s1 = s1.TrimRight(_T(" \t"));
							s2 = s2.TrimRight(_T(" \t"));
						}
						
						if (s1 != s2)
						{
							m_arStateYourBaseLeft.Add(DIFFSTATE_WHITESPACE);
							m_arStateYourBaseRight.Add(DIFFSTATE_WHITESPACE);
						}
						else
						{
							m_arStateYourBaseLeft.Add(DIFFSTATE_NORMAL);
							m_arStateYourBaseRight.Add(DIFFSTATE_NORMAL);
						}
					}
					else if (dwIgnoreWS == 0)
					{
						m_arStateYourBaseLeft.Add(DIFFSTATE_WHITESPACE);
						m_arStateYourBaseRight.Add(DIFFSTATE_WHITESPACE);
					}
					else
					{
						m_arStateYourBaseLeft.Add(DIFFSTATE_NORMAL);
						m_arStateYourBaseRight.Add(DIFFSTATE_NORMAL);
					}
				}
				else
				{
					m_arStateYourBaseLeft.Add(DIFFSTATE_NORMAL);
					m_arStateYourBaseRight.Add(DIFFSTATE_NORMAL);
				}
				baseline++;
				yourline++;
			} // for (int i=0; i<tempdiff->original_length; i++)
		} // if (tempdiff->type == svn_diff__type_common)
		if (tempdiff->type == svn_diff__type_diff_modified)
		{
			apr_off_t original_length = tempdiff->original_length;
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				m_arDiffYourBaseRight.Add(m_arYourFile.GetAt(yourline++));
				if (original_length-- <= 0)
				{
					m_arDiffYourBaseLeft.Add(_T(""));
					m_arStateYourBaseLeft.Add(DIFFSTATE_EMPTY);
					m_arLinesYourBaseLeft.Add(DIFF_EMPTYLINENUMBER);
					m_arStateYourBaseRight.Add(DIFFSTATE_ADDED);
					m_arLinesYourBaseRight.Add(yourline-1);
				}
				else
				{
					m_arDiffYourBaseLeft.Add(m_arBaseFile.GetAt(baseline++));
					m_arStateYourBaseLeft.Add(DIFFSTATE_REMOVED);
					m_arLinesYourBaseLeft.Add(baseline-1);
					m_arStateYourBaseRight.Add(DIFFSTATE_ADDED);
					m_arLinesYourBaseRight.Add(yourline-1);
				}
			}
			apr_off_t modified_length = tempdiff->modified_length;
			for (int i=0; i<tempdiff->original_length; i++)
			{
				if (modified_length-- <= 0)
				{
					m_arDiffYourBaseLeft.Add(m_arBaseFile.GetAt(baseline++));
					m_arStateYourBaseLeft.Add(DIFFSTATE_REMOVED);
					m_arLinesYourBaseLeft.Add(baseline-1);
					m_arDiffYourBaseRight.Add(_T(""));
					m_arStateYourBaseRight.Add(DIFFSTATE_EMPTY);
					m_arLinesYourBaseRight.Add(DIFF_EMPTYLINENUMBER);
				}
			}
		}
		tempdiff = tempdiff->next;
	}
	TRACE(_T("done with 2-way diff\n"));

	return true;
}

bool
CDiffData::DoThreeWayDiff(const CString& sBaseFilename, const CString& sYourFilename, const CString& sTheirFilename, apr_pool_t * pool)
{
	// convert CString filenames (UTF-16 or ANSI) to UTF-8
	CStringA sBaseFilenameUtf8  = CUnicodeUtils::GetUTF8(sBaseFilename);
	CStringA sYourFilenameUtf8  = CUnicodeUtils::GetUTF8(sYourFilename);
	CStringA sTheirFilenameUtf8 = CUnicodeUtils::GetUTF8(sTheirFilename);
	svn_diff_t * diffTheirYourBase = NULL;
	svn_error_t * svnerr = svn_diff_file_diff3(&diffTheirYourBase, sBaseFilenameUtf8, sTheirFilenameUtf8, sYourFilenameUtf8, pool);
	if (svnerr)
	{
		TRACE(_T("diff-error in CDiffData::Load()\n"));
		CString sMsg = CString(svnerr->message);
		while (svnerr->child)
		{
			svnerr = svnerr->child;
			sMsg += _T("\n");
			sMsg += CString(svnerr->message);
		} // while (m_err->child)
		apr_pool_destroy (pool);					// free the allocated memory
		m_sError.Format(IDS_ERR_DIFF_DIFF, sMsg);
		return false;
	} // if (m_svnerr)
	svn_diff_t * tempdiff = diffTheirYourBase;
	LONG baseline = 0;
	LONG yourline = 0;
	LONG theirline = 0;
	LONG resline = 0;
	while (tempdiff)
	{
		if (tempdiff->type == svn_diff__type_common)
		{
			ASSERT((tempdiff->latest_length == tempdiff->modified_length) && (tempdiff->modified_length == tempdiff->original_length));
			for (int i=0; i<tempdiff->original_length; i++)
			{
				m_arDiff3.Add(m_arYourFile.GetAt(yourline));
				m_arStateDiff3.Add(DIFFSTATE_NORMAL);
				m_arLinesDiff3.Add(resline);
				m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
				m_arStateYourBaseBoth.Add(DIFFSTATE_NORMAL);
				m_arLinesYourBaseBoth.Add(yourline);
				m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
				m_arStateTheirBaseBoth.Add(DIFFSTATE_NORMAL);
				m_arLinesTheirBaseBoth.Add(theirline);
				baseline++;
				yourline++;
				theirline++;
				resline++;
			} // iff->original_length; i++)
		} // if (tempdiff->type == svn_diff__type_common)
		else if (tempdiff->type == svn_diff__type_diff_common)
		{
			ASSERT(tempdiff->latest_length == tempdiff->modified_length);
			//both theirs and yours have lines replaced
			for (int i=0; i<tempdiff->original_length; i++)
			{
				m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
				m_arStateDiff3.Add(DIFFSTATE_IDENTICALREMOVED);
				m_arLinesDiff3.Add(DIFF_EMPTYLINENUMBER);
				m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
				m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
				m_arLinesYourBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
				m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
				m_arLinesTheirBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				baseline++;
			} // iff->original_length; i++)
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				m_arDiff3.Add(m_arYourFile.GetAt(yourline));
				m_arStateDiff3.Add(DIFFSTATE_IDENTICALADDED);
				m_arLinesDiff3.Add(resline);
				m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
				m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALADDED);
				m_arLinesYourBaseBoth.Add(yourline);
				m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
				m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALADDED);
				m_arLinesTheirBaseBoth.Add(theirline);
				yourline++;
				theirline++;
				resline++;
			} // iff->original_length; i++)
		}
		else if (tempdiff->type == svn_diff__type_conflict)
		{
			apr_off_t length = max(tempdiff->original_length, tempdiff->modified_length);
			length = max(tempdiff->latest_length, length);
			apr_off_t original = tempdiff->original_length;
			apr_off_t modified = tempdiff->modified_length;
			apr_off_t latest = tempdiff->latest_length;

			apr_off_t originalresolved = 0;
			apr_off_t modifiedresolved = 0;
			apr_off_t latestresolved = 0;

			LONG base = baseline;
			LONG your = yourline;
			LONG their = theirline;
			if (tempdiff->resolved_diff)
			{
				originalresolved = tempdiff->resolved_diff->original_length;
				modifiedresolved = tempdiff->resolved_diff->modified_length;
				latestresolved = tempdiff->resolved_diff->latest_length;
			}
			for (int i=0; i<length; i++)
			{
				if (original)
				{
					m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
					m_arStateDiff3.Add(DIFFSTATE_IDENTICALREMOVED);
					m_arLinesDiff3.Add(DIFF_EMPTYLINENUMBER);
				}
				else if ((originalresolved)||((modifiedresolved)&&(latestresolved)))
				{
					m_arDiff3.Add(_T(""));
					m_arStateDiff3.Add(DIFFSTATE_IDENTICALREMOVED);
					m_arLinesDiff3.Add(DIFF_EMPTYLINENUMBER);
				}
				if ((latest)&&(original))
				{
					m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
					m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
					m_arLinesYourBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				}
				else
				{
					if (original)
					{
						m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
						m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						m_arLinesYourBaseBoth.Add(DIFF_EMPTYLINENUMBER);
					}
					else if ((latestresolved)&&(modifiedresolved))
					{
						m_arDiffYourBaseBoth.Add(_T(""));
						m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						m_arLinesYourBaseBoth.Add(DIFF_EMPTYLINENUMBER);
					}
				}
				if ((modified)&&(original))
				{
					m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
					m_arLinesTheirBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				}
				else
				{
					if (original)
					{
						m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
						m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						m_arLinesTheirBaseBoth.Add(DIFF_EMPTYLINENUMBER);
					}
					else if ((modifiedresolved)&&(latestresolved))
					{
						m_arDiffTheirBaseBoth.Add(_T(""));
						m_arStateTheirBaseBoth.Add(DIFFSTATE_IDENTICALREMOVED);
						m_arLinesTheirBaseBoth.Add(DIFF_EMPTYLINENUMBER);
					}
				}
				if (original)
				{
					original--;
					baseline++;
				}
				if (originalresolved)
					originalresolved--;

				if (modified)
				{
					modified--;
					theirline++;
				}
				if (modifiedresolved)
					modifiedresolved--;
				if (latest)
				{
					latest--;
					yourline++;
				}
				if (latestresolved)
					latestresolved--;
			}
			original = tempdiff->original_length;
			modified = tempdiff->modified_length;
			latest = tempdiff->latest_length;
			baseline = base;
			yourline = your;
			theirline = their;
			if (tempdiff->resolved_diff)
			{
				originalresolved = tempdiff->resolved_diff->original_length;
				modifiedresolved = tempdiff->resolved_diff->modified_length;
				latestresolved = tempdiff->resolved_diff->latest_length;
			}
			for (int i=0; i<length; i++)
			{
				if ((latest)||(modified))
				{
					m_arDiff3.Add(_T("--- Unresolved Conflict!! ---"));
					m_arStateDiff3.Add(DIFFSTATE_CONFLICTED);
					m_arLinesDiff3.Add(resline);
					resline++;
				}

				if (latest)
				{
					m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
					m_arStateYourBaseBoth.Add(DIFFSTATE_CONFLICTADDED);
					m_arLinesYourBaseBoth.Add(yourline);
				}
				else if ((latestresolved)||(modified)||(modifiedresolved))
				{
					m_arDiffYourBaseBoth.Add(_T(""));
					m_arStateYourBaseBoth.Add(DIFFSTATE_CONFLICTEMPTY);
					m_arLinesYourBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				}
				if (modified)
				{
					m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_CONFLICTADDED);
					m_arLinesTheirBaseBoth.Add(theirline);
				}
				else if ((modifiedresolved)||(latest)||(latestresolved))
				{
					m_arDiffTheirBaseBoth.Add(_T(""));
					m_arStateTheirBaseBoth.Add(DIFFSTATE_CONFLICTEMPTY);
					m_arLinesTheirBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				}
				if (original)
				{
					original--;
					baseline++;
				}
				if (originalresolved)
					originalresolved--;
				if (modified)
				{
					modified--;
					theirline++;
				}
				if (modifiedresolved)
					modifiedresolved--;
				if (latest)
				{
					latest--;
					yourline++;
				}
				if (latestresolved)
					latestresolved--;
			}
		}
		else if (tempdiff->type == svn_diff__type_diff_modified)
		{
			//deleted!
			for (int i=0; i<tempdiff->original_length; i++)
			{
				m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
				m_arStateDiff3.Add(DIFFSTATE_THEIRSREMOVED);
				m_arLinesDiff3.Add(DIFF_EMPTYLINENUMBER);
				m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
				m_arStateYourBaseBoth.Add(DIFFSTATE_NORMAL);
				m_arLinesYourBaseBoth.Add(yourline);
				m_arDiffTheirBaseBoth.Add(m_arBaseFile.GetAt(baseline));
				m_arStateTheirBaseBoth.Add(DIFFSTATE_THEIRSREMOVED);
				m_arLinesTheirBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				baseline++;
				yourline++;
			}
			//added
			for (int i=0; i<tempdiff->modified_length; i++)
			{
				m_arDiff3.Add(m_arTheirFile.GetAt(theirline));
				m_arStateDiff3.Add(DIFFSTATE_THEIRSADDED);
				m_arLinesDiff3.Add(resline);
				m_arDiffYourBaseBoth.Add(_T(""));
				m_arStateYourBaseBoth.Add(DIFFSTATE_EMPTY);
				m_arLinesYourBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
				m_arStateTheirBaseBoth.Add(DIFFSTATE_THEIRSADDED);
				m_arLinesTheirBaseBoth.Add(theirline);
				theirline++;
				resline++;
			}
		}
		else if (tempdiff->type == svn_diff__type_diff_latest)
		{
			//YOURS differs from base

			for (int i=0; i<tempdiff->original_length; i++)
			{
				m_arDiff3.Add(m_arBaseFile.GetAt(baseline));
				m_arStateDiff3.Add(DIFFSTATE_YOURSREMOVED);
				m_arLinesDiff3.Add(DIFF_EMPTYLINENUMBER);
				m_arDiffYourBaseBoth.Add(m_arBaseFile.GetAt(baseline));
				m_arStateYourBaseBoth.Add(DIFFSTATE_YOURSREMOVED);
				m_arLinesYourBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				m_arDiffTheirBaseBoth.Add(m_arTheirFile.GetAt(theirline));
				m_arStateTheirBaseBoth.Add(DIFFSTATE_NORMAL);
				m_arLinesTheirBaseBoth.Add(theirline);
				baseline++;
				theirline++;
			}
			for (int i=0; i<tempdiff->latest_length; i++)
			{
				m_arDiff3.Add(m_arYourFile.GetAt(yourline));
				m_arStateDiff3.Add(DIFFSTATE_YOURSADDED);
				m_arLinesDiff3.Add(resline);
				m_arDiffYourBaseBoth.Add(m_arYourFile.GetAt(yourline));
				m_arStateYourBaseBoth.Add(DIFFSTATE_IDENTICALADDED);
				m_arLinesYourBaseBoth.Add(yourline);
				m_arDiffTheirBaseBoth.Add(_T(""));
				m_arStateTheirBaseBoth.Add(DIFFSTATE_EMPTY);
				m_arLinesTheirBaseBoth.Add(DIFF_EMPTYLINENUMBER);
				yourline++;
				resline++;
			}
		}
		else
		{
			TRACE(_T("something bad happened during diff!\n"));
		}
		tempdiff = tempdiff->next;

	}
	ASSERT(m_arDiff3.GetCount() == m_arDiffYourBaseBoth.GetCount());
	ASSERT(m_arDiffTheirBaseBoth.GetCount() == m_arDiffYourBaseBoth.GetCount());

	TRACE(_T("done with 3-way diff\n"));
	return true;
}
