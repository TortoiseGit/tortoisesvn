// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#pragma once
#include "svnproperties.h"

#define BUGTRAQPROPNAME_LABEL             _T("bugtraq:label")
#define BUGTRAQPROPNAME_MESSAGE           _T("bugtraq:message")
#define BUGTRAQPROPNAME_REGEX             _T("bugtraq:regex")
#define BUGTRAQPROPNAME_URL               _T("bugtraq:url")

class BugtraqInfo
{
public:
	BugtraqInfo(void);
	~BugtraqInfo(void);

	/**
	 * Reads the bugtracking properties from a path. If the path is a file
	 * then the properties are read from the parent folder of that file.
	 * \param path path to a file or a folder
	 */
	BOOL ReadProps(CString path);

public:
	/* The label to show in the commit dialog where the issue number/bug id
	 * is entered. Example: "Bug-ID: " or "Issue-No.:". Default is "Bug-ID :" */
	CString		sLabel;
	/* The message string to add below the log message the user entered.
	 * It must contain the string "%BUGID%" which gets replaced by the client 
	 * with the issue number / bug id the user entered. */
	CString		sMessage;
	/* A regular expression a client can use to check the validity of the
	 * issue number / bug id the user entered. The client should prompt
	 * the user if it is not valid. An empty string is always valid and
	 * means the commit is not assigned to a specific issue (e.g. spell fixes,
	 * comments, intendation, ... */
	CString		sRegex;
	/* The url pointing to the issue tracker. If the url contains the string
	 * "%BUGID% the client has to replace it with the issue number / bug id
	 * the user entered. */
	CString		sUrl;
};
