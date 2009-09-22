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
#include "LogDlgDataModel.h"
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
