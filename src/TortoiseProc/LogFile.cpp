// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2010 - TortoiseSVN

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
#include "LogFile.h"
#include "PathUtils.h"

#include "Streams/MappedInFile.h"
#include "Streams/StreamException.h"

CLogFile::CLogFile(void)
{
    m_maxlines = CRegStdDWORD(_T("Software\\TortoiseSVN\\MaxLinesInLogfile"), 4000);
}

CLogFile::~CLogFile(void)
{
}

bool CLogFile::Open()
{
    CTSVNPath logfile = CTSVNPath(CPathUtils::GetAppDataDirectory() + _T("\\logfile.txt"));
    return Open(logfile);
}

bool CLogFile::Open(const CTSVNPath& logfile)
{
    m_newLines.clear();
    m_logfile = logfile;
    if (!logfile.Exists())
    {
        CPathUtils::MakeSureDirectoryPathExists(logfile.GetContainingDirectory().GetWinPath());
    }

    return true;
}

bool CLogFile::AddLine(const CString& line)
{
    m_newLines.push_back(line);
    return true;
}

bool CLogFile::Close()
{
    std::deque<CString> lines;
    try
    {
        // limit log file growth

        size_t maxLines = (DWORD)m_maxlines;
        size_t newLines = m_newLines.size();
        TrimFile (max (maxLines, newLines) - newLines);

        // append new info

        CString strLine;
        CStdioFile file;

        int retrycounter = 10;
        // try to open the file for about two seconds - some other TSVN process might be
        // writing to the file and we just wait a little for this to finish
        while (!file.Open(m_logfile.GetWinPath(), CFile::typeText | CFile::modeReadWrite | CFile::modeCreate | CFile::modeNoTruncate) && retrycounter)
        {
            retrycounter--;
            Sleep(200);
        }
        if (retrycounter == 0)
            return false;

        file.SeekToEnd();
        for (std::deque<CString>::const_iterator it = m_newLines.begin(); it != m_newLines.end(); ++it)
        {
            file.WriteString(*it);
            file.WriteString(_T("\n"));
        }
        file.Close();
    }
    catch (CFileException* pE)
    {
        TRACE("CFileException loading autolist regex file\n");
        pE->Delete();
        return false;
    }
    return true;
}

bool CLogFile::AddTimeLine()
{
    CString sLine;
    // first add an empty line as a separator
    m_newLines.push_back(sLine);
    // now add the time string
    TCHAR datebuf[4096] = {0};
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, datebuf, 4096);
    sLine = datebuf;
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, datebuf, 4096);
    sLine += _T(" - ");
    sLine += datebuf;
    m_newLines.push_back(sLine);
    return true;
}

void CLogFile::TrimFile (DWORD maxLines)
{
    if (!PathFileExists (m_logfile.GetWinPath()))
        return;

    // find the start of the maxLines-th last line
    // (\n is always a new line - regardless of the encoding)

    try
    {
        CMappedInFile file (m_logfile.GetWinPath(), true);

        unsigned char* begin = file.GetWritableBuffer();
        unsigned char* end = begin + file.GetSize();

        unsigned char* trimPos = begin;
        for (unsigned char* s = end; s != begin; --s)
            if (*(s-1) == '\n')
                if (maxLines == 0)
                {
                    trimPos = s;
                    break;
                }
                else
                    --maxLines;

        // need to remove lines from the beginning of the file?

        if (trimPos == begin)
            return;

        // remove data

        size_t newSize = end - trimPos;
        memmove (begin, trimPos, newSize);
        file.Close (newSize);
    }
    catch (CStreamException&)
    {
        // can't trim the file right now
    }
}

