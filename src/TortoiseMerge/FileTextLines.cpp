// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004 - Stefan Kueng

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
#include ".\filetextlines.h"

CFileTextLines::CFileTextLines(void)
{
}

CFileTextLines::~CFileTextLines(void)
{
}

BOOL CFileTextLines::IsTextUnicode(LPVOID pBuffer, int cb)
{
	BOOL retval = FALSE;
	UINT16 * pVal = (UINT16 *)pBuffer;
	if (*pVal == 0xFEFF)
		return TRUE;
	if (*pVal == 0xFFFE)
		return TRUE;
	return FALSE;
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

BOOL CFileTextLines::Load(CString sFilePath)
{
	m_LineEndings = CFileTextLines::AUTOLINE;
	m_bUnicode = -1;
	RemoveAll();
	HANDLE hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		this->GetLastError();
		return FALSE;
	} // if (hFile == NULL) 

	char buf[2048];
	DWORD dwReadBytes = 0;
	do
	{
		if (!ReadFile(hFile, buf, sizeof(buf), &dwReadBytes, NULL))
		{
			this->GetLastError();
			CloseHandle(hFile);
			return FALSE;
		} // if (!ReadFile(hFile, buf, sizeof(buf), &dwReadBytes, NULL))
		if (m_bUnicode == -1)
		{
			m_bUnicode = this->IsTextUnicode(buf, dwReadBytes);
		}
		if (m_LineEndings == CFileTextLines::AUTOLINE)
		{
			m_LineEndings = CheckLineEndings(buf, dwReadBytes);
		}
	} while (((m_bUnicode == -1)||(m_LineEndings == CFileTextLines::AUTOLINE))&&(dwReadBytes > 0));
	CloseHandle(hFile);

	BOOL bRetval = TRUE;
	CString sLine;
	try
	{
		CStdioFile file(sFilePath, CFile::typeText | CFile::modeRead | CFile::shareDenyNone);
		while (file.ReadString(sLine))
		{
			Add(sLine);
		}
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

BOOL CFileTextLines::Save(CString sFilePath, BOOL bIgnoreWhitespaces /*= FALSE*/, BOOL bIgnoreLineendings /*= FALSE*/)
{
	if (bIgnoreLineendings)
		m_LineEndings = AUTOLINE;
	try
	{
		CFile file;
		if (!file.Open(sFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
		{
			m_sErrorString.Format(IDS_ERR_FILE_OPEN, sFilePath);
			return FALSE;
		} // if (!file.Open(sSavePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) 
		if (m_bUnicode)
		{
			//first write the BOM
			UINT16 wBOM = 0xFEFF;
			file.Write(&wBOM, 2);
			for (int i=0; i<GetCount(); i++)
			{
				CString sLine = GetAt(i);
				if (bIgnoreWhitespaces)
				{
					sLine.Replace(_T(" "), _T(""));
					sLine.Replace(_T("\t"), _T(""));
				}
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
		else
		{
			for (int i=0; i<GetCount(); i++)
			{
				CStringA sLine = CStringA(GetAt(i));
				if (bIgnoreWhitespaces)
				{
					sLine.Replace(" ", "");
					sLine.Replace("\t", "");
				}
				file.Write((LPCSTR)sLine, sLine.GetLength());
				switch (m_LineEndings)
				{
				case CR:
					sLine = ("\x0d");
					break;
				case CRLF:
				case AUTOLINE:
					sLine = ("\x0d\x0a");
					break;
				case LF:
					sLine = ("\x0a");
					break;
				case LFCR:
					sLine = ("\x0a\x0d");
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

void CFileTextLines::GetLastError()
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
		pFileToCopySettingsTo->m_bUnicode = m_bUnicode;
		pFileToCopySettingsTo->m_LineEndings = m_LineEndings;
	}
}





