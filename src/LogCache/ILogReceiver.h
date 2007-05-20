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

///////////////////////////////////////////////////////////////
// data structures to accomodate the change list 
// (taken from the SVN class)
///////////////////////////////////////////////////////////////

struct LogChangedPath
{
	CString sPath;
	CString sCopyFromPath;
	svn_revnum_t lCopyFromRev;
	DWORD action;

	// convenience method

	const CString& GetAction() const;

private:

	// cached return value of GetAction()

	mutable CString actionAsString;
};

enum
{
	LOGACTIONS_ADDED	= 0x00000001,
	LOGACTIONS_MODIFIED	= 0x00000002,
	LOGACTIONS_REPLACED	= 0x00000004,
	LOGACTIONS_DELETED	= 0x00000008
};

typedef CArray<LogChangedPath*, LogChangedPath*> LogChangedPathArray;

///////////////////////////////////////////////////////////////
//
// ILogReceiver
//
//		Implement this to receive log information. It will
//		be used as a callback in ILogQuery::Log().
//
//		To cancel the log and / or indicate errors, throw
//		a SVNError exception.
//
///////////////////////////////////////////////////////////////

class ILogReceiver
{
public:

	// call-back for every revision found
	// (called at most once per revision)

	// may throw a SVNError to cancel the log

	virtual void ReceiveLog ( LogChangedPathArray* changes
							, svn_revnum_t rev
							, const CString& author
							, const apr_time_t& timeStamp
							, const CString& message) = 0;
};
