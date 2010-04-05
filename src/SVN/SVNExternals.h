// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include "TSVNPath.h"
#include <string>
#include <vector>
#include <map>

/**
 * Data class to hold information about one svn:external 'line', i.e.,
 * one external definition.
 */
class SVNExternal
{
public:
	SVNExternal() : adjust(false) {}

	CTSVNPath			path;				///< path of the folder that has the property
	CString				pathurl;			///< the repository url of path
	CString				targetDir;			///< the target folder where the external is put
	CString				url;				///< the url of the external
	svn_opt_revision_t	revision;			///< the revision the external should be tagged to
	svn_opt_revision_t	origrevision;		///< the revision the external is tagged to
	svn_opt_revision_t	pegrevision;		///< the peg revision, if it has one
	bool				adjust;				///< whether the external requires tagging
};

/**
 * Handles svn:externals and helps with tagging them to a specific revision.
 */
class SVNExternals : public std::vector<SVNExternal>
{
public:
	SVNExternals();
	virtual ~SVNExternals();

	/**
	 * Adds a new svn:external property value, parses that value and fills the
	 * class vector with the individual externals.
	 * \param path the path of the folder that has the property
	 * \param extvalue the value of the svn:external property
	 * \param fetchrev if true, the highest 'last commit revision' is searched
	 *                 in the target dir (the external directory)
	 * \param headrev  the revision the external should be tagged to
	 */
	bool Add(const CTSVNPath& path, const std::string& extvalue, bool fetchrev, svn_revnum_t headrev = -1);
	
	/** 
	 * changes the svn:externals property with the fixed revisions.
	 * \param bRemote if false, the externals are changed in the working copy. If true
	 *                then \c message, \c headrev, \c origurl and \c tagurl must also be set
	 *                to change the properties in the repository.
	 */
	bool TagExternals(bool bRemote, const CString& message = CString(), svn_revnum_t headrev = -1, const CTSVNPath& origurl = CTSVNPath(), const CTSVNPath& tagurl = CTSVNPath());
	/// Restores all svn:external properties
	bool RestoreExternals();
	/// returns the original svn:externals value, without modifications
    std::string GetOriginalValue(const CTSVNPath& path) { return m_originals[path]; }
    /// returns the svn:externals value for the specified \c path
    std::string GetValue(const CTSVNPath& path);

private:
	std::map<CTSVNPath, std::string>	m_originals;
};