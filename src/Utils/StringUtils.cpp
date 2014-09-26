// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2014 - TortoiseSVN

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
#include "UnicodeUtils.h"
#include "StringUtils.h"
#include "ClipboardHelper.h"
#include "SmartHandle.h"
#include <memory>
#include <WinCrypt.h>
#include <atlstr.h>

#pragma comment(lib, "Crypt32.lib")

int strwildcmp(const char *wild, const char *string)
{
    const char *cp = NULL;
    const char *mp = NULL;
    while ((*string) && (*wild != '*'))
    {
        if ((*wild != *string) && (*wild != '?'))
        {
            return 0;
        }
        wild++;
        string++;
    }
    while (*string)
    {
        if (*wild == '*')
        {
            if (!*++wild)
            {
                return 1;
            }
            mp = wild;
            cp = string+1;
        }
        else if ((*wild == *string) || (*wild == '?'))
        {
            wild++;
            string++;
        }
        else
        {
            wild = mp;
            string = cp++;
        }
    }

    while (*wild == '*')
    {
        wild++;
    }
    return !*wild;
}

int wcswildcmp(const wchar_t *wild, const wchar_t *string)
{
    const wchar_t *cp = NULL;
    const wchar_t *mp = NULL;
    while ((*string) && (*wild != '*'))
    {
        if ((*wild != *string) && (*wild != '?'))
        {
            return 0;
        }
        wild++;
        string++;
    }
    while (*string)
    {
        if (*wild == '*')
        {
            if (!*++wild)
            {
                return 1;
            }
            mp = wild;
            cp = string+1;
        }
        else if ((*wild == *string) || (*wild == '?'))
        {
            wild++;
            string++;
        }
        else
        {
            wild = mp;
            string = cp++;
        }
    }

    while (*wild == '*')
    {
        wild++;
    }
    return !*wild;
}

#ifdef _MFC_VER

void CStringUtils::RemoveAccelerators(CString& text)
{
    int pos = 0;
    while ((pos=text.Find('&',pos))>=0)
    {
        if (text.GetLength() > (pos-1))
        {
            if (text.GetAt(pos+1)!=' ')
                text.Delete(pos);
        }
        pos++;
    }
}

bool CStringUtils::WriteAsciiStringToClipboard(const CStringA& sClipdata, LCID lcid, HWND hOwningWnd)
{
    CClipboardHelper clipboardHelper;
    if (clipboardHelper.Open(hOwningWnd))
    {
        EmptyClipboard();
        HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc(sClipdata.GetLength()+1);
        if (hClipboardData)
        {
            char* pchData = (char*)GlobalLock(hClipboardData);
            if (pchData)
            {
                strcpy_s(pchData, sClipdata.GetLength()+1, (LPCSTR)sClipdata);
                GlobalUnlock(hClipboardData);
                if (SetClipboardData(CF_TEXT, hClipboardData))
                {
                    HANDLE hlocmem = CClipboardHelper::GlobalAlloc(sizeof(LCID));
                    if (hlocmem)
                    {
                        PLCID plcid = (PLCID)GlobalLock(hlocmem);
                        if (plcid)
                        {
                            *plcid = lcid;
                            SetClipboardData(CF_LOCALE, static_cast<HANDLE>(plcid));
                        }
                        GlobalUnlock(hlocmem);
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

bool CStringUtils::WriteAsciiStringToClipboard(const CStringW& sClipdata, HWND hOwningWnd)
{
    CClipboardHelper clipboardHelper;
    if (clipboardHelper.Open(hOwningWnd))
    {
        EmptyClipboard();
        HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc((sClipdata.GetLength()+1)*sizeof(WCHAR));
        if (hClipboardData)
        {
            WCHAR* pchData = (WCHAR*)GlobalLock(hClipboardData);
            if (pchData)
            {
                wcscpy_s(pchData, sClipdata.GetLength()+1, (LPCWSTR)sClipdata);
                GlobalUnlock(hClipboardData);
                if (SetClipboardData(CF_UNICODETEXT, hClipboardData))
                {
                    // no need to also set CF_TEXT : the OS does this
                    // automatically.
                    return true;
                }
            }
        }
    }
    return false;
}

bool CStringUtils::WriteDiffToClipboard(const CStringA& sClipdata, HWND hOwningWnd)
{
    UINT cFormat = RegisterClipboardFormat(L"TSVN_UNIFIEDDIFF");
    if (cFormat == 0)
        return false;
    CClipboardHelper clipboardHelper;
    if (clipboardHelper.Open(hOwningWnd))
    {
        EmptyClipboard();
        HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc(sClipdata.GetLength()+1);
        if (hClipboardData)
        {
            char* pchData = (char*)GlobalLock(hClipboardData);
            if (pchData)
            {
                strcpy_s(pchData, sClipdata.GetLength()+1, (LPCSTR)sClipdata);
                GlobalUnlock(hClipboardData);
                if (SetClipboardData(cFormat,hClipboardData)==NULL)
                {
                    return false;
                }
                if (SetClipboardData(CF_TEXT,hClipboardData))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool CStringUtils::ReadStringFromTextFile(const CString& path, CString& text)
{
    if (!PathFileExists(path))
        return false;
    try
    {
        CStdioFile file;
        if (!file.Open(path, CFile::modeRead | CFile::shareDenyWrite))
            return false;

        CStringA filecontent;
        UINT filelength = (UINT)file.GetLength();
        int bytesread = (int)file.Read(filecontent.GetBuffer(filelength), filelength);
        filecontent.ReleaseBuffer(bytesread);
        text = CUnicodeUtils::GetUnicode(filecontent);
        file.Close();
    }
    catch (CFileException* pE)
    {
        text.Empty();
        pE->Delete();
    }
    return true;
}

#endif // #ifdef _MFC_VER

#if defined(CSTRING_AVAILABLE) || defined(_MFC_VER)
BOOL CStringUtils::WildCardMatch(const CString& wildcard, const CString& string)
{
    return _tcswildcmp(wildcard, string);
}

CString CStringUtils::LinesWrap(const CString& longstring, int limit /* = 80 */, bool bCompactPaths /* = true */)
{
    CString retString;
    if ((longstring.GetLength() < limit) || (limit == 0))
        return longstring;  // no wrapping needed.
    // now start breaking the string into lines

    int linepos = 0;
    int lineposold = 0;
    CString temp;
    while ((linepos = longstring.Find('\n', linepos)) >= 0)
    {
        temp = longstring.Mid(lineposold, linepos-lineposold);
        if ((linepos+1)<longstring.GetLength())
            linepos++;
        else
            break;
        lineposold = linepos;
        if (!retString.IsEmpty())
            retString += L"\n";
        retString += WordWrap(temp, limit, bCompactPaths, false, 4);
    }
    temp = longstring.Mid(lineposold);
    if (!temp.IsEmpty())
        retString += L"\n";
    retString += WordWrap(temp, limit, bCompactPaths, false, 4);
    retString.Trim();
    return retString;
}

CString CStringUtils::WordWrap(const CString& longstring, int limit, bool bCompactPaths, bool bForceWrap, int tabSize)
{
    int nLength = longstring.GetLength();
    CString retString;

    if (limit < 0)
        limit = 0;

    int nLineStart = 0;
    int nLineEnd = 0;
    int tabOffset = 0;
    for (int i = 0; i < nLength; ++i)
    {
        if (i-nLineStart+tabOffset >= limit)
        {
            if (nLineEnd == nLineStart)
            {
                if (bForceWrap)
                    nLineEnd = i;
                else
                {
                    while ((i < nLength) && (longstring[i] != ' ') && (longstring[i] != '\t'))
                        ++i;
                    nLineEnd = i;
                }
            }
            if (bCompactPaths)
            {
                CString longline = longstring.Mid(nLineStart, nLineEnd-nLineStart).Left(MAX_PATH-1);
                if ((bCompactPaths)&&(longline.GetLength() < MAX_PATH))
                {
                    if (((!PathIsFileSpec(longline))&&longline.Find(':')<3)||(PathIsURL(longline)))
                    {
                        TCHAR buf[MAX_PATH] = { 0 };
                        PathCompactPathEx(buf, longline, limit+1, 0);
                        longline = buf;
                    }
                }
                retString += longline;
            }
            else
                retString += longstring.Mid(nLineStart, nLineEnd-nLineStart);
            retString += L"\n";
            tabOffset = 0;
            nLineStart = nLineEnd;
        }
        if (longstring[i] == ' ')
            nLineEnd = i;
        if (longstring[i] == '\t')
        {
            tabOffset += (tabSize - i % tabSize);
            nLineEnd = i;
        }
    }
    if (bCompactPaths)
    {
        CString longline = longstring.Mid(nLineStart).Left(MAX_PATH-1);
        if ((bCompactPaths)&&(longline.GetLength() < MAX_PATH))
        {
            if (((!PathIsFileSpec(longline))&&longline.Find(':')<3)||(PathIsURL(longline)))
            {
                TCHAR buf[MAX_PATH] = { 0 };
                PathCompactPathEx(buf, longline, limit+1, 0);
                longline = buf;
            }
        }
        retString += longline;
    }
    else
        retString += longstring.Mid(nLineStart);

    return retString;
}
int CStringUtils::GetMatchingLength (const CString& lhs, const CString& rhs)
{
    int lhsLength = lhs.GetLength();
    int rhsLength = rhs.GetLength();
    int maxResult = min (lhsLength, rhsLength);

    LPCTSTR pLhs = lhs;
    LPCTSTR pRhs = rhs;

    for (int i = 0; i < maxResult; ++i)
        if (pLhs[i] != pRhs[i])
            return i;

    return maxResult;
}

int CStringUtils::FastCompareNoCase (const CStringW& lhs, const CStringW& rhs)
{
    // attempt latin-only comparison

    INT_PTR count = min (lhs.GetLength(), rhs.GetLength()+1);
    const wchar_t* left = lhs;
    const wchar_t* right = rhs;
    for (const wchar_t* last = left + count+1; left < last; ++left, ++right)
    {
        int leftChar = *left;
        int rightChar = *right;

        int diff = leftChar - rightChar;
        if (diff != 0)
        {
            // case-sensitive comparison found a difference

            if ((leftChar | rightChar) >= 0x80)
            {
                // non-latin char -> fall back to CRT code
                // (full comparison required as we might have
                // skipped special chars / UTF plane selectors)

                return _wcsicmp (lhs, rhs);
            }

            // normalize to lower case

            if ((leftChar >= 'A') && (leftChar <= 'Z'))
                leftChar += 'a' - 'A';
            if ((rightChar >= 'A') && (rightChar <= 'Z'))
                rightChar += 'a' - 'A';

            // compare again

            diff = leftChar - rightChar;
            if (diff != 0)
                return diff;
        }
    }

    // must be equal (both ended with a 0)

    return 0;
}

bool CStringUtils::WriteStringToTextFile(const std::wstring& path, const std::wstring& text, bool bUTF8 /* = true */)
{
    DWORD dwWritten = 0;
    CAutoFile hFile = CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hFile)
        return false;

    if (bUTF8)
    {
        std::string buf = CUnicodeUtils::StdGetUTF8(text);
        if (!WriteFile(hFile, buf.c_str(), (DWORD)buf.length(), &dwWritten, NULL))
        {
            return false;
        }
    }
    else
    {
        if (!WriteFile(hFile, text.c_str(), (DWORD)text.length(), &dwWritten, NULL))
        {
            return false;
        }
    }
    return true;
}

bool CStringUtils::WriteStringToTextFile(const std::wstring& path, const std::string& text)
{
    DWORD dwWritten = 0;
    CAutoFile hFile = CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hFile)
        return false;

    if (!WriteFile(hFile, text.c_str(), (DWORD)text.length(), &dwWritten, NULL))
    {
        return false;
    }
    return true;
}

#endif // #if defined(CSTRING_AVAILABLE) || defined(_MFC_VER)

inline static void PipeToNull(TCHAR* ptr)
{
    if (*ptr == '|')
        *ptr = '\0';
}

void CStringUtils::PipesToNulls(TCHAR* buffer, size_t length)
{
    TCHAR* ptr = buffer + length;
    while (ptr != buffer)
    {
        PipeToNull(ptr);
        ptr--;
    }
}

void CStringUtils::PipesToNulls(TCHAR* buffer)
{
    TCHAR* ptr = buffer;
    while (*ptr != 0)
    {
        PipeToNull(ptr);
        ptr++;
    }
}

std::unique_ptr<char[]> CStringUtils::Decrypt(const char * text)
{
    DWORD dwLen = 0;
    if (CryptStringToBinaryA(text, (DWORD)strlen(text), CRYPT_STRING_HEX, NULL, &dwLen, NULL, NULL)==FALSE)
        return NULL;

    std::unique_ptr<BYTE[]> strIn(new BYTE[dwLen + 1]);
    if (CryptStringToBinaryA(text, (DWORD)strlen(text), CRYPT_STRING_HEX, strIn.get(), &dwLen, NULL, NULL)==FALSE)
        return NULL;

    DATA_BLOB blobin;
    blobin.cbData = dwLen;
    blobin.pbData = strIn.get();
    LPWSTR descr;
    DATA_BLOB blobout = {0};
    if (CryptUnprotectData(&blobin, &descr, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blobout)==FALSE)
        return NULL;
    SecureZeroMemory(blobin.pbData, blobin.cbData);

    std::unique_ptr<char[]> result(new char[blobout.cbData + 1]);
    strncpy_s(result.get(), blobout.cbData+1, (const char*)blobout.pbData, blobout.cbData);
    SecureZeroMemory(blobout.pbData, blobout.cbData);
    LocalFree(blobout.pbData);
    LocalFree(descr);
    return result;
}

std::unique_ptr<wchar_t[]> CStringUtils::Decrypt(const wchar_t * text)
{
    DWORD dwLen = 0;
    if (CryptStringToBinaryW(text, (DWORD)wcslen(text), CRYPT_STRING_HEX, NULL, &dwLen, NULL, NULL)==FALSE)
        return NULL;

    std::unique_ptr<BYTE[]> strIn(new BYTE[dwLen + 1]);
    if (CryptStringToBinaryW(text, (DWORD)wcslen(text), CRYPT_STRING_HEX, strIn.get(), &dwLen, NULL, NULL)==FALSE)
        return NULL;

    DATA_BLOB blobin;
    blobin.cbData = dwLen;
    blobin.pbData = strIn.get();
    LPWSTR descr;
    DATA_BLOB blobout = {0};
    if (CryptUnprotectData(&blobin, &descr, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blobout)==FALSE)
        return NULL;
    SecureZeroMemory(blobin.pbData, blobin.cbData);

    std::unique_ptr<wchar_t[]> result(new wchar_t[(blobout.cbData) / sizeof(wchar_t) + 1]);
    wcsncpy_s(result.get(), (blobout.cbData)/sizeof(wchar_t)+1, (const wchar_t*)blobout.pbData, blobout.cbData/sizeof(wchar_t));
    SecureZeroMemory(blobout.pbData, blobout.cbData);
    LocalFree(blobout.pbData);
    LocalFree(descr);
    return result;
}

CStringA CStringUtils::Encrypt( const char * text )
{
    DATA_BLOB blobin = {0};
    DATA_BLOB blobout = {0};
    CStringA result;

    blobin.cbData = (DWORD)strlen(text);
    blobin.pbData = (BYTE*) (LPCSTR)text;
    if (CryptProtectData(&blobin, L"TSVNAuth", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blobout)==FALSE)
        return result;
    DWORD dwLen = 0;
    if (CryptBinaryToStringA(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX, NULL, &dwLen)==FALSE)
        return result;
    std::unique_ptr<char[]> strOut(new char[dwLen + 1]);
    if (CryptBinaryToStringA(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX, strOut.get(), &dwLen)==FALSE)
        return result;
    LocalFree(blobout.pbData);

    result = strOut.get();

    return result;
}

CStringW CStringUtils::Encrypt( const wchar_t * text )
{
    DATA_BLOB blobin = {0};
    DATA_BLOB blobout = {0};
    CStringW result;

    blobin.cbData = (DWORD)wcslen(text)*sizeof(wchar_t);
    blobin.pbData = (BYTE*) (LPCWSTR)text;
    if (CryptProtectData(&blobin, L"TSVNAuth", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &blobout)==FALSE)
        return result;
    DWORD dwLen = 0;
    if (CryptBinaryToStringW(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX, NULL, &dwLen)==FALSE)
        return result;
    std::unique_ptr<wchar_t[]> strOut(new wchar_t[dwLen + 1]);
    if (CryptBinaryToStringW(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX, strOut.get(), &dwLen)==FALSE)
        return result;
    LocalFree(blobout.pbData);

    result = strOut.get();

    return result;
}

BYTE HexLookup[513] = {
    "000102030405060708090a0b0c0d0e0f"
    "101112131415161718191a1b1c1d1e1f"
    "202122232425262728292a2b2c2d2e2f"
    "303132333435363738393a3b3c3d3e3f"
    "404142434445464748494a4b4c4d4e4f"
    "505152535455565758595a5b5c5d5e5f"
    "606162636465666768696a6b6c6d6e6f"
    "707172737475767778797a7b7c7d7e7f"
    "808182838485868788898a8b8c8d8e8f"
    "909192939495969798999a9b9c9d9e9f"
    "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
    "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
    "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
    "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
    "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
    "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"
};
BYTE DecLookup[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // gap before first hex digit
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       // 0123456789
    0, 0, 0, 0, 0, 0, 0,             // :;<=>?@ (gap)
    10, 11, 12, 13, 14, 15,         // ABCDEF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // GHIJKLMNOPQRS (gap)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // TUVWXYZ[/]^_` (gap)
    10, 11, 12, 13, 14, 15          // abcdef
};

std::string CStringUtils::ToHexString(BYTE* pSrc, int nSrcLen)
{
    WORD * pwHex = (WORD*)HexLookup;
    auto dest = std::make_unique<char[]>((nSrcLen * 2) + 1);
    WORD * pwDest = (WORD*)dest.get();
    for (int j = 0; j < nSrcLen; j++)
    {
        *pwDest = pwHex[*pSrc];
        pwDest++; pSrc++;
    }
    *((BYTE*)pwDest) = 0;  // terminate the string
    return std::string(dest.get());
}

bool CStringUtils::FromHexString(const std::string& src, BYTE* pDest)
{
    if (src.size() % 2)
        return false;
    for (auto it = src.cbegin(); it != src.cend(); ++it)
    {
        if ((*it < '0') || (*it > 'f'))
            return false;
        int d = DecLookup[*it] << 4;
        ++it;
        d |= DecLookup[*it];
        *pDest++ = (BYTE)d;
    }
    return true;
}

std::string CStringUtils::Encrypt(const std::string& s, const std::string& password)
{
    std::string encryptedstring;
    HCRYPTPROV hProv = NULL;
    // Get handle to user default provider.
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
    {
        HCRYPTHASH hHash = NULL;
        // Create hash object.
        if (CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash))
        {
            // Hash password string.
            DWORD dwLength = DWORD(sizeof(WCHAR)*password.size());
            if (CryptHashData(hHash, (BYTE *)password.c_str(), dwLength, 0))
            {
                // Create block cipher session key based on hash of the password.
                HCRYPTKEY hKey = NULL;
                if (CryptDeriveKey(hProv, CALG_AES_256, hHash, CRYPT_EXPORTABLE, &hKey))
                {
                    // Determine number of bytes to encrypt at a time.
                    std::string starname = "*";
                    starname += s;

                    dwLength = (DWORD)starname.size();
                    std::unique_ptr<BYTE[]> buffer(new BYTE[dwLength + 1024]);
                    memcpy(buffer.get(), starname.c_str(), dwLength);
                    // Encrypt data
                    if (CryptEncrypt(hKey, 0, true, 0, buffer.get(), &dwLength, dwLength + 1024))
                    {
                        encryptedstring = CStringUtils::ToHexString(buffer.get(), dwLength);
                    }
                    CryptDestroyKey(hKey);  // Release provider handle.
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    else
        DebugBreak();
    return encryptedstring;

}

std::string CStringUtils::Decrypt(const std::string& s, const std::string& password)
{
    std::string decryptstring;
    HCRYPTPROV hProv = NULL;
    // Get handle to user default provider.
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
    {
        HCRYPTHASH hHash = NULL;
        // Create hash object.
        if (CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash))
        {
            // Hash password string.
            DWORD dwLength = DWORD(sizeof(WCHAR)*password.size());
            if (CryptHashData(hHash, (BYTE *)password.c_str(), dwLength, 0))
            {
                HCRYPTKEY hKey = NULL;
                // Create block cipher session key based on hash of the password.
                if (CryptDeriveKey(hProv, CALG_AES_256, hHash, CRYPT_EXPORTABLE, &hKey))
                {
                    dwLength = DWORD(s.size() + 1024); // 1024 bytes should be enough for padding
                    std::unique_ptr<BYTE[]> buffer(new BYTE[dwLength]);

                    std::unique_ptr<BYTE[]> strIn(new BYTE[s.size() + 1]);
                    if (CStringUtils::FromHexString(s, strIn.get()))
                    {
                        // copy encrypted password to temporary buffer
                        memcpy(buffer.get(), strIn.get(), s.size());
                        dwLength = DWORD(s.size() / 2);
                        CryptDecrypt(hKey, 0, true, 0, (BYTE *)buffer.get(), &dwLength);
                        decryptstring = std::string((char*)buffer.get(), dwLength);
                        if (!decryptstring.empty() && (decryptstring[0] == '*'))
                        {
                            decryptstring = decryptstring.substr(1);
                        }
                        else
                            decryptstring.clear();
                    }
                    CryptDestroyKey(hKey);  // Release provider handle.
                }
            }
            CryptDestroyHash(hHash); // Destroy session key.
        }
        CryptReleaseContext(hProv, 0);
    }
    else
        DebugBreak();

    return decryptstring;
}

#define IsCharNumeric(C) (!IsCharAlpha(C) && IsCharAlphaNumeric(C))


#if defined(_DEBUG) && defined(_MFC_VER)
// Some test cases for these classes
static class StringUtilsTest
{
public:
    StringUtilsTest()
    {
        CString longline = L"this is a test of how a string can be splitted into several lines";
        CString splittedline = CStringUtils::WordWrap(longline, 10, true, false, 4);
        ATLTRACE(L"WordWrap:\n%s\n", splittedline);
        splittedline = CStringUtils::LinesWrap(longline, 10, true);
        ATLTRACE(L"LinesWrap:\n%s\n", splittedline);
        longline = L"c:\\this_is_a_very_long\\path_on_windows and of course some other words added to make the line longer";
        splittedline = CStringUtils::WordWrap(longline, 10, true, false, 4);
        ATLTRACE(L"WordWrap:\n%s\n", splittedline);
        splittedline = CStringUtils::LinesWrap(longline, 10);
        ATLTRACE(L"LinesWrap:\n%s\n", splittedline);
        longline = L"Forced failure in https://myserver.com/a_long_url_to_split PROPFIND error";
        splittedline = CStringUtils::WordWrap(longline, 20, true, false, 4);
        ATLTRACE(L"WordWrap:\n%s\n", splittedline);
        splittedline = CStringUtils::LinesWrap(longline, 20, true);
        ATLTRACE(L"LinesWrap:\n%s\n", splittedline);
        longline = L"Forced\nfailure in https://myserver.com/a_long_url_to_split PROPFIND\nerror";
        splittedline = CStringUtils::WordWrap(longline, 40, true, false, 4);
        ATLTRACE(L"WordWrap:\n%s\n", splittedline);
        splittedline = CStringUtils::LinesWrap(longline, 40);
        ATLTRACE(L"LinesWrap:\n%s\n", splittedline);
        longline = L"Failed to add file\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO1.java\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO2.java";
        splittedline = CStringUtils::WordWrap(longline, 80, true, false, 4);
        ATLTRACE(L"WordWrap:\n%s\n", splittedline);
        splittedline = CStringUtils::LinesWrap(longline);
        ATLTRACE(L"LinesWrap:\n%s\n", splittedline);
        longline = L"The commit comment is not properly formatted.\nFormat:\n  Field 1 : Field 2 : Field 3\nWhere:\nField 1 - Team Name|Triage|Merge|Goal\nField 2 - V1 Backlog Item ID|Triage Number|SVNBranch|Goal Name\nField 3 - Description of change\nExamples:\n\nTeam Gamma : B-12345 : Changed some code\n  Triage : 123 : Fixed production release bug\n  Merge : sprint0812 : Merged sprint0812 into prod\n  Goal : Implement Pre-Commit Hook : Commit message hook impl";
        splittedline = CStringUtils::LinesWrap(longline, 80);
        ATLTRACE(L"LinesWrap:\n%s\n", splittedline);
        CString widecrypt = CStringUtils::Encrypt(L"test");
        auto wide = CStringUtils::Decrypt(widecrypt);
        ATLASSERT(wcscmp(wide.get(), L"test") == 0);
        CStringA charcrypt = CStringUtils::Encrypt("test");
        auto charnorm = CStringUtils::Decrypt(charcrypt);
        ATLASSERT(strcmp(charnorm.get(), "test") == 0);

        std::string encrypted = CStringUtils::Encrypt("test", "test");
        std::string decrypted = CStringUtils::Decrypt(encrypted, "test");
        ATLASSERT(decrypted.compare("test") == 0);
    }
} StringUtilsTest;

#endif