// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - Will Dean

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
#include "StdAfx.h"
#include ".\statuscacheentry.h"
#include "SVNStatus.h"
#include "CacheInterface.h"

CStatusCacheEntry::CStatusCacheEntry()
{
	SetAsUnversioned();
	m_bSet = false;
	m_bSVNEntryFieldSet = false;
}

CStatusCacheEntry::CStatusCacheEntry(const svn_wc_status_t* pSVNStatus,__int64 lastWriteTime)
{
	SetStatus(pSVNStatus);
	m_lastWriteTime = lastWriteTime;
	m_discardAtTime = GetTickCount()+CACHETIMEOUT;
}

bool CStatusCacheEntry::SaveToDisk(HANDLE hFile)
{
#define WRITEVALUETOFILE(x) if (!WriteFile(hFile, &x, sizeof(x), &written, NULL)) return false;
#define WRITESTRINGTOFILE(x) if (x.IsEmpty()) {value=0;WRITEVALUETOFILE(value);}else{value=x.GetLength();WRITEVALUETOFILE(value);if (!WriteFile(hFile, x, value, &written, NULL)) return false;}
	DWORD written = 0;
	int value = 0;
	WRITEVALUETOFILE(value); // 'version' of this save-format
	WRITEVALUETOFILE(m_highestPriorityLocalStatus);
	WRITEVALUETOFILE(m_lastWriteTime);
	WRITEVALUETOFILE(m_bSet);
	WRITEVALUETOFILE(m_bSVNEntryFieldSet);
	WRITEVALUETOFILE(m_commitRevision);
	WRITESTRINGTOFILE(m_sUrl);

	// now save the status struct (without the entry field, because we don't use that)
	WRITEVALUETOFILE(m_svnStatus.copied);
	WRITEVALUETOFILE(m_svnStatus.locked);
	WRITEVALUETOFILE(m_svnStatus.prop_status);
	WRITEVALUETOFILE(m_svnStatus.repos_prop_status);
	WRITEVALUETOFILE(m_svnStatus.repos_text_status);
	WRITEVALUETOFILE(m_svnStatus.switched);
	WRITEVALUETOFILE(m_svnStatus.text_status);
	return true;
}

bool CStatusCacheEntry::LoadFromDisk(HANDLE hFile)
{
#define LOADVALUEFROMFILE(x) if (!ReadFile(hFile, &x, sizeof(x), &read, NULL)) return false;
	DWORD read = 0;
	int value = 0;
	LOADVALUEFROMFILE(value);
	if (value != 0)
		return false;		// not the correct version
	LOADVALUEFROMFILE(m_highestPriorityLocalStatus);
	LOADVALUEFROMFILE(m_lastWriteTime);
	LOADVALUEFROMFILE(m_bSet);
	LOADVALUEFROMFILE(m_bSVNEntryFieldSet);
	LOADVALUEFROMFILE(m_commitRevision);
	LOADVALUEFROMFILE(value);
	if (value != 0)
	{
		if (value > INTERNET_MAX_URL_LENGTH)
			return false;		// invalid length for an url
		if (!ReadFile(hFile, m_sUrl.GetBuffer(value), value, &read, NULL))
		{
			m_sUrl.ReleaseBuffer(0);
			return false;
		}
		m_sUrl.ReleaseBuffer(value);
	}
	LOADVALUEFROMFILE(m_svnStatus.copied);
	LOADVALUEFROMFILE(m_svnStatus.locked);
	LOADVALUEFROMFILE(m_svnStatus.prop_status);
	LOADVALUEFROMFILE(m_svnStatus.repos_prop_status);
	LOADVALUEFROMFILE(m_svnStatus.repos_text_status);
	LOADVALUEFROMFILE(m_svnStatus.switched);
	LOADVALUEFROMFILE(m_svnStatus.text_status);
	m_svnStatus.entry = NULL;
	m_discardAtTime = GetTickCount()+CACHETIMEOUT;
	return true;
}

void CStatusCacheEntry::SetStatus(const svn_wc_status_t* pSVNStatus)
{
	if(pSVNStatus == NULL)
	{
		SetAsUnversioned();
	}
	else
	{
		m_highestPriorityLocalStatus = SVNStatus::GetMoreImportant(pSVNStatus->prop_status,pSVNStatus->text_status);
		m_svnStatus = *pSVNStatus;

		// Currently we don't deep-copy the whole entry value, but we do take a few members
        if(pSVNStatus->entry != NULL)
		{
			m_sUrl = pSVNStatus->entry->url;
			m_commitRevision = pSVNStatus->entry->cmt_rev;
			m_bSVNEntryFieldSet = true;
		}
		else
		{
			m_sUrl.Empty();
			m_commitRevision = 0;
			m_bSVNEntryFieldSet = false;
		}
		m_svnStatus.entry = NULL;
	}
	m_discardAtTime = GetTickCount()+CACHETIMEOUT;
	m_bSet = true;
}


void CStatusCacheEntry::SetAsUnversioned()
{
	ZeroMemory(&m_svnStatus, sizeof(m_svnStatus));
	m_discardAtTime = GetTickCount()+CACHETIMEOUT;	// 10 minutes timeout - even unversioned items can get versioned
	m_highestPriorityLocalStatus = svn_wc_status_unversioned;
	m_svnStatus.prop_status = svn_wc_status_unversioned;
	m_svnStatus.text_status = svn_wc_status_unversioned;
	m_svnStatus.prop_status = svn_wc_status_unversioned;
	m_svnStatus.text_status = svn_wc_status_unversioned;
	m_lastWriteTime = 0;
}

bool CStatusCacheEntry::HasExpired(long now) const
{
	return m_discardAtTime != 0 && (now - m_discardAtTime) >= 0;
}

void CStatusCacheEntry::BuildCacheResponse(TSVNCacheResponse& response, DWORD& responseLength) const
{
	if(m_bSVNEntryFieldSet)
	{
		ZeroMemory(&response, sizeof(response));
		response.m_status = m_svnStatus;
		response.m_entry.cmt_rev = m_commitRevision;

		// There is no point trying to set these pointers here, because this is not 
		// the process which will be using the data.
		// The process which receives this response (generally the TSVN Shell Extension)
		// must fix-up these pointers when it gets them
		response.m_status.entry = NULL;
		response.m_entry.url = NULL;

		// The whole of response has been zero'd, so this will copy safely 
		strncat(response.m_url, m_sUrl, sizeof(response.m_url)-1);

		responseLength = sizeof(response);
	}
	else
	{
		response.m_status = m_svnStatus;
		responseLength = sizeof(response.m_status);
	}
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
		m_svnStatus.text_status = newStatus;
		m_svnStatus.prop_status = newStatus;
		m_discardAtTime = GetTickCount()+CACHETIMEOUT;
		return true;
	}
	return false;
}

bool 
CStatusCacheEntry::HasBeenSet() const
{
	return m_bSet;
}
