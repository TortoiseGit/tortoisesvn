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
#pragma once
#include "SVN.h"

#include <map>

using namespace std;

/**
 * \ingroup SVN
 * helper class for retrieving log messages without the log dialog.
 *
 * Can find the copy from revision of a tag/branch and the corresponding copy from URL.
 * Can also save log messages to a temp file for later use.
 */
class SVNLogHelper : public SVN
{
public:
    /**
     * Finds the copy from revision and URL of a branch/tag URL.
     * \param url           the url of the branch/tag to find the copy from data for (input)
     * \param pegrev        the peg revision to use to find the copy from data (input)
     * \param copyfromURL   the url the branch/tag was copied from (output)
     * \return              the copy from revision
     */
    SVNRev GetCopyFromRev(CTSVNPath url, SVNRev pegrev, CString& copyfromURL);

    /**
     * Finds all copy-from (path,rev) pairs along the history.
     * The first entry will contain the parameters passed to this function.
     * \param url           the url of the branch/tag to find the copy from data for
     * \param pegrev        the peg revision to use to find the copy from data
     * \return              full list of all copy operations
     */
    std::vector<std::pair<CTSVNPath, SVNRev> >
    SVNLogHelper::GetCopyHistory(const CTSVNPath& url, const SVNRev& pegrev);

    /**
     * Finds the lastest common (path,revision) pair in the history of the two
     * given (path, revision) pairs.
     * \param url1          the url of the first path
     * \param pegrev1       the peg and start revision for the search at @a url1
     * \param url2          the url of the second path
     * \param pegrev2       the peg and start revision for the search at @a url2
     * \return              latest common (path,rev) pair. Empty, if none found.
     */
    std::pair<CTSVNPath, SVNRev>
    GetCommonSource(const CTSVNPath& url1, const SVNRev& pegrev1,
                    const CTSVNPath& url2, const SVNRev& pegrev2);
};
