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
    UINT16 * pVal16 = (UINT16 *)pBuffer;
    UINT8 * pVal8 = (UINT8 *)(pVal16+1);
    // scan the whole buffer for a 0x0000 sequence
    // if found, we assume a binary file
    for (int i=0; i<(cb-2); i=i+2)
    {
        if (0x0000 == *pVal16++)
            return CFileTextLines::BINARY;
    }
    pVal16 = (UINT16 *)pBuffer;
    if (*pVal16 == 0xFEFF)
        return CFileTextLines::UNICODE_LE;
    if (*pVal16 == 0xFFFE)
        return CFileTextLines::UNICODE_BE;
    if (cb < 3)
        return ASCII;
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


bool ConvertToWideChar(const int nCodePage
        , const LPCSTR & pFileBuf
        , const DWORD dwReadBytes
        , wchar_t * & pTextBuf
        , int & nReadChars)
{
#define UTF16LE 1200
#define UTF16BE 1201
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
        case UNICODE_BE:
            nSourceCodePage = UTF16BE;
            break;
        case UNICODE_LE:
            nSourceCodePage = UTF16LE;
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
    if ((m_UnicodeType == UTF8BOM)||(m_UnicodeType == UNICODE_LE)||(m_UnicodeType == UNICODE_BE))
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

void CFileTextLines::StripAsciiWhiteSpace(CStringA& sLine, DWORD dwIgnoreWhitespaces, bool blame)
{
    if (blame)
    {
        if (sLine.GetLength() > 66)
            sLine = sLine.Mid(66);
    }
    switch (dwIgnoreWhitespaces)
    {
    case 0: // Compare whitespaces
        // do nothing
        break;
    case 1:
        // Ignore all whitespaces
        StripAsciiWhiteSpace(sLine);
        break;
    case 2:
        // Ignore leading whitespace
        sLine.TrimLeft(" \t");
        break;
    case 3:
        // Ignore ending whitespace
        sLine.TrimRight(" \t");
        break;
    }
}

//
// Fast in-place removal of spaces and tabs from CStringA line
//
void CFileTextLines::StripAsciiWhiteSpace(CStringA& sLine)
{
    int outputLen = 0;
    char* pWriteChr = sLine.GetBuffer(sLine.GetLength());
    const char* pReadChr = pWriteChr;
    while(*pReadChr)
    {
        if(*pReadChr != ' ' && *pReadChr != '\t')
        {
            *pWriteChr++ = *pReadChr;
            outputLen++;
        }
        ++pReadChr;
    }
    *pWriteChr = '\0';
    sLine.ReleaseBuffer(outputLen);
}

/**
    Encoding pattern:
        - Get Line
        - encode line
        - modify line - whitespaces, lowercase
        - save line
        - get encoded eol
        - save eol
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

        if ((bSaveAsUTF8)||((m_UnicodeType == CFileTextLines::UTF8BOM)||(m_UnicodeType == CFileTextLines::UTF8)))
        {
            if (m_UnicodeType == CFileTextLines::UTF8BOM)
            {
                //first write the BOM
                UINT16 wBOM = 0xBBEF;
                file.Write(&wBOM, 2);
                UINT8 uBOM = 0xBF;
                file.Write(&uBOM, 1);
            }
            for (int i=0; i<GetCount(); i++)
            {
                CString sLineT = GetAt(i);
                CStringA sLine = CUnicodeUtils::GetUTF8(sLineT);
                StripAsciiWhiteSpace(sLine, dwIgnoreWhitespaces, bBlame);
                if (bIgnoreCase)
                    sLine = sLine.MakeLower();
                file.Write((LPCSTR)sLine, sLine.GetLength());

                if ((m_bReturnAtEnd)||(i != GetCount()-1))
                {
                    EOL ending = GetLineEnding(i);
                    if (ending == EOL_AUTOLINE)
                        ending = m_LineEndings;
                    CStringA sEol;
                    switch (ending)
                    {
                    case EOL_CR:
                        sEol = '\x0d';
                        break;
                    case EOL_CRLF:
                    case EOL_AUTOLINE:
                        sEol = "\x0d\x0a";
                        break;
                    case EOL_LF:
                        sEol = '\x0a';
                        break;
                    case EOL_LFCR:
                        sEol = "\x0a\x0d";
                        break;
                    }
                    file.Write((LPCSTR)sEol, sEol.GetLength());
                }
            }
        }
        else if (m_UnicodeType == CFileTextLines::UNICODE_LE)
        {
            //first write the BOM
            UINT16 wBOM = 0xFEFF;
            file.Write(&wBOM, 2);
            for (int i=0; i<GetCount(); i++)
            {
                CString sLine = GetAt(i);
                StripWhiteSpace(sLine, dwIgnoreWhitespaces, bBlame);
                if (bIgnoreCase)
                    sLine = sLine.MakeLower();
                file.Write((LPCTSTR)sLine, sLine.GetLength()*sizeof(TCHAR));

                if ((m_bReturnAtEnd)||(i != GetCount()-1))
                {
                    EOL ending = GetLineEnding(i);
                    if (ending == EOL_AUTOLINE)
                        ending = m_LineEndings;
                    CString sEol;
                    switch (ending)
                    {
                    case EOL_CR:
                        sEol = _T("\x0d");
                        break;
                    case EOL_CRLF:
                    case EOL_AUTOLINE:
                        sEol = _T("\x0d\x0a");
                        break;
                    case EOL_LF:
                        sEol = _T("\x0a");
                        break;
                    case EOL_LFCR:
                        sEol = _T("\x0a\x0d");
                        break;
                    }
                    file.Write((LPCTSTR)sEol, sEol.GetLength()*sizeof(TCHAR));
                }
            }
        }
        else if (m_UnicodeType == CFileTextLines::UNICODE_BE)
        {
            int linebuflen = 0;
            std::unique_ptr<wchar_t[]> beBuf;
            //first write the BOM
            UINT16 wBOM = 0xFFFE;
            file.Write(&wBOM, 2);
            for (int i=0; i<GetCount(); i++)
            {
                CString sLine = GetAt(i);
                StripWhiteSpace(sLine, dwIgnoreWhitespaces, bBlame);
                if (bIgnoreCase)
                    sLine = sLine.MakeLower();
                int nWcharCount = sLine.GetLength();
                if (nWcharCount > linebuflen)
                {
                    // create/increase buffer size if necessary
                    linebuflen = (nWcharCount + 2047)&~0x3ff;
                    beBuf = std::unique_ptr<wchar_t[]>(new wchar_t[linebuflen]);
                }
                for (int spos = 0; spos < nWcharCount; spos++)
                {
                    // swap the bytes to big-endian order
                    beBuf[spos] = WideCharSwap(sLine[spos]);
                }
                file.Write(beBuf.get(), sLine.GetLength()*sizeof(WCHAR));

                if ((m_bReturnAtEnd)||(i != GetCount()-1))
                {
                    EOL ending = GetLineEnding(i);
                    if (ending == EOL_AUTOLINE)
                        ending = m_LineEndings;
                    CString sEol;
                    switch (ending)
                    {
                    case EOL_CR:
                        sEol = _T("\x0d");
                        break;
                    case EOL_CRLF:
                    case EOL_AUTOLINE:
                        sEol = _T("\x0d\x0a");
                        break;
                    case EOL_LF:
                        sEol = _T("\x0a");
                        break;
                    case EOL_LFCR:
                        sEol = _T("\x0a\x0d");
                        break;
                    }
                    // swap the bytes to big-endian order
                    wchar_t buf[2];
                    if (sEol.GetLength() > 0)
                    {
                        buf[0] = WideCharSwap(sEol[0]);
                    }
                    if (sEol.GetLength() > 1)
                    {
                        buf[1] = WideCharSwap(sEol[1]);
                    }

                    file.Write(buf, sEol.GetLength()*sizeof(wchar_t));
                }
            }
        }
        else if ((m_UnicodeType == CFileTextLines::ASCII)||(m_UnicodeType == CFileTextLines::AUTOTYPE)))
        {
            // ASCII have no BOM
            for (int i=0; i< GetCount(); i++)
            {
                // Copy CString to 8 bit without conversion - actualy there should be ASCII maping
                CString sLineT = GetAt(i);
                CStringA sLine = CStringA(sLineT);
                StripAsciiWhiteSpace(sLine, dwIgnoreWhitespaces, bBlame);
                if (bIgnoreCase)
                    sLine = sLine.MakeLower();
                file.Write((LPCSTR)sLine, sLine.GetLength());

                if ((m_bReturnAtEnd)||(i != GetCount()-1))
                {
                    EOL ending = GetLineEnding(i);
                    if (ending == EOL_AUTOLINE)
                        ending = m_LineEndings;
                    CStringA sEol;
                    switch (ending)
                    {
                    case EOL_CR:
                        sEol = '\x0d';
                        break;
                    case EOL_CRLF:
                    case EOL_AUTOLINE:
                        sEol = "\x0d\x0a";
                        break;
                    case EOL_LF:
                        sEol = '\x0a';
                        break;
                    case EOL_LFCR:
                        sEol = "\x0a\x0d";
                        break;
                    }
                    file.Write((LPCSTR)sEol, sEol.GetLength());
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