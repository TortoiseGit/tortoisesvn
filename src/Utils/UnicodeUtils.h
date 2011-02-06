// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007,2009,2011 - TortoiseSVN

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

#include <string>
#include "tstring.h"


/**
 * \ingroup Utils
 * Class to convert strings from/to UTF8 and UTF16.
 */
class CUnicodeUtils
{
public:
    CUnicodeUtils(void);
    ~CUnicodeUtils(void);

    // highly efficient conversion core routines

    static wchar_t* UTF8ToUTF16 (const char* source, size_t size, wchar_t* target);
    static char* UTF16ToUTF8 (const wchar_t* source, size_t size, char* target);

    // conversion from / to MFC strings

#if defined(_MFC_VER) || defined(CSTRING_AVAILABLE)
    static CStringA GetUTF8(const CStringW& string);
    static CString GetUnicode(const CStringA& string);
    static CString UTF8ToUTF16 (const std::string& string);
#endif

    // conversion from / to STL strings

    static std::string StdGetUTF8(const std::wstring& wide);
    static std::wstring StdGetUnicode(const std::string& utf8);
};

// necessary whenever an explict *A Windows API function needs to be 
// interacted with

std::string WideToMultibyte(const std::wstring& wide);

// resource handling utility

int LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage);
