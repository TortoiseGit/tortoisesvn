// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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

class CLogDlgFilter;

/**
 * \ingroup TortoiseProc
 * Contains the data of one log entry, used in the log dialog
 */

class LogEntryData
{   
private:

    /// encapsulate data

    LogEntryData* parent;
	bool hasChildren;
	DWORD childStackDepth;

    svn_revnum_t Rev;
	__time64_t tmDate;
	CString sDate;
	CString sAuthor;
	CString sMessage;
	CString sShortMessage;
	CString sBugIDs;

	const LogChangedPathArray* changedPaths;

	bool checked;

    /// no copy support

    LogEntryData (const LogEntryData&);
    LogEntryData& operator=(const LogEntryData&);

public:

    /// initialization

    LogEntryData ( LogEntryData* parent
                 , svn_revnum_t Rev
                 , __time64_t tmDate
                 , const CString& sDate
                 , const CString& sAuthor
                 , const CString& sMessage
                 , ProjectProperties* projectProperties
                 , LogChangedPathArray* changedPaths
                 , CString& selfRelativeURL);

    /// destruction

    ~LogEntryData();

    /// modification

    void SetAuthor 
        ( const CString& author);
    void SetMessage 
        ( const CString& message
        , ProjectProperties* projectProperties);
    void SetChecked
        ( bool newState);

    /// r/o access to the data

    LogEntryData* GetParent() {return parent;}
    const LogEntryData* GetParent() const {return parent;}
    bool HasChildren() const {return hasChildren;}
    DWORD GetDepth() const {return childStackDepth;}

    svn_revnum_t GetRevision() const {return Rev;}
    __time64_t GetDate() const {return tmDate;}

    const CString& GetDateString() const {return sDate;}
	const CString& GetAuthor() const {return sAuthor;}
	const CString& GetMessage() const {return sMessage;}
	const CString& GetShortMessage() const {return sShortMessage;}
	const CString& GetBugIDs() const {return sBugIDs;}

    const LogChangedPathArray& GetChangedPaths() const {return *changedPaths;}

    bool GetChecked() const {return checked;}
};

typedef LogEntryData LOGENTRYDATA, *PLOGENTRYDATA;

/**
 * \ingroup TortoiseProc
 * Helper class for the log dialog, handles all the log entries, including
 * sorting.
 */
class CLogDataVector : private std::vector<PLOGENTRYDATA>
{
private:

    typedef std::vector<PLOGENTRYDATA> inherited;

    /// indices of visible entries

    std::vector<size_t> visible;

    /// max of LogEntryData::GetDepth

	DWORD maxDepth;

	__time64_t minDate;
	__time64_t maxDate;

    svn_revnum_t minRevision;
    svn_revnum_t maxRevision;

    /// used temporarily when fetching logs with merge info

    std::vector<PLOGENTRYDATA> logParents;   
	
    /// filter utiltiy method

    std::vector<size_t> FilterRange 
        ( const CLogDlgFilter* filter
        , size_t first
        , size_t last);

public:

    /// construction

    CLogDataVector();

	/// De-allocates log items.

	void ClearAll();

    /// add / remove items

    void Add ( svn_revnum_t revision
             , __time64_t tmDate
             , const CString& date
             , const CString& author
             , const CString& message
             , ProjectProperties* projectProperties
             , LogChangedPathArray* changedPaths
             , CString& selfRelativeURL
             , bool childrenFollow);

    void AddSorted ( PLOGENTRYDATA item
                   , ProjectProperties* projectProperties);
    void RemoveLast();

    /// access to unfilered info

    size_t size() const {return inherited::size();}
    PLOGENTRYDATA operator[](size_t index) const {return at (index);}

    DWORD GetMaxDepth() const {return maxDepth;}
    __time64_t GetMinDate() const {return minDate;}
	__time64_t GetMaxDate() const {return maxDate;}
    svn_revnum_t GetMinRevision() const {return minRevision;}
    svn_revnum_t GetMaxRevision() const {return maxRevision;}

    /// access to the filtered info

    size_t GetVisibleCount() const;
    PLOGENTRYDATA GetVisible (size_t index) const;

    /// encapsulate sorting

    enum SortColumn
    {
        RevisionCol = 0,
        ActionCol,
        AuthorCol,
        DateCol,
        BugTraqCol,
        MessageCol
    };

    void Sort (SortColumn column, bool ascending);

    /// filter support

    void Filter (const CLogDlgFilter& filter);
    void Filter (__time64_t from, __time64_t to);

    void ClearFilter();
};
