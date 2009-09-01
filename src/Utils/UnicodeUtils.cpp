// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2009 - TortoiseSVN

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
#include "StdAfx.h"
#include "unicodeutils.h"
#include "auto_buffer.h"

CUnicodeUtils::CUnicodeUtils(void)
{
}

CUnicodeUtils::~CUnicodeUtils(void)
{
}

#if defined(_MFC_VER) || defined(CSTRING_AVAILABLE)

CStringA CUnicodeUtils::GetUTF8(const CStringW& string)
{
	char * buf;
	CStringA retVal;
	int len = string.GetLength();
	if (len==0)
		return retVal;
	buf = retVal.GetBuffer(len*4 + 1);
//	SecureZeroMemory(buf, (string.GetLength()*4 + 1)*sizeof(char));
	int lengthIncTerminator = WideCharToMultiByte(CP_UTF8, 0, string, -1, buf, len*4, NULL, NULL);
	retVal.ReleaseBuffer(lengthIncTerminator-1);
	return retVal;
}

CStringA CUnicodeUtils::GetUTF8(const CStringA& string)
{
	int len = string.GetLength();
	if (len==0)
		return CStringA();

	auto_buffer<WCHAR> buf (len*4 + 1);
	SecureZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, string, -1, buf, len*4);

    return (CUnicodeUtils::GetUTF8 (CStringW(buf)));
}

CString CUnicodeUtils::GetUnicode(const CStringA& string)
{
	int len = string.GetLength();
	if (len==0)
		return CString();

	auto_buffer<WCHAR> buf (len*4 + 1);
	SecureZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, string, -1, buf, len*4);

    return buf.get();
}

CStringA CUnicodeUtils::ConvertWCHARStringToUTF8(const CString& string)
{
	CStringA sRet;
	auto_buffer<char> buf (string.GetLength()+1);
	if (buf)
	{
		int i=0;
		for ( ; i<string.GetLength(); ++i)
		{
			buf[i] = (char)string.GetAt(i);
		}
		buf[i] = 0;
		sRet = buf.get();
	}

	return sRet;
}

#endif //_MFC_VER

#ifdef UNICODE
std::string CUnicodeUtils::StdGetUTF8(const std::wstring& wide)
{
	int len = (int)wide.size();
	if (len==0)
		return std::string();
	int size = len*4;
	
    auto_buffer<char> narrow (size);
	int ret = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), len, narrow, size-1, NULL, NULL);

    return std::string (narrow.get(), ret);
}

std::wstring CUnicodeUtils::StdGetUnicode(const std::string& multibyte)
{
	int len = (int)multibyte.size();
	if (len==0)
		return std::wstring();
	int size = len*4;

    auto_buffer<wchar_t> wide (size);
	int ret = MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), len, wide, size - 1);

    return std::wstring (wide.get(), ret);
}
#endif

std::string WideToMultibyte(const std::wstring& wide)
{
    auto_buffer<char> narrow (wide.length()*3+2);
	BOOL defaultCharUsed;
	int ret = (int)WideCharToMultiByte(CP_ACP, 0, wide.c_str(), (int)wide.size(), narrow, (int)wide.length()*3 - 1, ".", &defaultCharUsed);

    return std::string (narrow.get(), ret);
}

std::string WideToUTF8(const std::wstring& wide)
{
	auto_buffer<char> narrow (wide.length()*3+2);
	int ret = (int)WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), narrow, (int)wide.length()*3 - 1, NULL, NULL);

    return std::string (narrow.get(), ret);
}

std::wstring MultibyteToWide(const std::string& multibyte)
{
	size_t length = multibyte.length();
	if (length == 0)
		return std::wstring();

	auto_buffer<wchar_t> wide (multibyte.length()*2+2);
	int ret = wide.get() == NULL
        ? 0
        : (int)MultiByteToWideChar(CP_ACP, 0, multibyte.c_str(), (int)multibyte.size(), wide, (int)length*2 - 1);

    return std::wstring (wide.get(), ret);
}

std::wstring UTF8ToWide(const std::string& multibyte)
{
	size_t length = multibyte.length();
	if (length == 0)
		return std::wstring();

	auto_buffer<wchar_t> wide (length*2+2);
	int ret = wide.get() == NULL
        ? 0
        : (int)MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), (int)multibyte.size(), wide, (int)length*2 - 1);

    return std::wstring (wide.get(), ret);
}
#ifdef UNICODE
tstring UTF8ToString(const std::string& string) {return UTF8ToWide(string);}
std::string StringToUTF8(const tstring& string) {return WideToUTF8(string);}
#else
tstring UTF8ToString(const std::string& string) {return WideToMultibyte(UTF8ToWide(string));}
std::string StringToUTF8(const tstring& string) {return WideToUTF8(MultibyteToWide(string));}
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

