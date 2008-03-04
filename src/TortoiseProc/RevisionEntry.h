// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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

#include "DictionaryBasedTempPath.h"

using namespace LogCache;

/**
 * \ingroup TortoiseProc
 * Helper class, representing a revision with all the required information
 * which we need to draw a revision graph.
 */
class CRevisionEntry
{
public:
	enum Action
	{
		nothing = 0,
		modified = 8,
		deleted = 32,
		added = 4,
		addedwithhistory = 5,
		renamed = 49,
		replaced = 17,
		lastcommit = 50,
		initial = 51,
		source = 1,
	};

    struct SFoldedTag
    {
        CDictionaryBasedTempPath tag;
        bool isAlias;
        bool isDeleted;
        size_t depth;

	    SFoldedTag ( const CDictionaryBasedTempPath& tag
                   , bool isAlias
                   , bool isDeleted
                   , size_t depth)
    		: tag (tag), isAlias (isAlias), isDeleted (isDeleted), depth (depth)
        {
        }
    };

	///members

	CDictionaryBasedTempPath path;
	CDictionaryBasedPath realPath;

	std::vector<CRevisionEntry*>	copyTargets;
    std::vector<SFoldedTag>         tags;

	CRevisionEntry* prev;
	CRevisionEntry* next;

	CRevisionEntry* copySource;

	revision_t		revision;
	Action			action;
	bool			bWorkingCopy;
    DWORD           classification;

	int				column;
	int				row;

    /// pool type

    typedef boost::pool<> TPool;

    /// construction / destruction via pool

    static CRevisionEntry* Create ( const CDictionaryBasedTempPath& path
        				          , revision_t revision
		        		          , Action action
                                  , TPool& pool)
    {
        CRevisionEntry * result = static_cast<CRevisionEntry *>(pool.malloc());
        new (result) CRevisionEntry (path, revision, action);
        return result;
    }

    void Destroy (TPool& pool)
    {
        this->~CRevisionEntry();
        pool.free(this);
    }

protected:

	/// protect construction / destruction to force usage of pool

	CRevisionEntry ( const CDictionaryBasedTempPath& path
				   , revision_t revision
				   , Action action)
		: path (path), realPath (path.GetBasePath()), revision (revision)
		, action (action), classification (0)
		, prev (NULL), next (NULL), copySource (NULL)
		, row (0), column (0), bWorkingCopy(false) {}
    ~CRevisionEntry() {};
};
