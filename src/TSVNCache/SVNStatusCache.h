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
	// Clear the entire cache
	void Clear();

	// Get the status for a single path (main entry point, called from named-pipe code
	CStatusCacheEntry GetStatusForPath(const CTSVNPath& path, DWORD flags);

	// Find a directory in the cache (a new entry will be created if there isn't an existing entry)
	CCachedDirectory& GetDirectoryCacheEntry(const CTSVNPath& path);

	// Add a folder to the background crawler's work list
	void AddFolderForCrawling(const CTSVNPath& path);

	// Add an item to the list of paths which need a shell update
	void AddPathForShellUpdate(const CTSVNPath& path);
	// Flush the shell update list
	void FlushShellUpdateList();

private:

private:
	CCachedDirectory::CachedDirMap m_directoryCache; 
	CComAutoCriticalSection m_critSec;
	SVNHelper m_svnHelp;

	static CSVNStatusCache* m_pInstance;

	CFolderCrawler m_folderCrawler;

	CTSVNPath m_mostRecentPath;
	CStatusCacheEntry m_mostRecentStatus;
	long m_mostRecentExpiresAt;

	friend class CCachedDirectory;  // Needed for access to the SVN helpers
};
