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

class CTSVNPath;

/**
 * \ingroup TortoiseProc
 * An Utility class with static classes.
 */
class CUtils
{
public:
	CUtils(void);
	~CUtils(void);

	/**
	 * Launches the external merge program if there is one.
	 * \return TRUE if the program could be started
	 */
	static BOOL StartExtMerge(const CTSVNPath& basefile, const CTSVNPath& theirfile, const CTSVNPath& yourfile, const CTSVNPath& mergedfile,
		const CString& basename = CString(), const CString& theirname = CString(), const CString& yourname = CString(), const CString& mergedname = CString(), bool bReadOnly = false);

	/**
	 * Starts the external patch program (currently always TortoiseMerge)
	 */
	static BOOL StartExtPatch(const CTSVNPath& patchfile, const CTSVNPath& dir, 
			const CString& sOriginalDescription = CString(), const CString& sPatchedDescription = CString(), 
			BOOL bReversed = FALSE, BOOL bWait = FALSE);

	/**
	 * Starts the external unified diff viewer (the app associated with *.diff or *.patch files).
	 * If no app is associated with those filetypes, the default text editor is used.
	 */
	static BOOL StartUnifiedDiffViewer(const CTSVNPath& patchfile, BOOL bWait = FALSE);

	/**
	 * Starts the external diff application
	 */
	static BOOL StartExtDiff(const CTSVNPath& file1, const CTSVNPath& file2, 
			const CString& sName1 = CString(), const CString& sName2 = CString(), BOOL bWait = FALSE);

	/**
	 * Launches the standard text viewer/editor application which is associated
	 * with txt files.
	 * \return TRUE if the program could be started.
	 */
	static BOOL StartTextViewer(CString file);

	/**
	 * Replaces escaped sequences with the corresponding characters in a string.
	 */
	static void Unescape(char * psz);

	/**
	 * Replaces non-URI chars with the corresponding escape sequences.
	 */
	static CStringA PathEscape(const CStringA& path);

	/**
	 * Returns the version string from the VERSION resource of a dll or exe.
	 * \param p_strDateiname path to the dll or exe
	 * \return the version string
	 */
	static CString GetVersionFromFile(const CString & p_strDateiname);

	/**
	 * returns the filename of a full path
	 */
	static CString GetFileNameFromPath(CString sPath);

	/**
	 * returns the file extension from a full path
	 */
	static CString GetFileExtFromPath(const CString& sPath);

	/**
	 * Returns the long pathname of a path which may be in 8.3 format.
	 */
	static CString GetLongPathname(const CString& path);

	/**
	 * Copies a file or a folder from \a srcPath to \a destpath, creating
	 * intermediate folders if necessary. If \a force is TRUE, then files
	 * are overwritten if they already exist.
	 * Folders are just created at the new location, no files in them get
	 * copied.
	 */
	static BOOL FileCopy(CString srcPath, CString destPath, BOOL force = TRUE);

	/**
	 * Checks if the given file has a size of less than four, which means
	 * an 'empty' file or just newlines, i.e. an emtpy diff.
	 */
	static BOOL CheckForEmptyDiff(const CTSVNPath& sDiffPath);

	/**
	 * Removes all '&' chars from a string.
	 */
	static void RemoveAccelerators(CString& text);

	/**
	 * Writes an ASCII CString to the clipboard in CF_TEXT format
	 */
	static bool WriteAsciiStringToClipboard(const CStringA& sClipdata, HWND hOwningWnd = NULL);

	/**
	 * Create a font which can is used for log messages, etc
	 */
	static void CreateFontForLogs(CFont& fontToCreate);

	/**
	* Launch an external application (usually the diff viewer)
	*/
	static bool LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup);

	/**
	* Launch the external blame viewer
	*/
	static bool LaunchTortoiseBlame(const CString& sBlameFile, const CString& sLogFile, const CString& sOriginalFile);
	
	/**
	 * Returns the path to the installation folder, in our case the TortoiseSVN/bin folder.
	 * \remark the path returned has a trailing backslash
	 */
	static CString GetAppDirectory();
	
	/**
	 * Returns the path to the installation parent folder, in our case the TortoiseSVN folder.
	 * \remark the path returned has a trailing backslash
	 */
	static CString GetAppParentDirectory();

	/**
	 * Resizes all columns in a list control. Considers also icons in columns
	 * with no text.
	 */
	static void ResizeAllListCtrlCols(CListCtrl * pListCtrl);

	/**
	 * parses a string for a path or url. If no path or url is found,
	 * an empty string is returned.
	 * \remark if more than one path or url is inside the string, only
	 * the first one is returned.
	 */
	static CString ParsePathInString(const CString& Str);
};
