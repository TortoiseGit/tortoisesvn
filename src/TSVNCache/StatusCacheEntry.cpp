// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2006,2008, 2010 - TortoiseSVN

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
#include "StdAfx.h"
#include ".\statuscacheentry.h"
#include "SVNStatus.h"
#include "CacheInterface.h"
#include "registry.h"

#define CACHEENTRYDISKVERSION 9

DWORD cachetimeout = (DWORD)CRegStdDWORD(_T("Software\\TortoiseSVN\\Cachetimeout"), CACHETIMEOUT);

CStatusCacheEntry::CStatusCacheEntry()
{
    m_bSet = false;
    m_bSVNEntryFieldSet = false;
    m_kind = svn_node_unknown;
    m_highestPriorityLocalStatus = svn_wc_status_none;

    SetAsUnversioned();
}

CStatusCacheEntry::CStatusCacheEntry(const svn_client_status_t* pSVNStatus, bool needsLock, __int64 lastWriteTime, bool forceNormal)
{
    m_bSet = false;
    m_bSVNEntryFieldSet = false;
    m_kind = svn_node_unknown;
    m_highestPriorityLocalStatus = svn_wc_status_none;

    SetStatus(pSVNStatus, needsLock, forceNormal);
    m_lastWriteTime = lastWriteTime;
    m_discardAtTime = GetTickCount()+cachetimeout;
}

bool CStatusCacheEntry::SaveToDisk(FILE * pFile)
{
#define WRITEVALUETOFILE(x) if (fwrite(&x, sizeof(x), 1, pFile)!=1) return false;
#define WRITECOPYVALUETOFILE(x) if (fputc((char)x, pFile)==EOF) return false;

    unsigned int value = CACHEENTRYDISKVERSION;
    WRITEVALUETOFILE(value); // 'version' of this save-format

    WRITECOPYVALUETOFILE(m_highestPriorityLocalStatus);
    WRITEVALUETOFILE(m_lastWriteTime);
    WRITECOPYVALUETOFILE(m_bSet);
    WRITECOPYVALUETOFILE(m_bSVNEntryFieldSet);
    WRITEVALUETOFILE(m_commitRevision);
    WRITECOPYVALUETOFILE(m_bHasOwner);   
    WRITECOPYVALUETOFILE(m_kind);
    WRITECOPYVALUETOFILE(m_needsLock);

    // now save the status struct (without the entry field, because we don't use that)
    WRITECOPYVALUETOFILE(m_svnStatus.copied);
    WRITECOPYVALUETOFILE(m_svnStatus.locked);
    WRITECOPYVALUETOFILE(m_svnStatus.node_status);
    WRITECOPYVALUETOFILE(m_svnStatus.prop_status);
    WRITECOPYVALUETOFILE(m_svnStatus.repos_prop_status);
    WRITECOPYVALUETOFILE(m_svnStatus.repos_text_status);
    WRITECOPYVALUETOFILE(m_svnStatus.switched);
    WRITECOPYVALUETOFILE(m_svnStatus.text_status);
    WRITECOPYVALUETOFILE(m_treeconflict);
    return true;
}

bool CStatusCacheEntry::LoadFromDisk(FILE * pFile)
{
#define LOADVALUEFROMFILE(x) if (fread(&x, sizeof(x), 1, pFile)!=1) return false;
#define LOADCOPYVALUEFROMFILE(x,T) {int v=fgetc(pFile); if (v==EOF) return false; x=(T)v;}
    try
    {
        unsigned int value = 0;
        LOADVALUEFROMFILE(value);
        if (value != CACHEENTRYDISKVERSION)
            return false;       // not the correct version

        LOADCOPYVALUEFROMFILE(m_highestPriorityLocalStatus, svn_wc_status_kind);
        LOADVALUEFROMFILE(m_lastWriteTime);
        LOADCOPYVALUEFROMFILE(m_bSet, svn_boolean_t);
        LOADCOPYVALUEFROMFILE(m_bSVNEntryFieldSet, svn_boolean_t);
        LOADVALUEFROMFILE(m_commitRevision);

        LOADCOPYVALUEFROMFILE(m_bHasOwner, svn_boolean_t);
        LOADCOPYVALUEFROMFILE(m_kind, svn_node_kind_t);
        LOADCOPYVALUEFROMFILE(m_needsLock, svn_boolean_t);
        LOADCOPYVALUEFROMFILE(m_svnStatus.copied, svn_boolean_t);
        LOADCOPYVALUEFROMFILE(m_svnStatus.locked, svn_boolean_t);
        LOADCOPYVALUEFROMFILE(m_svnStatus.node_status, svn_wc_status_kind);
        LOADCOPYVALUEFROMFILE(m_svnStatus.prop_status, svn_wc_status_kind);
        LOADCOPYVALUEFROMFILE(m_svnStatus.repos_prop_status, svn_wc_status_kind);
        LOADCOPYVALUEFROMFILE(m_svnStatus.repos_text_status, svn_wc_status_kind);
        LOADCOPYVALUEFROMFILE(m_svnStatus.switched, svn_boolean_t);
        LOADCOPYVALUEFROMFILE(m_svnStatus.text_status, svn_wc_status_kind);
        LOADCOPYVALUEFROMFILE(m_treeconflict, svn_boolean_t);

        m_discardAtTime = GetTickCount()+cachetimeout;
    }
    catch ( CAtlException )
    {
        return false;
    }
    return true;
}

void CStatusCacheEntry::SetStatus(const svn_client_status_t* pSVNStatus, bool needsLock, bool forceNormal)
{
    if(pSVNStatus == NULL)
    {
        SetAsUnversioned();
    }
    else
    {
        m_svnStatus.node_status = pSVNStatus->node_status;
        m_svnStatus.text_status = pSVNStatus->text_status;
        m_svnStatus.prop_status = pSVNStatus->prop_status;

        m_svnStatus.locked = pSVNStatus->locked;
        m_svnStatus.copied = pSVNStatus->copied;
        m_svnStatus.switched = pSVNStatus->switched;

        m_svnStatus.repos_text_status = pSVNStatus->repos_text_status;
        m_svnStatus.repos_prop_status = pSVNStatus->repos_prop_status;
        m_kind = pSVNStatus->kind;

        if ((pSVNStatus->node_status == svn_wc_status_incomplete)&&(pSVNStatus->local_abspath))
        {
            // just in case: incomplete status can get the kind completely wrong
            // for example paths that are illegal on windows show up with status incomplete
            // but kind as directory, even if they in fact are files.
            CTSVNPath p;
            p.SetFromSVN(pSVNStatus->local_abspath);
            if (!p.Exists() || !p.IsDirectory())
                m_kind = svn_node_file;
        }

        if (forceNormal)
        {
            m_svnStatus.text_status = svn_wc_status_normal;
            m_svnStatus.prop_status = svn_wc_status_normal;
            m_svnStatus.node_status = svn_wc_status_normal;
        }

        m_highestPriorityLocalStatus = m_svnStatus.node_status;

        // Currently we don't deep-copy the whole entry value, but we do take a few members
        if(pSVNStatus->versioned)
        {
            m_commitRevision = pSVNStatus->changed_rev;
            m_bSVNEntryFieldSet = true;
            m_bHasOwner = pSVNStatus->lock && *pSVNStatus->lock->owner;
            m_needsLock = needsLock;
        }
        else
        {
            m_bHasOwner = FALSE;
            m_commitRevision = 0;
            m_bSVNEntryFieldSet = false;
        }
        m_treeconflict = pSVNStatus->conflicted != 0;
    }
    m_discardAtTime = GetTickCount()+cachetimeout;
    m_bSet = true;
}


void CStatusCacheEntry::SetAsUnversioned()
{
    SecureZeroMemory(&m_svnStatus, sizeof(m_svnStatus));
    m_discardAtTime = GetTickCount()+cachetimeout;
    svn_wc_status_kind status = svn_wc_status_none;
    if (m_highestPriorityLocalStatus == svn_wc_status_ignored)
        status = svn_wc_status_ignored;
    if (m_highestPriorityLocalStatus == svn_wc_status_unversioned)
        status = svn_wc_status_unversioned;
    m_highestPriorityLocalStatus = status;
    m_svnStatus.prop_status = status;
    m_svnStatus.text_status = status;
    m_svnStatus.node_status = status;
    m_lastWriteTime = 0;
    m_treeconflict = false;
    m_needsLock = false;
}

bool CStatusCacheEntry::HasExpired(long now) const
{
    return m_discardAtTime != 0 && (now - m_discardAtTime) >= 0;
}

void CStatusCacheEntry::BuildCacheResponse(TSVNCacheResponse& response, DWORD& responseLength) const
{
    SecureZeroMemory(&response, sizeof(response));

    response.m_textStatus = (INT8)m_svnStatus.text_status;
    response.m_propStatus = (INT8)m_svnStatus.prop_status;
    response.m_Status     = (INT8)m_svnStatus.node_status;
    response.m_tree_conflict = m_treeconflict != FALSE;

    if(m_bSVNEntryFieldSet)
    {
        response.m_cmt_rev    = m_commitRevision;

        response.m_kind = (INT8)m_kind;
        response.m_needslock = m_needsLock != FALSE;
        response.m_has_lockonwner = m_bHasOwner != FALSE;
    }

    responseLength = sizeof(response);
}

bool CStatusCacheEntry::IsVersioned() const
{
    return m_highestPriorityLocalStatus > svn_wc_status_unversioned;
}

bool CStatusCacheEntry::DoesFileTimeMatch(__int64 testTime) const
{
    return m_lastWriteTime == testTime;
}


bool CStatusCacheEntry::ForceStatus(svn_wc_status_kind forcedStatus)
{
    svn_wc_status_kind newStatus = forcedStatus;

    if(newStatus != m_highestPriorityLocalStatus)
    {
        // We've had a status change
        m_highestPriorityLocalStatus = newStatus;
        m_svnStatus.node_status = newStatus;
        m_svnStatus.prop_status = newStatus;
        m_svnStatus.text_status = newStatus;
        m_discardAtTime = GetTickCount()+cachetimeout;
        return true;
    }
    return false;
}

bool
CStatusCacheEntry::HasBeenSet() const
{
    return m_bSet != FALSE;
}

void CStatusCacheEntry::Invalidate()
{
    m_bSet = false;
}
