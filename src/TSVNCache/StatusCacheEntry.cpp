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

CStatusCacheEntry::CStatusCacheEntry()
{
	SetAsUnversioned();
	m_bSet = false;
}

CStatusCacheEntry::CStatusCacheEntry(const svn_wc_status_t* pSVNStatus,__int64 lastWriteTime)
{
	SetStatus(pSVNStatus);
	m_lastWriteTime = lastWriteTime;
	m_discardAtTime = GetTickCount()+600000;
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
		// Currently we don't deep-copy the entry value
		m_svnStatus.entry = NULL;
	}
	m_bSet = true;
}


void CStatusCacheEntry::SetAsUnversioned()
{
	ZeroMemory(&m_svnStatus, sizeof(m_svnStatus));
	m_discardAtTime = 0;
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

const svn_wc_status_t* CStatusCacheEntry::Status() const
{
	return &m_svnStatus;
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

		return true;
	}
	return false;
}

bool 
CStatusCacheEntry::HasBeenSet() const
{
	return m_bSet;
}
