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
#include "stdafx.h"
#include "UnicodeStrings.h"

std::string WideToMultibyte(const wide_string& wide)
{
	char * narrow = new char[wide.length()*3+2];
	BOOL defaultCharUsed;
	int ret = (int)WideCharToMultiByte(CP_ACP, 0, wide.c_str(), (int)wide.size(), narrow, (int)wide.length()*3 - 1, ".", &defaultCharUsed);
	narrow[ret] = 0;
	std::string str = narrow;
	delete[] narrow;
	return str;
}

std::string WideToUTF8(const wide_string& wide)
{
	char * narrow = new char[wide.length()*3+2];
	int ret = (int)WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), narrow, (int)wide.length()*3 - 1, NULL, NULL);
	narrow[ret] = 0;
	std::string str = narrow;
	delete[] narrow;
	return str;
}

wide_string MultibyteToWide(const std::string& multibyte)
{
	size_t length = multibyte.length();
	if (length == 0)
		return wide_string();

	wchar_t * wide = new wchar_t[multibyte.length()*2+2];
	if (wide == NULL)
		return wide_string();
	int ret = (int)MultiByteToWideChar(CP_ACP, 0, multibyte.c_str(), (int)multibyte.size(), wide, (int)length*2 - 1);
	wide[ret] = 0;
	wide_string str = wide;
	delete[] wide;
	return str;
}

wide_string UTF8ToWide(const std::string& multibyte)
{
	size_t length = multibyte.length();
	if (length == 0)
		return wide_string();
		
	wchar_t * wide = new wchar_t[length*2+2];
	if (wide == NULL)
		return wide_string();
	int ret = (int)MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), (int)multibyte.size(), wide, (int)length*2 - 1);
	wide[ret] = 0;
	wide_string str = wide;
	delete[] wide;
	return str;
}
#ifdef UNICODE
stdstring UTF8ToString(const std::string& string) {return UTF8ToWide(string);}
std::string StringToUTF8(const stdstring& string) {return WideToUTF8(string);}
#else
stdstring UTF8ToString(const std::string& string) {return WideToMultibyte(UTF8ToWide(string));}
std::string StringToUTF8(const stdstring& string) {return WideToUTF8(MultibyteToWide(string));}
#endif


#pragma warning(push)
#pragma warning(disable: 4200)
	struct STRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)	// C4200

int LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage)
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

	if (lpBuffer == NULL)
		return 0;
	lpBuffer[0] = 0;
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
	if (ret > nBufferMax)
		ret = nBufferMax;
	wcsncpy_s((wchar_t *)lpBuffer, nBufferMax, pImage->achString, ret);
	lpBuffer[ret] = 0;
#else
	ret = WideCharToMultiByte(CP_ACP, 0, pImage->achString, pImage->nLength, (LPSTR)lpBuffer, nBufferMax-1, ".", &defaultCharUsed);
	lpBuffer[ret] = 0;
#endif
	return ret;
}

