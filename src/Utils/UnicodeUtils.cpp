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

#ifdef UNICODE
CStringW CUnicodeUtils::GetUnicode(CStringA string)
{
	TCHAR buf[MAX_PATH * 4];
	MultiByteToWideChar(CP_UTF8, 0, string, -1, buf, MAX_PATH * 4);
	return CStringW(buf);
}
#endif
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