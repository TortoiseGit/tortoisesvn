// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "StdAfx.h"
#include "Resource.h"
#include "UnicodeUtils.h"
#include ".\filetextlines.h"

CFileTextLines::CFileTextLines(void)
{
}

CFileTextLines::~CFileTextLines(void)
{
}

CFileTextLines::UnicodeType CFileTextLines::CheckUnicodeType(LPVOID pBuffer, int cb)
{
	if (cb < 2)
		return CFileTextLines::ASCII;
	UINT16 * pVal = (UINT16 *)pBuffer;
	UINT8 * pVal2 = (UINT8 *)(pVal+1);
	// scan the whole buffer for a 0x0000 sequence
	// if found, we assume a binary file
	for (int i=0; i<(cb-1); i=i+2)
	{
		if (0x0000 == *pVal++)
			return CFileTextLines::BINARY;
	}
	pVal = (UINT16 *)pBuffer;
	if (*pVal == 0xFEFF)
		return CFileTextLines::UNICODE_LE;
	if (cb < 3)
		return ASCII;
	if (*pVal == 0xBBEF)
	{
		if (*pVal2 == 0xBF)
			return CFileTextLines::UTF8BOM;
	}
	// check for illegal UTF8 chars
	pVal2 = (UINT8 *)pBuffer;
	for (int i=0; i<cb; ++i)
	{
		if ((*pVal2 == 0xC0)||(*pVal2 == 0xC1)||(*pVal2 >= 0xF5))
			return CFileTextLines::ASCII;
		pVal2++;
	}
	pVal2 = (UINT8 *)pBuffer;
	bool bUTF8 = false;
	for (int i=0; i<cb; ++i)
	{
		if ((*pVal2 & 0xE0)==0xC0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			bUTF8 = true;
		}
		if ((*pVal2 & 0xF0)==0xE0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			bUTF8 = true;
		}
		if ((*pVal2 & 0xF8)==0xF0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return CFileTextLines::ASCII;
			bUTF8 = true;
		}
		pVal2++;
	}
	if (bUTF8)
		return CFileTextLines::UTF8;
	return CFileTextLines::ASCII;
}


CFileTextLines::LineEndings CFileTextLines::CheckLineEndings(LPVOID pBuffer, int cb)
{
	LineEndings retval = AUTOLINE;
	char * buf = (char *)pBuffer;
	for (int i=0; i<cb; i++)
	{
		//now search the buffer for line endings
		if (buf[i] == 0x0a)
		{
			if ((i+1)<cb)
			{
				if (buf[i+1] == 0)
				{
					//UNICODE
					if ((i+2)<cb)
					{
						if (buf[i+2] == 0x0d)
						{
							retval = LFCR;
							break;
						} // if (buf[i+2] == 0x0d) 
						else
						{
							retval = LF;
							break;
						}
					} // if ((i+2)<cb) 
				} // if (buf[i+1] == 0) 
				else if (buf[i+1] == 0x0d)
				{
					retval = LFCR;
					break;
				}
			} // if ((i+1)<cb) 
			retval = LF;
			break;
		} // if (buf[i] == 0x0a) 
		else if (buf[i] == 0x0d)
		{
			if ((i+1)<cb)
			{
				if (buf[i+1] == 0)
				{
					//UNICODE
					if ((i+2)<cb)
					{
						if (buf[i+2] == 0x0a)
						{
							retval = CRLF;
							break;
						} // if (buf[i+2] == 0x0a) 
						else
						{
							retval = CR;
							break;
						}
					} // if ((i+2)<cb) 
				} // if (buf[i+1] == 0) 
				else if (buf[i+1] == 0x0a)
				{
					retval = CRLF;
					break;
				}
			} // if ((i+1)<cb) 
			retval = CR;
			break;
		}
	} // for (int i=0; i<cb; i++) 
	return retval;	
}

BOOL CFileTextLines::Load(const CString& sFilePath, int lengthHint /* = 0*/)
{
	m_LineEndings = CFileTextLines::AUTOLINE;
	m_UnicodeType = CFileTextLines::AUTOTYPE;
	RemoveAll();
	if(lengthHint != 0)
	{
		Reserve(lengthHint);
	}
	
	if (PathIsDirectory(sFilePath))
	{
		m_sErrorString.Format(IDS_ERR_FILE_NOTAFILE, sFilePath);
		return FALSE;
	}
	
	if (!PathFileExists(sFilePath))
	{
		//file does not exist, so just return SUCCESS
		return TRUE;
	}

	HANDLE hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		SetErrorString();
		return FALSE;
	}

	char buf[10000];
	DWORD dwReadBytes = 0;
	do
	{
		if (!ReadFile(hFile, buf, sizeof(buf), &dwReadBytes, NULL))
		{
			SetErrorString();
			CloseHandle(hFile);
			return FALSE;
		}
		if (m_UnicodeType == CFileTextLines::AUTOTYPE)
		{
			m_UnicodeType = this->CheckUnicodeType(buf, dwReadBytes);
		}
		if (m_LineEndings == CFileTextLines::AUTOLINE)
		{
			m_LineEndings = CheckLineEndings(buf, dwReadBytes);
		}
	} while (((m_UnicodeType == CFileTextLines::AUTOTYPE)||(m_LineEndings == CFileTextLines::AUTOLINE))&&(dwReadBytes > 0));
	CloseHandle(hFile);

	if (m_UnicodeType == CFileTextLines::BINARY)
	{
		m_sErrorString.Format(IDS_ERR_FILE_BINARY, sFilePath);
		return FALSE;
	}

	if (m_UnicodeType == CFileTextLines::UNICODE_LE)
	{
		m_sErrorString.Format(IDS_ERR_FILE_BINARY, sFilePath);
		return FALSE;
	}
	
	BOOL bRetval = TRUE;
	CString sLine;
	try
	{
		CStdioFile file(sFilePath, (m_UnicodeType == CFileTextLines::UNICODE_LE ? CFile::typeBinary : CFile::typeText)
			| CFile::modeRead | CFile::shareDenyNone);

		switch (m_UnicodeType)
		{
		case CFileTextLines::UNICODE_LE:
			file.Seek(2,0);
			break;
		case CFileTextLines::UTF8BOM:
			file.Seek(3,0);
			break;
		default:
			break;
		} // switch (m_UnicodeType) 
		while (file.ReadString(sLine))
		{
			switch (m_UnicodeType)
			{
			case CFileTextLines::ASCII:
				Add(sLine);
				break;
			case CFileTextLines::UNICODE_LE:
				Add(sLine);
				break;
			case CFileTextLines::UTF8BOM:
			case CFileTextLines::UTF8:
				{
					Add(CUnicodeUtils::GetUnicode(CStringA(sLine)));
				}
				break;
			default:
				Add(sLine);
			} // switch (m_UnicodeType) 
		} // while (file.ReadString(sLine)) 
		file.Close();
	}
	catch (CException * e)
	{
		e->GetErrorMessage(m_sErrorString.GetBuffer(4096), 4096);
		m_sErrorString.ReleaseBuffer();
		e->Delete();
		bRetval = FALSE;
	}
	return bRetval;
}

void CFileTextLines::StripWhiteSpace(CString& sLine,DWORD dwIgnoreWhitespaces, bool blame)
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

void CFileTextLines::StripAsciiWhiteSpace(CStringA& sLine,DWORD dwIgnoreWhitespaces, bool blame)
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
		// Ignore leading whitespace
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

BOOL CFileTextLines::Save(const CString& sFilePath, DWORD dwIgnoreWhitespaces /*=0*/, BOOL bIgnoreLineendings /*= FALSE*/, BOOL bIgnoreCase /*= FALSE*/, bool bBlame /*= false*/)
{
	if (bIgnoreLineendings)
		m_LineEndings = AUTOLINE;
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
		
		CStdioFile file;			// Hugely faster the CFile for big file writes - because it uses buffering
		if (!file.Open(sFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
		{
			m_sErrorString.Format(IDS_ERR_FILE_OPEN, sFilePath);
			return FALSE;
		}
		if (m_UnicodeType == CFileTextLines::UNICODE_LE)
		{
			//first write the BOM
			UINT16 wBOM = 0xFEFF;
			file.Write(&wBOM, 2);
			for (int i=0; i<GetCount(); i++)
			{
				CString sLine = GetAt(i);
				StripWhiteSpace(sLine,dwIgnoreWhitespaces, bBlame);
				if (bIgnoreCase)
					sLine = sLine.MakeLower();
				file.Write((LPCTSTR)sLine, sLine.GetLength());
				switch (m_LineEndings)
				{
				case CR:
					sLine = _T("\x0d");
					break;
				case CRLF:
				case AUTOLINE:
					sLine = _T("\x0d\x0a");
					break;
				case LF:
					sLine = _T("\x0a");
					break;
				case LFCR:
					sLine = _T("\x0a\x0d");
					break;
				} // switch (endings)
				file.Write((LPCTSTR)sLine, sLine.GetLength());
			} // for (int i=0; i<arPatchLines.GetCount(); i++) 
		} // if (CUtils::IsFileUnicode(sPath)) 
		else if ((m_UnicodeType == CFileTextLines::ASCII)||(m_UnicodeType == CFileTextLines::AUTOTYPE))
		{
			for (int i=0; i< GetCount(); i++)
			{
				// Copy CString to 8 bit wihout conversion
				CString sLineT = GetAt(i);
				CStringA sLine = CStringA(sLineT);

				StripAsciiWhiteSpace(sLine,dwIgnoreWhitespaces, bBlame);
				if (bIgnoreCase)
					sLine = sLine.MakeLower();
				switch (m_LineEndings)
				{
				case CR:
					sLine += '\x0d';
					break;
				case CRLF:
				case AUTOLINE:
					sLine.Append("\x0d\x0a", 2);
					break;
				case LF:
					sLine += '\x0a';
					break;
				case LFCR:
					sLine.Append("\x0a\x0d", 2);
					break;
				} // switch (endings)
				file.Write((LPCSTR)sLine, sLine.GetLength());
			} // for (int i=0; i<arPatchLines.GetCount(); i++) 
		}
		else if ((m_UnicodeType == CFileTextLines::UTF8BOM)||(m_UnicodeType == CFileTextLines::UTF8))
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
				CStringA sLine = CUnicodeUtils::GetUTF8(GetAt(i));
				StripAsciiWhiteSpace(sLine,dwIgnoreWhitespaces, bBlame);
				if (bIgnoreCase)
					sLine = sLine.MakeLower();

				switch (m_LineEndings)
				{
				case CR:
					sLine += '\x0d';
					break;
				case CRLF:
				case AUTOLINE:
					sLine.Append("\x0d\x0a",2);
					break;
				case LF:
					sLine += '\x0a';
					break;
				case LFCR:
					sLine.Append("\x0a\x0d",2);
					break;
				} // switch (endings)
				file.Write((LPCSTR)sLine, sLine.GetLength());
			} // for (int i=0; i<arPatchLines.GetCount(); i++) 
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
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			::GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		m_sErrorString = (LPCTSTR)lpMsgBuf;
		LocalFree( lpMsgBuf );
}

void CFileTextLines::CopySettings(CFileTextLines * pFileToCopySettingsTo)
{
	if (pFileToCopySettingsTo)
	{
		pFileToCopySettingsTo->m_UnicodeType = m_UnicodeType;
		pFileToCopySettingsTo->m_LineEndings = m_LineEndings;
	}
}





