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
#include "RepositoryInfo.h"

#include "CompositeInStream.h"
#include "CompositeOutStream.h"
#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"
#include "PackedIntegerInStream.h"
#include "PackedIntegerOutStream.h"

#include "DirFileEnum.h"
#include "PathUtils.h"
#include "Registry.h"

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

CLogCachePool::CLogCachePool (SVN& svn, const CString& cacheFolderPath)
	: cacheFolderPath (cacheFolderPath)
    , repositoryInfo (new CRepositoryInfo (svn, cacheFolderPath))
{
}

CLogCachePool::~CLogCachePool()
{
    delete repositoryInfo;
    repositoryInfo = NULL;

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

	cache->Load();

	caches[uuid] = cache.get();

	// ready

	return cache.release();
}

// cached repository info

CRepositoryInfo& CLogCachePool::GetRepositoryInfo()
{
    return *repositoryInfo;
}

// return the size of the repository cache file 
// (returns 0 for new files)

size_t CLogCachePool::FileSize (const CString& uuid)
{
    // return 0 for unknown caches

	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (GetFileAttributesEx ( cacheFolderPath + uuid
						    , GetFileExInfoStandard
							, &fileInfo) == FALSE)
        return 0;

    // return result

    return fileInfo.nFileSizeLow;
}

// delete a cache along with all file(s)

void CLogCachePool::DropCache (const CString& uuid)
{
	// delete cache object

	TCaches::iterator iter = caches.find (uuid);
	if (iter != caches.end())
	{
		delete iter->second;
		caches.erase (iter);
	}

	// delete cache file

	std::wstring fileName = (LPCTSTR)(cacheFolderPath + uuid);
	if (FileExists (fileName))
		DeleteFile (fileName.c_str());

	if (FileExists (fileName + L".lock"))
        DeleteFile ((fileName + L".lock").c_str());

	// remove from cache info list

	repositoryInfo->DropEntry (uuid);
}

// other data access
// return as URL -> UUID map

std::map<CString, CString> CLogCachePool::GetRepositoryURLs() const
{
	std::map<CString, CString> result;

	// find all cache files 

	CString filePath;
	CDirFileEnum logenum (cacheFolderPath);
	while (logenum.NextFile (filePath, NULL))
	{
        // the repository list itself is not a repository cache

        if (   (filePath != repositoryInfo->GetFileName())
            && (filePath.Right (5) != L".lock"))
        {
		    CString uuid = CPathUtils::GetFileNameFromPath (filePath);
		    CString rootURL;
            if (!repositoryInfo->HasMultipleURLs (uuid)) 
                rootURL = repositoryInfo->GetRootFromUUID (uuid);

		    result[rootURL.IsEmpty() ? uuid : rootURL] = uuid;
        }
	}

	// add in-RAM-only caches

	for ( TCaches::const_iterator iter = caches.begin(), end = caches.end()
		; iter != end
		; ++iter)
	{
		CString uuid = iter->first;
		CString rootURL = repositoryInfo->GetRootFromUUID (uuid);

		result[rootURL.IsEmpty() ? uuid : rootURL] = uuid;
	}

	return result;
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

    repositoryInfo->Flush();
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

    if (repositoryInfo != NULL)
        repositoryInfo->Clear();
}

// has log caching been enabled?

bool CLogCachePool::IsEnabled() const
{
	CRegStdWORD useLogCache (_T("Software\\TortoiseSVN\\UseLogCache"), TRUE);
	return useLogCache != FALSE;
}

// end namespace LogCache

}

