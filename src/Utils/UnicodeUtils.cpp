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
//
#include "StdAfx.h"
#include "unicodeutils.h"

CUnicodeUtils::CUnicodeUtils(void)
{
}

CUnicodeUtils::~CUnicodeUtils(void)
{
}
#ifdef _MFC_VER

CStringA CUnicodeUtils::GetUTF8(CString string)
{
#ifdef UNICODE
	char buf[MAX_PATH * 4];
	WideCharToMultiByte(CP_UTF8, 0, string, -1, buf, MAX_PATH * 4, NULL, NULL);
	return CStringA(buf);
#else
	return string;
#endif
}

CString CUnicodeUtils::GetUnicode(CStringA string)
{
	WCHAR buf[MAX_PATH * 4];
	MultiByteToWideChar(CP_UTF8, 0, string, -1, buf, MAX_PATH * 4);
	return CString(buf);
}
#endif //_MFC_VER

#ifdef UNICODE
std::string CUnicodeUtils::StdGetUTF8(wide_string wide)
{
	char narrow[_MAX_PATH * 3];
	int ret = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), narrow, _MAX_PATH*3 - 1, NULL, NULL);
	narrow[ret] = 0;
	return std::string(narrow);
}

wide_string CUnicodeUtils::StdGetUnicode(std::string multibyte)
{
	wchar_t wide[_MAX_PATH * 3];
	int ret = MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), (int)multibyte.size(), wide, _MAX_PATH*3 - 1);
	wide[ret] = 0;
	return wide_string(wide);
}
#endif