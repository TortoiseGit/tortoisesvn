// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
#include "LogDlgFilter.h"
#include "LogDlg.h"

bool CLogDlgFilter::Match (wstring& text) const
{
    if (patterns.empty())
    {
        // normalize to lower case

        _wcslwr_s (&text.at(0), text.length()+1);
	    for (vector<wstring>::const_iterator it = subStrings.begin(); it != subStrings.end(); ++it)
            if (wcsstr (text.c_str(), it->c_str()) == NULL)
                return false;
    }
    else
    {
	    for (vector<tr1::wregex>::const_iterator it = patterns.begin(); it != patterns.end(); ++it)
		    if (!regex_search(text, *it, tr1::regex_constants::match_any))
                return false;
    }

    return true;
}

CLogDlgFilter::CLogDlgFilter 
    ( const CString& filter
	, bool filterWithRegex
    , int selectedFilter
    , __time64_t from
    , __time64_t to
    , bool scanRelevantPathsOnly
    , svn_revnum_t revToKeep)

    : negate (false)
    , attributeSelector ( selectedFilter == LOGFILTER_ALL 
                        ? UINT_MAX
                        : (1 << selectedFilter))
    , from (from)
    , to (to)
    , scanRelevantPathsOnly (scanRelevantPathsOnly)
    , revToKeep (revToKeep)
{
    // decode string matching spec

	bool useRegex = filterWithRegex && !filter.IsEmpty();
	CString sFilterText = filter;

	// if the first char is '!', negate the filter
	if (filter.GetLength() && filter[0] == '!')
	{
		negate = true;
		sFilterText = sFilterText.Mid(1);
	}

	if (useRegex)
        useRegex = CLogDataVector::ValidateRegexp (sFilterText, patterns);

	if (!useRegex)
	{
		// now split the search string into words so we can search for each of them

		CString sToken;
		int curPos = 0;
		sToken = sFilterText.Tokenize(_T(" "), curPos);
		while (!sToken.IsEmpty())
		{
            sToken.MakeLower();
            subStrings.push_back ((LPCTSTR)sToken);
			sToken = sFilterText.Tokenize(_T(" "), curPos);
		}
	}
}

bool CLogDlgFilter::operator() (const LogEntryData& entry) const
{
    // quick checks

    if (entry.GetRevision() == revToKeep)
        return true;

    __time64_t date = entry.GetDate();
    if ((date < from) || (date > to))
        return false;

    if (patterns.empty() && subStrings.empty())
        return !negate;

    // we need to perform expensive string / pattern matching

	wstring searchText;
	searchText.reserve(4096);

	if (attributeSelector & (1 << LOGFILTER_BUGID))
	{
		searchText.append(entry.GetBugIDs());
	}
	if (attributeSelector & (1 << LOGFILTER_MESSAGES))
	{
		searchText.append(_T(" "));
		searchText.append(entry.GetMessage());
	}
	if (attributeSelector & (1 << LOGFILTER_PATHS))
	{
		const LogChangedPathArray& paths = entry.GetChangedPaths();
		for ( size_t cpPathIndex = 0, pathCount = paths.GetCount()
            ; cpPathIndex < pathCount
            ; ++cpPathIndex)
		{
			const LogChangedPath& cpath = paths[cpPathIndex];
			if (!scanRelevantPathsOnly || cpath.IsRelevantForStartPath())
            {
			    searchText.append(_T(" "));
			    searchText.append(cpath.GetCopyFromPath());
			    searchText.append(_T(" "));
			    searchText.append(cpath.GetPath());
			    searchText.append(_T(" "));
			    searchText.append(cpath.GetActionString());
            }
		}
	}
	if (attributeSelector & (1 << LOGFILTER_AUTHORS))
	{
		searchText.append(_T(" "));
		searchText.append(entry.GetAuthor());
	}
	if (attributeSelector & (1 << LOGFILTER_REVS))
	{
		searchText.append(_T(" "));

        wchar_t buffer[10];
        _itow_s (entry.GetRevision(), buffer, 10);
		searchText.append (buffer);
	}
			
    return Match (searchText) ^ negate;
}

