#pragma once

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CCachedLogInfo;

///////////////////////////////////////////////////////////////
// 
// CLogCachePool
//
//		Central storage for all repository specific log
//		caches currently in use. New caches will be created
//		automatically.
//
//		Currently, there is no method to delete unused cache
//		files or to selectively remove cache info from RAM.
//
///////////////////////////////////////////////////////////////

class CLogCachePool
{
private:

	// where the log cache files are stored

	CString cacheFolderPath;

	// cache per repository (UUID)

	typedef std::map<CString, CCachedLogInfo*> TCaches;
	TCaches caches;

	// utility

	static bool FileExists (const std::wstring& filePath);

public:

	// construction / destruction
	// (Flush() on destruction)

	CLogCachePool (const CString& cacheFolderPath);
	virtual ~CLogCachePool(void);

	// auto-create and return cache for given repository

	CCachedLogInfo* GetCache (const CString& uuid);

	// cache management
	
	// write all changes to disk

	void Flush();

	// minimize memory usage

	void Clear();
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

