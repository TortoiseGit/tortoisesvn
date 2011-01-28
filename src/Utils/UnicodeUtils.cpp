// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2008-2011 - TortoiseSVN

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

static BOOL sse2supported = ::IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE );

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
    int lengthIncTerminator = WideCharToMultiByte(CP_UTF8, 0, string, -1, buf, len*4, NULL, NULL);
    retVal.ReleaseBuffer(lengthIncTerminator == 0 ? 0 : lengthIncTerminator-1);
    return retVal;
}

CStringA CUnicodeUtils::GetUTF8(const CStringA& string)
{
    int len = string.GetLength();
    if (len==0)
        return CStringA();

    auto_buffer<WCHAR> buf (len*4 + 1);
    int ret = MultiByteToWideChar(CP_ACP, 0, string, -1, buf, len*4);
    if (ret == 0)
        return CStringA();
    return (CUnicodeUtils::GetUTF8 (CStringW(buf)));
}

CString CUnicodeUtils::GetUnicode(const CStringA& string)
{
    int len = string.GetLength();
    if (len==0)
        return CString();

    auto_buffer<WCHAR> buf (len*4 + 1);
    int ret = MultiByteToWideChar(CP_UTF8, 0, string, -1, buf, len*4);
    if (ret == 0)
        return CString();

    return buf.get();
}

wchar_t* CUnicodeUtils::UTF8ToUTF16
    (const char* source, size_t size, wchar_t* target)
{
    // most of our strings will be longer, plain ASCII strings
    // -> affort some minor overhead to handle them very fast

    if (sse2supported)
    {
        __m128i zero = _mm_setzero_si128();
        __m128i ascii_limit = _mm_set_epi8 ( 0x7f,  0x7f,  0x7f,  0x7f
            , 0x7f,  0x7f,  0x7f,  0x7f
            , 0x7f,  0x7f,  0x7f,  0x7f
            , 0x7f,  0x7f,  0x7f,  0x7f);

        for (
            ; size >= sizeof(zero)
            ; size -= sizeof(zero), source += sizeof(zero), target += sizeof(zero))
        {
            // fetch the next 16 bytes from the source

            __m128i chunk = _mm_loadu_si128 ((const __m128i*)source);

            // check for non-ASCII

            int zero_flags = _mm_movemask_epi8 (_mm_cmpeq_epi8 (chunk, zero));
            int ascii_flags = _mm_movemask_epi8 (_mm_cmpgt_epi8 (chunk, ascii_limit));
            if (ascii_flags != 0)
                break;

            // all 16 bytes in chunk are ASCII.
            // Since we have not exceeded SIZE, we can savely write all data,
            // even if it should include a terminal 0.

            _mm_storeu_si128 ((__m128i*)target, _mm_unpacklo_epi8 (chunk, zero));
            _mm_storeu_si128 ((__m128i*)(target+8), _mm_unpackhi_epi8 (chunk, zero));

            // end of string?

            if (zero_flags != 0)
            {
                // terminator has already been written, we only need to update 
                // the target pointer

                if ((zero_flags & 0xff) == 0)
                {
                    zero_flags >>= 8;
                    target += 8;
                }

                while ((zero_flags & 1) == 0)
                {
                    zero_flags >>= 1;
                    ++target;
                }

                // done

                return target;
            }
        }
    }

    // non-ASCII and string tail handling

    const unsigned char* s = reinterpret_cast<const unsigned char*>(source);
    if (size > 0)
    {
        unsigned c;
        do
        {
            if (--size == 0)
            {
                *target = 0;
                ++target;
                break;
            }

            c = *s;

            // decode 1 .. 4 byte sequences

            if (c < 0x80)
            {
                ++s;
            }
            else if (c < 0xE0)
            {
                c = ((c & 0x1f) << 6) + (s[1] & 0x3f);
                s += 2;
            }
            else if (c < 0xF0)
            {
                c =   ((c & 0x1f) << 12)
                    + (static_cast<unsigned>(s[1] & 0x3f) << 6)
                    + (s[2] & 0x3f);
                s += 3;
            }
            else
            {
                c =   ((c & 0x1f) << 12)
                    + (static_cast<unsigned>(s[1] & 0x3f) << 6)
                    + (static_cast<unsigned>(s[2] & 0x3f) << 6)
                    + (s[3] & 0x3f);
                s += 4;
            }

            // write result (1 or 2 wchars)

            if (c < 0x10000)
            {
                *target = static_cast<wchar_t>(c);
                ++target;
            }
            else
            {
                c -= 0x10000;
                *target = static_cast<wchar_t>((c >> 10) + 0xd800);
                *(target+1) = static_cast<wchar_t>((c & 0x3ff) + 0xdc00);
                target += 2;
            }
        }
        while (c != 0);
    }
    else
    {
        *target = 0;
        ++target;
    }

    return target-1;
}

CString CUnicodeUtils::UTF8ToUTF16 (const std::string& string)
{
    enum {FIXED_BUFFER_SIZE = 1024};

    wchar_t fixedBuffer[FIXED_BUFFER_SIZE];
    auto_buffer<wchar_t> dynamicBuffer;

    wchar_t* result = NULL;

    size_t size = string.length()+1;
    if (size <= FIXED_BUFFER_SIZE)
    {
        result = fixedBuffer;
    }
    else
    {
        dynamicBuffer.reset (size);
        result = dynamicBuffer;
    }

    wchar_t* target = UTF8ToUTF16 (string.c_str(), size, result);
    return CString (result, static_cast<int>(target - result));
}

void CUnicodeUtils::UTF8ToUTF16 (const std::string& source, std::wstring& target)
{
    size_t size = source.size();
    target.resize (size);
    wchar_t* buffer = &target[0];
    target.resize (UTF8ToUTF16 (source.c_str(), size+1, buffer) - buffer);
}

CStringA CUnicodeUtils::ConvertWCHARStringToUTF8(const CString& string)
{
    auto_buffer<char> buf (string.GetLength()+1);

    int i=0;
    for ( ; i<string.GetLength(); ++i)
    {
        buf[i] = (char)string.GetAt(i);
    }
    buf[i] = 0;
    CStringA sRet = buf.get();
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
#pragma warning(pop)    // C4200

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