// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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

#include "SVNBase.h"

#ifdef _MFC_VER
#   include "SVNPrompt.h"
#   include "ShellUpdater.h"
#   include "ProgressDlg.h"
#endif

#include "SVNRev.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"

#include <tuple>

/**
 * \ingroup SVN
 * Read access to Subversion Properties.
 * Use this class to retrieve Subversion properties
 * for single files and directories.
 *
 * A property value is represented in std::string. If a property is a
 * known text property (like svn:* or bugtraq:* or tsvn:*), the value
 * is converted to(from) UTF-8 from(to) the native encoding in this
 * class. Otherwise the value is treated as a raw binary data, and no
 * conversion is performed. This behavior is same as "svn propset" and
 * "svn propget".
 */
class SVNReadProperties : public SVNBase
{
private:
    SVNReadProperties(const SVNReadProperties&);
    SVNReadProperties& operator=(SVNReadProperties&);

    /// construction utility

    void Construct();

public:

#ifdef _MFC_VER
    SVNReadProperties(SVNRev rev, bool bRevProps, bool includeInherited);
    SVNReadProperties(const CTSVNPath& filepath, SVNRev rev, bool bRevProps, bool includeInherited);
    SVNReadProperties(const CTSVNPath& filepath, SVNRev pegRev, SVNRev rev, bool suppressUI, bool includeInherited);
    void SetProgressDlg(CProgressDlg * dlg) { m_pProgress = dlg; }
#else
    SVNReadProperties(bool bRevProps, bool includeInherited);
    /**
     * Constructor. Creates a Subversion properties object for
     * the specified file/directory.
     * \param filepath the file/directory
     */
    SVNReadProperties(const CTSVNPath& filepath, bool bRevProps, bool includeInherited);
#endif
    virtual ~SVNReadProperties(void);

    /**
     * Run SVN query again on different path.
     * (session init takes a long time -> reuse it)
     */

    void SetFilePath (const CTSVNPath& filepath);

    /**
     * Returns the number of properties the file/directory has.
     */
    int GetCount() const;
    /**
     * Returns the name of the property.
     * \param index a zero based index
     * \return the name of the property
     */
    std::string GetItemName(int index) const;
    /**
     * Returns the value of the property.
     * \param index a zero based index
     * \return the value of the property
     */
    std::string GetItemValue(int index) const;
    /**
     * Checks if the property is an internal Subversion property. Internal
     * Subversion property names usually begin with 'svn:' and are used
     * by Subversion itself. Internal Properties are:\n
     * svn:mime-type    specifies the mime-type of the file. All mime-types except text are treated as binary.\n
     * svn:ignore       tells Subversion to ignore this file/directory\n
     * svn:eol-style    the EndOfLine style to use (like CR/LF or LF or ...)\n
     * svn:keywords     tells Subversion to activate the keyword expansion\n
     * svn:executable   if the file is executable or not
     * \param index     a zero based index
     */
    BOOL IsSVNProperty(int index) const;
    /**
     * Checks if te property is a "folder only" property, i.e., a property that isn't
     * allowed to be set on files, only on folders.
     * \param name the name of the property, e.g., "svn:externals"
     */
    bool IsFolderOnlyProperty(const std::string& name) const;
    /**
     * Checks if the property value is binary or text.
     * \return true if the property has a binary value.
     */
    bool IsBinary(int index) const;
    static bool IsBinary(const std::string& value);

    /**
     * Returns the index of the property.
     * \param name name of the property to find
     * \return index of the property. -1, if not found
     */
    int IndexOf (const std::string& name) const;

    /**
     * Test, whether a property exists.
     * \param name name of the property to find
     * \return true, if property has been found
     */
    bool HasProperty (const std::string& name) const;

    /**
     * Create a string containing all properties in
     * K / V notation suitable to be written in a file.
     * Formatting follows RFC 822 - the usual key /
     * value notation used throughout subversion:
     * https://svn.apache.org/repos/asf/subversion/trunk/notes/dump-load-format.txt
     * \return serialized text
     */
    std::string GetSerializedForm() const;

    /**
     * Adds a property name to the list of properties that can only be set on folders
     */
    void AddFolderPropName(const std::string& p) {m_folderprops.push_back(p);}

    /**
     * Returns the object with all inherited properties.
     */
    std::vector<std::tuple<std::string, std::map<std::string,std::string>>> GetInheritedProperties() const { return m_inheritedProperties; }

    CTSVNPath GetPath() const { return m_path; }

    /**
     * Returns the revision. If a commit has happened, this is the new head revision
     */
    SVNRev GetRevision() const { return m_rev; }

    /**
     * Set to true to cancel operations
     */
    bool * m_bCancelled;
private:        //methods
    /**
     * Builds the properties (again) and fills the apr_array_header_t structure.
     * \return the svn error structure
     */
    svn_error_t*            Refresh();

    /**
     * Returns either the property name (name == TRUE) or value (name == FALSE).
     */
    std::string             GetItem(int index, BOOL name) const;

protected:        //members
    apr_pool_t *                m_pool;             ///< memory pool baton
    CTSVNPath                   m_path;             ///< the path to the file/directory this properties object acts upon
    apr_hash_t *                m_props;
    apr_array_header_t *        m_inheritedprops;
    std::vector<std::tuple<std::string, std::map<std::string,std::string>>>  m_inheritedProperties;
    int                         m_propCount;        ///< number of properties found
    SVNRev                      m_rev;
    bool                        m_bRevProps;
    bool                        m_includeInherited;
    std::vector<std::string>    m_folderprops;
#ifdef _MFC_VER
    SVNPrompt                   m_prompt;
    CProgressDlg *              m_pProgress;
#endif

private:        //members
    SVNRev                      m_peg_rev;

private:
    static svn_error_t *        proplist_receiver(void *baton, const char *path, apr_hash_t *prop_hash, apr_array_header_t *inherited_props, apr_pool_t *pool);

};
