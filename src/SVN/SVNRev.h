// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "svn_opt.h"


/**
 * \ingroup SVN
 * SVNRev represents a subversion revision. A subversion revision can
 * be either a simple revision number, a date or a string with one
 * of the following keywords:
 * - BASE
 * - COMMITTED
 * - PREV
 * - HEAD
 *
 * For convenience, this class also accepts the string "WC" as a
 * keyword for the working copy revision.
 *
 * Right now, this class also accepts those "special" revisions
 * as a normal revision number. This is just for convenience to stay
 * compatible with the current implementation in TortoiseSVN, but
 * will be removed soon!
 */
class SVNRev
{
public:
	SVNRev(LONG nRev);
	SVNRev(CString sRev);
	SVNRev(svn_opt_revision_t revision) {rev = revision;}
	SVNRev(){rev.kind = svn_opt_revision_unspecified;m_bIsValid = FALSE;}
	~SVNRev();

	/// returns TRUE if the revision is valid (i.e. not unspecified)
	BOOL IsValid() const {return m_bIsValid;}
	/// returns TRUE if the revision is HEAD
	BOOL IsHead() const {return (rev.kind == svn_opt_revision_head);}
	/// returns TRUE if the revision is BASE
	BOOL IsBase() const {return (rev.kind == svn_opt_revision_base);}
	/// returns TRUE if the revision is WORKING
	BOOL IsWorking() const {return (rev.kind == svn_opt_revision_working);}
	/// returns TRUE if the revision is PREV
	BOOL IsPrev() const {return (rev.kind == svn_opt_revision_previous);}
	/// returns TRUE if the revision is COMMITTED
	BOOL IsCommitted() const {return (rev.kind == svn_opt_revision_committed);}
	/// returns TRUE if the revision is a date
	BOOL IsDate() const {return (rev.kind == svn_opt_revision_date);}
	/// returns TRUE if the revision is a number
	BOOL IsNumber() const {return (rev.kind == svn_opt_revision_number);}

	/// Returns a string representing the date of a DATE revision, otherwise an empty string.
	CString GetDateString() const {return sDate;}
	/// Converts the revision into a string representation.
	CString ToString() const;

	operator LONG () const;
	operator svn_opt_revision_t * ();
	enum
	{
		REV_HEAD = -1,			///< head revision
		REV_BASE = -2,			///< base revision
		REV_WC = -3,			///< revision of the working copy
		REV_UNSPECIFIED = -4,	///< unspecified revision
	};
protected:
	void Create(svn_revnum_t nRev);
	void Create(CString sRev);
private:
	svn_opt_revision_t rev;
	BOOL m_bIsValid;
	CString sDate;
};