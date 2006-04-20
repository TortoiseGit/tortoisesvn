// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006 - Will Dean, Stefan Kueng

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
#pragma once

#include "TSVNPath.h"
#include "SVNHelpers.h"
#include "StatusCacheEntry.h"
#include "CachedDirectory.h"
#include "FolderCrawler.h"
#include "DirectoryWatcher.h"
#include "ShellUpdater.h"
#include "RWSection.h"
#include "atlcoll.h"

//////////////////////////////////////////////////////////////////////////

/**
 * \ingroup TSVNCache
 * The main class handling the status cache.
 * Provides access to a global object of itself which handles all
 * the requests for status.
 */
class CSVNStatusCache
{
private:
	CSVNStatusCache(void);
	~CSVNStatusCache(void);

public:
	static CSVNStatusCache& Instance();
	static void Create();
	static void Destroy();
	static bool SaveCache();

public:
	/// Clear the entire cache
	void Clear();

	/// Get the status for a single path (main entry point, called from named-pipe code
	CStatusCacheEntry GetStatusForPath(const CTSVNPath& path, DWORD flags,  bool bFetch = true);

	/// Find a directory in the cache (a new entry will be created if there isn't an existing entry)
	CCachedDirectory * GetDirectoryCacheEntry(const CTSVNPath& path);
	CCachedDirectory * GetDirectoryCacheEntryNoCreate(const CTSVNPath& path);

	/// Add a folder to the background crawler's work list
	void AddFolderForCrawling(const CTSVNPath& path);

	/// Removes the cache for a specific path, e.g. if a folder got deleted/renamed
	void RemoveCacheForPath(const CTSVNPath& path);
	
	/// Call this method before getting the status for a shell request
	void StartRequest(const CTSVNPath& path);
	/// Call this method after the data for the shell request has been gathered
	void EndRequest(const CTSVNPath& path);
	
	/// Notifies the shell about file/folder status changes.
	/// A notification is only sent for paths which aren't currently
	/// in the list of handled shell requests to avoid deadlocks.
	void UpdateShell(const CTSVNPath& path);

	size_t GetCacheSize() {return m_directoryCache.size();}

	void Init();
	void Stop();

	void CloseWatcherHandles(HDEVNOTIFY hdev);

	void WaitToRead() {m_rwSection.WaitToRead();}
	void WaitToWrite() {m_rwSection.WaitToWrite();}
	void Done() {m_rwSection.Done();}

	bool IsPathAllowed(CTSVNPath path) {return !!m_shellCache.IsPathAllowed(path.GetWinPath());}
	bool IsPathGood(CTSVNPath path);
private:
	bool RemoveCacheForDirectory(CCachedDirectory * cdir);
	CRWSection m_rwSection;
	CAtlList<CString> m_askedList;
	CCachedDirectory::CachedDirMap m_directoryCache;
	std::set<CTSVNPath> m_NoWatchPaths;
	CComAutoCriticalSection m_critSec;
	SVNHelper m_svnHelp;
	ShellCache	m_shellCache;

	static CSVNStatusCache* m_pInstance;

	CFolderCrawler m_folderCrawler;
	CShellUpdater m_shellUpdater;

	CTSVNPath m_mostRecentPath;
	CStatusCacheEntry m_mostRecentStatus;
	long m_mostRecentExpiresAt;

	CDirectoryWatcher watcher;

	friend class CCachedDirectory;  // Needed for access to the SVN helpers
};
