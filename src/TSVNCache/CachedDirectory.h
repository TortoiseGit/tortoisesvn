#pragma once

#include "StatusCacheEntry.h"
#include "TSVNPath.h"

class CCachedDirectory
{
public:
	typedef std::map<CTSVNPath, CCachedDirectory> CachedDirMap; 
	typedef CachedDirMap::iterator ItDir;

public:

	CCachedDirectory();
//	CCachedDirectory(const svn_wc_status_t* pSVNStatus, __int64 lastWriteTime);
	CCachedDirectory(const CTSVNPath& directoryPath);
	~CCachedDirectory(void);
	bool IsVersioned() const;
	CStatusCacheEntry GetStatusForMember(const CTSVNPath& path, bool bRecursive);
	CStatusCacheEntry GetOwnStatus(bool bRecursive) const;
	bool IsOwnStatusValid() const;
	void PushOurStatusToParent();
	void RefreshStatus();

private:
	static void GetStatusCallback(void *baton, const char *path, svn_wc_status_t *status);
	void AddEntry(const CTSVNPath& path, const svn_wc_status_t* pSVNStatus);
	CString GetCacheKey(const CTSVNPath& path);
	CString GetFullPathString(const CString& cacheKey);
	CStatusCacheEntry LookForItemInCache(const CTSVNPath& path, bool &bFound);
	void UpdateChildDirectoryStatus(const CTSVNPath& childDir, svn_wc_status_kind childStatus);
	svn_wc_status_kind GetMostImportantStatus() const;

private:
	// The cache of files and directories within this directory
	typedef std::map<CString, CStatusCacheEntry> CacheEntryMap; 
	CacheEntryMap m_entryCache; 

	// A vector if iterators to child directorys - used to put-together recursive status
	typedef std::map<CTSVNPath, svn_wc_status_kind>  ChildDirStatus;
	ChildDirStatus m_childDirectories;

	// The timestamp of the .SVN\entries file.  For an unversioned directory, this will be zero
	__int64 m_entriesFileTime;

	CTSVNPath	m_directoryPath;
	CStatusCacheEntry m_ownStatus;

	// The most important status of any of file members
	svn_wc_status_kind m_mostImportantFileStatus;

		//	friend class CSVNStatusCache;		
};

