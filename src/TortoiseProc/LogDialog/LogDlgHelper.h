// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012,2013 - TortoiseSVN

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
#include "LogDlgDataModel.h"
#include "LogCacheGlobals.h"
#include "QuickHashSet.h"

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
    CStoreSelection(CLogDlg* dlg, const SVNRevRangeArray& revRange);
    ~CStoreSelection();
    void ClearSelection();
protected:
    CLogDlg* m_logdlg;
    quick_hash_set<long> m_SetSelectedRevisions;
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

public:

    /// construction

    CLogCacheUtility
        ( LogCache::CCachedLogInfo* cache
        , ProjectProperties* projectProperties = NULL);

    /// \returns @a false if standard revprops or changed paths are
    /// missing for the specififed \ref revision.

    bool IsCached (svn_revnum_t revision) const;

    /// \returns NULL if \ref IsCached returns false for that \ref revision.
    /// Otherwise, all cached log information for the respective revision
    /// will be returned.
    /// The bCopiedSelf, bChecked and hasChildren members will always be
    /// @a FALSE; childStackDepth will be 0.

    PLOGENTRYDATA GetRevisionData (svn_revnum_t revision);
};

/**
 * \ingroup TortoiseProc
 * Helper class containing info used for context menu for revisions selections.
 */
class CContextMenuInfoForRevisions
{
public:
    CContextMenuInfoForRevisions()
    {
        SelLogEntry = NULL;
        AllFromTheSameAuthor = false;
        RevPrevious = 0;
        RevSelected = 0;
        RevSelected2 = 0;
        RevHighest = 0;
        RevLowest = 0;
        PathURL = _T("");
        SelEntries.clear();
        RevisionRanges.Clear();
    }

    ~CContextMenuInfoForRevisions()
    {
        SelEntries.clear();
        RevisionRanges.Clear();
    }

    PLOGENTRYDATA SelLogEntry;
    std::vector<PLOGENTRYDATA> SelEntries;
    bool AllFromTheSameAuthor;
    SVNRevRangeArray RevisionRanges;
    SVNRev RevPrevious;
    SVNRev RevSelected;
    SVNRev RevSelected2;
    SVNRev RevHighest;
    SVNRev RevLowest;
    CString PathURL;
};

// get an alias to a shared automatic pointer
typedef std::shared_ptr<CContextMenuInfoForRevisions> ContextMenuInfoForRevisionsPtr;


/**
 * \ingroup TortoiseProc
 * Helper class containing info used for context menu for changed paths selections.
 */
class CContextMenuInfoForChangedPaths
{
public:
    CContextMenuInfoForChangedPaths()
    {
        Rev1 = 0;
        Rev2 = 0;
        OneRev = false;
        ChangedPaths.clear();
        ChangedLogPathIndices.clear();
        sUrl = _T("");
        wcPath = _T("");
        fileUrl = _T("");
    }

    ~CContextMenuInfoForChangedPaths()
    {
        ChangedPaths.clear();
        ChangedLogPathIndices.clear();
    }

    std::vector<CString> ChangedPaths;
    std::vector<size_t> ChangedLogPathIndices;
    svn_revnum_t Rev1;
    svn_revnum_t Rev2;
    CString sUrl;
    CString fileUrl;
    CString wcPath;

    bool OneRev;
};

// get an alias to a shared automatic pointer
typedef std::shared_ptr<CContextMenuInfoForChangedPaths> ContextMenuInfoForChangedPathsPtr;

/**
  * \ingroup TortoiseProc
  * Helper class to set the hour glass while executing external programs.
  * Utility also disables the OK button - reverts everything when going out of scope
  */
class CLogWndHourglass
{
    public:
        CLogWndHourglass(CLogDlg* parent);
        ~CLogWndHourglass();

    private:
        CLogDlg* m_pLogDlg;
};
