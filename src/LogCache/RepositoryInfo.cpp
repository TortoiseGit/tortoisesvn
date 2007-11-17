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
#include "RepositoryInfo.h"

#include "svn_client.h"

#include "SVN.h"
#include "TSVNPath.h"
#include "PathUtils.h"
#include "Registry.h"

// begin namespace LogCache

namespace LogCache
{

// construct the dump file name

CString CRepositoryInfo::GetFileName() const
{
    return cacheFolder + _T("\\Repositories.dat");
}

// read the dump file

void CRepositoryInfo::Load()
{
    modified = false;

    // any cached info at all?

	if (GetFileAttributes (GetFileName()) == INVALID_FILE_ATTRIBUTES)
        return;

	CFile file (GetFileName(), CFile::modeRead | CFile::shareDenyWrite);
    CArchive stream (&file, CArchive::load);

    int count = 0;
    stream >> count;

    for (int i = 0; i < count; ++i)
    {
        SPerRepositoryInfo info;
        stream >> info.root 
               >> info.uuid
               >> info.headURL
               >> info.headRevision
               >> info.headLookupTime;

        data[info.root] = info;
    }
}

// find cache entry (or data::end())

CRepositoryInfo::TData::iterator 
CRepositoryInfo::Lookup (const CTSVNPath& url)
{
    CString fullURL (url.GetSVNPathString());

    // the repository root URL should be a prefix of url

    TData::iterator iter = data.lower_bound (fullURL);

    // no suitable prefix?

    if (data.empty())
        return iter;

    // does prefix match?

    if ((iter != data.end()) && (iter->first == fullURL))
        return iter;

    if (iter != data.begin())
    {
        --iter;
        if (iter->first == fullURL.Left (iter->first.GetLength()))
            return iter;
    }

    // not found

    return data.end();
}

// construction / destruction: auto-load and save

CRepositoryInfo::CRepositoryInfo (const CString& cacheFolderPath)
    : cacheFolder (cacheFolderPath)
    , modified (false)
{
    Load();
}

CRepositoryInfo::~CRepositoryInfo(void)
{
}

// look-up and ask SVN if the info is not in cache. 
// cache the result.

CString CRepositoryInfo::GetRepositoryRoot (const CTSVNPath& url)
{
    CString uuid;
    return GetRepositoryRootAndUUID (url, uuid);
}

CString CRepositoryInfo::GetRepositoryRootAndUUID ( const CTSVNPath& url
                                                  , CString& sUUID)
{
    TData::const_iterator iter = Lookup (url);
    if (iter == data.end())
    {
        SPerRepositoryInfo info;
        info.root = SVN().GetRepositoryRootAndUUID (url, info.uuid);
        info.headRevision = NO_REVISION;
        info.headLookupTime = -1;

        if (!info.root.IsEmpty())
        {
            data [info.root] = info;
            modified = true;
        }

        sUUID = info.uuid;
        return info.root;
    }

    sUUID = iter->second.uuid;
    return iter->first;
}

revision_t CRepositoryInfo::GetHeadRevision (const CTSVNPath& url)
{
    // get the entry for that repository

    TData::iterator iter = Lookup (url);
    if (iter == data.end())
    {
        GetRepositoryRoot (url);
        iter = Lookup (url);
    }

    if (iter == data.end())
    {
        // there was some problem connecting to the repository

        return NO_REVISION;
    }

    // get time stamps and maximum head info age (default: 5 mins)

    __time64_t now = CTime::GetCurrentTime().GetTime();
    CRegStdWORD useLogCache (_T("Software\\TortoiseSVN\\HeadCacheAgeLimit"), 300);

    // if there a valid cached entry?

    if (   (now - iter->second.headLookupTime > useLogCache)
        || (   url.GetSVNPathString().Left (iter->second.headURL.GetLength())
            != iter->second.headURL))
    {
        // entry outdated or for not valid for this path

        iter->second.headLookupTime = now;
        iter->second.headURL = url.GetSVNPathString();
        iter->second.headRevision = SVN().GetHEADRevision (url);

        modified = true;
    }

    // ready

    return iter->second.headRevision;
}

// make sure, we will ask the repository for the HEAD

void CRepositoryInfo::ResetHeadRevision (const CTSVNPath& url)
{
    // get the entry for that repository

    TData::iterator iter = Lookup (url);
    if (iter != data.end())
    {
        // there is actually a cache for this URL.
        // Invalidate the HEAD info

        iter->second.headLookupTime = 0;
    }
}


// find the root folder to a given UUID (e.g. URL for given cache file).
// Returns an empty string, if no suitable entry has been found.

CString CRepositoryInfo::GetRootFromUUID (const CString& sUUID) const
{
    // scan all data

    for ( TData::const_iterator iter = data.begin(), end = data.end()
        ; iter != end
        ; ++iter)
    {
        if (iter->second.uuid == sUUID)
            return iter->first;
    }

    // not found

    return CString();
}

// remove a specific entry

void CRepositoryInfo::DropEntry (const CString& sUUID)
{
    for ( TData::iterator iter = data.begin(), end = data.end()
        ; iter != end
        ; ++iter)
    {
        if (iter->second.uuid == sUUID)
		{
			data.erase (iter);
            return;
		}
    }
}

// write all changes to disk

void CRepositoryInfo::Flush()
{
    if (!modified)
        return;

	CString filename = GetFileName();
	CPathUtils::MakeSureDirectoryPathExists(filename.Left(filename.ReverseFind('\\')));
	CFile file (filename, CFile::modeWrite | CFile::modeCreate);
    CArchive stream (&file, CArchive::store);

    stream << static_cast<int>(data.size());

    for ( TData::const_iterator iter = data.begin(), end = data.end()
        ; iter != end
        ; ++iter)
    {
        stream << iter->second.root 
               << iter->second.uuid 
               << iter->second.headURL 
               << iter->second.headRevision 
               << iter->second.headLookupTime;
    }

    modified = false;
}

// clear cache

void CRepositoryInfo::Clear()
{
    data.clear();
}

// end namespace LogCache

}
