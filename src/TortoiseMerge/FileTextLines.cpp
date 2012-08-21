// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2007-2012 - TortoiseSVN

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
#include "Resource.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include ".\filetextlines.h"
#include "FormatMessageWrapper.h"
#include "SmartHandle.h"

wchar_t inline WideCharSwap(wchar_t nValue)
{
    return (((nValue>> 8)) | (nValue << 8));
    //return _byteswap_ushort(nValue);
}

UINT32 inline DwordSwapBytes(UINT32 nValue)
{
    UINT32 nRet = (nValue<<16) | (nValue>>16); // swap WORDs
    nRet = ((nRet&0xff00ff)<<8) | ((nRet>>8)&0xff00ff); // swap BYTESs in WORDs
    return nRet;
    //return _byteswap_ulong(nValue);
}

UINT64 inline DwordSwapBytes(UINT64 nValue)
{
    UINT64 nRet = ((nValue&0xffff0000ffffL)<<16) | ((nValue>>16)&0xffff0000ffffL); // swap WORDs in DWORDs
    nRet = ((nRet&0xff00ff00ff00ff)<<8) | ((nRet>>8)&0xff00ff00ff00ff); // swap BYTESs in WORDs
    return nRet;
}

CFileTextLines::CFileTextLines(void)
    : m_UnicodeType(CFileTextLines::AUTOTYPE)
    , m_LineEndings(EOL_AUTOLINE)
    , m_bReturnAtEnd(false)
    , m_bNeedsConversion(false)
{
}

CFileTextLines::~CFileTextLines(void)
{
}

CFileTextLines::UnicodeType CFileTextLines::CheckUnicodeType(LPVOID pBuffer, int cb)
{
    if (cb < 2)
        return CFileTextLines::ASCII;
    const UINT32 * const pVal32 = (UINT32 *)pBuffer;
    const UINT16 * const pVal16 = (UINT16 *)pBuffer;
    const UINT8 * const pVal8 = (UINT8 *)pBuffer;
    // scan the whole buffer for a 0x00000000 sequence
    // if found, we assume a binary file
    int nDwords = cb/4;
    for (int i=0; i<nDwords; ++i)
    {
        if (0x00000000 == pVal32[i])
            return CFileTextLines::BINARY;
    }
    if (cb >=4 )
    {
        if (*pVal32 == 0x0000FEFF)
        {
            m_bNeedsConversion = true;
            return CFileTextLines::UTF32_LE;
        }
        if (*pVal32 == 0xFFFE0000)
        {
            m_bNeedsConversion = true;
            return CFileTextLines::UTF32_BE;
        }
    }
    if (*pVal16 == 0xFEFF)
    {
        m_bNeedsConversion = true;
        return CFileTextLines::UTF16_LE;
    }
    if (*pVal16 == 0xFFFE)
    {
        m_bNeedsConversion = true;
        return CFileTextLines::UTF16_BE;
    }
    if (cb < 3)
        return CFileTextLines::ASCII;
    if (*pVal16 == 0xBBEF)
    {
        if (pVal8[2] == 0xBF)
            return CFileTextLines::UTF8BOM;
    }
    // check for illegal UTF8 sequences
    bool bUTF8 = false;
    bool bNonANSI = false;
    int nNeedData = 0;
    for (int i=0; i<cb; ++i)
    {
        if ((pVal8[i] & 0x80)==0) // Ascii
        {
            if (nNeedData)
            {
                return CFileTextLines::ASCII;
            }
            continue;
        }
        bNonANSI = true;
        if ((pVal8[i] & 0x40)==0) // top bit
        {
            if (!nNeedData)
                return CFileTextLines::ASCII;
            --nNeedData;
            bUTF8 |= (nNeedData==0);
        }
        else if (nNeedData)
        {
            return CFileTextLines::ASCII;
        }
        else if ((pVal8[i] & 0x20)==0) // top two bits
        {
            nNeedData = 1;
        }
        else if ((pVal8[i] & 0x10)==0) // top three bits
        {
            nNeedData = 2;
        }
        else if ((pVal8[i] & 0x08)==0) // top four bits
        {
            nNeedData = 3;
        }
    }
    if (bUTF8)
        return CFileTextLines::UTF8;
    if ((!bNonANSI)&&(DWORD(CRegDWORD(_T("Software\\TortoiseMerge\\UseUTF8"), FALSE))))
        return CFileTextLines::UTF8;
    return CFileTextLines::ASCII;
}


/**
    can throw on memory allocation
*/
bool ConvertToWideChar(const int nCodePage
        , const LPCSTR & pFileBuf
        , const DWORD dwReadBytes
        , wchar_t * & pTextBuf
        , int & nReadChars)
{
#define UTF16LE 1200 // Unicode UTF-16, little endian byte order
#define UTF16BE 1201 // Unicode UTF-16, big endian byte order
#define UTF32LE 12000 // Unicode UTF-32, little endian byte order
#define UTF32BE 12001 // Unicode UTF-32, big endian byte order
    if (nCodePage == UTF32BE || nCodePage == UTF32LE)
    {
        // UTF32 have four bytes per char
        nReadChars = dwReadBytes/4;
        UINT32 * p32 = (UINT32 *)pFileBuf;
        if (nCodePage == UTF32BE)
        {
            // swap the bytes to little-endian order
            UINT64 * p64 = (UINT64 *)p32;
            int nQwords = nReadChars/2;
            for (int nQword = 0; nQword<nQwords; nQword++)
            {
                p64[nQword] = DwordSwapBytes(p64[nQword]);
            }

            for (int nDword = nQwords*2; nDword<nReadChars; nDword++)
            {
                p32[nDword] = DwordSwapBytes(p32[nDword]);
            }
        }

        // count chars which needs surrogate pair
        int nSurrogatePairCount = 0;
        for (int i = 0; i<nReadChars; ++i)
        {
            if (p32[i]<0x110000 && p32[i]>=0x10000)
            {
                ++nSurrogatePairCount;
            }
        }

        // fill buffer
        pTextBuf = new wchar_t[nReadChars+nSurrogatePairCount];
        wchar_t * pOut = pTextBuf;
        for (int i = 0; i<nReadChars; ++i, ++pOut)
        {
            __int32 zChar = p32[i]; 
            if (zChar>=0x110000)
            {
                *pOut=0xfffd; // ? mark
            }
            else if (zChar>=0x10000)
            {
                zChar-=0x10000;
                pOut[0] = ((zChar>>10)&0x3ff) | 0xd800; // lead surrogate
                pOut[1] = (zChar&0x7ff) | 0xdc00; // trail surrogate
                pOut++;
            }
            else 
            {
                *pOut = (wchar_t)zChar;
            }
        }

        nReadChars+=nSurrogatePairCount;
        return TRUE;
    }

    if ((nCodePage == UTF16LE)||(nCodePage == UTF16BE))
    {
        // UTF16 have two bytes per char
        nReadChars = dwReadBytes/2;
        pTextBuf = (wchar_t *)pFileBuf;
        if (nCodePage == UTF16BE)
        {
            // swap the bytes to little-endian order to get proper strings in wchar_t format
            for (int i = 0; i<nReadChars; ++i)
            {
                pTextBuf[i] = WideCharSwap(pTextBuf[i]);
            }
        }
        return TRUE;
    }

    int nFlags = (nCodePage==CP_ACP) ? MB_PRECOMPOSED : 0;
    nReadChars = MultiByteToWideChar(nCodePage, nFlags, pFileBuf, dwReadBytes, NULL, 0);
    pTextBuf = new wchar_t[nReadChars];
    int ret2 = MultiByteToWideChar(nCodePage, nFlags, pFileBuf, dwReadBytes, pTextBuf, nReadChars);
    if (ret2 != nReadChars)
    {
        return FALSE;
    }
    return TRUE;
}


BOOL CFileTextLines::Load(const CString& sFilePath, int lengthHint /* = 0*/)
{
    WCHAR exceptionError[1000] = {0};
    m_LineEndings = EOL_AUTOLINE;
    m_UnicodeType = CFileTextLines::AUTOTYPE;
    RemoveAll();
    m_endings.clear();
    if(lengthHint != 0)
    {
        Reserve(lengthHint);
    }

    if (PathIsDirectory(sFilePath))
    {
        m_sErrorString.Format(IDS_ERR_FILE_NOTAFILE, (LPCTSTR)sFilePath);
        return FALSE;
    }

    if (!PathFileExists(sFilePath))
    {
        //file does not exist, so just return SUCCESS
        return TRUE;
    }

    CAutoFile hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
    if (!hFile)
    {
        SetErrorString();
        return FALSE;
    }

    LARGE_INTEGER fsize;
    if (!GetFileSizeEx(hFile, &fsize))
    {
        SetErrorString();
        return false;
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
    wchar_t* pFileBuf = nullptr;
    try
    {
        pFileBuf = new wchar_t[fsize.LowPart/sizeof(wchar_t) + 1];
    }
    catch (CMemoryException* e)
    {
        e->GetErrorMessage(exceptionError, _countof(exceptionError));
        m_sErrorString = exceptionError;
        return FALSE;
    }

    // load file
    DWORD dwReadBytes = 0;
    if (!ReadFile(hFile, pFileBuf, fsize.LowPart, &dwReadBytes, NULL))
    {
        delete [] pFileBuf;
        SetErrorString();
        return FALSE;
    }
    hFile.CloseHandle();

    // detect type
    if (m_UnicodeType == CFileTextLines::AUTOTYPE)
    {
        m_UnicodeType = this->CheckUnicodeType(pFileBuf, dwReadBytes);
    }

    if (m_UnicodeType == CFileTextLines::BINARY)
    {
        m_sErrorString.Format(IDS_ERR_FILE_BINARY, (LPCTSTR)sFilePath);
        delete [] pFileBuf;
        return FALSE;
    }

    // convert text to UTF16LE
    // we may have to convert the file content
    int nReadChars = dwReadBytes/2;
    wchar_t * pTextBuf = pFileBuf;
    try
    {
        int nSourceCodePage = 0;
        switch (m_UnicodeType) 
        {
        case UTF8:
        case UTF8BOM:
            nSourceCodePage = CP_UTF8;
            break;
        case ASCII:
            nSourceCodePage = CP_ACP;
            break;
        case UTF16_BE:
            nSourceCodePage = UTF16BE;
            break;
        case UTF16_LE:
            nSourceCodePage = UTF16LE;
            break;
        case UTF32_BE:
            nSourceCodePage = UTF32BE;
            break;
        case UTF32_LE:
            nSourceCodePage = UTF32LE;
            break;
        }
        if (!ConvertToWideChar(nSourceCodePage, (LPCSTR)pFileBuf, dwReadBytes, pTextBuf, nReadChars))
        {
            return FALSE;
        }
    }
    catch (CMemoryException* e)
    {
        e->GetErrorMessage(exceptionError, _countof(exceptionError));
        m_sErrorString = exceptionError;
        delete [] pFileBuf;
        return FALSE;
    }
    if (pTextBuf!=pFileBuf)
    {
         delete [] pFileBuf;
    }
    pFileBuf = NULL;


    wchar_t * pTextStart = pTextBuf;
    wchar_t * pLineStart = pTextBuf;
    if ((m_UnicodeType == UTF8BOM)
        || (m_UnicodeType == UTF16_LE)
        || (m_UnicodeType == UTF16_BE)
        || (m_UnicodeType == UTF32_LE)
        || (m_UnicodeType == UTF32_BE))
    {
        // ignore the BOM
        ++pTextBuf;
        ++pLineStart;
        --nReadChars;
    }

    // fill in the lines into the array
    size_t countEOL_CR   = 0;
    size_t countEOL_CRLF = 0;
    size_t countEOL_LF   = 0;
    size_t countEOL_LFCR = 0;
    size_t countEOL_VT   = 0;
    size_t countEOL_FF   = 0;
    size_t countEOL_NEL  = 0;
    size_t countEOL_LS   = 0;
    size_t countEOL_PS   = 0;

    EOL eEol;
    int nEolAddBytes = 0;
    for (int i = nReadChars; i; --i)
    {
        switch (*pTextBuf++)
        {
        case '\r':
            if ((i > 1) && *(pTextBuf) == '\n')
            {
                // crlf line ending
                eEol = EOL_CRLF;
                ++countEOL_CRLF;
                nEolAddBytes = 1;
            }
            else
            {
                // cr line ending
                eEol = EOL_CR;
                ++countEOL_CR;
            }
            break;
        case '\n':
            if ((i > 1) && *(pTextBuf) == '\r')
            {
                // lfcr line ending
                eEol = EOL_LFCR;
                ++countEOL_LFCR;
                nEolAddBytes = 1;
            }
            else
            {
                // lf line ending
                eEol = EOL_LF;
                ++countEOL_LF;
            }
            break;
        case 0x000b:
            {
                eEol = EOL_VT;
                ++countEOL_VT;
            }
            break;
        case 0x000c:
            {
                eEol = EOL_FF;
                ++countEOL_FF;
            }
            break;
        case 0x0085:
            {
                eEol = EOL_NEL;
                ++countEOL_NEL;
            }
            break;
        case 0x2028:
            {
                eEol = EOL_LS;
                ++countEOL_LS;
            }
            break;
        case 0x2029:
            {
                eEol = EOL_PS;
                ++countEOL_PS;
            }
            break;
        default:
            continue;
        }
        CString line(pLineStart, (int)(pTextBuf-pLineStart)-1);
        Add(line, eEol);
        pLineStart = pTextBuf+=nEolAddBytes;
        i-=nEolAddBytes;
        nEolAddBytes = 0;
    }
    // some EOLs are not supported by the svn diff lib.
    m_bNeedsConversion |= (countEOL_LFCR!=0);
    m_bNeedsConversion |= (countEOL_FF!=0);  // LFCR EOLs are not supported by the svn diff lib.
    m_bNeedsConversion |= (countEOL_VT!=0);  // LFCR EOLs are not supported by the svn diff lib.
    m_bNeedsConversion |= (countEOL_NEL!=0);  // LFCR EOLs are not supported by the svn diff lib.
    m_bNeedsConversion |= (countEOL_LS!=0);  // LFCR EOLs are not supported by the svn diff lib.
    m_bNeedsConversion |= (countEOL_PS!=0);

    size_t eolmax = countEOL_CRLF;
    m_LineEndings = EOL_CRLF;
    if (eolmax < countEOL_CR)
    {
        eolmax = countEOL_CR;
        m_LineEndings = EOL_CR;
    }
    if (eolmax < countEOL_LF)
    {
        eolmax = countEOL_LF;
        m_LineEndings = EOL_LF;
    }
    if (eolmax < countEOL_LFCR)
    {
        eolmax = countEOL_LFCR;
        m_LineEndings = EOL_LFCR;
    }
    if (eolmax < countEOL_VT)
    {
        eolmax = countEOL_VT;
        m_LineEndings = EOL_VT;
    }
    if (eolmax < countEOL_FF)
    {
        eolmax = countEOL_FF;
        m_LineEndings = EOL_FF;
    }
    if (eolmax < countEOL_NEL)
    {
        eolmax = countEOL_NEL;
        m_LineEndings = EOL_NEL;
    }
    if (eolmax < countEOL_LS)
    {
        eolmax = countEOL_LS;
        m_LineEndings = EOL_LS;
    }
    if (eolmax < countEOL_PS)
    {
        eolmax = countEOL_PS;
        m_LineEndings = EOL_PS;
    }
    if (pLineStart < pTextBuf)
    {
        CString line(pLineStart, (int)(pTextBuf-pLineStart));
        Add(line, EOL_NOENDING);
        m_bReturnAtEnd = false;
    }
    else
        m_bReturnAtEnd = true;

    delete [] pTextStart;

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
        sLine.TrimLeft(_T(" \t"));
        sLine.TrimRight(_T(" \t"));
        break;
    case 2:
        // Ignore leading whitespace
        sLine.TrimLeft(_T(" \t"));
        break;
    case 3:
        // Ignore ending whitespace
        sLine.TrimRight(_T(" \t"));
        break;
    }
}

/**
    Encoding pattern:
        - encode & save BOM
        - Get Line
        - modify line - whitespaces, lowercase
        - encode & save line
        - get eol
        - encode & save eol
*/
BOOL CFileTextLines::Save(const CString& sFilePath, bool bSaveAsUTF8, bool bUseLF_EOLs, DWORD dwIgnoreWhitespaces /*=0*/, BOOL bIgnoreCase /*= FALSE*/, bool bBlame /*= false*/)
{
    try
    {
        CString destPath = sFilePath;
        // now make sure that the destination directory exists
        int ind = 0;
        while (destPath.Find('\\', ind)>=2)
        {
            if (!PathIsDirectory(destPath.Left(destPath.Find('\\', ind))))
            {
                if (!CreateDirectory(destPath.Left(destPath.Find('\\', ind)), NULL))
                    return FALSE;
            }
            ind = destPath.Find('\\', ind)+1;
        }

        CStdioFile file;            // Hugely faster than CFile for big file writes - because it uses buffering
        if (!file.Open(sFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
        {
            m_sErrorString.Format(IDS_ERR_FILE_OPEN, (LPCTSTR)sFilePath);
            return FALSE;
        }

        CBaseFilter * pFilter = NULL;
        bool bSaveBom = true;
        CFileTextLines::UnicodeType eUnicodeType = bSaveAsUTF8 ? CFileTextLines::UTF8 : m_UnicodeType;
        switch (eUnicodeType)
        {
        default:
        case CFileTextLines::ASCII:
        case CFileTextLines::AUTOTYPE:
            bSaveBom = false;
            pFilter = new CAsciiFilter(&file);
            break;
        case CFileTextLines::UTF8:
            bSaveBom = false;
        case CFileTextLines::UTF8BOM:
            pFilter = new CUtf8Filter(&file);
            break;
        case CFileTextLines::UTF16_BE:
            pFilter = new CUtf16beFilter(&file);
            break;
        case CFileTextLines::UTF16_LE:
            pFilter = new CUtf16leFilter(&file);
            break;
        case CFileTextLines::UTF32_BE:
            pFilter = new CUtf32beFilter(&file);
            break;
        case CFileTextLines::UTF32_LE:
            pFilter = new CUtf32leFilter(&file);
            break;
        }

        if (bSaveBom)
        {
            //first write the BOM
            pFilter->Write(L"\xfeff");
        }
        // cache EOLs
        CBuffer oCr = pFilter->Encode(_T("\x0d"));
        CBuffer oCrLf = pFilter->Encode(_T("\x0d\x0a"));
        CBuffer oLf = pFilter->Encode(_T("\x0a"));
        CBuffer oLfCr = pFilter->Encode(_T("\x0a\x0d"));
        CBuffer oVt = pFilter->Encode(_T("\x0b"));
        CBuffer oFf = pFilter->Encode(_T("\x0c"));
        CBuffer oNel = pFilter->Encode(_T("\x85"));
        CBuffer oLs = pFilter->Encode(_T("\x2028"));
        CBuffer oPs = pFilter->Encode(_T("\x2029"));
        CBuffer oAuto = oCrLf;
        switch (m_LineEndings)
        {
        case EOL_CR:
            oAuto = oCr;
            break;
        case EOL_CRLF:
        case EOL_AUTOLINE:
            oAuto = oCrLf;
            break;
        case EOL_LF:
            oAuto = oLf;
            break;
        case EOL_LFCR:
            oAuto = oLfCr;
            break;
        case EOL_VT:
            oAuto = oVt;
            break;
        case EOL_FF:
            oAuto = oFf;
            break;
        case EOL_NEL:
            oAuto = oNel;
            break;
        case EOL_LS:
            oAuto = oLs;
            break;
        case EOL_PS:
            oAuto = oPs;
            break;
        }

        for (int i=0; i<GetCount(); i++)
        {
            CString sLineT = GetAt(i);
            StripWhiteSpace(sLineT, dwIgnoreWhitespaces, bBlame);
            if (bIgnoreCase)
                sLineT = sLineT.MakeLower();
            pFilter->Write(sLineT);

            if ((m_bReturnAtEnd)||(i != GetCount()-1))
            {
                if (bUseLF_EOLs)
                    pFilter->Write(oLf);
                else
                {
                    switch (GetLineEnding(i))
                    {
                    case EOL_CR:
                        pFilter->Write(oCr);
                        break;
                    case EOL_CRLF:
                        pFilter->Write(oCrLf);
                        break;
                    case EOL_AUTOLINE:
                        pFilter->Write(oAuto);
                        break;
                    case EOL_LF:
                        pFilter->Write(oLf);
                        break;
                    case EOL_LFCR:
                        pFilter->Write(oLfCr);
                        break;
                    case EOL_VT:
                        pFilter->Write(oVt);
                        break;
                    case EOL_FF:
                        pFilter->Write(oFf);
                        break;
                    case EOL_NEL:
                        pFilter->Write(oNel);
                        break;
                    case EOL_LS:
                        pFilter->Write(oLs);
                        break;
                    case EOL_PS:
                        pFilter->Write(oPs);
                        break;
                    }
                }
            }
        }
        file.Close();
    }
    catch (CException * e)
    {
        e->GetErrorMessage(m_sErrorString.GetBuffer(4096), 4096);
        m_sErrorString.ReleaseBuffer();
        e->Delete();
        return FALSE;
    }
    return TRUE;
}

void CFileTextLines::SetErrorString()
{
    m_sErrorString = CFormatMessageWrapper();
}

void CFileTextLines::CopySettings(CFileTextLines * pFileToCopySettingsTo)
{
    if (pFileToCopySettingsTo)
    {
        pFileToCopySettingsTo->m_UnicodeType = m_UnicodeType;
        pFileToCopySettingsTo->m_LineEndings = m_LineEndings;
        pFileToCopySettingsTo->m_bReturnAtEnd = m_bReturnAtEnd;
    }
}



void CBuffer::ExpandToAtLeast(int nNewSize)
{ 
    if (nNewSize>m_nAllocated)
    {
        delete [] m_pBuffer; 
        nNewSize+=2048-1;
        nNewSize&=~(1024-1);
        m_pBuffer=new BYTE[nNewSize]; 
        m_nAllocated=nNewSize; 
    } 
}

void CBuffer::SetLength(int nUsed)
{
    ExpandToAtLeast(nUsed);
    m_nUsed = nUsed; 
}

void CBuffer::Copy(const CBuffer & Src)
{
    if (&Src != this)
    {
        delete [] m_pBuffer;
        m_nAllocated=Src.m_nAllocated;
        m_pBuffer=new BYTE[m_nAllocated];
        memcpy(m_pBuffer, Src.m_pBuffer, m_nAllocated);
        m_nUsed=Src.m_nUsed;
    }
}



const CBuffer & CBaseFilter::Encode(const CString s)
{
    m_oBuffer.SetLength(s.GetLength()*3+1); // set buffer to guessed max size
    int nConvertedLen = WideCharToMultiByte(m_nCodePage, 0, (LPCTSTR)s, s.GetLength(), (LPSTR)m_oBuffer, m_oBuffer.GetLength(), NULL, NULL);
    m_oBuffer.SetLength(nConvertedLen); // set buffer to used size
    return m_oBuffer;
}



const CBuffer & CUtf16leFilter::Encode(const CString s)
{
    int nNeedBytes = s.GetLength()*sizeof(TCHAR);
    m_oBuffer.SetLength(nNeedBytes);
    memcpy((void *)m_oBuffer, (LPCTSTR)s, nNeedBytes);
    return m_oBuffer;
}



const CBuffer & CUtf16beFilter::Encode(const CString s)
{
    int nNeedBytes = s.GetLength()*sizeof(TCHAR);
    m_oBuffer.SetLength(nNeedBytes);
    // copy swaping BYTE order in WORDs
    UINT64 * p_qwIn = (UINT64 *)(LPCTSTR)s;
    UINT64 * p_qwOut = (UINT64 *)(void *)m_oBuffer;
    int nQwords = nNeedBytes/8;
    for (int nQword = 0; nQword<nQwords; nQword++) {
        UINT64 nTemp = p_qwIn[nQword];
        p_qwOut[nQword] = ((nTemp&0xff00ff00ff00ffL)<<8) | ((nTemp>>8)&0xff00ff00ff00ffL);
    }
    wchar_t * p_wIn = (wchar_t *)p_qwIn;
    wchar_t * p_wOut = (wchar_t *)p_qwOut;
    int nWords = nNeedBytes/2;
    for (int nWord = nQwords*4; nWord<nWords; nWord++) {
        p_wOut[nWord] = WideCharSwap(p_wIn[nWord]);
    }
    return m_oBuffer;
}



const CBuffer & CUtf32leFilter::Encode(const CString s)
{
    int nInWords = s.GetLength();
    m_oBuffer.SetLength(nInWords*2);

    LPCTSTR p_In = (LPCTSTR)s;
    UINT32 * p_Out = (UINT32 *)(void *)m_oBuffer;
    int nOutDword = 0;
    for (int nInWord = 0; nInWord<nInWords; nInWord++, nOutDword++)
    {
        UINT32 zChar = p_In[nInWord];
        if ((zChar&0xfc00) == 0xd800) // lead surrogate
        {
            if (nInWord+1<nInWords && (p_In[nInWord+1]&0xfc00) == 0xdc00) // trail surrogate follows
            {
                zChar = 0x10000 + ((zChar&0x3ff)<<10) + (p_In[++nInWord]&0x3ff);
            }
            else
            {
                zChar = 0xfffd; // ? mark
            }
        }
        else if ((zChar&0xfc00) == 0xdc00) // trail surrogate without lead
        {
            zChar = 0xfffd; // ? mark
        }
        p_Out[nOutDword] = zChar;
    }
    m_oBuffer.SetLength(nOutDword*4); // store length reduced by surrogates
    return m_oBuffer;
}



const CBuffer & CUtf32beFilter::Encode(const CString s)
{
    CUtf32leFilter::Encode(s);

    // swap BYTEs order in DWORDs
    UINT64 * p64 = (UINT64 *)(void *)m_oBuffer;
    int nQwords = m_oBuffer.GetLength()/8;
    for (int nQword = 0; nQword<nQwords; nQword++)
    {
        p64[nQword] = DwordSwapBytes(p64[nQword]);
    }

    UINT32 * p32 = (UINT32 *)p64;
    int nDwords = m_oBuffer.GetLength()/4;
    for (int nDword = nQwords*2; nDword<nDwords; nDword++)
    {
        p32[nDword] = DwordSwapBytes(p32[nDword]);
    }
    return m_oBuffer;
}

