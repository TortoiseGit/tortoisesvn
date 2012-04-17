// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2012 - TortoiseSVN

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
#include "StdioFileT.h"

CStdioFileT::CStdioFileT() : CStdioFile()
{
}

CStdioFileT::CStdioFileT(LPCTSTR lpszFileName, UINT nOpenFlags)
{
    ASSERT(lpszFileName != NULL);
    ASSERT(AfxIsValidString(lpszFileName));

    CFileException e;
    if (!Open(lpszFileName, nOpenFlags, &e))
        AfxThrowFileException(e.m_cause, e.m_lOsError, e.m_strFileName);
}


void CStdioFileT::WriteString(LPCSTR lpsz)
{
    ASSERT(lpsz != NULL);
    ASSERT(m_pStream != NULL);

    if (lpsz == NULL)
    {
        AfxThrowInvalidArgException();
    }

    if (fputs(lpsz, m_pStream) == EOF)
        AfxThrowFileException(CFileException::diskFull, _doserrno, m_strFileName);
}


void CStdioFileT::WriteString(LPCWSTR lpsz)
{
    ASSERT(lpsz != NULL);
    ASSERT(m_pStream != NULL);

    if (lpsz == NULL)
    {
        AfxThrowInvalidArgException();
    }

    if (fputws(lpsz, m_pStream) == EOF)
        AfxThrowFileException(CFileException::diskFull, _doserrno, m_strFileName);
}
