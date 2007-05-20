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

