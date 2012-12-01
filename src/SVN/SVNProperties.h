// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012 - TortoiseSVN

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

#pragma once

#include "SVNReadProperties.h"
#include "SVNHelpers.h"

/**
 * \ingroup SVN
 * Subversion Properties.
 * Use this class to retrieve, add and remove Subversion properties
 * for single files and directories.
 *
 * A property value is represented in std::string. If a property is a
 * known text property (like svn:* or bugtraq:* or tsvn:*), the value
 * is converted to(from) UTF-8 from(to) the native encoding in this
 * class. Otherwise the value is treated as a raw binary data, and no
 * conversion is performed. This behavior is same as "svn propset" and
 * "svn propget".
 */
class SVNProperties : public SVNReadProperties
{
public:

#ifdef _MFC_VER
    SVNProperties(SVNRev rev, bool bRevProps, bool bIncludeInherited);
    SVNProperties(const CTSVNPath& filepath, SVNRev rev, bool bRevProps, bool bIncludeInherited);
#else
    SVNProperties(bool bRevProps, bool bIncludeInherited);
    /**
     * Constructor. Creates a Subversion properties object for
     * the specified file/directory.
     * \param filepath the file/directory
     */
    SVNProperties(const CTSVNPath& filepath, bool bRevProps, bool bIncludeInherited);
#endif
    virtual ~SVNProperties(void);
    /**
     * Adds a new property to the file/directory specified in the constructor.
     * \remark After using this method the indexes of the properties may change! Call Refresh() if you want to access other properties again.
     * \param Name the name of the new property
     * \param Value the value of the new property
     * \param depth the depth with which the property is added
     * \param message an optional commit message if the property is set directly on the repository
     * \return TRUE if the property is added successfully
     */
    BOOL Add(const std::string& Name, const std::string& Value, bool force = false, svn_depth_t depth = svn_depth_empty, const TCHAR * message = NULL);
    /**
     * Removes an existing property from the file/directory specified in the constructor.
     * \remark After using this method the indexes of the properties may change! Call Refresh() if you want to access other properties again.
     * \param Name the name of the property to delete
     * \param depth the depth with which the property is removed
     * \param message an optional commit message if the property is removed directly from the repository
     * \return TRUE if the property is removed successfully
     */
    BOOL Remove(const std::string& Name, svn_depth_t depth = svn_depth_empty, const TCHAR * message = NULL);

    /**
     * Clear current content and read properties from
     * K / V notation as created by \ref GetSerializedForm.
     * \param text serialized property list
     */
    void SetFromSerializedForm (const std::string& text);

    svn_revnum_t GetCommitRev() const { return rev_set; }
private:
    void PrepareMsgForUrl( const TCHAR * message, SVNPool& subpool );
    static svn_error_t* CommitCallback(const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool);

    svn_revnum_t rev_set;
};
