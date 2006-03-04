// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include ".\colors.h"

CColors::CColors(void) :
	m_regAdded(_T("Software\\TortoiseSVN\\Colors\\Added"), RGB(100, 0, 100))
	, m_regConflict(_T("Software\\TortoiseSVN\\Colors\\Conflict"), RGB(255, 0, 0))
	, m_regModified(_T("Software\\TortoiseSVN\\Colors\\Modified"), RGB(0, 50, 160))
	, m_regMerged(_T("Software\\TortoiseSVN\\Colors\\Merged"), RGB(0, 100, 0))
	, m_regDeleted(_T("Software\\TortoiseSVN\\Colors\\Deleted"), RGB(100, 0, 0))
	, m_regSkipped(_T("Software\\TortoiseSVN\\Colors\\Skipped"), RGB(255, 0, 0))
	, m_regLastCommit(_T("Software\\TortoiseSVN\\Colors\\LastCommit"), RGB(100, 100, 100))
	, m_regDeletedNode(_T("Software\\TortoiseSVN\\Colors\\DeletedNode"), RGB(255, 0, 0))
	, m_regAddedNode(_T("Software\\TortoiseSVN\\Colors\\AddedNode"), RGB(0, 255, 0))
	, m_regReplacedNode(_T("Software\\TortoiseSVN\\Colors\\ReplacedNode"), RGB(0, 255, 0))
	, m_regRenamedNode(_T("Software\\TortoiseSVN\\Colors\\RenamedNode"), RGB(0, 0, 255))
	, m_regLastCommitNode(_T("Software\\TortoiseSVN\\Colors\\LastCommitNode"), RGB(200, 200, 200))
{
}

CColors::~CColors(void)
{
}

COLORREF CColors::GetColor(Colors col, bool bDefault /*=true*/)
{
	switch (col)
	{
	case Conflict:
		if (bDefault)
			return RGB(255, 0, 0);
		return (COLORREF)(DWORD)m_regConflict;
	case Modified:
		if (bDefault)
			return RGB(0, 50, 160);
		return (COLORREF)(DWORD)m_regModified;
	case Merged:
		if (bDefault)
			return RGB(0, 100, 0);
		return (COLORREF)(DWORD)m_regMerged;
	case Deleted:
		if (bDefault)
			return RGB(100, 0, 0);
		return (COLORREF)(DWORD)m_regDeleted;
	case Added:
		if (bDefault)
			return RGB(100, 0, 100);
		return (COLORREF)(DWORD)m_regAdded;
	case Skipped:
		if (bDefault)
			return RGB(255, 0, 0);
		return (COLORREF)(DWORD)m_regSkipped;
	case LastCommit:
		if (bDefault)
			return RGB(100, 100, 100);
		return (COLORREF)(DWORD)m_regAdded;
	case DeletedNode:
		if (bDefault)
			return RGB(255, 0, 0);
		return (COLORREF)(DWORD)m_regDeletedNode;
	case AddedNode:
		if (bDefault)
			return RGB(0, 255, 0);
		return (COLORREF)(DWORD)m_regAddedNode;
	case ReplacedNode:
		if (bDefault)
			return RGB(0, 255, 0);
		return (COLORREF)(DWORD)m_regReplacedNode;
	case RenamedNode:
		if (bDefault)
			return RGB(0, 0, 255);
		return (COLORREF)(DWORD)m_regRenamedNode;
	case LastCommitNode:
		if (bDefault)
			return RGB(200, 200, 200);
		return (COLORREF)(DWORD)m_regLastCommitNode;
	}
	return RGB(0,0,0);
}

void CColors::SetColor(Colors col, COLORREF cr)
{
	switch (col)
	{
	case Conflict:
		m_regConflict = cr;
		break;
	case Modified:
		m_regModified = cr;
		break;
	case Merged:
		m_regMerged = cr;
		break;
	case Deleted:
		m_regDeleted = cr;
		break;
	case Added:
		m_regAdded = cr;
		break;
	case Skipped:
		m_regSkipped = cr;
		break;
	case LastCommit:
		m_regLastCommit = cr;
	default:
		ATLASSERT(false);
	}
}