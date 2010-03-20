// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010 - TortoiseSVN

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

/// forward declaration

class CLogEntryData;

/// structure containing the pre-processed filter spec.
/// Note that the \ref Matches() function cannot be called
/// from multiple threads. For use in a multi-threaded
/// application, clone the filter instance (one per thread).

class CLogDlgFilter
{
private:

    /// if empty, use sub-string matching

    vector<tr1::wregex> patterns;

    /// list of sub-strings to find; normalized to lower case

    vector<wstring> subStrings;

    /// indicated for each entry in \ref subStrings
    /// whether is had been predicated by a '-'

    vector<bool> exclude;

    /// negate pattern matching result

    bool negate;

    /// attribute selector 
    /// (i.e. what members of LogEntryData shall be used for comparison)

    DWORD attributeSelector;

    /// if false, normalize all strings to lower case before comparing them

    bool caseSensitive;

    /// if set, we don't need to apply locales to convert the string

    bool fastLowerCase;

    /// date range to filter for

    __time64_t from;
    __time64_t to;

    /// test paths only if they are related to the log path

    bool scanRelevantPathsOnly;

    /// revision number that will uncondionally return true

    svn_revnum_t revToKeep;

    /// temp / scratch objects to minimize the number memory 
    /// allocation operations

    mutable wstring scratch;
    mutable string utf8PathScratch;
    mutable wstring utf16PathScratch;

    /// called to parse a (potentially incorrect) regex spec

    bool ValidateRegexp 
        ( LPCTSTR regexp_str
        , vector<tr1::wregex>& patterns);

    // construction utility

    void AddSubString (CString token, bool negate);

public:

    /// construction

	CLogDlgFilter();
    CLogDlgFilter 
        ( const CString& filter
        , bool filterWithRegex
        , int selectedFilter
        , bool caseSensitive
        , __time64_t from
        , __time64_t to
        , bool scanRelevantPathsOnly
        , svn_revnum_t revToKeep);

    /// apply filter

    bool operator() (const CLogEntryData& entry) const;

	/// returns a vector with all the ranges where a match
	/// was found.
	std::vector<CHARRANGE> GetMatchRanges (wstring& text) const;

	/// filter utiltiy method
	bool Match (wstring& text) const;

    /// tr1::regex is very slow when running concurrently 
    /// in multiple threads. Empty filters don't need MT as well.

    bool BenefitsFromMT() const;
};
