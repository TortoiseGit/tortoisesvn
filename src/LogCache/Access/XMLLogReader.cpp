// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010, 2012, 2014 - TortoiseSVN

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
#include "XMLLogReader.h"
#include "FormatTime.h"
#include "../Streams/MappedInFile.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// a strstr-like utility that works on memory buffers
///////////////////////////////////////////////////////////////

const char* CXMLLogReader::limited_strstr ( const char* first
                                          , const char* last
                                          , const char* sub
                                          , size_t subLen)
{
    assert (last > first);
    assert (sub != NULL);

    for (; ; ++first)
    {
        first = (const char*) memchr (first, *sub, last - first);
        if ( (first == NULL) || (last < first + subLen))
            return NULL;

        if (memcmp (first, sub, subLen) == 0)
            break;
    }

    return first;
}

///////////////////////////////////////////////////////////////
// find the next tagged section
///////////////////////////////////////////////////////////////

bool CXMLLogReader::GetXMLTag ( const char* start
                              , const char* parentEnd
                              , const char* startTagName
                              , size_t startTagNameLen
                              , const char* endTagName
                              , size_t endTagNameLen
                              , const char*& tagStart
                              , const char*& tagEnd)
{
    // check our internal logic

    assert (start < parentEnd);
    assert (start != NULL);

    // find the next tag with the given name

    tagStart = limited_strstr (start, parentEnd, startTagName, startTagNameLen);
    if (tagStart == NULL)
        return false;

    // tag has been found. return first attribute or content position

    tagStart += startTagNameLen;

    // find end tag

    tagEnd = limited_strstr (tagStart, parentEnd, endTagName, endTagNameLen);
    if (tagEnd == NULL)
        return false;

    // both tags have been found

    return true;
}

bool CXMLLogReader::GetLogTag ( const char* start
                              , const char* parentEnd
                              , const char*& tagStart
                              , const char*& tagEnd)
{
    // find the next tag with the given name

    tagStart = limited_strstr (start, parentEnd, "<log", 4);
    if (tagStart == NULL)
        return false;

    // tag has been found. return first attribute or content position

    tagStart += 4;

    // find end tag
    // be clever and look at the end of the buffer

    const char* endTagSearchStart = parentEnd - tagStart > 100
                                  ? parentEnd - 100
                                  : tagStart;

    tagEnd = limited_strstr (endTagSearchStart, parentEnd, "</log>", 6);
    if (tagEnd == NULL)
    {
        // try harder

        tagEnd = limited_strstr (tagStart, parentEnd, "</log>", 6);
        if (tagEnd == NULL)
            return false;
    }

    // both tags have been found

    return true;
}

///////////////////////////////////////////////////////////////
// get attribute values from a range returned by GetXMLTag
///////////////////////////////////////////////////////////////

const char* CXMLLogReader::GetXMLAttributeOffset ( const char* start
                                                 , const char* end
                                                 , const char* attribute
                                                 , size_t attributeLen)
{
    const char* tagEnd = (const char*) memchr (start, '>', end - start);
    if ((tagEnd != NULL) && (tagEnd < end))
        end = tagEnd;

    while (start != NULL)
    {
        start = (const char*) memchr (start, '=', end - start);
        if (start == NULL)
            break;

        bool found = memcmp (start - attributeLen, attribute, attributeLen) == 0;
        start += 2;

        if (found)
            break;

        start = (const char*) memchr (start, '"', end - start);
    }

    return start;
}

revision_t CXMLLogReader::GetXMLRevisionAttribute ( const char* start
                                                  , const char* end
                                                  , const char* attribute
                                                  , size_t attributeLen)
{
    start = GetXMLAttributeOffset (start, end, attribute, attributeLen);
    return start == NULL
           ? NO_REVISION
           : atoi (start);
}

std::string CXMLLogReader::GetXMLTextAttribute ( const char* start
                                               , const char* end
                                               , const char* attribute
                                               , size_t attributeLen)
{
    start = GetXMLAttributeOffset (start, end, attribute, attributeLen);
    if (start == NULL)
        return std::string();

    const char* quotes = (const char*) memchr (start, '"', end - start);
    return std::string (start, quotes == NULL ? end : quotes);
}

///////////////////////////////////////////////////////////////
// get the text between the given tags (e.g. for <path>)
///////////////////////////////////////////////////////////////

std::string CXMLLogReader::GetXMLTaggedText ( const char* start
                                            , const char* end
                                            , const char* startTagName
                                            , size_t startTagNameLen
                                            , const char* endTagName
                                            , size_t endTagNameLen)
{
    if (GetXMLTag ( start
                  , end
                  , startTagName
                  , startTagNameLen
                  , endTagName
                  , endTagNameLen
                  , start
                  , end))
    {
        start = (const char*) memchr (start, '>', end - start) +1;
        if (start != NULL)
            return std::string (start, end);
    }

    return std::string();
}

///////////////////////////////////////////////////////////////
// parse all <path> tags within a <logentry> tag
///////////////////////////////////////////////////////////////

void CXMLLogReader::ParseChanges (const char* current
                                  , const char* changesEnd
                                  , CCachedLogInfo& target)
{
    const char* changeEnd = NULL;
    while (GetXMLTag ( current
                     , changesEnd
                     , "<path"
                     , 5
                     , "</path>"
                     , 7
                     , current
                     , changeEnd))
    {
        std::string actionText
            = GetXMLTextAttribute (current, changesEnd, "action", 6);
        std::string fromPath
            = GetXMLTextAttribute (current, changesEnd, "copyfrom-path", 13);
        revision_t fromRevision
            = GetXMLRevisionAttribute (current, changesEnd, "copyfrom-rev", 12);
        std::string textModifiesText
            = GetXMLTextAttribute (current, changesEnd, "textModifies", 12);
        std::string propsModifiesText
            = GetXMLTextAttribute (current, changesEnd, "propsModifies", 13);

        current = (const char*) memchr (current, '>', changesEnd - current) +1;
        std::string path (current, changeEnd);

        TChangeAction action = CRevisionInfoContainer::ACTION_ADDED;

        switch (actionText[0])
        {
            case 'A':
                break;

            case 'M':
                action = CRevisionInfoContainer::ACTION_CHANGED;
                break;

            case 'R':
                action = CRevisionInfoContainer::ACTION_REPLACED;
                break;

            case 'D':
                action = CRevisionInfoContainer::ACTION_DELETED;
                break;

            case 'V':
                action = CRevisionInfoContainer::ACTION_MOVED;
                break;

            case 'E':
                action = CRevisionInfoContainer::ACTION_MOVEREPLACED;
                break;

            default:

                std::cerr << "ignoring unknown action type" << std::endl;
                continue;
        }

        unsigned char textModifies = 0;
        switch (textModifiesText[0])
        {
        case 'T':
            textModifies = 2;
            break;

        case 'F':
            textModifies = 1;
            break;

        case 'U':
            textModifies = 0;
            break;

        default:
            std::cerr << "ignoring unknown textModifies type" << std::endl;
            continue;
    }

        unsigned char propsModifies = 0;
        switch (propsModifiesText[0])
        {
        case 'T':
            propsModifies = 2;
            break;

        case 'F':
            propsModifies = 1;
            break;

        case 'U':
            propsModifies = 0;
            break;

        default:
            std::cerr << "ignoring unknown textModifies type" << std::endl;
            continue;
        }


        target.AddChange (action, node_unknown, path, fromPath, fromRevision, textModifies, propsModifies);
    }
}

///////////////////////////////////////////////////////////////
// parse all <logentry> tags
///////////////////////////////////////////////////////////////

void CXMLLogReader::ParseXMLLog (const char* current
                                 , const char* logEnd
                                 , CCachedLogInfo& target)
{
    const char* revisionEnd = NULL;
    while (GetXMLTag ( current
                     , logEnd
                     , "<logentry"
                     , 9
                     , "</logentry>"
                     , 11
                     , current
                     , revisionEnd))
    {
        revision_t revision
            = GetXMLRevisionAttribute (current, revisionEnd, "revision", 8);
        std::string author
            = GetXMLTaggedText (current, revisionEnd, "<author", 7, "</author>", 9);
        std::string date
            = GetXMLTaggedText (current, revisionEnd, "<date", 5, "</date>", 7);
        std::string comment
            = GetXMLTaggedText (current, revisionEnd, "<msg", 4, "</msg>", 6);

        if (revision % 10000 == 0)
            printf (".");

        __time64_t timeStamp = ZuluStringToTime64 (date.c_str());
        target.Insert (revision, author, comment, timeStamp);

        const char* pathsEnd = NULL;
        if (GetXMLTag ( current
                      , revisionEnd
                      , "<paths"
                      , 6
                      , "</paths>"
                      , 8
                      , current
                      , pathsEnd))
        {
            ParseChanges (current, pathsEnd, target);
        }

        current = revisionEnd;
    }
}

// map file to memory, parse it and fill the target

void CXMLLogReader::LoadFromXML ( const TFileName& xmlFileName
                                , CCachedLogInfo& target)
{
    CMappedInFile file (xmlFileName);

    const char* logStart = NULL;
    const char* logEnd = NULL;

    if (GetLogTag ( (const char*) file.GetBuffer()
                  , (const char*) file.GetBuffer() + file.GetSize()
                  , logStart
                  , logEnd))
    {
        ParseXMLLog (logStart, logEnd, target);
    }
    else
    {
        std::cerr << "XML file contains no log information" << std::endl;
    }
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

