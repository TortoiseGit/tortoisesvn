// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009-2010, 2015 - TortoiseSVN

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
#pragma once

#include "RepositoryLister.h"

class CTreeItem;

class CRepositoryBrowserSelection
{
private:

    struct SPath
    {
        CTSVNPath url;
        CTSVNPath urlEscaped;

        bool isFolder;
        bool isLocked;
        bool isExternal;
        bool isRoot;
    };

    typedef std::vector<SPath> TPaths;

    struct SPathsPerRepository
    {
        SRepositoryInfo repository;
        TPaths paths;

        size_t folderCount;
        size_t lockCount;
        size_t externalCount;
    };

    typedef std::vector<SPathsPerRepository> TRepositories;
    TRepositories repositories;

    /// lookup bucket by repository. Automatically add a suitable
    /// bucket if there is none yet.

    SPathsPerRepository& AutoAddRepository (const SRepositoryInfo& repository);

    /// store & accumulate in the corresponding repository bucket

    void InternalAdd (const SRepositoryInfo& repository, SPath& path);

public:

    /// construction / destruction

    CRepositoryBrowserSelection(void);
    ~CRepositoryBrowserSelection(void);

    /// add new URL from \ref item.

    void Add (const CItem* item);
    void Add (const CTreeItem* item);

    /// get the number of repository buckets

    size_t GetRepositoryCount() const;
    bool IsEmpty() const;

    /// lookup

    std::pair<size_t, size_t> FindURL (const CTSVNPath& url) const;
    bool Contains (const CTSVNPath& url) const;

    /// access repository bucket properties

    const SRepositoryInfo& GetRepository (size_t repositoryIndex) const;
    size_t GetPathCount (size_t repositoryIndex) const;
    size_t GetFolderCount (size_t repositoryIndex) const;
    size_t GetExternalCount (size_t repositoryIndex) const;
    size_t GetLockCount (size_t repositoryIndex) const;

    /// access path properties

    const CTSVNPath& GetURL (size_t repositoryIndex, size_t index) const;
    const CTSVNPath& GetURLEscaped (size_t repositoryIndex, size_t index) const;

    bool IsFolder (size_t repositoryIndex, size_t index) const;
    bool IsExternal (size_t repositoryIndex, size_t index) const;
    bool IsLocked (size_t repositoryIndex, size_t index) const;
    bool IsRoot (size_t repositoryIndex, size_t index) const;

    /// convenience methods

    CTSVNPathList GetURLs (size_t repositoryIndex) const;
    CTSVNPathList GetURLsEscaped (size_t repositoryIndex, bool bIncludeExternals) const;
};
