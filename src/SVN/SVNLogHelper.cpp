// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007,2009,2011 - TortoiseSVN

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
    if (!pegrev.IsNumber())
        return SVNRev();

    std::auto_ptr<const CCacheLogQuery> query
        (ReceiveLog (CTSVNPathList(url), pegrev, pegrev, 1, 0, TRUE, FALSE, false));
    if (query.get() == NULL)
        return result;

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

    // no copy-from-info?

    if (!iterator.GetPath().IsValid())
        return SVNRev();

    // return the results

    copyfromURL = MakeUIUrlOrPath (  query->GetRootURL()
                                   + iterator.GetPath().GetPath().c_str());
    return result;
}

std::vector<std::pair<CTSVNPath, SVNRev> >
SVNLogHelper::GetCopyHistory(const CTSVNPath& url, const SVNRev& pegrev)
{
    std::vector<std::pair<CTSVNPath, SVNRev> > result;

    CTSVNPath path = url;
    SVNRev rev = pegrev.IsHead() ? GetHEADRevision (url) : pegrev;

    while (rev.IsNumber() && !path.IsEmpty())
    {
        result.push_back (std::make_pair (path, rev));

        CString copyFromURL;
        rev = GetCopyFromRev(path, rev, copyFromURL);
        path.SetFromSVN (copyFromURL);
    }

    return result;
}

std::pair<CTSVNPath, SVNRev>
SVNLogHelper::GetCommonSource(const CTSVNPath& url1, const SVNRev& pegrev1,
                              const CTSVNPath& url2, const SVNRev& pegrev2)
{
    // get the full copy histories of both urls

    auto copyHistory1 = GetCopyHistory(url1, pegrev1);
    auto copyHistory2 = GetCopyHistory(url2, pegrev2);

    // starting from the oldest paths, look for the first divergence
    // (this may be important in the case of cyclic renames)

    auto iter1 = copyHistory1.rbegin(), end1 = copyHistory1.rend();
    auto iter2 = copyHistory2.rbegin(), end2 = copyHistory2.rend();
    for (;    iter1 != end1 && iter2 != end2
           && iter1->first.IsEquivalentTo (iter2->first)
         ; ++iter1, ++iter2 )
    {
    }

    // if even the oldest paths were different, then there is no common source

    if (iter1 == copyHistory1.rbegin())
        return std::pair<CTSVNPath, SVNRev>();

    // return the last revision in the common history

    --iter1;
    --iter2;
    SVNRev commonRev = min ((LONG)iter1->second, (LONG)iter2->second);
    return std::make_pair (iter1->first, commonRev);
}

