// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2008, 2011-2012 - TortoiseSVN

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
#include "shlobj.h"
#include "SVNStatusCache.h"

CShellUpdater::CShellUpdater(void)
{
    m_hWakeEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    m_hTerminationEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    m_bRunning = FALSE;
    m_bItemsAddedSinceLastUpdate = false;
}

CShellUpdater::~CShellUpdater(void)
{
    Stop();
}

void CShellUpdater::Stop()
{
    InterlockedExchange(&m_bRunning, FALSE);
    if (m_hTerminationEvent)
    {
        SetEvent(m_hTerminationEvent);
        if(WaitForSingleObject(m_hThread, 200) != WAIT_OBJECT_0)
        {
            CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Error terminating shell updater thread\n");
        }
    }
    m_hThread.CloseHandle();
    m_hTerminationEvent.CloseHandle();
    m_hWakeEvent.CloseHandle();
}

void CShellUpdater::Initialise()
{
    // Don't call Initialize more than once
    ATLASSERT(!m_hThread);

    // Just start the worker thread.
    // It will wait for event being signaled.
    // If m_hWakeEvent is already signaled the worker thread
    // will behave properly (with normal priority at worst).

    InterlockedExchange(&m_bRunning, TRUE);
    unsigned int threadId = 0;
    m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
    SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST);
}

void CShellUpdater::AddPathForUpdate(const CTSVNPath& path)
{
    {
        AutoLocker lock(m_critSec);
        m_pathsToUpdate.push_back(path);

        // set this flag while we are synced
        // with the worker thread
        m_bItemsAddedSinceLastUpdate = true;
    }

    SetEvent(m_hWakeEvent);
}


unsigned int CShellUpdater::ThreadEntry(void* pContext)
{
    CCrashReportThread crashthread;
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
        DWORD waitResult = WaitForMultipleObjects(_countof(hWaitHandles), hWaitHandles, FALSE, INFINITE);

        // exit event/working loop if the first event (m_hTerminationEvent)
        // has been signaled or if one of the events has been abandoned
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
            Sleep(0);
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
            if (workingPath.IsEmpty())
                continue;
            CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": shell notification for %s\n"), workingPath.GetWinPath());
            if (workingPath.IsDirectory())
            {
                // first send a notification about a sub folder change, so explorer doesn't discard
                // the folder notification. Since we only know for sure that the subversion admin
                // dir is present, we send a notification for that folder.
                CString admindir = workingPath.GetWinPathString() + _T("\\") + g_SVNAdminDir.GetAdminDirName();
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, (LPCTSTR)admindir, NULL);
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
                // Sending an UPDATEDIR notification somehow overwrites/deletes the UPDATEITEM message. And without
                // that message, the folder overlays in the current view don't get updated without hitting F5.
                // Drawback is, without UPDATEDIR, the left tree view isn't always updated...

                //SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
            }
            else
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
        }
    }
    _endthread();
}

