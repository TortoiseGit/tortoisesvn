// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007,2009 - TortoiseSVN

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
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "Access\StrictLogIterator.h"

SVNRev SVNLogHelper::GetCopyFromRev(CTSVNPath url, SVNRev pegrev, CString& copyfromURL)
{
    SVNRev result;

    // fill / update a suitable log cache

    if (pegrev.IsHead())
        pegrev = GetHEADRevision (url);

    std::auto_ptr<const CCacheLogQuery> query
        (ReceiveLog (CTSVNPathList(url), pegrev, SVNRev::REV_HEAD, 1, 0, TRUE, FALSE, false));
    if (query.get() == NULL)
        return result;

    // get a concrete revision for the log start

    assert (pegrev.IsNumber());

	// construct the path object 
	// (URLs are always escaped, so we must unescape them)

    CStringA svnURLPath = CUnicodeUtils::GetUTF8 (url.GetSVNPathString());
	if (svnURLPath.Left(9).CompareNoCase("file:///\\") == 0)
		svnURLPath.Delete (7, 2);

    CStringA relPath = svnURLPath.Mid (query->GetRootURL().GetLength());
	relPath = CPathUtils::PathUnescape (relPath);

	const CPathDictionary* paths = &query->GetCache()->GetLogInfo().GetPaths();
	CDictionaryBasedTempPath path (paths, (const char*)relPath);

    // follow the log

    LogCache::CStrictLogIterator iterator 
        ( query->GetCache()
        , pegrev
        , path);

    result = pegrev;
    iterator.Retry();
    while (!iterator.EndOfPath())
    {
        result = iterator.GetRevision();
        iterator.Advance();
    }

    // return the results

    copyfromURL = MakeUIUrlOrPath (  query->GetRootURL() 
                                   + iterator.GetPath().GetPath().c_str());
	return result;
}

