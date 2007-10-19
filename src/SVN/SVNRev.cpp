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

#ifdef _MFC_VER

int SVNRevList::AddRevision(const SVNRev& rev)
{
	m_array.push_back(rev);
	m_sort = SVNRevListNoSort;
	return GetCount();
}

int SVNRevList::GetCount() const
{
	return (int)m_array.size();
}

void SVNRevList::Clear()
{
	m_array.clear();
}

const SVNRev& SVNRevList::operator[](int index) const
{
	ATLASSERT(index >= 0 && index < (int)m_array.size());
	return m_array[index];
}

bool SVNRevList::SaveToFile(LPCTSTR path, bool bANSI)
{
	try
	{
		if (bANSI)
		{
			CStdioFile file(path, CFile::typeText | CFile::modeReadWrite | CFile::modeCreate);
			CStringA temp;
			temp.Format("%d\n", m_sort);
			file.Write(temp, temp.GetLength());
			for (std::vector<SVNRev>::const_iterator it = m_array.begin(); it != m_array.end(); ++it)
			{
				CStringA line = CStringA(it->ToString()) + '\n';
				file.Write(line, line.GetLength());
			}
			file.Close();
		}
		else
		{
			CStdioFile file(path, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
			CString temp;
			temp.Format(_T("%d\n"), m_sort);
			file.Write(temp, temp.GetLength());
			for (std::vector<SVNRev>::const_iterator it = m_array.begin(); it != m_array.end(); ++it)
			{
				file.WriteString(it->ToString()+_T("\n"));
			} 
			file.Close();
		}
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in writing temp file\n");
		pE->Delete();
		return false;
	}

	return true;
}

bool SVNRevList::LoadFromFile(LPCTSTR path)
{
	Clear();
	try
	{
		CString strLine;
		CStdioFile file(path, CFile::typeBinary | CFile::modeRead | CFile::shareDenyWrite);

		if (file.ReadString(strLine))
		{
			m_sort = (SVNRevListSort)_ttol(strLine);
			while (file.ReadString(strLine))
			{
				AddRevision(SVNRev(strLine));
			}
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException loading target file list\n");
		pE->Delete();
		return false;
	}
	return true;
}

bool SVNRevList::FromListString(LPCTSTR string)
{
	Clear();
	if (_tcslen(string))
	{
		const TCHAR * str = string;
		const TCHAR * result = _tcspbrk(string, _T(",-"));
		SVNRev prevRev;
		while (result)
		{
			if (*result == ',')
			{
				SVNRev rev = SVNRev(std::wstring(str, result-str).c_str());
				if (!rev.IsValid())
				{
					Clear();
					return false;
				}
				if (prevRev.IsValid())
				{
					for (svn_revnum_t i = prevRev; i <= rev; ++i)
						AddRevision(SVNRev(i));
				}
				else
					AddRevision(rev);
				prevRev = SVNRev();
			}
			else if (*result == '-')
			{
				prevRev = SVNRev(std::wstring(str, result-str).c_str());
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
		SVNRev rev = SVNRev(std::wstring(str).c_str());
		if (prevRev.IsValid())
		{
			for (svn_revnum_t i = prevRev; i <= rev; ++i)
				AddRevision(SVNRev(i));
		}
		else
			AddRevision(rev);
	}

	return true;
}

std::wstring SVNRevList::ToListString(bool bCompact)
{
	std::wstring sRet;
	if (bCompact)
	{
		int index = 0;
		do
		{
			if (index < GetCount())
			{
				ATLASSERT((*this)[index].IsNumber());
				SVNRev start = (svn_revnum_t)(*this)[index];
				SVNRev end = SVNRev();
				svn_revnum_t r = (*this)[index++];
				while ((index < GetCount())&&((r+1) == (svn_revnum_t)(*this)[index]))
				{
					end = (*this)[index];
					r = end;
					index++;
				}
				if (!end.IsValid())
				{
					if (!sRet.empty())
						sRet += _T(",");	
					sRet += (LPCTSTR)start.ToString();
				}
				else
				{
					if (!sRet.empty())
						sRet += _T(",");	
					sRet += (LPCTSTR)start.ToString();
					sRet += _T("-");
					sRet += (LPCTSTR)end.ToString();
				}
			}
		} while (index < GetCount());
	}
	else
	{
		for (int i = 0; i < GetCount(); ++i)
		{
			if (!sRet.empty())
				sRet += _T(",");	
			sRet += (LPCTSTR)(*this)[i].ToString();
		}
	}
	return sRet;
}

bool SVNRevList::AscendingRevision(const SVNRev& lhs, const SVNRev& rhs)
{
	if (lhs.IsNumber() && rhs.IsNumber())
		return (svn_revnum_t)lhs < (svn_revnum_t)rhs;
	if (lhs.IsHead())
		return false;
	if (rhs.IsHead())
		return true;
	if (lhs.IsDate() && rhs.IsDate())
		return lhs.GetDate() < rhs.GetDate();
	ATLASSERT(FALSE);
	return true;
}

bool SVNRevList::DescendingRevision(const SVNRev& lhs, const SVNRev& rhs)
{
	if (lhs.IsNumber() && rhs.IsNumber())
		return (svn_revnum_t)lhs > (svn_revnum_t)rhs;
	if (lhs.IsHead())
		return true;
	if (rhs.IsHead())
		return false;
	if (lhs.IsDate() && rhs.IsDate())
		return lhs.GetDate() > rhs.GetDate();
	ATLASSERT(FALSE);
	return false;
}

void SVNRevList::Sort(bool bAscending)
{
	m_sort = SVNRevListNoSort;
	if (bAscending)
	{
		std::sort(m_array.begin(), m_array.end(), AscendingRevision);
		m_sort = SVNRevListASCENDING;
	}
	else
	{
		std::sort(m_array.begin(), m_array.end(), DescendingRevision);
		m_sort = SVNRevListDESCENDING;
	}
}

#endif

#if defined(_DEBUG) && defined(_MFC_VER)
// Some test cases for these classes
static class SVNRevListTests
{
public:
	SVNRevListTests()
	{
		SVNRevList list;
		list.AddRevision(SVNRev(1));
		list.AddRevision(SVNRev(3));
		list.AddRevision(SVNRev(4));
		list.AddRevision(SVNRev(5));
		list.AddRevision(SVNRev(7));
		list.AddRevision(SVNRev(8));
		list.AddRevision(SVNRev(9));
		list.AddRevision(SVNRev(20));
		ATLASSERT(_tcscmp(list.ToListString(true).c_str(), _T("1,3-5,7-9,20"))==0);
		SVNRevList list2;
		list2.FromListString(list.ToListString(true).c_str());
		ATLASSERT(list2.GetCount()==8);
		ATLASSERT(svn_revnum_t(list2[0]) == 1);
		ATLASSERT(svn_revnum_t(list2[1]) == 3);
		ATLASSERT(svn_revnum_t(list2[2]) == 4);
		ATLASSERT(svn_revnum_t(list2[3]) == 5);
		ATLASSERT(svn_revnum_t(list2[4]) == 7);
		ATLASSERT(svn_revnum_t(list2[5]) == 8);
		ATLASSERT(svn_revnum_t(list2[6]) == 9);
		ATLASSERT(svn_revnum_t(list2[7]) == 20);
	}
} SVNRevListTests;
#endif
