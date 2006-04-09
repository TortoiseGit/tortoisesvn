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
#include "StdAfx.h"
#include "unicodeutils.h"

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
//	ZeroMemory(buf, (string.GetLength()*4 + 1)*sizeof(char));
	int lengthIncTerminator = WideCharToMultiByte(CP_UTF8, 0, string, -1, buf, len*4, NULL, NULL);
	retVal.ReleaseBuffer(lengthIncTerminator-1);
	return retVal;
}

CStringA CUnicodeUtils::GetUTF8(const CStringA& string)
{
	WCHAR * buf;
	int len = string.GetLength();
	if (len==0)
		return CStringA();
	buf = new WCHAR[len*4 + 1];
	ZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, string, -1, buf, len*4);
	CStringW temp = CStringW(buf);
	delete [] buf;
	return (CUnicodeUtils::GetUTF8(temp));
}

CString CUnicodeUtils::GetUnicode(const CStringA& string)
{
	WCHAR * buf;
	int len = string.GetLength();
	if (len==0)
		return CString();
	buf = new WCHAR[len*4 + 1];
	ZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, string, -1, buf, len*4);
	CString ret = CString(buf);
	delete [] buf;
	return ret;
}

CStringA CUnicodeUtils::ConvertWCHARStringToUTF8(const CString& string)
{
	CStringA sRet;
	char * buf;
	buf = new char[string.GetLength()+1];
	if (buf)
	{
		int i=0;
		for ( ; i<string.GetLength(); ++i)
		{
			buf[i] = (char)string.GetAt(i);
		}
		buf[i] = 0;
		sRet = CStringA(buf);
		delete [] buf;
	}
	return sRet;
}

#endif //_MFC_VER

#ifdef UNICODE
std::string CUnicodeUtils::StdGetUTF8(const wide_string& wide)
{
	int len = (int)wide.size();
	if (len==0)
		return std::string();
	int size = len*4;
	char * narrow = new char[size];
	int ret = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), len, narrow, size-1, NULL, NULL);
	narrow[ret] = 0;
	std::string sRet = std::string(narrow);
	delete narrow;
	return sRet;
}

wide_string CUnicodeUtils::StdGetUnicode(const std::string& multibyte)
{
	int len = (int)multibyte.size();
	if (len==0)
		return wide_string();
	int size = len*4;
	wchar_t * wide = new wchar_t[size];
	int ret = MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), len, wide, size - 1);
	wide[ret] = 0;
	wide_string sRet = wide_string(wide);
	delete wide;
	return sRet;
}
#endif
