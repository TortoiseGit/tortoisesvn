// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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

/**
 * \ingroup TortoiseProc
 * Checks for files with endings specified in the registry as temporary
 * or generated. The registry key is HKCU\\Software\\TortoiseSVN\\TempFileExtensions
 *
 * \par requirements
 * win95 or later\n
 * winNT4 or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 03-06-2003
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo 
 *
 * \bug 
 *
 */
class CCheckTempFiles
{
public:
	CCheckTempFiles(void);
	~CCheckTempFiles(void);

	/**
	 * Checks if there are files in a directory with an extension marked as temporary,
	 * i.e. looks for temporary files. Then Shows a dialog to ask the user if (s)he
	 * wants to delete them.
	 * \param dirName the directory to scan for
	 * \param recurse TRUE if the directory should be searched recursively
	 * \param includeDirs TRUE if directories itself should be checked
	 * \return the number of found temporary files
	 */
	static int Check(CString dirName, const BOOL recurse, const BOOL includeDirs);
	/**
	 * Checks if the given file has an extension marked as temporary
	 * \param filename the file to check
	 */
	static BOOL IsTemp(CString filename);
	/**
	 * Checks if the \em folder is part of the \em path.
	 */
	static BOOL FolderMatch(CString folder, CString path);
};
