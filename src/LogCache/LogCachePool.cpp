// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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
    , repositoryInfo (cacheFolderPath)
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

// cached repository info

CRepositoryInfo& CLogCachePool::GetRepositoryInfo()
{
    return repositoryInfo;
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

    repositoryInfo.Flush();
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

    repositoryInfo.Clear();
}

// end namespace LogCache

}

