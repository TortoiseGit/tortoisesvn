// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010, 2012-2013, 2015, 2021-2022 - TortoiseSVN

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

/**
 * Data class to hold information about one svn:external 'line', i.e.,
 * one external definition.
 */
class SVNExternal
{
public:
    SVNExternal()
        : headRev(SVN_INVALID_REVNUM)
        , adjust(false)
    {
        revision.kind     = svn_opt_revision_unspecified;
        origRevision.kind = svn_opt_revision_unspecified;
        pegRevision.kind  = svn_opt_revision_unspecified;
    }
    virtual ~SVNExternal() = default;

    CTSVNPath          path;         ///< path of the folder that has the property
    CString            pathUrl;      ///< the repository url of path
    CString            targetDir;    ///< the target folder where the external is put
    CString            url;          ///< the url of the external
    CString            fullUrl;      ///< the full url of the external (relative urls are resolved)
    CString            root;         ///< the root of the repository or empty string if not known
    svn_revnum_t       headRev;      ///< the HEAD revision of the external repository, or SVN_INVALID_REVNUM if not known
    svn_opt_revision_t revision;     ///< the revision the external should be tagged to
    svn_opt_revision_t origRevision; ///< the revision the external is tagged to
    svn_opt_revision_t pegRevision;  ///< the peg revision, if it has one
    bool               adjust;       ///< whether the external requires tagging
};

/**
 * Handles svn:externals and helps with tagging them to a specific revision.
 */
class SVNExternals : public std::vector<SVNExternal>
{
public:
    SVNExternals();
    ~SVNExternals();

    /**
     * Adds a new svn:external property value, parses that value and fills the
     * class vector with the individual externals.
     * \param path the path of the folder that has the property
     * \param extValue the value of the svn:external property
     * \param fetchRev if true, the highest 'last commit revision' is searched
     *                 in the target dir (the external directory)
     * \param headRev  the revision the external should be tagged to
     */
    bool Add(const CTSVNPath& path, const std::string& extValue, bool fetchRev, svn_revnum_t headRev = -1);

    /**
     * changes the svn:externals property with the fixed revisions.
     * \param bRemote if false, the externals are changed in the working copy. If true
     *                then \c message, \c headrev, \c origurl and \c tagurl must also be set
     *                to change the properties in the repository.
     * \param message message for remote tag
     * \param headRev
     * \param origUrl
     * \param tagUrl
     */
    bool TagExternals(bool bRemote, const CString& message = CString(), svn_revnum_t headRev = -1, const CTSVNPath& origUrl = CTSVNPath(), const CTSVNPath& tagUrl = CTSVNPath());
    /// returns the svn:externals value for the specified \c path
    std::string GetValue(const CTSVNPath& path) const;
    /// return the error string of the last failed operation
    CString GetLastErrorString() const { return m_sError; }

    /// return a hash with all the externals that are used for tagging to be used in svn_client_copy7
    apr_hash_t* GetHash(bool bLocal, apr_pool_t* pool) const;

    /// returns true if any of the externals are marked to be tagged
    bool NeedsTagging() const;

    /// returns the full url of a (possible) relative external url
    static CString GetFullExternalUrl(const CString& extUrl, const CString& root, const CString& dirUrl);

private:
    CString m_sError;
};