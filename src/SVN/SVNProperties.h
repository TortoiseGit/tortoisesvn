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

#pragma once

#ifdef _MFC_VER
#	include "SVNRev.h"
#	include "SVNPrompt.h"
#	include "ShellUpdater.h"
#endif

#include "UnicodeUtils.h"
#include "TSVNPath.h"

/**
 * \ingroup SVN
 * Subversion Properties.
 * Use this class to retrieve, add and remove Subversion properties
 * for files and directories.
 *
 * A property value is represented in std::string. If a property is a
 * known text property (like svn:* or bugtraq:* or tsvn:*), the value
 * is converted to(from) UTF-8 from(to) the native encoding in this
 * class. Otherwise the value is treated as a raw binary data, and no
 * conversion is performed. This behavior is same as "svn propset" and
 * "svn propget".
 */
class SVNProperties
{
public:

#ifdef _MFC_VER
	SVNProperties(const CTSVNPath& filepath, SVNRev rev = SVNRev::REV_WC);
	void SaveAuthentication(BOOL save);
#else
	/**
	 * Constructor. Creates a Subversion properties object for
	 * the specified file/directory.
	 * \param filepath the file/directory
	 */
	SVNProperties(const CTSVNPath& filepath);
#endif
	~SVNProperties(void);

	/**
	 * Returns the number of properties the file/directory has.
	 */
	int GetCount();
	/**
	 * Returns the name of the property.
	 * \param index a zero based index
	 * \return the name of the property
	 */
	stdstring GetItemName(int index);
	/**
	 * Returns the value of the property.
	 * \param index a zero based index
	 * \return the value of the property
	 */
	std::string GetItemValue(int index);
	/**
	 * Checks if the property is an internal Subversion property. Internal
	 * Subversion property names usually begin with 'svn:' and are used
	 * by Subversion itself. Internal Properties are:\n
	 * svn:mime-type	specifies the mime-type of the file. All mime-types except text are treated as binary.\n
	 * svn:ignore		tells Subversion to ignore this file/directory\n
	 * svn:eol-style	the EndOfLine style to use (like CR/LF or LF or ...)\n
	 * svn:keywords		tells Subversion to activate the keyword expansion\n
	 * svn:executable	if the file is executable or not
	 * \param index		a zero based index
	 */
	BOOL IsSVNProperty(int index);
	/**
	 * Adds a new property to the file/directory specified in the constructor.
	 * \remark After using this method the indexes of the properties may change!
	 * \param Name the name of the new property
	 * \param Value the value of the new property
	 * \param recurse TRUE if the property should be added to subdirectories/files as well
	 * \return TRUE if the property is added successfully
	 */
	BOOL Add(const TCHAR * Name, std::string Value, BOOL recurse = false);
	/**
	 * Removes an existing property from the file/directory specified in the constructor.
	 * \remark After using this method the indexes of the properties may change!
	 * \param Name the name of the property to delete
	 * \param recurse TRUE if the property should be deleted from subdirectories/files as well
	 * \return TRUE if the property is removed successfully
	 */
	BOOL Remove(const TCHAR * Name, BOOL recurse = false);

	/**
	 * Checks if the property value is binary or text.
	 * \return true if the property has a binary value.
	 */
	bool IsBinary(int index);
	static bool IsBinary(std::string value);

	/**
	 * Returns the last error message as a CString object.
	 */
	stdstring GetLastErrorMsg();

private:		//methods
	/**
	 * Builds the properties (again) and fills the apr_array_header_t structure.
	 * \return the svn error structure
	 */
	svn_error_t*			Refresh();

	/**
	 * Returns either the property name (name == TRUE) or value (name == FALSE).
	 */
	std::string				GetItem(int index, BOOL name);

private:		//members
	apr_pool_t *				m_pool;				///< memory pool baton
	CTSVNPath					m_path;				///< the path to the file/directory this properties object acts upon
	apr_array_header_t *		m_props;			
	int							m_propCount;		///< number of properties found
	svn_error_t *				m_error;
#ifdef _MFC_VER
	SVNRev						m_rev;
	SVNPrompt					m_prompt;
#endif
	svn_client_ctx_t 			m_ctx;



};
