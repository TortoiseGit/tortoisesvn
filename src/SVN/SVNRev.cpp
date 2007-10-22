// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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

#include "StdAfx.h"
#include "SVNRev.h"
#include "svn_time.h"
#include "SVNHelpers.h"
#include <algorithm>


SVNRev::SVNRev(CString sRev)
{
	ZeroMemory(&rev, sizeof(svn_opt_revision_t));
	Create(sRev);
}

void SVNRev::Create(CString sRev)
{
	sDate.Empty();
	m_bIsValid = FALSE;
	memset (&rev, 0, sizeof (rev));
	if (sRev.Left(1).Compare(_T("{"))==0)
	{
		// brackets denote a date
		SVNPool pool;
		svn_boolean_t matched;
		apr_time_t tm;

		if ((apr_pool_t*)pool)
		{
			CStringA sRevA = CStringA(sRev);
			sRevA = sRevA.Mid(1, sRevA.GetLength()-2);
			svn_error_t * err = svn_parse_date(&matched, &tm, sRevA, apr_time_now(), pool);
			if (err == NULL)
			{
				if (!matched)
					return;
				rev.kind = svn_opt_revision_date;
				rev.value.date = tm;
				m_bIsValid = TRUE;
				sDate = sRev;
			}
			svn_error_clear(err);
		}
	}
	else if (sRev.CompareNoCase(_T("HEAD"))==0)
	{
		rev.kind = svn_opt_revision_head;
		m_bIsValid = TRUE;
	}
	else if (sRev.CompareNoCase(_T("BASE"))==0)
	{
		rev.kind = svn_opt_revision_base;
		m_bIsValid = TRUE;
	}
	else if (sRev.CompareNoCase(_T("WC"))==0)
	{
		rev.kind = svn_opt_revision_working;
		m_bIsValid = TRUE;
	}
	else if (sRev.CompareNoCase(_T("PREV"))==0)
	{
		rev.kind = svn_opt_revision_previous;
		m_bIsValid = TRUE;
	}
	else if (sRev.CompareNoCase(_T("COMMITTED"))==0)
	{
		rev.kind = svn_opt_revision_committed;
		m_bIsValid = TRUE;
	}
	else if (sRev.IsEmpty())
	{
		rev.kind = svn_opt_revision_head;
		m_bIsValid = TRUE;
	}
	else
	{
		bool bAllNumbers = true;
		for (int i=0; i<sRev.GetLength(); ++i)
		{
			if (!_istdigit(sRev[i]))
				bAllNumbers = false;
		}
		if (bAllNumbers)
		{
			LONG nRev = _ttol(sRev);
			if (nRev > 0)
			{
				rev.kind = svn_opt_revision_number;
				rev.value.number = nRev;
				m_bIsValid = TRUE;
			}
		}
	}
}

SVNRev::SVNRev(svn_revnum_t nRev)
{
	Create(nRev);
}

void SVNRev::Create(svn_revnum_t nRev)
{
	m_bIsValid = TRUE;
	memset (&rev, 0, sizeof (rev));
	if(nRev == SVNRev::REV_HEAD)
	{
		rev.kind = svn_opt_revision_head;
	} 
	else if (nRev == SVNRev::REV_BASE)
	{
		rev.kind = svn_opt_revision_base;
	}
	else if (nRev == SVNRev::REV_WC)
	{
		rev.kind = svn_opt_revision_working;
	}
	else if (nRev == SVNRev::REV_UNSPECIFIED)
	{
		rev.kind = svn_opt_revision_unspecified;
		m_bIsValid = FALSE;
	}
	else
	{
		rev.kind = svn_opt_revision_number;
		rev.value.number = nRev;
	}
}

bool SVNRev::IsEqual(const SVNRev& revision)
{
	if (rev.kind != revision.GetKind())
		return false;
	if (IsNumber())
	{
		return (rev.value.number == LONG(revision));
	}
	if (IsDate())
	{
		return (rev.value.date == revision.GetDate());
	}
	return true;
}

SVNRev::~SVNRev()
{
}

SVNRev::operator LONG() const
{
	switch (rev.kind)
	{
	case svn_opt_revision_head:		return SVNRev::REV_HEAD;
	case svn_opt_revision_base:		return SVNRev::REV_BASE;
	case svn_opt_revision_working:	return SVNRev::REV_WC;
	case svn_opt_revision_number:	return rev.value.number;
	case svn_opt_revision_unspecified: return SVNRev::REV_UNSPECIFIED;
	}
	return SVNRev::REV_HEAD;
}

SVNRev::operator svn_opt_revision_t * ()
{
	return &rev;
}

SVNRev::operator const svn_opt_revision_t * () const
{
	return &rev;
}

CString SVNRev::ToString() const
{
	CString sRev;
	switch (rev.kind)
	{
	case svn_opt_revision_head:			return _T("HEAD");
	case svn_opt_revision_base:			return _T("BASE");
	case svn_opt_revision_working:		return _T("WC");
	case svn_opt_revision_number:		sRev.Format(_T("%ld"), rev.value);return sRev;
	case svn_opt_revision_committed:	return _T("COMMITTED");
	case svn_opt_revision_previous:		return _T("PREV");
	case svn_opt_revision_unspecified:	return _T("UNSPECIFIED");
	case svn_opt_revision_date:			return GetDateString();
	}
	return sRev;
}


//////////////////////////////////////////////////////////////////////////


int SVNRevRangeArray::AddRevRange(const SVNRev& start, const SVNRev& end)
{
	SVNRevRange revrange(start, end);
	return AddRevRange(revrange);
}

int SVNRevRangeArray::AddRevRange(const SVNRevRange& revrange)
{
	m_array.push_back(revrange);
	return GetCount();
}

int SVNRevRangeArray::AddRevision(const SVNRev& revision)
{
	ATLASSERT(revision.IsNumber());
	svn_revnum_t nRev = revision;
	for (int i = 0; i < GetCount(); ++i)
	{
		svn_revnum_t start = m_array[i].GetStartRevision();
		svn_revnum_t end = m_array[i].GetEndRevision();
		bool reversed = false;
		if (start > end)
		{
			svn_revnum_t t = start;
			start = end;
			end = t;
			reversed = true;
		}
		if ((start <= nRev)&&(nRev <= end))
			return GetCount();	// revision is inside an existing range
		if (start == nRev + 1)
		{
			if (reversed)
				m_array[i] = SVNRevRange(start, nRev);
			else
				m_array[i] = SVNRevRange(nRev, end);
			return GetCount();
		}
		if (end == nRev - 1)
		{
			if (reversed)
				m_array[i] = SVNRevRange(nRev, end);
			else
				m_array[i] = SVNRevRange(start, nRev);
			return GetCount();
		}
	}
	return AddRevRange(SVNRevRange(revision, revision));
}

void SVNRevRangeArray::AdjustForMerge(bool bReverse /* = false */)
{
	for (int i = 0; i < GetCount(); ++i)
	{
		SVNRevRange range = m_array[i];
		if (range.GetStartRevision().IsNumber())
		{
			if (range.GetEndRevision().IsNumber())
			{
				// both ends of the range are revision numbers
				if (bReverse)
				{
					// reverse merge means: start is the higher value + 1, end is the lower value
					svn_revnum_t start = range.GetStartRevision();
					svn_revnum_t end = range.GetEndRevision();
					if (start > end)
					{
						svn_revnum_t t = start;
						start = end;
						end = t;
					}
					m_array[i] = SVNRevRange(end+1, start);					
				}
				else
				{
					// normal merge means: start is the lower value - 1, end is the higher value
					svn_revnum_t start = range.GetStartRevision();
					svn_revnum_t end = range.GetEndRevision();
					if (start > end)
					{
						svn_revnum_t t = start;
						start = end;
						end = t;
					}
					m_array[i] = SVNRevRange(start-1, end);					
				}
			}
			else
			{
				// only the end revision is not a number, we have to adjust the start revision
				if (bReverse)
					m_array[i] = SVNRevRange(LONG(range.GetStartRevision())+1, range.GetEndRevision());
				else
					m_array[i] = SVNRevRange(LONG(range.GetStartRevision())-1, range.GetEndRevision());
			}
		}
		else
		{
			if (range.GetEndRevision().IsNumber())
			{
				// only the start revision is not a number, we have to adjust the end revision
				if (bReverse)
					m_array[i] = SVNRevRange(range.GetStartRevision(), LONG(range.GetEndRevision())+1);
				else
					m_array[i] = SVNRevRange(range.GetStartRevision(), LONG(range.GetEndRevision())-1);
			}
		}
	}
}

int SVNRevRangeArray::GetCount() const
{
	return (int)m_array.size();
}

void SVNRevRangeArray::Clear()
{
	m_array.clear();
}

const SVNRevRange& SVNRevRangeArray::operator[](int index) const
{
	ATLASSERT(index >= 0 && index < (int)m_array.size());
	return m_array[index];
}

bool SVNRevRangeArray::FromListString(const CString& string)
{
	Clear();

	if (string.GetLength())
	{
		const TCHAR * str = (LPCTSTR)string;
		const TCHAR * result = _tcspbrk((LPCTSTR)string, _T(",-"));
		SVNRev prevRev;
		while (result)
		{
			if (*result == ',')
			{
				SVNRev rev = SVNRev(CString(str, result-str));
				if (!rev.IsValid())
				{
					Clear();
					return false;
				}
				if (prevRev.IsValid())
				{
					AddRevRange(prevRev, rev);
				}
				else
					AddRevRange(rev, rev);
				prevRev = SVNRev();
			}
			else if (*result == '-')
			{
				prevRev = SVNRev(CString(str, result-str));
				if (!prevRev.IsValid())
				{
					Clear();
					return false;
				}
			}
			result++;
			str = result;
			result = _tcspbrk(result, _T(",-"));
		}
		SVNRev rev = SVNRev(CString(str));
		if (!rev.IsValid())
		{
			Clear();
			return false;
		}
		if (prevRev.IsValid())
		{
			AddRevRange(prevRev, rev);
		}
		else
			AddRevRange(rev, rev);
	}

	return true;
}

CString SVNRevRangeArray::ToListString()
{
	CString sRet;
	for (int i = 0; i < GetCount(); ++i)
	{
		if (!sRet.IsEmpty())
			sRet += _T(",");
		SVNRevRange range = (*this)[i];
		if (range.GetStartRevision().IsEqual(range.GetEndRevision()))
			sRet += range.GetStartRevision().ToString();
		else
			sRet += range.GetStartRevision().ToString() + _T("-") + range.GetEndRevision().ToString();
	}
	return sRet;
}

const apr_array_header_t * SVNRevRangeArray::GetAprArray(apr_pool_t * pool)
{
	apr_array_header_t * sources = apr_array_make(pool, GetCount(),
		sizeof(svn_opt_revision_range_t *));

	for (int nItem = 0; nItem < GetCount(); ++nItem)
	{
		APR_ARRAY_PUSH(sources, svn_opt_revision_range_t *) = (svn_opt_revision_range_t*)m_array[nItem];
	}
	return sources;
}




#if defined(_DEBUG) && defined(_MFC_VER)
// Some test cases for these classes
static class SVNRevListTests
{
public:
	SVNRevListTests()
	{
		SVNRevRangeArray array;
		array.AddRevRange(SVNRev(1), SVNRev(1));
		array.AddRevRange(SVNRev(3), SVNRev(5));
		array.AddRevRange(SVNRev(7), SVNRev(9));
		array.AddRevRange(SVNRev(20), SVNRev(20));
		ATLASSERT(_tcscmp((LPCTSTR)array.ToListString(), _T("1,3-5,7-9,20"))==0);
		SVNRevRangeArray array2;
		array2.FromListString(array.ToListString());
		ATLASSERT(array2.GetCount()==4);
	}
} SVNRevListTests;
#endif
