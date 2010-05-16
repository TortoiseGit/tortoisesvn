// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007,2009-2010 - TortoiseSVN

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
// temporarily used to disambiguate LogChangedPath definitions
///////////////////////////////////////////////////////////////

#ifndef __ILOGRECEIVER_H__
#define __ILOGRECEIVER_H__
#endif

///////////////////////////////////////////////////////////////
// required includes
///////////////////////////////////////////////////////////////

#include "svn_types.h"

/**
 * data structure to accommodate the change list.
 */

struct SChangedPath
{
    CString path;
    CString copyFromPath;
    svn_revnum_t copyFromRev;
    svn_node_kind_t nodeKind;
    DWORD action;
    svn_tristate_t text_modified;
    svn_tristate_t props_modified;
};

enum
{
    LOGACTIONS_ADDED    = 0x00000001,
    LOGACTIONS_MODIFIED = 0x00000002,
    LOGACTIONS_REPLACED = 0x00000004,
    LOGACTIONS_DELETED  = 0x00000008
};

/**
 * Factory and container for LogChangedPath objects.
 * Provides just enough methods to read them.
 */

typedef std::vector<SChangedPath> TChangedPaths;

/**
 * standard revision properties
 */

class StandardRevProps
{
private:

    CString author;
    CString message;
    apr_time_t timeStamp;

public:

    /// construction

    StandardRevProps
        ( const CString& author
        , const CString& message
        , apr_time_t timeStamp);

    /// r/o data access

    const CString& GetAuthor() const {return author;}
    const CString& GetMessage() const {return message;}
    apr_time_t GetTimeStamp() const {return timeStamp;}

};

/**
 * data structure to accommodate the list of user-defined revision properties.
 */

class UserRevProp
{
private:

    CString name;
    CString value;

    // construction is only allowed through the container

    friend class UserRevPropArray;

public:

    /// r/o data access

    const CString& GetName() const {return name;}
    const CString& GetValue() const {return value;}

};

/**
 * Factory and container for UserRevProp objects.
 * Provides just enough methods to read them.
 */

class UserRevPropArray : private std::vector<UserRevProp>
{
public:

    /// construction

    UserRevPropArray();
    UserRevPropArray (size_t initialCapacity);

    /// modification

    void Add
        ( const CString& name
        , const CString& value);

    /// data access

    size_t GetCount() const {return size();}
    const UserRevProp& GetAt (size_t index) const {return at (index);}
    const UserRevProp& operator[] (size_t index) const {return at (index);}
};


/**
 * Interface for receiving log information. It will be used as a callback
 * in ILogQuery::Log().
 *
 * To cancel the log and/or indicate errors, throw an SVNError exception.
 */
class ILogReceiver
{
public:

    /// call-back for every revision found
    /// (called at most once per revision)
    ///
    /// the implementation may modify but not delete()
    /// the data containers passed to it
    ///
    /// any pointer may be NULL
    ///
    /// may throw a SVNError to cancel the log

    virtual void ReceiveLog ( TChangedPaths* changes
                            , svn_revnum_t rev
                            , const StandardRevProps* stdRevProps
                            , UserRevPropArray* userRevProps
                            , bool mergesFollow) = 0;
};
