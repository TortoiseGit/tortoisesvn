// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "LogCacheGlobals.h"
#include "QuickHashMap.h"

class CLogDlg;

/**
 * \ingroup TortoiseProc
 * Instances of CStoreSelection save the selection of the CLogDlg. When the instance
 * is deleted the destructor restores the selection.
 */
class CStoreSelection
{
public:
	CStoreSelection(CLogDlg* dlg);
	~CStoreSelection();
protected:
	CLogDlg* m_logdlg;
	std::set<LONG> m_SetSelectedRevisions;
};

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
    DWORD GetChildStackDepth() const {return childStackDepth;}

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

public:
	/// De-allocates log items.
	void ClearAll();

    /// add / remove items

    void Add (PLOGENTRYDATA item);
    void AddSorted (PLOGENTRYDATA item);
    void RemoveLast();

    /// access to unfilered info

    size_t size() const {return inherited::size();}
    PLOGENTRYDATA operator[](size_t index) const {return at (index);}

    /// access to the filtered info

    size_t GetVisibleCount() const;
    PLOGENTRYDATA GetVivisible (size_t index) const;

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

};

/**
 * \ingroup TortoiseProc
 * Helper class for the log dialog, provides some utility functions to
 * directly access arbitrary revisions w/o iterating.
 */
class CLogCacheUtility
{
private:

    /// access the info from this cache:

    LogCache::CCachedLogInfo* cache;

    /// optional: if NULL, sShortMessage and sBugIDs will not be set

    ProjectProperties* projectProperties;

    /// efficient map cached string / path -> CString

    typedef quick_hash_map<LogCache::index_t, CString> TID2String;
    TID2String pathToStringMap;

public:

    /// construction

    CLogCacheUtility 
        ( LogCache::CCachedLogInfo* cache
        , ProjectProperties* projectProperties = NULL);

    /// \returns @a false if standard revprops or changed paths are
    /// missing for the specififed \ref revision.

    bool IsCached (svn_revnum_t revision) const;

    /// \returns NULL if \ref IsCached returns false for that \ref revision.
    /// Otherwise, all cached log information for the respective revisin 
    /// will be returned. 
    /// The bCopiedSelf, bChecked and hasChildren members will always be 
    /// @a FALSE; childStackDepth will be 0.

    PLOGENTRYDATA GetRevisionData (svn_revnum_t revision);
};
