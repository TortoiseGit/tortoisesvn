#pragma once

#include "TSVNPath.h"
#include "SVNHelpers.h"
#include "StatusCacheEntry.h"
#include "CachedDirectory.h"
#include "FolderCrawler.h"

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////


class CSVNStatusCache
{
private:
	CSVNStatusCache(void);
	~CSVNStatusCache(void);

public:
	static CSVNStatusCache& Instance();
	static void Create();
	static void Destroy();

public:
	void Clear();
	CStatusCacheEntry GetStatusForPath(const CTSVNPath& path, bool bRecursive);
	void Dump();
	void EnableRecursiveFetch(bool bEnable);
	void EnableRemoteStatus(bool bEnable);

	void SetDirectoryStatus(const CTSVNPath& path, const svn_wc_status_t *pStatus);

	void AddFolderForCrawling(const CTSVNPath& path);

	CCachedDirectory& GetDirectoryCacheEntry(const CTSVNPath& path);

private:

private:
	CCachedDirectory::CachedDirMap m_directoryCache; 
	CComAutoCriticalSection m_critSec;
	SVNHelper m_svnHelp;
	bool m_bDoRecursiveFetches;
	bool m_bGetRemoteStatus;

	static CSVNStatusCache* m_pInstance;

	CFolderCrawler m_folderCrawler;

	CTSVNPath m_mostRecentPath;
	CStatusCacheEntry m_mostRecentStatus;
	long m_mostRecentExpiresAt;

	friend class CCachedDirectory;  // Needed for access to the SVN helpers
};
