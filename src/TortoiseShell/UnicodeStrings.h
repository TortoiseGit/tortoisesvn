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

#pragma warning (push,1)
typedef std::wstring wide_string;
#ifndef stdstring
#	ifdef UNICODE
#		define stdstring wide_string
#	else
#		define stdstring std::string
#	endif
#endif
#pragma warning (pop)
// String resource helpers


std::string WideToMultibyte(const wide_string& wide);
std::string WideToUTF8(const wide_string& wide);
wide_string MultibyteToWide(const std::string& multibyte);
wide_string UTF8ToWide(const std::string& multibyte);

#ifdef UNICODE
stdstring UTF8ToString(const std::string& string); 
std::string StringToUTF8(const stdstring& string); 
#else
stdstring UTF8ToString(const std::string& string); 
std::string StringToUTF8(const stdstring& string);
#endif

int LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage);
