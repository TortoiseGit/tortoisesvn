#include "StdAfx.h"
#include "SVNStatus.h"
#include "Svnstatuscache.h"

//////////////////////////////////////////////////////////////////////////

CSVNStatusCache* CSVNStatusCache::m_pInstance;

CSVNStatusCache& CSVNStatusCache::Instance()
{
	ATLASSERT(m_pInstance != NULL);
	return *m_pInstance;
}

void CSVNStatusCache::Create()
{
	ATLASSERT(m_pInstance == NULL);
	m_pInstance = new CSVNStatusCache;
}
void CSVNStatusCache::Destroy()
{
	delete m_pInstance;
	m_pInstance = NULL;
}


CSVNStatusCache::CSVNStatusCache(void) : 
	m_bDoRecursiveFetches(false),
	m_bGetRemoteStatus(false)
{

	m_folderCrawler.Initialise();

}

CSVNStatusCache::~CSVNStatusCache(void)
{
}

void CSVNStatusCache::Clear()
{

}

CCachedDirectory& CSVNStatusCache::GetDirectoryCacheEntry(const CTSVNPath& path)
{
	ATLASSERT(path.IsDirectory());

	CCachedDirectory::ItDir itMap;
	itMap = m_directoryCache.find(path);
	if(itMap != m_directoryCache.end())
	{
		// We've found this directory in the cache 
		return itMap->second;
	}
	else
	{
		// We don't know anything about this directory yet - lets add it to our cache
		return m_directoryCache.insert(m_directoryCache.lower_bound(path), std::make_pair(path, CCachedDirectory(path)))->second;
	}
}


CStatusCacheEntry CSVNStatusCache::GetStatusForPath(const CTSVNPath& path)
{
	AutoLocker lock(m_critSec);

	// Check a very short-lived 'mini-cache' of the last thing we were asked for.
	long now = (long)GetTickCount();
	if(now-m_mostRecentExpiresAt < 0)
	{
		if(path.IsEquivalentTo(m_mostRecentPath))
		{
			return m_mostRecentStatus;
		}
	}
	m_mostRecentPath = path;
	m_mostRecentExpiresAt = now+1000;

	ATLTRACE("Req: %ws\n", path.GetWinPathString());

	// Stop the crawler starting on a new folder while we're doing this much more important task...
	CCrawlInhibitor crawlInhibit(&m_folderCrawler);

	return m_mostRecentStatus = GetDirectoryCacheEntry(path.GetContainingDirectory()).GetStatusForMember(path);
}

// Get the status for a directory by asking the directory, rather 
// than the usual method of asking its parent
/*
CStatusCacheEntry 
CSVNStatusCache::GetDirectorysOwnStatus(const CTSVNPath& path)
{
	AutoLocker lock(m_critSec);

	ATLASSERT(path.IsDirectory());

	return GetDirectoryCacheEntry(path).GetOwnStatus();
}
*/


void CSVNStatusCache::Dump()
{
	CCachedDirectory::ItDir itMap;
	for(itMap = m_directoryCache.begin(); itMap != m_directoryCache.end(); ++itMap)
	{
		ATLTRACE("Dir %ws\n", itMap->first.GetWinPath());
	}
}

void CSVNStatusCache::EnableRecursiveFetch(bool bEnable)
{
	m_bDoRecursiveFetches = bEnable;
}

void CSVNStatusCache::EnableRemoteStatus(bool bEnable)
{
	m_bGetRemoteStatus = bEnable;
}

void CSVNStatusCache::AddFolderForCrawling(const CTSVNPath& path)
{
	m_folderCrawler.AddDirectoryForUpdate(path);
}

//void CSVNStatusCache::SetDirectoryStatus(const CTSVNPath& path, const svn_wc_status_t *pStatus)
//{
//	ATLASSERT(path.IsDirectory());
//	GetDirectoryCacheEntry(path)->second.SetOwnStatus(pStatus);
//}



static class StatusCacheTests
{
public:
	StatusCacheTests()
	{
		apr_initialize();
		CSVNStatusCache::Create();

		{
			CSVNStatusCache& cache = CSVNStatusCache::Instance();

//		cache.GetStatusForPath(CTSVNPath(_T("n:/will/tsvntest/file1.txt")));
//		cache.GetStatusForPath(CTSVNPath(_T("n:/will/tsvntest/NonVersion/Test1.txt")));

		cache.Dump();
		}

		CSVNStatusCache::Destroy();
		apr_terminate();
	}

} StatusCacheTests;


