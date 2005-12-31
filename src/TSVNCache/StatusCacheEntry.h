// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006 - Will Dean, Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once

struct TSVNCacheResponse;
#define CACHETIMEOUT	0x7FFFFFFF
extern DWORD cachetimeout;

/**
 * \ingroup TSVNCache
 * Holds all the status data of one file or folder.
 */
class CStatusCacheEntry
{
public:
	CStatusCacheEntry();
	CStatusCacheEntry(const svn_wc_status2_t* pSVNStatus, __int64 lastWriteTime, bool bReadOnly);
	bool HasExpired(long now) const;
	void BuildCacheResponse(TSVNCacheResponse& response, DWORD& responseLength) const;
	bool IsVersioned() const;
	bool DoesFileTimeMatch(__int64 testTime) const;
	bool ForceStatus(svn_wc_status_kind forcedStatus);
	svn_wc_status_kind GetEffectiveStatus() const { return m_highestPriorityLocalStatus; }
	void SetStatus(const svn_wc_status2_t* pSVNStatus);
	bool HasBeenSet() const;
	void Invalidate();
	bool IsDirectory() const {return m_kind == svn_node_dir;}
	bool SaveToDisk(FILE * pFile);
	bool LoadFromDisk(FILE * pFile);
private:
	void SetAsUnversioned();

private:
	long				m_discardAtTime;
	svn_wc_status_kind	m_highestPriorityLocalStatus;
	svn_wc_status2_t	m_svnStatus;
	__int64				m_lastWriteTime;
	bool				m_bSet;
	svn_node_kind_t		m_kind;
	bool				m_bReadOnly;

	// Values copied from the 'entries' structure
	bool				m_bSVNEntryFieldSet;
	CStringA			m_sUrl;
	CStringA			m_sOwner;
	CStringA			m_sAuthor;
	svn_revnum_t		m_commitRevision;

//	friend class CSVNStatusCache;
};
