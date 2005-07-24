// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
//
#pragma once
#include <iostream>
#include <string>
#include "regexpr2.h"
using namespace std;
using namespace regex;

#define BUGTRAQPROPNAME_LABEL             _T("bugtraq:label")
#define BUGTRAQPROPNAME_MESSAGE           _T("bugtraq:message")
#define BUGTRAQPROPNAME_NUMBER            _T("bugtraq:number")
#define BUGTRAQPROPNAME_LOGREGEX		  _T("bugtraq:logregex")
#define BUGTRAQPROPNAME_URL               _T("bugtraq:url")
#define BUGTRAQPROPNAME_WARNIFNOISSUE     _T("bugtraq:warnifnoissue")
#define BUGTRAQPROPNAME_APPEND		      _T("bugtraq:append")

#define PROJECTPROPNAME_LOGTEMPLATE		  _T("tsvn:logtemplate")
#define PROJECTPROPNAME_LOGWIDTHLINE	  _T("tsvn:logwidthmarker")
#define PROJECTPROPNAME_LOGMINSIZE		  _T("tsvn:logminsize")
#define PROJECTPROPNAME_LOCKMSGMINSIZE	  _T("tsvn:lockmsgminsize")
#define PROJECTPROPNAME_LOGFILELISTLANG	  _T("tsvn:logfilelistenglish")
#define PROJECTPROPNAME_PROJECTLANGUAGE   _T("tsvn:projectlanguage")

class CTSVNPathList;

/**
 * \ingroup TortoiseProc
 * Provides methods for retrieving information about bug/issuetrackers
 * associated with a Subversion repository/working copy and other project
 * related properties.
 *
 *
 * \version 1.0
 * first version
 *
 * \date 08-21-2004
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class ProjectProperties
{
public:
	ProjectProperties(void);
	~ProjectProperties(void);

	/**
	 * Reads the properties from a path. If the path is a file
	 * then the properties are read from the parent folder of that file.
	 * \param path path to a file or a folder
	 */
	BOOL ReadProps(CTSVNPath path);
	/**
	 * Reads the properties from all paths found in a pathlist.
	 * This method calls ReadProps() for each path .
	 * \param list of paths
	 */
	BOOL ReadPropsPathList(const CTSVNPathList& pathList);

	/**
	 * Searches for the BugID inside a log message. If one is found,
	 * the method returns TRUE. The rich edit control is used to set
	 * the CFE_LINK effect on the BugID's.
	 * \param msg the log message
	 * \param pWnd Pointer to a rich edit control
	 */
	BOOL FindBugID(const CString& msg, CWnd * pWnd);
	
	/**
	 * Checks if the bug ID is valid. If bugtraq:number is 'true', then the
	 * functions checks if the bug ID doens't contain any non-number chars in it.
	 */
	BOOL CheckBugID(const CString& sID);
	
	/**
	 * Checks if the log message \c sMessage contains a bug ID. This is done by
	 * using the bugtraq:checkre property.
	 */
	BOOL HasBugID(const CString& sMessage);
	
	/**
	 * Returns the URL pointing to the Issue in the issuetracker. The URL is
	 * created from the bugtraq:url property and the BugID found in the logmessage.
	 * \param msg the BugID extracted from the log message
	 */
	CString GetBugIDUrl(const CString& sBugID);

public:
	/** The label to show in the commit dialog where the issue number/bug id
	 * is entered. Example: "Bug-ID: " or "Issue-No.:". Default is "Bug-ID :" */
	CString		sLabel;

	/** The message string to add below the log message the user entered.
	 * It must contain the string "%BUGID%" which gets replaced by the client 
	 * with the issue number / bug id the user entered. */
	CString		sMessage;

	/** If this is set, then the bug-id / issue number must be a number, no text */
	BOOL		bNumber;

	/** replaces bNumer: a regular expression string to check the validity of
	  * the entered bug ID. */
	CString		sCheckRe;
	
	/** used to extract the bug ID from the string matched by sCheckRe */
	CString		sBugIDRe;
	
	/** The url pointing to the issue tracker. If the url contains the string
	 * "%BUGID% the client has to replace it with the issue number / bug id
	 * the user entered. */
	CString		sUrl;
	
	/** If set to TRUE, show a warning dialog if the user forgot to enter
	 * an issue number in the commit dialog. */
	BOOL		bWarnIfNoIssue;

	/** If set to FALSE, then the bugtracking entry is inserted at the top of the
	   log message instead of at the bottom. Default is TRUE */
	BOOL		bAppend;

	/** The number of chars the width marker should be shown at. If the property
	 * is not set, then this value is 80 by default. */
	int			nLogWidthMarker;

	/** The template to use for log messages. */
	CString		sLogTemplate;

	/** Minimum size a log message must have in chars */
	int			nMinLogSize;

	/** Minimum size a lock message must have in chars */
	int			nMinLockMsgSize;

	/** TRUE if the file list to be inserted in the commit dialog should be in
	 * english and not in the localized language. Default is TRUE */
	BOOL		bFileListInEnglish;
	
	/** The language identifier this project uses for log messages. */
	LONG		lProjectLanguage;
private:
	rpattern	patCheckRe;
	rpattern	patBugIDRe;
};
