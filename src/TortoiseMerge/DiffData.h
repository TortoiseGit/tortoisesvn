#pragma once

#include "svn_diff.h"
#include "apr_pools.h"
#include "FileTextLines.h"

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
		DIFFSTATE_YOURSADDED
	} ;

	BOOL						Load();
	int							GetLineCount();
	int							GetLineActualLength(int index);
	LPCTSTR						GetLineChars(int index);
	static void					GetColors(DiffStates state, COLORREF &crBkgnd, COLORREF &crText);
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
};
