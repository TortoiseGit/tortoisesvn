#pragma once
#include <string>
#include <atlstr.h>

#pragma warning (push,1)
typedef std::basic_string<wchar_t> wide_string;
#ifdef UNICODE
#define stdstring wide_string
#else
#define stdstring std::string
#endif
#pragma warning (pop)

class CUnicodeUtils
{
public:
	CUnicodeUtils(void);
	~CUnicodeUtils(void);
#ifdef _MFC_VER
#ifdef UNICODE
	static CStringA GetUTF8(CString string);
	static CStringW GetUnicode(CStringA string);
#else
	static CStringA GetUTF8(CString string);
	static CStringA GetUnicode(CStringA string) {return string;}
#endif
#endif
#ifdef UNICODE
	static std::string StdGetUTF8(wide_string wide);
	static wide_string StdGetUnicode(std::string multibyte);
#else
	static std::string StdGetUTF8(std::string str) {return str;}
	static std::string StdGetUnicode(std::string multibyte) {return multibyte;}
#endif
};
