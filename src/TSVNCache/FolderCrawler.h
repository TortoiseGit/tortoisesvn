#pragma once

#include "TSVNPath.h"

//////////////////////////////////////////////////////////////////////////



#pragma pack(push, r1, 16)

class CFolderCrawler
{
public:
	CFolderCrawler(void);
	~CFolderCrawler(void);

public:
	void Initialise();
	void AddDirectoryForUpdate(const CTSVNPath& path);

private:
	static unsigned int __stdcall ThreadEntry(void* pContext);
	void WorkerThread();
	void SetHoldoff();

private:
	CComAutoCriticalSection m_critSec;
	HANDLE m_hThread;
	std::deque<CTSVNPath> m_foldersToUpdate;
	HANDLE m_hTerminationEvent;
	HANDLE m_hWakeEvent;
	
	// This will be *asynchronously* modified by CCrawlInhibitor.
	// So, we have to mark it volatile, preparing compiler and
	// optimizer for the "worst".
	volatile LONG m_lCrawlInhibitSet;
	
	// While the shell is still asking for items, we don't
	// want to start crawling.  This timer is pushed-out for 
	// every shell request, and stops us crawling until
	// a bit of quiet time has elapsed
	long m_crawlHoldoffReleasesAt;
	bool m_bItemsAddedSinceLastCrawl;


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
