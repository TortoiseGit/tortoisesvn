#include "StdAfx.h"
#include "LogCachePool.h"
#include "CachedLogInfo.h"

#include "CompositeInStream.h"
#include "CompositeOutStream.h"
#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"
#include "PackedIntegerInStream.h"
#include "PackedIntegerOutStream.h"


// begin namespace LogCache

namespace LogCache
{

// utility

bool CLogCachePool::FileExists (const std::wstring& filePath)
{
	return GetFileAttributes (filePath.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// construction / destruction
// (Flush() on destruction)

CLogCachePool::CLogCachePool (const CString& cacheFolderPath)
	: cacheFolderPath (cacheFolderPath)
{
}

CLogCachePool::~CLogCachePool()
{
	Clear();
}

// auto-create and return cache for given repository

CCachedLogInfo* CLogCachePool::GetCache (const CString& uuid)
{
	// cache hit?

	TCaches::const_iterator iter = caches.find (uuid);
	if (iter != caches.end())
		return iter->second;

	// load / create

	std::wstring fileName = (LPCTSTR)(cacheFolderPath + uuid);
	std::auto_ptr<CCachedLogInfo> cache (new CCachedLogInfo (fileName));

	if (FileExists (fileName))
		cache->Load();

	caches[uuid] = cache.get();

	// ready

	return cache.release();
}

// cache management

// write all changes to disk

void CLogCachePool::Flush()
{
	for ( TCaches::iterator iter = caches.begin(), end = caches.end()
		; iter != end
		; ++iter)
	{
		if (iter->second->IsModified())
		{
			try
			{
				iter->second->Save();
			}
			catch (...)
			{
				// cache file could not be written 
				// (cache may be lost but that's o.k.)
			}
		}
	}
}

// minimize memory usage

void CLogCachePool::Clear()
{
	while (!caches.empty())
	{
		CCachedLogInfo* toDelete = caches.begin()->second;
		caches.erase (caches.begin());
		delete toDelete;
	}
}

// end namespace LogCache

}

