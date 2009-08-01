// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

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
// required includes
///////////////////////////////////////////////////////////////

#include "../Streams/FileName.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CStringDictionary;
class CPathDictionary;
class CCachedLogInfo;

/**
 * A simple exporter class that writes (most of) the
 * cached log info to a series of CSV files. 
 */

class CCSVWriter
{
private:

	/// utilities

	void Escape (std::string& value);

	/// write container content as CSV
	/// every method writes exactly one file

	void WriteStringList (std::ostream& os, const CStringDictionary& strings);
	void WritePathList (std::ostream& os, const CPathDictionary& dictionary);

	void WriteChanges (std::ostream& os, const CCachedLogInfo& cache);
	void WriteMerges (std::ostream& os, const CCachedLogInfo& cache);
	void WriteRevProps (std::ostream& os, const CCachedLogInfo& cache);

	void WriteRevisions (std::ostream& os, const CCachedLogInfo& cache);

public:

	/// construction / destruction (nothing to do)

	CCSVWriter(void);
	~CCSVWriter(void);

	/// write cache content as CSV files (overwrite existing ones)
	/// file names are extensions to the given fileName

	void Write (const CCachedLogInfo& cache, const TFileName& fileName);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

