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
#pragma once

///////////////////////////////////////////////////////////////
// base class includes
///////////////////////////////////////////////////////////////

#include "logiteratorbase.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{


/**
 * 
 */
class CLogBatchIteratorBase : public CLogIteratorBase
{
public:

	// container type for <path, revision> pairs

	typedef std::pair<CDictionaryBasedTempPath, revision_t> TPathRevision;
	typedef std::vector<TPathRevision> TPathRevisions;

protected:

	// the paths to log for and their revisions

	TPathRevisions pathRevisions;

	// path list folding utilities

	static revision_t MaxRevision (const TPathRevisions& pathRevisions);
	static CDictionaryBasedTempPath BasePath ( const CCachedLogInfo* cachedLog
										     , const TPathRevisions& pathRevisions);

	// construction 
	// (copy construction & assignment use default methods)

	CLogBatchIteratorBase ( const CCachedLogInfo* cachedLog
						  , const TPathRevisions& startPathRevisions);

	// react on cache updates

	virtual void HandleCacheUpdates();

	// implement log scanning sub-routines to
	// work on multiple paths

	virtual revision_t SkipNARevisions();
	virtual bool PathInRevision() const;

	virtual void ToNextRevision();

public:

	// destruction

	virtual ~CLogBatchIteratorBase(void);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

