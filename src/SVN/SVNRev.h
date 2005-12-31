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
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date MAR-2004
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
class SVNRev
{
public:
	SVNRev(LONG nRev);
	SVNRev(CString sRev);
	SVNRev(svn_opt_revision_t revision) {rev = revision;}
	SVNRev(){rev.kind = svn_opt_revision_unspecified;m_bIsValid = FALSE;}
	~SVNRev();

	BOOL IsValid() const {return m_bIsValid;}
	BOOL IsHead() const {return (rev.kind == svn_opt_revision_head);}
	BOOL IsBase() const {return (rev.kind == svn_opt_revision_base);}
	BOOL IsWorking() const {return (rev.kind == svn_opt_revision_working);}
	BOOL IsPrev() const {return (rev.kind == svn_opt_revision_previous);}
	BOOL IsCommitted() const {return (rev.kind == svn_opt_revision_committed);}
	BOOL IsDate() const {return (rev.kind == svn_opt_revision_date);}

	CString GetDateString() const {return sDate;}
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