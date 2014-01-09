// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010, 2014 - TortoiseSVN

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
#include "CSVWriter.h"
#include "FormatTime.h"

#include "../Containers/RevisionInfoContainer.h"
#include "../Containers/CachedLogInfo.h"

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

    os << "ID,Name\n";

    // content

    for (index_t i = 0, count = strings.size(); i < count; ++i)
        os << i << ",\"" << strings[i] << "\"\n";
}

void CCSVWriter::WritePathList (std::ostream& os, const CPathDictionary& dictionary)
{
    // header

    os << "ID,ParentID,Element,FullPath\n";

    // content

    for (index_t i = 0, count = dictionary.size(); i < count; ++i)
    {
        os << i << ','
           << (int)dictionary.GetParent(i) << ",\""
           << dictionary.GetPathElement(i) << "\",\""
           << CDictionaryBasedPath (&dictionary, i).GetPath().c_str()
           << "\"\n";
    }
}

void CCSVWriter::WriteChanges (std::ostream& os, const CCachedLogInfo& cache)
{
    // header

    os << "ID,Revision,Change,PathID,CopyFromRev,CopyFromPathID\n";

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
                       << "\n";
                }
                else
                {
                    os << -1 << ','
                       << -1
                       << "\n";
                }
            }
        }
    }
}

void CCSVWriter::WriteMerges (std::ostream& os, const CCachedLogInfo& cache)
{
    // header

    os << "ID,Revision,FromPathID,ToPathID,StartRevision,RangeLength\n";

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
                   << "\n";
            }
        }
    }
}

void CCSVWriter::WriteRevProps (std::ostream& os, const CCachedLogInfo& cache)
{
    // header

    os << "ID,Revision,RevPropID,Value\n";

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
            & CRevisionInfoContainer::HAS_USERREVPROPS)
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
                   << "\"\n";
            }
        }
    }
}

void CCSVWriter::WriteRevisions (std::ostream& os, const CCachedLogInfo& cache)
{
    // header

    os << "Revision,AuthorID,TimeStamp,Comment,"
       << "HasStdInfo,HasChangeInfo,HasMergeInfo,HasUserRevPropInfo\n";

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

        enum {BUFFER_SIZE = 100};
        char buffer[BUFFER_SIZE] = { 0 };

        __time64_t timestamp = logInfo.GetTimeStamp(index);

        os << revision << ','
           << logInfo.GetAuthorID(index) << ','
           << Time64ToZuluString (buffer, timestamp) << ",\""
           << comment.c_str() << "\","
           << hasStdInfo << ','
           << hasChangeInfo << ','
           << hasMergeInfo << ','
           << hasRevPropInfo
           << "\n";
    }
}

void CCSVWriter::WriteSkipRanges (std::ostream& os, const CCachedLogInfo& cache)
{
    // header

    os << "PathID,Path,StartRevision,Length\n";

    // content

    const CSkipRevisionInfo& info = cache.GetSkippedRevisions();

    // ids will be added on-the-fly

    for (size_t i = 0; i < info.GetPathCount(); ++i)
    {
        CDictionaryBasedPath path = info.GetPath(i);
        CSkipRevisionInfo::TRanges ranges = info.GetRanges(i);

        for (size_t k = 0, count = ranges.size(); k < count; ++k)
        {
            os << path.GetIndex() << ",\""
               << path.GetPath().c_str() << "\","
               << ranges[k].first << ','
               << ranges[k].second
               << "\n";
        }
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
                       , const TFileName& fileName)
{
    std::ofstream authors ((fileName + _T(".authors.csv")).c_str());
    WriteStringList (authors, cache.GetLogInfo().GetAuthors());
    std::ofstream userRevPropNames ((fileName + _T(".revpropnames.csv")).c_str());
    WriteStringList (userRevPropNames, cache.GetLogInfo().GetUserRevProps());
    std::ofstream paths ((fileName + _T(".paths.csv")).c_str());
    WritePathList (paths, cache.GetLogInfo().GetPaths());

    std::ofstream changes ((fileName + _T(".changes.csv")).c_str());
    WriteChanges (changes, cache);
    std::ofstream merges ((fileName + _T(".merges.csv")).c_str());
    WriteMerges (merges, cache);
    std::ofstream userRevProps ((fileName + _T(".userrevprops.csv")).c_str());
    WriteRevProps (userRevProps, cache);

    std::ofstream revisions ((fileName + _T(".revisions.csv")).c_str());
    WriteRevisions (revisions, cache);
    std::ofstream skipranges ((fileName + _T(".skipranges.csv")).c_str());
    WriteSkipRanges (skipranges, cache);
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
