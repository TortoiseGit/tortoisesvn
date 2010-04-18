// Copyright (C) 2007,2010 - TortoiseSVN

// this program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// this program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#include "StdAfx.h"
#include "UnicodeUtils.h"
#include "auto_buffer.h"

char * AnsiToUtf8(const char * pszAnsi, apr_pool_t *pool)
{
    // convert ANSI --> UTF16
    int utf16_count = MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, NULL, 0);
    auto_buffer<WCHAR> pwc(utf16_count);
    MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, pwc, utf16_count);

    // and now from URF16 --> UTF-8
    int utf8_count = WideCharToMultiByte(CP_UTF8, 0, pwc, utf16_count, NULL, 0, NULL, NULL);
    char * pch = (char*) apr_palloc(pool, utf8_count);
    WideCharToMultiByte(CP_UTF8, 0, pwc, utf16_count, pch, utf8_count, NULL, NULL);
    return pch;
}

char * Utf16ToUtf8(const WCHAR *pszUtf16, apr_pool_t *pool)
{
    // from URF16 --> UTF-8
    int utf8_count = WideCharToMultiByte(CP_UTF8, 0, pszUtf16, -1, NULL, 0, NULL, NULL);
    char * pch = (char*) apr_palloc(pool, utf8_count);
    WideCharToMultiByte(CP_UTF8, 0, pszUtf16, -1, pch, utf8_count, NULL, NULL);
    return pch;
}
