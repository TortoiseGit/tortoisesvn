// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012,2013-2016, 2020-2022 - TortoiseSVN

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
#include "DragDropImpl.h"

class CLogDlg;

/**
 * \ingroup TortoiseProc
 * Instances of CStoreSelection save the selection of the CLogDlg. When the instance
 * is deleted the destructor restores the selection.
 */
class CStoreSelection
{
public:
    explicit CStoreSelection(CLogDlg* dlg);
    CStoreSelection(CLogDlg* dlg, const SVNRevRangeArray& revRange);
    ~CStoreSelection();
    void ClearSelection();
    void AddSelections();
    void RestoreSelection();

protected:
    CLogDlg*       m_logDlg;
    std::set<long> m_setSelectedRevisions;
    bool           m_keepWhenAdding;
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

    explicit CLogCacheUtility(LogCache::CCachedLogInfo* cache, ProjectProperties* projectProperties = nullptr);

    /// \returns @a false if standard revprops or changed paths are
    /// missing for the specififed \ref revision.

    bool IsCached(svn_revnum_t revision) const;

    /// \returns NULL if \ref IsCached returns false for that \ref revision.
    /// Otherwise, all cached log information for the respective revision
    /// will be returned.
    /// The bCopiedSelf, bChecked and hasChildren members will always be
    /// @a FALSE; childStackDepth will be 0.

    std::unique_ptr<LOGENTRYDATA> GetRevisionData(svn_revnum_t revision) const;
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
        selLogEntry          = nullptr;
        allFromTheSameAuthor = false;
        revPrevious          = 0;
        revSelected          = 0;
        revSelected2         = 0;
        revHighest           = 0;
        revLowest            = 0;
        pathURL              = L"";
        selEntries.clear();
        revisionRanges.Clear();
    }

    ~CContextMenuInfoForRevisions()
    {
        selEntries.clear();
        revisionRanges.Clear();
    }

    PLOGENTRYDATA              selLogEntry;
    std::vector<PLOGENTRYDATA> selEntries;
    bool                       allFromTheSameAuthor;
    SVNRevRangeArray           revisionRanges;
    SVNRev                     revPrevious;
    SVNRev                     revSelected;
    SVNRev                     revSelected2;
    SVNRev                     revHighest;
    SVNRev                     revLowest;
    CString                    pathURL;
};


/**
 * \ingroup TortoiseProc
 * Helper class containing info used for context menu for changed paths selections.
 */
class CContextMenuInfoForChangedPaths
{
public:
    CContextMenuInfoForChangedPaths()
    {
        rev1   = 0;
        rev2   = 0;
        oneRev = false;
        changedPaths.clear();
        changedLogPathIndices.clear();
        sUrl    = L"";
        wcPath  = L"";
        fileUrl = L"";
    }

    ~CContextMenuInfoForChangedPaths()
    {
        changedPaths.clear();
        changedLogPathIndices.clear();
    }

    std::vector<CString> changedPaths;
    std::vector<size_t>  changedLogPathIndices;
    svn_revnum_t         rev1;
    svn_revnum_t         rev2;
    CString              sUrl;    // url is escaped
    CString              fileUrl; // url is unescaped
    CString              wcPath;

    bool oneRev;
};

// get an alias to a shared automatic pointer
typedef std::shared_ptr<CContextMenuInfoForChangedPaths> ContextMenuInfoForChangedPathsPtr;

/**
  * \ingroup TortoiseProc
  * Helper class to set the hour glass while executing external programs.
  */
class CLogWndHourglass
{
public:
    CLogWndHourglass();
    ~CLogWndHourglass();
};

class CMonitorTreeTarget : public CIDropTarget
{
public:
    explicit CMonitorTreeTarget(CLogDlg* pLogDlg);

    void HandleDropFormats(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* pdwEffect, POINTL pt, const CString& targetUrl) const;

    bool                      OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* pdwEffect, POINTL pt) override;
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject __RPC_FAR* pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect) override;
    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect) override;
    HRESULT STDMETHODCALLTYPE DragLeave() override;

protected:
    CLogDlg*  m_pLogDlg;
    ULONGLONG m_ullHoverStartTicks;
    HTREEITEM hLastItem;

    CString sNoDrop;
    CString sImportDrop;
};
