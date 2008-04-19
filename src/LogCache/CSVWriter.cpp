// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "CSVWriter.h"

#include "RevisionInfoContainer.h"
#include "CachedLogInfo.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// utilities
///////////////////////////////////////////////////////////////

void CCSVWriter::Escape (std::string& value)
{
	size_t pos = value.find ('"');
	while (pos != std::string::npos)
	{
		value.insert (pos+1, 1, '"');
		pos = value.find ('"', pos+2);
	}
}

///////////////////////////////////////////////////////////////
// write container content as CSV
///////////////////////////////////////////////////////////////

void CCSVWriter::WriteStringList ( std::ostream& os
								 , const CStringDictionary& strings)
{
	// header

	os << "ID,Name\r\n";

	// content

	for (index_t i = 0, count = strings.size(); i < count; ++i)
		os << i << ",\"" << strings[i] << "\"\r\n";
}

void CCSVWriter::WritePathList (std::ostream& os, const CPathDictionary& dictionary)
{
	// header

	os << "ID,ParentID,Element,FullPath\r\n";

	// content

	for (index_t i = 0, count = dictionary.size(); i < count; ++i)
	{
		os << i << ',' 
		   << (int)dictionary.GetParent(i) << ",\"" 
		   << dictionary.GetPathElement(i) << "\",\"" 
		   << CDictionaryBasedPath (&dictionary, i).GetPath().c_str()
		   << "\"\r\n";
	}
}

void CCSVWriter::WriteChanges (std::ostream& os, const CCachedLogInfo& cache)
{
	// header

	os << "ID,Revision,Change,PathID,CopyFromRev,CopyFromPathID\r\n";

	// content

	const CRevisionIndex& revisions = cache.GetRevisions();
	const CRevisionInfoContainer& logInfo = cache.GetLogInfo();

	// ids will be added on-the-fly

	size_t id = 0;
	for ( revision_t revision = revisions.GetFirstRevision()
		, last = revisions.GetLastRevision()
		; revision < last
		; ++revision)
	{
		index_t index = revisions[revision];
        if (index == NO_INDEX)
            continue;

		typedef CRevisionInfoContainer::CChangesIterator CI;

		if (  logInfo.GetPresenceFlags (index) 
			& CRevisionInfoContainer::HAS_CHANGEDPATHS)
		{
			// we actually have a valid (possibly empty) change list 
			// for this revision

			for ( CI iter = logInfo.GetChangesBegin (index)
				, end = logInfo.GetChangesEnd (index)
				; iter != end
				; ++iter)
			{
				static const char actions[9] = "AM R   D";
				char change = actions [iter->GetAction() /4 - 1];

				os << id++ << ',' 
				   << revision << ','
				   << change << ','
				   << iter->GetPathID() << ',';

				if (iter->HasFromPath())
				{
					os << iter->GetFromRevision() << ','
					   << iter->GetFromPathID() 
					   << "\r\n";
				}
				else
				{
					os << -1 << ','
					   << -1 
					   << "\r\n";
				}
			}
		}
	}
}

void CCSVWriter::WriteMerges (std::ostream& os, const CCachedLogInfo& cache)
{
	// header

	os << "ID,Revision,FromPathID,ToPathID,StartRevision,RangeLength\r\n";

	// content

	const CRevisionIndex& revisions = cache.GetRevisions();
	const CRevisionInfoContainer& logInfo = cache.GetLogInfo();

	// ids will be added on-the-fly

	size_t id = 0;
	for ( revision_t revision = revisions.GetFirstRevision()
		, last = revisions.GetLastRevision()
		; revision < last
		; ++revision)
	{
		index_t index = revisions[revision];
        if (index == NO_INDEX)
            continue;

		typedef CRevisionInfoContainer::CMergedRevisionsIterator MI;

		if (  logInfo.GetPresenceFlags (index) 
			& CRevisionInfoContainer::HAS_MERGEINFO)
		{
			// we actually have a valid (possibly empty) merge list 
			// for this revision

			for ( MI iter = logInfo.GetMergedRevisionsBegin (index)
				, end = logInfo.GetMergedRevisionsEnd (index)
				; iter != end
				; ++iter)
			{
				os << id++ << ',' 
				   << revision << ','
				   << iter->GetFromPathID() << ','
				   << iter->GetToPathID() << ','
				   << iter->GetRangeStart() << ','
				   << iter->GetRangeDelta()
				   << "\r\n";
			}
		}
	}
}

void CCSVWriter::WriteRevProps (std::ostream& os, const CCachedLogInfo& cache)
{
	// header

	os << "ID,Revision,RevPropID,Value\r\n";

	// content

	const CRevisionIndex& revisions = cache.GetRevisions();
	const CRevisionInfoContainer& logInfo = cache.GetLogInfo();

	// ids will be added on-the-fly

	size_t id = 0;
	for ( revision_t revision = revisions.GetFirstRevision()
		, last = revisions.GetLastRevision()
		; revision < last
		; ++revision)
	{
		index_t index = revisions[revision];
        if (index == NO_INDEX)
            continue;

		typedef CRevisionInfoContainer::CUserRevPropsIterator RI;

		if (  logInfo.GetPresenceFlags (index) 
			& CRevisionInfoContainer::HAS_MERGEINFO)
		{
			// we actually have a valid (possibly empty) merge list 
			// for this revision

			for ( RI iter = logInfo.GetUserRevPropsBegin (index)
				, end = logInfo.GetUserRevPropsEnd (index)
				; iter != end
				; ++iter)
			{
				std::string value = iter->GetValue();
				Escape (value);

				os << id++ << ',' 
				   << revision << ','
				   << iter->GetNameID()<< ",\""
				   << value.c_str()
				   << "\"\r\n";
			}
		}
	}
}

void CCSVWriter::WriteRevisions (std::ostream& os, const CCachedLogInfo& cache)
{
	// header

	os << "Revision,AuthorID,TimeStamp,Comment,"
	   << "HasStdInfo,HasChangeInfo,HasMergeInfo,HasUserRevPropInfo\r\n";

	// content

	const CRevisionIndex& revisions = cache.GetRevisions();
	const CRevisionInfoContainer& logInfo = cache.GetLogInfo();

	for ( revision_t revision = revisions.GetFirstRevision()
		, last = revisions.GetLastRevision()
		; revision < last
		; ++revision)
	{
		index_t index = revisions[revision];
        if (index == NO_INDEX)
            continue;

		std::string comment = logInfo.GetComment (index);
		Escape (comment);

		char presenceFlags = logInfo.GetPresenceFlags (index);
		bool hasStdInfo 
			= (presenceFlags & CRevisionInfoContainer::HAS_STANDARD_INFO) != 0;
		bool hasChangeInfo
			= (presenceFlags & CRevisionInfoContainer::HAS_CHANGEDPATHS) != 0;
		bool hasMergeInfo 
			= (presenceFlags & CRevisionInfoContainer::HAS_MERGEINFO) != 0;
		bool hasRevPropInfo 
			= (presenceFlags & CRevisionInfoContainer::HAS_USERREVPROPS) != 0;

		os << revision << ','
		   << logInfo.GetAuthorID(index) << ','
		   << logInfo.GetTimeStamp(index) << ",\""
		   << comment.c_str() << "\","
		   << hasStdInfo << ','
		   << hasChangeInfo << ','
		   << hasMergeInfo << ','
		   << hasRevPropInfo
		   << "\r\n";
	}
}

///////////////////////////////////////////////////////////////
// construction / destruction (nothing to do)
///////////////////////////////////////////////////////////////

CCSVWriter::CCSVWriter(void)
{
}

CCSVWriter::~CCSVWriter(void)
{
}

///////////////////////////////////////////////////////////////
// write cache content as CSV files (overwrite existing ones)
// file names are extensions to the given fileName
///////////////////////////////////////////////////////////////

void CCSVWriter::Write ( const CCachedLogInfo& cache
					   , const std::wstring& fileName)
{
	std::ofstream authors ((fileName + L".authors.csv").c_str());
	WriteStringList (authors, cache.GetLogInfo().GetAuthors());
	std::ofstream userRevPropNames ((fileName + L".revpropnames.csv").c_str());
	WriteStringList (userRevPropNames, cache.GetLogInfo().GetUserRevProps());
	std::ofstream paths ((fileName + L".paths.csv").c_str());
	WritePathList (paths, cache.GetLogInfo().GetPaths());

	std::ofstream changes ((fileName + L".changes.csv").c_str());
	WriteChanges (changes, cache);
	std::ofstream merges ((fileName + L".merges.csv").c_str());
	WriteMerges (merges, cache);
	std::ofstream userRevProps ((fileName + L".userrevprops.csv").c_str());
	WriteRevProps (userRevProps, cache);

	std::ofstream revisions ((fileName + L".revisions.csv").c_str());
	WriteRevisions (revisions, cache);
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}