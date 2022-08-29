// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2022 - TortoiseSVN

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
#include "stdafx.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "SmartHandle.h"
#ifndef _M_ARM64
#include <emmintrin.h>
#endif
#include <memory>
#include <set>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <shlobj.h>
#pragma warning(pop)

#ifdef CSTRING_AVAILABLE
#    include "SVNHelpers.h"
#    include "apr_uri.h"
#    include "svn_path.h"
#endif

static BOOL sse2Supported = ::IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE);

BOOL CPathUtils::MakeSureDirectoryPathExists(LPCWSTR path)
{
    const size_t        len             = wcslen(path);
    const size_t        fullLen         = len + 10;
    auto                buf             = std::make_unique<wchar_t[]>(fullLen);
    auto                internalPathBuf = std::make_unique<wchar_t[]>(fullLen);
    wchar_t*            pPath           = internalPathBuf.get();
    SECURITY_ATTRIBUTES attribs         = {0};

    attribs.nLength        = sizeof(SECURITY_ATTRIBUTES);
    attribs.bInheritHandle = FALSE;

    ConvertToBackslash(internalPathBuf.get(), path, fullLen);
    if (wcsncmp(internalPathBuf.get(), L"\\\\?\\", 4) == 0)
        pPath += 4;
    do
    {
        SecureZeroMemory(buf.get(), fullLen * sizeof(wchar_t));
        wchar_t* slashPos = wcschr(pPath, '\\');
        if (slashPos)
            wcsncpy_s(buf.get(), fullLen, internalPathBuf.get(), slashPos - internalPathBuf.get());
        else
            wcsncpy_s(buf.get(), fullLen, internalPathBuf.get(), fullLen);
        CreateDirectory(buf.get(), &attribs);
        pPath = wcschr(pPath, '\\');
    } while ((pPath++) && (wcschr(pPath, '\\')));

    const BOOL bRet = CreateDirectory(internalPathBuf.get(), &attribs);
    return bRet;
}

bool CPathUtils::ContainsEscapedChars(const char* psz, size_t length)
{
    // most of our strings will be tens of bytes long
    // -> afford some minor overhead to handle the main part very fast

    const char* end = psz + length;
#ifndef _M_ARM64
    if (sse2Supported)
    {
        __m128i mask = _mm_set_epi8('%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%', '%');

        for (; psz + sizeof(mask) <= end; psz += sizeof(mask))
        {
            // fetch the next 16 bytes from the source

            __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(psz));

            // check for non-ASCII

            int flags = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, mask));
            if (flags != 0)
                return true;
        };
    }
#endif

    // return odd bytes at the end of the string

    for (; psz < end; ++psz)
        if (*psz == '%')
            return true;

    return false;
}

char* CPathUtils::Unescape(char* psz)
{
    char* pszSource = psz;
    char* pszDest   = psz;

    static const char szHex[] = "0123456789ABCDEF";

    // Unescape special characters. The number of characters
    // in the *pszDest is assumed to be <= the number of characters
    // in pszSource (they are both the same string anyway)

    while (*pszSource != '\0' && *pszDest != '\0')
    {
        if (*pszSource == '%')
        {
            // The next two chars following '%' should be digits
            if (*(pszSource + 1) == '\0' ||
                *(pszSource + 2) == '\0')
            {
                // nothing left to do
                break;
            }

            char        nValue  = '?';
            const char* pszHigh = nullptr;
            pszSource++;

            *pszSource = static_cast<char>(toupper(*pszSource));
            pszHigh    = strchr(szHex, *pszSource);

            if (pszHigh != nullptr)
            {
                pszSource++;
                *pszSource         = static_cast<char>(toupper(*pszSource));
                const char* pszLow = strchr(szHex, *pszSource);

                if (pszLow != nullptr)
                {
                    nValue = static_cast<char>(((pszHigh - szHex) << 4) +
                                               (pszLow - szHex));
                }
            }
            else
            {
                pszSource--;
                nValue = *pszSource;
            }
            *pszDest++ = nValue;
        }
        else
            *pszDest++ = *pszSource;

        pszSource++;
    }

    *pszDest = '\0';
    return pszDest;
}

[[maybe_unused]] static const char iri_escape_chars[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const char uri_autoescape_chars[256] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    1,
    0,
    0,

    /* 64 */
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    1,
    0,

    /* 128 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    /* 192 */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

void CPathUtils::ConvertToBackslash(LPWSTR dest, LPCWSTR src, size_t len)
{
    wcscpy_s(dest, len, src);
    auto* p = dest;
    for (; *p != '\0'; ++p)
        if (*p == '/')
            *p = '\\';
}
std::wstring CPathUtils::GetLongPathname(LPCWSTR path)
{
    if (path == nullptr)
        return {};
    return CPathUtils::GetLongPathname(std::wstring(path));
}
std::wstring CPathUtils::GetLongPathname(const std::wstring& path)
{
    if (path.empty())
        return path;
    wchar_t pathBufCanonicalized[MAX_PATH] = {0}; // MAX_PATH ok.
    DWORD   ret                            = 0;
    if (!PathIsURL(path.c_str()) && PathIsRelative(path.c_str()))
    {
        ret = GetFullPathName(path.c_str(), 0, nullptr, nullptr);
        if (ret)
        {
            auto pathbuf = std::make_unique<wchar_t[]>(ret + 1);
            if ((ret = GetFullPathName(path.c_str(), ret, pathbuf.get(), nullptr)) != 0)
                return std::wstring(pathbuf.get(), ret);
        }
    }
    else if (PathCanonicalize(pathBufCanonicalized, path.c_str()))
    {
        ret = ::GetLongPathName(pathBufCanonicalized, nullptr, 0);
        if (ret == 0)
            return path;
        auto pathbuf = std::make_unique<wchar_t[]>(ret + 2);
        ret          = ::GetLongPathName(pathBufCanonicalized, pathbuf.get(), ret + 1);
        // GetFullPathName() sometimes returns the full path with the wrong
        // case. This is not a problem on Windows since its file system is
        // case-insensitive. But for SVN that's a problem if the wrong case
        // is inside a working copy: the svn wc database is case sensitive.
        // To fix the casing of the path, we use a trick:
        // convert the path to its short form, then back to its long form.
        // That will fix the wrong casing of the path.
        int shortret = ::GetShortPathName(pathbuf.get(), nullptr, 0);
        if (shortret)
        {
            auto shortpath = std::make_unique<wchar_t[]>(shortret + 2);
            if (::GetShortPathName(pathbuf.get(), shortpath.get(), shortret + 1))
            {
                int ret2 = ::GetLongPathName(shortpath.get(), pathbuf.get(), ret + 1);
                if (ret2)
                    return std::wstring(pathbuf.get(), ret2);
            }
        }
    }
    else
    {
        ret = ::GetLongPathName(path.c_str(), nullptr, 0);
        if (ret == 0)
            return path;
        auto pathbuf = std::make_unique<wchar_t[]>(ret + 2LL);
        ret          = ::GetLongPathName(path.c_str(), pathbuf.get(), ret + 1);
        // fix the wrong casing of the path. See above for details.
        int shortret = ::GetShortPathName(pathbuf.get(), nullptr, 0);
        if (shortret)
        {
            auto shortpath = std::make_unique<wchar_t[]>(shortret + 2LL);
            if (::GetShortPathName(pathbuf.get(), shortpath.get(), shortret + 1))
            {
                int ret2 = ::GetLongPathName(shortpath.get(), pathbuf.get(), ret + 1);
                if (ret2)
                    return std::wstring(pathbuf.get(), ret2);
            }
        }
        return std::wstring(pathbuf.get(), ret);
    }
    return path;
}

std::wstring CPathUtils::GetVersionFromFile(LPCWSTR pStrFilename)
{
    // ReSharper disable once CppInconsistentNaming
    struct TRANSARRAY
    {
        WORD wLanguageID;
        WORD wCharacterSet;
    };

    std::wstring strReturn;
    DWORD        dwReserved   = 0;
    DWORD        dwBufferSize = GetFileVersionInfoSize(const_cast<LPWSTR>(pStrFilename), &dwReserved);

    if (dwBufferSize > 0)
    {
        auto pBuffer = std::make_unique<BYTE[]>(dwBufferSize);

        if (pBuffer)
        {
            UINT nInfoSize        = 0,
                 nFixedLength     = 0;
            LPSTR       lpVersion = nullptr;
            VOID*       lpFixedPointer;
            TRANSARRAY* lpTransArray;
            wchar_t     strLangProductVersion[255];

            GetFileVersionInfo(const_cast<LPWSTR>(pStrFilename),
                               dwReserved,
                               dwBufferSize,
                               pBuffer.get());

            // Check the current language
            VerQueryValue(pBuffer.get(),
                          L"\\VarFileInfo\\Translation",
                          &lpFixedPointer,
                          &nFixedLength);
            lpTransArray = static_cast<TRANSARRAY*>(lpFixedPointer);

            swprintf_s(strLangProductVersion, L"\\StringFileInfo\\%04x%04x\\ProductVersion",
                       lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

            VerQueryValue(pBuffer.get(),
                          strLangProductVersion,
                          reinterpret_cast<LPVOID*>(&lpVersion),
                          &nInfoSize);
            if (nInfoSize && lpVersion)
                strReturn = reinterpret_cast<LPCWSTR>(lpVersion);
        }
    }

    return strReturn;
}

#ifdef CSTRING_AVAILABLE
CStringA CPathUtils::PathEscape(const CStringA& path)
{
    CStringA ret2;
    // if we encounter a char that needs escaping, we assume
    // that the hole string needs escaping, skipping the "DoesPercentNeedEscaping" check.
    int           j             = 0;
    bool          needsEscaping = false;
    std::set<int> escapedPositions;
    for (int i = 0; path[i]; ++i)
    {
        auto c = static_cast<unsigned char>(path[i]);
        if (iri_escape_chars[c])
        {
            // no escaping needed for that char
            ret2.AppendChar(static_cast<unsigned char>(path[i]));
        }
        else
        {
            // char needs escaping
            ret2.AppendFormat("%%%02X", static_cast<unsigned char>(c));
            escapedPositions.insert(j);
            needsEscaping = true;
            j += 2;
        }
        ++j;
    }
    CStringA ret;
    for (int i = 0; ret2[i]; ++i)
    {
        auto c = static_cast<unsigned char>(ret2[i]);
        if (uri_autoescape_chars[c])
        {
            if ((c == '%') && ((needsEscaping && (ret2.Mid(i).Left(3) != "%25") && (escapedPositions.find(i) == escapedPositions.end())) || DoesPercentNeedEscaping(ret2.Mid(i))))
            {
                // this percent sign needs escaping!
                ret.AppendFormat("%%%02X", static_cast<unsigned char>(c));
                needsEscaping = true;
            }
            else
            {
                // no escaping needed for that char
                ret.AppendChar(static_cast<unsigned char>(ret2[i]));
            }
        }
        else
        {
            // char needs escaping
            ret.AppendFormat("%%%02X", static_cast<unsigned char>(c));
            needsEscaping = true;
        }
    }

    if ((ret.Left(11).Compare("file:///%5C") == 0) && (ret.Find('%', 12) < 0))
        ret.Replace(("file:///%5C"), ("file://"));
    ret.Replace(("file:////%5C"), ("file://"));

    // properly handle ipv6 addresses
    int urlPos = ret.Find("://%5B");
    if (urlPos > 0)
    {
        int domainPos = ret.Find("/", urlPos + 6);
        if (domainPos > urlPos)
        {
            CStringA leftpart = ret.Left(domainPos + 1);
            if ((leftpart.Find("%5D:") > 0) || (leftpart.Find("%5D/") > 0))
            {
                leftpart.Replace("://%5B", "://[");
                leftpart.Replace("%5D/", "]/");
                leftpart.Replace("%5D:", "]:");
                ret = leftpart + ret.Mid(domainPos + 1);
            }
        }
    }
    return ret;
}

bool CPathUtils::Touch(const CString& path)
{
    CAutoFile hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile)
    {
        FILETIME   ft;
        SYSTEMTIME st;

        GetSystemTime(&st);                                  // Gets the current system time
        SystemTimeToFileTime(&st, &ft);                      // Converts the current system time to file time format
        return SetFileTime(hFile,                            // Sets last-write time of the file
                           static_cast<LPFILETIME>(nullptr), // to the converted current system time
                           static_cast<LPFILETIME>(nullptr),
                           &ft) != FALSE;
    }
    return false;
}

bool CPathUtils::DoesPercentNeedEscaping(LPCSTR str)
{
    if (str[1] == 0)
        return true;
    if (!(((str[1] >= '0') && (str[1] <= '9')) || ((str[1] >= 'A') && (str[1] <= 'F')) || ((str[1] >= 'a') && (str[1] <= 'f'))))
        return true;
    if (!(((str[2] >= '0') && (str[2] <= '9')) || ((str[2] >= 'A') && (str[2] <= 'F')) || ((str[2] >= 'a') && (str[2] <= 'f'))))
        return true;
    return false;
}

CString CPathUtils::GetAppPath(HMODULE hMod /*= NULL*/)
{
    CString path;
    DWORD   len       = 0;
    DWORD   bufferlen = MAX_PATH; // MAX_PATH is not the limit here!
    path.GetBuffer(bufferlen);
    do
    {
        bufferlen += MAX_PATH; // MAX_PATH is not the limit here!
        path.ReleaseBuffer(0);
        len = GetModuleFileName(hMod, path.GetBuffer(bufferlen + 1), bufferlen);
    } while (len == bufferlen);
    path.ReleaseBuffer();
    return GetLongPathname(static_cast<LPCWSTR>(path)).c_str();
}

CString CPathUtils::GetAppDirectory(HMODULE hMod /* = NULL */)
{
    CString path = GetAppPath(hMod);
    path         = path.Left(path.ReverseFind('\\') + 1);
    return GetLongPathname(static_cast<LPCWSTR>(path)).c_str();
}

CString CPathUtils::GetAppParentDirectory(HMODULE hMod /* = NULL */)
{
    CString path = GetAppDirectory(hMod);
    path         = path.Left(path.ReverseFind('\\'));
    path         = path.Left(path.ReverseFind('\\') + 1);
    return path;
}

CString CPathUtils::GetFileNameFromPath(CString sPath)
{
    sPath.Replace('/', '\\');
    CString ret = sPath.Mid(sPath.ReverseFind('\\') + 1);
    return ret;
}

CString CPathUtils::GetFileExtFromPath(const CString& sPath)
{
    int dotPos   = sPath.ReverseFind('.');
    int slashPos = sPath.ReverseFind('\\');
    if (slashPos < 0)
        slashPos = sPath.ReverseFind('/');
    if (dotPos > slashPos)
        return sPath.Mid(dotPos);
    return CString();
}

CStringA CPathUtils::GetAbsoluteURL(const CStringA& Url, const CStringA& repositoryRootURL, const CStringA& parentPathURL)
{
    CStringA errorResult;
    SVNPool  pool;

    /* If the URL is already absolute, there is nothing to do. */

    const char* canonicalizedUrl = nullptr;
    svn_error_clear(svn_uri_canonicalize_safe(&canonicalizedUrl, nullptr, Url, pool, pool));

    if (canonicalizedUrl && svn_path_is_url(canonicalizedUrl))
        return canonicalizedUrl;
    if (canonicalizedUrl == nullptr)
        canonicalizedUrl = Url;

    /* Parse the parent directory URL into its parts. */

    apr_uri_t parentDirParsedUri;
    if (apr_uri_parse(pool, parentPathURL, &parentDirParsedUri))
        return errorResult;

    /* If the parent directory URL is at the server root, then the URL
       may have no / after the hostname so apr_uri_parse() will leave
       the URL's path as NULL. */

    if (!parentDirParsedUri.path)
        parentDirParsedUri.path = apr_pstrmemdup(pool, "/", 1);

    /* Handle URLs relative to the current directory or to the
       repository root.  The backpaths may only remove path elements,
       not the hostname.  This allows an external to refer to another
       repository in the same server relative to the location of this
       repository, say using SVNParentPath. */

    if ((0 == strncmp("../", Url, 3)) || (0 == strncmp("^/", Url, 2)))
    {
        apr_array_header_t* baseComponents     = nullptr;
        apr_array_header_t* relativeComponents = nullptr;

        /* Decompose either the parent directory's URL path or the
           repository root's URL path into components.  */

        if (0 == strncmp("../", Url, 3))
        {
            baseComponents     = svn_path_decompose(parentDirParsedUri.path, pool);
            relativeComponents = svn_path_decompose(canonicalizedUrl, pool);
        }
        else
        {
            apr_uri_t reposRootParsedUri;
            if (apr_uri_parse(pool, repositoryRootURL, &reposRootParsedUri))
                return errorResult;

            /* If the repository root URL is at the server root, then
               the URL may have no / after the hostname so
               apr_uri_parse() will leave the URL's path as NULL. */

            if (!reposRootParsedUri.path)
                reposRootParsedUri.path = apr_pstrmemdup(pool, "/", 1);

            baseComponents     = svn_path_decompose(reposRootParsedUri.path, pool);
            relativeComponents = svn_path_decompose(canonicalizedUrl + 2, pool);
        }

        for (int i = 0; i < relativeComponents->nelts; ++i)
        {
            const char* component = APR_ARRAY_IDX(relativeComponents, i, const char*);

            if (0 == strcmp("..", component))
            {
                /* Constructing the final absolute URL together with
                   apr_uri_unparse() requires that the path be absolute,
                   so only pop a component if the component being popped
                   is not the component for the root directory. */

                if (baseComponents->nelts > 1)
                    apr_array_pop(baseComponents);
            }
            else
                APR_ARRAY_PUSH(baseComponents, const char*) = component;
        }

        parentDirParsedUri.path     = const_cast<char*>(svn_path_compose(baseComponents,
                                                                     pool));
        parentDirParsedUri.query    = nullptr;
        parentDirParsedUri.fragment = nullptr;

        return apr_uri_unparse(pool, &parentDirParsedUri, 0);
    }

    /* The remaining URLs are relative to the either the scheme or
       server root and can only refer to locations inside that scope, so
       backpaths are not allowed. */

    if (svn_path_is_backpath_present(canonicalizedUrl + 2))
        return errorResult;

    /* Relative to the scheme. */

    if (0 == strncmp("//", Url, 2))
    {
        CStringA scheme = repositoryRootURL.Left(repositoryRootURL.Find(':'));
        if (scheme.IsEmpty())
            return errorResult;

        canonicalizedUrl = nullptr;
        svn_error_clear(svn_uri_canonicalize_safe(&canonicalizedUrl, nullptr, apr_pstrcat(pool, static_cast<LPCSTR>(scheme), ":", static_cast<LPCSTR>(Url), NULL), pool, pool));
        return canonicalizedUrl;
    }

    /* Relative to the server root. */

    if (Url[0] == '/')
    {
        parentDirParsedUri.path     = const_cast<char*>(static_cast<const char*>(Url));
        parentDirParsedUri.query    = nullptr;
        parentDirParsedUri.fragment = nullptr;

        return apr_uri_unparse(pool, &parentDirParsedUri, 0);
    }

    return errorResult;
}

BOOL CPathUtils::FileCopy(CString srcPath, CString destPath, BOOL force)
{
    srcPath.Replace('/', '\\');
    destPath.Replace('/', '\\');
    CString destFolder = destPath.Left(destPath.ReverseFind('\\'));
    MakeSureDirectoryPathExists(destFolder);
    return (CopyFile(srcPath, destPath, !force));
}

CString CPathUtils::ParsePathInString(const CString& str)
{
    int     curPos = 0;
    CString sToken = str.Tokenize(L"'\t\r\n", curPos);
    while (!sToken.IsEmpty())
    {
        if ((sToken.Find('/') >= 0) || (sToken.Find('\\') >= 0))
        {
            sToken.Trim(L"'\"");
            return sToken;
        }
        sToken = str.Tokenize(L"'\t\r\n", curPos);
    }
    sToken.Empty();
    return sToken;
}

CString CPathUtils::GetAppDataDirectory()
{
    PWSTR pszPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
        return CString();

    CString path = pszPath;
    CoTaskMemFree(pszPath);

    path += L"\\TortoiseSVN";
    if (!PathIsDirectory(path))
        CreateDirectory(path, nullptr);

    return CString(path) + '\\';
}

CString CPathUtils::GetLocalAppDataDirectory()
{
    PWSTR pszPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
        return CString();

    CString path = pszPath;
    CoTaskMemFree(pszPath);

    path += L"\\TortoiseSVN";
    if (!PathIsDirectory(path))
        CreateDirectory(path, nullptr);

    return CString(path) + '\\';
}

CStringA CPathUtils::PathUnescape(const CStringA& path)
{
    auto urlaBuf = std::make_unique<char[]>(path.GetLength() + 1);

    strcpy_s(urlaBuf.get(), path.GetLength() + 1, path);
    Unescape(urlaBuf.get());

    return urlaBuf.get();
}

CStringW CPathUtils::PathUnescape(const CStringW& path)
{
    int len = path.GetLength();
    if (len == 0)
        return CStringW();
    CStringA patha = CUnicodeUtils::GetUTF8(path);

    patha = PathUnescape(patha);

    return CUnicodeUtils::GetUnicode(patha);
}

CString CPathUtils::PathUnescape(const char* path)
{
    // try quick path
    size_t i = 0;
    for (; char c = path[i]; ++i)
        if ((static_cast<unsigned char>(c) >= 0x80) || (c == '%'))
        {
            // quick path does not work for non-latin or escaped chars
            std::string utf8Path(path);

            CPathUtils::Unescape(&utf8Path[0]);
            return CUnicodeUtils::UTF8ToUTF16(utf8Path);
        }

    // no escapement necessary, just unicode conversion
    return CUnicodeUtils::GetUnicode(path);
}

CString CPathUtils::PathPatternEscape(const CString& path)
{
    CString result = path;
    // first remove already escaped patterns to avoid having those
    // escaped twice
    result.Replace(L"\\[", L"[");
    result.Replace(L"\\]", L"]");
    // now escape the patterns (again)
    result.Replace(L"[", L"\\[");
    result.Replace(L"]", L"\\]");
    return result;
}

CString CPathUtils::PathPatternUnEscape(const CString& path)
{
    CString result = path;
    result.Replace(L"\\[", L"[");
    result.Replace(L"\\]", L"]");
    return result;
}

CString CPathUtils::CombineUrls(CString first, CString second)
{
    first.TrimRight('/');
    second.TrimLeft('/');
    return first + L"/" + second;
}

#endif

#if defined(_DEBUG) && defined(_MFC_VER)
// Some test cases for these classes
[[maybe_unused]] static class CPathTests
{
public:
    CPathTests()
    {
        EscapeTest();
        UnescapeTest();
        ExtTest();
        ParseTests();
    }

private:
    static void EscapeTest()
    {
        CStringA test("http://domain.com/page");
        CStringA test2 = CPathUtils::PathEscape(test);
        ATLASSERT(test.Compare(test2) == 0);

        test  = "http://[::1]/page";
        test2 = CPathUtils::PathEscape(test);
        ATLASSERT(test.Compare(test2) == 0);

        test  = "https://[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443/page%5B%5D/test";
        test2 = CPathUtils::PathEscape("https://[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443/page[]/test");
        ATLASSERT(test.Compare(test2) == 0);
    }
    static void UnescapeTest()
    {
        CString test(L"file:///d:/REpos1/uCOS-100/Trunk/name%20with%20spaces/NewTest%20%25%20NewTest");
        CString test2 = CPathUtils::PathUnescape(test);
        ATLASSERT(test2.Compare(L"file:///d:/REpos1/uCOS-100/Trunk/name with spaces/NewTest % NewTest") == 0);
        test2 = CPathUtils::PathUnescape("file:///d:/REpos1/uCOS-100/Trunk/name with spaces/NewTest % NewTest");
        ATLASSERT(test2.Compare(L"file:///d:/REpos1/uCOS-100/Trunk/name with spaces/NewTest % NewTest") == 0);
        test2 = CPathUtils::PathUnescape("http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk");
        ATLASSERT(test2.Compare(L"http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk") == 0);
        CStringA test3 = CPathUtils::PathEscape("file:///d:/REpos1/uCOS-100/Trunk/name with spaces/NewTest % NewTest");
        ATLASSERT(test3.Compare("file:///d:/REpos1/uCOS-100/Trunk/name%20with%20spaces/NewTest%20%25%20NewTest") == 0);
        CStringA test4 = CPathUtils::PathEscape("file:///d:/REpos1/uCOS 1.0/Trunk/name with spaces/NewTest % NewTest");
        ATLASSERT(test4.Compare("file:///d:/REpos1/uCOS%201.0/Trunk/name%20with%20spaces/NewTest%20%25%20NewTest") == 0);
    }
    static void ExtTest()
    {
        CString test(L"d:\\test\filename.ext");
        ATLASSERT(CPathUtils::GetFileExtFromPath(test).Compare(L".ext") == 0);
        test = L"filename.ext";
        ATLASSERT(CPathUtils::GetFileExtFromPath(test).Compare(L".ext") == 0);
        test = L"d:\\test\filename";
        ATLASSERT(CPathUtils::GetFileExtFromPath(test).IsEmpty());
        test = L"filename";
        ATLASSERT(CPathUtils::GetFileExtFromPath(test).IsEmpty());
    }
    static void ParseTests()
    {
        CString test(L"test 'd:\\testpath with spaces' test");
        ATLASSERT(CPathUtils::ParsePathInString(test).Compare(L"d:\\testpath with spaces") == 0);
        test = L"d:\\testpath with spaces";
        ATLASSERT(CPathUtils::ParsePathInString(test).Compare(L"d:\\testpath with spaces") == 0);
    }

    // ReSharper disable once CppInconsistentNaming
} CPathTests;
#endif
