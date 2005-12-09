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

#pragma once
#include "SVNGlobal.h"
/**
 * \ingroup SVN
 * A small wrapper for the Subversion configs
 *
 * \par requirements
 * MFC\n
 *
 * \date 12-16-2004
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class SVNConfig
{
public:
	SVNConfig(void);
	~SVNConfig(void);

	/**
	 * Returns an array of ignore patterns which then can be passed to
	 * MatchIgnorePattern().
	 * \param ppPatterns the returned array
	 * \return TRUE if the function is successful
	 */
	BOOL GetDefaultIgnores(apr_array_header_t** ppPatterns);

	/**
	 * Checks if the \c sFilepath matches a pattern in the array of
	 * ignore patterns.
	 * \param sFilepath the path to check 
	 * \param *patterns the array of ignore patterns. Get this array with GetDefaultIgnores()
	 * \return TRUE if the filename matches a pattern, FALSE if it doesn't.
	 */
	static BOOL MatchIgnorePattern(const CString& sFilepath, apr_array_header_t *patterns);
private:
	apr_pool_t *				parentpool;
	apr_pool_t *				pool;			///< memory pool
	svn_client_ctx_t 			ctx;

};
