// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2005 - Stefan Kueng

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

class CProgressDlg;

/**
 * \ingroup TortoiseMerge
 *
 * Helper functions
 */
class CUtils
{
public:
	CUtils(void);
	~CUtils(void);

	/**
	 * Starts an external program to get a file with a specific revision. 
	 * \param sPath path to the file for which a specific revision is fetched
	 * \param sVersion the revision to get
	 * \param sSavePath the path to where the file version shall be saved
	 * \param hWnd the window handle of the calling app
	 * \return TRUE if successful
	 */
	static BOOL GetVersionedFile(CString sPath, CString sVersion, CString sSavePath, CProgressDlg * progDlg, HWND hWnd = NULL);
	static CString GetVersionFromFile(const CString & p_strDateiname);
	static CString GetFileNameFromPath(CString sPath);

};
