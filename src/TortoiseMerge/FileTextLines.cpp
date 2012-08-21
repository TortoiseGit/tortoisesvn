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
    UINT32 nRet = (nValue>>16) | (nValue<<16);
    nRet = ((nRet&0xff00ff)<<8) | ((nRet>>8)&0xff00ff);
    return nRet;
    //return _byteswap_ulong(nValue);
}

CFileTextLines::CFileTextLines(void)
    : m_UnicodeType(CFileTextLines::AUTOTYPE)
    , m_LineEndings(EOL_AUTOLINE)
    , m_bReturnAtEnd(false)
{
}

CFileTextLines::~CFileTextLines(void)
{
}

CFileTextLines::UnicodeType CFileTextLines::CheckUnicodeType(LPVOID pBuffer, int cb)
{
    if (cb < 2)
        return CFileTextLines::ASCII;
    UINT32 * pVal32 = (UINT32 *)pBuffer;
    // scan the whole buffer for a 0x00000000 sequence
    // if found, we assume a binary file
    for (int i=0; i<(cb-4); i=i+4)
    {
        if (0x00000000 == *pVal32++)
            return CFileTextLines::BINARY;
    }
    pVal32 = (UINT32 *)pBuffer;
    if (*pVal32 == 0x0000FEFF)
        return CFileTextLines::UTF32_LE;
    if (*pVal32 == 0xFFFE0000)
        return CFileTextLines::UTF32_BE;
    UINT16 * pVal16 = (UINT16 *)pBuffer;
    if (*pVal16 == 0xFEFF)
        return CFileTextLines::UTF16_LE;
    if (*pVal16 == 0xFFFE)
        return CFileTextLines::UTF16_BE;
    if (cb < 3)
        return ASCII;
    UINT8 * pVal8 = (UINT8 *)(pVal16+1);
    if (*pVal16 == 0xBBEF)
    {
        if (*pVal8 == 0xBF)
            return CFileTextLines::UTF8BOM;
    }
    // check for illegal UTF8 chars
    pVal8 = (UINT8 *)pBuffer;
    for (int i=0; i<cb; ++i)
    {
        if ((*pVal8 == 0xC0)||(*pVal8 == 0xC1)||(*pVal8 >= 0xF5))
            return CFileTextLines::ASCII;
        pVal8++;
    }
    pVal8 = (UINT8 *)pBuffer;
    bool bUTF8 = false;
    bool bNonANSI = false;
    for (int i=0; i<(cb-3); ++i)
    {
        if (*pVal8 > 127)
            bNonANSI = true;
        if ((*pVal8 & 0xE0)==0xC0)
        {
            pVal8++;i++;
            if ((*pVal8 & 0xC0)!=0x80)
                return CFileTextLines::ASCII;
            bUTF8 = true;
        }
        if ((*pVal8 & 0xF0)==0xE0)
        {
            pVal8++;i++;
            if ((*pVal8 & 0xC0)!=0x80)
                return CFileTextLines::ASCII;
            pVal8++;i++;
            if ((*pVal8 & 0xC0)!=0x80)
                return CFileTextLines::ASCII;
            bUTF8 = true;
        }
        if ((*pVal8 & 0xF8)==0xF0)
        {
            pVal8++;i++;
            if ((*pVal8 & 0xC0)!=0x80)
                return CFileTextLines::ASCII;
            pVal8++;i++;
            if ((*pVal8 & 0xC0)!=0x80)
                return CFileTextLines::ASCII;
            pVal8++;i++;
            if ((*pVal8 & 0xC0)!=0x80)
                return CFileTextLines::ASCII;
            bUTF8 = true;
        }
        pVal8++;
    }
    if (bUTF8)
        return CFileTextLines::UTF8;
    if ((!bNonANSI)&&(DWORD(CRegDWORD(_T("Software\\TortoiseMerge\\UseUTF8"), FALSE))))
        return CFileTextLines::UTF8;
    return CFileTextLines::ASCII;
}


/**
    returns first EOL type
*/
EOL CFileTextLines::CheckLineEndings(wchar_t * pBuffer, int nCharCount)
{
    EOL retval = EOL_AUTOLINE;
    for (int i=0; i<nCharCount; i++)
    {
        //now search the buffer for line endings
        if (pBuffer[i] == 0x0a)
        {
            if ((i+1)<nCharCount && pBuffer[i+1] == 0x0d)
            {
                retval = EOL_LFCR;
                break;
            }
            retval = EOL_LF;
            break;
        }
        else if (pBuffer[i] == 0x0d)
        {
            if ((i+1)<nCharCount && pBuffer[i+1] == 0x0a)
            {
                retval = EOL_CRLF;
                break;
            }
            retval = EOL_CR;
            break;
        }
    }
    return retval;
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
            for (int i = 0; i<nReadChars; ++i)
            {
                p32[i] = DwordSwapBytes(p32[i]);
            }
        }

        // count chars which needs surrogate pair
        int nSurrogatePairCount = 0;
        int nOverRange = 0;
        for (int i = 0; i<nReadChars; ++i)
        {
            if (p32[i]>=0x110000)
            {
                ++nOverRange;
            }
            else if (p32[i]>=0x10000)
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
                *pOut=0xfffd;
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

    // detect line EOL
    if (m_LineEndings == EOL_AUTOLINE)
    {
        m_LineEndings = CheckLineEndings(pTextBuf, min(16*1024, nReadChars));
    }

    // fill in the lines into the array
    for (int i = 0; i<nReadChars; ++i)
    {
        if (*pTextBuf == '\r')
        {
            if ((i + 1) < nReadChars)
            {
                if (*(pTextBuf+1) == '\n')
                {
                    // crlf line ending
                    CString line(pLineStart, (int)(pTextBuf-pLineStart));
                    Add(line, EOL_CRLF);
                    pLineStart = pTextBuf+2;
                    ++pTextBuf;
                    ++i;
                }
                else
                {
                    // cr line ending
                    CString line(pLineStart, (int)(pTextBuf-pLineStart));
                    Add(line, EOL_CR);
                    pLineStart =pTextBuf+1;
                }
            }
        }
        else if (*pTextBuf == '\n')
        {
            // lf line ending
            CString line(pLineStart, (int)(pTextBuf-pLineStart));
            Add(line, EOL_LF);
            pLineStart =pTextBuf+1;
        }
        ++pTextBuf;
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
BOOL CFileTextLines::Save(const CString& sFilePath, bool bSaveAsUTF8, DWORD dwIgnoreWhitespaces /*=0*/, BOOL bIgnoreCase /*= FALSE*/, bool bBlame /*= false*/)
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
            const CString sBomT = L"\xfeff"; // BOM
            pFilter->Write(sBomT);
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
                EOL ending = GetLineEnding(i);
                if (ending == EOL_AUTOLINE)
                    ending = m_LineEndings;
                CString sEolT;
                switch (ending)
                {
                case EOL_CR:
                    sEolT = _T("\x0d");
                    break;
                case EOL_CRLF:
                case EOL_AUTOLINE:
                    sEolT = _T("\x0d\x0a");
                    break;
                case EOL_LF:
                    sEolT = _T("\x0a");
                    break;
                case EOL_LFCR:
                    sEolT = _T("\x0a\x0d");
                    break;
                }
                pFilter->Write(sEolT);
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



void CBaseFilter::Encode(const CString s)
{
    int nNeedBytes = WideCharToMultiByte(m_nCodePage, 0, (LPCTSTR)s, s.GetLength(), NULL, 0, NULL, NULL);
    m_oBuffer.SetLength(nNeedBytes);
    int nConvertedLen = WideCharToMultiByte(m_nCodePage, 0, (LPCTSTR)s, s.GetLength(), (LPSTR)m_oBuffer, m_oBuffer.GetLength(), NULL, NULL);
    if (nConvertedLen!=nNeedBytes)
    {
        m_oBuffer.Clear();
    }
}



void CUtf16leFilter::Encode(const CString s)
{
    int nNeedBytes = s.GetLength()*sizeof(TCHAR);
    m_oBuffer.SetLength(nNeedBytes);
    memcpy((void *)m_oBuffer, (LPCTSTR)s, nNeedBytes);
}



void CUtf16beFilter::Encode(const CString s)
{
    int nNeedBytes = s.GetLength()*sizeof(TCHAR);
    m_oBuffer.SetLength(nNeedBytes);
    // copy swaping BYTE order in WORDs
    LPCTSTR p_In = (LPCTSTR)s;
    wchar_t * p_Out = (wchar_t *)(LPCSTR)m_oBuffer;
    int nWords = nNeedBytes/2;
    for (int nWord = 0; nWord<nWords; nWord++)
    {
        p_Out[nWord] = WideCharSwap(p_In[nWord]);
    }
}



void CUtf32leFilter::Encode(const CString s)
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
        { // tail surrogate
            zChar = 0xfffd; // ? mark
        }
        p_Out[nOutDword] = zChar;
    }
    m_oBuffer.SetLength(nOutDword*4); // store length reduced by surrogates
}



void CUtf32beFilter::Encode(const CString s)
{
    CUtf32leFilter::Encode(s);

    // swap BYTEs order in DWORDs
    UINT32 * p = (UINT32 *)(void *)m_oBuffer;
    int nDwords = m_oBuffer.GetLength()/4;
    for (int nDword = 0; nDword<nDwords; nDword++)
    {
        p[nDword] = DwordSwapBytes(p[nDword]);
    }
}

