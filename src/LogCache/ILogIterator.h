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
// necessary includes
///////////////////////////////////////////////////////////////

#include "./Containers/LogCacheGlobals.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CDictionaryBasedTempPath;


/**
 * Iterator interface for iterating over the log entries.
 */
class ILogIterator
{
public:

	/// data access

	virtual bool DataIsMissing() const = 0;
	virtual revision_t GetRevision() const = 0;
	virtual const CDictionaryBasedTempPath& GetPath() const = 0;
	virtual bool EndOfPath() const = 0;

	/// to next / previous revision for our path

	virtual void Advance (revision_t last = 0) = 0;

	/// call this to efficiently skip ranges where DataIsMissing()

	virtual void ToNextAvailableData() = 0;

	/// call this after DataIsMissing() and you added new
	/// revisions to the cache

	virtual void Retry (revision_t last = 0) = 0;

	/// modify cursor

	virtual void SetRevision (revision_t revision) = 0;
	virtual void SetPath (const CDictionaryBasedTempPath& path) = 0;
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

