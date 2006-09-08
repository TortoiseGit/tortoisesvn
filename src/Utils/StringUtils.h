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

#ifdef UNICODE
#define _tcswildcmp	wcswildcmp
#else
#define _tcswildcmp strwildcmp
#endif

/**
 * Performs a wildcard compare of two strings.
 * \param wild the wildcard string
 * \param string the string to compare the wildcard to
 * \return TRUE if the wildcard matches the string, 0 otherwise
 * \par example
 * \code 
 * if (strwildcmp("bl?hblah.*", "bliblah.jpeg"))
 *  printf("success\n");
 * else
 *  printf("not found\n");
 * if (strwildcmp("bl?hblah.*", "blabblah.jpeg"))
 *  printf("success\n");
 * else
 *  printf("not found\n");
 * \endcode
 * The output of the above code would be:
 * \code
 * success
 * not found
 * \endcode
 */
int strwildcmp(const char * wild, const char * string);
int wcswildcmp(const wchar_t * wild, const wchar_t * string);


class CStringUtils
{
public:
#ifdef _MFC_VER
	static BOOL WildCardMatch(const CString& wildcard, const CString& string);
	static CString LinesWrap(const CString& longstring, int limit = 80, bool bCompactPaths = true);
	static CString WordWrap(const CString& longstring, int limit = 80, bool bCompactPaths = true);

	/**
	 * Removes all '&' chars from a string.
	 */
	static void RemoveAccelerators(CString& text);

	/**
	 * Writes an ASCII CString to the clipboard in CF_TEXT format
	 */
	static bool WriteAsciiStringToClipboard(const CStringA& sClipdata, HWND hOwningWnd = NULL);

	/**
	* Writes an ASCII CString to the clipboard in TSVN_UNIFIEDDIFF format, which is basically the patchfile
	* as a ASCII string.
	*/
	static bool WriteDiffToClipboard(const CStringA& sClipdata, HWND hOwningWnd = NULL);

#endif

	/**
	 * Compares strings while trying to parse numbers in it too.
	 * This function can be used to sort numerically.
	 * For example, strings would be sorted like this:
	 * Version_1.0.3
	 * Version_2.0.4
	 * Version_10.0.2
	 * If a normal text like comparison is used for sorting, the Version_10.0.2
	 * would not be the last in the above example.
	 */
	static int CompareNumerical(LPCTSTR str1, LPCTSTR str2);

};

