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
#include "UnicodeStrings.h"
#include <windows.h>
#include <string.h>

std::string WideToMultibyte(wide_string wide)
{
	char narrow[_MAX_PATH * 3];
	BOOL defaultCharUsed;
	int ret = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), wide.size(), narrow, _MAX_PATH*3 - 1, ".", &defaultCharUsed);
	narrow[ret] = 0;
	return std::string(narrow);
}

std::string WideToUTF8(wide_string wide)
{
	char narrow[_MAX_PATH * 3];
	int ret = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), wide.size(), narrow, _MAX_PATH*3 - 1, NULL, NULL);
	narrow[ret] = 0;
	return std::string(narrow);
}

wide_string MultibyteToWide(std::string multibyte)
{
	wchar_t wide[_MAX_PATH * 3];
	int ret = MultiByteToWideChar(CP_ACP, 0, multibyte.c_str(), multibyte.size(), wide, _MAX_PATH*3 - 1);
	wide[ret] = 0;
	return wide_string(wide);
}

wide_string UTF8ToWide(std::string multibyte)
{
	wchar_t wide[_MAX_PATH * 3];
	int ret = MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), multibyte.size(), wide, _MAX_PATH*3 - 1);
	wide[ret] = 0;
	return wide_string(wide);
}
#ifdef UNICODE
stdstring UTF8ToString(std::string string) {return UTF8ToWide(string);}
std::string StringToUTF8(stdstring string) {return WideToUTF8(string);}
#else
stdstring UTF8ToString(std::string string) {return WideToMultibyte(UTF8ToWide(string));}
std::string StringToUTF8(stdstring string) {return WideToUTF8(MultibyteToWide(string));}
#endif


#pragma warning(push)
#pragma warning(disable: 4200)
	struct STRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)	// C4200

int LoadStringEx(HINSTANCE hInstance, UINT uID, LPCTSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
	const STRINGRESOURCEIMAGE* pImage;
	const STRINGRESOURCEIMAGE* pImageEnd;
	ULONG nResourceSize;
	HGLOBAL hGlobal;
	UINT iIndex;
#ifndef UNICODE
	BOOL defaultCharUsed;
#endif
	int ret;

	HRSRC hResource =  FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(((uID>>4)+1)), wLanguage);
	if (!hResource)
	{
		//try the default language before giving up!
		hResource = FindResource(hInstance, MAKEINTRESOURCE(((uID>>4)+1)), RT_STRING);
		if (!hResource)
			return 0;
	}
	hGlobal = LoadResource(hInstance, hResource);
	if (!hGlobal)
		return 0;
	pImage = (const STRINGRESOURCEIMAGE*)::LockResource(hGlobal);
	if(!pImage)
		return 0;

	nResourceSize = ::SizeofResource(hInstance, hResource);
	pImageEnd = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+nResourceSize);
	iIndex = uID&0x000f;

	while ((iIndex > 0) && (pImage < pImageEnd))
	{
		pImage = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+(sizeof(STRINGRESOURCEIMAGE)+(pImage->nLength*sizeof(WCHAR))));
		iIndex--;
	}
	if (pImage >= pImageEnd)
		return 0;
	if (pImage->nLength == 0)
		return 0;
#ifdef UNICODE
	ret = pImage->nLength;
	wcsncpy((wchar_t *)lpBuffer, pImage->achString, pImage->nLength);
	(wchar_t)lpBuffer[ret] = 0;
#else
	ret = WideCharToMultiByte(CP_ACP, 0, pImage->achString, pImage->nLength, (LPSTR)lpBuffer, nBufferMax-1, ".", &defaultCharUsed);
	(TCHAR)lpBuffer[ret] = 0;
#endif
	return ret;
}

