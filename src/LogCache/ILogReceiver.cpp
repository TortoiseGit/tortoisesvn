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
#include "StdAfx.h"
#include "Resource.h"

#include "ILogReceiver.h"

///////////////////////////////////////////////////////////////
// data structures to accommodate the change list 
// (taken from the SVN class)
///////////////////////////////////////////////////////////////

// convenience method

const CString& LogChangedPath::GetActionString (DWORD action)
{
	static CString addActionString;
	static CString deleteActionString;
	static CString replacedActionString;
	static CString modifiedActionString;
	static CString empty;

	switch (action)
	{
	case LOGACTIONS_ADDED: 
		if (addActionString.IsEmpty())
			addActionString.LoadString(IDS_SVNACTION_ADD);

		return addActionString;

	case LOGACTIONS_DELETED: 
		if (deleteActionString.IsEmpty())
			deleteActionString.LoadString(IDS_SVNACTION_DELETE);

		return deleteActionString;

	case LOGACTIONS_REPLACED: 
		if (replacedActionString.IsEmpty())
			replacedActionString.LoadString(IDS_SVNACTION_REPLACED);

		return replacedActionString;

	case LOGACTIONS_MODIFIED: 
		if (modifiedActionString.IsEmpty())
			modifiedActionString.LoadString(IDS_SVNACTION_MODIFIED);

		return modifiedActionString;

	default:
		// there should always be an action
		assert (0);

	}

    return empty;
}

const CString& LogChangedPath::GetActionString() const
{
	if (actionAsString.IsEmpty())
        actionAsString = GetActionString (action);

    return actionAsString;
}

// construction

LogChangedPathArray::LogChangedPathArray()
{
}

LogChangedPathArray::LogChangedPathArray (size_t initialCapacity)
{
    reserve (initialCapacity);
}

// modification

void LogChangedPathArray::Add
    ( const CString& path
    , const CString& copyFromPath
    , svn_revnum_t copyFromRev
    , svn_node_kind_t nodeKind
    , DWORD action)
{
    push_back (LogChangedPath());

    LogChangedPath& item = back();
    item.path = path;
    item.copyFromPath = copyFromPath;
    item.copyFromRev = copyFromRev;
    item.nodeKind = nodeKind;
    item.action = action;
}

void LogChangedPathArray::Add
    ( const CString& path
    , svn_node_kind_t nodeKind
    , DWORD action)
{
    push_back (LogChangedPath());

    LogChangedPath& item = back();
    item.path = path;
    item.copyFromRev = 0;
    item.nodeKind = nodeKind;
    item.action = action;
}

void LogChangedPathArray::Add (const LogChangedPath& item)
{
    push_back (item);
}

void LogChangedPathArray::RemoveAll()
{
    clear();
}

void LogChangedPathArray::Sort (int column, bool ascending)
{
    struct Order
    {
    private:

        int column;
        bool ascending;

    public:

        Order (int column, bool ascending)
            : column (column)
            , ascending (ascending)
        {
        }

        bool operator()(const LogChangedPath& lhs, const LogChangedPath& rhs) const
        {
            const LogChangedPath* cpath1 = ascending ? &lhs : &rhs;
	        const LogChangedPath* cpath2 = ascending ? &rhs : &lhs;

	        int cmp = 0;
	        switch (column)
	        {
	        case 0:	// action
			        cmp = cpath2->GetActionString().Compare (cpath1->GetActionString());
			        if (cmp)
				        return cmp > 0;
			        // fall through
	        case 1:	// path
			        cmp = cpath2->GetPath().CompareNoCase (cpath1->GetPath());
			        if (cmp)
				        return cmp > 0;
			        // fall through
	        case 2:	// copy from path
			        cmp = cpath2->GetCopyFromPath().Compare (cpath1->GetCopyFromPath());
			        if (cmp)
				        return cmp > 0;
			        // fall through
	        case 3:	// copy from revision
			        return cpath2->GetCopyFromRev() > cpath1->GetCopyFromRev();
	        }

	        return false;
        }
    };

    std::sort (begin(), end(), Order (column, ascending));
}

// construction

StandardRevProps::StandardRevProps 
    ( const CString& author
    , const CString& message
    , apr_time_t timeStamp)
    : author (author)
    , message (message)
    , timeStamp (timeStamp)
{
}

// construction

UserRevPropArray::UserRevPropArray()
{
}

UserRevPropArray::UserRevPropArray (size_t initialCapacity)
{
    reserve (initialCapacity);
}

// modification

void UserRevPropArray::Add
    ( const CString& name
    , const CString& value)
{
    push_back (UserRevProp());

    UserRevProp& item = back();
    item.name = name;
    item.value = value;
}

