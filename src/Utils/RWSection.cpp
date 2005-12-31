// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "RWSection.h"

CRWSection::CRWSection()
{
	m_nWaitingReaders = m_nWaitingWriters = m_nActive = 0;
	m_hReaders = CreateSemaphore(NULL, 0, MAXLONG, NULL);
	m_hWriters = CreateSemaphore(NULL, 0, MAXLONG, NULL);
	InitializeCriticalSection(&m_cs);
}

CRWSection::~CRWSection()
{
#ifdef _DEBUG
	if (m_nActive != 0)
		DebugBreak();
#endif

	m_nWaitingReaders = m_nWaitingWriters = m_nActive = 0;
	DeleteCriticalSection(&m_cs);
	CloseHandle(m_hWriters);
	CloseHandle(m_hReaders);
}

void CRWSection::WaitToRead()
{
	EnterCriticalSection(&m_cs);

	// Writers have priority
	BOOL fResourceWritePending = (m_nWaitingWriters || (m_nActive < 0));

	if (fResourceWritePending)
	{
		m_nWaitingReaders++;
	} 
	else 
	{
		m_nActive++;
	}

	LeaveCriticalSection(&m_cs);

	if (fResourceWritePending)
	{
		// wait until writer is finished
		WaitForSingleObject(m_hReaders, INFINITE);
	}
}

void CRWSection::WaitToWrite()
{
	EnterCriticalSection(&m_cs);

	BOOL fResourceOwned = (m_nActive != 0);

	if (fResourceOwned)
	{
		m_nWaitingWriters++;
	} 
	else 
	{
		m_nActive = -1;
	}

	LeaveCriticalSection(&m_cs);

	if (fResourceOwned)
	{
		WaitForSingleObject(m_hWriters, INFINITE);
	}
}

void CRWSection::Done()
{
	EnterCriticalSection(&m_cs);

	if (m_nActive > 0)
	{
		m_nActive--;
	}
	else
	{
		m_nActive++;
	}

	HANDLE hsem = NULL;
	LONG lCount = 1;

	if (m_nActive == 0)
	{
		// Note: If there are always writers waiting, then
		// it's possible that a reader never get's access
		// (reader starvation)
		if (m_nWaitingWriters > 0)
		{
			m_nActive = -1;
			m_nWaitingWriters--;
			hsem = m_hWriters;
		}
		else if (m_nWaitingReaders > 0)
		{
			m_nActive = m_nWaitingReaders;
			m_nWaitingReaders = 0;
			hsem = m_hReaders;
			lCount = m_nActive;
		}
		else
		{
			// no threads waiting, nothing to do
		}
	}

	LeaveCriticalSection(&m_cs);

	if (hsem != NULL)
	{
		ReleaseSemaphore(hsem, lCount, NULL);
	}
}

