// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#include "SVNLogHelper.h"

BOOL SVNLogHelper::Log(LONG rev, const CString& author, const CString& /*date*/, const CString& message, LogChangedPathArray * cpaths, apr_time_t /*time*/, int /*filechanges*/, BOOL /*copies*/, DWORD /*actions*/, BOOL /*haschildren*/)
{
	messages[rev] = message;
	authors[rev] = author;
	m_rev = rev;
	for (int i=0; i<cpaths->GetCount(); ++i)
	{
		LogChangedPath * cpath = cpaths->GetAt(i);
		if (m_relativeurl.Compare(cpath->sPath)== 0)
		{
			m_copyfromurl = m_reposroot + cpath->sCopyFromPath;
			m_rev = cpath->lCopyFromRev;
		}
	}
	return TRUE;
}

SVNRev SVNLogHelper::GetCopyFromRev(CTSVNPath url, SVNRev pegrev, CString& copyfromURL)
{
	SVNRev rev;

	messages.clear();
	authors.clear();
	if (m_reposroot.IsEmpty())
		m_reposroot = GetRepositoryRoot(url);
	m_relativeurl = url.GetSVNPathString().Mid(m_reposroot.GetLength());
	if (ReceiveLog (CTSVNPathList(url), pegrev, SVNRev::REV_HEAD, 1, 0, TRUE, FALSE, true))
	{
		rev = m_rev;
		copyfromURL = m_copyfromurl;
	}
	return rev;
}

bool SVNLogHelper::GetLogMessagesAndAuthors(CTSVNPath url, SVNRev start, SVNRev end, SVNRev pegrev)
{
	messages.clear();
	authors.clear();
	return !!ReceiveLog(CTSVNPathList(url), pegrev, start, end, 0, FALSE, FALSE, true);
}
