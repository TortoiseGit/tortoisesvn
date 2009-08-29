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

// construction 

LogChangedPath::LogChangedPath 
    ( const CString& path
    , const CString& copyFromPath
    , svn_revnum_t copyFromRev
    , svn_node_kind_t nodeKind
    , DWORD action)
    : path (path)
    , copyFromPath (copyFromPath)
    , copyFromRev (copyFromRev)
    , nodeKind (nodeKind)
    , action (action)
{
}

LogChangedPath::LogChangedPath 
    ( const CString& path
    , svn_node_kind_t nodeKind
    , DWORD action)
    : path (path)
    , copyFromPath()
    , copyFromRev (0)
    , nodeKind (nodeKind)
    , action (action)
{
}

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
