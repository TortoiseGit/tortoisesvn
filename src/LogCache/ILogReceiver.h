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
