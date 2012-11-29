// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010, 2012 - TortoiseSVN

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
#include "stdafx.h"
#include "RepositoryBrowserSelection.h"
#include "RepositoryBrowser.h"

// lookup bucket by repository. Automatically add a suitable
// bucket if there is none yet.

CRepositoryBrowserSelection::SPathsPerRepository&
CRepositoryBrowserSelection::AutoAddRepository
    (const SRepositoryInfo& repository)
{
    // search for an exisiting entry for that repository

    for (size_t i = 0, count = repositories.size(); i < count; ++i)
        if (repositories[i].repository == repository)
            return repositories[i];

    // not found -> add one and return it

    SPathsPerRepository newEntry;
    newEntry.repository = repository;
    newEntry.folderCount = 0;
    newEntry.externalCount = 0;
    newEntry.lockCount = 0;

    repositories.push_back (newEntry);
    return repositories.back();
}

// store & accumulate in the corresponding repository bucket

void CRepositoryBrowserSelection::InternalAdd
    ( const SRepositoryInfo& repository
    , SPath& path)
{
    // set common info

    path.isRoot = path.url.GetSVNPathString() == repository.root;

    // finally, add it

    SPathsPerRepository& repositoryInfo = AutoAddRepository (repository);

    repositoryInfo.paths.push_back (path);
    repositoryInfo.externalCount += path.isExternal ? 1 : 0;
    repositoryInfo.folderCount += path.isFolder ? 1 : 0;
    repositoryInfo.lockCount += path.isLocked ? 1 : 0;
}

// construction / destruction

CRepositoryBrowserSelection::CRepositoryBrowserSelection(void)
{
}

CRepositoryBrowserSelection::~CRepositoryBrowserSelection(void)
{
}

// add new URL from \ref item.

void CRepositoryBrowserSelection::Add (const CItem* item)
{
    if (item == NULL)
        return;

    // extract the info from the list item

    SPath path;
    CString absPath = item->absolutepath;

    absPath.Replace (_T("\\"), _T("%5C"));
    path.url = CTSVNPath (absPath);

    // we don't fully escape the urls, because the GetSVNApiPath() method
    // of the CTSVNPath already does escaping.
    // We only escape special chars here:
    // the '%' because we know that this char isn't escaped yet, and
    // the '"' char, because we pass these urls to the command line as well
    absPath.Replace (_T("%"), _T("%25"));
    absPath.Replace (_T("\""), _T("%22"));
    path.urlEscaped = CTSVNPath (absPath);

    path.isExternal = item->is_external;
    path.isFolder = item->kind == svn_node_dir;
    path.isLocked = !item->locktoken.IsEmpty();

    // store & accumulate in the corresponding repository bucket

    InternalAdd (item->repository, path);
}

void CRepositoryBrowserSelection::Add (const CTreeItem* item)
{
    if (item == NULL)
        return;

    // extract the info from the list item

    SPath path;
    CString absPath = item->url;
    path.url = CTSVNPath (absPath);

    // we don't fully escape the urls, because the GetSVNApiPath() method
    // of the CTSVNPath already does escaping.
    // We only escape special chars here:
    // the '%' because we know that this char isn't escaped yet, and
    // the '"' char, because we pass these urls to the command line as well
    absPath.Replace (_T("%"), _T("%25"));
    absPath.Replace (_T("\""), _T("%22"));
    path.urlEscaped = CTSVNPath (absPath);

    path.isExternal = item->is_external;
    path.isFolder = true;
    path.isLocked = false;

    // store & accumulate in the corresponding repository bucket

    InternalAdd (item->repository, path);
}

// get the number of repository buckets

size_t CRepositoryBrowserSelection::GetRepositoryCount() const
{
    return repositories.size();
}

bool CRepositoryBrowserSelection::IsEmpty() const
{
    return repositories.empty();
}

// lookup

std::pair<size_t, size_t>
CRepositoryBrowserSelection::FindURL (const CTSVNPath& url) const
{
    for (size_t i = 0, repoCount = GetRepositoryCount(); i < repoCount; ++i)
        for (size_t k = 0, pathCount = GetPathCount (i); k < pathCount; ++k)
            if (GetURL (i, k).IsEquivalentTo (url))
                return std::make_pair (i, k);

    return std::pair<size_t, size_t>((size_t)-1, (size_t)-1);
}

bool CRepositoryBrowserSelection::Contains (const CTSVNPath& url) const
{
    return FindURL (url).first != (size_t)(-1);
}

// access repository bucket properties

const SRepositoryInfo&
CRepositoryBrowserSelection::GetRepository (size_t repositoryIndex) const
{
    return repositories [repositoryIndex].repository;
}

size_t
CRepositoryBrowserSelection::GetPathCount (size_t repositoryIndex) const
{
    return repositories [repositoryIndex].paths.size();
}

size_t
CRepositoryBrowserSelection::GetFolderCount (size_t repositoryIndex) const
{
    return repositories [repositoryIndex].folderCount;
}

size_t
CRepositoryBrowserSelection::GetExternalCount (size_t repositoryIndex) const
{
    return repositories [repositoryIndex].externalCount;
}

size_t
CRepositoryBrowserSelection::GetLockCount (size_t repositoryIndex) const
{
    return repositories [repositoryIndex].lockCount;
}

// access path properties

const CTSVNPath&
CRepositoryBrowserSelection::GetURL
    ( size_t repositoryIndex
    , size_t index) const
{
    return repositories [repositoryIndex].paths[index].url;
}

const CTSVNPath&
CRepositoryBrowserSelection::GetURLEscaped
    ( size_t repositoryIndex
    , size_t index) const
{
    return repositories [repositoryIndex].paths[index].urlEscaped;
}

bool
CRepositoryBrowserSelection::IsFolder
    ( size_t repositoryIndex
    , size_t index) const
{
    return repositories [repositoryIndex].paths[index].isFolder;
}

bool
CRepositoryBrowserSelection::IsExternal
    ( size_t repositoryIndex
    , size_t index) const
{
    return repositories [repositoryIndex].paths[index].isExternal;
}

bool
CRepositoryBrowserSelection::IsLocked
    ( size_t repositoryIndex
    , size_t index) const
{
    return repositories [repositoryIndex].paths[index].isLocked;
}

bool
CRepositoryBrowserSelection::IsRoot
    ( size_t repositoryIndex
    , size_t index) const
{
    return repositories [repositoryIndex].paths[index].isRoot;
}

// convenience methods

CTSVNPathList
CRepositoryBrowserSelection::GetURLs (size_t repositoryIndex) const
{
    CTSVNPathList result;

    const SPathsPerRepository& info = repositories[repositoryIndex];
    for (size_t i = 0, count = info.paths.size(); i < count; ++i)
        result.AddPath (info.paths[i].url);

    return result;
}

CTSVNPathList
CRepositoryBrowserSelection::GetURLsEscaped (size_t repositoryIndex) const
{
    CTSVNPathList result;

    const SPathsPerRepository& info = repositories[repositoryIndex];
    for (size_t i = 0, count = info.paths.size(); i < count; ++i)
        result.AddPath (info.paths[i].urlEscaped);

    return result;
}
