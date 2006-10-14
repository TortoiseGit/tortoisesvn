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
#include "SVNUrl.h"

#include "SVN.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include "SVNRev.h"


SVNUrl::SVNUrl()
{
}

SVNUrl::SVNUrl(const CString& svn_url, bool bAlreadyUnescaped /* = false*/ ) :
	CString(bAlreadyUnescaped ? svn_url : Unescape(svn_url))
{
}

SVNUrl::SVNUrl(const CString& path, const CString& revision) :
	CString(Unescape(path + _T("?") + revision))
{
}

SVNUrl::SVNUrl(const CString& path, const SVNRev& revision) :
	CString(Unescape(path + _T("?") + GetTextFromRev(revision)))
{
}

SVNUrl::SVNUrl(const SVNUrl& other) :
	CString(other)
{
}

SVNUrl& SVNUrl::operator=(const CString& svn_url)
{
	CString::operator=(Unescape(svn_url));
	return *this;
}

SVNUrl& SVNUrl::operator=(const SVNUrl& svn_url)
{
	CString::operator=(svn_url);
	return *this;
}



// SVNUrl public interface

void SVNUrl::SetPath(const CString& path)
{
	CString new_path = Unescape(path);

	int rev_pos = ReverseFind(_T('?'));
	if (rev_pos >= 0)
		*this = new_path + Mid(rev_pos);
	else
		*this = new_path;
}

CString SVNUrl::GetPath() const
{
	CString path = *this;

	int rev_pos = path.ReverseFind(_T('?'));
	if (rev_pos >= 0)
		path = path.Left(rev_pos);

	return path;
}

void SVNUrl::SetRevision(const SVNRev& revision)
{
	CString svn_url;

	if (revision.IsHead())
	{
		svn_url = GetPath();
	}
	else
	{
		svn_url = GetPath() + GetTextFromRev(revision);
	}

	*this = svn_url;
}

SVNRev SVNUrl::GetRevision() const
{
	int rev_pos = ReverseFind(_T('?'));

	if (rev_pos < 0)
	{
		return SVNRev(SVNRev::REV_HEAD);
	}
	else
	{
		return SVNRev(Mid(rev_pos + 1));
	}
}

CString SVNUrl::GetRevisionText() const
{
	int rev_pos = ReverseFind(_T('?'));

	if (rev_pos < 0)
		return _T("HEAD");
	else
		return Mid(rev_pos + 1).MakeUpper();
}

bool SVNUrl::IsRoot() const
{
	return GetParentPath().IsEmpty();
}

CString SVNUrl::GetRootPath() const
{
	SVNUrl url = *this;

	while (!url.IsRoot())
		url = url.GetParentPath();

	return url;
}

CString SVNUrl::GetParentPath() const
{
	CString path = GetPath();

	int rev_pos = path.ReverseFind('/');

	if (rev_pos <= 0)
	{
		path = _T("");
	}
	else
	{
		path = path.Left(rev_pos);
		path.TrimRight('/');
		if (path.ReverseFind('/') < 0)
			path = _T("");
	}

	return path;
}

CString SVNUrl::GetName() const
{
	CString path = GetPath();

	int rev_pos = path.ReverseFind('/');

	if (rev_pos > 0)
	{
		path = path.Left(rev_pos - 1);
		if (path.ReverseFind('/') < 0)
			path = GetPath();
	}

	return path;
}



// SVNUrl static helpers

CString SVNUrl::Unescape(const CString& url)
{
	CString new_url = url;
	SVN::preparePath(new_url);

	CStringA temp = CUnicodeUtils::GetUTF8(new_url);
	CPathUtils::Unescape(temp.GetBuffer());
	temp.ReleaseBuffer();
	return CUnicodeUtils::GetUnicode(temp);
}

CString SVNUrl::GetTextFromRev(const SVNRev& revision)
{
	CString rev_text;

	if (revision.IsHead())
	{
		rev_text = _T("HEAD");
	}
	else if (revision.IsBase())
	{
		rev_text = _T("BASE");
	}
	else if (revision.IsWorking())
	{
		rev_text = _T("WC");
	}
	else if (revision.IsPrev())
	{
		rev_text = _T("PREV");
	}
	else if (revision.IsCommitted())
	{
		rev_text = _T("COMMITTED");
	}
	else if (revision.IsDate())
	{
		rev_text = revision.GetDateString();
	}
	else
	{
		rev_text.Format(_T("%u"), (LONG)revision);
	}

	return rev_text;
}

 

#ifdef _DEBUG

// This test can easily be integrated in a CPPUNIT framework

#ifndef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(x) ASSERT(x)
#endif

static class CSVNUrlTest {
public:
	CSVNUrlTest()
	{
		// Constructor tests
		{
			CPPUNIT_ASSERT( SVNUrl().IsEmpty() );
			CPPUNIT_ASSERT( SVNUrl() == _T(""));
			CPPUNIT_ASSERT( SVNUrl(_T("")).IsEmpty() );
			CPPUNIT_ASSERT( SVNUrl(_T("")) == _T(""));
			CPPUNIT_ASSERT( SVNUrl(_T("x")) == _T("x"));
			CPPUNIT_ASSERT( SVNUrl(_T("/x")) == _T("/x"));
			CPPUNIT_ASSERT( SVNUrl(_T("/x/")) == _T("/x"));
			CPPUNIT_ASSERT( SVNUrl(_T("\\x")) == _T("/x"));
			CPPUNIT_ASSERT( SVNUrl(_T("\\x\\")) == _T("/x"));
//			CPPUNIT_ASSERT( SVNUrl(_T("M%FCller")) == _T("Müller"));
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b/c")) == _T("http://a/b/c"));
			CPPUNIT_ASSERT( SVNUrl(_T("http:\\\\a\\b\\c")) == _T("http://a/b/c"));
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a/b/c")) == _T("file:///x:/a/b/c"));
			CPPUNIT_ASSERT( SVNUrl(_T("file:\\\\\\x:\\a\\b\\c")) == _T("file:///x:/a/b/c"));
		}

		// GetPath/SetPath tests
		{
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b")).GetPath() == _T("http://a/b") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b/")).GetPath() == _T("http://a/b") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?22")).GetPath() == _T("http://a/b") );
//			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b/?22")).GetPath() == _T("http://a/b") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a/b")).GetPath() == _T("file:///a/b") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a/b/")).GetPath() == _T("file:///a/b") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a/b?22")).GetPath() == _T("file:///a/b") );
//			CPPUNIT_ASSERT( SVNUrl(_T("file:///a/b/?22")).GetPath() == _T("file:///a/b") );
		}

		// GetRevision/SetRevision tests
		{
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b")).GetRevision().IsHead() );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b/")).GetRevision().IsHead() );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?42")).GetRevision() == 42 );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b/?42")).GetRevision() == 42 );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?HEAD")).GetRevision().IsHead() );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?BASE")).GetRevision().IsBase() );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?WC")).GetRevision().IsWorking() );

			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b")).GetRevisionText() == _T("HEAD") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?42")).GetRevisionText() == _T("42") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?HEAD")).GetRevisionText() == _T("HEAD") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?bAsE")).GetRevisionText() == _T("BASE") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b?wC")).GetRevisionText() == _T("WC") );
		}

		// URL root tests
		{
			CPPUNIT_ASSERT( SVNUrl(_T("http://a")).IsRoot() == true );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a")).GetRootPath() == _T("http://a") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/")).IsRoot() == true );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/")).GetRootPath() == _T("http://a") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b")).IsRoot() == false );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a/b")).GetRootPath() == _T("http://a") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a.b.com")).IsRoot() == true );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a.b.com")).GetRootPath() == _T("http://a.b.com") );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a.b.com/d/e")).IsRoot() == false );
			CPPUNIT_ASSERT( SVNUrl(_T("http://a.b.com/d/e")).GetRootPath() == _T("http://a.b.com") );

			CPPUNIT_ASSERT( SVNUrl(_T("file:///a")).IsRoot() == true );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a")).GetRootPath() == _T("file:///a") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a/")).IsRoot() == true );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a/")).GetRootPath() == _T("file:///a") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a/b")).IsRoot() == false );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a/b")).GetRootPath() == _T("file:///a") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a.b.com")).IsRoot() == true );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a.b.com")).GetRootPath() == _T("file:///a.b.com") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a.b.com/d/e")).IsRoot() == false );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///a.b.com/d/e")).GetRootPath() == _T("file:///a.b.com") );

			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:")).IsRoot() == true );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:")).GetRootPath() == _T("file:///x:") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/")).IsRoot() == true );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/")).GetRootPath() == _T("file:///x:") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a")).IsRoot() == false );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a")).GetRootPath() == _T("file:///x:") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a")).GetPath() == _T("file:///x:/a") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a/")).IsRoot() == false );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a/")).GetRootPath() == _T("file:///x:") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a/")).GetPath() == _T("file:///x:/a") );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a/b")).IsRoot() == false );
			CPPUNIT_ASSERT( SVNUrl(_T("file:///x:/a/b")).GetRootPath() == _T("file:///x:") );
		}
	}
} SVNUrlTest;

#endif
