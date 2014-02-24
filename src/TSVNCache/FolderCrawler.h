// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005-2007, 2009-2010, 2014 TortoiseSVN

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

#include "TSVNPath.h"
#include "CacheInterface.h"
#include "UniqueQueue.h"
#include "SmartHandle.h"
#include <set>
//////////////////////////////////////////////////////////////////////////



#pragma pack(push, r1, 16)

/**
 * \ingroup TSVNCache
 * Helper class to crawl folders in the background (in a separate thread)
 * so that the main cache isn't blocked until all the status are fetched.
 */
class CFolderCrawler
{
public:
    CFolderCrawler(void);
    ~CFolderCrawler(void);

public:
    void Initialise();
    void AddDirectoryForUpdate(const CTSVNPath& path);
    void AddPathForUpdate(const CTSVNPath& path);
    void Stop();
    bool SetHoldoff(DWORD milliseconds = 500);
    bool IsHoldOff() const;
    void BlockPath(const CTSVNPath& path, DWORD ticks = 0);
private:
    static unsigned int __stdcall ThreadEntry(void* pContext);
    void WorkerThread();

private:
    CComAutoCriticalSection m_critSec;
    CAutoGeneralHandle m_hThread;
    UniqueQueue<CTSVNPath> m_foldersToUpdate;
    UniqueQueue<CTSVNPath> m_pathsToUpdate;
    CAutoGeneralHandle m_hTerminationEvent;
    CAutoGeneralHandle m_hWakeEvent;

    // This will be *asynchronously* modified by CCrawlInhibitor.
    // So, we have to mark it volatile, preparing compiler and
    // optimizer for the "worst".
    volatile LONG m_lCrawlInhibitSet;

    // While the shell is still asking for items, we don't
    // want to start crawling.  This timer is pushed-out for
    // every shell request, and stops us crawling until
    // a bit of quiet time has elapsed
    LONGLONG m_crawlHoldoffReleasesAt;
    bool m_bItemsAddedSinceLastCrawl;
    bool m_bPathsAddedSinceLastCrawl;

    CTSVNPath m_blockedPath;
    ULONGLONG m_blockReleasesAt;
    bool m_bRun;


    friend class CCrawlInhibitor;
};


//////////////////////////////////////////////////////////////////////////


class CCrawlInhibitor
{
private:
    CCrawlInhibitor(); // Not defined
public:
    explicit CCrawlInhibitor(CFolderCrawler* pCrawler)
    {
        m_pCrawler = pCrawler;

        // Count locks instead of a mere flag toggle (exchange)
        // to allow for multiple, concurrent locks.
        ::InterlockedIncrement(&m_pCrawler->m_lCrawlInhibitSet);
    }
    ~CCrawlInhibitor()
    {
        ::InterlockedDecrement(&m_pCrawler->m_lCrawlInhibitSet);
        m_pCrawler->SetHoldoff();
    }
private:
    CFolderCrawler* m_pCrawler;
};

#pragma pack(pop, r1)
