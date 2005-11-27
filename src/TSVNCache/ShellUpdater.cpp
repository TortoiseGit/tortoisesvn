// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - Stefan Kueng

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
#include "shlobj.h"
#include "SVNStatusCache.h"

CShellUpdater::CShellUpdater(void)
{
	m_hWakeEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hTerminationEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hThread = INVALID_HANDLE_VALUE;
	m_bRunning = false;
}

CShellUpdater::~CShellUpdater(void)
{
	Stop();
}

void CShellUpdater::Stop()
{
	m_bRunning = false;
	if (m_hTerminationEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(m_hTerminationEvent);
		if(WaitForSingleObject(m_hThread, 200) != WAIT_OBJECT_0)
		{
			ATLTRACE("Error terminating shellupdater thread\n");
		}
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
		CloseHandle(m_hTerminationEvent);
		m_hTerminationEvent = INVALID_HANDLE_VALUE;
		CloseHandle(m_hWakeEvent);
		m_hWakeEvent = INVALID_HANDLE_VALUE;
	}
}

void CShellUpdater::Initialise()
{
	// Don't call Initalise more than once
	ATLASSERT(m_hThread == INVALID_HANDLE_VALUE);

	// Just start the worker thread. 
	// It will wait for event being signalled.
	// If m_hWakeEvent is already signalled the worker thread 
	// will behave properly (with normal priority at worst).

	m_bRunning = true;
	unsigned int threadId;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
	SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST);
}

void CShellUpdater::AddPathForUpdate(const CTSVNPath& path)
{
	{
		AutoLocker lock(m_critSec);
		m_pathsToUpdate.push_back(path);
		
		// set this flag while we are sync'ed 
		// with the worker thread
		m_bItemsAddedSinceLastUpdate = true;
	}

	SetEvent(m_hWakeEvent);
}


unsigned int CShellUpdater::ThreadEntry(void* pContext)
{
	((CShellUpdater*)pContext)->WorkerThread();
	return 0;
}

void CShellUpdater::WorkerThread()
{
	HANDLE hWaitHandles[2];
	hWaitHandles[0] = m_hTerminationEvent;	
	hWaitHandles[1] = m_hWakeEvent;

	for(;;)
	{
		DWORD waitResult = WaitForMultipleObjects(sizeof(hWaitHandles)/sizeof(hWaitHandles[0]), hWaitHandles, FALSE, INFINITE);
		
		// exit event/working loop if the first event (m_hTerminationEvent)
		// has been signalled or if one of the events has been abandoned
		// (i.e. ~CShellUpdater() is being executed)
		if(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED_0 || waitResult == WAIT_ABANDONED_0+1)
		{
			// Termination event
			break;
		}
		// wait some time before we notify the shell
		Sleep(50);
		for(;;)
		{
			CTSVNPath workingPath;
			if (!m_bRunning)
				return;
			{
				AutoLocker lock(m_critSec);
				if(m_pathsToUpdate.empty())
				{
					// Nothing left to do 
					break;
				}

				if(m_bItemsAddedSinceLastUpdate)
				{
					m_pathsToUpdate.erase(std::unique(m_pathsToUpdate.begin(), m_pathsToUpdate.end(), &CTSVNPath::PredLeftEquivalentToRight), m_pathsToUpdate.end());
					m_bItemsAddedSinceLastUpdate = false;
				}

				workingPath = m_pathsToUpdate.front();
				m_pathsToUpdate.pop_front();
			}

			ATLTRACE("Update notifications for: %ws\n", workingPath.GetWinPath());
			if (workingPath.IsDirectory())
			{
				// first send a notification about a subfolder change, so explorer doesn't discard
				// the folder notification. Since we only know for sure that the subversion admin
				// dir is present, we send a notification for that folder.
				CString admindir = workingPath.GetWinPathString() + g_SVNAdminDir.GetAdminDirName();
				SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, (LPCTSTR)admindir, NULL);
				SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
				// Sending an UPDATEDIR notification somehow overwrites/deletes the UPDATEITEM message. And without
				// that message, the folder overlays in the current view don't get updated without hitting F5.
				// Drawback is, without UPDATEDIR, the left treeview isn't always updated...
				
				//SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
			}
			else
				SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
		}
	}
	_endthread();
}

