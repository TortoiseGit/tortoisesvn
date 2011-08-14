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

char* CUnicodeUtils::UTF16ToUTF8
    (const wchar_t* source, size_t size, char* target)
{
    // most of our strings will be longer, plain ASCII strings
    // -> affort some minor overhead to handle them very fast

    if (sse2supported)
    {
        __m128i zero = _mm_setzero_si128();
        for (
            ; size >= sizeof(zero)
            ; size -= sizeof(zero), source += sizeof(zero), target += sizeof(zero))
        {
            // fetch the next 16 words from the source

            __m128i chunk0 = _mm_loadu_si128 ((const __m128i*)source);
            __m128i chunk1 = _mm_loadu_si128 ((const __m128i*)(source + 8));

            // convert to 8-bit chars. Values > 0xff will be mapped to 0xff

            __m128i packedChunk = _mm_packus_epi16 (chunk0, chunk1);

            // check for non-ASCII (SSE2 cmp* operations are signed!)

            int ascii_flags = _mm_movemask_epi8 (_mm_cmplt_epi8 (packedChunk, zero));
            if (ascii_flags != 0)
                break;

            // calculate end of string condition

            int zero_flags = _mm_movemask_epi8 (_mm_cmpeq_epi16 (packedChunk, zero));

            // all 16 chars are ASCII.
            // Since we have not exceeded SIZE, we can savely write all data,
            // even if it should include a terminal 0.

            _mm_storeu_si128 ((__m128i*)target, packedChunk);

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

    const unsigned short* s = reinterpret_cast<const unsigned short*>(source);
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

            // decode 1 and 2 word sequences

            if ((c < 0xd800) || (c > 0xe000))
            {
                ++s;
            }
            else
            {
                c = ((c & 0x3ff) << 10) + (s[1] & 0x3ff) + 0x10000;
                s += 2;
            }

            // write result (1 to 4 bytes)

            if (c < 0x80)
            {
                *target = static_cast<char>(c);
                ++target;
            }
            else if (c < 0x7ff)
            {
                *target = static_cast<char>((c >> 6) + 0xC0);
                *(target+1) = static_cast<char>((c & 0x3f) + 0x80);
                target += 2;
            }
            else if (c < 0xffff)
            {
                *target = static_cast<char>((c >> 12) + 0xE0);
                *(target+1) = static_cast<char>(((c >> 6) & 0x3f) + 0x80);
                *(target+2) = static_cast<char>((c & 0x3f) + 0x80);
                target += 3;
            }
            else
            {
                *target = static_cast<char>((c >> 18) + 0xF0);
                *(target+1) = static_cast<char>(((c >> 12) & 0x3f) + 0x80);
                *(target+2) = static_cast<char>(((c >> 6) & 0x3f) + 0x80);
                *(target+3) = static_cast<char>((c & 0x3f) + 0x80);
                target += 4;
            }
        }
        while (c != 0);
    }
    else
    {
        *target = 0;
    }

    return target-1;
}

wchar_t* CUnicodeUtils::UTF8ToUTF16
    (const char* source, size_t size, wchar_t* target)
{
    // most of our strings will be longer, plain ASCII strings
    // -> affort some minor overhead to handle them very fast

    if (sse2supported)
    {
        __m128i zero = _mm_setzero_si128();
        for (
            ; size >= sizeof(zero)
            ; size -= sizeof(zero), source += sizeof(zero), target += sizeof(zero))
        {
            // fetch the next 16 bytes from the source

            __m128i chunk = _mm_loadu_si128 ((const __m128i*)source);

            // check for non-ASCII (SSE2 cmp* operations are signed!)

            int zero_flags = _mm_movemask_epi8 (_mm_cmpeq_epi8 (chunk, zero));
            int ascii_flags = _mm_movemask_epi8 (_mm_cmplt_epi8 (chunk, zero));
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
    }

    return target-1;
}

namespace
{
    // simple utility class that provides an efficient
    // writable string buffer. std::basic_string<> could
    // be used as well but has a less suitable interface.

    template<class T> class CBuffer
    {
    private:

        enum {FIXED_BUFFER_SIZE = 1024};

        T fixedBuffer[FIXED_BUFFER_SIZE];
        auto_buffer<T> dynamicBuffer;

        T* buffer;

    public:

        CBuffer (size_t minCapacity)
        {
            if (minCapacity <= FIXED_BUFFER_SIZE)
            {
                buffer = fixedBuffer;
            }
            else
            {
                dynamicBuffer.reset (minCapacity);
                buffer = dynamicBuffer;
            }
        }

        operator T*()
        {
            return buffer;
        }
    };

}

// wrap core routines and have string objects in the signatures
// instead of string buffers

#if defined(_MFC_VER) || defined(CSTRING_AVAILABLE)

CStringA CUnicodeUtils::GetUTF8(const CStringW& string)
{
    size_t size = string.GetLength()+1;
    CBuffer<char> buffer (3 * size);

    char* end = UTF16ToUTF8 ((const wchar_t*)string, size, buffer);
    return CStringA (buffer, static_cast<int>(end - buffer));
}

CString CUnicodeUtils::GetUnicode(const CStringA& string)
{
    size_t size = string.GetLength()+1;
    CBuffer<wchar_t> buffer (size);

    wchar_t* end = UTF8ToUTF16 ((const char*)string, size, buffer);
    return CString (buffer, static_cast<int>(end - buffer));
}

CString CUnicodeUtils::UTF8ToUTF16 (const std::string& string)
{
    size_t size = string.length()+1;
    CBuffer<wchar_t> buffer (size);

    wchar_t* end = UTF8ToUTF16 (string.c_str(), size, buffer);
    return CString (buffer, static_cast<int>(end - buffer));
}
#endif //_MFC_VER

std::string CUnicodeUtils::StdGetUTF8(const std::wstring& wide)
{
    size_t size = wide.length()+1;
    CBuffer<char> buffer (3 * size);

    char* end = UTF16ToUTF8 (wide.c_str(), size, buffer);
    return std::string (buffer, end - buffer);
}

std::wstring CUnicodeUtils::StdGetUnicode(const std::string& utf8)
{
    size_t size = utf8.length()+1;
    CBuffer<wchar_t> buffer (size);

    wchar_t* end = UTF8ToUTF16 (utf8.c_str(), size, buffer);
    return std::wstring (buffer, end - buffer);
}

// use system function for string conversion

std::string WideToMultibyte(const std::wstring& wide)
{
    auto_buffer<char> narrow (wide.length()*3+2);
    BOOL defaultCharUsed;
    int ret = (int)WideCharToMultiByte(CP_ACP, 0, wide.c_str(), (int)wide.size(), narrow, (int)wide.length()*3 - 1, ".", &defaultCharUsed);

    return std::string (narrow.get(), ret);
}

// load a string resource

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