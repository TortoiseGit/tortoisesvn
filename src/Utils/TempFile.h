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
//
#pragma once

#include "TSVNPath.h"

/**
* \ingroup Utils
* This singleton class handles temporary files.
* All tempfiles are deleted at the end of a run of SVN operations
*/
class CTempFiles
{
private:
	CTempFiles(void);
	~CTempFiles(void);
public:
	static CTempFiles& Instance();
	
	/**
	 * Returns a path to a temporary file.
	 * \param bRemoveAtEnd if true, the temp file is removed when this object
	 *                     goes out of scope.
	 * \param path         if set, the temp file will have the same file extension
	 *                     as this path.
	 */
	CTSVNPath		GetTempFilePath(bool bRemoveAtEnd, const CTSVNPath& path = CTSVNPath());

private:

private:
	CTSVNPathList m_TempFileList;
};
