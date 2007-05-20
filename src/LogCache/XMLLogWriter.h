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

#include "CachedLogInfo.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CBufferedOutFile;

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
//
// CXMLLogReader
//
//		utility class to create an XML formatted log from
//		the given changed log info.
//
///////////////////////////////////////////////////////////////

class CXMLLogWriter
{
private:

	// for convenience

	typedef CRevisionInfoContainer::TChangeAction TChangeAction;
	typedef CRevisionInfoContainer::CChangesIterator CChangesIterator;

	// write <date> tag

	static void WriteTimeStamp ( CBufferedOutFile& file
							   , __time64_t timeStamp);

	// write <paths> tag
	
	static void WriteChanges ( CBufferedOutFile& file
							 , CChangesIterator& iter	
							 , CChangesIterator& last);

	// write <logentry> tag

	static void WriteRevisionInfo ( CBufferedOutFile& file
								  , const CRevisionInfoContainer& info
								  , revision_t revision
								  , index_t index);

	// dump the revisions in descending order

	static void WriteRevionsTopDown ( CBufferedOutFile& file
									, const CCachedLogInfo& source);

	// dump the revisions in ascending order

	static void WriteRevionsBottomUp ( CBufferedOutFile& file
									 , const CCachedLogInfo& source);

public:

	// write the whole change content

	static void SaveToXML ( const std::wstring& xmlFileName
						  , const CCachedLogInfo& source
						  , bool topDown);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

