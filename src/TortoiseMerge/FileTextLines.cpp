// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2007-2016, 2019, 2021 - TortoiseSVN

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
#include "resource.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "FileTextLines.h"
#include "FormatMessageWrapper.h"
#include "SmartHandle.h"

wchar_t inline WideCharSwap(wchar_t nValue)
{
    return (((nValue >> 8)) | (nValue << 8));
}

UINT64 inline WordSwapBytes(UINT64 nValue)
{
    return ((nValue & 0xff00ff00ff00ff) << 8) | ((nValue >> 8) & 0xff00ff00ff00ff); // swap BYTESs in WORDs
}

UINT32 inline DwordSwapBytes(UINT32 nValue)
{
    UINT32 nRet = (nValue << 16) | (nValue >> 16);                     // swap WORDs
    nRet        = ((nRet & 0xff00ff) << 8) | ((nRet >> 8) & 0xff00ff); // swap BYTESs in WORDs
    return nRet;
}

UINT64 inline DwordSwapBytes(UINT64 nValue)
{
    UINT64 nRet = ((nValue & 0xffff0000ffffL) << 16) | ((nValue >> 16) & 0xffff0000ffffL); // swap WORDs in DWORDs
    nRet        = ((nRet & 0xff00ff00ff00ff) << 8) | ((nRet >> 8) & 0xff00ff00ff00ff);     // swap BYTESs in WORDs
    return nRet;
}

CFileTextLines::CFileTextLines()
    : m_bNeedsConversion(false)
    , m_bKeepEncoding(false)
{
    m_saveParams.unicodeType = CFileTextLines::AUTOTYPE;
    m_saveParams.lineEndings = EOL_AUTOLINE;
}

CFileTextLines::~CFileTextLines()
{
}

CFileTextLines::UnicodeType CFileTextLines::CheckUnicodeType(LPVOID pBuffer, int cb)
{
    if (cb < 2)
        return CFileTextLines::ASCII;
    const UINT32* const pVal32 = static_cast<UINT32*>(pBuffer);
    const UINT16* const pVal16 = static_cast<UINT16*>(pBuffer);
    const UINT8* const  pVal8  = static_cast<UINT8*>(pBuffer);
    // scan the whole buffer for a 0x00000000 sequence
    // if found, we assume a binary file
    int nDwords = cb / 4;
    for (int j = 0; j < nDwords; ++j)
    {
        if (0x00000000 == pVal32[j])
            return CFileTextLines::BINARY;
    }
    if (cb >= 4)
    {
        if (*pVal32 == 0x0000FEFF)
        {
            return CFileTextLines::UTF32_LE;
        }
        if (*pVal32 == 0xFFFE0000)
        {
            return CFileTextLines::UTF32_BE;
        }
    }
    if (*pVal16 == 0xFEFF)
    {
        return CFileTextLines::UTF16_LEBOM;
    }
    if (*pVal16 == 0xFFFE)
    {
        return CFileTextLines::UTF16_BEBOM;
    }
    if (cb < 3)
        return CFileTextLines::ASCII;
    if (*pVal16 == 0xBBEF)
    {
        if (pVal8[2] == 0xBF)
            return CFileTextLines::UTF8BOM;
    }
    // check for illegal UTF8 sequences
    bool bNonANSI  = false;
    int  nNeedData = 0;
    int  i         = 0;
    int  nullCount = 0;
    for (; i < cb; ++i)
    {
        if (pVal8[i] == 0)
        {
            ++nullCount;
            // count the null chars, we do not want to treat an ASCII/UTF8 file
            // as UTF16 just because of some null chars that might be accidentally
            // in the file.
            // Use an arbitrary value of one fiftieth of the file length as
            // the limit after which a file is considered UTF16.
            if (nullCount > (cb / 50))
            {
                // null-chars are not allowed for ASCII or UTF8, that means
                // this file is most likely UTF16 encoded
                if (i % 2)
                    return CFileTextLines::UTF16_LE;
                else
                    return CFileTextLines::UTF16_BE;
            }
        }
        if ((pVal8[i] & 0x80) != 0) // non ASCII
        {
            bNonANSI = true;
            break;
        }
    }
    // check remaining text for UTF-8 validity
    for (; i < cb; ++i)
    {
        UINT8 zChar = pVal8[i];
        if ((zChar & 0x80) == 0) // Ascii
        {
            if (zChar == 0)
            {
                ++nullCount;
                // count the null chars, we do not want to treat an ASCII/UTF8 file
                // as UTF16 just because of some null chars that might be accidentally
                // in the file.
                // Use an arbitrary value of one fiftieth of the file length as
                // the limit after which a file is considered UTF16.
                if (nullCount > (cb / 50))
                {
                    // null-chars are not allowed for ASCII or UTF8, that means
                    // this file is most likely UTF16 encoded
                    if (i % 2)
                        return CFileTextLines::UTF16_LE;
                    else
                        return CFileTextLines::UTF16_BE;
                }
                nNeedData = 0;
            }
            else if (nNeedData)
            {
                return CFileTextLines::ASCII;
            }
            continue;
        }
        if ((zChar & 0x40) == 0) // top bit
        {
            if (!nNeedData)
                return CFileTextLines::ASCII;
            --nNeedData;
        }
        else if (nNeedData)
        {
            return CFileTextLines::ASCII;
        }
        else if ((zChar & 0x20) == 0) // top two bits
        {
            if (zChar <= 0xC1)
                return CFileTextLines::ASCII;
            nNeedData = 1;
        }
        else if ((zChar & 0x10) == 0) // top three bits
        {
            nNeedData = 2;
        }
        else if ((zChar & 0x08) == 0) // top four bits
        {
            if (zChar >= 0xf5)
                return CFileTextLines::ASCII;
            nNeedData = 3;
        }
        else
            return CFileTextLines::ASCII;
    }
    if (bNonANSI && nNeedData == 0)
        // if get here thru nonAscii and no missing data left then its valid UTF8
        return CFileTextLines::UTF8;
    if ((!bNonANSI) && static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseMerge\\UseUTF8", FALSE)))
        return CFileTextLines::UTF8;
    return CFileTextLines::ASCII;
}

BOOL CFileTextLines::Load(const CString& sFilePath, int /*lengthHint*/ /* = 0*/)
{
    WCHAR exceptionError[1000] = {0};
    m_saveParams.lineEndings   = EOL_AUTOLINE;
    if (!m_bKeepEncoding)
        m_saveParams.unicodeType = CFileTextLines::AUTOTYPE;
    RemoveAll();

    if (PathIsDirectory(sFilePath))
    {
        m_sErrorString.Format(IDS_ERR_FILE_NOTAFILE, static_cast<LPCWSTR>(sFilePath));
        return FALSE;
    }

    if (!PathFileExists(sFilePath))
    {
        //file does not exist, so just return SUCCESS
        return TRUE;
    }

    CAutoFile hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (!hFile)
    {
        SetErrorString();
        return FALSE;
    }

    LARGE_INTEGER fsize;
    if (!GetFileSizeEx(hFile, &fsize))
    {
        SetErrorString();
        return FALSE;
    }
    if (fsize.HighPart)
    {
        // file is way too big for us
        m_sErrorString.LoadString(IDS_ERR_FILE_TOOBIG);
        return FALSE;
    }

    // create buffer
    // If new[] was done for type T delete[] must be called on a pointer of type T*,
    // otherwise the behavior is undefined.
    // +1 is to address possible truncation when integer division is done
    CBuffer oFile;
    try
    {
        oFile.SetLength(fsize.LowPart);
    }
    catch (CMemoryException* e)
    {
        e->GetErrorMessage(exceptionError, _countof(exceptionError));
        m_sErrorString = exceptionError;
        return FALSE;
    }

    // load file
    DWORD dwReadBytes = 0;
    if (!ReadFile(hFile, static_cast<void*>(oFile), fsize.LowPart, &dwReadBytes, nullptr))
    {
        SetErrorString();
        return FALSE;
    }
    hFile.CloseHandle();

    // detect type
    if (m_saveParams.unicodeType == CFileTextLines::AUTOTYPE)
    {
        m_saveParams.unicodeType = this->CheckUnicodeType(static_cast<LPVOID>(oFile), dwReadBytes);
    }
    // enforce conversion for all but ASCII and UTF8 type
    m_bNeedsConversion = (m_saveParams.unicodeType != CFileTextLines::UTF8) && (m_saveParams.unicodeType != CFileTextLines::ASCII);

    // we may have to convert the file content - CString is UTF16LE
    try
    {
        CBaseFilter* pFilter = nullptr;
        switch (m_saveParams.unicodeType)
        {
            case BINARY:
                m_sErrorString.Format(IDS_ERR_FILE_BINARY, static_cast<LPCWSTR>(sFilePath));
                return FALSE;
            case UTF8:
            case UTF8BOM:
                pFilter = new CUtf8Filter(nullptr);
                break;
            default:
            case ASCII:
                pFilter = new CAsciiFilter(nullptr);
                break;
            case UTF16_BE:
            case UTF16_BEBOM:
                pFilter = new CUtf16BeFilter(nullptr);
                break;
            case UTF16_LE:
            case UTF16_LEBOM:
                pFilter = new CUtf16LeFilter(nullptr);
                break;
            case UTF32_BE:
                pFilter = new CUtf32BeFilter(nullptr);
                break;
            case UTF32_LE:
                pFilter = new CUtf32LeFilter(nullptr);
                break;
        }
        pFilter->Decode(oFile);
        delete pFilter;
    }
    catch (CMemoryException* e)
    {
        e->GetErrorMessage(exceptionError, _countof(exceptionError));
        m_sErrorString = exceptionError;
        return FALSE;
    }

    int      nReadChars = oFile.GetLength() / sizeof(wchar_t);
    wchar_t* pTextBuf   = static_cast<wchar_t*>(oFile);
    wchar_t* pLineStart = pTextBuf;
    if ((m_saveParams.unicodeType == UTF8BOM) || (m_saveParams.unicodeType == UTF16_LEBOM) || (m_saveParams.unicodeType == UTF16_BEBOM) || (m_saveParams.unicodeType == UTF32_LE) || (m_saveParams.unicodeType == UTF32_BE))
    {
        // ignore the BOM
        ++pTextBuf;
        ++pLineStart;
        --nReadChars;
    }

    // fill in the lines into the array
    size_t        countEOLs[EOL_COUNT] = {0};
    CFileTextLine oTextLine;
    for (int i = nReadChars; i; --i)
    {
        EOL eEol;
        switch (*pTextBuf++)
        {
            case '\r':
                // crlf line ending or cr line ending
                eEol = ((i > 1) && *(pTextBuf) == '\n') ? EOL_CRLF : EOL_CR;
                break;
            case '\n':
                // lfcr line ending or lf line ending
                eEol = ((i > 1) && *(pTextBuf) == '\r') ? EOL_LFCR : EOL_LF;
                if (eEol == EOL_LFCR)
                {
                    // LFCR is very rare on Windows, so we have to double check
                    // that this is not just a LF followed by CRLF
                    if (((countEOLs[EOL_CRLF] > 1) || (countEOLs[EOL_LF] > 1) || (GetCount() < 2)) &&
                        ((i > 2) && (*(pTextBuf + 1) == '\n')))
                    {
                        // change the EOL back to a simple LF
                        eEol = EOL_LF;
                    }
                }
                break;
            case 0x000b:
                eEol = EOL_VT;
                break;
            case 0x000c:
                eEol = EOL_FF;
                break;
            case 0x0085:
                eEol = EOL_NEL;
                break;
            case 0x2028:
                eEol = EOL_LS;
                break;
            case 0x2029:
                eEol = EOL_PS;
                break;
            default:
                continue;
        }
        oTextLine.sLine   = CString(pLineStart, static_cast<int>(pTextBuf - pLineStart) - 1);
        oTextLine.eEnding = eEol;
        CStdFileLineArray::Add(oTextLine);
        ++countEOLs[eEol];
        if (eEol == EOL_CRLF || eEol == EOL_LFCR)
        {
            ++pTextBuf;
            --i;
        }
        pLineStart = pTextBuf;
    }
    CString line(pLineStart, static_cast<int>(pTextBuf - pLineStart));
    Add(line, EOL_NOENDING);

    // some EOLs are not supported by the svn diff lib.
    m_bNeedsConversion |= (countEOLs[EOL_CRLF] != 0);
    m_bNeedsConversion |= (countEOLs[EOL_FF] != 0);
    m_bNeedsConversion |= (countEOLs[EOL_VT] != 0);
    m_bNeedsConversion |= (countEOLs[EOL_NEL] != 0);
    m_bNeedsConversion |= (countEOLs[EOL_LS] != 0);
    m_bNeedsConversion |= (countEOLs[EOL_PS] != 0);

    size_t eolmax = 0;
    for (int nEol = 0; nEol < EOL_COUNT; nEol++)
    {
        if (eolmax < countEOLs[nEol])
        {
            eolmax                   = countEOLs[nEol];
            m_saveParams.lineEndings = static_cast<EOL>(nEol);
        }
    }

    return TRUE;
}

void CFileTextLines::StripWhiteSpace(CString& sLine, DWORD dwIgnoreWhitespaces, bool blame)
{
    if (blame)
    {
        if (sLine.GetLength() > 66)
            sLine = sLine.Mid(66);
    }
    switch (dwIgnoreWhitespaces)
    {
        case 0:
            // Compare whitespaces
            // do nothing
            break;
        case 1:
            // Ignore all whitespaces
            sLine.TrimLeft(L" \t");
            sLine.TrimRight(L" \t");
            break;
        case 2:
            // Ignore leading whitespace
            sLine.TrimLeft(L" \t");
            break;
        case 3:
            // Ignore ending whitespace
            sLine.TrimRight(L" \t");
            break;
    }
}

/**
    Encoding pattern:
        - encode & save BOM
        - Get Line
        - modify line - whitespaces, lowercase
        - encode & save line
        - get cached encoded eol
        - save eol
*/
BOOL CFileTextLines::Save(const CString& sFilePath, bool bSaveAsUTF8 /*= false */
                          ,
                          bool bUseSVNCompatibleEOLs /*= false */
                          ,
                          DWORD dwIgnoreWhitespaces /*= 0 */
                          ,
                          BOOL bIgnoreCase /*= FALSE */
                          ,
                          bool bBlame /*= false*/
                          ,
                          bool bIgnoreComments /*= false*/
                          ,
                          const CString& lineStart /*= CString()*/
                          ,
                          const CString& blockStart /*= CString()*/
                          ,
                          const CString& blockEnd /*= CString()*/
                          ,
                          const std::wregex& rx /*= std::wregex(L"")*/
                          ,
                          const std::wstring& replacement /*=L""*/)
{
    m_sCommentLine       = lineStart;
    m_sCommentBlockStart = blockStart;
    m_sCommentBlockEnd   = blockEnd;

    try
    {
        CString destPath = sFilePath;
        // now make sure that the destination directory exists
        int ind = 0;
        while (destPath.Find('\\', ind) >= 2)
        {
            if (!PathIsDirectory(destPath.Left(destPath.Find('\\', ind))))
            {
                if (!CreateDirectory(destPath.Left(destPath.Find('\\', ind)), nullptr))
                    return FALSE;
            }
            ind = destPath.Find('\\', ind) + 1;
        }

        CStdioFile file; // Hugely faster than CFile for big file writes - because it uses buffering
        if (!file.Open(sFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary | CFile::shareDenyNone))
        {
            const_cast<CString*>(&m_sErrorString)->Format(IDS_ERR_FILE_OPEN, static_cast<LPCWSTR>(sFilePath));
            return FALSE;
        }

        CBaseFilter*                pFilter      = nullptr;
        bool                        bSaveBom     = true;
        CFileTextLines::UnicodeType eUnicodeType = bSaveAsUTF8 ? CFileTextLines::UTF8 : m_saveParams.unicodeType;
        switch (eUnicodeType)
        {
            default:
            case CFileTextLines::ASCII:
                bSaveBom = false;
                pFilter  = new CAsciiFilter(&file);
                break;
            case CFileTextLines::UTF8:
                bSaveBom = false;
                [[fallthrough]];
            case CFileTextLines::UTF8BOM:
                pFilter = new CUtf8Filter(&file);
                break;
            case CFileTextLines::UTF16_BE:
                bSaveBom = false;
                pFilter  = new CUtf16BeFilter(&file);
                break;
            case CFileTextLines::UTF16_BEBOM:
                pFilter = new CUtf16BeFilter(&file);
                break;
            case CFileTextLines::UTF16_LE:
                bSaveBom = false;
                pFilter  = new CUtf16LeFilter(&file);
                break;
            case CFileTextLines::UTF16_LEBOM:
                pFilter = new CUtf16LeFilter(&file);
                break;
            case CFileTextLines::UTF32_BE:
                pFilter = new CUtf32BeFilter(&file);
                break;
            case CFileTextLines::UTF32_LE:
                pFilter = new CUtf32LeFilter(&file);
                break;
        }

        if (bSaveBom)
        {
            //first write the BOM
            pFilter->Write(L"\xfeff");
        }
        // cache EOLs
        CBuffer oEncodedEol[EOL_COUNT];
        oEncodedEol[EOL_LF]   = pFilter->Encode(L"\n");   // x0a
        oEncodedEol[EOL_CR]   = pFilter->Encode(L"\r");   // x0d
        oEncodedEol[EOL_CRLF] = pFilter->Encode(L"\r\n"); // x0d x0a
        if (bUseSVNCompatibleEOLs)
        {
            // when using EOLs that are supported by the svn lib,
            // we have to use the same EOLs as the file has in case
            // they're already supported, but a different supported one
            // in case the original one isn't supported.
            // Only this way the option "ignore EOLs (recommended)" unchecked
            // actually shows the lines as different.
            // However, the diff won't find and differences in EOLs
            // for these special EOLs if they differ between those special ones
            // listed below.
            // But it will work properly for the most common EOLs LF/CR/CRLF.
            oEncodedEol[EOL_LFCR] = oEncodedEol[EOL_CR];
            for (int nEol = 0; nEol < EOL_NOENDING; nEol++)
            {
                if (oEncodedEol[nEol].IsEmpty())
                    oEncodedEol[nEol] = oEncodedEol[EOL_LF];
            }
        }
        else
        {
            oEncodedEol[EOL_LFCR] = pFilter->Encode(L"\n\r");
            oEncodedEol[EOL_VT]   = pFilter->Encode(L"\v"); // x0b
            oEncodedEol[EOL_FF]   = pFilter->Encode(L"\f"); // x0c
            oEncodedEol[EOL_NEL]  = pFilter->Encode(L"\x85");
            oEncodedEol[EOL_LS]   = pFilter->Encode(L"\x2028");
            oEncodedEol[EOL_PS]   = pFilter->Encode(L"\x2029");
        }
        oEncodedEol[EOL_AUTOLINE] = oEncodedEol[m_saveParams.lineEndings == EOL_AUTOLINE
                                                    ? EOL_CRLF
                                                    : m_saveParams.lineEndings];

        bool bInBlockComment = false;
        for (int i = 0; i < GetCount(); i++)
        {
            CString sLineT = GetAt(i);
            if (bIgnoreComments)
                bInBlockComment = StripComments(sLineT, bInBlockComment);
            if (!rx._Empty())
                LineRegex(sLineT, rx, replacement);
            StripWhiteSpace(sLineT, dwIgnoreWhitespaces, bBlame);
            if (bIgnoreCase)
                sLineT = sLineT.MakeLower();
            pFilter->Write(sLineT);
            EOL eEol = GetLineEnding(i);
            pFilter->Write(oEncodedEol[eEol]);
        }
        delete pFilter;
        file.Close();
    }
    catch (CException* e)
    {
        CString* psErrorString = const_cast<CString*>(&m_sErrorString);
        e->GetErrorMessage(psErrorString->GetBuffer(4096), 4096);
        psErrorString->ReleaseBuffer();
        e->Delete();
        return FALSE;
    }
    return TRUE;
}

void CFileTextLines::SetErrorString()
{
    m_sErrorString = CFormatMessageWrapper();
}

void CFileTextLines::CopySettings(CFileTextLines* pFileToCopySettingsTo) const
{
    if (pFileToCopySettingsTo)
    {
        pFileToCopySettingsTo->m_saveParams = m_saveParams;
    }
}

const wchar_t* CFileTextLines::GetEncodingName(UnicodeType eEncoding)
{
    switch (eEncoding)
    {
        case ASCII:
            return L"ASCII";
        case BINARY:
            return L"BINARY";
        case UTF16_LE:
            return L"UTF-16LE";
        case UTF16_LEBOM:
            return L"UTF-16LE BOM";
        case UTF16_BE:
            return L"UTF-16BE";
        case UTF16_BEBOM:
            return L"UTF-16BE BOM";
        case UTF32_LE:
            return L"UTF-32LE";
        case UTF32_BE:
            return L"UTF-32BE";
        case UTF8:
            return L"UTF-8";
        case UTF8BOM:
            return L"UTF-8 BOM";
        default:
            break;
    }
    return L"";
}

bool CFileTextLines::IsInsideString(const CString& sLine, int pos)
{
    int  sCount = 0;
    int  cCount = 0;
    auto sPos   = sLine.Find('"');
    while ((sPos >= 0) && (sPos < pos))
    {
        ++sCount;
        sPos = sLine.Find('"', sPos + 1);
    }
    auto cpos = sLine.Find('\'');
    while ((cpos >= 0) && (cpos < pos))
    {
        ++cCount;
        cpos = sLine.Find('"', cpos + 1);
    }
    return ((sCount % 2 != 0) || (cCount % 2 != 0));
}

bool CFileTextLines::StripComments(CString& sLine, bool bInBlockComment) const
{
    int startPos    = 0;
    int oldStartPos = -1;
    do
    {
        if (bInBlockComment)
        {
            int endPos = sLine.Find(m_sCommentBlockEnd);
            if (IsInsideString(sLine, endPos))
                endPos = -1;
            if ((endPos >= 0) && ((endPos > startPos) || (endPos == 0)))
            {
                sLine           = sLine.Left(startPos) + sLine.Mid(endPos + m_sCommentBlockEnd.GetLength());
                bInBlockComment = false;
                startPos        = endPos;
            }
            else
            {
                sLine    = sLine.Left(startPos);
                startPos = -1;
            }
        }
        if (!bInBlockComment)
        {
            startPos      = m_sCommentBlockStart.IsEmpty() ? -1 : sLine.Find(m_sCommentBlockStart, startPos);
            int startPos2 = m_sCommentLine.IsEmpty() ? -1 : sLine.Find(m_sCommentLine);
            if (((startPos2 < startPos) && (startPos2 >= 0)) ||
                ((startPos2 >= 0) && (startPos < 0)))
            {
                // line comment
                // look if there's a string marker (" or ') before that
                // note: this check is not fully correct. For example, it
                // does not account for escaped chars or even multiline strings.
                // but it has to be fast, so this has to do...
                if (!IsInsideString(sLine, startPos2))
                {
                    // line comment, erase the rest of the line
                    sLine    = sLine.Left(startPos2);
                    startPos = -1;
                }
                if (startPos == oldStartPos)
                    return false;
                oldStartPos = startPos;
            }
            else if (startPos >= 0)
            {
                // starting block comment
                if (!IsInsideString(sLine, startPos))
                    bInBlockComment = true;
                else
                    ++startPos;
            }
        }
    } while (startPos >= 0);

    return bInBlockComment;
}

void CFileTextLines::LineRegex(CString& sLine, const std::wregex& rx, const std::wstring& replacement) const
{
    std::wstring str  = static_cast<LPCWSTR>(sLine);
    std::wstring str2 = std::regex_replace(str, rx, replacement);
    sLine             = str2.c_str();
}

void CBuffer::ExpandToAtLeast(int nNewSize)
{
    if (nNewSize > m_nAllocated)
    {
        delete[] m_pBuffer; // we don't preserve buffer content intentionally
        nNewSize += 2048 - 1;
        nNewSize &= ~(1024 - 1);
        m_pBuffer    = new BYTE[nNewSize];
        m_nAllocated = nNewSize;
    }
}

void CBuffer::SetLength(int nUsed)
{
    ExpandToAtLeast(nUsed);
    m_nUsed = nUsed;
}

void CBuffer::Swap(CBuffer& src)
{
    std::swap(src.m_nAllocated, m_nAllocated);
    std::swap(src.m_pBuffer, m_pBuffer);
    std::swap(src.m_nUsed, m_nUsed);
}

void CBuffer::Copy(const CBuffer& src)
{
    if (&src != this)
    {
        SetLength(src.m_nUsed);
        memcpy(m_pBuffer, src.m_pBuffer, m_nUsed);
    }
}

bool CBaseFilter::Decode(/*in out*/ CBuffer& data)
{
    int nFlags = (m_nCodePage == CP_ACP) ? MB_PRECOMPOSED : 0;
    // dry decode is around 8 times faster then real one, alternatively we can set buffer to max length
    int nReadChars = MultiByteToWideChar(m_nCodePage, nFlags, static_cast<LPCSTR>(data), data.GetLength(), nullptr, 0);
    m_oBuffer.SetLength(nReadChars * sizeof(wchar_t));
    int ret2 = MultiByteToWideChar(m_nCodePage, nFlags, static_cast<LPCSTR>(data), data.GetLength(), static_cast<LPWSTR>(static_cast<void*>(m_oBuffer)), nReadChars);
    if (ret2 != nReadChars)
    {
        return FALSE;
    }
    data.Swap(m_oBuffer);
    return TRUE;
}

const CBuffer& CBaseFilter::Encode(const CString& s)
{
    m_oBuffer.SetLength(s.GetLength() * 3 + 1); // set buffer to guessed max size
    int nConvertedLen = WideCharToMultiByte(m_nCodePage, 0, static_cast<LPCWSTR>(s), s.GetLength(), static_cast<LPSTR>(m_oBuffer), m_oBuffer.GetLength(), nullptr, nullptr);
    m_oBuffer.SetLength(nConvertedLen); // set buffer to used size
    return m_oBuffer;
}

///< write preencoded internal buffer

// ReSharper disable once CppMemberFunctionMayBeConst
void CBaseFilter::Write(const CBuffer& buffer) 
{
    if (buffer.GetLength())
        m_pFile->Write(static_cast<void*>(buffer), buffer.GetLength());
}

bool CUtf16LeFilter::Decode(/*in out*/ CBuffer& /*data*/)
{
    // we believe data is ok for use
    return TRUE;
}

const CBuffer& CUtf16LeFilter::Encode(const CString& s)
{
    int nNeedBytes = s.GetLength() * sizeof(TCHAR);
    m_oBuffer.SetLength(nNeedBytes);
    memcpy(static_cast<void*>(m_oBuffer), static_cast<LPCWSTR>(s), nNeedBytes);
    return m_oBuffer;
}

bool CUtf16BeFilter::Decode(/*in out*/ CBuffer& data)
{
    int nNeedBytes = data.GetLength();
    // make in place WORD BYTEs swap
    UINT64* pQw     = static_cast<UINT64*>(static_cast<void*>(data));
    int     nQwords = nNeedBytes / 8;
    for (int nQword = 0; nQword < nQwords; nQword++)
    {
        pQw[nQword] = WordSwapBytes(pQw[nQword]);
    }
    wchar_t* pW     = reinterpret_cast<wchar_t*>(pQw);
    int      nWords = nNeedBytes / 2;
    for (int nWord = nQwords * 4; nWord < nWords; nWord++)
    {
        pW[nWord] = WideCharSwap(pW[nWord]);
    }
    return CUtf16LeFilter::Decode(data);
}

const CBuffer& CUtf16BeFilter::Encode(const CString& s)
{
    int nNeedBytes = s.GetLength() * sizeof(TCHAR);
    m_oBuffer.SetLength(nNeedBytes);
    // copy swaping BYTE order in WORDs
    const UINT64* pQwIn   = reinterpret_cast<const UINT64*>(static_cast<LPCWSTR>(s));
    UINT64*       pQwOut  = static_cast<UINT64*>(static_cast<void*>(m_oBuffer));
    int           nQwords = nNeedBytes / 8;
    for (int nQword = 0; nQword < nQwords; nQword++)
    {
        pQwOut[nQword] = WordSwapBytes(pQwIn[nQword]);
    }
    const wchar_t* pWIn   = reinterpret_cast<const wchar_t*>(pQwIn);
    wchar_t*       pWOut  = reinterpret_cast<wchar_t*>(pQwOut);
    int            nWords = nNeedBytes / 2;
    for (int nWord = nQwords * 4; nWord < nWords; nWord++)
    {
        pWOut[nWord] = WideCharSwap(pWIn[nWord]);
    }
    return m_oBuffer;
}

bool CUtf32LeFilter::Decode(/*in out*/ CBuffer& data)
{
    // UTF32 have four bytes per char
    int     nReadChars = data.GetLength() / 4;
    UINT32* p32        = static_cast<UINT32*>(static_cast<void*>(data));

    // count chars which needs surrogate pair
    int nSurrogatePairCount = 0;
    for (int i = 0; i < nReadChars; ++i)
    {
        if (p32[i] < 0x110000 && p32[i] >= 0x10000)
        {
            ++nSurrogatePairCount;
        }
    }

    // fill buffer
    m_oBuffer.SetLength((nReadChars + nSurrogatePairCount) * sizeof(wchar_t));
    wchar_t* pOut = static_cast<wchar_t*>(m_oBuffer);
    for (int i = 0; i < nReadChars; ++i, ++pOut)
    {
        UINT32 zChar = p32[i];
        if (zChar >= 0x110000)
        {
            *pOut = 0xfffd; // ? mark
        }
        else if (zChar >= 0x10000)
        {
            zChar -= 0x10000;
            pOut[0] = ((zChar >> 10) & 0x3ff) | 0xd800; // lead surrogate
            pOut[1] = (zChar & 0x7ff) | 0xdc00;         // trail surrogate
            pOut++;
        }
        else
        {
            *pOut = static_cast<wchar_t>(zChar);
        }
    }
    data.Swap(m_oBuffer);
    return TRUE;
}

const CBuffer& CUtf32LeFilter::Encode(const CString& s)
{
    int nInWords = s.GetLength();
    m_oBuffer.SetLength(nInWords * 2);

    LPCWSTR pIn       = static_cast<LPCWSTR>(s);
    UINT32* pOut      = static_cast<UINT32*>(static_cast<void*>(m_oBuffer));
    int     nOutDword = 0;
    for (int nInWord = 0; nInWord < nInWords; nInWord++, nOutDword++)
    {
        UINT32 zChar = pIn[nInWord];
        if ((zChar & 0xfc00) == 0xd800) // lead surrogate
        {
            if (nInWord + 1 < nInWords && (pIn[nInWord + 1] & 0xfc00) == 0xdc00) // trail surrogate follows
            {
                zChar = 0x10000 + ((zChar & 0x3ff) << 10) + (pIn[++nInWord] & 0x3ff);
            }
            else
            {
                zChar = 0xfffd; // ? mark
            }
        }
        else if ((zChar & 0xfc00) == 0xdc00) // trail surrogate without lead
        {
            zChar = 0xfffd; // ? mark
        }
        pOut[nOutDword] = zChar;
    }
    m_oBuffer.SetLength(nOutDword * 4); // store length reduced by surrogates
    return m_oBuffer;
}

bool CUtf32BeFilter::Decode(/*in out*/ CBuffer& data)
{
    // swap BYTEs order in DWORDs
    UINT64* p64     = static_cast<UINT64*>(static_cast<void*>(data));
    int     nQwords = data.GetLength() / 8;
    for (int nQword = 0; nQword < nQwords; nQword++)
    {
        p64[nQword] = DwordSwapBytes(p64[nQword]);
    }

    UINT32* p32     = reinterpret_cast<UINT32*>(p64);
    int     nDwords = data.GetLength() / 4;
    for (int nDword = nQwords * 2; nDword < nDwords; nDword++)
    {
        p32[nDword] = DwordSwapBytes(p32[nDword]);
    }
    return CUtf32LeFilter::Decode(data);
}

const CBuffer& CUtf32BeFilter::Encode(const CString& s)
{
    CUtf32LeFilter::Encode(s);

    // swap BYTEs order in DWORDs
    UINT64* p64     = static_cast<UINT64*>(static_cast<void*>(m_oBuffer));
    int     nQwords = m_oBuffer.GetLength() / 8;
    for (int nQword = 0; nQword < nQwords; nQword++)
    {
        p64[nQword] = DwordSwapBytes(p64[nQword]);
    }

    UINT32* p32     = reinterpret_cast<UINT32*>(p64);
    int     nDwords = m_oBuffer.GetLength() / 4;
    for (int nDword = nQwords * 2; nDword < nDwords; nDword++)
    {
        p32[nDword] = DwordSwapBytes(p32[nDword]);
    }
    return m_oBuffer;
}
