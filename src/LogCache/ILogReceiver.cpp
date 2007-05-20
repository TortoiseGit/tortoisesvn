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
// data structures to accomodate the change list 
// (taken from the SVN class)
///////////////////////////////////////////////////////////////

// convenience method

const CString& LogChangedPath::GetAction() const
{
	if (actionAsString.IsEmpty())
	{
		switch (action)
		{
		case LOGACTIONS_ADDED: 
			actionAsString.LoadString(IDS_SVNACTION_ADD);
			break;

		case LOGACTIONS_DELETED: 
			actionAsString.LoadString(IDS_SVNACTION_DELETE);
			break;

		case LOGACTIONS_REPLACED: 
			actionAsString.LoadString(IDS_SVNACTION_REPLACED);
			break;

		case LOGACTIONS_MODIFIED: 
			actionAsString.LoadString(IDS_SVNACTION_MODIFIED);
			break;

		default:
			// there should always be an action
			assert (0);
		}
	}

	return actionAsString;
}
