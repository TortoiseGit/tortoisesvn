// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

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
#include "XMLLogWriter.h"
#include "FormatTime.h"
#include "../Streams/BufferedOutFile.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// write <date> tag
///////////////////////////////////////////////////////////////

void CXMLLogWriter::WriteTimeStamp ( CBufferedOutFile& file
                                   , __time64_t timeStamp )
{
    enum {BUFFER_SIZE = 100};
    char buffer[BUFFER_SIZE];

    Time64ToZuluString (buffer, timeStamp);
    if (buffer[0] != 0)
        file << "<date>" << buffer << "</date>\n";
}

///////////////////////////////////////////////////////////////
// write <paths> tag
///////////////////////////////////////////////////////////////

void CXMLLogWriter::WriteChanges ( CBufferedOutFile& file
                                 , CChangesIterator iter
                                 , const CChangesIterator& last )
{
    // empty change sets won't show up in the log

    if ( iter == last )
        return;

    // static text blocks

    static const std::string startText = "<paths>\n";
    static const std::string pathStartText = "<path";

    static const std::string copyFromPathText = "\n   copyfrom-path=\"";
    static const std::string copyFromRevisionText = "\"\n   copyfrom-rev=\"";
    static const std::string copyFromEndText = "\"";

    static const std::string addedActionText = "\n   action=\"A\"";
    static const std::string modifiedActionText = "\n   action=\"M\"";
    static const std::string replacedActionText = "\n   action=\"R\"";
    static const std::string deletedActionText = "\n   action=\"D\"";

    static const std::string pathText = ">";
    static const std::string pathEndText = "</path>\n";
    static const std::string endText = "</paths>\n";

    // write all changes

    file << startText;

    for ( ; iter != last; ++iter )
    {
        file << pathStartText;

        if ( iter->HasFromPath() )
        {
            file << copyFromPathText;
            file << iter->GetFromPath().GetPath();
            file << copyFromRevisionText;
            file << iter->GetFromRevision();
            file << copyFromEndText;
        }

        switch ( iter->GetAction() )
        {
            case CRevisionInfoContainer::ACTION_CHANGED:
                file << modifiedActionText;
                break;
            case CRevisionInfoContainer::ACTION_ADDED:
                file << addedActionText;
                break;
            case CRevisionInfoContainer::ACTION_DELETED:
                file << deletedActionText;
                break;
            case CRevisionInfoContainer::ACTION_REPLACED:
                file << replacedActionText;
                break;
            default:
                break;
        }

        file << pathText;
        file << iter->GetPath().GetPath();

        file << pathEndText;
    }

    file << endText;
}

///////////////////////////////////////////////////////////////
// write <logentry> tag
///////////////////////////////////////////////////////////////

void CXMLLogWriter::WriteRevisionInfo ( CBufferedOutFile& file
                                      , const CRevisionInfoContainer& info
                                      , revision_t revision
                                      , index_t index )
{
    static const std::string startText = "<logentry\n   revision=\"";
    static const std::string revisionEndText = "\">\n";
    static const std::string startAuthorText = "<author>";
    static const std::string endAuthorText = "</author>\n";
    static const std::string msgText = "<msg>";
    static const std::string endText = "</msg>\n</logentry>\n";

    // <logentry> start tag (includes revision)

    file << startText;
    file << revision;
    file << revisionEndText;

    // write the author, if given

    const char* author = info.GetAuthor ( index );
    if ( *author != 0 )
    {
        file << startAuthorText;
        file << author;
        file << endAuthorText;
    }

    // add time stamp, if given

    WriteTimeStamp ( file, info.GetTimeStamp ( index ) );

    // list the changed paths, if given

    WriteChanges ( file, info.GetChangesBegin ( index ), info.GetChangesEnd ( index ) );

    // always add the commit message

    file << msgText;
    file << info.GetComment ( index );

    file << endText;
}

///////////////////////////////////////////////////////////////
// dump the revisions in descending order
///////////////////////////////////////////////////////////////

void CXMLLogWriter::WriteRevionsTopDown ( CBufferedOutFile& file
        , const CCachedLogInfo& source )
{
    const CRevisionIndex& revisions = source.GetRevisions();
    const CRevisionInfoContainer& info = source.GetLogInfo();

    for ( revision_t revision = revisions.GetLastRevision()-1
        , fristRevision = revisions.GetFirstRevision()
        ; revision+1 > fristRevision
        ; --revision )
    {
        index_t index = revisions[revision];
        if ( index != NO_INDEX )
        {
            WriteRevisionInfo ( file, info, revision, index );
        }
    }
}

///////////////////////////////////////////////////////////////
// dump the revisions in ascending order
///////////////////////////////////////////////////////////////

void CXMLLogWriter::WriteRevionsBottomUp ( CBufferedOutFile& file
                                         , const CCachedLogInfo& source )
{
    const CRevisionIndex& revisions = source.GetRevisions();
    const CRevisionInfoContainer& info = source.GetLogInfo();

    for ( revision_t revision = revisions.GetFirstRevision()
        , lastRevision = revisions.GetLastRevision()
        ; revision < lastRevision
        ; ++revision )
    {
        index_t index = revisions[revision];
        if ( index != NO_INDEX )
        {
            WriteRevisionInfo ( file, info, revision, index );
        }
    }
}


///////////////////////////////////////////////////////////////
// write the whole change content
///////////////////////////////////////////////////////////////

void CXMLLogWriter::SaveToXML ( const TFileName& xmlFileName
                              , const CCachedLogInfo& source
                              , bool topDown )
{
    CBufferedOutFile file ( xmlFileName );

    const char* header = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<log>\n";
    const char* footer = "</log>\n";

    file << header;

    if ( topDown )
    {
        WriteRevionsTopDown ( file, source );
    }
    else
    {
        WriteRevionsBottomUp ( file, source );
    }

    file << footer;
}


///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
