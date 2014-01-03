// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006, 2010, 2013-2014 - TortoiseSVN

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

struct TSVNCacheResponse;
#define CACHETIMEOUT    0x7FFFFFFFFFFFFFFF
extern ULONGLONG cachetimeout;

/**
 * \ingroup TSVNCache
 * Holds all the status data of one file or folder.
 */
class CStatusCacheEntry
{
public:
    CStatusCacheEntry();
    CStatusCacheEntry(const svn_client_status_t* pSVNStatus, bool needsLock, __int64 lastWriteTime, bool forceNormal);
    bool HasExpired(LONGLONG now) const;
    void BuildCacheResponse(TSVNCacheResponse& response, DWORD& responseLength) const;
    bool IsVersioned() const;
    bool DoesFileTimeMatch(__int64 testTime) const;
    bool ForceStatus(svn_wc_status_kind forcedStatus);
    svn_wc_status_kind GetEffectiveStatus() const { return m_highestPriorityLocalStatus; }
    bool IsKindKnown() const { return ((m_kind != svn_node_none)&&(m_kind != svn_node_unknown)); }
    void SetStatus(const svn_client_status_t* pSVNStatus, bool needsLock, bool forceNormal);
    bool HasBeenSet() const;
    void Invalidate();
    bool IsDirectory() const {return ((m_kind == svn_node_dir)&&(m_highestPriorityLocalStatus != svn_wc_status_ignored));}
    bool SaveToDisk(FILE * pFile);
    bool LoadFromDisk(FILE * pFile);
    void SetKind(svn_node_kind_t kind) {m_kind = kind;}
private:
    void SetAsUnversioned();

private:
    __int64             m_lastWriteTime;
    LONGLONG            m_discardAtTime;
    svn_revnum_t        m_commitRevision;

    struct
    {
        svn_wc_status_kind node_status:5;
        svn_wc_status_kind text_status:5;
        svn_wc_status_kind prop_status:5;

        svn_boolean_t locked:1;
        svn_boolean_t copied:1;
        svn_boolean_t switched:1;

        svn_wc_status_kind repos_text_status:5;
        svn_wc_status_kind repos_prop_status:5;
    } m_svnStatus;

    struct
    {
        svn_wc_status_kind  m_highestPriorityLocalStatus:5;
        svn_node_kind_t     m_kind:3;
        svn_boolean_t       m_bSet:1;
        svn_boolean_t       m_treeconflict:1;
        svn_boolean_t       m_bIgnoreOnCommit:1;

        // Values copied from the 'entries' structure
        svn_boolean_t       m_bSVNEntryFieldSet:1;
        svn_boolean_t       m_bHasOwner:1;
        svn_boolean_t       m_needsLock:1;
    };
//  friend class CSVNStatusCache;
};
