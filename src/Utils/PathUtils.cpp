// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include <shlobj.h>
#include "UnicodeUtils.h"

#include "SVNHelpers.h"
#include "apr_uri.h"
#include "svn_path.h"
#include <emmintrin.h>
#include <memory>

static BOOL sse2supported = ::IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE );

BOOL CPathUtils::MakeSureDirectoryPathExists(LPCTSTR path)
{
    const size_t len = wcslen(path);
    const size_t fullLen = len+10;
    std::unique_ptr<TCHAR[]> buf(new TCHAR[fullLen]);
    std::unique_ptr<TCHAR[]> internalpathbuf(new TCHAR[fullLen]);
    TCHAR * pPath = internalpathbuf.get();
    SECURITY_ATTRIBUTES attribs;

    SecureZeroMemory(&attribs, sizeof(SECURITY_ATTRIBUTES));

    attribs.nLength = sizeof(SECURITY_ATTRIBUTES);
    attribs.bInheritHandle = FALSE;

    ConvertToBackslash(internalpathbuf.get(), path, fullLen);
    if (wcsncmp(internalpathbuf.get(), L"\\\\?\\", 4) == 0)
        pPath += 4;
    do
    {
        SecureZeroMemory(buf.get(), fullLen*sizeof(TCHAR));
        TCHAR * slashpos = wcschr(pPath, '\\');
        if (slashpos)
            wcsncpy_s(buf.get(), fullLen, internalpathbuf.get(), slashpos - internalpathbuf.get());
        else
            wcsncpy_s(buf.get(), fullLen, internalpathbuf.get(), fullLen);
        CreateDirectory(buf.get(), &attribs);
        pPath = wcschr(pPath, '\\');
    } while ((pPath++)&&(wcschr(pPath, '\\')));

    const BOOL bRet = CreateDirectory(internalpathbuf.get(), &attribs);
    return bRet;
}

bool CPathUtils::ContainsEscapedChars(const char * psz, size_t length)
{
    // most of our strings will be tens of bytes long
    // -> affort some minor overhead to handle the main part very fast

    const char* end = psz + length;
    if (sse2supported)
    {
        __m128i mask = _mm_set_epi8 ( '%', '%', '%', '%', '%', '%', '%', '%'
            , '%', '%', '%', '%', '%', '%', '%', '%');

        for (; psz + sizeof (mask) <= end; psz += sizeof (mask))
        {
            // fetch the next 16 bytes from the source

            __m128i chunk = _mm_loadu_si128 ((const __m128i*)psz);

            // check for non-ASCII

            int flags = _mm_movemask_epi8 (_mm_cmpeq_epi8 (chunk, mask));
            if (flags != 0)
                return true;
        };
    }

    // return odd bytes at the end of the string

    for (; psz < end; ++psz)
        if (*psz == '%')
            return true;

    return false;
}

char* CPathUtils::Unescape(char * psz)
{
    char * pszSource = psz;
    char * pszDest = psz;

    static const char szHex[] = "0123456789ABCDEF";

    // Unescape special characters. The number of characters
    // in the *pszDest is assumed to be <= the number of characters
    // in pszSource (they are both the same string anyway)

    while (*pszSource != '\0' && *pszDest != '\0')
    {
        if (*pszSource == '%')
        {
            // The next two chars following '%' should be digits
            if ( *(pszSource + 1) == '\0' ||
                *(pszSource + 2) == '\0' )
            {
                // nothing left to do
                break;
            }

            char nValue = '?';
            const char * pszHigh = NULL;
            pszSource++;

            *pszSource = (char) toupper(*pszSource);
            pszHigh = strchr(szHex, *pszSource);

            if (pszHigh != NULL)
            {
                pszSource++;
                *pszSource = (char) toupper(*pszSource);
                const char * pszLow = strchr(szHex, *pszSource);

                if (pszLow != NULL)
                {
                    nValue = (char) (((pszHigh - szHex) << 4) +
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

static const char iri_escape_chars[256] = {
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,

    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
};

const char uri_autoescape_chars[256] = {
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

    /* 64 */
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

    /* 192 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

static const char uri_char_validity[256] = {
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 0, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

    /* 64 */
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

    /* 128 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

    /* 192 */
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};


void CPathUtils::ConvertToBackslash(LPTSTR dest, LPCTSTR src, size_t len)
{
    wcscpy_s(dest, len, src);
    TCHAR * p = dest;
    for (; *p != '\0'; ++p)
        if (*p == '/')
            *p = '\\';
}

#ifdef CSTRING_AVAILABLE
CStringA CPathUtils::PathEscape(const CStringA& path)
{
    CStringA ret2;
    int c;
    int i;
    for (i=0; path[i]; ++i)
    {
        c = (unsigned char)path[i];
        if (iri_escape_chars[c])
        {
            // no escaping needed for that char
            ret2 += (unsigned char)path[i];
        }
        else
        {
            // char needs escaping
            CStringA temp;
            temp.Format("%%%02X", (unsigned char)c);
            ret2 += temp;
        }
    }
    CStringA ret;
    for (i=0; ret2[i]; ++i)
    {
        c = (unsigned char)ret2[i];
        if (uri_autoescape_chars[c])
        {
            if ((c == '%')&&(DoesPercentNeedEscaping(ret2.Mid(i))))
            {
                // this percent sign needs escaping!
                CStringA temp;
                temp.Format("%%%02X", (unsigned char)c);
                ret += temp;
            }
            else
            {
                // no escaping needed for that char
                ret += (unsigned char)ret2[i];
            }
        }
        else
        {
            // char needs escaping
            CStringA temp;
            temp.Format("%%%02X", (unsigned char)c);
            ret += temp;
        }
    }

    if ((ret.Left(11).Compare("file:///%5C") == 0) && (ret.Find('%', 12) < 0))
        ret.Replace(("file:///%5C"), ("file://"));
    ret.Replace(("file:////%5C"), ("file://"));

    // properly handle ipv6 addresses
    int urlpos = ret.Find("://%5B");
    if (urlpos > 0)
    {
        int domainpos = ret.Find("/", urlpos+6);
        if (domainpos > urlpos)
        {
            CStringA leftpart = ret.Left(domainpos+1);
            if ((leftpart.Find("%5D:")>0)||(leftpart.Find("%5D/")>0))
            {
                leftpart.Replace("://%5B", "://[");
                leftpart.Replace("%5D/", "]/");
                leftpart.Replace("%5D:", "]:");
                ret = leftpart + ret.Mid(domainpos+1);
            }
        }
    }
    return ret;
}

bool CPathUtils::DoesPercentNeedEscaping(LPCSTR str)
{
    if (str[1] == 0)
        return true;
    if (!(((str[1] >= '0')&&(str[1] <= '9'))||((str[1] >= 'A')&&(str[1] <= 'F'))||((str[1] >= 'a')&&(str[1] <= 'f'))))
        return true;
    if (!(((str[2] >= '0')&&(str[2] <= '9'))||((str[2] >= 'A')&&(str[2] <= 'F'))||((str[2] >= 'a')&&(str[2] <= 'f'))))
        return true;
    return false;
}

CString CPathUtils::GetAppDirectory(HMODULE hMod /* = NULL */)
{
    CString path;
    DWORD len = 0;
    DWORD bufferlen = MAX_PATH;     // MAX_PATH is not the limit here!
    path.GetBuffer(bufferlen);
    do
    {
        bufferlen += MAX_PATH;      // MAX_PATH is not the limit here!
        path.ReleaseBuffer(0);
        len = GetModuleFileName(hMod, path.GetBuffer(bufferlen+1), bufferlen);
    } while(len == bufferlen);
    path.ReleaseBuffer();
    path = path.Left(path.ReverseFind('\\')+1);
    return GetLongPathname(path);
}

CString CPathUtils::GetAppParentDirectory(HMODULE hMod /* = NULL */)
{
    CString path = GetAppDirectory(hMod);
    path = path.Left(path.ReverseFind('\\'));
    path = path.Left(path.ReverseFind('\\')+1);
    return path;
}

CString CPathUtils::GetLongPathname(const CString& path)
{
    if (path.IsEmpty())
        return path;
    TCHAR pathbufcanonicalized[MAX_PATH] = { 0 }; // MAX_PATH ok.
    DWORD ret = 0;
    CString sRet = path;
    if (!PathIsURL(path) && PathIsRelative(path))
    {
        ret = GetFullPathName(path, 0, NULL, NULL);
        if (ret)
        {
            std::unique_ptr<TCHAR[]> pathbuf(new TCHAR[ret+1]);
            if ((ret = GetFullPathName(path, ret, pathbuf.get(), NULL))!=0)
            {
                sRet = CString(pathbuf.get(), ret);
            }
        }
    }
    else if (PathCanonicalize(pathbufcanonicalized, path))
    {
        ret = ::GetLongPathName(pathbufcanonicalized, NULL, 0);
        if (ret == 0)
            return path;
        std::unique_ptr<TCHAR[]> pathbuf(new TCHAR[ret+2]);
        ret = ::GetLongPathName(pathbufcanonicalized, pathbuf.get(), ret+1);
        // GetFullPathName() sometimes returns the full path with the wrong
        // case. This is not a problem on Windows since its filesystem is
        // case-insensitive. But for SVN that's a problem if the wrong case
        // is inside a working copy: the svn wc database is case sensitive.
        // To fix the casing of the path, we use a trick:
        // convert the path to its short form, then back to its long form.
        // That will fix the wrong casing of the path.
        int shortret = ::GetShortPathName(pathbuf.get(), NULL, 0);
        if (shortret)
        {
            std::unique_ptr<TCHAR[]> shortpath(new TCHAR[shortret+2]);
            if (::GetShortPathName(pathbuf.get(), shortpath.get(), shortret+1))
            {
                int ret2 = ::GetLongPathName(shortpath.get(), pathbuf.get(), ret+1);
                if (ret2)
                    sRet = CString(pathbuf.get(), ret2);
            }
        }
    }
    else
    {
        ret = ::GetLongPathName(path, NULL, 0);
        if (ret == 0)
            return path;
        std::unique_ptr<TCHAR[]> pathbuf(new TCHAR[ret+2]);
        ret = ::GetLongPathName(path, pathbuf.get(), ret+1);
        sRet = CString(pathbuf.get(), ret);
        // fix the wrong casing of the path. See above for details.
        int shortret = ::GetShortPathName(pathbuf.get(), NULL, 0);
        if (shortret)
        {
            std::unique_ptr<TCHAR[]> shortpath(new TCHAR[shortret+2]);
            if (::GetShortPathName(pathbuf.get(), shortpath.get(), shortret+1))
            {
                int ret2 = ::GetLongPathName(shortpath.get(), pathbuf.get(), ret+1);
                if (ret2)
                    sRet = CString(pathbuf.get(), ret2);
            }
        }
    }
    if (ret == 0)
        return path;
    return sRet;
}

CString CPathUtils::GetFileNameFromPath(CString sPath)
{
    CString ret;
    sPath.Replace('/', '\\');
    ret = sPath.Mid(sPath.ReverseFind('\\') + 1);
    return ret;
}

CString CPathUtils::GetFileExtFromPath(const CString& sPath)
{
    int dotPos = sPath.ReverseFind('.');
    int slashPos = sPath.ReverseFind('\\');
    if (slashPos < 0)
        slashPos = sPath.ReverseFind('/');
    if (dotPos > slashPos)
        return sPath.Mid(dotPos);
    return CString();
}

CStringA CPathUtils::GetAbsoluteURL
    ( const CStringA& URL
    , const CStringA& repositoryRootURL
    , const CStringA& parentPathURL)
{
    CStringA errorResult;
    SVNPool pool;

    /* If the URL is already absolute, there is nothing to do. */

    const char *canonicalized_url = svn_uri_canonicalize (URL, pool);
    if (svn_path_is_url (canonicalized_url))
        return canonicalized_url;

    /* Parse the parent directory URL into its parts. */

    apr_uri_t parent_dir_parsed_uri;
    if (apr_uri_parse (pool, parentPathURL, &parent_dir_parsed_uri))
        return errorResult;

    /* If the parent directory URL is at the server root, then the URL
       may have no / after the hostname so apr_uri_parse() will leave
       the URL's path as NULL. */

    if (! parent_dir_parsed_uri.path)
        parent_dir_parsed_uri.path = apr_pstrmemdup (pool, "/", 1);

    /* Handle URLs relative to the current directory or to the
       repository root.  The backpaths may only remove path elements,
       not the hostname.  This allows an external to refer to another
       repository in the same server relative to the location of this
       repository, say using SVNParentPath. */

    if ((0 == strncmp("../", URL, 3)) || (0 == strncmp("^/", URL, 2)))
    {
        apr_array_header_t *base_components = NULL;
        apr_array_header_t *relative_components = NULL;

        /* Decompose either the parent directory's URL path or the
           repository root's URL path into components.  */

        if (0 == strncmp ("../", URL, 3))
        {
            base_components
                = svn_path_decompose (parent_dir_parsed_uri.path, pool);
            relative_components
                = svn_path_decompose (canonicalized_url, pool);
        }
        else
        {
            apr_uri_t repos_root_parsed_uri;
            if (apr_uri_parse(pool, repositoryRootURL, &repos_root_parsed_uri))
                return errorResult;

            /* If the repository root URL is at the server root, then
               the URL may have no / after the hostname so
               apr_uri_parse() will leave the URL's path as NULL. */

            if (! repos_root_parsed_uri.path)
                repos_root_parsed_uri.path = apr_pstrmemdup (pool, "/", 1);

            base_components
                = svn_path_decompose (repos_root_parsed_uri.path, pool);
            relative_components
                = svn_path_decompose (canonicalized_url + 2, pool);
        }

        for (int i = 0; i < relative_components->nelts; ++i)
        {
            const char *component
                = APR_ARRAY_IDX(relative_components, i, const char *);

            if (0 == strcmp("..", component))
            {
                /* Constructing the final absolute URL together with
                   apr_uri_unparse() requires that the path be absolute,
                   so only pop a component if the component being popped
                   is not the component for the root directory. */

                if (base_components->nelts > 1)
                    apr_array_pop (base_components);
            }
            else
                APR_ARRAY_PUSH (base_components, const char *) = component;
        }

        parent_dir_parsed_uri.path = (char *)svn_path_compose(base_components,
                                                            pool);
        parent_dir_parsed_uri.query = NULL;
        parent_dir_parsed_uri.fragment = NULL;

        return apr_uri_unparse (pool, &parent_dir_parsed_uri, 0);
    }

    /* The remaining URLs are relative to the either the scheme or
       server root and can only refer to locations inside that scope, so
       backpaths are not allowed. */

    if (svn_path_is_backpath_present (canonicalized_url + 2))
        return errorResult;

    /* Relative to the scheme. */

    if (0 == strncmp("//", URL, 2))
    {
        CStringA scheme
            = repositoryRootURL.Left (repositoryRootURL.Find (':'));
        if (scheme.IsEmpty())
            return errorResult;

        return svn_uri_canonicalize ( apr_pstrcat ( pool
                                                   , scheme
                                                   , ":"
                                                   , URL
                                                   , NULL)
                                     , pool);
    }

    /* Relative to the server root. */

    if (URL[0] == '/')
    {
        parent_dir_parsed_uri.path = (char *)(const char*)URL;
        parent_dir_parsed_uri.query = NULL;
        parent_dir_parsed_uri.fragment = NULL;

        return apr_uri_unparse (pool, &parent_dir_parsed_uri, 0);
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

CString CPathUtils::ParsePathInString(const CString& Str)
{
    CString sToken;
    int curPos = 0;
    sToken = Str.Tokenize(L"'\t\r\n", curPos);
    while (!sToken.IsEmpty())
    {
        if ((sToken.Find('/')>=0)||(sToken.Find('\\')>=0))
        {
            sToken.Trim(L"'\"");
            return sToken;
        }
        sToken = Str.Tokenize(L"'\t\r\n", curPos);
    }
    sToken.Empty();
    return sToken;
}

CString CPathUtils::GetAppDataDirectory()
{
    PWSTR pszPath = NULL;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &pszPath) != S_OK)
        return CString();

    CString path = pszPath;
    CoTaskMemFree(pszPath);

    path += L"\\TortoiseSVN";
    if (!PathIsDirectory(path))
        CreateDirectory(path, NULL);

    return CString (path) + '\\';
}

CString CPathUtils::GetLocalAppDataDirectory()
{
    PWSTR pszPath = NULL;
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &pszPath) != S_OK)
        return CString();

    CString path = pszPath;
    CoTaskMemFree(pszPath);

    path += L"\\TortoiseSVN";
    if (!PathIsDirectory(path))
        CreateDirectory(path, NULL);

    return CString(path) + '\\';
}

CStringA CPathUtils::PathUnescape(const CStringA& path)
{
    std::unique_ptr<char[]> urlabuf (new char[path.GetLength()+1]);

    strcpy_s(urlabuf.get(), path.GetLength()+1, path);
    Unescape(urlabuf.get());

    return urlabuf.get();
}

CStringW CPathUtils::PathUnescape(const CStringW& path)
{
    int len = path.GetLength();
    if (len==0)
        return CStringW();
    CStringA patha = CUnicodeUtils::GetUTF8(path);

    patha = PathUnescape(patha);

    return CUnicodeUtils::GetUnicode(patha);
}

CString CPathUtils::PathUnescape (const char* path)
{
    // try quick path
    size_t i = 0;
    for (; char c = path[i]; ++i)
        if ((c >= 0x80) || (c == '%'))
        {
            // quick path does not work for non-latin or escaped chars
            std::string utf8Path (path);

            CPathUtils::Unescape (&utf8Path[0]);
            return CUnicodeUtils::UTF8ToUTF16 (utf8Path);
        }

    // no escapement necessary, just unicode conversion
    return CUnicodeUtils::GetUnicode(path);
}

CString CPathUtils::GetVersionFromFile(const CString & p_strFilename)
{
    struct TRANSARRAY
    {
        WORD wLanguageID;
        WORD wCharacterSet;
    };

    CString strReturn;
    DWORD dwReserved = 0;
    DWORD dwBufferSize = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)p_strFilename,&dwReserved);

    if (dwBufferSize > 0)
    {
        LPVOID pBuffer = (void*) malloc(dwBufferSize);

        if (pBuffer != (void*) NULL)
        {
            UINT        nInfoSize = 0,
                        nFixedLength = 0;
            LPSTR       lpVersion = NULL;
            VOID*       lpFixedPointer;
            TRANSARRAY* lpTransArray;
            CString     strLangProductVersion;

            GetFileVersionInfo((LPTSTR)(LPCTSTR)p_strFilename,
                dwReserved,
                dwBufferSize,
                pBuffer);

            // Check the current language
            VerQueryValue(  pBuffer,
                L"\\VarFileInfo\\Translation",
                &lpFixedPointer,
                &nFixedLength);
            lpTransArray = (TRANSARRAY*) lpFixedPointer;

            strLangProductVersion.Format(L"\\StringFileInfo\\%04x%04x\\ProductVersion",
                lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

            VerQueryValue(pBuffer,
                (LPTSTR)(LPCTSTR)strLangProductVersion,
                (LPVOID *)&lpVersion,
                &nInfoSize);
            if (nInfoSize && lpVersion)
                strReturn = (LPCTSTR)lpVersion;
            free(pBuffer);
        }
    }

    return strReturn;
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
static class CPathTests
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
    void EscapeTest()
    {
        CStringA test("http://domain.com/page");
        CStringA test2 = CPathUtils::PathEscape(test);
        ATLASSERT(test.Compare(test2) == 0);

        test = "http://[::1]/page";
        test2 = CPathUtils::PathEscape(test);
        ATLASSERT(test.Compare(test2) == 0);

        test = "https://[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443/page%5B%5D/test";
        test2 = CPathUtils::PathEscape("https://[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443/page[]/test");
        ATLASSERT(test.Compare(test2) == 0);
    }
    void UnescapeTest()
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
    void ExtTest()
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
    void ParseTests()
    {
        CString test(L"test 'd:\\testpath with spaces' test");
        ATLASSERT(CPathUtils::ParsePathInString(test).Compare(L"d:\\testpath with spaces") == 0);
        test = L"d:\\testpath with spaces";
        ATLASSERT(CPathUtils::ParsePathInString(test).Compare(L"d:\\testpath with spaces") == 0);
    }

} CPathTests;
#endif

